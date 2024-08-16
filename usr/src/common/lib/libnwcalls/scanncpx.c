/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:scanncpx.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpext.h"
#include "nwncpext.h"

/*manpage*NWScanNCPExtensions***********************************************
SYNTAX:  NWCCODE N_API NWScanNCPExtensions
         (
            NWCONN_HANDLE conn,
            pnuint32  pluNCPExtID,
            pnstr8    pbstrExtNameB32,
            pnuint8   pbuMajorVer,
            pnuint8   pbuMinorVer,
            pnuint8   pbuRev,
            pnuint8   pbuQueryDataB32
         );

REMARKS: Scans currently loaded NCP extensions

ARGS: <> pluNCPExtID
         The ID that will be used to scan NCP Extension handlers. Set
         to -1 for first iteration.

       < pbstrExtNameB32
         The name to of the NCP Extension handler. (33 bytes, optional)

       < pbuMajorVer
         the major version number of the service provider (optional)

       < pbuMinorVer
         the minor version number of the service provider (optional)

       < pbuRev
         the pbuRev number of the service provider (optional)

       < pbuQueryDataB32
         the 32 bytes of custom information that the service provider can
         optionally use (optional)

INCLUDE: nwncpext.h

RETURN:  n/a

SERVER:  3.0 3.11 4.0

CLIENT:  DOS WIN OS2

SEE:

NCP:     36 00 Scan Currently Loaded NCP Extensions

CHANGES: 31 Aug 1992 - Converted from clib - jwoodbur
         02 Dec 1992 - Modified to work - jwoodbur
         21 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWScanNCPExtensions
(
  NWCONN_HANDLE   conn,
  pnuint32        pluNCPExtID,
  pnstr8          pbstrExtNameB32,
  pnuint8         pbuMajorVer,
  pnuint8         pbuMinorVer,
  pnuint8         pbuRev,
  pnuint8         pbuQueryDataB32
)
{
   NWCCODE ccode;
   nuint8 buExtNameLen;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = (NWCCODE)NWNCP36s0ScanLoadedNCPExts(&access, pluNCPExtID, pbuMajorVer,
         pbuMinorVer, pbuRev, &buExtNameLen, pbstrExtNameB32,
         pbuQueryDataB32))==0)
   {
      if(pbstrExtNameB32)
         pbstrExtNameB32[buExtNameLen] = 0;
   }
   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/scanncpx.c,v 1.7 1994/09/26 17:49:21 rebekah Exp $
*/
