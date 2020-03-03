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
* Filename : ftp-s.c
* Version  : V1.98.00
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#define    MICRIUM_SOURCE
#define    FTPs_MODULE
#include  "ftp-s.h"
#include  <Source/net_sock.h>
#include  <Source/net_ascii.h>
#include  <Source/net_util.h>
#include  <IP/IPv4/net_ipv4.h>
#include  <KAL/kal.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


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

                                                                /* Used to keep track of the number of control tasks    */
static         CPU_INT32U        FTPs_CtrlTasks;                /* running on the system.  Actually 0 or 1.             */

                                                                /* Used to set the public IP address (the IP address    */
                                                                /* your NAT (router) on internet.  Needed only if you   */
static         NET_IPv4_ADDR       FTPs_PublicAddr;             /* use the FTP passive mode.                            */

                                                                /* Used to set the public IP port (the IP port you have */
                                                                /* opened in your router and redirected to the internal */
                                                                /* IP address and port of the machine on which this     */
                                                                /* server runs.  Needed only if you use the FTP passive */
static         NET_PORT_NBR      FTPs_PublicPort;               /* mode.                                                */


                                                                /* Used to know if the server is initialized in ...     */
                                                                /* secure cfg.                                          */
static  const  FTPs_SECURE_CFG  *FTPs_SecureCfgPtr = (FTPs_SECURE_CFG *)DEF_NULL;

static         CPU_CHAR          FTPs_FS_SepChar;               /* Stores the FS separator char.                        */

static         CPU_CHAR         *FTPs_FullAbsPathPtr;           /* Stores the full   absolute path string.              */

static         CPU_CHAR         *FTPs_FullRelPathPtr;           /* Stores the full   relative path string.              */

static         CPU_CHAR         *FTPs_ParentAbsPathPtr;         /* Stores the parent absolute path string.              */

static         CPU_CHAR         *FTPs_CurEntryPtr;              /* Stores the entry  file name.                         */

static         CPU_CHAR         *FTPs_RenAbsPathPtr;            /* Stores the        absolute entry rename.             */

static         CPU_CHAR         *FTPs_RenRelPathPtr;            /* Stores the        relative entry rename.             */

static         CPU_CHAR         *FTPs_NetBufCtrlCmdPtr;         /* Stores the net buf used in FTPs_ProcessCtrlCmd().    */

static         CPU_CHAR         *FTPs_NetBufCtrlTaskPtr;        /* Stores the net buf used in FTPs_CtrlTask().          */

static         CPU_CHAR         *FTPs_NetBufDtpCmdPtr;          /* Stores the net buf used in FTPs_ProcessDtpCmd().     */

static         CPU_CHAR         *FTPs_NetBufSendReplyPtr;       /* Stores the net buf used in FTPs_SendReply().         */


/*
*********************************************************************************************************
*                                          INITIALIZED DATA
*********************************************************************************************************
*/
                                                                /* This structure is used to build a table of command   */
                                                                /* codes and their corresponding string.  The context   */
                                                                /* is the state(s) in which the command is allowed.     */
static  const  FTPs_CMD_STRUCT  FTPs_Cmd[] = {
                                                 /*   L        L        G        G        G   */
                                                 /*   O        O        O        O        O   */
                                                 /*   G        G        T        T        T   */
                                                 /*   O        I        U        R        R   */
                                                 /*   U        N        S        N        E   */
                                                 /*   T                 E        F        S   */
                                                 /*                     R        R        T   */
    { FTP_CMD_NOOP,  (const  CPU_CHAR *)"NOOP",  { DEF_ON,  DEF_ON,  DEF_ON,  DEF_ON,  DEF_ON  } },
    { FTP_CMD_QUIT,  (const  CPU_CHAR *)"QUIT",  { DEF_ON,  DEF_ON,  DEF_ON,  DEF_ON,  DEF_ON  } },
    { FTP_CMD_REIN,  (const  CPU_CHAR *)"REIN",  { DEF_ON,  DEF_ON,  DEF_ON,  DEF_ON,  DEF_ON  } },
    { FTP_CMD_SYST,  (const  CPU_CHAR *)"SYST",  { DEF_ON,  DEF_ON,  DEF_ON,  DEF_ON,  DEF_ON  } },
    { FTP_CMD_FEAT,  (const  CPU_CHAR *)"FEAT",  { DEF_ON,  DEF_ON,  DEF_ON,  DEF_ON,  DEF_ON  } },
    { FTP_CMD_HELP,  (const  CPU_CHAR *)"HELP",  { DEF_ON,  DEF_ON,  DEF_ON,  DEF_ON,  DEF_ON  } },
    { FTP_CMD_USER,  (const  CPU_CHAR *)"USER",  { DEF_ON,  DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_PASS,  (const  CPU_CHAR *)"PASS",  { DEF_OFF, DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF } },
    { FTP_CMD_MODE,  (const  CPU_CHAR *)"MODE",  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_TYPE,  (const  CPU_CHAR *)"TYPE",  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_STRU,  (const  CPU_CHAR *)"STRU",  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_PASV,  (const  CPU_CHAR *)"PASV",  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_PORT,  (const  CPU_CHAR *)"PORT",  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_PWD,   (const  CPU_CHAR *)"PWD" ,  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_CWD,   (const  CPU_CHAR *)"CWD" ,  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_CDUP,  (const  CPU_CHAR *)"CDUP",  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_MKD,   (const  CPU_CHAR *)"MKD" ,  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_RMD,   (const  CPU_CHAR *)"RMD" ,  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_NLST,  (const  CPU_CHAR *)"NLST",  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_LIST,  (const  CPU_CHAR *)"LIST",  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_RETR,  (const  CPU_CHAR *)"RETR",  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_ON  } },
    { FTP_CMD_STOR,  (const  CPU_CHAR *)"STOR",  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_ON  } },
    { FTP_CMD_APPE,  (const  CPU_CHAR *)"APPE",  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_REST,  (const  CPU_CHAR *)"REST",  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_DELE,  (const  CPU_CHAR *)"DELE",  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_RNFR,  (const  CPU_CHAR *)"RNFR",  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_RNTO,  (const  CPU_CHAR *)"RNTO",  { DEF_OFF, DEF_OFF, DEF_OFF, DEF_ON,  DEF_OFF } },
    { FTP_CMD_SIZE,  (const  CPU_CHAR *)"SIZE",  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_MDTM,  (const  CPU_CHAR *)"MDTM",  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_PBSZ,  (const  CPU_CHAR *)"PBSZ",  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
    { FTP_CMD_PROT,  (const  CPU_CHAR *)"PROT",  { DEF_OFF, DEF_ON,  DEF_OFF, DEF_OFF, DEF_OFF } },
                                                                /* The following line MUST be the LAST!                 */
    { FTP_CMD_MAX,   (const  CPU_CHAR *)"MAX" ,  { DEF_OFF, DEF_OFF, DEF_OFF, DEF_OFF, DEF_OFF } }
};

                                                                /* This table is used to match the incoming reply code  */
                                                                /* to its corresponding message string.                 */
static  const  FTPs_REPLY_STRUCT  FTPs_Reply[] = {
    { FTP_REPLY_CODE_OKAYOPENING,      (const  CPU_CHAR *)"150 File status okay; about to open data connection."            },
    { FTP_REPLY_CODE_OKAY,             (const  CPU_CHAR *)"200 Command okay."                                               },
    { FTP_REPLY_CODE_SYSTEMSTATUS,     (const  CPU_CHAR *)"211-Extensions supported:\n" \
                                                          " REST STREAM\n"              \
                                                          " MDTM\n"                     \
                                                          " SIZE\n"                     \
                                                          "211 End"                                                         },
    { FTP_REPLY_CODE_FILESTATUS,       (const  CPU_CHAR *)"213 File status."                                                },
    { FTP_REPLY_CODE_HELPMESSAGE,      (const  CPU_CHAR *)"214-Commands recognized:\n"                        \
                                                          " NOOP  QUIT  REIN  SYST  FEAT  HELP  USER  PASS\n" \
                                                          " MODE  TYPE  STRU  PASV  PORT  PWD   CWD   CDUP\n" \
                                                          " MKD   RMD   NLST  LIST  RETR  STOR  APPE  REST\n" \
                                                          " DELE  RNFR  RNTO  SIZE  MDTM\n"                   \
                                                          "214 End"                                                         },
    { FTP_REPLY_CODE_SYSTEMTYPE,       (const  CPU_CHAR *)"215 UNIX Type: L8."                                              },
    { FTP_REPLY_CODE_SERVERREADY,      (const  CPU_CHAR *)"220 Service ready for new user."                                 },
    { FTP_REPLY_CODE_SERVERCLOSING,    (const  CPU_CHAR *)"221 Service closing control connection."                         },
    { FTP_REPLY_CODE_CLOSINGSUCCESS,   (const  CPU_CHAR *)"226 Closing data connection."                                    },
    { FTP_REPLY_CODE_ENTERPASVMODE,    (const  CPU_CHAR *)"227 Entering Passive Mode (%u,%u,%u,%u,%u,%u)."                  },
    { FTP_REPLY_CODE_LOGGEDIN,         (const  CPU_CHAR *)"230 User logged in, proceed."                                    },
    { FTP_REPLY_CODE_ACTIONCOMPLETE,   (const  CPU_CHAR *)"250 Requested file action okay, completed."                      },
    { FTP_REPLY_CODE_PATHNAME,         (const  CPU_CHAR *)"257 \"%s\" created."                                             },
    { FTP_REPLY_CODE_NEEDPASSWORD,     (const  CPU_CHAR *)"331 User name okay, need password."                              },
    { FTP_REPLY_CODE_NEEDMOREINFO,     (const  CPU_CHAR *)"350 Requested file action pending further information."          },
    { FTP_REPLY_CODE_NOSERVICE,        (const  CPU_CHAR *)"421 Service not available, closing control connection."          },
    { FTP_REPLY_CODE_CANTOPENDATA,     (const  CPU_CHAR *)"425 Can't open data connection."                                 },
    { FTP_REPLY_CODE_CLOSEDCONNABORT,  (const  CPU_CHAR *)"426 Connection closed; transfer aborted."                        },
    { FTP_REPLY_CODE_PARMSYNTAXERR,    (const  CPU_CHAR *)"501 Syntax error in parameters or arguments."                    },
    { FTP_REPLY_CODE_CMDNOSUPPORT,     (const  CPU_CHAR *)"502 Command not implemented."                                    },
    { FTP_REPLY_CODE_CMDBADSEQUENCE,   (const  CPU_CHAR *)"503 Bad sequence of commands."                                   },
    { FTP_REPLY_CODE_PARMNOSUPPORT,    (const  CPU_CHAR *)"504 Command not implemented for that parameter."                 },
    { FTP_REPLY_CODE_NOTLOGGEDIN,      (const  CPU_CHAR *)"530 Not logged in."                                              },
    { FTP_REPLY_CODE_NOTFOUND,         (const  CPU_CHAR *)"550 Requested action not taken. %s unavailable."                 },
    { FTP_REPLY_CODE_ACTIONABORTED,    (const  CPU_CHAR *)"551 Requested action aborted."                                   },
    { FTP_REPLY_CODE_NOSPACE,          (const  CPU_CHAR *)"552 Requested file action aborted. Exceeded storage allocation." },
    { FTP_REPLY_CODE_NAMEERR,          (const  CPU_CHAR *)"553 Requested action not taken. File name not allowed."          },
    { FTP_REPLY_CODE_PBSZ,             (const  CPU_CHAR *)"200 PBSZ=%s"                                                     },
    { FTP_REPLY_CODE_PROT,             (const  CPU_CHAR *)"200 Protection level set to %s"                                  }
};

                                                                /* Table used to display month name abbreviation in     */
                                                                /* directory listings like the UNIX command ls.         */
static  const  CPU_CHAR *  const  FTPs_Month_Name[] = {
    (const  CPU_CHAR *)"jan",
    (const  CPU_CHAR *)"feb",
    (const  CPU_CHAR *)"mar",
    (const  CPU_CHAR *)"apr",
    (const  CPU_CHAR *)"may",
    (const  CPU_CHAR *)"jun",
    (const  CPU_CHAR *)"jul",
    (const  CPU_CHAR *)"aug",
    (const  CPU_CHAR *)"sep",
    (const  CPU_CHAR *)"oct",
    (const  CPU_CHAR *)"nov",
    (const  CPU_CHAR *)"dec",
};


/*
*********************************************************************************************************
*                                      LOCAL FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  NET_SOCK_ID   FTPs_ServerSockInit(void);

static  void          FTPs_ToFSStylePath (CPU_CHAR              *path);

static  void          FTPs_ToFTPStylePath(CPU_CHAR              *path);

static  CPU_CHAR     *FTPs_FindArg       (CPU_CHAR             **pp_buf);

static  CPU_CHAR     *FTPs_FindFileName  (CPU_CHAR             **pp_buf);

static  CPU_BOOLEAN   FTPs_BuildPath     (CPU_CHAR              *full_abs_path,
                                          CPU_INT32U             full_abs_path_len,
                                          CPU_CHAR              *full_rel_path,
                                          CPU_INT32U             full_rel_path_len,
                                          CPU_CHAR              *parent_abs_path,
                                          CPU_INT32U             parent_abs_path_len,
                                          CPU_CHAR              *cur_entry,
                                          CPU_INT32U             cur_entry_len,
                                          CPU_CHAR              *base_path,
                                          CPU_CHAR              *rel_path,
                                          CPU_CHAR              *new_path);



static  void          FTPs_StartPasvMode (FTPs_SESSION_STRUCT   *ftp_session,
                                          NET_ERR               *p_err);

static  void          FTPs_StopPasvMode  (FTPs_SESSION_STRUCT   *ftp_session);



static  NET_ERR       FTPs_SendReply     (CPU_INT32S             sock_id,
                                          CPU_INT32S             reply_nbr,
                                          CPU_CHAR              *reply_msg);

static  CPU_BOOLEAN   FTPs_Tx            (CPU_INT32S             sock_id,
                                          CPU_CHAR              *net_buf,
                                          CPU_INT16U             net_buf_len,
                                          NET_ERR               *net_err);



static  void          FTPs_ProcessCtrlCmd(FTPs_SESSION_STRUCT   *ftp_session);

static  void          FTPs_ProcessDtpCmd (FTPs_SESSION_STRUCT   *ftp_session);

static  void          FTPs_DtpTask       (void                  *p_arg);


/*
*********************************************************************************************************
*                                     LOCAL CONFIGURATION ERRORS
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                              FTPs_Init()
*
* Description : FTP server initialization.
*
* Argument(s) : public_addr     for passive mode, you have to provide your public IP address (i.e. the
*                               IP address use to reach you over the internet).  It may be your router's
*                               public IP address.
*               public_port     for passive mode, you have to provide the public port you have opened
*                               and routed to this host to the FTPs_DTP_IPPORT port.
*
*               p_secure_cfg    Desired value for server secure mode :
*
*                                   Secure Configuration Pointer    Server operations will     be secured.
*                                   DEF_NULL                        Server operations will NOT be secured.
*
* Return(s)   : DEF_OK,   FTPs successfully initialized.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : Application code.
*
* Note(s)     : (1) Network security manager MUST be available & enabled to initialize the server in
*                   secure mode.
*
*               (2) Secure mode will is NOT supported in active mode.
*********************************************************************************************************
*/

