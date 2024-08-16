/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:21s9.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpmsg.h"

/*manpage*NWNCP21s9BroadcastToConsole***************************************
SYNTAX:  N_GLOBAL_LIBRARY( nint32 )
         NWNCP21s9BroadcastToConsole
         (
            pNWAccess pAccess,
            nuint8   buMsgLen,
            pnstr8   pbstrMsg,
         )

REMARKS:

ARGS: <  pAccess
      >  buMsgLen
      <  pbuMsg

INCLUDE: ncpmsg.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     21 9  Broadcast To Console

CHANGES: 10 Aug 1993 - written - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP21s9BroadcastToConsole
(
   pNWAccess pAccess,
   nuint8   buMsgLen,
   pnuint8  pbstrMsg
)
{
   #define NCP_FUNCTION    ((nuint) 21)
   #define NCP_SUBFUNCTION ((nuint8) 9)
   #define NCP_STRUCT_LEN  ((nuint16) (2 + buMsgLen))
   #define REQ_LEN         ((nuint) 4)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8 buReq[REQ_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;

   NCopyHiLo16(&buReq[0], &suNCPLen);
   buReq[2] = NCP_SUBFUNCTION;
   buReq[3] = buMsgLen;

   reqFrag[0].pAddr = buReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrMsg;
   reqFrag[1].uLen  = (nuint) buMsgLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/21s9.c,v 1.7 1994/09/26 17:33:40 rebekah Exp $
*/
