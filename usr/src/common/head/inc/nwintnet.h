/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/nwintnet.h	1.2"
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/
/****************************************************************************
 ****************************************************************************
 BEGIN_MANUAL_ENTRY ( nwintnet.h() )

 SOURCE MODULE: nwintnet.h

 API NAME     :

 SYNTAX       :

 DESCRIPTION  :

    This header is used internally to build the NWNET Library.

    The definition for the entries into the NWCallGate are defined.
    These definitions are extremely sensitive, and should not be readily
    shared with others, particularly they should not be shared with
    people outside the company.

    As this is the internal header, we will document some of the
    internal workings of the NWNET (as best we can) to ease some of
    the maintenance burden.

    Under OS/2, the NWNET use a CallGate (see MS reference doc) to
    access the NetWare requester.  The call gate consists of several
    functions that the requester can perform in behalf of the
    application making calls to the .DLL.  Upon startup, the NWNET.DLL
    gets the address of the call gate from the requester, then uses this
    address to communicate with the requester.  Whenever making calls to
    the call gate, the API's pass the address of the last parameter on
    the stack (or first depending how you look at it), then the
    requester uses this address to pull off its arguments from the
    application's stack.  This design limits us because the differente
    functions performed through the call gate expect the parameters to
    be at that address, in a certain order and with a certain size, so
    in order to maintain backward compatibility, we would not be able to
    change the requester (unless we added additional functions to the
    call gate).

    Also under OS/2, there is the possibility of the Requester running
    in the HPFS (Long Name) mode in which case, some of our API's need
    to make the corresponding NetWare calls.  Currently this is done by
    checking if the requester is a long name requester, then checking
    the destination volume on the server to see if the OS/2 name space
    is loaded.  If these two conditions are met, then the Long Name
    NCP's are used.

    Under DOS, the workstation is assumed to be running DOS, and thus no
    special considerations are used.


 PARAMETERS   :   -> input          <-output

 INCLUDE      :

 RETURN       :

 MODIFICATIONS:

      July 16, 1991
      October 16, 1991 - Joe - removed including bseerr.h for TURBOC
                             - moved FP_OFF... defines to nwcaldef.h
      January 8, 1992 - Cleaned up manpage - Joe Woodbury
      January 13, 1992 - Added Support for C++ - Joe
      April 13, 1992 - Changed name from internal.h to nwintnet.h - David Cox

 NCP CALLS
 ---------

 API CALLS
 ---------

 SEE ALSO:

 @Q1 HANDLES STRINGS? (Y\N): N

 @Q2 HANDLES PATH? (Y\N): N

 END_MANUAL_ENTRY
****************************************************************************/
#ifndef NWLIB_INTERNAL
#define NWLIB_INTERNAL

#ifndef NWNOT_USED
#define NWNOT_USED( n ) n = n;
#endif


#ifdef NWOS2

/* OS2 specific stuff */

#define NWI_ATTACH_CODE         0x0000
#define NWI_DETACH_CODE         0x0001
#define NWI_LOGIN_CODE          0x0002
#define NWI_LOGOUT_CODE         0x0003
#define NWI_NETREQUEST          0x0004
#define NWI_LOCK_NETREQUEST     0x0005
#define NWI_EXTENDED_NCP        0x0006
#define NWI_LNS_SUP_ON_VOL      0x0007
#define NWI_GET_CONN_LIST       0x0008
#define NWI_GET_CONN_INFO       0x0009
#define NWI_MAP_SERVER          0x000A
#define NWI_NETREQUESTALL       0x000B
#define NWI_SET_CONN_FLAGS      0x000C
#define NWI_GET_SHELL_VERSION   0x000D
#define NWI_ORDEREDNETREQUEST   0x000E
#define NWI_FIND_SERVER_CODE    0x000F
#define NWI_GET_STATS           0x0010
#define NWI_RESET_STATS         0x0011
#define NWI_GET_SESSION         0x0012
#define NWI_REGISTER_DAEMON     0x0013
#define NWI_GET_DAEMON_PID      0x0014
#define NWI_GET_MAX_CONNECTIONS 0x0015
#define NWI_WAIT_LOCK_AVAIL     0x0016
#define NWI_GET_SERVER_INFO     0x0017

/* Number 0x0018 is reserved by the Requester */

