/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gefright.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwundoc.h"
#include "nwserver.h"
#include "nwnamspc.h"
#include "nwclocal.h"
#include "nwdentry.h"

/*manpage*NWGetEffectiveRights**********************************************
SYNTAX:  NWCCODE N_API NWGetEffectiveRights
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         path,
            pnuint16       effectiveRights
         )

REMARKS:

ARGS: >  conn
      >  dirHandle
      >  path
      <  effectiveRights

INCLUDE: nwdentry.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FD  Bad Station Number
         0x89FF  Failure

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 03  Get Effective Directory Rights
         22 42  Get Effective Rights for Directory Entry
         87 29  Get Effective Directory Rights

CHANGES: 20 May 1993 - modified - jwoodbur
            added support for PNW. Also changed scope of several
            variables to reduce hit on stack.
         10 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetEffectiveRights
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   pnuint16       effectiveRights
)
{
   NWCCODE ccode;
   nuint16 serverVer;
   nuint8  namSpc, pathLen;
   NWNCPCompPath compPath;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   if(serverVer >= 3110)
   {
      NWNCPEntryStruct unused;

      namSpc = (nuint8) __NWGetCurNS(conn, dirHandle, path);

      compPath.luDirBase = dirHandle;
      compPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
      NWNCPPackCompPath((nint) -1, path, (nint) -1, &compPath,
                        (nflag32) 0);

      ccode = (NWCCODE) NWNCP87s29GetDirEffRights(&access, namSpc,
                  namSpc, (nuint16) 0x8000, (nuint32) 0x0800, &compPath,
                  effectiveRights, &unused);
   }
   else
   {
      nstr8 aTempPath[256];

      if(path)
      {
         NWCStrCpy(aTempPath, path);
         NWConvertToSpecChar(aTempPath);
         pathLen = (nuint8)NWCStrLen(aTempPath);
      }
      else
      {
         NWCStrCpy(aTempPath, "");
         pathLen = (nuint8) 0;
      }

      if(serverVer >= 3000 || serverVer < 2000)
      {
         ccode = (NWCCODE) NWNCP22s42GetEffRights(&access, dirHandle,
                  pathLen, aTempPath, effectiveRights);
      }
      else
      {
         nuint8 buTempRights;

         ccode = (NWCCODE) NWNCP22s3GetDirEffRights(&access, dirHandle,
                  pathLen, aTempPath, &buTempRights);
         *effectiveRights = (nuint16) buTempRights;
      }
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gefright.c,v 1.7 1994/09/26 17:45:39 rebekah Exp $
*/
