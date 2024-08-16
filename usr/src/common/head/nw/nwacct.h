/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwacct.h	1.4"
/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NOACCT_INC
#define NOACCT_INC

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

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
   nuint32  objectID;
   nint32   amount;
} HOLDS_INFO;

typedef struct
{
   nuint16  holdsCount;
   HOLDS_INFO holds[16];
} HOLDS_STATUS;

NWCCODE N_API NWGetAccountStatus
(
   NWCONN_HANDLE  conn,
   nuint16        objType,
   pnstr8         objName,
   pnint32        balance,
   pnint32        limit,
   HOLDS_STATUS N_FAR * holds
);

NWCCODE N_API NWQueryAccountingInstalled
(
   NWCONN_HANDLE  conn,
   pnuint8        installed
);

NWCCODE N_API NWSubmitAccountCharge
(
   NWCONN_HANDLE  conn,
   nuint16        objType,
   pnstr8         objName,
   nuint16        serviceType,
   nint32         chargeAmt,
   nint32         holdCancelAmt,
   nuint16        noteType,
   pnstr8         note
);

NWCCODE N_API NWSubmitAccountHold
(
   NWCONN_HANDLE  conn,
   nuint16        objType,
   pnstr8         objName,
   nint32         holdAmt
);

NWCCODE N_API NWSubmitAccountNote
(
   NWCONN_HANDLE  conn,
   nuint16        objType,
   pnstr8         objName,
   nuint16        serviceType,
   nuint16        noteType,
   pnstr8         note
);

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
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwacct.h,v 1.6 1994/06/08 23:32:21 rebekah Exp $
*/
