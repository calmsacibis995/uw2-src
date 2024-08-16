/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d20v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS20v0AddPartition
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            nuint32  luIterationHandle,
            nuint32  luParentID,
            nuint32  luPartitionRDNLen,
            pnstr16  pwstrPartitionRDN,
            nuint32  luTargetServerDNLen,
            pnstr16  pwstrTargetServerDN,
            nuint32  luAddPartitionLen,
            pnuint8  pAddPartitionData
         )

REMARKS:
/ *
DSV_ADD_PARTITION                               20 (0x14)


Request

  nuint32   version;
  nuint32   flags;
  nuint32   iterationHandle;
  nuint32   parentID;
  unicode   partitionRDN;
  unicode   targetServerDN;
  Attribute Attributeinfo[];      //defined below

Response

  Returns only the NCP header.

Definitions of Parameter Types

  struct  Attribute
  {
     unicode attrName;
     any_t   attrValue[];
  };

Remarks

This verb allows you to create the root entry in a new partition

This initial partition replica will always be of type RT_MASTER.

The info contains the attribute information, which combined with the partitionRDN,
constitutes the entry to be created. The Directory shall ensure that the entry conforms to the
Directory schema. Partition root entries may not be aliases.

This request always resolves to the master of the parent entry. The master partition will be
created on the server named in targetServerDN, which for 4.x must be the same as the master
of the parent.
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
NWNCPDS20v0AddPartition
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luIterationHandle,
   nuint32  luParentID,
   nuint32  luPartitionRDNLen,
   pnstr16  pwstrPartitionRDN,
   nuint32  luTargetServerDNLen,
   pnstr16  pwstrTargetServerDN,
   nuint32  luAddPartitionLen,
   pnuint8  pAddPartitionData
)
{
   nuint8   buReq1[283];   /* 4 + 4 + 4 + 4 + 4 + 258 + 2 + 3 (padding) */
   nuint8   buReq2[527];   /* 4 + 514 + 2 + 4 + 3 (padding) */
   NWCFrag  reqFrag[3];
   nuint    uCurPos;
   pnuint8  cur;

   cur = buReq1;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &flFlags);
   NCopyLoHi32(&cur[8], &luIterationHandle);
   NCopyLoHi32(&cur[12], &luParentID);
   NCopyLoHi32(&cur[16], &luPartitionRDNLen);
   NWCMemMove(&cur[20], pwstrPartitionRDN, (nuint)luPartitionRDNLen);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 20 + NPad32((nuint)luPartitionRDNLen);

   cur = buReq2;
   NAlign32(&cur);

   NCopyLoHi32(&cur[0], &luTargetServerDNLen);
   NWCMemMove(&cur[4], pwstrTargetServerDN, (nuint)luTargetServerDNLen);
   uCurPos = 4 + NPad32((nuint)luTargetServerDNLen);
   NCopyLoHi32(&cur[uCurPos], &luAddPartitionLen);

   reqFrag[1].pAddr = cur;
   reqFrag[1].uLen = uCurPos + 4;

   reqFrag[2].pAddr = pAddPartitionData;
   reqFrag[2].uLen = NPad32((nuint)luAddPartitionLen);

   return NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)20, (nuint)3,
      reqFrag, (nuint)0, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d20v0.c,v 1.7 1994/09/26 17:40:42 rebekah Exp $
*/
