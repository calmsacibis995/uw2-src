/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:glancoci.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetLANCommonCountersInfo****************************************
SYNTAX:  NWCCODE N_API NWGetLANCommonCountersInfo
         (
            NWCONN_HANDLE  conn,
            nuint32        boardNum,
            nuint32        blockNum,
            NWFSE_LAN_COMMON_COUNTERS_INFO NWPTR fseLANCommonCountersInfo
         )

REMARKS:

ARGS: >  conn
      >  boardNum
      >  blockNum
      <  fseLANCommonCountersInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 22  LAN Common Counters Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetLANCommonCountersInfo
(
   NWCONN_HANDLE     conn,
   nuint32           boardNum,
   nuint32           blockNum,
   NWFSE_LAN_COMMON_COUNTERS_INFO NWPTR fseLANCommonCountersInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s22GetLANCommonCounters(&access, boardNum,
            blockNum, (pNWNCPFSEVConsoleInfo)
            &fseLANCommonCountersInfo->serverTimeAndVConsoleInfo,
            &fseLANCommonCountersInfo->statisticsMajorVersion,
            &fseLANCommonCountersInfo->statisticsMinorVersion,
            &fseLANCommonCountersInfo->numberOfGenericCounters,
            &fseLANCommonCountersInfo->numberOfCounterBlocks,
            &fseLANCommonCountersInfo->customVariableCount,
            &fseLANCommonCountersInfo->NextCounterBlock,
            (pNWNCPFSECommonCounters)
            &fseLANCommonCountersInfo->LANCommonInfo));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/glancoci.c,v 1.7 1994/09/26 17:46:34 rebekah Exp $
*/

