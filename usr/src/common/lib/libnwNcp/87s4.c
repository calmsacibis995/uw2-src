/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s4.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s4RenameMoveEntry******************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s4RenameMoveEntry
         (
            pNWAccess          pAccess,
            nuint8            buNamSpc,
            nuint8            buRenameFlag,
            nuint16           suSrchAttrs,
            pNWNCPCompPath2   pCompPath2
         );

REMARKS:

ARGS: <> pAccess
       > buNamSpc
       > buRenameFlag
       > suSrchAttrs
       > pCompPath2

INCLUDE: ncpfile.h

RETURN:  0x0000 Success

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     69 --  Rename File

NCP:     87 04  Rename or Move A File or Subdirectory

CHANGES: 30 Aug 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s4RenameMoveEntry
(
   pNWAccess          pAccess,
   nuint8            buNamSpc,
   nuint8            buRenameFlag,
   nuint16           suSrchAttrs,
   pNWNCPCompPath2   pCompPath2
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 4)
   #define REQ_LEN         ((nuint) 5)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 0)

   NWCFrag reqFrag[REQ_FRAGS];
   nuint8 abuReq[REQ_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buNamSpc;
   abuReq[2] = buRenameFlag;
   NCopyLoHi16(&abuReq[3], &suSrchAttrs);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pCompPath2->abuPacked;
   reqFrag[1].uLen  = pCompPath2->suPackedLen;

   return NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s4.c,v 1.7 1994/09/26 17:39:38 rebekah Exp $
*/
