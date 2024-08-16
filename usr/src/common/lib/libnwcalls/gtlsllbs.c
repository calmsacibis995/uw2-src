/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtlsllbs.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"
#include "nwfse.h"

/*manpage*NWGetLSLLogicalBoardStats*****************************************
SYNTAX:  NWCCODE N_API NWGetLSLLogicalBoardStats
         (
            NWCONN_HANDLE  conn,
            nuint32        LANBoardNum,
            NWFSE_LSL_LOGICAL_BOARD_STATS NWPTR fseLSLLogicalBoardStats
         )

REMARKS:

ARGS: >  conn
      >  LANBoardNum
      <  fseLSLLogicalBoardStats

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 26  LSL Logical Board Statistics

CHANGES: 24 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetLSLLogicalBoardStats
(
   NWCONN_HANDLE  conn,
   nuint32        LANBoardNum,
   NWFSE_LSL_LOGICAL_BOARD_STATS NWPTR fseLSLLogicalBoardStats
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s26GetLSLBoardStats(&access, LANBoardNum,
            (pNWNCPFSEVConsoleInfo)
            &fseLSLLogicalBoardStats->serverTimeAndVConsoleInfo,
            &fseLSLLogicalBoardStats->reserved0,
            &fseLSLLogicalBoardStats->LogTtlTxPackets,
            &fseLSLLogicalBoardStats->LogTtlRxPackets,
            &fseLSLLogicalBoardStats->LogUnclaimedPackets,
            &fseLSLLogicalBoardStats->reserved1));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtlsllbs.c,v 1.7 1994/09/26 17:47:06 rebekah Exp $
*/

