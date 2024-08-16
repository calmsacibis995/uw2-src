/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtosvinf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"

#include "ncpfse.h"
#include "nwfse.h"

/*manpage*NWGetOSVersionInfo************************************************
SYNTAX:  NWCCODE N_API NWGetOSVersionInfo
         (
            NWCONN_HANDLE conn,
            NWFSE_OS_VERSION_INFO NWPTR fseOSVersionInfo
         )

REMARKS:

ARGS:

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 13  Get Operating System Version Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetOSVersionInfo
(
   NWCONN_HANDLE         conn,
   NWFSE_OS_VERSION_INFO NWPTR fseOSVersionInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE) NWNCP123s13GetOSVersionInfo(&access,
      (pNWNCPFSEVConsoleInfo) &fseOSVersionInfo->serverTimeAndVConsoleInfo,
      &fseOSVersionInfo->reserved,
      (pNWNCPFSEOSVerInfo) &fseOSVersionInfo->OSMajorVersion));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtosvinf.c,v 1.7 1994/09/26 17:47:22 rebekah Exp $
*/

