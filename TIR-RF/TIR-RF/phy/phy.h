/*
 * phy.h
 *
 *  Created on: May 15, 2023
 *      Author: Vahagn
 */

#ifndef PHY_PHY_H_
#define PHY_PHY_H_

#include <stdint.h>

/* TI Drivers */
#include <ti/drivers/rf/RF.h>
#include <ti/drivers/GPIO.h>



#define RX_START_TO_SETTLE_TICKS   256 /* Time between RX start trigger and the radio
                                        * being ready to receive the first preamble bit.
                                        * This is a fixed value for CMD_PROP_RX. */
#define TX_START_TO_PREAMBLE_TICKS 384 /* Time between TX start trigger and first bit on air.
                                        * This is a fixed value for CMD_PROP_TX. */

#define TX_CMD_TO_START 500 /* Time between TX start trigger and first bit on air.
                                        * This is a fixed value for CMD_PROP_TX. */

#define PREAMLBE_CNT 4
#define SYNCWORD_CNT 4
#define CRC_CNT 2

#define DATA_SPEED 1000000

/* Packet RX Configuration */
#define MAC_HEADER_SIZE             8
#define MAX_DATA_SIZE               240  /* Data Maximum Size of a Generic Data Entry */
#define MAX_LENGTH             MAC_HEADER_SIZE + MAX_DATA_SIZE /* Max length byte the radio will accept */
#define NUM_DATA_ENTRIES       2  /* NOTE: Only two data entries supported at the moment */
#define NUM_APPENDED_BYTES     2  /* The Data Entries data field will contain:
                                   * 1 Header byte (RF_cmdPropRx.rxConf.bIncludeHdr = 0x1)
                                   * Max 30 payload bytes
                                   * 1 status byte (RF_cmdPropRx.rxConf.bAppendStatus = 0x1) */
#define NO_PACKET              0
#define PACKET_RECEIVED        1


void phyInit(void);

/***** Function definitions *****/
void ReceivedOnRFcallback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e);
void TransmitPackageRFcallback(RF_Handle h, RF_CmdHandle ch, RF_EventMask e);

void phyCancel(void);
void phySendPackage(uint8_t* data, uint16_t len);
void phyStartReceive(void);

uint8_t phyRxPacketRdy(void);
uint32_t phyGetLastRxTickPassed(void);
void phyRxPacketRdyClean(void);

void phyProcess(void);



#endif /* PHY_PHY_H_ */
