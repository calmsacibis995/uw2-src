/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s51.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP22s51GetExtVolInfo*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s51GetExtVolInfo
         (
            pNWAccess pAccess,
            nuint8   buVolNum,
            pnuint8  pbuReserved1B2,
            pNWNCPExtVolInfo pExtVolInfo,
            pnuint8  pbuReserved2B16,
         )

REMARKS: This NCP will get full volume information about a mounted volume.

         Volume Info

         struct  NWNCPExtVolInfo
         {
            nuint32 luVolType;
            nuint32 luStatusFlag;
            nuint32 luSectorSize;
            nuint32 luSectorsPerCluster;
            nuint32 luVolSizeInClusters;
            nuint32 luFreeClusters;
            nuint32 luSubAllocFreeableClusters;
            nuint32 luFreeableLimboSectors;
            nuint32 luNonfreeableLimboSectors;
            nuint32 luAvailSubAllocSectors;             non freeable
            nuint32 luNonuseableSubAllocSectors;
            nuint32 luSubAllocClusters;
            nuint32 luNumDataStreams;
            nuint32 luNumLimboDataStreams;
            nuint32 luOldestDelFileAgeInTicks;
            nuint32 luNumCompDataStreams;
            nuint32 luNumCompLimboDataStreams;
            nuint32 luNumNoncompDataStreams;
            nuint32 luPrecompSectors;
            nuint32 luCompSectors;
            nuint32 luNumMigratedDataStreams;
            nuint32 luMigratedSectors;
            nuint32 luClustersUsedByFAT;
            nuint32 luClustersUsedByDirs;
            nuint32 luClustersUsedByExtDirs;
            nuint32 luTotalDirEntries;
            nuint32 luUnusedDirEntries;
            nuint32 luTotalExtDirExtants;
            nuint32 luUnusedExtDirExtants;
            nuint32 luExtAttrsDefined;
            nuint32 luExtAttrExtantsUsed;
            nuint32 luDirServicesObjID;
            nuint32 luVolLastModifiedDateAndTime;
         }

ARGS: <> pAccess,
      >  buVolNum,
      <  pbuReserved1B2;
      <  VolInfo pExtVolInfo
      <  pbuReserved2B16;

INCLUDE: ncpserve.h

RETURN:  0x0000  Successful
         0x897E  Invalid Length
         0x8998  Disk Map Error (Invalid volume)

SERVER:  4.0

CLIENT:

SEE:

NCP:     22 51  Get Extended Volume Information

CHANGES: 22 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s51GetExtVolInfo
(
   pNWAccess          pAccess,
   nuint8            buVolNum,
   pnuint8           pbuReserved1B2,
   pNWNCPExtVolInfo  pExtVolInfo,
   pnuint8           pbuReserved2B16
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 51)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define MAX_VOLINFO_LEN ((nuint) 132)
   #define NCP_REQ_LEN     ((nuint) 7)
   #define NCP_REPLY_LEN0  ((nuint) 2)
   #define NCP_REPLY_LEN2  ((nuint) 16)
   #define NCP_REQ_FRAGS   ((nuint) 1)
   #define NCP_REPLY_FRAGS ((nuint) 3)

   nint32   lCode;
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];
   nuint8  abuReq[NCP_REQ_LEN], abuReply[MAX_VOLINFO_LEN];
   nuint8  abuBucket2[NCP_REPLY_LEN0], abuBucket16[NCP_REPLY_LEN2];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &buVolNum);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = pbuReserved1B2 ? pbuReserved1B2 : abuBucket2;
   replyFrag[0].uLen  = NCP_REPLY_LEN0;

   replyFrag[1].pAddr = abuReply;
   replyFrag[1].uLen  = MAX_VOLINFO_LEN;

   replyFrag[2].pAddr = pbuReserved2B16 ? pbuReserved2B16 : abuBucket16;
   replyFrag[2].uLen  = NCP_REPLY_LEN2;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      NCopyLoHi32(&pExtVolInfo->luVolType,                     &abuReply[0]);
      NCopyLoHi32(&pExtVolInfo->luStatusFlag,                  &abuReply[4]);
      NCopyLoHi32(&pExtVolInfo->luSectorSize,                  &abuReply[8]);
      NCopyLoHi32(&pExtVolInfo->luSectorsPerCluster,           &abuReply[12]);
      NCopyLoHi32(&pExtVolInfo->luVolSizeInClusters,           &abuReply[16]);
      NCopyLoHi32(&pExtVolInfo->luFreeClusters,                &abuReply[20]);
      NCopyLoHi32(&pExtVolInfo->luSubAllocFreeableClusters,    &abuReply[24]);
      NCopyLoHi32(&pExtVolInfo->luFreeableLimboSectors,        &abuReply[28]);
      NCopyLoHi32(&pExtVolInfo->luNonfreeableLimboSectors,     &abuReply[32]);
      NCopyLoHi32(&pExtVolInfo->luAvailSubAllocSectors ,       &abuReply[36]);
      NCopyLoHi32(&pExtVolInfo->luNonuseableSubAllocSectors,   &abuReply[40]);
      NCopyLoHi32(&pExtVolInfo->luSubAllocClusters,            &abuReply[44]);
      NCopyLoHi32(&pExtVolInfo->luNumDataStreams,              &abuReply[48]);
      NCopyLoHi32(&pExtVolInfo->luNumLimboDataStreams,         &abuReply[52]);
      NCopyLoHi32(&pExtVolInfo->luOldestDelFileAgeInTicks,     &abuReply[56]);
      NCopyLoHi32(&pExtVolInfo->luNumCompDataStreams,          &abuReply[60]);
      NCopyLoHi32(&pExtVolInfo->luNumCompLimboDataStreams,     &abuReply[64]);
      NCopyLoHi32(&pExtVolInfo->luNumNoncompDataStreams,       &abuReply[68]);
      NCopyLoHi32(&pExtVolInfo->luPrecompSectors,              &abuReply[72]);
      NCopyLoHi32(&pExtVolInfo->luCompSectors,                 &abuReply[76]);
      NCopyLoHi32(&pExtVolInfo->luNumMigratedDataStreams,      &abuReply[80]);
      NCopyLoHi32(&pExtVolInfo->luMigratedSectors,             &abuReply[84]);
      NCopyLoHi32(&pExtVolInfo->luClustersUsedByFAT,           &abuReply[88]);
      NCopyLoHi32(&pExtVolInfo->luClustersUsedByDirs,          &abuReply[92]);
      NCopyLoHi32(&pExtVolInfo->luClustersUsedByExtDirs,       &abuReply[96]);
      NCopyLoHi32(&pExtVolInfo->luTotalDirEntries,             &abuReply[100]);
      NCopyLoHi32(&pExtVolInfo->luUnusedDirEntries,            &abuReply[104]);
      NCopyLoHi32(&pExtVolInfo->luTotalExtDirExtants,          &abuReply[108]);
      NCopyLoHi32(&pExtVolInfo->luUnusedExtDirExtants,         &abuReply[112]);
      NCopyLoHi32(&pExtVolInfo->luExtAttrsDefined,             &abuReply[116]);
      NCopyLoHi32(&pExtVolInfo->luExtAttrExtantsUsed,          &abuReply[120]);
      NCopyLoHi32(&pExtVolInfo->luDirServicesObjID,            &abuReply[124]);
      NCopyLoHi32(&pExtVolInfo->luVolLastModifiedDateAndTime,  &abuReply[128]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s51.c,v 1.7 1994/09/26 17:34:38 rebekah Exp $
*/
