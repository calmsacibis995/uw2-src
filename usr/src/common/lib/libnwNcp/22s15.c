/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s15.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s15RenameDir**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s15RenameDir
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
            nuint8   buNewPathLen,
            pnstr8   pbstrNewPath,
         )

REMARKS: This call allows a client rename a directory.  New Directory Path must
         contain only the new name of the target directory; this new name must not
         be preceded by a path specification.  Current implementations limit
         directory names to the DOS "8.3" naming conventions.  Longer paths will be
         truncated.

         For this call to be successful, the calling client must have access control
         and modify rights in the directory.

ARGS: <> pAccess
      >  buDirHandle
      >  buPathLen
      <  pbstrPath
      <  buNewPathLen
      <  pbstrNewPath

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x898B  No Rename Privileges
         0x8992  All Names Exist
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x899E  Bad File Name
         0x89A1  Directory I/O Error
         0x89EF  Illegal Name
         0x89FD  Bad Station Number
         0x89FF  Failure

SERVER:

CLIENT:  DOS OS2 WIN NT

SEE:     22 46  Rename Or Move

NCP:     22 15  Rename Directory

CHANGES: 16 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s15RenameDir
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   nuint8   buNewPathLen,
   pnstr8   pbstrNewPath
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 15)
   #define NCP_STRUCT_LEN  ((nuint16) 4 + buPathLen + buNewPathLen)
   #define NCP_REQ_LEN0    ((nuint) 5)
   #define NCP_REQ_LEN2    ((nuint) 1)
   #define NCP_REQ_FRAGS   ((nuint) 4)
   #define NCP_REPLY_FRAGS ((nuint) 0)

   NWCFrag reqFrag[NCP_REQ_FRAGS];
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN0];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0],&suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   abuReq[4] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = (nuint) NCP_REQ_LEN0;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = (nuint) buPathLen;

   reqFrag[2].pAddr = &buNewPathLen;
   reqFrag[2].uLen  = (nuint) NCP_REQ_LEN2;

   reqFrag[3].pAddr = pbstrNewPath;
   reqFrag[3].uLen  = (nuint) buNewPathLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s15.c,v 1.7 1994/09/26 17:33:51 rebekah Exp $
*/
