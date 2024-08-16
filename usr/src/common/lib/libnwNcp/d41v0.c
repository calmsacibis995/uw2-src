/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d41v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS41v0GetReplicaRootID
         (
            pNWAccess pNetAccess,
            nuint32  luEntryID,
            pnuint32 pluPartitionID
         )

REMARKS:
/ *
DSV_GET_REPLICA_ROOT_ID                         41 (0x29)


Request

  nuint32 version;
  nuint32 entryID;

Response

  nuint32 partitionID;

  Returns the partitionID of the given entry; if the entry is a partition root, it returns the
  name of the entry itself.

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
NWNCPDS41v0GetReplicaRootID
(
   pNWAccess pNetAccess,
   nuint32  luEntryID,
   pnuint32 pluPartitionID
)
{
   nuint8   buReq[11];  /* 4 + 4 + 3 (padding) */
   nuint8   buRep[7];   /* 4 + 3 (padding) */
   NWCFrag  reqFrag[1];
   NWCFrag  repFrag[1];
   nuint    uActualReplySize;
   pnuint8  cur;
   NWRCODE  err;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &luEntryID);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 8;

   cur = buRep;
   NAlign32(&cur);

   repFrag[0].pAddr = cur;
   repFrag[0].uLen = 4;

   err = NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)41, (nuint)1,
            reqFrag, (nuint)1, repFrag, &uActualReplySize);

   if (err < 0)
      return err;

   cur = repFrag[0].pAddr;
   NCopyLoHi32(pluPartitionID, &cur[0]);

   return err;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d41v0.c,v 1.7 1994/09/26 17:40:59 rebekah Exp $
*/
