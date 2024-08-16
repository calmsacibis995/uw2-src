/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s8.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s8GetCPUInfo*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s8GetCPUInfo
         (
            pNWAccess                 pAccess,
            nuint32                  luCPUNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluNumCPUs,
            pNWNCPFSECPUInfo         pCPUInfo,
            pnstr8                   pbstrCPUString,
            pnstr8                   pbstrCoprocessorPresentString,
            pnstr8                   pbstrBusString,
         )

REMARKS:

ARGS: <> pAccess
      >  luCPUNum
      <  pVConsoleInfo (optional)
      <  psuReserved (optional)
      <  pluNumCPUs (optional)
      <  pCPUInfo (optional)
      <  pbstrCPUString (optional)
      <  pbstrCoprocessorPresentString (optional)
      <  pbstrBusString (optional)

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 08  Get CPU Info

CHANGES: 24 Sep 1993 - written - lwiltban
         24 Sep 1993 - redone - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s8GetCPUInfo
(
   pNWAccess                 pAccess,
   nuint32                  luCPUNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNumCPUs,
   pNWNCPFSECPUInfo         pCPUInfo,
   pnstr8                   pbstrCPUString,
   pnstr8                   pbstrCoprocessorPresentString,
   pnstr8                   pbstrBusString
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 8)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define MAX_STRS_LEN    ((nuint) (NWNCP_FSE_MAX_CPU_STRING +\
                                     NWNCP_FSE_MAX_CPU_STRING +\
                                     NWNCP_FSE_MAX_BUS_STRING))
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 40)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 2)

   nint32   lCode;
   nuint8  abuReq[REQ_LEN],
           abuReply[REPLY_LEN],
           abuStrings[MAX_STRS_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luCPUNum);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = abuStrings;
   replyFrag[1].uLen  = MAX_STRS_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      nuint  iuLen;
      pnstr8 pbstrIndex;

      NWNCP_UNPACK_VCONS_INF( pVConsoleInfo, abuReply );

      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuReply[6]);
      if (pluNumCPUs)
         NCopyLoHi32(pluNumCPUs, &abuReply[8]);

      if (pCPUInfo)
      {
         NCopyLoHi32(&pCPUInfo->luWeOwnThePageTablesFlag, &abuReply[12]);
         NCopyLoHi32(&pCPUInfo->luCPUTypeFlag, &abuReply[16]);
         NCopyLoHi32(&pCPUInfo->luNumericCoprocessorFlag, &abuReply[20]);
         NCopyLoHi32(&pCPUInfo->luBusTypeFlag, &abuReply[24]);
         NCopyLoHi32(&pCPUInfo->luIOEngineFlag, &abuReply[28]);
         NCopyLoHi32(&pCPUInfo->luFSEngineFlag, &abuReply[32]);
         NCopyLoHi32(&pCPUInfo->luNonDedicatedFlag, &abuReply[36]);
      }

      pbstrIndex = (pnstr8) abuStrings;
      iuLen = NWCStrLen(pbstrIndex) + 1;
      if (pbstrCPUString)
         NWCMemMove(pbstrCPUString, pbstrIndex, iuLen);
      pbstrIndex += iuLen;
      iuLen = NWCStrLen(pbstrIndex) + 1;
      if (pbstrCoprocessorPresentString)
         NWCMemMove(pbstrCoprocessorPresentString, pbstrIndex, iuLen);
      pbstrIndex += iuLen;
      iuLen = NWCStrLen(pbstrIndex) + 1;
      if (pbstrBusString)
         NWCMemMove(pbstrBusString, pbstrIndex, iuLen);
      pbstrIndex += iuLen;
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s8.c,v 1.7 1994/09/26 17:33:03 rebekah Exp $
*/
