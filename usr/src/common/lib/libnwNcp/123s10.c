/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s10.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s10GetNLMLoadedList*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s10GetNLMLoadedList
         (
            pNWAccess                 pAccess,
            nuint32                  luStartNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuMoreFlag,
            pnuint32                 pluNumNLMsLoaded,
            pnuint32                 pluListB128,
         )

REMARKS:

ARGS: <  pAccess
      >  luStartNum
      <  pVConsoleInfo (optional)
      <  psuMoreFlag (optional)
      <  pluNumNLMsLoaded
      <  pluListB128

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 10 Get NLM Loaded List

CHANGES: 23 Sep 1993 - written - lwiltban
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s10GetNLMLoadedList
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuMoreFlag,
   pnuint32                 pluNumNLMsLoaded,
   pnuint32                 pluNLMCount,
   pnuint32                 pluListB128
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 10)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define MAX_LIST_LEN    ((nuint) (128 * 4))
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 16)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 2)

   nint32   lCode;
   nuint8  abuReq[REQ_LEN],
           abuReply[REPLY_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luStartNum);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = pluListB128;
   replyFrag[1].uLen  = MAX_LIST_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      nint i;

      NWNCP_UNPACK_VCONS_INF( pVConsoleInfo, abuReply);

      NCopyLoHi16(psuMoreFlag, &abuReply[6]);
      NCopyLoHi32(pluNumNLMsLoaded, &abuReply[8]);
      NCopyLoHi32(pluNLMCount, &abuReply[12]);

      for (i = 0; i < (nint)*pluNLMCount; i++)
         pluListB128[i] = NSwapLoHi32(pluListB128[i]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s10.c,v 1.7 1994/09/26 17:32:04 rebekah Exp $
*/
