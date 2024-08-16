/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s52.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s52GetRoutersInfo**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s52GetRoutersInfo
         (
            pNWAccess                 pAccess,
            nuint32                  luNetworkNum,
            nuint32                  luStartNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluNumEntries,
            pNWNCPFSERouterList      pList,
         )

REMARKS:

ARGS: <> pAccess
      >  luNetworkNum
      >  luStartNum
      <  pVConsoleInfo  (optional)
      <  psuReserved    (optional)
      <  pluNumEntries
      <  pList

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 52  Get Network Routers Information

CHANGES: 24 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s52GetRoutersInfo
(
   pNWAccess                 pAccess,
   nuint32                  luNetworkNum,
   nuint32                  luStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNumEntries,
   pNWNCPFSERouterList      pList
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 52)
   #define NCP_STRUCT_LEN  ((nuint16) 9)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 514)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;
   nint i;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luNetworkNum);
   NCopyLoHi32(&abuReq[7], &luStartNum);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuReply, NCP_REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuReply);
      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuReply[6]);
      NCopyLoHi32(pluNumEntries, &abuReply[8]);

      for (i=0; i < (nint) *pluNumEntries; i++)
      {
         NWCMemMove(pList->aRouters[i].abuNode, &abuReply[12+i*14], (nint) 6);
         NCopyLoHi32(&pList->aRouters[i].luConnectedLAN, &abuReply[18+i*14]);
         NCopyLoHi16(&pList->aRouters[i].suRouteHops, &abuReply[22+i*14]);
         NCopyLoHi16(&pList->aRouters[i].suRouteTime, &abuReply[24+i*14]);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s52.c,v 1.7 1994/09/26 17:32:47 rebekah Exp $
*/
