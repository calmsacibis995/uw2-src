/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:rtsvrrit.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
/*#include "nwclient.h"*/
#include "ncpqms.h"

#include "nwcaldef.h"
/*#include "nwintern.h"*/
/*#include "nwundoc.h"*/
#include "nwqms.h"
/*#include "nwmisc.h"*/

/*manpage*NWRestoreQueueServerRights****************************************
SYNTAX:  NWCCODE N_API NWRestoreQueueServerRights
         (
            NWCONN_HANDLE  conn
         )

REMARKS: Restores a queue server's identity after is has assumed
         its client's identity.

ARGS:

INCLUDE: nwqms.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 117  Restore Queue Server Rights

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWRestoreQueueServerRights
(
   NWCONN_HANDLE  conn
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE)NWNCP23s117RestoreQServerRights(&access));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/rtsvrrit.c,v 1.7 1994/09/26 17:49:17 rebekah Exp $
*/
