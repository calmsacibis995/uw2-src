/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s11.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s11DirDel**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s11DirDel
         (
            pNWAccess pAccess,
            nuint8     buDirHandle,
            nuint8     buReserved,
            nuint8     buPathLen,
            pnstr8     pbstrPath,
         );

REMARKS:

ARGS: <> pAccess
      >  buDirHandle
      >  buPathLen
      >  pbstrPath

INCLUDE: ncpfile.h

RETURN:  0x0000   Successful

SERVER:  2.0 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     87 08  Delete A File or SubDirectory

NCP:     22 11  Delete Directory

CHANGES: 14 Sep 1993 - written - anevarez
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s11DirDel
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buReserved,
   nuint8   buPathLen,
   pnstr8   pbstrPath
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 11)
   #define NCP_STRUCT_LEN  ((nuint16) 4 + buPathLen)
   #define NCP_REQ_LEN     ((nuint) 6)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 0)

   nuint8 abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   abuReq[4] = buReserved;
   abuReq[5] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = (nuint) NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = buPathLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s11.c,v 1.7 1994/09/26 17:33:45 rebekah Exp $
*/
