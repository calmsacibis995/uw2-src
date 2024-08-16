/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/nwintern.h	1.4"
/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWLIB_INTERNAL
#define NWLIB_INTERNAL

#ifndef NWCALDEF_INC
# include <nwcaldef.h>
#endif

#include <npackon.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef NWNOT_USED
#define NWNOT_USED( n ) n = n;
#endif

#ifndef NWFP_OFF
#define NWFP_OFF(fp) ((nuint16)(((nuint32)(void far *)(fp)) & 0xFFFF))
#endif

#ifndef NWFP_SEG
#define NWFP_SEG(fp) ((nuint16)(((nuint32)(void far *)(fp)) >> 16))
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

#define NWI_REGISTER_SPOOLER         0x0019
#define NWI_CLEANUP_COMPLETE         0x001A
#define NWI_RESET_BCAST_DAEMON       0x001B
#define NWI_REGISTER_JANITOR         0x001C
#define NWI_GET_TIME_STAMP           0x001D
#define NWI_TOGGLE_LNS               0x001F
#define NWI_CONNECTION_TABLE         0x0020
#define NWI_MAP_FLAT_TO_SELECTOR     0x0021
#define NWI_FREE_MAPPED_SELECTOR     0x0022
#define NWI_DISPLAY_HARD_ERRORS      0x0023
#define NWI_BLOCK_DAEMON             0x0024
#define NWI_NDS_GET_CONNECTION_INFO  0x0025
#define NWI_NDS_SET_CONNECTION_INFO  0x0026
#define NWI_GET_CONNECTION_SLOT      0x0027
#define NWI_FREE_CONNECTION_SLOT     0x0028
#define NWI_GET_CONNID_FROM_ADDR     0x0029
#define NWI_GET_TDS_INFO             0x002A
#define NWI_ALLOC_TDS                0x002B
#define NWI_FREE_TDS                 0x002C
#define NWI_READ_TDS                 0x002D
#define NWI_WRITE_TDS                0x002E
#define NWI_GET_DEFAULT_NAME_CONTEXT 0x002F
#define NWI_SET_DEFAULT_NAME_CONTEXT 0x0030
#define NWI_IS_NDS_PRESENT           0x0031
#define NWI_GET_INIT_DIRECTORY_SERVER 0x0032
#define NWI_FREE_TASK_RESOURCE       0x33
#define NWI_GET_TASK_RESOURCE        0x34
#define NWI_NOT_USED                 0x35
#define NWI_LOGOUT_WITH_CITRIX_ID    0x36
#define NWI_LOCK_CONNECTION_SLOT     0x37
#define NWI_UNLOCK_CONNECTION_SLOT   0x38
#define NWI_SET_PREFERRED_SERVER     0x39
#define NWI_GET_PREFERRED_SERVER     0x3A
#define NWI_CREATEKEY_CODE           0x3c
#define NWI_GET_MONITORED_CONN       0x3D
#define NWI_SET_MONITORED_CONN       0x3E

#define NET_SPECIAL_FILE "\\\\SPCLNET$\\SPCLNET$"

#define MAX_CONNECTIONS              32

extern nuint16 (N_API *NWCallGate)(nuint16, nptr);
extern void N_API CleanUpList(unsigned short); /* internal function in SYNC */

#else

/* DOS specific stuff */

/* VLM IDs */

#define VLM_ID_VLM    0x0001 /* VLM manager module                          */
#define VLM_ID_CONN   0x0010 /* Connection table manager module             */
#define VLM_ID_TRAN   0x0020 /* Transport multiplex module                  */
#define VLM_ID_IPX    0x0021 /* IPX transport module                        */
#define VLM_ID_TCP    0x0022 /* TCP/IP transport module                     */
#define VLM_ID_NWP    0x0030 /* NetWare protocol multiplex module           */
#define VLM_ID_BIND   0x0031 /* NetWare Bindery protocol module             */
#define VLM_ID_NDS    0x0032 /* NetWare Directory Services protocol module  */
#define VLM_ID_LITE   0x0033 /* NetWare Lite protocol module                */
#define VLM_ID_PNW    0x0033 /* Personal NetWare protocol module            */
#define VLM_ID_RSA    0x0034 /* RSA module used for NDS functions           */
#define VLM_ID_REDIR  0x0040 /* Redirector module                           */
#define VLM_ID_FIO    0x0041 /* File protocol module                        */
#define VLM_ID_PRINT  0x0042 /* Print module                                */
#define VLM_ID_GENR   0x0043 /* General purpose functions                   */
#define VLM_ID_NETX   0x0050 /* NETX compatible module                      */
#define VLM_ID_AUTO   0x0060 /* Auto reconnect/retry module                 */
#define VLM_ID_NMR    0x0100 /* Network Management Responder module         */
#define VLM_ID_NONAME 0x9999 /* NONAME overlay ID for NONAME.ASM            */

