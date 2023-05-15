/*
 * uart.h
 *
 *  Created on: May 15, 2023
 *      Author: Vahagn
 */

#ifndef UART_UART_H_
#define UART_UART_H_


#include "phy/phy.h"

#define UART_BUFFER_SIZE    MAX_DATA_SIZE * 2


extern volatile size_t uartInputBuff1ReadCount;
extern volatile size_t uartInputBuff2ReadCount;

extern char         uartInputData[UART_BUFFER_SIZE];
extern char         uartOutputData[UART_BUFFER_SIZE];

void uartInit(void);
void uartStart(void);

void uartSend(uint8_t * data, uint16_t len);

#endif /* UART_UART_H_ */
