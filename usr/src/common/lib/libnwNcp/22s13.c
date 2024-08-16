/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s13.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s13TrusteeAddToDir******************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s13TrusteeAddToDir
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint32  luTrusteeID,
            nuint8   buTrusteeAccessMask,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
         )

REMARKS: This call allows a client to add a new trustee to a directory's trustee list.

         The Trustee ID number must be the valid ID of an object in the server's
         bindery; normally this ID number is retrieved from the server using the
         bindery functions.

         The Trustee Access Mask indicates the access rights to be given to the
         specified object at the target directory.

         This call replaces the access mask of the listed trustee if the trustee
         already appears in the directory's trustee list; otherwise, the trustee is
         added to the directory's trustee list.

         Directory Path is the path relative to the specified Directory Handle.  The
         Directory path must follow the same convention as NetWare filenames.  See
         the introduction to the File Services chapter.

         Only a client with access control rights to either the target directory or
         its parent directory can make this call.

ARGS: <> pAccess,
      >  buDirHandle,
      >  luTrusteeID,
      >  buTrusteeAccessMask,
      >  buPathLen,
      >  pbstrPath,

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

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     22 39 --  Add Extended Trustee To Directory Or File
         22 14 --  Delete Trustee From Directory

NCP:     22 13  Add Extended Trustee To Directory or File

CHANGES: 8 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s13TrusteeAddToDir
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint32  luTrusteeID,
   nuint8   buTrusteeAccessMask,
   nuint8   buPathLen,
   pnstr8   pbstrPath
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 13)
   #define NCP_STRUCT_LEN  ((nuint16) 8 + buPathLen)
   #define NCP_REQ_LEN     ((nuint) 10)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 0)

   nuint8 abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);

   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   NCopyHiLo32(&abuReq[4], &luTrusteeID);
   abuReq[8] = buTrusteeAccessMask;
   abuReq[9] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = (nuint) NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = buPathLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s13.c,v 1.7 1994/09/26 17:33:48 rebekah Exp $
*/
