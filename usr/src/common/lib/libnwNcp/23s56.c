/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s56.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s56ChangeObjectSecurity**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s56ChangeObjSecurity
         (
            pNWAccess pAccess,
            nuint8   buNewSecurityLevel,
            nuint16  suObjType,
            nuint8   buObjNameLen,
            pnstr8   pbstrObjName,
            pnuint8  pbuConnStatus,
         )

REMARKS: This call allows an object supervisor to change the security level
         mask of an object in the bindery.

         The Object Type cannot be WILD (-1), and the Object Name cannot
         contain wildcard characters.

         Security levels above level 3 (supervisor) cannot be changed with
         this call.

         Only object supervisors can use this call.

ARGS: <> pAccess
      >  buNewSecurityLevel
      >  suObjType
      >  buObjNameLen
      >  pbstrObjName
      <  pbuConnStatus

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89F0  Illegal Wildcard
         0x89F1  Bindery Security
         0x89F5  No Object Create
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     23 70  Get Bindery Access Level
         23 72  Get Bindery Object Access Level

NCP:     23 56  Change Bindery Object Security

CHANGES: 24 Aug 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s56ChangeObjSecurity
(
   pNWAccess pAccess,
   nuint8   buNewSecurityLevel,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   pnuint8  pbuConnStatus
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 56)
   #define NCP_STRUCT_LEN  ((nuint16) (5 + buObjNameLen))
   #define REQ_LEN         ((nuint) 7)
   #define REPLY_LEN       ((nuint) 1)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 1)

   nuint8 abuReq[REQ_LEN], buBucket;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint16 suNCPLen;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0],&suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buNewSecurityLevel;
   NCopyHiLo16(&abuReq[4],&suObjType);
   abuReq[6] = buObjNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrObjName;
   reqFrag[1].uLen  = buObjNameLen;

   replyFrag[0].pAddr = pbuConnStatus ? pbuConnStatus: &buBucket;
   replyFrag[0].uLen  = REPLY_LEN;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s56.c,v 1.7 1994/09/26 17:37:17 rebekah Exp $
*/
