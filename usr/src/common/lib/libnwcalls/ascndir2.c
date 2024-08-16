/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:ascndir2.c	1.6"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwncp.h"
#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwdirect.h"
#include "nwmisc.h"
#include "nwerror.h"
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwlocale.h"
#include "nwclocal.h"
#include "nwnamspc.h"
#include "nwserver.h"

/*manpage*NWIntScanDirectoryInformation2************************************
SYNTAX:  NWCCODE N_API NWIntScanDirectoryInformation2
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         srchPath,
            pnuint8        sequence,
            pnstr8         dirName,
            pnuint32       dateTime,
            pnuint32       ownerID,
            pnuint8        rightsMask,
            nuint16        augmentFlag
         );


REMARKS: Returns directory information for a directory specified by the
         server connection ID, NetWare directory handle, and directory path.

         This function is the same as NWIntScanDirectoryInformation except
         that it can handle long names in the paths.

         The searchDirPath parameter can contain wildcard characters.

         The subdirectory number is an internal network number indicating
         the actual subdirectory slot of the directory.

ARGS:  < sequence
         Pointer to the iteration number (initialized to -1 for the first
         call) to use for subsequent calls

       < dirName (optional)
         Pointer to the name of the found directory (256-bytes)

       < dateTime (optional)
         Pointer to the creation date and time of the directory

       < ownerID (optional)
         Pointer to the bindery object ID of the directory's owner

       < rightsMask (optional)
         Pointer to the maximum rights mask of the found directory

       > augmentFlag
         if set to USE_NW_WILD_MATCH all wild card characters will be
         augmented.

INCLUDE: nwdirect.h

RETURN:  NO_FILES_FOUND_ERROR
         if no files were found matching the specified path

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 06  Obtain File or Subdirectory Information
         87 02  Initialize Search
         87 03  Search For File or Subdirectory
         22 02  Scan Directory Information

CHANGES: 8 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWIntScanDirectoryInformation2
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         srchPath,
   pnuint8        sequence,
   pnstr8         dirName,
   pnuint32       dateTime,
   pnuint32       ownerID,
   pnuint8        rightsMask,
   nuint16        augmentFlag
)
{
   pnstr8   pSrchPattern, pTmpPath;
   nstr8    saveChar, tmpPattern[256];
   nuint8   buNamSpc;
   nuint16  serverVer;
   NWCCODE  ccode;
   NWNCPEntryStruct entryInfo;
   NWNCPCompPath cPath;
   nint32   lTmpIter;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   /* use MemCpy() to avoid casting - some platforms choke if the
      array is cast to a long to check if the first four bytes are
      -1. lTmpIter is only used to check if these bytes are set for
      a first time scan. */

   NCopy32(&lTmpIter, sequence);

   if(serverVer >= 3110)
   {
      NWNCPSearchSeq iterHndStruct;

      cPath.luDirBase = (nuint32) dirHandle;
      cPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;

      augmentFlag = augmentFlag ? (nuint16) NCP_AUGMENT : (nuint16) 0;

      if(CheckPathAtRoot(conn, dirHandle, (pnuint8) srchPath))
      {
         /* If path points to a root, make Obtain File or Subdirectory Information NCP. */
         if(lTmpIter == -1L)
         {
            NWNCPPackCompPath(-1, srchPath, -1, &cPath,
               (nflag32) augmentFlag);

            if ((ccode = (NWCCODE)NWNCP87s6GetEntryInfo(&access,
                  cPath.buNamSpc, cPath.buNamSpc, FA_DIRECTORY,
                  (nuint32) 0x0901L, &cPath, &entryInfo)) == 0)
            {
               entryInfo.luCreatorID  = NSwap32(entryInfo.luCreatorID);
               entryInfo.luModifierID = NSwap32(entryInfo.luModifierID);
               entryInfo.luArchiverID = NSwap32(entryInfo.luArchiverID);

               if(dirName)
               {
                  NWCMemMove(dirName, entryInfo.abuName,
                     entryInfo.buNameLen);

                  dirName[entryInfo.buNameLen] = (nstr8) 0x00;
               }

               if(dateTime)
                  *dateTime = NMake32(entryInfo.suCreationTime,
                                         entryInfo.suCreationDate);

               if(ownerID)
                  *ownerID = entryInfo.luCreatorID;

               if(rightsMask)
                  *rightsMask = (nuint8)entryInfo.suInheritedRightsMask;

               lTmpIter = 0;
               NCopy32(sequence, &lTmpIter);
            }
            return(ccode);
         }
         else
            return(NO_FILES_FOUND_ERROR);
      }

      buNamSpc = (nuint8) __NWGetCurNS(conn, dirHandle, srchPath);

      if(lTmpIter == -1L)
      {
         if(srchPath == NULL || *srchPath == 0)
         {
            if ((ccode = NWGetDirectoryHandlePath(conn, dirHandle,
                     (pnstr8) entryInfo.abuName)) != 0)
            {
               return ccode;
            }

            pTmpPath = (pnstr8) entryInfo.abuName;
            dirHandle = 0;
         }
         else
            pTmpPath = srchPath;

         pSrchPattern = pTmpPath + NWCStrLen(pTmpPath);

         /* replace with parsing function which skips any escape chars */
         while( (pSrchPattern != pTmpPath) &&
               (*pSrchPattern != '\\')     &&
               (*pSrchPattern != '/')      &&
               (*pSrchPattern != ':'))
         {
            pSrchPattern = NWPrevChar(pTmpPath, pSrchPattern);
         }

         if(pSrchPattern != pTmpPath)
         {
            saveChar       = *pSrchPattern;
            *pSrchPattern  = 0;

            NWNCPPackCompPath(-1, pTmpPath, buNamSpc, &cPath,
               (nflag32)augmentFlag);

            *pSrchPattern  = saveChar;
            pSrchPattern = NWNextChar(pSrchPattern);
         }
         else
            NWNCPPackCompPath(-1, NULL, buNamSpc, &cPath, (nflag32) augmentFlag);

         if ((ccode = (NWCCODE) NWNCP87s2ScanFirst(&access, buNamSpc,
               (nuint8) 0, &cPath, &iterHndStruct)) != 0)
            return ccode;

         sequence[0] = iterHndStruct.buVolNum;
         NCopy32(&sequence[1], &iterHndStruct.luDirNum);
         NCopy32(&sequence[5], &iterHndStruct.luEntryNum);
      }
      else
      {
         if (srchPath == NULL || *srchPath == 0)
         {
            if ((ccode = NWGetDirectoryHandlePath(conn, dirHandle,
                  (pnstr8) entryInfo.abuName)) != 0)
               return ccode;

            pTmpPath = (pnstr8)entryInfo.abuName;
            dirHandle = 0;
         }
         else
            pTmpPath = srchPath;

         pSrchPattern = pTmpPath + NWCStrLen(pTmpPath);
         while( (pSrchPattern != pTmpPath) &&
               (*pSrchPattern != '\\')     &&
               (*pSrchPattern != '/')      &&
               (*pSrchPattern != ':'))
         {
            pSrchPattern = NWPrevChar(pTmpPath, pSrchPattern);
         }

         if(pSrchPattern != pTmpPath)
            pSrchPattern = NWNextChar(pSrchPattern);
      }

      NWNCPMakeACompPath(NWCStrLen(pSrchPattern), pSrchPattern,
         (nint)buNamSpc, NULL, tmpPattern, NULL, (nflag32) augmentFlag);

      iterHndStruct.buVolNum = sequence[0];
      NCopy32(&iterHndStruct.luDirNum, &sequence[1]);
      NCopy32(&iterHndStruct.luEntryNum, &sequence[5]);

      if ((ccode = (NWCCODE) NWNCP87s3ScanNext(&access, buNamSpc, (nuint8) 0,
            FA_DIRECTORY, (nuint32) 0x0901L, &iterHndStruct,
            (nuint8) tmpPattern[0], &tmpPattern[1], NULL, &entryInfo)) != 0)
      {
         return((NWCCODE) ccode);
      }

      sequence[0] = iterHndStruct.buVolNum;
      NCopy32(&sequence[1], &iterHndStruct.luDirNum);
      NCopy32(&sequence[5], &iterHndStruct.luEntryNum);

      entryInfo.luCreatorID  = NSwap32(entryInfo.luCreatorID);
      entryInfo.luModifierID = NSwap32(entryInfo.luModifierID);
      entryInfo.luArchiverID = NSwap32(entryInfo.luArchiverID);

      if(dirName)
      {
         NWCMemMove(dirName, entryInfo.abuName, entryInfo.buNameLen);
         dirName[entryInfo.buNameLen] = (nstr8) 0;
      }

      if(dateTime)
         *dateTime = NMake32(entryInfo.suCreationTime,
                                entryInfo.suCreationDate);
      if(ownerID)
         *ownerID = entryInfo.luCreatorID;

      if(rightsMask)
         *rightsMask = (nuint8)entryInfo.suInheritedRightsMask;
   }
   else
   {
      nuint16 suTemp;

      NCopy16(&suTemp, sequence);

      if(suTemp == 0xffff)
        suTemp = 0;

      if ((ccode = NWIntScanDirectoryInformation(conn, dirHandle,
            srchPath, &suTemp, dirName, dateTime,
            ownerID, rightsMask, (nuint16)(augmentFlag ?
            (nuint16)NCP_AUGMENT : (nuint16)0))) == 0)
      {
         if (ownerID)
            *ownerID = NSwap32(*ownerID);
      }

      NCopy16(sequence, &suTemp);
   }

   return(ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/ascndir2.c,v 1.9 1994/09/26 17:44:02 rebekah Exp $
*/