/* VLM function codes */
/* VLM.EXE module functions */

#define VLM_GEN    1   /* Common generic function w/multiple subfunctions */
#define   GEN_VER  0   /* Generic version function                        */
#define   GEN_CONN_ADD 2  /* Add a new connection                         */
#define   GEN_CONN_DEL 3  /* Delete an existing connection                */
#define   GEN_LOG_ADD  4  /* Login an existing connection                 */
#define   GEN_LOG_DEL  5  /* Logout an existing connection                */
#define   GEN_DRV_ADD  6  /* Add a new drive redirection                  */
#define   GEN_DRV_DEL  7  /* Delete a drive redirection                   */
#define   GEN_PRN_ADD  8  /* Add a new printer redirection                */
#define   GEN_PRN_DEL  9  /* Delete a printer redirection                 */
#define   GEN_CHNG_DIR 10 /* Change directories                           */
#define   GEN_PREL_DEL 11 /* Pre logout from server                       */
#define VLM_STA    3   /* Common VLM statistics function                  */
#define VLM_COM    4   /* VLM common function                             */

/* CONN.VLM functions (CONN) */

#define CONN_STA   3   /* CONN get statistics */
#define CONN_ALO   4   /* CONN alocate connection entry              */
#define CONN_VAL   5   /* CONN validate connection entry handle      */
#define CONN_FRE   6   /* CONN free connection entry handle          */
#define CONN_GET   7   /* CONN get element of connection entry       */
#define CONN_SET   8   /* CONN set element of connection entry       */
#define CONN_RES   9   /* CONN reset element of connection entry     */
#define CONN_LUP   10  /* CONN lookup handle by entry value          */
#define CONN_NLUN  13  /* CONN lookup name by connection             */
#define CONN_NLUC  14  /* CONN lookup connection by name             */
#define CONN_GNUM  15  /* CONN return num of conn entries supported  */
#define CONN_TID   16  /* CONN API to manage task IDs                */
#define  TID_GET   0   /* Get current task ID                        */
#define  TPS_CLR   1   /* Clear a task from table by PSP             */
#define  TVM_CLR   2   /* Clear a whole VM from the task table       */
#define  TLOCK_SET 3   /* Task lock mode set                         */
#define  TLOCK_GET 4   /* Task lock mode get                         */
#define  SET_GHARD 5   /* inc/dec the global hard count              */

#define STATIC_CONN_TYPE   0x1 /* Static connection type */
#define DYNAMIC_CONN_TYPE  0   /* Dynamic connection type */

#define CEI_ERROR          0  /* F R - CEI error and status */
#define CEI_NET_TYPE       1  /* V 2 - CEI network type */
#define CEI_NON_AUTH       2  /* F R - CEI connection not authenticated flag */
#define CEI_AUTH           3  /* F R - CEI connection authenticated flag */
#define CEI_PBURST         4  /* F   - CEI packet burst */
#define CEI_CHANGING       5  /* F R */
#define CEI_NEEDS_MAXIO    6  /* F R - CEI Max IO is needed */
#define CEI_PBURST_RESET   7  /* F R */
#define CEI_MAJ_MIN_VER    8  /* V 2 */
#define CEI_REF_COUNT      9  /* V 2 */
#define CEI_DIST_TIME     10  /* V 2 - CEI distance to the server in time */
#define CEI_MAX_IO        11  /* V 2 - CEI Max IO is needed */
#define CEI_NCP_SEQ       12  /* V 1 */
#define CEI_CONN_NUM      13  /* V 2 - CEI connection number */
#define CEI_ORDER_NUM     14  /* V 1 - if a conn has order number, it is valid */
#define CEI_TRAN_TYPE     15  /* V 2 - CEI transport type */
#define CEI_NCP_REQ_TYPE  16  /* V 2 */
#define CEI_TRAN_BUFF     17  /* V buffer - CEI transport buffer */
#define CEI_BCAST         18  /* F R */
#define CEI_LIP           19  /* V 2 */
#define CEI_SECURITY      20  /* F R */
#define CEI_RES_SOFT_COUNT 21 /* V 2 */

