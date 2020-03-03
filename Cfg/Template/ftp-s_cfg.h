/*
*********************************************************************************************************
*                                               uC/FTPs
*                                   File Transfer Protocol (server)
*
*                    Copyright 2004-2020 Silicon Laboratories Inc. www.silabs.com
*
*                                 SPDX-License-Identifier: APACHE-2.0
*
*               This software is subject to an open source license and is distributed by
*                Silicon Laboratories Inc. pursuant to the terms of the Apache License,
*                    Version 2.0 available at www.apache.org/licenses/LICENSE-2.0.
*
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                    FTP SERVER CONFIGURATION FILE
*
*                                              TEMPLATE
*
* Filename : ftp-s_cfg.h
* Version  : V1.98.00
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                           TASKS PRIORITIES
*
* Notes: (1) Task priorities configuration values should be used by the FTP OS port. The following task priorities
*            should be defined:
*
*                   FTPs_OS_CFG_SERVER_TASK_PRIO
*                   FTPs_OS_CFG_CTRL_TASK_PRIO
*
*            Task priorities can be defined either in this configuration file 'ftp-s_cfg.h' or in a global
*            OS tasks priorities configuration header file which must be included in 'ftp-s_cfg.h'.
*********************************************************************************************************
*/

                                                                /* See Note #1.                                         */
#define  FTPs_OS_CFG_SERVER_TASK_PRIO                     14
#define  FTPs_OS_CFG_CTRL_TASK_PRIO                       15


/*
*********************************************************************************************************
*                                              STACK SIZES
*                             Size of the task stacks (# of OS_STK entries)
*********************************************************************************************************
*/

#define  FTPs_OS_CFG_SERVER_TASK_STK_SIZE               1024
#define  FTPs_OS_CFG_CTRL_TASK_STK_SIZE                 2048


/*
*********************************************************************************************************
*                                                 FTPs
*********************************************************************************************************
*/

#define  FTPs_CFG_CTRL_IPPORT                             21    /* FTP        Control Port. Default is 21.              */
#define  FTPs_CFG_DATA_IPPORT                             20    /* FTP        Data    Port. Default is 20.              */

#define  FTPs_CFG_CTRL_IPPORT_SECURE                     990    /* FTP Secure Control Port. Default is 990.             */
#define  FTPs_CFG_DATA_IPPORT_SECURE                     989    /* FTP Secure Data    Port. Default is 989.             */


#define  FTPs_CFG_CTRL_MAX_RX_TIMEOUT_MS               30000    /* Maximum inactivity time (ms) on RX.                  */


#define  FTPs_CFG_DTP_MAX_ACCEPT_TIMEOUT_MS             5000    /* Maximum inactivity time (ms) on ACCEPT.              */
#define  FTPs_CFG_DTP_MAX_ACCEPT_RETRY                     3    /* Maximum number of retries on ACCEPT.                 */
#define  FTPs_CFG_DTP_MAX_CONN_TIMEOUT_MS               5000    /* Maximum inactivity time (ms) on CONNECT.             */
#define  FTPs_CFG_DTP_MAX_RX_TIMEOUT_MS                 5000    /* Maximum inactivity time (ms) on RX.                  */
#define  FTPs_CFG_DTP_MAX_TX_RETRY                         3    /* Maximum number of retries on TX.                     */


#define  FTPs_CFG_USER_LEN_MAX                            32    /* Maximum length for user name.                        */
#define  FTPs_CFG_PASS_LEN_MAX                            32    /* Maximum length for password.                         */

#define  FTPs_CFG_FS_PATH_LEN_MAX                        256    /* Maximum length for FS path.                          */
#define  FTPs_CFG_FS_NAME_LEN_MAX                        256    /* Maximum length for file name.                        */

