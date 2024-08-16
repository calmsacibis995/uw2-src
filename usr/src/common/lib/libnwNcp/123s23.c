/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s23.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s23GetLANCustomCounters**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s23GetLANCustomCounters
         (
            pNWAccess                 pAccess,
            nuint32                  luBoardNum,
            nuint32                  luStartNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluCurCounters,
            pnuint8                  pbuCustomCountersB512
         )

REMARKS:

ARGS: <> pAccess
      >  luBoardNum
      >  luStartNum
      <  pVConsoleInfo (optional)
      <  psuReserved (optional)
      <  pluCurCounters
      <  pbuCustomCountersB512

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 23  LAN Custom Counters Information

CHANGES: 24 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s23GetLANCustomCounters
(
   pNWAccess                 pAccess,
   nuint32                  luBoardNum,
   nuint32                  luStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluCurCounters,
   pnuint8                  pbuCustomCountersB512
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 23)
   #define NCP_STRUCT_LEN  ((nuint16) 9)
   #define MAX_COUNTERS_LEN ((nuint) 512)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 12)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 2)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luBoardNum);
   NCopyLoHi32(&abuReq[7], &luStartNum);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = pbuCustomCountersB512;
   replyFrag[1].uLen  = MAX_COUNTERS_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      nuint   i;
      pnuint8 pbuIndex = pbuCustomCountersB512;

      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuReply);

      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuReply[6]);
      NCopyLoHi32(pluCurCounters, &abuReply[8]);

      for (i = 0; i < (nuint)*pluCurCounters; i++, pbuIndex += 5+pbuIndex[5])
      {
         nuint32 luTemp;

         NCopyLoHi32(&luTemp, &pbuIndex[0]);
         NCopy32(&pbuIndex[0], &luTemp);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s23.c,v 1.7 1994/09/26 17:32:19 rebekah Exp $
*/
