/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:36s2.c	1.1"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpext.h"

/*manpage*NWNCP36s2ScanLoadedExtsByName***************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP36s2ScanLoadedExtsByName
         (
            pNWAccess    pAccess,
            nuint8      buSrcNameLen,
            pnstr8      pbstrSrcExtNameB32,
            pnuint32    pluNCPExtID,
            pnuint8     pbuMajorVer,
            pnuint8     pbuMinorVer,
            pnuint8     pbuRev,
            pnuint8     pbuExtNameLen,
            pnstr8      pbstrExtNameB32,
            pnuint8     pbuQueryDataB32,
         )

REMARKS: Scans currently loaded NCP extensions

ARGS: <> pAccess,
      >  buSrcNameLen,
      >  pbstrSrcExtNameB32,
      <> pluNCPExtID,
      <  pbuMajorVer,        (optional)
      <  pbuMinorVer,        (optional)
      <  pbuRev,             (optional)
      <  pbuExtNameLen,      (optional)
      <  pbstrExtNameB32,    (optional)
      <  pbuQueryDataB32,    (optional)

INCLUDE: ncpext.h

RETURN:  n/a

SERVER:  3.0 3.11

CLIENT:  DOS WIN OS2

SEE:     36 05  Get NCP Extension Info
         36 00  Scan Currently Loaded NCP Extensions

NCP:     36 02  Scan Currently Loaded NCP Extensions By Name (3.11)

CHANGES: 21 Sep 1993 - written (no documentation, so written from NWCALLS)
                               - dromrell
         24 June 1994 - Changed NCP 33 to 36.
                               - CJenkins
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP36s2ScanLoadedExtsByName
(
   pNWAccess    pAccess,
   nuint8      buSrcNameLen,
   pnstr8      pbstrSrcExtNameB32,
   pnuint32    pluNCPExtID,
   pnuint8     pbuMajorVer,
   pnuint8     pbuMinorVer,
   pnuint8     pbuRev,
   pnuint8     pbuExtNameLen,
   pnstr8      pbstrExtNameB32,
   pnuint8     pbuQueryDataB32
)
{
   #define NCP_FUNCTION    ((nuint) 36)
   #define NCP_SUBFUNCTION ((nuint8) 2)
   #define NCP_STRUCT_LEN  ((nuint16) (2 + buSrcNameLen))
   #define EXT_NAME_LEN    ((nuint) 32)
   #define QUERY_DATA_LEN  ((nuint) 32)
   #define REQ_LEN         ((nuint) 4)
   #define REPLY_LEN       ((nuint) 8)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 3)

   nint32   lCode;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint16 suNCPLen;
   nuint8 abuReq[REQ_LEN], abuRep[REPLY_LEN], abuBucket[EXT_NAME_LEN + QUERY_DATA_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0],&suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buSrcNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrSrcExtNameB32;
   reqFrag[1].uLen  = (nuint) buSrcNameLen;

   replyFrag[0].pAddr = abuRep;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = pbstrExtNameB32 ? (pnptr)pbstrExtNameB32 : 
										(pnptr)&abuBucket[0];
   replyFrag[1].uLen  = EXT_NAME_LEN;

   replyFrag[2].pAddr = pbuQueryDataB32 ? pbuQueryDataB32 : &abuBucket[EXT_NAME_LEN];
   replyFrag[2].uLen  = QUERY_DATA_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      if(pluNCPExtID)
            NCopyLoHi32(pluNCPExtID,&abuRep[0]);

      if(pbuMajorVer)
            *pbuMajorVer = abuRep[4];

      if(pbuMinorVer)
            *pbuMinorVer = abuRep[5];

      if(pbuRev)
            *pbuRev = abuRep[6];

      if(pbuExtNameLen)
            *pbuExtNameLen = abuRep[7];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/36s2.c,v 1.1 1994/09/26 17:38:33 rebekah Exp $
*/
