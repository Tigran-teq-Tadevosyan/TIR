#include "Interlink.h"

#include <string.h>

#include "definitions.h"
#include "Common/Common.h"
#include "Common/Debug.h"

#define START_DELIMITER_LENGTH (4)
#define PAYLOAD_LEN_ENTRY_SIZE (2)
static const uint8_t START_DELIMITER[START_DELIMITER_LENGTH] = {0x24, 0x26, 0x24, 0x26};// $, &, $, &

static uint8_t  rxBuffer[INTERLINK_READ_BUFFER_LENGTH] = {0};
static size_t   rxBufferReadIndex = 7,
                rxBufferWriteIndex = 7;
static bool     rxNewDataAvailable = false;

//static uint8_t  rxDelimiterFound

static void UART2RxEventHandler(UART_EVENT event, uintptr_t contextHandle);
static size_t rxDataLength(void);

void init_Interlink(void) {
    // Setting up UART2: Connected to pair board
    UART2_ReadCallbackRegister(UART2RxEventHandler, NO_CONTEXT);
    UART2_ReadNotificationEnable(true, true);
    UART2_ReadThresholdSet(0);
}

static void UART2RxEventHandler(UART_EVENT event, uintptr_t contextHandle) {
    (void)contextHandle;
    if(event == UART_EVENT_READ_THRESHOLD_REACHED) {
        size_t read_len = UART2_ReadCountGet();
        if(read_len > 0) {
            if(read_len < (INTERLINK_READ_BUFFER_LENGTH - 1 - rxBufferWriteIndex)) {
                UART2_Read( rxBuffer + rxBufferWriteIndex,
                            read_len);
            } else {
                UART2_Read( rxBuffer + rxBufferWriteIndex,
                            INTERLINK_READ_BUFFER_LENGTH - rxBufferWriteIndex);
                UART2_Read( rxBuffer,
                            read_len - (INTERLINK_READ_BUFFER_LENGTH - rxBufferWriteIndex));
//                rxBufferWriteIndex = read_len - (INTERLINK_READ_BUFFER_LENGTH - rxBufferWriteIndex);
            }
            
            rxBufferWriteIndex = (rxBufferWriteIndex + read_len)%INTERLINK_READ_BUFFER_LENGTH;
            rxNewDataAvailable = true;
        }
    }
}

static size_t rxDataLength(void) {
    return  (rxBufferWriteIndex >= rxBufferReadIndex) ?
            (rxBufferWriteIndex - rxBufferReadIndex) :
            (((INTERLINK_READ_BUFFER_LENGTH - 1) - rxBufferReadIndex) + rxBufferWriteIndex);
}

void process_Interlink(void) {
    if(get_SysTime_ms()%1000 != 0) return;
    if(!rxNewDataAvailable || rxDataLength() < (START_DELIMITER_LENGTH + PAYLOAD_LEN_ENTRY_SIZE)) return;
    
    rxNewDataAvailable = false;
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
    
    if(delimiterBytesFoundCount < START_DELIMITER_LENGTH) return; // We have not found the delimiter
    
    printDebug("Before: rxBufferWriteIndex %u\r\n", rxBufferWriteIndex);
    printDebug("Before: rxBufferReadIndex %u\r\n", rxBufferReadIndex);
    printDebug("Before: rxDataLen %u\r\n", rxDataLen);
    
    // If we have found the delimiter, we move 'rxBufferReadIndex' to the start of the delimiter as anything before it is useless
    rxBufferReadIndex = currentByteIndex - (START_DELIMITER_LENGTH - 1);
    
    if(rxBufferReadIndex < 0) 
        rxBufferReadIndex = INTERLINK_READ_BUFFER_LENGTH - rxBufferReadIndex; 
    
    rxDataLen = rxDataLength();
    if(rxDataLen < (START_DELIMITER_LENGTH + PAYLOAD_LEN_ENTRY_SIZE)) return; // The length of payload is yet not available
    
    uint16_t payload_len; 
    memmove(&payload_len, rxBuffer + rxBufferReadIndex + START_DELIMITER_LENGTH, PAYLOAD_LEN_ENTRY_SIZE);
  
    if(payload_len > (rxDataLen - START_DELIMITER_LENGTH - PAYLOAD_LEN_ENTRY_SIZE)) return; // The payload is yet not fully available
    
//    printDebug("After: rxBufferWriteIndex %u\r\n", rxBufferWriteIndex);
//    printDebug("After: rxBufferReadIndex %u\r\n", rxBufferReadIndex);
//    printDebug("After: rxDataLen %u\r\n", rxDataLen);
//    printDebug("After: payload_len %u\r\n", payload_len);
//    return;
    
    size_t  payload_start_index = (rxBufferReadIndex + START_DELIMITER_LENGTH + PAYLOAD_LEN_ENTRY_SIZE)%INTERLINK_READ_BUFFER_LENGTH,
            payload_end_index = (payload_start_index + payload_len)%INTERLINK_READ_BUFFER_LENGTH;
    printDebug("payload_len %u\r\n", payload_len);
    printDebug("payload_start_index %u\r\n", payload_start_index);
    printDebug("payload_end_index %u\r\n", payload_end_index);
    printDebug("rxBufferWriteIndex %u\r\n", rxBufferWriteIndex);
    uint8_t *payload = rxBuffer + payload_start_index;
    if(payload_end_index > payload_start_index) {
        UART4_Write(payload, payload_len);
//        memset(payload, 0, payload_len);
    } else {
        UART4_Write(payload,
                    INTERLINK_READ_BUFFER_LENGTH - payload_start_index);
        UART4_Write(rxBuffer,
                    payload_end_index);
//        memset(payload, 0, INTERLINK_READ_BUFFER_LENGTH - payload_start_index);
//        memset(rxBuffer, 0, payload_end_index);
    }
    
    rxBufferReadIndex = payload_end_index;
}