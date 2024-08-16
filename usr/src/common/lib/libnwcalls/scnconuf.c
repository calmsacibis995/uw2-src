/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:scnconuf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwmisc.h"
#include "nwfile.h"
#include "nwclocal.h"
#include "nwserver.h"
#include "nwerror.h"

/*manpage*NWScanConnectionsUsingFile****************************************
SYNTAX:  NWCCODE N_API NWScanConnectionsUsingFile
         (
            NWCONN_HANDLE           conn,
            NWDIR_HANDLE            dirHandle,
            pnstr8                  path,
            pnint16                 iterHandle,
            CONN_USING_FILE   NWPTR fileUse,
            CONNS_USING_FILE  NWPTR fileUsed
         );

REMARKS: Scans all connections that are using a specified file

ARGS: >  conn
      >  dirHandle
      >  path

      <> iterHandle
         Should be set to 0 for first call. After each call returns the
         number of the next record to be scanned.  Returns -1 upon
         completion. Should not be changed during the scan.

      <  fileUse (optional)
         Returns the CONN_USING_FILE structure as follows:

         typedef struct
         {
           NWCONN_NUM connNumber;
           nuint16 taskNumber;
           nuint8 lockType;
           nuint8 accessControl;
           nuint8 lockFlag;
         } CONN_USING_FILE;

         If this parameter is a NULL pointer, the records are returned in
         bunches in the 'fileUsed' parameter rather than individually.

      <  fileUsed
         Returns the CONNS_USING_FILE structure as follows:

         typedef struct
         {
            nuint16 nextRequest;
            nuint16 useCount;
            nuint16 openCount;
            nuint16 openForReadCount;
            nuint16 openForWriteCount;
            nuint16 denyReadCount;
            nuint16 denyWriteCount;
            nuint8  locked;
            nuint8  forkCount;
            nuint16 connCount;
            CONN_USING_FILE connInfo[70];
         } CONNS_USING_FILE;

INCLUDE: nwfile.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 220  Get Connections Using A File (2.x)
         23 236  Get Connections Using A File (3.x)

CHANGES: 31 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWScanConnectionsUsingFile
(
   NWCONN_HANDLE           conn,
   NWDIR_HANDLE            dirHandle,
   pnstr8                  path,
   pnint16                 iterHandle,
   CONN_USING_FILE   NWPTR fileUse,
   CONNS_USING_FILE  NWPTR fileUsed
)
{
   nuint16 i, serverVer;
   NWCCODE ccode = 0;
   CONN_USING_FILE NWPTR connPtr;
   nstr8 tempPath[260];
   NWNCPConnUsingFile2x conns[70];
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if (*iterHandle < 0)
      return ((NWCCODE) 0x88FF);    /* scan completed */

   if (!fileUse || *iterHandle == 0 ||
         (nuint16)*iterHandle >= fileUsed->connCount)
   {
      if (*iterHandle == 0)
      {
         fileUsed->connCount = 0;
         fileUsed->nextRequest = 1;
      }

      if (!fileUsed->nextRequest)
      {
         if (!fileUse || (nuint16) *iterHandle >= fileUsed->connCount)
         {
            *iterHandle = -1;
            return (0x88FF);   /* scan completed */
         }
      }

      if (WildCardCheck((pnstr8)path))
         return (WILD_CARD_NOT_ALLOWED);

      NWCStrCpy(tempPath, path);

      ccode = NWGetFileServerVersion(conn, &serverVer);
      if (ccode)
         return (ccode);

      if (serverVer < 3000)
      {
         nuint8 temp;
         nuint16 tempLastRec;

         NWConvertToSpecChar(tempPath);

         tempLastRec = (nuint16) (*iterHandle ? fileUsed->nextRequest : 0);

         ccode = (NWCCODE) NWNCP23s220GetConnsUsingAFile(&access,
                  &tempLastRec, (nuint8)dirHandle,
                  (nuint8) NWCStrLen(tempPath), tempPath,
                  &fileUsed->useCount, &fileUsed->openCount,
                  &fileUsed->openForReadCount, &fileUsed->openForWriteCount,
                  &fileUsed->denyReadCount, &fileUsed->denyWriteCount,
                  &fileUsed->locked, &temp, conns);

         if (ccode)
            return (ccode);

         fileUsed->forkCount = 0;
         fileUsed->connCount = (nuint16) temp;

         for(i = 0; i < fileUsed->connCount && i < 70; i++)
         {
            fileUsed->connInfo[i].connNumber = conns[i].suConnNum;
            fileUsed->connInfo[i].taskNumber = conns[i].buTaskNum;
            fileUsed->connInfo[i].lockType   = conns[i].buLockType;
            fileUsed->connInfo[i].accessControl = conns[i].buAccessControl;
            fileUsed->connInfo[i].lockFlag   = conns[i].buLockFlag;
         }

         *iterHandle = 1;
      }
      else
      {
         DIR_ENTRY tempDirEntry;
         nuint16 tempNextRec;

         ccode = NWConvertPathToDirEntry(conn, dirHandle, tempPath,
                     &tempDirEntry);

         tempNextRec = *iterHandle ? fileUsed->nextRequest : 0;

         ccode = (NWCCODE) NWNCP23s236GetConnsUsingAFile(&access, (nuint8) 0,
                  tempDirEntry.volNumber, tempDirEntry.dirEntry,
                  &tempNextRec, &fileUsed->useCount,
                  &fileUsed->openCount, &fileUsed->openForReadCount,
                  &fileUsed->openForWriteCount, &fileUsed->denyReadCount,
                  &fileUsed->denyWriteCount, &fileUsed->locked,
                  &fileUsed->forkCount, &fileUsed->connCount,
                  (pNWNCPConnUsingFile3x) fileUsed->connInfo);

         if (ccode)
            return (ccode);

/*         *iterHandle = 1;*/
      }
   }

   (*iterHandle)++;

   if (fileUse && fileUsed->connCount)
   {
      connPtr = &fileUsed->connInfo[*iterHandle - 1];
      fileUse->connNumber    = connPtr->connNumber;
      fileUse->taskNumber    = connPtr->taskNumber;
      fileUse->lockType      = connPtr->lockType;
      fileUse->accessControl = connPtr->accessControl;
      fileUse->lockFlag      = connPtr->lockFlag;
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/scnconuf.c,v 1.7 1994/09/26 17:49:25 rebekah Exp $
*/
