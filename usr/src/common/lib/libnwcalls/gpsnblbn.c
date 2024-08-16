/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gpsnblbn.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetProtocolStkNumsByLANBrdNum***********************************
SYNTAX:  NWCCODE N_API NWGetProtocolStkNumsByLANBrdNum
         (
            NWCONN_HANDLE  conn,
            nuint32        LANBoardNum,
            NWFSE_PROTOCOL_ID_NUMS NWPTR fseProtocolStkIDNums
         )

REMARKS:

ARGS:  > conn
       > LANBoardNum
       < fseProtocolStkIDNums

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 45  Get Protocol Stack Numbers by LAN Board Number

CHANGES: 23 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetProtocolStkNumsByLANBrdNum
(
   NWCONN_HANDLE          conn,
   nuint32                LANBoardNum,
   NWFSE_PROTOCOL_ID_NUMS NWPTR fseProtocolStkIDNums
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE) NWNCP123s45GetProtNumByBoard(&access, LANBoardNum,
         (pNWNCPFSEVConsoleInfo)
            &fseProtocolStkIDNums->serverTimeAndVConsoleInfo,
            &fseProtocolStkIDNums->reserved,
            &fseProtocolStkIDNums->stackIDCount,
            fseProtocolStkIDNums->stackIDs));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gpsnblbn.c,v 1.7 1994/09/26 17:46:46 rebekah Exp $
*/

