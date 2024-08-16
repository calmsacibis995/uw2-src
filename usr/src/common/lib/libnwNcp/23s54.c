/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s54.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s54GetObjName***************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s54GetObjName
         (
            pNWAccess pAccess,
            nuint32  luObjID,
            pnuint32 pluRetObjID,
            pnuint16 psuObjType,
            pnstr8   pbstrObjName,
         )

REMARKS: This call allows a client to map an Object ID number to an Object Name
         and Object Type.

         Any client can use this call successfully if an object in the bindery
         corresponds to the indicated Object ID number and if the client has
         read privileges to that object.

ARGS: <> pAccess
      >  luObjID
      <  pluRetObjID (optional)
      <  psuObjType (optional)
      <  pbstrObjName (optional)

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89F1  Bindery Security
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     23 53  Get Bindery Object ID

NCP:     23 54  Get Bindery Object Name

CHANGES: 25 Aug 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s54GetObjName
(
   pNWAccess pAccess,
   nuint32  luObjID,
   pnuint32 pluRetObjID,
   pnuint16 psuObjType,
   pnstr8   pbstrObjName
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 54)
   #define NCP_STRUCT_LEN  ((nuint16) 5)
   #define MAX_OBJNAME_LEN ((nuint) 48)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 6)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 2)

   nint32   lCode;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint8  abuReq[REQ_LEN], abuReply[REPLY_LEN], abuBucket[MAX_OBJNAME_LEN];
   nuint16 suNCPLen;

   suNCPLen = REQ_LEN;
   NCopyHiLo16(&abuReq[0],&suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo32(&abuReq[3],&luObjID);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

/*   replyFrag[1].pAddr = pbstrObjName ? pbstrObjName : abuBucket; */
   if (pbstrObjName != NULL)
      replyFrag[1].pAddr = pbstrObjName;
   else
      replyFrag[1].pAddr = abuBucket;

   replyFrag[1].uLen  = MAX_OBJNAME_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      if (pluRetObjID)
         NCopyHiLo32(pluRetObjID, &abuReply[0]);
      if (psuObjType)
         NCopyHiLo16(psuObjType, &abuReply[4]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s54.c,v 1.7 1994/09/26 17:37:14 rebekah Exp $
*/
