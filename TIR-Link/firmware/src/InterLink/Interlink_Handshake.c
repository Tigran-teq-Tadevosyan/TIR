#include "Interlink_Handshake.h"

#include "definitions.h"

#include "Common/Common.h"
#include "Common/Debug.h"
#include "Network/DHCP/DHCP_Server.h"

// Roles related definition section
#define LINK_HANSHAKE_TIMEOUT_MIN (250) // in milliseconds
#define LINK_HANSHAKE_TIMEOUT_MAX (1000) // in milliseconds

#define MAX_HANSHAKE_OFFER_REPEAT_COUNT (10)

static enum {
    HANDSHAKE_UNINTIALIZED,
    WAITING_ROLE_ACK,
    HANDSHAKE_FINISHED
} HANDSHAKE_STATE = HANDSHAKE_UNINTIALIZED; 

static InterlinkHostRole    SELF_LINK_ROLE = UNDEFINED_ROLE,
                            PAIR_LINK_ROLE = UNDEFINED_ROLE;
static uint16_t     LINK_HANSHAKE_TIMEOUT,
                    LINK_OFFER_TIMEOUT = 1000; // 1 sec in milliseconds
static uint16_t     OFFER_REPEAT_COUNT;
static systime_t    LAST_REPEAT_TIMESTAMP;

void init_InterlinkHandshake(void) {
    LINK_HANSHAKE_TIMEOUT = getRandNumber(LINK_HANSHAKE_TIMEOUT_MIN, LINK_HANSHAKE_TIMEOUT_MAX);
    #ifdef TIR_LINK_DEBUG_LEVEL0
    printDebug("LINK_HANSHAKE_TIMEOUT: %u \r\n", LINK_HANSHAKE_TIMEOUT);
    #endif

    LAST_REPEAT_TIMESTAMP = get_SysTime_ms();
}
void process_InterlinkHandshake(void) {
    if( HANDSHAKE_STATE == HANDSHAKE_UNINTIALIZED && 
        timeCompare(get_SysTime_ms() , LAST_REPEAT_TIMESTAMP) > LINK_HANSHAKE_TIMEOUT) {
        
        printDebug("Sending HANDSHAKE_REQUEST Repeat\r\n");
        send_InterLink(HANDSHAKE_REQUEST, NULL, 0);
        LAST_REPEAT_TIMESTAMP = get_SysTime_ms();
    } else if( HANDSHAKE_STATE == WAITING_ROLE_ACK && 
        timeCompare(get_SysTime_ms() , LAST_REPEAT_TIMESTAMP) > LINK_OFFER_TIMEOUT) {
        
        if(++OFFER_REPEAT_COUNT >= MAX_HANSHAKE_OFFER_REPEAT_COUNT) {
            HANDSHAKE_STATE = HANDSHAKE_UNINTIALIZED;
        } else {
            HANDSHAKE_ROLE_PAIR payload = {
                .role1 = DHCP_SERVER1, // Self role
                .role2 = DHCP_SERVER2  // Pair role
            };

            printDebug("Sending HANDSHAKE_OFFER Repeat\r\n");
            send_InterLink(HANDSHAKE_OFFER, (uint8_t*)&payload, sizeof(HANDSHAKE_ROLE_PAIR));
            LAST_REPEAT_TIMESTAMP = get_SysTime_ms();
        }
    }
}

InterlinkHostRole get_SelfLinkRole(void) {
    return SELF_LINK_ROLE;
}

InterlinkHostRole get_PairLinkRole(void) {
    return PAIR_LINK_ROLE;
}

void process_handshakeRequest(void) {
    if(HANDSHAKE_STATE != HANDSHAKE_UNINTIALIZED) return;

    HANDSHAKE_ROLE_PAIR payload = {
        .role1 = DHCP_SERVER1, // Self role
        .role2 = DHCP_SERVER2  // Pair role
    };
    
    send_InterLink(HANDSHAKE_OFFER, (uint8_t*)&payload, sizeof(HANDSHAKE_ROLE_PAIR));
    HANDSHAKE_STATE = WAITING_ROLE_ACK;
    OFFER_REPEAT_COUNT = 0;
}

void process_handshakeOffer(HANDSHAKE_ROLE_PAIR* roles) {
    SELF_LINK_ROLE = roles->role2;
    PAIR_LINK_ROLE = roles->role1;
    
    printDebug("Self Role: %d\r\n", SELF_LINK_ROLE);
    printDebug("Pair Role: %d\r\n", PAIR_LINK_ROLE);
    
    // Here we switch placed for response
    roles->role1 = SELF_LINK_ROLE;
    roles->role2 = PAIR_LINK_ROLE;
    
    send_InterLink(HANDSHAKE_ACK, (uint8_t*)roles, sizeof(HANDSHAKE_ROLE_PAIR));
    HANDSHAKE_STATE = HANDSHAKE_FINISHED;
    dhcpServerStart();
    UART2_ReadThresholdSet(50);
    printDebug("dhcpServerRunning(): %d\r\n", dhcpServerRunning());
}

void process_handshakeACK(HANDSHAKE_ROLE_PAIR* roles) {
    if(HANDSHAKE_STATE != WAITING_ROLE_ACK) return;
    
    SELF_LINK_ROLE = roles->role2;
    PAIR_LINK_ROLE = roles->role1;
    HANDSHAKE_STATE = HANDSHAKE_FINISHED;
    
    printDebug("Self Role: %d\r\n", SELF_LINK_ROLE);
    printDebug("Pair Role: %d\r\n", PAIR_LINK_ROLE);
    dhcpServerStart();
    UART2_ReadThresholdSet(50);
    printDebug("dhcpServerRunning(): %d\r\n", dhcpServerRunning());
}