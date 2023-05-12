#ifndef _W5500_H
#define _W5500_H
                        
typedef enum  
{ 
    MACRAW_Socket_Init = 0,     
    MACRAW_Socket_Idle,
    MACRAW_Socket_Send,
    MACRAW_Socket_Receive,
    MACRAW_Socket_Reset
} macrawSocketStateEnum;


void init_W5500(void);
void process_W5500 (void);
void process_W5500Int();

// Definitions in W5500_Debug.c
void printNetworkInfo (void);

#endif