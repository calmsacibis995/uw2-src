/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/wrap.h	1.3"
#ifndef  _WRAP_HEADER_
#define  _WRAP_HEADER_

#include <npackon.h>

#define ITER_HANDLE_SIGNATURE 0x7344774eL /* sDwN */

typedef  struct _continuation
{
      struct   _continuation N_FAR  *next;
      nuint8    N_FAR  *nameList;
      nuint32 N_FAR  *count;
} Continuation;

#define MAX_SERVER_ADDRESS 32

typedef  struct
{
   nuint32  signature;
   nuint32  realHandle;
   nuint32  objectID;
   nuint32  netAddrType;
   nuint32  netAddrLen;
   nuint8   netAddr[MAX_SERVER_ADDRESS];
   nint     useBaseName;
   nstr8    N_FAR  *currentName;
   Continuation N_FAR  *list;
} IterHandle;

/* NWClient DS prototypes */

NWDSCCODE NWCCheckLastConn
(
   NWDSContextHandle context,
   NWCONN_HANDLE  N_FAR *connectionID
);

NWDSCCODE _NWConnectToServer
(
   NWDSContextHandle    context,
   pnstr8               serverName,
   NWCONN_HANDLE N_FAR  *conn,
   nstr8                authFlag
);

NWDSCCODE CreateWrappedHandle
(
   nuint32        realHandle,
   NWCONN_HANDLE  conn,
   nuint32        objectID,
   IterHandle N_FAR * N_FAR *wrappedHandle
);

NWDSCCODE GetConnHandle
(
   NWDSContextHandle    context,
   NWCONN_HANDLE N_FAR  *conn,
   pnint32              iterationHandle,
   IterHandle N_FAR * N_FAR *wrappedHandle
);

NWDSCCODE CheckLastConn
(
   NWDSContextHandle context,
   NWCONN_HANDLE  N_FAR *connectionID
);

NWDSCCODE GetObjectAndConnHandle
(
   NWDSContextHandle    context,
   pnstr8               objectName,
   nuint32              replicaType,
   nuint32              resolveFlags,
   NWCONN_HANDLE N_FAR  *conn,
   pnuint32             objectHandle,
   pnint32              iterationHandle,
   IterHandle N_FAR * N_FAR *wrappedHandle
);

NWDSCCODE ReconnectServer
(
   nuint32              netAddrType,
   nuint32              netAddrLen,
   pnuint8              netAddr,
   NWCONN_HANDLE N_FAR  *conn
);

#include <npackoff.h>

#endif   /* _WRAP_HEADER_ */

/*
$Header: /SRCS/esmp/usr/src/nw/head/inc/wrap.h,v 1.3 1994/06/08 23:35:56 rebekah Exp $
*/
