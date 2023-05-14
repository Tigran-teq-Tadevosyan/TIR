#include "InterlinkBuffer.h"

#include <string.h>
#include <stdlib.h>
#include <endian.h>

#include "Interlink.h"
#include "Interlink_Handshake.h"
#include "Interlink_Forwarding.h"
#include "Common/Debug.h"
#include "Common/BufferPktQueue.h"

static uint8_t __attribute__((coherent)) INTERLINKE_BUFFER[INTERLINK_BUFFER_LENGTH];
static volatile uint16_t bufferTailIndex = 0;
static volatile uint16_t bufferReservedChunkLength = 0;

static PktQueue interlinkForwardingPktQueue = {
                                .head = NULL,
                                .tail = NULL,
                                .length = 0
                            };

uint8_t* interlinkBuffer_reserve(uint16_t length) {
    if(length > (INTERLINK_BUFFER_LENGTH - bufferTailIndex)) {
        if(bufferReservedChunkLength > 0) {
            memmove(INTERLINKE_BUFFER, (INTERLINKE_BUFFER + bufferTailIndex - bufferReservedChunkLength), bufferReservedChunkLength);
        }
        bufferTailIndex = bufferReservedChunkLength;
    }

    static uint16_t headIndex;
    if(!isEmpty_PktQueue(&interlinkForwardingPktQueue)) {
        headIndex = peekHeadIndex_PktQueue(&interlinkForwardingPktQueue);
        if( (bufferTailIndex - bufferReservedChunkLength) <= headIndex && 
            (bufferTailIndex + length) > headIndex) {
            
            printDebug("Interlink buffer overflow!\r\n");
            while(true);
        }
    }
    
    bufferReservedChunkLength += length;
    bufferTailIndex += length;
    
    return INTERLINKE_BUFFER + bufferTailIndex - length;
}

void interlinkBuffer_processReservedChunk(void) {
    uint16_t delimBDC;
    uint16_t payload_len;
    uint8_t messageType;
    uint16_t payload_index;
            
    while(true) {
        if(bufferReservedChunkLength < INTERLINK_HEADER_LENGTH) return;

        delimBDC = 0; // delimiterBytesFoundCount
        while(bufferReservedChunkLength >= (INTERLINK_HEADER_LENGTH)) { // test > 0 condition
            if(INTERLINKE_BUFFER[bufferTailIndex - bufferReservedChunkLength + delimBDC] == START_DELIMITER[delimBDC]) {
                if(++delimBDC == START_DELIMITER_LENGTH) break;
            } else {

                delimBDC = 0;
                --bufferReservedChunkLength;
            }
        }

        // We have not found a full delimiter
        if(delimBDC != START_DELIMITER_LENGTH) return;

        memmove(&payload_len, INTERLINKE_BUFFER + bufferTailIndex - bufferReservedChunkLength + START_DELIMITER_LENGTH, PAYLOAD_LEN_ENTRY_SIZE);

        if(payload_len > 1514) {
            printDebug("Something went very wrong (in interlinkBuffer_processReservedChunk) (%u)!!!!!\r\n", payload_len);
            bufferReservedChunkLength -=  START_DELIMITER_LENGTH;
            while(true);
        }

        memmove(&messageType, INTERLINKE_BUFFER + bufferTailIndex - bufferReservedChunkLength + START_DELIMITER_LENGTH + PAYLOAD_LEN_ENTRY_SIZE, MESSAGE_TYPE_LENGTH);

        if(payload_len > (bufferReservedChunkLength - INTERLINK_HEADER_LENGTH)) return; // The payload is yet not fully available

        payload_index = bufferTailIndex - bufferReservedChunkLength + INTERLINK_HEADER_LENGTH;
        bufferReservedChunkLength -=  INTERLINK_HEADER_LENGTH + payload_len;

        // switch/case apparently did not work, no idea why (XC32 is weird)
        if(messageType == (uint8_t)HANDSHAKE_REQUEST) {
            process_handshakeRequest();
        } else if(messageType == (uint8_t)HANDSHAKE_OFFER) {
            process_handshakeOffer((HANDSHAKE_ROLE_PAIR*)(INTERLINKE_BUFFER + payload_index));
        } else if(messageType == (uint8_t)HANDSHAKE_ACK) {
            process_handshakeACK((HANDSHAKE_ROLE_PAIR*)(INTERLINKE_BUFFER + payload_index));
        } else if(messageType == (uint8_t)FORWARDING_TABLE_ADDITION) {
            process_AddForwardingEntry((ForwardingBinding*)(INTERLINKE_BUFFER + payload_index));
        } else if(messageType == (uint8_t)FORWARDING_TABLE_REMOVAL) {
            process_RemoveForwardingEntry((ForwardingBinding*)(INTERLINKE_BUFFER + payload_index));
        } else if(messageType == (uint8_t)FORWARDING_REQUEST) {
//            printDebug("Forwarding received: %u\r\n", payload_len);
            enqueue_PktQueue(
                    &interlinkForwardingPktQueue,
                    payload_index,
                    payload_len
                    );
        }
    }
}


bool interlinkBuffer_isForwardingQueueEmpty(void) {
    return isEmpty_PktQueue(&interlinkForwardingPktQueue);
}

uint8_t* interlinkBuffer_getForwardingPkt(uint16_t *length) {
    if(isEmpty_PktQueue(&interlinkForwardingPktQueue)) {
        *length = 0;
        return NULL;
    }
    
    PktQueueEntry* pktEntry = dequeue_PktQueue(&interlinkForwardingPktQueue);
    
    uint8_t *pktPtr = INTERLINKE_BUFFER + pktEntry->buffer_index;
    *length = pktEntry->pkt_length;
    
    free(pktEntry);
    return pktPtr;
}