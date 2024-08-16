/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:setfile2.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwclocal.h"
#include "nwserver.h"
#include "nwfile.h"

/*manpage*NWSetFileInformation2*********************************************
SYNTAX:  NWCCODE N_API NWSetFileInformation2
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         fileName,
            nuint8         searchAttrs,
            NW_FILE_INFO2 NWPTR info
         );

REMARKS: Sets a file's NetWare information.

ARGS: >  conn
      >  dirHandle
      >  fileName
      >  searchAttr
      <  info

         typedef struct NW_FILE_INFO2
         {
            nuint8  fileAttributes;
            nuint8  extendedFileAttributes;
            nuint32 fileSize;
            nuint16 creationDate;
            nuint16 lastAccessDate;
            nuint32 lastUpdateDateAndTime;
            nuint32 fileOwnerID;
            nuint32 lastArchiveDateAndTime;
            nstr8   fileName[260];
         } NW_FILE_INFO2;

INCLUDE: nwfile.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 07  Modify File or Subdirectory DOS Information
         23 16  Set File Information

CHANGES: 28 Jun 1993 - fixed - jwoodbur
            sets fileMode to 0 instead of random value. As a side effect
            any bits in fileMode will be cleared.
         1 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWSetFileInformation2
(
   NWCONN_HANDLE        conn,
   NWDIR_HANDLE         dirHandle,
   pnstr8               fileName,
   nuint8               searchAttrs,
   NW_FILE_INFO2 NWPTR  info
)
{
   nuint16 serverVer;
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode= NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   if(serverVer >= 3110)
   {
      NWNCPModifyDosInfo dosInfo;
      NWNCPCompPath cPath;

      dosInfo.suFileAttrs    = info->fileAttributes;
      dosInfo.buFileMode     = (nuint8) 0;
      dosInfo.buFileXAttrs   = info->extendedFileAttributes;
      dosInfo.suCreationDate = info->creationDate;
      dosInfo.suCreationTime = (nuint16) 0;
      dosInfo.luCreatorID    = NSwap32(info->fileOwnerID);
      dosInfo.suModifiedDate = NGetHi16(info->lastUpdateDateAndTime);
      dosInfo.suModifiedTime = NGetLo16(info->lastUpdateDateAndTime);
      dosInfo.luModifierID   = (nuint32) 0;
      dosInfo.suArchivedDate = NGetHi16(info->lastArchiveDateAndTime);
      dosInfo.suArchivedTime = NGetLo16(info->lastArchiveDateAndTime);
      dosInfo.luArchiverID   = (nuint32) 0;
      dosInfo.suLastAccessDate = info->lastAccessDate;
      dosInfo.suInheritanceGrantMask = (nuint16) 0;
      dosInfo.suInheritanceRevokeMask = (nuint16) 0;
      dosInfo.luMaxSpace     = (nuint32) 0L;

      cPath.luDirBase = (nuint32) dirHandle;
      cPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;

      NWNCPPackCompPath(-1, fileName, -1, &cPath, (nflag32) 0);

      ccode = (NWCCODE) NWNCP87s7EntrySetDOSInfo(&access, cPath.buNamSpc,
                  (nuint8) 0, searchAttrs, (nuint32) 0x0B76L, &dosInfo,
                  &cPath);
   }
   else
   {
      nstr8 tempName[260];
      NWNCPFileInfo2 info2;

      NWCStrCpy(tempName, fileName);
      NWConvertToSpecChar(tempName);

      info2.buAttrs           = info->fileAttributes;
      info2.buExtAttrs        = info->extendedFileAttributes;
      info2.luSize            = info->fileSize;
      info2.suCreationDate    = info->creationDate;
      info2.suAccessedDate    = info->lastAccessDate;
      info2.suModifiedDate    = NGetLo16(info->lastUpdateDateAndTime);
      info2.suModifiedTime    = NGetHi16(info->lastUpdateDateAndTime);
      info2.luOwnerID         = NSwap32(info->fileOwnerID);
      info2.suArchiveDate     = NGetLo16(info->lastArchiveDateAndTime);
      info2.suArchiveTime     = NGetHi16(info->lastArchiveDateAndTime);

      ccode = (NWCCODE) NWNCP23s16FileSetInfo(&access, &info2, NULL,
               dirHandle, searchAttrs, (nuint8) NWCStrLen(fileName),
               tempName);
   }
   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/setfile2.c,v 1.7 1994/09/26 17:49:55 rebekah Exp $
*/
