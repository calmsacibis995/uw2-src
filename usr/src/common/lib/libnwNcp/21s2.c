/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:21s2.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpmsg.h"

/*manpage*NWNCP21s2DisableBroadcasts******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP21s2DisableBroadcasts
         (
            pNWAccess pAccess,
         )

REMARKS: Informs the server that a client does not want to receive messages
         from other clients. After receiving this command, the server will
         refuse to let other clients log messages for forwarding to this
         client.

ARGS: <> pAccess

INCLUDE: ncpmsg.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     21 03  Enable Broadcasts

NCP:     21 02  Disable Broadcasts

CHANGES: 13 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP21s2DisableBroadcasts
(
   pNWAccess pAccess
)
{
   #define NCP_FUNCTION    ((nuint) 21)
   #define NCP_SUBFUNCTION ((nuint8) 2)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 0)

   nuint8   abuReq[REQ_LEN];
   nuint16  suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;

   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   return ( (NWRCODE) NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
                        NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/21s2.c,v 1.7 1994/09/26 17:33:31 rebekah Exp $
*/
