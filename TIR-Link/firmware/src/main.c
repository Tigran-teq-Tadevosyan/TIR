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
  
//    RNG_TrngEnable(); 
    
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
            char str[] = "c string";
            send_InterLink(HANDSHAKE_ACK, (uint8_t*)str, strlen(str));
        } else { }
    } else if(pin == SW_3_PIN) {
        if(SW_3_Get() == 0) {
            LED_GG_Toggle();
            
            RNG_WaitForTrngCnt();
            rand_num[0] = RNG_Seed1Get();
            rand_num[1] = RNG_Seed2Get();
            printDebug("\r\n\r\nSeed available in TRNG 0x%x%x", rand_num[1], rand_num[0]);

            /* Prepare PRNG to get seed from TRNG */
            RNG_LoadSet();
            RNG_Poly1Set(0x00C00003);
            RNG_PrngEnable();
            /* Wait for at least 64 clock cycles */
            CORETIMER_DelayUs(1);
            rand_num[0] = RNG_NumGen1Get();
            rand_num[1] = RNG_NumGen2Get();
            RNG_PrngDisable();
            printDebug("\r\nGenerated 64-bit Pseudo-Random Number 0x%x%x\r\n", rand_num[1], rand_num[0]);
            /* Wait till SW1 is released */
            
        } else { }    
    } else if(pin == SW_4_PIN) {
        if(SW_4_Get() == 0) {
        } else { }    
    } 
}