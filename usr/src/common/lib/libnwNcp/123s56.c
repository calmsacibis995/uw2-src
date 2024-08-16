/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s56.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s56GetKnownServers**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s56GetKnownServers
         (
            pNWAccess                 pAccess,
            nuint32                  luStartNum,
            nuint32                  luServerType,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluNumEntries,
            pnuint8                  pServerInfoB512,
         )

REMARKS:

ARGS: <> pAccess
      >  luStartNum
      >  luServerType
      <  pVConsoleInfo
      <  psuReserved
      <  pluNumEntries
      <  pServerInfoB512

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 56  Get Known Servers Information

CHANGES: 24 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s56GetKnownServers
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   nuint32                  luServerType,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNumEntries,
   pnuint8                  pServerInfoB512
)
{
   #define NCP_FUNCTION     ((nuint) 123)
   #define NCP_SUBFUNCTION  ((nuint8) 56)
   #define NCP_STRUCT_LEN   ((nuint16) 9)
   #define MAX_SERVINFO_LEN ((nuint) 512)
   #define NCP_REQ_LEN      ((nuint) 11)
   #define NCP_REPLY_LEN    ((nuint) 12)
   #define NCP_REQ_FRAGS    ((nuint) 1)
   #define NCP_REPLY_FRAGS  ((nuint) 2)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[2];
   nint i;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luStartNum);
   NCopyLoHi32(&abuReq[7], &luServerType);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   replyFrag[1].pAddr = pServerInfoB512;
   replyFrag[1].uLen  = MAX_SERVINFO_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      pnuint8 pbuIndex;

      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuReply);
      if (psuReserved)
         NCopyLoHi16(psuReserved, &abuReply[6]);
      NCopyLoHi32(pluNumEntries, &abuReply[8]);
      for (i=0, pbuIndex = &pServerInfoB512[0];
           i < (nint) *pluNumEntries;
           i++, pbuIndex += 15+pbuIndex[14])
      {
         nuint16 suTemp;

         NCopyLoHi16(&suTemp, &pbuIndex[12]);
         NCopy16(&pbuIndex[12], &suTemp);
      }
   }
   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s56.c,v 1.7 1994/09/26 17:32:53 rebekah Exp $
*/
