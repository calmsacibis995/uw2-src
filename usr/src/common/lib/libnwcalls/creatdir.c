/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:creatdir.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwmisc.h"
#include "nwclocal.h"
#include "nwserver.h"

/*manpage*NWCreateDirectory*************************************************
SYNTAX:  NWCCODE N_API NWCreateDirectory
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         dirPath,
            nuint8         accessMask
         )

REMARKS:

ARGS: <> conn
      >  dirHandle
      >  dirPath
      >  accessMask

INCLUDE: nwdirect.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 01  Open Create File or Subdirectory
         22 10  Create Directory

CHANGES: 10 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWCreateDirectory
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         dirPath,
   nuint8         accessMask
)
{
   NWCCODE 			ccode;
   nuint16 			serverVer;
   NWNCPCompPath 	compPath;
   nstr8	 		tmpPath[260];
   nuint8			pathLen;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   if(serverVer >= 3110)
   {
      compPath.luDirBase = dirHandle;
      compPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;

      NWNCPPackCompPath(-1, dirPath, -1, &compPath, (nflag32) 0);

      return ((NWCCODE) NWNCP87s1EntryOpenCreate(&access, NWNCP_NS_DOS,
               (nuint8) 8, (nuint16) 0, (nuint32) 0, (nuint32) FA_DIRECTORY,
               (nuint16) accessMask, &compPath, NULL, NULL, NULL, NULL));
   }
   else
   {
      if (dirPath != NULL)
      {
         NWCStrCpy(tmpPath, dirPath);
         NWConvertToSpecChar((pnptr)tmpPath);
         pathLen = (nuint8)NWCStrLen(tmpPath);
      }
      else
      {
         pathLen= 0;
      }

      return ((NWCCODE)NWNCP22s10DirCreate (&access, dirHandle, accessMask,
                          pathLen, tmpPath));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/creatdir.c,v 1.7 1994/09/26 17:44:51 rebekah Exp $
*/
