/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d26v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS26v0RemoveReplica
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            nuint32  luPartitionRootID,
            pnstr16  pwstrTargetServerDN
         )

REMARKS:
/ *
DSV_REMOVE_REPLICA                              26 (0x1A)


Request

  nuint32   version;
  nuint32   flags;
  nuint32   partitionRootID;
  unicode   targetServerDN;

Response

  Returns only the NCP header.

Remarks

This verb allows you to remove a replica of a partition.

It may be used to remove any replica except the last (master) replica of a partition. (That
replica is removed by a Remove Partition transaction.) This request is issued by the client
agent to the server holding the master replica. The targetServerDN indicates the name of the
server from which the replica is to be removed.

An NCP header with the completion code is returned to the client.


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
NWNCPDS26v0RemoveReplica
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luPartitionRootID,
   nuint32  luTargetServerDNLen,
   pnstr16  pwstrTargetServerDN
)
{
   nuint8   buReq[535]; /* 4 + 4 + 4 + 4 + 514 + 2 + 3 (padding) */
   NWCFrag  reqFrag[1];
   pnuint8  cur;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &flFlags);
   NCopyLoHi32(&cur[8], &luPartitionRootID);
   NCopyLoHi32(&cur[12], &luTargetServerDNLen);
   NWCMemMove(&cur[16], pwstrTargetServerDN, (nuint)luTargetServerDNLen);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 16 + NPad32((nuint)luTargetServerDNLen);

   return NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)26, (nuint)1,
            reqFrag, (nuint)0, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d26v0.c,v 1.7 1994/09/26 17:40:50 rebekah Exp $
*/
