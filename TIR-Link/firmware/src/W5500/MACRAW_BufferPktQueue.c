#include "MACRAW_BufferPktQueue.h"

#include <stdlib.h>

static PktEntry   *HEAD = NULL,
                  *TAIL = NULL;
static size_t QUEUE_LENGTH = 0;

void enqueue_PktQueue(uint16_t buffer_index, uint16_t pkt_length) {
    PktEntry *new_entry = malloc(sizeof(*new_entry));
    
    new_entry->buffer_index = buffer_index;
    new_entry->pkt_length = pkt_length;
    new_entry->next = NULL;
    
    if(QUEUE_LENGTH == 0) {
        HEAD = new_entry;
    } else {
        TAIL->next = new_entry;
    }
    
    TAIL = new_entry;
    ++QUEUE_LENGTH;
}

PktEntry* dequeue_PktQueue(void) {
    if(QUEUE_LENGTH == 0) return NULL;
    
    PktEntry *temp = HEAD;
    
    HEAD = HEAD->next;
    --QUEUE_LENGTH;
    return temp;
}

bool isEmpty_PktQueue(void) { 
    return (QUEUE_LENGTH == 0);
}

