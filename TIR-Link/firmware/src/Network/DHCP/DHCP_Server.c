#include "DHCP_Server.h"

#include <stdlib.h>
#include <endian.h>

#include "DHCP_Debug.h"
#include "Common/Debug.h"
#include "InterLink/Interlink.h" 
#include "InterLink/Interlink_Forwarding.h"

#include "DHCP_MessageQueue.h"

Ipv4Addr DHCP_SERVER_IPv4_ADDRESS_MIN;
Ipv4Addr DHCP_SERVER_IPv4_ADDRESS_MAX;

Ipv4Addr DHCP_SERVER_NEXT_IPv4_ADDRESS;

DhcpServerBinding clientBinding[DHCP_SERVER_MAX_CLIENTS];
DhcpServerBinding pairServerClientBinding[DHCP_SERVER_MAX_CLIENTS];

systime_t last_maintenance_timestamp = 0;

static bool __dhcpServerRunning = false;

static void dhcpServerParseDiscover(const DhcpMessage *message, size_t length);
static void dhcpServerParseRequest(const DhcpMessage *message, size_t length);
static void dhcpServerParseDecline(const DhcpMessage *message, size_t length);
static void dhcpServerParseRelease(const DhcpMessage *message, size_t length);
static void dhcpServerParseInform(const DhcpMessage *message, size_t length);

TIR_Status dhcpServerStart(void) {
    const InterlinkHostRole selfLinkRole = get_SelfLinkRole();
    if(__dhcpServerRunning) return Failure;
    
    if(selfLinkRole == DHCP_SERVER1) {
        DHCP_SERVER_IPv4_ADDRESS_MIN = DHCP_SERVER1_IPv4_ADDRESS_MIN;
        DHCP_SERVER_IPv4_ADDRESS_MIN = DHCP_SERVER1_IPv4_ADDRESS_MAX;
        DHCP_SERVER_NEXT_IPv4_ADDRESS = DHCP_SERVER1_IPv4_ADDRESS_MIN;
    } else if(selfLinkRole == DHCP_SERVER2) {
        DHCP_SERVER_IPv4_ADDRESS_MIN = DHCP_SERVER2_IPv4_ADDRESS_MIN;
        DHCP_SERVER_IPv4_ADDRESS_MIN = DHCP_SERVER2_IPv4_ADDRESS_MAX;
        DHCP_SERVER_NEXT_IPv4_ADDRESS = DHCP_SERVER2_IPv4_ADDRESS_MIN;
    } else { return Failure; }
    
    #ifdef DHCP_SERVER_DEBUG_LEVEL0
    printDebug("DHCP server started\r\n");
    #endif
    
    __dhcpServerRunning = true;
    return Success;
}

bool dhcpServerRunning(void) {
    return __dhcpServerRunning;
}

