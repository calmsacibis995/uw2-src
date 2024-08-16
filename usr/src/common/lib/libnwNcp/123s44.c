/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s44.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s44GetProtNumByMedia**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s44GetProtNumByMedia
         (
            pNWAccess                 pAccess,
            nuint32                  luMediaType,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint32                 pluNumStacks,
            pnuint32                 pluStacksB128,
         )

REMARKS:

ARGS: <> pAccess
      >  luMediaType
      <  pVConsoleInfo     (optional)
      <  psuReserved       (optional)
      <  pluNumStacks      (optional)
      <  pluStacksB128

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 44  Get Protocol Stack Number by Media Number

CHANGES: 23 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s44GetProtNumByMedia
(
   pNWAccess                 pAccess,
   nuint32                  luMediaType,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNumStacks,
   pnuint32                 pluStacksB128
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 44)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define MAX_STACK_LEN   ((nuint) 128)
   #define NCP_REQ_LEN     ((nuint) 7)
   #define NCP_REPLY_LEN   ((nuint) 12)
   #define NCP_REQ_FRAGS   ((nuint) 1)
   #define NCP_REPLY_FRAGS ((nuint) 2)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];
   nint i;
   nuint16 luTemp;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luMediaType);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   replyFrag[1].pAddr = pluStacksB128;
   replyFrag[1].uLen  = MAX_STACK_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);

   if (lCode == 0)
   {
         NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuReply);
         NCopyLoHi16(psuReserved, &abuReply[6]);
         NCopyLoHi32(pluNumStacks, &abuReply[8]);

         for(i = 0; i < (nint)*pluNumStacks; ++i)
         {
            NCopy32(&luTemp, &pluStacksB128[i]);
            NCopyLoHi32(&pluStacksB128[i], &luTemp);
         }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s44.c,v 1.7 1994/09/26 17:32:37 rebekah Exp $
*/
