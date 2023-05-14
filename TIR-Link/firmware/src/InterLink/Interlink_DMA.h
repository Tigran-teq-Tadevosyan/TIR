#ifndef _INTERLINK_DMA_H
#define _INTERLINK_DMA_H

#define START_DELIMITER_VALUE 0x24262426
#define MAX_TX_PENDING_DATA_SIZE 20000 // 20 Kb in bytes

typedef struct Intlink_DMAEntry Intlink_DMAEntry;
typedef struct IntlinkHeader IntlinkHeader;

struct IntlinkHeader {
    uint32_t start_delimiter;
    uint16_t payload_length;
    uint8_t  message_type;
} __attribute__((__packed__));

struct Intlink_DMAEntry {
    IntlinkHeader header;
    uint8_t *payload;
    bool head_sent, payload_sent;
    Intlink_DMAEntry *next;
};

void init_InterlinkDMA(void);


#endif // _INTERLINK_DMA_H