TIR_Status dhcpServerProcessPkt(const EthFrame *ethFrame, const uint16_t frame_len) {
    if(!dhcpServerRunning()) return Failure;
//    if(betoh16(ethFrame->type) != ETH_TYPE_IPV4) {
//        #ifdef DHCP_SERVER_DEBUG_LEVEL1
//        printf("-> Non IPv4 internet layer package, ignoring!\n");
//        #endif
//        return Failure;
//    }

//    size_t ip_packet_length = frame_len - sizeof(EthFrame);
    Ipv4Header *ip_header = (Ipv4Header *) ethFrame->data;

//    if(ip_header->version != IPV4_VERSION) {
//        #ifdef DHCP_SERVER_DEBUG_LEVEL1
//        printf("-> Non IPv4 package, ignoring!\n");
//        #endif
//        return Failure;
//    } else if(ip_packet_length < sizeof(Ipv4Header)) {
//        #ifdef DHCP_SERVER_DEBUG_LEVEL1
//        printf("-> Invalid IPv4 packet length, ignoring!\n");
//        #endif
//        return Failure;
//    } else if(ip_header->headerLength < 5) { // lentgh is in multyply of words (32 bit)
//        #ifdef DHCP_SERVER_DEBUG_LEVEL1
//        printf("-> Invalid IPv4 header length (too short), ignoring!\n");
//        #endif
//        return Failure;
//    } else if(betoh16(ip_header->totalLength) < (ip_header->headerLength * 4)) {
//        #ifdef DHCP_SERVER_DEBUG_LEVEL1
//        printf("-> IPv4 invalid total length, ignoring!\n");
//        #endif
//        return Failure;
//    } else if(isFragmentedPacket(ip_header)) {
//        #ifdef DHCP_SERVER_DEBUG_LEVEL1
//        printf("-> Received fragmented IPv4 packet which is not supported, ignoring!\n");
//        #endif
//        return Failure;
//    } else if(ip_header->protocol != IPV4_PROTOCOL_UDP) {
//        #ifdef DHCP_SERVER_DEBUG_LEVEL1
//        printf("-> Received non UPD IPv4 packet, which is not supported, ignoring!\n");
//        #endif
//        return Failure;
//    }

    // (ip_header->headerLength * 4) is the length of IPv4 packet
    // again we multiply it by 4 (32 bit), as the length is given in word length
    UdpHeader *upd_header = (UdpHeader *) ((uint8_t*)ip_header + (ip_header->headerLength * 4));

//    if(betoh16(upd_header->destPort) != DHCP_SERVER_PORT || betoh16(upd_header->srcPort) != DHCP_CLIENT_PORT) {
//        #ifdef DHCP_SERVER_DEBUG_LEVEL1
//        printf("-> UDP dest. port %hu (expected %hu) and source port %hu (expected %hu), ignoring!\n",
//               betoh16(upd_header->destPort), (u_short)DHCP_SERVER_PORT,
//               betoh16(upd_header->srcPort), (u_short)DHCP_CLIENT_PORT);
//        #endif
//        return Failure;
//    }

    const DhcpMessage *dhcp_packet = (DhcpMessage *) ((uint8_t*)upd_header + UDP_HEADER_LENGTH);
    size_t dhcp_pkt_len = betoh16(upd_header->length) - UDP_HEADER_LENGTH;

    if(dhcp_packet->op != DHCP_OPCODE_BOOTREQUEST) {
        #ifdef DHCP_SERVER_DEBUG_LEVEL1
        printf("-> DHCP rerquest received, ignoring!\n");
        #endif
        return Failure;
    } else if(dhcp_packet->htype != DHCP_HARDWARE_TYPE_ETH) {
        #ifdef DHCP_SERVER_DEBUG_LEVEL1
        printf("-> DHCP hardware type non ethernet, ignoring!\n");
        #endif
        return Failure;
    } else if(dhcp_packet->hlen != sizeof(MacAddr)) {
        #ifdef DHCP_SERVER_DEBUG_LEVEL1
        printf("-> DHCP incorrect header length, ignoring!\n");
        #endif
        return Failure;
    } else if(dhcp_packet->magicCookie != HTONL(DHCP_MAGIC_COOKIE)) {
        #ifdef DHCP_SERVER_DEBUG_LEVEL1
        printf("-> DHCP incorrect magic cookie, ignoring!\n");
        #endif
        return Failure;
    }

    DhcpOption *dhcp_option = dhcpGetOption(dhcp_packet, dhcp_pkt_len, DHCP_OPT_DHCP_MESSAGE_TYPE);

    if(dhcp_option  == NULL || dhcp_option ->length != 1) {
        #ifdef DHCP_SERVER_DEBUG_LEVEL1
        printf("-> Failed to extract dhcp options, ignoring!\n");
        #endif
        return Failure;
    }

    switch(dhcp_option->value[0])
    {
    case DHCP_MSG_TYPE_DISCOVER:
       //Parse DHCPDISCOVER message
       dhcpServerParseDiscover(dhcp_packet, dhcp_pkt_len);
       break;
    case DHCP_MSG_TYPE_REQUEST:
       //Parse DHCPREQUEST message
       dhcpServerParseRequest(dhcp_packet, dhcp_pkt_len);
       break;
    case DHCP_MSG_TYPE_DECLINE:
       //Parse DHCPDECLINE message
       dhcpServerParseDecline(dhcp_packet, dhcp_pkt_len);
       break;
    case DHCP_MSG_TYPE_RELEASE:
       //Parse DHCPRELEASE message
       dhcpServerParseRelease(dhcp_packet, dhcp_pkt_len);
       break;
    case DHCP_MSG_TYPE_INFORM:
       //Parse DHCPINFORM message
       dhcpServerParseInform(dhcp_packet, dhcp_pkt_len);
       break;
    default:
       //Silently drop incoming message
       break;
    }

    
    #ifdef DHCP_SERVER_DEBUG_LEVEL1
    printf("------NEW PACKET------\n");

    ethDumpHeader(ethFrame);
    ipv4DumpHeader(ip_header);
    udpDumpHeader(upd_header);
    dhcpDumpMessage(dhcp_packet, dhcp_pkt_len);

    printf("------PACKET END------\n");
    #endif

    return Success;
}

