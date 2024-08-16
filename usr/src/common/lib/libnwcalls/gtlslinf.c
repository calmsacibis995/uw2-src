/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtlslinf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"
#include "nwfse.h"

/*manpage*NWGetLSLInfo******************************************************
SYNTAX:  NWCCODE N_API NWGetLSLInfo
         (
            NWCONN_HANDLE conn,
            NWFSE_LSL_INFO NWPTR fseLSLInfo
         )

REMARKS:

ARGS: >  conn
      <  fseLSLInfo


INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 25  LSL Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetLSLInfo
(
   NWCONN_HANDLE conn,
   NWFSE_LSL_INFO NWPTR fseLSLInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s25GetLSLInfo(&access,
            (pNWNCPFSEVConsoleInfo) &fseLSLInfo->serverTimeAndVConsoleInfo,
            &fseLSLInfo->reserved,
            (pNWNCPFSELSLInfo) &fseLSLInfo->LSLInfo));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtlslinf.c,v 1.7 1994/09/26 17:47:05 rebekah Exp $
*/
