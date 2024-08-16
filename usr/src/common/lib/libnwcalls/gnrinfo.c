/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gnrinfo.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetNetworkRouterInfo********************************************
SYNTAX:  NWCCODE N_API NWGetNetworkRouterInfo
         (
            NWCONN_HANDLE  conn,
            nuint32        networkNum,
            NWFSE_NETWORK_ROUTER_INFO NWPTR  fseNetworkRouterInfo
         )

REMARKS:

ARGS: >  conn
      >  networkNum
      <  fseNetworkRouterInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 51  Get Network Router Information

CHANGES: 23 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetNetworkRouterInfo
(
   NWCONN_HANDLE              conn,
   nuint32                    networkNum,
   NWFSE_NETWORK_ROUTER_INFO  NWPTR  fseNetworkRouterInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE) NWNCP123s51GetRouterInfo(&access, networkNum,
      (pNWNCPFSEVConsoleInfo)
      &fseNetworkRouterInfo->serverTimeAndVConsoleInfo,
      &fseNetworkRouterInfo->reserved,
      &fseNetworkRouterInfo->NetIDNumber,
      &fseNetworkRouterInfo->HopsToNet,
      &fseNetworkRouterInfo->NetStatus,
      &fseNetworkRouterInfo->TimeToNet));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gnrinfo.c,v 1.7 1994/09/26 17:46:40 rebekah Exp $
*/

