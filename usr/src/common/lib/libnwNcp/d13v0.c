/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d13v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS13v0RemoveAttrDef
         (
            pNWAccess pNetAccess,
            nuint32  luAttrNameLen,
            pnstr16  pwstrAttrName
         )

REMARKS:
/ *
DSV_REMOVE_ATTR_DEF                             13 (0x0D)


Request

  nuint32   version;
  unicode attrName;

Response

  Returns only the NCP header.

Remarks

This verb allows you to delete an attribute definition from the schema.

An attribute definition can be deleted only if it is not in use in any class definitions.

The attrName identifies the attribute definition to be deleted.

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
NWNCPDS13v0RemoveAttrDef
(
   pNWAccess pNetAccess,
   nuint32  luAttrNameLen,
   pnstr16  pwstrAttrName
)
{
   nuint8   buReq[79]; /* 4 + 4 + 66 + 2 + 3 (pad) */
   NWCFrag  reqFrag[1];
   pnuint8  cur;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &luAttrNameLen);
   NWCMemMove(&cur[8], pwstrAttrName, (nuint)luAttrNameLen);

   reqFrag[0].pAddr= cur;
   reqFrag[0].uLen = 8 + NPad32((nuint)luAttrNameLen);

   return NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)13, (nuint)1,
            reqFrag, (nuint)0, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d13v0.c,v 1.7 1994/09/26 17:40:32 rebekah Exp $
*/
