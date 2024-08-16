/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:15.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP15AllocResource**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP15AllocResource
         (
            pNWAccess pAccess,
            nuint8   buResNum,
         );

REMARKS: This call allocates a resource.  A resource number from 0 to 255 can be
         specified.

ARGS: <> pAccess
      >  buResNum

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x89FF   Failure

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     16 --  Deallocate A Resource

NCP:     15 --  Allocate A Resource

CHANGES: 28 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP15AllocResource
(
   pNWAccess pAccess,
   nuint8   buResNum
)
{
   #define NCP_FUNCTION    ((nuint) 15)
   #define REQ_LEN         ((nuint) 1)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];

   abuReq[0] = buResNum;

   return(NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/15.c,v 1.7 1994/09/26 17:33:08 rebekah Exp $
*/
