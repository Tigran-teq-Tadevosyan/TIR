#include "Interlink.h"

#include "Interlink_RxDMA.h"
#include "Interlink_TxDMA.h"
#include "Interlink_Handshake.h"

void init_Interlink(void) {
    
    init_intlinkTxDMA();
    init_intlinkRxDMA();
    init_InterlinkHandshake();
}

void process_Interlink(void) {
    process_InterlinkHandshake();
    process_intlinkRxDMA();
}
