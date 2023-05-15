#ifndef _DHCP_MESSAGEQUEUE_H
#define _DHCP_MESSAGEQUEUE_H

#include <inttypes.h>
#include <stdbool.h>

#define DHCP_MESSAGE_QUEUE_CAPACITY (10)
#define DHCP_MESSAGE_LENGTH (ETH_HEADER_SIZE + IPV4_MIN_HEADER_LENGTH + UDP_HEADER_LENGTH + DHCP_MAX_MSG_SIZE)

bool isEmpty_DHCPMessageQueue(void);
uint8_t* reserveNew_DHCPMessageQueue(void);
void appendReserved_DHCPMessageQueue(void);
uint8_t* peekHead_DHCPMessageQueue(void);
void removeHead_DHCPMessageQueue(void);

#endif // _DHCP_MESSAGEQUEUE_H