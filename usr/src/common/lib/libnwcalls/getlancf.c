/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getlancf.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWGetFSLANDriverConfigInfo****************************************
SYNTAX:  NWCCODE N_API NWGetFSLANDriverConfigInfo
         (
            NWCONN_HANDLE  conn,
            nuint8         lanBoardNum,
            NWLAN_CONFIG N_FAR * lanConfig
         );

REMARKS:

ARGS: >  conn
      >  lanBoardNum
      <  lanConfig

INCLUDE: nwserver.h

RETURN:

SERVER:  2.2

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 227  Get LAN Driver Configuration Information

CHANGES: 10 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetFSLANDriverConfigInfo
(
   NWCONN_HANDLE  conn,
   nuint8         lanBoardNum,
   NWLAN_CONFIG   N_FAR * lanConfig
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   NWCMemSet(lanConfig, 0x00, (nuint) sizeof(*lanConfig));

   return ((NWCCODE) NWNCP23s227GetLANDrvConfigInfo(&access, lanBoardNum,
               (pNWNCPLANConfig) lanConfig));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getlancf.c,v 1.7 1994/09/26 17:46:10 rebekah Exp $
*/
