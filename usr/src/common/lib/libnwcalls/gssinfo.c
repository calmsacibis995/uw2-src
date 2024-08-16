/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gssinfo.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfse.h"

#include "nwcaldef.h"
#include "nwfse.h"

/*manpage*NWGetServerSourcesInfo********************************************
SYNTAX:  NWCCODE N_API NWGetServerSourcesInfo
         (
            NWCONN_HANDLE  conn,
            nuint32        startNum,
            nuint32        serverType,
            pnstr8         serverName,
            NWFSE_SERVER_SRC_INFO NWPTR  fseServerSrcInfo
         )

REMARKS:

ARGS: > conn
      > startNum
      > serverType
      > serverName,
      < fseServerSrcInfo

INCLUDE: nwfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     123 55  Get Server Sources Information

CHANGES: 24 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetServerSourcesInfo
(
   NWCONN_HANDLE     conn,
   nuint32           startNum,
   nuint32           serverType,
   pnstr8            serverName,
   NWFSE_SERVER_SRC_INFO NWPTR  fseServerSrcInfo
)
{
   nuint8 serverNameLen;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   serverNameLen = (nuint8) NWCStrLen(serverName);

   return((NWCCODE) NWNCP123s55GetServerSourcesInfo(&access, startNum,
            serverType, serverNameLen, serverName,
            (pNWNCPFSEVConsoleInfo)
            &fseServerSrcInfo->serverTimeAndVConsoleInfo,
            (pnuint16) &fseServerSrcInfo->reserved,
            (pnuint32) &fseServerSrcInfo->numberOfEntries,
            (pNWNCPFSESourceList) fseServerSrcInfo->serversSrcInfo));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gssinfo.c,v 1.7 1994/09/26 17:46:51 rebekah Exp $
*/

