/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s3.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s3GetDirEffRights**********************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP22s3GetDirEffRights
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
            pnuint8  pbuEffRightsMask,
         )

REMARKS: This call allows a client to determine the access rights the client has for
         a specified directory.  The Effective Rights Mask returned to the client
         indicates which of the eight possible directory rights the client has in the
         targeted directory.  An Effective Rights Mask of zero (0) indicates that the
         client has no rights in the target directory.

ARGS: <> pAccess
      >  buDirHandle
      >  buPathLen
      >  pbstrPath
      <  pbuEffRightsMask

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FD  Bad Station Number
         0x89FF  Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 42  Get Effective Rights For Directory Entry

NCP:     22 03  Get Effective Directory Rights (old)

CHANGES: 9 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s3GetDirEffRights
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint8  pbuEffRightsMask
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 3)
   #define NCP_STRUCT_LEN  ((nuint16) 3 + buPathLen)
   #define NCP_REQ_LEN     ((nuint) 5)
   #define NCP_REPLY_LEN   ((nuint) 1)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 1)

   NWCFrag reqFrag[NCP_REQ_FRAGS], replyFrag[NCP_REPLY_FRAGS];
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = (nuint8) NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   abuReq[4] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = (nuint) NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = (nuint) buPathLen;

   replyFrag[0].pAddr = pbuEffRightsMask;
   replyFrag[0].uLen  = (nuint) NCP_REPLY_LEN;

   return (NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, replyFrag, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s3.c,v 1.7 1994/09/26 17:34:11 rebekah Exp $
*/
