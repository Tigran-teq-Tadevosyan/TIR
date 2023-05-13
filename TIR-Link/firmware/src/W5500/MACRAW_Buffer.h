#ifndef _MACRAW_BUFFER_H
#define _MACRAW_BUFFER_H

#include <inttypes.h>
#include <stdbool.h>

#define RX_BUFFER_LENGTH (30000) // in bytes
#define TX_BUFFER_LENGTH (30000) // in bytes

#define RX_BUFFER_MAX_ITEM_COUNT (100)
#define TX_BUFFER_MAX_ITEM_COUNT (100)

bool macraw_isRxEmpty(void);
uint8_t* macrawRx_reserve(uint16_t length);
void macrawRx_processReservedChunk();
uint8_t* macrawRx_getHeadPkt(uint16_t *length);

//bool macraw_isTxEmpty(void);
//uint8_t* macraw_reserveTx(uint16_t length);
//uint8_t* macraw_getTxHead(uint16_t *length);

#endif  // _MACRAW_BUFFER_H