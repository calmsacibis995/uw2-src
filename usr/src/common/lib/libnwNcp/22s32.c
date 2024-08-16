/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s32.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s32VolScanRestrict**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s32VolScanRestrict
         (
            pNWAccess             pAccess,
            nuint8               buVolNum,
            nuint32              luIterHnd,
            pnuint8              pbuNumEntries,
            pNWNCPRestrictions   pRestrictionsB12,
         );

REMARKS: This function returns a list of the object restrictions for a specified
         volume.  All restrictions are in 4K blocks.  A restriction may be zero (0).
         If a restriction is greater than 0x40000000, then that entry has no
         restriction.  The maximum number of entries is twelve (12).  The Sequence
         starts at zero (0) and increments by the value of NumberOfEntries.

ARGS: <> pAccess
      >  buVolNum
      >  luIterHnd
      <  pbuNumEntries
      <  pRestrictionsB12

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8998  Invalid Volume

SERVER:

CLIENT:  DOS OS2 WIN NT

SEE:     22 35  Get Directory Disk Space Restrictions
         22 36  Set Directory Disk Space Restrictions
         22 33  Add User Disk Space Restriction
         22 34  Remove User Disk Space Restriction
         22 41  Get Object Disk Usage And Restrictions

NCP:     22 32  Scan Volume's User Disk Restrictions

CHANGES: 15 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s32VolScanRestrict
(
   pNWAccess             pAccess,
   nuint8               buVolNum,
   nuint32              luIterHnd,
   pnuint8              pbuNumEntries,
   pNWNCPRestrictions   pRestrictionsB12
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 32)
   #define NCP_STRUCT_LEN  ((nuint16) 6)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 97)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN], i;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;
   NCopyLoHi32(&abuReq[4], &luIterHnd);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuReply, NCP_REPLY_LEN, NULL);
   if(lCode == 0)
   {
       *pbuNumEntries = abuReply[0];
       for(i = 0; i < *pbuNumEntries; i++)
       {
           NCopyHiLo32(&pRestrictionsB12[i].luObjID, &abuReply[i*8+1]);
           NCopyLoHi32(&pRestrictionsB12[i].luRestriction, &abuReply[i*8+5]);
       }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s32.c,v 1.7 1994/09/26 17:34:15 rebekah Exp $
*/
