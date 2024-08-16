/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s29.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s29DelPurge**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s29DelPurge
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint32  luIterHnd,
         );

REMARKS:

ARGS: <> pAccess
      >  buDirHandle
      >  luIterHnd

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8985  No Delete Privileges
         0x899C  Invalid Path

SERVER:

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     22 29  Purge Salvageable File

CHANGES: 30 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s29DelPurge
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint32  luIterHnd
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 29)
   #define NCP_STRUCT_LEN  ((nuint16) 6)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 0)

   nuint8  abuReq[REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   NCopyLoHi32(&abuReq[4], &luIterHnd);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s29.c,v 1.7 1994/09/26 17:34:10 rebekah Exp $
*/
