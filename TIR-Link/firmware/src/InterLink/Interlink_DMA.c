#include "Interlink_DMA.h"

#include <xc.h>
#include <stdlib.h>
#include <stdbool.h> 
#include "definitions.h"

#include "Common/Debug.h"

static Intlink_DMAEntry *TX_QUEUE_HEAD = NULL,
                        *TX_QUEUE_TAIL = NULL;

static uint32_t TX_PENDING_DATA = 0;

static void intlinkDMATransmitCompleteCallback(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle);
static void process_intlinkDMA(void);

void init_intlinkDMA(void) {
    DMAC_ChannelCallbackRegister(DMAC_CHANNEL_0, intlinkDMATransmitCompleteCallback, 0);    
}

static uint32_t droped_pkt_num = 0;
void append_intlinkDMA_TxQueue(InterlinkMessageType messageType, uint8_t *payload, uint16_t payload_length, bool free_payload) {
    if(MAX_TX_PENDING_DATA_SIZE < (TX_PENDING_DATA + payload_length)) {
        printDebug("Interlink DMA_TxQueue %u \r\n", ++droped_pkt_num);
        return;
    }
    
    Intlink_DMAEntry *new_entry = __pic32_alloc_coherent(sizeof(*new_entry));
    
    new_entry->header.start_delimiter = START_DELIMITER_VALUE;
    new_entry->header.payload_length = payload_length;
    new_entry->header.message_type = (uint8_t)messageType;

    new_entry->payload = payload;
    new_entry->head_sent = false;
    new_entry->payload_sent = (payload_length == 0);
    new_entry->free_payload = free_payload;
    new_entry->next = NULL;
    
    if(TX_QUEUE_HEAD == NULL) {
        TX_QUEUE_HEAD = new_entry;
    } else {
        TX_QUEUE_TAIL->next = new_entry;
    }
    
    TX_QUEUE_TAIL = new_entry;
    TX_PENDING_DATA += payload_length;
            
    if(!DMAC_ChannelIsBusy(DMAC_CHANNEL_0)) {
        process_intlinkDMA();
    }
}

bool isEmpty_intlinkDMA_TXQeueu(void) {
    return (TX_QUEUE_HEAD == NULL);
}

static void intlinkDMATransmitCompleteCallback(DMAC_TRANSFER_EVENT event, uintptr_t contextHandle) {
    (void)contextHandle;
    if(event == DMAC_TRANSFER_EVENT_COMPLETE) {
        process_intlinkDMA();
    } else if(event == DMAC_TRANSFER_EVENT_ERROR) {
        printDebug("DMAC_TRANSFER_EVENT_ERROR\r\n");
        while(true);
    }
}
    
static void process_intlinkDMA(void) {
    if(TX_QUEUE_HEAD == NULL) return;
    
    if(!TX_QUEUE_HEAD->head_sent) {
        TX_QUEUE_HEAD->head_sent = true;
        DMAC_ChannelTransfer(DMAC_CHANNEL_0, &(TX_QUEUE_HEAD->header), INTERLINK_HEADER_LENGTH, (const void *)&U2TXREG, 1, 1);
    } else if(!TX_QUEUE_HEAD->payload_sent) {
        TX_QUEUE_HEAD->payload_sent = true;
        DMAC_ChannelTransfer(DMAC_CHANNEL_0, TX_QUEUE_HEAD->payload, TX_QUEUE_HEAD->header.payload_length, (const void *)&U2TXREG, 1, 1);
//        printDebug("Interlink DMA sending payload %u bytes\r\n", TX_QUEUE_HEAD->header.payload_length);
    } else {
        Intlink_DMAEntry *prev_head = TX_QUEUE_HEAD;
        TX_PENDING_DATA -= prev_head->header.payload_length;
        TX_QUEUE_HEAD = TX_QUEUE_HEAD->next;
        if(prev_head->free_payload) {
            __pic32_free_coherent(prev_head->payload);
        }
        
        __pic32_free_coherent(prev_head);
        
        process_intlinkDMA();
    }
}