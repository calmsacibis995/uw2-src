/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:34s6.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncptts.h"

/*manpage*NWNCP34s6TTSSetAppThresholds*****************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP34s6TTSSetAppThresholds
         (
            pNWAccess pAccess,
            nuint8   buLogLockThreshold,
            nuint8   buPhyLockThreshold,
         )

REMARKS: Allows an application to specify the number of logical and
         physical record locks allowed before an implicit transaction
         begins. Thresholds set by this call are valid for the calling
         application only.

ARGS: <> pAccess
      >  buLogLockThreshold
      >  buPhyLockThreshold

INCLUDE: ncptts.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     34 05  TTS Get Application Thresholds

NCP:     34 06  TTS Set Application Thresholds

CHANGES: 13 Aug 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP34s6TTSSetAppThresholds
(
   pNWAccess pAccess,
   nuint8   buLogLockThreshold,
   nuint8   buPhyLockThreshold
)
{
   #define NCP_FUNCTION    ((nuint) 34)
   #define NCP_SUBFUNCTION ((nuint8) 6)
   #define REQ_LEN         ((nuint) 3)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buLogLockThreshold;
   abuReq[2] = buPhyLockThreshold;

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/34s6.c,v 1.7 1994/09/26 17:38:05 rebekah Exp $
*/
