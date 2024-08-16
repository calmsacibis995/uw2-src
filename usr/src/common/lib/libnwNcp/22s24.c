/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s24.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s24RestoreBaseHandle
         (
            pNWAccess pAccess,
            pnuint8  pbuSaveBuffer14,
            pnuint8  pbuNewDirHandle,
            pnuint8  pbuRightsMask,
         );

REMARKS:

ARGS: <> pAccess
      >  pbuSaveBuffer14
      <  pbuNewDirHandle
      <  pbuRightsMask (optional)

INCLUDE: ncpfile.h

RETURN:

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     22 24  Restore An Extracted Base Handle

CHANGES: 21 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s24RestoreBaseHandle
(
   pNWAccess pAccess,
   pnuint8  pbuSaveBuffer14,
   pnuint8  pbuNewDirHandle,
   pnuint8  pbuRightsMask
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 24)
   #define NCP_STRUCT_LEN  ((nuint16) 15)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 2)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NWCMemMove(&abuReq[3], pbuSaveBuffer14, (nuint) 14);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuReply, NCP_REPLY_LEN, NULL);

   if (lCode == 0)
   {
      *pbuNewDirHandle = abuReply[0];
      if (pbuRightsMask)
         *pbuRightsMask   = abuReply[1];
   }
   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s24.c,v 1.7 1994/09/26 17:34:04 rebekah Exp $
*/
