/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s27.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpconn.h"

/*manpage*NWNCP23s27GetObjConnList**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s27GetObjConnList
         (
            pNWAccess pAccess,
            nuint32  luSrchConnNum,
            nuint16  suObjType,
            nuint8   buObjNameLen,
            pnstr8   pbstrObjName,
            pnuint8  pbuListLen,
            pnuint32 pluConnNumList,
         );

REMARKS: This is a NetWare 386 v3.11 call that replaces the earlier call Get Object
         Connection List (0x2222  23  19).  This new NCP allows the use of the
         high connection byte in the Request/Reply header of the packet.  A new
         job structure has also been defined for this new NCP.  See Introduction
         to Queue NCPs for information on both the old and new job structure.

         The SearchConnNumber field should be set to 0 initially.  Subsequent
         calls use the highest connection number return in the ConnNumberList.

ARGS: <> pAccess
       > luSrchConnNum
       > suObjType
       > buObjNameLen
       > pbstrObjName
      <  pbuListLen
      <  pluConnNumList

INCLUDE: ncpconn.h

RETURN:  0x0000 SUCCESSFUL

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 21  Get Object Connection List

NCP:     23 27  Get Object Connection List

CHANGES: 16 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s27GetObjConnList
(
   pNWAccess pAccess,
   nuint32  luSrchConnNum,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   pnuint8  pbuListLen,
   pnuint32 pluConnNumList
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 27)
   #define NCP_STRUCT_LEN  ((nuint16) (8 + buObjNameLen))
   #define REQ_LEN         ((nuint) 10)
   #define REPLY_LEN       ((nuint) 512)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 2)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REQ_FRAGS];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyLoHi32(&abuReq[3], &luSrchConnNum);
   NCopyHiLo16(&abuReq[7], &suObjType);
   abuReq[9] = buObjNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrObjName;
   reqFrag[1].uLen  = buObjNameLen;

   replyFrag[0].pAddr = pbuListLen;
   replyFrag[0].uLen  = 1;

   replyFrag[1].pAddr = abuReply;
   replyFrag[1].uLen  = REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      nint i;

      for(i = 0; i < (nint) *pbuListLen; i++)
         NCopyLoHi32(&pluConnNumList[i], &abuReply[4 * i]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s27.c,v 1.7 1994/09/26 17:37:05 rebekah Exp $
*/
