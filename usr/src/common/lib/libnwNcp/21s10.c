/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:21s10.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpmsg.h"

/*manpage*NWNCP21s10MsgSendBroadcast********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP21s10MsgSendBroadcast
         (
            pNWAccess  pAccess,
            nuint16   suNumClients,
            pnuint32  pluClientB,
            nuint8    buMsgLen,
            pnstr8    pbstrMsg,
            pnuint16  psuNumStatus,
            pnuint32  pluSendStatusB,
         )

REMARKS: This is a NetWare 386 v3.2 call.  It is similar to the
         earlier Send Broadcast Message NCP (0x2222  21  --) except
         that this newer version allows station numbers greater than
         256, and it gives back unique error codes on message send
         failures.


ARGS: <> pAccess
      >  suNumClients
      >  pluClientB
      >  buMsgLen
      >  pbstrMsg
      <  psuNumStatus
      <  pluSendStatusB

INCLUDE: ncpmsg.h

RETURN:  0x0000   Successful

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     21 10  Send Broadcast Message

CHANGES: 25 Aug 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP21s10MsgSendBroadcast
(
   pNWAccess  pAccess,
   nuint16   suNumClients,
   pnuint32  pluClientB,
   nuint8    buMsgLen,
   pnstr8    pbstrMsg,
   pnuint16  psuNumStatus,
   pnuint32  pluSendStatusB
)
{
   #define NCP_FUNCTION    ((nuint) 21)
   #define NCP_SUBFUNCTION ((nuint8) 10)
   #define NCP_STRUCT_LEN  ((nuint16) (4 + CLIENT_LST_LEN + buMsgLen))
   #define CLIENT_LST_LEN  ((nuint) (suNumClients * 4))
   #define NCP_REQ_LEN     ((nuint) 2 + 4 + CLIENT_LST_LEN)
   #define MAX_NCP_REQ_LEN ((nuint) 6 + (4 * 100) + 55)
   #define NCP_REPLY_LEN   ((nuint) 2)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 2)

   nint32   lCode;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];
   nuint16 suNCPLen, suNumStatusRecs, i;
   nuint8  abuReq[MAX_NCP_REQ_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi16(&abuReq[3], &suNumClients);

   for (i = 0; i < suNumClients; i++)
      NCopyLoHi32(&abuReq[5+(i*4)], &pluClientB[i]);

   abuReq[5+(i*4)] = buMsgLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrMsg;
   reqFrag[1].uLen  = buMsgLen;

   replyFrag[0].pAddr = &suNumStatusRecs;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   replyFrag[1].pAddr = pluSendStatusB;
   replyFrag[1].uLen  = CLIENT_LST_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
                     NCP_REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      NCopyLoHi16(psuNumStatus, &suNumStatusRecs);

      for (i = 0; i < suNumStatusRecs; i++)
      {
         pluSendStatusB[i] = NSwapLoHi32(pluSendStatusB[i]);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/21s10.c,v 1.7 1994/09/26 17:33:29 rebekah Exp $
*/
