/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s210.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s210GetPath*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s210ClearConn
         (
            pNWAccess pAccess,
            nuint8   luConnNum,
         )

REMARKS: This call logs out the specified station.  Any resources or files the station
         is using are released and the station's service connection is broken.  The
         station must re-establish a service connection and log in before it can
         resume work on the file server.

         If the calling station lacks supervisor privileges, the No Console Rights
         completion code is returned and the target station is not affected.

         This call is equivalent to typing "CLEAR #" at the NetWare v2.x file
         server console; the pound symbol (#) represents the connection number
         of the station to be cleared.


ARGS: <> pAccess
      >  luConnNum

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x89C6  No Console Rights
         0x89FD  Bad Station Number

SERVER:  DOS OS2 WIN

CLIENT:  2.0 3.11 4.0

SEE:     23 254 Clear Connection Number

NCP:     23 210 Clear Connection Number (old)

CHANGES: 14 Sep 1993 - written - anevarez
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s210ClearConn
(
   pNWAccess pAccess,
   nuint8   luConnNum
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        254)
   #define NCP_STRUCT_LEN     ((nuint16)         2)
   #define NCP_REQ_LEN        ((nuint)           4)
   #define NCP_REQ_FRAGS      ((nuint)           1)
   #define NCP_REP_FRAGS      ((nuint)           0)

   nuint8 abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);

   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = luConnNum;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   return (NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
            NCP_REP_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s210.c,v 1.7 1994/09/26 17:36:06 rebekah Exp $
*/
