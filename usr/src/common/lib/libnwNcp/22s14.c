/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s14.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s14TrusteeDelDir**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s14TrusteeDelDir
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint32  luTrusteeID,
            nuint8   buReserved,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
         );

REMARKS: This call deletes the specified Trustee ID from the trustee list of the
         specified directory.  This call will succeed only if the client has access
         control rights to the target directory or its parent directory.


ARGS: <> pAccess
       > buDirHandle
       > luTrusteeID
       > buReserved
       > buPathLen
       > pbstrPath

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x898C  No Set Privileges
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x8999  Directory Full Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FC  No Such Object
         0x89FD  Bad Station Number
         0x89FF  Hard Failure, Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 13  Add Trustee To Directory
         22 39  Add Extended Trustee To Directory Or File

NCP:     22 14  Delete Trustee From Directory

CHANGES: 13 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s14TrusteeDelDir
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint32  luTrusteeID,
   nuint8   buReserved,
   nuint8   buPathLen,
   pnstr8   pbstrPath
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 14)
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
   NCopyHiLo32(&abuReq[4], &luTrusteeID);
   abuReq[8] = buReserved;
   abuReq[9] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = (nuint) buPathLen;

   return ((NWRCODE) NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s14.c,v 1.7 1994/09/26 17:33:49 rebekah Exp $
*/
