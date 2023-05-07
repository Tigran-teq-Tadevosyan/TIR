#ifndef _DHCP_DEBUG_H
#define _DHCP_DEBUG_H

#include "DHCP.h"

TIR_Status dhcpDumpMessage(const DhcpMessage *message, size_t length);
TIR_Status dhcpDumpMessageType(const DhcpOption *option);
TIR_Status dhcpDumpParamRequestList(const DhcpOption *option);
TIR_Status dhcpDumpBoolean(const DhcpOption *option);
TIR_Status dhcpDumpInt8(const DhcpOption *option);
TIR_Status dhcpDumpInt16(const DhcpOption *option);
TIR_Status dhcpDumpInt32(const DhcpOption *option);
TIR_Status dhcpDumpString(const DhcpOption *option);
TIR_Status dhcpDumpIpv4Addr(const DhcpOption *option);
TIR_Status dhcpDumpIpv4AddrList(const DhcpOption *option);
TIR_Status dhcpDumpRawData(const DhcpOption *option);

#endif
