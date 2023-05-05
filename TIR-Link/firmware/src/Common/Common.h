#ifndef _COMMON_H    /* Guard against multiple inclusion */
#define _COMMON_H

#include <device.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define NO_CONTEXT 0

typedef enum {
    Success = 0,
    Failure = 1
} TIR_Status;

// Initializes the system timer
void init_SysClock(void);

void delay_us(uint32_t us_to_wait);
void delay_ms(uint32_t ms);

#endif