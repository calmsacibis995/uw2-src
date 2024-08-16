/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:21s4.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpmsg.h"

/*manpage*NWNCP21s4MsgSendPersonal********************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCP21s4MsgSendPersonal
         (
            pNWAccess pAccess,
            nuint8   buNumClients,
            pnuint8  pbuClientBuf,
            nuint8   buMsgLen,
            pnstr8   pbstrMsg,
            pnuint8  pbuNumStatus,
            pnuint8  pbuSendStatusBuf,
         )

REMARKS: This function allows a client to send a message through a
         communication pipe to another client.  Personal messages
         cannot exceed 60 bytes;  messages longer than this will be
         truncated.  The message can be sent to multiple clients at
         once.  The server returns a list of send status codes in
         Send Status.

         A success status (0x00) indicates that the message was
         successfully entered into the pipe.

         A failure status (0xFF) indicates that no pipe half
         exists from the calling client to the target client.

         An incomplete pipe status (0xFE) indicates that no pipe
         half exists from the target client to the calling client.

         A pipe full status (0xFD) indicates that the message pipe
         is full and the server is unable queue the message.

         This function can be used by any client.

         This function is not supported

ARGS: <> pAccess
      <  buNumClients
      <  pbuClientBuf
      <  buMsgLen
      <  pbstrMsg
      >  pbuNumStatus
      >  pbuSendStatusBuf


INCLUDE: nwmsg.h

RETURN:  0x89ff  Failure - no pipe half exists from calling client
         0x89fe  Failure - no pipe half exists to destination client
         0x89fd  Failure - message queue full

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     21 04  Send Personal Message

CHANGES: 25 Aug 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY(NWRCODE)
NWNCP21s4MsgSendPersonal
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
   #define NCP_SUBFUNCTION ((nuint8) 4)
   #define NCP_STRUCT_LEN  ((nuint16) (3 + buNumClients + buMsgLen))
   #define REQ_LEN         ((nuint) 4)
   #define REQ_FRAGS       ((nuint) 4)
   #define REPLY_FRAGS     ((nuint) 2)

   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint16 suSubFunStructLen;
   nuint8  abuReq[REQ_LEN];

   suSubFunStructLen= NCP_STRUCT_LEN;

   NCopyHiLo16 (abuReq, &suSubFunStructLen);

   abuReq[2]= NCP_SUBFUNCTION;

   abuReq[3]= buNumClients;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbuClientBuf;
   reqFrag[1].uLen  = buNumClients;

   reqFrag[2].pAddr = &buMsgLen;
   reqFrag[2].uLen  = (nuint) 1;

   reqFrag[3].pAddr = pbstrMsg;
   reqFrag[3].uLen  = buMsgLen;

   replyFrag[0].pAddr = pbuNumStatus;
   replyFrag[0].uLen  = (nuint) 1;

   replyFrag[1].pAddr = pbuSendStatusBuf;
   replyFrag[1].uLen  = buNumClients;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/21s4.c,v 1.7 1994/09/26 17:33:33 rebekah Exp $
*/
