/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:ismgr.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpbind.h"

#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWIsManager********************************************************
SYNTAX:  NWCCODE N_API NWIsManager
         (
            NWCONN_HANDLE conn
         );

REMARKS: Checks if calling station is a manager. A station is a manager if
         it is a supervisor or if it appears in the MANAGERS property of
         the supervisor object.

ARGS: >  conn

INCLUDE: nwserver.h

RETURN:  0x0000   Calling station is a manager
         0x89FF   Calling station is not a manager

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 73  Is Calling Station a Manager

CHANGES: 27 May 1993 - written - jwoodbur
         13 Sep 1993 - NWNCP Enabled - jsumsion
****************************************************************************/
NWCCODE N_API NWIsManager
(
   NWCONN_HANDLE conn
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return (NWCCODE) NWNCP23s73IsObjManager(&access);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/ismgr.c,v 1.7 1994/09/26 17:47:41 rebekah Exp $
*/
