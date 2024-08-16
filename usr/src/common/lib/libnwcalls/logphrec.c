/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:logphrec.c	1.5"
#if defined N_PLAT_WNT && defined N_ARCH_32
#include <windows.h>
#endif

#ifdef N_PLAT_OS2
#include <os2def.h>
#include <bsedos.h>
#endif

#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpsync.h"

#include "nwcaldef.h"
#include "nwcint.h"
#include "nwundoc.h"
#include "nwmisc.h"
#define  FILE_LOCKS_ONLY  /* only use the locks part of nwfile.h */
#include "nwfile.h"


/*manpage*NWLogPhysicalRecord***********************************************
SYNTAX:  NWCCODE N_API NWLogPhysicalRecord
         (
            NWFILE_HANDLE  fileHandle,
            nuint32        recordStartOffset,
            nuint32        recordLength,
            nuint8         lockFlags,
            nuint16        timeOutLimit
         )

REMARKS: Logs the specified record use by the workstation.  If bit 0 of the
         Lock flag is set, the server will immediately lock the logical
         record.  The values for the  lock flags are interpreted as follows:

           0x00 = Log Record
           0x01 = Log and Lock the record

         In order to avoid deadlock, a workstation is required to request
         those resources it will need to lock, and it does so by making an
         entry into a log table at the file server.  Once the log table is
         complete, the application can then lock those records.  The locking
         will only work if all records in the table are available.

         A physical record lock, as opposed to a logical lock, is the actual
         lock of a specified record relative to a physical file.  When a
         record is locked, it is also entered into a log table.  Records are
         allowed to be locked only if all records in the log table are
         available for locking.

         The timeout value determines how long the call will attempt to lock
         the file before returning the completion code (0x89FE - Timeout
         Failure).  Timeout values are measured in ticks - a tick is
         equivalent to 1/18th of a second - If this value is 0, then there
         is now wait.  This call cannot lock files that are already logged
         and exclusively locked by other applications.

ARGS:  > fileHandle
       > recordStartOffset
       > recordLength
       > lockFlags
       > timeOutLimit

INCLUDE: nwfile.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     26 --  Log Physical Record

CHANGES: Art 9/22/93
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWLogPhysicalRecord
(
   NWFILE_HANDLE  fileHandle,
   nuint32        recordStartOffset,
   nuint32        recordLength,
   nuint8         lockFlags,
   nuint16        timeOutLimit
)
{
#if  defined N_PLAT_WNT || \
     defined(N_PLAT_UNIX) || \
     defined(N_PLAT_NLM)
   NWCONN_HANDLE conn;
   NWCCODE ccode;
   nuint8 NWHandle[6];
   NWCDeclareAccess(access);


   if((ccode = NWConvertFileHandle((NWFILE_HANDLEINT) fileHandle,
                              6, NWHandle, &conn)) != 0)
       return (ccode);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP26SyncLogPhyRec( &access, lockFlags, NWHandle,
         recordStartOffset, recordLength, timeOutLimit));

#elif defined(N_PLAT_OS2)
   nuint8   reqBufA[8],
            reqBufB[10],
            lockMade = 0;
   nuint16  dif;
   USHORT   returnLength;
   ULONG    timeStamp1,
            timeStamp2;
   NWCCODE  ccode, ccode2;
   LOCK_SEM_NODE NWPTR lockHandle;
   NW_FRAGMENT       reqFrag[3];
   SFD_INFO          sourceFileBuffer;

   ccode = DosFSCtl((PBYTE)&sourceFileBuffer, sizeof(sourceFileBuffer),
                     (PUSHORT)&returnLength, NULL, 0, NULL, 0xC000 | 0x0003,
                     NULL, fileHandle, 1, 0L);
   if(ccode == 0x0032)
      return (0xff03);
   if(ccode != 0)
      return (ccode);

   NWCMemMove(&reqBufA[2], sourceFileBuffer.netwareFileHandle, 6);

   *(pnuint16)reqBufA = (nuint16)sourceFileBuffer.connID;

   *(pnuint32)&reqBufB[0] = NWLongSwap(recordStartOffset);
   *(pnuint32)&reqBufB[4] = NWLongSwap(recordLength);
   *(pnuint16)&reqBufB[8]  = NWWordSwap(timeOutLimit);

   reqFrag[0].fragAddress = &lockFlags;
   reqFrag[0].fragSize = 1;
   reqFrag[1].fragAddress = &reqBufA[2]; /* Don't send connID */
   reqFrag[1].fragSize = 6;
   reqFrag[2].fragAddress = reqBufB;
   reqFrag[2].fragSize = 10;

   if (NWCCallGate(_NWC_WAIT_LOCK_AVAIL, &sourceFileBuffer.connID))
      ccode = NWLockRequest(*(pnuint16)&reqBufA[0], 109, 3, reqFrag, 0, NULL);
   else
   {
      NWCCallGate(_NWC_GET_TIME_STAMP, &timeStamp1);
      do
      {
         ccode = NWRequest(*(pnuint16)&reqBufA[0], 26, 3, reqFrag, 0, NULL);

         NWCCallGate(_NWC_GET_TIME_STAMP, &timeStamp2);
         if(timeStamp2 < timeStamp1)
            dif = (USHORT)((-1L - timeStamp1 + timeStamp2) >> 6);
         else
            dif = (USHORT)((timeStamp2 - timeStamp1) >> 6);
         if(ccode == 0x89ff && (timeOutLimit == (nuint16)-1 || dif < timeOutLimit))
         {
            if(lockMade == 0)
            {
               ccode2 = NWAddLockRequestToList(0, &lockHandle);
               if(ccode2)
                  return (ccode2);
               lockMade = 1;
            }
            ccode2 = DosSemSetWait(lockHandle,
                  timeOutLimit == (nuint16)-1 ? (nuint32)-1L : (timeOutLimit - dif) * 56);
         }
         NWCCallGate(_NWC_GET_TIME_STAMP, &timeStamp2);
         if(timeStamp2 < timeStamp1)
            dif = (USHORT)((-1L - timeStamp1 + timeStamp2) >> 6);
         else
            dif = (USHORT)((timeStamp2 - timeStamp1) >> 6);
      } while(ccode == 0x89ff && (timeOutLimit == (nuint16)-1 || dif < timeOutLimit));

      if(lockMade == 1)
         NWRemoveLockRequestFromList(0, lockHandle);
   }

   if(ccode == 0x89ff || ccode == 0x897f)
      return (0x89fe);

   return (ccode);

