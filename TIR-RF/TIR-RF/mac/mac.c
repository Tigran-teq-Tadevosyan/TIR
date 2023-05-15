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

rfMacDataPacket_t rfMacTxPacket[MAC_RX_PACKET_BUFF_CNT];
rfMacDataPacket_t rfMacRxPacket[MAC_RX_PACKET_BUFF_CNT];

volatile uint8_t macRxInPacket = 0, macRxOutPacket = 0;
volatile uint8_t macTxInPacket = 0, macTxOutPacket = 0;

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
    if(macTxRdyPacketCnt() == MAC_RX_PACKET_BUFF_CNT - 1) return NULL;

    pct = &rfMacTxPacket[macTxInPacket];
    macTxInPacket = (1 + macTxInPacket) % MAC_RX_PACKET_BUFF_CNT;
    return pct;
}

rfMacDataPacket_t * macGetTxRdyPacket(void)
{
    rfMacDataPacket_t *pct;
    if(macTxOutPacket == macTxInPacket) return NULL;

    pct = &rfMacTxPacket[macTxOutPacket];
    macTxOutPacket = (1 + macTxOutPacket) % MAC_RX_PACKET_BUFF_CNT;
    return pct;
}

uint8_t macTxRdyPacketCnt(void)
{
   return (MAC_RX_PACKET_BUFF_CNT + macTxInPacket - macTxOutPacket) % MAC_RX_PACKET_BUFF_CNT;
}

rfMacDataPacket_t * macGetRxFreePacket(void)
{
    rfMacDataPacket_t *pct;
    pct = &rfMacRxPacket[macRxInPacket];
    macRxInPacket = (1 + macRxInPacket) % MAC_RX_PACKET_BUFF_CNT;
    return pct;
}

rfMacDataPacket_t * macGetRxRdyPacket(void)
{
    rfMacDataPacket_t *pct;
    if(macRxOutPacket == macRxInPacket) return NULL;
    pct = &rfMacRxPacket[macRxOutPacket];
    macRxOutPacket = (1 + macRxOutPacket) % MAC_RX_PACKET_BUFF_CNT;
    return pct;
}

uint8_t macGetRxRdyCnt(void)
{
    return (MAC_RX_PACKET_BUFF_CNT + macRxInPacket - macRxOutPacket) % MAC_RX_PACKET_BUFF_CNT;
}

void macProcess(void)
{
    /* Check if anything has been received via RF*/
     if(macGetRxRdyCnt() > 0)
     {
         rfMacDataPacket_t *rxPct;
         uint32_t rxTickPassed = phyGetLastRxTickPassed();
         rxPct = macGetRxRdyPacket();
         if(rxPct != NULL)
         {
             if(deviceType == RF_DEVICE_SLAVE)
             {
                 uint32_t packetDuration = (PREAMLBE_CNT + SYNCWORD_CNT + CRC_CNT + MAC_HEADER_SIZE + rxPct->hdr.lenght) * 8 * 1000000 / DATA_SPEED;
                 system_timer_tick = rxPct->hdr.tick +
                                     RF_convertRatTicksToUs(TX_CMD_TO_START + TX_START_TO_PREAMBLE_TICKS) / SYTEM_TICK_US +
                                     packetDuration/SYTEM_TICK_US +
                                     (50)/SYTEM_TICK_US +
                                     rxTickPassed;
             }
             uartSend((uint8_t * )&(rxPct->data), (rxPct->hdr.lenght));
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
         phyCancel();
         macProcessStatus = 2;
     }

     if(macProcessStatus == 2)
     {
         rfMacDataPacket_t *txRdy = macGetTxRdyPacket();
         /*Send packet*/
         txRdy->hdr.tick = system_timer_tick;
         phySendPackage((uint8_t *)txRdy, txRdy->hdr.lenght  + MAC_HEADER_SIZE);
         macProcessStatus = 0;
     }
}

void startMacTransmitt(void)
{
    if(macTxRdyPacketCnt() > 0)
        macProcessStatus = 1;
}

void prepMacTrasnmitt(void)
{
//    if(macTxRdyPacketCnt() > 0)
//    {
//        macProcessStatus = 1;
//    }
}

