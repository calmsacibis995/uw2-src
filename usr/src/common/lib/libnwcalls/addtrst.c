/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:addtrst.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwundoc.h"
#include "nwdirect.h"
#include "nwnamspc.h"
#include "nwserver.h"
#include "nwclocal.h"

/*manpage*NWAddTrusteeToDirectory*******************************************
SYNTAX:  NWCCODE N_API NWAddTrusteeToDirectory
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         path,
            nuint32        objID,
            nuint8         rightsMask
         )

REMARKS: Adds a trustee to the list of trustees in a directory.

         If the object is already a trustee for the specified directory,
         the trustee's current access mask is replaced by the value
         contained in the rightsMask parameter.  Otherwise, the object is
         added as a trustee to the directory and given a rights mask equal
         to the rightsMask parameter.

         To modify a trustee rights list, the requesting workstation must
         have parental rights to the directory, or to a parent of the
         directory.

ARGS: >  conn
      >  dirHandle
      >  path
      >  objID
         The bindery object ID for the object being added as a trustee

      >  rightsMask
         The access rights mask the new trustee is being granted

INCLUDE: nwdirect.h

RETURN:  0x0000  Successful
         0x898C  No Set Privileges
         0x8990  Volume Is Read Only
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x8999  Directory Full Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FC  No Such Object
         0x89FD  Bad Station Number
         0x89FF  Hard Failure, Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 10  Add Trustee Set To Directory File or Subdirectory
         22 39  Add Extended Trustee To Directory or File
         22 13  Add Trustee To Directory

CHANGES: 8 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All Rights Reserved.
****************************************************************************/
NWCCODE N_API NWAddTrusteeToDirectory
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   nuint32        objID,
   nuint8         rightsMask
)
{
   NWCCODE ccode;
   nuint16 serverVer;
   nuint8  namSpc;
   nuint8  pathLen;
   NWNCPCompPath compPath;
   NWNCPTrustees aTrustees[1];
   nstr8 tempPath[256];

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   /* NWCalls is broken; this needs to happen before the NCP call */
   objID = NSwap32(objID);

   if(serverVer >= 3110)
   {
      namSpc = (nuint8) __NWGetCurNS(conn, dirHandle, path);

      compPath.luDirBase = (nuint32) dirHandle;
      compPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
      NWNCPPackCompPath((nint) -1, path, (nint) -1, &compPath,
                        (nflag32) 0);

      aTrustees[0].luObjID = objID;
      aTrustees[0].suRights = rightsMask;

      return ((NWCCODE) NWNCP87s10TrusteeAddSet(&access, namSpc, (nuint8) 0,
               (nuint16) 0x0010, (nuint16) 0xFFFF, (nuint16) 1, &compPath,
               aTrustees));
   }
   else if(serverVer >= 3000 || serverVer < 2000)
   {
      if(path != NULL)
      {
         NWCStrCpy(tempPath, path);
         NWConvertToSpecChar((char N_FAR *) tempPath);
         pathLen = (nuint8) NWCStrLen(tempPath);
      }
      else
      {
         NWCStrCpy(tempPath, "");
         pathLen = (nuint8) 0;
      }
      return ((NWCCODE) NWNCP22s39TrusteeAddExt(&access, dirHandle,
                  objID, rightsMask, pathLen, tempPath));
   }
   else
   {

      if(path != NULL)
      {
         NWCStrCpy(tempPath, path);
         NWConvertToSpecChar((char N_FAR *) tempPath);
         pathLen = (nuint8) NWCStrLen(tempPath);
      }
      else
      {
         NWCStrCpy(tempPath, "");
         pathLen = (nuint8) 0;
      }
      return ((NWCCODE) NWNCP22s13TrusteeAddToDir(&access, dirHandle,
                  objID, rightsMask, pathLen, tempPath));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/addtrst.c,v 1.7 1994/09/26 17:43:46 rebekah Exp $
*/
