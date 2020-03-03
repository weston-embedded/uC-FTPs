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
*                                             FTP SERVER
*
* Filename : ftp-s.h
* Version  : V1.98.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                               MODULE
*
* Note(s) : (1) This header file is protected from multiple pre-processor inclusion through use of the
*               FTPs present pre-processor macro definition.
*********************************************************************************************************
*/

#ifndef  FTPs_PRESENT                                           /* See Note #1.                                         */
#define  FTPs_PRESENT


/*
*********************************************************************************************************
*                                         FTPs VERSION NUMBER
*
* Note(s) : (1) (a) The FTPs module software version is denoted as follows :
*
*                       Vx.yy.zz
*
*                           where
*                                   V               denotes 'Version' label
*                                   x               denotes     major software version revision number
*                                   yy              denotes     minor software version revision number
*                                   zz              denotes sub-minor software version revision number
*
*               (b) The FTPs software version label #define is formatted as follows :
*
*                       ver = x.yyzz * 100 * 100
*
*                           where
*                                   ver             denotes software version number scaled as an integer value
*                                   x.yyzz          denotes software version number, where the unscaled integer
*                                                       portion denotes the major version number & the unscaled
*                                                       fractional portion denotes the (concatenated) minor
*                                                       version numbers
*********************************************************************************************************
*/

#define  FTPs_VERSION                                  19800u   /* See Note #1.                                         */


/*
*********************************************************************************************************
*                                               EXTERNS
*********************************************************************************************************
*/

#ifdef   FTPs_MODULE
#define  FTPs_EXT
#else
#define  FTPs_EXT  extern
#endif


/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <cpu.h>                                               /* CPU Configuration             (see Note #2b)         */
#include  <cpu_core.h>                                          /* CPU Core Library              (see Note #2a)         */

#include  <lib_def.h>                                           /* Standard        Defines       (see Note #3a)         */
#include  <lib_str.h>                                           /* Standard String Library       (see Note #3a)         */
#include  <lib_ascii.h>                                         /* Standard ASCII  Library       (see Note #3a)         */

#include  <ftp-s_cfg.h>                                         /* FTP server Configuration File (see Note #1a)         */
#include  <Source/net.h>                                        /* Network Protocol Suite        (see Note #1b)         */
#include  <Source/net_type.h>
#include  <Source/net_sock.h>
#include  <IP/IPv4/net_ipv4.h>
#include  <FS/net_fs.h>                                         /* File System Interface         (see Note #1b)         */

#if 0                                                           /* See Note #3b.                                        */
#include  <stdio.h>
#endif


/*
*********************************************************************************************************
*                                                  FTP
*                                           SPECIFIC DEFINES
*********************************************************************************************************
*/

                                                                /* The maximum number of control tasks supported.       */
#define  FTPs_CTRL_TASKS_MAX                               1    /* Actually 1 (serving 1 client at a time).             */
                                                                /* The maximum number of data transfer tasks supported. */

#define  FTPs_CTRL_CONN_Q_SIZE                             3    /* Control connection queue size.                       */
#define  FTPs_DTP_CONN_Q_SIZE                              1    /* Data transfer protocol connection queue size.        */

#define  FTPs_NET_BUF_LEN                               1460    /* Network buffer length.                               */

#define  FTPs_PATH_SEP_CHAR                     '/'             /* Define the path separator character used FTP.        */

                                                                /* underlying filesystem.                               */
#define  FTPs_ROOT_PATH                         "/"             /* Define the root path name.                           */
#define  FTPs_CURRENT_PATH                      "."             /* Define the current path name.                        */
#define  FTPs_PARENT_PATH                       ".."            /* Define the parent path name.                         */


/*
*********************************************************************************************************
*                                             SERVER STATES
*********************************************************************************************************
*/

#define  FTPs_STATE_LOGOUT                                 0
#define  FTPs_STATE_LOGIN                                  1
#define  FTPs_STATE_GOTUSER                                2
#define  FTPs_STATE_GOTRNFR                                3
#define  FTPs_STATE_GOTREST                                4
#define  FTPs_STATE_MAX                                    5    /* This line MUST be the LAST!                          */


/*
*********************************************************************************************************
*                                             FTP COMMANDS
*********************************************************************************************************
*/