#define NWI_REGISTER_SPOOLER    0x0019
#define NWI_CLEANUP_COMPLETE    0x001A
#define NWI_RESET_BCAST_DAEMON  0x001B
#define NWI_REGISTER_JANITOR    0x001C
#define NWI_GET_TIME_STAMP      0x001D
#define NWI_TOGGLE_LNS          0x001F
#define NWI_CONNECTION_TABLE    0x0020

#define NWI_MAP_FLAT_TO_SELECTOR  0x0021
#define NWI_FREE_MAPPED_SELECTOR  0x0022
#define NWI_DISPLAY_HARD_ERRORS   0x0023

#define NWI_BLOCK_DAEMON      0x0024

#define NWI_NDS_GET_CONNECTION_INFO 0x0025
#define NWI_NDS_SET_CONNECTION_INFO 0x0026
#define NWI_GET_CONNECTION_SLOT   0x0027
#define NWI_FREE_CONNECTION_SLOT  0x0028
#define NWI_GET_CONNID_FROM_ADDR  0x0029

#define NWI_GET_TDS_INFO      0x002A
#define NWI_ALLOC_TDS       0x002B
#define NWI_FREE_TDS        0x002C
#define NWI_READ_TDS        0x002D
#define NWI_WRITE_TDS       0x002E

#define NWI_GET_DEFAULT_NAME_CONTEXT  0x002F
#define NWI_SET_DEFAULT_NAME_CONTEXT  0x0030

#define NWI_IS_NDS_PRESENT            0x0031
#define NWI_GET_INIT_DIRECTORY_SERVER 0x0032

#define NWI_FREE_TASK_RESOURCE    0x33
#define NWI_GET_TASK_RESOURCE     0x34
#define NWI_NOT_USED              0x35

#define NWI_LOCK_CONNECTION_SLOT   0x37
#define NWI_UNLOCK_CONNECTION_SLOT 0x38

#define NWI_SET_PREFERRED_SERVER    0x39
#define NWI_GET_PREFERRED_SERVER    0x3A

#define NWI_GET_MONITORED_CONN      0x3D
#define NWI_SET_MONITORED_CONN      0x3E

#define NWI_GET_PREF_TREE_NAME      0x34
#define NWI_SET_PREF_TREE_NAME      0x35
#define NWI_GET_PREF_TREE_CONN      0x3F

#ifndef FP_OFF
#define FP_OFF(fp) (*((unsigned _far *)&(fp)))
#endif

#ifndef FP_SEG
#define FP_SEG(fp) (*((unsigned _far *)&(fp)+1))
#endif

#define NET_SPECIAL_FILE "\\\\SPCLNET$\\SPCLNET$"

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned int (_far NWPASCAL *NWCallGate)(unsigned, void N_FAR * );
extern void N_FAR NWPASCAL CleanUpList(unsigned short); /* internal function in SYNC */
#ifdef __cplusplus
}
#endif

#else

/* DOS specific stuff */

#ifndef FP_OFF
#define FP_OFF(fp) (*((unsigned _far *)&(fp)))
#endif

#ifndef FP_SEG
#define FP_SEG(fp) (*((unsigned _far *)&(fp)+1))
#endif


