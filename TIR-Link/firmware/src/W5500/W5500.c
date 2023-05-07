#include "W5500.h"

#include <stddef.h>                     
#include <stdbool.h>                    
#include <stdlib.h>                     
#include <stdio.h>
#include "definitions.h"                
#include <string.h>
#include <endian.h>

#include "./driver/Ethernet/W5500/w5500.h"
#include "./driver/Ethernet/wizchip_conf.h"   
#include "./driver/Ethernet/socket.h"

#include "Common/Common.h"
#include "Common/Debug.h"

#include "Network/Network.h"
#include "Network/DHCP/DHCP.h"
#include "Network/DHCP/DHCP_Debug.h"
#include "MACRAW_FrameFIFO.h"

#define MACRAW_SOCKET   0 // The socket number for MACRAW socket (only 0 can operate in MACRAW mode)

const uint8_t SOCKET_MEM_LAYOUT[2][8] = { { 8, 2, 1, 1, 1, 1, 1, 1 }, { 8, 2, 1, 1, 1, 1, 1, 1 } };
const wiz_NetInfo DEFAULT_NETWORK_CONFIG = {  .mac   = {0xA5, 0xE6, 0x38, 0x61, 0xB8, 0x71},
                                                .ip     = {100, 100, 0, 1},
                                                .sn     = {255, 255, 255, 0},
                                                .gw     = {0, 0, 0, 0},
                                                .dns    = {8, 8, 8, 8},		// Google public DNS (8.8.8.8 , 8.8.4.4), OpenDNS (208.67.222.222 , 208.67.220.220)
                                                .dhcp   = NETINFO_STATIC };

bool W5500_CURRENTLY_SENDING = false;

const uint16_t W5500_INT_MASK = IK_SOCK_ALL; // Enable interrupts from all sockets
const uint8_t MACRAW_SOCKET_INT_MASK =  SIK_RECEIVED |  // Socket data receive interrupt
                                        SIK_SENT;       // Socket data sent interrupt;

TIR_Status init_MACRAWSocket(void);
void process_W5500Int();


// Callback handler functions for wizchip internal use
void  wizchip_select(void);
void  wizchip_deselect(void);
uint8_t wizchip_read();
void  wizchip_write(uint8_t wb);
void wizchip_readBurst(uint8_t* pBuf, uint16_t len);
void  wizchip_writeBurst(uint8_t* pBuf, uint16_t len);
void wizchip_critEnter(void);
void  wizchip_critExit(void);

#define W5500_ENTER_RESET_DELAY (10)    // in miliseconds
#define W5500_EXIT_RESET_DELAY  (200)   // in miliseconds

void init_W5500(void)
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
  
//    GPIO_PinInterruptCallbackRegister(W5500_INT_PIN, W5500IntEventHandler, NO_CONTEXT);
//    GPIO_PinIntEnable(W5500_INT_PIN, GPIO_INTERRUPT_ON_BOTH_EDGES);
    
	/* Setting wizchip sockets memory layout */
	if (ctlwizchip(CW_INIT_WIZCHIP, (void*) SOCKET_MEM_LAYOUT) == -1) {
		printDebug("WIZCHIP Initialized fail.\r\n");
    	while (1);
	}

    wizchip_setnetinfo((wiz_NetInfo*)&DEFAULT_NETWORK_CONFIG);
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

void process_W5500 (void)
{
    if(W5500_INT_Get() == 0) {
        LED_BB_Toggle();
        process_W5500Int();
    }
    
    if(!W5500_CURRENTLY_SENDING && !isEmpty_TxFIFO()) {
        W5500_CURRENTLY_SENDING = true;
        uint16_t frame_length;
        EthFrame* frame = peekHead_TxFIFO(&frame_length);

        wiz_send_data(MACRAW_SOCKET, (uint8_t*)frame, frame_length);
        setSn_CR(MACRAW_SOCKET, Sn_CR_SEND);
        while(getSn_CR(MACRAW_SOCKET));
        
        removeHead_TxFIFO();
    }
    
    if(!isEmpty_RxFIFO()) {
        uint16_t new_frame_len;
        EthFrame *new_frame = peekHead_RxFIFO(&new_frame_len);
        ethDumpHeader(new_frame);
        
        if(betoh16(new_frame->type) == ETH_TYPE_IPV4) {
            Ipv4Header *ip_header = (Ipv4Header*)((uint8_t*)new_frame + ETH_HEADER_SIZE); 
            ipv4DumpHeader(ip_header);
            
            if(ip_header->protocol == IPV4_PROTOCOL_UDP) {
                UdpHeader *upd_header = (UdpHeader *) ((uint8_t*)ip_header + (ip_header->headerLength * 4));
                udpDumpHeader(upd_header);
                
                if(betoh16(upd_header->destPort) == DHCP_SERVER_PORT && betoh16(upd_header->srcPort) == DHCP_CLIENT_PORT) {
                    size_t dhcp_pkt_len = betoh16(upd_header->length) - UDP_HEADER_LENGTH;
                    DhcpMessage *dhcp_packet = (DhcpMessage *) ((uint8_t*)upd_header + UDP_HEADER_LENGTH);
                    dhcpDumpMessage(dhcp_packet, dhcp_pkt_len);
                }
            }
        }
        
        removeHead_RxFIFO();
    }
}

