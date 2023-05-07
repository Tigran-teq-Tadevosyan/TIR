#include "DHCP_Server.h"

#include <stdlib.h>

#include "../Network.h"
#include "DHCP_Debug.h"

const Ipv4Addr DHCP_SERVER_IPv4_ADDRESS_MIN;
const Ipv4Addr DHCP_SERVER_IPv4_ADDRESS_MAX;

const Ipv4Addr DHCP_SERVER_NEXT_IPv4_ADDRESS;

DhcpServerBinding clientBinding[DHCP_SERVER_MAX_CLIENTS];

void DHCP_server_packet_callback(u_char *args, const struct pcap_pkthdr *packet_hdr, const u_char *frame);

TIR_Status DHCP_server_start(const char* device_name) {

    ipv4StringToAddr("100.100.0.1", &DHCP_SERVER_IPv4_ADDRESS);
    ipv4StringToAddr("255.255.255.0", &DHCP_SERVER_SUBNET_MUSK);
 
//    DHCP_SERVER_IPv4_ADDRESS_MIN = IPV4_UNSPECIFIED_ADDR;
//    DHCP_SERVER_IPv4_ADDRESS_MAX = IPV4_UNSPECIFIED_ADDR;

    ipv4StringToAddr("100.100.0.2", &DHCP_SERVER_IPv4_ADDRESS_MIN);
    ipv4StringToAddr("100.100.0.250", &DHCP_SERVER_IPv4_ADDRESS_MAX);

    ipv4StringToAddr("100.100.0.2", &DHCP_SERVER_NEXT_IPv4_ADDRESS);

    char errbuf[PCAP_ERRBUF_SIZE];
    handle = pcap_open_live(device_name, BUFSIZ, 1, 1000, errbuf);

    if (handle == NULL) {
        fprintf(stderr, "Faile to start DHCP server on device %s: %s\n", device_name, errbuf);
        return Failure;
    } else if(pcap_datalink(handle) != DLT_EN10MB) {
        fprintf(stderr, "Faile to start DHCP server. Unsupported data link. Only Ethernet is supported.\n");
        return Failure;
    }

    initClock();

    printf("Starting the DHCP server!\n");
    pcap_loop(handle, -1, DHCP_server_packet_callback, NULL);

    pcap_close(handle);

    return Success;
}

void DHCP_server_packet_callback(u_char *args, const struct pcap_pkthdr *packet_hdr, const u_char *frame) {
    (void)args; // We do not pass context in the loop intialization so "args" is always NULL

    if(packet_hdr->caplen != packet_hdr->len) {
        printf("-> Packat read %u of %u bytes, ignoring!\n", packet_hdr->caplen, packet_hdr->len);
        return;
    }

    if(packet_hdr->len < sizeof(EthFrame))
    {
        printf("-> Invalid ethernet header (too short), ignoring!\n");
        return;
    }

    EthFrame *eth_header = (EthFrame *) frame; // frame is the raw ethernet frame in out case

    if(ntohs(eth_header->type) != ETH_TYPE_IPV4) {
        printf("-> Non IPv4 internet layer package, ignoring!\n");
        return;
    }

    size_t ip_packet_length = packet_hdr->len - sizeof(EthFrame);
    Ipv4Header *ip_header = (Ipv4Header *) eth_header->data;

    if(ip_header->version != IPV4_VERSION) {
        printf("-> Non IPv4 package, ignoring!\n");
        return;
    } else if(ip_packet_length < sizeof(Ipv4Header)) {
        printf("-> Invalid IPv4 packet length, ignoring!\n");
        return;
    } else if(ip_header->headerLength < 5) { // lentgh is in multyply of words (32 bit)
       printf("-> Invalid IPv4 header length (too short), ignoring!\n");
       return;
    } else if(ntohs(ip_header->totalLength) < (ip_header->headerLength * 4)) {
        printf("-> IPv4 invalid total length, ignoring!\n");
        return;
    } else if(isFragmentedPacket(ip_header)) {
        printf("-> Received fragmented IPv4 packet which is not supported, ignoring!\n");
        return;
    } else if(ip_header->protocol != IPV4_PROTOCOL_UDP) {
        printf("-> Received non UPD IPv4 packet, which is not supported, ignoring!\n");
        return;
    }

    // (ip_header->headerLength * 4) is the length of IPv4 packet
    // again we multipy it by 4 (32 bit), as the length is given in word length
    UdpHeader *upd_header = (UdpHeader *) ((uint8_t*)ip_header + (ip_header->headerLength * 4));

    if(ntohs(upd_header->destPort) != DHCP_SERVER_PORT || ntohs(upd_header->srcPort) != DHCP_CLIENT_PORT) {
        printf("-> UDP dest. port %hu (expected %hu) and source port %hu (expected %hu), ignoring!\n",
               ntohs(upd_header->destPort), (u_short)DHCP_SERVER_PORT,
               ntohs(upd_header->srcPort), (u_short)DHCP_CLIENT_PORT);
        return;
    }

    const DhcpMessage *dhcp_packet = (DhcpMessage *) ((uint8_t*)upd_header + UDP_HEADER_LENGTH);
    size_t dhcp_pkt_len = ntohs(upd_header->length) - UDP_HEADER_LENGTH;

    if(dhcp_packet->op != DHCP_OPCODE_BOOTREQUEST) {
        printf("-> DHCP rerquest received, ignoring!\n");
        return;
    } else if(dhcp_packet->htype != DHCP_HARDWARE_TYPE_ETH) {
        printf("-> DHCP hardware type non ethernet, ignoring!\n");
        return;
    } else if(dhcp_packet->hlen != sizeof(MacAddr)) {
        printf("-> DHCP incorrect header length, ignoring!\n");
        return;
    } else if(dhcp_packet->magicCookie != HTONL(DHCP_MAGIC_COOKIE)) {
        printf("-> DHCP incorrect magic cookie, ignoring!\n");
        return;
    }

    DhcpOption *dhcp_option = dhcpGetOption(dhcp_packet, dhcp_pkt_len, DHCP_OPT_DHCP_MESSAGE_TYPE);

    if(dhcp_option  == NULL || dhcp_option ->length != 1) {
        printf("-> Failed to extract dhcp options, ignoring!\n");
        return;
    }

    dhcpServerTick();

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

    printf("------NEW PACKET------\n");

    ethDumpHeader(eth_header);
    ipv4DumpHeader(ip_header);
    udpDumpHeader(upd_header);
    dhcpDumpMessage(dhcp_packet, dhcp_pkt_len);

    printf("------PACKET END------\n");
}

