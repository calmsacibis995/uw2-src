/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtjobsiz.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
#include "nwcaldef.h"
#include "nwqms.h"
#include "ncpqms.h"

/*manpage*NWGetQueueJobFileSize*********************************************
SYNTAX:  NWCCODE N_API NWGetQueueJobFileSize
         (
            NWCONN_HANDLE  conn,
            nuint32        queueID,
            nuint16        jobNumber,
            pnuint32       fileSize
         );

REMARKS:

ARGS:  > conn
       > queueID
       > jobNumber
      <  fileSize

INCLUDE: nwqms.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 120  Get Queue Job File Size

CHANGES: 15 Sep 1993 - NWNCP Enabled - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetQueueJobFileSize
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint16        jobNumber,
   pnuint32       fileSize
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   /* NWCalls is broken and expects the queue ID to be anti-native */
   queueID = NSwap32(queueID);

   return ((NWCCODE) NWNCP23s120GetQJobFileSize(&access, queueID,
      jobNumber, NULL, NULL, fileSize));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtjobsiz.c,v 1.7 1994/09/26 17:47:02 rebekah Exp $
*/
