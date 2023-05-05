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

void printDebug (const char * format, ... )
{
    uint16_t sz = 0;
    char pt[200];
    va_list arg;
    va_start(arg, format);
    sz = vsnprintf(pt, sizeof(pt), format, arg);
    
    UART4_Write((uint8_t *)(pt), sz);
    va_end(arg);
}

static void UartRxEventHandler(UART_EVENT event, uintptr_t contextHandle)
{
   LED_GG_Toggle();
   
   if(event == UART_EVENT_READ_THRESHOLD_REACHED)
   {
       // No use currently
   }
}