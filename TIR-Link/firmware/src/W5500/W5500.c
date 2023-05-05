#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <stdio.h>
#include <string.h>
#include <endian.h>

#include "definitions.h"                // SYS function prototypes

#include "W5500.h"      
#include "./driver/Ethernet/W5500/w5500.h"    
#include "./driver/Ethernet/wizchip_conf.h"   
#include "./driver/Ethernet/socket.h"
#include "Common/Debug.h"
#include "Common/Common.h"

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

wiz_NetInfo myNetwork = {   .mac = {0x98, 0x28, 0xA6, 0x1A, 0x6D, 0x49},
                            .ip = {192,168,1,80},
                            .sn = {255, 255, 255, 0},
                            .gw = {192,168,1,1},
                            .dns = {8, 8, 8, 8},		// Google public DNS (8.8.8.8 , 8.8.4.4), OpenDNS (208.67.222.222 , 208.67.220.220)
                            .dhcp = NETINFO_STATIC };


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

static void W5500IntEventHandler(GPIO_PIN pin, uintptr_t contextHandle);

void  wizchip_select(void) { W5500_CS_Clear(); }
void  wizchip_deselect(void) { W5500_CS_Set(); }

uint8_t wizchip_read()
{
	uint8_t rb;
    SPI1_Read(&rb, 1);
	while(SPI1_IsBusy());
    return rb;
}

void  wizchip_write(uint8_t wb)
{
    SPI1_Write(&wb, 1);
    while(SPI1_IsBusy());
}

void wizchip_readBurst(uint8_t* pBuf, uint16_t len)
{
    SPI1_Read(pBuf, len);
	while(SPI1_IsBusy());
}

void  wizchip_writeBurst(uint8_t* pBuf, uint16_t len)
{
    SPI1_Write(pBuf, len);
    while(SPI1_IsBusy());
}

void wizchip_critEnter(void)
{
	//while(SPI1_IsBusy());
    //GPIO_PinIntDisable(W5500_INT_PIN);
}

void  wizchip_critExit(void)
{
    //while(SPI1_IsBusy());
    //GPIO_PinIntEnable(W5500_INT_PIN, GPIO_INTERRUPT_ON_BOTH_EDGES);
}

bool createMACRAWSocket(void)
{
    int8_t status;
    uint16_t timeout = 0; 

    printDebug("Creating MACRAW socket...\r\n");
    close(MACRAW_SOCKET);
    status = socket(MACRAW_SOCKET, Sn_MR_MACRAW, 0x00, Sn_MR_MIP6B | Sn_MR_MULTI);//, 

    if(status != MACRAW_SOCKET) {
        printDebug("MACRAW socket() failed, code = %d\r\n", status);
        return false;
    }
    else 
    {
        while(status != SOCK_MACRAW)
        {
            status = getSn_SR(MACRAW_SOCKET);
            delay_ms(10);
            timeout++;
            if(timeout > 150)
            {
                printDebug("Cant initialize MACRAW socket!\r\n");
                return false;
            }
        }

        printDebug("MACRAW Socket created, connecting...\r\n");    
    }
    
    if(status == SOCK_MACRAW)
    {
        uint16_t flags16 = IK_SOCK_ALL;
        uint8_t flags8 =  SIK_RECEIVED | SIK_CONNECTED | SIK_DISCONNECTED; //SIK_SENT | 
        ctlwizchip(CW_SET_INTRMASK, (void *)&flags16);
        ctlsocket(MACRAW_SOCKET, CS_SET_INTMASK, (void *)&flags8);
    }
    
    return true;
}

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

void sendPackage(uint8_t *payload, uint16_t length) {    
    if(ether_dataToSend) return;
    
    ethernetHeader.destAddr = MAC_BROADCAST_ADDR;
    ethernetHeader.srcAddr = MAC_SOURCE_ADDR;
    ethernetHeader.type = htobe16(0x5E2);
    
    ether_TxData = malloc(length);
    memmove(ether_TxData, payload, length);
    ether_TxLength = length;
    
    ether_dataToSend = true;
    
    printDebug("Created PKG\r\n");
}

