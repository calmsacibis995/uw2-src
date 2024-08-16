/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gpscoinf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetProtocolStackConfigInfo**************************************
SYNTAX:  NWCCODE N_API NWGetProtocolStackConfigInfo
         (
            NWCONN_HANDLE  conn,
            nuint32        stackNum,
            pnstr8         stackFullName,
            NWFSE_PROTOCOL_STK_CONFIG_INFO NWPTR fseProtocolStkConfigInfo
         )

REMARKS:

ARGS:  > conn
       > stackNum
       < stackFullName
       < fseProtocolStkConfigInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 41  Get Protocol Stack Configuration Information

CHANGES: 23 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetProtocolStackConfigInfo
(
   NWCONN_HANDLE     conn,
   nuint32           stackNum,
   pnstr8            stackFullName,
   NWFSE_PROTOCOL_STK_CONFIG_INFO NWPTR fseProtocolStkConfigInfo
)
{
   nuint8 stackFullNameLen;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s41GetProtocolConfig(&access, stackNum,
      (pNWNCPFSEVConsoleInfo) &fseProtocolStkConfigInfo->serverTimeAndVConsoleInfo,
      &fseProtocolStkConfigInfo->reserved,
      &fseProtocolStkConfigInfo->configMajorVersionNum,
      &fseProtocolStkConfigInfo->configMinorVersionNum,
      &fseProtocolStkConfigInfo->stackMajorVersionNum,
      &fseProtocolStkConfigInfo->stackMinorVersionNum,
      (pnstr8)&fseProtocolStkConfigInfo->stackShortName,
      &stackFullNameLen, stackFullName));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gpscoinf.c,v 1.7 1994/09/26 17:46:44 rebekah Exp $
*/

