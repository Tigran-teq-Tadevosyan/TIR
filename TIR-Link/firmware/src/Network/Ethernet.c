#include "Ethernet.h"

#include <stdio.h>
#include <inttypes.h>
#include <endian.h>

#include "Common/Debug.h"

//Unspecified MAC address
const MacAddr MAC_UNSPECIFIED_ADDR = {{{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}};
//Broadcast MAC address
const MacAddr MAC_BROADCAST_ADDR = {{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}};

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

void ethDumpHeader(const EthFrame *EthFrame)
{
    //Dump Ethernet header contents
    printDebug("## Ethernet Section\r\n");
    printDebug("  Dest Addr = %s\r\n", macAddrToString(&EthFrame->destAddr, NULL));
    printDebug("  Src Addr = %s\r\n", macAddrToString(&EthFrame->srcAddr, NULL));
    printDebug("  Type = 0x%04" PRIX16 "\r\n", betoh16(EthFrame->type));
}
