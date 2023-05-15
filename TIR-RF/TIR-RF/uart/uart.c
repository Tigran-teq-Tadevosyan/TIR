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
    uartParams.baudRate = 460800;
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

void uartSend(uint8_t * data, uint16_t len)
{
    size_t bytesWritten = 0;
    while (bytesWritten == 0)
    {
        status = UART2_write(uart, data, len, &bytesWritten);
        if (status != UART2_STATUS_SUCCESS)
        {
            /* UART2_write() failed */
            while (1);
        }
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
    if(buffer == uartInputData)
    {
        uartInputBuff1ReadCount = count;
        UART2_read(uart, uartInputData + MAX_DATA_SIZE, MAX_DATA_SIZE, NULL);
    }
    else
    {
        uartInputBuff2ReadCount = count;
        UART2_read(uart, uartInputData, MAX_DATA_SIZE, NULL);
    }
}