void dhcpServerMaintanance(void) {   
    if(!dhcpServerRunning()) return;
    
    //Get current time
    systime_t time = get_SysTime_ms();
    
    // We check if its time to perform maintenance
    if(timeCompare(time , last_maintenance_timestamp) < DHCP_SERVER_MAINTENANCE_PERIOD) return;
    last_maintenance_timestamp = time;
    #ifdef DHCP_SERVER_DEBUG_LEVEL1
    printDebug("Performing maintenance\r\n");
    #endif

    systime_t leaseTime = DHCP_SERVER_DEFAULT_LEASE_TIME;
    DhcpServerBinding *binding;

    //Loop through the list of bindings
    for(uint16_t i = 0; i < DHCP_SERVER_MAX_CLIENTS; i++)
    {
        //Point to the current binding
        binding = &clientBinding[i];

        //Valid binding?
        if(!macCompAddr(&binding->macAddr, &MAC_UNSPECIFIED_ADDR))
        {
            //Check whether the network address has been committed
            if(binding->validLease)
            {
                //Check whether the lease has expired
                if(timeCompare(time, binding->timestamp + leaseTime) >= 0)
                {
                    #ifdef DHCP_SERVER_DEBUG_LEVEL0
                    printDebug("Removed IP on maintenance: %s\r\n", ipv4AddrToString(binding->ipAddr, NULL));
                    #endif

                    // We also notify the pair server about removal
                    send_RemoveForwardingEntry(binding);
                    
                    //The address lease is not more valid
                    binding->validLease = 0;
                }
            }
        }
    }
}

void dhcpServerParseDiscover(const DhcpMessage *message, size_t length)
{
    TIR_Status status;
    Ipv4Addr requestedIpAddr;
    DhcpOption *option;
    DhcpServerBinding *binding;

    //Retrieve Server Identifier option
    option = dhcpGetOption(message, length, DHCP_OPT_SERVER_ID);

    // Here we check if the option give the server ip, does it match our id (it is statically assigned)
    // Option found?
    if(option != NULL && option->length == 4)
    {
       //Unexpected server identifier?
       if(!ipv4CompAddr(option->value, &HOST_IPv4_ADDRESS))
          return;
    }

    //Retrieve Requested IP Address option
    option = dhcpGetOption(message, length, DHCP_OPT_REQUESTED_IP_ADDR);

    //The client may include the 'requested IP address' option to suggest
    //that a particular IP address be assigned
    if(option != NULL && option->length == 4)
       ipv4CopyAddr(&requestedIpAddr, option->value);
    else
       requestedIpAddr = IPV4_UNSPECIFIED_ADDR;

    //Search the list for a matching binding
    binding = dhcpServerFindBindingByMacAddr(&message->chaddr);

    //Matching binding found?
    if(binding != NULL)
    {
       //Different IP address than cached?
       if(requestedIpAddr != binding->ipAddr)
       {
          //Ensure the IP address is in the server's pool of available addresses
          if(betoh32(requestedIpAddr) >= betoh32(DHCP_SERVER_IPv4_ADDRESS_MIN) &&
             betoh32(requestedIpAddr) <= betoh32(DHCP_SERVER_IPv4_ADDRESS_MAX))
          {
             //Make sure the IP address is not already allocated
             if(!dhcpServerFindBindingByIpAddr(requestedIpAddr))
             {
                //Record IP address
                binding->ipAddr = requestedIpAddr;
                //Get current time
                binding->timestamp = get_SysTime_ms();
             }
          }
       }

       //Successful processing
       status = Success;
    }
    else
    {
       //Create a new binding
       binding = dhcpServerCreateBinding();

       //Binding successfully created
       if(binding != NULL)
       {
          //Ensure the IP address is in the server's pool of available addresses
          if(betoh32(requestedIpAddr) >= betoh32(DHCP_SERVER_IPv4_ADDRESS_MIN) &&
             betoh32(requestedIpAddr) <= betoh32(DHCP_SERVER_IPv4_ADDRESS_MAX))
          {
             //Make sure the IP address is not already allocated
             if(!dhcpServerFindBindingByIpAddr(requestedIpAddr))
             {
                //Record IP address
                binding->ipAddr = requestedIpAddr;
                //Successful processing
                status = Success;
             }
             else
             {
                //Retrieve the next available IP address from the pool of addresses
                status = dhcpServerGetNextIpAddr(&binding->ipAddr);
             }
          }
          else
          {
             //Retrieve the next available IP address from the pool of addresses
             status = dhcpServerGetNextIpAddr(&binding->ipAddr);
          }

          //Check status code
          if(!status)
          {
             //Record MAC address
             binding->macAddr = message->chaddr;
             //Get current time
             binding->timestamp = get_SysTime_ms();
          }
       }
       else
       {
          //Failed to create a new binding
          status = Failure;
       }
    }

    //Check status code
    if(!status)
    {   
        #ifdef DHCP_SERVER_DEBUG_LEVEL0
        printDebug("Offered IP on Discover: %s\r\n", ipv4AddrToString(binding->ipAddr, NULL));
        #endif
        
       //The server responds with a DHCPOFFER message that includes an
       //available network address in the 'yiaddr' field (and other
       //configuration parameters in DHCP options)
       dhcpServerSendReply(DHCP_MSG_TYPE_OFFER, binding->ipAddr, message, length);
    }
}

