/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:finishj2.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpqms.h"

#include "nwcaldef.h"
#include "nwintern.h"
#include "nwundoc.h"
#include "nwqms.h"
#include "nwfile.h"
#include "nwserver.h"

/*manpage*NWFinishServicingQueueJob2****************************************
SYNTAX:  NWCCODE N_API NWFinishServicingQueueJob2
         (
            NWCONN_HANDLE  conn,
            nuint32        queueID,
            nuint32        jobNumber,
            NWFILE_HANDLE  fileHandle
         )

REMARKS: This call signals the queue management process the the job the job
         specified by 'jobNumber' is finished. This destroys the job entry,
         closes and deletes the job file, and restores the calling servers
         access rights to their original value.

ARGS: <> conn
       > queueID
       > jobNumber
       > fileHandle

INCLUDE: nwqms.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 131  Finish Servicing Queue Job
         23 114  Finish Servicing Queue Job

CHANGES: 21 Sep 1993 - NWNCP Enabled - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWFinishServicingQueueJob2
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

   if (( ccode = NWCloseFile(fileHandle)) != 0)
		return(ccode);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   queueID = NSwap32(queueID);

   if(serverVer >= 3110 || serverVer < 2000)
   {
      return ((NWCCODE) NWNCP23s131FinishServicingQJob(&access, queueID,
         jobNumber, (nuint32) 0L));
   }
   else
   {
      return ((NWCCODE) NWNCP23s114FinishServicingQJob(&access, queueID,
         (nuint16) jobNumber));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/finishj2.c,v 1.7 1994/09/26 17:45:32 rebekah Exp $
*/
