/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d53v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS53v0GetServerAddr
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            pnuint32 pluServerInfoLen,
            pnuint8  pbuServerInfo
         )

REMARKS:
/ *
DSV_GET_SERVER_ADDRESS                          53 (0x35)


Request

  nuint32 version;
  nuint32 flags;

Response

  unicode   dn;
  Referral  referral[]; //defined below

Definitions of Parameter Types

  struct  Referral
  {
     nuint32type;    //address type
     int  address[];
  }


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
NWNCPDS53v0GetServerAddr
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   pnuint32 pluServerInfoLen,
   pnuint8  pbuServerInfo
)
{
   nuint8   buReq[11];  /* 4 + 4 + 3 (padding) */
   NWCFrag  reqFrag[1];
   NWCFrag  repFrag[1];
   nuint    uActualReplyLen;
   pnuint8  cur;
   NWRCODE  err;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &flFlags);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 8;

   repFrag[0].pAddr = pbuServerInfo;
   repFrag[0].uLen = (nuint)*pluServerInfoLen;

   err = NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)53, (nuint)1,
            reqFrag, (nuint)1, repFrag, &uActualReplyLen);

   if (err < 0)
      return err;

   *pluServerInfoLen = repFrag[0].uLen;

   return err;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d53v0.c,v 1.7 1994/09/26 17:41:08 rebekah Exp $
*/
