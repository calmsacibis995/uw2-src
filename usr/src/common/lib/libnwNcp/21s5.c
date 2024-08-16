/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:21s5.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpmsg.h"

/*manpage*NWNCP21s5MsgGetPersonal*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP21s5MsgGetPersonal
         (
            pNWAccess pAccess,
            pnuint8  pbuSendingConnNum,
            pnuint8  pbuMsgLen,
            pnuint8  pbuMsg,
         );

REMARKS: This function allows a client to retrieve a message
         addressed to it by some other client.  If the server is
         holding more than one message addressed to the client making
         this call, the "oldest" message is retrieved.  A Failure
         Completion Code indicates that the server has no messages to
         deliver.

ARGS: <> pAccess
      <  pbuSendingConnNum
      <  pbuMsgLen
      <  pbuMsg

INCLUDE: ncpmsg.h

RETURN:
         0x00  Successful
         0xFD  Bad Station Number
         0xFF  Failure

SERVER:  NetWare 286 v2.0 through v2.15

CLIENT:  DOS OS2 WIN

SEE:

NCP:     21 05  Get Personal Message

CHANGES: 16 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP21s5MsgGetPersonal
(
   pNWAccess pAccess,
   pnuint8  pbuSendingConnNum,
   pnuint8  pbuMsgLen,
   pnuint8  pbuMsg
)
{
   #define NCP_FUNCTION    ((nuint) 21)
   #define NCP_SUBFUNCTION ((nuint8) 5)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define MAX_MSG_LEN     ((nuint) 126)
   #define REQ_LEN         ((nuint) 3)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 3)

   nuint8  abuReq[REQ_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);

   abuReq[2] = NCP_SUBFUNCTION;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = pbuSendingConnNum;
   replyFrag[0].uLen  = (nuint) 1;

   replyFrag[1].pAddr = pbuMsgLen;
   replyFrag[1].uLen  = (nuint) 1;

   replyFrag[2].pAddr = pbuMsg;
   replyFrag[2].uLen  = MAX_MSG_LEN;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/21s5.c,v 1.7 1994/09/26 17:33:35 rebekah Exp $
*/
