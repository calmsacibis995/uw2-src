/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/ncpqms.h	1.5"
#ifndef NCPQMS_H
#define NCPQMS_H

/* Queue Services */

#if !defined( NTYPES_H )
#ifdef N_PLAT_UNIX
#include <nw/ntypes.h>
#else /* !N_PLAT_UNIX */
#include <ntypes.h>
#endif /* N_PLAT_UNIX */
#endif

#if !defined( NWACCESS_H )
#ifdef N_PLAT_UNIX
#include <nw/nwaccess.h>
#else /* !N_PLAT_UNIX */
#include <nwaccess.h>
#endif /* N_PLAT_UNIX */
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else /* !N_PLAT_UNIX */
#include <npackon.h>
#endif /* N_PLAT_UNIX */

typedef struct tagNWNCPQMSJobStruct
{
  nuint8  buClientStation;
  nuint8  buClientTask;
  nuint32 luClientID;
  nuint32 luTargetServerID;
  nuint8  abuTargetExecutionTime[6];
  nuint8  abuJobEntryTime[6];
  nuint16 suJobNumber;
  nuint16 suJobType;
  nuint8  buJobPosition;
  nuint8  buJobControlFlags;
  nstr8   abuJobFileName[14];
  nuint8  abuJobFileHandle[6];
  nuint8  buServicingServerStation;
  nuint8  buServicingServerTask;
  nuint32 luServicingServerID;
  nstr8   abuJobDescription[50];
  nuint8  abuClientRecordArea[152];
} NWNCPQMSJobStruct, N_FAR * pNWNCPQMSJobStruct;

