/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/nwcint.h	1.3"
/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#if !defined(NWCINT_H)
#define NWCINT_H

#if !(defined N_PLAT_WNT && defined N_ARCH_32)
#ifndef NTYPES_H
#include <ntypes.h>
#endif
#endif

#include <npackon.h>

#ifdef N_PLAT_OS2                                /* OS2 specific stuff   */
                                                   /* CallGate functions   */
#define _NWC_ATTACH                 0x0000
#define _NWC_DETACH                 0x0001
#define _NWC_LOGIN                  0x0002
#define _NWC_LOGOUT                 0x0003
#define _NWC_NET_REQUEST            0x0004

#define _NWC_LOCK_NETREQUEST        0x0005
#define _NWC_EXTENDED_NCP           0x0006
#define _NWC_LNS_SUP_ON_VOL         0x0007
#define _NWC_GET_CONN_LIST          0x0008
#define _NWC_GET_CONN_INFO          0x0009
#define _NWC_MAP_SERVER             0x000A
#define _NWC_NETREQUESTALL          0x000B
#define _NWC_SET_CONN_FLAGS         0x000C
#define _NWC_GET_SHELL_VERSION      0x000D
#define _NWC_ORDEREDNETREQUEST      0x000E
#define _NWC_FIND_SERVER            0x000F
#define _NWC_GET_STATS              0x0010
#define _NWC_RESET_STATS            0x0011
#define _NWC_GET_SESSION            0x0012
#define _NWC_REGISTER_DAEMON        0x0013
#define _NWC_GET_DAEMON_PID         0x0014
#define _NWC_GET_MAX_CONNS          0x0015
#define _NWC_WAIT_LOCK_AVAIL        0x0016
#define _NWC_GET_SERVER_INFO        0x0017
#define _NWC_REGISTER_SPOOLER       0x0019
#define _NWC_CLEANUP_COMPLETE       0x001A
#define _NWC_RESET_BCAST_DAEMON     0x001B
#define _NWC_REGISTER_JANITOR       0x001C
#define _NWC_GET_TIME_STAMP         0x001D
/* #define _NWC_TOGGLE_LNS           0x001F - OBSOLETE - DO NOT USE */
#define _NWC_CONN_TABLE             0x0020
#define _NWC_MAP_FLAT_TO_SELECTOR   0x0021
#define _NWC_FREE_MAPPED_SELECTOR   0x0022
#define _NWC_DISPLAY_HARD_ERRORS    0x0023
#define _NWC_BLOCK_DAEMON           0x0024
#define _NWC_NDS_GET_CONN_INFO      0x0025
#define _NWC_NDS_SET_CONN_INFO      0x0026
#define _NWC_GET_CONN_SLOT          0x0027
#define _NWC_FREE_CONN_SLOT         0x0028
#define _NWC_GET_CONN_FROM_ADDR     0x0029
#define _NWC_GET_TDS_INFO           0x002A
#define _NWC_ALLOC_TDS              0x002B
#define _NWC_FREE_TDS               0x002C
#define _NWC_READ_TDS               0x002D
#define _NWC_WRITE_TDS              0x002E
#define _NWC_GET_DEF_NAME_CONTEXT   0x002F
#define _NWC_SET_DEF_NAME_CONTEXT   0x0030
#define _NWC_IS_NDS_PRESENT         0x0031
#define _NWC_GET_INIT_DIR_SERVER    0x0032
#define _NWC_FREE_TASK_RESOURCE     0x0033
#define _NWC_GET_PREF_TREE_NAME     0x0034
#define _NWC_SET_PREF_TREE_NAME     0x0035
#define _NWC_LOGOUT_W_CITRIX_ID     0x0036

#define _NWC_LOCK_CONN_SLOT         0x0037
#define _NWC_UNLOCK_CONN_SLOT       0x0038

#define _NWC_SET_PREF_SERVER        0x0039
#define _NWC_GET_PREF_SERVER        0x003A
#define _NWC_CREATEKEY_CODE         0x003C
#define _NWC_GET_MONITORED_CONN     0x003D
#define _NWC_SET_MONITORED_CONN     0x003E
#define _NWC_RESET_CONN_CONFIG      0x0040

#define _NWC_NET_SPECIAL_FILE "\\\\SPCLNET$\\SPCLNET$"

#define _NWC_MAX_CONNS        32

N_EXTERN_LIBRARY(nuint16)
NWCCallGate
(
   nuint16  function,
   nptr     params
);

extern void N_API CleanUpList(nuint16);      /* internal function in SYNC  */

                           /* Connection flag definitions from NWREQ.INC   */
#define _NWC_CONN_AVAILABLE            0x0001
#define _NWC_CONN_RELEASE_TEMP         0x0002
#define _NWC_CONN_LOGGED_IN            0x0004
#define _NWC_CONN_BROADCAST            0x0008
#define _NWC_CONN_ABORTED              0x0010
#define _NWC_CONN_GEN_BCAST            0x0020
#define _NWC_CONN_CON_BCAST            0x0040
#define _NWC_CONN_PRIMARY              0x0080
#define _NWC_CONN_NDS                  0x0100
#define _NWC_CONN_RIPL                 0x0200
#define _NWC_CONN_SERVER_BUSY          0x0400
#define _NWC_CONN_LIP_NEGOTIATED       0x0800
#define _NWC_CONN_LIP_SIZE_NOT_VALID   0x1000
#define _NWC_CONN_LIP_NEG_BUSY         0x2000
#define _NWC_CONN_NDS_AUTHENTICATION   0x8000

                                 /* NDS flags definitions from NWREQ.INC   */
#define _NWC_NDS_DIRECTORY_SERVICES    0x01
#define _NWC_NDS_LICENSED              0x02
#define _NWC_NDS_AUTHENTICATED         0x04
#define _NWC_NDS_DS_CAPABLE            0x08
#define _NWC_NDS_CONNECTION_DYNAMIC    0x10


#elif defined(N_PLAT_DOS) || (defined N_PLAT_MSW && defined N_ARCH_16 && !defined N_PLAT_WNT)

/* VLM IDs */

#define _NWC_VLM_VLM    0x0001 /* VLM manager module                          */
#define _NWC_VLM_CONN   0x0010 /* Connection table manager module             */
#define _NWC_VLM_TRAN   0x0020 /* Transport multiplex module                  */
#define _NWC_VLM_IPX    0x0021 /* IPX transport module                        */
#define _NWC_VLM_TCP    0x0022 /* TCP/IP transport module                     */
#define _NWC_VLM_NWP    0x0030 /* NetWare protocol multiplex module           */
#define _NWC_VLM_BIND   0x0031 /* NetWare Bindery protocol module             */
#define _NWC_VLM_NDS    0x0032 /* NetWare Directory Services protocol module  */
#define _NWC_VLM_LITE   0x0033 /* NetWare Lite protocol module                */
#define _NWC_VLM_PNW    0x0033 /* Personal NetWare protocol module            */
#define _NWC_VLM_RSA    0x0034 /* RSA module used for NDS functions           */
#define _NWC_VLM_REDIR  0x0040 /* Redirector module                           */
#define _NWC_VLM_FIO    0x0041 /* File protocol module                        */
#define _NWC_VLM_PRINT  0x0042 /* Print module                                */
#define _NWC_VLM_GEN    0x0043 /* General purpose functions                   */
#define _NWC_VLM_NETX   0x0050 /* NETX compatible module                      */
#define _NWC_VLM_AUTO   0x0060 /* Auto reconnect/retry module                 */
#define _NWC_VLM_NMR    0x0100 /* Network Management Responder module         */
#define _NWC_VLM_NONAME 0x9999 /* NONAME overlay ID for NONAME.ASM            */

