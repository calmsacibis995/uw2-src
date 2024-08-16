/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:ttssetco.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncptts.h"
#include "nwtts.h"
#include "nwcaldef.h"

/*manpage*NWTTSSetConnectionThresholds**************************************
SYNTAX:  NWCCODE N_API NWTTSSetConnectionThresholds
         (
            NWCONN_HANDLE conn,
            nuint8 buLogLockLevel,
            nuint8 buPhyLockLevel
         );

REMARKS:
         Allows an application to specify the number of logical
         and physical record locks allowed before an implicit transaction
         begins. This value is in effect for all applications not just the
         one that calls this function. The default threshold is 0.

ARGS: >  conn
      >  buLogLockLevel
      >  buPhyLockLevel

INCLUDE: nwtts.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     34 08  TTS Set Workstation Thresholds

CHANGES: 13 Aug 1993 - NWNCP enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWTTSSetConnectionThresholds
(
   NWCONN_HANDLE conn,
   nuint8 buLogLockLevel,
   nuint8 buPhyLockLevel
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ( (NWCCODE) NWNCP34s8TTSSetThresholds(
                         &access, buLogLockLevel, buPhyLockLevel) );
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/ttssetco.c,v 1.7 1994/09/26 17:50:25 rebekah Exp $
*/
