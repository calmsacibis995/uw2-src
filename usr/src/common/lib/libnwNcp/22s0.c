/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s0SetDirHandle**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s0SetDirHandle
         (
            pNWAccess pAccess,
            nuint8   buTargetDirHandle,
            nuint8   buDirHandle,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
            pnuint8  pbuNewDirHandle,
            pnuint8  pbuRightsMask,
            pnuint8  pbuReserved,
         );

REMARKS: This call changes the directory pointed to by Target Directory Handle to the
         directory pointed to by Source Directory Handle and Directory Path.  The
         Target Directory Handle must already exist.  To create a new directory handle,
         a client must use one of the functions to create a permanent or temporary
         directory handle (function 22, subfunctions 18, 19, or 22).

         The client can specify the complete Directory Path and can use a Source
         Directory Handle of zero, if desired.

         The Directory Handle returned by the server will be the same handle as the
         Target Directory Handle in the request message.

         The Access Rights Mask returned by the server contains the client's effective
         access privileges in the indicated directory.

ARGS: <> pAccess
      >  buTargetDirHandle
      >  buDirHandle
      >  buPathLen
      >  pbstrPath
      <  pbuNewDirHandle
      <  pbuRightsMask (optional)
      <  pbuReserved (optional)

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FA  Temporary Remap Error
         0x89FD  Bad Station Number
         0x89FF  Failure

SERVER:

CLIENT:  DOS OS2 WIN NT

SEE:     22 18  Alloc Permanent Directory Handle
         22 19  Alloc Temporary Directory Handle
         22 20  Deallocate Directory Handle
         22 22  Alloc Special Temporary Directory Handle

NCP:     22 00  Set Directory Handle

CHANGES: 13 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s0SetDirHandle
(
   pNWAccess pAccess,
   nuint8   buTargetDirHandle,
   nuint8   buDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint8  pbuNewDirHandle,
   pnuint8  pbuRightsMask,
   pnuint8  pbuReserved
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 0)
   #define NCP_STRUCT_LEN  ((nuint16) 4 +buPathLen)
   #define NCP_REQ_LEN     ((nuint) 6)
   #define NCP_REPLY_LEN   ((nuint) 3)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 1)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8  abuReq[NCP_REQ_LEN],   abuReply[NCP_REPLY_LEN];
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buTargetDirHandle;
   abuReq[4] = buDirHandle;
   abuReq[5] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = (nuint) NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = (nuint) buPathLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = (nuint) NCP_REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      *pbuNewDirHandle = abuReply[0];
      if (pbuRightsMask)
         *pbuRightsMask = abuReply[1];
      if (pbuReserved)
         *pbuReserved = abuReply[2];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s0.c,v 1.7 1994/09/26 17:33:41 rebekah Exp $
*/