void dhcpServerParseRequest(const DhcpMessage *message, size_t length)
{
   Ipv4Addr clientIpAddr;
   DhcpOption *option;
   DhcpServerBinding *binding;

   //Retrieve Server Identifier option
   option = dhcpGetOption(message, length, DHCP_OPT_SERVER_ID);

   //Option found?
   if(option != NULL && option->length == 4)
   {
      //Unexpected server identifier?
      if(!ipv4CompAddr(option->value, &HOST_IPv4_ADDRESS))
         return;
   }

   //Check the 'ciaddr' field
   if(message->ciaddr != IPV4_UNSPECIFIED_ADDR)
   {
      //Save client's network address
      clientIpAddr = message->ciaddr;
   }
   else
   {
      //Retrieve Requested IP Address option
      option = dhcpGetOption(message, length, DHCP_OPT_REQUESTED_IP_ADDR);

      //Option found?
      if(option != NULL && option->length == 4)
         ipv4CopyAddr(&clientIpAddr, option->value);
      else
         clientIpAddr = IPV4_UNSPECIFIED_ADDR;
   }

   //Valid client IP address?
   if(clientIpAddr != IPV4_UNSPECIFIED_ADDR)
   {
      //Search the list for a matching binding
      binding = dhcpServerFindBindingByMacAddr(&message->chaddr);

      //Matching binding found?
      if(binding != NULL)
      {
         //Make sure the client's IP address is valid
         if(clientIpAddr == binding->ipAddr)
         {
            //Commit network address
            binding->validLease = 1;
            //Save lease start time
            binding->timestamp = get_SysTime_ms();
            
            #ifdef DHCP_SERVER_DEBUG_LEVEL0
            printDebug("Acknowledged requested IP: %s\r\n", ipv4AddrToString(binding->ipAddr, NULL));
            #endif
            
            // As we send an acknowledgment we also notify the pair server about addition
            send_AddForwardingEntry(binding);
            
            //The server responds with a DHCPACK message containing the
            //configuration parameters for the requesting client
            dhcpServerSendReply(DHCP_MSG_TYPE_ACK, binding->ipAddr, message, length);

            //Exit immediately
            return;
         }
      }
      else
      {
         //Ensure the IP address is in the server's pool of available addresses
         if(betoh32(clientIpAddr) >= betoh32(DHCP_SERVER_IPv4_ADDRESS_MIN) &&
            betoh32(clientIpAddr) <= betoh32(DHCP_SERVER_IPv4_ADDRESS_MAX))
         {
            //Make sure the IP address is not already allocated
            if(!dhcpServerFindBindingByIpAddr(clientIpAddr))
            {
               //Create a new binding
               binding = dhcpServerCreateBinding();

               //Binding successfully created
               if(binding != NULL)
               {
                  //Record MAC address
                  binding->macAddr = message->chaddr;
                  //Record IP address
                  binding->ipAddr = clientIpAddr;
                  //Commit network address
                  binding->validLease = 1;
                  //Get current time
                  binding->timestamp = get_SysTime_ms();
                  
                  #ifdef DHCP_SERVER_DEBUG_LEVEL0
                  printDebug("Assigned new IP from request:  %s\r\n", ipv4AddrToString(binding->ipAddr, NULL));
                  #endif

                    // As we assign a new IP address we also notify the pair server about addition
                    send_AddForwardingEntry(binding);
                    
                  //The server responds with a DHCPACK message containing the
                  //configuration parameters for the requesting client
                  dhcpServerSendReply(DHCP_MSG_TYPE_ACK, binding->ipAddr, message, length);

                  //Exit immediately
                  return;
               }
            }
         }
      }
   }

    #ifdef DHCP_SERVER_DEBUG_LEVEL0
    printDebug("Failed to satisfy request.\r\n");
    #endif
   //If the server is unable to satisfy the DHCPREQUEST message, the
   //server should respond with a DHCPNAK message
   dhcpServerSendReply(DHCP_MSG_TYPE_NAK, IPV4_UNSPECIFIED_ADDR, message, length);
}