#define  FTP_CMD_NOOP                                      0
#define  FTP_CMD_QUIT                                      1
#define  FTP_CMD_REIN                                      2
#define  FTP_CMD_SYST                                      3
#define  FTP_CMD_FEAT                                      4
#define  FTP_CMD_HELP                                      5
#define  FTP_CMD_USER                                      6
#define  FTP_CMD_PASS                                      7
#define  FTP_CMD_MODE                                      8
#define  FTP_CMD_TYPE                                      9
#define  FTP_CMD_STRU                                     10
#define  FTP_CMD_PASV                                     11
#define  FTP_CMD_PORT                                     12
#define  FTP_CMD_PWD                                      13
#define  FTP_CMD_CWD                                      14
#define  FTP_CMD_CDUP                                     15
#define  FTP_CMD_MKD                                      16
#define  FTP_CMD_RMD                                      17
#define  FTP_CMD_NLST                                     18
#define  FTP_CMD_LIST                                     19
#define  FTP_CMD_RETR                                     20
#define  FTP_CMD_STOR                                     21
#define  FTP_CMD_APPE                                     22
#define  FTP_CMD_REST                                     23
#define  FTP_CMD_DELE                                     24
#define  FTP_CMD_RNFR                                     25
#define  FTP_CMD_RNTO                                     26
#define  FTP_CMD_SIZE                                     27
#define  FTP_CMD_MDTM                                     28
#define  FTP_CMD_PBSZ                                     29
#define  FTP_CMD_PROT                                     30
#define  FTP_CMD_MAX                                      31    /* This line MUST be the LAST!                          */


/*
*********************************************************************************************************
*                                          FTP REPLY MESSAGES
*********************************************************************************************************
*/

#define  FTP_REPLY_OKAYOPENING                             0
#define  FTP_REPLY_OKAY                                    1
#define  FTP_REPLY_SYSTEMSTATUS                            2
#define  FTP_REPLY_FILESTATUS                              3
#define  FTP_REPLY_HELPMESSAGE                             4
#define  FTP_REPLY_SYSTEMTYPE                              5
#define  FTP_REPLY_SERVERREADY                             6
#define  FTP_REPLY_SERVERCLOSING                           7
#define  FTP_REPLY_CLOSINGSUCCESS                          8
#define  FTP_REPLY_ENTERPASVMODE                           9
#define  FTP_REPLY_LOGGEDIN                               10
#define  FTP_REPLY_ACTIONCOMPLETE                         11
#define  FTP_REPLY_PATHNAME                               12
#define  FTP_REPLY_NEEDPASSWORD                           13
#define  FTP_REPLY_NEEDMOREINFO                           14
#define  FTP_REPLY_NOSERVICE                              15
#define  FTP_REPLY_CANTOPENDATA                           16
#define  FTP_REPLY_CLOSEDCONNABORT                        17
#define  FTP_REPLY_PARMSYNTAXERR                          18
#define  FTP_REPLY_CMDNOSUPPORT                           19
#define  FTP_REPLY_CMDBADSEQUENCE                         20
#define  FTP_REPLY_PARMNOSUPPORT                          21
#define  FTP_REPLY_NOTLOGGEDIN                            22
#define  FTP_REPLY_NOTFOUND                               23
#define  FTP_REPLY_ACTIONABORTED                          24
#define  FTP_REPLY_NOSPACE                                25
#define  FTP_REPLY_NAMEERR                                26
#define  FTP_REPLY_PBSZ                                   27
#define  FTP_REPLY_PROT                                   28
#define  FTP_REPLY_MAX                                    29    /* This line MUST be the LAST!                          */

