/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d16v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS16v0ModifyClassDef
         (
            pNWAccess pNetAccess,
            nuint32  luClassNameLen,
            pnstr16  pwstrClassName,
            nuint32  luOptionalAttrsLen,
            pnuint8  pOptionalAttrs
         )

REMARKS:
/ *

DSV_MODIFY_CLASS_DEF                            16 (0x10)


Request

  nuint32   version;
  unicode   className;
  unicode   optionalAttrs[];

Response

  Returns only the NCP header.

Remarks

This verb allows you to change a class definition in the domain schema.

This operation is used to add additional optional attributes to an existing class definition.

The following request parameters are defined:

The className identifies the class to be modified.

The optionalAttrs gives the attribute names which are to be added as additional optional
attributes for the class.

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
NWNCPDS16v0ModifyClassDef
(
   pNWAccess pNetAccess,
   nuint32  luClassNameLen,
   pnstr16  pwstrClassName,
   nuint32  luOptionalAttrsLen,
   pnuint8  pOptionalAttrs
)
{
   nuint8   buReq[79]; /* 4 + 4 + 66 + 2 + 3 (padding) */
   NWCFrag  reqFrag[2];
   pnuint8  cur;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &luClassNameLen);
   NWCMemMove(&cur[8], pwstrClassName, (nuint)luClassNameLen);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 8 + NPad32((nuint)luClassNameLen);

   reqFrag[1].pAddr = pOptionalAttrs;
   reqFrag[1].uLen = (nuint)luOptionalAttrsLen;

   return NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)16, (nuint)2,
            reqFrag, (nuint)0, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d16v0.c,v 1.7 1994/09/26 17:40:36 rebekah Exp $
*/
