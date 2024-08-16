/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s20.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s20FreeDirHandle**********************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP22s20FreeDirHandle
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
         )

REMARKS: This call deallocates the specified directory handle, whether the handle is
         temporary or permanent.


ARGS: <> pAccess
      >  buDirHandle

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x899B  Bad Directory Handle

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 18  Alloc Permanent Directory Handle
         22 22  Alloc Special Temporary Directory Handle
         22 19  Alloc Temporary Directory Handle

NCP:     22 20  Deallocate Directory Handle

CHANGES: 10 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s20FreeDirHandle
(
   pNWAccess pAccess,
   nuint8   buDirHandle
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 20)
   #define NCP_STRUCT_LEN  ((nuint16) 2)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 0)

   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               NULL, NCP_REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s20.c,v 1.7 1994/09/26 17:34:00 rebekah Exp $
*/
