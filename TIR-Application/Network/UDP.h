#ifndef _UDP_H
#define _UDP_H

#include <pcap.h>

#define UDP_HEADER_LENGTH (8) // in bytes

typedef struct
{
   uint16_t srcPort;  //0-1
   uint16_t destPort; //2-3
   uint16_t length;   //4-5
   uint16_t checksum; //6-7
   uint8_t data[];    //8
} __attribute__((__packed__)) UdpHeader;


void udpDumpHeader(const UdpHeader *datagram);
#endif
