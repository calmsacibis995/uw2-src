/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s27.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s27NSSetHugeInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s27NSSetHugeInfo
         (
            pNWAccess pAccess,
            nuint8   buNamSpc,
            nuint8   buVolNum,
            nuint32  luDirBase,
            nuint32  luHugeMask,
            pnuint8  pbuHugeStateInfoB16,
            nuint32  luHugeDataLen,
            pnuint8  pbuHugeDataB512,
            pnuint8  pbuNextHugeStateInfoB16,
            pnuint32 pHugeDataUsed,
         );

REMARKS: This is a NetWare 386 v3.11 call.  It is used to set huge NS information;
         however, it is used only when the NS has indicated that there is huge
         information available via the Query NS Information Format NCP (0x2222  87
         23).

         The HugeMask field the HugeStateInfo field, and the NextHugeStateInfo field
         are explained in more detail in the Introduction to Directory Services.

         The HugeDataUsed field contains the number of bytes consumed by the NS out
         of the total data sent to the NS.

ARGS: <> pAccess
      >  buNamSpc
      >  buVolNum
      >  luDirBase
      >  luHugeMask
      >  pbuHugeStateInfoB16
      >  luHugeDataLen
      >  pbuHugeDataB512
      <  pbuNextHugeStateInfoB16  (optional)
      <  pHugeDataUsed            (optional)

INCLUDE: ncp.h

RETURN:

SERVER:

CLIENT:  DOS OS2 WIN NT

SEE:     87 26  Get Huge NS Information
         87 23  Query NS Information Format

NCP:     87 27  Set Huge NS Information


CHANGES: 14 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s27NSSetHugeInfo
(
   pNWAccess pAccess,
   nuint8   buNamSpc,
   nuint8   buVolNum,
   nuint32  luDirBase,
   nuint32  luHugeMask,
   pnuint8  pbuHugeStateInfoB16,
   nuint32  luHugeDataLen,
   pnuint8  pbuHugeDataB512,
   pnuint8  pbuNextHugeStateInfoB16,
   pnuint32 pHugeDataUsed
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 27)
   #define REQ_LEN         ((nuint) 11)
   #define REPLY_LEN       ((nuint) 20)
   #define REQ_FRAGS       ((nuint) 4)
   #define REPLY_FRAGS     ((nuint) 1)

   nint32   lCode;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buNamSpc;
   abuReq[2] = buVolNum;
   NCopyLoHi32(&abuReq[3], &luDirBase);
   NCopyLoHi32(&abuReq[7], &luHugeMask);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbuHugeStateInfoB16;
   reqFrag[1].uLen  = (nuint) 16;

   reqFrag[2].pAddr = &luHugeDataLen;
   reqFrag[2].uLen  = (nuint) 1;

   reqFrag[3].pAddr = pbuHugeDataB512;
   reqFrag[3].uLen  = (nuint) 512;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REQ_LEN;


   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if(lCode == 0)
   {
      if(pbuNextHugeStateInfoB16 != NULL)
      {
         NWCMemMove(pbuNextHugeStateInfoB16, &abuReply[0], (nuint) 16);
      }

      if(pHugeDataUsed != NULL)
      {
         NCopyLoHi32(pHugeDataUsed, &abuReply[16]);
      }
   }
   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s27.c,v 1.7 1994/09/26 17:39:31 rebekah Exp $
*/
