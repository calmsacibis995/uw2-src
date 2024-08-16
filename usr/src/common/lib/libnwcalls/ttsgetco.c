/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:ttsgetco.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"

#include "nwcaldef.h"
#include "nwtts.h"

#include "ncptts.h"

/*manpage*NWTTSGetConnectionThresholds**************************************
SYNTAX:  NWCCODE N_API NWTTSGetConnectionThresholds
         (
            NWCONN_HANDLE conn,
            pnuint8 pbuLogLockThreshold,
            pnuint8 pbuPhyLockThreshold
         )
REMARKS:

ARGS:   <> conn
        <  pbuLogLockThreshold
        <  pbuPhyLockThreshold

INCLUDE: nwtts.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     34 07  TTS Get Workstation Thresholds

CHANGES: 13 Aug 1993 - NWCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWTTSGetConnectionThresholds(
   NWCONN_HANDLE conn,
   pnuint8 pbuLogLockThreshold,
   pnuint8 pbuPhyLockThreshold)
{
   NWCDeclareAccess(access)

   NWCSetConn(access, conn);

   return (NWCCODE) NWNCP34s7TTSGetThresholds(&access, pbuLogLockThreshold,
               pbuPhyLockThreshold);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/ttsgetco.c,v 1.7 1994/09/26 17:50:22 rebekah Exp $
*/
