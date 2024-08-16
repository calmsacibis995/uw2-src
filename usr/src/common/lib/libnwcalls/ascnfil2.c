/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:ascnfil2.c	1.6"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwfile.h"
#include "nwmisc.h"
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwlocale.h"
#include "nwclocal.h"
#include "nwserver.h"

/*manpage*NWIntScanFileInformation2*****************************************
SYNTAX:  NWCCODE N_API NWIntScanFileInformation2
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         fileName,
            nuint8         searchAttr,
            pnuint8        iterHandle,
            NW_FILE_INFO2 NWPTR info,
            nuint16        augmentFlag
         )
REMARKS: Scans the specified directory for the specified file (or directory),
         and return the associated directory entry information.

         This function is a duplicate of NWIntScanFileInformation and it was
         written to handle long names as well as function as the old function
         did.  The new structure used in this function is:

         typedef struct NW_FILE_INFO2
         {
            nuint8 fileAttributes;
            nuint8 extendedFileAttributes;
            nuint32 fileSize;
            nuint16 creationDate;
            nuint16 lastAccessDate;
            nuint32 lastUpdateDateAndTime;
            nuint32 fileOwnerID;
            nuint32 lastArchiveDateAndTime;
            char fileName[260];
         } NW_FILE_INFO2;

         The sequence number is a nine (9) byte identifier that the server
         uses as an index for searching.  In the first call to this function
         the first 4 bytes of the number need to be set to 0xFF.  This can be
         done by typecasting the pointer to a LONG, and assigning -1L, or
         0xFFFFFFFF to it.

ARGS:

INCLUDE: nwfile.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 02  Initialize Search
         87 03  Search For File or Subdirectory
         23 15  Scan File Information

CHANGES: 31 Aug 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWIntScanFileInformation2
(
   NWCONN_HANDLE     conn,
   NWDIR_HANDLE      dirHandle,
   pnstr8            fileName,
   nuint8            searchAttr,
   pnuint8           iterHandle,
   NW_FILE_INFO2  NWPTR info,
   nuint16           augmentFlag
)
{
   nuint8  formattedPath[NW_HANDLE_PATH_OFFSET];
   pnstr8  srchPattern;
   nuint8  saveChar;
   nuint16 serverVer, temp;
   nuint32 retMask;
   nint32  tmpIterHnd;
   NWCCODE ccode;
   NWNCPEntryStruct entryInfo;

   NWNCPCompPath compPath;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   /* Use MemCpy() to avoid casting - some platforms choke if the
      array is cast to long to check the first four bytes.  tmpIterHand
      is only used to check if these bytes are set for  a first time scan.
   */

   NCopy32(&tmpIterHnd, iterHandle);

   if(serverVer >= 3110)
   {
      NWNCPSearchSeq iterHndStruct;

      if(tmpIterHnd == -1L)
      {
         /* This is the first time through - we need to do the
         * initialize search NCP first.
         */

         compPath.luDirBase = dirHandle;
         compPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;

         /*...cut off last portion of path...*/
         srchPattern = &fileName[NWCStrLen(fileName)];

         while((srchPattern != fileName) &&
               (*srchPattern != '\\') &&
               (*srchPattern != '/') &&
               (*srchPattern != ':'))
         {
            srchPattern = NWPrevChar(fileName, srchPattern);
         }

         if (srchPattern != fileName)
         {
            saveChar = *srchPattern;     /* must be a single byte char     */
            *srchPattern = 0;

            NWNCPPackCompPath(-1, fileName , -1, &compPath,
                     (nflag32) (augmentFlag ? 1 : 0));

            *srchPattern = saveChar;
         }
         else
            NWNCPPackCompPath(-1, NULL, -1, &compPath, (nflag32) 0);

         if ((ccode = (NWCCODE) NWNCP87s2ScanFirst(&access, compPath.buNamSpc,
                     (nuint8) 0, &compPath, &iterHndStruct)) != 0)
            return ccode;

         iterHandle[0] = iterHndStruct.buVolNum;
         NCopy32(&iterHandle[1], &iterHndStruct.luDirNum);
         NCopy32(&iterHandle[5], &iterHndStruct.luEntryNum);
      }
      else
      {

         srchPattern = &fileName[NWCStrLen(fileName)];

         /* Isolate the search pattern */

         while((srchPattern != fileName) &&
               (*srchPattern != '\\') &&
               (*srchPattern != '/') &&
               (*srchPattern != ':'))
         {
            srchPattern = NWPrevChar(fileName, srchPattern);
         }

         if(srchPattern != fileName)
            srchPattern = NWNextChar(srchPattern); /* Point at the start of the pattern */
      }

      /*...return information bits...*/
      retMask = 0x01CD;

      __NWFillWildPath(formattedPath, srchPattern, (nuint16) (augmentFlag ? 1 : 0));

      iterHndStruct.buVolNum = iterHandle[0];
      NCopy32(&iterHndStruct.luDirNum, &iterHandle[1]);
      NCopy32(&iterHndStruct.luEntryNum, &iterHandle[5]);

      if((ccode = (NWCCODE) NWNCP87s3ScanNext(&access, compPath.buNamSpc,
            (nuint8) 0, (nuint16) searchAttr, retMask,
            &iterHndStruct, formattedPath[0], &formattedPath[1],
            NULL, &entryInfo)) != 0)
      {
         return (ccode);
      }

      iterHandle[0] = iterHndStruct.buVolNum;
      NCopy32(&iterHandle[1], &iterHndStruct.luDirNum);
      NCopy32(&iterHandle[5], &iterHndStruct.luEntryNum);

      if (info != NULL)
      {
         NWCMemMove(info->fileName, entryInfo.abuName, entryInfo.buNameLen);
         info->fileName[entryInfo.buNameLen] = 0;

         info->fileAttributes         = NGetLo8(NGetLo16(entryInfo.luAttrs));
         info->extendedFileAttributes = NGetHi8(NGetLo16(entryInfo.luAttrs));
         info->fileSize               = entryInfo.luDataStreamSize;
         info->creationDate           = entryInfo.suCreationDate;
         info->lastAccessDate         = entryInfo.suAccessedDate;
         info->lastUpdateDateAndTime  = NMake32(entryInfo.suModifiedTime,
                                                entryInfo.suModifiedDate);
         info->fileOwnerID            = NSwap32(entryInfo.luCreatorID);
         info->lastArchiveDateAndTime = NMake32(entryInfo.suArchivedTime,
                                                entryInfo.suArchivedDate);
      }
   }
   else
   {
      nuint8 intfileName[256];
      NWNCPFileInfo2 info2;

      NWCStrCpy(intfileName, fileName);
      NWConvertAndAugment(intfileName, augmentFlag);

      NCopy16(&temp, iterHandle);

      if ((ccode = (NWCCODE) NWNCP23s15ScanFiles(&access, dirHandle,
            searchAttr, &temp, (nuint8) NWCStrLen(intfileName),
            intfileName, info->fileName, &info2, NULL)) == 0)
      {
         NCopy16(iterHandle, &temp);

         info->fileAttributes          = info2.buAttrs;
         info->extendedFileAttributes  = info2.buExtAttrs;
         info->fileSize                = info2.luSize;
         info->creationDate            = info2.suCreationDate;
         info->lastAccessDate          = info2.suAccessedDate;
         info->lastUpdateDateAndTime   = NMake32(info2.suModifiedDate,
                                                 info2.suModifiedTime);
         info->fileOwnerID             = NSwap32(info2.luOwnerID);
         info->lastArchiveDateAndTime  = NMake32(info2.suArchiveDate,
                                                 info2.suArchiveTime);
      }
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/ascnfil2.c,v 1.9 1994/09/26 17:44:07 rebekah Exp $
*/

