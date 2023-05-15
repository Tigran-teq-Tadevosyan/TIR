/*
 * phy.c
 *
 *  Created on: May 15, 2023
 *      Author: Vahagn
 */

/* Standard C Libraries */
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>

/* Board Header files */
#include "ti_drivers_config.h"

/* Application Header files */
#include "RFQueue.h"
#include <ti_radio_config.h>

#include "phy.h"
#include "mac/mac.h"
#include "SystemTimer/system_timer.h"



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

/*******Global variable declarations*********/

volatile uint8_t packetRxCb;



static RF_Object rfObject;
static RF_Handle rfHandle;
RF_CmdHandle rfPostHandle;

/* Receive dataQueue for RF Core to fill in data */
static dataQueue_t dataQueue;
static rfc_dataEntryGeneral_t* currentDataEntry;
static uint8_t packetLength;
static uint8_t* packetDataPointer;

uint32_t rxRdyTick;
uint32_t txStartedTick;
uint32_t txEndTick;
uint8_t resumeRx = 0;


void phyInit(void)
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

    RF_cmdPropTx.pPkt = NULL;
    RF_cmdPropTx.startTrigger.triggerType = TRIG_ABSTIME;
    RF_cmdPropTx.startTime = 400;


    /* Request access to the radio */
    rfHandle = RF_open(&rfObject, &RF_prop, (RF_RadioSetup*)&RF_cmdPropRadioDivSetup, &rfParams);
    uint32_t swiPriority = 2;
    RF_control(rfHandle, RF_CTRL_SET_SWI_PRIORITY, &swiPriority);
    /* Set the frequency */
    RF_postCmd(rfHandle, (RF_Op*)&RF_cmdFs, RF_PriorityNormal, NULL, 0);

}

void phyCancel(void)
{
    /*Cancel the ongoing command*/
    RF_cancelCmd(rfHandle, rfPostHandle, 1);
}

void phySendPackage(uint8_t* data, uint16_t len)
{
    RF_cmdPropTx.pktLen = len;
    RF_cmdPropTx.pPkt = data;
    RF_cmdPropTx.startTime = RF_getCurrentTime() + TX_CMD_TO_START;

    //RF_runCmd(rfHandle, (RF_Op*)&RF_cmdPropTx, RF_PriorityNormal, &TransmitPackageRFcallback, RF_EventLastCmdDone);
    RF_postCmd(rfHandle, (RF_Op*)&RF_cmdPropTx, RF_PriorityNormal, &TransmitPackageRFcallback, RF_EventLastCmdDone);
    txStartedTick = system_timer_tick;
    phyStartReceive();
}

void phyStartReceive(void)
{

    rfPostHandle = RF_postCmd(rfHandle, (RF_Op*)&RF_cmdPropRx,
                                                           RF_PriorityNormal, &ReceivedOnRFcallback,
                                                           RF_EventRxEntryDone | RF_EventRxOk);
}

uint8_t phyRxPacketRdy(void)
{
    return packetRxCb;
}

void phyRxPacketRdyClean(void)

{
    packetRxCb = NO_PACKET;
}

void phyProcess(void)
{
//    if(resumeRx == 1)
//    {
//        resumeRx = 0;
//        /* Resume RF RX */
//        phyStartReceive();
//    }

}

uint32_t phyGetLastRxTickPassed(void)
{
    return system_timer_tick - rxRdyTick;;
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
        memcpy((uint8_t*)macGetRxFreePacket(), packetDataPointer, (packetLength));

        /* Move read entry pointer to next entry */
        RFQueue_nextEntry();

    }
}

/* Callback function called when data is transmitted via RF
 * Function copies the data in a variable, packet, and sets packetRxCb */
void TransmitPackageRFcallback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e)
{
    if (e & RF_EventLastCmdDone)
    {
        //resumeRx = 1;
        txEndTick = system_timer_tick;
    }
}

