/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s75.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s75KeyedChangePwd*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s75KeyedChangePwd
         (
            pNWAccess pAccess,
            pnuint8  pbuKeyB8,
            nuint16  suObjType,
            nuint8   buObjNameLen,
            pnstr8   pbstrObjName,
            nuint8   buNewPwdLen,
            pnstr8   pbstrNewPwd,
         )

REMARKS: Keyed Change Password changes the password for a bindery object
         that requires keyed login.

ARGS: <> pAccess
      >  pbuKeyB8
      >  suObjType
      >  buObjNameLen
      >  pbstrObjName
      >  buNewPwdLen
      >  pbstrNewPwd

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x89C5  Account Locked
         0x89FE  Bindery Locked
         0x89FF  Password Verification Failed

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 74  Keyed Verify Password

NCP:     23 75  Keyed Change Password

CHANGES: 08 Sep 1993 - written - anevarez
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s75KeyedChangePwd
(
   pNWAccess pAccess,
   pnuint8  pbuKeyB8,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buNewPwdLen,
   pnstr8   pbstrNewPwd
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 75)
   #define NCP_STRUCT_LEN  ((nuint16) (13 + buObjNameLen + buNewPwdLen))
   #define MAX_PWD_LEN     ((nuint) 256)
   #define KEY_LEN         ((nuint) 8)
   #define REQ_LEN_A       ((nuint) 3)
   #define REQ_LEN_B       ((nuint) 3)
   #define REQ_LEN_C       ((nuint) (1 + MAX_PWD_LEN))
   #define REQ_FRAGS       ((nuint) 5)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS];
   nuint8 abuReqA[REQ_LEN_A], abuReqB[REQ_LEN_B], abuReqC[REQ_LEN_C];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReqA[0], &suNCPLen);
   abuReqA[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReqB[0], &suObjType);
   abuReqB[2] = buObjNameLen;
   abuReqC[0] = buNewPwdLen;
   NWCMemMove(&abuReqC[1], pbstrNewPwd, buNewPwdLen);

   reqFrag[0].pAddr = abuReqA;
   reqFrag[0].uLen  = REQ_LEN_A;

   reqFrag[1].pAddr = pbuKeyB8;
   reqFrag[1].uLen  = KEY_LEN;

   reqFrag[2].pAddr = abuReqB;
   reqFrag[2].uLen  = REQ_LEN_B;

   reqFrag[3].pAddr = pbstrObjName;
   reqFrag[3].uLen  = (nuint) buObjNameLen;

   reqFrag[4].pAddr = abuReqC;
   reqFrag[4].uLen  = (nuint) buNewPwdLen+1;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s75.c,v 1.7 1994/09/26 17:37:38 rebekah Exp $
*/
