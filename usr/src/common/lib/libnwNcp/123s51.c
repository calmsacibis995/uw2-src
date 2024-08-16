/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s51.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s51GetRouterInfo**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s51GetRouterInfo
         (
            pNWAccess                 pAccess,
            nuint32                  luNetworkNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluNetIDNum,
            pnuint16                 psuHopsToNetCnt,
            pnuint16                 psuNetStatus,
            pnuint16                 psuTimeToNet,
         )

REMARKS:

ARGS: <> pAccess
      >  luNetworkNum
      <  pVConsoleInfo        (optional)
      <  psuReserved          (optional)
      <  pluNetIDNum          (optional)
      <  psuHopsToNetCnt      (optional)
      <  psuNetStatus         (optional)
      <  psuTimeToNet         (optional)

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 51  Get Network Router Information

CHANGES: 23 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s51GetRouterInfo
(
   pNWAccess                 pAccess,
   nuint32                  luNetworkNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNetIDNum,
   pnuint16                 psuHopsToNetCnt,
   pnuint16                 psuNetStatus,
   pnuint16                 psuTimeToNet
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 51)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 18)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luNetworkNum);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuReply, NCP_REPLY_LEN, NULL);
   if (lCode == 0)
   {
         NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuReply);
         if(psuReserved!=NULL)  NCopyLoHi16(psuReserved, &abuReply[6]);
         if(pluNetIDNum!=NULL)  NCopyLoHi32(pluNetIDNum,  &abuReply[8]);
         if(psuHopsToNetCnt!=NULL) NCopyLoHi16(psuHopsToNetCnt, &abuReply[12]);
         if(psuNetStatus!=NULL)  NCopyLoHi16(psuNetStatus,  &abuReply[14]);
         if(psuTimeToNet!=NULL)  NCopyLoHi16(psuTimeToNet, &abuReply[16]);

   }
   return((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s51.c,v 1.7 1994/09/26 17:32:46 rebekah Exp $
*/
