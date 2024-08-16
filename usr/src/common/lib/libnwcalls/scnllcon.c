/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:scnllcon.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpserve.h"

#include "nwserver.h"
#include "nwsync.h"

/*manpage*NWScanLogicalLocksByConn******************************************
SYNTAX:  NWCCODE N_API NWScanLogicalLocksByConn
         (
            NWCONN_HANDLE  conn,
            NWCONN_NUM     connNum,
            pnint16        iterHandle,
            CONN_LOGICAL_LOCK N_FAR * logicalLock,
            CONN_LOGICAL_LOCKS N_FAR * logicalLocks
         )

REMARKS: Scans for all logical record locks by a specified connection

ARGS: >  conn
      >  connNum
         The connection number of the logged-in object to be scanned

      <> iterHandle
         Should be set to 0 initially and upon each subsequent call comes
         back with the number of the next record to be scanned.  Returns -1
         upon completion. Should not be changed during the scan.

      <  logicalLock  (optional)
         typedef struct
         {
            nuint16 taskNumber;
            nuint8  lockStatus;
            nstr8   logicalName[128];
         } CONN_LOGICAL_LOCK;

      <  logicalLocks
         typedef struct
         {
            nuint16 nextRequest;
            nuint16 numRecords;
            nuint8  records[508];
            nuint16 curOffset;
            nuint16 curRecord;
         } CONN_LOGICAL_LOCKS;

INCLUDE: nwsync.h

RETURN:  0x0000 Successful
         0x8801 Invalid Connection
         0x88FF Scan Completed
         0x89C6 No Console Privileges
         0x89FD Bad Connection Number

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 223  Get Logical Records By Connection (2.x)
         23 239  Get Logical Records By Connection (3.x)

CHANGES: 01 Jul 1992 - Rewritten - jwoodbur
         12 Nov 1992 - Modified to work - jwoodbur
         16 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWScanLogicalLocksByConn
(
   NWCONN_HANDLE   conn,
   NWCONN_NUM      connNum,
   pnint16         iterHandle,
   CONN_LOGICAL_LOCK N_FAR * pLogicalLock,
   CONN_LOGICAL_LOCKS N_FAR * pLogicalLocks
)
{
   NWCCODE ccode;
   nuint16 i, serverVer, temp, tmpIterHandle;
   pnuint8 tmpBuf, newBuf;
   nuint8  numRecs, records[512];

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if(*iterHandle < 0)
      return 0x88ff;

   if(pLogicalLock == NULL || *iterHandle == 0 ||
      pLogicalLocks->curRecord >= pLogicalLocks->numRecords)
   {

      if(*iterHandle && !pLogicalLocks->nextRequest)
      {
         *iterHandle = -1;
         return (0x88ff);
      }

      if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
         return (ccode);

      if(serverVer < 3000)
      {
         tmpIterHandle = (nuint16) *iterHandle;

         if ((ccode = (NWCCODE)NWNCP23s223GetLogicalRecsByConn(&access, connNum, &tmpIterHandle,
            &numRecs, records)) != 0)
         {
            return ((NWCCODE) ccode);
         }

         *iterHandle = (nint16) tmpIterHandle;
         pLogicalLocks->nextRequest = *iterHandle;
         pLogicalLocks->numRecords  = (nuint16) numRecs;

         tmpBuf = records;
         newBuf = pLogicalLocks->records;

         /* fill the pLogicalLocks->records byte stream */
         for(i = 0; i < pLogicalLocks->numRecords; i++)
         {
            temp = (nuint16) *tmpBuf++;
            NCopy16(newBuf, &temp); /* copies taskNumber (byte -> word) */
            newBuf += 2;

            *newBuf++ = *tmpBuf++; /* lock status */

            temp = ((nuint16) *tmpBuf) + 1;
            NWCMemMove(newBuf, tmpBuf, temp);

            newBuf += temp;
            tmpBuf += temp;
         }
      }
      else
      {
         tmpIterHandle = (nuint16) *iterHandle;

         if ((ccode = (NWCCODE)NWNCP23s239GetLogicalRecsByConn(&access, connNum,
               &tmpIterHandle, &pLogicalLocks->numRecords,
               pLogicalLocks->records)) != 0)
         {
            return ((NWCCODE) ccode);
         }

         pLogicalLocks->nextRequest = *iterHandle = (nint16) tmpIterHandle;
      }

      pLogicalLocks->curOffset = 0;
      pLogicalLocks->curRecord = 0;

      if(pLogicalLocks->numRecords == 0)
      {
         *iterHandle = -1;
         return(0);
      }

      /* pLogicalLocks->nextRequest will be non-zero ONLY if number of records
         exceeds the size of one reply buffer. If only one reply buffer was
         used, pLogicalLocks->numRecords is returned instead so that iterHandle
         will be non-zero.
      */

      *iterHandle = pLogicalLocks->nextRequest ? pLogicalLocks->nextRequest :
                                                pLogicalLocks->numRecords;
   }

   if(pLogicalLock)
   {
      pnuint8 pbuRecords = &pLogicalLocks->records[pLogicalLocks->curOffset];
      nuint8 buNameLen;

      NCopy16(&pLogicalLock->taskNumber, pbuRecords);
      pbuRecords += 2;
      pLogicalLock->lockStatus = *pbuRecords++;
      buNameLen = *pbuRecords++;
      NWCMemMove(pLogicalLock->logicalName, pbuRecords, buNameLen);
      pLogicalLock->logicalName[buNameLen] = 0;

      pLogicalLocks->curOffset += (4 + buNameLen);
      pLogicalLocks->curRecord++;
   }

   return (0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/scnllcon.c,v 1.7 1994/09/26 17:49:27 rebekah Exp $
*/
