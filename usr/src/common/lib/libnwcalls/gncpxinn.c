/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gncpxinn.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpext.h"
#include "nwncpext.h"
#include "nwserver.h"

/*manpage*NWGetNCPExtensionInfoByName***************************************
SYNTAX:  NWCCODE N_API NWGetNCPExtensionInfoByName
         (
            NWCONN_HANDLE conn,
            pnstr8    pbstrExtNameB32,
            pnuint32  pluNCPExtID,
            pnuint8   pbuMajorVer,
            pnuint8   pbuMinorVer,
            pnuint8   pbuRev,
            pnuint8   pbuQueryDataB32
         );

REMARKS: Scans currently loaded NCP extensions

ARGS: >  pbstrExtNameB32
         The name to of the NCP Extension handler. (33 bytes)

      <  pluNCPExtID (optional)
         The ID of the NCP extension handler.

      <  pbuMajorVer (optional)
         the major version number of the service provider

      <  pbuMinorVer (optional)
         the minor version number of the service provider

      <  pbuRev (optional)
         the pbuRev number of the service provider

      <  pbuQueryDataB32 (optional)
         the 32 bytes of custom information that the service provider can
         optionally use

INCLUDE: nwncpext.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     36 02 Scan Currently Loaded NCP Extensions By Name (3.11)
         36 00 Scan Currently Loaded NCP Extensions

CHANGES: 02 Dec 1992 - Written - jwoodbur
         21 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWGetNCPExtensionInfoByName
(
   NWCONN_HANDLE conn,
   pnstr8    pbstrExtNameB32,
   pnuint32  pluNCPExtID,
   pnuint8   pbuMajorVer,
   pnuint8   pbuMinorVer,
   pnuint8   pbuRev,
   pnuint8   pbuQueryDataB32
)
{
   NWCCODE  ccode;
   nuint16  suServVer;
   nuint8   buNameLen;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   buNameLen = (nuint8)NWCStrLen(pbstrExtNameB32);

   if((ccode = NWGetFileServerVersion(conn, &suServVer)) != 0)
      return ccode;

   if(buNameLen > 32)
      return(0x8836);

   if(suServVer > 3110)
   {
      ccode = (NWCCODE) NWNCP36s2ScanLoadedExtsByName(&access, buNameLen,
            pbstrExtNameB32, pluNCPExtID, pbuMajorVer, pbuMinorVer,
            pbuRev, NULL, NULL, pbuQueryDataB32);
   }
   else if(suServVer >= 3000)
   {
      nstr8    abstrTName[32];
      nuint8   buTNameLen;
      *pluNCPExtID = (nuint32) -1;

      do
      {
      ccode = (NWCCODE) NWNCP36s0ScanLoadedNCPExts(&access, pluNCPExtID,
                  pbuMajorVer,pbuMinorVer, pbuRev, &buTNameLen, abstrTName,
                  pbuQueryDataB32);

      } while(!ccode &&
                  NWCMemCmp(abstrTName, pbstrExtNameB32, buTNameLen));
   }
   else if(suServVer < 3000)
      return(0x89FD);

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gncpxinn.c,v 1.7 1994/09/26 17:46:38 rebekah Exp $
*/