/* VLM function codes */
/* VLM.EXE module functions */
#define _NWC_VLM             1  /* Common generic function          */
#define  _NWC_VLM_VER        0  /* Generic version function         */
#define  _NWC_VLM_CONN_ADD   2  /* Add a new connection             */
#define  _NWC_VLM_CONN_DEL   3  /* Delete an existing connection    */
#define  _NWC_VLM_LOGIN      4  /* Login an existing connection     */
#define  _NWC_VLM_LOGOUT     5  /* Logout an existing connection    */
#define  _NWC_VLM_DRV_ADD    6  /* Add a new drive redirection      */
#define  _NWC_VLM_DRV_DEL    7  /* Delete a drive redirection       */
#define  _NWC_VLM_PRN_ADD    8  /* Add a new printer redirection    */
#define  _NWC_VLM_PRN_DEL    9  /* Delete a printer redirection     */
#define  _NWC_VLM_CHNG_DIR   10 /* Change directories               */
#define  _NWC_VLM_PREL_DEL   11 /* Pre logout from server           */
#define _NWC_VLM_STA             3  /* Common VLM statistics function  */
#define _NWC_VLM_COM             4  /* VLM common function             */

/* CONN.VLM functions */
#define _NWC_CONN_GET_STATS            3   /* get statistics */
#define _NWC_CONN_ALOCATE_ENTRY        4   /* alocate connection entry              */
#define _NWC_CONN_VALIDATE_HANDLE      5   /* validate connection entry handle      */
#define _NWC_CONN_FREE_HANDLE          6   /* free connection entry handle          */
#define _NWC_CONN_GET_ITEM             7   /* get element of connection entry       */
#define _NWC_CONN_SET_ITEM             8   /* set element of connection entry       */
#define _NWC_CONN_RESET_ITEM           9   /* reset element of connection entry     */
#define _NWC_CONN_GET_HANDLE_BY_VAL    10  /* lookup handle by entry value          */
#define _NWC_CONN_GET_NAME_BY_CONN     13  /* lookup name by connection             */
#define _NWC_CONN_GET_CONN_BY_NAME     14  /* lookup connection by name             */
#define _NWC_CONN_NUM_CONNS_SUPPORTED  15  /* return num of conn entries supported  */
#define _NWC_CONN_TID                  16  /* API to manage task IDs                */
#define  _NWC_CONN_TID_GET_CURRENT     0   /* Get current task ID                   */
#define  _NWC_CONN_TID_TLOCK_SET       3   /* Task lock mode set                    */
#define  _NWC_CONN_TID_TLOCK_GET       4   /* Task lock mode get                    */
#define  _NWC_CONN_TID_SET_GLOB_HARD   5   /* inc/dec the global hard count         */

#define _NWC_STATIC_CONN_TYPE   0x1 /* Static connection type */
#define _NWC_DYNAMIC_CONN_TYPE  0   /* Dynamic connection type */

#define _NWC_CEI_ERROR          0  /* F R - CEI error and status */
#define _NWC_CEI_NET_TYPE       1  /* V 2 - CEI network type */
#define _NWC_CEI_NON_AUTH       2  /* F R - CEI connection not authenticated flag */
#define _NWC_CEI_AUTH           3  /* F R - CEI connection authenticated flag */
#define _NWC_CEI_PBURST         4  /* F   - CEI packet burst */
#define _NWC_CEI_CHANGING       5  /* F R */
#define _NWC_CEI_NEEDS_MAXIO    6  /* F R - CEI Max IO is needed */
#define _NWC_CEI_PBURST_RESET   7  /* F R */
#define _NWC_CEI_MAJ_MIN_VER    8  /* V 2 */
#define _NWC_CEI_REF_COUNT      9  /* V 2 */
#define _NWC_CEI_DIST_TIME     10  /* V 2 - CEI distance to the server in time */
#define _NWC_CEI_MAX_IO        11  /* V 2 - CEI Max IO is needed */
#define _NWC_CEI_NCP_SEQ       12  /* V 1 */
#define _NWC_CEI_CONN_NUM      13  /* V 2 - CEI connection number */
#define _NWC_CEI_ORDER_NUM     14  /* V 1 - if a conn has order number, it is valid */
#define _NWC_CEI_TRAN_TYPE     15  /* V 2 - CEI transport type */
#define _NWC_CEI_NCP_REQ_TYPE  16  /* V 2 */
#define _NWC_CEI_TRAN_BUFF     17  /* V buffer - CEI transport buffer */
#define _NWC_CEI_BCAST         18  /* F R */
#define _NWC_CEI_LIP           19  /* V 2 */
#define _NWC_CEI_SECURITY      20  /* F R */
#define _NWC_CEI_RES_SOFT_COUNT 21 /* V 2 */

/* NWP.VLM module functions */
#define _NWC_NWP_CONNECT         4   /* connect to server              */
#define _NWC_NWP_DISCONNECT      5   /* disconnect from server         */
#define _NWC_NWP_ATTACH          6   /* attach to server (SYSTEM ONLY) */
#define _NWC_NWP_LOGIN           8   /* login to connected server      */
#define _NWC_NWP_LOGOUT          9   /* logout of connected server     */
#define _NWC_NWP                 10
#define  _NWC_NWP_GET_PREF_SRV   0   /* get preferred server           */
#define  _NWC_NWP_SET_PREF_SRV   1   /* set preferred server           */
#define  _NWC_NWP_GETSET_BCAST   2   /* get/set broadcast mode         */
#define  _NWC_NWP_GET_OBJ_ID     3   /* get bindery object ID          */
#define  _NWC_NWP_GET_OBJ_NAME   4   /* get bindery object name        */
#define  _NWC_NWP_GET_MSG        5   /* get/display broadcast message  */
#define _NWC_NWP_BIND            11  /* bind specific functions        */
#define _NWC_NWP_NDS             12  /* NDS specific functions         */
#define _NWC_NWP_LITE            13  /* lite specific functions        */
#define _NWC_NWP_ORD_ALL         14  /* ordered NCP send to all        */
#define _NWC_NWP_MD4             16

