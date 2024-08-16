/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:114s1.c	1.3"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpds.h"


/*manpage*NWNCP*******************************************
SYNTAX:  N_EXTERN_LIBRARY(NWRCODE)
         NWNCP114s1GetUTCTime
         (
            pNWAccess pAccess,
            pnuint32 pluSeconds,
            pnuint32 pluSubSeconds,
            pnuint32 pluStatus,
            pnuint32 pluEventOffset1,
            pnuint32 pluEventOffset2,
            pnuint32 pluAdjustmentTime,
            pnuint32 pluEventType,
         )

REMARKS:

ARGS:

INCLUDE:

RETURN:

SERVER:

CLIENT:

SEE:

NCP:

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY(NWRCODE)
NWNCP114s1GetUTCTime
(
   pNWAccess pAccess,
   pnuint32 pluSeconds,
   pnuint32 pluSubSeconds,
   pnuint32 pluStatus,
   pnuint32 pluEventOffset1,
   pnuint32 pluEventOffset2,
   pnuint32 pluAdjustmentTime,
   pnuint32 pluEventType
)
{
   nint32   lCode;
   nuint8   abuReq[3];
   nuint8   abuRep[100];
   nuint    uRepLen;

   abuReq[0] = (nuint8) 0;   /* length, 2 bytes */
   abuReq[1] = (nuint8) 1;
   abuReq[2] = (nuint8) 1;    /* subfunction */

   lCode = NWCRequestSingle(pAccess, (nuint)114, abuReq, (nuint)3, abuRep,
                            (nuint)100, &uRepLen);

   if (lCode == 0)
   {
      if (pluSeconds)
         NCopyLoHi32(pluSeconds, &abuRep[0]);
      if (pluSubSeconds)
         NCopyLoHi32(pluSubSeconds, &abuRep[4]);
      if (pluStatus)
         NCopyLoHi32(pluStatus, &abuRep[8]);
      if (pluEventOffset1)
         NCopyLoHi32(pluEventOffset1, &abuRep[12]);
      if (pluEventOffset2)
         NCopyLoHi32(pluEventOffset2, &abuRep[16]);
      if (pluAdjustmentTime)
         NCopyLoHi32(pluAdjustmentTime, &abuRep[20]);
      if (pluEventType)
         NCopyLoHi32(pluEventType, &abuRep[24]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/114s1.c,v 1.4 1994/09/26 17:32:01 rebekah Exp $
*/
