#ifndef _MACRAW_BUFFERPKTQUEUE_H
#define _MACRAW_BUFFERPKTQUEUE_H

#include <inttypes.h>
#include <stdbool.h>

typedef struct PktEntry PktEntry;

struct PktEntry {
    uint16_t buffer_index;
    uint16_t pkt_length;
    PktEntry *next;
};

void enqueue_PktQueue(uint16_t buffer_index, uint16_t pkt_length);
PktEntry* dequeue_PktQueue(void);
bool isEmpty_PktQueue(void);

#endif // _MACRAW_BUFFERPKTQUEUE_H