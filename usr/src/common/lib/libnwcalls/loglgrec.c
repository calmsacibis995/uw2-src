/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:loglgrec.c	1.5"
#ifdef N_PLAT_OS2
# include <os2def.h>
# include <bsedos.h>
#endif

#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpsync.h"

#include "nwcaldef.h"
#define FILE_LOCKS_ONLY     /* only use the locks part of nwfile.h */
#include "nwfile.h"

#ifdef N_PLAT_OS2
#include "nwundoc.h"
#include "nwcint.h"
#include "nwmisc.h"
#endif

/*manpage*NWLogLogicalRecord************************************************
SYNTAX:  NWCCODE N_API NWLogLogicalRecord
         (
            NWCONN_HANDLE  conn,
            pnstr8         logRecName,
            nuint8         lockFlags,
            nuint16        timeOut
         );

REMARKS: Logs the specified record use by the workstation.
         If bit 0 of the Lock flag is set, the server will immediately lock
         the logical record.  The values for the  lock flags are
         interpreted as follows:

         0x00 = Log Record
         0x01 = Log and Lock the record

         In order to avoid deadlock, a workstation is required to request
         those resources it will need to lock, and it does so by making an
         entry into a log table at the file server.  Once the log table is
         complete, the application can then lock those records.  The
         locking will only work if all records in the table are available.

         A logical record is simply a name (a string) registered with the
         file server.  The name (as with a semaphore) can then be locked or
         unlocked by applications, and can be used as a "good-guy"
         inter-application locking mechanism.  Note that locking or
         unlocking a logical record does not physically lock or unlock
         those resources associated with the logical record since only the
         applications using the record know about such an association.

         The timeout value determines how long the call will attempt to
         lock the file before returning the completion code (0x89FE -
         Timeout Failure).  Timeout values are measured in ticks - a tick
         is equivalent to 1/18th of a second - If this value is 0, then
         there is now wait.  This call cannot lock files that are already
         logged and exclusively locked by other applications.

ARGS: >  conn
      >  logRecName
      >  lockFlags
      >  timeOut

INCLUDE: nwfile.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     09 --  Log Logical Record

CHANGES: 30 Aug 1993 - purposely NOT NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWLogLogicalRecord
(
   NWCONN_HANDLE  conn,
   pnstr8         logRecName,
   nuint8         lockFlags,
   nuint16        timeOut
)
{
#ifdef N_PLAT_OS2

   LOCK_SEM_NODE NWPTR lockHandle;
   ULONG       timeStamp1;
   ULONG       timeStamp2;
   nuint16     dif;
   nuint8      lockMade = 0;
   NWCCODE     ccode2;
   NW_FRAGMENT reqFrag[2];
   nuint8      reqBuf[4];

#endif

   nuint8 buNameLen;
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   buNameLen = (nuint8) NWCStrLen(logRecName);
   if (buNameLen > 128)
      buNameLen = (nuint8) 128;

#ifdef N_PLAT_OS2

   reqBuf[0] = lockFlags;
   *((nuint16 NWPTR)(&(reqBuf[1]))) = NWWordSwap(timeOut);
   reqBuf[3] = buNameLen;

   reqFrag[0].fragAddress = reqBuf;
   reqFrag[0].fragSize = 4;
   reqFrag[1].fragAddress = logRecName;
   reqFrag[1].fragSize = reqBuf[3];

   if (NWCCallGate(_NWC_WAIT_LOCK_AVAIL, &conn))
      ccode = NWLockRequest(conn, 107, 2, reqFrag, 0, NULL);
   else
   {
      NWCCallGate(_NWC_GET_TIME_STAMP, &timeStamp1);
      do
      {

#endif

         ccode = (NWCCODE) NWNCP9SyncLogLogRec(&access, lockFlags, timeOut,
                     buNameLen, logRecName);

#ifdef N_PLAT_OS2

         NWCCallGate(_NWC_GET_TIME_STAMP, &timeStamp2);
         if(timeStamp2 < timeStamp1)
            dif = (USHORT)((-1L - timeStamp1 + timeStamp2) >> 6);
         else
            dif = (USHORT)((timeStamp2 - timeStamp1) >> 6);
         if(ccode == 0x89ff && (timeOut == (nuint16)-1 ||
               dif < timeOut))
         {
            if(lockMade == 0)
            {
               ccode2 = NWAddLockRequestToList(1, &lockHandle);
               if(ccode2)
                  return (ccode2);
               lockMade = 1;
            }
            ccode2 = DosSemSetWait(lockHandle,
                        timeOut == (nuint16)-1 ? (nuint32)-1L :
                        (timeOut - dif) * 56);
         }
         NWCCallGate(_NWC_GET_TIME_STAMP, &timeStamp2);
         if(timeStamp2 < timeStamp1)
            dif = (USHORT)((-1L - timeStamp1 + timeStamp2) >> 6);
         else
            dif = (USHORT)((timeStamp2 - timeStamp1) >> 6);
      } while(ccode == 0x89ff && (timeOut == (nuint16)-1 || dif < timeOut));

      if(lockMade == 1)
         NWRemoveLockRequestFromList(1, lockHandle);
   }
   if(ccode == 0x89ff)
      return (0x89fe);

#endif
   if(ccode == 0x897f)
      ccode = 0x89fe;

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/loglgrec.c,v 1.7 1994/09/26 17:47:54 rebekah Exp $
*/

