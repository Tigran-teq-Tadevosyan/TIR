/*
 * mac.c
 *
 *  Created on: May 9, 2023
 *      Author: Vahagn
 */
/* Standard C Libraries */
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>


#include "mac.h"
#include "SystemTimer/system_timer.h"
#include "uart/uart.h"
#include "ti_drivers_config.h"

#include <stdbool.h>
#include <stdint.h>

uint8_t deviceType = RF_DEVICE_MASTER;

rfMacDataPacket_t rfMacTxPacket[MAC_PACKET_BUFF_CNT];
rfMacDataPacket_t rfMacRxPacket[MAC_PACKET_BUFF_CNT];
uint32_t rfMacRxPacketTick[MAC_PACKET_BUFF_CNT];
uint8_t  rfMacRxPacketStatus[MAC_PACKET_BUFF_CNT];

rfMacDataPacket_t LostPacket;
rfMacDataPacket_t *txRdy;
rfMacDataPacket_t *rxRdy;

volatile uint8_t macRxInPacket = 0, macRxOutPacket = 0;
volatile uint8_t macTxInPacket = 0, macTxOutPacket = 0;

uint32_t txSyncTick;

uint32_t macDataBytesSent = 0, macDataBytesRecieved = 0, macLostPacket = 0;

uint8_t macProcessStatus = 0;
void macProcessInt(void);

void macInit(void)
{
    if(GPIO_read(CONFIG_GPIO_DEV_TYPE) == 1)
    {
        deviceType = RF_DEVICE_MASTER;
    }
    else
        deviceType = RF_DEVICE_SLAVE;

}


rfMacDataPacket_t * macGetTxFreePacket(void)
{
    rfMacDataPacket_t *pct;
    if(macTxRdyPacketCnt() == MAC_PACKET_BUFF_CNT - 1) return NULL;

    pct = &rfMacTxPacket[macTxInPacket];
    macTxInPacket = (1 + macTxInPacket) % MAC_PACKET_BUFF_CNT;
    return pct;
}

rfMacDataPacket_t * macGetTxRdyPacket(void)
{
    rfMacDataPacket_t *pct;
    if(macTxOutPacket == macTxInPacket) return NULL;

    pct = &rfMacTxPacket[macTxOutPacket];
    macTxOutPacket = (1 + macTxOutPacket) % MAC_PACKET_BUFF_CNT;
    return pct;
}

uint8_t macTxRdyPacketCnt(void)
{
   return (MAC_PACKET_BUFF_CNT + macTxInPacket - macTxOutPacket) % MAC_PACKET_BUFF_CNT;
}

rfMacDataPacket_t * macFillRxFreePacketInfo(uint32_t rxTick, uint8_t StatusByte)
{
    rfMacDataPacket_t *pct;
    if(macRxRdyPacketCnt() == MAC_PACKET_BUFF_CNT - 1)
    {
        macLostPacket++;
        return &LostPacket;
    }
    else
    {
        rfMacRxPacketTick[macRxInPacket] = rxTick;
        rfMacRxPacketStatus[macRxInPacket] = StatusByte;
        pct = &rfMacRxPacket[macRxInPacket];
        macRxInPacket = (1 + macRxInPacket) % MAC_PACKET_BUFF_CNT;
    }
    return pct;
}

rfMacDataPacket_t * macGetRxFreePacket(void)
{
    rfMacDataPacket_t *pct;
    pct = &rfMacRxPacket[macRxInPacket];
    macRxInPacket = (1 + macRxInPacket) % MAC_PACKET_BUFF_CNT;
    return pct;
}

uint32_t macGetRxRdyPacketTick(void)
{
    return rfMacRxPacketTick[macRxOutPacket];
}

uint8_t macGetRxRdyPacketStatus(void)
{
    return rfMacRxPacketStatus[macRxOutPacket];
}


