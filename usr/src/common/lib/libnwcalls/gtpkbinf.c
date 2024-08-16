/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtpkbinf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetPacketBurstInfo**********************************************
SYNTAX:  NWCCODE N_API NWGetPacketBurstInfo
         (
            NWCONN_HANDLE conn,
            NWFSE_PACKET_BURST_INFO NWPTR fsePacketBurstInfo
         )

REMARKS:

ARGS: >  conn
      <  fsePacketBurstInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 05  Packet Burst Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetPacketBurstInfo
(
   NWCONN_HANDLE           conn,
   NWFSE_PACKET_BURST_INFO NWPTR fsePacketBurstInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s5GetPacketBurstInfo(&access,
      (pNWNCPFSEVConsoleInfo) &fsePacketBurstInfo->serverTimeAndVConsoleInfo,
      &fsePacketBurstInfo->reserved,
      (pNWNCPFSEBurstInfo) &fsePacketBurstInfo->packetBurstInfo));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtpkbinf.c,v 1.7 1994/09/26 17:47:23 rebekah Exp $
*/

