/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwsync.h	1.4"
/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWSYNC_INC
#define NWSYNC_INC

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

typedef struct
{
  NWCONN_NUM connNumber;
  nuint16 taskNumber;
  nuint8  lockStatus;
} LOGICAL_LOCK;

typedef struct
{
  nuint16 useCount;
  nuint16 shareableLockCount;
  nuint8  locked;
  nuint16 nextRequest;
  nuint16 numRecords;
  LOGICAL_LOCK logicalLock[128];
  nuint16 curRecord;
} LOGICAL_LOCKS;

typedef struct
{
  nuint16 taskNumber;
  nuint8  lockStatus;
  nstr8   logicalName[128];
} CONN_LOGICAL_LOCK;

typedef struct
{
  nuint16 nextRequest;
  nuint16 numRecords;
  nuint8  records[508];
  nuint16 curOffset;
  nuint16 curRecord;
} CONN_LOGICAL_LOCKS;

typedef struct
{
  nuint16 loggedCount;
  nuint16 shareableLockCount;
  nuint32 recordStart;
  nuint32 recordEnd;
  nuint16 connNumber;
  nuint16 taskNumber;
  nuint8  lockType;
} PHYSICAL_LOCK;

typedef struct
{
  nuint16 nextRequest;
  nuint16 numRecords;
  PHYSICAL_LOCK locks[32];
  nuint16 curRecord;
  nuint8  reserved[8];
} PHYSICAL_LOCKS;

typedef struct
{
  nuint16 taskNumber;
  nuint8  lockType;
  nuint32 recordStart;
  nuint32 recordEnd;
} CONN_PHYSICAL_LOCK;

typedef struct
{
  nuint16 nextRequest;
  nuint16 numRecords;
  CONN_PHYSICAL_LOCK locks[51];
  nuint16 curRecord;
  nuint8  reserved[22];
} CONN_PHYSICAL_LOCKS;

typedef struct
{
  NWCONN_NUM connNumber;
  nuint16 taskNumber;
} SEMAPHORE;

typedef struct
{
  nuint16 nextRequest;
  nuint16 openCount;
  nuint16 semaphoreValue;
  nuint16 semaphoreCount;
  SEMAPHORE semaphores[170];
  nuint16 curRecord;
} SEMAPHORES;

typedef struct
{
  nuint16 openCount;
  nuint16 semaphoreValue;
  nuint16 taskNumber;
  nstr8   semaphoreName[128];
} CONN_SEMAPHORE;

typedef struct
{
  nuint16 nextRequest;
  nuint16 numRecords;
  nuint8  records[508];
  nuint16 curOffset;
  nuint16 curRecord;
} CONN_SEMAPHORES;


NWCCODE N_API NWScanPhysicalLocksByFile
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   nuint8         dataStream,
   pnint16        iterHandle,
   PHYSICAL_LOCK N_FAR * lock,
   PHYSICAL_LOCKS N_FAR * locks
);

NWCCODE N_API NWScanLogicalLocksByConn
(
   NWCONN_HANDLE  conn,
   NWCONN_NUM     connNum,
   pnint16        iterHandle,
   CONN_LOGICAL_LOCK N_FAR * logicalLock,
   CONN_LOGICAL_LOCKS N_FAR * logicalLocks
);

NWCCODE N_API NWScanPhysicalLocksByConnFile
(
   NWCONN_HANDLE  conn,
   NWCONN_NUM     connNum,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   nuint8         dataStream,
   pnint16        iterHandle,
   CONN_PHYSICAL_LOCK N_FAR * lock,
   CONN_PHYSICAL_LOCKS N_FAR * locks
);

NWCCODE N_API NWScanLogicalLocksByName
(
   NWCONN_HANDLE  conn,
   pnstr8         logicalName,
   pnint16        iterHandle,
   LOGICAL_LOCK N_FAR * logicalLock,
   LOGICAL_LOCKS N_FAR * logicalLocks
);

NWCCODE N_API NWScanSemaphoresByConn
(
   NWCONN_HANDLE  conn,
   NWCONN_NUM     connNum,
   pnint16        iterHandle,
   CONN_SEMAPHORE N_FAR * semaphore,
   CONN_SEMAPHORES N_FAR * semaphores
);

NWCCODE N_API NWScanSemaphoresByName
(
   NWCONN_HANDLE  conn,
   pnstr8         semName,
   pnint16        iterHandle,
   SEMAPHORE N_FAR * semaphore,
   SEMAPHORES N_FAR * semaphores
);

NWCCODE N_API NWSignalSemaphore
(
   NWCONN_HANDLE  conn,
   nuint32        semHandle
);

NWCCODE N_API NWCloseSemaphore
(
   NWCONN_HANDLE  conn,
   nuint32        semHandle
);

NWCCODE N_API NWOpenSemaphore
(
   NWCONN_HANDLE  conn,
   pnstr8         semName,
   nint16         initSemHandle,
   pnuint32       semHandle,
   pnuint16       semOpenCount
);

NWCCODE N_API NWExamineSemaphore
(
   NWCONN_HANDLE  conn,
   nuint32        semHandle,
   pnint16        semValue,
   pnuint16       semOpenCount
);

NWCCODE N_API NWWaitOnSemaphore
(
   NWCONN_HANDLE  conn,
   nuint32        semHandle,
   nuint16        timeOutValue
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
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwsync.h,v 1.6 1994/06/08 23:33:27 rebekah Exp $
*/
