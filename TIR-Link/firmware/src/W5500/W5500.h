#ifndef _W5500_H
#define _W5500_H

#include <inttypes.h>

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
void processMACrawSocket(void);
void sendDataMACrawSocket(uint8_t *dt, uint16_t sz);
void sendDataMACrawSocketDirect(uint8_t *dt, uint16_t sz);

void processW5500 (void);

// Definitions in W5500_Debug.c
void printNetworkInfo (void);

#endif