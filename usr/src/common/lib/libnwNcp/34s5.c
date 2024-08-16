/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:34s5.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncptts.h"

/*manpage*NWNCP34s5TTSGetAppThresholds***************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP34s5TTSGetAppThresholds
         (
            pNWAccess pAccess,
            pnuint8  pbuLogLockThreshold,
            pnuint8  pbuPhyLockThreshold,
         );

REMARKS: Returns the number of logical and physical record locks allowed
         before an implicit transaction begins. The default threshold for
         both types is 0. If a value of 0x89FF is returned this means that
         implicit transcations for that lock type are not in effect.

ARGS: <> pAccess
      <  pbuLogLockThreshold (optional)
      <  pbuPhyLockThreshold (optional)

INCLUDE: ncptts.h

RETURN:  0x0000   Successful
         0x89FF   Implicit transactions for that lock type not in effect

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     34 06  TTS Set Application Thresholds

NCP:     34 05  TTS Get Application Thresholds

CHANGES: 13 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP34s5TTSGetAppThresholds
(
   pNWAccess pAccess,
   pnuint8  pbuLogLockThreshold,
   pnuint8  pbuPhyLockThreshold
)
{
   #define NCP_FUNCTION    ((nuint) 34)
   #define NCP_SUBFUNCTION ((nuint8) 5)
   #define REQ_LEN         ((nuint) 1)
   #define REPLY_LEN       ((nuint) 2)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];

   abuReq[0] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      if(pbuLogLockThreshold)
         *pbuLogLockThreshold = abuReply[0];

      if(pbuPhyLockThreshold)
         *pbuPhyLockThreshold = abuReply[1];
   }

   return( (NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/34s5.c,v 1.7 1994/09/26 17:38:04 rebekah Exp $
*/