#define  FTP_REPLY_CODE_OKAYOPENING                      150
#define  FTP_REPLY_CODE_OKAY                             200
#define  FTP_REPLY_CODE_SYSTEMSTATUS                     211
#define  FTP_REPLY_CODE_FILESTATUS                       213
#define  FTP_REPLY_CODE_HELPMESSAGE                      214
#define  FTP_REPLY_CODE_SYSTEMTYPE                       215
#define  FTP_REPLY_CODE_SERVERREADY                      220
#define  FTP_REPLY_CODE_SERVERCLOSING                    221
#define  FTP_REPLY_CODE_CLOSINGSUCCESS                   226
#define  FTP_REPLY_CODE_ENTERPASVMODE                    227
#define  FTP_REPLY_CODE_LOGGEDIN                         230
#define  FTP_REPLY_CODE_ACTIONCOMPLETE                   250
#define  FTP_REPLY_CODE_PATHNAME                         257
#define  FTP_REPLY_CODE_NEEDPASSWORD                     331
#define  FTP_REPLY_CODE_NEEDMOREINFO                     350
#define  FTP_REPLY_CODE_NOSERVICE                        421
#define  FTP_REPLY_CODE_CANTOPENDATA                     425
#define  FTP_REPLY_CODE_CLOSEDCONNABORT                  426
#define  FTP_REPLY_CODE_PARMSYNTAXERR                    501
#define  FTP_REPLY_CODE_CMDNOSUPPORT                     502
#define  FTP_REPLY_CODE_CMDBADSEQUENCE                   503
#define  FTP_REPLY_CODE_PARMNOSUPPORT                    504
#define  FTP_REPLY_CODE_NOTLOGGEDIN                      530
#define  FTP_REPLY_CODE_NOTFOUND                         550
#define  FTP_REPLY_CODE_ACTIONABORTED                    551
#define  FTP_REPLY_CODE_NOSPACE                          552
#define  FTP_REPLY_CODE_NAMEERR                          553
#define FTP_REPLY_CODE_PBSZ                              554
#define FTP_REPLY_CODE_PROT                              555


/*
*********************************************************************************************************
*                                           FTP TRANSFER MODE
*********************************************************************************************************
*/

                                                                /* Transfer mode "STREAM" supported only.               */
#define  FTP_MODE_STREAM                        'S'
#define  FTP_MODE_BLOCK                         'B'
#define  FTP_MODE_COMPRESSED                    'C'


/*
*********************************************************************************************************
*                                             FTP DATA TYPE
*********************************************************************************************************
*/

                                                                /* Data type "ASCII" and "IMAGE" supported only.        */
#define  FTP_TYPE_ASCII                         'A'
#define  FTP_TYPE_EBCDIC                        'E'
#define  FTP_TYPE_IMAGE                         'I'
#define  FTP_TYPE_LOCAL                         'L'


/*
*********************************************************************************************************
*                                            FTP ASCII FORM
*********************************************************************************************************
*/

                                                                /* FTP ASCII form "NON_PRINT" supported only.           */
#define  FTP_FORM_NONPRINT                      'N'
#define  FTP_FORM_TELNET                        'T'
#define  FTP_FORM_CARGCTRL                      'C'


/*
*********************************************************************************************************
*                                          FTP DATA STRUCTURE
*********************************************************************************************************
*/

                                                                /* FTP data structure "FILE" supported only.            */
#define  FTP_STRU_FILE                          'F'
#define  FTP_STRU_RECORD                        'R'
#define  FTP_STRU_PAGE                          'P'


/*
*********************************************************************************************************
*                                              DATA TYPES
*********************************************************************************************************
*/

                                                                /* This structure is used to build a table of command   */
                                                                /* codes and their corresponding strings.  The context  */
                                                                /* is the state(s) in which the command is allowed.     */
typedef struct  FTPs_CmdStruct {
           CPU_INT08U    CmdCode;
    const  CPU_CHAR     *CmdStr;
           CPU_BOOLEAN   CmdCntxt[FTPs_STATE_MAX];
} FTPs_CMD_STRUCT;

                                                                /* This structure is used to build a table of reply     */
                                                                /* codes and their corresponding messages.              */
typedef struct  FTPs_ReplyStruct {
           CPU_INT16U    ReplyCode;
    const  CPU_CHAR     *ReplyStr;
} FTPs_REPLY_STRUCT;

                                                                /* A structure of this type is created for each         */
                                                                /* established connection with the FTP server.          */
                                                                /* A pointer to it is passed around for use by the      */
                                                                /* various functions within the server.                 */
