/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s238.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s238GetPhyRecLocksFile**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s238GetPhyRecLocksFile
         (
            pNWAccess pAccess,
            nuint8   buDataStream,
            nuint8   buVolNum,
            nuint32  luDirEntry,
            pnuint16 psuIterHnd,
            pnuint16 psuNumLocks,
            pNWNCPPhysRecLocks3x pPhysRecLocks,
         );

REMARKS:

ARGS: <> pAccess
      >  buDataStream
      >  buVolNum
      >  luDirEntry
      <> psuIterHnd
      <  psuNumLocks
      <  pPhysRecLocks (optional)

INCLUDE: ncpserve.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 222  Get Physical Record Locks By File (old)
         23 237  Get Physical Records Locks By Connection And File

NCP:     23 238  Get Physical Record Locks By File

CHANGES: 7 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s238GetPhyRecLocksFile
(
   pNWAccess       pAccess,
   nuint8         buDataStream,
   nuint8         buVolNum,
   nuint32        luDirEntry,
   pnuint16       psuIterHnd,
   pnuint16       psuNumLocks,
   pNWNCPPhysRecLocks3x pPhysRecLocks
)
{
   #define NCP_FUNCTION       ((nuint) 23)
   #define NCP_SUBFUNCTION    ((nuint8) 238)
   #define NCP_STRUCT_LEN     ((nuint16) 9)
   #define NCP_REQ_LEN        ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REP_LEN        ((nuint) 4)
   #define NCP_REQ_FRAGS      ((nuint) 1)
   #define NCP_REP_FRAGS      ((nuint) 2)
   #define NCP_MAX_REC_LOCKS  ((nuint) 512)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN],
           abuRecLocks[NCP_MAX_REC_LOCKS];
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REP_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   abuReq[3] = buDataStream;
   abuReq[4] = buVolNum;
   NCopyLoHi32(&abuReq[5], &luDirEntry);
   NCopyLoHi16(&abuReq[9], psuIterHnd);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REP_LEN;

   replyFrag[1].pAddr = abuRecLocks;
   replyFrag[1].uLen  = NCP_MAX_REC_LOCKS;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag, NCP_REP_FRAGS,
         replyFrag, NULL);
   if (lCode == 0)
   {
      nint i, n;

      NCopyLoHi16(psuIterHnd, &abuReply[0]);
      NCopyLoHi16(psuNumLocks, &abuReply[2]);

      if (pPhysRecLocks)
      {
         for (i = 0; i < (nint)*psuNumLocks; i++)
         {
            n = i*18;

            NCopyLoHi16(&pPhysRecLocks->suLoggedCount, &abuRecLocks[n]);
            NCopyLoHi16(&pPhysRecLocks->suShareableLockCount, &abuRecLocks[n+2]);
            NCopyLoHi32(&pPhysRecLocks->luRecStart, &abuRecLocks[n+4]);
            NCopyLoHi32(&pPhysRecLocks->luRecEnd, &abuRecLocks[n+8]);
            NCopyLoHi16(&pPhysRecLocks->suLogConnNum, &abuRecLocks[n+12]);
            NCopyLoHi16(&pPhysRecLocks->suTaskNum, &abuRecLocks[n+14]);
            NCopyLoHi16(&pPhysRecLocks->suLockType, &abuRecLocks[n+16]);
         }
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s238.c,v 1.7 1994/09/26 17:36:51 rebekah Exp $
*/