void processMACrawSocket(void)
{
    
    switch(macrawSocketState)
    {
        case MACRAW_Socket_Init:
            
            if(createMACRAWSocket())
            {
                macrawSocketState = MACRAW_Socket_Idle;
            }
            
            break;
        
        case MACRAW_Socket_Idle:
            
//            if(W5500_INT_Get() == 0)
//            {
//                W5500IntEventHandler(W5500_CS_PIN, 0);
//                LED_BB_Toggle();
//            }
            
            if(macrawTxListIn != macrawTxListOut)
            {
                
                if(macrawWaitTxEnd == 1)
                {
                    if(getSn_IR(MACRAW_SOCKET) & Sn_IR_SENDOK)
                    {
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
            
            if(macrawRxBytesLeft > 40)
            {
                printDebug("\r\nRx MACRAW PCT %u\r\n", macrawRxBytesLeft);
                macPacketeLenght = (((uint16_t)macrawRxBuff[0]) << 8) + (uint16_t)macrawRxBuff[1];
                    
                if(macPacketeLenght > macrawRxBytesLeft)
                {
                    printDebug("waiting for %u\r\n", macPacketeLenght);
                }
                else if(macPacketeLenght == 0)
                {
                    
                }
                else
                {
                    printDebug(macrawRxBuff);
                    
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
            
            
//                memcpy(&pct, &macrawRxBuff, sizeof(pct) - sizeof(pct.payload));
//                macrawRxBytes = macrawRxBytes - 2;
                
                
                /*
                pct.payload = macrawRxBuff + sizeof(pct) - sizeof(pct.payload);
                // ETH is LSB first
                pct.mac_lenght = htobe16(pct.mac_lenght);
                pct.ethertype = htobe16(pct.ethertype);
//                uint16_t macPacketeLenght = (pct.mac_lenght[0] << 8)+ pct.mac_lenght[1];
//                uint16_t macEthernetType = (pct.ethertype[0] << 8) + pct.ethertype[1];
                
                printDebug("Rx MACRAW PCT\r\n");
                printDebug("Size = %u\r\n", pct.mac_lenght);
               
                uint16_t dataBytesCnt = 0;
                if(pct.ethertype > 1500)
                {
                    dataBytesCnt = pct.mac_lenght - sizeof(pct) + sizeof(pct.payload);   
                    switch(pct.ethertype)
                    {
                        case 0x0800:
                            printDebug("Type = %04x IPv4\r\n", pct.ethertype);
                            break;
                        case 0x0806:
                            printDebug("Type = %04x ARP\r\n", pct.ethertype);
                            break;
                        case 0x86DD:
                            printDebug("Type = %04x IPv6\r\n", pct.ethertype);
                            break;
                        case 0x88B5:
                            printDebug("Type = %04x RED FEATHER\r\n", pct.ethertype);
                            printDebug("Data Size = %u\r\n", dataBytesCnt);
                            printDebug("Data = ");
                            for(i = 0; i < dataBytesCnt; i++)
                                printDebug("%02X ", pct.payload[i]);
                            printDebug("\r\n");
                            break;
                        default:
                            printDebug("Type = %04x unknown\r\n", pct.ethertype);
                            break;   
                    }
                    
                }
                else
                {
                    dataBytesCnt = pct.ethertype;
                    printDebug("Type = %04x\r\n", pct.ethertype);
                    printDebug("Data Size = %u\r\n", dataBytesCnt);
                    printDebug("Data = ");
                    for(i = 0; i < dataBytesCnt; i++)
                        printDebug("%02X ", pct.payload[i]);
                    printDebug("\r\n");
                }
                  
                 */
                
            LED_RR_Toggle();
            break;
                    
        case MACRAW_Socket_Reset:
            break;
    }
    

}

void W5500_Init()
{
	uint8_t memsize[2][8] = { { 8, 2, 1, 1, 1, 1, 1, 1 }, { 8, 2, 1, 1, 1, 1, 1, 1 } };
    
	W5500_CS_Set();
    W5500_RESET_Clear();
    delay_ms(10);
	W5500_RESET_Set();
    delay_ms(200);
    
    reg_wizchip_cs_cbfunc(wizchip_select, wizchip_deselect);
	reg_wizchip_spi_cbfunc(wizchip_read, wizchip_write);
    reg_wizchip_spiburst_cbfunc(wizchip_readBurst, wizchip_writeBurst);
    reg_wizchip_cris_cbfunc(wizchip_critEnter, wizchip_critExit);
    
//    GPIO_PinInterruptCallbackRegister(W5500_INT_PIN, W5500IntEventHandler, 0);
//    GPIO_PinIntEnable(W5500_INT_PIN, GPIO_INTERRUPT_ON_BOTH_EDGES);
    GPIO_PinIntDisable(W5500_INT_PIN);
    
	/* wizchip initialize*/
	if (ctlwizchip(CW_INIT_WIZCHIP, (void*) memsize) == -1) {
		printDebug("WIZCHIP Initialized fail.\r\n");
    	while (1);
	}

    wizchip_setnetinfo(&myNetwork);
    printDebug("\r\n\r\n\r\nWIZCHIP Initialized\r\n");
    printNetworkInfo();
 
}

void printNetworkInfo (void)
{
    wiz_NetInfo pnetinfo;
    uint8_t chipVersion;
    
    chipVersion = getVERSIONR();
    wizchip_getnetinfo(&pnetinfo);
    printDebug("CHIP VER. %02x\r\nGW %u.%u.%u.%u \r\n", chipVersion ,
                                                        pnetinfo.gw[0],
                                                        pnetinfo.gw[1],
                                                        pnetinfo.gw[2],
                                                        pnetinfo.gw[3]);
    
    printDebug("MAC  %02x %02x %02x %02x %02x %02x\r\n", (int)pnetinfo.mac[0],
                                                        (int)pnetinfo.mac[1],
                                                        (int)pnetinfo.mac[2],
                                                        (int)pnetinfo.mac[3],
                                                        (int)pnetinfo.mac[4],
                                                        (int)pnetinfo.mac[5]);
    
    printDebug("IP %u.%u.%u.%u\r\n",    pnetinfo.ip[0],
                                        pnetinfo.ip[1],
                                        pnetinfo.ip[2],
                                        pnetinfo.ip[3]);

    
}

static void W5500IntEventHandler(GPIO_PIN pin, uintptr_t contextHandle)
{

    if(pin == W5500_INT_PIN)
    {
        if(W5500_INT_Get() == 0)
        {
            // Interrupt is present
            uint32_t intChipStatus, flag16 = IK_SOCK_ALL;
            uint8_t intStatus, flag = SIK_CONNECTED | SIK_DISCONNECTED | SIK_RECEIVED;
            
            ctlwizchip(CW_GET_INTERRUPT, &intChipStatus);
            printDebug("Int Status %04x\r\n", intChipStatus);
            
            //if(intChipStatus & IK_SOCK_0)
            {
                ctlsocket(MACRAW_SOCKET, CS_GET_INTERRUPT, &intStatus);
                ctlsocket(MACRAW_SOCKET, CS_CLR_INTERRUPT, (void *)&flag);

                if(intStatus & SIK_CONNECTED)
                {
                    
                }
                else if(intStatus & SIK_RECEIVED)
                {
                    macrawSocketState = MACRAW_Socket_Receive;
                }
                else if(intStatus & SIK_DISCONNECTED)
                {
   
                }
                else if(intStatus & SIK_SENT)
                {

                }            
            }
            
            ctlwizchip(CW_CLR_INTERRUPT, &flag16);
            
        }
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