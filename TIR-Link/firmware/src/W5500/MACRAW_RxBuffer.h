#ifndef _MACRAW_BUFFER_H
#define _MACRAW_BUFFER_H

#include <inttypes.h>
#include <stdbool.h>

#define MACRAW_RX_BUFFER_LENGTH (30000) // in bytes

bool macrawRx_isEmpty(void);
uint8_t* macrawRx_reserve(uint16_t length);
void macrawRx_processReservedChunk();
uint8_t* macrawRx_getHeadPkt(uint16_t *length);

#endif  // _MACRAW_BUFFER_H