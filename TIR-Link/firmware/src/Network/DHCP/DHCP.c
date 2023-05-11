#include "DHCP.h"

bool valid_dhcpPkt(EthFrame *frame, uint16_t frame_length) {
    if(betoh16(frame->type) != ETH_TYPE_IPV4) {
        return false;
    }

    size_t ip_packet_length = frame_length - sizeof(EthFrame);
    Ipv4Header *ip_header = (Ipv4Header *) frame->data;

    if(ip_header->version != IPV4_VERSION) {
        return false;
    } else if(ip_packet_length < sizeof(Ipv4Header)) {
        return false;
    } else if(ip_header->headerLength < 5) {
        return false;
    } else if(betoh16(ip_header->totalLength) < (ip_header->headerLength * 4)) {
        return false;
    } else if(isFragmentedPacket(ip_header)) {
        return false;
    } else if(ip_header->protocol != IPV4_PROTOCOL_UDP) {
        return false;
    }

    // (ip_header->headerLength * 4) is the length of IPv4 packet
    // again we multiply it by 4 (32 bit), as the length is given in word length
    UdpHeader *upd_header = (UdpHeader *) ((uint8_t*)ip_header + (ip_header->headerLength * 4));

    if(betoh16(upd_header->destPort) != DHCP_SERVER_PORT || betoh16(upd_header->srcPort) != DHCP_CLIENT_PORT) {
        return false;
    }
    
    return true;
}
DhcpOption *dhcpGetOption(const DhcpMessage *message, size_t length, uint8_t optionCode)
{
   size_t i;
   DhcpOption *option;

   //Make sure the DHCP header is valid
   if(length >= sizeof(DhcpMessage))
   {
      //Get the length of the options field
      length -= sizeof(DhcpMessage);

      //Loop through the list of options
      for(i = 0; i < length; i++)
      {
         //Point to the current option
         option = (DhcpOption *) (message->options + i);

         //Check option code
         if(option->code == DHCP_OPT_PAD)
         {
            //The pad option can be used to cause subsequent fields to align
            //on word boundaries
         }
         else if(option->code == DHCP_OPT_END)
         {
            //The end option marks the end of valid information in the vendor
            //field
            break;
         }
         else
         {
            //The option code is followed by a one-byte length field
            if((i + 1) >= length)
            {
               break;
            }

            //Check the length of the option
            if((i + sizeof(DhcpOption) + option->length) > length)
            {
               break;
            }

            //Matching option code?
            if(option->code == optionCode)
            {
               return option;
            }

            //Jump to the next option
            i += option->length + 1;
         }
      }
   }

   //The specified option code was not found
   return NULL;
}

TIR_Status dhcpAddOption(DhcpMessage *message, size_t *messageLen, uint8_t optionCode, const void *optionValue, size_t optionLen)
{
   size_t n;
   DhcpOption *option;

   //Check parameters
   if(message == NULL || messageLen == NULL)
      return Failure;

   //Check the length of the DHCP message
   if(*messageLen < (sizeof(DhcpMessage) + sizeof(uint8_t)))
      return Failure;

   //Check the length of the option
   if(optionLen > 0 && optionValue == NULL)
      return Failure;

   if(optionLen > UINT8_MAX)
      return Failure;

   //Ensure that the length of the resulting message will not exceed the
   //maximum DHCP message size
   if((*messageLen + sizeof(DhcpOption) + optionLen) > DHCP_MAX_MSG_SIZE)
      return Failure;

   //Retrieve the total length of the options field, excluding the end option
   n = *messageLen - sizeof(DhcpMessage) - sizeof(uint8_t);

   //Point to the buffer where to format the option
   option = (DhcpOption *) (message->options + n);

   //Set option code
   option->code = optionCode;
   //Set option length
   option->length = (uint8_t) optionLen;
   //Copy option value
   memcpy(option->value, optionValue, optionLen);

   //Determine the length of the options field
   n += sizeof(DhcpOption) + option->length;

   //Always terminate the options field with 255
   message->options[n++] = DHCP_OPT_END;

   //Update the length of the DHCPv6 message
   *messageLen = sizeof(DhcpMessage) + n;

   //Successful processing
   return Success;
}
