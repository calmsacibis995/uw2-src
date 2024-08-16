/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s61.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s61GetServerSetCategory**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s61GetServerSetCategory
         (
            pNWAccess                 pAccess,
            nuint32                  luStartNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluNextStartNum,
            pnuint32                 pluNumCatagories,
            pnuint8                  pbuNameLen,
            pnstr8                   pbstrCategoryName484,
         )

REMARKS:

ARGS: <> pAccess
      >  luStartNum
      <  pVConsoleInfo (optional)
      <  psuReserved (optional)
      <  pluNextStartNum (optional)
      <  pluNumCatagories
      <  pbuNameLen
      <  pbstrCategoryNameB484

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 61  Get Server Set Categories

CHANGES: 23 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s61GetServerSetCategory
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNextStartNum,
   pnuint32                 pluNumCatagories,
   pnuint8                  pbuNameLen,
   pnstr8                   pbstrCategoryNameB496
)
{
   #define NCP_FUNCTION       ((nuint) 123)
   #define NCP_SUBFUNCTION    ((nuint8) 61)
   #define NCP_STRUCT_LEN     ((nuint16) 5)
   #define MAX_CATEGORY_NAMES ((nuint) 496)
   #define NCP_REQ_LEN        ((nuint) 7)
   #define NCP_REPLY_LEN      ((nuint) 17)
   #define NCP_REQ_FRAGS      ((nuint) 1)
   #define NCP_REPLY_FRAGS    ((nuint) 2)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luStartNum);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   replyFrag[1].pAddr = pbstrCategoryNameB496;
   replyFrag[1].uLen  = MAX_CATEGORY_NAMES;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuReply);

      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuReply[6]);

      if (pluNextStartNum)
         NCopyLoHi32(pluNextStartNum, &abuReply[8]);
      NCopyLoHi32(pluNumCatagories, &abuReply[12]);
      *pbuNameLen = abuReply[16];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s61.c,v 1.7 1994/09/26 17:33:00 rebekah Exp $
*/
