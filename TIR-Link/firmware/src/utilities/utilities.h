#ifndef _UTILITIES_H    /* Guard against multiple inclusion */
#define _UTILITIES_H


#include <device.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

void delay_us(uint32_t us_to_wait);
void delay_ms(uint32_t ms);

void printDebug (const char * format, ... );

#endif