#include "IP.h"

#include <ctype.h>

uint16_t IPv4_IDENTIFICATION = 0;

char *ipv4AddrToString(Ipv4Addr ipAddr, char *str)
{
   uint8_t *p;
   static char buffer[16];

   //If the NULL pointer is given as parameter, then the internal buffer is used
   if(str == NULL)
      str = buffer;

   //Cast the address to byte array
   p = (uint8_t *) &ipAddr;
   //Format IPv4 address
   sprintf(str, "%" PRIu8 ".%" PRIu8 ".%" PRIu8 ".%" PRIu8 "", p[0], p[1], p[2], p[3]);

   //Return a pointer to the formatted string
   return str;
}

Status ipv4StringToAddr(const char *str, Ipv4Addr *ipAddr)
{
    Status status;
    int16_t i = 0;
    int16_t value = -1;

    //Parse input string
    while(1)
    {
       //Decimal digit found?
       if(isdigit((uint8_t) (*str)))
       {
          //First digit to be decoded?
          if(value < 0)
             value = 0;

          //Update the value of the current byte
          value = (value * 10) + (*str - '0');

          //The resulting value shall be in range 0 to 255
          if(value > 255)
          {
             //The conversion failed
             status = Failure;
             break;
          }
       }
       //Dot separator found?
       else if(*str == '.' && i < 4)
       {
          //Each dot must be preceded by a valid number
          if(value < 0)
          {
             //The conversion failed
             status = Failure;
             break;
          }

          //Save the current byte
          ((uint8_t *) ipAddr)[i++] = value;
          //Prepare to decode the next byte
          value = -1;
       }
       //End of string detected?
       else if(*str == '\0' && i == 3)
       {
          //The NULL character must be preceded by a valid number
          if(value < 0)
          {
             //The conversion failed
             status = Failure;
          }
          else
          {
             //Save the last byte of the IPv4 address
             ((uint8_t *) ipAddr)[i] = value;
             //The conversion succeeded
             status = Success;
          }

          //We are done
          break;
       }
       //Invalid character...
       else
       {
          //The conversion failed
          status = Failure;
          break;
       }

       //Point to the next character
       str++;
    }

    //Return status code
    return status;
 }

int isFragmentedPacket(Ipv4Header *packet) {
    return (ntohs(packet->fragmentOffset) & (IPV4_FLAG_MF | IPV4_OFFSET_MASK)) != 0;
}

uint16_t ipCalcChecksum(const void *data, size_t length)
{
   uint32_t temp;
   uint32_t checksum;
   const uint8_t *p;

   //Checksum preset value
   checksum = 0x0000;

   //Point to the data over which to calculate the IP checksum
   p = (const uint8_t *) data;

   //Pointer not aligned on a 16-bit boundary?
   if(((uintptr_t) p & 1) != 0)
   {
      if(length >= 1)
      {
#ifdef _CPU_BIG_ENDIAN
         //Update checksum value
         checksum += (uint32_t) *p;
#else
         //Update checksum value
         checksum += (uint32_t) *p << 8;
#endif
         //Restore the alignment on 16-bit boundaries
         p++;
         //Number of bytes left to process
         length--;
      }
   }

   //Pointer not aligned on a 32-bit boundary?
   if(((uintptr_t) p & 2) != 0)
   {
      if(length >= 2)
      {
         //Update checksum value
         checksum += (uint32_t) *((uint16_t *) p);

         //Restore the alignment on 32-bit boundaries
         p += 2;
         //Number of bytes left to process
         length -= 2;
      }
   }

   //Process the data 4 bytes at a time
   while(length >= 4)
   {
      //Update checksum value
      temp = checksum + *((uint32_t *) p);

      //Add carry bit, if any
      if(temp < checksum)
      {
        checksum = temp + 1;
      }
      else
      {
        checksum = temp;
      }

      //Point to the next 32-bit word
      p += 4;
      //Number of bytes left to process
      length -= 4;
   }

   //Fold 32-bit sum to 16 bits
   checksum = (checksum & 0xFFFF) + (checksum >> 16);

   //Add left-over 16-bit word, if any
   if(length >= 2)
   {
      //Update checksum value
      checksum += (uint32_t) *((uint16_t *) p);

      //Point to the next byte
      p += 2;
      //Number of bytes left to process
      length -= 2;
   }

   //Add left-over byte, if any
   if(length >= 1)
   {
#ifdef _CPU_BIG_ENDIAN
      //Update checksum value
      checksum += (uint32_t) *p << 8;
#else
      //Update checksum value
      checksum += (uint32_t) *p;
#endif
   }

   //Fold 32-bit sum to 16 bits (first pass)
   checksum = (checksum & 0xFFFF) + (checksum >> 16);
   //Fold 32-bit sum to 16 bits (second pass)
   checksum = (checksum & 0xFFFF) + (checksum >> 16);

   //Restore checksum endianness
   if(((uintptr_t) data & 1) != 0)
   {
      //Swap checksum value
      checksum = ((checksum >> 8) | (checksum << 8)) & 0xFFFF;
   }

   //Return 1's complement value
   return checksum ^ 0xFFFF;
}

void ipv4DumpHeader(const Ipv4Header *ipHeader)
{
    //Dump IP header contents
    printf("## IPv4 Section\r\n");
    printf("  Version = %" PRIu8 "\r\n", ipHeader->version);
    printf("  Header Length = %" PRIu8 "\r\n", ipHeader->headerLength);
    printf("  Type Of Service = %" PRIu8 "\r\n", ipHeader->typeOfService);
    printf("  Total Length = %" PRIu16 "\r\n", ntohs(ipHeader->totalLength));
    printf("  Identification = %" PRIu16 "\r\n", ntohs(ipHeader->identification));
    printf("  Flags = 0x%01X\r\n", ntohs(ipHeader->fragmentOffset) >> 13);
    printf("  Fragment Offset = %" PRIu16 "\r\n", ntohs(ipHeader->fragmentOffset) & 0x1FFF);
    printf("  Time To Live = %" PRIu8 "\r\n", ipHeader->timeToLive);
    printf("  Protocol = %" PRIu8 "\r\n", ipHeader->protocol);
    printf("  Header Checksum = 0x%04" PRIX16 "\r\n", ntohs(ipHeader->headerChecksum));
    printf("  Src Addr = %s\r\n", ipv4AddrToString(ipHeader->srcAddr, NULL));
    printf("  Dest Addr = %s\r\n", ipv4AddrToString(ipHeader->destAddr, NULL));
}
