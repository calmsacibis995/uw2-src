/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s19.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s19AllocTempDirHandle**********************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP22s19AllocTempDirHandle
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint8   buRequestedDirHandle,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
            pnuint8  pbuNewDirHandle,
            pnuint8  pbuRightsMask,
         )

REMARKS: This call creates a new temporary directory handle for the client.  A client
         can have up to 255 directory handles in any combination of permanent and
         temporary handles.  When the client signals that the task is completed, the
         file server automatically deallocates any temporary directory handles
         associated with the task.

         Temporary directory handles do not need different handle names.  Each
         separate call will create a new handle unless the 255 handle maximum has
         already been reached.


ARGS: <> pAccess
      >  buDirHandle
      >  buRequestedDirHandle
      >  buPathLen
      >  pbstrPath
      <  pbuNewDirHandle
      <  pbuRightsMask (optional)

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x8999  Directory Full Error
         0x899C  Invalid Path
         0x899D  No Directory Handles
         0x89A1  Directory I/O Error
         0x89FD  Bad Station Number
         0x89FF  Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 18  Alloc Permanent Directory Handle
         22 22  Alloc Special Temporary Directory Handle
         22 20  Deallocate Directory Handle

NCP:     22 19  Alloc Temporary Directory Handle

CHANGES: 10 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s19AllocTempDirHandle
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buRequestedDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint8  pbuNewDirHandle,
   pnuint8  pbuRightsMask
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 19)
   #define NCP_STRUCT_LEN  ((nuint16) 4 + buPathLen)
   #define NCP_REQ_LEN     ((nuint) 6)
   #define NCP_REPLY_LEN   ((nuint) 2)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 1)

   nint32   lCode;
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   abuReq[4] = buRequestedDirHandle;
   abuReq[5] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = (nuint) buPathLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
   if(lCode == 0)
   {
      *pbuNewDirHandle = abuReply[0];

      if (pbuRightsMask)
         *pbuRightsMask   = abuReply[1];
   }
   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s19.c,v 1.7 1994/09/26 17:33:57 rebekah Exp $
*/
