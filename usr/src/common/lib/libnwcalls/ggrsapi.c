/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:ggrsapi.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetGeneralRouterAndSAPInfo**************************************
SYNTAX:  NWCCODE N_API NWGetGeneralRouterAndSAPInfo
         (
            NWCONN_HANDLE conn,
            NWFSE_GENERAL_ROUTER_SAP_INFO NWPTR  fseGeneralRouterSAPInfo
         )

REMARKS:

ARGS: >  conn
      <  fseGeneralRouterSAPInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 50  Get General Router And SAP Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetGeneralRouterAndSAPInfo
(
   NWCONN_HANDLE conn,
   NWFSE_GENERAL_ROUTER_SAP_INFO NWPTR  fseGeneralRouterSAPInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s50GetRouterAndSapInfo(&access,
      (pNWNCPFSEVConsoleInfo)
      &fseGeneralRouterSAPInfo->serverTimeAndVConsoleInfo,
      &fseGeneralRouterSAPInfo->reserved,
      &fseGeneralRouterSAPInfo->internalRIPSocket,
      &fseGeneralRouterSAPInfo->internalRouterDownFlag,
      &fseGeneralRouterSAPInfo->trackOnFlag,
      &fseGeneralRouterSAPInfo->externalRouterActiveFlag,
      &fseGeneralRouterSAPInfo->internalSAPSocketNumber,
      &fseGeneralRouterSAPInfo->replyToNearestServerFlag));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/ggrsapi.c,v 1.7 1994/09/26 17:46:29 rebekah Exp $
*/