/* TRAN.VLM functions */
#define _NWC_TRAN_1ST         4   /* connect to first available service  */
#define _NWC_TRAN_REQ         6   /* request/reply function              */
#define _NWC_TRAN_CAN         7   /* cancel request/reply (system only)  */
#define _NWC_TRAN_SCH         8   /* schedule asynchronous event         */
#define _NWC_TRAN_MAX         9   /* get maximum physical size           */
#define  _NWC_TRAN_MAX_NODE   0  /* get maximum node size                */
#define  _NWC_TRAN_MAX_ROUTE  1  /* get maximum route size               */
#define _NWC_TRAN_UDS         11  /* undirected send                     */

/* NDS.VLM sub-functions */
#define _NWC_NDS_GDN             0   /* get default context name */
#define _NWC_NDS_SDN             1   /* set default context name */
#define _NWC_NDS_RTD             2   /* read tagged data store   */
#define _NWC_NDS_WTD             3   /* write tagged data store  */
#define _NWC_NDS_GTD             4   /* get TDS control info     */
#define _NWC_NDS_RCC             5   /* resource count change    */
#define _NWC_NDS_GSL             7   /* get/set logged server    */
#define  _NWC_NDS_GSL_GET_LOGGED 1
#define  _NWC_NDS_GSL_SET_LOGGED 2

/* PRINT.VLM module functions */
#define _NWC_PRINT_GET_SET_STRUCTS     4  /* Set/Get the print specific structures    */
#define _NWC_PRINT_GET_SET_PRINT_MODE  5  /* Set/Get the printer mode (binary/text)   */
#define _NWC_PRINT_WRITE_TO_Q_JOB      6  /* Write block of data to queue job         */
#define _NWC_PRINT_GET_NUM_PINTERNS    7  /* Get the number of printers               */
#define _NWC_PRINT_GET_SET_REDIR_STATE 8  /* Set/Get/Cancel print redirection state   */
#define _NWC_PRINT_FLUSH_CLOSE_JOB     9  /* Flush & close the print queue job        */
#define _NWC_PRINT_GET_SET_BANNER_NAME 12 /* Set/Get global banner name               */
#define _NWC_PRINT_GET_SET_RESET_STR   13 /* Set/Get the pointer to either the setup  */
                                          /* or reset strings (header/tail)           */

/* REDIR.VLM module functions */
#define _NWC_REDIR_BLD_SFT             4 /* Get available SFT and map it to NW file*/
#define _NWC_REDIR_FILE_INFO           5 /* Convert file handle to NW handle       */
#define _NWC_REDIR_SEARCH_STRING       6 /* Searches string until specified character or NULL */
#define _NWC_REDIR                     8 /* Subfunction dispatcher for APIs                   */
#define  _NWC_REDIR_GET_ITEM           0 /* Get information for a specified device            */
#define  _NWC_REDIR_SET_SHARE          1 /* Set share bit on/off applied to next function     */
#define  _NWC_REDIR_UPDATE_DIR_HANDLE  2  /* Update the directory handle at reconnect time */

/* GENERAL.VLM module functions */
#define _NWC_GEN_GET_SET_PRIMARY_CONN  4 /* get/set primary connection             */
#define _NWC_GEN_RESTORE_MAIN_COMSPEC  5 /* Restore main comspec                   */
#define _NWC_GEN                       6 /* Misc GENERAL functions                 */
#define  _NWC_GEN_GET_ENVIRONMENT      0 /* Get command com and master environment */
#define  _NWC_GEN_DEF_PRIMARY_CONN     1 /* Gets the default or primary connection */
#define  _NWC_GEN_LAST_Q               2 /* Get/Set last Q accessed info (NETQ)    */
#define   _NWC_GEN_LAST_Q_ZAP          0
#define   _NWC_GEN_LAST_Q_SET          1
#define   _NWC_GEN_LAST_Q_GET          2
#define  _NWC_GEN_MNAME                3
#define   _NWC_GEN_MNAME_GET_SHORT     0
#define   _NWC_GEN_MNAME_GET_LONG      2
#define   _NWC_GEN_MNAME_SET_SHORT     4
#define   _NWC_GEN_MNAME_SET_LONG      6
#define   _NWC_GEN_MNAME_GET_DOS       8
#define   _NWC_GEN_MNAME_SET_DOS       10
#define _NWC_GEN_OPEN_SRCH_MOD_HANDLER 7 /* Open search mode handler               */
#define _NWC_GEN_DRV_DEL_SRCH          8 /* delete search drive from mastr env     */
#define _NWC_GEN_DRV                   9 /* return drive information               */
#define  _NWC_GEN_DRV_GET_FIRST_AVAIL  0 /* returns first available drive  */
#define  _NWC_GEN_DRV_GET_TABLE_INFO   1 /* get drive table info           */

#define _NWC_GET_PRIMARY_CONN 1
#define _NWC_SET_PRIMARY_CONN 2

#elif defined N_PLAT_WNT && defined N_ARCH_32

typedef struct NUNICODE_STRING
{
   USHORT Length;
   USHORT MaximumLength;
   PWSTR  Buffer;
} NUNICODE_STRING, *pNUNICODE_STRING;

N_EXTERN_LIBRARY( int )
NWCUpcaseUnicodeString
(
   pNUNICODE_STRING dstStr,
   pNUNICODE_STRING srcStr,
   int allocDstStr
);

N_EXTERN_LIBRARY( void )
NWCInitUnicodeString
(
   pNUNICODE_STRING dstStr,
   PCWSTR srcStr
);


/*===[ Include files specific to NT ]======================================*/
#define _NWC_MAX_OBJ_NAME_LEN   0x30
#define _NWC_MAX_PASS_LEN       0x30

/* Device Types */
#define _NWC_DEVICE_DISK            0x00000001
#define _NWC_DEVICE_PARALLEL_PORT   0x00000002
#define _NWC_DEVICE_SERIAL_PORT     0x00000003

/* This constant must always be equal to the last device */
#define _NWC_DEVICE_LAST_DEVICE     0x00000003

/* Used for enumerating all device types */
#define _NWC_DEVICE_ALL             0xFFFFFFFF

/* Map Types */
#define _NWC_MAP_LINK_TEMP          0x00000000
#define _NWC_MAP_LINK_PERM          0x00000001

/* Transport independant definitions */
#define _NWC_TRAN_BUFF_SIZE         32

/* NDS definitions */
#define _NWC_MAX_NAME_CONTEXT       256
#define _NWC_MAX_SRV_NAME_LEN    40

#define _NWC_CONN_TYPE_PERM         0
#define _NWC_CONN_TYPE_TEMP         1

/* Server type Ids (to be used when creating new connections. */
#define _NWC_SRV_TYPE_UNKNOWN    0
#define _NWC_SRV_TYPE_NETWARE    1
#define _NWC_SRV_TYPE_PNW        2
#define _NWC_SRV_TYPE_NDS        3

#define _NWC_FSTYPE_210        0x00000000     /* servers <= 2.10 */
#define _NWC_FSTYPE_215        0x00000001     /* servers > 2.10 and < 3.00 */
#define _NWC_FSTYPE_310        0x00000002     /* servers >= 3.00 and <= 3.10 */
#define _NWC_FSTYPE_311        0x00000004     /* servers > 3.10 and < 4.00 */
#define _NWC_FSTYPE_LITE       0x00000008     /* NetWare Lite servers */
#define _NWC_FSTYPE_ENHANCED   0x00000010     /* Supports enhanced NCPs */
#define _NWC_FSTYPE_400        0x00000020     /* servers >= 4.00 */

