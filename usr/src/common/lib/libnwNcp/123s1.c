/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s1.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s1GetCacheInfo*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s1GetCacheInfo
         (
            pNWAccess                 pAccess,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pNWNCPFSECacheCounters   pCacheCounters,
            pNWNCPFSEMemoryCounters  pMemoryCounters,
            pNWNCPFSETrendCounters   pTrendCounters,
            pNWNCPFSECacheInfo       pCacheInfo,
         )

REMARKS:

ARGS: <> pAccess
      <  pVConsoleInfo
      <  psuReserved (optional)
      <  pCacheCounters (optional)
      <  pMemoryCounters (optional)
      <  pTrendCounters (optional)
      <  pCacheInfo (optional)

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 01  Get Cache Information

CHANGES: 23 Sep 1993 - written - lwiltban
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s1GetCacheInfo
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSECacheCounters   pCacheCounters,
   pNWNCPFSEMemoryCounters  pMemoryCounters,
   pNWNCPFSETrendCounters   pTrendCounters,
   pNWNCPFSECacheInfo       pCacheInfo
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 1)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 232)

   nint32   lCode;
   nuint8  abuReq[REQ_LEN],
           abuReply[REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF( pVConsoleInfo, abuReply );

      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuReply[6]);

      if (pCacheCounters)
      {
         NCopyLoHi32(&pCacheCounters->luReadExistingBlockCnt, &abuReply[8]);
         NCopyLoHi32(&pCacheCounters->luReadExistingWriteWaitCnt, &abuReply[12]);
         NCopyLoHi32(&pCacheCounters->luReadExistingPartialReadCnt, &abuReply[16]);
         NCopyLoHi32(&pCacheCounters->luReadExistingReadErrorCnt, &abuReply[20]);
         NCopyLoHi32(&pCacheCounters->luWriteBlockCnt, &abuReply[24]);
         NCopyLoHi32(&pCacheCounters->luWriteEntireBlockCnt, &abuReply[28]);
         NCopyLoHi32(&pCacheCounters->luIntDiskGetCnt, &abuReply[32]);
         NCopyLoHi32(&pCacheCounters->luIntDiskGetNeedToAllocCnt, &abuReply[36]);
         NCopyLoHi32(&pCacheCounters->luIntDiskGetSomeoneBeatMeCnt, &abuReply[40]);
         NCopyLoHi32(&pCacheCounters->luIntDiskGetPartialReadCnt, &abuReply[44]);
         NCopyLoHi32(&pCacheCounters->luIntDiskGetReadErrorCnt, &abuReply[48]);
         NCopyLoHi32(&pCacheCounters->luAsyncIntDiskGetCnt, &abuReply[52]);
         NCopyLoHi32(&pCacheCounters->luAsyncIntDiskGetNeedToAlloc, &abuReply[56]);
         NCopyLoHi32(&pCacheCounters->luAsyncIntDiskGetSomeoneBeatMe, &abuReply[60]);
         NCopyLoHi32(&pCacheCounters->luErrDoingAsyncReadCnt, &abuReply[64]);
         NCopyLoHi32(&pCacheCounters->luIntDiskGetNoReadCnt, &abuReply[68]);
         NCopyLoHi32(&pCacheCounters->luIntDiskGetNoReadAllocCnt, &abuReply[72]);
         NCopyLoHi32(&pCacheCounters->luIntDiskGetNoReadSomeoneBeat, &abuReply[76]);
         NCopyLoHi32(&pCacheCounters->luIntDiskWriteCnt, &abuReply[80]);
         NCopyLoHi32(&pCacheCounters->luIntDiskWriteAllocCnt, &abuReply[84]);
         NCopyLoHi32(&pCacheCounters->luIntDiskWriteSomeoneBeatMe, &abuReply[88]);
         NCopyLoHi32(&pCacheCounters->luWriteErrorCnt, &abuReply[92]);
         NCopyLoHi32(&pCacheCounters->luWaitOnSemaphoreCnt, &abuReply[96]);
         NCopyLoHi32(&pCacheCounters->luAllocBlockHadToWaitForBeat, &abuReply[100]);
         NCopyLoHi32(&pCacheCounters->luAllocBlockCnt, &abuReply[104]);
         NCopyLoHi32(&pCacheCounters->luAllocBlockIHadToWaitCnt, &abuReply[108]);
      }

      if (pMemoryCounters)
      {
         NCopyLoHi32(&pMemoryCounters->luOriginalNumCacheBuffs, &abuReply[112]);
         NCopyLoHi32(&pMemoryCounters->luCurNumCacheBuffs, &abuReply[116]);
         NCopyLoHi32(&pMemoryCounters->luCacheDirtyBlockThreshold, &abuReply[120]);
         NCopyLoHi32(&pMemoryCounters->luWaitNodeCnt, &abuReply[124]);
         NCopyLoHi32(&pMemoryCounters->luWaitNodeAllocFailureCnt, &abuReply[128]);
         NCopyLoHi32(&pMemoryCounters->luMoveCacheNodeCnt, &abuReply[132]);
         NCopyLoHi32(&pMemoryCounters->luMoveCacheNodeFromAvailCnt, &abuReply[136]);
         NCopyLoHi32(&pMemoryCounters->luAccelerateCacheNodeWriteCnt, &abuReply[140]);
         NCopyLoHi32(&pMemoryCounters->luRemoveCacheNodeCnt, &abuReply[144]);
         NCopyLoHi32(&pMemoryCounters->luRemoveCacheNodeFromAvailCnt, &abuReply[148]);
      }

      if (pTrendCounters)
      {
         NCopyLoHi32(&pTrendCounters->luNumCacheChecks, &abuReply[152]);
         NCopyLoHi32(&pTrendCounters->luNumCacheHits, &abuReply[156]);
         NCopyLoHi32(&pTrendCounters->luNumCacheDirtyChecks, &abuReply[160]);
         NCopyLoHi32(&pTrendCounters->luNumCacheDirtyHits, &abuReply[164]);
         NCopyLoHi32(&pTrendCounters->luCacheUsedWhileChecking, &abuReply[168]);
         NCopyLoHi32(&pTrendCounters->luWaitTillDirtyBlksDecreaseCnt, &abuReply[172]);
         NCopyLoHi32(&pTrendCounters->luAllocBlockFromAvailableCnt, &abuReply[176]);
         NCopyLoHi32(&pTrendCounters->luAllocBlockFromLRUCnt, &abuReply[180]);
         NCopyLoHi32(&pTrendCounters->luAllocBlockAlreadyWaiting, &abuReply[184]);
         NCopyLoHi32(&pTrendCounters->luLRUSittingTime, &abuReply[188]);
      }

      if (pCacheInfo)
      {
         NCopyLoHi32(&pCacheInfo->luMaxByteCnt, &abuReply[192]);
         NCopyLoHi32(&pCacheInfo->luMinNumCacheBuffs, &abuReply[196]);
         NCopyLoHi32(&pCacheInfo->luMinCacheReportThreshold, &abuReply[200]);
         NCopyLoHi32(&pCacheInfo->luAllocWaitingCnt, &abuReply[204]);
         NCopyLoHi32(&pCacheInfo->luNDirtyBlocks, &abuReply[208]);
         NCopyLoHi32(&pCacheInfo->luCacheDirtyWaitTime, &abuReply[212]);
         NCopyLoHi32(&pCacheInfo->luCacheMaxConcurrentWrites, &abuReply[216]);
         NCopyLoHi32(&pCacheInfo->luMaxDirtyTime, &abuReply[220]);
         NCopyLoHi32(&pCacheInfo->luNumDirCacheBuffs, &abuReply[224]);
         NCopyLoHi32(&pCacheInfo->luCacheByteToBlockShiftFactor, &abuReply[228]);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s1.c,v 1.7 1994/09/26 17:32:03 rebekah Exp $
*/
