/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s60.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage* ****************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s60ScanProperty
         (
            pNWAccess pAccess,
            nuint16  suObjType,
            nuint8   buObjNameLen,
            pnstr8   pbstrObjName,
            pnuint32 pluIterHnd,
            nuint8   buSrchPropertyNameLen,
            pnstr8   pbstrSrchPropertyName,
            pnstr8   pbstrPropertyName,
            pnuint8  pbuPropertyFlags,
            pnuint8  pbuPropertySecurity,
            pnuint8  pbuValueAvailable,
            pnuint8  pbuMoreFlag,
         )

REMARKS: This call allows a client to scan the properties that are attached to a
         bindery object.

         The Object Type cannot be WILD (-1);the Object Name and the Property Name
         cannot contain wildcard characters.

         The client must set Last Instance to -1L (nuint32 -1) the first time this
         call is used.  If the client uses this call iteratively to scan an object's
         properties, Last Instance must be set to the Search Instance returned by
         the previous call.

         Value Available is 0 if the property does not have an associated value and
         -1 (0xFF) if the property does have a value.

         More Properties is 0 if the property is the last property associated with
         the object and -1 (0xFF) if it is not.

         This call can be used successfully by clients that have read privileges
         for both the target object (Object Name) and the target property (Search
         Property Name).

ARGS: <> pAccess
      >  suObjType
      >  buObjNameLen
      >  pbstrObjName
      <> pluIterHnd
      >  buSrchPropertyNameLen
      >  pbstrSrchPropertyName
      <  pbstrPropertyName (optional)
      <  pbuPropertyFlags (optional)
      <  pbuPropertySecurity (optional)
      <  pbuValueAvailable (optional)
      <  pbuMoreFlag (optional)

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89F0  Illegal Wildcard
         0x89FB  No Such Property
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     23 57  Create Property
         23 73  Is Calling Station a Manager
         23 61  Read Property Value

NCP:     23 60  Scan Property

CHANGES: 26 Aug 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s60ScanProperty
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   pnuint32 pluIterHnd,
   nuint8   buSrchPropertyNameLen,
   pnstr8   pbstrSrchPropertyName,
   pnstr8   pbstrPropertyName,
   pnuint8  pbuPropertyFlags,
   pnuint8  pbuPropertySecurity,
   pnuint8  pbuValueAvailable,
   pnuint8  pbuMoreFlag
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 60)
   #define NCP_STRUCT_LEN  ((nuint16) (9 + buObjNameLen + buSrchPropertyNameLen))
   #define REQ1_LEN        ((nuint) 6)
   #define REQ2_LEN        ((nuint) 5)
   #define REPLY1_LEN      ((nuint) 16)
   #define REPLY2_LEN      ((nuint) 8)
   #define REQ_FRAGS       ((nuint) 4)
   #define REPLY_FRAGS     ((nuint) 2)

   nint32   lCode;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint16 suNCPLen;
   nuint8  abuReq1[REQ1_LEN], abuReq2[REQ2_LEN],
           abuReply[REPLY2_LEN], abuBucket[REPLY1_LEN];

   suNCPLen   = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq1[0], &suNCPLen);
   abuReq1[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReq1[3], &suObjType);
   abuReq1[5] = buObjNameLen;

   NCopyHiLo32(&abuReq2[0], pluIterHnd);
   abuReq2[4] = buSrchPropertyNameLen;

   reqFrag[0].pAddr = abuReq1;
   reqFrag[0].uLen  = REQ1_LEN;

   reqFrag[1].pAddr = pbstrObjName;
   reqFrag[1].uLen  = buObjNameLen;

   reqFrag[2].pAddr = abuReq2;
   reqFrag[2].uLen  = REQ2_LEN;

   reqFrag[3].pAddr = pbstrSrchPropertyName;
   reqFrag[3].uLen  = buSrchPropertyNameLen;

   if (pbstrPropertyName != NULL)
      replyFrag[0].pAddr = pbstrPropertyName;
   else
      replyFrag[0].pAddr = abuBucket;

   replyFrag[0].uLen  = REPLY1_LEN;

   replyFrag[1].pAddr = abuReply;
   replyFrag[1].uLen  = REPLY2_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);

   if (lCode == 0)
   {
      NCopyHiLo32(pluIterHnd, &abuReply[2]);

      if(pbuPropertyFlags)
         *pbuPropertyFlags    = abuReply[0];
      if(pbuPropertySecurity)
         *pbuPropertySecurity = abuReply[1];
      if(pbuValueAvailable)
         *pbuValueAvailable   = abuReply[6];
      if(pbuMoreFlag)
         *pbuMoreFlag         = abuReply[7];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s60.c,v 1.7 1994/09/26 17:37:22 rebekah Exp $
*/
