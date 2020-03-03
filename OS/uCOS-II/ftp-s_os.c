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
*                                  FTP SERVER OPERATING SYSTEM LAYER
*
*                                          Micrium uC/OS-II
*
* Filename : ftp-s_os.c
* Version  : V1.98.00
*********************************************************************************************************
* Note(s)  : (1) Assumes uC/OS-II V2.86 (or more recent version) is included in the project build.
*
*            (2) REQUIREs the following uC/OS-II feature(s) to be ENABLED :
*
*                    ------- FEATURE ------        -- MINIMUM CONFIGURATION FOR FTPs/OS PORT --
*
*                (a) Tasks
*                    (1) OS_TASK_DEL_EN            Enabled
*                    (2) OS_TASK_SUSPEND_EN        Enabled
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  "../../Source/ftp-s.h"
#include  <ucos_ii.h>                                           /* See this 'ftp-s_os.c  Note #1'.                      */


/*
*********************************************************************************************************
*                                        DEFAULT CONFIGURATION
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/

                                                                /* See this 'ftp-s_os.c  Note #1'.                      */
#if     (OS_VERSION < 286u)
#error  "OS_VERSION                        [SHOULD be >= V2.86]"
#endif



                                                                /* See this 'ftp-s_os.c  Note #2a'.                     */
#if     (OS_TASK_DEL_EN < 1u)
#error  "OS_TASK_DEL_EN                    illegally #define'd in 'os_cfg.h'            "
#error  "                                  [MUST be  > 0, (see 'ftp-s_os.c  Note #2a1')]"
#endif

#if     (OS_TASK_SUSPEND_EN < 1u)
#error  "OS_TASK_SUSPEND_EN                illegally #define'd in 'os_cfg.h'            "
#error  "                                  [MUST be  > 0, (see 'ftp-s_os.c  Note #2a2')]"
#endif




#ifndef  FTPs_OS_CFG_SERVER_TASK_PRIO
#error  "FTPs_OS_CFG_SERVER_TASK_PRIO            not #define'd in 'app_cfg.h'"
#error  "                                  [MUST be  >= 0u]                  "

#elif   (FTPs_OS_CFG_SERVER_TASK_PRIO < 0u)
#error  "FTPs_OS_CFG_SERVER_TASK_PRIO      illegally #define'd in 'app_cfg.h'"
#error  "                                  [MUST be  >= 0u]                  "
#endif


#ifndef  FTPs_OS_CFG_CTRL_TASK_PRIO
#error  "FTPs_OS_CFG_CTRL_TASK_PRIO              not #define'd in 'app_cfg.h'"
#error  "                                  [MUST be  >= 0u]                  "

#elif   (FTPs_OS_CFG_CTRL_TASK_PRIO   < 0u)
#error  "FTPs_OS_CFG_CTRL_TASK_PRIO        illegally #define'd in 'app_cfg.h'"
#error  "                                  [MUST be  >= 0u]                  "
#endif



#ifndef  FTPs_OS_CFG_SERVER_TASK_STK_SIZE
#error  "FTPs_OS_CFG_SERVER_TASK_STK_SIZE        not #define'd in 'app_cfg.h'"
#error  "                                  [MUST be  > 0u]                   "

#elif   (FTPs_OS_CFG_SERVER_TASK_STK_SIZE < 1u)
#error  "FTPs_OS_CFG_SERVER_TASK_STK_SIZE  illegally #define'd in 'app_cfg.h'"
#error  "                                  [MUST be  > 0u]                   "
#endif


#ifndef  FTPs_OS_CFG_CTRL_TASK_STK_SIZE
#error  "FTPs_OS_CFG_CTRL_TASK_STK_SIZE          not #define'd in 'app_cfg.h'"
#error  "                                  [MUST be  > 0u]                   "

#elif   (FTPs_OS_CFG_CTRL_TASK_STK_SIZE   < 1u)
#error  "FTPs_OS_CFG_CTRL_TASK_STK_SIZE    illegally #define'd in 'app_cfg.h'"
#error  "                                  [MUST be  > 0u]                   "
#endif


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                     OS TASK/OBJECT NAME DEFINES
*********************************************************************************************************
*/

                                                                /* -------------------- TASK NAMES -------------------- */
                                          /*           1         2 */
                                          /* 012345678901234567890 */
#define  FTPs_OS_SERVER_TASK_NAME           "FTP (Server)"
#define  FTPs_OS_CTRL_TASK_NAME             "FTP (Control)"

#define  FTPs_OS_OBJ_NAME_SIZE_MAX                        14    /* Maximum of ALL FTPs object name sizes.               */


/*
*********************************************************************************************************
*                                           LOCAL CONSTANTS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                          LOCAL DATA TYPES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                            LOCAL TABLES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

                                                                /* ------------------- TASK STACKS -------------------- */
static  OS_STK  FTPs_OS_ServerTaskStk[FTPs_OS_CFG_SERVER_TASK_STK_SIZE];
static  OS_STK  FTPs_OS_CtrlTaskStk[FTPs_OS_CFG_CTRL_TASK_STK_SIZE];


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

                                                                /* ---------- FTPs TASK MANAGEMENT FUNCTION ----------- */
