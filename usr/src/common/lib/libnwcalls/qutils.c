/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:qutils.c	1.7"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwqms.h"

/*manpage*ConvertQueueToNWQueue**********************************************
SYNTAX:  void N_API ConvertQueueToNWQueue
         (
            NWQueueJobStruct NWPTR NWJob,
            QueueJobStruct   NWPTR qJob
         )

REMARKS:

ARGS: <  NWJob
      >  qJob

INCLUDE: nwintern.h

RETURN:

CLIENT:  DOS WIN OS2 NT

SEE:

CHANGES: 21 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
void N_API ConvertQueueToNWQueue
(
   NWQueueJobStruct NWPTR NWJob,
   QueueJobStruct   NWPTR qJob
)
{
   NWJob->clientStation          = (nuint32) qJob->clientStation;
   NWJob->clientTask             = (nuint32) qJob->clientTask;
   NWJob->clientID               = (nuint32) qJob->clientID;
   NWJob->targetServerID         = (nuint32) qJob->targetServerID;
   NWJob->jobNumber              = (nuint32) qJob->jobNumber;
   NWJob->jobType                = (nuint16) qJob->jobType;
   NWJob->jobPosition            = (nuint16) qJob->jobPosition;
   NWJob->jobControlFlags        = (nuint16) qJob->jobControlFlags;
   NWJob->servicingServerStation = (nuint32) qJob->servicingServerStation;
   NWJob->servicingServerTask    = (nuint32) qJob->servicingServerTask;
   NWJob->servicingServerID      = (nuint32) qJob->servicingServerID;

   NWCMemMove(NWJob->targetExecutionTime, qJob->targetExecutionTime, (nuint) 6);
   NWCMemMove(NWJob->jobEntryTime, qJob->jobEntryTime, (nuint) 6);
   NWCMemMove(NWJob->jobFileName, qJob->jobFileName, (nuint) 14);
   NCopyHiLo32(&NWJob->jobFileHandle, &qJob->jobFileHandle[2]);

   /* move last two fields of struct */
   NWCMemMove(NWJob->jobDescription, qJob->jobDescription,
         (nuint) 50);
   NWCMemMove(NWJob->clientRecordArea, qJob->clientRecordArea,
         (nuint) 152);
}

void N_API ConvertNWQueueToQueue
(
   QueueJobStruct   NWPTR qJob,
   NWQueueJobStruct NWPTR NetWareJob
)
{
   qJob->clientStation          = (nuint8)  NetWareJob->clientStation;
   qJob->clientTask             = (nuint8)  NetWareJob->clientTask;
   qJob->clientID               = (nuint32) NetWareJob->clientID;
   qJob->targetServerID         = (nuint32) NetWareJob->targetServerID;
   qJob->jobNumber              = (nuint16) NetWareJob->jobNumber;
   qJob->jobType                = (nuint16) NetWareJob->jobType;
   qJob->jobPosition            = (nuint8)  NetWareJob->jobPosition;
   qJob->jobControlFlags        = (nuint8)  NetWareJob->jobControlFlags;
   qJob->servicingServerStation = (nuint8)  NetWareJob->servicingServerStation;
   qJob->servicingServerTask    = (nuint8)  NetWareJob->servicingServerTask;
   qJob->servicingServerID      = (nuint32) NetWareJob->servicingServerID;

   NWCMemMove(qJob->targetExecutionTime, NetWareJob->targetExecutionTime, 6);
   NWCMemMove(qJob->jobEntryTime, NetWareJob->jobEntryTime, 6);
   NWCMemMove(qJob->jobFileName, NetWareJob->jobFileName, 14);
   NCopyHiLo32(&qJob->jobFileHandle[2], &NetWareJob->jobFileHandle, 4);

   /* move last two fields of struct */
   NWCMemMove(qJob->jobDescription, NetWareJob->jobDescription,
         (nuint) 50);
   NWCMemMove(qJob->clientRecordArea, NetWareJob->clientRecordArea,
         (nuint) 152);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/qutils.c,v 1.9 1994/09/30 22:56:38 rebekah Exp $
*/
