/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s25.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s25GetLSLInfo**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s25GetLSLInfo
         (
            pNWAccess                 pAccess,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pNWNCPFSELSLInfo         pInfo,
         )

REMARKS:

ARGS: <> pAccess
      <  pVConsoleInfo
      <  psuReserved
      <  pInfo

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 25  LSL Information

CHANGES: 23 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s25GetLSLInfo
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSELSLInfo         pInfo
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 25)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 82)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuRep[REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuRep, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuRep);

      if(psuReserved)
         NCopyLoHi16(psuReserved,&abuRep[6]);

      NCopyLoHi32(&pInfo->luRxBuffers,                   &abuRep[8]);
      NCopyLoHi32(&pInfo->luRxBuffers75,                 &abuRep[12]);
      NCopyLoHi32(&pInfo->luRxBuffersCheckedOut,         &abuRep[16]);
      NCopyLoHi32(&pInfo->luRxBufferSize,                &abuRep[20]);
      NCopyLoHi32(&pInfo->luMaxPacketSize,               &abuRep[24]);
      NCopyLoHi32(&pInfo->luLastTimeRxBufferWasAlloced,  &abuRep[28]);
      NCopyLoHi32(&pInfo->luMaxNumProtocols,             &abuRep[32]);
      NCopyLoHi32(&pInfo->luMaxNumMedias,                &abuRep[36]);
      NCopyLoHi32(&pInfo->luTotalTxPackets,              &abuRep[40]);
      NCopyLoHi32(&pInfo->luGetECBBuffers,               &abuRep[44]);
      NCopyLoHi32(&pInfo->luGetECBFailures,              &abuRep[48]);
      NCopyLoHi32(&pInfo->luAESEvents,                   &abuRep[52]);
      NCopyLoHi32(&pInfo->luPosponedEvents,              &abuRep[56]);
      NCopyLoHi32(&pInfo->luECBCancelFailures,           &abuRep[60]);
      NCopyLoHi32(&pInfo->luValidBuffersReused,          &abuRep[64]);
      NCopyLoHi32(&pInfo->luEnqueuedSends,               &abuRep[68]);
      NCopyLoHi32(&pInfo->luTotalRxPackets,              &abuRep[72]);
      NCopyLoHi32(&pInfo->luUnclaimedPackets,            &abuRep[76]);
      pInfo->buStatMajorVer                              =abuRep[80];
      pInfo->buStatMinorVer                              =abuRep[81];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s25.c,v 1.7 1994/09/26 17:32:20 rebekah Exp $
*/
