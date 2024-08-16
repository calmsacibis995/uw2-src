/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s58.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s58DeleteProperty*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s58DeleteProperty
         (
            pNWAccess pAccess,
            nuint16  suObjType,
            nuint8   buObjNameLen,
            pnstr8   pbstrObjName,
            nuint8   buPropertyNameLen,
            pnstr8   pbstrPropertyName,
         );

REMARKS: Remove one or more properties from the specified object.

         The Object Type cannot be WILD (-1); the Object Name cannot contain wildcard
         characters.  The Property Name can contain wildcard characters, if desired;
         if the property name contains wildcard characters, all properties that match
         the name string will be removed from the object.

         The object and the property's security level masks determine whether a
         property can be removed.  Properties with security levels above that of the
         requesting client will not be "visible" to the client and, therefore, cannot
         be read, written to, or deleted by the client.

ARGS: <> pAccess
      >  suObjType
      >  buObjNameLen
      >  pbstrObjName
      >  buPropertyNameLen
      >  pbstrPropertyName

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89F0  Illegal Wildcard
         0x89F1  Bindery Security
         0x89F6  No Property Delete
         0x89FB  No Such Property
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 57  Create Property

NCP:     23 58  Delete Property

CHANGES: 24 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s58DeleteProperty
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buPropertyNameLen,
   pnstr8   pbstrPropertyName
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 58)
   #define NCP_STRUCT_LEN  ((nuint16) (5 + buObjNameLen + buPropertyNameLen))
   #define REQ_LEN         ((nuint) 6)
   #define REQ_FRAGS       ((nuint) 4)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8 abuReq[REQ_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReq[3], &suObjType);
   abuReq[5] = buObjNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrObjName;
   reqFrag[1].uLen  = buObjNameLen;

   reqFrag[2].pAddr = &buPropertyNameLen;
   reqFrag[2].uLen  = (nuint) 1;

   reqFrag[3].pAddr = pbstrPropertyName;
   reqFrag[3].uLen  = buPropertyNameLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s58.c,v 1.7 1994/09/26 17:37:19 rebekah Exp $
*/