CPU_BOOLEAN  FTPs_Init (       NET_IPv4_ADDR     public_addr,
                               NET_PORT_NBR      public_port,
                        const  FTPs_SECURE_CFG  *p_secure_cfg)
{
    CPU_BOOLEAN  rtn_val;
    CPU_INT32U   path_len_max;
    NET_SOCK_ID  ctrl_sock_id;
    CPU_INT32U   max_path_name_len;
    CPU_SIZE_T   heap_rem_size;
    LIB_ERR      lib_err;
    CPU_SR_ALLOC();


#ifndef  NET_SECURE_MODULE_PRESENT                              /* See Note #1.                                         */
    if (p_secure_cfg != (FTPs_SECURE_CFG *)0) {
        FTPs_TRACE_DBG(("FTPs init failed. Security manager NOT available.\n"));
        return (DEF_FAIL);
    }
#endif

    FTPs_CtrlTasks  = 0;
    FTPs_PublicAddr = public_addr;
    FTPs_PublicPort = public_port;


    path_len_max = NetFS_CfgPathGetLenMax();

    if (FTPs_CFG_FS_PATH_LEN_MAX > path_len_max) {
        FTPs_TRACE_DBG(("FTPs init failed. FTPs_CFG_FS_PATH_LEN_MAX is larger than the File System allows.\n"));
        return (DEF_FAIL);
    }

    FTPs_FS_SepChar = NetFS_CfgPathGetSepChar();

    max_path_name_len = NetFS_CfgPathGetLenMax();

    heap_rem_size = Mem_HeapGetSizeRem(sizeof(CPU_CHAR),
                                      &lib_err);

    if ((heap_rem_size  > max_path_name_len) &&
        (lib_err       == LIB_MEM_ERR_NONE)) {
        FTPs_FullAbsPathPtr = (CPU_CHAR *)Mem_HeapAlloc(max_path_name_len + 1,
                                                        sizeof(CPU_CHAR),
                                                        0,
                                                       &lib_err);

        if (lib_err != LIB_MEM_ERR_NONE) {
            FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
            return (DEF_FAIL);
        }

    } else {
        FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
        return (DEF_FAIL);
    }

    heap_rem_size = Mem_HeapGetSizeRem(sizeof(CPU_CHAR),
                                      &lib_err);

    if ((heap_rem_size  > max_path_name_len) &&
        (lib_err       == LIB_MEM_ERR_NONE)) {
        FTPs_FullRelPathPtr = (CPU_CHAR *)Mem_HeapAlloc(max_path_name_len + 1,
                                                        sizeof(CPU_CHAR),
                                                        0,
                                                       &lib_err);

        if (lib_err != LIB_MEM_ERR_NONE) {
            FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
            return (DEF_FAIL);
        }

    } else {
        FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
        return (DEF_FAIL);
    }

    heap_rem_size = Mem_HeapGetSizeRem(sizeof(CPU_CHAR),
                                      &lib_err);

    if ((heap_rem_size  > max_path_name_len) &&
        (lib_err       == LIB_MEM_ERR_NONE)) {
        FTPs_ParentAbsPathPtr = (CPU_CHAR *)Mem_HeapAlloc(max_path_name_len + 1,
                                                          sizeof(CPU_CHAR),
                                                          0,
                                                         &lib_err);

        if (lib_err != LIB_MEM_ERR_NONE) {
            FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
            return (DEF_FAIL);
        }

    } else {
        FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
        return (DEF_FAIL);
    }

    heap_rem_size = Mem_HeapGetSizeRem(sizeof(CPU_CHAR),
                                      &lib_err);

    if ((heap_rem_size  > max_path_name_len) &&
        (lib_err       == LIB_MEM_ERR_NONE)) {
        FTPs_CurEntryPtr = (CPU_CHAR *)Mem_HeapAlloc(max_path_name_len + 1,
                                                     sizeof(CPU_CHAR),
                                                     0,
                                                    &lib_err);

        if (lib_err != LIB_MEM_ERR_NONE) {
            FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
            return (DEF_FAIL);
        }

    } else {
        FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
        return (DEF_FAIL);
    }

    heap_rem_size = Mem_HeapGetSizeRem(sizeof(CPU_CHAR),
                                      &lib_err);

    if ((heap_rem_size  > max_path_name_len) &&
        (lib_err       == LIB_MEM_ERR_NONE)) {
        FTPs_RenAbsPathPtr = (CPU_CHAR *)Mem_HeapAlloc(max_path_name_len + 1,
                                                       sizeof(CPU_CHAR),
                                                       0,
                                                      &lib_err);

        if (lib_err != LIB_MEM_ERR_NONE) {
            FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
            return (DEF_FAIL);
        }

    } else {
        FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
        return (DEF_FAIL);
    }

    heap_rem_size = Mem_HeapGetSizeRem(sizeof(CPU_CHAR),
                                      &lib_err);

    if ((heap_rem_size  > max_path_name_len) &&
        (lib_err       == LIB_MEM_ERR_NONE)) {
        FTPs_RenRelPathPtr = (CPU_CHAR *)Mem_HeapAlloc(max_path_name_len + 1,
                                                       sizeof(CPU_CHAR),
                                                       0,
                                                      &lib_err);

        if (lib_err != LIB_MEM_ERR_NONE) {
            FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
            return (DEF_FAIL);
        }

    } else {
        FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
        return (DEF_FAIL);
    }

    heap_rem_size = Mem_HeapGetSizeRem(sizeof(CPU_CHAR),
                                      &lib_err);

    if ((heap_rem_size >= FTPs_NET_BUF_LEN) &&
        (lib_err       == LIB_MEM_ERR_NONE)) {
        FTPs_NetBufCtrlCmdPtr = (CPU_CHAR *)Mem_HeapAlloc(FTPs_NET_BUF_LEN,
                                                   sizeof(CPU_CHAR),
                                                   0,
                                                  &lib_err);

        if (lib_err != LIB_MEM_ERR_NONE) {
            FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
            return (DEF_FAIL);
        }

    } else {
        FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
        return (DEF_FAIL);
    }

    heap_rem_size = Mem_HeapGetSizeRem(sizeof(CPU_CHAR),
                                      &lib_err);

    if ((heap_rem_size >= FTPs_NET_BUF_LEN) &&
        (lib_err       == LIB_MEM_ERR_NONE)) {
        FTPs_NetBufCtrlTaskPtr = (CPU_CHAR *)Mem_HeapAlloc(FTPs_NET_BUF_LEN,
                                                       sizeof(CPU_CHAR),
                                                       0,
                                                      &lib_err);

        if (lib_err != LIB_MEM_ERR_NONE) {
            FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
            return (DEF_FAIL);
        }

    } else {
        FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
        return (DEF_FAIL);
    }

    heap_rem_size = Mem_HeapGetSizeRem(sizeof(CPU_CHAR),
                                      &lib_err);

    if ((heap_rem_size >= FTPs_NET_BUF_LEN) &&
        (lib_err       == LIB_MEM_ERR_NONE)) {
        FTPs_NetBufDtpCmdPtr = (CPU_CHAR *)Mem_HeapAlloc(FTPs_NET_BUF_LEN,
                                                         sizeof(CPU_CHAR),
                                                         0,
                                                        &lib_err);

        if (lib_err != LIB_MEM_ERR_NONE) {
            FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
            return (DEF_FAIL);
        }

    } else {
        FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
        return (DEF_FAIL);
    }

    heap_rem_size = Mem_HeapGetSizeRem(sizeof(CPU_CHAR),
                                      &lib_err);

    if ((heap_rem_size >= FTPs_NET_BUF_LEN) &&
        (lib_err       == LIB_MEM_ERR_NONE)) {
        FTPs_NetBufSendReplyPtr = (CPU_CHAR *)Mem_HeapAlloc(FTPs_NET_BUF_LEN,
                                                            sizeof(CPU_CHAR),
                                                            0,
                                                           &lib_err);

        if (lib_err != LIB_MEM_ERR_NONE) {
            FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
            return (DEF_FAIL);
        }

    } else {
        FTPs_TRACE_DBG(("FTPs init failed. Memory heap size insufficient.\n"));
        return (DEF_FAIL);
    }


    if (p_secure_cfg != DEF_NULL) {
#ifdef  NET_SECURE_MODULE_PRESENT                               /* See Note #1.                                         */

        if (p_secure_cfg->CertPtr == DEF_NULL) {
            FTPs_TRACE_DBG(("FTPs init failed. Invalid Certificate pointer.\n"));
            return (DEF_FAIL);
        }

        if (p_secure_cfg->CertLen == 0u) {
            FTPs_TRACE_DBG(("FTPs init failed. Invalid Certificate lenght.\n"));
            return (DEF_FAIL);
        }

        if (p_secure_cfg->KeyPtr == DEF_NULL) {
            FTPs_TRACE_DBG(("FTPs init failed. Invalid Key pointer.\n"));
            return (DEF_FAIL);
        }

        if (p_secure_cfg->KeyLen == 0u) {
            FTPs_TRACE_DBG(("FTPs init failed. Invalid Key lenght.\n"));
            return (DEF_FAIL);
        }

        CPU_CRITICAL_ENTER();
        FTPs_SecureCfgPtr = p_secure_cfg;                       /* Save secure mode cfg.                                */
        CPU_CRITICAL_EXIT();
#else
        FTPs_TRACE_DBG(("FTPs init failed. Security NOT available.\n"));
        return (DEF_FAIL);
#endif
    }

    FTPs_TRACE_INFO(("FTPs INIT Control socket.\n"));
    ctrl_sock_id = FTPs_ServerSockInit();
    if (ctrl_sock_id == NET_SOCK_ID_NONE) {
        FTPs_TRACE_DBG(("FTPs FTPs_SockCtrlInit() failed.\n"));
        return (DEF_FAIL);
    }


    FTPs_TRACE_INFO(("FTPs CREATE SERVER Task.\n"));
    rtn_val = FTPs_OS_ServerTaskInit((void *)&ctrl_sock_id);
    if (rtn_val == DEF_FAIL) {
        FTPs_TRACE_DBG(("FTPs FTPs_Srv_OS_TaskCreate() failed.\n"));
    }

    return (rtn_val);
}

/*
*********************************************************************************************************
*                                        FTPs_SetPublicAddr()
*
* Description : Set public ip address and port use by FTP server for passive mode.
*
* Argument(s) : public_addr     for passive mode, you have to provide your public IP address (i.e. the
*                               IP address use to reach you over the internet).  It may be your router's
*                               public IP address.
*               public_port     for passive mode, you have to provide the public port you have opened
*                               and routed to this host to the FTPs_DTP_IPPORT port.
*
* Return(s)   : none.
*
* Caller(s)   : (1) 'FTPs_Public' variables MUST ALWAYS be accessed exclusively in critical sections.
*
* Note(s)     : none.
*********************************************************************************************************
*/

void  FTPs_SetPublicAddr (NET_IPv4_ADDR   public_addr,
                          NET_PORT_NBR  public_port)
{
    CPU_SR_ALLOC();


    CPU_CRITICAL_ENTER();
    FTPs_PublicAddr = public_addr;
    FTPs_PublicPort = public_port;
    CPU_CRITICAL_EXIT();
}


