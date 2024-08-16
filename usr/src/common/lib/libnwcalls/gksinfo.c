/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gksinfo.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetKnownServersInfo*********************************************
SYNTAX:  NWCCODE N_API NWGetKnownServersInfo
         (
            NWCONN_HANDLE  conn,
            nuint32        startNum,
            nuint32        serverType,
            NWFSE_KNOWN_SERVER_INFO NWPTR  fseKnownServerInfo
         )

REMARKS:

ARGS: >  conn
      >  startNum
      >  serverType
      <  OWN_SERVER_INFO

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 56  Get Known Servers Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetKnownServersInfo
(
   NWCONN_HANDLE        conn,
   nuint32              startNum,
   nuint32              serverType,
   NWFSE_KNOWN_SERVER_INFO NWPTR  fseKnownServerInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE) NWNCP123s56GetKnownServers(&access, startNum,
         serverType, (pNWNCPFSEVConsoleInfo)
         &fseKnownServerInfo->serverTimeAndVConsoleInfo,
         &fseKnownServerInfo->reserved,
         &fseKnownServerInfo->numberOfEntries,
         fseKnownServerInfo->data));

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gksinfo.c,v 1.7 1994/09/26 17:46:33 rebekah Exp $
*/

