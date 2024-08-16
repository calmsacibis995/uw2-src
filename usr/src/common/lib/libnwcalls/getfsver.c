/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getfsver.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWGetFileServerVersionInfo****************************************
SYNTAX:  NWCCODE N_API NWGetFileServerVersionInfo
         (
            NWCONN_HANDLE  conn,
            VERSION_INFO N_FAR * versBuffer
         );

REMARKS:

ARGS: >  conn
      <  versBuffer

INCLUDE: nwserver.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 17  Get File Server Information

CHANGES: 10 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetFileServerVersionInfo
(
   NWCONN_HANDLE  conn,
   VERSION_INFO   N_FAR * versBuffer
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   NWCMemSet(versBuffer, 0x00, (nuint) sizeof(*versBuffer));

   return ((NWCCODE) NWNCP23s17GetServerInfo(&access,
               (pNWNCPVersionInfo) versBuffer));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getfsver.c,v 1.7 1994/09/26 17:46:05 rebekah Exp $
*/
