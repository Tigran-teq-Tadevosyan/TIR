#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <stdio.h>
#include <string.h>

#include <inttypes.h>

#include "definitions.h"                // SYS function prototypes

#include "W5500/W5500.h"
#include "Common/Common.h"
#include "Common/Debug.h"
#include "Network/DHCP/DHCP_Server.h"
#include "InterLink/Interlink.h"
#include "W5500/MACRAW_FrameFIFO.h"

static void SwitchEventHandler(GPIO_PIN pin, uintptr_t contextHandle);

int main(void) {   
    /* Initialize all modules */
    SYS_Initialize(NULL);
    
    // Turning off all LEDs 
    LED_RR_Set();
    LED_GG_Set();
    LED_BB_Set();

    // Registering the callbacks and enabling interrupts for the 'buttons' on the 'PIC32 MZ DA Curiosity' board
    GPIO_PinInterruptCallbackRegister(SW_1_PIN, SwitchEventHandler, NO_CONTEXT);
    GPIO_PinInterruptCallbackRegister(SW_2_PIN, SwitchEventHandler, NO_CONTEXT);
    GPIO_PinInterruptCallbackRegister(SW_3_PIN, SwitchEventHandler, NO_CONTEXT);
    GPIO_PinInterruptCallbackRegister(SW_4_PIN, SwitchEventHandler, NO_CONTEXT);
    GPIO_PinIntEnable(SW_1_PIN, GPIO_INTERRUPT_ON_BOTH_EDGES);
    GPIO_PinIntEnable(SW_2_PIN, GPIO_INTERRUPT_ON_BOTH_EDGES);
    GPIO_PinIntEnable(SW_3_PIN, GPIO_INTERRUPT_ON_BOTH_EDGES);
    GPIO_PinIntEnable(SW_4_PIN, GPIO_INTERRUPT_ON_BOTH_EDGES);
  
    init_SysClock();
    init_Debug();
    init_Interlink();
    init_W5500();
    
    while(true) {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks();
        
        process_W5500();
        
        process_Interlink();
        
        if(dhcpServerRunning()) {
            dhcpServerMaintanance();
        }
    }
    
    /* Execution should not come here during normal operation */
    return ( EXIT_FAILURE );
}

uint32_t rand_num[2];

static void SwitchEventHandler(GPIO_PIN pin, uintptr_t contextHandle) {
    (void)contextHandle;
    if(pin == SW_1_PIN) {
        if(SW_1_Get() == 0) {
            printDebug("Hello Sailor!!\r\n");
        } else { }
    } else if(pin == SW_2_PIN) {
        if(SW_2_Get() == 0) {
        } else { }
    } else if(pin == SW_3_PIN) {
        if(SW_3_Get() == 0) {
        } else { }    
    } else if(pin == SW_4_PIN) {
        if(SW_4_Get() == 0) {
        } else { }    
    } 
}