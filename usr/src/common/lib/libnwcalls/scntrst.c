/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:scntrst.c	1.4"
#include "nwcaldef.h"
#include "nwdentry.h"

#undef NWScanForTrustees
   NWCCODE N_API NWScanForTrustees
   (
      NWCONN_HANDLE  conn,
      NWDIR_HANDLE   dirHandle,
      pnstr8         dirPath,
      pnuint32       iterHandle,
      pnuint16       numEntries,
      NWET_INFO NWPTR entryTrusteeInfo
   );

/*manpage*NWScanForTrustees*************************************************
SYNTAX:  NWCCODE N_API NWScanForTrustees
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         dirPath,
            pnuint32       iterHandle,
            pnuint16       numEntries,
            NWET_INFO NWPTR entryTrusteeInfo
         );

REMARKS:

ARGS: >  conn
      >  dirHandle
      >  dirPath
      <> iterHandle
      <  numEntries
      <  entryTrusteeInfo

INCLUDE: nwdentry.h

RETURN:

CLIENT:  DOS WIN OS2 NT

SEE:     NWIntScanForTrustees

CHANGES: 15 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWScanForTrustees
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         dirPath,
   pnuint32       iterHandle,
   pnuint16       numEntries,
   NWET_INFO NWPTR entryTrusteeInfo
)
{
   return (NWIntScanForTrustees(conn, dirHandle, dirPath, iterHandle,
            numEntries, entryTrusteeInfo, 0));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/scntrst.c,v 1.6 1994/06/08 23:13:11 rebekah Exp $
*/
