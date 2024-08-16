/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s237.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s237GtPhyRecLocksConFl**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s237GtPhyRecLocksConFl
         (
            pNWAccess pAccess,
            nuint16  suTargetConnNum,
            nuint8   buDataStream,
            nuint8   buVolNum,
            nuint32  luDirEntry,
            pnuint16 psuIterHnd,
            pnuint16 psuNumOfLocks,
            pNWNCPPhysRecLocksByFile3x pPhysRecLocks,
         );

REMARKS:

ARGS:
      <> pAccess
      >  suTargetConnNum
      >  buDataStream
      >  buVolNum
      >  luDirEntry
      <> psuIterHnd
      <  psuNumOfLocks
      <  pPhysRecLocks (optional)

INCLUDE: ncpserve.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 221  Get Physical Record Locks By Connection And File (old)
         23 23  Get Physical Record Locks By File

NCP:     23 237  Get Physical Record Locks By Connection And File

CHANGES: 7 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s237GtPhyRecLocksConFl
(
   pNWAccess pAccess,
   nuint16  suTargetConnNum,
   nuint8   buDataStream,
   nuint8   buVolNum,
   nuint32  luDirEntry,
   pnuint16 psuIterHnd,
   pnuint16 psuNumOfLocks,
   pNWNCPPhysRecLocksByFile3x pPhysRecLocks
)
{
   #define NCP_FUNCTION       ((nuint) 23)
   #define NCP_SUBFUNCTION    ((nuint8) 237)
   #define NCP_STRUCT_LEN     ((nuint16) 11)
   #define NCP_REQ_LEN        ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REP_LEN        ((nuint) 4)
   #define NCP_REQ_FRAGS      ((nuint) 1)
   #define NCP_REP_FRAGS      ((nuint) 2)
   #define NCP_MAX_LOCK_INFO  ((nuint) 512)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN],
           abuLockInfo[NCP_MAX_LOCK_INFO];
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REP_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi16(&abuReq[3], &suTargetConnNum);
   abuReq[5] = buDataStream;
   abuReq[6] = buVolNum;
   NCopyLoHi32(&abuReq[7], &luDirEntry);
   NCopyLoHi16(&abuReq[11], psuIterHnd);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REP_LEN;

   replyFrag[1].pAddr = abuLockInfo;
   replyFrag[1].uLen  = NCP_MAX_LOCK_INFO;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag, NCP_REP_FRAGS,
         replyFrag, NULL);
   if (lCode == 0)
   {
      nint i, n;

      NCopyLoHi16(&abuReply[0], psuIterHnd);
      NCopyLoHi16(&abuReply[2], psuNumOfLocks);

      if (pPhysRecLocks)
      {
         for (i = 0; i < (nint)*psuNumOfLocks; i++)
         {
            n = i*11;

            NCopyLoHi16(&pPhysRecLocks[i].suTaskNum, &abuLockInfo[n]);
            pPhysRecLocks[i].buLockType = abuLockInfo[n+1];

            NCopyLoHi32(&pPhysRecLocks[i].luRecStart, &abuLockInfo[n+2]);
            NCopyLoHi32(&pPhysRecLocks[i].luRecEnd, &abuLockInfo[n+6]);
         }
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s237.c,v 1.7 1994/09/26 17:36:49 rebekah Exp $
*/
