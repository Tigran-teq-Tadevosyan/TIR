#ifndef _DHCP_SERVER_H
#define _DHCP_SERVER_H

#include "DHCP.h"

#define DHCP_SERVER_MAX_CLIENTS 16
#define DHCP_SERVER_DEFAULT_LEASE_TIME 120000 // 2 minutes in milliseconds

extern const Ipv4Addr DHCP_SERVER_IPv4_ADDRESS_MIN;
extern const Ipv4Addr DHCP_SERVER_IPv4_ADDRESS_MAX;

typedef struct
{
   MacAddr macAddr;     ///<Client's MAC address
   Ipv4Addr ipAddr;     ///<Client's IPv4 address
   bool validLease;     ///<Valid lease
   systime_t timestamp; ///<Timestamp
} DhcpServerBinding;

extern DhcpServerBinding clientBinding[DHCP_SERVER_MAX_CLIENTS];

TIR_Status DHCP_server_start(const char* device_name);

void dhcpServerTick();
void dhcpServerParseDiscover(const DhcpMessage *message, size_t length);
void dhcpServerParseRequest(const DhcpMessage *message, size_t length);
void dhcpServerParseDecline(const DhcpMessage *message, size_t length);
void dhcpServerParseRelease(const DhcpMessage *message, size_t length);
void dhcpServerParseInform(const DhcpMessage *message, size_t length);

DhcpServerBinding *dhcpServerFindBindingByMacAddr(const MacAddr *macAddr);
DhcpServerBinding *dhcpServerFindBindingByIpAddr(Ipv4Addr ipAddr);

DhcpServerBinding *dhcpServerCreateBinding();
TIR_Status dhcpServerGetNextIpAddr(Ipv4Addr *ipAddr);

TIR_Status dhcpServerSendReply(uint8_t type, Ipv4Addr yourIpAddr, const DhcpMessage *request, size_t requestLen);

#endif
