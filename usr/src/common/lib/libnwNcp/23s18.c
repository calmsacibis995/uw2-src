/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s18.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s18GetNetSerialNum**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s18GetNetSerialNum
         (
            pNWAccess pAccess,
            pnuint32 pluServerSerialNum,
            pnuint16 psuAppNum,
         );

REMARKS: This call returns a file server's serial number and the application
         number.  The combination of the server's serial number and the
         application number is unique.

ARGS: <> pAccess
      <  pluServerSerialNum
      <  psuAppNum (optional)

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 12  Verify Serialization

NCP:     23 18  Get Network Serial Number

CHANGES: 13 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s18GetNetSerialNum
(
   pNWAccess pAccess,
   pnuint32 pluServerSerialNum,
   pnuint16 psuAppNum
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 18)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 6)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN, abuReply,
            REPLY_LEN, NULL);

   if (lCode == 0)
   {
      NCopyLoHi32(pluServerSerialNum, &abuReply[0]);
      if (psuAppNum)
         NCopyLoHi16(psuAppNum, &abuReply[4]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s18.c,v 1.7 1994/09/26 17:35:50 rebekah Exp $
*/
