/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s67.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s67IsObjInSet*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s67IsObjInSet
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
         )

REMARKS:

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
         0x89EA  No Such Member
         0x89EB  Property Not Set Property
         0x89EC  No Such Set
         0x89F0  Illegal Wildcard
         0x89FB  No Such Property
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     23 65  Add Bindery Object To Set

NCP:     23 67  Is Bindery Object In Set

CHANGES: 25 Aug 1993 - written - srydalch
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s67IsObjInSet
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
   #define NCP_SUBFUNCTION ((nuint8) 67)
   #define NCP_STRUCT_LEN  ((nuint16) (8 + buObjNameLen + buPropertyNameLen + buMemberNameLen))
   #define MAX_PROPERTY_LEN ((nuint) 16)
   #define REQ1_LEN        ((nuint) 6)
   #define REQ2_LEN        ((nuint) 4)
   #define REQ_FRAGS       ((nuint) 4)
   #define REPLY_FRAGS     ((nuint) 0)

   NWCFrag  reqFrag[REQ_FRAGS];
   nuint16  suNCPLen;
   nuint8   abuReq1[REQ1_LEN], abuReq2[REQ2_LEN+MAX_PROPERTY_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq1[0], &suNCPLen);
   abuReq1[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReq1[3], &suObjType);
   abuReq1[5] = buObjNameLen;

   abuReq2[0] = buPropertyNameLen;
   NWCMemMove(&abuReq2[1], pbstrPropertyName, buPropertyNameLen);
   NCopyHiLo16(&abuReq2[1+buPropertyNameLen], &suMemberType);
   abuReq2[3+buPropertyNameLen] = buMemberNameLen;

   reqFrag[0].pAddr = abuReq1;
   reqFrag[0].uLen  = REQ1_LEN;

   reqFrag[1].pAddr = pbstrObjName;
   reqFrag[1].uLen  = buObjNameLen;

   reqFrag[2].pAddr = abuReq2;
   reqFrag[2].uLen  = (nuint) (REQ2_LEN + buPropertyNameLen);

   reqFrag[3].pAddr = pbstrMemberName;
   reqFrag[3].uLen  = buMemberNameLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s67.c,v 1.7 1994/09/26 17:37:29 rebekah Exp $
*/