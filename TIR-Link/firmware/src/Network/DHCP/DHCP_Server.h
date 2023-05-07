#ifndef _DHCP_SERVER_H
#define _DHCP_SERVER_H

#include "DHCP.h"

#define DHCP_SERVER_DEBUG_LEVEL0 // Sending to debug uart basic debugging data like assining new ip address
//#define DHCP_SERVER_DEBUG_LEVEL1 // Sending to debug uart all debugging data like what packages where recieved, sent or rejected (including dumping content)

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

TIR_Status dhcpServerProcessPkt(const EthFrame *ethFrame, const uint16_t frame_len);

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

#endif // _DHCP_SERVER_H
