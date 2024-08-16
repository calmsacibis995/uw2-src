/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwndscon.h	1.4"
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/
/****************************************************************************

 BEGIN_MANUAL_ENTRY ( nwndscon.h() )

 SOURCE MODULE: nwndscon.h

 API NAME     :

 SYNTAX       :

 DESCRIPTION  :

 PARAMETERS   :   -> input          <-output

 INCLUDE      :

 RETURN       :

 MODIFICATIONS:

     January 20, 1992 - Art Nevarez, rolled-in from the OS/2 team.

 NCP CALLS
 ---------

 API CALLS
 ---------

 SEE ALSO:

 @Q1 HANDLES STRINGS? (Y\N):

 @Q2 HANDLES PATH? (Y\N):

 END_MANUAL_ENTRY
****************************************************************************/
#ifndef NWNDSCON_INC
#define NWNDSCON_INC

#define NWNDS_CONNECTION         0x0001
#define NWNDS_LICENSED           0x0002
#define NWNDS_AUTHENTICATED      0x0004
#define NWNDS_PACKET_BURST_AVAIL 0x0001
#define NWNDS_NEEDED_MAX_IO      0x0040
#define SYSTEM_LOCK              0x0
#define TASK_LOCK                0x4
#define SYSTEM_DISCONNECT        0x0
#define TASK_DISCONNECT          0x1

#define ALLREADY_ATTACHED        0x1
#define ATTACHED_NOT_AUTH        0X2
#define ATTACHED_AND_AUTH        0X4


#ifdef __cplusplus
   extern "C" {
#endif

/* NWClient DS prototypes */

NWCCODE N_API NWCGetNearestDirectoryService
(
   NWCONN_HANDLE N_FAR *conn
);

NWCCODE N_API NWCGetNearestDSConnRef
(
   pnuint32    connRef
);

NWCCODE N_API NWCGetPreferredDSServer
(
   NWCONN_HANDLE N_FAR *conn
);

NWCCODE N_API NWCDSGetConnectionInfo
(
   NWCONN_HANDLE  conn,
   nuint8   N_FAR *connStatus,
   nuint8   N_FAR *connType,
   nuint8   N_FAR *serverFlags,
   nuint8   N_FAR *serverName,
   nuint8   N_FAR *transType,
   nuint32  N_FAR *transLen,
   nuint8   N_FAR *transBuf,
   nuint16  N_FAR *distance,
   nuint16  N_FAR *maxPacketSize
);

NWCCODE N_API NWCDSGetConnectionSlot
(
   nuint8         connType,
   nuint8         transType,
   nuint32        transLen,
   nuint8 N_FAR   *transBuf,
   NWCONN_HANDLE N_FAR *conn
);

NWCCODE N_API NWCDSLockConnection
(
   NWCONN_HANDLE conn
);

NWCCODE N_API NWGetNearestDirectoryService
(
   NWCONN_HANDLE N_FAR *conn
);

NWCCODE N_API NWSetDefaultNameContext
(
   nuint16        contextLength,
   nuint8 N_FAR   *context
);

NWCCODE N_API NWGetDefaultNameContext
(
   nuint16        bufferSize,
   nuint8 N_FAR   *context
);

NWCCODE N_API NWGetConnectionIDFromAddress
(
   nuint8               transType,
   nuint32              transLen,
   nuint8        N_FAR  *transBuf,
   NWCONN_HANDLE N_FAR  *conn
);

NWCCODE N_API NWDSGetConnectionInfo
(
   NWCONN_HANDLE  conn,
   nuint8   N_FAR *connStatus,
   nuint8   N_FAR *connType,
   nuint8   N_FAR *serverFlags,
   nuint8   N_FAR *serverName,
   nuint8   N_FAR *transType,
   nuint32  N_FAR *transLen,
   nuint8   N_FAR *transBuf,
   nuint16  N_FAR *distance,
   nuint16  N_FAR *maxPacketSize
);

NWCCODE N_API NWDSGetConnectionSlot
(
   nuint8         connType,
   nuint8         transType,
   nuint32        transLen,
   nuint8 N_FAR   *transBuf,
   NWCONN_HANDLE N_FAR *conn
);

NWCCODE N_API NWGetPreferredDSServer
(
   NWCONN_HANDLE N_FAR *conn
);

NWCCODE N_API NWSetPreferredDSTree
(
   nuint16        length,
   nuint8 N_FAR   *treeName
);

NWCCODE N_API NWGetNumConnections
(
   nuint16 N_FAR  *numConnections
);

NWCCODE N_API NWDSGetMonitoredConnection
(
   NWCONN_HANDLE N_FAR *conn
);

NWCCODE N_API NWDSSetMonitoredConnection
(
   NWCONN_HANDLE conn
);

NWCCODE N_API NWGetConnectionIDFromName
(
   nuint32              nameLen,
   nuint8         N_FAR *name,
   NWCONN_HANDLE  N_FAR *conn
);

NWCCODE N_API NWIsDSAuthenticated
(
   void
);

NWCCODE N_API NWDSLockConnection
(
   NWCONN_HANDLE conn
);

NWCCODE N_API NWDSUnlockConnection
(
   NWCONN_HANDLE conn
);

NWCCODE N_API NWGetPreferredConnName
(
   nuint8 N_FAR *preferredName,
   nuint8 N_FAR *preferredType
);

NWCCODE N_API NWFreeConnectionSlot
(
   NWCONN_HANDLE  conn,
   nuint8         disconnectType
);

NWCCODE N_API NWGetNextConnectionID
(
   NWCONN_HANDLE N_FAR *conn
);

#ifdef __cplusplus
   }
#endif

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwndscon.h,v 1.5 1994/09/26 17:12:24 rebekah Exp $
*/
