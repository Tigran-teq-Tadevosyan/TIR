/*
 * mac.h
 *
 *  Created on: May 9, 2023
 *      Author: Vahagn
 */

#ifndef MAC_MAC_H_
#define MAC_MAC_H_

#include <stdint.h>

#define MAC_HEADER_SIZE 8

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
    uint8_t data[200];
}rfMacDataPacket_t;


extern rfMacDataPacket_t rfMacTxPacket;
extern rfMacDataPacket_t rfMacRxPacket;



#endif /* MAC_MAC_H_ */
