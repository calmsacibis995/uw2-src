/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:downfs.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWDownFileServer**************************************************
SYNTAX:  NWCCODE N_API NWDownFileServer
         (
            NWCONN_HANDLE  conn,
            nuint8         forceFlag
         );

REMARKS:

ARGS: >  conn
      >  forceFlag

INCLUDE: nwserver.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 211  Down File Server

CHANGES: 18 Aug 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
*****************************************************************************/
NWCCODE N_API NWDownFileServer
(
   NWCONN_HANDLE  conn,
   nuint8         forceFlag
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP23s211DownServer(&access, forceFlag));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/downfs.c,v 1.7 1994/09/26 17:45:19 rebekah Exp $
*/
