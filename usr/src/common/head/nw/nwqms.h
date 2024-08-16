/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwqms.h	1.4"
/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWQMS_INC
#define NWQMS_INC

#ifndef NWCALDEF_INC
#ifdef N_PLAT_UNIX
# include <nw/nwcaldef.h>
#else /* !N_PLAT_UNIX */
# include <nwcaldef.h>
#endif /* N_PLAT_UNIX */
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else /* !N_PLAT_UNIX */
#include <npackon.h>
#endif /* N_PLAT_UNIX */

#ifdef __cplusplus
extern "C" {
#endif

#define QF_AUTO_START          0x08
#define QF_ENTRY_RESTART       0x10
#define QF_ENTRY_OPEN          0x20
#define QF_USER_HOLD           0x40
#define QF_OPERATOR_HOLD       0x80

#define QS_CANT_ADD_JOBS       0x01
#define QS_SERVERS_CANT_ATTACH 0x02
#define QS_CANT_SERVICE_JOBS   0x04

typedef struct
{
  nuint8  clientStation;
  nuint8  clientTask;
  nuint32 clientID;
  nuint32 targetServerID;
  nuint8  targetExecutionTime[6];
  nuint8  jobEntryTime[6];
  nuint16 jobNumber;
  nuint16 jobType;
  nuint8  jobPosition;
  nuint8  jobControlFlags;
  nuint8  jobFileName[14];
  nuint8  jobFileHandle[6];
  nuint8  servicingServerStation;
  nuint8  servicingServerTask;
  nuint32 servicingServerID;
  nuint8  jobDescription[50];
  nuint8  clientRecordArea[152];
} QueueJobStruct;

typedef struct
{
  nuint8  clientStation;
  nuint8  clientTask;
  nuint32 clientID;
  nuint32 targetServerID;
  nuint8  targetExecutionTime[6];
  nuint8  jobEntryTime[6];
  nuint16 jobNumber;
  nuint16 jobType;
  nuint8  jobPosition;
  nuint8  jobControlFlags;
  nuint8  jobFileName[14];
  nuint8  jobFileHandle[6];
  nuint8  servicingServerStation;
  nuint8  servicingServerTask;
  nuint32 servicingServerID;
} ReplyJobStruct;

typedef struct
{
  nuint32 clientStation;
  nuint32 clientTask;
  nuint32 clientID;
  nuint32 targetServerID;
  nuint8  targetExecutionTime[6];
  nuint8  jobEntryTime[6];
  nuint32 jobNumber;
  nuint16 jobType;
  nuint16 jobPosition;
  nuint16 jobControlFlags;
  nuint8  jobFileName[14];
  nuint32 jobFileHandle;
  nuint32 servicingServerStation;
  nuint32 servicingServerTask;
  nuint32 servicingServerID;
  nuint8  jobDescription[50];
  nuint8  clientRecordArea[152];
} NWQueueJobStruct;

typedef struct
{
  nuint32 clientStation;
  nuint32 clientTask;
  nuint32 clientID;
  nuint32 targetServerID;
  nuint8  targetExecutionTime[6];
  nuint8  jobEntryTime[6];
  nuint32 jobNumber;
  nuint16 jobType;
  nuint16 jobPosition;
  nuint16 jobControlFlags;
  nuint8  jobFileName[14];
  nuint32 jobFileHandle;
  nuint32 servicingServerStation;
  nuint32 servicingServerTask;
  nuint32 servicingServerID;
} NWReplyJobStruct;

typedef struct
{
  nuint32 totalQueueJobs;
  nuint32 replyQueueJobNumbers;
  nuint32 jobNumberList[250];   /* 250 to hold job #'s for old NCP*/
} QueueJobListReply;

NWCCODE N_API NWCreateQueueFile
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   QueueJobStruct N_FAR * job,
   NWFILE_HANDLE N_FAR * fileHandle
);

NWCCODE N_API NWCreateQueueFile2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   NWQueueJobStruct N_FAR * job,
   NWFILE_HANDLE N_FAR * fileHandle
);

NWCCODE N_API NWCloseFileAndStartQueueJob
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint16        jobNumber,
   NWFILE_HANDLE  fileHandle
);

NWCCODE N_API NWCloseFileAndStartQueueJob2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        jobNumber,
   NWFILE_HANDLE  fileHandle
);

NWCCODE N_API NWCloseFileAndAbortQueueJob
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint16        jobNumber,
   NWFILE_HANDLE  fileHandle
);

NWCCODE N_API NWCloseFileAndAbortQueueJob2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        jobNumber,
   NWFILE_HANDLE  fileHandle
);

NWCCODE N_API NWRemoveJobFromQueue
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint16        jobNumber
);

NWCCODE N_API NWRemoveJobFromQueue2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        jobNumber
);

