#ifndef _NETWORK_H    /* Guard against multiple inclusion */
#define _NETWORK_H

#include "Ethernet.h"
#include "IP.h"
#include "UDP.h"

extern const MacAddr    HOST_MAC_ADDR; //A5-E6-38-61-B8-71
extern const Ipv4Addr   HOST_IPv4_ADDRESS; // 100.100.0.1
extern const Ipv4Addr   HOST_IPv4_SUBNET_MUSK; // 255.255.255.0

#endif