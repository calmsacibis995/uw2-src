/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gnrsinfo.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetNetworkRoutersInfo*******************************************
SYNTAX:  NWCCODE N_API NWGetNetworkRoutersInfo
         (
            NWCONN_HANDLE  conn,
            nuint32        networkNum,
            nuint32        startNum,
            NWFSE_NETWORK_ROUTERS_INFO NWPTR  fseNetworkRoutersInfo
         )

REMARKS:

ARGS: >  conn
      >  networkNum
      >  startNum
      <  fseNetworkRoutersInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 52  Get Network Routers Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetNetworkRoutersInfo
(
   NWCONN_HANDLE        conn,
   nuint32              networkNum,
   nuint32              startNum,
   NWFSE_NETWORK_ROUTERS_INFO NWPTR  fseNetworkRoutersInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE) NWNCP123s52GetRoutersInfo(&access, networkNum,
            startNum, (pNWNCPFSEVConsoleInfo)
            &fseNetworkRoutersInfo->serverTimeAndVConsoleInfo,
            &fseNetworkRoutersInfo->reserved,
            &fseNetworkRoutersInfo->NumberOfEntries,
            (pNWNCPFSERouterList) fseNetworkRoutersInfo->routersInfo));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gnrsinfo.c,v 1.7 1994/09/26 17:46:42 rebekah Exp $
*/

