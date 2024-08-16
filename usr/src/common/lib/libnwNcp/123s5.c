/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s5.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s5GetPacketBurstInfo*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s5GetPacketBurstInfo
         (
            pNWAccess                 pAccess,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pNWNCPFSEBurstInfo       pBurstInfo,
         )

REMARKS:

ARGS: <> pAccess
      <  pVConsoleInfo (optional)
      <  psuReserved (optional)
      <  pBurstInfo

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 05  Packet Burst Information

CHANGES: 23 Sep 1993 - written - lwiltban
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s5GetPacketBurstInfo
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSEBurstInfo       pBurstInfo
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 5)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 208)

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

      NCopyLoHi32(&pBurstInfo->luBigInvalidSlotCnt, &abuReply[8]);
      NCopyLoHi32(&pBurstInfo->luBigForgedPacketCnt, &abuReply[12]);
      NCopyLoHi32(&pBurstInfo->luBigInvalidPacketCnt, &abuReply[16]);
      NCopyLoHi32(&pBurstInfo->luBigStillTransmittingCnt, &abuReply[20]);
      NCopyLoHi32(&pBurstInfo->luStillDoingTheLastRequestCnt, &abuReply[24]);
      NCopyLoHi32(&pBurstInfo->luInvalidControlRequestCnt, &abuReply[28]);
      NCopyLoHi32(&pBurstInfo->luControlInvalidMsgNumCnt, &abuReply[32]);
      NCopyLoHi32(&pBurstInfo->luControlBeingTornDownCnt, &abuReply[36]);
      NCopyLoHi32(&pBurstInfo->luBigRepeatTheFileReadCnt, &abuReply[40]);
      NCopyLoHi32(&pBurstInfo->luBigSendExtraCCCnt, &abuReply[44]);
      NCopyLoHi32(&pBurstInfo->luBigReturnAbortMsgCnt, &abuReply[48]);
      NCopyLoHi32(&pBurstInfo->luBigReadInvalidMsgNumCnt, &abuReply[52]);
      NCopyLoHi32(&pBurstInfo->luBigReadDoItOverCnt, &abuReply[56]);
      NCopyLoHi32(&pBurstInfo->luBigReadBeingTornDownCnt, &abuReply[60]);
      NCopyLoHi32(&pBurstInfo->luPreviousControlPacketCnt, &abuReply[64]);
      NCopyLoHi32(&pBurstInfo->luSendHoldOffMsgCnt, &abuReply[68]);
      NCopyLoHi32(&pBurstInfo->luBigReadNoDataAvailableCnt, &abuReply[72]);
      NCopyLoHi32(&pBurstInfo->luBigReadTryToReadTooMuchCnt, &abuReply[76]);
      NCopyLoHi32(&pBurstInfo->luAsyncReadErrorCnt, &abuReply[80]);
      NCopyLoHi32(&pBurstInfo->luBigReadPhysicalReadErrorCnt, &abuReply[84]);
      NCopyLoHi32(&pBurstInfo->luControlBadACKFragListCnt, &abuReply[88]);
      NCopyLoHi32(&pBurstInfo->luControlNoDataReadCnt, &abuReply[92]);
      NCopyLoHi32(&pBurstInfo->luWriteDupRequestCnt, &abuReply[96]);
      NCopyLoHi32(&pBurstInfo->luShouldntBeACKingHereCnt, &abuReply[100]);
      NCopyLoHi32(&pBurstInfo->luWriteInconsistentPktLenCnt, &abuReply[104]);
      NCopyLoHi32(&pBurstInfo->luFirstPacketIsntAWriteCnt, &abuReply[108]);
      NCopyLoHi32(&pBurstInfo->luWriteTrashedDupRequestCnt, &abuReply[112]);
      NCopyLoHi32(&pBurstInfo->luBigWriteInvalidMsgNumCnt, &abuReply[116]);
      NCopyLoHi32(&pBurstInfo->luBigWriteBeingTornDownCnt, &abuReply[120]);
      NCopyLoHi32(&pBurstInfo->luBigWriteBeingAbortedCnt, &abuReply[124]);
      NCopyLoHi32(&pBurstInfo->luZeroACKFragCnt, &abuReply[128]);
      NCopyLoHi32(&pBurstInfo->luWriteCurrentlyTransCnt, &abuReply[132]);
      NCopyLoHi32(&pBurstInfo->luTryingToWriteTooMuchCnt, &abuReply[136]);
      NCopyLoHi32(&pBurstInfo->luWriteOutOfMemForCtrlNodesCnt, &abuReply[140]);
      NCopyLoHi32(&pBurstInfo->luWriteDidntNeedThisFragCnt, &abuReply[144]);
      NCopyLoHi32(&pBurstInfo->luWriteTooManyBufsChkedOutCnt, &abuReply[148]);
      NCopyLoHi32(&pBurstInfo->luWriteTimeOutCnt, &abuReply[152]);
      NCopyLoHi32(&pBurstInfo->luWriteGotAnACKCnt0, &abuReply[156]);
      NCopyLoHi32(&pBurstInfo->luWriteGotAnACKCnt1, &abuReply[160]);
      NCopyLoHi32(&pBurstInfo->luPollerAbortedConnCnt, &abuReply[164]);
      NCopyLoHi32(&pBurstInfo->luMaybeHadOutOfOrderWritesCnt, &abuReply[168]);
      NCopyLoHi32(&pBurstInfo->luHadAnOutOfOrderWriteCnt, &abuReply[172]);
      NCopyLoHi32(&pBurstInfo->luMovedTheACKBitDownCnt, &abuReply[176]);
      NCopyLoHi32(&pBurstInfo->luBumpedOutOfOrderWriteCnt, &abuReply[180]);
      NCopyLoHi32(&pBurstInfo->luPollerRemovedOutOfOrderCnt, &abuReply[184]);
      NCopyLoHi32(&pBurstInfo->luWriteRequestedUnneededACKCnt, &abuReply[188]);
      NCopyLoHi32(&pBurstInfo->luWriteTrashedPacketCnt, &abuReply[192]);
      NCopyLoHi32(&pBurstInfo->luTooManyACKFragCnt, &abuReply[196]);
      NCopyLoHi32(&pBurstInfo->luSavedAnOutOfOrderPacketCnt, &abuReply[200]);
      NCopyLoHi32(&pBurstInfo->luConnBeingAbortedCnt, &abuReply[204]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s5.c,v 1.7 1994/09/26 17:32:43 rebekah Exp $
*/
