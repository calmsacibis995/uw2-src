/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:scnplfil.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwmisc.h"
#include "nwsync.h"
#include "nwserver.h"

/*manpage*NWScanPhysicalLocksByFile*****************************************
SYNTAX:  NWCCODE N_API NWScanPhysicalLocksByFile
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         path,
            nuint8         dataStream,
            pnint16        IterHandle,
            PHYSICAL_LOCK N_FAR * lock,
            PHYSICAL_LOCKS N_FAR * locks
         );

REMARKS: Scans for all record locks on a specified physical file

ARGS: >  conn
      >  dirHandle
      >  path
      >  dataStream
      <> IterHandle
         Should be set to 0 initially and upon each subsequent call
         comes back with the number of the next record to be scanned.
         Returns -1 upon completion. Should not be changed during the scan.

      <  lock (optional)
         Returns the PHYSICAL_LOCK structure as follows:

            nuint16 loggedCount;
            nuint16 shareableLockCount;
            nuint32 recordStart;
            nuint32 recordEnd;
            nuint16 connNumber;
            nuint16 taskNumber;
            nuint16 lockType;

         If this parameter is a NULL pointer, then instead of being
         returned one by one, the records are returned in bunches in
         the 'locks' parameter.

      <  locks
         Returns the PHYSICAL_LOCKS structure as follows:

            nuint16 nextRequest;
            nuint16 numRecords;
            PHYSICAL_LOCK locks[32];
            nuint16 curRecord;
            nuint8  reserved[8];

INCLUDE: nwsync.h

RETURN:  0x0000  SUCCESSFUL
         0x8801  INVALID_CONNECTION_ID
         0x88FF  SCAN_COMPLETED
         0x89C6  NO_CONSOLE_RIGHTS
         0x89FF  FILE_NOT_OPEN

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23  222  Get Physical Record Locks By File
         23  238  Get Physical Record Locks By File

CHANGES: 16 Jul 1992 - Rewritten - jwoodbur
         7 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWScanPhysicalLocksByFile
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   nuint8         dataStream,
   pnint16        IterHandle,
   PHYSICAL_LOCK  N_FAR * lock,
   PHYSICAL_LOCKS N_FAR * locks
)
{
   nuint16 serverVer, nextReq;
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if(*IterHandle < 0)
      return (0x88ff);

   if(lock == NULL ||
         *IterHandle == 0 ||
      locks->curRecord >= locks->numRecords)
   {
      /* if *IterHandle is non-zero and locks->nextRequest is 0, then
         all the records that could be returned have been returned. */
      if(*IterHandle && !locks->nextRequest)
      {
         *IterHandle = -1;
         return (0x88ff);
      }

      if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
         return (ccode);

      if(serverVer >= 3000)
      {
         nuint16 suNumLocks;
         DIR_ENTRY dir;

         if(*IterHandle == 0)
         {
            if((ccode = NWConvertPathToDirEntry(conn, dirHandle,
                           path, &dir)) != 0)
               return (ccode);

            nextReq = (nuint16) 0;
         }
         else
            nextReq = locks->nextRequest;

         ccode = (NWCCODE) NWNCP23s238GetPhyRecLocksFile(&access,
                     dataStream, dir.volNumber, dir.dirEntry, &nextReq,
                     &suNumLocks, (pNWNCPPhysRecLocks3x)locks->locks);
         if (ccode)
            return (ccode);
      }
      else
      {
         NWNCPPhysRecLocks2x aRecLocks[32];
         nuint8 buNumLocks, buPathLen;
         nint i;

         buPathLen = (nuint8) NWCStrLen(path);
         nextReq = (nuint16) *IterHandle;

         ccode = (NWCCODE) NWNCP23s222GetPhyRecLocksFile(&access, &nextReq,
                     (nuint8) dirHandle, buPathLen, path, &buNumLocks,
                     NULL, aRecLocks);

         if (ccode)
            return (ccode);

         locks->nextRequest = nextReq;
         locks->numRecords  = (nuint16) buNumLocks;

         for(i = 0; i < (nint)buNumLocks; i++)
         {
            locks->locks[i].loggedCount = aRecLocks[i].suLoggedCount;
            locks->locks[i].shareableLockCount =
               aRecLocks[i].suShareableLockCount;
            locks->locks[i].recordStart = aRecLocks[i].luRecStart;
            locks->locks[i].recordEnd   = aRecLocks[i].luRecEnd;
            locks->locks[i].connNumber  = aRecLocks[i].suLogConnNum;
            locks->locks[i].taskNumber  = (nuint16) aRecLocks[i].buTaskNum;
            locks->locks[i].lockType    = (nuint8) aRecLocks[i].buLockType;
         }
      }

      locks->curRecord = 0;

      if(locks->numRecords == 0)
      {
         *IterHandle = -1;
         return (0);
      }

      /* locks->nextRequest will be non-zero ONLY if number of records
         exceeds the size of one reply buffer. If only one reply buffer was
         used, locks->numRecords is returned instead so that IterHandle
         will be non-zero.
      */
      *IterHandle = locks->nextRequest ? locks->nextRequest : locks->numRecords;
   }

   if(lock)
      NWCMemMove(lock, &locks->locks[locks->curRecord++], sizeof(PHYSICAL_LOCK));

   return (0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/scnplfil.c,v 1.7 1994/09/26 17:49:35 rebekah Exp $
*/
