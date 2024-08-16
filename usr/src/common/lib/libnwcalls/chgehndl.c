/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:chgehndl.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwclocal.h"
#include "nwserver.h"

/*manpage*NWSetDirectoryHandlePath******************************************
SYNTAX:  NWCCODE N_API NWSetDirectoryHandlePath
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   sourceDirHandle,
            pnstr8         dirPath,
            NWDIR_HANDLE   destDirHandle
         );

REMARKS: Sets the current directory for the specified directory handle.

         If this call fails, the dstDirHandle parameter remains unchanged.

         In cases where multiple file servers are being used, the
         srcDirHandle and dstDirHandle parameter must have the same
         server connection identifier.

         This function assigns the target directory handle to a directory
         path defined by the combined source directory handle and the source
         directory path.

         The srcDirHandle, an index number from 1 to 255, points to a
         volume or a directory on the file server.  A file server maintains a
         Directory Handle table for logged in workstation.

         The dstDirHandle is another index number from the file server's
         Directory Handle table.

         The dirPath parameter can identify a full or partial directory path.
         A full directory path defines a volume or a directory on a given
         file server in the format VOLUME:DIRECTORY/.../DIRECTORY.  A partial
         directory path specifies at least a directory, and possibly one or
         more parent directories.

         Applications frequently combine a directory handle and a directory
         path to specify a target directory.  For example, if the specified
         directory handle points to SYS: and the the specified directory path
         = PUBLIC/WORDP, then the specified directory is SYS:PUBLIC/WORDP.

         When an application defines a target directory using only a
         directory handle, the application must pass a null string in
         dirPath.  When an application defines a directory using only a
         directory path, the application must set srcDirHandle to zero.

ARGS: >  conn
      >  sourceDirHandle
      >  dirPath
      <  destDirHandle

INCLUDE: nwdirect.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 00  Set Directory Handle
         87 09  Set Short Directory Handle

CHANGES: 13 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWSetDirectoryHandlePath
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   sourceDirHandle,
   pnstr8         dirPath,
   NWDIR_HANDLE   destDirHandle
)
{
   NWCCODE ccode;
   nuint16 serverVer;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   if(serverVer >= 3110)
   {
      NWNCPCompPath cPath;

      cPath.luDirBase = (nuint32) sourceDirHandle;
      cPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
      NWNCPPackCompPath(-1, dirPath, -1, &cPath, (nflag32) 0);

      ccode = (NWCCODE) NWNCP87s9SetDirHandle(&access, cPath.buNamSpc,
                  (nuint8) 0, (nuint8) destDirHandle, (nuint8) 0, &cPath);
   }
   else
   {
      nstr8  abstrTempPath[260];
      nuint8 buBucket;

      if(dirPath)
      {
         NWCStrCpy(abstrTempPath, dirPath);
         NWConvertToSpecChar(abstrTempPath);
      }

      ccode = (NWCCODE) NWNCP22s0SetDirHandle(&access,
                  destDirHandle, sourceDirHandle,
                  (nuint8) NWCStrLen(dirPath), dirPath,
                  &buBucket, NULL, NULL);
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/chgehndl.c,v 1.7 1994/09/26 17:44:16 rebekah Exp $
*/
