/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:104s4.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpds.h"


/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCP104s4GetBindContext
         (
            pNWAccess pAccess,
            nuint    suReplyDataLen,
            pnuint8  pbuReplyData,
         )

REMARKS:

ARGS:

INCLUDE:

RETURN:

SERVER:

CLIENT:

SEE:

NCP:

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY(NWRCODE)
NWNCP104s4GetBindContext
(
   pNWAccess pAccess,
   nuint    suReplyDataLen,
   pnuint8  pbuReplyData
)
{
   nuint8   abuReq[1];
   nuint    suReplySize;

   abuReq[0] = (nuint8) 4;    /* subfunction */

   return (NWCRequestSingle(pAccess, (nuint)104, abuReq, (nuint)1, pbuReplyData,
                            suReplyDataLen, &suReplySize));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/104s4.c,v 1.7 1994/09/26 17:31:53 rebekah Exp $
*/
