/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s26.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s26NSGetHugeInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s26NSGetHugeInfo
         (
            pNWAccess pAccess,
            nuint8   buNamSpc,
            nuint8   buVolNum,
            nuint32  luDirBase,
            nuint32  luHugeMask,
            pnuint8  pbuHugeStateInfoB16,
            pnuint8  pbuNxtHugeStateInfoB16,
            pnuint32 pluHugeDataLen,
            pnuint8  pbuHugeDataB500
         );

REMARKS: This is a NetWare 386 v3.11 call.  It is used to get huge NS information;
         however, it is used only when the NS has indicated that there is huge
         information available via the Query NS Information Format NCP (0x2222  87
         23).

         The Huge State Info field contains information used to help the NS in
         transferring Huge NS info across the wire.  On the initial call, all 16
         bytes are set to zero, but subsquent calls will set the HugeStateInfo field
         from the NextStateInfo field in the reply.

ARGS: <> pAccess
       > buNamSpc
       > buVolNum
       > luDirBase
       > luHugeMask
       > pbuHugeStateInfoB16
      <  pbuNxtHugeStateInfoB16
      <  pluHugeDataLen
      <  pbuHugeDataB500

INCLUDE: ncpfile.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     87 26  Get Huge NS Information

CHANGES: 4 Oct 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s26NSGetHugeInfo
(
   pNWAccess pAccess,
   nuint8   buNamSpc,
   nuint8   buVolNum,
   nuint32  luDirBase,
   nuint32  luHugeMask,
   pnuint8  pbuHugeStateInfoB16,
   pnuint8  pbuNxtHugeStateInfoB16,
   pnuint32 pluHugeDataLen,
   pnuint8  pbuHugeDataB500
)
{
   #define NCP_FUNCTION    ((nuint)    87)
   #define NCP_SUBFUNCTION ((nuint8)   26)
   #define REQ_FRAGS       ((nuint)    2)
   #define REPLY_FRAGS     ((nuint)    3)
   #define REQ_LEN         ((nuint)    11)
   #define REPLY_LEN       ((nuint)    500)

   nint32  lCode;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint8  abuReq[REQ_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buNamSpc;
   abuReq[2] = buVolNum;
   NCopyLoHi32(&abuReq[3], &luDirBase);
   NCopyLoHi32(&abuReq[7], &luHugeMask);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbuHugeStateInfoB16;
   reqFrag[1].uLen  = 16;

   replyFrag[0].pAddr = pbuNxtHugeStateInfoB16;
   replyFrag[0].uLen  = 16;

   replyFrag[1].pAddr = pluHugeDataLen;
   replyFrag[1].uLen  = 4;

   replyFrag[2].pAddr = pbuHugeDataB500;
   replyFrag[2].uLen  = REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);

   if(lCode == 0)
   {
      *pluHugeDataLen = NSwapLoHi32(*pluHugeDataLen);
   }

   return((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s26.c,v 1.7 1994/09/26 17:39:30 rebekah Exp $
*/
