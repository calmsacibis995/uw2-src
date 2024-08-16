/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtcpuinf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetCPUInfo******************************************************
SYNTAX:  NWCCODE N_API NWGetCPUInfo
         (
            NWCONN_HANDLE conn,
            nuint32 CPUNum,
            pnstr8  CPUName,
            pnstr8  numCoprocessor,
            pnstr8  bus,
            NWFSE_CPU_INFO NWPTR fseCPUInfo
         )

REMARKS:

ARGS: >  conn
      >  CPUNum
      <  CPU
      <  numCoprocessor
      <  bus
      <  fseCPUInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 08  CPU Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetCPUInfo
(
   NWCONN_HANDLE  conn,
   nuint32        CPUNum,
   pnstr8         CPUName,
   pnstr8         numCoprocessor,
   pnstr8         bus,
   NWFSE_CPU_INFO NWPTR fseCPUInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s8GetCPUInfo(&access, CPUNum,
         (pNWNCPFSEVConsoleInfo) &fseCPUInfo->serverTimeAndVConsoleInfo,
         &fseCPUInfo->reserved, &fseCPUInfo->numOfCPUs,
         (pNWNCPFSECPUInfo) &fseCPUInfo->CPUInfo,
         CPUName, numCoprocessor, bus));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtcpuinf.c,v 1.7 1994/09/26 17:46:57 rebekah Exp $
*/

