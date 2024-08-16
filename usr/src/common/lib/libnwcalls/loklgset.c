/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:loklgset.c	1.5"
#ifdef N_PLAT_OS2
#include <os2def.h>
#include <bsedos.h>
#endif

#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"

#include "nwundoc.h"
#define  FILE_LOCKS_ONLY    /* only use the locks part of nwfile.h */
#include "nwfile.h"
#include "nwmisc.h"

#ifdef N_PLAT_OS2
#include "nwcint.h"
#endif

/*manpage*NWLockLogicalRecordSet********************************************
SYNTAX:  NWCCODE N_API NWLockLogicalRecordSet
         (
            nuint8   lockFlags,
            nuint16  timeOut
         )

REMARKS: Attempts to lock all logical record names in the current log table.

         In order to avoid deadlock, a workstation is required to request
         those files it will need to lock, and it does so by making an
         entry into a log table at the file server.  Once the log table is
         complete, the application can then lock those files.  The locking
         will only work if all files in the table are available.

         A logical record is simply a name (a string) registered with the
         file server.  The name (as with a semaphore) can then be locked or
         unlocked by applications, and can be used as a "good-guy"
         inter-application locking mechanism.  Note that locking or
         unlocking a logical record does not physically lock or unlock
         those resources associated with the logical record since only the
         applications using the record know about such an association.

         The lockFlags is interpreted as follows:

         0x00  Lock record with an exclusive lock
         0x01  Lock record with a shareable lock

         The timeout value determines how long the call will attempt to
         lock the logical record name(s) before returning a failure.
         Timeout values are measured in units of 1/18th of a second.

ARGS: >  lockFlags,
      >  timeOut

INCLUDE: nwfile.h

RETURN:  0x0000  Successful
         0x89FE  Timeout
         0x89FF  Lock Error

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     108 --  Lock Logical Record Set
         10 --  Lock Logical Record Set (old)

CHANGES: 3 Sep 1993 - purposely NOT NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWLockLogicalRecordSet
(
   nuint8   lockFlags,
   nuint16  timeOut
)
{
   nuint8        reqBuf[3],
               errorBuf[1];
   NWCCODE ccode;
   NW_FRAGMENT reqFrag[1],
               errorFrag[1];
#ifdef N_PLAT_OS2
   NWCCODE ccode2;
   nuint8  lockMade = 0;
   nuint16 dif;
   nuint32 timeStamp1,
           timeStamp2;
   LOCK_SEM_NODE NWPTR lockHandle;
#endif

   reqBuf[0] = lockFlags;
   NCopyHiLo16(&reqBuf[1], &timeOut);

   reqFrag[0].fragAddress = reqBuf;
   reqFrag[0].fragSize = 3;

   errorBuf[0] = 1;  /* Unlock last set locked */
   errorFrag[0].fragAddress = errorBuf;
   errorFrag[0].fragSize = 1;

#ifdef N_PLAT_OS2

   NWCCallGate(_NWC_GET_TIME_STAMP, &timeStamp1);
   do
   {

#endif

      ccode = (NWOrderedRequestToAll( (nuint16) 108, (nuint16) 10,
               (nuint16) 1, reqFrag, (nuint16) 13, (nuint16) 1, errorFrag));

#ifdef N_PLAT_OS2

      NWCCallGate(_NWC_GET_TIME_STAMP, &timeStamp2);
      if (timeStamp2 < timeStamp1)
         dif = (nuint16)((-1L - timeStamp1 + timeStamp2) >> 6);
      else
         dif = (nuint16)((timeStamp2 - timeStamp1) >> 6);
      if (ccode == 0x89ff && (timeOut == (nuint16)-1 || dif < timeOut))
      {
         if (lockMade == 0)
         {
            ccode2 = NWAddLockRequestToList( (nuint8) 1, &lockHandle);
            if (ccode2)
               return(ccode2);
            lockMade = 1;
         }
         ccode2 = DosSemSetWait(lockHandle,
            timeOut == (nuint16)-1 ? (nuint32)-1L : (timeOut - dif) * 56);
      }
      NWCCallGate(_NWC_GET_TIME_STAMP, &timeStamp2);
      if (timeStamp2 < timeStamp1)
         dif = (nuint16)((-1L - timeStamp1 + timeStamp2) >> 6);
      else
         dif = (nuint16)((timeStamp2 - timeStamp1) >> 6);
   } while (ccode == 0x89ff && (timeOut == (nuint16)-1 || dif < timeOut));

   if(lockMade == 1)
      NWRemoveLockRequestFromList( (nuint8) 1, lockHandle);

   if(ccode == 0x89ff)
      return (0x89fe);

#endif
   if(ccode == 0x897f)
      ccode = 0x89fe;

   return (ccode);

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/loklgset.c,v 1.7 1994/09/26 17:47:59 rebekah Exp $
*/
