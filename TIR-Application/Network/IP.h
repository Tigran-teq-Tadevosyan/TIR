#ifndef _IP_H
#define _IP_H

#include <stdio.h>
#include <string.h>

#include <pcap.h>

#include "Common.h"

//Version number for IPv4
#define IPV4_VERSION (4)
//Minimum MTU
#define IPV4_MINIMUM_MTU (68)
//Default MTU
#define IPV4_DEFAULT_MTU (576)
//Minimum header length
#define IPV4_MIN_HEADER_LENGTH (20)
//Maximum header length
#define IPV4_MAX_HEADER_LENGTH (60)

#define IPV4_DEFAULT_TTL (15)

#ifdef _CPU_BIG_ENDIAN
   #define IPV4_ADDR(a, b, c, d) (((uint32_t) (a) << 24) | ((b) << 16) | ((c) << 8) | (d))
#else
   #define IPV4_ADDR(a, b, c, d) ((a) | ((b) << 8) | ((c) << 16) | ((uint32_t) (d) << 24))
#endif

//Unspecified IPv4 address
#define IPV4_UNSPECIFIED_ADDR IPV4_ADDR(0, 0, 0, 0)
//Broadcast IPV4 address
#define IPV4_BROADCAST_ADDR IPV4_ADDR(255, 255, 255, 255)

//Copy IPv4 address
#define ipv4CopyAddr(destIpAddr, srcIpAddr) \
   memcpy(destIpAddr, srcIpAddr, sizeof(Ipv4Addr))

typedef enum
{
   IPV4_FLAG_RES    = 0x8000,
   IPV4_FLAG_DF     = 0x4000,
   IPV4_FLAG_MF     = 0x2000,
   IPV4_OFFSET_MASK = 0x1FFF
} Ipv4FragmentOffset;

typedef enum
{
   IPV4_PROTOCOL_ICMP = 1,
   IPV4_PROTOCOL_IGMP = 2,
   IPV4_PROTOCOL_TCP  = 6,
   IPV4_PROTOCOL_UDP  = 17,
   IPV4_PROTOCOL_ESP  = 50,
   IPV4_PROTOCOL_AH   = 51
} Ipv4Protocol;

//Forward declaration of structures
struct _Ipv4Header;
#define Ipv4Header struct _Ipv4Header

struct _Ipv4PseudoHeader;
#define Ipv4PseudoHeader struct _Ipv4PseudoHeader

typedef uint32_t Ipv4Addr;

struct _Ipv4Header
{
#if defined(_CPU_BIG_ENDIAN) && !defined(__ICCRX__)
   uint8_t version : 4;      //0
   uint8_t headerLength : 4;
#else
   uint8_t headerLength : 4; //0
   uint8_t version : 4;
#endif
   uint8_t typeOfService;    //1
   uint16_t totalLength;     //2-3
   uint16_t identification;  //4-5
   uint16_t fragmentOffset;  //6-7
   uint8_t timeToLive;       //8
   uint8_t protocol;         //9
   uint16_t headerChecksum;  //10-11
   Ipv4Addr srcAddr;         //12-15
   Ipv4Addr destAddr;        //16-19
   uint8_t options[];        //20
} __attribute__((__packed__));

/**
 * @brief IPv4 pseudo header
 **/
struct _Ipv4PseudoHeader
{
   Ipv4Addr srcAddr;  //0-3
   Ipv4Addr destAddr; //4-7
   uint8_t reserved;  //8
   uint8_t protocol;  //9
   uint16_t length;   //10-11
} __attribute__((__packed__));

#define ipv4CompAddr(ipAddr1, ipAddr2) \
   (!memcmp(ipAddr1, ipAddr2, sizeof(Ipv4Addr)))

extern uint16_t IPv4_IDENTIFICATION;


char *ipv4AddrToString(Ipv4Addr ipAddr, char *str);
Status ipv4StringToAddr(const char *str, Ipv4Addr *ipAddr);

int isFragmentedPacket(Ipv4Header *packet);

uint16_t ipCalcChecksum(const void *data, size_t length);

void ipv4DumpHeader(const Ipv4Header *ipHeader);

#endif
