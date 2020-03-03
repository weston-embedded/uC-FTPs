/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                        uC/FTPs EXAMPLE CODE
*
* Filename : ftp-s_init.c
* Version  : V1.98.00
*********************************************************************************************************
* Note(s)  : (1) Assumes the following versions (or more recent) of software modules are included in
*                the project build :
*
*                (a) uC/FTPs   V1.97
*                (b) uC/TCP-IP V3.00
*                (c) uC/OS-II  V2.90 or
*                    uC/OS-III V3.01
*                (d) uC/KAL    V1.00
*********************************************************************************************************
*/

#include  <lib_def.h>
#include  <lib_str.h>
#include  <Source/ftp-s.h>
#include  <Source/net.h>
#include  <Source/net_ascii.h>
#include  <IP/IPv4/net_ipv4.h>

/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                         FTPs_AppInitServer()
*
* Description : Initialize FTP server
*
* Arguments   : p_host_addr         Pointer to a string that contains the IP address of the local host.
*
*               passive_port_nbr    Port number to use when is passive mode.
*
* Returns     : DEF_OK,   Server successfully initialized.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : FTPs_ProcessCtrlCmd().
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  FTPs_AppInitServer (CPU_CHAR    *p_host_addr,
                                 CPU_INT16U   passive_port_nbr)
{
    NET_IPv4_ADDR  addr;
    CPU_BOOLEAN    result;
    NET_ERR        net_err;


    addr = NetASCII_Str_to_IPv4(p_host_addr,
                               &net_err);
    if (net_err != NET_ASCII_ERR_NONE) {
        return (DEF_FAIL);
    }


    result = FTPs_Init(addr, passive_port_nbr, DEF_NULL);

    return (result);
}