/* Message Flags */
#define	MESSAGE_GET_NEXT_MESSAGE			1
#define	MESSAGE_RECEIVED_FOR_CONNECTION	2

/* Bits for the ConnectionFlags field in the connection table */
#define _NWC_CONN_FREE                  0x00000001     /* Connection is free. */
#define _NWC_CONN_CLAIMED               0x00000002     /* Connection has been tagged to be claimed */
#define _NWC_CONN_ATTACHED              0x00000004     /* Connection has created a service connection with a server */
#define _NWC_CONN_LOGGED_IN             0x00000008     /* Connection logged in to a server */
#define _NWC_CONN_PRIMARY               0x00000010     /* Primary or default connection */
#define _NWC_CONN_NO_BRDCAST            0x00000020     /* Connection broadcasts disabled */
#define _NWC_CONN_NO_GEN_BRDCAST        0x00000040     /* General broadcasts disabled */
#define _NWC_CONN_BRDCAST_WAITING       0x00000080     /* Broadcast message on this connection. */

#define _NWC_CONN_NDS_CONNECTION        0x00000100
#define _NWC_CONN_NDS_TEMPORARY         0x00000200     /* This is a temporary NDS connection. */
#define _NWC_CONN_NDS_AUTHENT           0x00000400     /* This is an authenticated NDS connection. */
#define _NWC_CONN_NDS_MONITORED         0x00000800     /* This is the NDS monitored connection. */
#define _NWC_CONN_NDS_PREF_TREE         0x00001000

#define _NWC_CONN_SFT3_CHANGING         0x00002000     /* SFT III is changing servers. */
#define _NWC_CONN_SFT3_COPY_IMMED       0x00004000     /* SFT III has changed servers. */
#define _NWC_CONN_SFT3_CACHE_FLUSHED    0x00008000     /* SFT III server cache flushed. */
#define _NWC_CONN_SFT3_IO_PENDING       0x00010000     /* IO is pending. */

#define _NWC_CONN_NEED_MAX_IO           0x00020000     /* This server needs maximum IO. */

#define _NWC_CONN_PBURST_AVAIL          0x00040000     /* Packet burst is available. */
#define _NWC_CONN_PBURST_RESET          0x00080000     /* Packet burst session is being reset. */

#define _NWC_CONN_DISCONNECTED          0x00100000     /* Connection was invalidated for some reason or another */
#define _NWC_CONN_TERMINATING           0x00200000     /* Connection is in the process of being torn down. */
#define _NWC_CONN_HEAVY_NETWARE         0x00400000
#define _NWC_CONN_LIGHT_NETWARE         0x00800000

#define _NWC_CONN_USER_SETTABLE_FLAGS   _NWC_CONN_NO_BRDCAST | _NWC_CONN_NO_GEN_BRDCAST | _NWC_CONN_NDS_AUTHENT

/* FSCTRL User requests for the NetWare Redirector

 Macro definition for defining FSCTRL function control codes. Function codes
 0-2047 are reserved for Microsoft. Function codes 2048-4096 are reserved
 for customers. The NetWare redirector will use codes starting at 3500.

 METHOD_DIRECT means we want the user's buffers passed directly to the file
 system where will will do the probing and locking.
*/
/*
#define FILE_DEVICE_NETWORK_FILE_SYSTEM 0x00000014
#define METHOD_NEITHER                  3
#define FILE_ANY_ACCESS                 0

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define _NWC_ATTACH  \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3500, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_DETACH  \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3501, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_LOGIN   \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3502, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_LOGOUT  \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3503, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_NET_REQUEST   \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3504, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_GET_PRIMARY_CONN  \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3505, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_SET_PRIMARY_CONN  \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3506, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_GET_CONN_INFO \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3507, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_GET_CONN_LIST \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3508, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_GET_SERVER_INFO  \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3509, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_MAP_SERVER_TO_CONN  \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3510, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_SET_CONN_FLAGS   \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3511, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_GET_REDIRECTOR_VERSION \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3512, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_MAP_DEVICE \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3513, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_UNMAP_DEVICE  \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3514, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_GET_MAPPED_PATH  \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3515, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_ENUM_MAPPED_DEVICES \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3516, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_GET_BCAST_MSG \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3517, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_CLEAR_MSG_EVENT \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3518, METHOD_NEITHER, FILE_ANY_ACCESS)

#define _NWC_GET_CONN_SLOT \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3519, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_GET_CONN_FROM_ADDR \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3520, METHOD_NEITHER, FILE_ANY_ACCESS)

#define _NWC_GET_TDS_INFO \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3521, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_ALLOC_TDS \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3522, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_FREE_TDS \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3523, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_READ_TDS \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3524, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_WRITE_TDS \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3525, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_GET_DEF_NAME_CONTEXT \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3526, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_SET_DEF_NAME_CONTEXT \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3527, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_IS_NDS_PRESENT \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3528, METHOD_NEITHER, FILE_ANY_ACCESS)

#define _NWC_GET_PREF_DS_TREE \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3529, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_SET_PREF_DS_TREE \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3530, METHOD_NEITHER, FILE_ANY_ACCESS)

#define _NWC_LOCK_CONN \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3531, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_UNLOCK_CONN \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3532, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_SET_PREF_SERVER \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3533, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_GET_PREF_SERVER \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3534, METHOD_NEITHER, FILE_ANY_ACCESS)

#define _NWC_SETUP_NCP_AUTHENTICATION \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3535, METHOD_NEITHER, FILE_ANY_ACCESS)

#define _NWC_GET_MONITORED_CONN \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3536, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_SET_MONITORED_CONN \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3537, METHOD_NEITHER, FILE_ANY_ACCESS)

#define _NWC_GET_PREF_DS_TREE_CONN \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3538, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_PREPARE_SHUTDOWN \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3539, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_GET_DEBUG_BUFFER \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3540, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_REGISTER_SERVICE_PROVIDER \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3541, METHOD_NEITHER, FILE_ANY_ACCESS)

#define _NWC_NET_REQUEST2 \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3542, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_ORDERED_NET_REQUEST_ALL \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3543, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_NET_REQUEST_ALL \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3544, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_ASSOCIATE_NW_HANDLE \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3545, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_GET_NW_FILE_HANDLE \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3546, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_GET_NUM_CONNS \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3547, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_GET_PRINTER_PORT_FLAGS \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3548, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_SET_PRINTER_PORT_FLAGS \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3549, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_DEBUG_BLOCK \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3550, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_DEBUG_UNBLOCK \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3551, METHOD_NEITHER, FILE_ANY_ACCESS)
#define _NWC_DEBUG_GET_RESOURCE \
			CTL_CODE(FILE_DEVICE_NETWORK_FILE_SYSTEM, 3552, METHOD_NEITHER, FILE_ANY_ACCESS)
*/

