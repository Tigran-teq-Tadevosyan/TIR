#ifndef _INTERLINKBUFFER_H
#define _INTERLINKBUFFER_H

#include <inttypes.h>
#include <stdbool.h>

#define INTERLINK_TIMEOUT_US (10000) // in microseconds
#define INTERLINK_TIMEOUT_MS (INTERLINK_TIMEOUT_US/1000) // in microseconds

#define INTERLINK_BUFFER_LENGTH (30000) // in bytes
#define INTERLINK_BUFFER_BLOCK_SIZE (100) // in bytes

void init_intlinkRxDMA(void);
void process_intlinkRxDMA(void);

bool isForwardingQueueEmpty_intlinkRxDMA(void);
uint8_t* getForwardingPkt_intlinkRxDMA(uint16_t *length);

#endif // _INTERLINKBUFFER_H