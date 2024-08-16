/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d25v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS25v0AddReplica
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            nuint32  luPartitionRootID,
            nuint32  luReplicaType,
            nuint32  luTargetServerNameLen,
            pnstr16  pwstrTargetServerName
         )

REMARKS:
/ *
DSV_ADD_REPLICA                                 25 (0x19)


Request

  nuint32   version;
  nuint32   flags;
  nuint32   partitionRootID;
  nuint32   replicaType;
  unicode   targetServerName;

Response

  Returns only the NCP header.

Remarks

This verb allows you to add a replica of an existing partition to the server named in
targetServerName. Only replicas of type RT_SECONDARY and RT_READONLY may be
added by this transaction. This request always resolves to the master of the partition root.

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
NWNCPDS25v0AddReplica
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luPartitionRootID,
   nuint32  luReplicaType,
   nuint32  luTargetServerNameLen,
   pnstr16  pwstrTargetServerName
)
{
   nuint8   buReq[539]; /* 4 + 4 + 4 + 4 + 4 + 514 + 2 + 3 (padding) */
   NWCFrag  reqFrag[1];
   pnuint8  cur;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &flFlags);
   NCopyLoHi32(&cur[8], &luPartitionRootID);
   NCopyLoHi32(&cur[12], &luReplicaType);
   NCopyLoHi32(&cur[16], &luTargetServerNameLen);
   NWCMemMove(&cur[20], pwstrTargetServerName, (nuint)luTargetServerNameLen);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 20 + NPad32((nuint)luTargetServerNameLen);

   return NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)25, (nuint)1,
      reqFrag, (nuint)0, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d25v0.c,v 1.7 1994/09/26 17:40:49 rebekah Exp $
*/
