/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s59.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s59ChgPropertySecurity*************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s59ChgPropertySecurity
         (
            pNWAccess pAccess,
            nuint16  suObjType,
            nuint8   buObjNameLen,
            pnstr8   pbstrObjName,
            nuint8   buNewPropertySecurity,
            nuint8   buPropertyNameLen,
            pnstr8   pbstrPropertyName,
            pnuint8  pbuConnStatus,
         );

REMARKS: Allow a client to change the security level mask on a property.

         The Object Type cannot be WILD (-1), and the Object Name and the Property
         Name cannot contain wildcard characters.

         This call can be used successfully by clients who have property creation
         privileges for the specified object and property read and write privileges
         for the specified property.  A client cannot change a property's security
         to any level above the security level the client has to the property.

ARGS:<>  pAccess,
      >  suObjType,
      >  buObjNameLen,
      >  pbstrObjName,
      >  buNewPropertySecurity,
      >  buPropertyNameLen,
      >  pbstrPropertyName,
      <  pbuConnStatus,

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89F0  Illegal Wildcard
         0x89F1  Bindery Security
         0x89F2  No Object Read
         0x89F6  No Property Delete
         0x89FB  No Such Property
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 57  Create Property

NCP:     23 59  Change Property Security

CHANGES: 24 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s59ChgPropertySecurity
(
   pNWAccess pAccess,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName,
   nuint8   buNewPropertySecurity,
   nuint8   buPropertyNameLen,
   pnstr8   pbstrPropertyName,
   pnuint8  pbuConnStatus
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 59)
   #define NCP_STRUCT_LEN  ((nuint16) (6 + buObjNameLen + buPropertyNameLen))
   #define REQ_LEN_A       ((nuint) 6)
   #define REQ_LEN_B       ((nuint) 2)
   #define REQ_FRAGS       ((nuint) 4)
   #define REPLY_FRAGS     ((nuint) 1)

   nuint8 abuReqA[REQ_LEN_A], abuReqB[REQ_LEN_B];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReqA[0], &suNCPLen);
   abuReqA[2] = NCP_SUBFUNCTION;
   NCopyHiLo16(&abuReqA[3], &suObjType);
   abuReqA[5] = buObjNameLen;

   abuReqB[0] = buNewPropertySecurity;
   abuReqB[1] = buPropertyNameLen;

   reqFrag[0].pAddr = abuReqA;
   reqFrag[0].uLen  = REQ_LEN_A;

   reqFrag[1].pAddr = pbstrObjName;
   reqFrag[1].uLen  = buObjNameLen;

   reqFrag[2].pAddr = abuReqB;
   reqFrag[2].uLen  = REQ_LEN_B;

   reqFrag[3].pAddr = pbstrPropertyName;
   reqFrag[3].uLen  = buPropertyNameLen;

   replyFrag[0].pAddr = pbuConnStatus;
   replyFrag[0].uLen  = (nuint) 1;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s59.c,v 1.7 1994/09/26 17:37:20 rebekah Exp $
*/
