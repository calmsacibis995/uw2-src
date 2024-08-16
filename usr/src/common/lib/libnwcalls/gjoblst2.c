/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gjoblst2.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
#include "ncpqms.h"

#include "nwcaldef.h"
#include "nwqms.h"
#include "nwserver.h"

/*manpage*NWGetQueueJobList2************************************************
SYNTAX:  NWCCODE N_API NWGetQueueJobList2
         (
            NWCONN_HANDLE           conn,
            nuint32                 queueID,
            nuint32                 queueStartPosition,
            QueueJobListReply NWPTR job
         )

REMARKS: Returns the number of jobs on queue 'queueID' in the
         variable 'jobCount' and a list of the job numbers in the current
         queue order in the array 'jobList.'

ARGS:  > conn
       > queueID
      <  queueStartPosition
      <  job

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

NCP:     23 129  Get Queue Job List (3.11 and above)
         23 107  Get Queue Job List (less than 3.11)

CHANGES: 2 Sep 1993 - NWNCP Enabled - djharris
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWGetQueueJobList2
(
   NWCONN_HANDLE           conn,
   nuint32                 queueID,
   nuint32                 queueStartPosition,
   QueueJobListReply NWPTR job
)
{
   NWCCODE ccode;
   nuint16 i, serverVer, jobCount, ajobList[250];

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   queueID = NSwap32(queueID);

   if(serverVer >= 3110 || serverVer < 2000)
   {
      ccode = (NWCCODE)NWNCP23s129GetQJobList(&access, queueID,
         queueStartPosition,  &(job->totalQueueJobs),
         &(job->replyQueueJobNumbers), job->jobNumberList);
   }
   else
   {
      if ((ccode = (NWCCODE)NWNCP23s107GetQJobList(&access, queueID,
         &jobCount, ajobList)) == 0)
      {
         for(i = 0; i < jobCount; i++)
            job->jobNumberList[i] = (nuint32)ajobList[i];

         job->replyQueueJobNumbers = job->totalQueueJobs = jobCount;
      }
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gjoblst2.c,v 1.7 1994/09/26 17:46:31 rebekah Exp $
*/
