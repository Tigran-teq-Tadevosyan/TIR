#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <stdio.h>
#include <stdarg.h>



#include "definitions.h"                // SYS function prototypes
#include "utilities.h"

extern uint64_t system_tick_us;
extern uint32_t delay_tick_us;

void delay_us(uint32_t us_to_wait)
{
    delay_tick_us = 0;
    while(delay_tick_us < us_to_wait);
}

void delay_ms(uint32_t ms)
{
    while(ms > 0)
    {
        delay_us(1000);
        ms--;
    }
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