#define _NWC_ATTACH                    0x001436B3   /* NW_API_ATTACH                    */
#define _NWC_DETACH                    0x001436B7   /* NW_API_DETACH                    */
#define _NWC_LOGIN                     0x001436BB   /* NW_API_LOGIN                     */
#define _NWC_LOGOUT                    0x001436BF   /* NW_API_LOGOUT                    */
#define _NWC_NET_REQUEST               0x001436C3   /* NW_API_NET_REQUEST               */
#define _NWC_GET_PRIMARY_CONN          0x001436C7   /* NW_API_GET_PRIMARY_CONN_ID       */
#define _NWC_SET_PRIMARY_CONN          0x001436CB   /* NW_API_SET_PRIMARY_CONN_ID       */
#define _NWC_GET_CONN_INFO             0x001436CF   /* NW_API_GET_CONN_INFO             */
#define _NWC_GET_CONN_LIST             0x001436D3   /* NW_API_GET_CONN_LIST             */
#define _NWC_GET_SERVER_INFO           0x001436D7   /* NW_API_GET_SERVER_INFO           */
#define _NWC_MAP_SERVER_TO_CONN        0x001436DB   /* NW_API_MAP_SERVER_TO_CONN_ID     */
#define _NWC_SET_CONN_FLAGS            0x001436DF   /* NW_API_SET_CONNECTION_FLAGS      */
#define _NWC_GET_REDIR_VER             0x001436E3   /* NW_API_GET_REDIRECTOR_VERSION    */
#define _NWC_MAP_DEVICE                0x001436E7   /* NW_API_MAP_DEVICE                */
#define _NWC_UNMAP_DEVICE              0x001436EB   /* NW_API_UNMAP_DEVICE              */
#define _NWC_GET_MAPPED_PATH           0x001436EF   /* NW_API_GET_MAPPED_PATH           */
#define _NWC_ENUM_MAPPED_DEVICES       0x001436F3   /* NW_API_ENUM_MAPPED_DEVICES       */
#define _NWC_GET_BCAST_MSG             0x001436F7   /* NW_API_GET_BROADCAST_MESSAGE     */
#define _NWC_CLEAR_MSG_EVENT           0x001436FB   /* NW_API_CLEAR_MESSAGE_EVENT       */
#define _NWC_GET_CONN_SLOT             0x001436FF   /* NW_API_GET_CONNECTION_SLOT       */
#define _NWC_GET_CONN_FROM_ADDR        0x00143703   /* NW_API_GET_CONNID_FROM_ADDR      */
#define _NWC_GET_TDS_INFO              0x00143707   /* NW_API_GET_TDS_INFO              */
#define _NWC_ALLOC_TDS                 0x0014370B   /* NW_API_ALLOC_TDS                 */
#define _NWC_FREE_TDS                  0x0014370F   /* NW_API_FREE_TDS                  */
#define _NWC_READ_TDS                  0x00143713   /* NW_API_READ_TDS                  */
#define _NWC_WRITE_TDS                 0x00143717   /* NW_API_WRITE_TDS                 */
#define _NWC_GET_DEF_NAME_CONTEXT      0x0014371B   /* NW_API_GET_DEFAULT_NAME_CONTEXT  */
#define _NWC_SET_DEF_NAME_CONTEXT      0x0014371F   /* NW_API_SET_DEFAULT_NAME_CONTEXT  */
#define _NWC_IS_NDS_PRESENT            0x00143723   /* NW_API_IS_NDS_PRESENT            */
#define _NWC_GET_PREF_DS_TREE          0x00143727   /* NW_API_GET_PREFERRED_DS_TREE     */
#define _NWC_SET_PREF_DS_TREE          0x0014372B   /* NW_API_SET_PREFERRED_DS_TREE     */
#define _NWC_LOCK_CONN                 0x0014372F   /* NW_API_LOCK_CONNECT_SLOT         */
#define _NWC_UNLOCK_CONN               0x00143733   /* NW_API_UNLOCK_CONNECT_SLOT       */
#define _NWC_SET_PREF_SERVER           0x00143737   /* NW_API_SET_PREFERRED_SERVER      */
#define _NWC_GET_PREF_SERVER           0x0014373B   /* NW_API_GET_PREFERRED_SERVER      */
#define _NWC_SETUP_NCP_AUTHENTICATION  0x0014373F   /* NW_API_SETUP_NCP_AUTHENTICATION  */
#define _NWC_GET_MONITORED_CONN        0x00143743   /* NW_API_GET_MONITORED_CONNECTION  */
#define _NWC_SET_MONITORED_CONN        0x00143747   /* NW_API_SET_MONITORED_CONNECTION  */
#define _NWC_GET_PREF_DS_TREE_CONN     0x0014374B   /* NW_API_GET_PREF_DS_TREE_CONN_ID  */
#define _NWC_PREPARE_SHUTDOWN          0x0014374F   /* NW_API_PREPARE_SHUTDOWN          */
#define _NWC_GET_DEBUG_BUFFER          0x00143753   /* NW_API_GET_DEBUG_BUFFER          */
#define _NWC_REGISTER_SERVICE_PROVIDER 0x00143757   /* NW_API_REGISTER_SERVICE_PROVIDER */
#define _NWC_NET_REQUEST2              0x0014375B   /* NW_API_NET_REQUEST2              */
#define _NWC_ORDERED_NET_REQUEST_ALL   0x0014375F   /* NW_API_ORDERED_NET_REQUEST_ALL   */
#define _NWC_NET_REQUEST_ALL           0x00143763   /* NW_API_NET_REQUEST_ALL           */
#define _NWC_ASSOCIATE_NW_HANDLE       0x00143767   /* NW_API_ASSOCIATE_NETWARE_HANDLE  */
#define _NWC_GET_NW_FILE_HANDLE        0x0014376B   /* NW_API_GET_NETWARE_FILE_HANDLE   */
#define _NWC_GET_NUM_CONNS             0x0014376F   /* NW_API_GET_NUM_CONNECTIONS       */
#define _NWC_GET_PRINTER_PORT_FLAGS    0x00143773   /* NW_API_GET_PRINTER_PORT_FLAGS    */
#define _NWC_SET_PRINTER_PORT_FLAGS    0x00143777   /* NW_API_SET_PRINTER_PORT_FLAGS    */
#define _NWC_DEBUG_BLOCK               0x0014377B   /* NW_API_DEBUG_BLOCK               */
#define _NWC_DEBUG_UNBLOCK             0x0014377F   /* NW_API_DEBUG_UNBLOCK             */
#define _NWC_DEBUG_GET_RESOURCE        0x00143783   /* NW_API_DEBUG_GET_RESOURCE        */

#define _NWC_MAX_FS_CONTROL_CODE   _NWC_REGISTER_SERVICE_PROVIDER