void dhcpServerParseDecline(const DhcpMessage *message, size_t length)
{
   DhcpOption *option;
   DhcpServerBinding *binding;
   Ipv4Addr requestedIpAddr;

   //Retrieve Requested IP Address option
   option = dhcpGetOption(message, length, DHCP_OPT_REQUESTED_IP_ADDR);

   //Option found?
   if(option != NULL && option->length == 4)
   {
      //Copy the requested IP address
      ipv4CopyAddr(&requestedIpAddr, option->value);

      //Search the list for a matching binding
      binding = dhcpServerFindBindingByMacAddr(&message->chaddr);

      //Matching binding found?
      if(binding != NULL)
      {
         //Check the IP address against the requested IP address
         if(binding->ipAddr == requestedIpAddr)
         {
            #ifdef DHCP_SERVER_DEBUG_LEVEL0
            printDebug("Removed binding from decline for IP:  %s\r\n", ipv4AddrToString(binding->ipAddr, NULL));
            #endif

            // We also notify the pair server about removal
            send_RemoveForwardingEntry(binding);
             
            //Remote the binding from the list
            memset(binding, 0, sizeof(DhcpServerBinding));
         }
      }
   }
}

void dhcpServerParseRelease(const DhcpMessage *message, size_t length)
{
    (void)length;
   DhcpServerBinding *binding;

   //Search the list for a matching binding
   binding = dhcpServerFindBindingByMacAddr(&message->chaddr);

   //Matching binding found?
   if(binding != NULL)
   {
      //Check the IP address against the client IP address
      if(binding->ipAddr == message->ciaddr)
      {
        #ifdef DHCP_SERVER_DEBUG_LEVEL0
        printDebug("Released:  %s\r\n", ipv4AddrToString(binding->ipAddr, NULL));
        #endif
          
        // We also notify the pair server about removal
        send_RemoveForwardingEntry(binding);
                    
         //Release the network address and cancel remaining lease
         binding->validLease = 0;
      }
   }
}

void dhcpServerParseInform(const DhcpMessage *message, size_t length)
{
   //Make sure the client IP address is valid
   if(message->ciaddr != IPV4_UNSPECIFIED_ADDR)
   {
      //Servers receiving a DHCPINFORM message construct a DHCPACK message
      //with any local configuration parameters appropriate for the client
      dhcpServerSendReply(DHCP_MSG_TYPE_ACK, IPV4_UNSPECIFIED_ADDR, message, length);
   }
}

DhcpServerBinding *dhcpServerFindBindingByMacAddr(const MacAddr *macAddr)
{
   DhcpServerBinding *binding;

   //Loop through the list of bindings
   for(uint16_t i = 0; i < DHCP_SERVER_MAX_CLIENTS; i++)
   {
      //Point to the current binding
      binding = &clientBinding[i];

      //Valid binding?
      if(!macCompAddr(&binding->macAddr, &MAC_UNSPECIFIED_ADDR))
      {
         //Check whether the current binding matches the specified MAC address
         if(macCompAddr(&binding->macAddr, macAddr))
         {
            //Return the pointer to the corresponding binding
            return binding;
         }
      }
   }

   //No matching binding...
   return NULL;
}

