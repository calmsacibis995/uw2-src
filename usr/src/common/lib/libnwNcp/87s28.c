/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s28.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s28GetFullPath**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s28GetFullPath
         (
            pNWAccess         pAccess,
            nuint8           buSrcNamSpc,
            nuint8           buDstNamSpc,
            pNWNCPCompPath   pCompPath,
            pNWNCPPathCookie pPathCookie,
            pnuint16         psuPathCompSize,
            pnuint16         psuPathCompCnt,
            pnuint8          pbuPathComponentsB512,
         );

REMARKS: The path is returned in reverse order, with the root being the last
         component, the current directory being the first.

ARGS: <> pAccess
      >  buSrcNamSpc
      >  buDstNamSpc
      >  pCompPath
      <> pOutPathCookie
         Initially, Cookie1 & Cookie2 are set to -1. When Cookie2 comes back
         in the reply as -1, all path components have been returned. If bit 0
         of the Flags field is set to 1, then the last component is a file
         name.

            struct PathCookie
            {
               word Flags;
               long Cookie1;
               long Cookie2;
            };

      <  psuPathCompSize
         Total byte length of the path components in the reply packet.

      <  psuPathCompCnt
         Number of components in the reply packet.

      <  pbuPathComponentsB512
         The path compents. Each component is a (byte) length preceeded
         string. These strings are not zero terminated.


INCLUDE: ncpfile.h

RETURN:  0x0000    SUCCESSFUL

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     87 28  Get Full Path String

CHANGES: 14 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s28GetFullPath
(
   pNWAccess         pAccess,
   nuint8           buSrcNamSpc,
   nuint8           buDstNamSpc,
   pNWNCPCompPath   pCompPath,
   pNWNCPPathCookie pPathCookie,
   pnuint16         psuPathCompSize,
   pnuint16         psuPathCompCnt,
   pnuint8          pbuPathComponentsB512
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 28)
   #define REQ_LEN         ((nuint) 13)
   #define REPLY_LEN       ((nuint) 14)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 2)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buSrcNamSpc;
   abuReq[2] = buDstNamSpc;
   NCopyLoHi16(&abuReq[3], &pPathCookie->suFlags);
   NCopyLoHi32(&abuReq[5], &pPathCookie->luCookie1);
   NCopyLoHi32(&abuReq[9], &pPathCookie->luCookie2);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pCompPath->abuPacked;
   reqFrag[1].uLen  = pCompPath->suPackedLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = pbuPathComponentsB512;
   replyFrag[1].uLen  = (nuint) 512;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if(lCode == 0)
   {
      NCopyLoHi16(&pPathCookie->suFlags,   &abuReply[0]);
      NCopyLoHi32(&pPathCookie->luCookie1, &abuReply[2]);
      NCopyLoHi32(&pPathCookie->luCookie2, &abuReply[6]);
      NCopyLoHi16(psuPathCompSize, &abuReply[10]);
      NCopyLoHi16(psuPathCompCnt,  &abuReply[12]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s28.c,v 1.7 1994/09/26 17:39:33 rebekah Exp $
*/
