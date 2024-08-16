/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gnninfo.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetKnownNetworksInfo********************************************
SYNTAX:  NWCCODE N_API NWGetKnownNetworksInfo
         (
            NWCONN_HANDLE  conn,
            nuint32        startNum,
            NWFSE_KNOWN_NETWORKS_INFO NWPTR  fseKnownNetworksInfo
         )

REMARKS:

ARGS: >  conn
      >  startNum
      <  fseKnownNetworksInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 53  Get Known Networks Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
          Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetKnownNetworksInfo
(
   NWCONN_HANDLE             conn,
   nuint32                   startNum,
   NWFSE_KNOWN_NETWORKS_INFO NWPTR  fseKnownNetworksInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE) NWNCP123s53GetKnownNetworks(&access, startNum,
      (pNWNCPFSEVConsoleInfo)
      &fseKnownNetworksInfo->serverTimeAndVConsoleInfo,
      &fseKnownNetworksInfo->reserved,
      &fseKnownNetworksInfo->numberOfEntries,
      (pNWNCPFSENetList) fseKnownNetworksInfo->knownNetInfo));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gnninfo.c,v 1.7 1994/09/26 17:46:39 rebekah Exp $
*/
