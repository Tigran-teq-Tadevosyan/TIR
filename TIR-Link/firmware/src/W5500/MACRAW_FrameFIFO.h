#ifndef _MACRAW_FRAMEFIFO_H
#define _MACRAW_FRAMEFIFO_H

#include <stdbool.h>
#include "Network/Ethernet.h"

void printBothQueueSpaces(void);

EthFrame* reserveItem_RxFIFO(uint16_t frame_length);
void incremetTailIndex_RxFIFO(void);
EthFrame* peekHead_RxFIFO(uint16_t* frame_length);
void removeHead_RxFIFO(void);
bool isEmpty_RxFIFO(void);

EthFrame* reserveItem_TxFIFO(uint16_t frame_length);
void incremetTailIndex_TxFIFO(void);
EthFrame* peekHead_TxFIFO(uint16_t* frame_length);
void removeHead_TxFIFO(void);
bool isEmpty_TxFIFO(void);

#endif // _MACRAW_FRAMEFIFO_H