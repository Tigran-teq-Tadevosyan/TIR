#ifndef _ETHERNET_H
#define _ETHERNET_H

#include <inttypes.h>

#define ETH_HEADER_SIZE 14

typedef enum
{
   ETH_TYPE_IPV4  = 0x0800,
   ETH_TYPE_ARP   = 0x0806,
   ETH_TYPE_RARP  = 0x8035,
   ETH_TYPE_VLAN  = 0x8100,
   ETH_TYPE_IPV6  = 0x86DD,
   ETH_TYPE_EAPOL = 0x888E,
   ETH_TYPE_VMAN  = 0x88A8,
   ETH_TYPE_LLDP  = 0x88CC,
   ETH_TYPE_PTP   = 0x88F7
} EthType;

typedef struct
{
   union
   {
      uint8_t b[6];
      uint16_t w[3];
   };
} __attribute__((__packed__)) MacAddr;

typedef struct
{
   MacAddr destAddr; //0-5
   MacAddr srcAddr;  //6-11
   uint16_t type;    //12-13
   uint8_t data[];   //14
} __attribute__((__packed__)) EthFrame;

extern const MacAddr MAC_UNSPECIFIED_ADDR;
extern const MacAddr MAC_BROADCAST_ADDR;
//extern const MacAddr MAC_SOURCE_ADDR;

#define macCompAddr(macAddr1, macAddr2) (!memcmp(macAddr1, macAddr2, sizeof(MacAddr)))

char *macAddrToString(const MacAddr *macAddr, char *str);
void ethDumpHeader(const EthFrame *EthFrame);

#endif // _ETHERNET_H
