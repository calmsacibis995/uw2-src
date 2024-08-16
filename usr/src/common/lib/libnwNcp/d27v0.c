/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:d27v0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpds.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCPDS27v0OpenStream
         (
            pNWAccess pNetAccess,
            nflag32  flFlags,
            nuint32  luEntryID,
            nuint32  luAttrNameLen,
            pnstr16  pwstrAttrName,
            pnuint32 pluFileID,
            pnuint32 pluFileSize
         )

REMARKS:
/ *

DSV_OPEN_STREAM                                 27 (0x1B)


Request

  nuint32   version;
  nuint32   flags;
  nuint32   entryID;
  unicode   attrName;

Response

  nuint32 fileID;
  nuint32 fileSize;

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
NWNCPDS27v0OpenStream
(
   pNWAccess pNetAccess,
   nflag32  flFlags,
   nuint32  luEntryID,
   nuint32  luAttrNameLen,
   pnstr16  pwstrAttrName,
   pnuint32 pluFileID,
   pnuint32 pluFileSize
)
{
   nuint8   buReq[19];  /* 4 + 4 + 4 + 4 + 3 (pad) */
   nuint8   buRep[11];  /* 4 + 4 + 3 (pad) */
   NWCFrag  reqFrag[2];
   NWCFrag  repFrag[1];
   pnuint8  cur;
   nuint    szActRep;
   NWRCODE  err;

   cur = buReq;
   NAlign32(&cur);

   /* version */
   *(pnuint32)&cur[0] = (nuint32)0;
   NCopyLoHi32(&cur[4], &flFlags);
   NCopyLoHi32(&cur[8], &luEntryID);
   NCopyLoHi32(&cur[12], &luAttrNameLen);

   reqFrag[0].pAddr = cur;
   reqFrag[0].uLen = 16;

   reqFrag[1].pAddr = pwstrAttrName;
   reqFrag[1].uLen = NPad32((nuint)luAttrNameLen);

   cur = buRep;
   NAlign32(&cur);

   repFrag[0].pAddr = cur;
   repFrag[0].uLen = 8;

   err =  NWCFragmentRequest(pNetAccess, (nuint)2, (nuint32)27, (nuint)2,
            reqFrag, (nuint)1, repFrag, &szActRep);
   if (err)
      return (err);

   NCopyLoHi32(pluFileID, &cur[0]);
   NCopyLoHi32(pluFileSize, &cur[4]);

   return (err);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/d27v0.c,v 1.7 1994/09/26 17:40:51 rebekah Exp $
*/