#ifndef  _TDI_USER_
/*
 Known Address types
*/
#define TDI_ADDRESS_TYPE_UNSPEC    ((USHORT)0)  /* unspecified */
#define TDI_ADDRESS_TYPE_UNIX      ((USHORT)1)  /* local to host (pipes, portals) */
#define TDI_ADDRESS_TYPE_IP        ((USHORT)2)  /* internetwork: UDP, TCP, etc. */
#define TDI_ADDRESS_TYPE_IMPLINK   ((USHORT)3)  /* arpanet imp addresses */
#define TDI_ADDRESS_TYPE_PUP       ((USHORT)4)  /* pup protocols: e.g. BSP */
#define TDI_ADDRESS_TYPE_CHAOS     ((USHORT)5)  /* mit CHAOS protocols */
#define TDI_ADDRESS_TYPE_NS        ((USHORT)6)  /* XEROX NS protocols */
#define TDI_ADDRESS_TYPE_IPX       ((USHORT)6)  /* Netware IPX */
#define TDI_ADDRESS_TYPE_NBS       ((USHORT)7)  /* nbs protocols */
#define TDI_ADDRESS_TYPE_ECMA      ((USHORT)8)  /* european computer manufacturers */
#define TDI_ADDRESS_TYPE_DATAKIT   ((USHORT)9)  /* datakit protocols */
#define TDI_ADDRESS_TYPE_CCITT     ((USHORT)10) /* CCITT protocols, X.25 etc */
#define TDI_ADDRESS_TYPE_SNA       ((USHORT)11) /* IBM SNA */
#define TDI_ADDRESS_TYPE_DECnet    ((USHORT)12) /* DECnet */
#define TDI_ADDRESS_TYPE_DLI       ((USHORT)13) /* Direct data link interface */
#define TDI_ADDRESS_TYPE_LAT       ((USHORT)14) /* LAT */
#define TDI_ADDRESS_TYPE_HYLINK    ((USHORT)15) /* NSC Hyperchannel */
#define TDI_ADDRESS_TYPE_APPLETALK ((USHORT)16) /* AppleTalk */
#define TDI_ADDRESS_TYPE_NETBIOS   ((USHORT)17) /* Netbios Addresses */
#define TDI_ADDRESS_TYPE_8022      ((USHORT)18)
#define TDI_ADDRESS_TYPE_OSI_TSAP  ((USHORT)19)
#define TDI_ADDRESS_TYPE_NETONE    ((USHORT)20) /* for WzMail */

typedef struct _TA_ADDRESS
{
  USHORT len;          /* length in bytes of Address[] in this */
  USHORT type;         /* type of this address                 */
  UCHAR  data[1];      /* actually AddressLength bytes long    */
} TA_ADDRESS, *PTA_ADDRESS;

typedef struct _TRANSPORT_ADDRESS
{
  int numAddrs;                  /* number of addresses following */
  TA_ADDRESS addr[1];            /* actually numAddrs elements long */
} TRANSPORT_ADDRESS, *PTRANSPORT_ADDRESS;

/* NetBIOS */

typedef struct _TDI_ADDRESS_NETBIOS {
    USHORT NetbiosNameType;
    UCHAR NetbiosName[16];
} TDI_ADDRESS_NETBIOS, *PTDI_ADDRESS_NETBIOS;

#define TDI_ADDRESS_NETBIOS_TYPE_UNIQUE ((USHORT)0x0000)
#define TDI_ADDRESS_NETBIOS_TYPE_GROUP  ((USHORT)0x0001)

#define TDI_ADDRESS_LENGTH_NETBIOS sizeof (TDI_ADDRESS_NETBIOS)

/* Xns address for UB */

typedef struct _TDI_ADDRESS_NETONE {
    USHORT NetoneNameType;
    UCHAR NetoneName[20];
} TDI_ADDRESS_NETONE, *PTDI_ADDRESS_NETONE;

#define TDI_ADDRESS_NETONE_TYPE_UNIQUE  ((USHORT)0x0000)
#define TDI_ADDRESS_NETONE_TYPE_ROTORED ((USHORT)0x0001)

#define TDI_ADDRESS_LENGTH_NETONE sizeof (TDI_ADDRESS_NETONE)

/* 802.2 MAC addresses */

typedef struct _TDI_ADDRESS_8022 {
    UCHAR MACAddress[6];
} TDI_ADDRESS_8022, *PTDI_ADDRESS_8022;

#define TDI_ADDRESS_8022_LENGTH  sizeof (TDI_ADDRESS_8022);

/* IP address */

typedef struct _TDI_ADDRESS_IP {
    USHORT sin_family;
    USHORT sin_port;
    ULONG  in_addr;
    UCHAR  sin_zero[8];
} TDI_ADDRESS_IP, *PTDI_ADDRESS_IP;

#define TDI_ADDRESS_IP_AF_UNSPEC        ((USHORT) 0x0000)
#define TDI_ADDRESS_IP_AF_UNIX          ((USHORT) 0x0001)
#define TDI_ADDRESS_IP_AF_INET          ((USHORT) 0x0002)
#define TDI_ADDRESS_IP_PF_UNSPEC        TDI_ADDRESS_IP_AF_UNSPEC
#define TDI_ADDRESS_IP_PF_UNIX          TDI_ADDRESS_IP_AF_UNIX
#define TDI_ADDRESS_IP_PF_INET          TDI_ADDRESS_IP_AF_INET

/* #define TDI_ADDRESS_LENGTH_IP sizeof(TDI_ADDRESS_IP) */

typedef struct	_TDI_ADDRESS_IPX {
   ULONG    networkAddress;
   UCHAR    nodeAddress[6];
	USHORT	Socket;
} TDI_ADDRESS_IPX, *PTDI_ADDRESS_IPX;

/* #define  TDI_ADDRESS_IPX_LENGTH  sizeof(TDI_ADDRESS_IPX); */
#endif /* _TDI_USER_ */

/* Novell's definition of a transport independent address */
typedef TRANSPORT_ADDRESS NETWORK_ADDRESS, *PNETWORK_ADDRESS;

typedef struct NWCNT_FRAG
{
  ULONG len;
  PVOID addr;
} NWCNT_FRAG, *pNWCNT_FRAG;

typedef struct NWCNT_ATTACH
{
  IN  NUNICODE_STRING srvName;
  OUT ULONG          conn;
} NWCNT_ATTACH, *pNWCNT_ATTACH;

typedef struct NWCNT_DETACH
{
  IN  ULONG   conn;
  IN  ULONG   connType;
} NWCNT_DETACH, *pNWCNT_DETACH;

typedef struct NWCNT_GET_PRIMARY_CONN
{
  OUT ULONG   conn;
} NWCNT_GET_PRIMARY_CONN, *pNWCNT_GET_PRIMARY_CONN;

typedef struct NWCNT_SET_PRIMARY_CONN
{
  IN  ULONG   newPrimary;
  OUT ULONG   oldPrimary;
} NWCNT_SET_PRIMARY_CONN, *pNWCNT_SET_PRIMARY_CONN;

typedef struct NWCNT_NET_REQUEST
{
   IN       ULONG         conn;
   IN       ULONG         function;
   IN       ULONG         numReqFrags;
   IN       ULONG         numReplyFrags;
   IN OUT   NWCNT_FRAG    frags[1];
} NWCNT_NET_REQUEST, *pNWCNT_NET_REQUEST;

