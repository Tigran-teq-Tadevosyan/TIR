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

//Swap a 16-bit integer
#define SWAPINT16(x) ( \
   (((uint16_t)(x) & 0x00FFU) << 8) | \
   (((uint16_t)(x) & 0xFF00U) >> 8))

//Swap a 32-bit integer
#define SWAPINT32(x) ( \
   (((uint32_t)(x) & 0x000000FFUL) << 24) | \
   (((uint32_t)(x) & 0x0000FF00UL) << 8) | \
   (((uint32_t)(x) & 0x00FF0000UL) >> 8) | \
   (((uint32_t)(x) & 0xFF000000UL) >> 24))

//Swap a 64-bit integer
#define SWAPINT64(x) ( \
   (((uint64_t)(x) & 0x00000000000000FFULL) << 56) | \
   (((uint64_t)(x) & 0x000000000000FF00ULL) << 40) | \
   (((uint64_t)(x) & 0x0000000000FF0000ULL) << 24) | \
   (((uint64_t)(x) & 0x00000000FF000000ULL) << 8) | \
   (((uint64_t)(x) & 0x000000FF00000000ULL) >> 8) | \
   (((uint64_t)(x) & 0x0000FF0000000000ULL) >> 24) | \
   (((uint64_t)(x) & 0x00FF000000000000ULL) >> 40) | \
   (((uint64_t)(x) & 0xFF00000000000000ULL) >> 56))

#define HTONS(value) SWAPINT16(value)
#define HTONL(value) SWAPINT32(value)
#define HTONLL(value) SWAPINT64(value)

typedef enum {
    Success = 0,
    Failure = 1
} TIR_Status;

typedef uint64_t systime_t;
typedef uint32_t size_t;

// Initializes the system timer
void init_SysClock(void);
systime_t get_SysTime_us(void); // Returns system time in microseconds
systime_t get_SysTime_ms(void); // Returns system time in milliseconds

void delay_us(systime_t us_to_wait);
void delay_ms(systime_t ms);

#endif