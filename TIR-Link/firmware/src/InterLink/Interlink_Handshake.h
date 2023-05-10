#ifndef _INTERLINK_HANDSHAKE_H
#define _INTERLINK_HANDSHAKE_H

#include "Interlink.h"

typedef struct {
    InterlinkHostRole role1; // Self role for sender and pair role for receiver
    InterlinkHostRole role2; // Self role for receiver and pair role for sender
} __attribute__((__packed__)) HANDSHAKE_ROLE_PAIR;

void init_InterlinkHandshake(void);
void process_InterlinkHandshake(void);

void process_handshakeRequest(void);
void process_handshakeOffer(HANDSHAKE_ROLE_PAIR* roles);
void process_handshakeACK(HANDSHAKE_ROLE_PAIR* roles);

#endif // _INTERLINK_HANDSHAKE_H