/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d57v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS57v0BeginLogin
         (
            pNWAccess pNetAccess,
            nuint32  luEntryID,
            pnuint32 pluPseudoID,
            pnuint32 pluServerRandomSeed
         )

REMARKS:
/ *
DSV_BEGIN_LOGIN                                 57 (0x39)

Request

  nuint32   version;
  nuint32   entryID;

Response

  nuint32   pseudoID;
  nuint32   serverRandomSeed;
* /

ARGS:    <> pNetAccess
         >  luEntryID
         <  pluPseudoID
         <  pluServerRandomSeed

INCLUDE:

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

N_GLOBAL_LIBRARY(NWRCODE)
NWNCPDS57v0BeginLogin
(
   pNWAccess pNetAccess,
   nuint32  luEntryID,
   pnuint32 pluPseudoID,
   pnuint32 pluServerRandomSeed
)
{
   nuint8   buReq[11];  /* 4 + 4 + 3 (padding) */
   nuint8   buRep[11];  /* 4 + 4 + 3 (padding) */
   NWCFrag  reqFrag[1];
   NWCFrag  replyFrag[1];
   pnuint8  cur;
   NWRCODE  err;
   nuint    suActualReplyLen;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &luEntryID);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 8;

   cur = buRep;
   NAlign32(&cur);

   replyFrag[0].pAddr = cur;
   replyFrag[0].uLen = 8;

   err = NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)57, (nuint)1,
            reqFrag, (nuint)1, replyFrag, &suActualReplyLen);

   if (err < 0)
      return(err);

   NCopyLoHi32(pluPseudoID, &cur[0]);
   NCopyLoHi32(pluServerRandomSeed, &cur[4]);

   return(err);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d57v0.c,v 1.7 1994/09/26 17:41:12 rebekah Exp $
*/
