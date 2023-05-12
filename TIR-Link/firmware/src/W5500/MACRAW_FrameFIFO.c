#include "MACRAW_FrameFIFO.h"

#include <stdlib.h>
#include <string.h>
#include "Common/Debug.h"

#define MAX_MACRAW_FRAMEFIFO_SIZE (50)

EthFrame    *rxFIFO[MAX_MACRAW_FRAMEFIFO_SIZE] = {NULL},
            *txFIFO[MAX_MACRAW_FRAMEFIFO_SIZE] = {NULL}; 
uint16_t    rxFIFOItemLengths[MAX_MACRAW_FRAMEFIFO_SIZE] = {0},
            txFIFOItemLengths[MAX_MACRAW_FRAMEFIFO_SIZE] = {0}; 

uint16_t    rxFIFOHeadIndex = 0, 
            rxFIFOTailIndex = 0,
            txFIFOHeadIndex = 0, 
            txFIFOTailIndex = 0;

// For debuging purposes
static void printBothQueueSpaces(void) {
    size_t  rxSpace = 0,
            txSpace = 0;
    
    for(int i = 0; i < MAX_MACRAW_FRAMEFIFO_SIZE; ++i) {
        rxSpace += rxFIFOItemLengths[i];
        txSpace += txFIFOItemLengths[i];
    }
    
    printDebug("Rx space used: %u\r\n", rxSpace);
    printDebug("tx space used: %u\r\n", txSpace);
}



// RX SECTION
EthFrame* reserveItem_RxFIFO(uint16_t frame_length) {
    if(rxFIFO[rxFIFOTailIndex] != NULL) 
        free(rxFIFO[rxFIFOTailIndex]);
    
    rxFIFO[rxFIFOTailIndex] = malloc(frame_length); 
    if(rxFIFO[rxFIFOTailIndex] == NULL) {
        printBothQueueSpaces();
        printDebug("Failed to allocate memory in 'reserveItem_RxFIFO' %u\r\n", frame_length); 
        while(true);
    }
    else
    {
//        printDebug("Reserved Memory in 'reserveItem_RxFIFO' %u\r\n", frame_length); 
    }
    
    rxFIFOItemLengths[rxFIFOTailIndex] = frame_length;
    return rxFIFO[rxFIFOTailIndex];
}


void incremetTailIndex_RxFIFO(void) {
    rxFIFOTailIndex = (rxFIFOTailIndex + 1)%MAX_MACRAW_FRAMEFIFO_SIZE;
}

EthFrame* peekHead_RxFIFO(uint16_t* frame_length) {
    if(isEmpty_RxFIFO()) {
        *frame_length = 0;
        return NULL;
    }
    
    *frame_length = rxFIFOItemLengths[rxFIFOHeadIndex];
    return rxFIFO[rxFIFOHeadIndex];
}

void removeHead_RxFIFO(void) {
    free(rxFIFO[rxFIFOHeadIndex]);
    rxFIFO[rxFIFOHeadIndex] = NULL;
    rxFIFOItemLengths[rxFIFOHeadIndex] = 0;
    rxFIFOHeadIndex = (rxFIFOHeadIndex + 1)%MAX_MACRAW_FRAMEFIFO_SIZE;
}

bool isEmpty_RxFIFO(void) {
    return rxFIFOHeadIndex == rxFIFOTailIndex;
}

// TX SECTION

EthFrame* reserveItem_TxFIFO(uint16_t frame_length) {
    if(txFIFO[txFIFOTailIndex] != NULL) 
    {
        printDebug("TXFIFO Overflow\r\n");
        free(txFIFO[txFIFOTailIndex]);
    }
    
    
    txFIFO[txFIFOTailIndex] = malloc(frame_length); 
    if(txFIFO[txFIFOTailIndex] == NULL) {
        printBothQueueSpaces();
        printDebug("Failed to allocate memory in 'reserveItem_TxFIFO'\r\n");
        while(true);
    }
        
    txFIFOItemLengths[txFIFOTailIndex] = frame_length;
    return txFIFO[txFIFOTailIndex]; 
}


void incremetTailIndex_TxFIFO(void) {
    txFIFOTailIndex = (txFIFOTailIndex + 1)%MAX_MACRAW_FRAMEFIFO_SIZE;
}

EthFrame* peekHead_TxFIFO(uint16_t* frame_length) {
    if(isEmpty_TxFIFO()) {
        *frame_length = 0;
        return NULL;
    }
    
    *frame_length = txFIFOItemLengths[txFIFOHeadIndex];
    return txFIFO[txFIFOHeadIndex];
}


void removeHead_TxFIFO(void) {
    free(txFIFO[txFIFOHeadIndex]);
    txFIFO[txFIFOHeadIndex] = NULL;
    txFIFOItemLengths[txFIFOHeadIndex] = 0;
    txFIFOHeadIndex = (txFIFOHeadIndex + 1)%MAX_MACRAW_FRAMEFIFO_SIZE;
}

bool isEmpty_TxFIFO(void) {
    return txFIFOHeadIndex == txFIFOTailIndex;
}