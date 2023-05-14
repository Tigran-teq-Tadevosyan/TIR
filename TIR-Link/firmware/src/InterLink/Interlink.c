#include "Interlink.h"

#include <string.h>

#include "definitions.h"

#include "Common/Common.h"
#include "Common/Debug.h"
#include "Interlink_Handshake.h"
#include "Interlink_Forwarding.h"
#include "InterlinkBuffer.h"
#include "Network/DHCP/DHCP_Server.h"

#define UART2_TIMEOUT_US (1000) // in microseconds
#define INTERLINK_READ_BUFFER_LENGTH (10000) // in bytes

static void     UART2RxEventHandler(UART_EVENT event, uintptr_t contextHandle);

void init_Interlink(void) {
    // Setting up UART2: Connected to pair board
    UART2_ReadCallbackRegister(UART2RxEventHandler, NO_CONTEXT);
    UART2_ReadNotificationEnable(true, true);
    UART2_ReadThresholdSet(0);
    
    init_InterlinkDMA();
    init_InterlinkHandshake();
}

static void read_UART2() {
    LED_BB_Clear();
    size_t read_len = UART2_ReadCountGet();
    if(read_len > 0) {
        uint8_t *read_buffer = interlinkBuffer_reserve(read_len);
        size_t reallyRead = UART2_Read(read_buffer, read_len);
//        printDebug("read_UART2 ");
        if(read_len != reallyRead) {
            printDebug("read_UART2 read %u of %u\r\n", reallyRead, read_len);
            while(true);
        }
//        if(read_len < (INTERLINK_READ_BUFFER_LENGTH - rxBufferWriteIndex)) {
//            UART2_Read( rxBuffer + rxBufferWriteIndex,
//                        read_len);
//        } else {
//            UART2_Read( rxBuffer + rxBufferWriteIndex,
//                        INTERLINK_READ_BUFFER_LENGTH - rxBufferWriteIndex);
//            UART2_Read( rxBuffer,
//                        read_len - (INTERLINK_READ_BUFFER_LENGTH - rxBufferWriteIndex));
//        }
//
//        rxBufferWriteIndex = (rxBufferWriteIndex + read_len)%INTERLINK_READ_BUFFER_LENGTH;
//        rxNewDataAvailable = true;
    }
    LED_BB_Set();
}

static void UART2RxEventHandler(UART_EVENT event, uintptr_t contextHandle) {
    (void)contextHandle;
    if(event == UART_EVENT_READ_THRESHOLD_REACHED) {
        read_UART2();
        UART2_timeout_us = 0;
    } else if(event == UART_EVENT_READ_ERROR) {
        printDebug("UART2 Rx error %u\r\n", UART2_ErrorGet());
        while(true);
    }
}

static uint32_t droped_pkt_num = 0;
void send_InterLink(InterlinkMessageType messageType, uint8_t *payload, uint16_t payload_len) {
    if(UART2_WriteFreeBufferCountGet() < (INTERLINK_HEADER_LENGTH + payload_len) ) {
        printDebug("UART2 dropped packet count %u \r\n", ++droped_pkt_num);
        return;
    }
    
    uint8_t _messageType = (uint8_t)messageType;

    UART2_Write((uint8_t*)START_DELIMITER, START_DELIMITER_LENGTH);
    UART2_Write((uint8_t*)&payload_len, PAYLOAD_LEN_ENTRY_SIZE);
    UART2_Write(&_messageType, MESSAGE_TYPE_LENGTH);
    if(payload_len > 0) {
        size_t written_size = UART2_Write((uint8_t*)payload, payload_len);
        if(written_size != payload_len) {
            printDebug("UART2 buffer overflow, written %u of %u \r\n", written_size, payload_len);
            while(true);
        } else {
//            printDebug("UART2 buffer  written %u \r\n", payload_len);
            
            if(messageType == FORWARDING_REQUEST) {
                printDebug("Forwarding sent: %u\r\n", payload_len);
            }
        }
    }
}

void process_Interlink(void) {
    
    process_InterlinkHandshake();
    
    UART2_ReadNotificationEnable(false, false);
    if(UART2_timeout_us > UART2_TIMEOUT_US) {
        read_UART2();
//        UART2_timeout_us = 0;
    }
    
//    UART2_ReadNotificationEnable(false, false);
    LED_GG_Clear();
    interlinkBuffer_processReservedChunk();
    LED_GG_Set();
    UART2_ReadNotificationEnable(true, true);
    /*
    if((!rxNewDataAvailable && rxDelimiterFound == true) || rxDataLength() < (INTERLINK_HEADER_LENGTH)) return false;
    rxNewDataAvailable = false;

    if(!rxDelimiterFound) {
        uint16_t delimBDC = 0; // delimiterBytesFoundCount
        while(rxDataLength() >= (INTERLINK_HEADER_LENGTH)) {
            if(rxBuffer[rxBufferReadIndex + delimBDC] == START_DELIMITER[delimBDC]) {
                if(++delimBDC == START_DELIMITER_LENGTH) break;
            } else {
                delimBDC = 0;
                rxBufferReadIndex = (rxBufferReadIndex + 1)%INTERLINK_READ_BUFFER_LENGTH;
            }
        }

        if(delimBDC == START_DELIMITER_LENGTH) rxDelimiterFound = true;
        else return false;
    }

    size_t rxDataLen = rxDataLength();
    if(rxDataLen < INTERLINK_HEADER_LENGTH) return false; // The header is yet not available

    uint16_t payload_len;
    memmove(&payload_len, rxBuffer + rxBufferReadIndex + START_DELIMITER_LENGTH, PAYLOAD_LEN_ENTRY_SIZE);
    
    if(payload_len > 1514)
        rxBufferReadIndex = (rxBufferReadIndex + START_DELIMITER_LENGTH )%INTERLINK_READ_BUFFER_LENGTH;
    
    uint8_t messageType;
    memmove(&messageType, rxBuffer + rxBufferReadIndex + START_DELIMITER_LENGTH + PAYLOAD_LEN_ENTRY_SIZE, MESSAGE_TYPE_LENGTH);
    

    
    if(payload_len > (rxDataLen - INTERLINK_HEADER_LENGTH)) return false; // The payload is yet not fully available

    uint8_t *payload = NULL;
    
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

    rxBufferReadIndex = (rxBufferReadIndex + INTERLINK_HEADER_LENGTH + payload_len)%INTERLINK_READ_BUFFER_LENGTH;
    rxDelimiterFound = false;

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

    if(payload != NULL) free(payload);
    return true;
    */
}
