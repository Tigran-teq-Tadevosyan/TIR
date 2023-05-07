#include "DHCP_Debug.h"

#include "DHCP.h"
#include "Common/Debug.h"
#include <endian.h>
//#include "CPU_Endian.h"

#include <stdio.h>
#include <inttypes.h>
#include <sys/types.h>

//DHCP message opcodes
static const char *const opcodeLabel[] =
{
   "",            //0
   "BOOTREQUEST", //1
   "BOOTREPLY"    //2
};

//DHCP options
static const char *const optionLabel[] =
{
   "Pad",                          //0
   "Subnet Mask",                  //1
   "Time Offset",                  //2
   "Router",                       //3
   "Time Server",                  //4
   "Name Server",                  //5
   "DNS Server",                   //6
   "Log Server",                   //7
   "Cookie Server",                //8
   "LPR Server",                   //9
   "Impress Server",               //10
   "Resource Location Server",     //11
   "Host Name",                    //12
   "Boot File Size",               //13
   "Merit Dump File",              //14
   "Domain Name",                  //15
   "Swap Server",                  //16
   "Root Path",                    //17
   "Extensions Path",              //18
   "IP Forwarding",                //19
   "Non-Local Source Routing",     //20
   "Policy Filter",                //21
   "Max Datagram Reassembly Size", //22
   "Default IP TTL",               //23
   "Path MTU Aging Timeout",       //24
   "Path MTU Plateau Table",       //25
   "Interface MTU",                //26
   "All Subnets Are Local",        //27
   "Broadcast Address",            //28
   "Perform Mask Discovery",       //29
   "Mask Supplier",                //30
   "Perform Router Discovery",     //31
   "Router Solicitation Address",  //32
   "Static Route",                 //33
   "Trailer Encapsulation",        //34
   "ARP Cache Timeout",            //35
   "Ethernet Encapsulation",       //36
   "TCP Default TTL",              //37
   "TCP Keepalive Interval",       //38
   "TCP Keepalive Garbage",        //39
   "NIS Domain",                   //40
   "NIS Server",                   //41
   "NTP Server",                   //42
   "Vendor Specific Information",  //43
   "NetBIOS NBNS Server",          //44
   "NetBIOS NBDD Server",          //45
   "NetBIOS Node Type",            //46
   "NetBIOS Scope",                //47
   "X11 Font Server",              //48
   "X11 Display Manager",          //49
   "Requested IP Address",         //50
   "IP Address Lease Time",        //51
   "Option Overload",              //52
   "DHCP Message Type",            //53
   "Server Identifier",            //54
   "Parameter Request List",       //55
   "Message",                      //56
   "Max DHCP Message Size",        //57
   "Renewal (T1) Time Value",      //58
   "Rebinding (T2) Time Value",    //59
   "Vendor Class Identifier",      //60
   "Client Identifier",            //61
   "",                             //62
   "",                             //63
   "NISP Domain",                  //64
   "NISP Server",                  //65
   "TFTP Server Name",             //66
   "Bootfile Name",                //67
   "Mobile IP Home Agent",         //68
   "SMTP Server",                  //69
   "POP3 Server",                  //70
   "NNTP Server",                  //71
   "Default WWW Server",           //72
   "Default Finger Server",        //73
   "Default IRC Server",           //74
   "StreetTalk Server",            //75
   "STDA Server",                  //76
   "",                             //77
   "",                             //78
   "",                             //79
   "Rapid Commit"                  //80
};

//DHCP message types
static const char *const messageLabel[] =
{
   "",             //0
   "DHCPDISCOVER", //1
   "DHCPOFFER",    //2
   "DHCPREQUEST",  //3
   "DHCPDECLINE",  //4
   "DHCPACK",      //5
   "DHCPNAK",      //6
   "DHCPRELEASE",  //7
   "DHCPINFORM"    //8
};

