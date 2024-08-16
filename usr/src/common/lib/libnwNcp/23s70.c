/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s70.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s70GetClntAccessMask**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s70GetClntAccessMask
         (
            pNWAccess pAccess,
            pnuint8  pbuSecurityLevel,
            pnuint32 pluObjID,
         )

REMARKS: This call returns the client's current security level mask in the bindery;
         the client can then examine the two nibbles of this mask to determine
         the client's general bindery rights.  The meanings of the two nibbles in
         the security level mask are explained in the introduction to this section.

         The client also has access privileges (read and write) to the object used
         when logging in.  These privileges will not be reflected in the general
         security access level mask, so the trustee ID number of the object used to
         log in is also returned to the client as Logged Object ID.  The client has
         supervisor access privileges (0x44) to any object that contains the Logged
         Object ID in its OPERATORS property.

         This call can be used by any client.

ARGS: <> pAccess
      <  pbuSecurityLevel
      <  pluObjID

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 72  Get Bindery Object Access Level

NCP:     23 70  Get Bindery Access Level

CHANGES: 20 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s70GetClntAccessMask
(
   pNWAccess pAccess,
   pnuint8  pbuSecurityLevel,
   pnuint32 pluObjID
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 70)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 5)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      if (pbuSecurityLevel)
         *pbuSecurityLevel = abuReply[0];
      if (pluObjID)
         NCopyHiLo32(pluObjID, &abuReply[1]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s70.c,v 1.7 1994/09/26 17:37:33 rebekah Exp $
*/
