/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s17.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s17DelRecover**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s17DelRecover
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint8   buPathNameLen,
            pnstr8   pbstrPathName,
            pnstr8   pbstrOldFileNameB15,
            pnstr8   pbstrNewFileNameB15,
         );

REMARKS: This is an old NetWare 286 call that is supported in NetWare v2.1 or previous.
         It is replaced by the NetWare 386 v3.0 call, Recover Salvageable File
         (0x2222  22  28).

         This call allows a client to recover one file on the volume as specified
         by Directory Handle and Path Name.  When a client requests that a file be
         erased, the file is marked for deletion and moved to a special holding area
         in the volume's directory structure.  Deleted files placed in the holding
         area can be recovered only until the client attempts another file erase or
         file create request; the server holds only the most recently "deleted" files
         for each client and purges the older files so that their disk and directory
         space can be reclaimed.

         Old File Name must contain the file's original name, null-padded.  If a file
         having the same name already exists in the target directory, the server will
         assign a new filename.  This filename will be recorded in New File Name and
         will also be null-padded as required.

ARGS: <> pAccess
      >  buDirHandle
      >  buPathNameLen
      >  pbstrPathName
      <  pbstrOldFileNameB15 (optional)
      <  pbstrNewFileNameB15 (optional)

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FD  Bad Station Number
         0x89FF  Failure

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     22 17  Purge Erased Files (old)
         22 28  Recover Salvageable File
         22 29  Purge Salvageable File

NCP:     22 17  Recover Erased File (old)

CHANGES: 16 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s17DelRecover
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buPathNameLen,
   pnstr8   pbstrPathName,
   pnstr8   pbstrOldFileNameB15,
   pnstr8   pbstrNewFileNameB15
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 17)
   #define NCP_STRUCT_LEN  ((nuint16) 3 + buPathNameLen)
   #define NCP_REQ_LEN     ((nuint) 5)
   #define NCP_REPLY_LEN   ((nuint) 30)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 1)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   abuReq[4] = buPathNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrPathName;
   reqFrag[1].uLen  = buPathNameLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      if (pbstrOldFileNameB15)
         NWCMemMove(pbstrOldFileNameB15, &abuReply[0], (nuint) 15);
      if (pbstrNewFileNameB15)
         NWCMemMove(pbstrNewFileNameB15, &abuReply[15], (nuint) 15);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s17.c,v 1.7 1994/09/26 17:33:54 rebekah Exp $
*/
