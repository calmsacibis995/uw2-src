/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s20.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s20GetActLANBoardLst**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s20GetActLANBoardLst
         (
            pNWAccess                 pAccess,
            nuint32                  luStartNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluMaxNumLANs,
            pnuint32                 pluNumEntries,
            pnuint32                 pluListB64,
         )

REMARKS:

ARGS: <> pAccess
      >  luStartNum
      <  pVConsoleInfo
      <  psuReserved
      <  pluMaxNumLANs
      <  pluNumEntries
      <  pluListB64

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 20  Active LAN Board List

CHANGES: 23 Sep 1993 - written -  rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s20GetActLANBoardLst
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluMaxNumLANs,
   pnuint32                 pluNumEntries,
   pnuint32                 pluListB64
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 20)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define MAX_LIST_LEN    ((nuint) (64 * 4))
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 16)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 2)

   nint32   lCode;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint16 suNCPLen;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   nuint32 luTemp;
   nint    i;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0],&suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luStartNum);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = pluListB64;
   replyFrag[1].uLen  = MAX_LIST_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
         NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuReply);
      if(psuReserved!=NULL) NCopyLoHi16(psuReserved, &abuReply[6]);
      if(pluMaxNumLANs!=NULL) NCopyLoHi32(pluMaxNumLANs, &abuReply[8]);
      if(pluNumEntries!=NULL) NCopyLoHi32(pluNumEntries, &abuReply[12]);
      for(i=0; i < (nint) *pluNumEntries; ++i)
      {
           luTemp= pluListB64[i];
           NCopyLoHi32(&pluListB64[i], &luTemp);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s20.c,v 1.7 1994/09/26 17:32:14 rebekah Exp $
*/
