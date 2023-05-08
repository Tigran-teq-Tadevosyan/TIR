#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <stdio.h>
#include <string.h>

#include "definitions.h"                // SYS function prototypes

#include "W5500/W5500.h"
#include "Common/Common.h"
#include "Common/Debug.h"
#include "Network/DHCP/DHCP_Server.h"

uint8_t uartTxBuffer[256] = {0};
uint8_t uartRxBuffer[256] = {0};
uint16_t uartRxBufferIn = 0;

uint8_t uart2RxBuffer[6000] = {0};
uint16_t uart2RxBufferIn = 0;
uint16_t uart2RxBufferOut = 0;

bool macPctStartFound;
uint8_t *macPctStart_p;

uint16_t macPctSize;
extern const uint8_t startDel[4];

static volatile bool readNetworkInfo = false;

static void SwitchEventHandler(GPIO_PIN pin, uintptr_t contextHandle);
static void Uart2RxEventHandler(UART_EVENT event, uintptr_t contextHandle);

uint16_t parseUartMessage(uint8_t * rxbuff, uint16_t sz);

int main ( void )
{   
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    
    // Turning off all LEDs 
    LED_RR_Set();
    LED_GG_Set();
    LED_BB_Set();
    
    init_SysClock();
    init_Debug();

    // Registering the callbacks and enabling interrupts for the 'buttons' on the 'PIC32 MZ DA Curiosity' board
    GPIO_PinInterruptCallbackRegister(SW_1_PIN, SwitchEventHandler, NO_CONTEXT);
    GPIO_PinInterruptCallbackRegister(SW_2_PIN, SwitchEventHandler, NO_CONTEXT);
    GPIO_PinInterruptCallbackRegister(SW_3_PIN, SwitchEventHandler, NO_CONTEXT);
    GPIO_PinInterruptCallbackRegister(SW_4_PIN, SwitchEventHandler, NO_CONTEXT);
    GPIO_PinIntEnable(SW_1_PIN, GPIO_INTERRUPT_ON_BOTH_EDGES);
    GPIO_PinIntEnable(SW_2_PIN, GPIO_INTERRUPT_ON_BOTH_EDGES);
    GPIO_PinIntEnable(SW_3_PIN, GPIO_INTERRUPT_ON_BOTH_EDGES);
    GPIO_PinIntEnable(SW_4_PIN, GPIO_INTERRUPT_ON_BOTH_EDGES);
    
    // Setting up UART2: Connected to pair board
    UART2_ReadCallbackRegister(Uart2RxEventHandler, NO_CONTEXT);
    UART2_ReadNotificationEnable(true, true);
    UART2_ReadThresholdSet(0);
           
    init_W5500();
    
//    uint16_t UAR4_readByteCount = 0;
    
    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks();
        process_W5500();
        dhcpServerMaintanance();
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}

static void Uart2RxEventHandler(UART_EVENT event, uintptr_t contextHandle)
{
   if(event == UART_EVENT_READ_THRESHOLD_REACHED)
   {
        
   }
}

uint16_t parseUartMessage(uint8_t * rxbuff, uint16_t sz)
{
    char cmd, pl[200];
    int ret;
    char * cmdBreak;
     
    if(sz > 200)
    {
        // Check Command Length
        printDebug("Too Long Command\r\n");
        return 0;
    }
    cmdBreak = strchr((char *)rxbuff, '\n');
    
    if(cmdBreak == NULL) return sz;
    
    ret = sscanf((char *)rxbuff, "-%c%*[ ]%s\n", &cmd, pl);
    if(ret >= 1)
    {
        switch(cmd)
        {
            case 'c':
                // Connect Command
                if(ret == 2)
                {
                    uint8_t ipToConnect[4];
                    uint16_t tcpPort;

                    if(sscanf((char *)pl, "%hhu.%hhu.%hhu.%hhu:%hu",    &ipToConnect[0], 
                                                                        &ipToConnect[1],
                                                                        &ipToConnect[2],
                                                                        &ipToConnect[3],
                                                                        &tcpPort) == 5)
                    {                        
//                        connectTCPSocket(ipToConnect, tcpPort);
                        
                        return 0;
                    }
                }
                break;    
            case 'd':
            // Disconnect Command
                printDebug("Disconnect\r\n");
//                disconnectTCPSocket();
                return 0;
                break;
            case 's':
            // Send Data Command
                printDebug("Send Data\r\n");
                if(ret == 2)
                {    
//                    sendDataTCPSocket((uint8_t *)pl, strlen(pl));
                    return 0;
                }
                break;
            default:
                break;      
        }
    }
    else
    {
        printDebug("Wrong Format\r\n");
        return 0;            
    }

    printDebug("Wrong Command\r\n");
    return 0;        
}

static void SwitchEventHandler(GPIO_PIN pin, uintptr_t contextHandle)
{
    (void)contextHandle;
    if(pin == SW_1_PIN) {
        if(SW_1_Get() == 0) {
            printDebug("Hello Sailor\r\n");
        } else {}
    } else if(pin == SW_2_PIN) {
        if(SW_2_Get() == 0) {
            send_pkt();
        } else { }
    } else if(pin == SW_3_PIN) {
        if(SW_3_Get() == 0) {
            LED_GG_Toggle();
        } else { }    
    } else if(pin == SW_4_PIN) {
        if(SW_4_Get() == 0) {
        } else { }    
    } 
}