static  void  FTPs_OS_ServerTask(void  *p_data);

                                                                /* -------- FTPs CTRL TASK MANAGEMENT FUNCTION -------- */
static  void  FTPs_OS_CtrlTask  (void  *p_data);


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           FTPs FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                      FTPs_OS_ServerTaskInit()
*
* Description : (1) Perform FTP server/OS initialization :
*
*                   (a) Create FTP server task
*
*
* Argument(s) : p_data      Pointer to task initialization data (required by uC/OS-II).
*
* Return(s)   : DEF_OK,   if FTP server task successfully created.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : FTPs_Init().
*
*               This function is an INTERNAL FTP server function & MUST NOT be called by application
*               function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  FTPs_OS_ServerTaskInit (void*  p_data)
{
    INT8U  os_err;


                                                                /* Create FTP server task.                              */
#if (OS_TASK_CREATE_EXT_EN > 0u)
    #if (OS_STK_GROWTH == 1u)
    os_err = OSTaskCreateExt((void (*)(void *)) FTPs_OS_ServerTask,
                             (void          * ) p_data,
                             (OS_STK        * )&FTPs_OS_ServerTaskStk[FTPs_OS_CFG_SERVER_TASK_STK_SIZE - 1],    /* Set Top-Of-Stack.    */
                             (INT8U           ) FTPs_OS_CFG_SERVER_TASK_PRIO,
                             (INT16U          ) FTPs_OS_CFG_SERVER_TASK_PRIO,
                             (OS_STK        * )&FTPs_OS_ServerTaskStk[0],                                       /* Set Bottom-Of-Stack. */
                             (INT32U          ) FTPs_OS_CFG_SERVER_TASK_STK_SIZE,
                             (void          * ) 0,                                                              /* No TCB extension.    */
                             (INT16U          ) OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
    #else
    os_err = OSTaskCreateExt((void (*)(void *)) FTPs_OS_ServerTask,
                             (void          * ) p_data,
                             (OS_STK        * )&FTPs_OS_ServerTaskStk[0],                                       /* Set Top-Of-Stack.    */
                             (INT8U           ) FTPs_OS_CFG_SERVER_TASK_PRIO,
                             (INT16U          ) FTPs_OS_CFG_SERVER_TASK_PRIO,
                             (OS_STK        * )&FTPs_OS_ServerTaskStk[FTPs_OS_CFG_SERVER_TASK_STK_SIZE - 1],    /* Set Bottom-Of-Stack. */
                             (INT32U          ) FTPs_OS_CFG_SERVER_TASK_STK_SIZE,
                             (void          * ) 0,                                                              /* No TCB extension.    */
                             (INT16U          ) OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
    #endif
#else
    #if (OS_STK_GROWTH == 1u)
    os_err = OSTaskCreate((void (*)(void *)) FTPs_OS_ServerTask,
                          (void          * ) p_data,
                          (OS_STK        * )&FTPs_OS_ServerTaskStk[FTPs_OS_CFG_SERVER_TASK_STK_SIZE - 1],       /* Set Bottom-Of-Stack. */
                          (INT8U           ) FTPs_OS_CFG_SERVER_TASK_PRIO);
    #else
    os_err = OSTaskCreate((void (*)(void *)) FTPs_OS_ServerTask,
                          (void          * ) p_data,
                          (OS_STK        * )&FTPs_OS_ServerTaskStk[0],                                          /* Set Top-Of-Stack.    */
                          (INT8U           ) FTPs_OS_CFG_SERVER_TASK_PRIO);
    #endif
#endif

    if (os_err != OS_ERR_NONE) {
        return (DEF_FAIL);
    }



#if (((OS_VERSION >= 288u) && (OS_TASK_NAME_EN   >  0u)) || \
     ((OS_VERSION <  288u) && (OS_TASK_NAME_SIZE >= FTPs_OS_OBJ_NAME_SIZE_MAX)))
    OSTaskNameSet((INT8U  ) FTPs_OS_CFG_SERVER_TASK_PRIO,
                  (INT8U *) FTPs_OS_SERVER_TASK_NAME,
                  (INT8U *)&os_err);
#endif


    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                        FTPs_OS_ServerTask()
*
* Description : OS-dependent FTP server task.
*
* Argument(s) : p_data      Pointer to task initialization data (required by uC/OS-II).
*
* Return(s)   : none.
*
* Created by  : FTPs_OS_ServerTaskInit().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FTPs_OS_ServerTask (void  *p_data)
{
    FTPs_ServerTask(p_data);                                    /* Call FTP server task.                                */
}


/*
*********************************************************************************************************
*                                       FTPs_OS_CtrlTaskInit()
*
* Description : (1) Perform FTP control server/OS task initialization :
*
*                   (a) Create FTP control task
*
*
* Argument(s) : p_data      Pointer to task initialization data (required by uC/OS-II).
*
* Return(s)   : DEF_OK,   if FTP control task successfully created.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : FTPs_ServerTask().
*
*               This function is an INTERNAL FTP server function & MUST NOT be called by application
*               function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

CPU_BOOLEAN  FTPs_OS_CtrlTaskInit (void  *p_data)
{
    INT8U  os_err;


                                                                /* Create FTP ctrl task.                                */
#if (OS_TASK_CREATE_EXT_EN > 0u)
    #if (OS_STK_GROWTH == 1u)
    os_err = OSTaskCreateExt((void (*)(void *)) FTPs_OS_CtrlTask,
                             (void          * ) p_data,
                             (OS_STK        * )&FTPs_OS_CtrlTaskStk[FTPs_OS_CFG_CTRL_TASK_STK_SIZE - 1],        /* Set Top-Of-Stack.    */
                             (INT8U           ) FTPs_OS_CFG_CTRL_TASK_PRIO,
                             (INT16U          ) FTPs_OS_CFG_CTRL_TASK_PRIO,
                             (OS_STK        * )&FTPs_OS_CtrlTaskStk[0],                                         /* Set Bottom-Of-Stack. */
                             (INT32U          ) FTPs_OS_CFG_CTRL_TASK_STK_SIZE,
                             (void          * ) 0,                                                              /* No TCB extension.    */
                             (INT16U          ) OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
    #else
    os_err = OSTaskCreateExt((void (*)(void *)) FTPs_OS_CtrlTask,
                             (void          * ) p_data,
                             (OS_STK        * )&FTPs_OS_CtrlTaskStk[0],                                         /* Set Top-Of-Stack.    */
                             (INT8U           ) FTPs_OS_CFG_CTRL_TASK_PRIO,
                             (INT16U          ) FTPs_OS_CFG_CTRL_TASK_PRIO,
                             (OS_STK        * )&FTPs_OS_CtrlTaskStk[FTPs_OS_CFG_CTRL_TASK_STK_SIZE - 1],        /* Set Bottom-Of-Stack. */
                             (INT32U          ) FTPs_OS_CFG_CTRL_TASK_STK_SIZE,
                             (void          * ) 0,                                                              /* No TCB extension.    */
                             (INT16U          ) OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR);
    #endif
#else
    #if (OS_STK_GROWTH == 1u)
    os_err = OSTaskCreate((void (*)(void *)) FTPs_OS_CtrlTask,
                          (void          * ) p_data,
                          (OS_STK        * )&FTPs_OS_CtrlTaskStk[FTPs_OS_CFG_CTRL_TASK_STK_SIZE - 1],           /* Set Bottom-Of-Stack. */
                          (INT8U           ) FTPs_OS_CFG_CTRL_TASK_PRIO);
    #else
    os_err = OSTaskCreate((void (*)(void *)) FTPs_OS_CtrlTask,
                          (void          * ) p_data,
                          (OS_STK        * )&FTPs_OS_CtrlTaskStk[0],                                            /* Set Top-Of-Stack.    */
                          (INT8U           ) FTPs_OS_CFG_CTRL_TASK_PRIO);
    #endif
#endif

    if (os_err != OS_ERR_NONE) {
        return (DEF_FAIL);
    }



#if (((OS_VERSION >= 288u) && (OS_TASK_NAME_EN   >  0u)) || \
     ((OS_VERSION <  288u) && (OS_TASK_NAME_SIZE >= FTPs_OS_OBJ_NAME_SIZE_MAX)))
    OSTaskNameSet((INT8U  ) FTPs_OS_CFG_CTRL_TASK_PRIO,
                  (INT8U *) FTPs_OS_CTRL_TASK_NAME,
                  (INT8U *)&os_err);
#endif


    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                         FTPs_OS_CtrlTask()
*
* Description : OS-dependent FTP control task.
*
* Argument(s) : p_data      Pointer to task initialization data (required by uC/OS-II).
*
* Return(s)   : none.
*
* Created by  : FTPs_OS_CtrlTaskInit().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FTPs_OS_CtrlTask (void  *p_data)
{
    FTPs_CtrlTask(p_data);                                      /* Call FTP ctrl task.                                  */
}


/*
*********************************************************************************************************
*                                        FTPs_OS_TaskSuspend()
*
* Description : Suspend the current FTP task.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : FTPs_ServerTask().
*
*               This function is an INTERNAL FTP server function & MUST NOT be called by application
*               function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FTPs_OS_TaskSuspend (void)
{
    OSTaskSuspend(OS_PRIO_SELF);                                /* Suspend cur FTP task.                                */
}


/*
*********************************************************************************************************
*                                          FTPs_OS_TaskDel()
*
* Description : Delete the current FTP task.
*
* Argument(s) : none.
*
* Return(s)   : none.
*
* Caller(s)   : FTPs_CtrlTask().
*
*               This function is an INTERNAL FTP server function & MUST NOT be called by application
*               function(s).
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FTPs_OS_TaskDel (void)
{
    OSTaskDel(OS_PRIO_SELF);                                    /* Del     cur FTP task.                                */
}

