/*
 * uart.c
 *
 *  Created on: May 15, 2023
 *      Author: Vahagn
 */


/***** Includes *****/

/* Standard C Libraries */
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>


/* ======== uart2 ======== */
#include <ti/drivers/UART2.h>
#include <ti/drivers/uart2/UART2Support.h>
#include <stdio.h>


/* Board Header files */
#include "ti_drivers_config.h"

#include "uart.h"


UART2_Handle uart;
UART2_Params uartParams;

char         uartInputData[UART_BUFFER_SIZE];
char         uartOutputData[UART_BUFFER_SIZE];
int32_t             UARTwrite_semStatus;
int_fast16_t        status = UART2_STATUS_SUCCESS;


volatile size_t uartInputBuff1ReadCount;
volatile size_t uartInputBuff2ReadCount;

static void ReceiveonUARTcallback(UART2_Handle handle, void *buffer, size_t count, void *userArg, int_fast16_t status);


void uartInit(void)
{
    /* Initialize UART with callback read mode */
    UART2_Params_init(&uartParams);
    uartParams.baudRate = 230400;   //460800; 300000;//
    uartParams.writeMode = UART2_Mode_NONBLOCKING;
    uartParams.readMode = UART2_Mode_CALLBACK;
    uartParams.readCallback = ReceiveonUARTcallback;
    uartParams.readReturnMode = UART2_ReadReturnMode_PARTIAL;

    /* Access UART */
    uart = UART2_open(CONFIG_UART2_0, &uartParams);

    uartInputBuff1ReadCount = 0;
    uartInputBuff2ReadCount = 0;

}


void uartStart(void)
{
    /* Print to the terminal that the program has started */
    const char        startMsg[] = "\r\nRF-UART bridge started:\r\n";
    UART2_write(uart, startMsg, sizeof(startMsg), NULL);
    UART2_read(uart, &uartInputData, MAX_DATA_SIZE, NULL);
}


int_fast16_t UART2_writeMy(UART2_Handle handle,
                                           const void *buffer,
                                           size_t size,
                                           size_t *bytesWritten)
{
    UART2_Object *object         = handle->object;
    UART2_HWAttrs const *hwAttrs = handle->hwAttrs;
    int_fast16_t status          = UART2_STATUS_SUCCESS;
    unsigned char *dstAddr;
    int space;

    /* Update state variables */
    object->writeInUse          = true;
    object->writeBuf            = buffer;
    object->writeSize           = size;
    object->writeCount          = size;
    object->bytesWritten        = 0;
    object->txStatus            = UART2_STATUS_SUCCESS;
    object->state.writeTimedOut = false;
    object->state.txCancelled   = false;

    /* Copy as much data as we can to the ring buffer */
    do
    {
        /* Get the number of bytes we can copy to the ring buffer
         * and the location where we can start the copy into the
         * ring buffer.
         */

        space = RingBuf_putPointer(&object->txBuffer, &dstAddr);

        if (space > object->writeCount)
        {
            space = object->writeCount;
        }
        memcpy(dstAddr, (unsigned char *)buffer + object->bytesWritten, space);

        /* Update the ring buffer state with the number of bytes copied */
        RingBuf_putAdvance(&object->txBuffer, space);

        object->writeCount -= space;
        object->bytesWritten += space;

    } while ((space > 0) && (object->writeCount > 0));

    /* If anything was written to the ring buffer, enable and start TX */
    if (object->bytesWritten > 0)
    {
        /* Try to start the DMA, it may already be started. */
        UART2Support_enableTx(hwAttrs);
        UART2Support_dmaStartTx(handle);
    }

    /* If there was no space in the ring buffer, set the status to indicate so */
    if (object->bytesWritten == 0)
    {
        status = UART2_STATUS_EAGAIN;
    }

    /* Set output argument, if supplied */
    if (bytesWritten != NULL)
    {
        *bytesWritten = object->bytesWritten;
    }

    object->writeInUse = false;

    return status;
}




void uartSend(uint8_t * data, uint16_t len)
{
        size_t bytesWritten = 0;
    while (bytesWritten == 0)
    {
        status = UART2_writeMy(uart, data, len, &bytesWritten);
        if (status != UART2_STATUS_SUCCESS)
        {
            /* UART2_write() failed */
            while (1);
        }
    }

}

int_fast16_t UART2_readMy(UART2_Handle handle, void *buffer, size_t size, size_t *bytesRead)
{
    UART2_Object *object = handle->object;
    int status           = UART2_STATUS_SUCCESS;
    int available;          /* Number of available bytes in ring buffer */
    unsigned char *srcAddr; /* Address in ring buffer where data can be read from */

    /* The driver is now free to start a read transaction. Even though there is no active "user read" ongoing,
     * the ring buffer could be receiving data in the background. Stop potentially ongoing transfers before proceeding.
     */
    UART2Support_dmaStopRx(handle);

    /* Update driver state variables */
    object->readInUse          = true;
    object->readBuf            = (unsigned char *)buffer;
    object->readSize           = size;
    object->readCount          = size; /* Number remaining to be read */
    object->bytesRead          = 0;    /* Number of bytes read */
    object->rxStatus           = 0;    /* Clear receive errors */
    object->state.rxCancelled  = false;
    object->state.readTimedOut = false;

    /* Enable RX. If RX has already been enabled, this function call does nothing */
    UART2_rxEnable(handle);

    /* Start RX. Depending on the number of bytes in the ring buffer, this will either start a DMA transaction
     * straight into the user buffer, or continue reading into the ring buffer (if it contains enough data for this read
     */
    UART2Support_dmaStartRx(handle);


    /* Read data from the ring buffer */
    do
    {
        available = RingBuf_getPointer(&object->rxBuffer, &srcAddr);

        if (available > object->readCount)
        {
            available = object->readCount;
        }
        memcpy((unsigned char *)buffer + object->bytesRead, srcAddr, available);

        RingBuf_getConsume(&object->rxBuffer, available);

        object->readCount -= available;
        object->bytesRead += available;

    } while ((available > 0) && (object->readCount > 0));


    /* If we are in nonblocking mode, the read operation is done. Update state variables */
    if (object->state.readMode == UART2_Mode_NONBLOCKING)
    {
        object->readCount = 0;
        object->readBuf   = NULL;
        object->readInUse = false;
    }
    /* If there are no more bytes to read, the read operation is complete */
    if (object->readCount == 0)
    {
        /* Update driver state variables */
        object->readInUse = false;
        /* Set readBuf to NULL, but first make a copy to pass to the callback. We cannot set it to NULL after
         * the callback in case another read was issued from the callback.
         */
        //readBufCopy       = object->readBuf;
        object->readBuf   = NULL;

    }

    return status;
}


/* Callback function called when data is received via UART */
void ReceiveonUARTcallback(UART2_Handle handle, void *buffer, size_t count, void *userArg, int_fast16_t status)
{
    if (status != UART2_STATUS_SUCCESS)
    {
        /* RX error occured in UART2_read() */
        while (1) {}
    }
    if(buffer == uartInputData)
    {
        uartInputBuff1ReadCount = count;
        UART2_readMy(uart, uartInputData + MAX_DATA_SIZE, MAX_DATA_SIZE, NULL);
    }
    else
    {
        uartInputBuff2ReadCount = count;
        UART2_readMy(uart, uartInputData, MAX_DATA_SIZE, NULL);
    }
}
