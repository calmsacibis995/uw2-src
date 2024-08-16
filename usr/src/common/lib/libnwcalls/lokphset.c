/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:lokphset.c	1.5"
#ifdef N_PLAT_OS2
#include <os2def.h>
#include <bsedos.h>
#endif

#include "nwcaldef.h"
#include "nwintern.h"
#include "nwundoc.h"
#include "ntypes.h"
#define  FILE_LOCKS_ONLY       /* only use the locks part of nwfile.h */
#include "nwfile.h"
#include "nwmisc.h"

#ifdef N_PLAT_OS2
#include "nwcint.h"
#endif

/*manpage*NWLockPhysicalRecordSet*******************************************
SYNTAX:  NWCCODE N_API NWLockPhysicalRecordSet
         (
            nuint8   lockFlags,
            nuint16  timeOut
         );

REMARKS: Attempts to lock a set of physical records that have
         previously been logged.

         The lockFlags is interpreted as follows:

         0x00 Lock records with exclusive lock
         0x01 Lock records with shearable lock

         The timeout limit is the time to wait before returning a failure
         if unable to lock the record set. The timeout value is specified
         in units of 1/18th of a second.

         A physical record lock, as opposed to a logical lock, is the
         actual lock of a specified record relative to a physical file.
         When a record is locked, it is also entered into a log table.
         Records are allowed to be locked only if all records in the log
         table are available for locking.  This is done to avoid deadlock.

         In order to avoid deadlock, a workstation is required to request
         those records it will need to lock, and it does so by making an
         entry into a log table at the file server. Once the log table is
         complete, the application can then lock those records. The locking
         will only work if all records in the table are available.

ARGS: >  lockFlags
      >  timeOut

INCLUDE: nwfile.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     110 --  Lock Physical Record Set
         27 --  Lock Physical Record Set (old)
         29 --  Release Physical Record Set

CHANGES: 1 Sep 1993 - purposely NOT NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWLockPhysicalRecordSet
(
   nuint8   lockFlags,
   nuint16  timeOut
)
{
   nuint8 reqBuf[3], errorBuf[1];
   NWCCODE ccode;
   NW_FRAGMENT reqFrag[1], errorFrag[1];

#ifdef N_PLAT_OS2

   nuint16 dif;
   NWCCODE ccode2;
   nuint8 lockMade = 0;
   nuint32 timeStamp1, timeStamp2;
   LOCK_SEM_NODE NWPTR lockHandle;

#endif

   reqBuf[0] = lockFlags;
   NCopyHiLo16( &reqBuf[1], &timeOut);

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

      ccode = (NWOrderedRequestToAll( (nuint16) 110, (nuint16) 27,
               (nuint16) 1, reqFrag, (nuint16) 29, (nuint16) 1, errorFrag));

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
            ccode2 = NWAddLockRequestToList( (nuint8) 0, &lockHandle);
            if(ccode2)
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
   } while(ccode == 0x89ff && (timeOut == (nuint16)-1 || dif < timeOut));

   if(lockMade == 1)
      NWRemoveLockRequestFromList( (nuint8) 0, lockHandle);

   if(ccode == 0x89ff)
      return (0x89fe);

#endif
   if(ccode == 0x897f)
      ccode = 0x89fe;

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/lokphset.c,v 1.7 1994/09/26 17:48:01 rebekah Exp $
*/

