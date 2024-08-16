/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d61.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS61Logout
         (
            pNWAccess pNetAccess
         )

REMARKS:
/ *
DSV_LOGOUT                                      61 (0x3D)

Request

  Only the NCP header.

Response

  Returns only the NCP header.
* /

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
NWNCPDS61Logout
(
   pNWAccess pNetAccess
)
{
   NWCFrag  reqFrag[1];

   reqFrag[0].pAddr = NULL;
   reqFrag[0].uLen = 0;

   return NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)61, (nuint)1,
            reqFrag, (nuint)0, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d61.c,v 1.7 1994/09/26 17:41:19 rebekah Exp $
*/
