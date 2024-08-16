/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:loklokst.c	1.5"
#ifdef N_PLAT_OS2
# include <os2def.h>
# include <bsedos.h>
#endif

#if defined N_PLAT_WNT && defined N_ARCH_32
#include <windows.h>
#endif

#include "nwcaldef.h"
#include "ntypes.h"
#include "nwcint.h"
#include "nwundoc.h"
#define  FILE_LOCKS_ONLY       /* only use the locks part of nwfile.h */
#include "nwfile.h"
#include "nwmisc.h"

/*manpage*NWLockFileLockSet*************************************************
SYNTAX:  NWCCODE N_API NWLockFileLockSet
         (
            nuint16 timeOut
         );

REMARKS: Attempts to lock the specified file lock set.

         In order to avoid deadlock, a workstation is required to request
         those files it will need to lock, and it does so by making an
         entry into a log table at the file server.  Once the log table is
         complete, the application can then lock those files.  The locking
         will only work if all files in the table are available.

         The timeout value is used to specify the time before giving up the
         attempt and returning a failure.  The timeout is specified in
         units of 1/18th of a second.

ARGS: >  timeOut

INCLUDE: nwfile.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     04 --  Lock File Set
         06 --  Release File Set
         106 --  Lock File Set

CHANGES: 30 Aug 1993 - purposely NOT NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWLockFileLockSet
(
   nuint16 timeOut
)
{
   nuint8        reqBuf[2],
               errorBuf[1];
   NWCCODE ccode;
   NW_FRAGMENT   reqFrag[1],
               errorFrag[1];
#ifdef N_PLAT_OS2
   nuint8  lockMade = 0;
   NWCCODE ccode2;
   nuint16 dif;
   ULONG   timeStamp1;
   ULONG   timeStamp2;
   LOCK_SEM_NODE NWPTR lockHandle;
#endif

   NCopyHiLo16(&reqBuf[0], &timeOut);

   reqFrag[0].fragAddress = reqBuf;
   reqFrag[0].fragSize = 2;

   errorBuf[0] = 1;  /* Unlock last set locked */
   errorFrag[0].fragAddress = errorBuf;
   errorFrag[0].fragSize = 1;

#ifdef N_PLAT_OS2
   NWCCallGate(_NWC_GET_TIME_STAMP, &timeStamp1);
   do
   {
#endif
      ccode = (NWOrderedRequestToAll(106, 4, 1, reqFrag, 6, 1, errorFrag));

#ifdef N_PLAT_OS2
      NWCCallGate(_NWC_GET_TIME_STAMP, &timeStamp2);
      if (timeStamp2 < timeStamp1)
         dif = (USHORT)((-1L - timeStamp1 + timeStamp2) >> 6);
      else
         dif = (USHORT)((timeStamp2 - timeStamp1) >> 6);
      if (ccode == 0x89ff && (timeOut == (nuint16)-1 || dif < timeOut))
      {
         if (lockMade == 0)
         {
            ccode2 = NWAddLockRequestToList(2, &lockHandle);
            if(ccode2)
               return (ccode2);
            lockMade = 1;
         }
         ccode2 = DosSemSetWait(lockHandle,
            timeOut == (nuint16)-1 ? (nuint32)-1L : (timeOut - dif) * 56);
      }
      NWCCallGate(_NWC_GET_TIME_STAMP, &timeStamp2);
      if (timeStamp2 < timeStamp1)
         dif = (USHORT)((-1L - timeStamp1 + timeStamp2) >> 6);
      else
         dif = (USHORT)((timeStamp2 - timeStamp1) >> 6);
   } while(ccode == 0x89ff && (timeOut == (nuint16)-1 || dif < timeOut));

   if(lockMade == 1)
      NWRemoveLockRequestFromList(2, lockHandle);

   if(ccode == 0x89ff)
      return (0x89fe);
#endif
   if(ccode == 0x897f)
      ccode = 0x89fe;

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/loklokst.c,v 1.7 1994/09/26 17:48:00 rebekah Exp $
*/
