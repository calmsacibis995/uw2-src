/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:86s1.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpea.h"

/*manpage*NWNCP86s1EACloseHandle********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP86s1EACloseHandle
         (
            pNWAccess pAccess,
            nuint16  buReserved,
            nuint32  luEAHandle
         );

REMARKS:

ARGS: <  pAccess
      >  buReserved
      >  luEAHandle

INCLUDE: ncpea.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     86 01  Close Extended Attribute Handle

CHANGES: 10 Aug 1993 - written - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP86s1EACloseHandle
(
   pNWAccess pAccess,
   nuint16  suReserved,
   nuint32  luEAHandle
)
{
   #define NCP_FUNCTION    ((nuint) 86)
   #define NCP_SUBFUNCTION ((nuint8) 1)
   #define REQ_LEN         ((nuint) 7)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];

   abuReq[0] = NCP_SUBFUNCTION;

   NCopyLoHi16(&abuReq[1], &suReserved);
   NCopyLoHi32(&abuReq[3], &luEAHandle);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
                     NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/86s1.c,v 1.7 1994/09/26 17:39:06 rebekah Exp $
*/
