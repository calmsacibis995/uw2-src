/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:rsvrsta2.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
#include "ncpqms.h"

#include "nwcaldef.h"
#include "nwqms.h"
#include "nwserver.h"

/*manpage*NWReadQueueServerCurrentStatus2***********************************
SYNTAX:  NWCCODE N_API NWReadQueueServerCurrentStatus2
         (
            NWCONN_HANDLE  conn,
            nuint32        queueID,
            nuint32        serverID,
            nuint32        serverConnection,
            void NWPTR     statusRecord
         )

REMARKS: This call returns the current 64 byte status record for a queue
         server. This record id not used or interpreted by the queue
         manager.

ARGS:  > conn
       > queueID
       > serverID
       > serverConnection
      <  statusRecord

INCLUDE: nwqms.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 134  Read Queue Server Current Status
         23 118  Read Queue Server Current Status

CHANGES: 15 Sep 1993 - NWNCP Enabled - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWReadQueueServerCurrentStatus2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        serverID,
   nuint32        serverConnection,
   void NWPTR     statusRecord
)
{
   nuint16 serverVer;
   NWCCODE ccode;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   queueID   = NSwap32(queueID);
   serverID  = NSwap32(serverID);

   if(serverVer >= 3110 || serverVer < 2000)
   {
      return ((NWCCODE)NWNCP23s134ReadQServerCurrStat(&access, queueID,
         serverID, serverConnection, statusRecord));
   }
   else
   {
      return ((NWCCODE)NWNCP23s118ReadQServerCurrStat(&access, queueID,
         serverID, (nuint8) serverConnection, statusRecord));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/rsvrsta2.c,v 1.7 1994/09/26 17:49:16 rebekah Exp $
*/
