#include "UDP.h"

#include <endian.h>
#include "Common/Debug.h"

void udpDumpHeader(const UdpHeader *datagram)
{
    //Dump UDP header contents
    printDebug("## UPD Section\r\n");
    printDebug("  Source Port = %" PRIu16 "\r\n", betoh16(datagram->srcPort));
    printDebug("  Destination Port = %" PRIu16 "\r\n", betoh16(datagram->destPort));
    printDebug("  Length = %" PRIu16 "\r\n", betoh16(datagram->length));
    printDebug("  Checksum = 0x%04" PRIX16 "\r\n", betoh16(datagram->checksum));
}