DhcpServerBinding *dhcpServerFindBindingByIpAddr(Ipv4Addr ipAddr)
{
   DhcpServerBinding *binding;

   //Loop through the list of bindings
   for(uint16_t i = 0; i < DHCP_SERVER_MAX_CLIENTS; i++)
   {
      //Point to the current binding
      binding = &clientBinding[i];

      //Valid binding?
      if(!macCompAddr(&binding->macAddr, &MAC_UNSPECIFIED_ADDR))
      {
         //Check whether the current binding matches the specified IP address
         if(binding->ipAddr == ipAddr)
         {
            //Return the pointer to the corresponding binding
            return binding;
         }
      }
   }

   //No matching binding...
   return NULL;
}

DhcpServerBinding *dhcpServerCreateBinding()
{
   systime_t time;
   DhcpServerBinding *binding;
   DhcpServerBinding *oldestBinding;

   //Get current time
   time = get_SysTime_ms();

   //Keep track of the oldest binding
   oldestBinding = NULL;

   //Loop through the list of bindings
   for(uint16_t i = 0; i < DHCP_SERVER_MAX_CLIENTS; i++)
   {
      //Point to the current binding
      binding = &clientBinding[i];

      //Check whether the binding is available
      if(macCompAddr(&binding->macAddr, &MAC_UNSPECIFIED_ADDR))
      {
         //Erase contents
         memset(binding, 0, sizeof(DhcpServerBinding));
         //Return a pointer to the newly created binding
         return binding;
      }
      else
      {
         //Bindings that have been committed cannot be removed
         if(!binding->validLease)
         {
            //Keep track of the oldest binding in the list
            if(oldestBinding == NULL)
            {
               oldestBinding = binding;
            }
            else if((time - binding->timestamp) > (time - oldestBinding->timestamp))
            {
               oldestBinding = binding;
            }
         }
      }
   }

   //Any binding available in the list?
   if(oldestBinding != NULL)
   {
      //Erase contents
      memset(oldestBinding, 0, sizeof(DhcpServerBinding));
   }

   //Return a pointer to the oldest binding
   return oldestBinding;
}

TIR_Status dhcpServerGetNextIpAddr(Ipv4Addr *ipAddr)
{
    DhcpServerBinding *binding;

    //Search the pool for any available IP address
    for(uint16_t i = 0; i < DHCP_SERVER_MAX_CLIENTS; i++)
    {
       //Check whether the current IP address is already allocated
       binding = dhcpServerFindBindingByIpAddr(DHCP_SERVER_NEXT_IPv4_ADDRESS);

       //If the IP address is available, then it can be assigned to a new client
       if(binding == NULL)
          *ipAddr = DHCP_SERVER_NEXT_IPv4_ADDRESS;

       //Compute the next IP address that will be assigned by the DHCP server
       if(betoh32(DHCP_SERVER_NEXT_IPv4_ADDRESS) >= betoh32(DHCP_SERVER_IPv4_ADDRESS_MAX))
       {
          //Wrap around to the beginning of the pool
          DHCP_SERVER_NEXT_IPv4_ADDRESS = DHCP_SERVER_IPv4_ADDRESS_MIN;
       }
       else
       {
          //Increment IP address
          DHCP_SERVER_NEXT_IPv4_ADDRESS = htobe32(betoh32(DHCP_SERVER_NEXT_IPv4_ADDRESS) + 1);
       }

       //If the IP address is available, we are done
       if(binding == NULL)
          return Success;
    }

    //No available addresses in the pool...
    return Failure;
}

