/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d8v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS8v0RemoveObject
         (
            pNWAccess pNetAccess,
            nuint32  luEntryID
         )

REMARKS:
/ *
DSV_REMOVE_ENTRY                                8 (0x08)


Request

  nuint32 version;
  nuint32 entryID;

Response

  Returns only the NCP header.

Remarks

This verb allows you to remove a leaf entry from the Directory.

The entryID should be obtained as a result of a Resolve Name transaction.

If the call succeeds, only the NCP header with a zero completion code is returned.

Please see the DSV_MODIFY_RDN for a description of how the alias dereferencing works.

 Check to see if we need to implement this one
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
NWNCPDS8v0RemoveObject
(
   pNWAccess pNetAccess,
   nuint32  luEntryID
)
{
   nuint8   buReq[11];  /* 4 + 4 + 3 (padding) */
   NWCFrag  reqFrag[1];
   pnuint8  cur;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &luEntryID);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 8;

   return NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)8, (nuint)1,
            reqFrag, (nuint)0, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d8v0.c,v 1.7 1994/09/26 17:41:29 rebekah Exp $
*/
