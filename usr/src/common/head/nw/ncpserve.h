/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/ncpserve.h	1.5"
#if !defined( NCPSERVE_H )
#define NCPSERVE_H

#ifndef NTYPES_H
#ifdef N_PLAT_UNIX
#include <nw/ntypes.h>
#else /* !N_PLAT_UNIX */
#include "ntypes.h"
#endif /* N_PLAT_UNIX */
#endif

#ifndef NWACCESS_H
#ifdef N_PLAT_UNIX
#include <nw/nwaccess.h>
#else /* !N_PLAT_UNIX */
#include "nwaccess.h"
#endif /* N_PLAT_UNIX */
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else /* !N_PLAT_UNIX */
#include <npackon.h>
#endif /* N_PLAT_UNIX */

typedef struct tagNWNCPFSStats
{
   nuint32 luSysIntervalMarker;
   nuint16 suConfigMaxOpenFiles;
   nuint16 suActualMaxOpenFiles;
   nuint16 suCurrOpenFiles;
   nuint32 luTotalFilesOpened;
   nuint32 luTotalReadReqs;
   nuint32 luTotalWriteReqs;
   nuint16 suCurrChangedFATs;
   nuint32 luTotalChangedFATs;
   nuint16 suFATWriteErrors;
   nuint16 suFatalFATWriteErrors;
   nuint16 suFATScanErrors;
   nuint16 suActualMaxIndexedFiles;
   nuint16 suActiveIndexedFiles;
   nuint16 suAttachedIndexedFiles;
   nuint16 suAvailableIndexedFiles;
} NWNCPFSStats, N_FAR *pNWNCPFSStats;

typedef struct tagNWNCPDiskStats
{
   nuint32 luSysIntervalMarker;
   nuint8  buPhysicalDiskChannel;
   nuint8  buDriveRemovableFlag;
   nuint8  buPhysicalDriveType;
   nuint8  buControllerDriveNum;
   nuint8  buControllerNum;
   nuint8  buControllerType;
   nuint32 luDriveSize;
   nuint16 suDriveCylinders;
   nuint8  buDriveHeads;
   nuint8  buSectorsPerTrack;
   nuint8  abuDriveDefStrB64[64];
   nuint16 suIOErrorCnt;
   nuint32 luHotFixTableStart;
   nuint16 suHotFixTableSize;
   nuint16 suHotFixBlocksAvailable;
   nuint8  buHotFixDisabled;
} NWNCPDiskStats, N_FAR *pNWNCPDiskStats;

typedef struct tagNWNCPDiskChannelStats
{
   nuint32 luSysElapsedTime;
   nuint16 suChannelState;
   nuint16 suChannelSyncState;
   nuint8  buDrvType;
   nuint8  buDrvMajorVer;
   nuint8  buDrvMinorVer;
   nuint8  abuDrvDescr[65];
   nuint16 suIOAddr1;
   nuint16 suIOAddr1Size;
   nuint16 suIOAddr2;
   nuint16 suIOAddr2Size;
   nuint8  abuSharedMem1Seg[3];
   nuint16 suSharedMem1Ofs;
   nuint8  abuSharedMem2Seg[3];
   nuint16 suSharedMem2Ofs;
   nuint8  buInterrupt1Used;
   nuint8  buInterrupt1;
   nuint8  buInterrupt2Used;
   nuint8  buInterrupt2;
   nuint8  buDMAChannel1Used;
   nuint8  buDMAChannel1;
   nuint8  buDMAChannel2Used;
   nuint8  buDMAChannel2;
   nuint8  buFlagBits;
   nuint8  buReserved;
   nuint8  abuConfigDescr[80];
} NWNCPDiskChannelStats, N_FAR *pNWNCPDiskChannelStats;