/* NWP.VLM module functions */
#define NWP_CONN      4   /* NWP connect to server                  */
#define NWP_DISC      5   /* NWP disconnect from server             */
#define NWP_ATTACH    6   /* NWP attach to server (SYSTEM ONLY)     */
#define NWP_LOGIN     8   /* NWP login to connected server          */
#define NWP_LOGOUT    9   /* NWP logout of connected server         */
#define NWP_SPEC      10
#define  GET_PREF_SRV 0   /* NWP get preferred server               */
#define  SET_PREF_SRV 1   /* NWP set preferred server               */
#define  GETSET_BCAST 2   /* NWP get/set broadcast mode             */
#define  GET_OBJ_ID   3   /* NWP get bindery object ID              */
#define  GET_OBJ_NAME 4   /* NWP get bindery object name            */
#define  GET_MSG      5   /* NWP get/display broadcast message      */
#define NWP_BIND      11  /* NWP bind specific functions            */
#define NWP_NDS       12  /* NWP NDS specific functions             */
#define NWP_LITE      13  /* NWP lite specific functions            */
#define NWP_ORD_ALL   14  /* NWP ordered NCP send to all            */
#define NWP_MD4       16

/* TRAN.VLM functions (TRAN/IPX/TCP) */
#define TRAN_1ST  4   /* TRAN connect to first available service  */
#define TRAN_REQ  6   /* TRAN request/reply function              */
#define TRAN_CAN  7   /* TRAN cancel request/reply (system only)  */
#define TRAN_SCH  8   /* TRAN schedule asynchronous event         */
#define TRAN_MAX  9   /* TRAN get maximum physical size           */
#define  NODE_MAX  0  /* TRAN get maximum node size               */
#define  ROUTE_MAX 1  /* TRAN get maximum route size              */
#define TRAN_UDS  11  /* TRAN undirected send                     */

/* NDS.VLM sub-functions */

#define NDS_GDN   0   /* NDS get default context name */
#define NDS_SDN   1   /* NDS set default context name */
#define NDS_RTD   2   /* NDS read tagged data store   */
#define NDS_WTD   3   /* NDS write tagged data store  */
#define NDS_GTD   4   /* NDS get TDS control info     */
#define NDS_RCC   5   /* NDS resource count change    */
#define NDS_GSL   7   /* NDS get/set logged server    */
#define   GET_LOGGED  1
#define   SET_LOGGED  2

/* PRINT.VLM module functions */
#define PRINT_SGPD 4  /* Set/Get the print specific structures    */
#define PRINT_MOD  5  /* Set/Get the printer mode (binary/text)   */
#define PRINT_WRT  6  /* Write block of data to queue job         */
#define PRINT_GPN  7  /* Get the number of printers               */
#define PRINT_GSR  8  /* Set/Get/Cancel print redirection state   */
#define PRINT_FCJ  9  /* Flush & close the print queue job        */
#define PRINT_BAN  12 /* Set/Get global banner name               */
#define PRINT_HT   13 /* Set/Get the pointer to either the setup  */
                      /* or reset strings (header/tail)           */

/* REDIR.VLM module functions */
#define REDIR_BLD_SFT   4 /* Get available SFT and map it to NW file*/
#define REDIR_FILE_INFO 5 /* Convert file handle to NW handle       */
#define REDIR_SEARCH_STRING 6 /* Searches string until specified character or NULL */
#define REDIR_APIS      8     /* Subfunction dispatcher for APIs                   */
#define   REDIR_GET_ITEM   0  /* Get information for a specified device            */
#define   REDIR_SET_SHARE  1  /* Set share bit on/off applied to next function     */
#define   REDIR_UPDATE_DIR_HANDLE 2  /* Update the directory handle at reconnect time */

/* GENERAL.VLM module functions */
#define GENR_GSPR       4 /* get/set primary connection             */
#define GENR_RMC        5 /* Restore main comspec                   */
#define GENR_SPEC       6 /* Misc GENERAL functions                 */
#define   GEN_GET_ENV   0 /* Get command com and master environment */
#define   GEN_DEF_PRIM  1 /* Gets the default or primary connection */
#define   GEN_LAST_Q    2 /* Get/Set last Q accessed info (NETQ)    */
#define     ZAP_LAST_Q  0
#define     SET_LAST_Q  1
#define     GET_LAST_Q  2
#define   GEN_GET_SET_MNAME 3
#define     GET_SHORT_NAME  0
#define     GET_LONG_NAME   2
#define     SET_SHORT_NAME  4
#define     SET_LONG_NAME   6
#define     GET_DOS_NAME    8
#define     SET_DOS_NAME    10
#define GENR_3D         7 /* Open search mode handler               */
#define GENR_DSD        8 /* delete search drive from mastr env     */
#define GENR_RDI        9 /* return drive information               */
#define   GET_FIRST_AVAIL_DRIVE 0 /* returns first available drive  */
#define   GET_CDS_INFO          1 /* get drive table info           */


