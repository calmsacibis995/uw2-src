/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s21.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpconn.h"

/*manpage*NWNCP23s21GetObjConnList**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s21GetObjConnList
         (
            pNWAccess pAccess,
            nuint16  suObjType,
            nuint8   buObjNameLen,
            pnstr8   pbstrObjName,
            pnuint8  pbuListLen,
            pnuint8  pbuConnNumList,
         );

REMARKS: This call returns a list of the connection numbers on the server for
         clients logged in with the specified Object Name and Object Type.  If no
         client is logged in using the specified Object Name and Object Type, the
         list length returned by the server is set to zero.

         This function is the general case of, and replaces the earlier call Get
         User Connection List (0x2222  23  2).

         This call is replaced by the NetWare 386 v3.11 call Get Object
         Connection List  (0x2222  23  27).

ARGS: <> pAccess
       > suObjType
       > buObjNameLen
       > pbstrObjName
      <  pbuListLen
      <  pbuConnNumList

INCLUDE: ncpconn.h

RETURN:  0x0000    Successful
         0x8996    Server Out Of Space
         0x89F0    Illegal Wildcard
         0x89FC    No Such Object
         0x89FE    Directory Locked
         0x89FF    Hard Failure

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     23 19  Get Internet Address
         23 27  Get Object Connection List

NCP:     23 21  Get Object Connection List

CHANGES: 16 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s21GetObjConnList
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   pnuint8  pbuListLen,
   pnuint8  pbuConnNumList
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 21)
   #define NCP_STRUCT_LEN  ((nuint16) (4 + buObjNameLen))
   #define MAX_CONN_LEN    ((nuint) 256)
   #define REQ_LEN         ((nuint) 6)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 2)

   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint16 suNCPLen;
   nuint8 abuReq[REQ_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   NCopyHiLo16(&abuReq[3], &suObjType);
   abuReq[5] = buObjNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrObjName;
   reqFrag[1].uLen  = buObjNameLen;

   replyFrag[0].pAddr = pbuListLen;
   replyFrag[0].uLen  = (nuint) 1;

   replyFrag[1].pAddr = pbuConnNumList;
   replyFrag[1].uLen  = MAX_CONN_LEN;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s21.c,v 1.7 1994/09/26 17:36:05 rebekah Exp $
*/