TIR_Status dhcpDumpMessage(const DhcpMessage *message, size_t length)
{
   TIR_Status status;
   uint16_t i;
   const char *label;
   DhcpOption *option;

   //Ensure the length of the DHCP message is acceptable
   if(length < sizeof(DhcpMessage))
   {
      //Report a warning
      printf("DHCP message length is invalid!\r\n");

      //Dump message contents for debugging purpose
      printDebugArray("  ", message, length);

      //Report an error
      return Failure;
   }

   //Retrieve the name associated with the opcode
   label = (message->op < arraysize(opcodeLabel)) ? opcodeLabel[message->op] : "";

   //Dump DHCP message contents
   printDebug("  Op Code (op) = %" PRIu8 " (%s)\r\n", message->op, label);
   printDebug("  Hardware Type (htype) = %" PRIu8 "\r\n", message->htype);
   printDebug("  Hardware Address Length (hlen) = %" PRIu8 "\r\n", message->hlen);
   printDebug("  Hops (hops) = %" PRIu8 "\r\n", message->hops);
   printDebug("  Transaction ID (xid) = 0x%08" PRIX32 "\r\n", betoh32(message->xid));
   printDebug("  Seconds (secs) = %" PRIu16 "s\r\n", betoh16(message->secs));
   printDebug("  Flags (flags) = 0x%04" PRIX16 "\r\n", betoh16(message->flags));
   printDebug("  Client IP Address (ciaddr) = %s\r\n", ipv4AddrToString(message->ciaddr, NULL));
   printDebug("  Your IP Address (yiaddr) = %s\r\n", ipv4AddrToString(message->yiaddr, NULL));
   printDebug("  Server IP Address (siaddr) = %s\r\n", ipv4AddrToString(message->siaddr, NULL));
   printDebug("  Relay IP Address (giaddr) = %s\r\n", ipv4AddrToString(message->giaddr, NULL));
   printDebug("  Client Hardware Address (chaddr) = %s\r\n", macAddrToString(&message->chaddr, NULL));
   printDebug("  Magic Cookie = 0x%08" PRIX32 "\r\n", betoh32(message->magicCookie));

   //Get the length of the options field
   length -= sizeof(DhcpMessage);

   //Parse DHCP options
   for(i = 0; i < length; i++)
   {
      //Point to the current option
      option = (DhcpOption *) (message->options + i);

      //Pad option detected?
      if(option->code == DHCP_OPT_PAD)
         continue;

      //End option detected?
      if(option->code == DHCP_OPT_END)
         break;

      //Check option length
      if((i + 1) >= length || (i + 1 + option->length) >= length)
      {
         //Report a warning
         printDebug("DHCP option length is invalid!\r\n");

         //Dump message contents for debugging purpose
         printDebugArray("  ", message, length);

         //Report an error
         return Failure;
      }

      //Display the name of the current option
      if(option->code < arraysize(optionLabel))
      {
         printDebug("  %s option (%" PRIu8 " bytes)\r\n", optionLabel[option->code], option->length);
      }
      else
      {
         printDebug("  Option %" PRIu8 " (%" PRIu8 " bytes)\r\n", option->code, option->length);
      }

      //Check option code
      switch(option->code)
      {
      //Message type?
      case DHCP_OPT_DHCP_MESSAGE_TYPE:
         status = dhcpDumpMessageType(option);
         break;

      //Parameter Request List option
      case DHCP_OPT_PARAM_REQUEST_LIST:
         status = dhcpDumpParamRequestList(option);
         break;

      //Boolean value?
      case DHCP_OPT_IP_FORWARDING:
      case DHCP_OPT_NON_LOCAL_SOURCE_ROUTING:
      case DHCP_OPT_ALL_SUBNETS_ARE_LOCAL:
      case DHCP_OPT_PERFORM_MASK_DISCOVERY:
      case DHCP_OPT_MASK_SUPPLIER:
      case DHCP_OPT_PERFORM_ROUTER_DISCOVERY:
      case DHCP_OPT_TRAILER_ENCAPSULATION:
      case DHCP_OPT_ETHERNET_ENCAPSULATION:
      case DHCP_OPT_TCP_KEEPALIVE_GARBAGE:
         status = dhcpDumpBoolean(option);
         break;

      //8-bit unsigned integer?
      case DHCP_OPT_DEFAULT_IP_TTL:
      case DHCP_OPT_TCP_DEFAULT_TTL:
      case DHCP_OPT_NETBIOS_NODE_TYPE:
      case DHCP_OPT_OPTION_OVERLOAD:
         status = dhcpDumpInt8(option);
         break;

      //16-bit unsigned integer?
      case DHCP_OPT_BOOT_FILE_SIZE:
      case DHCP_OPT_MAX_DATAGRAM_REASSEMBLY_SIZE:
      case DHCP_OPT_INTERFACE_MTU:
      case DHCP_OPT_MAX_DHCP_MESSAGE_SIZE:
         status = dhcpDumpInt16(option);
         break;

      //32-bit unsigned integer?
      case DHCP_OPT_PATH_MTU_AGING_TIMEOUT:
      case DHCP_OPT_ARP_CACHE_TIMEOUT:
      case DHCP_OPT_TCP_KEEPALIVE_INTERVAL:
      case DHCP_OPT_IP_ADDRESS_LEASE_TIME:
      case DHCP_OPT_RENEWAL_TIME_VALUE:
      case DHCP_OPT_REBINDING_TIME_VALUE:
         status = dhcpDumpInt32(option);
         break;

      //Character strings?
      case DHCP_OPT_HOST_NAME:
      case DHCP_OPT_MERIT_DUMP_FILE:
      case DHCP_OPT_DOMAIN_NAME:
      case DHCP_OPT_ROOT_PATH:
      case DHCP_OPT_EXTENSIONS_PATH:
      case DHCP_OPT_NIS_DOMAIN:
      case DHCP_OPT_MESSAGE:
      case DHCP_OPT_NISP_DOMAIN:
      case DHCP_OPT_TFTP_SERVER_NAME:
      case DHCP_OPT_BOOTFILE_NAME:
         status = dhcpDumpString(option);
         break;

      //IPv4 address?
      case DHCP_OPT_SUBNET_MASK:
      case DHCP_OPT_SWAP_SERVER:
      case DHCP_OPT_BROADCAST_ADDR:
      case DHCP_OPT_ROUTER_SOLICITATION_ADDR:
      case DHCP_OPT_REQUESTED_IP_ADDR:
      case DHCP_OPT_SERVER_ID:
         status = dhcpDumpIpv4Addr(option);
         break;

      //List of IPv4 addresses?
      case DHCP_OPT_ROUTER:
      case DHCP_OPT_TIME_SERVER:
      case DHCP_OPT_NAME_SERVER:
      case DHCP_OPT_DNS_SERVER:
      case DHCP_OPT_LOG_SERVER:
      case DHCP_OPT_COOKIE_SERVER:
      case DHCP_OPT_LPR_SERVER:
      case DHCP_OPT_IMPRESS_SERVER:
      case DHCP_OPT_RESOURCE_LOCATION_SERVER:
      case DHCP_OPT_NIS_SERVER:
      case DHCP_OPT_NTP_SERVER:
      case DHCP_OPT_NETBIOS_NBNS_SERVER:
      case DHCP_OPT_NETBIOS_NBDD_SERVER:
      case DHCP_OPT_X11_FONT_SERVER:
      case DHCP_OPT_X11_DISPLAY_MANAGER:
      case DHCP_OPT_NISP_SERVER:
      case DHCP_OPT_MOBILE_IP_HOME_AGENT:
      case DHCP_OPT_SMTP_SERVER:
      case DHCP_OPT_POP3_SERVER:
      case DHCP_OPT_NNTP_SERVER:
      case DHCP_OPT_DEFAULT_WWW_SERVER:
      case DHCP_OPT_DEFAULT_FINGER_SERVER:
      case DHCP_OPT_DEFAULT_IRC_SERVER:
      case DHCP_OPT_STREET_TALK_SERVER:
      case DHCP_OPT_STDA_SERVER:
         status = dhcpDumpIpv4AddrList(option);
         break;

      //Raw data?
      default:
         status = dhcpDumpRawData(option);
         break;
      }

      //Failed to parse current option?
      if(status == Failure)
      {
         //Report a warning
         printDebug("Failed to parse DHCP options!\r\n");
         //Dump message contents for debugging purpose
         printDebugArray("  ", message, length);
      }

      //Jump to the next option
      i += option->length + 1;
   }

   //No error to report
   return Success;
}


