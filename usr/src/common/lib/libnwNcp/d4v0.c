/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d4v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS4v0Compare
         (
            pNWAccess pNetAccess,
            nuint32  luEntryID,
            nuint32  luCompDataLen,
            pnuint8  pCompData,
            pnbool8  pMatched
         )

REMARKS:
/ *
DSV_COMPARE                                     4 (0x04)


Request

  nuint32   version;
  nuint32   entryID;
  nuint32   count;      //should be 1
  unicode attrName;
  nuint32   count;      //should be 1
  any_t   attrValue;

Response

  boolean matched;

Remarks

This verb allows you to compare a value with the value(s) of a particular attribute in a
particular entry.

The entryID should be obtained as a result of a Resolve Name transaction.

The attribute syntax must support comparison for equality. If it does not, an
ERR_INVALID_COMPARISON is returned.

If the request succeeds (i.e. the comparison is actually carried out), the result will be returned
in the matched boolean.

Check to see if we need to implement this one ????

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
NWNCPDS4v0Compare
(
   pNWAccess pNetAccess,
   nuint32  luEntryID,
   nuint32  luCompDataLen,
   pnuint8  pCompData,
   pnbool8  pMatched
)
{
   nuint8   buReq[11];   /* 4 + 4 + 3 */
   nuint8   buRep[7];    /* 4 + 4 + 3 */
   NWCFrag  reqFrag[2];
   NWCFrag  repFrag[1];
   pnuint8  cur;
   nuint    uActRepSize;
   NWRCODE  err;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &luEntryID);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 8;

   reqFrag[1].pAddr = pCompData;
   reqFrag[1].uLen = (nuint)luCompDataLen;

   cur = buRep;
   NAlign32(&cur);

   repFrag[0].pAddr = cur;
   repFrag[0].uLen = 4;


   err = NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)4, (nuint)2,
            reqFrag, (nuint)1, repFrag, &uActRepSize);
   if (err < 0)
      return (err);

   *pMatched = (nbool8)cur[0];
   return (err);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d4v0.c,v 1.7 1994/09/26 17:41:06 rebekah Exp $
*/
