/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s43.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s43TrusteeRemoveExt**********************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP22s43TrusteeRemoveExt
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint32  luObjID,
            nuint8   buReserved,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
         )

REMARKS: This function removes a trustee from a file or directory.


ARGS: <> pAccess
      >  buDirHandle
      >  luObjID
      >  buReserved
      >  buPathLen
      >  pbstrPath

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8990  Read-only Access To Volume
         0x899C  Invalid Path
         0x89FE  Trustee Not Found
         0x89FF  No Trustee Change Privileges

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 39  Add Extended Trustee To Directory Or File

NCP:     22 43  Remove Extended Trustee From Dir Or File

CHANGES: 13 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s43TrusteeRemoveExt
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint32  luObjID,
   nuint8   buReserved,
   nuint8   buPathLen,
   pnstr8   pbstrPath
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 43)
   #define NCP_STRUCT_LEN  ((nuint16) 8 + buPathLen)
   #define NCP_REQ_LEN     ((nuint) 10)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 0)

   NWCFrag reqFrag[NCP_REQ_FRAGS];
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);

   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   NCopyHiLo32(&abuReq[4], &luObjID);
   abuReq[8] = buReserved;
   abuReq[9] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = (nuint) buPathLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s43.c,v 1.7 1994/09/26 17:34:30 rebekah Exp $
*/
