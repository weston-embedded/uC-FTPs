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
* Filename : ftp-s_auth.c
* Version  : V1.98.00
*********************************************************************************************************
* Note(s)  : (1) Assumes the following versions (or more recent) of software modules are included in
*                the project build :
*
*                (a) uC/FTPc   V1.94
*                (b) uC/TCP-IP V3.00
*                (c) uC/OS-II  V2.90 or
*                    uC/OS-III V3.00
*********************************************************************************************************
*/

#include  <lib_def.h>
#include  <lib_str.h>
#include  <Source/ftp-s.h>

/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

CPU_CHAR     FTPs_username[] = "DUT";
CPU_CHAR     FTPs_pw[]       = "Micrium";

/*
*********************************************************************************************************
*                                           FTPs_AuthUser()
*
* Description : Callback function authenticating a user on a FTP session.
*
* Arguments   : ftp_session     Pointer to FTP session.
*
* Returns     : DEF_OK,   user authenticated.
*               DEF_FAIL, otherwise.
*
* Caller(s)   : FTPs_ProcessCtrlCmd().
*
* Note(s)     : (1) This authentication is trivial, accepts everyone, sets BasePath & RelPath to ROOT
*                   allowing access to the whole filesystem.  This MUST be modified according to your
*                   security policies.
*********************************************************************************************************
*/

CPU_BOOLEAN  FTPs_AuthUser (FTPs_SESSION_STRUCT  *ftp_session)
{
                                                                /* Initialize BasePath to ROOT.                         */
    Str_FmtPrint((char *)ftp_session->BasePath,
                  sizeof(ftp_session->BasePath),
                 "%c",
                  FTPs_PATH_SEP_CHAR);

                                                                /* Initialize RelPath to ROOT.                          */
    Str_FmtPrint((char *)ftp_session->RelPath,
                  sizeof(ftp_session->RelPath),
                 "%c",
                  FTPs_PATH_SEP_CHAR);

    if ((Str_Cmp(FTPs_username, ftp_session->User) == 0) && (Str_Cmp(FTPs_pw, ftp_session->Pass) == 0)) {
        return (DEF_OK);
    } else {
        return (DEF_FAIL);
    }
}
