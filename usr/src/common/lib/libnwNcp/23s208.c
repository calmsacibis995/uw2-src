/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s208.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncptts.h"

/*manpage*NWNCP23s208EnableTracking******************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP23s208EnableTracking
         (
            pNWAccess pAccess,
         );

REMARKS: Re-enables the Transaction Tracking functions of an SFT file
         server.  The Transaction Tracking system can be disabled manually
         using the Disable Transaction Tracking call, or automatically by
         the server when the transaction volume becomes full.  After
         freeing space on the transaction volume for the transaction files,
         the operator can use this call to re-enable Transaction Tracking.

         If the calling station does not have operator privileges, the No
         Console Rights completion code is returned.  The server's
         transaction status remains unchanged.

ARGS: <> pAccess

INCLUDE: ncptts.h

RETURN:  0x8900   Successful
         0x89C6   No Console Rights

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN

SEE:     23 207  Enable Transaction Tracking
         23 213  Get Transaction Tracking Statistics

NCP:     23 208  Enable Transaction Tracking

CHANGES: 19 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s208EnableTracking
(
   pNWAccess pAccess
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        208)
   #define NCP_STRUCT_LEN     ((nuint16)         1)
   #define NCP_REQ_LEN        ((nuint)           3)
   #define NCP_REP_LEN        ((nuint)           0)

   nuint8  abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   return( NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN, NULL,
                            NCP_REP_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s208.c,v 1.7 1994/09/26 17:36:02 rebekah Exp $
*/
