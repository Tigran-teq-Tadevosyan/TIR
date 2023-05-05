/* ************************************************************************** */
/** Descriptive File Name

  @Company
    Company Name

  @File Name
    filename.c

  @Summary
    Brief description of the file.

  @Description
    Describe the purpose of this file.
 */
/* ************************************************************************** */

/* ************************************************************************** */
/* ************************************************************************** */
/* Section: Included Files                                                    */
/* ************************************************************************** */
/* ************************************************************************** */

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <stdio.h>
#include <string.h>
#include <endian.h>

#include "definitions.h"                // SYS function prototypes

#include "W5500_app.h"      
#include "./driver/Ethernet/W5500/w5500.h"    
#include "./driver/Ethernet/wizchip_conf.h"   
#include "./driver/Ethernet/socket.h"

/* TODO:  Include other files here if needed. */


#define MAX_MSG_SIZE    4000
#define MACRAW_SOCKET   0
#define TCP_SOCKET      1

const uint8_t startDel[4] = {'$','_','$','_'};

enum macrawSocketStateEnum macrawSocketState = MACRAW_Socket_Init;

uint8_t *macrawTxListBuff[200];
uint16_t macrawTxListSize[200];
uint8_t macrawTxListIn = 0, macrawTxListOut = 0;


uint8_t macrawTxBuff[MAX_MSG_SIZE];
uint8_t macrawRxBuff[MAX_MSG_SIZE];
uint16_t macrawRxBytes, macrawTxBytes, macrawWaitTxEnd, macrawRxBytesLeft = 0; 
uint16_t macPacketeLenght, macPacketeProc, LenHigh, LenLow;


uint8_t tcpTxBuff[MAX_MSG_SIZE];
uint8_t tcpRxBuff[MAX_MSG_SIZE];

enum tcpSocketModeEnum tcpSocketMode = TCP_Socket_Mode_Server;
enum tcpSocketStateEnum tcpSocketState = TCP_Socket_Init;

uint16_t tcpRxBytes, tcpTxBytes; 

wiz_NetInfo myNetwork = {   .mac = {0x98, 0x28, 0xA6, 0x1A, 0x6D, 0x49},//{0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef}, //{0x49, 0x6D, 0x1A, 0xA6, 0x28, 0x98},//
                            .ip = {192,168,1,80},
                            .sn = {255, 255, 255, 0},
                            .gw = {192,168,1,1},
                            .dns = {8, 8, 8, 8},		// Google public DNS (8.8.8.8 , 8.8.4.4), OpenDNS (208.67.222.222 , 208.67.220.220)
                            .dhcp = NETINFO_STATIC };


raw_ArpRequest arpRequenst = {  .arp_header = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                .source_mac1 = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef},
                                .ip_header = {0x08, 0x06},
                                .hard_prot_info = {0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01},                                
                                .source_mac2 = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef},
                                .source_ip =  {192,168,1,80},
                                .dest_mac = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                .dest_ip = {0x00, 0x00, 0x00, 0x00},
                                .padding = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20}
};

raw_ArpRequest arpReply = {     .arp_header = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
                                .source_mac1 = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef},
                                .ip_header = {0x08, 0x06},
                                .hard_prot_info = {0x00, 0x01, 0x08, 0x00, 0x06, 0x04, 0x00, 0x01},                                
                                .source_mac2 = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef},
                                .source_ip =  {192,168,1,80},
                                .dest_mac = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                .dest_ip = {0x00, 0x00, 0x00, 0x00},
                                .padding = {0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20}
};


uint8_t tcpServerIp[4];
uint16_t tcpServerPort;

static void W5500IntEventHandler(GPIO_PIN pin, uintptr_t contextHandle);


void  wizchip_select(void)
{
	W5500_CS_Clear();
    //Chip_GPIO_SetPinState(LPC_GPIO, 0, 2, false);	// SSEL(CS)
}

void  wizchip_deselect(void)
{
	W5500_CS_Set();
    //Chip_GPIO_SetPinState(LPC_GPIO, 0, 2, true);	// SSEL(CS)
}

uint8_t wizchip_read()
{
	uint8_t rb;
    //while(SPI1_IsBusy());
    SPI1_Read(&rb, 1);
    //Chip_SSP_ReadFrames_Blocking(LPC_SSP0, &rb, 1);
	while(SPI1_IsBusy());
    return rb;
}

void  wizchip_write(uint8_t wb)
{
	//while(SPI1_IsBusy());
    SPI1_Write(&wb, 1);
    while(SPI1_IsBusy());
}

