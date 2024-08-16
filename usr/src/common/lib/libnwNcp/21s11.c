/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:21s11.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpmsg.h"

/*manpage*NWNCP21s11MsgGetBroadcast*********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP21s11MsgGetBroadcast
         (
            pNWAccess pAccess,
            pnuint8  pbuMsgLen,
            pnuint8  pbuMsg,
         );

REMARKS:

ARGS: <> pAccess
       < pbuMsgLen
       < pbuMsg

INCLUDE: ncpmsg.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     Send Broadcast Message (0x2222  21  0)

NCP:     21 11  Get Broadcast Message

CHANGES: 13 Aug 1993 - written - srydalch
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP21s11MsgGetBroadcast
(
   pNWAccess pAccess,
   pnuint8  pbuMsgLen,
   pnuint8  pbuMsg
)
{
   #define NCP_FUNCTION    ((nuint) 21)
   #define NCP_SUBFUNCTION ((nuint8) 11)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define MAX_MSG_LEN     ((nuint) 255)
   #define REQ_LEN         ((nuint) 3)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 2)

   nuint16  suNCPLen;
   nuint8   abuReq[REQ_LEN];
   NWCFrag  reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;

   NCopyHiLo16(&abuReq[0], &suNCPLen);

   abuReq[2] = NCP_SUBFUNCTION;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = (nptr)  pbuMsgLen;
   replyFrag[0].uLen  = (nuint) 1;

   replyFrag[1].pAddr = (nptr)  pbuMsg;
   replyFrag[1].uLen  = MAX_MSG_LEN;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/21s11.c,v 1.7 1994/09/26 17:33:30 rebekah Exp $
*/
