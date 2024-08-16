/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d43v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS43v0FinishMoveEntry
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            nuint32  luSourceID,
            nuint32  luDestinationParentID,
            nuint32  luNewRDNLen,
            pnstr16  pwstrNewRDN,
            nuint32  luDestinationServerDNLen,
            pnstr16  pwstrDestinationServerDN,
         )

REMARKS:
/ *
DSV_FINISH_MOVE_ENTRY                           43 (0x2B)


Request

  nuint32   version;
  nuint32   flags;
  nuint32   sourceID;
  nuint32   destinationParentID;
  unicode   newRDN;
  unicode   destinationServerDN;

Response

  Returns only the NCP header.

Remarks

This request goes to the master of the source.

This verb allows you to finish moving a leaf entry from one partition to another in the
Directory.

Upon success, only the NCP header with a zero completion code is returned.

The destinationParentID should be obtained as a result of a Resolve Name transaction.

The moved entry is created as a subordinate of the specified parent entry, and is placed in the
same partition as the parent entry. (To create a subordinate partition, use Add Partition or
Split Partition.)

The newRDN of the moved entry is specified relative to the parent entry.

This request always resolves to the master of the destination parent.

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
NWNCPDS43v0FinishMoveEntry
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luSourceID,
   nuint32  luDestinationParentID,
   nuint32  luNewRDNLen,
   pnstr16  pwstrNewRDN,
   nuint32  luDestinationServerDNLen,
   pnstr16  pwstrDestinationServerDN
)
{
   nuint8   buReq[803]; /* 4 + 4 + 4 + 4 + 4 + 258 + 2 + 4 + 514 + 2 + 3 (padding) */
   NWCFrag  reqFrag[1];
   nuint    uCurPos;
   pnuint8  cur;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &flFlags);
   NCopyLoHi32(&cur[8], &luSourceID);
   NCopyLoHi32(&cur[12], &luDestinationParentID);
   NCopyLoHi32(&cur[16], &luNewRDNLen);
   NWCMemMove(&cur[20], pwstrNewRDN, (nuint)luNewRDNLen);
   uCurPos = 20 + NPad32((nuint)luNewRDNLen);
   NCopyLoHi32(&cur[uCurPos], &luDestinationServerDNLen);
   uCurPos += 4;
   NWCMemMove(&cur[uCurPos], pwstrDestinationServerDN, (nuint)luDestinationServerDNLen);


   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = uCurPos + NPad32((nuint)luDestinationServerDNLen);

   return NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)43, (nuint)1,
            reqFrag, (nuint)0, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d43v0.c,v 1.7 1994/09/26 17:41:02 rebekah Exp $
*/
