/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s55.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s55GetServerSourcesInfo**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s55GetServerSourcesInfo
         (
            pNWAccess                 pAccess,
            nuint32                  luStartNum,
            nuint32                  luServerType,
            nuint8                   buServerNameLen,
            pnstr8                   pbstrServerName,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluNumEntries,
            pNWNCPFSESourceList      pSources,
         )

REMARKS:

ARGS: <> pAccess
      >  luStartNum
      >  luServerType
      >  pbstrServerName
      <  pVConsoleInfo
      <  psuReserved
      <  pluNumEntries
      <  pSources

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 55  Get Server Sources Information

CHANGES: 24 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s55GetServerSourcesInfo
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   nuint32                  luServerType,
   nuint8                   buServerNameLen,
   pnstr8                   pbstrServerName,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNumEntries,
   pNWNCPFSESourceList      pSources
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 55)
   #define NCP_STRUCT_LEN  ((nuint16) 10 + buServerNameLen)
   #define NCP_REQ_LEN     ((nuint) 12)
   #define NCP_REPLY_LEN   ((nuint) 516)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 1)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];
   nint i;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luStartNum);
   NCopyLoHi32(&abuReq[7], &luServerType);
   abuReq[11] = buServerNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrServerName;
   reqFrag[1].uLen  = buServerNameLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuReply);
      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuReply[6]);
      NCopyLoHi32(pluNumEntries, &abuReply[8]);
      for (i=0; i < (nint) *pluNumEntries; i++)
      {
         NWCMemMove(pSources->aSources[i].abuServerNode, &abuReply[12+i*12], (nint) 6);
         NCopyLoHi32(&pSources->aSources[i].luConnectedLAN, &abuReply[18+i*12]);
         NCopyLoHi16(&pSources->aSources[i].suSourceHops, &abuReply[22+i*12]);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s55.c,v 1.7 1994/09/26 17:32:52 rebekah Exp $
*/
