/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:32s0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP32s0SyncSemOpen**********************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP32s0SyncSemOpen
         (
            pNWAccess pAccess,
            nuint8   buInitSemVal,
            nuint8   buSemNameLen,
            pnuint8  pbuSemName,
            pnuint32 pluSemHandle,
            pnuint8  pbuSemOpenCnt,
         )

REMARKS:

ARGS: <> pAccess
      >  buInitSemVal
      >  buSemNameLen
      <  pbuSemName
      <  pluSemHandle
      <  pbuSemOpenCnt

INCLUDE: ncpsync.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     32 00  Open Semaphore

CHANGES: 10 Aug 1993 - written - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP32s0SyncSemOpen
(
   pNWAccess pAccess,
   nuint8   buInitSemVal,
   nuint8   buSemNameLen,
   pnstr8  pbuSemName,
   pnuint32 pluSemHandle,
   pnuint8  pbuSemOpenCnt
)
{
   #define NCP_FUNCTION    ((nuint) 32)
   #define NCP_SUBFUNCTION ((nuint8) 0)
   #define REQ_LEN         ((nuint) 3)
   #define REPLY_LEN       ((nuint) 5)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 1)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buInitSemVal;
   abuReq[2] = buSemNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbuSemName;
   reqFrag[1].uLen  = (nuint) buSemNameLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      NCopyLoHi32(pluSemHandle, &abuReply[0]);
      *pbuSemOpenCnt = abuReply[4];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/32s0.c,v 1.7 1994/09/26 17:37:50 rebekah Exp $
*/