typedef struct tagNWNCPQMSJobStruct2
{
  nuint16 suRecordInUseFlag;
  nuint32 luPreviousRecord;
  nuint32 luNextRecord;
  nuint32 luClientStation;
  nuint32 luClientTask;
  nuint32 luClientID;
  nuint32 luTargetServerID;
  nuint8  abuTargetExecutionTime[6];
  nuint8  abuJobEntryTime[6];
  nuint32 luJobNumber;
  nuint16 suJobType;
  nuint16 suJobPosition;
  nuint16 suJobControlFlags;
  nstr8   abuJobFileName[14];
  nuint32 luJobFileHandle;
  nuint32 luServicingServerStation;
  nuint32 luServicingServerTask;
  nuint32 luServicingServerID;
  nstr8   abuJobDescription[50];
  nuint8  abuClientRecordArea[152];
} NWNCPQMSJobStruct2, N_FAR * pNWNCPQMSJobStruct2;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   Function prototypes
****************************************************************************/

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s115AbortServicingQJob
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint16  suJobNumber
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s132AbortServicingQJob
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint32  luJobNumber
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s111AttachQServerToQ
(
   pNWAccess pAccess,
   nuint32  luQueueID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s130ChangeJobPriority
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint32  luJobNumber,
   nuint32  luPriority
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s136MoveQueueJob
(
   pNWAccess          pAccess,
   nuint32           luSrcQueueID,
   nuint32           luSrcQueuePos,
   nuint32           luDstQueueID,
   pnuint32          pluNewJobNum
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s137GetJobsFromList
(
   pNWAccess    pAccess,
   nuint32     luQueueID,
   nuint32     luQueueStartPos,
   nuint32     luFormCount,
   pnuint16    psuFormList,
   pnuint32    pluTotalJobs,
   pnuint32    pluJobNumCnt,
   pnuint32    pluJobNumListB128
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s138ServiceQJobList
(
   pNWAccess          pAccess,
   nuint32           luQueueID,
   nuint32           luFormCount,
   pnuint16          psuFormListB,
   pNWNCPQMSJobStruct2 pJobStruct2
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s109ChangeQJobEntry
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   pNWNCPQMSJobStruct pQMSJobStruct
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s123ChangeQJobEntry
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   pNWNCPQMSJobStruct2 pQMSJobStruct2
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s110ChangeQJobPosition
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint16  suJobNumber,
   nuint8   buNewPosition
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s116ChangeToClientRights
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint16  suJobID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s133ChangeToClientRights
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint32  luJobNumber
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s105CloseFileStartQJob
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint16  suJobNumber
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s127CloseFileStartQJob
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint32  luJobNumber
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s100CreateQ
(
   pNWAccess pAccess,
   nuint16  suQueueType,
   nuint8   buQueueNameLength,
   pnstr8   pbstrQueueName,
   nuint8   buPathBase,
   nuint8   buPathLength,
   pnstr8   pbstrPathName,
   pnuint32 pluQueueID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s104CreateQJobAndFile
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   pNWNCPQMSJobStruct  pJobStructIn,
   pNWNCPQMSJobStruct  pJobStructOut
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s121CreateQJobAndFile
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   pNWNCPQMSJobStruct2 pJobStructIn,
   pNWNCPQMSJobStruct2 pJobStructOut
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s101DestroyQ
(
   pNWAccess pAccess,
   nuint32  luQueueID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s112DetachQServerFromQ
(
   pNWAccess pAccess,
   nuint32  luQueueID
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s114FinishServicingQJob
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint16  suJobNumber
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s131FinishServicingQJob
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint32  luJobNumber,
   nuint32  luChargeInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s120GetQJobFileSize
(
   pNWAccess pAccess,
   nuint32  luInQueueID,
   nuint16  suInJobNumber,
   pnuint32 pluOutQueueID,
   pnuint16 psuOutJobNumber,
   pnuint32 pluFileSize
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s135GetQJobFileSize
(
   pNWAccess pAccess,
   nuint32  luInQueueID,
   nuint32  luInJobNumber,
   pnuint32 pluOutQueueID,
   pnuint32 pluOutJobNumber,
   pnuint32 pluFileSize
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s107GetQJobList
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   pnuint16 psuJobCount,
   pnuint16 psuJobNumbersB250
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s129GetQJobList
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint32  luQueueStartPosition,
   pnuint32 pluTotalQueueJobs,
   pnuint32 pluReplyQueueJobNumbers,
   pnuint32 pluJobNumberListB125
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s102ReadQCurrStatus
(
   pNWAccess pAccess,
   nuint32  luInQueueID,
   pnuint32 pluOutQueueID,
   pnuint8  pbuQueueStatus,
   pnuint8  pbuCurrentEntries,
   pnuint8  pbuCurrentServers,
   pnuint32 pluServerIDList,
   pnuint8  pbuServerStationList
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s125ReadQCurrStatus
(
   pNWAccess pAccess,
   nuint32  luInQueueID,
   pnuint32 pluOutQueueID,
   pnuint32 pluQueueStatus,
   pnuint32 pluCurrentEntries,
   pnuint32 pluCurrentServers,
   pnuint32 pluServerIDList,
   pnuint32 pluServerStationList
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s108ReadQJobEntry
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint16  suJobNumber,
   pNWNCPQMSJobStruct pQMSJobStruct
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s122ReadQJobEntry
(
   pNWAccess pAccess,
   nuint32  pluQueueID,
   nuint32  pluJobNumber,
   pNWNCPQMSJobStruct2 pQMSJobStruct2
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s118ReadQServerCurrStat
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint32  luServerID,
   nuint8   buServerStation,
   pnuint8  pbuServerStatusRecordB64
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s134ReadQServerCurrStat
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint32  luServerID,
   nuint32  luServerStation,
   pnuint8  pbuServerStatusRecordB64
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s106RemoveJobFromQ
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint16  luJobNumber
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s128RemoveJobFromQ
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint32  luJobNumber
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s117RestoreQServerRights
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s113ServiceQJob
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint16  suTargetServiceType,
   pNWNCPQMSJobStruct  pJobStructOut
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s124ServiceQJob
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint16  suTargetServiceType,
   pNWNCPQMSJobStruct2 pJobStructOut
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s103SetQCurrStatus
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint8   buQueueStatus
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s126SetQCurrStatus
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   nuint32  luQueueStatus
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s119SetQServerCurrStatus
(
   pNWAccess pAccess,
   nuint32  luQueueID,
   pnuint8  pbuServerStatusRecordB64
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP17s10GetPrintersQueue
(
   pNWAccess pAccess,
   nuint8   buPrinterNumber,
   pnuint32 pluQueueID
);

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else /* !N_PLAT_UNIX */
#include <npackoff.h>
#endif /* N_PLAT_UNIX */

#ifdef __cplusplus
}
#endif

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/ncpqms.h,v 1.8 1994/09/26 17:11:32 rebekah Exp $
*/
