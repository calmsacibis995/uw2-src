/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s2.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s2GetFileServerInfo*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s2GetServerInfo
         (
            pNWAccess                 pAccess,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluStationsInUseCnt,
            pnuint32                 pluPeakStationsInUse,
            pnuint32                 pluNumNCPRequests,
            pnuint32                 pluServerUtilization,
            pNWNCPFSEFileServerInfo  pServerInfo,
            pNWNCPFSEFileServerCnts  pServerCounters,
         )

REMARKS:

ARGS: <> pAccess
      <  pVConsoleInfo (optional)
      <  psuReserved (optional)
      <  pluStationsInUseCnt (optional)
      <  pluPeakStationsInUse (optional)
      <  pluNumNCPRequests (optional)
      <  pluServerUtilization (optional)
      <  pServerInfo (optional)
      <  pServerCounters (optional)

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 02  Get Server Information

CHANGES: 23 Sep 1993 - written - lwiltban
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s2GetServerInfo
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluStationsInUseCnt,
   pnuint32                 pluPeakStationsInUse,
   pnuint32                 pluNumNCPRequests,
   pnuint32                 pluServerUtilization,
   pNWNCPFSEServerInfo      pServerInfo,
   pNWNCPFSEServerCnts      pServerCounters
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 2)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 152)

   nint32   lCode;
   nuint8  abuReq[REQ_LEN],
           abuRep[REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuRep, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF( pVConsoleInfo, abuRep );

      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuRep[6]);

      if (pluStationsInUseCnt)
         NCopyLoHi32(pluStationsInUseCnt, &abuRep[8]);
      if (pluPeakStationsInUse)
         NCopyLoHi32(pluPeakStationsInUse, &abuRep[12]);
      if (pluNumNCPRequests)
         NCopyLoHi32(pluNumNCPRequests, &abuRep[16]);
      if (pluServerUtilization)
         NCopyLoHi32(pluServerUtilization, &abuRep[20]);

      if (pServerInfo)
      {
         NCopyLoHi32(&pServerInfo->luReplyCancledCnt, &abuRep[24]);
         NCopyLoHi32(&pServerInfo->luWriteHeldOffCnt, &abuRep[28]);
         NCopyLoHi32(&pServerInfo->luWriteHeldOffWithDupRequest, &abuRep[32]);
         NCopyLoHi32(&pServerInfo->luInvalidRequestTypeCnt, &abuRep[36]);
         NCopyLoHi32(&pServerInfo->luBeingAbortedCnt, &abuRep[40]);
         NCopyLoHi32(&pServerInfo->luAlreadyDoingReAllocCnt, &abuRep[44]);
         NCopyLoHi32(&pServerInfo->luDeallocInvalidSlotCnt, &abuRep[48]);
         NCopyLoHi32(&pServerInfo->luDeallocBeingProcessedCnt, &abuRep[52]);
         NCopyLoHi32(&pServerInfo->luDeallocForgedPacketCnt, &abuRep[56]);
         NCopyLoHi32(&pServerInfo->luDeallocStillTransmittingCnt, &abuRep[60]);
         NCopyLoHi32(&pServerInfo->luStartStationErrorCnt, &abuRep[64]);
         NCopyLoHi32(&pServerInfo->luInvalidSlotCnt, &abuRep[68]);
         NCopyLoHi32(&pServerInfo->luBeingProcessedCnt, &abuRep[72]);
         NCopyLoHi32(&pServerInfo->luForgedPacketCnt, &abuRep[76]);
         NCopyLoHi32(&pServerInfo->luStillTransmittingCnt, &abuRep[80]);
         NCopyLoHi32(&pServerInfo->luReExecuteRequestCnt, &abuRep[84]);
         NCopyLoHi32(&pServerInfo->luInvalidSequenceNumCnt, &abuRep[88]);
         NCopyLoHi32(&pServerInfo->luDupIsBeingSentAlreadyCnt, &abuRep[92]);
         NCopyLoHi32(&pServerInfo->luSentPositiveAcknowledgeCnt, &abuRep[96]);
         NCopyLoHi32(&pServerInfo->luSentADupReplyCnt, &abuRep[100]);
         NCopyLoHi32(&pServerInfo->luNoMemoryForStationControlCnt, &abuRep[104]);
         NCopyLoHi32(&pServerInfo->luNoAvailableConnsCnt, &abuRep[108]);
         NCopyLoHi32(&pServerInfo->luReAllocSlotCnt, &abuRep[112]);
         NCopyLoHi32(&pServerInfo->luReAllocSlotCameTooSoonCnt, &abuRep[116]);
      }

      if (pServerCounters)
      {
         NCopyLoHi16(&pServerCounters->suTooManyHops, &abuRep[120]);
         NCopyLoHi16(&pServerCounters->suUnknownNetwork, &abuRep[124]);
         NCopyLoHi16(&pServerCounters->suNoSpaceForService, &abuRep[128]);
         NCopyLoHi16(&pServerCounters->suNoReceiveBuffers, &abuRep[132]);
         NCopyLoHi16(&pServerCounters->suNotMyNetwork, &abuRep[136]);
         NCopyLoHi32(&pServerCounters->luNetBIOSProgatedCnt, &abuRep[140]);
         NCopyLoHi32(&pServerCounters->luTotalPacketsServiced, &abuRep[144]);
         NCopyLoHi32(&pServerCounters->luTotalPacketsRouted, &abuRep[148]);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s2.c,v 1.7 1994/09/26 17:32:13 rebekah Exp $
*/
