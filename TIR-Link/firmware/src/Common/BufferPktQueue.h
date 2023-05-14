#ifndef _BUFFERPKTQUEUE_H
#define _BUFFERPKTQUEUE_H

#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

typedef struct PktQueueEntry PktQueueEntry;
typedef struct PktQueue PktQueue;

struct PktQueueEntry {
    uint16_t buffer_index;
    uint16_t pkt_length;
    PktQueueEntry *next;
};

struct PktQueue {
    PktQueueEntry *head, *tail;
    size_t length;
};

void enqueue_PktQueue(PktQueue *queue, uint16_t buffer_index, uint16_t pkt_length);
PktQueueEntry* dequeue_PktQueue(PktQueue *queue);
uint16_t peekHeadIndex_PktQueue(PktQueue *queue);
bool isEmpty_PktQueue(PktQueue *queue);

#endif // _BUFFERPKTQUEUE_H