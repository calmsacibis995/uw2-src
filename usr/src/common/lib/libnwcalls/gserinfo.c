/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gserinfo.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetServerInfo***************************************************
SYNTAX:  NWCCODE N_API NWGetServerInfo
         (
            NWCONN_HANDLE  conn,
            nuint32        serverType,
            pnstr8         serverName,
            NWFSE_SERVER_INFO NWPTR  fseServerInfo
         )

REMARKS:

ARGS: > conn
      > serverType
      > serverName
      < fseServerInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 54  Get Server Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetServerInfo
(
   NWCONN_HANDLE     conn,
   nuint32           serverType,
   pnstr8            serverName,
   NWFSE_SERVER_INFO NWPTR  fseServerInfo
)
{
   nuint8 buServerNameLen;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   buServerNameLen = (nuint8) NWCStrLen(serverName);

   return((NWCCODE) NWNCP123s54GetServerInfo(&access, serverType,
         buServerNameLen, serverName,
         (pNWNCPFSEVConsoleInfo) &fseServerInfo->serverTimeAndVConsoleInfo,
         &fseServerInfo->reserved, (pnuint8) &fseServerInfo->serverAddress,
         &fseServerInfo->hopsToServer));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gserinfo.c,v 1.7 1994/09/26 17:46:47 rebekah Exp $
*/

