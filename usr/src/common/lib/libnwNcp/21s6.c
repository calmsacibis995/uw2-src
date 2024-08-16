/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:21s6.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpmsg.h"

/*manpage*NWNCP21s6MsgOpenPipe**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP21s6MsgOpenPipe
         (
            pNWAccess pAccess,
            nuint8   buNumClients,
            pnuint8  pbuClientBuf,
            pnuint8  pbuNumStatus,
            pnuint8  pbuPipeStatusBuf,
         );

REMARKS: This function allows a client to open communication pipes
         with a list of other clients.  This request creates half of
         a communication pipe to each client specified in the Target
         Client List Field;  before messages can be exchanged on a
         pipe, each client intending to communicate must offer half a
         pipe to the other client(s) with which it wishes to exchange
         messages.  Once two clients have offered half a pipe to each
         other, the communication pipe can be completed by the server
         and messages can be passed.

         The server attempts to create a pipe to each client in the
         list, and returns a list of status codes in Pipe Status
         indicating the status of each pipe that the server tried to
         construct.

         A failure status (0xFF) indicates that the client
         specified at the corresponding offset in the Target
         Client List does not exist or is not logged in.

         An incomplete pipe status (0xFE) indicates that the
         targeted client exists but has not yet tried to open a
         pipe to the requesting client.

         A success status (0x00) indicates that the communication
         pipe is now complete and the client can begin sending and
         receiving messages.

         This function can be used by all clients.

ARGS: <> pAccess
      >  buNumClients
      >  pbuClientBuf
      <  pbuNumStatus
      <  pbuPipeStatusBuf

INCLUDE: ncpmsg.h

RETURN:  0x0000  Successful

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     21 06  Open Message Pipe

CHANGES: 29 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP21s6MsgOpenPipe
(
   pNWAccess pAccess,
   nuint8   buNumClients,
   pnuint8  pbuClientBuf,
   pnuint8  pbuNumStatus,
   pnuint8  pbuPipeStatusBuf
)
{
   #define NCP_FUNCTION    ((nuint) 21)
   #define NCP_SUBFUNCTION ((nuint) 6)
   #define NCP_STRUCT_LEN  ((nuint16) (2 + buNumClients))
   #define MAX_CLIENTS     ((nuint) 200)
   #define REQ_LEN         ((nuint) 4)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 2)

   nuint8  abuReq[REQ_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buNumClients;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbuClientBuf;
   reqFrag[1].uLen  = buNumClients;

   replyFrag[0].pAddr = pbuNumStatus;
   replyFrag[0].uLen  = (nuint) 1;

   replyFrag[1].pAddr = pbuPipeStatusBuf;
   replyFrag[1].uLen  = MAX_CLIENTS;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/21s6.c,v 1.7 1994/09/26 17:33:36 rebekah Exp $
*/
