#include "Interlink_Handshake.h"

#include "definitions.h"

#include "Common/Common.h"
#include "Common/Debug.h"
#include "Network/DHCP/DHCP_Server.h"
#include "Interlink_TxDMA.h"
#include "Interlink_Forwarding.h"

#define LINK_HANSHAKE_REQUEST_TIMEOUT_MIN (250) // in milliseconds
#define LINK_HANSHAKE_REQUEST_TIMEOUT_MAX (1000) // in milliseconds

#define LINK_HANSHAKE_OFFER_TIMEOUT_MIN (250) // in milliseconds
#define LINK_HANSHAKE_OFFER_TIMEOUT_MAX (1000) // in milliseconds

#define MAX_HANSHAKE_REQUEST_REPEAT_COUNT (10)
#define MAX_HANSHAKE_OFFER_REPEAT_COUNT (10)

static HANDSHAKE_ROLE_PAIR __attribute__((coherent)) offer_payload;
static HANDSHAKE_ROLE_PAIR __attribute__((coherent)) ack_payload;

static enum {
    HANDSHAKE_UNINTIALIZED,
    WAITING_ROLE_ACK,
    HANDSHAKE_FINISHED
} HANDSHAKE_STATE = HANDSHAKE_UNINTIALIZED; 

static InterlinkHostRole    SELF_LINK_ROLE = UNDEFINED_ROLE,
                            PAIR_LINK_ROLE = UNDEFINED_ROLE;

static uint16_t LINK_HANSHAKE_REQUEST_TIMEOUT, LINK_HANSHAKE_OFFER_TIMEOUT; 
static uint16_t REQUEST_ATTEMP_COUNT = 0, OFFER_ATTEMP_COUNT = 0;

static systime_t    LAST_REPEAT_TIMESTAMP;

void init_InterlinkHandshake(void) {
    LINK_HANSHAKE_REQUEST_TIMEOUT = getRandNumber(LINK_HANSHAKE_REQUEST_TIMEOUT_MIN, LINK_HANSHAKE_REQUEST_TIMEOUT_MAX);
    LINK_HANSHAKE_OFFER_TIMEOUT = getRandNumber(LINK_HANSHAKE_OFFER_TIMEOUT_MIN, LINK_HANSHAKE_OFFER_TIMEOUT_MAX);
    #ifdef TIR_LINK_DEBUG_LEVEL0
    printDebug("LINK_HANSHAKE_REQUEST_TIMEOUT: %u \r\n", LINK_HANSHAKE_REQUEST_TIMEOUT);
    printDebug("LINK_HANSHAKE_OFFER_TIMEOUT: %u \r\n", LINK_HANSHAKE_OFFER_TIMEOUT);
    #endif

    LAST_REPEAT_TIMESTAMP = get_SysTime_ms();
}
void process_InterlinkHandshake(void) {
    if(HANDSHAKE_STATE == HANDSHAKE_FINISHED) {
        return;
    } else if(  HANDSHAKE_STATE == HANDSHAKE_UNINTIALIZED && 
                timeCompare(get_SysTime_ms() , LAST_REPEAT_TIMESTAMP) > LINK_HANSHAKE_REQUEST_TIMEOUT) {
        
        ++REQUEST_ATTEMP_COUNT;
        if(REQUEST_ATTEMP_COUNT > MAX_HANSHAKE_REQUEST_REPEAT_COUNT) {
            // The pair is not available so we choose role for ourself and start.
            SELF_LINK_ROLE = DHCP_SERVER1;
            PAIR_LINK_ROLE = DHCP_SERVER2;
            HANDSHAKE_STATE = HANDSHAKE_FINISHED;
            dhcpServerStart();
        } else {
            printDebug("Sending HANDSHAKE_REQUEST Repeat\r\n");
            append2Queue_intlinkTxDMA(HANDSHAKE_REQUEST, NULL, 0, false);
            LAST_REPEAT_TIMESTAMP = get_SysTime_ms();
        }
    } else if(  HANDSHAKE_STATE == WAITING_ROLE_ACK && 
                timeCompare(get_SysTime_ms() , LAST_REPEAT_TIMESTAMP) > LINK_HANSHAKE_OFFER_TIMEOUT) {
        
        ++OFFER_ATTEMP_COUNT;
        if(OFFER_ATTEMP_COUNT > MAX_HANSHAKE_OFFER_REPEAT_COUNT) {
            HANDSHAKE_STATE = HANDSHAKE_UNINTIALIZED;
            HANDSHAKE_STATE = HANDSHAKE_FINISHED;
        } else {
            offer_payload.role1 = SELF_LINK_ROLE; // Self role
            offer_payload.role2 = PAIR_LINK_ROLE; // Pair role

            printDebug("Sending HANDSHAKE_OFFER Repeat\r\n");
            append2Queue_intlinkTxDMA(HANDSHAKE_OFFER, (uint8_t*)&offer_payload, sizeof(HANDSHAKE_ROLE_PAIR), false);
            LAST_REPEAT_TIMESTAMP = get_SysTime_ms();
        }
    }
}

