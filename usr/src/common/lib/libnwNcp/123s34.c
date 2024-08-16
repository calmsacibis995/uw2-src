/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s34.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s34GetVolInfoByLevel**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s34GetVolInfoByLevel
         (
            pNWAccess                 pAccess,
            nuint32                  luVolNum,
            nuint32                  luInfoLevel,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluInfoLevel,
            pNWNCPFSEVolInfo         pInfo,
         )

REMARKS:

ARGS: <> pAccess
      >  luVolNum
      >  luInfoLevel
      <  pVConsoleInfo (optional)
      <  psuReserved (optional)
      <  pluInfoLevel (optional)
      <  pInfo

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 34  Get Volume Information By Level

CHANGES: 23 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s34GetVolInfoByLevel
(
   pNWAccess                 pAccess,
   nuint32                  luVolNum,
   nuint32                  luInfoLevel, /* either 1 or 2 */
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluInfoLevel,
   pNWNCPFSEVolInfo         pInfo
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 34)
   #define NCP_STRUCT_LEN  ((nuint16) 9)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 144)

   nint32   lCode;
   nuint8 abuReq[11], abuReply[144];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luVolNum);
   NCopyLoHi32(&abuReq[7], &luInfoLevel);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuReply, NCP_REPLY_LEN, NULL);
   if (lCode == 0)
   {
      nuint32 luTemp;

      NWNCP_UNPACK_VCONS_INF( pVConsoleInfo, abuReply );

      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuReply[6]);
      NCopyLoHi32(&luTemp, &abuReply[8]);
      if (pluInfoLevel)
         *pluInfoLevel = luTemp;
      switch (luTemp)
      {
         case 1:
            NCopyLoHi32(&pInfo->Info1.luVolType, &abuReply[12]);
            NCopyLoHi32(&pInfo->Info1.luStatusFlag, &abuReply[16]);
            NCopyLoHi32(&pInfo->Info1.luSectorSize, &abuReply[20]);
            NCopyLoHi32(&pInfo->Info1.luSectorsPerCluster, &abuReply[24]);
            NCopyLoHi32(&pInfo->Info1.luVolSizeInClusters, &abuReply[28]);
            NCopyLoHi32(&pInfo->Info1.luFreedClusters, &abuReply[32]);
            NCopyLoHi32(&pInfo->Info1.luSubAllocFreeableClusters, &abuReply[36]);
            NCopyLoHi32(&pInfo->Info1.luFreeableLimboSectors, &abuReply[40]);
            NCopyLoHi32(&pInfo->Info1.luNonFreeableLimboSectors, &abuReply[44]);
            NCopyLoHi32(&pInfo->Info1.luNonFreeableSubAllocSectors, &abuReply[48]);
            NCopyLoHi32(&pInfo->Info1.luNotUsableSubAllocSectors, &abuReply[52]);
            NCopyLoHi32(&pInfo->Info1.luSubAllocClusters, &abuReply[56]);
            NCopyLoHi32(&pInfo->Info1.luDataStreams, &abuReply[60]);
            NCopyLoHi32(&pInfo->Info1.luLimboDataStreams, &abuReply[64]);
            NCopyLoHi32(&pInfo->Info1.luOldestDeletedFileAgeInTicks, &abuReply[68]);
            NCopyLoHi32(&pInfo->Info1.luCompressedDataStreams, &abuReply[72]);
            NCopyLoHi32(&pInfo->Info1.luCompressedLimboDataStreams, &abuReply[76]);
            NCopyLoHi32(&pInfo->Info1.luUncompressedDataStreams, &abuReply[80]);
            NCopyLoHi32(&pInfo->Info1.luPrecompressedSectors, &abuReply[84]);
            NCopyLoHi32(&pInfo->Info1.luCompressedSectors, &abuReply[88]);
            NCopyLoHi32(&pInfo->Info1.luMigratedFiles, &abuReply[92]);
            NCopyLoHi32(&pInfo->Info1.luMigratedSectors, &abuReply[96]);
            NCopyLoHi32(&pInfo->Info1.luClustersUsedByFAT, &abuReply[100]);
            NCopyLoHi32(&pInfo->Info1.luClustersUsedByDirs, &abuReply[104]);
            NCopyLoHi32(&pInfo->Info1.luClustersUsedByExtDirs, &abuReply[108]);
            NCopyLoHi32(&pInfo->Info1.luTotalDirEntries, &abuReply[112]);
            NCopyLoHi32(&pInfo->Info1.luUnusedDirEntries, &abuReply[116]);
            NCopyLoHi32(&pInfo->Info1.luTotalExtDirEntries, &abuReply[120]);
            NCopyLoHi32(&pInfo->Info1.luUnusedExtDirEntries, &abuReply[124]);
            NCopyLoHi32(&pInfo->Info1.luExtAttrsDefined, &abuReply[128]);
            NCopyLoHi32(&pInfo->Info1.luExtAttrExtantsUsed, &abuReply[132]);
            NCopyLoHi32(&pInfo->Info1.luDSObjID, &abuReply[136]);
            NCopyLoHi32(&pInfo->Info1.luVolLastModifiedDateAndTime, &abuReply[140]);
            break;
         case 2:
            NCopyLoHi32(&pInfo->Info2.luVolActiveCnt, &abuReply[12]);
            NCopyLoHi32(&pInfo->Info2.luVolUsageCnt, &abuReply[16]);
            NCopyLoHi32(&pInfo->Info2.luMACRootIDs, &abuReply[20]);
            NCopyLoHi32(&pInfo->Info2.luVolLastModifiedDataAndTime, &abuReply[24]);
            NCopyLoHi32(&pInfo->Info2.luVolRefCnt, &abuReply[28]);
            NCopyLoHi32(&pInfo->Info2.luCompressionLowerLimit, &abuReply[32]);
            NCopyLoHi32(&pInfo->Info2.luOutstandingIOs, &abuReply[36]);
            NCopyLoHi32(&pInfo->Info2.luOutstandingCompressionIOs, &abuReply[40]);
            NCopyLoHi32(&pInfo->Info2.luCompressionIOLimit, &abuReply[44]);
            break;
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s34.c,v 1.7 1994/09/26 17:32:29 rebekah Exp $
*/
