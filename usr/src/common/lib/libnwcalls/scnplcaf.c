/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:scnplcaf.c	1.6"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwmisc.h"
#include "nwsync.h"
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwlocale.h"
#include "nwclocal.h"
#include "nwserver.h"
#include "nwfile.h"
#include "nwerror.h"

/*manpage*NWScanPhysicalLocksByConnFile*************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWCCODE ) NWScanPhysicalLocksByConnFile
         (
            NWCONN_HANDLE  conn,
            NWCONN_NUM connNum,
            NWDIR_HANDLE dirHandle,
            pnstr8  pbstrPath,
            nuint8  buDataStream,
            pnint16 psIterHnd,
            CONN_PHYSICAL_LOCK NWPTR lock,
            CONN_PHYSICAL_LOCKS NWPTR locks
         );

REMARKS: Scans for all physical record locks by a specified
         connection on a specified file

ARGS:  > conn
       > connNum
       > dirHandle
       > pbstrPath
       > buDataStream
      <> psIterHnd
      <  lock (optional)
         Returns the CONN_PHYSICAL_LOCK structure as follows:

            nuint16     taskNumber;
            nuint8     lockType;
            nuint32    recordStart;
            nuint32    recordEnd;

         If this parameter is a NULL pointer, then instead of being
         returned one by one, the records are returned in bunches in
         the 'locks' parameter.

      <  locks
         Returns the CONN_PHYSICAL_LOCKS structure as follows:

            typedef struct
            {
               nuint16 nextRequest;
               nuint16 numRecords;
               CONN_PHYSICAL_LOCK locks[51];
               nuint16 curRecord;
               nuint8 reserved[22];
            } CONN_PHYSICAL_LOCKS;

INCLUDE: nwsync.h

RETURN:  0x0000  SUCCESSFUL
         0x8801  INVALID_CONNECTION_ID
         0x88FF  SCAN_COMPLETED
         0x89C6  NO_CONSOLE_RIGHTS
         0x89FF  FILE_NOT_OPEN
         0x89FD  BAD_STATION_NUMBER     (bad connection number)

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 221  Get Physical Record Locks By Connection And File
         23 237  Get Physical Record Locks By Connection And File

CHANGES: 16 Jul 1992 - Rewritten - jwoodbur
         7 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWCCODE )
NWScanPhysicalLocksByConnFile
(
   NWCONN_HANDLE  conn,
   NWCONN_NUM connNum,
   NWDIR_HANDLE dirHandle,
   pnstr8  pbstrPath,
   nuint8  buDataStream,
   pnint16 psIterHnd,
   CONN_PHYSICAL_LOCK NWPTR lock,
   CONN_PHYSICAL_LOCKS NWPTR locks
)
{
   int i;
   nuint16 suServerVer;
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if(*psIterHnd < 0)
      return 0x88ff;

   if(lock == NULL ||
      *psIterHnd == 0 ||
      locks->curRecord >= locks->numRecords)
   {
      /* if *psIterHnd is non-zero and locks->nextRequest is 0, then
         all the records that could be returned have been returned. */
      if(*psIterHnd && !locks->nextRequest)
      {
         *psIterHnd = -1;
         return 0x88ff;
      }

      if((ccode = NWGetFileServerVersion(conn, &suServerVer)) != 0)
         return ccode;

      if(*psIterHnd == 0)
      {
         if(suServerVer >= 3000)
         {
            DIR_ENTRY entry;

            if((ccode = NWConvertPathToDirEntry(conn, dirHandle, pbstrPath,
                  &entry)) != 0)
            {
               return ccode;
            }

            NCopy16(&locks->reserved[0], &connNum);
            locks->reserved[2] = buDataStream;
            locks->reserved[3] = entry.volNumber;
            NCopy32(&locks->reserved[4], &entry.dirEntry);
            NCopy16(&locks->reserved[8], psIterHnd);
         }
         else
         {
            pnstr8 tmpPtr;
            nstr8 tmpPath[260];
            nuint16 suTemp;

            NWCStrCpy(tmpPath, pbstrPath);

            tmpPtr = &tmpPath[NWCStrLen(tmpPath)];
            while(*tmpPtr != '\\' && *tmpPtr != ':' && tmpPtr > &tmpPath[0])
            {
               tmpPtr = NWPrevChar(tmpPath, tmpPtr);
            }

            if (*tmpPtr == '\\' || *tmpPtr == ':')
            {
               tmpPtr++;
            }

            if(*tmpPtr == 0)
               return(INVALID_PARAMETER);

            NWCMemSet(&locks->reserved[7], 0, 14);
            NWCStrCpy(&locks->reserved[7], tmpPtr);

            if (tmpPtr > &tmpPath[0] && *(tmpPtr - 1) != ':')
               tmpPtr--;

            *tmpPtr = '\0';

            ccode = NWFileSearchInitialize(conn, dirHandle, tmpPath,
                  &locks->reserved[4], &suTemp, NULL, NULL);
            NCopySwap16(&locks->reserved[5], &suTemp);

            if (ccode != 0)
            {
               return ccode;
            }

            NCopySwap16(&locks->reserved[0], &connNum);
            NCopy16(&locks->reserved[2], psIterHnd);
         }
      }

      if(suServerVer >= 3000)
      {
         nuint8  buVolNum;
         nuint32 luDirEntry;

         buVolNum = locks->reserved[3];
         NCopy32(&luDirEntry, &locks->reserved[4]);

         ccode = (NWCCODE) NWNCP23s237GtPhyRecLocksConFl(&access, connNum,
                     buDataStream, buVolNum, luDirEntry, &locks->nextRequest,
                     &locks->numRecords,
                     (pNWNCPPhysRecLocksByFile3x)locks->locks);
         if (ccode)
            return ccode;
      }
      else
      {
         nuint8  buNumOfLocks211, buVolNum;
         nuint16 suDirID;
         NWNCPPhysRecLocksByFile2x aLockRecs[51];

         buVolNum = locks->reserved[4];
         NCopy16(&suDirID, &locks->reserved[5]);

         if ((ccode = (NWCCODE) NWNCP23s221GtPhyRecLocksConFl(&access, connNum,
                  &locks->nextRequest, buVolNum, suDirID,
                  pbstrPath, &buNumOfLocks211, NULL,
                  aLockRecs)) == 0)
         {
            locks->numRecords = (nuint16) buNumOfLocks211;
            for (i = 0; i < (int) buNumOfLocks211; i++)
            {
               locks->locks[i].taskNumber  = (nuint16) aLockRecs[i].buTaskNum;
               locks->locks[i].lockType    = aLockRecs[i].buLockType;
               locks->locks[i].recordStart = aLockRecs[i].luRecStart;
               locks->locks[i].recordEnd   = aLockRecs[i].luRecEnd;
            }
         }
         else
            return ccode;
      }

      locks->curRecord = 0;
      if(locks->numRecords == 0)
      {
         *psIterHnd = -1;
         return(0);
      }

      /* locks->nextRequest will be non-zero ONLY if number of records
         exceeds the size of one reply buffer. If only one reply buffer was
         used, locks->numRecords is returned instead so that psIterHnd
         will be non-zero.
      */
      *psIterHnd = (nint16) (locks->nextRequest ? locks->nextRequest :
            locks->numRecords);
   }

   if(lock)
   {
      i = locks->curRecord++;
      lock->taskNumber  = locks->locks[i].taskNumber;
      lock->lockType    = locks->locks[i].lockType;
      lock->recordStart = locks->locks[i].recordStart;
      lock->recordEnd   = locks->locks[i].recordEnd;
   }

   return 0;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/scnplcaf.c,v 1.8 1994/09/26 17:49:33 rebekah Exp $
*/

