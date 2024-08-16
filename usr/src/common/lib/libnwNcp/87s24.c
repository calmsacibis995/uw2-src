/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s24.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s24NSGetLoadedList*****************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s24NSGetLoadedList
         (
            pNWAccess pAccess,
            nuint16  suReserved,
            nuint8   buVolNum,
            pnuint16 psuNumNSLoaded,
            pnuint8  pbuNSLoadedList
         );

REMARKS:

ARGS: <> pAccess
       > suReserved
       > buVolNum
      <  psuNumNSLoaded
      <  pbuNSLoadedList

INCLUDE: ncpfile.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 24  Get Name Spaces Loaded List From Volume Number

CHANGES: 10 Aug 1993 - written - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s24NSGetLoadedList
(
   pNWAccess pAccess,
   nuint16  suReserved,
   nuint8   buVolNum,
   pnuint16 psuNumNSLoaded,
   pnuint8  pbuNSLoadedList
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 24)
   #define REQ_LEN         ((nuint) 4)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 2)
   #define MAX_LIST_LEN    ((nuint) 512)

   nint32   lCode;
   nuint8  abuReq[REQ_LEN];
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   abuReq[0] = NCP_SUBFUNCTION;
   NCopyLoHi16(&abuReq[1], &suReserved);
   abuReq[3] = buVolNum;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = psuNumNSLoaded;
   replyFrag[0].uLen  = 2;

   replyFrag[1].pAddr = pbuNSLoadedList;
   replyFrag[1].uLen  = MAX_LIST_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);

   if(lCode == 0)
   {
      *psuNumNSLoaded = NSwapLoHi16(*psuNumNSLoaded);
   }

   return((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s24.c,v 1.7 1994/09/26 17:39:27 rebekah Exp $
*/
