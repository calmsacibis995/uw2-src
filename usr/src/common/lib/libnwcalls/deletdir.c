/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:deletdir.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwmisc.h"
#include "nwnamspc.h"
#include "nwclocal.h"
#include "nwserver.h"

/*manpage*NWDeleteDirectory*************************************************
SYNTAX:  NWCCODE N_API NWDeleteDirectory
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         dirPath
         )

REMARKS:

ARGS:

INCLUDE: nwdirect.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 08  Delete A File or Subdirectory
         22 11  Delete Directory

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWDeleteDirectory
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         dirPath
)
{
   nuint16 serverVer;
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   if(serverVer >= 3110)
   {
      NWNCPCompPath cPath;
      nuint8 buNameSpace;

      buNameSpace = (nuint8) __NWGetCurNS(conn, dirHandle, dirPath);

      cPath.luDirBase = (nuint32) dirHandle;
      cPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
      NWNCPPackCompPath(-1, dirPath, buNameSpace, &cPath, (nuint32)0);

      return ((NWCCODE)NWNCP87s8DelEntry(&access, cPath.buNamSpc,
                 (nuint8) 0, FA_DIRECTORY, &cPath));
  }
  else
  {
      nstr8  tPath[260];
      nuint8 pathLen;

      if(dirPath)
      {
         NWCStrCpy(tPath, dirPath);
         NWConvertToSpecChar(tPath);

         pathLen = (nuint8) NWCStrLen(tPath);
      }
      else
      {
         pathLen = (nuint8) 0;
      }

      return ((NWCCODE) NWNCP22s11DirDel( &access,  dirHandle, (nuint8) 0,
                              pathLen, tPath) );
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/deletdir.c,v 1.7 1994/09/26 17:45:04 rebekah Exp $
*/
