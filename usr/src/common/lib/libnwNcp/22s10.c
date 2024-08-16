/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s10.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s10DirCreate**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s10DirCreate
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint8   buAccessMask,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
         );

REMARKS: This call allows a client to create a directory. The Directory Path string
         must contain at least one element.  The last element of this string will be
         used as the name of the newly created directory.  Wildcard characters are not
         allowed in the new directory name.  Directory names are restricted to the
         DOS "8.3" names; longer names will be truncated.

         For this call to be successful, the client must have creation privileges in
         the directory that will become the parent directory of the newly created
         directory.

ARGS: <> pAccess
      >  buDirHandle
      >  buAccessMask
      >  buPathLen
      >  pbstrPath

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8984  No Create Privileges
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x8999  Directory Full Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x899E  Bad File Name
         0x89A1  Directory I/O Error
         0x89FD  Bad Station Number
         0x89FF  Failure

SERVER:

CLIENT:  DOS OS2 WIN NT

SEE:     22 11  Delete Directory

NCP:     22 10  Create Directory

CHANGES: 14 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s10DirCreate
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buAccessMask,
   nuint8   buPathLen,
   pnstr8   pbstrPath
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 10)
   #define NCP_STRUCT_LEN  ((nuint16) 4 + buPathLen)
   #define NCP_REQ_LEN     ((nuint) 6)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 0)

   NWCFrag reqFrag[NCP_REQ_FRAGS];
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   abuReq[4] = buAccessMask;
   abuReq[5] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = (nuint) NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = (nuint) buPathLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s10.c,v 1.7 1994/09/26 17:33:44 rebekah Exp $
*/
