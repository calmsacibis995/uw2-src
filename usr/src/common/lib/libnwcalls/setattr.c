/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:setattr.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwmisc.h"
#include "nwnamspc.h"
#include "nwclocal.h"
#include "nwserver.h"
#include "nwfile.h"

/*manpage*NWSetFileAttributes***********************************************
SYNTAX:  NWCCODE N_API NWSetFileAttributes
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         fileName,
            nuint8         searchAttrs,
            nuint8         newAttr
         )

REMARKS: This is a NetWare 386 v3.11 call.  It is used for modifying DOS information
         while in another name space.

         The SearchAttributes field, ModifyDosMask field, and ModifyDosInfoStruct
         field are explained in more detail in the Introduction to Directory Services.

ARGS:    > conn
         > dirhandle

INCLUDE: nwfile.h

RETURN:  0x00  Successful
         0x8C  No Set Privileges
         0x8D  Some Files In Use
         0x8E  All Files In Use
         0x96  Server Out Of Memory
         0x98  Disk Map Error
         0x9B  Bad Directory Handle
         0x9C  Invalid Path
         0xA1  Directory I/O Error
         0xFD  Bad Station Number
         0xFF  Failure, No Files Found

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 07  Modify File or Subdirectory DOS Information
         70 --  Set File Attributes

CHANGES: 30 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWSetFileAttributes
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         fileName,
   nuint8         searchAttrs,
   nuint8         newAttrs
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
      nuint8 namSpc;
      NWNCPCompPath compPath;
      NWNCPModifyDosInfo dosInfo;

      namSpc  = (nuint8) __NWGetCurNS(conn, dirHandle, fileName);

      NWCMemSet(&dosInfo, 0, sizeof(dosInfo));
      dosInfo.suFileAttrs = newAttrs;

      compPath.luDirBase = (nuint32) dirHandle;
      compPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
      NWNCPPackCompPath( -1, fileName, namSpc, &compPath, (nflag32) 0);

      return ((NWCCODE) NWNCP87s7EntrySetDOSInfo(&access, namSpc, (nuint8) 0,
                           (nuint16) searchAttrs, (nuint32) 0x0002L, &dosInfo,
                           &compPath));
   }
   else
   {
      nuint8 tmpName[260];
      nuint8 fileNameLen;

      NWCStrCpy(tmpName, fileName);
      NWConvertToSpecChar(tmpName);
      fileNameLen = (nuint8)NWCStrLen(tmpName);

      return((NWCCODE) NWNCP70FileSetAttrs(&access, newAttrs, dirHandle,
                        searchAttrs, fileNameLen, (pnstr8)tmpName));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/setattr.c,v 1.7 1994/09/26 17:49:48 rebekah Exp $
*/

