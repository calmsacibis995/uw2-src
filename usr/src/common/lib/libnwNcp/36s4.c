/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:36s4.c	1.1"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpext.h"

/*manpage*NWNCP36s4GetNCPExtsList*****************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP36s4GetNCPExtsList
         (
            pNWAccess    pAccess,
            pnuint32    pluStartNCPExtID,
            pnuint16    psuListItems,
            pnuint8     psuReservedB2,
            pnuint32    pluNCPExtIDListB128,
         );

REMARKS: Scans currently loaded NCP extensions

ARGS: <> pAccess,
      <> pluStartNCPExtID,
      <> psuListItems,
      <> psuReservedB2,
      <> pluNCPExtIDListB128,

INCLUDE: ncpext.h

RETURN:  n/a

SERVER:  4.0

CLIENT:  DOS WIN OS2

SEE:

NCP:     36 04 Get List Of Loaded NCP Extensions IDs

CHANGES: 21 Sep 1993 - written (no documentation, so written from NWCALLS)
                               - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP36s4GetNCPExtsList
(
   pNWAccess    pAccess,
   pnuint32    pluStartNCPExtID,
   pnuint16    psuListItems,
   pnuint8     psuReservedB2,
   pnuint32    pluNCPExtIDListB128
)
{
   #define NCP_FUNCTION    ((nuint) 36)
   #define NCP_SUBFUNCTION ((nuint8) 4)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define MAX_EXT_ID_LEN  ((nuint) 512)
   #define REQ_LEN         ((nuint) 7)
   #define REPLY_LEN       ((nuint) 8)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 2)

   nint32   lCode;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint16 suNCPLen;
   nuint8 abuReq[REQ_LEN], abuRep[REPLY_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0],&suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[3],pluStartNCPExtID);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = abuRep;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = pluNCPExtIDListB128;
   replyFrag[1].uLen  = MAX_EXT_ID_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      if(psuListItems)
            NCopyLoHi16(psuListItems, &abuRep[0]);

      if(psuReservedB2)
            NCopyLoHi16(psuReservedB2, &abuRep[2]);

      NCopyLoHi32(pluStartNCPExtID, &abuRep[4]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/36s4.c,v 1.1 1994/09/26 17:38:35 rebekah Exp $
*/
