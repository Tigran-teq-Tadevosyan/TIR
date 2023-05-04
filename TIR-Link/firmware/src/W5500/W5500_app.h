/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.h

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

#ifndef _W5500_APP_H    /* Guard against multiple inclusion */
#define _W5500_APP_H


/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

/* This section lists the other files that are included in this file.
 */

/* TODO:  Include other files here if needed. */


/* Provide C++ Compatibility */
#ifdef __cplusplus
extern "C" {
#endif
    
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
   uint8_t source_mac[6];  ///< Source Mac Address
   uint16_t ethertype;     ///< Ethernet Type 
   uint8_t *payload;        ///< Payload of pacekt 46 - 1500
}ethPacket;

enum tcpSocketModeEnum {    TCP_Socket_Mode_Server = 0, 
                        TCP_Socket_Mode_Client
                   };

enum tcpSocketStateEnum {   TCP_Socket_Init = 0, 
                            TCP_Socket_Start_Listen,
                            TCP_Socket_Wait_For_Connection,
                            TCP_Socket_Start_Connect,
                            TCP_Socket_Connected,
                            TCP_Socket_Wait_For_Receive,
                            TCP_Socket_Receive,
                            TCP_Socket_Disconnect,
                            TCP_Socket_Reset
                        };
                        
enum macrawSocketStateEnum { MACRAW_Socket_Init = 0, 
                            MACRAW_Socket_Idle,
                            MACRAW_Socket_Send,
                            MACRAW_Socket_Receive,
                            MACRAW_Socket_Reset
                        };





void W5500_Init();
void printNetworkInfo (void);
void processMACrawSocket(void);
void sendDataMACrawSocket(uint8_t *dt, uint16_t sz);
void sendDataMACrawSocketDirect(uint8_t *dt, uint16_t sz);
void sendArpRequestIprawSocket(uint8_t *destIp);


void processTCPSocket(void);
void connectTCPSocket(uint8_t *ip, uint16_t port);
void disconnectTCPSocket(void);
void sendDataTCPSocket(uint8_t *dt, uint16_t sz);

void processW5500 (void);

/* Provide C++ Compatibility */
#ifdef __cplusplus
}
#endif

#endif /* _EXAMPLE_FILE_NAME_H */

/* *****************************************************************************
 End of File
 */