NWCCODE N_API NWGetQueueJobList
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   pnuint16       jobCount,
   pnuint16       jobList
);

NWCCODE N_API NWGetQueueJobList2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        queueStartPos,
   QueueJobListReply N_FAR * job
);

NWCCODE N_API NWReadQueueJobEntry
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint16        jobNumber,
   QueueJobStruct N_FAR * job
);

NWCCODE N_API NWReadQueueJobEntry2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        jobNumber,
   NWQueueJobStruct N_FAR * job
);

NWCCODE N_API NWGetQueueJobFileSize
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint16        jobNumber,
   pnuint32       fileSize
);

NWCCODE N_API NWGetQueueJobFileSize2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        jobNumber,
   pnuint32       fileSize
);

NWCCODE N_API NWChangeQueueJobEntry
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   QueueJobStruct N_FAR * job
);

NWCCODE N_API NWChangeQueueJobEntry2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   NWQueueJobStruct N_FAR * job
);

NWCCODE N_API NWChangeQueueJobPosition
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint16        jobNumber,
   nuint8         newJobPos
);

NWCCODE N_API NWChangeQueueJobPosition2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        jobNumber,
   nuint32        newJobPos
);

NWCCODE N_API NWServiceQueueJob
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint16        targetJobType,
   QueueJobStruct N_FAR * job,
   NWFILE_HANDLE N_FAR * fileHandle
);

NWCCODE N_API NWServiceQueueJob2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint16        targetJobType,
   NWQueueJobStruct N_FAR * job,
   NWFILE_HANDLE N_FAR * fileHandle
);

NWCCODE N_API NWAbortServicingQueueJob
(
   NWCONN_HANDLE  conn,
   nuint32        QueueID,
   nuint16        JobNumber,
   NWFILE_HANDLE  fileHandle
);

NWCCODE N_API NWAbortServicingQueueJob2
(
   NWCONN_HANDLE  conn,
   nuint32        QueueID,
   nuint32        JobNumber,
   NWFILE_HANDLE  fileHandle
);

NWCCODE N_API NWChangeToClientRights
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint16        jobNumber
);

NWCCODE N_API NWChangeToClientRights2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        jobNumber
);

NWCCODE N_API NWFinishServicingQueueJob
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint16        jobNumber,
   NWFILE_HANDLE  fileHandle
);

NWCCODE N_API NWFinishServicingQueueJob2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        jobNumber,
   NWFILE_HANDLE  fileHandle
);

NWCCODE N_API NWGetPrinterQueueID
(
   NWCONN_HANDLE  conn,
   nuint16        printerNum,
   pnuint32       queueID
);

NWCCODE N_API NWCreateQueue
(
   NWCONN_HANDLE  conn,
   pnstr8         queueName,
   nuint16        queueType,
   nuint8         dirPath,
   pnstr8         path,
   pnuint32       queueID
);

NWCCODE N_API NWDestroyQueue
(
   NWCONN_HANDLE  conn,
   nuint32        queueID
);

NWCCODE N_API NWReadQueueCurrentStatus
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   pnuint8        queueStatus,
   pnuint16       numberOfJobs,
   pnuint16       numberOfServers,
   pnuint32       serverIDlist,
   pnuint16       serverConnList
);

NWCCODE N_API NWReadQueueCurrentStatus2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   pnuint32       queueStatus,
   pnuint32       numberOfJobs,
   pnuint32       numberOfServers,
   pnuint32       serverIDlist,
   pnuint32       serverConnList
);

NWCCODE N_API NWSetQueueCurrentStatus
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint8         queueStatus
);

NWCCODE N_API NWSetQueueCurrentStatus2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        queueStatus
);

NWCCODE N_API NWReadQueueServerCurrentStatus
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        serverID,
   nuint16        serverConn,
   nptr           statusRec
);

NWCCODE N_API NWReadQueueServerCurrentStatus2
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nuint32        serverID,
   nuint32        serverConn,
   nptr           statusRec
);

NWCCODE N_API NWAttachQueueServerToQueue
(
   NWCONN_HANDLE  conn,
   nuint32        queueID
);

NWCCODE N_API NWDetachQueueServerFromQueue
(
   NWCONN_HANDLE  conn,
   nuint32        queueID
);

NWCCODE N_API NWRestoreQueueServerRights
(
   NWCONN_HANDLE  conn
);

NWCCODE N_API NWSetQueueServerCurrentStatus
(
   NWCONN_HANDLE  conn,
   nuint32        queueID,
   nptr           statusRec
);

#ifdef __cplusplus
}
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else /* !N_PLAT_UNIX */
#include <npackoff.h>
#endif /* N_PLAT_UNIX */

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwqms.h,v 1.6 1994/06/08 23:33:23 rebekah Exp $
*/
