/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gethndl.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwintern.h"
#include "nwundoc.h"
#include "nwdirect.h"

#ifdef N_PLAT_OS2
#include "nwnamspc.h"
#include "nwserver.h"
#endif

/*manpage*NWGetDirectoryHandlePath******************************************
SYNTAX:  NWCCODE N_API NWGetDirectoryHandlePath
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         dirPath
         )

REMARKS:

ARGS:

INCLUDE: nwdirect.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 21  Get Path String From Short Directory Handle
         22 01  Get Directory Path

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetDirectoryHandlePath
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         dirPath
)
{
   nuint8 dirPathLength, tPath[NW_MAX_OS2_NAME_LENGTH];
   NWCCODE ccode;
#ifdef N_PLAT_OS2
   nuint16 serverVer;
#endif
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

#ifdef N_PLAT_OS2

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   if(serverVer >= 3110)
       ccode = (NWCCODE) NWNCP87s21GetDirHandlePath( &access,
                  (nuint8) __NWGetCurNS( conn, dirHandle, dirPath ),
                           dirHandle, &dirPathLength, tPath);
   else
#endif
      ccode = (NWCCODE) NWNCP22s1GetPath( &access, dirHandle,
                              &dirPathLength, tPath) ;

   if(ccode)
   dirPath[0] = '\0';
   else
   {
      NWCMemMove(dirPath, tPath, dirPathLength);
      dirPath[dirPathLength] = '\0';
   }

   CleanPath(dirPath);

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gethndl.c,v 1.7 1994/09/26 17:46:06 rebekah Exp $
*/