#define GET_PRIMARY_CONNECTION  1
#define SET_PRIMARY_CONNECTION  2

/* AUTO.VLM module functions  */
#define AUTO_RECON 4      /* Reconnect a connection handle          */

/* FIO.VLM module functions */
#define FIO_OPEN_NOTIFY 4 /* File open notify */
#define FIO_CLOSE_FILE  5 /* Close file       */
#define FIO_COMMIT      6 /* Commit file      */
#define FIO_WRITE       7 /* Write file       */
#define FIO_READ        8 /* Read file        */
#define FIO_FLUSH       9 /* Flush file       */
#define FIO_RFC        10 /* Remote file copy */

#define GLOBAL_DIS         0x0
#define DESTROY_CONN       0x1

#define SET_PREFERRED   0xf000
#define CARRY_FLAG      0x0001

#endif

typedef struct PATHPOINTERStag
{
  pnstr8 startServer;
  nint16 serverLength;
  pnstr8 startVolume;
  nint16 volumeLength;
  pnstr8 startPath;
  nint16 pathLength;
  nuint16 drivePathFlag;
} PATHPOINTERS;

#define CLIENT_AREA_SIZE 152
#define JOB_DESCRIPTION_SIZE 50

N_EXTERN_LIBRARY( NWCCODE )
NWDosClose
(
   NWFILE_HANDLE fileHandle
);

N_EXTERN_LIBRARY( NWCCODE )
NWDosOpen
(
   pnstr8   pbstrFileName,
   NWFILE_HANDLE N_FAR * fileHandle,
   pnuint16 psuActionTaken,
   nuint32  luFileSize,
   nuint16  suAttr,
   nuint16  suOpenFlags,
   nuint16  suOpenMode,
   nuint32  luReserved
);

N_EXTERN_LIBRARY( NWCCODE )
CleanPath
(
   pnstr8   pbstrPath
);

N_EXTERN_LIBRARY( NWCCODE )
IndexPath
(
   pnstr8   pbstrPath,
   pnuint16 psuStartServer,
   pnuint16 psuServerLen,
   pnuint16 psuStartVolume,
   pnuint16 psuVolumeLen,
   pnuint16 psuStartPath
);

N_EXTERN_LIBRARY( NWCCODE )
_NWCreateSessionKey
(
   NWCONN_HANDLE conn,
   pnuint8 pbuSessionKey
);

N_EXTERN_LIBRARY( NWCCODE )
_NWGetSessionKey
(
   NWCONN_HANDLE conn,
   pnuint8 pbuSessionKey
);

N_EXTERN_LIBRARY( NWCCODE )
_NWRenegotiateSecurityLevel
(
   NWCONN_ID conn
);

N_EXTERN_LIBRARY( NWCCODE )
_NWGetSecurityFlags
(
   NWCONN_HANDLE conn,
   pnuint32 pluFlags
);

N_EXTERN_LIBRARY( NWCCODE )
_NWSetSecurityFlags
(
   NWCONN_HANDLE conn,
   nuint32 luFlags
);

#ifdef THISISAQMSFUNCTION

#ifdef NWOS2
extern nint8 _NWNetSpecialFile[];
#elif defined(WIN32)
#else
extern nint8 _NWQMSJobFileName[];
#endif

#ifndef NWQMS_INC
#include <nwqms.h>
#endif

N_EXTERN_LIBRARY( void )
ConvertQueueToNWQueue
(
  NWQueueJobStruct N_FAR *NWJob,
  QueueJobStruct N_FAR   *qJob
);

N_EXTERN_LIBRARY( void )
ConvertNWQueueToQueue
(
   QueueJobStruct N_FAR   *qJob,
   NWQueueJobStruct N_FAR *NWJob
);

N_EXTERN_LIBRARY( void )
__NWSwapJobStructIDs
(
   QueueJobStruct N_FAR  *pJob
);

N_EXTERN_LIBRARY( void )
__NWSwapNWJobStructIDs
(
   NWQueueJobStruct N_FAR *pJob
);

#endif

#ifdef __cplusplus
}
#endif

#include <npackoff.h>

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/inc/nwintern.h,v 1.4 1994/09/26 17:09:31 rebekah Exp $
*/
