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
#include <stdio.h>


/* TI Drivers */
#include <ti/drivers/rf/RF.h>
#include <ti/drivers/GPIO.h>

/* Driverlib Header files */
#include DeviceFamily_constructPath(driverlib/rf_prop_mailbox.h)
#include DeviceFamily_constructPath(driverlib/cpu.h)
#include DeviceFamily_constructPath(driverlib/ioc.h)

/* Board Header files */
#include "ti_drivers_config.h"

/* Application Header files */
#include "RFQueue.h"
#include <ti_radio_config.h>


#include "uart/uart.h"
#include "phy/phy.h"
#include "mac/mac.h"
#include "SystemTimer/system_timer.h"

/***** Defines *****/

uint8_t printStats = 0;
void buttonCallbackFunction(uint_least8_t index);

void initGpio(void)
{
    GPIO_setConfig(CONFIG_GPIO_RLED, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_write(CONFIG_GPIO_RLED, CONFIG_GPIO_LED_OFF);

    GPIO_setConfig(CONFIG_GPIO_GLED, GPIO_CFG_OUT_STD | GPIO_CFG_OUT_LOW);
    GPIO_write(CONFIG_GPIO_GLED, CONFIG_GPIO_LED_OFF);

    GPIO_setConfig(CONFIG_GPIO_BTN1, GPIO_CFG_IN_PU | GPIO_CFG_IN_INT_FALLING);

    /* Install Button callback */
    GPIO_setCallback(CONFIG_GPIO_BTN1, buttonCallbackFunction);
    /* Enable interrupts */
    GPIO_enableInt(CONFIG_GPIO_BTN1);

}

/* Pin interrupt Callback function board buttons configured in the pinTable. */
void buttonCallbackFunction(uint_least8_t index) {

    if (GPIO_read(index) == 1)
    {
        return;
    }

    printStats = 1;
}

void *mainThread(void *arg0)
{
    initGpio();

    uartInit();

    phyInit();
    macInit();

    phyStartReceive();
    uartStart();

    GPIO_setMux(CONFIG_GPIO_RLED, IOC_PORT_RFC_GPO1);

    while(1)
    {
        macProcess();

        if(printStats)
        {
            printStats = 0;
            uint8_t statTxt[100];
            sprintf((char *)statTxt, "\r\nTx Bytes - %lu\r\nRx Bytes - %lu\r\nLost Packets - %lu\r\n", macDataBytesSent, macDataBytesRecieved, macLostPacket);
            macDataBytesSent = 0;
            macDataBytesRecieved = 0;
            uartSend(statTxt, strlen((char *)statTxt));
        }
    }
}


