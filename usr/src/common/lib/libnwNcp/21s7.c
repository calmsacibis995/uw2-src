/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:21s7.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpmsg.h"

/*manpage*NWNCP21s7CloseMsgPipe**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP21s7CloseMsgPipe
         (
            pNWAccess pAccess,
            nuint8   buNumClients,
            pnuint8  pbuClientBuf,
            pnuint8  pbuNumStatus,
            pnuint8  pbuPipeStatusBuf,
         );

REMARKS: This function allows a client to close pipes that it has
         previously opened with other clients.  When a client closes
         its half of a message pipe, any messages that have been
         waiting in the pipe from the client that owned the other
         pipe end are lost.

         Pipe Status is a list of status codes returned for each
         corresponding client in the request's Target Client List.  A
         success status (0x00) indicates that the pipe was
         successfully closed.  A failure status (0xFF) indicates that
         no pipe exists between the calling client and the specified
         client.

         This function can be used by any client.

ARGS: <> pAccess
      >  buNumClients
      >  pbuClientBuf
      >  pbuNumStatus
      >  pbuPipeStatusBuf

INCLUDE: ncpmsg.h

RETURN:  0x0000  Successful
         0x89FD  Bad Station Number

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     21 07  Close Message Pipe

CHANGES: 29 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP21s7CloseMsgPipe
(
   pNWAccess pAccess,
   nuint8   buNumClients,
   pnuint8  pbuClientBuf,
   pnuint8  pbuNumStatus,
   pnuint8  pbuPipeStatusBuf
)
{
   #define NCP_FUNCTION    ((nuint) 21)
   #define NCP_SUBFUNCTION ((nuint) 7)
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
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/21s7.c,v 1.7 1994/09/26 17:33:37 rebekah Exp $
*/
