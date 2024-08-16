/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gjobsiz2.c	1.5"
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

/*manpage*NWGetQueueJobFileSize2********************************************
SYNTAX:  NWCCODE N_API NWGetQueueJobFileSize2
         (
            NWCONN_HANDLE  conn,
            nuint32        queueID,
            nuint32        jobNumber,
            pnuint32       fileSize
         )

REMARKS: Returns the file size of the file associated with a queue entry.

ARGS:  > conn
       > queueID
       > jobNumber
      <  fileSize

INCLUDE: nwqms.h

RETURN:  0x0000  Successful
         0x8999  Directory Full Error
         0x89D0  Queue Error
         0x89D1  No Queue
         0x89D2  No Queue Server
         0x89D3  No Queue Rights
         0x89D4  Queue Full
         0x89D5  No Queue Job
         0x89D6  No Job Right
         0x89D7  Queue Servicing
         0x89D8  Queue Not Active
         0x89D9  Station Not Server
         0x89DA  Queue Halted
         0x89DB  Maximum Queue Servers
         0x89FF  Failure

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 135  Get Queue Job Size
         23 120  Get Queue Job Size

CHANGES: 2 Sep 1993 - NWNCP Enabled - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWGetQueueJobFileSize2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        jobNumber,
   pnuint32       fileSize
)
{
   nuint16 serverVer;
   NWCCODE ccode;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   /* NWCalls is broken and expects the queue ID to be anti-native */
   queueID = NSwap32(queueID);

   if(serverVer >= 3110 || serverVer < 2000)
   {
      return ((NWCCODE) NWNCP23s135GetQJobFileSize(&access, queueID,
         jobNumber, NULL, NULL, fileSize));
   }
   else
   {
      return ((NWCCODE) NWNCP23s120GetQJobFileSize(&access, queueID,
         (nuint16) jobNumber, NULL, NULL, fileSize));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gjobsiz2.c,v 1.7 1994/09/26 17:46:32 rebekah Exp $
*/
