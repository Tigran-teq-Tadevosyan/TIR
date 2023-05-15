#include "DHCP_MessageQueue.h"

#include "DHCP.h"

static uint8_t DHCP_MESSAGE_BUFFER[DHCP_MESSAGE_QUEUE_CAPACITY][DHCP_MESSAGE_LENGTH];
static uint8_t headIndex = 0, 
               tailIndex = 0;

bool isEmpty_DHCPMessageQueue(void) {
    return (headIndex == tailIndex);
}

uint8_t* reserveNew_DHCPMessageQueue(void) {
    return DHCP_MESSAGE_BUFFER[tailIndex];
}

void appendReserved_DHCPMessageQueue(void) {
    tailIndex = (tailIndex + 1)%DHCP_MESSAGE_QUEUE_CAPACITY;
}

uint8_t* peekHead_DHCPMessageQueue(void) {
    return DHCP_MESSAGE_BUFFER[headIndex];
}

void removeHead_DHCPMessageQueue(void) {
    headIndex = (headIndex + 1)%DHCP_MESSAGE_QUEUE_CAPACITY;
}