TIR_Status dhcpDumpMessageType(const DhcpOption *option)
{
   uint8_t type;
   const char *label;

   //Check option length
   if(option->length != 1)
      return Failure;

   //Get the message type
   type = option->value[0];
   //Retrieve the name of the current DHCP message
   label = (type < arraysize(messageLabel)) ? messageLabel[type] : "Unknown";
   //Display message type
   printDebug("    %" PRIu8 " (%s)\r\n", type, label);

   //No error to report
   return Success;
}

TIR_Status dhcpDumpParamRequestList(const DhcpOption *option)
{
   size_t i;
   uint8_t code;
   const char *label;

   //Parse the list of requested options
   for(i = 0; i < option->length; i++)
   {
      //Get current option code
      code = option->value[i];
      //Find the name associated with this option code
      label = (code < arraysize(optionLabel)) ? optionLabel[code] : "Unknown";
      //Display option code and option name
      printDebug("    %" PRIu8 " (%s option)\r\n", code, label);
   }

   //No error to report
   return Success;
}

TIR_Status dhcpDumpBoolean(const DhcpOption *option)
{
   //Check option length
   if(option->length != 1)
      return Failure;

   //Dump option contents
   printDebug("    %" PRIu8 " (%s)\r\n", option->value[0], option->value[0] ? "True" : "False");

   //No error to report
   return Success;
}

