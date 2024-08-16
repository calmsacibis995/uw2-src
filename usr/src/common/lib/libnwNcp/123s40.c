/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s40.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s40GetActiveProtoStacks**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s40GetActiveProtoStacks
         (
            pNWAccess                 pAccess,
            nuint32                  luStartNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluMaxNumStacks,
            pnuint32                 pluStackCnt,
            pnuint32                 pluNextStartNum,
            pNWNCPFSEActiveStackList pListB24,
         )

REMARKS:

ARGS: <> pAccess
      >  luStartNum
      <  pVConsoleInfo (optional)
      <  psuReserved (optional)
      <  pluMaxNumStacks (optional)
      <  pluStackCnt
      <  pluNextStartNum
      <  pListB24

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 40  Active Protocol Stacks

CHANGES: 23 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s40GetActiveProtoStacks
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluMaxNumStacks,
   pnuint32                 pluStackCnt,
   pnuint32                 pluNextStartNum,
   pNWNCPFSEStackInfo       pListB24
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 40)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 500)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luStartNum);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuReply, NCP_REPLY_LEN, NULL);
   if (lCode == 0)
   {
      nint i;

      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuReply);

      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuReply[6]);
      if (pluMaxNumStacks)
         NCopyLoHi32(pluMaxNumStacks, &abuReply[8]);
      NCopyLoHi32(pluStackCnt, &abuReply[12]);
      NCopyLoHi32(pluNextStartNum, &abuReply[16]);

      for (i = 0; i < (nint) *pluStackCnt; i++)
      {
         NCopyLoHi32(&pListB24[i].luStackNum, &abuReply[20+i*20]);
         NWCMemMove(pListB24[i].abuStackShortName, &abuReply[24+i*20],
            (nuint) 16);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s40.c,v 1.7 1994/09/26 17:32:31 rebekah Exp $
*/
