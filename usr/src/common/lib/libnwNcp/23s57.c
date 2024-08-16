/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s57.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s57CreateProperty*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s57CreateProperty
         (
            pNWAccess pAccess,
            nuint16  suObjType,
            nuint8   buObjNameLen,
            pnstr8   pbstrObjName,
            nuint8   buPropertyFlags,
            nuint8   buPropertySecurity,
            nuint8   buPropertyNameLen,
            pnstr8   pbstrPropertyName,
         );

REMARKS: Create a new property and attaches that property to the specified
         object.

         The Object Type cannot be WILD (-1); the Object Name and Property Name cannot
         contain wildcard characters.

         Bit 0 of Property Flags should be set if the property is dynamic; bit 1 of
         Property Flags should be set if the property is a set property.  Property
         Security mask should be constructed according to the instructions for Access
         Rights located in the introduction to this section.

         The Property Security mask attached to the target object regulates which
         clients can use this call.

ARGS: <> pAccess
      >  suObjType
      >  buObjNameLen
      >  pbstrObjName
      >  buPropertyFlags
      >  buPropertySecurity
      >  buPropertyNameLen
      >  pbstrPropertyName

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89ED  Property Exists
         0x89EF  Illegal Name
         0x89F0  Illegal Wildcard
         0x89F1  Bindery Security
         0x89F2  No Object Read
         0x89F6  No Property Delete
         0x89F7  No Property Create
         0x89FB  No Such Property
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 59  Change Property Security
         23 58  Delete Property
         23 60  Scan Property

NCP:     23 57  Create Property

CHANGES: 23 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s57CreateProperty
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buPropertyFlags,
   nuint8   buPropertySecurity,
   nuint8   buPropertyNameLen,
   pnstr8   pbstrPropertyName
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 57)
   #define NCP_STRUCT_LEN  ((nuint16) (7 + buObjNameLen + buPropertyNameLen))
   #define REQ_LEN_A       ((nuint) 6)
   #define REQ_LEN_B       ((nuint) 3)
   #define REQ_FRAGS       ((nuint) 4)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8 abuReqA[REQ_LEN_A], abuReqB[REQ_LEN_B];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReqA[0], &suNCPLen);
   abuReqA[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReqA[3], &suObjType);
   abuReqA[5] = buObjNameLen;

   abuReqB[0] = buPropertyFlags;
   abuReqB[1] = buPropertySecurity;
   abuReqB[2] = buPropertyNameLen;

   reqFrag[0].pAddr = abuReqA;
   reqFrag[0].uLen  = REQ_LEN_A;

   reqFrag[1].pAddr = pbstrObjName;
   reqFrag[1].uLen  = buObjNameLen;

   reqFrag[2].pAddr = abuReqB;
   reqFrag[2].uLen  = REQ_LEN_B;

   reqFrag[3].pAddr = pbstrPropertyName;
   reqFrag[3].uLen  = buPropertyNameLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s57.c,v 1.7 1994/09/26 17:37:18 rebekah Exp $
*/
