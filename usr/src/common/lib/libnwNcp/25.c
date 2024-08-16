/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:25.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpconn.h"

/*manpage*NWNCP25Logout**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP25Logout
         (
            pNWAccess pAccess,
         )

REMARKS: This function allows a client to relinquish its current server access
         privileges without breaking its service connection.

ARGS: <> pAccess

INCLUDE: ncpconn.h

RETURN:  0x0000    Successful

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 00  Login User (old)
         23 20  Login Object

NCP:     25 --  Logout

CHANGES: 4 Oct 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP25Logout
(
   pNWAccess pAccess
)
{
   #define NCP_FUNCTION    ((nuint) 25)
   #define NCP_REQ_LEN     ((nuint) 0)
   #define NCP_REPLY_LEN   ((nuint) 0)

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, NULL, NCP_REQ_LEN,
               NULL, NCP_REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/25.c,v 1.7 1994/09/26 17:37:40 rebekah Exp $
*/
