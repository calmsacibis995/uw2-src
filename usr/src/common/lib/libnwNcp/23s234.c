/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s234.c	1.6"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s234GetConnTaskInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s234GetConnTaskInfo
         (
            pNWAccess pAccess,
            nuint16  suConnNum,
            pnuint8  pbuLockStatus,
            pNWNCPWaitRec3x pWaitRec,
            pnuint8  pbuTaskCount,
            pNWNCPTaskStruct3x pTasksB127,
         )

REMARKS: Given the Connection Number, this call allows an operator to retrieve
         control information about the current lock status of the connection's
         active tasks.  The format of the information returned depends in part on
         the value returned in Connection Status.

         On return, the Lock Status field will contain one of the following values:

            0     Normal (connection free to run)
            1     Connection waiting on a physical record lock
            2     Connection waiting on a file lock
            3     Connection waiting on a logical record lock
            4     Connection waiting on a semaphore

         The Wait Record contains information about the current lock state of the
         target connection.  If the target connection is waiting for a lock request
         to be serviceable, then the Wait Record contains information about the
         resource for which the connection is waiting.  The Wait Record format
         varies depending on the value of Lock Status.

            If Lock Status = 0, Wait Record includes
                  When Lock Status is 0, no Wait Record field exists in the
                  reply message.

            If Lock Status = 1, Wait Record includes

                  word             waitingTaskNumber
                  long             beginAddress
                  long             endAddress
                  byte             volumeNumber
                  word             directoryEntry
                  byte             lockedName (filename)

            If Lock Status = 2, Wait Record includes

                  word             waitingTaskNumber
                  byte             volumeNumber
                  word             directoryEntry
                  byte             lockedName (file name)

            If Lock Status = 3, Wait Record includes

                  word             waitingTaskNumber
                  byte             lockedName (record name)

            If Lock Status = 4, Wait Record includes

                  word             waitingTaskNumber
                  byte             lockedName (semaphore name)

         Number Of Active Tasks indicates the number of Task and Task State
         pairs to follow.

         Task(#) indicates the task number for which information is to be
         returned.

         Task State(#) is the task's state at the time of the request.  The
         following states are defined:

            Value                State

            0x00         Normal task
            0x01         TTS explicit transaction in progress
            0x02         TTS implicit transaction in progress
            0x04         Shared file set lock in progress

         This routine replaces the NetWare 286 v2.1 NCP Get Connection Task
         Information (0x2222  23  218).

ARGS: <> pAccess
      >  suConnNum
      <  pWaitRec
      <  pbuLockStatus
      <  pbuTaskCount
      <  pTasksB127,

INCLUDE: ncpserve.h

RETURN:  0x0000 Successful

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 218  Get Connection Task Information (old)
         23 236  Get Connections Using A File
         23 235  Get Connection's Open Files

NCP:     23 234  Get Connection Task Information

CHANGES: 23 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s234GetConnTaskInfo
(
   pNWAccess          pAccess,
   nuint16           suConnNum,
   pnuint8           pbuLockStatus,
   pNWNCPWaitRec3x   pWaitRec,
   pnuint8           pbuTaskCount,
   pNWNCPTaskStruct3x pTasksB127
)
{
   #define NCP_FUNCTION       ((nuint)           23)
   #define NCP_SUBFUNCTION    ((nuint8)         234)
   #define NCP_STRUCT_LEN     ((nuint16)          5)
   #define NCP_REQ_LEN        ((nuint)            5)
   #define NCP_REP_LEN        ((nuint)          510)
   #define NCP_REQ_FRAGS      ((nuint)            1)
   #define NCP_REP_FRAGS      ((nuint)            1)

   nint32   lCode;
   pnuint8 pbuBuf;
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REP_FRAGS];
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];
   nint i;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReq[3], &suConnNum);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REP_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REP_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      pbuBuf = abuReply;
      *pbuLockStatus = *pbuBuf++;
      if (*pbuLockStatus == (nuint8) 0)
      {
         pWaitRec->u.case0.buReserved = *pbuBuf++;
      }
      if (*pbuLockStatus == (nuint8) 1)
      {
         NCopyLoHi32(&pWaitRec->u.case1.luRecStart, pbuBuf);
         pbuBuf += 4;
         NCopyLoHi32(&pWaitRec->u.case1.luRecEnd, pbuBuf);
         pbuBuf += 4;
         pWaitRec->u.case1.buVolNum = *pbuBuf++;
         NCopyLoHi32(pWaitRec->u.case1.luDirEntry, pbuBuf);
         pbuBuf += 4;
         pWaitRec->u.case1.buNamSpc = *pbuBuf++;
         pWaitRec->u.case1.buLockedFileNameLen = *pbuBuf++;
         for (i=0; i < (nint) pWaitRec->u.case1.buLockedFileNameLen; i++)
         {
            pWaitRec->u.case1.pbstrLockedFileName[i] = *pbuBuf++;
         }
      }
      if (*pbuLockStatus == (nuint8) 2)
      {
         pWaitRec->u.case2.buVolNum = *pbuBuf++;
         NCopyLoHi32(pWaitRec->u.case2.luDirEntry, pbuBuf);
         pbuBuf += 4;
         pWaitRec->u.case2.buNamSpc = *pbuBuf++;
         pWaitRec->u.case2.buLockedFileNameLen = *pbuBuf++;
         for (i=0; i < (nint) pWaitRec->u.case2.buLockedFileNameLen; i++)
         {
            pWaitRec->u.case2.pbstrLockedFileName[i] = *pbuBuf++;
         }
      }
      if (*pbuLockStatus == (nuint8) 3)
      {
         pWaitRec->u.case3.buLockedRecNameLen = *pbuBuf++;
         for (i=0; i < (nint) pWaitRec->u.case3.buLockedRecNameLen; i++)
         {
            pWaitRec->u.case3.pbstrLockedRecName[i] = *pbuBuf++;
         }
      }
      if (*pbuLockStatus == (nuint8) 4)
      {
         pWaitRec->u.case4.buLockedSemNameLen = *pbuBuf++;
         for (i=0; i < (nint) pWaitRec->u.case4.buLockedSemNameLen; i++)
         {
            pWaitRec->u.case4.pbstrLockedSemName[i] = *pbuBuf++;
         }
      }
      *pbuTaskCount = *pbuBuf++;
      for (i=0; i < (nint) *pbuTaskCount; i++)
      {
         NCopyLoHi16(&pTasksB127[i].suTaskNum, pbuBuf);
         pbuBuf += 2;
         pTasksB127[i].buTaskState = *pbuBuf++;
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s234.c,v 1.8 1994/09/26 17:36:44 rebekah Exp $
*/