typedef  struct  FTPs_SessionStruct {
    NET_SOCK_ID          CtrlSockID;
    CPU_INT08U           CtrlState;
    CPU_INT08U           CtrlCmd;
    CPU_CHAR            *CtrlCmdArgs;

    NET_SOCK_ADDR_IPv4   DtpSockAddr;
    CPU_INT32S           DtpSockID;
    CPU_INT32S           DtpPasvSockID;
    CPU_BOOLEAN          DtpPasv;
    CPU_INT08U           DtpMode;
    CPU_INT08U           DtpType;
    CPU_INT08U           DtpForm;
    CPU_INT08U           DtpStru;
    CPU_INT08U           DtpCmd;
    CPU_INT32U           DtpOffset;

    CPU_CHAR             User[FTPs_CFG_USER_LEN_MAX];
    CPU_CHAR             Pass[FTPs_CFG_PASS_LEN_MAX];

    CPU_CHAR             BasePath[FTPs_CFG_FS_PATH_LEN_MAX];
    CPU_CHAR             RelPath [FTPs_CFG_FS_PATH_LEN_MAX];
    CPU_CHAR             CurEntry[FTPs_CFG_FS_PATH_LEN_MAX];
} FTPs_SESSION_STRUCT;


typedef  struct  FTPs_SecureCfg {
    CPU_CHAR                      *CertPtr;
    CPU_INT32U                     CertLen;
    CPU_CHAR                      *KeyPtr;
    CPU_INT32U                     KeyLen;
    NET_SOCK_SECURE_CERT_KEY_FMT   Fmt;
    CPU_BOOLEAN                    CertChain;
} FTPs_SECURE_CFG;


/*
*********************************************************************************************************
*                                          FUNCTION PROTOTYPES
*********************************************************************************************************
*/

                                                                /* FTPs module initialization.                          */
CPU_BOOLEAN  FTPs_Init         (       NET_IPv4_ADDR     public_addr,
                                       NET_PORT_NBR      public_port,
                                const  FTPs_SECURE_CFG  *p_secure_cfg);

                                                                /* Modify FTP server public addr for passive mode.      */
void         FTPs_SetPublicAddr(       NET_IPv4_ADDR     public_addr,
                                       NET_PORT_NBR      public_port);

                                                                /* Server  task: waits for clients to connect.          */
void         FTPs_ServerTask   (       void             *p_arg);

                                                                /* Control task: control session with the client.       */
void         FTPs_CtrlTask     (       void             *p_arg);


/*
*********************************************************************************************************
*                                          FUNCTION PROTOTYPES
*                               DEFINED IN USER'S APPLICATION (CALLBACKS)
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            FTPs_AuthUser()
*
* Description : Authenticate User.
*
* Argument(s) : ftp_session     structure that contains FTP session states and control data.
*
* Return(s)   : DEF_OK:         authentication succeeded.
*               DEF_FAIL:       authentication failed.
*
* Caller(s)   : FTPs_Ctrl_Task()
*
* Note        : The application SHOULD use the User and Pass fields of the ftp_session structure to
*               authorize or not the FTP client to connect.
*
*               If the FTP client is authorized to connect, the application MUST set two fields of
*               the ftp_session structure: BasePath and RelPath.
*               1. BasePath is defined for each user and represent the higher level of path that the
*                  user can access.  Ex.: if BasePath = "/FTPROOT", when user does "CWD /", the
*                  system will actually points to "/FTPROOT" in the filesystem, but the user see "/".
*               2. BasePath can be set to "/".  With this setting, user will see all the filesystem.
*                  This configuration is, however, insecure if you want to hide some files to user.
*               3. RelPath is the current path that the user sees.  It is concatenated to the BasePath
*                  for filesystem translation.  Ex.: if user types "CWD /test" and base_path is
*                  "/TFTPROOT", rel_path will be "/test", but the directory accessed on filesystem
*                  will be "/TFTPROOT/test".
*
*               BasePath and RelPath MUST be formatted by the following way:
*               1. The path separator is the UNIX '/'.
*               2. The path MUST start by a '/'.
*               3. The path MUST not end by a '/', except for the root path "/".
*
*               BasePath and RelPath MAY be case sensitive, depending on the underlying filesystem.
*               BasePath and RelPath MAY use name lengths according to the underlying filesystem.
*               Paths separators WILL be converted to the underlying filesystem separator by FTPs.
*
*********************************************************************************************************
*/

CPU_BOOLEAN  FTPs_AuthUser(FTPs_SESSION_STRUCT  *ftp_session);


/*
*********************************************************************************************************
*                                       RTOS INTERFACE FUNCTIONS
*                                           (see ftp-s_os.c)
*********************************************************************************************************
*/

