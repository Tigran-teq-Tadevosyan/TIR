#ifndef _COMMON_H    /* Guard against multiple inclusion */
#define _COMMON_H

#include <device.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define NO_CONTEXT (0)

#define timeCompare(t1, t2) ((int32_t) ((t1) - (t2)))
#define arraysize(a) (sizeof(a) / sizeof(a[0]))

//Load unaligned 16-bit integer (big-endian encoding)
#define LOAD16BE(p) ( \
   ((uint16_t)(((uint8_t *)(p))[0]) << 8) | \
   ((uint16_t)(((uint8_t *)(p))[1]) << 0))

//Load unaligned 32-bit integer (big-endian encoding)
#define LOAD32BE(p) ( \
   ((uint32_t)(((uint8_t *)(p))[0]) << 24) | \
   ((uint32_t)(((uint8_t *)(p))[1]) << 16) | \
   ((uint32_t)(((uint8_t *)(p))[2]) << 8) | \
   ((uint32_t)(((uint8_t *)(p))[3]) << 0))

typedef enum {
    Success = 0,
    Failure = 1
} TIR_Status;

typedef uint32_t systime_t;

// Initializes the system timer
void init_SysClock(void);
systime_t get_SysTime(void); // Returns system time in microseconds

void delay_us(systime_t us_to_wait);
void delay_ms(systime_t ms);

#endif