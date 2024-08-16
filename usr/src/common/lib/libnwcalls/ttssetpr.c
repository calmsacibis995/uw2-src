/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:ttssetpr.c	1.4"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncptts.h"
#include "nwcaldef.h"
#include "nwtts.h"

/*manpage*NWTTSSetProcessThresholds*****************************************
SYNTAX:  NWCCODE N_API NWTTSSetProcessThresholds
         (
            NWCONN_HANDLE conn,
            nuint8 logicalLockLevel,
            nuint8 physicalLockLevel
         )

REMARKS: Allows an application to specify the number of logical
         and physical record locks allowed before an implicit transaction
         begins. Thresholds set by this call are valid for the calling
         application only.

ARGS:    > conn
         > logicalLockLevel
         > physicalLockLevel

INCLUDE: nwtts.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     34 05  TTS Get Application Thresholds

NCP:     34 06  TTS Set Application Thresholds

CHANGES: 13 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWTTSSetProcessThresholds
(
   NWCONN_HANDLE conn,
   nuint8 logicalLockLevel,
   nuint8 physicalLockLevel
)
{
NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE) NWNCP34s6TTSSetAppThresholds(&access, logicalLockLevel,
            physicalLockLevel));
}
