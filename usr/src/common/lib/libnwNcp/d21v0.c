/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d21v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS21v0RemovePartition
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            nuint32  luPartitionRootID
         )

REMARKS:
/ *
DSV_REMOVE_PARTITION                            21 (0x15)


Request

  nuint32 version;
  nuint32 flags;
  nuint32 partitionRootID;

Response

  Returns only the NCP header.

Remarks

This verb allows you to remove the master replica of a partition.

This transaction removes an existing partition from the Directory. The partition must be
completely empty or the deletion will fail. There must be no other replicas than the master.
An empty partition contains no subordinate entry entries or subordinate partition entries. This
request is issued by at the client agent to the server controlling the master replica of the
partition.

If the call succeeds, an NCP header with the zero completion code is returned to the client.

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
NWNCPDS21v0RemovePartition
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luPartitionRootID
)
{
   nuint8   buReq[15];  /* 4 + 4 + 4 + 3 (padding) */
   NWCFrag  reqFrag[1];
   pnuint8  cur;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &flFlags);
   NCopyLoHi32(&cur[8], &luPartitionRootID);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 12;

   return NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)21, (nuint)1,
      reqFrag, (nuint)0, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d21v0.c,v 1.7 1994/09/26 17:40:44 rebekah Exp $
*/
