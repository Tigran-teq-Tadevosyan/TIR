#include "Ethernet.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <inttypes.h>

//Unspecified MAC address
const MacAddr MAC_UNSPECIFIED_ADDR = {{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}};
//Broadcast MAC address
const MacAddr MAC_BROADCAST_ADDR = {{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}};

const MacAddr MAC_SOURCE_ADDR = {{{0x00, 0xE0, 0x4C, 0x02, 0x4D, 0x37}}};

char *macAddrToString(const MacAddr *macAddr, char *str)
{
   static char buffer[24];

   //The str parameter is optional
   if(str == NULL)
   {
      str = buffer;
   }

   //Format MAC address
   sprintf(str, "%02" PRIX8 "-%02" PRIX8 "-%02" PRIX8
      "-%02" PRIX8 "-%02" PRIX8 "-%02" PRIX8,
      macAddr->b[0], macAddr->b[1], macAddr->b[2],
      macAddr->b[3], macAddr->b[4], macAddr->b[5]);

   //Return a pointer to the formatted string
   return str;
}

void ethDumpHeader(const EthHeader *ethHeader)
{
    //Dump Ethernet header contents
    printf("## Ethernet Section\r\n");
    printf("  Dest Addr = %s\r\n", macAddrToString(&ethHeader->destAddr, NULL));
    printf("  Src Addr = %s\r\n", macAddrToString(&ethHeader->srcAddr, NULL));
    printf("  Type = 0x%04" PRIX16 "\r\n", ntohs(ethHeader->type));
}
