/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s201.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s201GetServerDescStr**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s201GetServerDescStr
         (
            pNWAccess pAccess,
            pnstr8   pbstrDescStringB512,
         );

REMARKS: This call returns up to 512 bytes of information (in descriptive strings)
         about the file server.  Each string is null-terminated.  Strings that can
         be returned might include a company's name, the version of NetWare,
         the revision date, a copyright notice, and so on.

         Any client can make this call.


ARGS: <> pAccess
      <  pbstrDescStringB512

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 17  Get File Server Information

NCP:     23 201  Get File Server Description Strings

CHANGES: 13 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s201GetServerDescStr
(
   pNWAccess pAccess,
   pnstr8   pbstrDescStringB512
)
{
   #define NCP_FUNCTION         ((nuint)      23)
   #define NCP_SUBFUNCTION      ((nuint8)    201)
   #define NCP_STRUCT_LEN       ((nuint16)     1)
   #define NCP_REQ_LEN          ((nuint)       3)
   #define NCP_REP_LEN          ((nuint)     512)

   nuint8 abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               pbstrDescStringB512, NCP_REP_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s201.c,v 1.7 1994/09/26 17:35:54 rebekah Exp $
*/
