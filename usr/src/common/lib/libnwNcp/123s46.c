/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s46.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s46GetMediaNameByNum**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s46GetMediaNameByNum
         (
            pNWAccess                 pAccess,
            nuint32                  luMediaNum,
            pNWNCPFSEVConsoleInfo    pVConsoleInfo,
            pnuint16                 psuReserved,
            pnuint8                  pbuNameLen,
            pnstr8                   pbstrMediaNameB81,
         )

REMARKS:

ARGS: <> pAccess
      >  luMediaNum
      <  pVConsoleInfo
      <  psuReserved
      <  pbuNameLen
      <  pbstrMediaNameB81

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 46  Get Media Name by Media Number

CHANGES: 23 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s46GetMediaNameByNum
(
   pNWAccess                 pAccess,
   nuint32                  luMediaNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint8                  pbuNameLen,
   pnstr8                   pbstrMediaNameB81
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 46)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define MEDIA_NAME_LEN  ((nuint) 81)
   #define NCP_REQ_LEN     ((nuint) 7)
   #define NCP_REPLY_LEN   ((nuint) 9)
   #define NCP_REQ_FRAGS   ((nuint) 1)
   #define NCP_REPLY_FRAGS ((nuint) 2)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuRep[NCP_REPLY_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3], &luMediaNum);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   replyFrag[0].pAddr = abuRep;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   replyFrag[1].pAddr = pbstrMediaNameB81;
   replyFrag[1].uLen  = MEDIA_NAME_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuRep);
      if(psuReserved)
         NCopyLoHi16(psuReserved, &abuRep[6]);
      *pbuNameLen = abuRep[7];
   }
   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s46.c,v 1.7 1994/09/26 17:32:40 rebekah Exp $
*/
