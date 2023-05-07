#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <stdio.h>
#include <stdarg.h>

#include "definitions.h"                // SYS function prototypes

// TIRLink includes
    #include "Common.h"

static void sysTimerTick(uint32_t status, uintptr_t contextHandle);

systime_t system_tick_us;
systime_t delay_tick_us;

void init_SysClock() {
    system_tick_us = 0;
    delay_tick_us = 0;
            
    TMR1_CallbackRegister(sysTimerTick, NO_CONTEXT);
    TMR1_Start();
}

systime_t get_SysTime(void) { 
    return system_tick_us;
}

void delay_us(systime_t us_to_wait)
{
    delay_tick_us = 0;
    while(delay_tick_us < us_to_wait);
}

void delay_ms(systime_t ms)
{
    while(ms > 0)
    {
        delay_us(1000);
        ms--;
    }
}

static void sysTimerTick(uint32_t status, uintptr_t contextHandle)
{
    system_tick_us++;
    delay_tick_us++;
}