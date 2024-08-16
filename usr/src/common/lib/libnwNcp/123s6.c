/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s6.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s6GetIPXSPXInfo*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s6GetIPXSPXInfo
         (
            pNWAccess                 pAccess,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pNWNCPFSEIPXInfo         pIPXInfo,
            pNWNCPFSESPXInfo         pSPXInfo
         )

REMARKS:

ARGS: <> pAccess
      <  pVConsoleInfo (optional)
      <  psuReserved (optional)
      <  pIPXInfo (optional)
      <  pSPXInfo (optional)

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 06  IPX SPX Information

CHANGES: 24 Sep 1993 - written - lwiltban
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s6GetIPXSPXInfo
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSEIPXInfo         pIPXInfo,
   pNWNCPFSESPXInfo         pSPXInfo
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 6)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 86)

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
         NCopyLoHi16(psuReserved,&abuReply[6]);

      if (pIPXInfo)
      {
         NCopyLoHi32(&pIPXInfo->luIPXSendPacketCnt, &abuReply[8]);
         NCopyLoHi16(&pIPXInfo->suIPXMalformPacketCnt, &abuReply[12]);
         NCopyLoHi32(&pIPXInfo->luIPXGetEcbRequestCnt, &abuReply[14]);
         NCopyLoHi32(&pIPXInfo->luIPXGetEcbFailCnt, &abuReply[18]);
         NCopyLoHi32(&pIPXInfo->luIPXAesEventCnt, &abuReply[22]);
         NCopyLoHi16(&pIPXInfo->suIPXPostponedAesCnt, &abuReply[26]);
         NCopyLoHi16(&pIPXInfo->suIPXMaxConfiguredSocketCnt, &abuReply[28]);
         NCopyLoHi16(&pIPXInfo->suIPXMaxOpenSocketCnt, &abuReply[30]);
         NCopyLoHi16(&pIPXInfo->suIPXOpenSocketFailCnt, &abuReply[32]);
         NCopyLoHi32(&pIPXInfo->luIPXListenEcbCnt, &abuReply[34]);
         NCopyLoHi16(&pIPXInfo->suIPXEcbCancelFailCnt, &abuReply[38]);
         NCopyLoHi16(&pIPXInfo->suIPXGetLocalTargetFailCnt, &abuReply[40]);
      }

      if (pSPXInfo)
      {
         NCopyLoHi16(&pSPXInfo->suSPXMaxConnsCnt, &abuReply[42]);
         NCopyLoHi16(&pSPXInfo->suSPXMaxUsedConns, &abuReply[44]);
         NCopyLoHi16(&pSPXInfo->suSPXEstConnReq, &abuReply[46]);
         NCopyLoHi16(&pSPXInfo->suSPXEstConnFail, &abuReply[48]);
         NCopyLoHi16(&pSPXInfo->suSPXListenConnectReq, &abuReply[50]);
         NCopyLoHi16(&pSPXInfo->suSPXListenConnectFail, &abuReply[52]);
         NCopyLoHi32(&pSPXInfo->luSPXSendCnt, &abuReply[54]);
         NCopyLoHi32(&pSPXInfo->luSPXWindowChokeCnt, &abuReply[58]);
         NCopyLoHi16(&pSPXInfo->suSPXBadSendCnt, &abuReply[62]);
         NCopyLoHi16(&pSPXInfo->suSPXSendFailCnt, &abuReply[64]);
         NCopyLoHi16(&pSPXInfo->suSPXAbortedConn, &abuReply[66]);
         NCopyLoHi32(&pSPXInfo->luSPXListenPacketCnt, &abuReply[68]);
         NCopyLoHi16(&pSPXInfo->suSPXBadListenCnt, &abuReply[72]);
         NCopyLoHi32(&pSPXInfo->luSPXIncomingPacketCnt, &abuReply[74]);
         NCopyLoHi16(&pSPXInfo->suSPXBadInPacketCnt, &abuReply[78]);
         NCopyLoHi16(&pSPXInfo->suSPXSuppressedPackCnt, &abuReply[80]);
         NCopyLoHi16(&pSPXInfo->suSPXNoSesListenEcbCnt, &abuReply[82]);
         NCopyLoHi16(&pSPXInfo->suSPXWatchDogDestSesCnt, &abuReply[84]);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s6.c,v 1.7 1994/09/26 17:32:55 rebekah Exp $
*/
