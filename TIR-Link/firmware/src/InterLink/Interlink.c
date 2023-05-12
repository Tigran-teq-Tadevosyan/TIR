#include "Interlink.h"

#include <string.h>

#include "definitions.h"

#include "Common/Common.h"
#include "Common/Debug.h"
#include "Interlink_Handshake.h"
#include "Interlink_Forwarding.h"
#include "Network/DHCP/DHCP_Server.h"

// Link communication related definition section

#define INTERLINK_READ_BUFFER_LENGTH (10000) // in bytes 

#define START_DELIMITER_LENGTH  (4)
#define PAYLOAD_LEN_ENTRY_SIZE  (2)
#define MESSAGE_TYPE_LENGTH     (1)
#define INTERLINK_HEADER_LENGTH (START_DELIMITER_LENGTH + PAYLOAD_LEN_ENTRY_SIZE + MESSAGE_TYPE_LENGTH)

static const uint8_t START_DELIMITER[START_DELIMITER_LENGTH] = {0x24, 0x26, 0x24, 0x26};// $, &, $, &

static uint8_t  rxBuffer[INTERLINK_READ_BUFFER_LENGTH] = {0};
static size_t   rxBufferReadIndex = 0,
                rxBufferWriteIndex = 0;
static bool     rxNewDataAvailable = false,
                rxDelimiterFound = false;

static void     UART2RxEventHandler(UART_EVENT event, uintptr_t contextHandle);
static size_t   rxDataLength(void);
static void     rxExtractPayload(uint8_t *buffer, uint16_t length);

void init_Interlink(void) {
    // Setting up UART2: Connected to pair board
    UART2_ReadCallbackRegister(UART2RxEventHandler, NO_CONTEXT);
    UART2_ReadNotificationEnable(true, true);
    UART2_ReadThresholdSet(0);
    
    init_InterlinkHandshake();
}

static void UART2RxEventHandler(UART_EVENT event, uintptr_t contextHandle) {
    (void)contextHandle;
    if(event == UART_EVENT_READ_THRESHOLD_REACHED) {
        size_t read_len = UART2_ReadCountGet();
        if(read_len > 0) {
            if(read_len < (INTERLINK_READ_BUFFER_LENGTH - rxBufferWriteIndex)) {
                UART2_Read( rxBuffer + rxBufferWriteIndex,
                            read_len);
            } else {
                UART2_Read( rxBuffer + rxBufferWriteIndex,
                            INTERLINK_READ_BUFFER_LENGTH - rxBufferWriteIndex);
                UART2_Read( rxBuffer,
                            read_len - (INTERLINK_READ_BUFFER_LENGTH - rxBufferWriteIndex));
            }
            
            rxBufferWriteIndex = (rxBufferWriteIndex + read_len)%INTERLINK_READ_BUFFER_LENGTH;
            rxNewDataAvailable = true;
        }
    }
}

static size_t rxDataLength(void) {

    return (INTERLINK_READ_BUFFER_LENGTH + rxBufferWriteIndex - rxBufferReadIndex) % INTERLINK_READ_BUFFER_LENGTH;
}

static void rxExtractPayload(uint8_t *buffer, uint16_t length) {
    size_t  payload_start_index = (rxBufferReadIndex + INTERLINK_HEADER_LENGTH)%INTERLINK_READ_BUFFER_LENGTH,
            payload_end_index = (payload_start_index + length)%INTERLINK_READ_BUFFER_LENGTH;
    
    if(payload_end_index > payload_start_index) {
        memmove(buffer, rxBuffer + payload_start_index, length);
    } else {
        memmove(buffer, rxBuffer + payload_start_index, INTERLINK_READ_BUFFER_LENGTH - payload_start_index);
        memmove(buffer, rxBuffer, payload_end_index);
    }
}

void send_InterLink(InterlinkMessageType messageType, uint8_t *payload, uint16_t payload_len) {
    uint8_t _messageType = (uint8_t)messageType;
    
    UART2_Write((uint8_t*)START_DELIMITER, START_DELIMITER_LENGTH);
    UART2_Write((uint8_t*)&payload_len, PAYLOAD_LEN_ENTRY_SIZE);
    UART2_Write(&_messageType, MESSAGE_TYPE_LENGTH);
    if(payload_len > 0) {
        size_t written_size = UART2_Write((uint8_t*)payload, payload_len);
        if(written_size != payload_len) {
            printDebug("UART2 buffer overflow, written %u of %u \r\n", written_size, payload_len);
        }
    }
}

