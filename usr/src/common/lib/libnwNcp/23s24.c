/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s24.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s24KeyedObjLogin*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s24KeyedObjLogin
         (
            pNWAccess pAccess,
            pnuint8  pbuKeyB8,
            nuint16  suObjType,
            nuint8   buObjNameLen,
            pnstr8   pbstrObjName,
         )

REMARKS: Keyed Object Login logs in the specified bindery object,
         using the 8-byte key previously assigned to the object.

ARGS: <> pAccess
      >  pbuKeyB8
      >  suObjType
      >  buObjNameLen
      >  pbstrObjName

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89C1  No Account Balance
         0x89C2  Credit Limit Exceeded
         0x89C5  Server Login Locked
         0x89D9  Maximum Logins Exceeded
         0x89DA  Bad Login Time
         0x89DB  Node Address Violation
         0x89DC  Account Expired
         0x89DE  Bad Password

SERVER:  DOS OS2 WIN

CLIENT:  2.2 3.11 4.0

SEE:     23 23  Get Login Key

NCP:     23 24  Keyed Object Login

CHANGES: 08 Sep 1993 - written - anevarez
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
#ifdef N_PLAT_OS2X
N_INTERN_FUNC_PAS( NWCCODE )
InternalLogin
(
   NWCONN_HANDLE  conn,
   nuint16        function,
   nuint16        reqCount,
   NWCFRAG N_PTR  reqList,
   nuint16        replyCount,
   NWCFRAG N_PTR  replyList,
   nuint8  N_PTR  clntName,
   nuint16        clntType
);

N_INTERN_FUNC_PAS( NWCCODE )
InternalLogin
(
   NWCONN_HANDLE  conn,
   nuint16        function,
   nuint16        reqCount,
   NWCFRAG N_PTR  reqList,
   nuint16        replyCount,
   NWCFRAG N_PTR  replyList,
   nuint8  N_PTR  clntName,
   nuint16        clntType
)
{
   conn       = conn;
   function   = function;
   reqCount   = reqCount;
   reqList    = reqList;
   replyCount = replyCount;
   replyList  = replyList;
   clntName   = clntName;
   return ((*NWCallGate)(NWI_LOGIN_CODE, (void N_PTR )&clntType));
}
#endif


N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s24KeyedObjLogin
(
   pNWAccess pAccess,
   pnuint8  pbuKeyB8,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 24)
   #define NCP_STRUCT_LEN  ((nuint16) (REQ_LEN_A+KEY_LEN+REQ_LEN_B-2+buObjNameLen))
   #define REQ_LEN_A       ((nuint) 3)
   #define KEY_LEN         ((nuint) 8)
   #define REQ_LEN_B       ((nuint) 3)
   #define REQ_FRAGS       ((nuint) 4)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS];
   nuint8 abuReqA[REQ_LEN_A], abuReqB[REQ_LEN_B];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReqA[0], &suNCPLen);
   abuReqA[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReqB[0], &suObjType );
   abuReqB[2] = buObjNameLen;

   reqFrag[0].pAddr = abuReqA;
   reqFrag[0].uLen  = REQ_LEN_A;

   reqFrag[1].pAddr = pbuKeyB8;
   reqFrag[1].uLen  = KEY_LEN;

   reqFrag[2].pAddr = abuReqB;
   reqFrag[2].uLen  = REQ_LEN_B;

   reqFrag[3].pAddr = pbstrObjName;
   reqFrag[3].uLen  = (nuint) buObjNameLen;

#ifdef N_PLAT_OS2X
   return (InternalLogin( NWCGetConnP(pAccess), NCP_FUNCTION, REQ_FRAGS,
                           reqFrag, REPLY_FRAGS, NULL, pbstrObjName,
                           suObjType ) );
#else
   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s24.c,v 1.7 1994/09/26 17:36:54 rebekah Exp $
*/
