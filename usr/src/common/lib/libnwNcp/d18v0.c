/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d18v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS18v0ListContClasses
         (
            pNWAccess pNetAccess,
            pnuint32 pluIterationHandle,
            nuint32  luParentEntryID,
            pnuint32 pluContainableClassesLen,
            pnuint8  pContainableClasses
         )

REMARKS:
/ *
DSV_LIST_CONTAINABLE_CLASSES                    18 (0x12)


Request

  nuint32   version;
  nuint32   iterationHandle;
  nuint32   parentEntryID;

Response

  nuint32   iterationHandle;
  unicode   containableClasses[];

Remarks

This verb returns the set of classes which may be contained in the Directory subordinate to an
entry of the specified class.

The following request parameters are defined:

The iterationHandle in the request would be 0xFFFFFFFF if this is the initial message in an
iteration. Subsequent requests should supply the iteration ID returned in the previous
response.

The parentEntryID identifies the entry, relative to which information is to be returned.

The following response parameters are defined:

The iterationHandle in the response is set to 0xFFFFFFFF if this message contains the
remainder of the response. Otherwise, this ID can be used in subsequent List Containable
Classes requests to obtain further portions of the response.

The containableClasses lists the entry classes which may be created as subordinates of the
specified entry in the Directory.

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
NWNCPDS18v0ListContClasses
(
   pNWAccess pNetAccess,
   pnuint32 pluIterationHandle,
   nuint32  luParentEntryID,
   pnuint32 pluContainableClassesLen,
   pnuint8  pContainableClasses
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
   NCopyLoHi32(&cur[4], pluIterationHandle);
   NCopyLoHi32(&cur[8], &luParentEntryID);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 12;

   cur = buRep;
   NAlign32(&cur);

   /* iteration handle */
   repFrag[0].pAddr = cur;
   repFrag[0].uLen = 4;

   repFrag[1].pAddr = pContainableClasses;
   repFrag[1].uLen = NPad32((nuint)*pluContainableClassesLen);

   err = NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)18, (nuint)1,
            reqFrag, (nuint)2, repFrag, &uActualReplySize);

   if (err < 0)
      return err;

   cur = repFrag[0].pAddr;
   NCopyLoHi32(pluIterationHandle, &cur[0]);

   *pluContainableClassesLen = repFrag[1].uLen;

   return err;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d18v0.c,v 1.7 1994/09/26 17:40:38 rebekah Exp $
*/
