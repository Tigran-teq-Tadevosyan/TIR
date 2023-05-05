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

static void UartRxEventHandler(UART_EVENT event, uintptr_t contextHandle) {  
    (void)contextHandle;
    if(event == UART_EVENT_READ_THRESHOLD_REACHED) {;
    
        // No use currently
    }
}