CPU_BOOLEAN  FTPs_OS_ServerTaskInit(void  *p_arg);              /* Create server task.                                  */

CPU_BOOLEAN  FTPs_OS_CtrlTaskInit  (void  *p_arg);              /* Create ctrl   task.                                  */

void         FTPs_OS_TaskSuspend   (void);                      /* Suspend   cur task.                                  */

void         FTPs_OS_TaskDel       (void);                      /* Terminate cur task.                                  */


/*
*********************************************************************************************************
*                                              TRACING
*********************************************************************************************************
*/

                                                                /* Trace level, default to TRACE_LEVEL_OFF              */
#ifndef  TRACE_LEVEL_OFF
#define  TRACE_LEVEL_OFF                                 0
#endif

#ifndef  TRACE_LEVEL_INFO
#define  TRACE_LEVEL_INFO                                1
#endif

#ifndef  TRACE_LEVEL_DBG
#define  TRACE_LEVEL_DBG                                 2
#endif

#ifndef  FTPs_TRACE_LEVEL
#define  FTPs_TRACE_LEVEL                       TRACE_LEVEL_OFF
#endif

#ifndef  FTPs_TRACE
#define  FTPs_TRACE                             printf
#endif

#if    ((defined(FTPs_TRACE))       && \
        (defined(FTPs_TRACE_LEVEL)) && \
        (FTPs_TRACE_LEVEL >= TRACE_LEVEL_INFO) )

    #if  (FTPs_TRACE_LEVEL >= TRACE_LEVEL_DBG)
        #define  FTPs_TRACE_DBG(msg)     FTPs_TRACE  msg
    #else
        #define  FTPs_TRACE_DBG(msg)
    #endif

    #define  FTPs_TRACE_INFO(msg)        FTPs_TRACE  msg

#else
    #define  FTPs_TRACE_DBG(msg)
    #define  FTPs_TRACE_INFO(msg)
#endif


/*
*********************************************************************************************************
*                                         CONFIGURATION ERRORS
*********************************************************************************************************
*/
                                                                /* FTP        Control IP port. Default is 21.           */
#ifndef  FTPs_CFG_CTRL_IPPORT
#error  "FTPs_CFG_CTRL_IPPORT                       not #define'd in 'ftp-s_cfg.h'"
#error  "                                     see template file in package        "
#error  "                                     named 'ftp-s_cfg.h'                 "
#endif

                                                                /* FTP        Data    IP port. Default is 20.           */
#ifndef  FTPs_CFG_DATA_IPPORT
#error  "FTPs_CFG_DATA_IPPORT                       not #define'd in 'ftp-s_cfg.h'"
#error  "                                     see template file in package        "
#error  "                                     named 'ftp-s_cfg.h'                 "
#endif

                                                                /* FTP Secure Control IP port. Default is 990.          */
#ifndef  FTPs_CFG_CTRL_IPPORT_SECURE
#error  "FTPs_CFG_CTRL_IPPORT_SECURE                not #define'd in 'ftp-s_cfg.h'"
#error  "                                     see template file in package        "
#error  "                                     named 'ftp-s_cfg.h'                 "
#endif


                                                                /* FTP Secure Data    IP port. Default is 989.          */
#ifndef  FTPs_CFG_DATA_IPPORT_SECURE
#error  "FTPs_CFG_DATA_IPPORT_SECURE                not #define'd in 'ftp-s_cfg.h'"
#error  "                                     see template file in package        "
#error  "                                     named 'ftp-s_cfg.h'                 "
#endif

                                                                /* Maximum inactivity time (ms) on RX.                  */
#ifndef  FTPs_CFG_CTRL_MAX_RX_TIMEOUT_MS
#error  "FTPs_CFG_CTRL_MAX_RX_TIMEOUT_MS             not define'd in 'ftp-s_cfg.h'"
#error  "                                     see template file in package        "
#error  "                                     named 'ftp-s_cfg.h'                 "
#endif
                                                                /* Maximum inactivity time (ms) on ACCEPT.              */
