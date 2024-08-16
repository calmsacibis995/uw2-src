/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s4.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s4DirModMaxRightsMask**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s4DirModMaxRightsMask
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint8   buRightsGrantMask,
            nuint8   buRightsRevokeMask,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
         )

REMARKS: This call allows a client to modify the maximum access rights mask associated
         with a directory.  The directory's maximum access rights mask is first
         ANDed with the NOT of the Rights Revoke Mask specified by the client.  This
         operation removes any rights included in the Rights Revoke Mask from the
         directory's maximum access rights mask.  The result of this operation is then
         ORed with the Rights Grant Mask specified.  This operation adds the rights
         included in the Rights Grant Mask to the directory's maximum access rights
         mask.  The result is the directory's new maximum access rights mask.

         This call is successful if the client has access control rights to either
         the target directory or its parent directory.

ARGS: <> pAccess
      >  buDirHandle
      >  buRightsGrantMask
      >  buRightsRevokeMask
      >  buPathLen
      >  pbstrPath

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x898C  No Set Privileges
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FD  Bad Station Number
         0x89FF  Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     22 04  Modify Maximum Rights Mask

CHANGES: 15 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s4DirModMaxRightsMask
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buRightsGrantMask,
   nuint8   buRightsRevokeMask,
   nuint8   buPathLen,
   pnstr8   pbstrPath
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 4)
   #define NCP_STRUCT_LEN  ((nuint16) 5 + buPathLen)
   #define NCP_REQ_LEN     ((nuint) 7)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 0)

   NWCFrag reqFrag[NCP_REQ_FRAGS];
   nuint8  abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   abuReq[4] = buRightsGrantMask;
   abuReq[5] = buRightsRevokeMask;
   abuReq[6] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = buPathLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s4.c,v 1.7 1994/09/26 17:34:25 rebekah Exp $
*/
