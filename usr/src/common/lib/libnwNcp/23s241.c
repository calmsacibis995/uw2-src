/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s241.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s241GetConnSems**********************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP23s241GetConnSems
         (
            pNWAccess pAccess,
            nuint16  suConnNum,
            pnuint16 psuIterHnd,
            pnuint16 psuNumSems,
            pnuint8  pConnSemsInfo,
         );

REMARKS:

ARGS: <> pAccess
      >  suConnNum
      <> psuIterHnd
      <  psuNumSems
      <  pConnSemsInfo

INCLUDE: ncpserve.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 225  Get Connection's Semaphores (old)

NCP:     23 241  Get Connection's Semaphores

CHANGES: 9 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s241GetConnSems
(
   pNWAccess pAccess,
   nuint16  suConnNum,
   pnuint16 psuIterHnd,
   pnuint16 psuNumSems,
   pnuint8  pConnSemsInfo
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        241)
   #define NCP_STRUCT_LEN     ((nuint16)         5)
   #define NCP_REQ_LEN        ((nuint)           7)
   #define NCP_REP_LEN        ((nuint)           4)
   #define NCP_REQ_FRAGS      ((nuint)           1)
   #define NCP_REP_FRAGS      ((nuint)           2)
   #define NCP_MAX_SEMS_INFO  ((nuint)         508)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];
   nuint16 suNCPLen, suTemp;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REP_FRAGS];
   nint i, position;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi16(&abuReq[3], &suConnNum);
   NCopyLoHi16(&abuReq[5], psuIterHnd);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REP_LEN;

   replyFrag[1].pAddr = pConnSemsInfo;
   replyFrag[1].uLen  = NCP_MAX_SEMS_INFO;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag, NCP_REP_FRAGS,
         replyFrag, NULL);
   if (lCode == 0)
   {
      NCopyLoHi16(psuIterHnd, &abuReply[0]);
      NCopyLoHi16(psuNumSems, &abuReply[2]);
      position = (nint) 0;
      for (i=0; i < (nint)*psuNumSems; i++)
      {
         NCopyLoHi16(&suTemp, &pConnSemsInfo[position]);
         NCopy16(&pConnSemsInfo[position], &suTemp);
         position += 2;
         NCopyLoHi16(&suTemp, &pConnSemsInfo[position]);
         NCopy16(&pConnSemsInfo[position], &suTemp);
         position += 2;
         NCopyLoHi16(&suTemp, &pConnSemsInfo[position]);
         NCopy16(&pConnSemsInfo[position], &suTemp);
         position += 2;
         position += pConnSemsInfo[position] + 1;
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s241.c,v 1.7 1994/09/26 17:36:57 rebekah Exp $
*/
