/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getfslog.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWGetFileServerLoginStatus****************************************
SYNTAX:  NWCCODE N_API NWGetFileServerLoginStatus
         (
            NWCONN_HANDLE  conn,
            pnuint8        loginEnabledFlag
         );

REMARKS:

ARGS: >  conn
      <  loginEnabledFlag (optional)
         will be zero if clients can log in, otherwise it will be non-zero.

INCLUDE: nwserver.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23  205  Get File Server Login Status

CHANGES: 10 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetFileServerLoginStatus
(
   NWCONN_HANDLE  conn,
   pnuint8        loginEnabledFlag
)
{
   nuint8 buFoo;     /* it was like this in the old code */
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP23s205GetServerLoginStatus(&access,
               loginEnabledFlag ? loginEnabledFlag: &buFoo));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getfslog.c,v 1.7 1994/09/26 17:46:04 rebekah Exp $
*/
