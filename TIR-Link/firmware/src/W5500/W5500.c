#include "W5500.h"

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <stdio.h>
#include <string.h>
#include <endian.h>

#include "definitions.h"                // SYS function prototypes

#include "./driver/Ethernet/W5500/w5500.h"
#include "./driver/Ethernet/wizchip_conf.h"   
#include "./driver/Ethernet/socket.h"
#include "Common/Debug.h"
#include "Common/Common.h"

// Maqur section
#define MACRAW_SOCKET   0 // The socket number for MACRAW socket (only 0 can serve in MACRAW mode)

const uint8_t SOCKET_MEM_LAYOUT[2][8] = { { 8, 2, 1, 1, 1, 1, 1, 1 }, { 8, 2, 1, 1, 1, 1, 1, 1 } };
const wiz_NetInfo DEFAULT_NETWORK_CONFIG = { .mac   = {0x98, 0x28, 0xA6, 0x1A, 0x6D, 0x49},
                                            .ip     = {192,168,1,80},
                                            .sn     = {255, 255, 255, 0},
                                            .gw     = {192,168,1,1},
                                            .dns    = {8, 8, 8, 8},		// Google public DNS (8.8.8.8 , 8.8.4.4), OpenDNS (208.67.222.222 , 208.67.220.220)
                                            .dhcp   = NETINFO_STATIC };

const uint16_t W5500_INT_MASK = IK_SOCK_ALL; // Enable interrupts from all sockets
const uint8_t MACRAW_SOCKET_INT_MASK =  SIK_RECEIVED |  // Socket data receive interrupt
                                        SIK_SENT;       // Socket data sent interrupt;

TIR_Status init_MACRAWSocket(void);

// Callback handler functions for wizchip internal use
void  wizchip_select(void);
void  wizchip_deselect(void);
uint8_t wizchip_read();
void  wizchip_write(uint8_t wb);
void wizchip_readBurst(uint8_t* pBuf, uint16_t len);
void  wizchip_writeBurst(uint8_t* pBuf, uint16_t len);
void wizchip_critEnter(void);
void  wizchip_critExit(void);

// END


#define MAX_MSG_SIZE    4000
#define MACRAW_SOCKET   0

const uint8_t startDel[4] = {'$','_','$','_'};

macrawSocketStateEnum macrawSocketState = MACRAW_Socket_Init;

uint8_t *macrawTxListBuff[200];
uint16_t macrawTxListSize[200];
uint8_t macrawTxListIn = 0, macrawTxListOut = 0;

uint8_t macrawTxBuff[MAX_MSG_SIZE];
uint8_t macrawRxBuff[MAX_MSG_SIZE];
uint16_t macrawRxBytes, macrawTxBytes, macrawWaitTxEnd, macrawRxBytesLeft = 0; 
uint16_t macPacketeLenght, macPacketeProc, LenHigh, LenLow;




typedef struct
{
   union
   {
      uint8_t b[6];
      uint16_t w[3];
   };
} __attribute__((__packed__)) MacAddr;

typedef struct
{
   MacAddr destAddr; //0-5
   MacAddr srcAddr;  //6-11
   uint16_t type;    //12-13
//   uint8_t data[];   //14
} __attribute__((__packed__)) EthHeader;

typedef enum
{
   ETH_TYPE_IPV4  = 0x0800,
   ETH_TYPE_ARP   = 0x0806,
   ETH_TYPE_RARP  = 0x8035,
   ETH_TYPE_VLAN  = 0x8100,
   ETH_TYPE_IPV6  = 0x86DD,
   ETH_TYPE_EAPOL = 0x888E,
   ETH_TYPE_VMAN  = 0x88A8,
   ETH_TYPE_LLDP  = 0x88CC,
   ETH_TYPE_PTP   = 0x88F7
} EthType;

