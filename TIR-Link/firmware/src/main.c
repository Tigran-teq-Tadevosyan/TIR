/*******************************************************************************
  Main Source File

  Company:
    Microchip Technology Inc.

  File Name:
    main.c

  Summary:
    This file contains the "main" function for a project.

  Description:
    This file contains the "main" function for a project.  The
    "main" function calls the "SYS_Initialize" function to initialize the state
    machines of all modules in the system
 *******************************************************************************/

// *****************************************************************************
// *****************************************************************************
// Section: Included Files
// *****************************************************************************
// *****************************************************************************

#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include <stdio.h>
#include <string.h>
#include "definitions.h"                // SYS function prototypes

#include "W5500/W5500_app.h"
#include "utilities/utilities.h"     

#define TEMP_SENSOR_SLAVE_ADDR                  0x18
#define TEMP_SENSOR_REG_ADDR                    0x05
static volatile bool isTemperatureRead = false;

uint64_t system_tick_us;
uint32_t delay_tick_us;

static uint8_t temperatureVal;
static uint8_t i2cWrData = TEMP_SENSOR_REG_ADDR;
static uint8_t i2cRdData[2] = {0};

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
static void UartRxEventHandler(UART_EVENT event, uintptr_t contextHandle);
static void Uart2RxEventHandler(UART_EVENT event, uintptr_t contextHandle);

static void Timer1Handler(uint32_t status, uintptr_t contextHandle);
static void i2cEventHandler(uintptr_t contextHandle);
static void MCP9808TempSensorInit(void);
static uint8_t getTemperature(uint8_t* rawTempValue);

uint16_t parseUartMessage(uint8_t * rxbuff, uint16_t sz);

// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

int main ( void )
{
    uint16_t sz;
    
    /* Initialize all modules */
    SYS_Initialize ( NULL );
    
    LED_RR_Set();
    LED_GG_Set();
    LED_BB_Set();

    GPIO_PinInterruptCallbackRegister(SW_1_PIN, SwitchEventHandler, 0);
    GPIO_PinInterruptCallbackRegister(SW_2_PIN, SwitchEventHandler, 0);
    GPIO_PinInterruptCallbackRegister(SW_3_PIN, SwitchEventHandler, 0);
    GPIO_PinInterruptCallbackRegister(SW_4_PIN, SwitchEventHandler, 0);
    GPIO_PinIntEnable(SW_1_PIN, GPIO_INTERRUPT_ON_BOTH_EDGES);
    GPIO_PinIntEnable(SW_2_PIN, GPIO_INTERRUPT_ON_BOTH_EDGES);
    GPIO_PinIntEnable(SW_3_PIN, GPIO_INTERRUPT_ON_BOTH_EDGES);
    GPIO_PinIntEnable(SW_4_PIN, GPIO_INTERRUPT_ON_BOTH_EDGES);
    
    UART4_ReadCallbackRegister(UartRxEventHandler, 0);
    UART4_ReadNotificationEnable(true, true);
    UART4_ReadThresholdSet(0);
    
    UART2_ReadCallbackRegister(Uart2RxEventHandler, 0);
    UART2_ReadNotificationEnable(true, true);
    UART2_ReadThresholdSet(0);
    
    TMR1_CallbackRegister(Timer1Handler, 0);
    TMR1_Start();
    I2C1_CallbackRegister(i2cEventHandler, 0);
    MCP9808TempSensorInit();
    
    W5500_Init();
    uint16_t rxRdy = 0;
    while ( true )
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
        
        processW5500();
        rxRdy = UART4_ReadCountGet();
        if(rxRdy > 0)
        {
//            rxRdy = UART4_Read(uartRxBuffer, rxRdy);
//            rxRdy = UART2_Write(uartRxBuffer, rxRdy);
        
//            rxRdy = UART4_Read(uartRxBuffer + uartRxBufferIn, rxRdy);
//            UART4_Write(uartRxBuffer + uartRxBufferIn, rxRdy);
//            uartRxBufferIn += rxRdy;  
//            uartRxBuffer[uartRxBufferIn] = 0x00;
//            uartRxBufferIn = parseUartMessage(uartRxBuffer, uartRxBufferIn);
        
        }
        
        rxRdy = UART2_ReadCountGet();
        if(rxRdy > 0)
        {
            if(uart2RxBufferIn > 3000)
            {
                printDebug("UART to much data Size = %u\r\n", uart2RxBufferIn);
                uart2RxBufferIn = 0;
                
                macPctStartFound = false;
                uart2RxBufferOut = 0; 
            }
                
            rxRdy = UART2_Read(uart2RxBuffer + uart2RxBufferIn, rxRdy);
            uart2RxBufferIn += rxRdy;
            
            // Find The Start of IP Packet
            if(macPctStartFound == false)
            {
                for(uint16_t i = 0; i < uart2RxBufferIn; i++)
                {
                    if(uart2RxBuffer[i] == startDel[0] && i < uart2RxBufferIn - 6)
                    {
                        if(uart2RxBuffer[i + 1] == startDel[1] && uart2RxBuffer[i + 2] == startDel[2] && uart2RxBuffer[i + 3] == startDel[3])
                        {
                            // This is the start
                            macPctStartFound = true;
                            macPctSize = (uart2RxBuffer[i + 5] << 8)  +  uart2RxBuffer[i + 4];
                            printDebug("UART MACRAW Size = %u\r\n", macPctSize);
                            macPctStart_p = uart2RxBuffer + i + 6;
                            uart2RxBufferOut = i + 6;
                            break;
                        }
                    }
                    else
                    {
                    //wait for start
                    }

                }
            }
            
            if(macPctStartFound == true)
            {
                if(uart2RxBufferIn - uart2RxBufferOut >= macPctSize)
                {
                    printDebug("UART MACRAW Sent\r\n", macPctSize);

                    sendDataMACrawSocketDirect(macPctStart_p, macPctSize);
                    
                    if(uart2RxBufferIn - uart2RxBufferOut - macPctSize > 0)
                    {
                        memmove(uart2RxBuffer, macPctStart_p + macPctSize, uart2RxBufferIn - uart2RxBufferOut - macPctSize);
                    }
                    uart2RxBufferIn = uart2RxBufferIn - uart2RxBufferOut - macPctSize;
                    
                    macPctStartFound = false;
                    uart2RxBufferOut = 0; 
                            
                }
            }
            // Find Size of IP Packet
            
            
            //rxRdy = UART4_Write(uartRxBuffer, rxRdy);
        }
        
        if(isTemperatureRead)    
        {
            isTemperatureRead = false;
            getTemperature(i2cRdData);
            temperatureVal = getTemperature(i2cRdData);
            sz = sprintf((char*)(uartTxBuffer), "Temperature = %02d C\r\n", (int)temperatureVal);
            UART4_Write(uartTxBuffer, sz);
            sendDataTCPSocket(uartTxBuffer, sz);
        }
        
        if(readNetworkInfo)
        {
            readNetworkInfo = false;
            printNetworkInfo();
        }
       
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}


