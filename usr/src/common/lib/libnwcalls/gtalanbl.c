/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtalanbl.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetActiveLANBoardList*******************************************
SYNTAX:  NWCCODE N_API NWGetActiveLANBoardList
         (
            NWCONN_HANDLE  conn,
            nuint32        startNum,
            NWFSE_ACTIVE_LAN_BOARD_LIST NWPTR fseActiveLANBoardList
         )

REMARKS:

ARGS:  > conn
       > startNum
      <  fseActiveLANBoardList

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 20  Active LAN Board List

CHANGES: 27 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetActiveLANBoardList
(
   NWCONN_HANDLE  conn,
   nuint32        startNum,
   NWFSE_ACTIVE_LAN_BOARD_LIST NWPTR fseActiveLANBoardList
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE)NWNCP123s20GetActLANBoardLst(&access, startNum,
               (pNWNCPFSEVConsoleInfo)
               &fseActiveLANBoardList->serverTimeAndVConsoleInfo,
               &fseActiveLANBoardList->reserved,
               &fseActiveLANBoardList->MaxNumOfLANs,
               &fseActiveLANBoardList->LANLoadedCount,
               fseActiveLANBoardList->boardNums));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtalanbl.c,v 1.7 1994/09/26 17:46:55 rebekah Exp $
*/
