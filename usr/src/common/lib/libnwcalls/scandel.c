/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:scandel.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwserver.h"
#include "nwnamspc.h"
#include "nwdel.h"

/*manpage*NWScanForDeletedFiles*********************************************
SYNTAX:  NWCCODE N_API NWScanForDeletedFiles
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnuint32       iterHandle,
            pnuint32       volNum,
            pnuint32       dirBase,
            NWDELETED_INFO NWPTR entryInfo
         );

REMARKS:

ARGS:  > conn
       > dirHandle
      <> iterHandle
      <  volNum (optional)
      <  dirBase (optional)
      <  entryInfo

INCLUDE: nwdel.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 27  Scan Salvageable Files
         87 16  Scan Salvageable Files

CHANGES: 17 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWScanForDeletedFiles
(
  NWCONN_HANDLE  conn,
  NWDIR_HANDLE   dirHandle,
  pnuint32       iterHandle,
  pnuint32       volNum,
  pnuint32       dirBase,
  NWDELETED_INFO NWPTR entryInfo
)
{
   NWCCODE ccode;
   nuint16 serverVer;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if ((entryInfo == NULL) || (iterHandle == NULL) || (dirHandle == 0))
      return ((NWCCODE) -1);

   NWCMemSet(entryInfo, 0, sizeof(*entryInfo));

   if ((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   if (serverVer >= 3110)
   {
      nuint16 suDelTime, suDelDate;
      nuint32 luRetMask, luDelID;
      NWNCPCompPath cPath;
      NWNCPEntryStruct eInfo;

      luRetMask = IM_ENTRY_NAME | IM_RIGHTS | IM_SIZE     |
                  IM_ARCHIVE    | IM_MODIFY | IM_CREATION |
                  IM_ATTRIBUTES | IM_OWNING_NAMESPACE;

      cPath.luDirBase = (nuint32) dirHandle;
      cPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
      NWNCPPackCompPath(-1, NULL, -1, &cPath, (nflag32) 0);

      if ((ccode = (NWCCODE) NWNCP87s16DelScan(&access, cPath.buNamSpc,
            (nuint8) 0, luRetMask, &cPath, iterHandle, &suDelTime,
            &suDelDate, &luDelID, volNum, dirBase, &eInfo)) == 0)
      {
         entryInfo->parent     = (nuint32) 0;
         entryInfo->attributes = eInfo.luAttrs;
         entryInfo->uniqueID   = (nuint32) 0;
         entryInfo->flags      = (nuint8) eInfo.suFlags;
         entryInfo->nameSpace  = (nuint8) eInfo.luNamSpcCreator;
         entryInfo->nameLength = eInfo.buNameLen;
         NWCStrCpy(entryInfo->name, eInfo.abuName);

         entryInfo->creationDateAndTime   =
            NMake32(eInfo.suCreationTime, eInfo.suCreationDate);
         entryInfo->ownerID               = NSwap32(eInfo.luCreatorID);
         entryInfo->lastArchiveDateAndTime=
            NMake32(eInfo.suArchivedTime, eInfo.suArchivedDate);
         entryInfo->lastArchiverID        = NSwap32(eInfo.luArchiverID);
         entryInfo->updateDateAndTime     =
            NMake32(eInfo.suModifiedTime, eInfo.suModifiedDate);
         entryInfo->updatorID             = NSwap32(eInfo.luModifierID);
         entryInfo->fileSize              = eInfo.luDataStreamSize;
         entryInfo->inheritedRightsMask   = eInfo.suInheritedRightsMask;
         entryInfo->lastAccessDate        = eInfo.suAccessedDate;
         entryInfo->deletedTime           = (nuint32) 0;
         entryInfo->deletedDateAndTime    = NMake32(suDelTime, suDelDate);
         entryInfo->deletorID             = NSwap32(luDelID);
      }
   }
   else
   {
      if ((ccode = (NWCCODE) NWNCP22s27DelScan(&access, (nuint8) dirHandle,
            iterHandle, (pNWNCPDelEntryInfo) &entryInfo->parent)) == 0)
      {
         entryInfo->ownerID        = NSwap32(entryInfo->ownerID);
         entryInfo->lastArchiverID = NSwap32(entryInfo->lastArchiverID);
         entryInfo->updatorID      = NSwap32(entryInfo->updatorID);
         entryInfo->deletorID      = NSwap32(entryInfo->deletorID);

         entryInfo->name[entryInfo->nameLength] = (nstr8) 0x00;
      }

      if (volNum)
         *volNum = (nuint32) -1;

      if (dirBase)
         *dirBase = (nuint32) -1;
   }

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/scandel.c,v 1.7 1994/09/26 17:49:20 rebekah Exp $
*/
