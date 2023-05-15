#ifndef _DHCP_SERVER_H
#define _DHCP_SERVER_H

#include "DHCP.h"

#define DHCP_SERVER_DEBUG_LEVEL0 // Sending to debug uart basic debugging data like assining new ip address and removal of expired once
//#define DHCP_SERVER_DEBUG_LEVEL1 // Sending to debug uart all debugging data like what packages where recieved, sent or rejected (including dumping content)

#define DHCP_SERVER1_IPv4_ADDRESS_MIN IPV4_ADDR(100, 100, 0, 2);
#define DHCP_SERVER1_IPv4_ADDRESS_MAX IPV4_ADDR(100, 100, 0, 100);

#define DHCP_SERVER2_IPv4_ADDRESS_MIN IPV4_ADDR(100, 100, 0, 101);
#define DHCP_SERVER2_IPv4_ADDRESS_MAX IPV4_ADDR(100, 100, 0, 200);

#define DHCP_SERVER_MAX_CLIENTS (16)
#define DHCP_SERVER_DEFAULT_LEASE_TIME (120000) // 2 minutes in milliseconds

#define DHCP_SERVER_MAINTENANCE_PERIOD (5000) // 2 seconds in milliseconds

typedef struct
{
   MacAddr macAddr;     ///<Client's MAC address
   Ipv4Addr ipAddr;     ///<Client's IPv4 address
   bool validLease;     ///<Valid lease
   systime_t timestamp; ///<Timestamp
} DhcpServerBinding;

TIR_Status dhcpServerStart(void);
bool dhcpServerRunning(void);

TIR_Status dhcpServerProcessPkt(const EthFrame *ethFrame, const uint16_t frame_len);

void dhcpServerMaintanance(void);

DhcpServerBinding *dhcpServerFindBindingByMacAddr(const MacAddr *macAddr);
DhcpServerBinding *dhcpServerFindBindingByIpAddr(Ipv4Addr ipAddr);

DhcpServerBinding *dhcpServerCreateBinding();
TIR_Status dhcpServerGetNextIpAddr(Ipv4Addr *ipAddr);

TIR_Status dhcpServerSendReply(uint8_t type, Ipv4Addr yourIpAddr, const DhcpMessage *request, size_t requestLen);

#endif // _DHCP_SERVER_H