MacAddr MAC_SOURCE_ADDR = {{{0x98, 0x28, 0xA6, 0x1A, 0x6D, 0x49}}};
MacAddr MAC_BROADCAST_ADDR = {{{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}};

EthHeader ethernetHeader; 

#define ETH_HEADER_SIZE 14

uint8_t *ether_TxData = NULL;
uint16_t ether_TxLength = 0;
bool ether_dataToSend = false;

static void W5500InterruptEventHandler(GPIO_PIN pin, uintptr_t contextHandle);

void sendDataMACrawSocket(uint8_t *dt, uint16_t sz)
{
    if(macrawSocketState == MACRAW_Socket_Idle)
    {
        memcpy(macrawTxBuff + macrawTxBytes, dt, sz);
        macrawTxBytes += sz;
    }
}

void sendDataMACrawSocketDirect(uint8_t *dt, uint16_t sz)
{
    uint8_t dataSize[2];
    dataSize[0] = ((sz & 0xFF00) >> 8) ;
    dataSize[1] = ((sz & 0x00FF)) ;
    
//    sz = htobe16(htobe16); // size to network byte order
    
    if(macrawSocketState == MACRAW_Socket_Idle)
    {
        macrawTxListBuff[macrawTxListIn] = (uint8_t *)malloc(sz + 2); //+ 2
        
        if(macrawTxListBuff[macrawTxListIn] != NULL)
        {
            macrawTxListSize[macrawTxListIn] = sz + 2; //+ 2;

            memcpy(macrawTxListBuff[macrawTxListIn], dataSize, 2);
            memcpy(macrawTxListBuff[macrawTxListIn] + 2, dt, sz); // + 2
            
            macrawTxListIn = (macrawTxListIn + 1) % 200;
        }
        else
        {
            printDebug("Error malloc %u bytes\r\n", sz + 2);
        }
    }
}

void processMACrawSocket(void)
{
    switch(macrawSocketState)
    {
        case MACRAW_Socket_Init:
           
            if(createMACRAWSocket()) {
                macrawSocketState = MACRAW_Socket_Idle;
            }
            break;
        case MACRAW_Socket_Idle:
//            if(W5500_INT_Get() == 0)
//            {
//                W5500IntEventHandler(W5500_CS_PIN, 0);
//                LED_BB_Toggle();
//            }
            if(macrawTxListIn != macrawTxListOut) { 
                if(macrawWaitTxEnd == 1) {
                    if(getSn_IR(MACRAW_SOCKET) & Sn_IR_SENDOK) {
                        macrawWaitTxEnd = 0;
                        setSn_IR(MACRAW_SOCKET , Sn_IR_SENDOK);
                    } 
                }
                
                
                if(macrawWaitTxEnd == 0)
                {
                    if(ether_dataToSend) {
                        uint8_t *pkt = malloc(2 + ETH_HEADER_SIZE + ether_TxLength); 
                        
                        memmove(pkt, htobe16(ETH_HEADER_SIZE + ether_TxLength), 2);
                        memmove(pkt + 2, &ethernetHeader, ETH_HEADER_SIZE);
                        memmove(pkt + 2 + ETH_HEADER_SIZE, ether_TxData, ether_TxLength);
                        
                        free(pkt);
                        free(ether_TxData);
                        ether_dataToSend = false;
                        
                        printDebug("Sent PKG\r\n");
                    } else {
                        wiz_send_data(MACRAW_SOCKET, macrawTxListBuff[macrawTxListOut], macrawTxListSize[macrawTxListOut]);
                        setSn_CR(MACRAW_SOCKET,Sn_CR_SEND);
                        while(getSn_CR(MACRAW_SOCKET));
                        printDebug("Sent MACRAW = %u, %u\r\n", macrawTxListSize[macrawTxListOut], macrawTxListOut);

                        free(macrawTxListBuff[macrawTxListOut]);
                        macrawTxListOut = (macrawTxListOut + 1) % 200;
                        macrawWaitTxEnd = 1;
                    }
                }

            }
            
            
            macrawRxBytes = getSn_RX_RSR(MACRAW_SOCKET);

            if(macrawRxBytes > 0)
            {
                ethPacket pct;
                if(macrawRxBytes + macrawRxBytesLeft > MAX_MSG_SIZE) 
                    macrawRxBytes = MAX_MSG_SIZE - macrawRxBytesLeft - 5;
                
                wiz_recv_data(MACRAW_SOCKET, &macrawRxBuff[macrawRxBytesLeft], macrawRxBytes);
                
                setSn_CR(MACRAW_SOCKET, Sn_CR_RECV);
                while(getSn_CR(MACRAW_SOCKET));
                
                macPacketeProc = 0;
                macrawRxBytesLeft += macrawRxBytes; 
            }
            
            if(macrawRxBytesLeft > 40) {
                printDebug("\r\nRx MACRAW PCT %u\r\n", macrawRxBytesLeft);
                macPacketeLenght = (((uint16_t)macrawRxBuff[0]) << 8) + (uint16_t)macrawRxBuff[1];
                    
                if(macPacketeLenght > macrawRxBytesLeft) {
                    printDebug("waiting for %u\r\n", macPacketeLenght);
                } else if(macPacketeLenght == 0) {
                    
                } else {
                    printDebug((char*)macrawRxBuff);
                    
                    UART2_Write((uint8_t *)startDel, 4);
                    macPacketeLenght -= 2;
                    UART2_Write((uint8_t *)&(macPacketeLenght), 2);
                    macPacketeLenght += 2;
                    UART2_Write(&macrawRxBuff[2], macPacketeLenght - 2);          
                    
                    macrawRxBytesLeft -= macPacketeLenght;

                    printDebug("Size %u, Left %u\r\n", macPacketeLenght, macrawRxBytesLeft);
                    if(macrawRxBytesLeft > 0)
                        memmove(macrawRxBuff, &macrawRxBuff[macPacketeLenght], macrawRxBytesLeft);
                }
            } 

            break;
            
        case MACRAW_Socket_Send:
            break;
            
        case MACRAW_Socket_Receive:
                
            LED_RR_Toggle();
            break;
                    
        case MACRAW_Socket_Reset:
            break;
    }
}

static void W5500InterruptEventHandler(GPIO_PIN pin, uintptr_t contextHandle) {
    (void)pin;
    (void)contextHandle;
    
    if(W5500_INT_Get() == 1) return; // Check for falling edge (as we use active low)
    
    uint16_t W5500_int;
    uint8_t socket_int;

    ctlwizchip(CW_GET_INTERRUPT, &W5500_int);
    ctlsocket(MACRAW_SOCKET, CS_GET_INTERRUPT, &socket_int);
    ctlsocket(MACRAW_SOCKET, CS_CLR_INTERRUPT, (void *)&MACRAW_SOCKET_INT_MASK);

    if(socket_int & SIK_CONNECTED) { } // Currently not used 
    if(socket_int & SIK_DISCONNECTED) { } // Currently not used 

    if(socket_int & SIK_RECEIVED) {
        macrawSocketState = MACRAW_Socket_Receive;
    }

    if(socket_int & SIK_SENT) {

    }   

    // Would only be called if we have chip interrupts not related to sockets, so we clear that interrupts.
    if(W5500_INT_MASK & 0x00FF) {
        ctlwizchip(CW_CLR_INTERRUPT, &W5500_INT_MASK );
    }
}

void processW5500 (void)
{
    if(W5500_INT_Get() == 0)
    {
        LED_BB_Toggle();
    }
    
    processMACrawSocket();
}

/////////////////////////////// GOOD Section

#define W5500_ENTER_RESET_DELAY (10)    // in miliseconds
#define W5500_EXIT_RESET_DELAY  (200)   // in miliseconds

void W5500_Init()
{   
    // Reseting the W5500
	W5500_CS_Set();
    W5500_RESET_Clear();
    delay_ms(W5500_ENTER_RESET_DELAY);
	W5500_RESET_Set();
    delay_ms(W5500_EXIT_RESET_DELAY);
    
    // Setting the callback handler functions
    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
	reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);
    reg_wizchip_spiburst_cbfunc(wizchip_readBurst, wizchip_writeBurst);
    reg_wizchip_cris_cbfunc(wizchip_critEnter, wizchip_critExit);
    
    GPIO_PinInterruptCallbackRegister(W5500_INT_PIN, W5500InterruptEventHandler, 0);
    GPIO_PinIntEnable(W5500_INT_PIN, GPIO_INTERRUPT_ON_BOTH_EDGES);
    
	/* Setting wizchip sockets memory layout */
	if (ctlwizchip(CW_INIT_WIZCHIP, (void*) SOCKET_MEM_LAYOUT) == -1) {
		printDebug("WIZCHIP Initialized fail.\r\n");
    	while (1);
	}

    wizchip_setnetinfo(&DEFAULT_NETWORK_CONFIG);
    printDebug("\r\n\r\n\r\nWIZCHIP Initialized\r\n");
    printNetworkInfo();
    
    init_MACRAWSocket();
}

