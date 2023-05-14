#ifndef _INTERLINKBUFFER_H
#define _INTERLINKBUFFER_H

#include <inttypes.h>
#include <stdbool.h>

#define INTERLINK_BUFFER_LENGTH (30000) // in bytes

uint8_t* interlinkBuffer_reserve(uint16_t length);
void interlinkBuffer_processReservedChunk(void);
bool interlinkBuffer_isForwardingQueueEmpty(void);
uint8_t* interlinkBuffer_getForwardingPkt(uint16_t *length);

#endif // _INTERLINKBUFFER_H