void dhcpServerTick()
{
   uint_t i;
   systime_t time;
   systime_t leaseTime = DHCP_SERVER_DEFAULT_LEASE_TIME;
   DhcpServerBinding *binding;

   //Get current time
   time = osGetSystemTime();

   //Loop through the list of bindings
   for(i = 0; i < DHCP_SERVER_MAX_CLIENTS; i++)
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
//    uint_t i;
    Ipv4Addr requestedIpAddr;
    DhcpOption *option;
    DhcpServerBinding *binding;

    //Index of the IP address assigned to the DHCP server
//    i = 0;

    //Retrieve Server Identifier option
    option = dhcpGetOption(message, length, DHCP_OPT_SERVER_ID);

    // Here we check if the option give the server ip, does it match our id (it is statically assigned)
    // Option found?
    if(option != NULL && option->length == 4)
    {
       //Unexpected server identifier?
       if(!ipv4CompAddr(option->value, &DHCP_SERVER_IPv4_ADDRESS))
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
          if(ntohl(requestedIpAddr) >= ntohl(DHCP_SERVER_IPv4_ADDRESS_MIN) &&
             ntohl(requestedIpAddr) <= ntohl(DHCP_SERVER_IPv4_ADDRESS_MAX))
          {
             //Make sure the IP address is not already allocated
             if(!dhcpServerFindBindingByIpAddr(requestedIpAddr))
             {
                //Record IP address
                binding->ipAddr = requestedIpAddr;
                //Get current time
                binding->timestamp = osGetSystemTime();
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
          if(ntohl(requestedIpAddr) >= ntohl(DHCP_SERVER_IPv4_ADDRESS_MIN) &&
             ntohl(requestedIpAddr) <= ntohl(DHCP_SERVER_IPv4_ADDRESS_MAX))
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
             binding->timestamp = osGetSystemTime();
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
      if(!ipv4CompAddr(option->value, &DHCP_SERVER_IPv4_ADDRESS))
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
            binding->timestamp = osGetSystemTime();

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
         if(ntohl(clientIpAddr) >= ntohl(DHCP_SERVER_IPv4_ADDRESS_MIN) &&
            ntohl(clientIpAddr) <= ntohl(DHCP_SERVER_IPv4_ADDRESS_MAX))
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
                  binding->timestamp = osGetSystemTime();

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
   uint_t i;
   DhcpServerBinding *binding;

   //Loop through the list of bindings
   for(i = 0; i < DHCP_SERVER_MAX_CLIENTS; i++)
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
   uint_t i;
   DhcpServerBinding *binding;

   //Loop through the list of bindings
   for(i = 0; i < DHCP_SERVER_MAX_CLIENTS; i++)
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
   uint_t i;
   systime_t time;
   DhcpServerBinding *binding;
   DhcpServerBinding *oldestBinding;

   //Get current time
   time = osGetSystemTime();

   //Keep track of the oldest binding
   oldestBinding = NULL;

   //Loop through the list of bindings
   for(i = 0; i < DHCP_SERVER_MAX_CLIENTS; i++)
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
    uint_t i;
    DhcpServerBinding *binding;

    //Search the pool for any available IP address
    for(i = 0; i < DHCP_SERVER_MAX_CLIENTS; i++)
    {
       //Check whether the current IP address is already allocated
       binding = dhcpServerFindBindingByIpAddr(DHCP_SERVER_NEXT_IPv4_ADDRESS);

       //If the IP address is available, then it can be assigned to a new client
       if(binding == NULL)
          *ipAddr = DHCP_SERVER_NEXT_IPv4_ADDRESS;

       //Compute the next IP address that will be assigned by the DHCP server
       if(ntohl(DHCP_SERVER_NEXT_IPv4_ADDRESS) >= ntohl(DHCP_SERVER_IPv4_ADDRESS_MAX))
       {
          //Wrap around to the beginning of the pool
          DHCP_SERVER_NEXT_IPv4_ADDRESS = DHCP_SERVER_IPv4_ADDRESS_MIN;
       }
       else
       {
          //Increment IP address
          DHCP_SERVER_NEXT_IPv4_ADDRESS = htonl(ntohl(DHCP_SERVER_NEXT_IPv4_ADDRESS) + 1);
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
//    uint_t i;
//    uint_t n;
//    uint32_t value;
//    size_t offset;
    size_t dhcp_packet_length;
//    NetBuffer *buffer;
//    NetInterface *interface;
//    DhcpMessage *reply;

    Ipv4Addr srcIpAddr;
    Ipv4Addr destIpAddr;
    uint16_t destPort;

    //Index of the IP address assigned to the DHCP server
//    i = context->settings.ipAddrIndex;


    uint32_t    frame_length    = ETH_HEADER_SIZE + IPV4_MIN_HEADER_LENGTH + UDP_HEADER_LENGTH + DHCP_MAX_MSG_SIZE;
    uint8_t     *frame          = malloc(frame_length);
    EthFrame   *eth_header     = (EthFrame *) frame;
    Ipv4Header  *ip_header      = (Ipv4Header *) ((uint8_t*)eth_header + ETH_HEADER_SIZE);
    UdpHeader   *upd_header     = (UdpHeader *) ((uint8_t*)ip_header + IPV4_MIN_HEADER_LENGTH);
    DhcpMessage *dhcp_packet    = (DhcpMessage *) ((uint8_t*)upd_header + UDP_HEADER_LENGTH);

    // Cleaning the frame buffer
    memset(frame, 0, frame_length);

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
    dhcpAddOption(dhcp_packet, &dhcp_packet_length, DHCP_OPT_SERVER_ID, &DHCP_SERVER_IPv4_ADDRESS, sizeof(Ipv4Addr));

    //DHCPOFFER or DHCPACK message?
    if(type == DHCP_MSG_TYPE_OFFER || type == DHCP_MSG_TYPE_ACK)
    {
       //Convert the lease time to network byte order
       uint32_t value = htonl(DHCP_SERVER_DEFAULT_LEASE_TIME/1000);

       //When responding to a DHCPINFORM message, the server must not
       //send a lease expiration time to the client
       if(yourIpAddr != IPV4_UNSPECIFIED_ADDR)
       {
          //Add IP Address Lease Time option
          dhcpAddOption(dhcp_packet, &dhcp_packet_length, DHCP_OPT_IP_ADDRESS_LEASE_TIME, &value, sizeof(value));
       }

       //Add Subnet Mask option
       if(DHCP_SERVER_SUBNET_MUSK != IPV4_UNSPECIFIED_ADDR)
       {
          dhcpAddOption(dhcp_packet, &dhcp_packet_length, DHCP_OPT_SUBNET_MASK, &DHCP_SERVER_SUBNET_MUSK, sizeof(Ipv4Addr));
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

    srcIpAddr = DHCP_SERVER_IPv4_ADDRESS;


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
            if(ntohs(request->flags) & DHCP_FLAG_BROADCAST)
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
    upd_header->srcPort = htons(DHCP_SERVER_PORT);
    upd_header->destPort = htons(destPort);
    upd_header->length = htons(UDP_HEADER_LENGTH + DHCP_MAX_MSG_SIZE);
    upd_header->checksum = 0; // For IPv4 checksum is not mandatory

    // Setting up the IP header
    //ip_header
    ip_header->version = IPV4_VERSION;
    ip_header->headerLength = 5;
    ip_header->typeOfService = 0;
    ip_header->totalLength = htons(IPV4_MIN_HEADER_LENGTH + UDP_HEADER_LENGTH + DHCP_MAX_MSG_SIZE);
    ip_header->identification = htons(IPv4_IDENTIFICATION++);
    ip_header->fragmentOffset = htons(0);
    ip_header->timeToLive = IPV4_DEFAULT_TTL;
    ip_header->protocol = IPV4_PROTOCOL_UDP;
    ip_header->headerChecksum = 0;
    ip_header->srcAddr = srcIpAddr;
    ip_header->destAddr = destIpAddr;

    ip_header->headerChecksum = ipCalcChecksum(ip_header, IPV4_MIN_HEADER_LENGTH);

    eth_header->srcAddr = MAC_SOURCE_ADDR;
    eth_header->destAddr = MAC_BROADCAST_ADDR;
    eth_header->type = htons(ETH_TYPE_IPV4);

    pcap_inject(handle, frame, frame_length);

    printf("------SENT PACKET------\n");

    ethDumpHeader(eth_header);
    ipv4DumpHeader(ip_header);
    udpDumpHeader(upd_header);
    dhcpDumpMessage(dhcp_packet, DHCP_MAX_MSG_SIZE);

    printf("------PACKET END------\n");

    free(frame);

    return status;
}
