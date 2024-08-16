/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s52.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s52RenameObj
         (
            pNWAccess pAccess,
            nuint16  suObjType,
            nuint8   buObjNameLen,
            pnstr8   pbstrObjName,
            nuint8   buNewObjNameLen,
            pnstr8   pbstrNewObjName,
         );

REMARKS:
         This call renames a bindery object.

         Wildcard characters are not allowed in either the old or new name
         specifications.  The Object Type must not be WILD (-1) and must agree with
         the Object Type recorded in the bindery.

         Only an object supervisor can use this call.

ARGS: >  pAccess
      >  suObjType
      >  buObjNameLen
      >  pbstrObjName
      >  buNewObjNameLen
      >  pbstrNewObjName

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89EE  Object Exists
         0x89F0  Illegal Wildcard
         0x89F3  No Object Rename
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     23 52  Rename Object

CHANGES: 26 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s52RenameObj
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buNewObjNameLen,
   pnstr8   pbstrNewObjName
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 52)
   #define NCP_STRUCT_LEN  ((nuint16) (5 + buObjNameLen + buNewObjNameLen))
   #define REQ_LEN         ((nuint) 6)
   #define REQ_FRAGS       ((nuint) 4)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8 reqBuf[REQ_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&reqBuf[0], &suNCPLen);
   reqBuf[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&reqBuf[3], &suObjType);
   reqBuf[5] = buObjNameLen;

   reqFrag[0].pAddr = reqBuf;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrObjName;
   reqFrag[1].uLen  = buObjNameLen;

   reqFrag[2].pAddr = &buNewObjNameLen;
   reqFrag[2].uLen  = (nuint) 1;

   reqFrag[3].pAddr = pbstrNewObjName;
   reqFrag[3].uLen  = buNewObjNameLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s52.c,v 1.7 1994/09/26 17:37:11 rebekah Exp $
*/
