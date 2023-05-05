#ifndef _W5500_H
#define _W5500_H

    
typedef struct raw_ArpRequest_t
{
   uint8_t arp_header[6];  ///< ARP Header 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF 
   uint8_t source_mac1[6];  ///< Source Mac Address
   uint8_t ip_header[2];  ///< Protocol Info
   uint8_t hard_prot_info[8];  ///< Protocol Info
   //   uint16_t hardwareType;   ///< Hardware Access Type, 0x0001 Ethernet
//   uint16_t protAddType;   ///< The Protocol Address Type for IPv4 is 0x0800
//   uint8_t hardwareAddLenght;   ///< HW Address length is 6 for Ethernet. 
//   uint8_t protAddLenght;   ///< Protocol Address Length is 4; 
   uint8_t source_mac2[6];  ///< Source Mac Address
   uint8_t source_ip[4];  ///< Source IP Address
   uint8_t dest_mac[6];  ///< Source Mac Address
   uint8_t dest_ip[4];  ///< Source IP Address   
   uint8_t padding[8];  ///< DNS server IP Address
}raw_ArpRequest;

typedef struct ethPacket_t
{
   uint16_t mac_lenght;     ///< Ethernet Type 
   uint8_t dest_mac[6];     ///< Destination Mac Address
   uint8_t source_mac[6];   ///< Source Mac Address
   uint16_t ethertype;      ///< Ethernet Type 
   uint8_t *payload;        ///< Payload of pacekt 46 - 1500
} ethPacket;

                        
typedef enum  
{ 
    MACRAW_Socket_Init = 0,     
    MACRAW_Socket_Idle,
    MACRAW_Socket_Send,
    MACRAW_Socket_Receive,
    MACRAW_Socket_Reset
} macrawSocketStateEnum;


void W5500_Init();
void printNetworkInfo (void);
void processMACrawSocket(void);
void sendDataMACrawSocket(uint8_t *dt, uint16_t sz);
void sendDataMACrawSocketDirect(uint8_t *dt, uint16_t sz);

void processW5500 (void);

#endif