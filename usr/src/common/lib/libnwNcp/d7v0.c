/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d7v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS7v0AddObject
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            nuint32  luParentID,
            nuint32  luRDNLen,
            pnstr16  pwstrRDN,
            nuint32  luAddObjectLen,
            pnuint8  pAddObjectData
         )

REMARKS:
/ *
DSV_ADD_ENTRY                                   07 (0x07)


Request

  nuint32   version;
  nuint32   flags;
  nuint32   parentID;
  unicode   rdn;
  any_t     Attribute,

Response

  Returns only the NCP header.

Definitions of Parameter Types

   struct  Attribute
   {
      unicode  attrName;
      nuint32  numAttrs;
      any_t    attrValue[];
   };

Remarks

This verb allows you to add a leaf entry to the Directory.

The parentID should be obtained as a result of a Resolve Name transaction. The new entry is
created as a subordinate of the specified parent entry, and is placed in the same partition as
the parent entry. (To create a subordinate partition, use Add Partition or Split Partition.)

The rdn of the new entry is specified relative to the parent entry.

The info contains the attribute information, which combined with the rdn, constitutes the entry
to be created. The Directory ensures that the entry conforms to the Directory schema.
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
NWNCPDS7v0AddObject
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luParentID,
   nuint32  luRDNLen,
   pnstr16  pwstrRDN,
   nuint32  luAddObjectLen,
   pnuint8  pAddObjectData
)
{
   nuint8   buReq[535];   /* 4 + 4 + 4 + 4 + 514 + 2 + 3 (padding) */
   NWCFrag  reqFrag[2];
   pnuint8  cur;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &flFlags);
   NCopyLoHi32(&cur[8], &luParentID);
   NCopyLoHi32(&cur[12], &luRDNLen);
   NWCMemMove(&cur[16], pwstrRDN, (nuint)luRDNLen);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 16 + NPad32((nuint)luRDNLen);

   reqFrag[1].pAddr = pAddObjectData;
   reqFrag[1].uLen = (nuint)luAddObjectLen;

   return NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)7, (nuint)2,
      reqFrag, (nuint)0, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d7v0.c,v 1.7 1994/09/26 17:41:27 rebekah Exp $
*/