#define W5500_LENGTH_SECTION_LENGTH (2)

void process_W5500Int() {    
    if(W5500_INT_Get() == 1) return; // Check for falling edge (as we use active low)
    
    uint16_t W5500_int;
    uint8_t socket_int;

    ctlwizchip(CW_GET_INTERRUPT, &W5500_int);
    ctlsocket(MACRAW_SOCKET, CS_GET_INTERRUPT, &socket_int);
    
    if(socket_int & SIK_CONNECTED) { } // Currently not used 
    if(socket_int & SIK_DISCONNECTED) { } // Currently not used 

    if(socket_int & SIK_RECEIVED) {
        uint16_t rx_buffer_length = getSn_RX_RSR(MACRAW_SOCKET);
        if(rx_buffer_length > 0) {
            uint16_t rx_buffer_pkt_length;    
            
            // Reading the length of current packet we want to read
            wiz_recv_peek_data(MACRAW_SOCKET, (uint8_t*)&rx_buffer_pkt_length, W5500_LENGTH_SECTION_LENGTH);
            rx_buffer_pkt_length = be16toh(rx_buffer_pkt_length) - W5500_LENGTH_SECTION_LENGTH;
            
            // We read only in case the whole packet is currently available
            while(rx_buffer_pkt_length <= (rx_buffer_length - W5500_LENGTH_SECTION_LENGTH)) {
                // We ignore this 2 bytes for the read pointer to go over packet size, so we can read the frame itself 
                wiz_recv_ignore(MACRAW_SOCKET, W5500_LENGTH_SECTION_LENGTH); 
                setSn_CR(MACRAW_SOCKET, Sn_CR_RECV);
                while(getSn_CR(MACRAW_SOCKET));        
                
                EthFrame* read_buffer = reserveItem_RxFIFO(rx_buffer_pkt_length);
                wiz_recv_data(MACRAW_SOCKET, (uint8_t*)read_buffer, rx_buffer_pkt_length);
                incremetTailIndex_RxFIFO();
                
                setSn_CR(MACRAW_SOCKET, Sn_CR_RECV);
                while(getSn_CR(MACRAW_SOCKET));
                
                // If we only had exactly one package in the rx buffer of the chip we just break as the is nothing else to check
                if(rx_buffer_pkt_length == (rx_buffer_length - W5500_LENGTH_SECTION_LENGTH)) break;
                
                // If there is something else in the rx buffer we update the values to check (in while statement) if we have another complete packet in th buffer
                rx_buffer_pkt_length -= (rx_buffer_length - W5500_LENGTH_SECTION_LENGTH);
                wiz_recv_peek_data(MACRAW_SOCKET, (uint8_t*)&rx_buffer_pkt_length, W5500_LENGTH_SECTION_LENGTH);
                rx_buffer_pkt_length = be16toh(rx_buffer_pkt_length) - W5500_LENGTH_SECTION_LENGTH;
            }
        }
    }

    if(socket_int & SIK_SENT) {
        W5500_CURRENTLY_SENDING = false;
        if(!isEmpty_TxFIFO()) {
            W5500_CURRENTLY_SENDING = true;
            uint16_t frame_length;
            EthFrame* frame = peekHead_TxFIFO(&frame_length);
            
            wiz_send_data(MACRAW_SOCKET, (uint8_t*)frame, frame_length);
            setSn_CR(MACRAW_SOCKET, Sn_CR_SEND);
            while(getSn_CR(MACRAW_SOCKET));
            
            removeHead_TxFIFO();
        }
    }

    ctlsocket(MACRAW_SOCKET, CS_CLR_INTERRUPT, (void *)&MACRAW_SOCKET_INT_MASK);
    
    // Would only be called if we have chip interrupts not related to sockets, so we clear that interrupts.
    if(W5500_INT_MASK & 0x00FF) {
        ctlwizchip(CW_CLR_INTERRUPT, (void*)&W5500_INT_MASK );
    }
}

void send_pkt(void) {
    EthFrame* frame = reserveItem_TxFIFO(ETH_HEADER_SIZE + 2);
    
    memcpy(&frame->destAddr, &MAC_BROADCAST_ADDR, 6);
    memcpy(&frame->srcAddr, &HOST_MAC_ADDR, 6);
    frame->type = htobe16(0xF322);
    frame->data[0] = 4;
    frame->data[1] = 4;
    
    incremetTailIndex_TxFIFO();
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
//	while(SPI1_IsBusy());
//    GPIO_PinIntDisable(W5500_INT_PIN);
}

void  wizchip_critExit(void) {
//    while(SPI1_IsBusy());
//    GPIO_PinIntEnable(W5500_INT_PIN, GPIO_INTERRUPT_ON_BOTH_EDGES);
}