void process_Interlink(void) {
    process_InterlinkHandshake();
    
    if(!rxNewDataAvailable || rxDataLength() < (INTERLINK_HEADER_LENGTH)) return;
    
    rxNewDataAvailable = false;
    size_t rxDataLen;
    
    if(!rxDelimiterFound) {
        size_t      rxDataLen = rxDataLength();
        uint16_t    dataByteNum = 1,
                    delimiterBytesFoundCount = 0;
        size_t      currentByteIndex = rxBufferReadIndex;
        while(dataByteNum <= rxDataLen) {
            if(rxBuffer[currentByteIndex] == START_DELIMITER[delimiterBytesFoundCount]) {
                if(++delimiterBytesFoundCount == START_DELIMITER_LENGTH) break;
            }

            ++dataByteNum;
            currentByteIndex = (currentByteIndex + 1)%INTERLINK_READ_BUFFER_LENGTH;
        }

        // No delimiter yet so we skip everything before it
        if(delimiterBytesFoundCount == 0) {
            rxBufferReadIndex = currentByteIndex;
            return;
        }
        if(delimiterBytesFoundCount < START_DELIMITER_LENGTH) return; // We have not found the delimiter

        // If we have found the delimiter, we move 'rxBufferReadIndex' to the start of the delimiter as anything before it is useless
        rxBufferReadIndex = currentByteIndex - (START_DELIMITER_LENGTH - 1);

        // We need a fix for the case when the start of the start delimiter is at the and of the rx buffer
        if(rxBufferReadIndex < 0) rxBufferReadIndex = INTERLINK_READ_BUFFER_LENGTH - rxBufferReadIndex; 

        rxDelimiterFound = true;
    }
    
    rxDataLen = rxDataLength();
    if(rxDataLen < INTERLINK_HEADER_LENGTH) return; // The header is yet not available
    
    uint16_t payload_len;
    memmove(&payload_len, rxBuffer + rxBufferReadIndex + START_DELIMITER_LENGTH, PAYLOAD_LEN_ENTRY_SIZE);
  
    uint8_t messageType;
    memmove(&messageType, rxBuffer + rxBufferReadIndex + START_DELIMITER_LENGTH + PAYLOAD_LEN_ENTRY_SIZE, MESSAGE_TYPE_LENGTH);
    
    if(payload_len > (rxDataLen - INTERLINK_HEADER_LENGTH)) return; // The payload is yet not fully available

    uint8_t *payload = NULL;

//    printDebug("Payload Length: %u\r\n", payload_len);
//    printDebug("Message Type: %x\r\n", (uint8_t)messageType);
    if(payload_len > 0) {
        payload = malloc(payload_len);
        if(payload == NULL) {
            printDebug("Failed to allocate memory in 'process_Interlink'\r\n"); 
            while(true);
        }
        
        rxExtractPayload(payload, payload_len);
        
//        printDebug("Payload: ");
//        UART4_Write(payload, payload_len);
//        printDebug("\r\n");
    }

    // switch/case apparently did not work, no idea  why?
    if(messageType == (uint8_t)HANDSHAKE_REQUEST) { 
        process_handshakeRequest();
    } else if(messageType == (uint8_t)HANDSHAKE_OFFER) { 
        process_handshakeOffer((HANDSHAKE_ROLE_PAIR*)payload);
    } else if(messageType == (uint8_t)HANDSHAKE_ACK) { 
        process_handshakeACK((HANDSHAKE_ROLE_PAIR*)payload);
    } else if(messageType == (uint8_t)FORWARDING_TABLE_ADDITION) {
        process_AddForwardingEntry((ForwardingBinding*)payload);
    } else if(messageType == (uint8_t)FORWARDING_TABLE_REMOVAL) {
        process_RemoveForwardingEntry((ForwardingBinding*)payload);
    } else if(messageType == (uint8_t)FORWARDING_REQUEST) {
        process_ForwardingRequest((EthFrame*)payload, payload_len);
    }
    
    rxBufferReadIndex = (rxBufferReadIndex + INTERLINK_HEADER_LENGTH + payload_len)%INTERLINK_READ_BUFFER_LENGTH;
    rxDelimiterFound = false;
    
    if(payload != NULL) free(payload);
}