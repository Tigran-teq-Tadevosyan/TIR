#include "MACRAW_Buffer.h"

#include <string.h>
#include <stdlib.h>
#include <endian.h>

#include "Common/Debug.h"
#include "MACRAW_BufferPktQueue.h"

static uint8_t RX_BUFFER[RX_BUFFER_LENGTH];
//static uint8_t TX_BUFFER[TX_BUFFER_LENGTH];

static uint16_t     rxHeadIndex = 0,
                    rxTailIndex = 0;

static uint16_t rxReservedChunkLength = 0;

bool macraw_isRxEmpty(void) {
    return isEmpty_PktQueue();
}

uint8_t* macrawRx_reserve(uint16_t length) {
    if(length > (RX_BUFFER_LENGTH - rxTailIndex)) {
        if(rxReservedChunkLength > 0) {
            memmove(RX_BUFFER, (RX_BUFFER + rxTailIndex - rxReservedChunkLength), rxReservedChunkLength);
        }
        rxTailIndex = rxReservedChunkLength;
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
                    rxTailIndex - rxReservedChunkLength + W5500_LENGTH_SECTION_LENGTH,
                    current_pkt_len
                );
        
        rxReservedChunkLength -= W5500_LENGTH_SECTION_LENGTH + current_pkt_len;
        
        memmove(&current_pkt_len, RX_BUFFER + rxTailIndex - rxReservedChunkLength, W5500_LENGTH_SECTION_LENGTH);
        current_pkt_len = be16toh(current_pkt_len) - W5500_LENGTH_SECTION_LENGTH;
    }
}

uint8_t* macrawRx_getHeadPkt(uint16_t *length) {
    if(isEmpty_PktQueue()) {
        *length = 0;
        return NULL;
    }
    
    PktEntry* pktEntry = dequeue_PktQueue();
    
    uint8_t *pktPtr = RX_BUFFER + pktEntry->buffer_index;
    *length = pktEntry->pkt_length;
    
    
    free(pktEntry);
    return pktPtr;
}