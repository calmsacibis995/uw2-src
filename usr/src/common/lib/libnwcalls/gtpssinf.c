/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtpssinf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetProtocolStackStatsInfo***************************************
SYNTAX:  NWCCODE N_API NWGetProtocolStackStatsInfo
         (
            NWCONN_HANDLE conn,
            nuint32 luStackNum,
            NWFSE_PROTOCOL_STK_STATS_INFO NWPTR fseProtocolStkStatsInfo
         )

REMARKS:

ARGS:

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 42  Get Protocol Stack Statistics Information

CHANGES: 23 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetProtocolStackStatsInfo
(
   NWCONN_HANDLE                 conn,
   nuint32                       luStackNum,
   NWFSE_PROTOCOL_STK_STATS_INFO NWPTR fseProtocolStkStatsInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return((NWCCODE) NWNCP123s42GetProtocolStats(&access, luStackNum,
      (pNWNCPFSEVConsoleInfo)
      &fseProtocolStkStatsInfo->serverTimeAndVConsoleInfo,
      &fseProtocolStkStatsInfo->reserved,
      &fseProtocolStkStatsInfo->statMajorVersionNum,
      &fseProtocolStkStatsInfo->statMinorVersionNum,
      &fseProtocolStkStatsInfo->commonCounters,
      &fseProtocolStkStatsInfo->validCountersMask,
      &fseProtocolStkStatsInfo->totalTxPackets,
      &fseProtocolStkStatsInfo->totalRxPackets,
      &fseProtocolStkStatsInfo->ignoredRxPackets,
      &fseProtocolStkStatsInfo->numCustomCounters));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtpssinf.c,v 1.7 1994/09/26 17:47:25 rebekah Exp $
*/

