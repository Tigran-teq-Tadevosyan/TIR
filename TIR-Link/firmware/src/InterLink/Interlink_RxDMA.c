#include "Interlink_RxDMA.h"

#include <string.h>
#include <stdlib.h>
#include <endian.h>

#include "definitions.h"

#include "Interlink.h"
#include "Interlink_Handshake.h"
#include "Interlink_Forwarding.h"
#include "Common/Common.h"
#include "Common/Debug.h"
#include "Common/BufferPktQueue.h"

static uint8_t __attribute__((coherent)) INTERLINKE_BUFFER[INTERLINK_BUFFER_LENGTH];
static volatile uint16_t    rxRecSecLen = INTERLINK_BUFFER_BLOCK_SIZE,
                            rxUnprocSecIndex = 0, 
                            rxUnprocSecLen = 0;

static PktQueue interlinkForwardingPktQueue = {
                                .head = NULL,
                                .tail = NULL,
                                .length = 0
                            };

static void intlinkDMAReceiveCompleteCallback(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle);

void init_intlinkRxDMA(void) {
    DMAC_ChannelCallbackRegister(DMAC_CHANNEL_1, intlinkDMAReceiveCompleteCallback, 0);
    DMAC_ChannelTransfer(DMAC_CHANNEL_1, (const void *)&U2RXREG, 1, INTERLINKE_BUFFER, INTERLINK_BUFFER_BLOCK_SIZE, 1);
}

static uint16_t timeoutBytesRead = 0;
static uint16_t bytesAvailable = 0;
void process_intlinkRxDMA(void) {
    if(UART2_timeout_us > INTERLINK_TIMEOUT_US) {
        UART2_timeout_us = 0;
        bytesAvailable = DCH1DPTR - timeoutBytesRead;
        if(bytesAvailable > 0) { // DCH1DPTR - Channel Destination Pointer bits
            timeoutBytesRead += bytesAvailable;
            rxRecSecLen -= bytesAvailable;
            rxUnprocSecLen += bytesAvailable;
        }
    }
    
    static uint16_t delimBDC;
    static uint16_t payload_len;
    static uint8_t messageType;
    static uint16_t payload_index;
    
    while(true){
        if(rxUnprocSecLen < INTERLINK_HEADER_LENGTH) return;

        delimBDC = 0; // delimiterBytesFoundCount
        while(rxUnprocSecLen >= (INTERLINK_HEADER_LENGTH)) { // test > 0 condition
            if(INTERLINKE_BUFFER[rxUnprocSecIndex + delimBDC] == START_DELIMITER[delimBDC]) {
                if(++delimBDC == START_DELIMITER_LENGTH) break;
            } else {
                delimBDC = 0;
                ++rxUnprocSecIndex;
                --rxUnprocSecLen;
            }
        }

        // We have not found a full delimiter
        if(delimBDC != START_DELIMITER_LENGTH) return;

        memmove(&payload_len, INTERLINKE_BUFFER + rxUnprocSecIndex + START_DELIMITER_LENGTH, PAYLOAD_LEN_ENTRY_SIZE);

        if(payload_len > 1514) {
            printDebug("Something went very wrong (in process_intlinkRxDMA) (%u)!!!!!\r\n", payload_len);
            while(true);
        }

        memmove(&messageType, INTERLINKE_BUFFER + rxUnprocSecIndex + START_DELIMITER_LENGTH + PAYLOAD_LEN_ENTRY_SIZE, MESSAGE_TYPE_LENGTH);

        if(payload_len > (rxUnprocSecLen - INTERLINK_HEADER_LENGTH)) return; // The payload is yet not fully available

        payload_index = rxUnprocSecIndex + INTERLINK_HEADER_LENGTH;
        rxUnprocSecIndex += INTERLINK_HEADER_LENGTH + payload_len;
        rxUnprocSecLen -= INTERLINK_HEADER_LENGTH + payload_len;

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
    //        printDebug("Forwarding received: %u\r\n", payload_len);
            enqueue_PktQueue(
                    &interlinkForwardingPktQueue,
                    payload_index,
                    payload_len
                    );
        }
    }
}

static void intlinkDMAReceiveCompleteCallback(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle) {
    (void)contextHandle;
    if(event == DMAC_TRANSFER_EVENT_COMPLETE) {
        if((rxUnprocSecIndex + rxUnprocSecLen + rxRecSecLen + INTERLINK_BUFFER_BLOCK_SIZE) > INTERLINK_BUFFER_LENGTH) {
            DMAC_ChannelTransfer(DMAC_CHANNEL_1, (const void *)&U2RXREG, 1, INTERLINKE_BUFFER + rxUnprocSecLen + rxRecSecLen, INTERLINK_BUFFER_BLOCK_SIZE, 1);
            if((rxUnprocSecLen + rxRecSecLen) > 0) {
                memmove(INTERLINKE_BUFFER, (INTERLINKE_BUFFER + rxUnprocSecIndex), rxUnprocSecLen + rxRecSecLen);
            }
            rxUnprocSecIndex = 0;
        } else {
            DMAC_ChannelTransfer(DMAC_CHANNEL_1, (const void *)&U2RXREG, 1, INTERLINKE_BUFFER + rxUnprocSecIndex + rxUnprocSecLen + rxRecSecLen, INTERLINK_BUFFER_BLOCK_SIZE, 1);
        }

        timeoutBytesRead = 0;
        rxUnprocSecLen += rxRecSecLen;
        rxRecSecLen = INTERLINK_BUFFER_BLOCK_SIZE;
    } else if(event == DMAC_TRANSFER_EVENT_ERROR) {
        printDebug("DMAC_TRANSFER_EVENT_ERROR in interlink Rx\r\n");
        while(true);
    }
}

bool isForwardingQueueEmpty_intlinkRxDMA(void) {
    return isEmpty_PktQueue(&interlinkForwardingPktQueue);
}

uint8_t* getForwardingPkt_intlinkRxDMA(uint16_t *length) {
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