/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s49.c	1.6"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s49OpenDataStream**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s49OpenDataStream
         (
            pNWAccess pAccess,
            nuint8   buDataStream,
            nuint8   buDirHandle,
            nuint8   buFileAttrs,
            nuint8   buOpenRights,
            nuint8   buFileNameLen,
            nuint8   pbuFileName,
            pnuint8  pbuNWHandleB4
         )

REMARKS: This function opens a data stream associated with any supported name space
         on the server.  It also returns a NetWare file handle.


ARGS: <> pAccess
       > buDataStream
       > buDirHandle
       > buFileAttrs
       > buOpenRights
       > buFileNameLen
       > pbuFileName
      <  pbuNWHandleB4

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8980  Lock Failure
         0x8982  No Open Privileges
         0x8990  Read-only Access To Volume
         0x89BE  Invalid Data Stream
         0x89FF  No Files Found

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     22 49  Open Data Stream

CHANGES: 13 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s49OpenDataStream
(
   pNWAccess pAccess,
   nuint8   buDataStream,
   nuint8   buDirHandle,
   nuint8   buFileAttrs,
   nuint8   buOpenRights,
   nuint8   buFileNameLen,
   pnstr8   pbuFileName,
   pnuint8  pbuNWHandleB4
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 49)
   #define NCP_STRUCT_LEN  ((nuint16) 6 + buFileNameLen)
   #define NCP_REQ_LEN     ((nuint) 8)
   #define NCP_REPLY_LEN   ((nuint) 4)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 1)

   nint32  lCode;
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDataStream;
   abuReq[4] = buDirHandle;
   abuReq[5] = buFileAttrs;
   abuReq[6] = buOpenRights;
   abuReq[7] = buFileNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbuFileName;
   reqFrag[1].uLen  = buFileNameLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = NCP_REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL);

   if (lCode == 0)
   {
	  NCopyHiLo32(pbuNWHandleB4, &abuReply[0]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s49.c,v 1.9 1994/09/26 17:34:34 rebekah Exp $
*/
