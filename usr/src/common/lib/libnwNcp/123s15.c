/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s15.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s15GetNLMResTagList**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s15GetNLMResTagList
         (
            pNWAccess                 pAccess,
            nuint32                  luNLMNum,
            nuint32                  luNLMStartNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluTotalTags,
            pnuint32                 pluCurTags,
            pnuint8                  pbuResources,
         )

REMARKS:

ARGS: <> pAccess
      >  luNLMNum
      >  luNLMStartNum
      <  pVConsoleInfo
      <  psuReserved
      <  pluTotalTags
      <  pluCurTags
      <  pbuResources

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 15  Get NLM's Resource Tag List

CHANGES: 23 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s15GetNLMResTagList
(
   pNWAccess                 pAccess,
   nuint32                  luNLMNum,
   nuint32                  luNLMStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluTotalTags,
   pnuint32                 pluCurTags,
   pnuint8                  pbuResources
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 15)
   #define NCP_STRUCT_LEN  ((nuint16) 9)
   #define MAX_RES_LEN     ((nuint) 512)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 16)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 2)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuRep[REPLY_LEN], buNameLen;
   nint i=0;
   nuint16 suNCPLen;
   nuint32 luRTagNum, luRSignature, luRCount, luStructNum=0;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luNLMNum);
   NCopyLoHi32(&abuReq[7], &luNLMStartNum);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = abuRep;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = pbuResources;
   replyFrag[1].uLen  = MAX_RES_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuRep);
      NCopyLoHi16(psuReserved,&abuRep[6]);
      NCopyLoHi32(pluTotalTags,&abuRep[8]);
      NCopyLoHi32(pluCurTags,&abuRep[12]);

      while (luStructNum++ < *pluCurTags)
      {
         NCopyLoHi32(&luRTagNum, &pbuResources[i]);
         NCopyLoHi32(&luRSignature, &pbuResources[i+4]);
         NCopyLoHi32(&luRCount, &pbuResources[i+8]);
         NCopy32(&pbuResources[i], &luRTagNum);
         NCopy32(&pbuResources[i+4], &luRSignature);
         NCopy32(&pbuResources[i+8], &luRCount);

         buNameLen = (nuint8) NWCStrLen(&pbuResources[i+12]);
         i += (nint)(buNameLen + 13);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s15.c,v 1.7 1994/09/26 17:32:11 rebekah Exp $
*/
