/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/ncpacct.h	1.3"
#if !defined( NCPACCT_H )
#define NCPACCT_H

#if !defined( NTYPES_H )
#ifdef N_PLAT_UNIX
#include <nw/ntypes.h>
#else /* !N_PLAT_UNIX */
#include <ntypes.h>
#endif /* N_PLAT_UNIX */
#endif

#if !defined( NWACCESS_H )
#ifdef N_PLAT_UNIX
#include <nw/nwaccess.h>
#else /* !N_PLAT_UNIX */
#include <nwaccess.h>
#endif /* N_PLAT_UNIX */
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else /* !N_PLAT_UNIX */
#include <npackon.h>
#endif /* N_PLAT_UNIX */

/***************************************************************************
   Constant declarations
****************************************************************************/

/***************************************************************************
   Structure declarations
****************************************************************************/

typedef struct tagNWNCPHoldsInfo
{
   nuint32 luHolderID;
   nint32  lAmount;
} NWNCPHoldsInfo, N_FAR *pNWNCPHoldsInfo;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   Function prototypes
****************************************************************************/

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s150GetAccountStatus
(
   pNWAccess   pAccess,
   nuint16    suClientType,
   nuint8     buClientNameLen,
   pnstr8     pbstrClientName,
   pnint32    plAccBalance,
   pnint32    plCreditLim,
   pnuint8    pbuReservedB120,
   pNWNCPHoldsInfo pHolderInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s151SubmitAccountCharge
(
   pNWAccess   pAccess,
   nuint16    suServiceType,
   nint32     lChargeAmt,
   nint32     lHoldCancelAmt,
   nuint16    suClientType,
   nuint16    suCommentType,
   nuint8     buClientNameLen,
   pnstr8     pbstrClientName,
   nuint8     buCommentLen,
   pnstr8     pbstrComment
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s152SubmitAccountHold
(
   pNWAccess   pAccess,
   nint32     lAmount,
   nuint16    suClientType,
   nuint8     buClientNameLen,
   pnstr8     pbstrClientName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s153SubmitAccountNote
(
   pNWAccess   pAccess,
   nuint16    suServiceType,
   nuint16    suClientType,
   nuint16    suCommentType,
   nuint8     buClientNameLen,
   pnstr8     pbstrClientName,
   nuint8     buCommentLen,
   pnstr8     pbstrComment
);

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else /* !N_PLAT_UNIX */
#include <npackoff.h>
#endif /* N_PLAT_UNIX */

#ifdef __cplusplus
}
#endif

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/ncpacct.h,v 1.5 1994/09/26 17:11:05 rebekah Exp $
*/