InterlinkHostRole get_SelfLinkRole(void) { return SELF_LINK_ROLE; }
InterlinkHostRole get_PairLinkRole(void) { return PAIR_LINK_ROLE; }

void process_handshakeRequest(void) {
    if(HANDSHAKE_STATE == HANDSHAKE_UNINTIALIZED) {
        SELF_LINK_ROLE = DHCP_SERVER1;
        PAIR_LINK_ROLE = DHCP_SERVER2;
        offer_payload.role1 = SELF_LINK_ROLE;
        offer_payload.role2 = PAIR_LINK_ROLE;
        
        HANDSHAKE_STATE = WAITING_ROLE_ACK;
        append2Queue_intlinkTxDMA(HANDSHAKE_OFFER, (uint8_t*)&offer_payload, sizeof(HANDSHAKE_ROLE_PAIR), false);
        OFFER_ATTEMP_COUNT = 0;
    } else if(HANDSHAKE_STATE == WAITING_ROLE_ACK || HANDSHAKE_STATE == HANDSHAKE_FINISHED) {
        offer_payload.role1 = SELF_LINK_ROLE;
        offer_payload.role2 = PAIR_LINK_ROLE;
        
        append2Queue_intlinkTxDMA(HANDSHAKE_OFFER, (uint8_t*)&offer_payload, sizeof(HANDSHAKE_ROLE_PAIR), false);
        OFFER_ATTEMP_COUNT = 0;
    }
}

void process_handshakeOffer(HANDSHAKE_ROLE_PAIR* roles) {
    if(HANDSHAKE_STATE == HANDSHAKE_UNINTIALIZED) {
        SELF_LINK_ROLE = roles->role2;
        PAIR_LINK_ROLE = roles->role1;

        // Here we switch placed for response
        ack_payload.role1 = SELF_LINK_ROLE;
        ack_payload.role2 = PAIR_LINK_ROLE;
        append2Queue_intlinkTxDMA(HANDSHAKE_ACK, (uint8_t*)&ack_payload, sizeof(HANDSHAKE_ROLE_PAIR), false);
        HANDSHAKE_STATE = HANDSHAKE_FINISHED;
        dhcpServerStart();
    } else if(HANDSHAKE_STATE == WAITING_ROLE_ACK) {
        if(roles->role2 == SELF_LINK_ROLE && roles->role1 == PAIR_LINK_ROLE) { // The roles match
            // Here we switch placed for response
            ack_payload.role1 = SELF_LINK_ROLE;
            ack_payload.role2 = PAIR_LINK_ROLE;
            append2Queue_intlinkTxDMA(HANDSHAKE_ACK, (uint8_t*)&ack_payload, sizeof(HANDSHAKE_ROLE_PAIR), false);
            HANDSHAKE_STATE = HANDSHAKE_FINISHED;
            dhcpServerStart();
        } else { // roles do not match
            return; // We ignore
        }
    } else if(HANDSHAKE_STATE == HANDSHAKE_FINISHED) {
        if(roles->role2 == SELF_LINK_ROLE && roles->role1 == PAIR_LINK_ROLE) { // The roles match
            // Here we switch placed for response
            ack_payload.role1 = SELF_LINK_ROLE;
            ack_payload.role2 = PAIR_LINK_ROLE;
            append2Queue_intlinkTxDMA(HANDSHAKE_ACK, (uint8_t*)&ack_payload, sizeof(HANDSHAKE_ROLE_PAIR), false);

            DhcpServerBinding *binding;
            for(uint16_t i = 0; i < DHCP_SERVER_MAX_CLIENTS; i++)
            {
               binding = &clientBinding[i];
               if(binding->validLease) {
                   send_AddForwardingEntry(binding);
               }
            }
        } else { // roles do not match
            return; // We ignore
        }
    }
}

void process_handshakeACK(HANDSHAKE_ROLE_PAIR* roles) {
    if(HANDSHAKE_STATE == HANDSHAKE_UNINTIALIZED) {
        return; // We ignore
    } else if(HANDSHAKE_STATE == WAITING_ROLE_ACK) {
        if(roles->role2 == SELF_LINK_ROLE && roles->role1 == PAIR_LINK_ROLE) { // The roles match
            HANDSHAKE_STATE = HANDSHAKE_FINISHED;
            dhcpServerStart();
        } else { // roles do not match
            return; // We ignore
        }
    } else if(HANDSHAKE_STATE == HANDSHAKE_FINISHED) {
        if(roles->role2 == SELF_LINK_ROLE && roles->role1 == PAIR_LINK_ROLE) { // The roles match
            DhcpServerBinding *binding;
            for(uint16_t i = 0; i < DHCP_SERVER_MAX_CLIENTS; i++)
            {
               binding = &clientBinding[i];
               if(binding->validLease) {
                   send_AddForwardingEntry(binding);
               }
            }
        } else { // roles do not match
            return; // We ignore
        }
    }
}