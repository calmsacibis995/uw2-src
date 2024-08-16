/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s254.c	1.6"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s254ClearConn**********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s254ClearConn
         (
            pNWAccess pAccess,
            nuint32  luConnNum,
         )

REMARKS: This NCP logs out the specified station.  Any resources or files
         the station is using are released and the station's service
         connection is broken.  The station must re-establish a service
         connection and log in before it can resume work on the server.

         If the colling station lacks supervisor privileges, the No Console
         Rights completion code is returned and the target station is not
         affected.

         This call is equivalent to typing "CLEAR #" at the NetWare v2.x
         file server console; the pound symbol (#) represents the
         connection number of the station to be cleared.

ARGS: <> pAccess
      >  luConnNum

INCLUDE: ncpserve.h

RETURN:  0x0000  Successful
         0x89C6  No Console Rights
         0x89FD  Bad Station Number

SERVER:  DOS OS2 WIN NT

CLIENT:  3.11 4.0

SEE:     23 210  Clear Connection Number (old)

NCP:     23 254  Clear Connection Number

CHANGES: 14 Sep 1993 - written - anevarez
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s254ClearConn
(
   pNWAccess pAccess,
   nuint32  luConnNum
)
{
   #define NCP_FUNCTION       ((nuint)     23)
   #define NCP_SUBFUNCTION    ((nuint8)   254)
   #define NCP_STRUCT_LEN     ((nuint16)    5)
   #define NCP_REQ_LEN        ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN      ((nuint)      0)

   nuint8 abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo32(&abuReq[3], &luConnNum);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
            NULL, NCP_REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s254.c,v 1.8 1994/09/28 06:26:14 rebekah Exp $
*/
