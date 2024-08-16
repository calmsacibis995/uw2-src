/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d22v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS22v0ListPartitions
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            pnuint32 pluIterationHandle,
            pnuint8  pPartitionsInfo,
            pnstr16  pwstrServerName,
            pnuint32 pluPartitionInfoLen,
            pnstr8   pPartitionInfo
         )

REMARKS:
/ *
DSV_LIST_PARTITIONS                             22 (0x16)


Request

  nuint32 version;
  nuint32 flags;
  nuint32 iterationHandle;

Response

  nuint32         iterationHandle;
  unicode         serverName;
  PartitionInfo   partitions[];//defined below

Definitions of Parameter Types

  struct  PartitionInfo
  {
     unicode   partitionName;
     nuint32   replicaType;
  };

Remarks

This verb allows you to list the partitions available on a server.

In some cases a client agent may have the address of a name server but have no other
information about that server. This can happen, for example, if the server address was
obtained using the IPX/SAP Get Nearest Server capability. The List Partitions request allows
the client to obtain the distinguished name of the server.

The server name is returned, as the client agent may not yet know the Directory Service name
of this server.

A reference is returned for each partition stored on the server. The partition reference
includes the partition name and the type of replica stored on the server.

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
NWNCPDS22v0ListPartitions
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   pnuint32 pluIterationHandle,
   pnuint32 pluPartitionInfoLen,
   pnstr8   pPartitionInfo
)
{
   nuint8   buReq[15];  /* 4 + 4 + 4 + 3 (padding) */
   nuint8   buRep[7];   /* 4 + 3 (padding) */
   NWCFrag  reqFrag[1];
   NWCFrag  repFrag[2];
   nuint    uActualReplySize;
   pnuint8  cur;
   NWRCODE  err;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &flFlags);
   NCopyLoHi32(&cur[8], pluIterationHandle);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 12;

   cur = buRep;
   NAlign32(&cur);

   repFrag[0].pAddr = cur;
   repFrag[0].uLen = 4;

   repFrag[1].pAddr = pPartitionInfo;
   repFrag[1].uLen = (nuint)*pluPartitionInfoLen;

   err = NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)22, (nuint)1,
            reqFrag, (nuint)2, repFrag, &uActualReplySize);

   if (err < 0)
      return err;

   NCopyLoHi32(pluIterationHandle, cur);
   *pluPartitionInfoLen = repFrag[1].uLen;

   return err;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d22v0.c,v 1.7 1994/09/26 17:40:45 rebekah Exp $
*/
