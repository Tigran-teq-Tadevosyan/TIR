/*
 * mac.h
 *
 *  Created on: May 9, 2023
 *      Author: Vahagn
 */

#ifndef MAC_MAC_H_
#define MAC_MAC_H_

#include <stdint.h>

/* TI Drivers */
#include <ti/drivers/rf/RF.h>
#include <ti/drivers/GPIO.h>

#include "phy/phy.h"


#define MAC_PACKET_BUFF_CNT  10

enum rfDeviceType {
    RF_DEVICE_MASTER = 0,
    RF_DEVICE_SLAVE = 1
};

enum rfMacFrameType {
    MAC_SERVICE_PACKET = 0,
    MAC_DATA_PACKET = 1
};

typedef struct rfMacPacketHeader {
    uint8_t pctType;
    uint8_t pctId;
    uint16_t lenght;
    uint32_t tick;
}rfMacPacketHeader_t;


typedef struct rfMacDataPacket {
    rfMacPacketHeader_t hdr;
    uint8_t data[MAX_DATA_SIZE];
}rfMacDataPacket_t;


extern uint32_t macDataBytesSent;
extern uint32_t macDataBytesRecieved;
extern uint32_t macLostPacket;
extern uint8_t macProcessStatus;
extern uint8_t deviceType;

void macInit(void);
void macProcess(void);
void macProcessRxPacket(rfMacDataPacket_t * pct, uint32_t tick);
void startMacTransmitt(void);
void prepMacTrasnmitt(void);

uint8_t macTxRdyPacketCnt(void);
rfMacDataPacket_t * macGetTxFreePacket(void);
rfMacDataPacket_t * macGetTxRdyPacket(void);


rfMacDataPacket_t * macGetRxFreePacket(void);
rfMacDataPacket_t * macFillRxFreePacketInfo(uint32_t rxTick, uint8_t StatusByte);
rfMacDataPacket_t * macGetRxRdyPacket(void);
uint8_t macRxRdyPacketCnt(void);

uint32_t macGetRxRdyPacketTick(void);
uint8_t macGetRxRdyPacketStatus(void);


#endif /* MAC_MAC_H_ */