TIR_Status dhcpDumpInt16(const DhcpOption *option)
{
   uint16_t value;

   //Check option length
   if(option->length != 2)
      return Failure;

   //Retrieve 16-bit value
   value = LOAD16BE(option->value);
   //Dump option contents
   printDebug("    %" PRIu16 "\r\n", value);

   //No error to report
   return Success;
}

TIR_Status dhcpDumpInt32(const DhcpOption *option)
{
   uint32_t value;

   //Check option length
   if(option->length != 4)
      return Failure;

   //Retrieve 32-bit value
   value = LOAD32BE(option->value);
   //Dump option contents
   printDebug("    %" PRIu32 "\r\n", value);

   //No error to report
   return Success;
}

TIR_Status dhcpDumpInt8(const DhcpOption *option)
{
   //Check option length
   if(option->length != 1)
      return Failure;

   //Dump option contents
   printDebug("    %" PRIu8 "\r\n", option->value[0]);

   //No error to report
   return Success;
}

TIR_Status dhcpDumpString(const DhcpOption *option)
{
   size_t i;

   //Append prefix
   printDebug("    ");
   //Dump option contents
   for(i = 0; i < option->length; i++)
      printDebug("%c", option->value[i]);
   //Add a line feed
   printDebug("\r\n");

   //No error to report
   return Success;
}

TIR_Status dhcpDumpIpv4Addr(const DhcpOption *option)
{
   Ipv4Addr ipAddr;

   //Check option length
   if(option->length != sizeof(Ipv4Addr))
      return Failure;

   //Retrieve IPv4 address
   ipv4CopyAddr(&ipAddr, option->value);
   //Dump option contents
   printDebug("    %s\r\n", ipv4AddrToString(ipAddr, NULL));

   //No error to report
   return Success;
}

TIR_Status dhcpDumpIpv4AddrList(const DhcpOption *option)
{
   size_t i;
   Ipv4Addr ipAddr;

   //Check option length
   if((option->length % sizeof(Ipv4Addr)) != 0)
      return Failure;

   //Parse the list of IPv4 addresses
   for(i = 0; i < option->length; i += sizeof(Ipv4Addr))
   {
      //Retrieve the current IPv4 address
      ipv4CopyAddr(&ipAddr, option->value + i);
      //Display current address
      printDebug("    %s\r\n", ipv4AddrToString(ipAddr, NULL));
   }

   //No error to report
   return Success;
}

TIR_Status dhcpDumpRawData(const DhcpOption *option)
{
   //Dump option contents
   printDebugArray("    ", option->value, option->length);

   //No error to report
   return Success;
}