#ifndef  FTPs_CFG_DTP_MAX_ACCEPT_TIMEOUT_MS
#error  "FTPs_CFG_DTP_MAX_ACCEPT_TIMEOUT_MS         not #define'd in 'ftp-s_cfg.h'"
#error  "                                     see template file in package        "
#error  "                                     named 'ftp-s_cfg.h'                 "
#endif

                                                                /* Maximum inactivity time (ms) on CONNECT.             */
#ifndef  FTPs_CFG_DTP_MAX_CONN_TIMEOUT_MS
#error  "FTPs_CFG_DTP_MAX_CONN_TIMEOUT_MS           not #define'd in 'ftp-s_cfg.h'"
#error  "                                     see template file in package        "
#error  "                                     named 'ftp-s_cfg.h'                 "
#endif

                                                                /* Maximum inactivity time (ms) on RX.                  */
#ifndef  FTPs_CFG_DTP_MAX_RX_TIMEOUT_MS
#error  "FTPs_CFG_DTP_MAX_RX_TIMEOUT_MS             not #define'd in 'ftp-s_cfg.h'"
#error  "                                     see template file in package        "
#error  "                                     named 'ftp-s_cfg.h'                 "
#endif


                                                                /* Maximum number of retries on ACCEPT.                 */
#ifndef  FTPs_CFG_DTP_MAX_ACCEPT_RETRY
#error  "FTPs_CFG_DTP_MAX_ACCEPT_RETRY              not #define'd in 'ftp-s_cfg.h'"
#error  "                                     see template file in package        "
#error  "                                     named 'ftp-s_cfg.h'                 "
#endif

                                                                /* Maximum number of retries on TX.                     */
#ifndef  FTPs_CFG_DTP_MAX_TX_RETRY
#error  "FTPs_CFG_DTP_MAX_TX_RETRY                  not #define'd in 'ftp-s_cfg.h'"
#error  "                                     see template file in package        "
#error  "                                     named 'ftp-s_cfg.h'                 "
#endif

                                                                /* Maximum length for user name.                        */
#ifndef  FTPs_CFG_USER_LEN_MAX
#error  "FTPs_CFG_USER_LEN_MAX                      not #define'd in 'ftp-s_cfg.h'"
#error  "                                     see template file in package        "
#error  "                                     named 'ftp-s_cfg.h'                 "
#endif

                                                                /* Maximum length for password.                         */
#ifndef  FTPs_CFG_PASS_LEN_MAX
#error  "FTPs_CFG_PASS_LEN_MAX                      not #define'd in 'ftp-s_cfg.h'"
#error  "                                     see template file in package        "
#error  "                                     named 'ftp-s_cfg.h'                 "
#endif


#if     (FTPs_OS_CFG_SERVER_TASK_PRIO <= NET_OS_CFG_IF_TX_DEALLOC_TASK_PRIO)
#error  "FTPs_OS_CFG_SERVER_TASK_PRIO         illegally #define'd in 'net_cfg.h'             "
#error  "                                     [MUST be >  NET_OS_CFG_IF_TX_DEALLOC_TASK_PRIO]"
#endif

#if     (FTPs_OS_CFG_CTRL_TASK_PRIO <= NET_OS_CFG_IF_TX_DEALLOC_TASK_PRIO)
#error  "FTPs_OS_CFG_CTRL_TASK_PRIO           illegally #define'd in 'net_cfg.h'             "
#error  "                                     [MUST be >  NET_OS_CFG_IF_TX_DEALLOC_TASK_PRIO]"
#endif

#if 0
#if     (FTPs_OS_CFG_SERVER_TASK_PRIO >= NET_OS_CFG_IF_RX_TASK_PRIO)
#error  "FTPs_OS_CFG_SERVER_TASK_PRIO         illegally #define'd in 'net_cfg.h'     "
#error  "                                     [MUST be < NET_OS_CFG_IF_RX_TASK_PRIO]"
#endif

#if     (FTPs_OS_CFG_CTRL_TASK_PRIO >= NET_OS_CFG_IF_RX_TASK_PRIO)
#error  "FTPs_OS_CFG_CTRL_TASK_PRIO           illegally #define'd in 'net_cfg.h'     "
#error  "                                     [MUST be < NET_OS_CFG_IF_RX_TASK_PRIO]"
#endif
#endif

/*
*********************************************************************************************************
*                                    NETWORK CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                             MODULE END
*********************************************************************************************************
*/

#endif                                                          /* End of FTPs module include.                          */

