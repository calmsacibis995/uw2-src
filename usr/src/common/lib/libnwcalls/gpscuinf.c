/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gpscuinf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwfse.h"

/*manpage*NWGetProtocolStackCustomInfo**************************************
SYNTAX:  NWCCODE N_API NWGetProtocolStackCustomInfo
         (
            NWCONN_HANDLE  conn,
            nuint32        stackNum,
            nuint32        customStartNum,
            NWFSE_PROTOCOL_CUSTOM_INFO NWPTR fseProtocolStackCustomInfo
         )

REMARKS:

ARGS:

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 43  Get Protocol Stack Custom Information

CHANGES: 23 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetProtocolStackCustomInfo
(
   NWCONN_HANDLE              conn,
   nuint32                    stackNum,
   nuint32                    customStartNum,
   NWFSE_PROTOCOL_CUSTOM_INFO NWPTR fseProtocolStackCustomInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE) NWNCP123s43GetProtCustInfo(&access, stackNum,
         customStartNum, (pNWNCPFSEVConsoleInfo)
         &fseProtocolStackCustomInfo->serverTimeAndVConsoleInfo,
         &fseProtocolStackCustomInfo->reserved0,
         &fseProtocolStackCustomInfo->customCount,
         fseProtocolStackCustomInfo->customStruct));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gpscuinf.c,v 1.7 1994/09/26 17:46:45 rebekah Exp $
*/

