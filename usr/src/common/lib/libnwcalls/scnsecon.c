/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:scnsecon.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwmisc.h"
#include "nwsync.h"
#include "nwserver.h"

/*manpage*NWScanSemaphoresByConn********************************************
SYNTAX:  NWCCODE N_API NWScanSemaphoresByConn
         (
            NWCONN_HANDLE conn,
            NWCONN_NUM connNum,
            pnint16 iterHandle,
            CONN_SEMAPHORE N_FAR * semaphore,
            CONN_SEMAPHORES N_FAR * semaphores
         );

REMARKS: Scans information about the semaphores a specified connection
         has open

ARGS: >  conn
      >  connNum
         The connection number of the logged-in object to be scanned

      <> iterHandle
         Should be set to 0 initially and upon each subsequent call
         comes back with the number of the next record to be scanned.
         Returns -1 upon completion. Should not be changed during the scan.

      <  semaphore (optional)
         Returns the CONN_SEMAPHORE structure as follows:

            nuint16 openCount;
            nuint16 semaphoreValue;
            nuint16 taskNumber;
            nstr8   semaphoreName[128];

         If this parameter is a NULL pointer, then instead of being
         returned one by one, the records are returned in bunches in
         the 'semaphores' parameter.

      <  semaphores
         Returns the CONN_SEMAPHORES structure as follows:

            nuint16 nextRequest;
            nuint16 numRecords;
            nuint8  records[508];
            nuint16 curOffset;
            nuint16 curRecord;

INCLUDE: nwsync.h

RETURN:  0x0000  SUCCESSFUL
         0x8801  INVALID_CONNECTION_ID
         0x88FF  SCAN_COMPLETED
         0x89C6  NO_CONSOLE_RIGHTS
         0x89FD  BAD_STATION_NUMBER     (bad connection number)

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 225  Get Connection's Semaphores
         23 241  Get Connection's Semaphores

CHANGES: 8 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWScanSemaphoresByConn
(
   NWCONN_HANDLE     conn,
   NWCONN_NUM        connNum,
   pnint16           iterHandle,
   CONN_SEMAPHORE    N_FAR * semaphore,
   CONN_SEMAPHORES   N_FAR * semaphores
)
{
   nint i;
   NWCCODE ccode;
   nuint16 serverVer, tmpBuf;
   nuint8  buf[508];
   pnuint8 pbuTmpBuf, pbuNewBuf;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if(*iterHandle < 0)
      return (0x88ff);

   if(semaphore == NULL ||
      *iterHandle == 0 ||
      semaphores->curRecord >= semaphores->numRecords)
   {
      nuint16 suTempNums;
      nuint16 suTempIterHnd = (nuint16) *iterHandle;

      /* if *iterHandle is non-zero and semaphores->nextRequest is 0, then
         all the records that could be returned have been returned. */

      if(*iterHandle && !semaphores->nextRequest)
      {
         *iterHandle = -1;
         return (0x88ff);
      }

      ccode = NWGetFileServerVersion(conn, &serverVer);
      if(ccode)
         return (ccode);

      if(serverVer < 3000)
      {
         if ((ccode = (NWCCODE) NWNCP23s225GetConnSems(&access, connNum,
                     &suTempIterHnd, &suTempNums, buf)) == 0)
         {
            semaphores->numRecords = suTempNums;
         }
      }
      else
      {
         ccode = (NWCCODE) NWNCP23s241GetConnSems(&access, connNum,
                  &suTempIterHnd, &semaphores->numRecords, semaphores->records);
      }

      if (ccode)
         return (ccode);

      semaphores->nextRequest = suTempIterHnd;

      if(serverVer < 3000)
      {
         nuint16 temp;

         pbuTmpBuf = buf;
         pbuNewBuf = semaphores->records;

         for(i = 0; i < (nint)semaphores->numRecords; i++)
         {
            temp = (nuint16) *pbuTmpBuf++;  /* open count */
            NCopy16(pbuNewBuf, &temp);

            pbuNewBuf += 2;

            temp = (nuint16) *pbuTmpBuf++;  /* semaphore value */
            NCopy16(pbuNewBuf, &temp);

            pbuNewBuf += 2;

            temp = (nuint16) *pbuTmpBuf++;  /* task number */
            NCopy16(pbuNewBuf, &temp);

            pbuNewBuf += 2;

            temp = ((nuint16)*pbuTmpBuf) + 1;
            NWCMemMove(pbuNewBuf, pbuTmpBuf, temp);

            pbuNewBuf += temp;
            pbuTmpBuf += temp + 1; /* 2.x semaphores are zero terminated */
         }
      }

      semaphores->curOffset = 0;
      semaphores->curRecord = 0;

      if(semaphores->numRecords == 0)
      {
         *iterHandle = -1;
         return (0);
      }

      /* semaphores->nextRequest will be non-zero ONLY if number of records
         exceeds the size of one reply buffer. If only one reply buffer was
         used, semaphores->numRecords is returned instead so that iterHandle
         will be non-zero.
      */
      *iterHandle = semaphores->nextRequest ?
                   semaphores->nextRequest :
                   semaphores->numRecords;
   }

   if(semaphore)
   {
      nuint16 temp;

      tmpBuf = semaphores->curOffset;

      NCopy16(&semaphore->openCount, &semaphores->records[tmpBuf]);
      tmpBuf += 2;
      NCopy16(&semaphore->semaphoreValue, &semaphores->records[tmpBuf]);
      tmpBuf += 2;
      NCopy16(&semaphore->taskNumber, &semaphores->records[tmpBuf]);
      tmpBuf += 2;

      temp = (nuint16) semaphores->records[tmpBuf++];
      NWCMemMove(semaphore->semaphoreName, &semaphores->records[tmpBuf],
                 (nuint) temp);
      semaphore->semaphoreName[temp] = 0x00;
      semaphores->curOffset = tmpBuf + temp;
      semaphores->curRecord++;
   }

   return (0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/scnsecon.c,v 1.7 1994/09/26 17:49:36 rebekah Exp $
*/