/*
*********************************************************************************************************
*                                        FTPs_ServerSockInit()
*
* Description : Initialize server socket.
*
* Argument(s) : none.
*
* Return(s)   : Server socket ID, if successfully initialized.
*               NET_SOCK_ID_NONE, otherwise.
*
* Caller(s)   : FTPs_Init(),
*               FTPs_ServerTask().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  NET_SOCK_ID  FTPs_ServerSockInit (void)
{
    NET_SOCK_ID         server_sock_id;
    NET_SOCK_ADDR_IPv4  server_addr_ip;
    NET_ERR             net_err;


                                                                /* Open a socket.                                       */
    FTPs_TRACE_INFO(("FTPs OPEN SRV socket.\n"));
    server_sock_id = NetSock_Open(NET_SOCK_ADDR_FAMILY_IP_V4,
                                  NET_SOCK_TYPE_STREAM,
                                  NET_SOCK_PROTOCOL_TCP,
                                 &net_err);
    if (net_err != NET_SOCK_ERR_NONE) {
        FTPs_TRACE_DBG(("FTPs NetSock_Open() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
        NetSock_Close(server_sock_id, &net_err);
        return (NET_SOCK_ID_NONE);
    }

#ifdef  NET_SECURE_MODULE_PRESENT                               /* Set sock secure mode.                                */
    if (FTPs_SecureCfgPtr != DEF_NULL) {
       (void)NetSock_CfgSecure(server_sock_id,
                               DEF_YES,
                              &net_err);
        if (net_err != NET_SOCK_ERR_NONE) {
            FTPs_TRACE_INFO(("FTPs NetSock_Open() failed: No secure socket available.\n"));
            NetSock_Close(server_sock_id, &net_err);
            return (NET_SOCK_ID_NONE);
        }

       (void)NetSock_CfgSecureServerCertKeyInstall(server_sock_id,
                                                   FTPs_SecureCfgPtr->CertPtr,
                                                   FTPs_SecureCfgPtr->CertLen,
                                                   FTPs_SecureCfgPtr->KeyPtr,
                                                   FTPs_SecureCfgPtr->KeyLen,
                                                   FTPs_SecureCfgPtr->Fmt,
                                                   FTPs_SecureCfgPtr->CertChain,
                                                  &net_err);
        if (net_err != NET_SOCK_ERR_NONE) {
            FTPs_TRACE_INFO(("FTPs NetSock_Open() failed: No secure socket available.\n"));
            NetSock_Close(server_sock_id, &net_err);
            return (NET_SOCK_ID_NONE);
        }
    }
#endif

                                                                /* Bind a local address so the clients can send to us.  */
    Mem_Set(&server_addr_ip, (CPU_CHAR)0, sizeof(server_addr_ip));
    server_addr_ip.AddrFamily = NET_SOCK_ADDR_FAMILY_IP_V4;
    server_addr_ip.Addr       = NET_UTIL_HOST_TO_NET_32(NET_IPv4_ADDR_ANY);

    if (FTPs_SecureCfgPtr != DEF_NULL) {                        /* Set the port according to the secure mode cfg.       */
        server_addr_ip.Port = NET_UTIL_HOST_TO_NET_16(FTPs_CFG_CTRL_IPPORT_SECURE);
    } else {
        server_addr_ip.Port = NET_UTIL_HOST_TO_NET_16(FTPs_CFG_CTRL_IPPORT);
    }

    NetSock_Bind(                  server_sock_id,
                 (NET_SOCK_ADDR *)&server_addr_ip,
                                   NET_SOCK_ADDR_SIZE,
                                  &net_err);
    if (net_err != NET_SOCK_ERR_NONE) {
        FTPs_TRACE_DBG(("FTPs NetSock_Bind() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
        NetSock_Close(server_sock_id, &net_err);
        return (NET_SOCK_ID_NONE);
    }


    NetSock_Listen(server_sock_id,                              /* Listen to the socket for clients.                    */
                   FTPs_CTRL_CONN_Q_SIZE,
                  &net_err);
    if (net_err != NET_SOCK_ERR_NONE) {
        FTPs_TRACE_DBG(("FTPs NetSock_Listen() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
        NetSock_Close(server_sock_id, &net_err);
        return (NET_SOCK_ID_NONE);
    }

    return (server_sock_id);
}


/*
*********************************************************************************************************
*                                          FTPs_ServerTask()
*
* Description : FTP server task.
*
* Argument(s) : p_arg       argument passed to the task (server socket id).
*
* Return(s)   : none.
*
* Caller(s)   : FTPs_OS_ServerTask().
*
* Note(s)     : This task uses a socket already initialized.  If a connection request is received and there
*               is currently no other active connection, the control task is started and interaction with
*               the client begins.  If a connection request is received and there is currently an active
*               connection, then a reply code indicating this is sent, and the client is denied access.
*********************************************************************************************************
*/

void  FTPs_ServerTask (void  *p_arg)
{
    NET_SOCK_ID        *p_sock_id;
    NET_SOCK_ID         srv_sock_id;
    NET_SOCK_ID         ctrl_sock_id;
    NET_SOCK_ADDR       client_addr;
    NET_SOCK_ADDR_LEN   client_addr_len;
    CPU_BOOLEAN         rtn_val;
    NET_ERR             net_err;


    p_sock_id   = (NET_SOCK_ID*)p_arg;
    srv_sock_id = *p_sock_id;

    while (DEF_TRUE) {
                                                            /* When a client make a request, accept it and create a */
                                                            /* new socket for it.                                   */
                                                            /* Wait on socket, accept with timeout.                 */
        FTPs_TRACE_INFO(("FTPs ACCEPT CTRL socket.\n"));
        client_addr_len = (NET_SOCK_ADDR_LEN)sizeof(client_addr);
        ctrl_sock_id    =  NetSock_Accept(srv_sock_id,
                                         &client_addr,
                                         &client_addr_len,
                                         &net_err);
        switch (net_err) {
            case NET_SOCK_ERR_NONE:
                 if (FTPs_CtrlTasks >= FTPs_CTRL_TASKS_MAX) {
                                                                /* If a control process is already active, then tell    */
                                                                /* this client that the service is not available now.   */
                     FTPs_SendReply(ctrl_sock_id, FTP_REPLY_NOSERVICE, (CPU_CHAR *)0);
                     NetSock_Close(ctrl_sock_id, &net_err);
                     break;
                 }

                                                                /* Create a task for FTP session control and pass       */
                                                                /* socket ID as argument to the task.                   */
                 FTPs_TRACE_INFO(("FTPs CREATE CTRL task.\n"));
                 rtn_val = FTPs_OS_CtrlTaskInit((void*)&ctrl_sock_id);
                 if (rtn_val == DEF_FAIL) {
                     FTPs_SendReply(ctrl_sock_id, FTP_REPLY_NOSERVICE, (CPU_CHAR *)0);
                     NetSock_Close(ctrl_sock_id, &net_err);
                 }
                 break;


            case NET_SOCK_ERR_NONE_AVAIL:
            case NET_SOCK_ERR_CONN_ACCEPT_Q_NONE_AVAIL:
            case NET_SOCK_ERR_CONN_SIGNAL_TIMEOUT:
            case NET_ERR_FAULT_LOCK_ACQUIRE:
                 break;                                         /* Ignore transitory socket error.                      */


            case NET_ERR_INIT_INCOMPLETE:
            case NET_SOCK_ERR_NULL_PTR:
            case NET_SOCK_ERR_NOT_USED:
            case NET_SOCK_ERR_INVALID_SOCK:
            case NET_SOCK_ERR_INVALID_TYPE:
            case NET_SOCK_ERR_INVALID_FAMILY:
            case NET_SOCK_ERR_INVALID_STATE:
            case NET_SOCK_ERR_INVALID_OP:
            case NET_SOCK_ERR_CONN_FAIL:
            default:
                 FTPs_TRACE_DBG(("FTPs NetSock_Accept() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
                 NetSock_Close(srv_sock_id, &net_err);
                 srv_sock_id = FTPs_ServerSockInit();
                 if (srv_sock_id == NET_SOCK_ID_NONE) {
                     FTPs_OS_TaskSuspend();
                 }
                 break;
        }
    }
}


/*
*********************************************************************************************************
*                                           FTPs_CtrlTask()
*
* Description : FTP control task.
*
* Argument(s) : p_arg       argument passed to the task (cast to control socket ID).
*
* Return(s)   : none.
*
* Caller(s)   : FTPs_OS_CtrlTask().
*
* Note(s)     : This task manages the control connection of FTP.  The control connection is used for the
*               transfer of commands (which describes the functions to be performed) and the replies to
*               these commands.
*
*               Section 5.3.1 (FTP COMMANDS) of RFC959 lists the syntax of each ftp command that can be
*               sent by a client.
*
*               Section 5.1 (MINIMUM IMPLEMENTATION) of RFC959 lists the set of commands that must be
*               supported.
*
*               TYPE - ASCII Non-Print
*               MODE - Stream
*               STRUCTURE - File, Record
*               COMMANDS - USER, QUIT, PORT, TYPE, MODE, STRU, RETR, STOR, NOOP
*********************************************************************************************************
*/

void  FTPs_CtrlTask (void  *p_arg)
{
    FTPs_SESSION_STRUCT   ftp_session;
    NET_SOCK_ID          *p_sock_id;

    CPU_INT32U            net_buf_len;
    CPU_CHAR             *p_net_buf;
    CPU_CHAR             *p_net_buf2;
    CPU_CHAR             *p_cmd;

    NET_ERR               net_err;
    CPU_INT32S            pkt_len;
    CPU_INT16S            cmp_val;
    CPU_INT32U            i;


    FTPs_CtrlTasks++;

    ftp_session.DtpSockAddr.AddrFamily = NET_SOCK_ADDR_FAMILY_IP_V4;
    ftp_session.DtpSockAddr.Addr       = NET_UTIL_HOST_TO_NET_32(NET_IPv4_ADDR_ANY);

    if (FTPs_SecureCfgPtr != DEF_NULL) {                        /* Set the port according to the secure mode cfg.       */
        ftp_session.DtpSockAddr.Port       = NET_UTIL_HOST_TO_NET_16(FTPs_CFG_DATA_IPPORT_SECURE);
    } else {
        ftp_session.DtpSockAddr.Port       = NET_UTIL_HOST_TO_NET_16(FTPs_CFG_DATA_IPPORT);
    }

    p_sock_id                          = (NET_SOCK_ID *)p_arg;
    ftp_session.CtrlSockID             = *p_sock_id;
    ftp_session.CtrlState              =  FTPs_STATE_LOGOUT;
    ftp_session.CtrlCmd                =  FTP_CMD_NOOP;

    ftp_session.DtpSockID              = -1;
    ftp_session.DtpPasv                = DEF_NO;

                                                                /* Defaults specified in RFC959.                        */
    ftp_session.DtpMode                = FTP_MODE_STREAM;
    ftp_session.DtpType                = FTP_TYPE_ASCII;
    ftp_session.DtpForm                = FTP_FORM_NONPRINT;
    ftp_session.DtpStru                = FTP_STRU_FILE;

    ftp_session.DtpCmd                 = FTP_CMD_NOOP;

    ftp_session.DtpOffset              = 0;

    FTPs_SendReply(ftp_session.CtrlSockID, FTP_REPLY_SERVERREADY, (CPU_CHAR *)0);

    while (DEF_TRUE) {
                                                                /* Receive data until NEWLINE and replace it by a NULL. */
        p_net_buf   = FTPs_NetBufCtrlTaskPtr;
        net_buf_len = FTPs_NET_BUF_LEN;

        NetSock_CfgTimeoutRxQ_Set((NET_SOCK_ID  ) ftp_session.CtrlSockID,
                                  (CPU_INT32U   ) FTPs_CFG_CTRL_MAX_RX_TIMEOUT_MS,
                                  (NET_ERR     *)&net_err);

        while (DEF_TRUE) {
            pkt_len = NetSock_RxData( ftp_session.CtrlSockID,
                                      p_net_buf,
                                      net_buf_len,
                                      NET_SOCK_FLAG_NONE,
                                     &net_err);
            if ((net_err != NET_SOCK_ERR_NONE) &&
                (net_err != NET_SOCK_ERR_RX_Q_EMPTY)) {
                FTPs_TRACE_DBG(("FTPs NetSock_RxData() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
                break;
            }
            if (net_err == NET_SOCK_ERR_RX_Q_EMPTY) {
                FTPs_TRACE_DBG(("FTPs NetSock_RxData() timeout, line #%u.\n", (unsigned int)__LINE__));
                break;
            }

            p_net_buf2 = (CPU_CHAR *)Str_Char_N(p_net_buf, pkt_len, '\n');
            if ( p_net_buf2  != (CPU_CHAR *)0) {
                *p_net_buf2   = (CPU_CHAR)0;
                 net_buf_len  = p_net_buf2 - FTPs_NetBufCtrlTaskPtr;
                 break;
            }

            net_buf_len -= pkt_len;
            p_net_buf   += pkt_len;
        }

        if (net_err != NET_SOCK_ERR_NONE) {
            FTPs_StopPasvMode(&ftp_session);
            FTPs_SendReply(ftp_session.CtrlSockID, FTP_REPLY_SERVERCLOSING, (CPU_CHAR *)0);
            break;
        }

                                                                /* Process the received line.                           */
                                                                /*                                                      */
                                                                /* The line will be a command of the format             */
                                                                /* <COMMAND> [<ARG1>] [...]                             */
                                                                /*                                                      */
                                                                /* Where:                                               */
                                                                /* COMMAND is a 3 or 4 letter command.                  */
                                                                /* ARG1 is command specific.                            */
                                                                /* ARG2 is command specific.                            */
                                                                /* ...                                                  */

                                                                /* Find the command.                                    */
        FTPs_TRACE_INFO(("FTPs RX: %s\n", FTPs_NetBufCtrlTaskPtr));

        p_net_buf = FTPs_NetBufCtrlTaskPtr;
        p_cmd     = FTPs_FindArg(&p_net_buf);
        if (*p_cmd == (CPU_CHAR)0) {
            continue;
        }

                                                                /* Convert command to uppercase.                        */
        p_net_buf2 = p_cmd;
        while (*p_net_buf2 != (CPU_CHAR)0) {
            *p_net_buf2 = ASCII_ToUpper(*p_net_buf2);
             p_net_buf2++;
        }

                                                                /* Find the command code.                               */
        i = 0;
        while (FTPs_Cmd[i].CmdCode != FTP_CMD_MAX) {
            cmp_val = Str_Cmp((CPU_CHAR *)p_cmd,
                              (CPU_CHAR *)FTPs_Cmd[i].CmdStr);
            if (cmp_val == 0) {
                ftp_session.CtrlCmd = FTPs_Cmd[i].CmdCode;
                break;
            }
            i++;
        }
        if (FTPs_Cmd[i].CmdCode == FTP_CMD_MAX) {
            FTPs_SendReply(ftp_session.CtrlSockID, FTP_REPLY_CMDNOSUPPORT, (CPU_CHAR *)0);
            continue;
        }

                                                                /* Determine if the command entered is compatible with  */
                                                                /* the current state (context check).                   */
        if (FTPs_Cmd[ftp_session.CtrlCmd].CmdCntxt[ftp_session.CtrlState] == DEF_OFF) {
            if (ftp_session.CtrlState == FTPs_STATE_LOGOUT) {
                FTPs_SendReply(ftp_session.CtrlSockID, FTP_REPLY_NOTLOGGEDIN, (CPU_CHAR *)0);
            } else {
                FTPs_SendReply(ftp_session.CtrlSockID, FTP_REPLY_CMDBADSEQUENCE, (CPU_CHAR *)0);
                ftp_session.CtrlState = FTPs_STATE_LOGIN;
            }
            continue;
        }

        ftp_session.CtrlCmdArgs = p_net_buf;
        FTPs_ProcessCtrlCmd(&ftp_session);

        if (ftp_session.CtrlCmd == FTP_CMD_QUIT) {
            break;
        }
    }

    FTPs_TRACE_INFO(("FTPs CLOSE CTRL socket.\n"));
    NetSock_Close(ftp_session.CtrlSockID, &net_err);

    FTPs_TRACE_INFO(("FTPs DELETE CTRL task.\n"));
    FTPs_CtrlTasks--;
    FTPs_OS_TaskDel();
}


/*
*********************************************************************************************************
*********************************************************************************************************
*                                           LOCAL FUNCTIONS
*********************************************************************************************************
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                         FTPs_ToFSStylePath()
*
* Description : Convert FTP (Unix) style path to FS style path.
*
* Argument(s) : path        path to be converted.
*
* Return(s)   : none.
*
* Caller(s)   : FTPs_ProcessCtrlCmd().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FTPs_ToFSStylePath (CPU_CHAR  *path)
{
    if (FTPs_FS_SepChar != FTPs_PATH_SEP_CHAR) {
        while (*path != (CPU_CHAR)0) {
            if (*path == FTPs_PATH_SEP_CHAR) {
                *path  = FTPs_FS_SepChar;
            }
            path++;
        }
    }
}


/*
*********************************************************************************************************
*                                        FTPs_ToFTPStylePath()
*
* Description : Convert FS style path to FTP (Unix) style path.
*
* Argument(s) : path        path to be converted.
*
* Return(s)   : none.
*
* Caller(s)   : FTPs_ProcessCtrlCmd().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FTPs_ToFTPStylePath (CPU_CHAR  *path)
{
    if (FTPs_FS_SepChar != FTPs_PATH_SEP_CHAR) {
        while (*path != (CPU_CHAR)0) {
            if (*path == FTPs_FS_SepChar) {
                *path  = FTPs_PATH_SEP_CHAR;
            }
            path++;
        }
    }
}


/*
*********************************************************************************************************
*                                            FTPs_FindArg()
*
* Description : Find and tokenize next argument.
*
* Argument(s) : pp_buf      buffer to find next argument.
*
* Return(s)   : Pointer to the found argument.
*
* Caller(s)   : FTPs_ProcessCtrlCmd().
*
* Note(s)     : This function will find the next argument.
*               First, all characters that are verified as space characters by ASCII_IsSpace() are skipped.
*               Second, all characters that are not verified as space characters by ASCII_IsSpace() are
*               considered to be the next argument.  A NULL character is inserted at the end of the argument.
*               The return value is a pointer to the beginning of the argument.  pp_buf is modified and
*               points to the character next to the inserted NULL character (ready to find another
*               argument).  You can call this function over and over to find all arguments.  pp_buf will
*               point to a NULL character when there are no more arguments.
*********************************************************************************************************
*/

static  CPU_CHAR  *FTPs_FindArg (CPU_CHAR  **pp_buf)
{
    CPU_CHAR     *p_arg;
    CPU_CHAR     *p_buf;
    CPU_BOOLEAN   rtn_val;


                                                                /* Find the beginning of the argument.                  */
    p_buf = *pp_buf;
    while (*p_buf != (CPU_CHAR)0) {
        rtn_val = ASCII_IsSpace(*p_buf);
        if (rtn_val == DEF_NO) {
            break;
        }
        p_buf++;
    }
    p_arg = p_buf;

                                                                /* Find the end of the argument.                        */
    while (*p_buf != (CPU_CHAR)0) {
        rtn_val = ASCII_IsSpace(*p_buf);
        if (rtn_val != DEF_NO) {
            break;
        }
        p_buf++;
    }

                                                                /* If not the end of the command line, prepare to fetch */
                                                                /* next parameter.                                      */
    if (*p_buf != (CPU_CHAR)0) {
        *p_buf = (CPU_CHAR)0;
        p_buf++;
    }

    *pp_buf = p_buf;
    return (p_arg);
}


/*
*********************************************************************************************************
*                                          FTPs_FindFileName()
*
* Description : Find file name (last argument).
*
* Argument(s) : pp_buf      buffer to find file name.
*
* Return(s)   : Pointer to the found argument.
*
* Caller(s)   : FTPs_ProcessCtrlCmd().
*
* Note(s)     : This function will find a file name in the string.
*               First, every characters that verifies the ASCII_IsSpace() function are skipped.
*               Second, every characters that verifies the ASCII_IsPrint() function are considered to
*               be the next argument.  NULL character is inserted at the end of the argument.
*********************************************************************************************************
*/

static  CPU_CHAR  *FTPs_FindFileName (CPU_CHAR  **pp_buf)
{
    CPU_CHAR     *p_arg;
    CPU_CHAR     *p_buf;
    CPU_BOOLEAN   rtn_val;


                                                                /* Find the beginning of the argument.                  */
    p_buf = *pp_buf;
    while (*p_buf != (CPU_CHAR)0) {
        rtn_val = ASCII_IsSpace(*p_buf);
        if (rtn_val == DEF_NO) {
            break;
        }
        p_buf++;
    }
    p_arg = p_buf;

                                                                /* Find the end of the argument.                        */
    while (*p_buf != (CPU_CHAR)0) {
        rtn_val = ASCII_IsPrint(*p_buf);
        if (rtn_val == DEF_NO) {
            break;
        }
        p_buf++;
    }

    *p_buf = (CPU_CHAR)0;
    return (p_arg);
}


/*
*********************************************************************************************************
*                                           FTPs_BuildPath()
*
* Description : Do all path name parsing and generation stuff.
*
* Argument(s) : full_abs_path           caller-allocated buffer to contain new fully-qualified path.
*               full_abs_path_len       length of full_abs_path.
*               full_rel_path           caller-allocated buffer to contain new relative path.
*               full_rel_path_len       length of full_rel_path.
*               parent_abs_path         caller-allocated buffer to contain new fully-qualified parent path.
*               parent_abs_path_len     length of parent_abs_path.
*               cur_entry               caller-allocated buffer to contain new directory or file name only.
*               cur_entry_len           length of cur_entry.
*               base_path               user base path.  Relative path will be build from this path.
*               rel_path                current relative path.  This is the path returned by the PWD command.
*               new_path                new directory or file name.
*
* Return(s)   : DEF_OK,   path successfully builded.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : FTPs_Ctrl_Task().
*
* Note(s)     : More explanations about the path generated by this function:
*                   1. base_path is defined for each user and represent the higher level of path that the
*                      user can access.  Ex.: if base_path = "/FTPROOT", when user does "CWD /", the
*                      system will actually points to "/FTPROOT" in the filesystem, but the user see "/".
*                   2. base_path can be set to "/".  With this setting, user will see all the filesystem.
*                      This configuration is, however, insecure if you want to hide some files to user.
*                   3. rel_path is the current path that the user see.  Is is concatenated to the base_path
*                      for filesystem translation.  Ex.: if user types "CWD /test" and base_path is
*                      "/TFTPROOT", rel_path will be "/test", but the directory accessed on filesystem
*                      will be "/TFTPROOT/test".
*                   4. new_path is the directory or file accessed.  Generation of new paths depends
*                      greatly if new_path is a relative or absolute path.  If new_path is an absolute
*                      path, it will be copied directly into full_rel_path.
*                   5. If new_path is a relative path, it will be concatenated with rel_path to create
*                      full_rel_path.
*
*               I will now represent generation of path by arithmetics.  I define '+' as the operator of
*               concatenation, '=' is the operator of affectation, parent() returns the parent
*               directory and entry() returns the file of leaf directory name only.
*
*                   If new_path is an absolute path :
*                       1.  full_rel_path   = new_path
*                       2.  full_abs_path   = base_path + full_rel_path
*                       3.  parent_abs_path = base_path + parent(full_rel_path)
*                       4.  cur_entry       = entry(full_rel_path)
*                   If new_path is a relative path :
*                       if new_path == "."
*                           1.  full_rel_path   = rel_path
*                           2.  full_abs_path   = base_path + full_rel_path
*                           3.  parent_abs_path = base_path + parent(full_rel_path)
*                           4.  cur_entry       = entry(full_rel_path)
*                       if new_path == ".."
*                           2.  full_rel_path   = parent(rel_path)
*                           3.  full_abs_path   = base_path + full_rel_path
*                           4.  parent_abs_path = base_path + parent(full_rel_path)
*                           5.  cur_entry       = entry(full_rel_path)
*                       else
*                           1.  full_rel_path   = rel_path + new_path
*                           2.  full_abs_path   = base_path + full_rel_path
*                           3.  parent_abs_path = base_path + parent(full_rel_path)
*                           4.  cur_entry       = entry(full_rel_path)
*
*               PATH SEPARATOR CHARACTER: FTP uses FTPs_PATH_SEP_CHAR path separator character.
*               This function assumes and generate path with FTPs_PATH_SEP_CHAR character only.
*
*********************************************************************************************************
*/

static  CPU_BOOLEAN  FTPs_BuildPath (CPU_CHAR    *full_abs_path,
                                     CPU_INT32U   full_abs_path_len,
                                     CPU_CHAR    *full_rel_path,
                                     CPU_INT32U   full_rel_path_len,
                                     CPU_CHAR    *parent_abs_path,
                                     CPU_INT32U   parent_abs_path_len,
                                     CPU_CHAR    *cur_entry,
                                     CPU_INT32U   cur_entry_len,
                                     CPU_CHAR    *base_path,
                                     CPU_CHAR    *rel_path,
                                     CPU_CHAR    *new_path)
{
    CPU_INT16S   cmp_val;
    CPU_SIZE_T   len;
    CPU_CHAR    *p_buf;


                                                                /* An empty new_path means current path.                */
    if (*new_path == (CPU_CHAR)0) {
        new_path = ".";
    }

                                                                /* Remove the ending FTPs_PATH_SEP_CHAR, if any.        */
    cmp_val = Str_Cmp(new_path, FTPs_ROOT_PATH);
    if (cmp_val != 0) {
        len = Str_Len(new_path);
        if (new_path[len - 1] == FTPs_PATH_SEP_CHAR) {
            new_path[len - 1] = (CPU_CHAR)0;
        }
    }

                                                                /* Determine if new_path is an absolute path, a re-     */
                                                                /* lative path, FTPs_CURRENT_PATH or FTPs_PARENT_PATH.  */

    if (*new_path == FTPs_PATH_SEP_CHAR) {                      /* Absolute path.                                       */
                                                                /* 1.  full_rel_path   = new_path                       */
        Str_FmtPrint((char *)full_rel_path,
                             full_rel_path_len,
                            "%s",
                             new_path);
    } else {
                                                                /* Current path.                                        */
                                                                /* 1.  full_rel_path   = rel_path                       */
       cmp_val = Str_Cmp(new_path, FTPs_CURRENT_PATH);
        if (cmp_val == 0) {
            Str_FmtPrint((char *)full_rel_path,
                                 full_rel_path_len,
                                "%s",
                                 rel_path);
        } else {
                                                                /* Parent path.                                         */
                                                                /* 1.  full_rel_path   = parent(rel_path)               */
            cmp_val = Str_Cmp(new_path, FTPs_PARENT_PATH);
            if (cmp_val == 0) {
                Str_FmtPrint((char *)full_rel_path,
                                     full_rel_path_len,
                                    "%s",
                                     rel_path);
                p_buf = Str_Char_Last(full_rel_path, FTPs_PATH_SEP_CHAR);
                if (p_buf == full_rel_path) {
                    p_buf++;
                }
                *p_buf = (CPU_CHAR)0;
            } else {
                                                                /* Relative path.                                       */
                                                                /* 1.  full_rel_path   = rel_path + new_path            */
                cmp_val = Str_Cmp(rel_path, FTPs_ROOT_PATH);
                if (cmp_val == 0) {
                    Str_FmtPrint((char *)full_rel_path,
                                         full_rel_path_len,
                                        "%s%s",
                                         rel_path,
                                         new_path);
                } else {
                    Str_FmtPrint((char *)full_rel_path,
                                         full_rel_path_len,
                                        "%s%c%s",
                                         rel_path,
                                         FTPs_PATH_SEP_CHAR,
                                         new_path);
                }
            }
        }
    }

                                                                /* 2.  full_abs_path   = base_path + full_rel_path      */
    cmp_val = Str_Cmp(base_path, FTPs_ROOT_PATH);
    if (cmp_val == 0) {
        Str_FmtPrint((char *)full_abs_path,
                             full_abs_path_len,
                            "%s",
                             full_rel_path);
    } else {
        Str_FmtPrint((char *)full_abs_path,
                             full_abs_path_len,
                            "%s%s",
                             base_path,
                             full_rel_path);
    }

                                                                /* 3.  parent_abs_path = base_path +                    */
                                                                /*                       parent(full_rel_path)          */
    Str_FmtPrint((char *)parent_abs_path,
                         parent_abs_path_len,
                        "%s",
                         full_abs_path);

    p_buf = Str_Char_Last(parent_abs_path, FTPs_PATH_SEP_CHAR);

                                                                /* 4.  cur_entry       = entry(full_rel_path)           */
    Str_FmtPrint((char *)cur_entry,
                         cur_entry_len,
                        "%s",
                         p_buf + 1);

    if (p_buf == parent_abs_path) {
        p_buf++;
    }
    *p_buf = (CPU_CHAR)0;

    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                         FTPs_StartPasvMode()
*
* Description : Start passive mode listening socket.
*
* Argument(s) : ftp_session     structure that contains FTP session states and control data.
*
* Return(s)   : none.
*
* Caller(s)   : FTPs_Ctrl_Task().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FTPs_StartPasvMode (FTPs_SESSION_STRUCT  *ftp_session,
                                  NET_ERR              *p_err)
{
    CPU_INT32S  pasv_sock_id;
    NET_ERR     net_err;


    if (ftp_session->DtpPasv == DEF_NO) {
                                                                /* Open a socket.                                       */
        FTPs_TRACE_INFO(("FTPs OPEN passive DTP socket.\n"));
        pasv_sock_id = NetSock_Open( NET_SOCK_ADDR_FAMILY_IP_V4,
                                     NET_SOCK_TYPE_STREAM,
                                     NET_SOCK_PROTOCOL_TCP,
                                     p_err);
        if (*p_err != NET_SOCK_ERR_NONE) {
            FTPs_TRACE_DBG(("FTPs NetSock_Open() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
            return;
        }

#ifdef  NET_SECURE_MODULE_PRESENT                               /* Set or clear socket secure mode.                     */
        if (FTPs_SecureCfgPtr != DEF_NULL) {
           (void)NetSock_CfgSecure(pasv_sock_id,
                                   DEF_YES,
                                   p_err);
            if (*p_err != NET_SOCK_ERR_NONE) {
                FTPs_TRACE_DBG(("FTPs NetSock_CfgSecure() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
                NetSock_Close(pasv_sock_id, &net_err);
                return;
            }

           (void)NetSock_CfgSecureServerCertKeyInstall(pasv_sock_id,
                                                       FTPs_SecureCfgPtr->CertPtr,
                                                       FTPs_SecureCfgPtr->CertLen,
                                                       FTPs_SecureCfgPtr->KeyPtr,
                                                       FTPs_SecureCfgPtr->KeyLen,
                                                       FTPs_SecureCfgPtr->Fmt,
                                                       FTPs_SecureCfgPtr->CertChain,
                                                       p_err);
            if (*p_err != NET_SOCK_ERR_NONE) {
                FTPs_TRACE_DBG(("FTPs NetSock_CfgSecureServerCertKeyInstall() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
                NetSock_Close(pasv_sock_id, &net_err);
                return;
            }
        }
#endif

                                                                /* Bind a local address so the clients can send to us.  */
        NetSock_Bind(pasv_sock_id,
                     (NET_SOCK_ADDR *)&ftp_session->DtpSockAddr,
                     sizeof(ftp_session->DtpSockAddr),
                     p_err);
        if (*p_err != NET_SOCK_ERR_NONE) {
            NetSock_Close(pasv_sock_id, &net_err);
            FTPs_TRACE_DBG(("FTPs NetSock_Bind() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
            return;
        }

                                                                /* Listen to the socket for clients.                    */
        NetSock_Listen(pasv_sock_id, FTPs_DTP_CONN_Q_SIZE, p_err);
        if (*p_err != NET_SOCK_ERR_NONE) {
            NetSock_Close(pasv_sock_id, &net_err);
            FTPs_TRACE_DBG(("FTPs NetSock_Listen() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
            return;
        }

        ftp_session->DtpPasv       = DEF_YES;
        ftp_session->DtpPasvSockID = pasv_sock_id;

        *p_err = NET_ERR_NONE;
    }
}


/*
*********************************************************************************************************
*                                          FTPs_StopPasvMode()
*
* Description : Stop passive mode listening socket.
*
* Argument(s) : ftp_session     structure that contains FTP session states and control data.
*
* Return(s)   : none.
*
* Caller(s)   : FTPs_Ctrl_Task()
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FTPs_StopPasvMode (FTPs_SESSION_STRUCT  *ftp_session)
{
    NET_ERR  net_err;


    if (ftp_session->DtpPasv == DEF_YES) {
        FTPs_TRACE_INFO(("FTPs CLOSE passive DTP socket.\n"));
        NetSock_Close(ftp_session->DtpPasvSockID, &net_err);
        ftp_session->DtpPasv =  DEF_NO;
    }
}


/*
*********************************************************************************************************
*                                           FTPs_SendReply()
*
* Description : Send reply message to a command.
*
* Argument(s) : sock_id         control socket ID.
*               reply_nbr       reply message number.
*               reply_msg       reply message (set to NULL to send the official message related
*                               to reply_nbr).
*
* Return(s)   : Network error code.
*
* Caller(s)   : FTPs_Ctrl_Task().
*
* Note(s)     : This function is a convenience function to build and send a reply message back to a
*               client.  If the incoming reply_msg string is NULL, then the FTPs_Reply[] table is
*               searched for reply code's corresponding message string.  If the reply_msg string pointer
*               is not NULL, then it is used instead of the default reply message.
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  NET_ERR  FTPs_SendReply (CPU_INT32S   sock_id,
                                 CPU_INT32S   reply_nbr,
                                 CPU_CHAR    *reply_msg)
{
    CPU_INT32S  net_buf_len;
    NET_ERR     net_err;


    if (reply_msg == (CPU_CHAR *)0) {
        net_buf_len = Str_FmtPrint((char *)FTPs_NetBufSendReplyPtr,
                                           FTPs_NET_BUF_LEN,
                                   (char *)"%s\r\n",
                                   (char *)FTPs_Reply[reply_nbr].ReplyStr);
    } else {
        net_buf_len = Str_FmtPrint((char *)FTPs_NetBufSendReplyPtr,
                                           FTPs_NET_BUF_LEN,
                                   (char *)"%s\r\n",
                                           reply_msg);
    }

    FTPs_TRACE_INFO(("FTPs TX: %s", FTPs_NetBufCtrlTaskPtr));

    FTPs_Tx(sock_id, FTPs_NetBufSendReplyPtr, net_buf_len, &net_err);
    if (net_err != NET_SOCK_ERR_NONE) {
        FTPs_TRACE_DBG(("FTPs FTPs_Tx() failed: reply #%d, error #%u, line #%u.\n", reply_nbr, (unsigned int)net_err, (unsigned int)__LINE__));
    }
    return (net_err);
}


/*
*********************************************************************************************************
*                                               FTPs_Tx()
*
* Description : Transmit data to TCP socket, handling transient errors and incomplete buffer transmit.
*
* Argument(s) : sock_id             TCP socket ID.
*               net_buf             buffer to send.
*               net_buf_len         length of buffer to send.
*               net_err             contains error message returned.
*
* Return(s)   : DEF_OK,   data successfully transmitted.
*
*               DEF_FAIL, otherwise.
*
* Caller(s)   : FTPs_SendReply(),
*               FTPs_ProcessDtpCmd().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  CPU_BOOLEAN  FTPs_Tx (CPU_INT32S   sock_id,
                              CPU_CHAR    *net_buf,
                              CPU_INT16U   net_buf_len,
                              NET_ERR     *net_err)
{
    CPU_CHAR     *tx_buf;
    CPU_INT16S    tx_buf_len;
    CPU_INT16S    tx_len;
    CPU_INT16S    tx_len_tot;
    CPU_INT32U    tx_retry_cnt;
    CPU_BOOLEAN   tx_done;
    CPU_BOOLEAN   tx_dly;


    tx_len_tot   = 0;
    tx_retry_cnt = 0;
    tx_done      = DEF_NO;
    tx_dly       = DEF_NO;

    while ((tx_len_tot   <  net_buf_len)               &&       /* While tx tot len < buf len ...                       */
           (tx_retry_cnt <  FTPs_CFG_DTP_MAX_TX_RETRY) &&       /* ... & tx retry   < MAX     ...                       */
           (tx_done      == DEF_NO)) {                          /* ... & tx NOT done;         ...                       */

        if (tx_dly == DEF_YES) {                                /* Dly tx, if req'd.                                    */
            KAL_Dly(100u);
        }

        tx_buf     = net_buf     + tx_len_tot;
        tx_buf_len = net_buf_len - tx_len_tot;
        tx_len     = NetSock_TxData(sock_id,                    /* ... tx data.                                         */
                                    tx_buf,
                                    tx_buf_len,
                                    NET_SOCK_FLAG_NONE,
                                    net_err);
        switch (*net_err) {
            case NET_SOCK_ERR_NONE:
                 if (tx_len > 0) {                              /* If          tx len > 0, ...                          */
                     tx_len_tot += tx_len;                      /* ... inc tot tx len.                                  */
                     tx_dly      = DEF_NO;
                 } else {                                       /* Else dly next tx.                                    */
                     tx_dly      = DEF_YES;
                 }
                 tx_retry_cnt = 0;
                 break;

            case NET_SOCK_ERR_NOT_USED:
            case NET_SOCK_ERR_INVALID_TYPE:
            case NET_SOCK_ERR_INVALID_FAMILY:
            case NET_SOCK_ERR_INVALID_STATE:
                 tx_done = DEF_YES;
                 break;

            case NET_ERR_TX:                                    /* If transitory tx err, ...                            */
            default:
                 tx_dly = DEF_YES;                              /* ... dly next tx.                                     */
                 tx_retry_cnt++;
                 break;
        }
    }

    if (*net_err != NET_SOCK_ERR_NONE) {
        return (DEF_FAIL);
    }
    return (DEF_OK);
}


/*
*********************************************************************************************************
*                                         FTPs_ProcessCtrlCmd()
*
* Description : Core execution routines of the FTP commands.
*
* Argument(s) : ftp_session     structure that contains FTP session states and control data.
*
* Return(s)   : none.
*
* Caller(s)   : FTPs_Ctrl_Task().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FTPs_ProcessCtrlCmd (FTPs_SESSION_STRUCT  *ftp_session)
{
    CPU_CHAR       *p_cmd_arg;
    CPU_CHAR       *p_file_time;

    void           *p_file;
    void           *p_dir;
    NET_FS_ENTRY    dirent;
    CPU_INT32U      path_name_len;

    CPU_INT08U     *p_addr;
    CPU_INT08U     *p_port;

    NET_IPv4_ADDR   addr;
    NET_PORT_NBR    port;

    CPU_INT16S      cmp_val;
    CPU_BOOLEAN     dig;
    CPU_BOOLEAN     rtn_val;
    CPU_INT32U      i;

    NET_ERR         net_err;

    CPU_SR_ALLOC();


    p_dir = (void *)0;

                                                                /* Execute the command.                                 */
    switch (ftp_session->CtrlCmd) {
                                                                /* NOOP:   No operation (keep-alive).                   */
                                                                /* Syntax: NOOP                                         */
        case FTP_CMD_NOOP:
             FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_OKAY, (CPU_CHAR *)0);
             break;

                                                                /* QUIT:   Terminate the FTP session.                   */
                                                                /* Syntax: QUIT                                         */
        case FTP_CMD_QUIT:
             FTPs_StopPasvMode(ftp_session);
             FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_SERVERCLOSING, (CPU_CHAR *)0);
             break;

                                                                /* REIN:   Reinitialize the FTP session.                */
                                                                /* Syntax: REIN                                         */
        case FTP_CMD_REIN:
             FTPs_StopPasvMode(ftp_session);
             ftp_session->CtrlState = FTPs_STATE_LOGOUT;
             ftp_session->DtpMode   = FTP_MODE_STREAM;
             ftp_session->DtpType   = FTP_TYPE_ASCII;
             ftp_session->DtpForm   = FTP_FORM_NONPRINT;
             ftp_session->DtpStru   = FTP_STRU_FILE;
             ftp_session->DtpCmd    = FTP_CMD_NOOP;
             FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_OKAY, (CPU_CHAR *)0);
             break;

                                                                /* SYST:   Get system name.                             */
                                                                /* Syntax: SYST                                         */
        case FTP_CMD_SYST:
             FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_SYSTEMTYPE, (CPU_CHAR *)0);
             break;

                                                                /* FEAT:   Advertise server features.                   */
                                                                /* Syntax: FEAT                                         */
        case FTP_CMD_FEAT:
             FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_SYSTEMSTATUS, (CPU_CHAR *)0);
             break;

                                                                /* HELP:   Advertise server help.                       */
                                                                /* Syntax: HELP                                         */
        case FTP_CMD_HELP:
             FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_HELPMESSAGE, (CPU_CHAR *)0);
             break;

                                                                /* USER:   Set username.                                */
                                                                /* Syntax: USER <username>                              */
        case FTP_CMD_USER:
             p_cmd_arg = FTPs_FindArg(&ftp_session->CtrlCmdArgs);
             if (*p_cmd_arg != (CPU_CHAR)0) {
                 Str_Copy_N(ftp_session->User, p_cmd_arg, sizeof(ftp_session->User));
                 ftp_session->CtrlState = FTPs_STATE_GOTUSER;
                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_NEEDPASSWORD, (CPU_CHAR *)0);
             } else {
                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_PARMSYNTAXERR, (CPU_CHAR *)0);
             }
             break;

                                                                /* PASS:   Enter password.                              */
                                                                /* Syntax: PASS <password>                              */
        case FTP_CMD_PASS:
             p_cmd_arg = FTPs_FindArg(&ftp_session->CtrlCmdArgs);
             if (*p_cmd_arg != (CPU_CHAR)0) {
                 Str_Copy_N(ftp_session->Pass, p_cmd_arg, sizeof(ftp_session->Pass));
                 rtn_val = FTPs_AuthUser(ftp_session);
                 if (rtn_val == DEF_OK) {
                     ftp_session->CtrlState = FTPs_STATE_LOGIN;
                     FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_LOGGEDIN, (CPU_CHAR *)0);
                 } else {
                     ftp_session->CtrlState = FTPs_STATE_LOGOUT;
                     FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_NOTLOGGEDIN, (CPU_CHAR *)0);
                 }
             } else {
                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_PARMSYNTAXERR, (CPU_CHAR *)0);
             }
             break;

                                                                /* MODE:   Set transfer mode (stream, block, compress). */
                                                                /* Syntax: MODE <S|B|C>                                 */
                                                                /* NOTE:   Server supports STREAM mode only.            */
        case FTP_CMD_MODE:
             p_cmd_arg = FTPs_FindArg(&ftp_session->CtrlCmdArgs);
             switch (*p_cmd_arg) {
                 case FTP_MODE_STREAM:
                      ftp_session->DtpMode = FTP_MODE_STREAM;
                      FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_OKAY, (CPU_CHAR *)0);
                      break;

                 case FTP_MODE_BLOCK:
                 case FTP_MODE_COMPRESSED:
                      FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_PARMNOSUPPORT, (CPU_CHAR *)0);
                      break;

                 default:
                      FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_PARMSYNTAXERR, (CPU_CHAR *)0);
                      break;
             }
             break;

                                                                /* TYPE:   Set data representation type.                */
                                                                /* Syntax: TYPE <A|E|I|L> [<N|T|C|bytesize>]            */
                                                                /* NOTE:   Server supports ASCII NON_PRINT or IMAGE.    */
        case FTP_CMD_TYPE:
             p_cmd_arg = FTPs_FindArg(&ftp_session->CtrlCmdArgs);
             switch (*p_cmd_arg) {
                 case FTP_TYPE_ASCII:
                      p_cmd_arg = FTPs_FindArg(&ftp_session->CtrlCmdArgs);
                      switch (*p_cmd_arg) {
                          case (CPU_CHAR)0:
                          case FTP_FORM_NONPRINT:
                               ftp_session->DtpType = FTP_TYPE_ASCII;
                               ftp_session->DtpForm = FTP_FORM_NONPRINT;
                               FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_OKAY, (CPU_CHAR *)0);
                               break;

                          case FTP_FORM_TELNET:
                          case FTP_FORM_CARGCTRL:
                               FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_PARMNOSUPPORT, (CPU_CHAR *)0);
                               break;

                          default:
                               FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_PARMSYNTAXERR, (CPU_CHAR *)0);
                               break;
                      }
                      break;

                 case FTP_TYPE_IMAGE:
                      ftp_session->DtpType = FTP_TYPE_IMAGE;
                      FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_OKAY, (CPU_CHAR *)0);
                      break;

                 case FTP_TYPE_LOCAL:
                 case FTP_TYPE_EBCDIC:
                      FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_PARMNOSUPPORT, (CPU_CHAR *)0);
                      break;

                 default:
                      FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_PARMSYNTAXERR, (CPU_CHAR *)0);
                      break;
             }
             break;

                                                                /* STRU:   Set file structure (file, record, page).     */
                                                                /* Syntax: STRU <F|R|P>                                 */
                                                                /* NOTE:   Server supports FILE structure only.         */
        case FTP_CMD_STRU:
             p_cmd_arg = FTPs_FindArg(&ftp_session->CtrlCmdArgs);
             switch (*p_cmd_arg) {
                 case FTP_STRU_FILE:
                      ftp_session->DtpStru = FTP_STRU_FILE;
                      FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_OKAY, (CPU_CHAR *)0);
                      break;

                 case FTP_STRU_RECORD:
                 case FTP_STRU_PAGE:
                      FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_PARMNOSUPPORT, (CPU_CHAR *)0);
                      break;

                 default:
                      FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_PARMSYNTAXERR, (CPU_CHAR *)0);
                      break;
             }
             break;

                                                                /* PASV:   Enter passive mode.                          */
                                                                /* Syntax: PASV                                         */
        case FTP_CMD_PASV:
             CPU_CRITICAL_ENTER();
             addr = FTPs_PublicAddr;
             port = FTPs_PublicPort;
             CPU_CRITICAL_EXIT();

             ftp_session->DtpSockAddr.Addr = NET_UTIL_HOST_TO_NET_32(NET_IPv4_ADDR_ANY);
             ftp_session->DtpSockAddr.Port = NET_UTIL_HOST_TO_NET_16(port);
             FTPs_StartPasvMode(ftp_session, &net_err);
             if (ftp_session->DtpPasv == DEF_YES) {
                 addr = NET_UTIL_HOST_TO_NET_32(addr);
                 port = NET_UTIL_HOST_TO_NET_16(port);

                 p_addr = (CPU_INT08U *)&addr;
                 p_port = (CPU_INT08U *)&port;
                 Str_FmtPrint((char       *)FTPs_NetBufCtrlCmdPtr,
                                            FTPs_NET_BUF_LEN,
                              (char       *)FTPs_Reply[FTP_REPLY_ENTERPASVMODE].ReplyStr,
                              (unsigned int)p_addr[0],
                              (unsigned int)p_addr[1],
                              (unsigned int)p_addr[2],
                              (unsigned int)p_addr[3],
                              (unsigned int)p_port[0],
                              (unsigned int)p_port[1]);

                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_ENTERPASVMODE, FTPs_NetBufCtrlCmdPtr);
             } else {
                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_CANTOPENDATA, (CPU_CHAR *)0);
             }
             break;

                                                                /* PORT:   Set data port configuration.                 */
                                                                /* Syntax: PORT <ip0, ip1, ip2, ip3, port0, port1>      */
        case FTP_CMD_PORT:
             FTPs_StopPasvMode(ftp_session);
             if (ftp_session->DtpPasv == DEF_NO) {
                 p_addr = (CPU_INT08U *)&ftp_session->DtpSockAddr.Addr;
                 p_port = (CPU_INT08U *)&ftp_session->DtpSockAddr.Port;

                 for (i = 0; i <= 3; i++) {
                     dig = ASCII_IsDig(*ftp_session->CtrlCmdArgs);
                     while ((dig == DEF_NO) && (ftp_session->CtrlCmdArgs != (CPU_CHAR *)0)) {
                         ftp_session->CtrlCmdArgs++;
                         dig = ASCII_IsDig(*ftp_session->CtrlCmdArgs);
                     }
                     p_addr[i] = Str_ParseNbr_Int32U(ftp_session->CtrlCmdArgs, &ftp_session->CtrlCmdArgs, 10);
                 }
                 for (i = 0; i <= 1; i++) {
                     dig = ASCII_IsDig(*ftp_session->CtrlCmdArgs);
                     while ((dig == DEF_NO) && (ftp_session->CtrlCmdArgs != 0)) {
                         ftp_session->CtrlCmdArgs++;
                         dig = ASCII_IsDig(*ftp_session->CtrlCmdArgs);
                     }
                     p_port[i] = Str_ParseNbr_Int32U(ftp_session->CtrlCmdArgs, &ftp_session->CtrlCmdArgs, 10);
                 }

                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_OKAY, (CPU_CHAR *)0);
             } else {
                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_CANTOPENDATA, (CPU_CHAR *)0);
             }
             break;

                                                                /* REST:   Next transfer will start at offset <offset>. */
                                                                /* Syntax: REST <offset>                                */
        case FTP_CMD_REST:
             p_cmd_arg = FTPs_FindArg(&ftp_session->CtrlCmdArgs);
             if (*p_cmd_arg != (CPU_CHAR)0) {
                 ftp_session->DtpOffset = Str_ParseNbr_Int32U(p_cmd_arg, 0, 10);
                 ftp_session->CtrlState = FTPs_STATE_GOTREST;
                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_NEEDMOREINFO, (CPU_CHAR *)0);
             }
             break;

                                                                /* PWD:    Get present working directory.               */
                                                                /* Syntax: PWD                                          */

                                                                /* CWD:    Change working directory.                    */
                                                                /* Syntax: CWD <dirname>                                */

                                                                /* CDUP:   Change to parent directory.                  */
                                                                /* Syntax: CDUP <filename>                              */

                                                                /* MKD:    Create directory.                            */
                                                                /* Syntax: MKD <dirname>                                */

                                                                /* RMD:    Delete directory.                            */
                                                                /* Syntax: RMD <dirname>                                */

                                                                /* NLST:   Get brief directory entries list.            */
                                                                /* Syntax: NLST [<pathname>]                            */

                                                                /* LIST:   Get detailed directory entries list.         */
                                                                /* Syntax: LIST [<pathname>]                            */

                                                                /* RETR:   Retrieve file.                               */
                                                                /* Syntax: RETR <filename>                              */

                                                                /* STOR:   Store file.                                  */
                                                                /* Syntax: STOR <filename>                              */

                                                                /* APPE:   Append file.                                 */
                                                                /* Syntax: APPE <filename>                              */

                                                                /* DELE:   Delete file.                                 */
                                                                /* Syntax: DELE <filename>                              */

                                                                /* RNFR:   Rename file from name.                       */
                                                                /* Syntax: RNFR <filename>                              */

                                                                /* RNTO:   Rename file to name.                         */
                                                                /* Syntax: RNTO <filename>                              */

                                                                /* SIZE:   Get file size.                               */
                                                                /* Syntax: SIZE <filename>                              */

                                                                /* MDTM:   Get file modification date/time.             */
                                                                /* Syntax: MDTM <filename>                              */
        case FTP_CMD_PWD:
        case FTP_CMD_CWD:
        case FTP_CMD_CDUP:
        case FTP_CMD_MKD:
        case FTP_CMD_RMD:
        case FTP_CMD_NLST:
        case FTP_CMD_LIST:
        case FTP_CMD_RETR:
        case FTP_CMD_STOR:
        case FTP_CMD_APPE:
        case FTP_CMD_DELE:
        case FTP_CMD_RNFR:
        case FTP_CMD_RNTO:
        case FTP_CMD_SIZE:
        case FTP_CMD_MDTM:
                                                                /* Parameter handling.                                  */
             if (ftp_session->CtrlCmd == FTP_CMD_PWD) {
                 p_cmd_arg = (CPU_CHAR *)".";
             } else if (ftp_session->CtrlCmd == FTP_CMD_CDUP) {
                 p_cmd_arg = (CPU_CHAR *)"..";
             } else if (ftp_session->CtrlCmd == FTP_CMD_MDTM) {
                                                                /* MDTM command may have one or two arguments           */
                                                                /* 1. If the first argument are a file name, return     */
                                                                /*    file date and time.                               */
                                                                /* 2. If the first argument are a date and time and the */
                                                                /*    second argument are a file name, modify the file  */
                                                                /*    date and time.                                    */
                 p_cmd_arg   = FTPs_FindFileName(&ftp_session->CtrlCmdArgs);
                 p_file_time = FTPs_FindArg(&ftp_session->CtrlCmdArgs);

                                                                /* Check if first argument is a string of 14 digits.    */
                                                                /* The date and time string has exactly 14 digits.      */
                 cmp_val = Str_Len(p_file_time);
                 if (cmp_val == 14) {
                    for (i = 0; i < 14; i++) {
                        dig = ASCII_IsDig(p_file_time[i]);
                        if (dig == DEF_NO) {
                            break;
                        }
                    }
                    if (i == 14) {
                                                                /* Yes: The file name starts after.                     */
                                                                /* No:  The file name IS the first argument.            */
                        p_cmd_arg = FTPs_FindFileName(&ftp_session->CtrlCmdArgs);
                    } else {
                        p_file_time = "";
                    }
                 } else {
                    p_file_time = "";
                 }
             } else {
                 p_cmd_arg = FTPs_FindFileName(&ftp_session->CtrlCmdArgs);
             }

             if (*p_cmd_arg != (CPU_CHAR)0) {
                 FTPs_ToFTPStylePath(p_cmd_arg);
             }

                                                                /* Skip "-xxxx" argument.                               */
             if (*p_cmd_arg == '-') {
                 p_cmd_arg = (CPU_CHAR *)".";
             }

             path_name_len = NetFS_CfgPathGetLenMax();

             rtn_val = FTPs_BuildPath(FTPs_FullAbsPathPtr,   path_name_len,
                                      FTPs_FullRelPathPtr,   path_name_len,
                                      FTPs_ParentAbsPathPtr, path_name_len,
                                      FTPs_CurEntryPtr,      path_name_len,
                                      ftp_session->BasePath,
                                      ftp_session->RelPath,
                                      p_cmd_arg);

             FTPs_ToFSStylePath(FTPs_FullAbsPathPtr);
             FTPs_ToFSStylePath(FTPs_ParentAbsPathPtr);

                                                                /* Verify presence of CurEntry/directory/parent         */
                                                                /* directory.                                           */
             if (rtn_val == DEF_OK) {
                 switch (ftp_session->CtrlCmd) {
                     case FTP_CMD_PWD:
                     case FTP_CMD_CWD:
                     case FTP_CMD_CDUP:
                     case FTP_CMD_RMD:
                     case FTP_CMD_NLST:
                     case FTP_CMD_LIST:
                          p_dir = NetFS_DirOpen(FTPs_FullAbsPathPtr);
                          if (p_dir == (void *)0) {
                              rtn_val = DEF_FAIL;
                          } else {
                              rtn_val = DEF_OK;
                          }
                          NetFS_DirClose(p_dir);
                          break;


                     case FTP_CMD_DELE:
                          p_dir = NetFS_DirOpen(FTPs_ParentAbsPathPtr);
                          if (p_dir == (void *)0) {
                              rtn_val = DEF_FAIL;
                          }
                          NetFS_DirClose(p_dir);
                          break;


                     case FTP_CMD_MKD:
                     case FTP_CMD_RETR:
                     case FTP_CMD_STOR:
                     case FTP_CMD_APPE:
                     case FTP_CMD_RNFR:
                     case FTP_CMD_RNTO:
                     case FTP_CMD_SIZE:
                     case FTP_CMD_MDTM:
                          p_file = NetFS_FileOpen(FTPs_FullAbsPathPtr,
                                                  NET_FS_FILE_MODE_OPEN,
                                                  NET_FS_FILE_ACCESS_RD);
                          if (p_file != (void *)0) {
                              switch (ftp_session->CtrlCmd) {
                                  case FTP_CMD_MKD:
                                  case FTP_CMD_APPE:
                                  case FTP_CMD_RNTO:
                                       rtn_val = DEF_FAIL;
                                       break;

                                  case FTP_CMD_SIZE:
                                       NetFS_FileSizeGet(p_file, &dirent.Size);
                                       break;

                                  case FTP_CMD_MDTM:
                                       NetFS_FileDateTimeCreateGet(p_file, &dirent.DateTimeCreate);
                                       break;

                                  default:
                                       rtn_val = DEF_OK;
                                       break;
                              }
                              NetFS_FileClose(p_file);

                          } else {
                              switch (ftp_session->CtrlCmd) {
                                  case FTP_CMD_MKD:
                                  case FTP_CMD_STOR:
                                  case FTP_CMD_APPE:
                                  case FTP_CMD_RNTO:
                                       rtn_val = DEF_OK;
                                       break;

                                  default:
                                       rtn_val = DEF_FAIL;
                                       break;
                              }
                          }
                          break;


                     default:
                          break;
                 }

                                                                /* Do action.                                           */
                 if (rtn_val == DEF_OK) {
                     switch (ftp_session->CtrlCmd) {
                         case FTP_CMD_PWD:
                              Str_FmtPrint((char *)FTPs_NetBufCtrlCmdPtr,
                                                   FTPs_NET_BUF_LEN,
                                           (char *)"257 \"%s\" is current directory.",
                                                   FTPs_FullRelPathPtr);
                              FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_PATHNAME, FTPs_NetBufCtrlCmdPtr);
                              break;

                         case FTP_CMD_CWD:
                         case FTP_CMD_CDUP:
                              Str_Copy_N(ftp_session->RelPath,  FTPs_FullRelPathPtr, FTPs_CFG_FS_PATH_LEN_MAX);
                              FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_ACTIONCOMPLETE, (CPU_CHAR *)0);
                              break;

                         case FTP_CMD_NLST:
                         case FTP_CMD_LIST:
                         case FTP_CMD_RETR:
                         case FTP_CMD_STOR:
                         case FTP_CMD_APPE:
                              FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_OKAYOPENING, (CPU_CHAR *)0);
                              ftp_session->DtpCmd = ftp_session->CtrlCmd;
                              Str_Copy_N(ftp_session->CurEntry, FTPs_FullAbsPathPtr, FTPs_CFG_FS_PATH_LEN_MAX);
                              FTPs_DtpTask((void *)ftp_session);
                              ftp_session->DtpOffset = 0;
                              ftp_session->CtrlState = FTPs_STATE_LOGIN;
                              break;

                         case FTP_CMD_MKD:
                              rtn_val = NetFS_EntryCreate(FTPs_FullAbsPathPtr, DEF_YES);
                              if (rtn_val == DEF_OK) {
                                  Str_FmtPrint((char *)FTPs_NetBufCtrlCmdPtr,
                                                       FTPs_NET_BUF_LEN,
                                               (char *)FTPs_Reply[FTP_REPLY_PATHNAME].ReplyStr,
                                                       FTPs_FullRelPathPtr);
                                  FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_PATHNAME, FTPs_NetBufCtrlCmdPtr);
                              } else {
                                  Str_FmtPrint((char *)FTPs_NetBufCtrlCmdPtr,
                                                       FTPs_NET_BUF_LEN,
                                               (char *)FTPs_Reply[FTP_REPLY_NOTFOUND].ReplyStr,
                                                       FTPs_FullRelPathPtr);
                                  FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_NOTFOUND, FTPs_NetBufCtrlCmdPtr);
                              }
                              break;

                         case FTP_CMD_RMD:
                              rtn_val = NetFS_EntryDel(FTPs_FullAbsPathPtr, DEF_NO);
                              if (rtn_val == DEF_OK) {
                                  FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_ACTIONCOMPLETE, (CPU_CHAR *)0);
                              } else {
                                  Str_FmtPrint((char *)FTPs_NetBufCtrlCmdPtr,
                                                       FTPs_NET_BUF_LEN,
                                               (char *)FTPs_Reply[FTP_REPLY_NOTFOUND].ReplyStr,
                                                       FTPs_FullRelPathPtr);
                                  FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_NOTFOUND, FTPs_NetBufCtrlCmdPtr);
                              }
                              break;

                         case FTP_CMD_DELE:
                              rtn_val = NetFS_EntryDel(FTPs_FullAbsPathPtr, DEF_YES);
                              if (rtn_val == DEF_OK) {
                                  FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_ACTIONCOMPLETE, (CPU_CHAR *)0);
                              } else {
                                  Str_FmtPrint((char *)FTPs_NetBufCtrlCmdPtr,
                                                       FTPs_NET_BUF_LEN,
                                               (char *)FTPs_Reply[FTP_REPLY_NOTFOUND].ReplyStr,
                                                       FTPs_FullRelPathPtr);
                                  FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_NOTFOUND, FTPs_NetBufCtrlCmdPtr);
                              }
                              break;

                         case FTP_CMD_RNFR:
                              Str_Copy_N(FTPs_RenAbsPathPtr, FTPs_FullAbsPathPtr, FTPs_CFG_FS_PATH_LEN_MAX);
                              Str_Copy_N(FTPs_RenRelPathPtr, FTPs_FullRelPathPtr, FTPs_CFG_FS_PATH_LEN_MAX);
                              ftp_session->CtrlState = FTPs_STATE_GOTRNFR;
                              FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_NEEDMOREINFO, (CPU_CHAR *)0);
                              break;

                         case FTP_CMD_RNTO:
                              rtn_val = NetFS_EntryRename(FTPs_RenAbsPathPtr, FTPs_FullAbsPathPtr);
                              ftp_session->CtrlState = FTPs_STATE_LOGIN;
                              if (rtn_val == DEF_OK) {
                                  FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_ACTIONCOMPLETE, (CPU_CHAR *)0);
                              } else {
                                  Str_FmtPrint((char *)FTPs_NetBufCtrlCmdPtr,
                                                       FTPs_NET_BUF_LEN,
                                               (char *)FTPs_Reply[FTP_REPLY_NOTFOUND].ReplyStr,
                                                       FTPs_RenRelPathPtr);
                                  FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_NOTFOUND, FTPs_NetBufCtrlCmdPtr);
                              }
                              break;

                         case FTP_CMD_SIZE:
                              Str_FmtPrint((char       *)FTPs_NetBufCtrlCmdPtr,
                                                         FTPs_NET_BUF_LEN,
                                           (char       *)"213 %u",
                                           (unsigned int)dirent.Size);
                              FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_FILESTATUS, FTPs_NetBufCtrlCmdPtr);
                              break;

                         case FTP_CMD_MDTM:
                              cmp_val = Str_Cmp(p_file_time, "");
                              if (cmp_val == 0) {
                                                                /* Return file date and time.                           */
                                  Str_FmtPrint((char *)FTPs_NetBufCtrlCmdPtr,
                                                       FTPs_NET_BUF_LEN,
                                               (char *)"213 %04hu%02hu%02hu%02hu%02hu%02hu",
                                                       dirent.DateTimeCreate.Yr,
                                                       dirent.DateTimeCreate.Month,
                                                       dirent.DateTimeCreate.Day,
                                                       dirent.DateTimeCreate.Hr,
                                                       dirent.DateTimeCreate.Min,
                                                       dirent.DateTimeCreate.Sec);
                                  FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_FILESTATUS, FTPs_NetBufCtrlCmdPtr);
                              } else {
                                                                /* Modify file date and time.                           */
                                  Str_FmtScan((char *)p_file_time,
                                                     "%04hu%02hu%02hu%02hu%02hu%02hu",
                                                     (CPU_INT16U *)&dirent.DateTimeCreate.Yr,
                                                     (CPU_INT16U *)&dirent.DateTimeCreate.Month,
                                                     (CPU_INT16U *)&dirent.DateTimeCreate.Day,
                                                     (CPU_INT16U *)&dirent.DateTimeCreate.Hr,
                                                     (CPU_INT16U *)&dirent.DateTimeCreate.Min,
                                                     (CPU_INT16U *)&dirent.DateTimeCreate.Sec);
                                 (void)NetFS_EntryTimeSet(FTPs_FullAbsPathPtr, &dirent.DateTimeCreate);
                                  FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_ACTIONCOMPLETE, (CPU_CHAR *)0);
                              }
                              break;

                         default:
                            break;
                     }
                 } else {
                     Str_FmtPrint((char *)FTPs_NetBufCtrlCmdPtr,
                                          FTPs_NET_BUF_LEN,
                                  (char *)FTPs_Reply[FTP_REPLY_NOTFOUND].ReplyStr,
                                          FTPs_FullRelPathPtr);
                     FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_NOTFOUND, FTPs_NetBufCtrlCmdPtr);
                 }
             } else {
                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_NAMEERR, (CPU_CHAR *)0);
             }
             break;

       case FTP_CMD_PBSZ:
            Str_FmtPrint((char *)FTPs_NetBufCtrlCmdPtr,
                                 FTPs_NET_BUF_LEN,
                         (char *)FTPs_Reply[FTP_REPLY_PBSZ].ReplyStr,
                                 ftp_session->CtrlCmdArgs);
             FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_PBSZ, FTPs_NetBufCtrlCmdPtr);
             break;


       case FTP_CMD_PROT:
             Str_FmtPrint((char *)FTPs_NetBufCtrlCmdPtr,
                                  FTPs_NET_BUF_LEN,
                          (char *)FTPs_Reply[FTP_REPLY_PROT].ReplyStr,
                                  ftp_session->CtrlCmdArgs);
             FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_PROT, FTPs_NetBufCtrlCmdPtr);
             break;


        default:
             FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_CMDNOSUPPORT, (CPU_CHAR *)0);
             break;
    }
}


