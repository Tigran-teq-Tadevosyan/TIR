#ifndef _INTERLINK_H
#define _INTERLINK_H

#include <inttypes.h>
#include <stddef.h>

typedef enum {
    UNDEFINED_ROLE   = 0x00,
    DHCP_SERVER1	    = 0x01,
    DHCP_SERVER2	    = 0x02
} InterlinkHostRole;

typedef enum {
    HANDSHAKE_REQUEST		= 0x01,
    HANDSHAKE_OFFER		= 0x02,
    HANDSHAKE_ACK		= 0x03,
    FORWARDING_TABLE_ADDITION	= 0x04,
    FORWARDING_TABLE_REMOVAL	= 0x05,
    FORWARDING_REQUEST		= 0x06
} InterlinkMessageType;

void init_Interlink(void);
void process_Interlink(void);
void send_InterLink(InterlinkMessageType messageType, uint8_t *payload, uint16_t payload_len);

size_t   rxDataLength(void);

// Definitions in Interlink_Handshake.c
InterlinkHostRole get_SelfLinkRole(void);
InterlinkHostRole get_PairLinkRole(void);

#endif // _INTERLINK_H