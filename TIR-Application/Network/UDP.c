#include "UDP.h"

void udpDumpHeader(const UdpHeader *datagram)
{
    //Dump UDP header contents
    printf("## UPD Section\r\n");
    printf("  Source Port = %" PRIu16 "\r\n", ntohs(datagram->srcPort));
    printf("  Destination Port = %" PRIu16 "\r\n", ntohs(datagram->destPort));
    printf("  Length = %" PRIu16 "\r\n", ntohs(datagram->length));
    printf("  Checksum = 0x%04" PRIX16 "\r\n", ntohs(datagram->checksum));
}
