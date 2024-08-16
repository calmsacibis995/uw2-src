/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:21s0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpmsg.h"

/*manpage*NWNCP21s0MsgSendBroadcast**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP21s0MsgSendBroadcast
         (
            pNWAccess pAccess,
            nuint8   buNumClients,
            pnuint8  pbuClientBuf,
            nuint8   buMsgLen,
            pnstr8   pbstrMsg,
            pnuint8  pbuNumStatus,
            pnuint8  pbuSendStatusBuf,
         );

REMARKS: This call allows a client to send short text messages to a
         list of clients.  The message can be no longer than 58
         bytes; longer messages will be truncated.

         The server returns a list of status codes in Send Status
         indicating the result of trying to log the message for
         delivery to the target client(s).

         A success status (0x00) indicates that the message has
         been recorded for later delivery.

         A failure status (0xFF) indicates that the target client
         does not exist or has signaled the server not to accept
         messages.

         A message pending status (0xFC) indicates that the server
         is already holding a message for delivery to the target
         client and cannot accept another.

         This call can be used by any client.

ARGS: <> pAccess
      >  buNumClients
      >  pbuClientBuf
      >  buMsgLen
      >  pbstrMsg
      <  pbuNumStatus
      <  pbuSendStatusBuf

INCLUDE: ncpmsg.h

RETURN:  0x0000  Successful
         0x89FD  Bad Station Number
         0x89FF  Failure

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     21 01  Get Broadcast Message

NCP:     21 00  Send Broadcast Message

CHANGES: 29 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP21s0MsgSendBroadcast
(
   pNWAccess pAccess,
   nuint8   buNumClients,
   pnuint8  pbuClientBuf,
   nuint8   buMsgLen,
   pnstr8   pbstrMsg,
   pnuint8  pbuNumStatus,
   pnuint8  pbuSendStatusBuf
)
{
   #define NCP_FUNCTION    ((nuint) 21)
   #define NCP_SUBFUNCTION ((nuint) 0)
   #define NCP_STRUCT_LEN  ((nuint16) (3 + buNumClients + buMsgLen))
   #define MAX_CLIENTS     ((nuint) 200)
   #define MAX_MSG_LEN     ((nuint) 58)
   #define REQ1_LEN        ((nuint) 4)
   #define REQ2_LEN        ((nuint) 1 + MAX_MSG_LEN)
   #define REQ2_FRAG_LEN   ((nuint) (1 + buMsgLen))
   #define REQ_FRAGS       ((nuint) 3)
   #define REPLY_FRAGS     ((nuint) 2)

   nuint8  abuReq1[REQ1_LEN], abuReq2[REQ2_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   suNCPLen   = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq1[0], &suNCPLen);
   abuReq1[2] = NCP_SUBFUNCTION;
   abuReq1[3] = buNumClients;

   abuReq2[0] = buMsgLen;
   NWCMemMove(&abuReq2[1], pbstrMsg, buMsgLen);

   reqFrag[0].pAddr = abuReq1;
   reqFrag[0].uLen  = REQ1_LEN;

   reqFrag[1].pAddr = pbuClientBuf;
   reqFrag[1].uLen  = buNumClients;

   reqFrag[2].pAddr = abuReq2;
   reqFrag[2].uLen  = REQ2_FRAG_LEN;

   replyFrag[0].pAddr = pbuNumStatus;
   replyFrag[0].uLen  = (nuint) 1;

   replyFrag[1].pAddr = pbuSendStatusBuf;
   replyFrag[1].uLen  = MAX_CLIENTS;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/21s0.c,v 1.7 1994/09/26 17:33:26 rebekah Exp $
*/
