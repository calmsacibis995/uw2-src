/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:setdir.c	1.6"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwdirect.h"
#include "nwclocal.h"
#include "nwserver.h"

/*manpage*NWSetDirectoryInformation*****************************************
SYNTAX:  NWCCODE N_API NWSetDirectoryInformation
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         path,
            nuint32        dirDateTime,
            nuint32        ownerID,
            nuint8         rightsMask
         );

REMARKS: Changes information about a directory.

         In addition to changing information about a directory, this
         function also specifies a creation date and time, owner object ID,
         and maximum rights mask.  The function defines the target directory
         by passing a directory handle and a directory path.

ARGS: >  conn
      >  dirHandle
      >  path
      >  dirDateTime
         The new creation date and time

      >  ownerID
         The bindery object ID of the directory's new owner

      >  rightsMask
         The new maximum rights mask for the directory

         0x01 - Read
         0x02 - Write
         0x04 - Open
         0x08 - Create
         0x10 - Delete
         0x20 - Parental
         0x40 - Search
         0x80 - Modify

INCLUDE: nwdirect.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 07  Modify File or Subdirectory DOS Information
         22 25  Set Directory Information

CHANGES: 15 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWSetDirectoryInformation
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   nuint32        dirDateTime,
   nuint32        ownerID,
   nuint8         rightsMask
)
{
   nuint16 serverVer;
   NWCCODE ccode;
   NWNCPCompPath cPath;
   NWNCPModifyDosInfo info;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   /* NWCalls is broken; this swap must occur before NCP call(s) */
   ownerID = NSwap32(ownerID);

   if(serverVer >= 3110)
   {
      info.suFileAttrs    = (nuint16) 0;
      info.buFileMode     = (nuint8) 0;
      info.buFileXAttrs   = (nuint8) 0;

      info.suCreationDate = NGetHi16(dirDateTime);
      info.suCreationTime = NGetLo16(dirDateTime);
      info.luCreatorID    = ownerID;

      info.suModifiedDate = (nuint16) 0;
      info.suModifiedTime = (nuint16) 0;
      info.luModifierID   = (nuint32) 0;

      info.suArchivedDate = (nuint16) 0;
      info.suArchivedTime = (nuint16) 0;
      info.luArchiverID   = (nuint32) 0;

      info.suLastAccessDate = (nuint16) 0;
      info.suInheritanceGrantMask  = (nuint16) rightsMask;
      info.suInheritanceRevokeMask = (nuint16) 0xFFFF;
      info.luMaxSpace     = (nuint32) 0;

      cPath.luDirBase = (nuint32) dirHandle;
      cPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
      NWNCPPackCompPath(-1, path, -1, &cPath, (nflag32) 0);

      ccode = (NWCCODE) NWNCP87s7EntrySetDOSInfo(&access, cPath.buNamSpc,
                  (nuint8) 0, FA_DIRECTORY, (nuint32) 0x101C, &info,
                  &cPath);
   }
   else
   {
      nstr8 abstrTempPath[256];

      NWCStrCpy(abstrTempPath, path);
      NWConvertToSpecChar(abstrTempPath);

      ccode = (NWCCODE) NWNCP22s25SetDirInfo(&access, (nuint8) dirHandle,
                  NGetHi16(dirDateTime), NGetLo16(dirDateTime),
                  ownerID, rightsMask, (nuint8) NWCStrLen(abstrTempPath),
                  abstrTempPath);
   }
   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/setdir.c,v 1.8 1994/09/26 17:49:50 rebekah Exp $
*/
