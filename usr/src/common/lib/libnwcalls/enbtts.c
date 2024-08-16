/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:enbtts.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncptts.h"
#include "nwtts.h"
#include "nwcaldef.h"

/*manpage*NWEnableTTS*******************************************************
SYNTAX:  NWCCODE N_API NWEnableTTS
         (
            NWCONN_HANDLE conn
         );

REMARKS:

ARGS: >  conn

INCLUDE: nwtts.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 208  Enable Transaction Tracking

CHANGES: 19 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
*****************************************************************************/
NWCCODE N_API NWEnableTTS
(
   NWCONN_HANDLE conn
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ( (NWCCODE) NWNCP23s208EnableTracking(&access) );
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/enbtts.c,v 1.7 1994/09/26 17:45:21 rebekah Exp $
*/