#else

   NWCRegs regs;
   NWCCODE ccode;
   NWCDeclareAccess(access);

   if(_NWShellIsLoaded != _NETX_COM)
   {

      nuint8  NWHandle[11];

      regs.s.bx = (nuint16) fileHandle;
      regs.p.es_di = NWHandle;  /* this call actual returns size and
                                          mode but we ignore them */
      ccode = NWCVlmReq(0, _NWC_VLM_REDIR, _NWC_REDIR_FILE_INFO, &regs, USE_ES);
      if(ccode)
      {
         if(ccode == 0x42)
            return (6);
         return (ccode);
      }

      NWCSetConn(access, regs.s.cx);

      ccode = (NWCCODE) NWNCP26SyncLogPhyRec( &access, lockFlags, NWHandle,
               recordStartOffset, recordLength, timeOutLimit);
   }
   else
   {
      regs.b.ah = 0xbc;
      regs.b.al = lockFlags;
      regs.s.bx = fileHandle;
      regs.s.cx = (nuint16)(recordStartOffset >> 16);
      regs.s.dx = (nuint16)recordStartOffset;
      regs.s.bp = timeOutLimit;
      regs.s.si = (nuint16)(recordLength >> 16);
      regs.s.di = (nuint16)recordLength;
      /* this NWShellRequest uses DOS because windows cannot allow data
         to be passed in bp. Since no pointers need to be thunked, I believe
         that this call will work going through int 21h */
      NWCShellReq(&regs, USE_DOS);
      ccode = regs.b.al ? (nuint16) regs.b.al | 0x8900 : 0;
   }

   if(ccode == 0x897F)
      ccode = 0x89FE;

   return (ccode);

#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/logphrec.c,v 1.7 1994/09/26 17:47:57 rebekah Exp $
*/

