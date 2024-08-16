/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:ascnentr.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwncp.h"
#include "nwdentry.h"
#include "nwundoc.h"
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwlocale.h"
#include "nwclocal.h"

/*manpage*NWIntScanDirEntryInfo*********************************************
SYNTAX:  NWCCODE N_API NWIntScanDirEntryInfo
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            nuint16        attrs,
            pnuint32       iterHandle,
            pnuint8        srchPattern,
            NWENTRY_INFO NWPTR entryInfo,
            nuint16        augmentFlag
         )

REMARKS: Gets information about 3.x directory entries (files or directories).

         This call can be used iteratively with wildcards. On the first call,
         iterHandle should be set to -1.  After that, the server will manage the
         information. All scanning is done when 0x89FF is returned by the
         server.

         This function can also be used to scan for information about a
         directory (including the root). In this mode the dirHandle needs to
         be pointing at the directory and srchPattern needs to be NULL.

         This function is a direct call into the DOS name space, and thus
         won't take other name space names. if using long names, the
         NWScanNSEntryInfo function will return similar information, and will
         also return information about extended attributes.

         In order to use this call with long names, the long name must be
         converted to a a DOS name using the NWGetNSName call.

         NOTE: The srchPattern cannot contain any path elements, the
               directory handle must point to the correct path. Wild cards
               are OK and will be augmented if augmentFlag = USE_NW_WILD_MATCH


ARGS:  > conn
       > dirHandle
         The NetWare directory handle pointing to the directory to scan.

       > attrs
         Attributes to be used for the scan.

           FA_NORMAL         0x00
           FA_READ_ONLY      0x01
           FA_HIDDEN         0x02
           FA_SYSTEM         0x04
           FA_EXECUTE_ONLY   0x08
           FA_DIRECTORY      0x10
           FA_NEEDS_ARCHIVED 0x20
           FA_SHAREABLE      0x80


      <> iterHandle
         Pointer a nuint32 buffer that will receive the
         search iterHandle from the server. (set to 0 to initialize)

       > srchPattern
         Pointer to the name of the entry to scan for. (Wildcards
         are allowed) (optional)

      <  entryInfo
         Pointer to the NWENTRY_INFO structure.

          typedef struct
          {
            nuint32  sequence;
            nuint32  parent;
            nuint32  attributes;
            nuint8   uniqueID;
            nuint8   flags;
            nuint8   nameSpace;
            nuint8   nameLength;
            nuint8   name [12];
            nuint32  creationDateAndTime;
            nuint32  ownerID;
            nuint32  lastArchiveDateAndTime;
            nuint32  lastArchiverID;

            union
            {
              NWFILE_INFO file;
              NWDIR_INFO  dir;
            }info;
         } NWENTRY_INFO;

         where:

         typedef struct
         {
            nuint32  updateDateAndTime;
            nuint32  updatorID;
            nuint32  fileSize;
            nuint8   reserved[44];
            nuint16  inheritedRightsMask;
            nuint16  lastAccessDate;
            nuint8   reserved2[28];
         } NWFILE_INFO;

         typedef struct
         {
            nuint32  lastModifyDateAndTime;
            nuint32  nextTrusteeEntry;
            nuint8   reserved[48];
            nuint32  maximumSpace;
            nuint16  inheritedRightsMask;
            nuint8   reserved2[14];
            nuint32  volObjectID;
            nuint8   reserved3[8];
         } NWDIR_INFO;


       > augmentFlag
         if this flag equals USE_NW_WILD_MATCH all wild cards are augmented.

INCLUDE: nwdentry.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 31  Get Directory Entry
         22 30  Scan a Directory

CHANGES: 10 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWIntScanDirEntryInfo
(
  NWCONN_HANDLE   conn,
  NWDIR_HANDLE    dirHandle,
  nuint16         attrs,
  pnuint32        iterHandle,
  pnuint8         srchPattern,
  NWENTRY_INFO    NWPTR entryInfo,
  nuint16         augmentFlag
)
{
   NWCCODE ccode;
   nuint8 tmpSrchPattern[256];
   nuint8 tmpWildPath[258];
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if(srchPattern == NULL)     /* check if they're checking the root */
   {
      *iterHandle = (nuint32) 0L;

      ccode = (NWCCODE)NWNCP22s31GetEntryInfo(&access, dirHandle,
                  (pNWNCPEntryUnion)&entryInfo->parent);
   }
   else
   {
    NWCStrCpy(tmpSrchPattern, srchPattern);
    /* NWConvertAndAugment(tmpSrchPattern, augmentFlag); */

    __NWFillWildPath(tmpWildPath, (pnstr8)tmpSrchPattern, augmentFlag);

      ccode = (NWCCODE)NWNCP22s30Scan(&access, dirHandle, (nuint8) attrs,
            tmpWildPath[0], &tmpWildPath[1],
            iterHandle, (pNWNCPEntryUnion)&entryInfo->parent);
   }

   if(ccode == 0)
   {
      entryInfo->ownerID = NSwap32(entryInfo->ownerID);
      entryInfo->lastArchiverID = NSwap32(entryInfo->lastArchiverID);
      entryInfo->name[entryInfo->nameLength] = 0;

      if(entryInfo->attributes & (nuint8) FA_DIRECTORY)
         entryInfo->info.dir.volObjectID = NSwap32(entryInfo->info.dir.volObjectID);
      else
         entryInfo->info.file.updatorID = NSwap32(entryInfo->info.file.updatorID);

      entryInfo->sequence = *iterHandle;
      if(srchPattern == NULL)
      {
         NWCCODE flag;
         pnstr8 pbstrTPath;
         nuint8 bTPath[256];

         flag = NWGetDirectoryHandlePath(conn, dirHandle, (pnstr8)bTPath);
         if(!flag)
         {
            pbstrTPath = (pnstr8)&bTPath[NWCStrLen(bTPath)];
            pbstrTPath = NWPrevChar((pnstr8)bTPath, pbstrTPath);

            if(*pbstrTPath == ':')
               flag = 1;
         }

         if (flag)
         {
            entryInfo->nameLength = (nuint8) 0;
         }
      }
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/ascnentr.c,v 1.7 1994/09/26 17:44:05 rebekah Exp $
*/
