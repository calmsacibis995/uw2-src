/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwncpext.h	1.4"
/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NCPEXT_INC
#define NCPEXT_INC

#ifndef NWCALDEF_INC
#ifdef N_PLAT_UNIX
# include <nw/nwcaldef.h>
#else /* !N_PLAT_UNIX */
# include <nwcaldef.h>
#endif /* N_PLAT_UNIX */
#endif

#ifndef NWMISC_INC
#ifdef N_PLAT_UNIX
# include <nw/nwmisc.h>
#else /* !N_PLAT_UNIX */
# include <nwmisc.h>
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

#define NW_NCPX_BEGIN_SCAN 0xFFFFFFFF

NWCCODE N_API NWGetNCPExtensionInfo
(
   NWCONN_HANDLE  conn,
   nuint32        NCPExtensionID,
   pnstr8         NCPExtensionName,
   pnuint8        majorVersion,
   pnuint8        minorVersion,
   pnuint8        revision,
   pnuint8        queryData
);

NWCCODE N_API NWNCPExtensionRequest
(
   NWCONN_HANDLE  conn,
   nuint32        NCPExtensionID,
   nptr           requestData,
   nuint16        requestDataLen,
   nptr           replyData,
   pnuint16       replyDataLen
);

NWCCODE N_API NWFragNCPExtensionRequest
(
   NWCONN_HANDLE  conn,
   nuint32        NCPExtensionID,
   nuint16        reqFragCount,
   NW_FRAGMENT N_FAR * reqFragList,
   nuint16        replyFragCount,
   NW_FRAGMENT N_FAR * replyFragList
);

NWCCODE N_API NWScanNCPExtensions
(
   NWCONN_HANDLE  conn,
   pnuint32       NCPExtensionID,
   pnstr8         NCPExtensionName,
   pnuint8        majorVersion,
   pnuint8        minorVersion,
   pnuint8        revision,
   pnuint8        queryData
);

NWCCODE N_API NWGetNCPExtensionInfoByName
(
   NWCONN_HANDLE  conn,
   pnstr8         NCPExtensionName,
   pnuint32       NCPExtensionID,
   pnuint8        majorVersion,
   pnuint8        minorVersion,
   pnuint8        revision,
   pnuint8        queryData
);

NWCCODE N_API NWGetNCPExtensionsList
(
   NWCONN_HANDLE  conn,
   pnuint32       startNCPExtensionID,
   pnuint16       itemsInList,
   pnuint32       NCPExtensionIDList
);

NWCCODE N_API NWGetNumberNCPExtensions
(
   NWCONN_HANDLE  conn,
   pnuint32       numNCPExtensions
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
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwncpext.h,v 1.6 1994/06/08 23:33:18 rebekah Exp $
*/
