/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s22.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s22GetLANCommonCounters**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s22GetLANCommonCounters
         (
            pNWAccess                 pAccess,
            nuint32                  luBoardNum,
            nuint32                  luBlockNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint8                  pbuStatMajorVer,
            pnuint8                  pbuStatMinorVer,
            pnuint32                 pluTotalCounters,
            pnuint32                 pluTotalBlocks,
            pnuint32                 pluCustomCounters,
            pnuint32                 pluNextBlock,
            pNWNCPFSECommonCounters  pCounters,
         )

REMARKS:

ARGS: <> pAccess
      >  luBoardNum
      >  luBlockNum
      <  pVConsoleInfo (optional)
      <  pbuStatMajorVer (optional)
      <  pbuStatMinorVer (optional)
      <  pluTotalCounters (optional)
      <  pluTotalBlocks (optional)
      <  pluCustomCounters
      <  pluNextBlock (optional)
      <  pCounters

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 22  LAN Common Counters Information

CHANGES: 24 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s22GetLANCommonCounters
(
   pNWAccess                 pAccess,
   nuint32                  luBoardNum,
   nuint32                  luBlockNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint8                  pbuStatMajorVer,
   pnuint8                  pbuStatMinorVer,
   pnuint32                 pluTotalCounters,
   pnuint32                 pluTotalBlocks,
   pnuint32                 pluCustomCounters,
   pnuint32                 pluNextBlock,
   pNWNCPFSECommonCounters  pCounters
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 22)
   #define NCP_STRUCT_LEN  ((nuint16) 9)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 130)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luBoardNum);
   NCopyLoHi32(&abuReq[7], &luBlockNum);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      nint i;

      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuReply);

      if (pbuStatMajorVer)
         *pbuStatMajorVer = abuReply[6];
      if (pbuStatMinorVer)
         *pbuStatMinorVer = abuReply[7];

      if (pluTotalCounters)
         NCopyLoHi32(pluTotalCounters, &abuReply[8]);
      if (pluTotalBlocks)
         NCopyLoHi32(pluTotalBlocks, &abuReply[12]);

      NCopyLoHi32(pluCustomCounters, &abuReply[16]);

      if (pluNextBlock)
         NCopyLoHi32(pluNextBlock, &abuReply[20]);

      NCopyLoHi32(&pCounters->luNotSupportedMask,    &abuReply[24]);
      NCopyLoHi32(&pCounters->luTotalTxPacketCnt,    &abuReply[28]);
      NCopyLoHi32(&pCounters->luTotalRxPacketCnt,    &abuReply[32]);
      NCopyLoHi32(&pCounters->luNoECBAvailableCnt,   &abuReply[36]);
      NCopyLoHi32(&pCounters->luPacketTxTooBigCnt,   &abuReply[40]);
      NCopyLoHi32(&pCounters->luPacketTxTooSmallCnt, &abuReply[44]);
      NCopyLoHi32(&pCounters->luPacketRxOverflowCnt, &abuReply[48]);
      NCopyLoHi32(&pCounters->luPacketRxTooBigCnt,   &abuReply[52]);
      NCopyLoHi32(&pCounters->luPacketRxTooSmallCnt, &abuReply[56]);
      NCopyLoHi32(&pCounters->luPacketTxMiscErrCnt,  &abuReply[60]);
      NCopyLoHi32(&pCounters->luPacketRxMiscErrCnt,  &abuReply[64]);
      NCopyLoHi32(&pCounters->luRetryTxCnt,          &abuReply[68]);
      NCopyLoHi32(&pCounters->luChecksumErrCnt,      &abuReply[72]);
      NCopyLoHi32(&pCounters->luHardwareRxMismatchCnt, &abuReply[76]);
      for (i = 0; i < 50; i++)
         NCopyLoHi32(&pCounters->aluReserved[i], &abuReply[80+i*4]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s22.c,v 1.7 1994/09/26 17:32:17 rebekah Exp $
*/
