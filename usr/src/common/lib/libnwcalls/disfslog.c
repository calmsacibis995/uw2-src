/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:disfslog.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWDisableFileServerLogin******************************************
SYNTAX:  NWCCODE N_API NWDisableFileServerLogin
         (
            NWCONN_HANDLE conn
         );

REMARKS: This call allows an operator to instruct the file server to refuse
         new login requests.  This call is usually made during some crucial
         time--before taking the server down, for instance.

ARGS: >  conn

INCLUDE: nwserver.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23  203  Disable File Server Login

CHANGES: 17 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWDisableFileServerLogin
(
  NWCONN_HANDLE conn
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP23s203DisableServerLogin(&access));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/disfslog.c,v 1.7 1994/09/26 17:45:17 rebekah Exp $
*/