typedef struct tagNWNCPDskCacheStats
{
   nuint32 luSysElapsedTime;
   nuint16 suCacheBufCount;
   nuint16 suCacheBufSize;
   nuint16 suDirtyCacheBufs;
   nuint32 luCacheReadReqs;
   nuint32 luCacheWriteReqs;
   nuint32 luCacheHits;
   nuint32 luCacheMisses;
   nuint32 luPhysReadReqs;
   nuint32 luPhysWriteReqs;
   nuint16 suPhysReadErrors;
   nuint16 suPhysWriteErrors;
   nuint32 luCacheGetReqs;
   nuint32 luCacheFullWriteReqs;
   nuint32 luCachePartialWriteReqs;
   nuint32 luBackgroundDirtyWrites;
   nuint32 luBackgroundAgedWrites;
   nuint32 luTotCacheWrites;
   nuint32 luCacheAllocs;
   nuint16 suThrashingCount;
   nuint16 suLRUBlockWasDirtyCount;
   nuint16 suReadBeyondWriteCount;
   nuint16 suFragmentedWriteCount;
   nuint16 suCacheHitOnUnavailCount;
   nuint16 suCacheBlockScrappedCount;
} NWNCPDskCacheStats, N_FAR *pNWNCPDskCacheStats;

typedef struct tagNWNCPDrvMapTable
{
   nuint32 luSysElapsedTime;
   nuint8  buSFTSupportLevel;
   nuint8  buLogDriveCnt;
   nuint8  buPhysDriveCnt;
   nuint8  abuDiskChannelTable[5];
   nuint16 suPendingIOCmds;
   nuint8  abuDriveMappingTable[32];
   nuint8  abuDriveMirrorTable[32];
   nuint8  abuDeadMirrorTable[32];
   nuint8  buReMirrorDriveNumber;
   nuint8  buReserved;
   nuint32 luReMirrorCurrentOffset;
   nuint16 asuSFTErrorTable[60];
} NWNCPDrvMapTable, N_FAR *pNWNCPDrvMapTable;

typedef struct tagNWNCPVersionInfo
{
   nstr8   abstrServerName[48];
   nuint8  buNetWareVer;
   nuint8  buNetWareSubVer;
   nuint16 suMaxServConns;
   nuint16 suConnsInUse;
   nuint16 suNumMountedVols;
   nuint8  buRev;
   nuint8  buSFTLevel;
   nuint8  buTTSLevel;
   nuint16 suMaxConnsEverUsed;
   nuint8  buAcctVer;
   nuint8  buVAPVer;
   nuint8  buQueueVer;
   nuint8  buPrintVer;
   nuint8  buVirtualConsVer;
   nuint8  buRestrictionLevel;
   nuint8  buInternetBridge;
   nuint8  abuReserved[60];
} NWNCPVersionInfo, N_FAR *pNWNCPVersionInfo;

typedef struct tagNWNCPLANConfig
{
   nuint8  abuNetworkAddr[4];
   nuint8  abuHostAddr[6];
   nuint8  buBoardInstalled;
   nuint8  buOptionNum;
   nuint8  abuConfigText1[80];
   nuint8  abuConfigText2[80];
} NWNCPLANConfig, N_FAR *pNWNCPLANConfig;

typedef struct tagNWNCPLANIOStats
{
   nuint32 luSysIntervalMarker;
   nuint16 suConfigMaxRoutingBufs;
   nuint16 suActualMaxUsedRoutingBufs;
   nuint16 suCurrUsedRoutingBufs;
   nuint32 luTotFileServicePkts;
   nuint16 suTurboFileService;
   nuint16 suPktsFromInvalidConn;
   nuint16 suPktsFromBadLogConn;
   nuint16 suPktsRcvdDuringProcessing;
   nuint16 suReqsReprocessed;
   nuint16 suPktsWithBadSeqNum;
   nuint16 suDuplicRepliesSent;
   nuint16 suAcksSent;
   nuint16 suPktsWithBadReqType;
   nuint16 suAttachDuringProcessing;
   nuint16 suAttachsDuringAttachs;
   nuint16 suForgedDetachedReqs;
   nuint16 suDetachForBadConnNum;
   nuint16 suDetachDuringProcessing;
   nuint16 suRepliesCancelled;
   nuint16 suPktsDiscardedByHopCount;
   nuint16 suPktsDiscardedUnknownNet;
   nuint16 suInPktsDiscardedNoDGrp;
   nuint16 suOutPktsDiscardedNoBuffer;
   nuint16 suIPXNotMyNetwork;
   nuint32 luNetBIOSBrdcstsPropagated;
   nuint32 luTotOtherPkts;
   nuint32 luTotRoutedPkts;
} NWNCPLANIOStats, N_FAR *pNWNCPLANIOStats;

