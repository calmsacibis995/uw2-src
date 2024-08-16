/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtlancin.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetLANConfigInfo************************************************
SYNTAX:  NWCCODE N_API NWGetLANConfigInfo
         (
            NWCONN_HANDLE  conn,
            nuint32        boardNum,
            NWFSE_LAN_CONFIG_INFO NWPTR fseLANConfigInfo
         )

REMARKS:

ARGS:  > conn
       > boardNum
       < fseLANConfigInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 21  LAN Configuration Information

CHANGES: 27 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetLANConfigInfo
(
   NWCONN_HANDLE  conn,
   nuint32        boardNum,
   NWFSE_LAN_CONFIG_INFO NWPTR fseLANConfigInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE)NWNCP123s21GetLANConfigInfo(&access, boardNum,
            (pNWNCPFSEVConsoleInfo)
            &fseLANConfigInfo->serverTimeAndVConsoleInfo,
            &fseLANConfigInfo->reserved,
            (pNWNCPFSELANInfo) &fseLANConfigInfo->LANConfigInfo));

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtlancin.c,v 1.7 1994/09/26 17:47:04 rebekah Exp $
*/

