/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s200.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s200CheckConsPriv******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s200CheckConsPriv
         (
            pNWAccess pAccess,
         );

REMARKS: This call allows a station to determine whether it has console
         privileges on the target server.  If it does, the Successful
         completion code will be returned.  If it does not, the No Console
         Rights completion code will be returned.

ARGS: <> pAccess

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 200  Check Console Privileges

CHANGES: 17 Aug 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
*****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s200CheckConsPriv
(
   pNWAccess pAccess
)
{
   #define NCP_FUNCTION      ((nuint)     23)
   #define NCP_SUBFUNCTION   ((nuint8)   200)
   #define NCP_STRUCT_LEN    ((nuint16)    1)

   return (NWCRequestSimple(pAccess, NCP_FUNCTION, NCP_STRUCT_LEN,
               NCP_SUBFUNCTION));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s200.c,v 1.7 1994/09/26 17:35:53 rebekah Exp $
*/
