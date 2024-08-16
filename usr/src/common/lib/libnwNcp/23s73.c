/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s73.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s73IsObjManager**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s73IsObjManager
         (
            pNWAccess pAccess
         );

REMARKS: This function checks to see if the calling station has manager privileges
         in the bindery.  A station is a manager if it is a supervisor or if it
         appears in the MANAGERS property of the supervisor object.

ARGS: <> pAccess

INCLUDE: ncpbind.h

RETURN:  0x0000     Calling Station is a Manager
         0x89FF     Calling Station is Not a Manager

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 60  Scan Property
         23 61  Read Property Value

NCP:     23 73  Is Calling Station a Manager

CHANGES: 13 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s73IsObjManager
(
   pNWAccess pAccess
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 73)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   return NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s73.c,v 1.7 1994/09/26 17:37:36 rebekah Exp $
*/
