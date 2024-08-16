/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:36s3.c	1.1"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpext.h"

/*manpage*NWNCP36s0GetNumNCPExts***************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP36s3GetNumNCPExts
         (
            pNWAccess    pAccess,
            pnuint32    pluNumNCPExts,
         )

REMARKS: Scans currently loaded NCP extensions

ARGS: <> pAccess,
      <  pluNumNCPExts,

INCLUDE: ncpext.h

RETURN:  n/a

SERVER:  3.0 3.11

CLIENT:  DOS WIN OS2

SEE:

NCP:     36 03 Get Number Of Loaded NCP Extensions (4.0)

CHANGES: 21 Sep 1993 - written (no documentation, so written from NWCALLS)
                               - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP36s3GetNumNCPExts
(
   pNWAccess    pAccess,
   pnuint32    pluNumNCPExts
)
{
   #define NCP_FUNCTION    ((nuint) 36)
   #define NCP_SUBFUNCTION ((nuint8) 3)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 4)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[REQ_LEN], abuRep[REPLY_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0],&suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuRep, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NCopyLoHi32(pluNumNCPExts,&abuRep[0]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/36s3.c,v 1.1 1994/09/26 17:38:34 rebekah Exp $
*/
