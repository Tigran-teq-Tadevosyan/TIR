#include "W5500.h"

#include "./driver/Ethernet/W5500/w5500.h"
#include "Common/Debug.h"

void printNetworkInfo (void)
{
    wiz_NetInfo pnetinfo;
    uint8_t chipVersion;
    
    chipVersion = getVERSIONR();
    wizchip_getnetinfo(&pnetinfo);
    printDebug("CHIP VER. %02x\r\nGW %u.%u.%u.%u \r\n", chipVersion ,
                                                        pnetinfo.gw[0],
                                                        pnetinfo.gw[1],
                                                        pnetinfo.gw[2],
                                                        pnetinfo.gw[3]);
    
    printDebug("MAC  %02x %02x %02x %02x %02x %02x\r\n", (int)pnetinfo.mac[0],
                                                        (int)pnetinfo.mac[1],
                                                        (int)pnetinfo.mac[2],
                                                        (int)pnetinfo.mac[3],
                                                        (int)pnetinfo.mac[4],
                                                        (int)pnetinfo.mac[5]);
    
    printDebug("IP %u.%u.%u.%u\r\n",    pnetinfo.ip[0],
                                        pnetinfo.ip[1],
                                        pnetinfo.ip[2],
                                        pnetinfo.ip[3]);
}