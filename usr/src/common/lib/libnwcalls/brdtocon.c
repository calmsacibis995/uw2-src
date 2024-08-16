/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:brdtocon.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpmsg.h"

#include "nwcaldef.h"
#include "nwmsg.h"
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwlocale.h"

/*manpage*NWBroadcastToConsole**********************************************
SYNTAX:  NWCCODE N_API NWBroadcastToConsole
         (
            NWCONN_HANDLE  conn,
            pnstr8         message
         );

REMARKS:

ARGS:

INCLUDE: nwmsg.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     21 09  Broadcast To Console

CHANGES: 10 Aug 1993 - NWNCP Enabled - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWBroadcastToConsole
(
  NWCONN_HANDLE   conn,
  pnstr8          message
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP21s9BroadcastToConsole(&access,
                            (nuint8) NWLTruncateString(message, 59),
                            (pnstr8) message));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/brdtocon.c,v 1.7 1994/09/26 17:44:14 rebekah Exp $
*/
