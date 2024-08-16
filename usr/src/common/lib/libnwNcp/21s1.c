/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:21s1.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpmsg.h"

/*manpage*NWNCP21s1MsgGetBroadcast**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP21s1MsgGetBroadcast
         (
            pNWAccess pAccess,
            pnuint8  pbuMsgLen,
            pnuint8  pbuMsg,
         );

REMARKS: This call allows a client to retrieve a message sent by
         another client.  If no message has been left, then
         MessageLength will contain a zero.

         This call can be used by any client.

ARGS: <> pAccess
      <  pbuMsgLen
      <  pbuMsg

INCLUDE: ncpmsg.h

RETURN:  0x0000  Successful
         0x89FD  Bad Station Number

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     21 00  Send Broadcast Message

NCP:     21 01  Get Broadcast Message

CHANGES: 29 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP21s1MsgGetBroadcast
(
   pNWAccess pAccess,
   pnuint8  pbuMsgLen,
   pnuint8  pbuMsg
)
{
   #define NCP_FUNCTION    ((nuint) 21)
   #define NCP_SUBFUNCTION ((nuint) 1)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define MAX_MSG_LEN     ((nuint) 58)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 2)

   nuint8  abuReq[REQ_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = pbuMsgLen;
   replyFrag[0].uLen  = (nuint) 1;

   replyFrag[1].pAddr = pbuMsg;
   replyFrag[1].uLen  = MAX_MSG_LEN;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/21s1.c,v 1.7 1994/09/26 17:33:28 rebekah Exp $
*/
