/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/ncpsync.h	1.5"
#if !defined( NCPSYNC_H )
#define NCPSYNC_H

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

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   Function prototypes
****************************************************************************/

N_EXTERN_LIBRARY( NWRCODE )
NWNCP4SyncLockFileSet
(
   pNWAccess pAccess,
   nuint16  suLockTimeOut
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP6SyncReleaseFileSet
(
   pNWAccess pAccess,
   nuint8   buLockFlag
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP7SyncClrFile
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buFileNameLen,
   pnstr8   pbstrFileName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP8SyncClrFileSet
(
   pNWAccess pAccess,
   nuint8   buLockFlag
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP11SyncClrLogRec
(
   pNWAccess pAccess,
   nuint8   buRecNameLen,
   pnstr8   pbstrRecName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP14SyncClrLogRecSet
(
   pNWAccess pAccess,
   nuint8   buLockFlag
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP30SyncClrPhyRec
(
   pNWAccess pAccess,
   nuint8   buReserved,
   pnuint8  pbuNWHandleB6,
   nuint32  luLockOffset,
   nuint32  luLockLen
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP31SyncClrPhyRecSet
(
   pNWAccess pAccess,
   nuint8   buLockFlags
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP32s4SyncSemClose
(
   pNWAccess pAccess,
   nuint32  luSemHandle
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP32s1SyncSemExamine
(
   pNWAccess pAccess,
   nuint32  luSemHandle,
   pnuint8  pbuSemVal,
   pnuint8  pbuSemOpenCnt
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP1SyncSetFileLock
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP2SyncRelFileLock
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP4SyncLockFileSet
(
   pNWAccess pAccess,
   nuint16  suLockTimeout
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP10SyncLockLogRecSet
(
   pNWAccess pAccess,
   nuint8   buLockFlag,
   nuint16  suLockTimeout
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP27SyncLockPhyRecSet
(
   pNWAccess pAccess,
   nuint8   buLockFlag,
   nuint16  suLockTimeout
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP3SyncLogFile
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buLockFlag,
   nuint16  suLockTimeout,
   nuint8   buFileNameLen,
   pnstr8   pbstrFileName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP9SyncLogLogRec
(
   pNWAccess pAccess,
   nuint8   buLockFlag,
   nuint16  suLockTimeout,
   nuint8   buRecNameLen,
   pnstr8   pbstrRecName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP26SyncLogPhyRec
(
   pNWAccess pAccess,
   nuint8   buLockFlag,
   pnuint8  pbuNWHandleB6,
   nuint32  luLockOffset,
   nuint32  luLockLen,
   nuint16  suLockTimeout
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP32s0SyncSemOpen
(
   pNWAccess pAccess,
   nuint8   buInitSemVal,
   nuint8   buSemNameLen,
   pnstr8   pbstrSemName,
   pnuint32 pluSemHandle,
   pnuint8  pbuSemOpenCnt
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP5SyncRelFile
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buFileNameLen,
   pnstr8   pbstrFileName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP6SyncRelFileSet
(
   pNWAccess pAccess,
   nuint8   buLockFlag
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP12SyncRelLogRec
(
   pNWAccess pAccess,
   nuint8   buRecNameLen,
   pnstr8   pbstrRecName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP13SyncRelLogRecSet
(
   pNWAccess pAccess,
   nuint8   buLockFlag
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP28SyncRelPhyRec
(
   pNWAccess pAccess,
   nuint8   buReserved,
   pnuint8  pbuNWHandleB6,
   nuint32  luLockOffset,
   nuint32  luLockLen
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP29SyncRelPhyRecSet
(
   pNWAccess pAccess,
   nuint8   buLockFlags
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP32s3SyncSemSignal
(
   pNWAccess pAccess,
   nuint32  luSemHandle
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP32s2SyncSemWaitOn
(
   pNWAccess pAccess,
   nuint32  luSemHandle,
   nuint16  suTimeOut
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP111s0SyncOpenSem
(
   pNWAccess pAccess,
   nuint8   buSemValue,
   nuint8   buSemLen,
   pnuint8  pbuSemNameB512,
   pnuint32 pluSemHandle,
   pnuint8  pbuSemOpenCount
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP111s4SyncCloseSem
(
   pNWAccess pAccess,
   nuint32  luSemHandle
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP111s2SyncSemWaitOn
(
   pNWAccess pAccess,
   nuint32  luSemHandle,
   nuint16  suSemTimeOut
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP111s3SyncSignalSem
(
   pNWAccess pAccess,
   nuint32  luSemHandle
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP111s1SyncExamineSem
(
   pNWAccess pAccess,
   nuint32  luSemHandle,
   pnuint8  pbuSemValue,
   pnuint8  pbuSemOpenCount
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP108SyncLockLogRecSet
(
   pNWAccess pAccess,
   nuint8   buLockFlag,
   nuint16  suLockTimeout
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP110SyncLockPhyRecSet
(
   pNWAccess pAccess,
   nuint8   buLockFlag,
   nuint16  suLockTimeout
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP105SyncLogFile
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint8   buLockFlag,
   nuint16  suLockTimeout,
   nuint8   buFileNameLen,
   pnstr8   pbstrFileName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP106LockFileSet
(
   pNWAccess pAccess,
   nuint16  suLockTimeout
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP107LogLogicalRecord
(
   pNWAccess pAccess,
   nuint8   buLockFlag,
   nuint16  suLockTimeout,
   nuint8   buSynchNameLen,
   pnstr8   pbstrSynchNameB255
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP109LogPhysicalRecord
(
   pNWAccess pAccess,
   nuint8   buLockFlag,
   pnuint8  pbuNWHandleB6,
   nuint32  luLockAreaStartOffset,
   nuint32  luLockAreaLen,
   nuint16  suLockTimeOut
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
$Header: /SRCS/esmp/usr/src/nw/head/nw/ncpsync.h,v 1.7 1994/09/26 17:11:37 rebekah Exp $
*/
