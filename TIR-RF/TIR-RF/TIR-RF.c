/*
 * Copyright (c) 2019, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/***** Includes *****/

/* Standard C Libraries */
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>


/* TI Drivers */
#include <ti/drivers/rf/RF.h>
#include <ti/drivers/GPIO.h>

/* Driverlib Header files */
#include DeviceFamily_constructPath(driverlib/rf_prop_mailbox.h)

/* Board Header files */
#include "ti_drivers_config.h"

/* Application Header files */
#include "RFQueue.h"
#include <ti_radio_config.h>

/* ======== uart2 ======== */
#include <ti/drivers/UART2.h>
#include <stdio.h>

#include "mac/mac.h"
#include "SystemTimer/system_timer.h"

/***** Defines *****/

/* Packet RX Configuration */
#define DATA_ENTRY_HEADER_SIZE 8  /* Constant header size of a Generic Data Entry */
#define MAX_LENGTH             200 /* Max length byte the radio will accept */
#define NUM_DATA_ENTRIES       2  /* NOTE: Only two data entries supported at the moment */
#define NUM_APPENDED_BYTES     2  /* The Data Entries data field will contain:
                                   * 1 Header byte (RF_cmdPropRx.rxConf.bIncludeHdr = 0x1)
                                   * Max 30 payload bytes
                                   * 1 status byte (RF_cmdPropRx.rxConf.bAppendStatus = 0x1) */
#define NO_PACKET              0
#define PACKET_RECEIVED        1

#define RX_START_TO_SETTLE_TICKS   256 /* Time between RX start trigger and the radio
                                        * being ready to receive the first preamble bit.
                                        * This is a fixed value for CMD_PROP_RX. */
#define TX_START_TO_PREAMBLE_TICKS 384 /* Time between TX start trigger and first bit on air.
                                        * This is a fixed value for CMD_PROP_TX. */

#define TX_CMD_TO_START 2000 /* Time between TX start trigger and first bit on air.
                                        * This is a fixed value for CMD_PROP_TX. */

/*******Global variable declarations*********/
static RF_Object rfObject;
static RF_Handle rfHandle;
RF_CmdHandle rfPostHandle;

UART2_Handle uart;
UART2_Params uartParams;

static char         uartInputData[MAX_LENGTH];
static char         uartOutputData[MAX_LENGTH];
int32_t             UARTwrite_semStatus;
int_fast16_t        status = UART2_STATUS_SUCCESS;
volatile uint8_t packetRxCb;
volatile size_t bytesReadCount;

/* Buffer which contains all Data Entries for receiving data.
 * Pragmas are needed to make sure this buffer is 4 byte aligned (requirement from the RF Core) */
#if defined(__TI_COMPILER_VERSION__)
#pragma DATA_ALIGN (rxDataEntryBuffer, 4);
static uint8_t
rxDataEntryBuffer[RF_QUEUE_DATA_ENTRY_BUFFER_SIZE(NUM_DATA_ENTRIES,
                                                  MAX_LENGTH,
                                                  NUM_APPENDED_BYTES)];
#elif defined(__IAR_SYSTEMS_ICC__)
#pragma data_alignment = 4
static uint8_t
rxDataEntryBuffer[RF_QUEUE_DATA_ENTRY_BUFFER_SIZE(NUM_DATA_ENTRIES,
                                                  MAX_LENGTH,
                                                  NUM_APPENDED_BYTES)];
#elif defined(__GNUC__)
static uint8_t
rxDataEntryBuffer[RF_QUEUE_DATA_ENTRY_BUFFER_SIZE(NUM_DATA_ENTRIES,
                                                  MAX_LENGTH,
                                                  NUM_APPENDED_BYTES)]
                                                  __attribute__((aligned(4)));
#else
#error This compiler is not supported.
#endif

/* Receive dataQueue for RF Core to fill in data */
static dataQueue_t dataQueue;
static rfc_dataEntryGeneral_t* currentDataEntry;
static uint8_t packetLength;
static uint8_t* packetDataPointer;

uint32_t rxRdyTick;
uint32_t txStartedTick;
uint32_t txEndTick;
uint8_t resumeRx = 0;

/***** Function definitions *****/
static void ReceivedOnRFcallback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e);
static void TransmitPackageRFcallback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e);




static void ReceiveonUARTcallback(UART2_Handle handle, void *buffer, size_t count, void *userArg, int_fast16_t status);

