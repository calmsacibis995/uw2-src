/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s1.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP22s1GetPath************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s1GetPath
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            pnuint8  pbuPathLen,
            pnstr8   pbstrPath,
         )

REMARKS: Gets the path associated with the specified short directory handle.


ARGS: <> pAccess
      >  buDirHandle,
      <  pbuPathLen,
      <  pbstrPath,

INCLUDE: ncpbind.h

RETURN: 0x0000  Successful

SERVER:  DOS OS2 WIN

CLIENT:  2.0 3.11 4.0

SEE:     22 00  Set Directory Handle

NCP:     22 01  Get Directory Path

CHANGES: 14 Sep 1993 - written - anevarez
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s1GetPath
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   pnuint8  pbuPathLen,
   pnstr8   pbstrPath
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 1)
   #define NCP_STRUCT_LEN  ((nuint16) 2)
   #define MAX_PATH_LEN    ((nuint) 255)
   #define NCP_REQ_LEN     ((nuint) 4)
   #define NCP_REPLY_LEN   ((nuint) 1)
   #define NCP_REQ_FRAGS   ((nuint) 1)
   #define NCP_REPLY_FRAGS ((nuint) 2)

   nuint8 abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], repFrag[NCP_REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);

   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = (nuint) NCP_REQ_LEN;

   repFrag[0].pAddr = pbuPathLen;
   repFrag[0].uLen  = (nuint) NCP_REPLY_LEN;

   repFrag[1].pAddr = pbstrPath;
   repFrag[1].uLen  = MAX_PATH_LEN;

   return (NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, repFrag, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s1.c,v 1.7 1994/09/26 17:33:42 rebekah Exp $
*/
