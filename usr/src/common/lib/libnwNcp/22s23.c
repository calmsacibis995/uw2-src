/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s23.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s23ExtractBaseHandle*********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s23SaveBaseHandle
         (
            pNWAccess    pAccess,
            nuint8      buDirHandle,
            pnuint8     pbuSaveBuffer,
         )

REMARKS:

ARGS: <> pAccess
      >  buDirHandle
      <  pbstrSaveBuffer
            The buffer in which the directory handle information is to be
            saved; it should be 14 bytes long

INCLUDE: ncpfile.h

RETURN:

SERVER:  2.2

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 23  Extract A Base Handle

CHANGES: 16 Sep 1993 - NWNCP Enabled, there was no documentation that could
                        be found on this NCP, so I wrote it using the NWCALL
                        as a model.  - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s23SaveBaseHandle
(
   pNWAccess    pAccess,
   nuint8      buDirHandle,
   pnuint8     pbuSaveBuffer
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 23)
   #define NCP_STRUCT_LEN  ((nuint16) 2)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 14)

   nuint8 abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               pbuSaveBuffer, NCP_REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s23.c,v 1.7 1994/09/26 17:34:03 rebekah Exp $
*/
