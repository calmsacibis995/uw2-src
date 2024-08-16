/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:glancuci.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetLANCustomCountersInfo****************************************
SYNTAX:  NWCCODE N_API NWGetLANCustomCountersInfo
         (
            NWCONN_HANDLE  conn,
            nuint32        boardNum,
            nuint32        startingNum,
            NWFSE_LAN_CUSTOM_INFO NWPTR fseLANCustomInfo
         )

REMARKS:

ARGS: >  conn
      >  boardNum
      >  startingNum
      <  fseLANCustomInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 23  LAN Custom Counters Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetLANCustomCountersInfo
(
   NWCONN_HANDLE     conn,
   nuint32           boardNum,
   nuint32           startingNum,
   NWFSE_LAN_CUSTOM_INFO NWPTR fseLANCustomInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP123s23GetLANCustomCounters(&access, boardNum,
               startingNum, (pNWNCPFSEVConsoleInfo)
               &fseLANCustomInfo->serverTimeAndVConsoleInfo,
               &fseLANCustomInfo->reserved,
               &fseLANCustomInfo->numCustomVar,
               fseLANCustomInfo->customInfo));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/glancuci.c,v 1.7 1994/09/26 17:46:35 rebekah Exp $
*/

