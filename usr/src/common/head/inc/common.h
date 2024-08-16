/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/common.h	1.3"
/****************************************************************************
 *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written permission of
 * Novell, Inc.
 *
 ****************************************************************************/

#ifndef  _COMMON_HEADER_
#define  _COMMON_HEADER_

#include <nwdstype.h>
#include <nwdsbuft.h>
#include <tdrapi.h>
#include <ndsncp.h>
#include <unicode.h>
#include <nwconnec.h>

#include <npackon.h>

#define  AUTHENTICATE_FLAGS   0     /* authenticate without exchanging session key */
#define  MAX_PAD              3

/* The following macros are used in APIs that "put" data into the */
/* the buf_t buffer.                                              */

#define  ADJUST_CUR_LEN {buf->curLen =  buf->curPos - buf->data;}

#define  ALIGN_LENGTH(x)   ((x) + 3) & ~3

#define  CHECK_LENGTH(l) \
   {                     \
      if ((buf->maxLen-buf->curLen) < ((nuint32)(l))) \
            return ERR_BUFFER_FULL;        \
   }

#define  ALIGN_PTR(dataPtr, curPosPtr, bPtr) \
   {  \
         dataPtr = bPtr;               \
         TDRAlignStructure(&dataPtr);  \
         curPosPtr = dataPtr;          \
   }

typedef struct
{
   nuint8   N_FAR *curPos;
   nuint8   N_FAR *data;
}  Buf_Ptrs_T;

/* NWClient DS prototypes */

NWDSCCODE _CanonicalizeAndResolveNameFree
(
   NWDSContextHandle    context,
   pnstr8               objectName,
   nuint32              replicaType,
   nuint32              resolveFlags,
   NWCONN_HANDLE  N_FAR *conn,
   pnuint32             objectHandle
);

void _NWCReleaseConnection
(
   NWCONN_HANDLE  connectionID
);

NWDSCCODE isAuthenticated
(
   NWCONN_HANDLE connectionID
);

void _ReleaseConnection
(
   NWCONN_HANDLE  connectionID
);

NWDSCCODE CanonicalizeAndResolveName
(
   NWDSContextHandle    context,
   pnstr8               objectName,
   nuint32              replicaType,
   nuint32              resolveFlags,
   NWCONN_HANDLE  N_FAR *conn,
   pnuint32             objectHandle
);

NWDSCCODE CanonicalizeAndResolveNameFree
(
   NWDSContextHandle    context,
   pnstr8               objectName,
   nuint32              replicaType,
   nuint32              resolveFlags,
   NWCONN_HANDLE  N_FAR *conn,
   pnuint32             objectHandle
);

NWDSCCODE CanonAndResolveParentName
(
   NWDSContextHandle    context,
   pnstr8               objectName,
   punicode             RDN,
   nuint32              replicaType,
   nuint32              resolveFlags,
   NWCONN_HANDLE N_FAR  *conn,
   pnuint32             parentObjectHandle
);

punicode FindDelimiterInName
(
   punicode string,
   punicode delimStr
);

NWDSCCODE N_API GetDN
(
   NWDSContextHandle context,
   nptr              buf,
   pnstr8            dn
);

NWDSCCODE N_FAR GetDistinguishedName
(
   NWDSContextHandle context,
   nuint32           flags,
   pnstr8            limit,
   pBuf_T            buf,
   nptr              attrVal
);

#include <npackoff.h>

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/inc/common.h,v 1.3 1994/06/08 23:35:15 rebekah Exp $
*/