/*  Vise Loadable Module manager module function codes */
#define VLM_INI            0x0
#define VLM_ID_OVL         0x01     /* Overlay manager module                               */
#define VLM_ID_CONN        0x10     /* Connection table manager module                      */
#define VLM_ID_IPX         0x21     /* IPX transport module */
#define VLM_ID_BIND        0x31     /* NetWare Bindary protocol module   */
#define VLM_ID_NDS         0x32     /* NetWare Directory Services NetWare protocol module   */
#define VLM_ID_NWP         0x30     /* Netware Protocol module */
#define VLM_ID_REDIR       0x40     /* Netware Protocol module */
#define VLM_ID_SECURITY    0x61     /* Security message digest module */
#define CONN_STA           0x3      /* CONN get statistics */
#define CONN_ALO           0x4      /* CONN alocate connection entry           */
#define CONN_VAL           0x5      /* CONN validate connection entry handle   */
#define CONN_FRE           0x6      /* CONN free connection entry handle       */
#define CONN_GET           0x7      /* CONN get element of connection entry    */
#define CONN_SET           0x8      /* CONN set element of connection entry    */
#define CONN_RES           0x9      /* CONN reset element of connection entry    */
#define CONN_LUP           0xA      /* CONN lookup handle by entry value       */
#define CONN_NLUN          0xD      /* CONN lookup name by connection          */
#define CONN_NLUC          0xE      /* CONN lookup connection by name          */
#define CONN_GNUM          0xF      /* CONN get number of connections supported */
#define STATIC_CONN_TYPE   0x1      /* Static connection type */
#define DYNAMIC_CONN_TYPE  0        /* Dynamic connection type */
#define CEI_ERROR          0        /* CEI error and status */
#define CEI_NET_TYPE       1        /* CEI network type */
#define CEI_PERM           2        /* CEI connection not authenticated flag */
#define CEI_AUTH           3        /* CEI connection authenticated flag */
#define CEI_PBURST         4        /* CEI packet burst */
#define CEI_NEEDS_MAXIO    6        /* CEI Max IO is needed */
#define CEI_RES_HARD_COUNT 9        /* CEI hard resource count */
#define CEI_DIST_TIME      10       /* CEI distance to the server in time */
#define CEI_MAX_IO         11       /* CEI Max IO is needed */
#define CEI_CON_NUM        13       /* CEI connection number */
#define CEI_ORDER_NUM      14       /* CEI order number */
#define CEI_TRAN_TYPE      15       /* CEI transport type */
#define CEI_TRAN_BUFF      17       /* CEI transport buffer */
#define CEI_SECURITY       20       /* CEI security flags */
#define AUTHENTICATED      4        /* Authenticated Flag */
#define IPX_TRANSPORT      0x1      /* IPX transport ID */
#define IPX_BUFFER_SIZE    12       /* size of an IPX buffer */
#define IPX_REQ_REPLY      0x6      /* IPXRequestReply (NCP stuff ) */
#define NDS_BUF            10       /* NWP NDS Set CEI buffer */
#define NWP_MD4            16       /* NWP MD4 security */
#define NWP_NDS            12       /* NWP NDS specific functions */
#define NDS_CON            0x4      /* NDS Connect to File Server */
#define NDS_DIS            0x5      /* NDS disconnect from server              */
#define NDS_ATT            0x6      /* NDS attach to server                    */
#define NDS_GDN            0x0      /* NDS get default context name            */
#define NDS_SDN            0x1      /* NDS set default context name            */
#define NDS_RTD            0x2      /* NDS read tagged data store              */
#define NDS_WTD            0x3      /* NDS write tagged data store             */
#define NDS_GTD            0x4      /* NDS get TDS control info                */
#define NDS_RCC            0x5      /* Change Resource Count */
#define NDS_GSL            0x7      /* GetSetLogged */
#define SECURITY_MD4       0x4      /* Security MD4 function */
#define NWP_MUX_DX         15       /* NWP  multiplex through DX and BX subfunctions */
#define GET_PREF_NAME      0
#define SET_PREF           1
#define GET_PREF_CONN_ID   2
#define REDIR_BLD_SFT		4
#define GET_LOGGED         0x1
#define SET_LOGGED         0x2
#define GLOBAL_DIS         0x0
#define DESTROY_CONN       0x1
#define LOCK               0x8       /* Lock connection */
#define UNLOCK             0x9       /* Unlock connection */
#define VLM_IPX_TYPE       0X21      /* VLM IPX type */
#define IPX_TRAN_TYPE      0X1       /* Server transport type for IPX */
#define IPX_TRANS_LENGTH   12        /* IPX transport length */

#define SET_PREFERRED   0xf000
#define CARRY_FLAG      0x0001

#endif

#ifdef __cplusplus
extern "C" {
#endif

/* NWClient DS prototypes */
NWCCODE N_API _NWCGetConnectionSecurity(NWCONN_HANDLE conn, nuint8 N_FAR *security);
NWCCODE N_API _NWCGetSessionKey(NWCONN_HANDLE conn,
                                       nuint8 N_FAR *sessionKeyComponents,
                                       nuint8 N_FAR *sessionKey);

/* These in sign.c module and are used for signing packets */
NWCCODE N_API NWGetSessionKey(NWCONN_HANDLE conn,
                                       BYTE N_FAR *sessionKeyComponents,
                                       BYTE N_FAR *sessionKey);
NWCCODE N_API _NWGetConnectionSecurity(NWCONN_HANDLE conn, BYTE N_FAR *security);

#ifdef __cplusplus
}
#endif

#endif


