/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:asextfil.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwncp.h"
#include "nwcaldef.h"
#include "nwdirect.h"
#include "nwdentry.h"
#include "nwundoc.h"


/*manpage*NWIntScanExtendedInfo*********************************************
SYNTAX:  NWCCODE N_API NWIntScanExtendedInfo
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            nuint8         attrs,
            pnuint32       iterHandle,
            pnstr8         srchPattern,
            NW_EXT_FILE_INFO NWPTR entryInfo,
            nuint16        augmentFlag
         )

REMARKS: Scans a directory for the extended file (or dir) info.

         The extended file information contains the information returned by
         NWScanInfo plus the sizes of the data and resource forks.
         Additionally, this call will return the physical size of a file.
         In the case of sparse files, the logical size may be much larger
         than the physical size.

         This call is synonimous with NWScanDirEntryInfo and uses an
         extension of the information structure.

ARGS: >  conn
      >  dirHandle
      >  attrs
      <> iterHandle
         the search sequence number - (-1L) to initialize

      >  srchPattern
         the pattern to search for (no wild cards)

      <  entryInfo
         the extended file (or dir) info

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
            nuint32 updateDateAndTime;
            nuint32 lastUpdatorID;
            nuint32 dataForkSize;         /-* file size *-/
            nuint32 dataForkFirstFAT;
            nuint32 nextTrusteeEntry;
            nuint8  reserved[36];
            nuint16  inheritedRightsMask;
            nuint16  lastAccessDate;
            nuint32 deletedFileTime;
            nuint32 deletedDateAndTime;
            nuint32 deletorID;
            nuint8  reserved2 [16];
            nuint32 otherForkSize[2];
         }  NW_EXT_FILE_INFO;

      >  augmentFlag
         If this flag equals USE_NW_WILD_MATCH all wild cards are augmented.

INCLUDE: nwdentry.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 40  Scan Directory Disk Space

CHANGES: 9 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All Rights Reserved.
****************************************************************************/
NWCCODE N_API NWIntScanExtendedInfo
(
   NWCONN_HANDLE     conn,
   NWDIR_HANDLE      dirHandle,
   nuint8            attrs,
   pnuint32          iterHandle,
   pnstr8            srchPattern,
   NW_EXT_FILE_INFO  NWPTR entryInfo,
   nuint16           augmentFlag
)
{
   NWCCODE ccode;
   nuint8 newPattern[256];
   NWNCPEntryUnion NCPEntry;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   __NWFillWildPath(newPattern, srchPattern, augmentFlag);

   if ((ccode = (NWCCODE)NWNCP22s40ScanDirDiskSpace(&access, (nuint8) dirHandle, attrs,
         newPattern[0], &newPattern[1], iterHandle,
         &NCPEntry, entryInfo->otherForkSize)) == 0)
   {
      entryInfo->sequence = *iterHandle;
      entryInfo->parent = NCPEntry.luSubdir;
      entryInfo->attributes = NCPEntry.luAttrs;
      entryInfo->uniqueID = NCPEntry.buUniqueID;
      entryInfo->flags = NCPEntry.buFlags;
      entryInfo->nameSpace = NCPEntry.buNamSpc;
      entryInfo->nameLength = NCPEntry.buNameLen;
      NWCMemMove(entryInfo->name, NCPEntry.info.entry.abuName, (nuint) 12);
      entryInfo->creationDateAndTime = NCPEntry.info.entry.luCreationDateAndTime;
      entryInfo->ownerID = NSwap32(NCPEntry.info.entry.luOwnerID);
      entryInfo->lastArchiveDateAndTime = NCPEntry.info.entry.luArchivedDateAndTime;
      entryInfo->lastArchiverID = NSwap32(NCPEntry.info.entry.luArchiverID);
      entryInfo->updateDateAndTime = NCPEntry.info.entry.luModifiedDateAndTime;
      entryInfo->lastUpdatorID = NSwap32(NCPEntry.info.entry.luModifierID);
      entryInfo->dataForkSize = NCPEntry.info.entry.luDataForkSize;
      entryInfo->dataForkFirstFAT = NCPEntry.info.entry.luDataForkFirstFAT;
      entryInfo->nextTrusteeEntry = NCPEntry.info.entry.luNextTrusteeEntry;
      NWCMemMove(entryInfo->reserved, NCPEntry.info.entry.abuReserved1, (nuint) 36);
      entryInfo->inheritedRightsMask = NCPEntry.info.entry.suInheritedRightsMask;
      entryInfo->lastAccessDate = NCPEntry.info.entry.suLastAccessedDate;
      entryInfo->deletedFileTime = NCPEntry.info.entry.luDeletedFileTime;
      entryInfo->deletedDateAndTime = NCPEntry.info.entry.luDeletedDateAndTime;
      entryInfo->deletorID = NSwap32(NCPEntry.info.entry.luDeletorID);
      NWCMemMove(entryInfo->reserved2, NCPEntry.info.entry.abuReserved2, (nuint) 8);
      NCopyLoHi32(&entryInfo->reserved2[8], &NCPEntry.info.entry.luPrimaryEntry);
      NCopyLoHi32(&entryInfo->reserved2[12], &NCPEntry.info.entry.luNameList);
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/asextfil.c,v 1.7 1994/09/26 17:44:11 rebekah Exp $
*/
