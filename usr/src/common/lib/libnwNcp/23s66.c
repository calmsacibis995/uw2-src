/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s66.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s66DeleteObjFromSet*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s66DeleteObjFromSet
         (
            pNWAccess pAccess,
            nuint16  suObjType,
            nuint8   buObjNameLen,
            pnstr8   pbstrObjName,
            nuint8   buPropertyNameLen,
            pnstr8   pbstrPropertyName,
            nuint16  suMemberType,
            nuint8   buMemberNameLen,
            pnstr8   pbstrMemberName,
         );

REMARKS: Remove a member from a set property of the specified object.

         The object, the property, and the member are specified by name.  The Object
         Type cannot be WILD (-1); the Object Name and Property Name cannot contain
         wildcard characters.

         Clients must have write privileges to the specified property.  If the
         property security is "write supervisor," the client must supervise both the
         object being deleted and the object containing the affected set property.

ARGS: <> pAccess
      >  suObjType
      >  buObjNameLen
      >  pbstrObjName
      >  buPropertyNameLen
      >  pbstrPropertyName
      >  suMemberType
      >  buMemberNameLen
      >  pbstrMemberName

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89EB  Property Not Set Property
         0x89F0  Illegal Wildcard
         0x89F8  No Property Write
         0x89FB  No Such Property
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 65  Add Bindery Object To Set

NCP:     23 66  Delete Bindery Object From Set

CHANGES: 24 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s66DeleteObjFromSet
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buPropertyNameLen,
   pnstr8   pbstrPropertyName,
   nuint16  suMemberType,
   nuint8   buMemberNameLen,
   pnstr8   pbstrMemberName
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 66)
   #define NCP_STRUCT_LEN  ((nuint16) (8 + buObjNameLen + buPropertyNameLen + buMemberNameLen))
   #define MAX_PROPERTY_LEN ((nuint) 16)
   #define REQ1_LEN        ((nuint) 6)
   #define REQ2_LEN        ((nuint) 4)
   #define REQ_FRAGS       ((nuint) 4)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8 abuReqA[REQ1_LEN], abuReqB[REQ2_LEN+MAX_PROPERTY_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReqA[0], &suNCPLen);
   abuReqA[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReqA[3], &suObjType);
   abuReqA[5] = buObjNameLen;

   abuReqB[0] = buPropertyNameLen;
   NWCMemMove(&abuReqB[1], pbstrPropertyName, buPropertyNameLen);
   NCopyHiLo16(&abuReqB[1+buPropertyNameLen], &suMemberType);
   abuReqB[3+buPropertyNameLen] = buMemberNameLen;

   reqFrag[0].pAddr = abuReqA;
   reqFrag[0].uLen  = REQ1_LEN;

   reqFrag[1].pAddr = pbstrObjName;
   reqFrag[1].uLen  = (nuint) buObjNameLen;

   reqFrag[2].pAddr = abuReqB;
   reqFrag[2].uLen  = (nuint) (REQ2_LEN + buPropertyNameLen);

   reqFrag[3].pAddr = pbstrMemberName;
   reqFrag[3].uLen  = (nuint) buMemberNameLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s66.c,v 1.7 1994/09/26 17:37:28 rebekah Exp $
*/
