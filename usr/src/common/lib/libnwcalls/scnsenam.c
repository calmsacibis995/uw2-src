/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:scnsenam.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwsync.h"
#include "nwserver.h"

/*manpage*NWScanSemaphoresByName********************************************
SYNTAX:  NWCCODE N_API NWScanSemaphoresByName
         (
            NWCONN_HANDLE conn,
            pnstr8 semName,
            pnint16 iterHandle,
            SEMAPHORE N_FAR * semaphore,
            SEMAPHORES N_FAR * semaphores
         );

REMARKS: Scans information about a semaphore by name

ARGS: >  conn
      >  semName
         The name of the semaphore to be scanned

      <> iterHandle
         Should be set to 0 initially and upon each subsequent call
         comes back with the number of the next record to be scanned.
         Returns -1 upon completion. Should not be changed during the scan.

      <  semaphore (optional)
         Returns the SEMAPHORE structure as follows:

            NWCONN_NUM connNumber;
            nuint16    taskNumber;

         If this parameter is a NULL pointer, then instead of being
         returned one by one, the records are returned in bunches in
         the 'semaphores' parameter.

      <  semaphores
         Returns the SEMAPHORES structure as follows:

            nuint16 nextRequest;
            nuint16 openCount;
            nuint16 semaphoreValue;
            nuint16 semaphoreCount;
            SEMAPHORE semaphores[170];
            nuint16 curRecord;

INCLUDE: nwsync.h

RETURN:  0x0000  SUCCESSFUL
         0x8801  INVALID_CONNECTION_ID
         0x88FF  SCAN_COMPLETED
         0x89C6  NO_CONSOLE_RIGHTS

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 226  Get Semaphore Information
         23 242  Get Semaphore Information

CHANGES: 16 Jul 1992 - Rewritten - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWScanSemaphoresByName
(
   NWCONN_HANDLE     conn,
   pnstr8            semName,
   pnint16           iterHandle,
   SEMAPHORE   N_FAR * semaphore,
   SEMAPHORES  N_FAR * semaphores
)
{
   NWCCODE ccode;
   nuint16 serverVer;
   nint i;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if(*iterHandle < 0)
      return (0x88ff);

   if(semaphore == NULL || *iterHandle == 0 ||
      semaphores->curRecord >= semaphores->semaphoreCount)
   {
      /* if *iterHandle is non-zero and semaphores->nextRequest is 0, then
         all the records that could be returned have been returned. */

      if (*iterHandle && !semaphores->nextRequest)
      {
         *iterHandle = -1;
         return (0x88ff);
      }

      ccode = NWGetFileServerVersion(conn, &serverVer);
      if (ccode)
         return (ccode);

      if (serverVer >= 3000)
      {
         ccode = (NWCCODE) NWNCP23s242GetSemInfo(&access,
                     &semaphores->nextRequest, (nuint8)NWCStrLen(semName),
                     semName, &semaphores->openCount,
                     &semaphores->semaphoreValue, &semaphores->semaphoreCount,
                     (pNWNCPSemInfo3x)semaphores->semaphores);

         if (ccode)
            return (ccode);
      }
      else
      {
         nuint8 buTempSemVal, buTempSemCount;
         NWNCPSemInfo2x aSems[170];

         ccode = (NWCCODE) NWNCP23s226GetSemInfo(&access,
                     &semaphores->nextRequest, (nuint8)NWCStrLen(semName),
                     semName, &semaphores->openCount,
                     &buTempSemVal, &buTempSemCount,
                     (pNWNCPSemInfo2x)aSems);

         if (ccode)
            return (ccode);

         semaphores->semaphoreValue = (nuint16) buTempSemVal;
         semaphores->semaphoreCount = (nuint16) buTempSemCount;

         for (i = 0; i < (nint)semaphores->semaphoreCount; i++)
         {
            semaphores->semaphores[i].connNumber = aSems[i].suLogConnNum;
            semaphores->semaphores[i].taskNumber = (nuint16) aSems[i].buTaskNum;
         }
      }

      semaphores->curRecord = 0;

      if(semaphores->semaphoreCount == 0)
      {
         *iterHandle = -1;
         return (0);
      }

      /* semaphores->nextRequest will be non-zero ONLY if number of records
         exceeds the size of one reply buffer. If only one reply buffer was
         used, semaphores->numRecords is returned instead so that iterHandle
         will be non-zero.
      */
      *iterHandle = semaphores->nextRequest ? semaphores->nextRequest :
                                                   semaphores->semaphoreCount;
   }

   if(semaphore)
   {
      /* Yes, an NWCMemMove could be used here, but this saves space and is faster. */

      i = semaphores->curRecord++;
      semaphore->connNumber = semaphores->semaphores[i].connNumber;
      semaphore->taskNumber = semaphores->semaphores[i].taskNumber;
   }
   return (0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/scnsenam.c,v 1.7 1994/09/26 17:49:38 rebekah Exp $
*/
