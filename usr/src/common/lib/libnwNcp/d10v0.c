/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d10v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"


/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS10v0ModifyRDN
         (
            pNWAccess pNetAccess,
            nuint32  luEntryID,
            nuint32  blDelOldRDN,
            nuint32  luNewRDNLen,
            pnstr16  pwstrNewRDN
         )

REMARKS:
/ *
DSV_MODIFY_RDN                                  10 (0x0A)


Request

  nuint32   version;
  nuint32   entryID;
  boolean   delOldRDN;
  pnstr16   newRDN;

Response

  Returns only the NCP header.

Remarks

This verb allows you to change the rdn (relative distinguished name) of an entry in the
Directory.

The entryID should be obtained as a result of a Resolve Name transaction. The entryID
identifies the entry whose RDN is to be modified.

The newRDN specifies the new RDN of the entry. If an attribute value in the new RDN does
not already exist in the entry (either as a part of the old RDN or as a non-distinguished value)
it is added. If it cannot be added, an error is returned except in the use of alias. It is only
legal to have the naming value in an alias, not more values of the naming attribute.

The dereferencing of aliases is handled in the following manner when resolving the name:

1)Dereference of alias is requested: The name is divided into 2 parts, parent and child. The
  parent name is resolved (requesting dereferencing). The child is concatenated with the
  dereferenced parent name.

2)Dereference of alias NOT requested: The name is resolved requesting that the name is not
  dereferenced.

If the delOldRDN flag is set, all attribute values in the old RDN which are not in the new
RDN are deleted.

If the delOldRDN flag is not set, the old attribute values should remain in the entry as
multiple values of the attribute (not as a part of the new RDN). If this operation removes the
last value of an attribute, that attribute shall be deleted.
This verb returns an error if requested to retain an old RDN on a single-valued attribute.

Partition names can be modified by this request. The transaction agent must propagate the
name change to the parent partition's subordinate reference.

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
NWNCPDS10v0ModifyRDN
(
   pNWAccess pNetAccess,
   nuint32  luEntryID,
   nuint32  blDelOldRDN,
   nuint32  luNewRDNLen,
   pnstr16  pwstrNewRDN
)
{
   nuint8   buReq[279]; /* 4 + 4 + 4 + 4 + 258 + 2 + 3 (padding) */
   NWCFrag  reqFrag[1];
   pnuint8  cur;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;

   NCopyLoHi32(&cur[4], &luEntryID);
   NCopyLoHi32(&cur[8], &blDelOldRDN);
   NCopyLoHi32(&cur[12], &luNewRDNLen);
   NWCMemMove(&cur[16], pwstrNewRDN, (nuint)luNewRDNLen);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 16 + NPad32((nuint)luNewRDNLen);

   return NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)10, (nuint)1,
      reqFrag, (nuint)0, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d10v0.c,v 1.7 1994/09/26 17:40:27 rebekah Exp $
*/
