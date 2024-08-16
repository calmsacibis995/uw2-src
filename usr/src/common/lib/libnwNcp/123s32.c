/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s32.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s32GetMediaChildList***********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s32GetMediaChildList
         (
            pNWAccess                 pAccess,
            nuint32                  luStartNum,
            nuint32                  luObjType,
            nuint32                  luParentNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluNextStartNum,
            pnuint32                 pluChildCnt,
            pnuint32                 pluChildrenB128,
         );

REMARKS:

ARGS: <> pAccess
      >  luStartNum
      >  luObjType
      >  luParentNum
      <  pVConsoleInfo (optional)
      <  psuReserved (optional)
      <  pluNextStartNum
      <  pluChildCount
      <  pluChildrenB128

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 32  GetMedia Manager Object Children's List

CHANGES: 22 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s32GetMediaChildList
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   nuint32                  luObjType,
   nuint32                  luParentNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNextStartNum,
   pnuint32                 pluChildCnt,
   pnuint32                 pluChildrenB128
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 32)
   #define NCP_STRUCT_LEN  ((nuint16) 13)
   #define MAX_CHILDREN    ((nuint) 128)
   #define NCP_REQ_LEN     ((nuint) 15)
   #define NCP_REPLY_LEN   ((nuint) 16)
   #define NCP_REQ_FRAGS   ((nuint) 1)
   #define NCP_REPLY_FRAGS ((nuint) 2)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luStartNum);
   NCopyLoHi32(&abuReq[7], &luObjType);
   NCopyLoHi32(&abuReq[11], &luParentNum);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   replyFrag[1].pAddr = pluChildrenB128;
   replyFrag[1].uLen  = MAX_CHILDREN * 4;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      nint i;

      NWNCP_UNPACK_VCONS_INF( pVConsoleInfo, abuReply );

      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuReply[6]);

      NCopyLoHi32(pluNextStartNum, &abuReply[8]);
      NCopyLoHi32(pluChildCnt, &abuReply[12]);

      for (i = 0; i < (nint)*pluChildCnt && i < 128; i++)
      {
         nuint32 luTemp;

         NCopyLoHi32(&luTemp, &pluChildrenB128[i]);
         NCopy32(&pluChildrenB128[i], &luTemp);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s32.c,v 1.7 1994/09/26 17:32:26 rebekah Exp $
*/
