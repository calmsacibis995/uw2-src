/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s205.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s205GetServerLoginStats**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s205GetServerLoginStatus
         (
            pNWAccess pAccess,
            pnuint8  pbuUserLoginAllowed,
         );

REMARKS: This call allows the calling operator to determine whether user logins
         are currently allowed on the target server.

         User Login Allowed will be zero if clients cannot log in, and non-zero
         otherwise.

         If the calling station does not have operator privileges, the No Console
         Rights completion code will be returned.


ARGS: <> pAccess
      <  pbuUserLoginAllowed

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x89C6   No Console Rights

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23  203  Disable File Server Login
         23  211  Down File Server

NCP:     23  205  Get File Server Login Status

CHANGES: 10 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s205GetServerLoginStatus
(
   pNWAccess pAccess,
   pnuint8  pbuUserLoginAllowed
)
{
   #define NCP_FUNCTION         ((nuint)     23)
   #define NCP_SUBFUNCTION      ((nuint8)   205)
   #define NCP_STRUCT_LEN       ((nuint16)    1)
   #define NCP_REQ_LEN          ((nuint)      3)
   #define NCP_REP_LEN          ((nuint)      1)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
         pbuUserLoginAllowed, NCP_REP_LEN, NULL);
   if (lCode != 0)
   {
      *pbuUserLoginAllowed = (nuint8) 0;
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s205.c,v 1.7 1994/09/26 17:35:59 rebekah Exp $
*/
