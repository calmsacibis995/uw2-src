/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwconnec.h	1.6"
/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWCONNECT_INC
#define NWCONNECT_INC

#ifndef NWCALDEF_INC
#ifdef N_PLAT_UNIX
# include <nw/nwcaldef.h>
#else /* !N_PLAT_UNIX */
# include <nwcaldef.h>
#endif /* N_PLAT_UNIX */
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else /* !N_PLAT_UNIX */
#include <npackon.h>
#endif /* N_PLAT_UNIX */

#ifdef N_PLAT_UNIX
#include <nw/nwclient.h>
#else /* !N_PLAT_UNIX */
#include <nwclient.h>
#endif /* N_PLAT_UNIX */

#ifdef __cplusplus
extern "C" {
#endif

#define C_SNAMESIZE 48
typedef struct
{
  NWCONN_HANDLE   connID;
  nuint16         connectFlags;
  nuint16         sessionID;
  NWCONN_NUM      connNumber;
  nuint8          serverAddr[12];
  nuint16         serverType;
  nstr8           serverName[C_SNAMESIZE];
  nuint16         clientType;
  nstr8           clientName[C_SNAMESIZE];
} CONNECT_INFO;

typedef struct
{
   nuint32  systemElapsedTime;
   nuint8   bytesRead[6];
   nuint8   bytesWritten[6];
   nuint32  totalRequestPackets;
} CONN_USE;

typedef struct tNWINET_ADDR
{
  nuint8   networkAddr[4];
  nuint8   netNodeAddr[6];
  nuint16  socket;
  nuint16  connType;  /* 3.11 and above only: 0=not in use, 2=NCP, 3=AFP */
} NWINET_ADDR;

#define CONNECTION_AVAILABLE            0x0001
#define CONNECTION_PRIVATE              0x0002  /* obsolete */
#define CONNECTION_LOGGED_IN            0x0004
#define CONNECTION_LICENSED             0x0004
#define CONNECTION_BROADCAST_AVAILABLE  0x0008
#define CONNECTION_ABORTED              0x0010
#define CONNECTION_REFUSE_GEN_BROADCAST 0x0020
#define CONNECTION_BROADCASTS_DISABLED  0x0040
#define CONNECTION_PRIMARY              0x0080
#define CONNECTION_NDS                  0x0100
#define CONNECTION_PNW                  0x4000
#define CONNECTION_AUTHENTICATED        0x8000  /* obsolete */

/* the following are for NWGetConnInfo */
/* ALL is VLM, OS2 and NT - NOT NETX */
#define NW_CONN_TYPE           1   /* returns nuint16  (VLM) */
#define NW_CONN_BIND      0x0031
#define NW_CONN_NDS       0x0032
#define NW_CONN_PNW       0x0033
#define NW_AUTHENTICATED       3  /* returns nuint16  = 1 if authenticated (ALL)*/
#define NW_PBURST              4  /* returns nuint16  = 1 if pburst (VLM) */
#define NW_VERSION             8  /* returns nuint16  (VLM)  */
#define NW_HARD_COUNT          9  /* returns WORD (VLM)  */
#define NW_CONN_NUM           13  /* returns nuint16  (ALL)  */
#define NW_TRAN_TYPE          15  /* returns nuint16  (VLM)  */
#define NW_TRAN_IPX       0x0021
#define NW_TRAN_TCP       0x0022
#define NW_SESSION_ID     0x8000  /* returns nuint16) (VLM) */
#define NW_SERVER_ADDRESS 0x8001  /* returns 12 byte address (ALL) */
#define NW_SERVER_NAME    0x8002  /* returns 48 byte string  (ALL) */

/* New connection model calls. */

NWCCODE N_API NWOpenConnByAddr
(
   pnstr          serviceType,
   nuint          connFlags,
   pNWCTranAddr   tranAddr,
   NWCONN_HANDLE N_FAR * conn
);

NWCCODE N_API NWOpenConnByName
(
   NWCONN_HANDLE  startConn,
   pNWCConnString name,
   pnstr          serviceType,
   nuint          connFlags,
   nuint          tranType,
   NWCONN_HANDLE N_FAR * conn
);

NWCCODE N_API NWOpenConnByReference
(
   nuint32        connReference,
   nuint          connFlags,
   NWCONN_HANDLE N_FAR * conn
);

NWCCODE N_API NWCloseConn
(
   NWCONN_HANDLE  conn
);

NWCCODE N_API NWSysCloseConn
(
   NWCONN_HANDLE  conn
);

NWCCODE N_API NWMakeConnPermanent
(
   NWCONN_HANDLE  conn
);

NWCCODE N_API NWLicenseConn
(
   NWCONN_HANDLE connHandle
);

NWCCODE N_API NWUnlicenseConn
(
   NWCONN_HANDLE  conn
);

NWCCODE N_API NWGetConnInformation
(
   NWCONN_HANDLE  conn,
   nuint          infoLevel,
	nuint				infoLen,
   nptr           connInfo
);

NWCCODE N_API NWScanConnInformation
(
   pnuint32       scanIndex,
   nuint          scanInfoLevel,
   nuint          scanInfoLen,
   nptr           scanConnInfo,
   nuint          scanFlags,
   nuint          returnInfoLevel,
   nuint          teturnInfoLen,
   pnuint32       connReference,
   nptr           returnConnInfo
);

NWCCODE N_API NWSetConnInformation
(
   NWCONN_HANDLE  conn,
   nuint          infoLevel,
   nuint          infoLength,
   nptr           connInfo
);

NWCCODE N_API NWGetPrimaryConnRef
(
   pnuint32       connRef
);

NWCCODE N_API NWSetPrimaryConn
(
   NWCONN_HANDLE  conn
);

/* End of new connection model calls. */

NWCCODE N_API NWGetConnInfo
(
   NWCONN_HANDLE  conn,
   nuint16        type,
   nptr           data
);

NWCCODE N_API NWLockConnection
(
   NWCONN_HANDLE conn
);

NWCCODE N_API NWGetConnectionUsageStats
(
   NWCONN_HANDLE  conn,
   NWCONN_NUM     connNumber,
   CONN_USE N_FAR * statusBuffer
);

NWCCODE N_API NWGetConnectionInformation
(
   NWCONN_HANDLE  conn,
   NWCONN_NUM     connNumber,
   pnstr8         objName,
   pnuint16       objType,
   pnuint32       objID,
   pnuint8        loginTime
);

NWCCODE N_API NWGetInternetAddress
(
   NWCONN_HANDLE conn,
   NWCONN_NUM    connNumber,
   pnuint8       inetAddr
);

NWCCODE N_API NWGetInetAddr
(
   NWCONN_HANDLE  conn,
   NWCONN_NUM     connNum,
   NWINET_ADDR N_FAR * inetAddr
);

void N_API NWGetMaximumConnections
(
   pnuint16    maxConns
);

NWCCODE N_API NWGetConnectionList
(
   nuint16        Mode,
   NWCONN_HANDLE N_FAR * connListBuffer,
   nuint16        connListSize,
   pnuint16       numConns
);

NWCCODE N_API NWGetConnectionStatus
(
   NWCONN_HANDLE        conn,
   CONNECT_INFO N_FAR *   connInfo,
   nuint16              connInfoSize
);

NWCCODE N_API NWGetConnectionNumber
(
   NWCONN_HANDLE     conn,
   NWCONN_NUM N_FAR *  connNumber
);

NWCCODE N_API NWClearConnectionNumber
(
   NWCONN_HANDLE  conn,
   NWCONN_NUM     connNumber
);

NWCCODE N_API NWGetDefaultConnRef
(
   pnuint32 connRef
);

NWCCODE N_API NWGetDefaultConnectionID
(
   NWCONN_HANDLE N_FAR * conn
);

#define NWGetConnectionID(a, b, c, d) NWGetConnectionHandle(a, b, c, d)

NWCCODE N_API NWGetConnectionHandle
(
   pnuint8        serverName,
   nuint16        reserved1,
   NWCONN_HANDLE N_FAR * conn,
   pnuint16       reserved2
);

NWCCODE N_API NWSetPrimaryConnectionID
(
   NWCONN_HANDLE conn
);

NWCCODE N_API NWGetPrimaryConnectionID
(
   NWCONN_HANDLE N_FAR * conn
);

NWCCODE N_API NWGetObjectConnectionNumbers
(
   NWCONN_HANDLE     conn,
   pnstr8            objName,
   nuint16           objType,
   pnuint16          numConns,
   NWCONN_NUM N_FAR *  connList,
   nuint16           maxConns
);

NWCCODE N_API NWGetConnListFromObject
(
   NWCONN_HANDLE  conn,
   nuint32        objID,
   nuint32        searchConnNum,
   pnuint16       connListLen,
   pnuint32       connList
);

#ifndef NWOS2
NWCCODE N_API NWIsIDInUse
(
   NWCONN_HANDLE conn
);

NWCCODE N_API NWGetPreferredServer
(
   NWCONN_HANDLE N_FAR * conn
);

NWCCODE N_API NWSetPreferredServer
(
   NWCONN_HANDLE conn
);

#else
NWCCODE N_API NWResetConnectionConfig
(
   nuint32  flags
);
#endif

#ifdef __cplusplus
}
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else /* !N_PLAT_UNIX */
#include <npackoff.h>
#endif /* N_PLAT_UNIX */

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwconnec.h,v 1.9 1994/09/26 17:11:57 rebekah Exp $
*/