void *mainThread(void *arg0)
{
    packetRxCb = NO_PACKET;

    RF_Params rfParams;
    RF_Params_init(&rfParams);

    if(RFQueue_defineQueue(&dataQueue,
                                rxDataEntryBuffer,
                                sizeof(rxDataEntryBuffer),
                                NUM_DATA_ENTRIES,
                                MAX_LENGTH + NUM_APPENDED_BYTES))
    {
        /* Failed to allocate space for all data entries */
        while(1);
    }

    GPIO_setConfig(CONFIG_GPIO_RLED, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_write(CONFIG_GPIO_RLED, CONFIG_GPIO_LED_OFF);

    GPIO_setConfig(CONFIG_GPIO_GLED, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_write(CONFIG_GPIO_GLED, CONFIG_GPIO_LED_OFF);

    /*Modifies settings to be able to do RX*/
    /* Set the Data Entity queue for received data */
    RF_cmdPropRx.pQueue = &dataQueue;
    /* Discard ignored packets from Rx queue */
    RF_cmdPropRx.rxConf.bAutoFlushIgnored = 1;
    /* Discard packets with CRC error from Rx queue */
    RF_cmdPropRx.rxConf.bAutoFlushCrcErr = 1;
    /* Implement packet length filtering to avoid PROP_ERROR_RXBUF */
    RF_cmdPropRx.maxPktLen = MAX_LENGTH;
    RF_cmdPropRx.pktConf.bRepeatOk = 1;
    RF_cmdPropRx.pktConf.bRepeatNok = 1;

    RF_cmdPropTx.pPkt = (uint8_t*)&rfMacTxPacket;
    RF_cmdPropTx.startTrigger.triggerType = TRIG_ABSTIME;
    RF_cmdPropTx.startTime = 400;

    /* Set the max amount of bytes to read via UART */
    size_t bytesToRead = MAX_LENGTH;

    bytesReadCount = 0;

    /* Initialize UART with callback read mode */
    UART2_Params_init(&uartParams);
    uartParams.baudRate = 921600;
    uartParams.readMode = UART2_Mode_CALLBACK;
    uartParams.readCallback = ReceiveonUARTcallback;
    uartParams.readReturnMode = UART2_ReadReturnMode_PARTIAL;

    /* Access UART */
    uart = UART2_open(CONFIG_UART2_0, &uartParams);

    /* Print to the terminal that the program has started */
    const char        startMsg[] = "\r\nRF-UART bridge started:\r\n";
    UART2_write(uart, startMsg, sizeof(startMsg), NULL);

    /* Request access to the radio */
    rfHandle = RF_open(&rfObject, &RF_prop, (RF_RadioSetup*)&RF_cmdPropRadioDivSetup, &rfParams);
    uint32_t swiPriority = 2;
    RF_control(rfHandle, RF_CTRL_SET_SWI_PRIORITY, &swiPriority);
    /* Set the frequency */
    RF_postCmd(rfHandle, (RF_Op*)&RF_cmdFs, RF_PriorityNormal, NULL, 0);

    rfPostHandle = RF_postCmd(rfHandle, (RF_Op*)&RF_cmdPropRx,
                                                           RF_PriorityNormal, &ReceivedOnRFcallback,
                                                           RF_EventRxEntryDone | RF_EventRxOk);

    UART2_read(uart, &uartInputData, bytesToRead, NULL);
    GPIO_setMux(CONFIG_GPIO_RLED, IOC_PORT_RFC_GPO1);

    while(1)
    {
        /* Check if anything has been received via RF*/
        if(packetRxCb)
        {
            char txt[200];
            rxRdyTick = system_timer_tick - rxRdyTick;

            sprintf(txt, "\r\nt %lu , c %lu, rxdt %lu, rt %lu\r\n", system_timer_tick,
                                                        RF_convertRatTicksToUs(TX_CMD_TO_START + TX_START_TO_PREAMBLE_TICKS) / SYTEM_TICK_US + 2*175/SYTEM_TICK_US + rxRdyTick,
                                                        rxRdyTick,
                                                        rfMacRxPacket.hdr.tick);


            system_timer_tick = rfMacRxPacket.hdr.tick + RF_convertRatTicksToUs(TX_CMD_TO_START + TX_START_TO_PREAMBLE_TICKS) / SYTEM_TICK_US + 2*175/SYTEM_TICK_US + rxRdyTick;

            status = UART2_write(uart, txt, strlen(txt), NULL);
            memcpy(uartOutputData, &rfMacRxPacket.data, (rfMacRxPacket.hdr.lenght));
            size_t bytesWritten = 0;
            while (bytesWritten == 0)
            {
                status = UART2_write(uart, &uartOutputData, rfMacRxPacket.hdr.lenght, &bytesWritten);
                if (status != UART2_STATUS_SUCCESS)
                {
                    /* UART2_write() failed */
                    while (1);
                }
            }

            /* Reset RF RX callback flag */
            packetRxCb = NO_PACKET;
        }

        /* Check if anything has been received via UART*/
        if (bytesReadCount != 0)
        {
            /*The packet length is set to the number of
             * bytes read by UART2_read() */

            RF_cmdPropTx.pktLen = bytesReadCount + MAC_HEADER_SIZE;
            RF_cmdPropTx.pPkt = (uint8_t*)&rfMacTxPacket;

            rfMacTxPacket.hdr.pctType = MAC_DATA_PACKET;
            rfMacTxPacket.hdr.pctId = 0x01;
            rfMacTxPacket.hdr.lenght = bytesReadCount;


            memcpy(&rfMacTxPacket.data, (uint8_t*)uartInputData, bytesReadCount);

            /*Cancel the ongoing command*/
            RF_cancelCmd(rfHandle, rfPostHandle, 1);
            /*Send packet*/
            rfMacTxPacket.hdr.tick = system_timer_tick;
            RF_cmdPropTx.startTime = RF_getCurrentTime() + TX_CMD_TO_START;


            //RF_runCmd(rfHandle, (RF_Op*)&RF_cmdPropTx, RF_PriorityNormal, &TransmitPackageRFcallback, RF_EventLastCmdDone);
            RF_postCmd(rfHandle, (RF_Op*)&RF_cmdPropTx, RF_PriorityNormal, &TransmitPackageRFcallback, RF_EventLastCmdDone);
            txStartedTick = system_timer_tick;
            bytesReadCount = 0;

            /* Resume UART read */
            status = UART2_read(uart, &uartInputData, bytesToRead, NULL);
        }

        if(resumeRx == 1)
        {
            resumeRx = 0;
            /* Resume RF RX */
            rfPostHandle = RF_postCmd(rfHandle, (RF_Op*)&RF_cmdPropRx,
                                                                 RF_PriorityNormal, &ReceivedOnRFcallback,
                                                                 RF_EventRxEntryDone | RF_EventRxOk);

            char txt[200];
            sprintf(txt, "\r\nt %lu , dTx %lu\r\n", system_timer_tick, txEndTick - txStartedTick);
            status = UART2_write(uart, txt, strlen(txt), NULL);

        }

    }
}

/* Callback function called when data is received via RF
 * Function copies the data in a variable, packet, and sets packetRxCb */
void ReceivedOnRFcallback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e)
{
    if(e & RF_EventRxOk)
    {
        rxRdyTick = system_timer_tick;
    }

    if (e & RF_EventRxEntryDone)
    {
        //GPIO_toggle(CONFIG_GPIO_RLED);

        /* Get current unhandled data entry */
        currentDataEntry = RFQueue_getDataEntry(); //loads data from entry

        /* Handle the packet data, located at &currentDataEntry->data:
         * - Length is the first byte with the current configuration
         * - Data starts from the second byte */
        packetLength      = *(uint8_t*)(&currentDataEntry->data); //gets the packet length (send over with packet)
        packetDataPointer = (uint8_t*)(&currentDataEntry->data + 1); //data starts from 2nd byte

        /* Copy the payload + the status byte to the packet variable */
        memcpy(&rfMacRxPacket, packetDataPointer, (packetLength + 1));

        /* Move read entry pointer to next entry */
        RFQueue_nextEntry();

        packetRxCb = PACKET_RECEIVED;
    }
}
/* Callback function called when data is transmitted via RF
 * Function copies the data in a variable, packet, and sets packetRxCb */
void TransmitPackageRFcallback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e)
{
    if (e & RF_EventLastCmdDone)
    {
        resumeRx = 1;
        txEndTick = system_timer_tick;
    }
}

/* Callback function called when data is received via UART */
void ReceiveonUARTcallback(UART2_Handle handle, void *buffer, size_t count, void *userArg, int_fast16_t status)
{
    if (status != UART2_STATUS_SUCCESS)
    {
        /* RX error occured in UART2_read() */
        while (1) {}
    }

    bytesReadCount = count;
}