/* structures for 23 232 (2.2 only) */
typedef struct tagNWNCPDynMemAreas
{
   nuint32 luTotal;
   nuint32 luMaxUsed;
   nuint32 luCurrUsed;
} NWNCPDynMemAreas, N_FAR *pNWNCPDynMemAreas;

typedef struct tagNWNCPMiscServerInfo
{
   nuint32 luUpTime;
   nuint8  buProcessor;
   nuint8  buReserved;
   nuint8  buNumServiceProcs;
   nuint8  buUtilizationPerc;
   nuint16 suConfigMaxObjs;
   nuint16 suActualMaxObjs;
   nuint16 suCurrUsedObjs;
   nuint16 suTotServerMem;
   nuint16 suWastedMem;
   nuint16 suNumMemAreas;
   NWNCPDynMemAreas aDynMem[3];
} NWNCPMiscServerInfo, N_FAR *pNWNCPMiscServerInfo;

typedef struct tagNWNCPConnUsingFile2x
{
   nuint16  suConnNum;
   nuint8   buTaskNum;
   nuint8   buLockType;
   nuint8   buAccessControl;
   nuint8   buLockFlag;
} NWNCPConnUsingFile2x, N_FAR *pNWNCPConnUsingFile2x;

typedef struct tagNWNCPConnUsingFile3x
{
   nuint16  suConnNum;
   nuint16  suTaskNum;
   nuint8   buLockType;
   nuint8   buAccessControl;
   nuint8   buLockFlag;
} NWNCPConnUsingFile3x, N_FAR *pNWNCPConnUsingFile3x;

typedef struct tagNWNCPConnOpenFiles2x
{
   nuint8  buTaskNum;
   nuint8  buLockType;
   nuint8  buAccessControl;
   nuint8  buLockFlag;
   nuint8  buVolNum;
   nuint16 suDirEntry;
   nstr8   pbstrFileName[14];
} NWNCPConnOpenFiles2x, N_FAR *pNWNCPConnOpenFiles2x;

/* struct NWNCPConnOpenFiles3x is not used, due to the large amount
   of stack space that would be needed to allocate enough structs to
   hold as many "open file" structures as could be returned.  This struct
   is just a template, so you know what the data looks like. */

typedef struct tagNWNCPConnOpenFiles3x
{
   nuint16 suTaskNum;
   nuint8  buLockType;
   nuint8  buAccessControl;
   nuint8  buLockFlag;
   nuint8  buVolNum;
   nuint32 luParentDirEntry;
   nuint32 luDirEntry;
   nuint8  buForkCount;
   nuint8  buNameSpc;
   nuint8  buNameLen;
   nstr8   pbstrFileName[256];
} NWNCPConnOpenFiles3x, N_FAR *pNWNCPConnOpenFiles3x;

typedef struct tagNWNCPLogicalRecInfo2x
{
   nuint16 suConnNum;
   nuint8  buTaskNum;
   nuint8  buLockStatus;
} NWNCPLogicalRecInfo2x, N_FAR *pNWNCPLogicalRecInfo2x;

typedef struct tagNWNCPLogicalRecInfo3x
{
   nuint16 suConnNum;
   nuint16 suTaskNum;
   nuint8  buLockStatus;
} NWNCPLogicalRecInfo3x, N_FAR *pNWNCPLogicalRecInfo3x;

typedef struct tagNWNCPPhysRecLocks2x
{
   nuint16 suLoggedCount;
   nuint16 suShareableLockCount;
   nuint32 luRecStart;
   nuint32 luRecEnd;
   nuint16 suLogConnNum;
   nuint8  buTaskNum;
   nuint8  buLockType;
} NWNCPPhysRecLocks2x, N_FAR *pNWNCPPhysRecLocks2x;

typedef struct tagNWNCPPhysRecLocks3x
{
   nuint16 suLoggedCount;
   nuint16 suShareableLockCount;
   nuint32 luRecStart;
   nuint32 luRecEnd;
   nuint16 suLogConnNum;
   nuint16 suTaskNum;
   nuint16 suLockType;
} NWNCPPhysRecLocks3x, N_FAR *pNWNCPPhysRecLocks3x;

