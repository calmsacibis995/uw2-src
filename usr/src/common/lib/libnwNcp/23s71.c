/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s71.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s71ScanObjTrustPaths*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s71ScanObjTrustPaths
         (
            pNWAccess pAccess,
            nuint8   buVolNum,
            pnuint16 psuIterHnd,
            nuint32  luObjID,
            pnuint32 pluRetObjID,
            pnuint8  pbuTrusAccessMask,
            pnuint8  pbuPathNameLen,
            pnstr8   pbstrPathName,
         );

REMARKS:

ARGS: <> pAccess
      >  buVolNum
      <> psuIterHnd
      >  luObjID
      <  pluRetObjID (optional)
      <  pbuTrusAccessMask (optional)
      <  pbuPathNameLen
      <  pbstrPathName

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x8993  No Read Privileges
         0x8996  Server Out Of Memory
         0x89A1  Directory I/O Error
         0x89F0  Illegal Wildcard
         0x89F1  Bindery Security
         0x89F2  No Object Read
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 55  Scan Bindery Object

NCP:     23 71  Scan Bindery Object Trustee Paths

CHANGES: 23 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s71ScanObjTrustPaths
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   pnuint16 psuIterHnd,
   nuint32  luObjID,
   pnuint32 pluRetObjID,
   pnuint8  pbuTrusAccessMask,
   pnuint8  pbuPathNameLen,
   pnstr8   pbstrPathName
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 71)
   #define NCP_STRUCT_LEN  ((nuint16) 8)
   #define MAX_NAME_LEN    ((nuint) 255)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 8)
   #define REQ_FRAGS       ((nuint) 1)
   #define REPLY_FRAGS     ((nuint) 2)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;
   NCopyHiLo16(&abuReq[4], psuIterHnd);
   NCopyHiLo32(&abuReq[6], &luObjID);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = pbstrPathName;
   replyFrag[1].uLen  = MAX_NAME_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      NCopyHiLo16(psuIterHnd, &abuReply[0]);
      if (pluRetObjID)
         NCopyHiLo32(pluRetObjID, &abuReply[2]);
      if (pbuTrusAccessMask)
         *pbuTrusAccessMask = abuReply[6];
      *pbuPathNameLen = abuReply[7];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s71.c,v 1.7 1994/09/26 17:37:34 rebekah Exp $
*/
