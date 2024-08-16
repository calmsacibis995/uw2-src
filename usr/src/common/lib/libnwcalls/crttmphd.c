/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:crttmphd.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwundoc.h"
#include "nwdirect.h"
#include "nwnamspc.h"
#include "nwclocal.h"
#include "nwserver.h"

/*manpage*NWAllocTemporaryDirectoryHandle***********************************
SYNTAX:  NWCCODE N_API NWAllocTemporaryDirectoryHandle
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         dirPath,
            NWDIR_HANDLE NWPTR newDirHandle,
            pnuint8        rightsMask
         )

REMARKS:

ARGS: >  conn
      >  dirHandle
      >  dirPath
      <  newDirHandle
      <  rightsMask (optional)

INCLUDE: nwdirect.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x8999  Directory Full Error
         0x899C  Invalid Path
         0x899D  No Directory Handles
         0x89A1  Directory I/O Error
         0x89FD  Bad Station Number
         0x89FF  Failure

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 12  Allocate Short Directory Handle
         22 19  Alloc Temporary Directory Handle
         22 03  Get Effective Directory Rights

CHANGES: 10 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWAllocTemporaryDirectoryHandle
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         dirPath,
   NWDIR_HANDLE   NWPTR newDirHandle,
   pnuint8        rightsMask
)
{
   nuint16 serverVer;
   NWCCODE ccode;
   nuint8  pathLen, bucket[5], handleName;
   NWNCPCompPath compPath;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   if(serverVer >= 3110)
   {
      compPath.luDirBase = dirHandle;
      compPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
      NWNCPPackCompPath((nint) -1, dirPath, (nint) -1, &compPath,
                        (nflag32) 0);

      if((ccode = (NWCCODE) NWNCP87s12AllocDirHandle(&access,
                     compPath.buNamSpc, (nuint8) 0, (nuint16) 1, &compPath,
                     newDirHandle, &bucket[0], &bucket[1])) != 0)
         return (ccode);

      /*...get effective rights for the new dir handle...*/
      if(rightsMask)
      {
         ccode = (NWCCODE) NWNCP22s3GetDirEffRights(&access, *newDirHandle,
                     (nuint8) 0, dirPath, rightsMask);
      }
   }
   else
   {
      nstr8 aTempPath[260];

      if(dirPath)
      {
         NWCStrCpy(aTempPath, dirPath);
         NWConvertToSpecChar(aTempPath);
         pathLen = (nuint8) NWCStrLen(aTempPath);
      }
      else
      {
         NWCStrCpy(aTempPath, "");
         pathLen = (nuint8) 0;
      }

      handleName = (nuint8) 63;  /* The Drive Letter (used to be '?') */

      ccode = (NWCCODE) NWNCP22s19AllocTempDirHandle(&access, dirHandle,
                  handleName, pathLen, aTempPath, newDirHandle,
                  rightsMask);
   }
   if(ccode)
      *newDirHandle = (NWDIR_HANDLE) 0;

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/crttmphd.c,v 1.7 1994/09/26 17:45:02 rebekah Exp $
*/
