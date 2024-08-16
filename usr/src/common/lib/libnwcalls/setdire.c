/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:setdire.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwserver.h"
#include "nwdentry.h"
#include "nwerror.h"

/*manpage*NWSetDirEntryInfo*************************************************
SYNTAX:  NWCCODE N_API NWSetDirEntryInfo
         (
            NWCONN_HANDLE conn,
            NWDIR_HANDLE dirHandle,
            nuint8       srchAttr,
            nuint32      iterHandle,
            nuint32      changeBits,
            NWENTRY_INFO NWPTR newEntryInfo
         );

REMARKS: This function changes information about a directory entry (file or
         directory).

         To change a directory's information, the requesting workstation
         must have access control and modify rights. Only SUPERVISOR can
         change the owner of a directory.

         The specific directory entry is specified in the sequence parameter
         being passed in. The sequence number is obtained by making a call
         to NWScanDirEntryInfo.

         This API will rename entries for 3.0 and 3.1 servers, but not for
         3.11 servers. If you need to rename a file, regardless of the
         platform, use NWMoveDirEntry.

ARGS: >  conn
      >  dirHandle
      >  srchAttr
         Search attribute to use in searching for the directory entry.

         FA_NORMAL         0x00
         FA_READ_ONLY      0x01
         FA_HIDDEN         0x02
         FA_SYSTEM         0x04
         FA_EXECUTE_ONLY   0x08
         FA_DIRECTORY      0x10
         FA_NEEDS_ARCHIVED 0x20
         FA_SHAREABLE      0x80

      >  iterHandle
         An index number used to identify a specific directory entry
         relative to the dirHandle.

      >  changeBits
         A set of bits to indicate which attributes to change.

         MFileAttributesBit       0x0002L
         MCreateDateBit           0x0004L
         MCreateTimeBit           0x0008L
         MOwnerIDBit              0x0010L
         MLastArchivedDateBit     0x0020L
         MLastArchivedTimeBit     0x0040L
         MLastArchivedIDBit       0x0080L
         MLastUpdatedDateBit      0x0100L
         MLastUpdatedTimeBit      0x0200L
         MLastUpdatedIDBit        0x0400L
         MLastAccessedDateBit     0x0800L
         MInheritedRightsMaskBit  0x1000L
         MMaximumSpaceBit         0x2000L


      >  newEntryInfo
         Pointer to the information structure.

         typedef struct
         {
           nuint32 sequence;
           nuint32 parent;
           nuint32 attributes;
           nuint8  uniqueID;
           nuint8  flags;
           nuint8  nameSpace;
           nuint8  nameLength;
           nuint8  name [12];
           nuint32 creationDateAndTime;
           nuint32 ownerID;
           nuint32 lastArchiveDateAndTime;
           nuint32 lastArchiverID;

           union
           {
             NWFILE_INFO file;
             NWDIR_INFO  dir;
           }info;
         } NWENTRY_INFO;

         where:

         typedef struct
         {
           nuint32 updateDateAndTime;
           nuint32 updatorID;
           nuint32 fileSize;
           nuint8  reserved[44];
           nuint16  inheritedRightsMask;
           nuint16  lastAccessDate;
           nuint8  reserved2 [28];
         } NWFILE_INFO;

         typedef struct
         {
           nuint32 lastModifyDateAndTime;
           nuint32 nextTrusteeEntry;
           nuint8  reserved[48];
           nuint32 maximumSpace;
           nuint16  inheritedRightsMask;
           nuint8  reserved1 [26];
         } NWDIR_INFO;

INCLUDE: nwdentry.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 07  Modify File or SubDirectory DOS Information
         22 37  Set Directory Entry Information

CHANGES: 15 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWSetDirEntryInfo
(
   NWCONN_HANDLE conn,
   NWDIR_HANDLE dirHandle,
   nuint8       srchAttr,
   nuint32      iterHandle,
   nuint32      changeBits,
   NWENTRY_INFO NWPTR newEntryInfo
)
{
   NWCCODE ccode;
   nuint16 serverVer;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if(!newEntryInfo)
      return(0x88FF);

   if(!dirHandle)
      return BAD_DIRECTORY_HANDLE;

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   if(serverVer >= 3110)
   {
      NWNCPCompPath cPath;
      NWNCPModifyDosInfo dInfo;
      nstr8 abstrDosName[18];
      nint i;

      for(i = 0; i < (nint)newEntryInfo->nameLength; i++)
         abstrDosName[i] = newEntryInfo->name[i];
      abstrDosName[i] = (nstr8) 0;

      cPath.luDirBase = (nuint32) dirHandle;
      cPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
      NWNCPPackCompPath(-1, abstrDosName, -1, &cPath, (nflag32) 0);

      /************** translate structures  *****************/

      NWCMemSet(&dInfo, 0, (nuint) sizeof(dInfo));
      dInfo.suFileAttrs    = (nuint16)newEntryInfo->attributes;
      dInfo.suCreationDate =
         (nuint16) (newEntryInfo->creationDateAndTime>>16);
      dInfo.suCreationTime =
         (nuint16) (newEntryInfo->creationDateAndTime);
      dInfo.luCreatorID    = NSwap32(newEntryInfo->ownerID);
      dInfo.suArchivedDate =
         (nuint16) (newEntryInfo->lastArchiveDateAndTime>>16);
      dInfo.suArchivedTime =
         (nuint16) (newEntryInfo->lastArchiveDateAndTime);
      dInfo.luArchiverID   = NSwap32(newEntryInfo->lastArchiverID);
      dInfo.suModifiedDate =
         (nuint16) (newEntryInfo->info.dir.lastModifyDateAndTime>>16);
      dInfo.suModifiedTime =
         (nuint16) (newEntryInfo->info.dir.lastModifyDateAndTime);
      dInfo.suInheritanceRevokeMask = (nuint8) 0xFF;

      if(srchAttr & FA_DIRECTORY)
      {
         dInfo.luMaxSpace = newEntryInfo->info.dir.maximumSpace;
         dInfo.suInheritanceGrantMask =
            newEntryInfo->info.dir.inheritedRightsMask;
      }
      else
      {
         dInfo.luModifierID     = NSwap32(newEntryInfo->info.file.updatorID);
         dInfo.suLastAccessDate = newEntryInfo->info.file.lastAccessDate;
         dInfo.suInheritanceGrantMask =
            newEntryInfo->info.file.inheritedRightsMask;
      }

      return ((NWCCODE) NWNCP87s7EntrySetDOSInfo(&access, cPath.buNamSpc,
                  (nuint8) 0, (nuint8) srchAttr, changeBits, &dInfo,
                  &cPath));
   }
   else
   {
      NWENTRY_INFO entryInfo;

      /* we must do this because newEntryInfo is an input,
         and we can not modify inputs, even for swapping */

      NWCMemMove(&entryInfo, newEntryInfo, sizeof(NWENTRY_INFO));

      entryInfo.ownerID = NSwap32(entryInfo.ownerID);
      entryInfo.lastArchiverID = NSwap32(entryInfo.lastArchiverID);
      if (srchAttr & FA_DIRECTORY)
      {
         entryInfo.info.dir.volObjectID =
            NSwap32(entryInfo.info.dir.volObjectID);
      }
      else
      {
         entryInfo.info.file.updatorID =
            NSwap32(entryInfo.info.file.updatorID);
      }

      return ((NWCCODE) NWNCP22s37SetEntryInfo(&access, (nuint8) dirHandle,
                  srchAttr, iterHandle, changeBits,
                  (pNWNCPEntryUnion) &entryInfo.parent));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/setdire.c,v 1.7 1994/09/26 17:49:51 rebekah Exp $
*/
