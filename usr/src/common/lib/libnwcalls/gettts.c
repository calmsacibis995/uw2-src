/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gettts.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncptts.h"
#include "nwtts.h"
#include "nwcaldef.h"

/*manpage*NWGetTTSStats*****************************************************
SYNTAX:  NWCCODE N_API NWGetTTSStats
         (
            NWCONN_HANDLE conn,
            TTS_STATS NWPTR ttsStats
         );

REMARKS:

ARGS: > conn
      < ttsStats

INCLUDE: nwtts.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 213  Get Transaction Tracking Statistics

CHANGES: 18 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetTTSStats
(
   NWCONN_HANDLE conn,
   TTS_STATS NWPTR ttsStats
)
{
   NWCCODE ccode;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   ccode = (NWCCODE) NWNCP23s213GetTrackingStats(&access,
               (pNWNCPTTSStats) ttsStats);

   if (ccode != 0)
      NWCMemSet(ttsStats, 0x00, sizeof(TTS_STATS));

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gettts.c,v 1.7 1994/09/26 17:46:24 rebekah Exp $
*/
