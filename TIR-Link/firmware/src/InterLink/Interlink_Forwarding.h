#ifndef _INTERLINK_DHCP_H
#define _INTERLINK_DHCP_H

#include "Network/DHCP/DHCP_Server.h"

typedef struct {
    MacAddr     macAddr;
    Ipv4Addr    ipAddr;
} __attribute__((__packed__)) ForwardingBinding;


void interlink_ForwardIfAppropriate(EthFrame *frame, uint16_t frame_length);

void send_AddForwardingEntry(DhcpServerBinding *binding);
void send_RemoveForwardingEntry(DhcpServerBinding *binding);

void process_AddForwardingEntry(ForwardingBinding *fBinding);
void process_RemoveForwardingEntry(ForwardingBinding *fBinding);
//void process_ForwardingRequest(EthFrame* frame, uint16_t frame_length);
//void process_ForwardingRequest(EthFrame* frame, uint16_t frame_length);

#endif // _INTERLINK_DHCP_H