rfMacDataPacket_t * macGetRxRdyPacket(void)
{
    rfMacDataPacket_t *pct;
    if(macRxOutPacket == macRxInPacket) return NULL;
    pct = &rfMacRxPacket[macRxOutPacket];
    macRxOutPacket = (1 + macRxOutPacket) % MAC_PACKET_BUFF_CNT;
    return pct;
}

uint8_t macRxRdyPacketCnt(void)
{
    return (MAC_PACKET_BUFF_CNT + macRxInPacket - macRxOutPacket) % MAC_PACKET_BUFF_CNT;
}

void macProcess(void)
{
    /* Check if anything has been received via RF*/
    if(macRxRdyPacketCnt() > 0)
    {
        uint32_t rxTickPassed = self_timer_tick - macGetRxRdyPacketTick(); //phyGetLastRxTickPassed();
        rxRdy = macGetRxRdyPacket();
        if(rxRdy != NULL)
        {
            if(deviceType == RF_DEVICE_SLAVE)
            {
                uint32_t packetDuration = (PREAMLBE_CNT + SYNCWORD_CNT + LENGHT_DATA_CNT + CRC_CNT + MAC_HEADER_SIZE + rxRdy->hdr.lenght) * 8 * 1000000 / DATA_SPEED; // Packet duration in microseconds (* 1000000)
                system_timer_tick = rxRdy->hdr.tick +
                                    RF_convertRatTicksToUs(TX_CMD_TO_START + TX_START_TO_PREAMBLE_TICKS) / SYTEM_TICK_US +
                                    packetDuration/SYTEM_TICK_US +
                                    (100)/SYTEM_TICK_US + // fix 50 us padding (found experimentally)
                                    rxTickPassed;
            }
            uartSend((uint8_t * )&(rxRdy->data), (rxRdy->hdr.lenght));
            macDataBytesRecieved += rxRdy->hdr.lenght; // for statistical purposes
        }
    }

    /* Check if anything has been received via UART*/
    if ((uartInputBuff1ReadCount + uartInputBuff2ReadCount )!= 0)
    {
        rfMacDataPacket_t *txPct = macGetTxFreePacket();
        /*The packet length is set to the number of
        * bytes read by UART2_read() */
        if(txPct != NULL)
        {
            uint16_t bytesToSend;
            uint8_t * buff;

            if(uartInputBuff1ReadCount > 0)
            {
                bytesToSend = uartInputBuff1ReadCount;
                buff = (uint8_t *)uartInputData;
                uartInputBuff1ReadCount = 0;
            }
            else
            {
                bytesToSend = uartInputBuff2ReadCount;
                buff = (uint8_t *)(uartInputData + MAX_DATA_SIZE);
                uartInputBuff2ReadCount = 0;
            }

            if(bytesToSend > MAX_DATA_SIZE)
                bytesToSend = MAX_DATA_SIZE;

            txPct->hdr.pctType = MAC_DATA_PACKET;
            txPct->hdr.pctId = 0x01;
            txPct->hdr.lenght = bytesToSend;

            memcpy(&(txPct->data), (uint8_t*)buff, bytesToSend);

        }
    }

    if(macProcessStatus == 1)
    {
        uint32_t startRAT = RF_getCurrentTime();
        fixCounters();
        phyCancel();
        /*Send packet*/
        txRdy = macGetTxRdyPacket();

        txRdy->hdr.tick = txSyncTick;
        macDataBytesSent += txRdy->hdr.lenght;
        phySendPackage((uint8_t *)txRdy, txRdy->hdr.lenght  + MAC_HEADER_SIZE);
        phyStartReceive();

        releaseCounters(RAT_TO_SYSTICK(RF_getCurrentTime() - startRAT));
        macProcessStatus = 0;
    }
}


void prepMacTrasnmitt(void)
{
    if(macTxRdyPacketCnt() > 0)
    {
        macProcessStatus = 1;
        txSyncTick = system_timer_tick;
        phyTxStartTimeSet();
    }
}

