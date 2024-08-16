/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d59v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS59v0BeginAuthen
         (
            pNWAccess pNetAccess,
            nuint32  luEntryID,
            nuint32  luClientRandom,
            pnuint32 pluServerRandom,
            pnuint32 pluEncryptedClientRandomLen,
            pnuint8  pEncryptedClientRandom
         )

REMARKS:
/ *
DSV_BEGIN_AUTHENTICATION                        59 (0x3B)   0

Request

  nuint32   version;
  nuint32   entryID;
  nuint32   clientRandom;

Response

  nuint32   serverRandom;
  nuint32   encryptedClientRandom;
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
NWNCPDS59v0BeginAuthen
(
   pNWAccess pNetAccess,
   nuint32  luEntryID,
   nuint32  luClientRandom,
   pnuint32 pluServerRandom,
   pnuint32 pluEncryptedClientRandomLen,
   pnuint8  pEncryptedClientRandom
)
{
   nuint8   buReq[15]; /* 4 + 4 + 4 + 3 (pad) */
   nuint8   buRep[7];  /* 4 + 3 (pad) */
   NWCFrag  reqFrag[1];
   NWCFrag  repFrag[2];
   pnuint8  cur;
   nuint    uActRepSize;
   NWRCODE  err;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;

   NCopyLoHi32(&cur[4], &luEntryID);
   NCopyLoHi32(&cur[8], &luClientRandom);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 12;

   cur = buRep;
   NAlign32(&cur);

   repFrag[0].pAddr = cur;
   repFrag[0].uLen = 4;

   repFrag[1].pAddr = pEncryptedClientRandom;
   repFrag[1].uLen = (nuint)*pluEncryptedClientRandomLen;

   err = NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)59, (nuint)1,
            reqFrag, (nuint)2, repFrag, &uActRepSize);

   if (err < 0)
      return (err);

   NCopyLoHi32(pluServerRandom, &cur[0]);
   /* take off the 4 bytes for the length that precedes the data */
   *pluEncryptedClientRandomLen = repFrag[1].uLen - 4;

   return (err);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d59v0.c,v 1.7 1994/09/26 17:41:15 rebekah Exp $
*/
