/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtfsver.c	1.5"
#include "nwcaldef.h"
#include "nwintern.h"
#include "nwundoc.h"
#include "nwserver.h"
#include "nwaccess.h"
#include "nwclient.h"

/*manpage*NWGetFileServerVersion********************************************
SYNTAX:  NWCCODE N_API NWGetFileServerVersion
         (
            NWCONN_HANDLE  conn,
            pnuint16       serverVer
         );

REMARKS:

ARGS:
      >  conn
      <  serverVer

INCLUDE: nwserver.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     NWGetFileServerInformation

NCP:     n/a

CHANGES: 21 Aug 1992 - written - jwoodbur
         13 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetFileServerVersion
(
   NWCONN_HANDLE  conn,
   pnuint16       serverVer
)
{
   NWCCODE ccode;
   nuint8 buMajorVer, buMinorVer;

#if defined(N_PLAT_UNIX) || defined(N_PLAT_DOS) || (defined N_PLAT_MS && defined N_ARCH_16 && !defined N_PLAT_WNT)
   nuint  uMajorVer, uMinorVer;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   ccode = (NWCCODE)NWCGetServerVersion(&access, &uMajorVer, &uMinorVer);
   buMajorVer = (nuint8)uMajorVer;
   buMinorVer = (nuint8)uMinorVer;

#else
#ifndef N_PLAT_DOS
   ccode = NWGetFileServerInformation(conn, NULL, &buMajorVer, &buMinorVer,
               NULL, NULL, NULL, NULL, NULL, NULL, NULL);
#endif
#endif
   if (ccode)
      return (ccode);

   *serverVer = buMinorVer * 10 + buMajorVer * 1000;

   return (0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtfsver.c,v 1.7 1994/09/26 17:47:00 rebekah Exp $
*/

