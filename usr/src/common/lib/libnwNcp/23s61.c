/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s61.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s61ReadPropertyValue***********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s61ReadPropertyValue
         (
            pNWAccess pAccess,
            nuint16  suObjType,
            nuint8   buObjNameLen,
            pnstr8   pbstrObjName,
            nuint8   buPropertyNameLen,
            pnstr8   pbstrPropertyName,
            pnuint8  pbuPropertyValueB128,
            pnuint8  pbuMoreFlag,
            pnuint8  pbuPropertyFlags,
         )

REMARKS: Allow a client to retrieve the value associated with
         the specified property.  Property values are stored in a single
         128-byte segment.  The Segment Number must be set to 1 to read the
         first segment of a value and must be incremented by one for each
         subsequent call.  The 128-byte segment corresponding to Segment
         Number is returned to the client.

         The More Flag will be set to 0xFF if the requested Segment Number
         is not the last segment in the value and 0 if the Segment Number
         is the last segment.

         The Property Flags byte contains the property's status flags.  This
         byte can be tested to determine whether the property is a set property
         or an item property and whether the property is static or dynamic.

         The Object Type cannot be WILD (-1); the Object Name and Property Name
         cannot contain wildcard characters.

         This call can be used successfully by clients with read privileges to
         the specified property.

ARGS: <> pAccess
      >  suObjType
      >  buObjNameLen
      >  pbstrObjName
      >  buSegNum
      >  buPropertyNameLen
      >  pbstrPropertyName
      <  pbuPropertyValueB128
      <  pbuMoreFlag       (optional)
      <  pbuPropertyFlags  (optional)

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x8988  Invalid File Handle
         0x8993  No Read Privileges
         0x8996  Server Out Of Memory
         0x89EC  No Such Set
         0x89F0  Illegal Wildcard
         0x89F1  Bindery Security
         0x89F9  No Property Read
         0x89FB  No Such Property
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     23 73  Is Calling Station a Manager
         23 60  Scan Property

NCP:     23 61  Read Property Value

CHANGES: 27 Aug 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s61ReadPropertyValue
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buSegNum,
   nuint8   buPropertyNameLen,
   pnstr8   pbstrPropertyName,
   pnuint8  pbuPropertyValueB128,
   pnuint8  pbuMoreFlag,
   pnuint8  pbuPropertyFlags
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 61)
   #define NCP_STRUCT_LEN  ((nuint16) (6 + buObjNameLen + buPropertyNameLen))
   #define REQ1_LEN        ((nuint) 6)
   #define REQ2_LEN        ((nuint) 2)
   #define PROPERTY_LEN    ((nuint) 128)
   #define REPLY_LEN       ((nuint) 2)
   #define REQ_FRAGS       ((nuint) 4)
   #define REPLY_FRAGS     ((nuint) 2)

   nint32   lCode;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint16 suNCPLen;
   nuint8 abuReq[REQ1_LEN], abuReq2[REQ2_LEN], abuReply[REPLY_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(abuReq, &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReq[3], &suObjType);
   abuReq[5] = buObjNameLen;

   abuReq2[0] = buSegNum;
   abuReq2[1] = buPropertyNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ1_LEN;

   reqFrag[1].pAddr = pbstrObjName;
   reqFrag[1].uLen  = (nuint) buObjNameLen;

   reqFrag[2].pAddr = abuReq2;
   reqFrag[2].uLen  = REQ2_LEN;

   reqFrag[3].pAddr = pbstrPropertyName;
   reqFrag[3].uLen  = (nuint) buPropertyNameLen;

   replyFrag[0].pAddr = pbuPropertyValueB128;
   replyFrag[0].uLen  = PROPERTY_LEN;

   replyFrag[1].pAddr = abuReply;
   replyFrag[1].uLen  = REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      if (pbuMoreFlag)
         *pbuMoreFlag = abuReply[0];
      if (pbuPropertyFlags)
         *pbuPropertyFlags = abuReply[1];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s61.c,v 1.7 1994/09/26 17:37:23 rebekah Exp $
*/
