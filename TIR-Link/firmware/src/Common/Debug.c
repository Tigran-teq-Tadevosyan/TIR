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