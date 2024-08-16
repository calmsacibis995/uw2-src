/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:ttsgetpr.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"

#include "nwcaldef.h"
#include "nwtts.h"

#include "ncptts.h"

/*manpage*NWTTSGetProcessThresholds*****************************************
SYNTAX:  NWCCODE N_API NWTTSGetProcessThresholds
         (
            NWCONN_HANDLE conn,
            pnuint8 pbuLogLockLevel,
            pnuint8 pbuPhyLockLevel
         );

REMARKS:
         Returns the number of logical and physical record locks
         allowed before an implicit transaction begins. The default
         threshold for both types is 0. If a value of 0xFF is returned this
         means that implicit transcations for that lock type are not in
         effect.

ARGS:  > conn
      <  pbuLogLockLevel (optional)
      <  pbuPhyLockLevel (optional)

INCLUDE: nwtts.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     34 05  TTS Get Application Thresholds

CHANGES: 13 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWTTSGetProcessThresholds
(
  NWCONN_HANDLE conn,
  pnuint8 pbuLogLockLevel,
  pnuint8 pbuPhyLockLevel
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP34s5TTSGetAppThresholds(&access, pbuLogLockLevel,
               pbuPhyLockLevel));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/ttsgetpr.c,v 1.7 1994/09/26 17:50:24 rebekah Exp $
*/
