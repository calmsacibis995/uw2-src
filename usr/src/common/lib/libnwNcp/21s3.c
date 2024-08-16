/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:21s3.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpmsg.h"

/*manpage*NWNCP21s3EnableBroadcasts************************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCP21s3EnableBroadcasts
         (
            pNWAccess pAccess,
         )

REMARKS: Allows a client to enable message reception after
         message reception has been disabled.

ARGS: <> pAccess

INCLUDE: nwmsg.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     21 03  Enable Broadcasts

CHANGES: 13 Aug 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY(NWRCODE)
NWNCP21s3EnableBroadcasts
(
   pNWAccess pAccess
)
{
   #define NCP_FUNCTION    ((nuint) 21)
   #define NCP_SUBFUNCTION ((nuint8) 3)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];
   nuint16 suTemp;

   suTemp= NCP_STRUCT_LEN;
   NCopyHiLo16(abuReq, &suTemp);
   abuReq[2]= NCP_SUBFUNCTION;

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN, NULL,
               REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/21s3.c,v 1.7 1994/09/26 17:33:32 rebekah Exp $
*/
