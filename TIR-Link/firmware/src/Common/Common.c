#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include "definitions.h"                // SYS function prototypes

#include "Common.h"

static void sysTimerTick(uint32_t status, uintptr_t contextHandle); // called every 100 us

static volatile systime_t system_tick_us;
static volatile systime_t delay_tick_us;
volatile systime_t UART2_timeout_us;

static volatile systime_t system_tick_ms;
static volatile systime_t delay_tick_ms;
volatile systime_t UART2_timeout_ms;

void init_SysClock() {
    srand(DEVSN0);
    
    system_tick_us = 0;
    delay_tick_us = 0;
            
    TMR1_CallbackRegister(sysTimerTick, NO_CONTEXT);
    TMR1_Start();
}

systime_t get_SysTime_ms(void) {
    return system_tick_us/1000;
}

void delay_ms(systime_t ms)
{
    ms *= 1000;
    delay_tick_us = 0;
    while(delay_tick_us < ms);
}

static void sysTimerTick(uint32_t status, uintptr_t contextHandle) { 
//    ++system_tick_ms;
//    ++delay_tick_ms;
//    ++UART2_timeout_ms;
    ++system_tick_us;
    ++delay_tick_us;
    ++UART2_timeout_us;
    
    
//        system_tick_us += 10;
//    delay_tick_us += 10;
//    UART2_timeout_us += 10;
//    LED_GG_Toggle();

}

uint16_t getRandNumber(uint16_t min, uint16_t max) {
    return rand() % (max - min) + min;
}
