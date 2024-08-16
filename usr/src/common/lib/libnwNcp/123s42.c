/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s42.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s42GetProtocolStats**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s42GetProtocolStats
         (
            pNWAccess                 pAccess,
            nuint32                  luStackNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,         (optional)
            pnuint16                 psuReserved,           (optional)
            pnuint8                  pbuStatMajorVer,       (optional)
            pnuint8                  pbuStatMinorVer,       (optional)
            pnuint16                 psuCommonCounters,     (optional)
            pnuint32                 pluCounterMask,        (optional)
            pnuint32                 pluTotalTxPackets,     (optional)
            pnuint32                 pluTotalRxPackets,     (optional)
            pnuint32                 pluIgnoredRxPackets,   (optional)
            pnuint16                 psuCustomCounters,     (optional)
         )

REMARKS:

ARGS: <> pAccess
      >  luStackNum
      <  pVConsoleInfo
      <  psuReserved
      <  pbuStatMajorVer
      <  pbuStatMinorVer
      <  psuCommonCounters
      <  pluCounterMask
      <  pluTotalTxPackets
      <  pluTotalRxPackets
      <  pluIgnoredRxPackets
      <  psuCustomCounters

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 42  Get Protocol Stack Statistics Information

CHANGES: 23 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s42GetProtocolStats
(
   pNWAccess                 pAccess,
   nuint32                  luStackNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint8                  pbuStatMajorVer,
   pnuint8                  pbuStatMinorVer,
   pnuint16                 psuCommonCounters,
   pnuint32                 pluCounterMask,
   pnuint32                 pluTotalTxPackets,
   pnuint32                 pluTotalRxPackets,
   pnuint32                 pluIgnoredRxPackets,
   pnuint16                 psuCustomCounters
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 42)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 34)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuRep[NCP_REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3],&luStackNum);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuRep, NCP_REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuRep);
      if(psuReserved)
         NCopyLoHi16(psuReserved,&abuRep[6]);
      if(pbuStatMajorVer)
         *pbuStatMajorVer = abuRep[8];
      if(pbuStatMinorVer)
         *pbuStatMinorVer = abuRep[9];
      if(psuCommonCounters)
         NCopyLoHi16(psuCommonCounters,&abuRep[10]);

      if(pluCounterMask)
      {
         NCopyLoHi32(&pluCounterMask[0],&abuRep[12]);
         NCopyLoHi32(&pluCounterMask[1],&abuRep[16]);
      }

      if(pluTotalTxPackets)
         NCopyLoHi32(pluTotalTxPackets,&abuRep[20]);
      if(pluTotalRxPackets)
         NCopyLoHi32(pluTotalRxPackets,&abuRep[24]);
      if(pluIgnoredRxPackets)
         NCopyLoHi32(pluIgnoredRxPackets,&abuRep[28]);
      if(psuCustomCounters)
         NCopyLoHi16(psuCustomCounters,&abuRep[32]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s42.c,v 1.7 1994/09/26 17:32:34 rebekah Exp $
*/
