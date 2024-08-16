/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:clsabtq2.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
#include "ncpqms.h"

#include "nwcaldef.h"
#include "nwintern.h"
#include "nwqms.h"
#include "nwfile.h"
#include "nwserver.h"

/*manpage*NWCloseFileAndAbortQueueJob2**************************************
SYNTAX:  NWCCODE N_API NWCloseFileAndAbortQueueJob2
         (
            NWCONN_HANDLE  conn,
            nuint32        queueID,
            nuint32        jobNumber,
            NWFILE_HANDLE  fileHandle
         )

REMARKS: Closes the queue file and aborts the job.

ARGS:

INCLUDE: nwcaldef.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 128  Remove Job From Queue
         23 106  Remove Job From Queue

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWCloseFileAndAbortQueueJob2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        jobNumber,
   NWFILE_HANDLE  fileHandle
)
{
   NWCCODE ccode;
   nuint16 serverVer;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWCloseFile(fileHandle)) != 0)
      return ccode;

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   /* NWCalls is broken and expects the queue ID to be anti-native */
   queueID = NSwap32(queueID);

   if(serverVer >= 3110 || serverVer < 2000)
   {
      return  ((NWCCODE)NWNCP23s128RemoveJobFromQ(&access, queueID,
         jobNumber));
   }
   else
   {
      return  ((NWCCODE)NWNCP23s106RemoveJobFromQ(&access, queueID,
         (nuint16)jobNumber));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/clsabtq2.c,v 1.7 1994/09/26 17:44:37 rebekah Exp $
*/