typedef struct tagNWNCPPhysRecLocksByFile2x
{
   nuint8  buTaskNum;
   nuint8  buLockType;
   nuint32 luRecStart;
   nuint32 luRecEnd;
} NWNCPPhysRecLocksByFile2x, N_FAR *pNWNCPPhysRecLocksByFile2x;

typedef struct tagNWNCPPhysRecLocksByFile3x
{
   nuint16 suTaskNum;
   nuint8  buLockType;
   nuint32 luRecStart;
   nuint32 luRecEnd;
} NWNCPPhysRecLocksByFile3x, N_FAR *pNWNCPPhysRecLocksByFile3x;

typedef struct tagNWNCPSemInfo2x
{
   nuint16 suLogConnNum;
   nuint8  buTaskNum;
} NWNCPSemInfo2x, N_FAR *pNWNCPSemInfo2x;

typedef struct tagNWNCPSemInfo3x
{
   nuint16 suLogConnNum;
   nuint16 suTaskNum;
} NWNCPSemInfo3x, N_FAR *pNWNCPSemInfo3x;

typedef struct tagNWNCPLogRecInfo
{
   nuint16 suTaskNum;
   nuint8  buLockStatus;
   nuint8  buLockNameLen;
   nstr8   abstrLockName[99];
} NWNCPLogRecInfo, N_FAR *pNWNCPLogRecInfo;

typedef struct tagNWNCPExtVolInfo
{
  nuint32 luVolType;
  nuint32 luStatusFlag;
  nuint32 luSectorSize;
  nuint32 luSectorsPerCluster;
  nuint32 luVolSizeInClusters;
  nuint32 luFreeClusters;
  nuint32 luSubAllocFreeableClusters;
  nuint32 luFreeableLimboSectors;
  nuint32 luNonfreeableLimboSectors;
  nuint32 luAvailSubAllocSectors;            /* non freeable */
  nuint32 luNonuseableSubAllocSectors;
  nuint32 luSubAllocClusters;
  nuint32 luNumDataStreams;
  nuint32 luNumLimboDataStreams;
  nuint32 luOldestDelFileAgeInTicks;
  nuint32 luNumCompDataStreams;
  nuint32 luNumCompLimboDataStreams;
  nuint32 luNumNoncompDataStreams;
  nuint32 luPrecompSectors;
  nuint32 luCompSectors;
  nuint32 luNumMigratedDataStreams;
  nuint32 luMigratedSectors;
  nuint32 luClustersUsedByFAT;
  nuint32 luClustersUsedByDirs;
  nuint32 luClustersUsedByExtDirs;
  nuint32 luTotalDirEntries;
  nuint32 luUnusedDirEntries;
  nuint32 luTotalExtDirExtants;
  nuint32 luUnusedExtDirExtants;
  nuint32 luExtAttrsDefined;
  nuint32 luExtAttrExtantsUsed;
  nuint32 luDirServicesObjID;
  nuint32 luVolLastModifiedDateAndTime;
} NWNCPExtVolInfo, N_FAR *pNWNCPExtVolInfo;

typedef struct tagNWNCPTaskStruct2x
{
   nuint8 buTaskNum;
   nuint8 buTaskState;
} NWNCPTaskStruct2x, N_FAR *pNWNCPTaskStruct2x;

typedef struct tagNWNCPWaitRec2x
{
   nuint8  buWaitingTaskNum;
   union
   {
      struct
      {
         nuint32 luRecStart;
         nuint32 luRecEnd;
         nuint8  buVolNum;
         nuint16 suDirID;
         nstr8   pbstrLockedFileName[14];
      } case1;
      struct
      {
         nuint8  buVolNum;
         nuint16 suDirID;
         nstr8   pbstrLockedFileName[14];
      } case2;
      struct
      {
         nstr8   pbstrLockedRecName[99];
      } case3;
      struct
      {
         nstr8   pbstrLockedSemName[127];
      } case4;
   } u;
} NWNCPWaitRec2x, N_FAR *pNWNCPWaitRec2x;

typedef struct tagNWNCPTaskStruct3x
{
   nuint16 suTaskNum;
   nuint8  buTaskState;
} NWNCPTaskStruct3x, N_FAR *pNWNCPTaskStruct3x;

