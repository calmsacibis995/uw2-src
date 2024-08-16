/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d58v2.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS58v2FinishLogin
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            nuint32  luEntryID,
            nuint32  luEncryptedDataLen,
            pnuint8  pEncryptedData,
            pnuint32 pBeginLoginTime,
            pnuint32 pLogoutTime,
            pnuint32 pluEncryptedResponseLen,
            pnuint8  pEncryptedResponse
         )

REMARKS:
/ *
DSV_FINISH_LOGIN                                58 (0x3A)

Request

  nuint32   version;
  nuint32   flags;
  nuint32   entryID;
  ????      encryptedData;

Response

  nuint32   beginLoginTime;
  nuint32   LogoutTime;
  ????      encryptedResponse

* /

ARGS:

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
NWNCPDS58v2FinishLogin
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luEntryID,
   nuint32  luEncryptedDataLen,
   pnuint8  pEncryptedData,
   pnuint32 pBeginLoginTime,
   pnuint32 pLogoutTime,
   pnuint32 pluEncryptedResponseLen,
   pnuint8  pEncryptedResponse
)
{
   nuint8   buReq[19];  /* 4 + 4 + 4 + 4 + 3 (padding) */
   nuint8   buRep[11];  /* 4 + 4 + 3 (padding) */
   NWCFrag  reqFrag[2];
   NWCFrag  repFrag[2];
   pnuint8  cur;
   nuint    suActualReplySize;
   NWRCODE  err;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)2;
   NCopyLoHi32(&cur[4], &flFlags);
   NCopyLoHi32(&cur[8], &luEntryID);
   NCopyLoHi32(&cur[12], &luEncryptedDataLen);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 16;

   reqFrag[1].pAddr = pEncryptedData;
   reqFrag[1].uLen = (nuint)luEncryptedDataLen;

   cur = buRep;
   NAlign32(&cur);

   repFrag[0].pAddr = cur;
   repFrag[0].uLen = 8;

   repFrag[1].pAddr = pEncryptedResponse;
   repFrag[1].uLen = (nuint)*pluEncryptedResponseLen;

   err = NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)58, (nuint)2,
            reqFrag, (nuint)2, repFrag, &suActualReplySize);

   if (err < 0)
      return (err);

   if (pBeginLoginTime != NULL)
      NCopyLoHi32(pBeginLoginTime, &cur[0]);

   if (pLogoutTime != NULL)
      NCopyLoHi32(pLogoutTime, &cur[4]);

   *pluEncryptedResponseLen = repFrag[1].uLen;

   return (err);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d58v2.c,v 1.7 1994/09/26 17:41:14 rebekah Exp $
*/
