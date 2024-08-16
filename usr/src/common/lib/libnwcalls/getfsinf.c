/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getfsinf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetFileServerInfo***********************************************
SYNTAX:  NWCCODE N_API NWGetFileServerInfo
         (
            NWCONN_HANDLE  conn,
            NWFSE_FILE_SERVER_INFO N_FAR * fseFileServerInfo
         )

REMARKS:

ARGS: >  conn
      <  fseFileServerInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 02  Get File Server Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetFileServerInfo
(
   NWCONN_HANDLE conn,
   NWFSE_FILE_SERVER_INFO N_FAR * fseFileServerInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s2GetServerInfo(&access,
         (pNWNCPFSEVConsoleInfo) &fseFileServerInfo->serverTimeAndVConsoleInfo,
         &fseFileServerInfo->reserved,
         &fseFileServerInfo->NCPStationsInUseCount,
         &fseFileServerInfo->NCPPeakStationsInUseCount,
         &fseFileServerInfo->numOfNCPRequests,
         &fseFileServerInfo->serverUtilization,
         (pNWNCPFSEServerInfo) &fseFileServerInfo->ServerInfo,
         (pNWNCPFSEServerCnts) &fseFileServerInfo->fileServerCounters));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getfsinf.c,v 1.7 1994/09/26 17:46:02 rebekah Exp $
*/
