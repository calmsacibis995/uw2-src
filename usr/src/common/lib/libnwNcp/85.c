/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:85.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP85FileGetSparseBitMap*********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP85FileGetSparseBitMap
         (
            pNWAccess pAccess,
            pnuint8  pbuNWHandleB6,
            nuint32  luStartingOffset,
            pnuint32 pluAllocationBlockSize,
            pnuint8  pbuReservedB4,
            pnuint8  pbuBitMapB512
         );

REMARKS: Returns a bit map that shows which blocks contain data and which do
         not. There is one bit for each block in the sparse file. A one (1)
         means that there is data in that block and a zero (0) means that
         there is no data in that block.

ARGS: <> pAccess
       > pbuNWHandleB6
       > luStartingOffset
      <  pluAllocationBlockSize
      <  pbuReservedB4 (optional)
      <  pbuBitMapB512

INCLUDE: ncpfile.h

RETURN:  0x8801  INVALID_CONNECTION
         0x8988  INVALID_FILE_HANDLE

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     85 --  Get Sparse File Data Block Bit Map

CHANGES: 30 Aug 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP85FileGetSparseBitMap
(
   pNWAccess pAccess,
   pnuint8  pbuNWHandleB6,
   nuint32  luStartingOffset,
   pnuint32 pluAllocationBlockSize,
   pnuint8  pbuReservedB4,
   pnuint8  pbuBitMapB512
)
{
   #define NCP_FUNCTION    ((nuint) 85)
   #define NCP_STRUCT_LEN  ((nuint16) 10)
   #define NCP_REQ_LEN     ((nuint) 12)
   #define NCP_REPLY_LEN   ((nuint) 8)
   #define MAX_BITMAP_LEN  ((nuint) 512)
   #define NCP_REQ_FRAGS   ((nuint) 1)
   #define NCP_REPLY_FRAGS ((nuint) 2)

   nint32  lCode;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];
   nuint16 suNCPLen;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(abuReq, &suNCPLen);
   NWCMemMove(&abuReq[2], pbuNWHandleB6, 6);
   NCopyLoHi32(&abuReq[8], &luStartingOffset);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   replyFrag[1].pAddr = pbuBitMapB512;
   replyFrag[1].uLen  = MAX_BITMAP_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);

   if (lCode == 0)
   {
      NCopyLoHi32(pluAllocationBlockSize, &abuReply[0]);

      if (pbuReservedB4)
         NWCMemMove(pbuReservedB4, &abuReply[4], 4);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/85.c,v 1.7 1994/09/26 17:39:05 rebekah Exp $
*/