void wizchip_readBurst(uint8_t* pBuf, uint16_t len)
{
    //while(SPI1_IsBusy());
    SPI1_Read(pBuf, len);
	while(SPI1_IsBusy());
}

void  wizchip_writeBurst(uint8_t* pBuf, uint16_t len)
{
	//while(SPI1_IsBusy());
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

    printDebug("Creating IPRAW socket...\r\n");
    //IINCHIP_WRITE(Sn_PROTO(MACRAW_SOCKET), IPPROTO_ICMP);
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
                printDebug("Cant initialize MACRAW socket!!!\r\n");
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

void sendArpRequestIprawSocket(uint8_t *destIp)
{
    memcpy(arpRequenst.dest_ip, destIp, sizeof(arpRequenst.dest_ip));
    sendDataMACrawSocket((uint8_t *)&arpRequenst, sizeof(arpRequenst));
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
                    wiz_send_data(MACRAW_SOCKET, macrawTxListBuff[macrawTxListOut], macrawTxListSize[macrawTxListOut]);
                    setSn_CR(MACRAW_SOCKET,Sn_CR_SEND);
                    while(getSn_CR(MACRAW_SOCKET));
                    printDebug("Sent MACRAW = %u, %u\r\n", macrawTxListSize[macrawTxListOut], macrawTxListOut);

                    free(macrawTxListBuff[macrawTxListOut]);
                    macrawTxListOut = (macrawTxListOut + 1) % 200;
                    macrawWaitTxEnd = 1;
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


bool createTCPSocket(void)
{
    int8_t status;
    uint16_t timeout = 0; 

    printDebug("Creating TCP socket...\r\n");
    status = socket(TCP_SOCKET, Sn_MR_TCP, 80, 0x00);

    if(status != TCP_SOCKET) {
        printDebug("TCP socket() failed, code = %d\r\n", status);
        return false;
    }
    else 
    {
        while(status != SOCK_INIT)
        {
            status = getSn_SR(TCP_SOCKET);
            delay_ms(10);
            timeout++;
            if(timeout > 150)
            {
                printDebug("Cant initialize TCP socket!!!\r\n");
                return false;
            }
        }

        printDebug("TCP Socket created, connecting...\r\n");    
    }
    
    
    if(status == SOCK_INIT)
    {
        uint16_t flags16 = IK_SOCK_0 | IK_SOCK_1;
        uint8_t flags8 =  SIK_RECEIVED | SIK_CONNECTED | SIK_DISCONNECTED; //SIK_SENT |
        ctlwizchip(CW_SET_INTRMASK, (void *)&flags16);
        ctlsocket(TCP_SOCKET, CS_SET_INTMASK, (void *)&flags8);
    }
    
    return true;
}

void processTCPSocket(void)
{
    int8_t status;
    
    switch(tcpSocketState)
    {
        case TCP_Socket_Init:
            // Create Socket
            if(createTCPSocket())
            {
                if(tcpSocketMode == TCP_Socket_Mode_Server)
                {
                    tcpSocketState = TCP_Socket_Start_Listen;           
                }
                else
                {
                    tcpSocketState = TCP_Socket_Start_Connect;  
                }
            }    
            break;
        
        case TCP_Socket_Start_Listen:
            // Initialize Listen
            status = listen(TCP_SOCKET);
            if(status != SOCK_OK) 
            {
                printDebug("TCP listen() failed, code = %d\r\n", status);
                tcpSocketState = TCP_Socket_Init;
            }
            else 
            {
                printDebug("TCP listen() OK\r\n");
                tcpSocketState = TCP_Socket_Wait_For_Connection;
            }
            break;
            
        case TCP_Socket_Start_Connect:
            status = connectNonBlocking(TCP_SOCKET,tcpServerIp,tcpServerPort);
            if(status != SOCK_OK) 
            {
                printDebug("TCP connect() failed, code = %d\r\n", status);
                tcpSocketState = TCP_Socket_Init;
            }
            else 
            {
                printDebug("TCP connect() OK\r\n");
                tcpSocketState = TCP_Socket_Wait_For_Connection;
            }
            
            break;
            
        case TCP_Socket_Wait_For_Connection:
            // Wait for connection
            break;
            
        case TCP_Socket_Connected:
            // Is connected
            tcpSocketState = TCP_Socket_Wait_For_Receive;
            printDebug("Input connection\r\n");
            while(getSn_SR(TCP_SOCKET) != SOCK_ESTABLISHED) 
            {
                // Something went terribly wrong 
                printDebug("Error socket status\r\n");
            }

            uint8_t rIP[4];
            char Message[250];
            getsockopt(TCP_SOCKET, SO_DESTIP, rIP);

            printDebug("IP:  %d.%d.%d.%d\r\n", rIP[0], rIP[1], rIP[2], rIP[3]);

            sprintf(Message, "You Are Connected to  - %d.%d.%d.%d\r\n",     myNetwork.ip[0],
                                                                            myNetwork.ip[1],
                                                                            myNetwork.ip[2],
                                                                            myNetwork.ip[3]);

            send(TCP_SOCKET, (uint8_t*)Message, strlen(Message));
            break;
            
        case TCP_Socket_Wait_For_Receive:
            // If there is something to send send it, else wait for RX
            if(tcpTxBytes > 0)
            {
                send(TCP_SOCKET, tcpTxBuff, tcpTxBytes);   
                tcpTxBytes = 0;
            }
            break;
            
        case TCP_Socket_Receive:
            // Something to RX
            tcpRxBytes = getSn_RX_RSR(TCP_SOCKET);

            if(tcpRxBytes > 0)
            {
                tcpRxBytes = recv(TCP_SOCKET,tcpRxBuff, MAX_MSG_SIZE);

                printDebug("Received Data %u \r\n", tcpRxBytes);
                tcpRxBuff[tcpRxBytes] = 0;
                printDebug((char *)tcpRxBuff);
                printDebug("\r\n", tcpRxBytes);

                // Loop Back
                send(TCP_SOCKET, (uint8_t*)tcpRxBuff, tcpRxBytes);
                if(tcpRxBytes == 5)
                {
                    if(strcmp((char *)tcpRxBuff, "close") == 0)
                    {
                        tcpSocketState = TCP_Socket_Disconnect;
                        return;
                    }
                }
                tcpSocketState = TCP_Socket_Wait_For_Receive;
                LED_BB_Toggle();
            }
            else
            {
                tcpSocketState = TCP_Socket_Wait_For_Receive;
            } 
            break;
            
        case TCP_Socket_Disconnect:
            // Disconnect Socket
            if(tcpSocketMode == TCP_Socket_Mode_Server)
            {
                tcpSocketState = TCP_Socket_Start_Listen;           
            }
            else
            {
                tcpSocketState = TCP_Socket_Wait_For_Connection;  
            }
            disconnect(TCP_SOCKET);
            break;
            
        case TCP_Socket_Reset:
            // Reset Socket
            tcpSocketState = TCP_Socket_Init;
            disconnect(TCP_SOCKET);
            delay_ms(500);
            printDebug("Closing socket.\r\n\r\n\r\n\r\n");
            close(TCP_SOCKET);
            break;
    }

}

void connectTCPSocket(uint8_t *ip, uint16_t port)
{
    memcpy(tcpServerIp, ip , 4);
    tcpServerPort = port;
    
    if(tcpSocketMode == TCP_Socket_Mode_Server)
    {
        tcpSocketState = TCP_Socket_Init;
        tcpSocketMode = TCP_Socket_Mode_Client;
    }
    else
    {
        tcpSocketState = TCP_Socket_Init;
    }
}

void disconnectTCPSocket(void)
{
    if(tcpSocketMode == TCP_Socket_Mode_Server)
    {
        tcpSocketState = TCP_Socket_Disconnect;
    }
    else
    {
        tcpSocketMode = TCP_Socket_Mode_Server;
        tcpSocketState = TCP_Socket_Disconnect;
    }
}

void sendDataTCPSocket(uint8_t *dt, uint16_t sz)
{
    if(tcpSocketState == TCP_Socket_Wait_For_Receive || tcpSocketState == TCP_Socket_Receive)
    {
        memcpy(tcpTxBuff + tcpTxBytes, dt, sz);
        tcpTxBytes += sz;
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
            
            //if(intChipStatus & IK_SOCK_1)
            {
                ctlsocket(TCP_SOCKET, CS_GET_INTERRUPT, &intStatus);
                ctlsocket(TCP_SOCKET, CS_CLR_INTERRUPT, (void *)&flag);

                if(intStatus & SIK_CONNECTED)
                {
                    tcpSocketState = TCP_Socket_Connected;
                }
                else if(intStatus & SIK_RECEIVED)
                {
                    tcpSocketState = TCP_Socket_Receive;
                }
                else if(intStatus & SIK_DISCONNECTED)
                {
                    tcpSocketState = TCP_Socket_Disconnect;
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
    //processTCPSocket();

}


/* *****************************************************************************
 End of File
 */
