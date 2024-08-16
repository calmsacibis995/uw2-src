/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s74.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s74KeyedVerifyPwd*****************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s74KeyedVerifyPwd
         (
            pNWAccess pAccess,
            pnuint8  pbuKeyB8,
            nuint16  suObjType,
            nuint8   buObjNameLen,
            pnstr8   pbstrObjName,
         )

REMARKS: Keyed Verify Password verifies a password for a bindery object
         that requires keyed login.

ARGS: <> pAccess
      >  pbuKeyB8
      >  suObjType
      >  buObjNameLen
      >  pbstrObjName

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x89C5  Account Locked
         0x89FE  Bindery Locked
         0x89FF  Password Verification Failed

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 75  Keyed Change Password

NCP:     23 74  Keyed Verify Password

CHANGES: 08 Sep 1993 - written - anevarez
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s74KeyedVerifyPwd
(
   pNWAccess pAccess,
   pnuint8  pbuKeyB8,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 74)
   #define NCP_STRUCT_LEN  ((nuint16) (12 + buObjNameLen))
   #define REQ1_LEN        ((nuint) 3)
   #define REQ2_LEN        ((nuint) 8)
   #define REQ3_LEN        ((nuint) 3)
   #define REQ_FRAGS       ((nuint) 4)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8 abuReqA[REQ1_LEN], abuReqB[REQ3_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReqA[0], &suNCPLen);
   abuReqA[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReqB[0], &suObjType );
   abuReqB[2] = buObjNameLen;

   reqFrag[0].pAddr = abuReqA;
   reqFrag[0].uLen  = REQ1_LEN;

   reqFrag[1].pAddr = pbuKeyB8;
   reqFrag[1].uLen  = REQ2_LEN;

   reqFrag[2].pAddr = abuReqB;
   reqFrag[2].uLen  = REQ3_LEN;

   reqFrag[3].pAddr = pbstrObjName;
   reqFrag[3].uLen  = (nuint) buObjNameLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s74.c,v 1.7 1994/09/26 17:37:37 rebekah Exp $
*/