typedef struct tagNWNCPWaitRec3x
{
   nuint16 suWaitingTaskNum;
   union
   {
      struct
      {
         nuint8  buReserved;
      } case0;
      struct
      {
         nuint32 luRecStart;
         nuint32 luRecEnd;
         nuint8  buVolNum;
         nuint32 luDirEntry;
         nuint8  buNamSpc;
         nuint8  buLockedFileNameLen;
         nstr8   pbstrLockedFileName[14];
      } case1;
      struct
      {
         nuint8  buVolNum;
         nuint32 luDirEntry;
         nuint8  buNamSpc;
         nuint8  buLockedFileNameLen;
         nstr8   pbstrLockedFileName[14];
      } case2;
      struct
      {
         nuint8  buLockedRecNameLen;
         nstr8   pbstrLockedRecName[99];
      } case3;
      struct
      {
         nuint8  buLockedSemNameLen;
         nstr8   pbstrLockedSemName[127];
      } case4;
   } u;
} NWNCPWaitRec3x, N_FAR *pNWNCPWaitRec3x;


#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else /* !N_PLAT_UNIX */
#include <npackoff.h>
#endif /* N_PLAT_UNIX */

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   Function prototypes
****************************************************************************/

N_EXTERN_LIBRARY( NWRCODE )
NWNCP15AllocResource
(
   pNWAccess pAccess,
   nuint8   buResNum
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s200CheckConsPriv
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s254ClearConn
(
   pNWAccess pAccess,
   nuint32  luConnNum
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s210ClearConn
(
   pNWAccess pAccess,
   nuint8   buConnNum
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP16FreeResource
(
   pNWAccess pAccess,
   nuint8   buResNum
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s203DisableServerLogin
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s211DownServer
(
   pNWAccess pAccess,
   nuint8   buForceFlag
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s204EnableServerLogin
(
   pNWAccess pAccess
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s219GetConnOpenFiles
(
   pNWAccess pAccess,
   nuint16  suConnNum,
   pnuint16 psuIterHnd,
   pnuint8  pbuNumRecs,
   pNWNCPConnOpenFiles2x pConnOpenFileInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s235GetConnOpenFiles
(
   pNWAccess pAccess,
   nuint16  suConnNum,
   pnuint16 psuIterHnd,
   pnuint16 psuNumRecs,
   pnuint8  pbuConnOpenFileData
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s225GetConnSems
(
   pNWAccess pAccess,
   nuint16  suConnNum,
   pnuint16 psuIterHnd,
   pnuint16 psuNumSems,
   pnuint8  pConnSemsInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s241GetConnSems
(
   pNWAccess pAccess,
   nuint16  suConnNum,
   pnuint16 psuIterHnd,
   pnuint16 psuNumSems,
   pnuint8  pConnSemsInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s218GetConnTaskInfo
(
   pNWAccess pAccess,
   nuint16  suConnNum,
   pnuint8  pbuLockStatus,
   pNWNCPWaitRec2x   pWaitRec,
   pnuint8  pbuTaskCount,
   pNWNCPTaskStruct2x pTasksB191
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s234GetConnTaskInfo
(
   pNWAccess pAccess,
   nuint16  suConnNum,
   pnuint8  pbuLockStatus,
   pNWNCPWaitRec3x    pWaitRec,
   pnuint8  pbuTaskCount,
   pNWNCPTaskStruct3x pTasksB127
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s229GetConnUsageStats
(
   pNWAccess pAccess,
   nuint16  suConnNum,
   pnuint32 pluSysIntervalMarker,
   pnuint8  pbuBytesReadB6,
   pnuint8  pbuBytesWrittenB6,
   pnuint32 pluTotalReqPackets
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s220GetConnsUsingAFile
(
   pNWAccess pAccess,
   pnuint16 psuIterHnd,
   nuint8   buDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint16 psuUseCnt,
   pnuint16 psuOpenCnt,
   pnuint16 psuOpenForRead,
   pnuint16 psuOpenForWrite,
   pnuint16 psuDenyReadCnt,
   pnuint16 psuDenyWriteCnt,
   pnuint8  pbuLocked,
   pnuint8  pbuNumRecs,
   pNWNCPConnUsingFile2x pConnsB70
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s236GetConnsUsingAFile
(
   pNWAccess pAccess,
   nuint8   buDataStream,
   nuint8   buVolNum,
   nuint32  luDirEntry,
   pnuint16 psuIterHnd,
   pnuint16 psuUseCnt,
   pnuint16 psuOpenCnt,
   pnuint16 psuOpenForRead,
   pnuint16 psuOpenForWrite,
   pnuint16 psuDenyReadCnt,
   pnuint16 psuDenyWriteCnt,
   pnuint8  pbuLocked,
   pnuint8  pbuForkCount,
   pnuint16 psuNumRecs,
   pNWNCPConnUsingFile3x pConnsB70
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s217GetDiskChannelStats
(
   pNWAccess pAccess,
   nuint8   buDiskChannelNum,
   pNWNCPDiskChannelStats pStats
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s14GetDiskUtilization
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luTrusteeID,
   pnuint8  pbuRepVolNum,
   pnuint32 pluRepTrusteeID,
   pnuint16 psuDirCnt,
   pnuint16 psuFileCnt,
   pnuint16 psuClusterCnt
);
N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s215GetDriveMapTable
(
   pNWAccess pAccess,
   pnuint32 pluSysIntervalMarker,
   pnuint8  pbuSFTSupportLevel,
   pnuint8  pbuLogicalDriveCnt,
   pnuint8  pbuPhysicalDriveCnt,
   pnuint8  pbuDiskChannelTableB5,
   pnuint16 psuPendingIOCommands,
   pnuint8  pbuDriveMapTableB32,
   pnuint8  pbuDriveMirrorTableB32,
   pnuint8  pbuDeadMirrorTableB32,
   pnuint8  pbuReMirrorDriveNum,
   pnuint8  pbuFiller,
   pnuint32 pluReMirrorCurrentOffset,
   pnuint16 psuSFTErrorTableB60
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP20GetServerDateAndTime
(
   pNWAccess pAccess,
   pnuint8  pbuYear,
   pnuint8  pbuMonth,
   pnuint8  pbuDay,
   pnuint8  pbuHour,
   pnuint8  pbuMinute,
   pnuint8  pbuSecond,
   pnuint8  pbuDayOfWeek
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s201GetServerDescStr
(
   pNWAccess pAccess,
   pnstr8   pbstrDescStringB512
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s17GetServerInfo
(
   pNWAccess pAccess,
   pNWNCPVersionInfo pVerInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s231GetServerLANIOStats
(
   pNWAccess pAccess,
   pNWNCPLANIOStats pStats
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s205GetServerLoginStatus
(
   pNWAccess pAccess,
   pnuint8  pbuUserLoginAllowed
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s232GetServerMiscInfo
(
   pNWAccess pAccess,
   pNWNCPMiscServerInfo pInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s212GetFileSysStats
(
   pNWAccess pAccess,
   pNWNCPFSStats pFileSysStats
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s227GetLANDrvConfigInfo
(
   pNWAccess pAccess,
   nuint8   buLANDriverNum,
   pNWNCPLANConfig pCfg
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s224GetLogicalRecInfo
(
   pNWAccess pAccess,
   pnuint16 psuIterHnd,
   nuint8   buLogicalRecNameLen,
   pnuint8  pbstrLogicalRecName,
   pnuint16 psuUseCnt,
   pnuint16 psuShareableLockCnt,
   pnuint8  pbuLocked,
   pnuint8  pbuNumRecs,
   pNWNCPLogicalRecInfo2x pLogRecInfoB128
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s240GetLogicalRecInfo
(
   pNWAccess pAccess,
   pnuint16 psuIterHnd,
   nuint8   buLogicalRecNameLen,
   pnuint8  pbstrLogicalRecName,
   pnuint16 psuUseCnt,
   pnuint16 psuShareableLockCnt,
   pnuint8  pbuLocked,
   pnuint16 psuNumRecs,
   pNWNCPLogicalRecInfo3x pLogRecInfoB102
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s223GetLogicalRecsByConn
(
   pNWAccess pAccess,
   nuint16  suTargetConnNum,
   pnuint16 psuIterHnd,
   pnuint8  pbuNumRecs,
   pnuint8  pbuLogRecsB508
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s239GetLogicalRecsByConn
(
   pNWAccess pAccess,
   nuint16  suTargetConnNum,
   pnuint16 psuIterHnd,
   pnuint16 psuNumRecs,
   pnuint8  pbuLogRecsB508
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s18GetNetSerialNum
(
   pNWAccess pAccess,
   pnuint32 pluServerSerialNum,
   pnuint16 psuAppNum
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s222GetPhyRecLocksFile
(
   pNWAccess pAccess,
   pnuint16 psuIterHnd,
   nuint8   buDirHandle,
   nuint8   buPathLen,
   pnstr8   pbstrFilePath,
   pnuint8  pbuNumOfLocks,
   pnuint8  pbuReserved,
   pNWNCPPhysRecLocks2x pPhysRecLocks
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s238GetPhyRecLocksFile
(
   pNWAccess pAccess,
   nuint8   buDataStream,
   nuint8   buVolNum,
   nuint32  luDirEntry,
   pnuint16 psuIterHnd,
   pnuint16 psuNumLocks,
   pNWNCPPhysRecLocks3x pPhysRecLocks
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s226GetSemInfo
(
   pNWAccess pAccess,
   pnuint16 psuIterHnd,
   nuint8   buSemNameLen,
   pnstr8   pbstrSemName,
   pnuint16 psuOpenCount,
   pnuint8  pbuSemValue,
   pnuint8  pbuNumRecs,
   pNWNCPSemInfo2x pSemInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s242GetSemInfo
(
   pNWAccess pAccess,
   pnuint16 psuIterHnd,
   nuint8   buSemNameLen,
   pnstr8   pbstrSemName,
   pnuint16 psuOpenCnt,
   pnuint16 psuSemValue,
   pnuint16 psuNumRecs,
   pNWNCPSemInfo3x pSemInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s233GetVolumeInfo
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   pnuint32 pluSystemIntervalMarker,
   pnuint8  pbuVolNum,
   pnuint8  pbuLogicalDriveNum,
   pnuint16 psuBlockSize,
   pnuint16 psuStartingBlock,
   pnuint16 psuTotalBlocks,
   pnuint16 psuFreeBlocks,
   pnuint16 psuTotalDirEntries,
   pnuint16 psuFreeDirEntries,
   pnuint16 psuActualMaxUsedDirEntries,
   pnuint8  pbuVolHashedFlag,
   pnuint8  pbuVolCachedFlag,
   pnuint8  pbuVolRemovableFlag,
   pnuint8  pbuVolMountedFlag,
   pnstr8   pbstrVolName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s214ReadDiskCacheStats
(
   pNWAccess pAccess,
   pNWNCPDskCacheStats pStats
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s216ReadPhyDiskStats
(
   pNWAccess       pAccess,
   nuint8         buPhysicalDiskNum,
   pNWNCPDiskStats pDiskStats
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s202SetServerDateTime
(
   pNWAccess pAccess,
   nuint8   buYear,
   nuint8   buMonth,
   nuint8   buDay,
   nuint8   buHour,
   nuint8   buMinute,
   nuint8   buSecond
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s252ReleaseAResource
(
   pNWAccess pAccess,
   nuint8   buResNum
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s12VerifySerialization
(
   pNWAccess pAccess,
   nuint32  luServerSerialNum,
   pnuint16 psuServerAppNum
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s221GtPhyRecLocksConFl
(
   pNWAccess pAccess,
   nuint16  suTargetConnNum,
   pnuint16 psuIterHnd,
   nuint8   buVolNum,
   nuint16  suDirID,
   pnstr8   pbstrFileName,
   pnuint8  pbuNumOfLocks,
   pnuint8  pbuReserved,
   pNWNCPPhysRecLocksByFile2x pPhysRecLocks
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP23s237GtPhyRecLocksConFl
(
   pNWAccess pAccess,
   nuint16  suTargetConnNum,
   nuint8   buDataStream,
   nuint8   buVolNum,
   nuint32  luDirEntry,
   pnuint16 psuIterHnd,
   pnuint16 psuNumOfLocks,
   pNWNCPPhysRecLocksByFile3x pPhysRecLocks
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
NWNCP22s51GetExtVolInfo
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   pnuint8  pbuReserved1B2,
   pNWNCPExtVolInfo pExtVolInfo,
   pnuint8  pbuReserved2B16
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
$Header: /SRCS/esmp/usr/src/nw/head/nw/ncpserve.h,v 1.8 1994/09/26 17:11:34 rebekah Exp $
*/
