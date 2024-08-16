/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getconus.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpserve.h"

#include "nwconnec.h"
#include "nwserver.h"

/*manpage*NWGetConnectionUsageStats*****************************************
SYNTAX:  NWCCODE N_API NWGetConnectionUsageStats
         (
            NWCONN_HANDLE  conn,
            NWCONN_NUM     connNumber,
            CONN_USE NWPTR statusBuffer
         )

REMARKS:

ARGS: >  conn
      >  connNumber
      <  statusBuffer

INCLUDE: nwconnec.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 229  Get Connection Usage Statistics

CHANGES: 20 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetConnectionUsageStats
(
   NWCONN_HANDLE  conn,
   NWCONN_NUM     connNumber,
   CONN_USE NWPTR statusBuffer
)
{
   NWCCODE ccode;
   nuint16 serverVer;

   NWCDeclareAccess(access);
   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
    return (ccode);

   if(serverVer >= 3000) 
      return (0x89fd);


   if((ccode = (NWCCODE) NWNCP23s229GetConnUsageStats(&access, connNumber,
      &statusBuffer->systemElapsedTime, (pnuint8)&statusBuffer->bytesRead,
      (pnuint8)&statusBuffer->bytesWritten, &statusBuffer->totalRequestPackets)) != 0)
   {
      NWCMemSet(statusBuffer, 0, sizeof(CONN_USE));
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getconus.c,v 1.7 1994/09/26 17:45:48 rebekah Exp $
*/
