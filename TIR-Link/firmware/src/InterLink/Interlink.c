#include "Interlink.h"

#include "Interlink_RxDMA.h"
#include "Interlink_TxDMA.h"
#include "Interlink_Handshake.h"

#include "definitions.h"

bool _RX_DMAC_TRANSFER_EVENT_ERROR_FLAG = false;
bool _TX_DMAC_TRANSFER_EVENT_ERROR_FLAG = false; 

void init_Interlink(void) {    
    init_intlinkTxDMA();
    init_intlinkRxDMA();
    init_InterlinkHandshake();
}

void process_Interlink(void) {
    LED_BB_Clear();
    process_InterlinkHandshake();
    LED_BB_Set();
    
    LED_RR_Clear();
    process_intlinkRxDMA();
    LED_RR_Set();
}
