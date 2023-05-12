#include "Interlink_Forwarding.h"

#include "Interlink.h"
#include "Network/Network.h"
#include "Common/Debug.h"
#include "W5500/MACRAW_FrameFIFO.h"

struct ForwardingListEntry_t; // Forward declaration for next entry
typedef struct ForwardingListEntry_t ForwardingListEntry;

struct ForwardingListEntry_t {
    ForwardingBinding   binding;
    ForwardingListEntry *next;
};

ForwardingListEntry *FORWARDING_LIST_HEAD = NULL;
size_t              FORWARDING_LIST_LENGTH = 0;

static ForwardingListEntry* findEntry(ForwardingBinding *fBinding);
static bool ipAddrPresent(Ipv4Addr ipAddr);
static bool macAddrPresent(MacAddr *macAddr);

void send_AddForwardingEntry(DhcpServerBinding *binding) {
    ForwardingBinding fBinding = {
                                    .macAddr = binding->macAddr,
                                    .ipAddr = binding->ipAddr
                                };

    send_InterLink(FORWARDING_TABLE_ADDITION, (uint8_t*)&fBinding, sizeof(ForwardingBinding));
}

void send_RemoveForwardingEntry(DhcpServerBinding *binding) {
    ForwardingBinding fBinding = {
                                    .macAddr = binding->macAddr,
                                    .ipAddr = binding->ipAddr
                                };

    send_InterLink(FORWARDING_TABLE_REMOVAL, (uint8_t*)&fBinding, sizeof(ForwardingBinding));
}

void process_AddForwardingEntry(ForwardingBinding *fBinding) {
    if(findEntry(fBinding) != NULL) return; // Binding already exists

    ForwardingListEntry *new_entry = malloc(sizeof(*new_entry));
    if(new_entry == NULL) {
        printDebug("Failed to allocate memory in 'process_AddForwardingEntry'\r\n");
    }

    new_entry->binding.macAddr = fBinding->macAddr;
    new_entry->binding.ipAddr = fBinding->ipAddr;
    new_entry->next = FORWARDING_LIST_HEAD; // Append it to the of the beginning list
    FORWARDING_LIST_HEAD = new_entry;
    ++FORWARDING_LIST_LENGTH;

    printDebug("Added forwarding entry -> Mac %s; IP: %s; Len: %u\r\n",  macAddrToString(&fBinding->macAddr, NULL),
                                                                       ipv4AddrToString(fBinding->ipAddr, NULL),
                                                                       FORWARDING_LIST_LENGTH);
}

void process_RemoveForwardingEntry(ForwardingBinding *fBinding) {
    if(FORWARDING_LIST_LENGTH == 0) return; // Nothing to remove

    // First we check if need to remove the head
    if( FORWARDING_LIST_HEAD->binding.ipAddr == fBinding->ipAddr &&
        macCompAddr(&FORWARDING_LIST_HEAD->binding.macAddr, &fBinding->macAddr)) {

        printDebug("Removed (head) forwarding entry -> Mac %s; IP: %s\r\n", macAddrToString(&fBinding->macAddr, NULL),
                                                                            ipv4AddrToString(fBinding->ipAddr, NULL));
        ForwardingListEntry *oldHead = FORWARDING_LIST_HEAD;
        FORWARDING_LIST_HEAD = oldHead->next;
        free(oldHead);
        --FORWARDING_LIST_LENGTH;
        return;
    }

    ForwardingListEntry *previous = FORWARDING_LIST_HEAD,
                        *current = FORWARDING_LIST_HEAD->next;

    while(current != NULL) {
        if( current->binding.ipAddr == fBinding->ipAddr &&
            macCompAddr(&current->binding.macAddr, &fBinding->macAddr)) break;

        previous = current;
        current = current->next;
    }

    if(current == NULL) return; // Entry was  not found

    // We have found the entry and not just need to remove it
    previous->next = current->next;
    free(current);
    --FORWARDING_LIST_LENGTH;

    printDebug("Removed (head) forwarding entry -> Mac %s; IP: %s\r\n", macAddrToString(&fBinding->macAddr, NULL),
                                                                        ipv4AddrToString(fBinding->ipAddr, NULL));
}

void process_ForwardingRequest(EthFrame* frame, uint16_t frame_length) {
//    printDebug("Forwarding processing to -> Mac %s;\r\n", macAddrToString(&frame->destAddr, NULL));

//    if(betoh16(frame->type) != ETH_TYPE_IPV4) return;

//    size_t ip_packet_length = frame_length - sizeof(EthFrame);
//    Ipv4Header *ip_header = (Ipv4Header *) frame->data;

//    ethDumpHeader(frame);
//    ipv4DumpHeader(ip_header);

    uint8_t *tx_frame = (uint8_t*)reserveItem_TxFIFO(frame_length);
    memmove(tx_frame, frame, frame_length);
    incremetTailIndex_TxFIFO();
}

void interlink_ForwardIfAppropriate(EthFrame *frame, uint16_t frame_length) {
    if(betoh16(frame->type) != ETH_TYPE_IPV4 && betoh16(frame->type) != ETH_TYPE_ARP) return;

    // First we check based on destination MAC address, if it is broadcast or in forwarding table we forward it
    if(macCompAddr(&frame->destAddr, &MAC_BROADCAST_ADDR) || macAddrPresent(&frame->destAddr)) {
//        printDebug("Forwarding req to -> Mac %s;\r\n",
//                macAddrToString(&frame->destAddr, NULL));
        send_InterLink(FORWARDING_REQUEST, (uint8_t*)frame, frame_length);
        return;
    }

    // Then we check if it is an ip packet and check based on ip address 
    if(betoh16(frame->type) == ETH_TYPE_IPV4) {
        Ipv4Header *ip_header = (Ipv4Header *) frame->data;

        if(ip_header->destAddr == IPV4_BROADCAST_ADDR || ipAddrPresent(ip_header->destAddr)) {  
            printDebug("Forwarding req to -> Mac %s; IP: %s\r\n",
                    macAddrToString(&frame->destAddr, NULL),
                    ipv4AddrToString(ip_header->destAddr, NULL));
            send_InterLink(FORWARDING_REQUEST, (uint8_t*)frame, frame_length);
            return;
        }
    }
}

static ForwardingListEntry* findEntry(ForwardingBinding *fBinding) {
    ForwardingListEntry* current = FORWARDING_LIST_HEAD;

    while(current != NULL) {
        if( current->binding.ipAddr == fBinding->ipAddr &&
            macCompAddr(&current->binding.macAddr, &fBinding->macAddr)) return current;

        current = current->next;
    }

    return NULL;
}

static bool ipAddrPresent(Ipv4Addr ipAddr) {
    ForwardingListEntry* current = FORWARDING_LIST_HEAD;

    while(current != NULL) {
        if(current->binding.ipAddr == ipAddr) return true;

        current = current->next;
    }

    return false;
}

static bool macAddrPresent(MacAddr *macAddr) {
    ForwardingListEntry* current = FORWARDING_LIST_HEAD;

    while(current != NULL) {
        if( macCompAddr(&current->binding.macAddr, macAddr)) return true;

        current = current->next;
    }

    return false;
}
