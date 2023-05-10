#include "Interlink_Forwarding.h"

#include "Interlink.h"
#include "Network/Network.h"
#include "Common/Debug.h"

struct ForwardingListEntry_t; // Forward declaration for next entry 
typedef struct ForwardingListEntry_t ForwardingListEntry;

struct ForwardingListEntry_t {
    ForwardingBinding   binding;
    ForwardingListEntry *next;
};

ForwardingListEntry *FORWARDING_LIST_HEAD = NULL;
size_t              FORWARDING_LIST_LENGTH = 0;

static ForwardingListEntry* findEntry(ForwardingBinding *fBinding);

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
 
static ForwardingListEntry* findEntry(ForwardingBinding *fBinding) {
    ForwardingListEntry* current = FORWARDING_LIST_HEAD;
    
    while(current != NULL) {
//        printDebug("Comp ip1 (%s) and ip2 (%s), equal: %d\r\n",     ipv4AddrToString(current->binding.ipAddr), ipv4AddrToString(fBinding.ipAddr),
//                                                                    current->binding.ipAddr == fBinding->ipAddr);
//        printDebug("Comp mac1 (%s) and mac2 (%s), equal: %d\r\n",     ipv4AddrToString(current->binding.ipAddr), ipv4AddrToString(fBinding.ipAddr),
//                                                                    macCompAddrcurrent->binding.ipAddr == fBinding->ipAddr);
//        
        if( current->binding.ipAddr == fBinding->ipAddr && 
            macCompAddr(&current->binding.macAddr, &fBinding->macAddr)) return current;
        
        current = current->next;
    }
    
    return NULL;
}
