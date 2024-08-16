/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:scnllnam.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwmisc.h"
#include "nwsync.h"
#include "nwserver.h"

/*manpage*NWScanLogicalLocksByName******************************************
SYNTAX:  NWCCODE N_API NWScanLogicalLocksByName
         (
            NWCONN_HANDLE  conn,
            pnstr8         logicalName,
            pnint16        iterHandle,
            LOGICAL_LOCK N_FAR * logicalLock,
            LOGICAL_LOCKS N_FAR * logicalLocks
         );

REMARKS: Scans for all record locks on a specified logical name

ARGS: >  conn
      >  logicalName
         The logical lock name which is to be scanned

      <> iterHandle
         Should be set to 0 initially and upon each subsequent call comes
         back with the number of the next record to be scanned.  Returns -1
         upon completion. Should not be changed during the scan.

      <  logicalLock
         Returns the LOGICAL_LOCK structure to track what's going on.
         typedef struct
         {
            NWCONN_NUM connNumber;
            nuint16 taskNumber;
            nuint8  lockStatus;
         } LOGICAL_LOCK;

      <  logicalLocks
         Returns the LOGICAL_LOCKS structure
         typedef struct
         {
            nuint16 useCount;
            nuint16 shareableLockCount;
            nuint8  locked;
            nuint16 nextRequest;
            nuint16 numRecords;
            LOGICAL_LOCK logicalLock[128];
            nuint16 curRecord;
         } LOGICAL_LOCKS;

INCLUDE: nwsync.h

RETURN:  0x88ff  SCAN_COMPLETED
         0x89c6  NO_CONSOLE_RIGHTS

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 224  Get Logical Record Information
         23 240  Get Logical Record Information

CHANGES: 16 Jul 1992 - Completely rewritten - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWScanLogicalLocksByName
(
   NWCONN_HANDLE     conn,
   pnstr8            logicalName,
   pnint16           iterHandle,
   LOGICAL_LOCK N_FAR * logicalLock,
   LOGICAL_LOCKS N_FAR * logicalLocks
)
{
   nuint16 i, serverVer;
   NWCCODE ccode;
   NWNCPLogicalRecInfo3x	LogicalRecInfo[128];
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if (*iterHandle < 0)
      return (0x88ff);

   if (*iterHandle == 0 ||
      logicalLock == NULL ||
      logicalLocks->curRecord >= logicalLocks->numRecords)
   {
      nuint16 temp;

      /* if *iterHandle is non-zero and logicalLocks->nextRequest is 0, then
         all the records that could be returned have been returned. */
      if (*iterHandle && !logicalLocks->nextRequest)
      {
         *iterHandle = -1;
         return (0x88ff);
      }

      if ((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
         return (ccode);

      temp = (nuint16) *iterHandle;

      if (serverVer >= 3000)
      {
         ccode = (NWCCODE) NWNCP23s240GetLogicalRecInfo (&access,
                     &temp, (nuint8) NWCStrLen(logicalName),
                     (pnuint8)logicalName, &logicalLocks->useCount,
                     &logicalLocks->shareableLockCount,
                     &logicalLocks->locked, &logicalLocks->numRecords,
					 LogicalRecInfo);

		if (ccode == 0)
			for (i=0; i < logicalLocks->numRecords; i++) {
				logicalLocks->logicalLock[i].connNumber =
									(NWCONN_NUM)(LogicalRecInfo[i].suConnNum);
				logicalLocks->logicalLock[i].taskNumber =
									LogicalRecInfo[i].suTaskNum;
				logicalLocks->logicalLock[i].lockStatus =
									LogicalRecInfo[i].buLockStatus;
			}
      }
      else
      {
         NWNCPLogicalRecInfo2x locks[128];
         nuint8 tempNumRecs;

         ccode = (NWCCODE) NWNCP23s224GetLogicalRecInfo (&access,
                     &temp, (nuint8) NWCStrLen(logicalName),
                     (pnuint8)logicalName, &logicalLocks->useCount,
                     &logicalLocks->shareableLockCount,
                     &logicalLocks->locked, &tempNumRecs, locks);

         if (ccode == 0)
         {
            nint i;
            LOGICAL_LOCK N_FAR * pLock = logicalLocks->logicalLock;

            for (i = 0; i < (nint) tempNumRecs; i++)
            {
               pLock[i].connNumber = locks[i].suConnNum;
               pLock[i].taskNumber = (nuint16) locks[i].buTaskNum;
               pLock[i].lockStatus = locks[i].buLockStatus;
            }
         }

         logicalLocks->numRecords = (nuint16) tempNumRecs;
      }

      if (ccode != 0)
         return ccode;

      *iterHandle = (nint16) temp;
      logicalLocks->curRecord = 0;
      if (logicalLocks->numRecords == 0)
      {
         *iterHandle = -1;
         return (0);
      }
      /* logicalLocks->nextRequest will be non-zero ONLY if number of records
         exceeds the size of one reply buffer. If only one reply buffer was
         used, logicalLocks->numRecords is returned instead so that iterHandle
         will be non-zero.
      */
      *iterHandle = logicalLocks->nextRequest ? logicalLocks->nextRequest :
                                                logicalLocks->numRecords;
   }

   if (logicalLock)
   {
      i = logicalLocks->curRecord++;
      logicalLock->connNumber = logicalLocks->logicalLock[i].connNumber;
      logicalLock->taskNumber = logicalLocks->logicalLock[i].taskNumber;
      logicalLock->lockStatus = logicalLocks->logicalLock[i].lockStatus;
   }
   return (0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/scnllnam.c,v 1.7 1994/09/26 17:49:29 rebekah Exp $
*/
