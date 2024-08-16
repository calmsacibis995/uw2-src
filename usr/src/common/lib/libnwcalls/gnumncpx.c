/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gnumncpx.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpext.h"

#include "nwmisc.h"
#include "nwserver.h"
#include "nwncpext.h"

/*manpage*NWGetNumberNCPExtensions******************************************
SYNTAX:  NWCCODE N_API NWGetNumberNCPExtensions
         (
            NWCONN_HANDLE conn,
            pnuint32 pluNumNCPExts
         );

REMARKS: Scans currently loaded NCP extensions

ARGS:  < pluNumNCPExts
         The number of loaded NCP extensions.

INCLUDE: nwncpext.h

RETURN:  n/a

SERVER:  3.0 3.11 4.0

CLIENT:  DOS WIN OS2

SEE:

NCP:     36 00 Scan Currently Loaded NCP Extensions
         36 03 Get Number Of Loaded NCP Extensions (4.0)

CHANGES: 02 Dec 1992 - Written - jwoodbur
         21 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWGetNumberNCPExtensions
(
   NWCONN_HANDLE conn,
   pnuint32 pluNumNCPExts
)
{
   nuint16 suServVer;
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &suServVer)) != 0)
      return ccode;

   if(suServVer > 3110)
   {
      ccode = (NWCCODE) NWNCP36s3GetNumNCPExts(&access, pluNumNCPExts);
   }
   else
   {
      nuint32 luItems;
      nuint32 luNCPExtID = (nuint32) -1;

      for(luItems = 0;; luItems++)
      {
         if((ccode = NWScanNCPExtensions(conn, &luNCPExtID, NULL, NULL,
                                       NULL, NULL, NULL)) != 0)
         {
            if(ccode == 0x89ff)
            {
               *pluNumNCPExts = luItems;
               ccode = 0;
            }
            break;
         }
      }
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gnumncpx.c,v 1.7 1994/09/26 17:46:43 rebekah Exp $
*/
