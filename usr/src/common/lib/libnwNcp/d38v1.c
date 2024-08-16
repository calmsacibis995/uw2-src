/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d38v1.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS38v1SyncPartition
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            nuint32  luSeconds,
            nuint32  luPartitionDNLen,
            pnstr16  pwstrPartitionDN
         )

REMARKS:
/ *
DSV_SYNC_PARTITION                              38 (0x26)


Request

  nuint32   version;
  nuint32   flags;
  nuint32   seconds;
  unicode partitionDN;

Response

  Returns only the NCP header.

Remarks

This verb allows you to initiate synchronization on a partition.

The synchronization is the central algorithm for maintaining the structure of the Directory.
Synchronizations are done on a per-partition basis rather than on a per-server basis.

If this request succeeds, an NCP header with a zero completion code is returned.

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
NWNCPDS38v1SyncPartition
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luSeconds,
   nuint32  luPartitionDNLen,
   pnstr16  pwstrPartitionDN
)
{
   nuint8   buReq[535]; /* 4 + 4 + 4 + 4 + 514 + 2 + 3 (padding) */
   NWCFrag  reqFrag[1];
   pnuint8  cur;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)1;
   NCopyLoHi32(&cur[4], &flFlags);
   NCopyLoHi32(&cur[8], &luSeconds);
   NCopyLoHi32(&cur[12], &luPartitionDNLen);
   NWCMemMove(&cur[16], pwstrPartitionDN, (nuint)luPartitionDNLen);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 16 + NPad32((nuint)luPartitionDNLen);

   return NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)38, (nuint)1,
            reqFrag, (nuint)0, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d38v1.c,v 1.7 1994/09/26 17:40:55 rebekah Exp $
*/
