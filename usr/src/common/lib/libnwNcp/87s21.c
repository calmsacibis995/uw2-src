/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s21.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s21GetDirHandlePath****************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s21GetDirHandlePath
         (
            pNWAccess pAccess,
            nuint8   buNamSpc,
            nuint8   buDirHandle,
            pnuint8  pbuPathLen,
            pnstr8   pbstrPath
         );

REMARKS: Gets the path associated with the specified short directory handle.

ARGS: <> pAccess
       > buNamSpc
       > buDirHandle
      <  pbuPathLen
      <  pbstrPath - Must be able to hold 255 characters

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful

CLIENT:  3.11 4.0

SERVER:  DOS OS2 WIN NT

SEE:

NCP:     87 21  Get Path String From Short Directory Handle

CHANGES: 14 Sep 1993 - written - anevarez
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s21GetDirHandlePath
(
   pNWAccess pAccess,
   nuint8   buNamSpc,
   nuint8   buDirHandle,
   pnuint8  pbuPathLen,
   pnstr8   pbstrPath
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 21)
   #define REQ_LEN         ((nuint) 3)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 2)
   #define PATH_LEN        ((nuint) 256)

   nuint8 abuReq[REQ_LEN];
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buNamSpc;
   abuReq[2] = buDirHandle;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = pbuPathLen;
   replyFrag[0].uLen  = 1;

   replyFrag[1].pAddr = pbstrPath;
   replyFrag[1].uLen  = PATH_LEN;

   return NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s21.c,v 1.7 1994/09/26 17:39:24 rebekah Exp $
*/
