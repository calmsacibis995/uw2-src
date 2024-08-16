/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s53.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s53GetObjID***************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s53GetObjID
         (
            pNWAccess pAccess,
            nuint16  suObjType,
            nuint8   buObjNameLen,
            pnstr8   pbstrObjName,
            pnuint32 pluObjID,
            pnuint16 psuRetObjType,
            pnstr8   pbstrRetObjNameB48
         )

REMARKS: This call allows a client to map an Object Name to its corresponding
         unique Object ID number.  Object ID numbers are valid only on the
         server from which they are extracted since each server maintains its
         own bindery.

         The Object Type cannot be WILD (-1); the Object Name cannot contain
         wildcard characters.

         Any client can use this call successfully if the specified object is
         in the bindery and if the client has read security privileges to the
         object.

ARGS: <> pAccess
      >  suObjType
      >  buObjNameLen
      >  pbstrObjName
      <  pluObjID
      <  psuRetObjType (optional)
      <  pbstrRetObjNameB48 (optional)

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89F0  Illegal Wildcard
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     23 54  Get Bindery Object Name

NCP:     23 53  Get Bindery Object ID

CHANGES: 23 Aug 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s53GetObjID
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   pnuint32 pluObjID,
   pnuint16 psuRetObjType,
   pnstr8   pbstrRetObjNameB48
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 53)
   #define NCP_STRUCT_LEN  ((nuint16) (4 + buObjNameLen))
   #define REQ_LEN         ((nuint) 6)
   #define RETTYPE_LEN     ((nuint) 2)
   #define RETNAME_LEN     ((nuint) 48)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 3)

   nint32   lCode;
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint8  abuReq[REQ_LEN], abuBucket[RETNAME_LEN], abuBucket1[RETTYPE_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0],&suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReq[3],&suObjType);
   abuReq[5] = buObjNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrObjName;
   reqFrag[1].uLen  = buObjNameLen;

   replyFrag[0].pAddr = pluObjID;
   replyFrag[0].uLen  = (nuint) 4;

   replyFrag[1].pAddr = abuBucket1;
   replyFrag[1].uLen  = RETTYPE_LEN;

/*   replyFrag[2].pAddr = pbstrRetObjNameB48 ? pbstrRetObjNameB48: abuBucket; */

   if (pbstrRetObjNameB48 != NULL)
      replyFrag[2].pAddr = pbstrRetObjNameB48;
   else
      replyFrag[2].pAddr = abuBucket;

   replyFrag[2].uLen  = RETNAME_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      if(psuRetObjType)
      {
         NCopyHiLo16(psuRetObjType, &abuBucket1[0]);
      }

      *pluObjID = NSwapHiLo32(*pluObjID);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s53.c,v 1.7 1994/09/26 17:37:13 rebekah Exp $
*/

