/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d78v0s1.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS78v0s1PartFuncRcvUpd
         (
            pNWAccess pNetAccess,
            nuint32  luReplicaRootID,
            nuint32  luTargetServerID
         )

REMARKS:
/ *
DSV_PARTITION_FUNCTION                          78 (0x4E)

Request

  nuint32   version;
  nuint32   subFunction;
  nuint32   replicaRootID;
  nuint32   targetServerID;

Response

  Returns only the NCP header.
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
NWNCPDS78v0s1PartFuncRcvUpd
(
   pNWAccess pNetAccess,
   nuint32  luReplicaRootID,
   nuint32  luTargetServerID
)
{
   nuint8   buReq[19];  /* 4 + 4 + 4 + 4 + 3 (padding) */
   NWCFrag  reqFrag[1];
   pnuint8  cur;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   *(pnuint32)&cur[4] = (nuint32)1;
   NCopyLoHi32(&cur[8], &luReplicaRootID);
   NCopyLoHi32(&cur[12], &luTargetServerID);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 16;

   return NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)78, (nuint)1,
            reqFrag, (nuint)0, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d78v0s1.c,v 1.7 1994/09/26 17:41:25 rebekah Exp $
*/