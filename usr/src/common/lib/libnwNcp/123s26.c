/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s26.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s26GetLSLBoardStats**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s26GetLSLBoardStats
         (
            pNWAccess                 pAccess,
            nuint32                  luBoardNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluTotalTxPackets,
            pnuint32                 pluTotalRxPackets,
            pnuint32                 pluUnclaimedPackets,
            pnuint32                 pluReserved,
         )

REMARKS:

ARGS: <> pAccess
      >  luBoardNum
      <  pVConsoleInfo
      <  psuReserved
      <  pluTotalTxPackets
      <  pluTotalRxPackets
      <  pluUnclaimedPackets
      <  pluReserved

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 26  LSL Logical Board Statistics

CHANGES: 23 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s26GetLSLBoardStats
(
   pNWAccess                 pAccess,
   nuint32                  luBoardNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluTotalTxPackets,
   pnuint32                 pluTotalRxPackets,
   pnuint32                 pluUnclaimedPackets,
   pnuint32                 pluReserved
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 26)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 24)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuRep[REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luBoardNum);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuRep, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuRep);

      if(psuReserved)
            NCopyLoHi16(psuReserved,&abuRep[6]);

      if(pluTotalTxPackets)
         NCopyLoHi32(pluTotalTxPackets,&abuRep[8]);

      if(pluTotalRxPackets)
         NCopyLoHi32(pluTotalRxPackets,&abuRep[12]);

      if(pluUnclaimedPackets)
         NCopyLoHi32(pluUnclaimedPackets,&abuRep[16]);

      if(pluReserved)
         NCopyLoHi32(pluReserved,&abuRep[20]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s26.c,v 1.7 1994/09/26 17:32:21 rebekah Exp $
*/