TIR_Status dhcpServerSendReply(uint8_t type, Ipv4Addr yourIpAddr, const DhcpMessage *request, size_t requestLen)
{
    (void)requestLen;

    TIR_Status status = Success;
    size_t dhcp_packet_length;

    Ipv4Addr srcIpAddr;
    Ipv4Addr destIpAddr;
    uint16_t destPort;

//    uint32_t    frame_length    = ETH_HEADER_SIZE + IPV4_MIN_HEADER_LENGTH + UDP_HEADER_LENGTH + DHCP_MAX_MSG_SIZE;
//    uint8_t     *frame          = (uint8_t*)reserveNew_DHCPMessageQueue();
    EthFrame    *eth_header     = (EthFrame *) reserveNew_DHCPMessageQueue();
    Ipv4Header  *ip_header      = (Ipv4Header *) ((uint8_t*)eth_header + ETH_HEADER_SIZE);
    UdpHeader   *upd_header     = (UdpHeader *) ((uint8_t*)ip_header + IPV4_MIN_HEADER_LENGTH);
    DhcpMessage *dhcp_packet    = (DhcpMessage *) ((uint8_t*)upd_header + UDP_HEADER_LENGTH);

    // Cleaning the frame buffer
    memset(eth_header, 0, DHCP_MESSAGE_LENGTH);

    //Format DHCP reply message
    dhcp_packet->op = DHCP_OPCODE_BOOTREPLY;
    dhcp_packet->htype = DHCP_HARDWARE_TYPE_ETH;
    dhcp_packet->hlen = sizeof(MacAddr);
    dhcp_packet->xid = request->xid;
    dhcp_packet->secs = 0;
    dhcp_packet->flags = request->flags;
    dhcp_packet->ciaddr = IPV4_UNSPECIFIED_ADDR;
    dhcp_packet->yiaddr = yourIpAddr;
    dhcp_packet->siaddr = IPV4_UNSPECIFIED_ADDR;
    dhcp_packet->giaddr = request->giaddr;
    dhcp_packet->chaddr = request->chaddr;

    //Write magic cookie before setting any option
    dhcp_packet->magicCookie = HTONL(DHCP_MAGIC_COOKIE);
    //Properly terminate options field
    dhcp_packet->options[0] = DHCP_OPT_END;

    //Total length of the DHCP message
    dhcp_packet_length = sizeof(DhcpMessage) + sizeof(uint8_t);

    //Add DHCP Message Type option
    dhcpAddOption(dhcp_packet, &dhcp_packet_length, DHCP_OPT_DHCP_MESSAGE_TYPE, &type, sizeof(type));

    //Add Server Identifier option
    dhcpAddOption(dhcp_packet, &dhcp_packet_length, DHCP_OPT_SERVER_ID, &HOST_IPv4_ADDRESS, sizeof(Ipv4Addr));

    //DHCPOFFER or DHCPACK message?
    if(type == DHCP_MSG_TYPE_OFFER || type == DHCP_MSG_TYPE_ACK)
    {
       //Convert the lease time to network byte order
       uint32_t value = htobe32(DHCP_SERVER_DEFAULT_LEASE_TIME/1000);

       //When responding to a DHCPINFORM message, the server must not
       //send a lease expiration time to the client
       if(yourIpAddr != IPV4_UNSPECIFIED_ADDR)
       {
          //Add IP Address Lease Time option
          dhcpAddOption(dhcp_packet, &dhcp_packet_length, DHCP_OPT_IP_ADDRESS_LEASE_TIME, &value, sizeof(value));
       }

       //Add Subnet Mask option
       if(HOST_IPv4_SUBNET_MUSK != IPV4_UNSPECIFIED_ADDR)
       {
          dhcpAddOption(dhcp_packet, &dhcp_packet_length, DHCP_OPT_SUBNET_MASK, &HOST_IPv4_SUBNET_MUSK, sizeof(Ipv4Addr));
       }

        //Add Router option (currently unsupported)
        //if(context->settings.defaultGateway != IPV4_UNSPECIFIED_ADDR)
        //{
        //  dhcpAddOption(reply, &replyLen, DHCP_OPT_ROUTER,
        //     &context->settings.defaultGateway, sizeof(Ipv4Addr));
        //}

        //Retrieve the number of DNS servers (currently unsupported)
        //for(n = 0; n < DHCP_SERVER_MAX_DNS_SERVERS; n++)
        //{
        //  //Check whether the current DNS server is valid
        //  if(context->settings.dnsServer[n] == IPV4_UNSPECIFIED_ADDR)
        //     break;
        //}

        //Add DNS Server option
        //if(n > 0)
        //{
        //  dhcpAddOption(reply, &replyLen, DHCP_OPT_DNS_SERVER,
        //     context->settings.dnsServer, n * sizeof(Ipv4Addr));
        //}
    }

    srcIpAddr = HOST_IPv4_ADDRESS;


    //Check whether the 'giaddr' field is non-zero
    if(request->giaddr != IPV4_UNSPECIFIED_ADDR)
    {
        //If the 'giaddr' field in a DHCP message from a client is non-zero,
        //the server sends any return messages to the 'DHCP server' port
        destPort = DHCP_SERVER_PORT;

        //The DHCP message is sent to the BOOTP relay agent whose address
        //appears in 'giaddr'
        destIpAddr = request->giaddr;
    }
    else
    {
        //If the 'giaddr' field in a DHCP message from a client is zero,
        //the server sends any return messages to the 'DHCP client'
        destPort = DHCP_CLIENT_PORT;

        //DHCPOFFER or DHCPACK message?
        if(type == DHCP_MSG_TYPE_OFFER || type == DHCP_MSG_TYPE_ACK)
        {
            //Check whether the 'giaddr' field is non-zero
            if(request->ciaddr != IPV4_UNSPECIFIED_ADDR)
            {
                //If the 'giaddr' field is zero and the 'ciaddr' field is nonzero,
                //then the server unicasts DHCPOFFER and DHCPACK messages to the
                //address in 'ciaddr'
                destIpAddr = request->ciaddr;
            }
            else
            {
            //Check whether the broadcast bit is set
            if(betoh16(request->flags) & DHCP_FLAG_BROADCAST)
            {
               //If 'giaddr' is zero and 'ciaddr' is zero, and the broadcast bit is
               //set, then the server broadcasts DHCPOFFER and DHCPACK messages
               destIpAddr = IPV4_BROADCAST_ADDR;
            }
            else
            {
               //If 'giaddr' is zero and 'ciaddr' is zero, and the broadcast bit is
               //not set, then the server unicasts DHCPOFFER and DHCPACK messages
               //to the client's hardware address and 'yiaddr' address
               destIpAddr = IPV4_BROADCAST_ADDR;
            }
            }
        }
        //DHCPNAK message?
        else
        {
         //In all cases, when 'giaddr' is zero, the server broadcasts any
         //DHCPNAK messages
         destIpAddr = IPV4_BROADCAST_ADDR;
        }
    }

    // Setting up the UDP header
    upd_header->srcPort = htobe16(DHCP_SERVER_PORT);
    upd_header->destPort = htobe16(destPort);
    upd_header->length = htobe16(UDP_HEADER_LENGTH + DHCP_MAX_MSG_SIZE);
    upd_header->checksum = 0; // For IPv4 checksum is not mandatory

    // Setting up the IP header
    //ip_header
    ip_header->version = IPV4_VERSION;
    ip_header->headerLength = 5;
    ip_header->typeOfService = 0;
    ip_header->totalLength = htobe16(IPV4_MIN_HEADER_LENGTH + UDP_HEADER_LENGTH + DHCP_MAX_MSG_SIZE);
    ip_header->identification = htobe16(IPv4_IDENTIFICATION++);
    ip_header->fragmentOffset = htobe16(0);
    ip_header->timeToLive = IPV4_DEFAULT_TTL;
    ip_header->protocol = IPV4_PROTOCOL_UDP;
    ip_header->headerChecksum = 0;
    ip_header->srcAddr = srcIpAddr;
    ip_header->destAddr = destIpAddr;

    ip_header->headerChecksum = ipCalcChecksum(ip_header, IPV4_MIN_HEADER_LENGTH);

    eth_header->srcAddr = HOST_MAC_ADDR;
    eth_header->destAddr = MAC_BROADCAST_ADDR;
    eth_header->type = htobe16(ETH_TYPE_IPV4);
    
    #ifdef DHCP_SERVER_DEBUG_LEVEL1
    printf("------SENT PACKET------\n");

    ethDumpHeader(eth_header);
    ipv4DumpHeader(ip_header);
    udpDumpHeader(upd_header);
    dhcpDumpMessage(dhcp_packet, DHCP_MAX_MSG_SIZE);

    printf("------PACKET END------\n");
    #endif
    
//    incremetTailIndex_TxFIFO();
    appendReserved_DHCPMessageQueue();
    
    return status;
}