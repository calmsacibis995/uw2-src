/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:setqsta2.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpqms.h"

#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwqms.h"
#include "nwmisc.h"
#include "nwserver.h"
#include "nwerror.h"

/*manpage*NWSetQueueCurrentStatus2******************************************
SYNTAX:  NWCCODE N_API NWSetQueueCurrentStatus2
         (
            NWCONN_HANDLE  conn,
            nuint32        queueID,
            nuint32        queueStatus
         )

REMARKS: Must have operator privileges to make this call

ARGS:
       > queueID

       > queueStatus
         Bit 1 - Prevent jobs from being added to the queue

         Bit 2 - Prevent servers from attaching to the queue in order to
                 service it.

         Bit 3 - Prevent servers from servicing jobs in the queue

INCLUDE: nwqms.h

RETURN:  0x8999  Directory Full Error
         0x89d0  Queue Error
         0x89d1  No Queue
         0x89d2  No Queue Server
         0x89d3  No Queue Rights
         0x89d4  Queue Full
         0x89d5  No Queue Job
         0x89d6  No Job Right
         0x89d7  Queue Servicing
         0x89d8  Queue Not Active
         0x89d9  Station Not Server
         0x89da  Queue Halted
         0x89db  Maximum Queue Servers
         0x89ff  Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 126  Set Queue Current Status
         23 103  Set Queue Current Status

CHANGES: 21 Sep 1993 - NWNCP Enabled - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWSetQueueCurrentStatus2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        queueStatus
)
{
   nuint16 serverVer;
   NWCCODE ccode;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   /*
   **  If more than the first 3 bits are set then return an error.
   */
   if (queueStatus > 7)
      return (INVALID_PARAMETER);

   if ((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   /* NWCalls is broken and expects the queue ID to be anti-native */
   queueID = NSwap32(queueID);

   if(serverVer >= 3110 || serverVer < 2000)
   {
      return ((NWCCODE)NWNCP23s126SetQCurrStatus(&access, queueID,
         queueStatus));
   }
   else
   {
      return ((NWCCODE)NWNCP23s103SetQCurrStatus(&access, queueID,
         (nuint8) queueStatus));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/setqsta2.c,v 1.7 1994/09/26 17:50:01 rebekah Exp $
*/
