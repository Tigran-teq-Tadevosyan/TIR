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


#define MAC_RX_PACKET_BUFF_CNT  10

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
    uint8_t data[MAX_LENGTH];
}rfMacDataPacket_t;


extern uint8_t macProcessStatus;
extern uint8_t deviceType;

void macInit(void);
void macProcess(void);
void startMacTransmitt(void);

uint8_t macTxRdyPacketCnt(void);
rfMacDataPacket_t * macGetTxFreePacket(void);
rfMacDataPacket_t * macGetTxRdyPacket(void);


rfMacDataPacket_t * macGetRxFreePacket(void);
rfMacDataPacket_t * macGetRxRdyPacket(void);


#endif /* MAC_MAC_H_ */