typedef struct NWCNT_NET_REQUEST2
{
   IN       ULONG         conn;
   IN       ULONG         function;
   IN       ULONG         numReqFrags;
   IN       pNWCNT_FRAG   reqFrags;
   IN       ULONG         numReplyFrags;
   IN OUT   pNWCNT_FRAG   replyFrags;
} NWCNT_NET_REQUEST2, *pNWCNT_NET_REQUEST2;

typedef struct NWCNT_NET_REQUEST_ALL
{
   IN       ULONG          function;
   IN       ULONG          numReqFrags;
   IN       pNWCNT_FRAG    reqFrags;
   IN       ULONG          numReplyFrags;
   IN OUT   pNWCNT_FRAG    replyFrags;
   OUT      ULONG          replyLen;
} NWCNT_NET_REQUEST_ALL, *pNWCNT_NET_REQUEST_ALL;

typedef struct NWCNT_LOGIN
{
  IN  ULONG          conn;
  IN  ULONG          objType;
  IN  NUNICODE_STRING objName;
  IN  NUNICODE_STRING password;
} NWCNT_LOGIN, *pNWCNT_LOGIN;

typedef struct _NWCNT_LOGOUT_
{
  IN  ULONG  conn;
} NWCNT_LOGOUT, *pNWCNT_LOGOUT;

typedef  struct NWCNT_CONN_NUM
{
  UCHAR   low;
  UCHAR   high;
  USHORT  reserved;
} NWCNT_CONN_NUM, *pNWCNT_CONN_NUM;

/* Levels of info available from the GetConnectionInfo FSCTRL */
#define NWC_CONN_INFO_USR   0x00000001
#define NWC_CONN_INFO_SRV   0x00000002
#define NWC_CONN_INFO_DS    0x00000003
#define NWC_CONN_INFO_MISC  0x00000004

typedef struct NWCNT_CONN_INFO
{
   IN    ULONG            conn;
   IN    ULONG            level;
   OUT   union
         {
            struct
            {
            OUT NUNICODE_STRING name;
            OUT ULONG          objType;
            OUT ULONG          objID;
            OUT struct
                  {
                     ULONG low;
                     ULONG high;
                  } time;
            } usr;

            struct
            {
            OUT NUNICODE_STRING     name;
            OUT ULONG              type;
            OUT PTRANSPORT_ADDRESS addr;
            } srv;

            struct
            {
            OUT ULONG timeDistance;
            OUT ULONG tConnectUseCount;
            OUT ULONG tConnectTimeStamp;
            } ds;

            struct
            {
            OUT ULONG            connFlags;
            OUT NWCNT_CONN_NUM   connNum;
            OUT ULONG            refCount;
            OUT ULONG            maxPacketSize;
            OUT ULONG            nameSpaceMask;
            OUT ULONG            defRetryCount;
            } misc;
         } info;
} NWCNT_CONN_INFO, *pNWCNT_CONN_INFO;

typedef struct NWCNT_SET_CONNECTION_FLAGS
{
   IN ULONG conn;
   IN ULONG connFlags;
   IN ULONG changeMask;
} NWCNT_SET_CONN_FLAGS, *pNWCNT_SET_CONN_FLAGS;

typedef struct NWCNT_CONN_LIST
{
  IN  ULONG connListSize;
  OUT PVOID connListBuf;
  OUT ULONG numConns;
} NWCNT_CONN_LIST, *pNWCNT_CONN_LIST;

typedef struct NWCNT_SERVER_INFO
{
  IN  ULONG srvInfo;
  IN  ULONG connListSize;
  OUT PVOID connListBuf;
  OUT ULONG numConns;
} NWCNT_SERVER_INFO, *pNWCNT_SERVER_INFO;

typedef struct NWCNT_SERVER_TO_CONN
{
  IN  NUNICODE_STRING srvName;
  OUT ULONG          conn;
} NWCNT_SERVER_TO_CONN, *pNWCNT_SERVER_TO_CONN;

typedef struct NWCNT_REDIR_VER
{
  OUT ULONG majorVer;
  OUT ULONG minorVer;
  OUT ULONG rev;
} NWCNT_REDIR_VER, *pNWCNT_REDIR_VER;

typedef struct NWCNT_MAP_DEVICE
{
  IN  ULONG             conn;
  IN  ULONG             deviceType;
  IN  NUNICODE_STRING   deviceName;
  IN  NUNICODE_STRING   targetObj;
  IN  ULONG             mapType;
} NWCNT_MAP_DEVICE, *pNWCNT_MAP_DEVICE;

typedef struct NWCNT_UNMAP_DEVICE
{
  IN  ULONG          conn;
  IN  NUNICODE_STRING deviceName;
} NWCNT_UNMAP_DEVICE, *pNWCNT_UNMAP_DEVICE;

typedef struct NWCNT_GET_MAPPED_PATH
{
   IN     NUNICODE_STRING  deviceName;
   IN OUT NUNICODE_STRING  uncPath;
   OUT    ULONG            conn;
} NWCNT_GET_MAPPED_PATH, *pNWCNT_GET_MAPPED_PATH;

typedef struct NWCNT_ENUM_MAPPED_DEVICES
{
   IN  ULONG   bufLen;
   IN  ULONG   deviceType;
   OUT PWSTR   deviceBuf;
   OUT ULONG   devicesEnumerated;
} NWCNT_ENUM_MAPPED_DEVICES, *pNWCNT_ENUM_MAPPED_DEVICES;

typedef struct NWCNT_GET_BCAST_MSG
{
   IN       ULONG msgFlags;
   IN OUT   ULONG conn;
} NWCNT_GET_BCAST_MSG, *pNWCNT_GET_BCAST_MSG;

typedef struct NWCNT_GET_CONN_SLOT
{
   IN  ULONG               connType;
   IN  ULONG               tranAddrLen;
   IN  PTRANSPORT_ADDRESS  tranAddr;
   OUT ULONG               conn;
} NWCNT_GET_CONN_SLOT, *pNWCNT_GET_CONN_SLOT;

typedef struct NWCNT_GET_CONN_FROM_ADDR
{
  IN  ULONG              tranAddrLen;
  IN  PTRANSPORT_ADDRESS tranAddr;
  OUT ULONG              conn;
} NWCNT_GET_CONN_FROM_ADDR, *pNWCNT_GET_CONN_FROM_ADDR;

typedef struct NWCNT_GET_TDS_INFO
{
  IN  ULONG tag;
  OUT ULONG maxSize;
  OUT ULONG dataSize;
  OUT ULONG tdsFlags;
} NWCNT_GET_TDS_INFO, *pNWCNT_GET_TDS_INFO;

typedef struct NWCNT_ALLOC_TDS
{
  IN  ULONG tag;
  IN  ULONG tdsSize;
  IN  ULONG tdsFlags;
} NWCNT_ALLOC_TDS, *pNWCNT_ALLOC_TDS;

