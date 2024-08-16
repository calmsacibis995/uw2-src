/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/ncpext.h	1.3"
#if !defined( NCPEXT_H )
#define NCPEXT_H

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

/********************************************************************
   Structure Declaration
********************************************************************/

typedef struct tagNWNCPExtFrag
{
   nptr     pAddr;
   nuint    uLen;
} NWNCPExtFrag, N_FAR *pNWNCPExtFrag;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   Function prototypes
****************************************************************************/

N_EXTERN_LIBRARY( NWRCODE )
NWNCP36s0ScanLoadedNCPExts
(
   pNWAccess    pAccess,
   pnuint32    pluNCPExtID,
   pnuint8     pbuMajorVer,
   pnuint8     pbuMinorVer,
   pnuint8     pbuRev,
   pnuint8     pbuExtNameLen,
   pnstr8      pbstrExtNameB32,
   pnuint8     pbuQueryDataB32
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP36s2ScanLoadedExtsByName
(
   pNWAccess    pAccess,
   nuint8      buSrcNameLen,
   pnstr8      pbstrSrcExtNameB32,
   pnuint32    pluNCPExtID,
   pnuint8     pbuMajorVer,
   pnuint8     pbuMinorVer,
   pnuint8     pbuRev,
   pnuint8     pbuExtNameLen,
   pnstr8      pbstrExtNameB32,
   pnuint8     pbuQueryDataB32
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP36s3GetNumNCPExts
(
   pNWAccess    pAccess,
   pnuint32    pluNumNCPExts
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP36s4GetNCPExtsList
(
   pNWAccess    pAccess,
   pnuint32    pluStartNCPExtID,
   pnuint16    psuListItems,
   pnuint8     psuReservedB2,
   pnuint32    pluNCPExtIDListB128
);


N_EXTERN_LIBRARY( NWRCODE )
NWNCP36s5GetNCPExtInfo
(
   pNWAccess    pAccess,
   pnuint32    pluNCPExtID,
   pnuint8     pbuMajorVer,
   pnuint8     pbuMinorVer,
   pnuint8     pbuRev,
   pnuint8     pbuExtNameLen,
   pnstr8      pbstrExtNameB32,
   pnuint8     pbuQueryDataB32
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP37ExtNCPRequest
(
   pNWAccess    pAccess,
   nuint32     luNCPExtID,
   nptr        pReqData,
   nuint16     suReqDataLen,
   nptr        pReplyData,
   pnuint16    psuReplyDataLen
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP37FragExtNCPRequest
(
   pNWAccess       pAccess,
   nuint32        luNCPExtID,
   pNWNCPExtFrag  pReqDataFrag,
   nuint16        suReqDataLen,
   pNWNCPExtFrag  pReplyDataFrag,
   pnuint16       psuReplyDataLen
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
$Header: /SRCS/esmp/usr/src/nw/head/nw/ncpext.h,v 1.5 1994/09/26 17:11:16 rebekah Exp $
*/
