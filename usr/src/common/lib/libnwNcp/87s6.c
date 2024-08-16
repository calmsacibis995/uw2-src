/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s6.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s6GetEntryInfo*********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s6GetEntryInfo
         (
            pNWAccess          pAccess,
            nuint8            buNamSpc,
            nuint8            buDstNamSpc,
            nuint16           suSrchAttrs,
            nuint32           luRetMask,
            pNWNCPCompPath    pCompPath,
            pNWNCPEntryStruct pEntryInfo
         );

REMARKS:

ARGS: <> pAccess
       > buNamSpc
       > buDstNamSpc
       > suSrchAttrs
       > luRetMask
       > pCompPath
      <  pEntryInfo

INCLUDE: ncpfile.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 06  Obtain File or SubDirectory Information

CHANGES: 7 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s6GetEntryInfo
(
   pNWAccess          pAccess,
   nuint8            buNamSpc,
   nuint8            buDstNamSpc,
   nuint16           suSrchAttrs,
   nuint32           luRetMask,
   pNWNCPCompPath    pCompPath,
   pNWNCPEntryStruct pEntryInfo
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 6)
   #define REQ_LEN         ((nuint) 9)
   #define REPLY_LEN       ((nuint) 77)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 2)
   #define NAME_LEN        ((nuint) 256)

   nint32  lCode;
   nuint8  abuReq[REQ_LEN], abuReply[REPLY_LEN];
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buNamSpc;
   abuReq[2] = buDstNamSpc;
   NCopyLoHi16(&abuReq[3], &suSrchAttrs);
   NCopyLoHi32(&abuReq[5], &luRetMask);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pCompPath->abuPacked;
   reqFrag[1].uLen  = pCompPath->suPackedLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = pEntryInfo->abuName;
   replyFrag[1].uLen  = NAME_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);

   if(lCode == 0)
   {
      NWNCPUnpackEntryStruct(pEntryInfo, &abuReply[0], luRetMask);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s6.c,v 1.7 1994/09/26 17:39:41 rebekah Exp $
*/
