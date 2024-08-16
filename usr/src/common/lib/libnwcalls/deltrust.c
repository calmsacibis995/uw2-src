/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:deltrust.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwmisc.h"
#include "nwserver.h"
#include "nwdentry.h"
#include "nwnamspc.h"
#include "nwclocal.h"

/*manpage*NWDeleteTrustee***************************************************
SYNTAX:  NWCCODE N_API NWDeleteTrustee
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         dirPath,
            nuint32        objID
         )

REMARKS: Removes a trustee from a directory, or a file's trustee list.

         This function revokes a trustee's rights in a specific for the
         specified path.  The requesting workstation must have access control
         (parental for 2.x) rights in the directory or in the parent
         directory, to delete a trustee.

         Deleting the explicit assignment of an object's trustee in a
         directory is not the same as assigning that object no rights in the
         directory.  If no rights are assigned in a directory, the object
         inherits the same rights it has in the parent directory.  Does this
         work the same on 3x?

         (For internal use only)

         The NetWare 3.11 structure used to handle the component path
         structures has a hard-coded buffer 300-bytes long for use in the
         packet xmit/rceive.  Hence this function has a hard-coded 300 byte
         component size used in it.

ARGS: >  conn
      >  dirHandle
      >  dirPath
      >  objID

INCLUDE: nwdentry.h

RETURN:  0x0000  Successful
         0x898C  No Set Privileges
         0x8990  Read-only Access To Volume
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x8999  Directory Full Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FC  No Such Object
         0x89FD  Bad Station Number
         0x89FE  Trustee Not Found
         0x89FF  Hard Failure, Failure

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 14  Delete Trustee From Directory
         22 43  Remove Extended Trustee From Dir Or File
         87 11  Delete Trustee Set From File or Subdirectory

CHANGES: 13 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
            Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWDeleteTrustee
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         dirPath,
   nuint32        objID
)
{
   NWCCODE ccode;
   nuint16 serverVer;
   nuint8 namSpc, pathLen;
   NWNCPCompPath compPath;
   NWNCPTrustees aTrustees[1];
   nstr8 tempPath[256];
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   /* NWCalls is broken; this needs to be swapped before the NCP */
   objID = NSwap32(objID);

   if(serverVer >= 3110)
   {
      namSpc = (nuint8) __NWGetCurNS(conn, dirHandle, dirPath);

      compPath.luDirBase = (nuint32) dirHandle;
      compPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
      NWNCPPackCompPath((nint) -1, dirPath, (nint) -1, &compPath,
                        (nflag32) 0);

      aTrustees[0].luObjID  = objID;
      aTrustees[0].suRights = (nuint16) 0;

      return ((NWCCODE) NWNCP87s11TrusteeDelSet(&access, namSpc,
                  (nuint8) 0, (nuint16) 1, &compPath, aTrustees));
   }
   else
   {
      if(dirPath)
      {
         NWCStrCpy(tempPath, dirPath);
         NWConvertToSpecChar(tempPath);
         pathLen = (nuint8) NWCStrLen(tempPath);
      }
      else
      {
         pathLen = (nuint8) 0;
      }

      if (serverVer == 3000)
      {
         return ((NWCCODE) NWNCP22s43TrusteeRemoveExt(&access, dirHandle,
                  objID, (nuint8) 0, pathLen, tempPath));
      }
      else
      {
         return((NWCCODE) NWNCP22s14TrusteeDelDir(&access, dirHandle,
                  objID, (nuint8) 0, pathLen, tempPath));
      }
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/deltrust.c,v 1.7 1994/09/26 17:45:13 rebekah Exp $
*/
