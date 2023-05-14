#include "MACRAW_RxBuffer.h"

#include <string.h>
#include <stdlib.h>
#include <endian.h>

#include "Common/Debug.h"
#include "Common/BufferPktQueue.h"

static uint8_t __attribute__((coherent)) RX_BUFFER[MACRAW_RX_BUFFER_LENGTH];
static uint16_t rxTailIndex = 0;
static uint16_t rxReservedChunkLength = 0;

static PktQueue rxPktQueue = {
                                .head = NULL,
                                .tail = NULL,
                                .length = 0
                            };

bool macrawRx_isEmpty(void) {
    return isEmpty_PktQueue(&rxPktQueue);
}

uint8_t* macrawRx_reserve(uint16_t length) {
    if(length > (MACRAW_RX_BUFFER_LENGTH - rxTailIndex)) {
        if(rxReservedChunkLength > 0) {
            memmove(RX_BUFFER, (RX_BUFFER + rxTailIndex - rxReservedChunkLength), rxReservedChunkLength);
        }
        rxTailIndex = rxReservedChunkLength;
    }

    static uint16_t headIndex;
    if(!isEmpty_PktQueue(&rxPktQueue)) {
        headIndex = peekHeadIndex_PktQueue(&rxPktQueue);
        if( (rxTailIndex - rxReservedChunkLength) <= headIndex && 
            (rxTailIndex + length) > headIndex) {
            
            printDebug("macrawRx buffer overflow!\r\n");
            while(true);
        }
    }
    
    rxReservedChunkLength += length;
    rxTailIndex += length;
    
    return (RX_BUFFER + rxTailIndex - length);
}

#define W5500_LENGTH_SECTION_LENGTH (2)

void macrawRx_processReservedChunk() {
    if(rxReservedChunkLength < W5500_LENGTH_SECTION_LENGTH) return;
    
    uint16_t current_pkt_len;
    memmove(&current_pkt_len, RX_BUFFER + rxTailIndex - rxReservedChunkLength, W5500_LENGTH_SECTION_LENGTH);
    current_pkt_len = be16toh(current_pkt_len) - W5500_LENGTH_SECTION_LENGTH;
    
    while(current_pkt_len <= (rxReservedChunkLength - W5500_LENGTH_SECTION_LENGTH)) {
        enqueue_PktQueue(
                    &rxPktQueue,
                    rxTailIndex - rxReservedChunkLength + W5500_LENGTH_SECTION_LENGTH,
                    current_pkt_len
                );
        
        rxReservedChunkLength -= W5500_LENGTH_SECTION_LENGTH + current_pkt_len;
        
        memmove(&current_pkt_len, RX_BUFFER + rxTailIndex - rxReservedChunkLength, W5500_LENGTH_SECTION_LENGTH);
        current_pkt_len = be16toh(current_pkt_len) - W5500_LENGTH_SECTION_LENGTH;
    }
}

uint8_t* macrawRx_getHeadPkt(uint16_t *length) {
    if(isEmpty_PktQueue(&rxPktQueue)) {
        *length = 0;
        return NULL;
    }
    
    PktQueueEntry* pktEntry = dequeue_PktQueue(&rxPktQueue);
    
    uint8_t *pktPtr = RX_BUFFER + pktEntry->buffer_index;
    *length = pktEntry->pkt_length;
    
    free(pktEntry);
    return pktPtr;
}