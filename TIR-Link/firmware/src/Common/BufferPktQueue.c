#include "BufferPktQueue.h"

void enqueue_PktQueue(PktQueue *queue, uint16_t buffer_index, uint16_t pkt_length) {
    PktQueueEntry *new_entry = malloc(sizeof(*new_entry));
    
    new_entry->buffer_index = buffer_index;
    new_entry->pkt_length = pkt_length;
    new_entry->next = NULL;
    
    if(queue->length == 0) {
        queue->head = new_entry;
    } else {
        queue->tail->next = new_entry;
    }
    
    queue->tail = new_entry;
    ++queue->length;
}

PktQueueEntry* dequeue_PktQueue(PktQueue *queue) {
    if(queue->length == 0) return NULL;
    
    PktQueueEntry *temp = queue->head;
    
    queue->head = queue->head->next;
    --queue->length;
    return temp;
}

uint16_t peekHeadIndex_PktQueue(PktQueue *queue) {
    return queue->head->buffer_index;
}

bool isEmpty_PktQueue(PktQueue *queue) { 
    return (queue->length == 0);
}

