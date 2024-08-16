/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:104s1.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpds.h"


/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCP104s1PingServer
         (
            pNWAccess pAccess,
            pnuint   psuReplyLen,
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
NWNCP104s1PingServer
(
   pNWAccess pAccess,
   pnuint   psuReplyLen,
   pnuint8  pbuReplyData
)
{
   nint32   lCode;
   nuint8   abuReq[4];
   nuint8   abuRep[100];

   abuReq[0] = (nuint8) 1;    /* subfunction */
   abuReq[1] = (nuint8) 0;
   abuReq[2] = (nuint8) 0;
   abuReq[3] = (nuint8) 0;

   lCode = NWCRequestSingle(pAccess, (nuint)104, abuReq, (nuint)1, abuRep,
                            *psuReplyLen, psuReplyLen);
   if (lCode == 0)
   {
      if (pbuReplyData)
         NWCMemMove(pbuReplyData, abuRep, *psuReplyLen);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/104s1.c,v 1.7 1994/09/26 17:31:31 rebekah Exp $
*/