typedef struct NWCNT_FREE_TDS
{
  IN  ULONG tag;
} NWCNT_FREE_TDS, *pNWCNT_FREE_TDS;

typedef struct NWCNT_READ_TDS
{
  IN  ULONG tag;
  IN  ULONG readLen;
  IN  ULONG readOffset;
  OUT PVOID buf;
  OUT ULONG bytesRead;
} NWCNT_READ_TDS, *pNWCNT_READ_TDS;

typedef struct NWCNT_WRITE_TDS
{
  IN  ULONG tag;
  IN  ULONG writeLen;
  IN  ULONG writeOffset;
  IN  PVOID buf;
  OUT ULONG bytesWritten;
} NWCNT_WRITE_TDS, *pNWCNT_WRITE_TDS;

typedef struct NWCNT_GET_DEF_NAME_CONTEXT
{
  IN OUT ULONG  defNameContextLen;
  OUT    PUCHAR defNameContext;
} NWCNT_GET_DEF_NAME_CONTEXT, *pNWCNT_GET_DEF_NAME_CONTEXT;

typedef struct NWCNT_SET_DEF_NAME_CONTEXT
{
  IN  ULONG  defNameContextLen;
  IN  PUCHAR defNameContext;
} NWCNT_SET_DEF_NAME_CONTEXT, *pNWCNT_SET_DEF_NAME_CONTEXT;

typedef struct NWCNT_IS_NDS_PRESENT
{
  ULONG dummy;
} NWCNT_IS_NDS_PRESENT, *pNWCNT_IS_NDS_PRESENT;

typedef struct NWCNT_GET_PREF_DS_TREE
{
  IN OUT ULONG  treeNameLen;
  OUT    PUCHAR treeName;
} NWCNT_GET_PREF_DS_TREE, *pNWCNT_GET_PREF_DS_TREE;

typedef struct NWCNT_SET_PREF_DS_TREE
{
  IN ULONG  treeNameLen;
  IN PUCHAR treeName;
} NWCNT_SET_PREF_DS_TREE, *pNWCNT_SET_PREF_DS_TREE;

typedef struct NWCNT_LOCK_CONN
{
  IN ULONG conn;
} NWCNT_LOCK_CONN, *pNWCNT_LOCK_CONN;

typedef struct NWCNT_UNLOCK_CONN
{
  IN ULONG conn;
} NWCNT_UNLOCK_CONN, *pNWCNT_UNLOCK_CONN;

typedef struct _NWCNT_SET_PREF_SERVER_
{
  IN ULONG  srvNameLen;
  IN PUCHAR srvName;
} NWCNT_SET_PREF_SERVER, *pNWCNT_SET_PREF_SERVER;

typedef struct _NWCNT_GET_PREF_SERVER_
{
  IN OUT ULONG  srvNameLen;
  OUT    PUCHAR srvName;
} NWCNT_GET_PREF_SERVER, *pNWCNT_GET_PREF_SERVER;

typedef struct NWCNT_SETUP_NCP_AUTHENTICATION
{
  ULONG dummy;
} NWCNT_SETUP_NCP_AUTHENTICATION, *pNWCNT_SETUP_NCP_AUTHENTICATION;

typedef struct NWCNT_GET_MONITORED_CONN
{
  OUT ULONG conn;
} NWCNT_GET_MONITORED_CONN, *pNWCNT_GET_MONITORED_CONN;

typedef struct NWCNT_SET_MONITORED_CONN
{
  IN  ULONG conn;
} NWCNT_SET_MONITORED_CONN, *pNWCNT_SET_MONITORED_CONN;

typedef struct NWCNT_GET_PREF_DS_TREE_CONN
{
  OUT ULONG conn;
} NWCNT_GET_PREF_DS_TREE_CONN, *pNWCNT_GET_PREF_DS_TREE_CONN;

typedef struct NWCNT_GET_DEBUG_BUFFER
{
  PUCHAR buf;
} NWCNT_GET_DEBUG_BUFFER, *pNWCNT_GET_DEBUG_BUFFER;

typedef struct NWCNT_ASSOCIATE_NETWARE_HANDLE_
{
   IN       ULONG       conn;
   IN       UCHAR       NWHandle[6];
} NWCNT_ASSOCIATE_NW_HANDLE, *pNWCNT_ASSOCIATE_NW_HANDLE;

typedef struct NWCNT_GET_NETWARE_FILE_HANDLE
{
   OUT      ULONG       conn;
   OUT      UCHAR       NWHandle[6];
} NWCNT_GET_NW_FILE_HANDLE, *pNWCNT_GET_NW_FILE_HANDLE;

typedef struct NWCNT_GET_NUM_CONNS
{
   OUT      ULONG       numConns;
} NWCNT_GET_NUM_CONNS, *pNWCNT_GET_NUM_CONNS;

/* Get/Set Items for the printer port api */
#define _NWC_PRN_BANNER                0x00000001
#define _NWC_PRN_FORM_FEED             0x00000002
#define _NWC_PRN_NUM_COPIES            0x00000004
#define _NWC_PRN_TAB_SIZE              0x00000008
#define _NWC_PRN_MAX_CHARS_PER_LINE    0x00000010
#define _NWC_PRN_MAX_LINES_PER_PAGE    0x00000020
#define _NWC_PRN_JOB_DESC              0x00000040
#define _NWC_PRN_SETUP_STRING          0x00000080
#define _NWC_PRN_RESET_STRING          0x00000100

typedef struct NWCNT_PRINTER_PORT_FLAGS
{
   IN       NUNICODE_STRING   deviceName;
   IN       ULONG             getSetFlag;
   OUT      BOOLEAN           banner;
   OUT      BOOLEAN           formFeed;
   IN       USHORT            reserved;
   OUT      ULONG             numCopies;
   OUT      ULONG             tabSize;
   OUT      ULONG             maxCharsPerLine;
   OUT      ULONG             maxLinesPerPage;
   IN       ULONG             jobDescLen;
   IN OUT   PWSTR             jobDesc;
   IN       ULONG             setupStrLen;
   IN OUT   PVOID             setupStr;
   IN       ULONG             resetStrLen;
   IN OUT   PVOID             resetStr;
} NWCNT_PRINTER_PORT_FLAGS, *pNWCNT_PRINTER_PORT_FLAGS;

/* stuff for NWClient pieces */
N_EXTERN_LIBRARY( pnstr8 )
NWCStrWcToMb
(
  pnstr8    pMbStr,
  pnstr16   pWcStr
);

N_EXTERN_LIBRARY( pnstr16 )
NWCStrMbToWc
(
  pnstr16   pWcStr,
  pnstr8    pMbStr
);

extern HANDLE _hNetWare;

N_EXTERN_LIBRARY( NWRCODE )
NWCNTReq
(
  nuint32   request,
  nptr      pInBuf,
  nuint32   inLen,
  nptr      pOutBuf,
  nuint32   outLen
);

#endif /* platforms */

#include <npackoff.h>

#endif /* NWCINT_H */

