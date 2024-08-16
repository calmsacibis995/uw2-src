/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getisinf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetIPXSPXInfo***************************************************
SYNTAX:  NWCCODE N_API NWGetIPXSPXInfo
         (
            NWCONN_HANDLE conn,
            NWFSE_IPXSPX_INFO NWPTR fseIPXSPXInfo
         )

REMARKS:

ARGS: >  conn
      <  fseIPXSPXInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 06  IPX SPX Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetIPXSPXInfo
(
   NWCONN_HANDLE     conn,
   NWFSE_IPXSPX_INFO NWPTR fseIPXSPXInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s6GetIPXSPXInfo(&access,
         (pNWNCPFSEVConsoleInfo) &fseIPXSPXInfo->serverTimeAndVConsoleInfo,
         &fseIPXSPXInfo->reserved,
         (pNWNCPFSEIPXInfo) &fseIPXSPXInfo->IPXInfo,
         (pNWNCPFSESPXInfo) &fseIPXSPXInfo->SPXInfo));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getisinf.c,v 1.7 1994/09/26 17:46:07 rebekah Exp $
*/