static void UartRxEventHandler(UART_EVENT event, uintptr_t contextHandle)
{
   LED_GG_Toggle();
   
   if(event == UART_EVENT_READ_THRESHOLD_REACHED)
   {
        
   }
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
                        connectTCPSocket(ipToConnect, tcpPort);
                        
                        return 0;
                    }
                }
                break;    
            case 'd':
            // Disconnect Command
                printDebug("Disconnect\r\n");
                disconnectTCPSocket();
                return 0;
                break;
            case 's':
            // Send Data Command
                printDebug("Send Data\r\n");
                if(ret == 2)
                {    
                    sendDataTCPSocket((uint8_t *)pl, strlen(pl));
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

    if(pin == SW_1_PIN)
    {
        if(SW_1_Get() == 0)
        {
            uint8_t destIP[4] = {192,168,1,3};
            printDebug("Sending ARP Request for 192.168.1.3\r\n");
            sendArpRequestIprawSocket(destIP);
            
        }
    }
    else if(pin == SW_2_PIN)
    {
        UART4_Write((uint8_t *)"SW2\r\n", 5);
        if(SW_2_Get() == 0)
        {
            readNetworkInfo = true;
        }else
        {

        }
    }
    else if(pin == SW_3_PIN)
    {
        UART4_Write((uint8_t *)"SW3\r\n", 5);
        if(SW_3_Get() == 0)
        {
            isTemperatureRead = false;
            I2C1_WriteRead(TEMP_SENSOR_SLAVE_ADDR, &i2cWrData, 1, i2cRdData, 2);
        }else
        {

        }    
    }
}

static void i2cEventHandler(uintptr_t contextHandle)
{
    if (I2C1_ErrorGet() == I2C_ERROR_NONE)
    {
        isTemperatureRead = true;
    }
}

static void MCP9808TempSensorInit(void)
{
    uint8_t config[3] = {0};
	config[0] = 0x01;
	config[1] = 0x00;
	config[2] = 0x00;
    
    I2C1_Write(TEMP_SENSOR_SLAVE_ADDR, config, 3);
    
    while (isTemperatureRead != true);
    isTemperatureRead = false;
    
    config[0] = 0x08;
	config[1] = 0x03;
	I2C1_Write(TEMP_SENSOR_SLAVE_ADDR, config, 2);
    
    while (isTemperatureRead != true);
    isTemperatureRead = false;    
}

static uint8_t getTemperature(uint8_t* rawTempValue)
{
    
    int temp = ((rawTempValue[0] & 0x1F) * 256 + rawTempValue[1]);
    if(temp > 4095)
    {
        temp -= 8192;
    }
    float cTemp = temp * 0.0625;
//    float fTemp = cTemp * 1.8 + 32;
    return (uint8_t)cTemp;
}


static void Timer1Handler(uint32_t status, uintptr_t contextHandle)
{
    system_tick_us++;
    delay_tick_us++;
}


/*******************************************************************************
 End of File
*/

