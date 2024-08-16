/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gncpxinf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpext.h"
#include "nwncpext.h"
#include "nwserver.h"

/*manpage*NWScanNCPExtensions***********************************************
SYNTAX:  NWCCODE N_API NWScanNCPExtensions
         (
            NWCONN_HANDLE conn,
            nuint32   luNCPExtID,
            pnstr8    pbstrExtNameB32,
            pnuint8   pbuMajorVer,
            pnuint8   pbuMinorVer,
            pnuint8   pbuRev,
            pnuint8   pbuQueryDataB32
         );

REMARKS: Scans currently loaded NCP extensions

ARGS:  > luNCPExtID
         The ID for which to get NCP Extension information.

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
         36 05 Get NCP Extension Info

CHANGES: 02 Dec 1992 - written - jwoodbur
         21 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWGetNCPExtensionInfo
(
  NWCONN_HANDLE conn,
  nuint32   luNCPExtID,
  pnstr8    pbstrExtNameB32,
  pnuint8   pbuMajorVer,
  pnuint8   pbuMinorVer,
  pnuint8   pbuRev,
  pnuint8   pbuQueryDataB32
)
{
   nuint8   buNameLen;
   nuint16  suServVer;
   NWCCODE  ccode;
   nuint32  luOrgExtID;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &suServVer)) != 0)
      return ccode;

   if(suServVer > 3110)
   {
      if ((ccode = (NWCCODE)NWNCP36s5GetNCPExtInfo(&access, &luNCPExtID,
         pbuMajorVer, pbuMinorVer, pbuRev, &buNameLen, pbstrExtNameB32,
         pbuQueryDataB32)) == 0)
      {
         if(pbstrExtNameB32)
            pbstrExtNameB32[buNameLen] = 0;
      }
   }
   else
   {
      luOrgExtID = luNCPExtID--;

      if ((ccode = (NWCCODE)NWNCP36s0ScanLoadedNCPExts(&access, &luNCPExtID,
         pbuMajorVer, pbuMinorVer, pbuRev, &buNameLen, pbstrExtNameB32,
         pbuQueryDataB32)) == 0)
      {
         if(pbstrExtNameB32)
            pbstrExtNameB32[buNameLen] = 0;

         ccode = NWGetNCPExtensionInfoByName(conn, pbstrExtNameB32,
                     &luNCPExtID, NULL, NULL, NULL, NULL);

         if(!ccode && (luOrgExtID != luNCPExtID))
            ccode = 0x89FE;
      }
   }
   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gncpxinf.c,v 1.7 1994/09/26 17:46:37 rebekah Exp $
*/
