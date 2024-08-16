/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:ttsbegin.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncptts.h"

#include "nwcaldef.h"
#include "nwtts.h"
#include "nwmisc.h"

/*manpage*NWTTSBeginTransaction*********************************************
SYNTAX:  NWCCODE N_API NWTTSBeginTransaction
         (
            NWCONN_HANDLE conn
         )

REMARKS:

ARGS:

INCLUDE: ncptts.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     34 03  TTS Abort Transaction
         34 02  TTS End Transaction

NCP:     34 01  TTS Begin Transaction

CHANGES: 13 Aug 1993 - NWNCP Enabled - srydalch
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWTTSBeginTransaction
(
   NWCONN_HANDLE conn
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP34s1TTSBeginTrans(&access));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/ttsbegin.c,v 1.7 1994/09/26 17:50:20 rebekah Exp $
*/