#define SOCKET_OPEN_CHECK_DELAY (1)     // in miliseconds
#define SOCKET_OPEN_MAX_TIMEOUT (1000)  // in miliseconds

TIR_Status init_MACRAWSocket(void) {
    printDebug("Creating MACRAW socket...\r\n");
    close(MACRAW_SOCKET);
    int8_t status_socket_created = socket(
                                            MACRAW_SOCKET,  // MACROW socket number
                                            Sn_MR_MACRAW,   // Socket protocol
                                            0x00,           // No need for port number in MACRAW mode
                                            Sn_MR_MIP6B | Sn_MR_MULTI // Flags for blocking IPv6 and enabling MAC filtering
                                        );

    if(status_socket_created != MACRAW_SOCKET) { // Checking if the sockets was successfully created
        printDebug("MACRAW socket() failed, code = %d\r\n", status_socket_created);
        return Failure;
    } else {
        int8_t status_socket_opened = getSn_SR(MACRAW_SOCKET);
        uint16_t socket_opened_timeout = 0;
        while(status_socket_opened != SOCK_MACRAW) {
            delay_ms(SOCKET_OPEN_CHECK_DELAY);
            status_socket_opened = getSn_SR(MACRAW_SOCKET);
            if(socket_opened_timeout++ >= SOCKET_OPEN_MAX_TIMEOUT) {
                printDebug("Cant initialize MACRAW socket!\r\n");
                return Failure;
            }
        }
        printDebug("MACRAW Socket created, connecting...\r\n");    
    }
                           
    ctlwizchip(CW_SET_INTRMASK, (void *)&W5500_INT_MASK);
    ctlsocket(MACRAW_SOCKET, CS_SET_INTMASK, (void *)&MACRAW_SOCKET_INT_MASK);
    
    return Success;
}

// Callback handler functions for wizchip internal use
void  wizchip_select(void) { W5500_CS_Clear(); }
void  wizchip_deselect(void) { W5500_CS_Set(); }

uint8_t wizchip_read() {
	uint8_t rb;
    SPI1_Read(&rb, 1);
	while(SPI1_IsBusy());
    return rb;
}

void  wizchip_write(uint8_t wb) {
    SPI1_Write(&wb, 1);
    while(SPI1_IsBusy());
}

void wizchip_readBurst(uint8_t* pBuf, uint16_t len) {
    SPI1_Read(pBuf, len);
	while(SPI1_IsBusy());
}

void  wizchip_writeBurst(uint8_t* pBuf, uint16_t len) {
    SPI1_Write(pBuf, len);
    while(SPI1_IsBusy());
}

void wizchip_critEnter(void) {
	while(SPI1_IsBusy());
    GPIO_PinIntDisable(W5500_INT_PIN);
}

void  wizchip_critExit(void) {
    while(SPI1_IsBusy());
    GPIO_PinIntEnable(W5500_INT_PIN, GPIO_INTERRUPT_ON_BOTH_EDGES);
}