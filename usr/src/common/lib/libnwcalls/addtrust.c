/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:addtrust.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwundoc.h"
#include "nwserver.h"
#include "nwdentry.h"
#include "nwnamspc.h"
#include "nwclocal.h"

/*manpage*NWAddTrustee******************************************************
SYNTAX:  NWCCODE N_API NWAddTrustee
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         path,
            nuint32        objID,
            nuint16        rightsMask
         )

REMARKS: Adds a trustee to the list of trustees in a file or a directory.

         If the object is already a trustee for the specified file or
         directory, the trustee's current access rigths are replaced by the
         value contained in the rightsMask parameter.  Otherwise, the object
         is added as a trustee to the directory with the given access rights
         in the rightsMask parameter.

         Only a workstation with access control rights (parental rights) to
         either the target directory, or its parent directory can modify the
         trustee rights for a file or directory.

         NOTE: on 2.x servers, file level trustee assigments are NOT
         supported.  This API does NOT, however, check whether the specified
         path is a file, or a directory path.  The input parameters are
         passed "as is" to the server, and the responding error from the
         server is returned.

         (For internal use only)

         The NetWare 3.11 structure used to handle the component path
         structures has a hard-coded buffer 300-bytes long for use in
         the packet xmit/rceive.  Hence this function has a hard-coded
         300 byte component size used in it.

ARGS: >  conn
      >  dirHandle
      >  path
      >  objID
      >  rightsMask
         The access rights mask the new trustee is being granted

         The rights mask is defined as follows:

            2x                     3x                        new

         TA_NONE      0x00                            TR_NONE         0x0000
         TA_READ      0x01    TR_READ        0x0001   TR_READ         0x0001
         TA_WRITE     0x02    TR_WRITE       0x0002   TR_WRITE        0x0002
         TA_OPEN      0x04      (implied)             TR_OPEN         0x0004
         TA_CREATE    0x08    TR_CREATE      0x0008   TR_CREATE       0x0008
         TA_DELETE    0x10    TR_ERASE       0x0010   TR_ERASE        0x0010
         TA_OWNERSHIP 0x20    TR_ACCESS      0x0020   TR_ACCESS_CTRL  0x0020
         TA_SEARCH    0x40    TR_FILE        0x0040   TR_FILE_ACCESS  0x0040
         TA_MODIFY    0x80    TR_MODIFY      0x0080   TR_MODIFY       0x0080
         TA_ALL       0xFF    TR_ALL         0x01FB   TR_ALL          0x01FF
                              TR_SUPERVISOR  0x0100   TR_SUPERVISOR   0x0100
                              TR_NORMAL      0x00FB   TR_NORMAL       0x00FF


         In the 3.x world the OPEN bit (0x04) is undefined, and whenever READ
         or WRITE are specified, the OPEN is implied. For the 2.x world, the
         high order byte values will be ignored, in other words, the
         supervisor, and normal values will have no meaning if passed to 2.x
         servers. For 3.x servers, the open bit will be masked off by the
         API.


INCLUDE: nwdentry.h

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

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 10  Add Trustee Set To Directory File or Subdirectory
         22 39  Add Extended Trustee To Directory or File
         22 13  Add Trustee To Directory

CHANGES: 7 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWAddTrustee
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   nuint32        objID,
   nuint16        rightsMask
)
{
   NWCCODE ccode;
   nuint16 serverVer;
   nuint8 namSpc;
   nuint8 pathLen;
   NWNCPCompPath compPath;
   NWNCPTrustees aTrustees[1];
   nstr8 tmpPath[256];

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
      (void)NWNCPPackCompPath((nint) -1, path, (nint) -1, &compPath,
                        (nflag32) 0);

      aTrustees[0].luObjID = objID;
      aTrustees[0].suRights = rightsMask;

      return ((NWCCODE) NWNCP87s10TrusteeAddSet(&access, namSpc, (nuint8) 0,
               (nuint16) 0x8000, (nuint16) 0xFFFF, (nuint16) 1, &compPath,
               aTrustees));
   }
   else if(serverVer >= 3000 || serverVer < 2000)
   {
      if(path != NULL)
      {
         NWCStrCpy(tmpPath, path);
         NWConvertToSpecChar((char N_FAR *) tmpPath);
         pathLen = (nuint8) NWCStrLen(tmpPath);
      }
      else
      {
         NWCStrCpy(tmpPath, "");
         pathLen = (nuint8) 0;
      }
      return ((NWCCODE) NWNCP22s39TrusteeAddExt(&access, dirHandle,
                  objID, rightsMask, pathLen, tmpPath));
   }
   else
   {
      if(path != NULL)
      {
         NWCStrCpy(tmpPath, path);
         NWConvertToSpecChar((char N_FAR *) tmpPath);
         pathLen = (nuint8) NWCStrLen(tmpPath);

      }
      else
      {
         NWCStrCpy(tmpPath, "");
         pathLen = (nuint8) 0;
      }
      return ((NWCCODE) NWNCP22s13TrusteeAddToDir(&access, dirHandle,
                  objID, (nuint8) rightsMask, pathLen, tmpPath));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/addtrust.c,v 1.7 1994/09/26 17:43:48 rebekah Exp $
*/
