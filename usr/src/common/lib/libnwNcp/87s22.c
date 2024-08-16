/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s22.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s22GenDirBaseVolNum**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s22GenDirBaseVolNum
         (
            pNWAccess       pAccess,
            nuint8         buNamSpc,
            pnuint8        pbuReservedB3,
            pNWNCPCompPath pCompPath,
            pnuint32       pluNSDirBase,
            pnuint32       pluDOSDirBase,
            pnuint8        pbuVolNum,
         )

REMARKS:

ARGS: <> pAccess
      >  buNamSpc
      >  pbuReservedB3
      >  pCompPath
      <  pluNSDirBase
      <  pluDOSDirBase
      <  pbuVolNum

INCLUDE: ncpfile.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     87 22  Generate Directory Base And Volume Number

CHANGES: 14 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s22GenDirBaseVolNum
(
   pNWAccess       pAccess,
   nuint8         buNamSpc,
   pnuint8        pbuReservedB3,
   pNWNCPCompPath pCompPath,
   pnuint32       pluNSDirBase,
   pnuint32       pluDOSDirBase,
   pnuint8        pbuVolNum
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 22)
   #define REQ_LEN         ((nuint) 5)
   #define REPLY_LEN       ((nuint) 9)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 1)

   nint32   lCode;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buNamSpc;
   abuReq[2] = pbuReservedB3[0];
   abuReq[3] = pbuReservedB3[1];
   abuReq[4] = pbuReservedB3[2];

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pCompPath->abuPacked;
   reqFrag[1].uLen  = pCompPath->suPackedLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if(lCode == 0)
   {
      NCopyLoHi32(pluNSDirBase ,&abuReply[0]);
      NCopyLoHi32(pluDOSDirBase ,&abuReply[4]);
      *pbuVolNum = abuReply[8];
   }
   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s22.c,v 1.7 1994/09/26 17:39:25 rebekah Exp $
*/
