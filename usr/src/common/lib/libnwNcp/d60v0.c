/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d60v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS60v0FinishAuthen
         (
            pNWAccess pNetAccess,
            nuint32  luEncryptedSessionKeyLen,
            pnuint8  pEncryptedSessionKey,
            nuint32  luCredentialLen,
            pnuint8  pCredential,
            nuint32  luProofLen,
            pnuint8  pProof
         )

REMARKS:
/ *
DSV_FINISH_AUTHENTICATION                       60 (0x3C)   0

Request

  nuint32   version;
  ????      encryptedSessionKey;
  ????      credential;
  ????      proof;

Response

  Returns only the NCP header. ????
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
NWNCPDS60v0FinishAuthen
(
   pNWAccess pNetAccess,
   nuint32  luEncryptedSessionKeyLen,
   pnuint8  pEncryptedSessionKey,
   nuint32  luCredentialLen,
   pnuint8  pCredential,
   nuint32  luProofLen,
   pnuint8  pProof
)
{
   nuint8   buReq1[11]; /* 4 + 4 + 3 (pad) */
   nuint8   buReq2[7]; /* 4 + 3 (pad) */
   nuint8   buReq3[7]; /* 4 + 3 (pad) */
   NWCFrag  reqFrag[6];
   pnuint8  cur;

   cur = buReq1;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &luEncryptedSessionKeyLen);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 8;

   reqFrag[1].pAddr = pEncryptedSessionKey;
   reqFrag[1].uLen = NPad32((nuint)luEncryptedSessionKeyLen);

   cur = buReq2;
   NAlign32(&cur);

   NCopyLoHi32(&cur[0], &luCredentialLen);

   reqFrag[2].pAddr = cur;
   reqFrag[2].uLen = 4;

   reqFrag[3].pAddr = pCredential;
   reqFrag[3].uLen = NPad32((nuint)luCredentialLen);

   cur = buReq3;
   NAlign32(&cur);

   NCopyLoHi32(&cur[0], &luProofLen);

   reqFrag[4].pAddr = cur;
   reqFrag[4].uLen = 4;

   reqFrag[5].pAddr = pProof;
   reqFrag[5].uLen = (nuint)luProofLen;


   return NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)60, (nuint)6,
            reqFrag, (nuint)0, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d60v0.c,v 1.7 1994/09/26 17:41:18 rebekah Exp $
*/
