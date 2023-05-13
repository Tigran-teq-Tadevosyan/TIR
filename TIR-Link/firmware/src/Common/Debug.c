#include "Debug.h"
#include "Common.h"

#include <stdio.h>
#include <stdarg.h>

#include "definitions.h"                // SYS function prototypes

static void UartRxEventHandler(UART_EVENT event, uintptr_t contextHandle);

void init_Debug(void) {
    // Setting up UART4: Connected to the debug port
    UART4_ReadCallbackRegister(UartRxEventHandler, NO_CONTEXT);
    UART4_ReadNotificationEnable(true, true);
    UART4_ReadThresholdSet(0);
}

#define DEBUG_BUFFER_LENGTH (200)
char DEBUG_BUFFER[DEBUG_BUFFER_LENGTH];
void printDebug (const char * format, ... ) {
    va_list arg;
    va_start(arg, format);
    uint16_t sz = vsnprintf(DEBUG_BUFFER, DEBUG_BUFFER_LENGTH, format, arg);
    
    UART4_Write((uint8_t *)(DEBUG_BUFFER), sz);
    va_end(arg);
}


void printDebugArray(const char *prepend, const void *data, uint32_t length) {
   uint16_t i;

   for(i = 0; i < length; i++)
   {
      //Beginning of a new line?
      if((i % 16) == 0)
         printDebug("%s", prepend);
      //Display current data byte
      printDebug("%02" PRIX8 " ", *((uint8_t *) data + i));
      //End of current line?
      if((i % 16) == 15 || i == (length - 1))
         printDebug("\r\n");
   }
}

static void UartRxEventHandler(UART_EVENT event, uintptr_t contextHandle) {  
    (void)contextHandle;
    if(event == UART_EVENT_READ_THRESHOLD_REACHED) {
        // No use currently
//        size_t len = UART4_ReadCountGet();
//        uint8_t *buff = malloc(len);
//        
//        UART4_Read(buff, len);
//        UART2_Write(buff, len);
//        
//        free(buff);
    }
}

#define MAX_RECORDING_SECTIONS (8)

static systime_t recordingStartTimeStamp;
static systime_t recordingSectionsStartTimeStamps[MAX_RECORDING_SECTIONS];

static systime_t recordingSectionsSum[MAX_RECORDING_SECTIONS];

static char* recordingSectionNames[MAX_RECORDING_SECTIONS] = {
                                                                "Sending 1",
                                                                "RECEIVED",
                                                                "SENDING 2",
                                                                "macrawRx_processReservedChunk",
                                                                "!macraw_isRxEmpty()",
                                                                "!process_Interlink()",
                                                                "dhcpServerRunning()",
                                                                "WTF"
                                                            };

void restart_recordingSections() {
    recordingStartTimeStamp = get_SysTime_ms();
    for(int i = 0; i < MAX_RECORDING_SECTIONS; i++) {
        recordingSectionsSum[i] = 0;
    }
}
void print_recordingSections() {
    systime_t overall_time = get_SysTime_ms() - recordingStartTimeStamp;
    for(int i = 0; i < MAX_RECORDING_SECTIONS; i++) {
        printDebug("%s: %u\r\n", recordingSectionNames[i], (100*recordingSectionsSum[i])/overall_time);
    }
    restart_recordingSections();
}
void start_recordSection(uint8_t section_num) {
    recordingSectionsStartTimeStamps[section_num] = get_SysTime_ms();
}

void end_recordSection(uint8_t section_num) {
    recordingSectionsSum[section_num] += get_SysTime_ms() - recordingSectionsStartTimeStamps[section_num] ;
}