/*
*********************************************************************************************************
*                                         FTPs_ProcessDtpCmd()
*
* Description : Core execution routines of the FTP data transfer commands.
*
* Argument(s) : ftp_session     structure that contains FTP session states and control data.
*
* Return(s)   : none.
*
* Caller(s)   : FTPs_DtpTask().
*
* Note(s)     : none.
*********************************************************************************************************
*/

static  void  FTPs_ProcessDtpCmd (FTPs_SESSION_STRUCT  *ftp_session)
{
    CPU_INT16S     net_len;
    CPU_SIZE_T     fs_len;
    CPU_INT32U     str_len_ttl;
    CPU_INT32U     str_len;
    CPU_CHAR      *prn_buf;
    CPU_INT16S     prn_buf_len;
    void          *p_file;
    void          *p_dir;
    NET_FS_ENTRY   dirent;
    CPU_CHAR       dirent_name[FTPs_CFG_FS_NAME_LEN_MAX];
    CPU_CHAR       attr_dir;
    CPU_CHAR       attr_ro;
    NET_ERR        net_err;
    CPU_BOOLEAN    fs_err;


    str_len_ttl    =         0;
    p_file         = (void *)0;
    p_dir          = (void *)0;
    net_err        =         NET_SOCK_ERR_NONE;
    dirent.NamePtr =        &dirent_name[0];

    switch (ftp_session->DtpCmd) {
        case FTP_CMD_NLST:
             p_dir = NetFS_DirOpen(ftp_session->CurEntry);
             if (p_dir != (void *)0) {
                 fs_err = NetFS_DirRd(p_dir, &dirent);
                 while (fs_err == DEF_OK) {
                     prn_buf     = FTPs_NetBufDtpCmdPtr + str_len_ttl;
                     prn_buf_len = FTPs_NET_BUF_LEN - str_len_ttl;
                     str_len = Str_FmtPrint((char *)prn_buf,
                                                    prn_buf_len,
                                                   "%s\n",
                                                    dirent.NamePtr);
                     if (str_len_ttl + str_len >= FTPs_NET_BUF_LEN) {
                         FTPs_Tx(ftp_session->DtpSockID, FTPs_NetBufDtpCmdPtr, str_len_ttl, &net_err);
                         if (net_err != NET_SOCK_ERR_NONE) {
                             FTPs_TRACE_DBG(("FTPs FTPs_Tx() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
                             str_len_ttl = 0;
                             break;
                         }

                         str_len_ttl = 0;
                         continue;
                     }

                     str_len_ttl += str_len;
                     fs_err = NetFS_DirRd(p_dir, &dirent);
                 }

                 if (str_len_ttl > 0) {
                     FTPs_Tx(ftp_session->DtpSockID, FTPs_NetBufDtpCmdPtr, str_len_ttl, &net_err);
                     if (net_err != NET_SOCK_ERR_NONE) {
                         FTPs_TRACE_DBG(("FTPs FTPs_Tx() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
                         break;
                     }
                 }
                 NetFS_DirClose(p_dir);
             }

             if (net_err == NET_SOCK_ERR_NONE) {
                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_CLOSINGSUCCESS, (CPU_CHAR *)0);
             } else {
                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_CLOSEDCONNABORT, (CPU_CHAR *)0);
             }
             break;

        case FTP_CMD_LIST:
             p_dir = NetFS_DirOpen(ftp_session->CurEntry);
             if (p_dir != (void *)0) {
                 fs_err = NetFS_DirRd(p_dir, &dirent);
                 while (fs_err == DEF_OK) {
                     if (DEF_BIT_IS_CLR(dirent.Attrib, NET_FS_ENTRY_ATTRIB_HIDDEN) == DEF_YES) {
                         if (DEF_BIT_IS_CLR(dirent.Attrib, NET_FS_ENTRY_ATTRIB_DIR) == DEF_YES) {
                             attr_dir = '-';
                         } else {
                             attr_dir = 'd';
                         }
                         if (DEF_BIT_IS_CLR(dirent.Attrib, NET_FS_ENTRY_ATTRIB_WR) == DEF_YES) {
                             attr_ro  = '-';
                         } else {
                             attr_ro  = 'w';
                         }

                         prn_buf     = FTPs_NetBufDtpCmdPtr + str_len_ttl;
                         prn_buf_len = FTPs_NET_BUF_LEN     - str_len_ttl;
                         str_len     = Str_FmtPrint((char       *)prn_buf,
                                                                  prn_buf_len,
                                                                 "%cr%c-r%c-r%c-   1 user     group    %8u %3s %2u  %4u %s\n",
                                                                  attr_dir,
                                                                  attr_ro, attr_ro, attr_ro,
                                                                  dirent.Size,
                                                                  FTPs_Month_Name[dirent.DateTimeCreate.Month - 1],
                                                    (unsigned int)dirent.DateTimeCreate.Day,
                                                    (unsigned int)dirent.DateTimeCreate.Yr,
                                                                  dirent.NamePtr);

                         if (str_len_ttl + str_len >= FTPs_NET_BUF_LEN) {
                             FTPs_Tx(ftp_session->DtpSockID, FTPs_NetBufDtpCmdPtr, str_len_ttl, &net_err);
                             if (net_err != NET_SOCK_ERR_NONE) {
                                 FTPs_TRACE_DBG(("FTPs FTPs_Tx() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
                                 str_len_ttl = 0;
                                 break;
                             }

                             str_len_ttl = 0;
                             continue;
                         }

                         str_len_ttl += str_len;
                     }
                     fs_err = NetFS_DirRd(p_dir, &dirent);
                 }

                 if (str_len_ttl > 0) {
                     FTPs_Tx(ftp_session->DtpSockID, FTPs_NetBufDtpCmdPtr, str_len_ttl, &net_err);
                     if (net_err != NET_SOCK_ERR_NONE) {
                         FTPs_TRACE_DBG(("FTPs FTPs_Tx() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
                         break;
                     }
                 }
                 NetFS_DirClose(p_dir);
             }

             if (net_err == NET_SOCK_ERR_NONE) {
                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_CLOSINGSUCCESS, (CPU_CHAR *)0);
             } else {
                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_CLOSEDCONNABORT, (CPU_CHAR *)0);
             }
             break;

        case FTP_CMD_RETR:
             p_file = NetFS_FileOpen(ftp_session->CurEntry,
                                     NET_FS_FILE_MODE_OPEN,
                                     NET_FS_FILE_ACCESS_RD);
             if (p_file == (void *)0) {
                 Str_FmtPrint((char *)FTPs_NetBufDtpCmdPtr,
                                      FTPs_NET_BUF_LEN,
                              (char *)"551 Cannot open %s: access denied.",
                                      ftp_session->CurEntry);
                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_ACTIONABORTED, FTPs_NetBufDtpCmdPtr);
                 break;
             }

             if (ftp_session->CtrlState == FTPs_STATE_GOTREST) {
                 fs_err = NetFS_FilePosSet(p_file, ftp_session->DtpOffset, NET_FS_SEEK_ORIGIN_START);
                 if (fs_err != DEF_OK) {
                     NetFS_FileClose(p_file);
                     Str_FmtPrint((char       *)FTPs_NetBufDtpCmdPtr,
                                                FTPs_NET_BUF_LEN,
                                  (char       *)"551 Cannot seek file %s to offset %u.",
                                  (char       *)ftp_session->CurEntry,
                                  (unsigned int)ftp_session->DtpOffset);
                     FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_ACTIONABORTED, FTPs_NetBufDtpCmdPtr);
                     break;
                 }
             }

             while (DEF_TRUE) {
                 fs_err = NetFS_FileRd((void       *) p_file,
                                         (void       *) FTPs_NetBufDtpCmdPtr,
                                         (CPU_SIZE_T  ) FTPs_NET_BUF_LEN,
                                         (CPU_SIZE_T *)&fs_len);
                 if (fs_len == 0) {
                     if (fs_err == DEF_FAIL) {
                         FTPs_TRACE_DBG(("FTPs NetFS_FileRd() failed: line #%u.\n", (unsigned int)__LINE__));
                     }
                     break;
                 }

                 FTPs_Tx(ftp_session->DtpSockID, FTPs_NetBufDtpCmdPtr, fs_len, &net_err);
                 if (net_err != NET_SOCK_ERR_NONE) {
                     FTPs_TRACE_DBG(("FTPs FTPs_Tx() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
                     break;
                 }
                 if (fs_len != FTPs_NET_BUF_LEN) {
                     break;
                 }
             }
             NetFS_FileClose(p_file);

             if ((net_err == NET_SOCK_ERR_NONE) && (fs_err == DEF_OK)) {
                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_CLOSINGSUCCESS, (CPU_CHAR *)0);
             } else {
                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_CLOSEDCONNABORT, (CPU_CHAR *)0);
             }
             break;

        case FTP_CMD_STOR:
        case FTP_CMD_APPE:
             if ((ftp_session->CtrlCmd   == FTP_CMD_STOR) &&
                 (ftp_session->CtrlState != FTPs_STATE_GOTREST)) {
                 p_file = NetFS_FileOpen(ftp_session->CurEntry,
                                         NET_FS_FILE_MODE_CREATE,
                                         NET_FS_FILE_ACCESS_WR);
             } else {
                 p_file = NetFS_FileOpen(ftp_session->CurEntry,
                                         NET_FS_FILE_MODE_APPEND,
                                         NET_FS_FILE_ACCESS_RD_WR);
             }

             if (p_file == (void *)0) {
                 Str_FmtPrint((char *)FTPs_NetBufDtpCmdPtr,
                                      FTPs_NET_BUF_LEN,
                              (char *)"551 Cannot open %s: access denied.",
                                      ftp_session->CurEntry);
                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_ACTIONABORTED, FTPs_NetBufDtpCmdPtr);
                 break;
             }

             if (ftp_session->CtrlState == FTPs_STATE_GOTREST) {
                 fs_err = NetFS_FilePosSet(p_file, ftp_session->DtpOffset, NET_FS_SEEK_ORIGIN_START);
                 if (fs_err != DEF_OK) {
                     NetFS_FileClose(p_file);
                     Str_FmtPrint((char       *)FTPs_NetBufDtpCmdPtr,
                                                FTPs_NET_BUF_LEN,
                                  (char       *)"551 Cannot seek file %s to offset %u.",
                                  (char       *)ftp_session->CurEntry,
                                  (unsigned int)ftp_session->DtpOffset);
                     FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_ACTIONABORTED, FTPs_NetBufDtpCmdPtr);
                     break;
                 }
             }

             NetSock_CfgTimeoutRxQ_Set((NET_SOCK_ID  ) ftp_session->DtpSockID,
                                       (CPU_INT32U   ) FTPs_CFG_DTP_MAX_RX_TIMEOUT_MS,
                                       (NET_ERR     *)&net_err);

             while (DEF_TRUE) {
                 net_len = NetSock_RxData(ftp_session->DtpSockID, FTPs_NetBufDtpCmdPtr, FTPs_NET_BUF_LEN, NET_SOCK_FLAG_NONE, &net_err);
                 if ((net_err != NET_SOCK_ERR_NONE) &&
                     (net_err != NET_SOCK_ERR_RX_Q_CLOSED) &&
                     (net_err != NET_SOCK_ERR_RX_Q_EMPTY)) {
                     FTPs_TRACE_DBG(("FTPs NetSock_RxData() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
                     break;
                 }

                                                            /* In this case, a timeout represent an end-of-file     */
                                                            /* condition.                                           */
                 if ((net_err == NET_SOCK_ERR_RX_Q_CLOSED) ||
                     (net_err == NET_SOCK_ERR_RX_Q_EMPTY)) {
                     break;
                 }
                 fs_err = NetFS_FileWr((void       *) p_file,
                                         (void       *) FTPs_NetBufDtpCmdPtr,
                                         (CPU_SIZE_T  ) net_len,
                                         (CPU_SIZE_T *)&fs_len);
                 if (fs_len != net_len) {
                     FTPs_TRACE_DBG(("FTPs NetFS_FileWr() failed: line #%u.\n", (unsigned int)__LINE__));
                     break;
                 }
             }
             NetFS_FileClose(p_file);

             if (((net_err == NET_SOCK_ERR_NONE)        ||
                  (net_err == NET_SOCK_ERR_RX_Q_CLOSED) ||
                  (net_err == NET_SOCK_ERR_RX_Q_EMPTY)) &&
                  (fs_err  == DEF_OK)) {
                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_CLOSINGSUCCESS, (CPU_CHAR *)0);
             } else {
                 FTPs_SendReply(ftp_session->CtrlSockID, FTP_REPLY_CLOSEDCONNABORT, (CPU_CHAR *)0);
             }
             break;

        default:
             break;
    }
}


/*
*********************************************************************************************************
*                                           FTPs_DtpTask()
*
* Description : FTP data transfer task
*
* Argument(s) : p_arg       argument passed to the task (cast to control socket ID).
*
* Return(s)   : none.
*
* Caller(s)   : FTPs_ProcessCtrlCmd().
*
* Note(s)     : This task implements the "DTP" (Data Transfer Process) as described in RFC 959.  The
*               means by which the connection is established with the client depends on whether or not
*               the DTP is to be passive or not.
*********************************************************************************************************
*/

void  FTPs_DtpTask (void  *p_arg)
{
    FTPs_SESSION_STRUCT  *ftp_session;
    NET_SOCK_ID           pasv_sock_id;
    NET_SOCK_ID           dtp_sock_id;
    NET_SOCK_ADDR         client_addr;
    NET_SOCK_ADDR_LEN     client_addr_len;
    CPU_INT32U            retry_cnt;
    NET_ERR               net_err;


    ftp_session = (FTPs_SESSION_STRUCT *)p_arg;

    if (ftp_session->DtpPasv == DEF_YES) {
        pasv_sock_id = ftp_session->DtpPasvSockID;

        retry_cnt = 0;
        while (DEF_TRUE) {
                                                                /* When a client make a request, accept it and create a */
                                                                /* new socket for it.                                   */
                                                                /* Wait on socket, accept with timeout.                 */
            FTPs_TRACE_INFO(("FTPs ACCEPT passive DTP socket.\n"));
            client_addr_len = (NET_SOCK_ADDR_LEN)sizeof(client_addr);
            dtp_sock_id     =  NetSock_Accept(pasv_sock_id,
                                             &client_addr,
                                             &client_addr_len,
                                             &net_err);
            switch (net_err) {
                case NET_SOCK_ERR_NONE:
                     retry_cnt = 0;
                     break;

                case NET_ERR_INIT_INCOMPLETE:
                case NET_SOCK_ERR_NULL_PTR:
                case NET_SOCK_ERR_NONE_AVAIL:
                case NET_SOCK_ERR_CONN_ACCEPT_Q_NONE_AVAIL:
                case NET_SOCK_ERR_CONN_SIGNAL_TIMEOUT:
                case NET_ERR_FAULT_LOCK_ACQUIRE:
                     retry_cnt++;
                     if (retry_cnt < FTPs_CFG_DTP_MAX_ACCEPT_RETRY) {
                         continue;                              /* Ignore transitory socket error.                      */
                     }
                     retry_cnt = 0;
                     break;                                     /* Close server socket on too many transitory errors.   */

                case NET_SOCK_ERR_NOT_USED:
                case NET_SOCK_ERR_INVALID_SOCK:
                case NET_SOCK_ERR_INVALID_TYPE:
                case NET_SOCK_ERR_INVALID_FAMILY:
                case NET_SOCK_ERR_INVALID_STATE:
                case NET_SOCK_ERR_INVALID_OP:
                case NET_SOCK_ERR_CONN_FAIL:
                default:
                     retry_cnt = 0;
                     break;
            }

            if (net_err != NET_SOCK_ERR_NONE) {
                                                                /* Close server socket on fatal error.                  */
                FTPs_TRACE_DBG(("FTPs NetSock_Accept() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
            }
            break;                                              /* Break while loop.                                    */
        }
    } else {
                                                                /* Open a socket.                                       */
        FTPs_TRACE_INFO(("FTPs OPEN active DTP socket.\n"));
        dtp_sock_id = NetSock_Open(NET_SOCK_ADDR_FAMILY_IP_V4,
                                   NET_SOCK_TYPE_STREAM,
                                   NET_SOCK_PROTOCOL_TCP,
                                  &net_err);
        if (net_err == NET_SOCK_ERR_NONE) {

#ifdef  NET_SECURE_MODULE_PRESENT                               /* Set or clear socket secure mode.                     */
            if (FTPs_SecureCfgPtr != DEF_NULL) {
                FTPs_TRACE_DBG(("NetSecure and FTPs do not support active mode. Connect to uC-FTPs in passive mode. line #%u.\n", (unsigned int)__LINE__));
                NetSock_Close(dtp_sock_id, &net_err);
                return;
            }
#endif
            NetSock_Conn(dtp_sock_id,
                         (NET_SOCK_ADDR *)&ftp_session->DtpSockAddr,
                         sizeof(NET_SOCK_ADDR),
                         &net_err);
            if ((net_err != NET_SOCK_ERR_NONE) &&
                (net_err != NET_SOCK_ERR_RX_Q_EMPTY)) {
                FTPs_TRACE_DBG(("FTPs NetSock_Conn() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
            }
            if (net_err == NET_SOCK_ERR_RX_Q_EMPTY) {
                FTPs_TRACE_DBG(("FTPs NetSock_Conn() timeout, line #%u.\n", (unsigned int)__LINE__));
            }
        } else {
            FTPs_TRACE_DBG(("FTPs NetSock_Open() failed: error #%u, line #%u.\n", (unsigned int)net_err, (unsigned int)__LINE__));
        }
    }

    if (net_err == NET_SOCK_ERR_NONE) {
        ftp_session->DtpSockID = dtp_sock_id;
        FTPs_TRACE_INFO(("FTPs START transfer.\n"));
        FTPs_ProcessDtpCmd(ftp_session);
        FTPs_TRACE_INFO(("FTPs STOP transfer.\n"));
    }

    FTPs_TRACE_INFO(("FTPs CLOSE DTP socket.\n"));
    NetSock_Close(ftp_session->DtpSockID, &net_err);
}

