/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:35s6.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpafp.h"

/*manpage*NWAFPGetEntryIDFromHandle*****************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP35s6AFPGEntryIDFrmNWHan
         (
            pNWAccess  pAccess,
            pnuint8   pbuNWDirHandleB6,
            pnuint8   pbuVolNum,
            pnuint32  pluAFPEntryID,
            pnuint8   pbuResFork
         )

REMARKS: This call returns an AFP Entry ID for the specified NetWare
         file handle.

ARGS: <> pAccess
      >  pbuNWDirHandle
      <  buVolNum (optional)
      <  pluAFPEntryID (optional)
      <  pbuResFork (optional)

INCLUDE: ncpafp.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     35 06  AFP Get Entry ID Form NetWare Handle

CHANGES: 19 Aug 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP35s6AFPGEntryIDFrmNWHan
(
   pNWAccess  pAccess,
   pnuint8   pbuNWDirHandleB6,
   pnuint8   pbuVolNum,
   pnuint32  pluAFPEntryID,
   pnuint8   pbuResFork
)
{
   #define NCP_FUNCTION    ((nuint) 35)
   #define NCP_SUBFUNCTION ((nuint8) 6)
   #define NCP_STRUCT_LEN  ((nuint16) 7)
   #define REQ_LEN         ((nuint) 3)
   #define DIR_HANDLE_LEN  ((nuint) 6)
   #define REPLY_LEN       ((nuint) 6)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 1)

   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint16 suNCPLen;
   nuint8  abuReq[REQ_LEN], abuReply[REPLY_LEN];
   NWRCODE ccode;

   suNCPLen=NCP_STRUCT_LEN;
   NCopyHiLo16 (abuReq, &suNCPLen);
   abuReq[2]=NCP_SUBFUNCTION;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbuNWDirHandleB6;
   reqFrag[1].uLen  = DIR_HANDLE_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   if ((ccode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
         REPLY_FRAGS, replyFrag, NULL)) == 0)
   {
      if (pbuVolNum)
         *pbuVolNum = abuReply[0];

      if (pluAFPEntryID)
         NCopyHiLo32(pluAFPEntryID, &abuReply[1]);

      if (pbuResFork)
         *pbuResFork = abuReply[5];
   }

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/35s6.c,v 1.7 1994/09/26 17:38:26 rebekah Exp $
*/
