/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s41.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s41VolGetRestrict**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s41VolGetRestrict
         (
            pNWAccess pAccess,
            nuint8   buVolNum,
            nuint32  luObjID,
            pnuint32 pluRestriction,
            pnuint32 pluInUse
         )

REMARKS: This function scans a user's disk restrictions for a volume and returns the
         amount of space currently being used.  The space values are in 4K blocks.
         If the restriction is 0x40000000 then there is no restriction for the object.
         Note that this call succeeds if the object ID is invalid--returning no
         restrictions and no space being used.

ARGS: <> pAccess
      >  buVolNum
      >  luObjID
      <  pluRestriction (optional)
      <  pluInUse (optional)

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8998  Invalid Volume

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 33  Add User Disk Space Restriction
         22 34  Remove User Disk Space Restriction
         22 32  Scan Volume's User Disk Restrictions
         22 36  Set Directory Disk Space Restriction
         22 40  Scan Directory Disk Space

NCP:     22 41  Get Object Disk Usage And Restrictions

CHANGES: 14 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s41VolGetRestrict
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luObjID,
   pnuint32 pluRestriction,
   pnuint32 pluInUse
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 41)
   #define NCP_STRUCT_LEN  ((nuint16) 6)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 8)

   nint32  lCode;
   nuint16 suNCPLen;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);

   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;
   NCopyHiLo32(&abuReq[4], &luObjID);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuReply, NCP_REPLY_LEN, NULL);
   if (lCode == 0)
   {
      if(pluRestriction)
         NCopyLoHi32(pluRestriction, &abuReply[0]);
      if(pluInUse)
         NCopyLoHi32(pluInUse, &abuReply[4]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s41.c,v 1.7 1994/09/26 17:34:27 rebekah Exp $
*/
