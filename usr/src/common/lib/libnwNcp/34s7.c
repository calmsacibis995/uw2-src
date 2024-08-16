/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:34s7.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncptts.h"

/*manpage*NWNCP34s7TTSGetThresholds**************************************
SYNTAX:  N_GLOBAL_LIBRARY (NWRCODE)
         NWNCP34s7TTSGetThresholds
         (
            pNWAccess pAccess,
            pnuint8  pbuLogLockThreshold,
            pnuint8  pbuPhyLockThreshold,
         )
REMARKS:

ARGS: <> pAccess
      <  pbuLogLockThreshold (optional)
      <  pbuPhyLockThreshold (optional)

INCLUDE: nwtts.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     34 07  TTS Get Workstation Thresholds

CHANGES: 12 Aug 1993 -written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY (NWRCODE)
NWNCP34s7TTSGetThresholds
(
   pNWAccess pAccess,
   pnuint8  pbuLogLockThreshold,
   pnuint8  pbuPhyLockThreshold
)
{
   #define NCP_FUNCTION    ((nuint) 34)
   #define NCP_SUBFUNCTION ((nuint8) 7)
   #define REQ_LEN         ((nuint) 1)
   #define REPLY_LEN       ((nuint) 2)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];

   abuReq[0] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);

   if (lCode == 0)
   {
      if(pbuLogLockThreshold != NULL)
         *pbuLogLockThreshold = abuReply[0];

      if(pbuPhyLockThreshold != NULL)
         *pbuPhyLockThreshold = abuReply[1];
   }

   return( (NWRCODE) lCode );
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/34s7.c,v 1.7 1994/09/26 17:38:06 rebekah Exp $
*/
