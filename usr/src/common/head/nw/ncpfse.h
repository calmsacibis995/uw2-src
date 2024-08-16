/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/ncpfse.h	1.3"
/* FSE */
#if !defined NCPFSE_H
#define NCPFSE_H

#if !defined NTYPES_H
#ifdef N_PLAT_UNIX
#include <nw/ntypes.h>
#else /* !N_PLAT_UNIX */
#include "ntypes.h"
#endif /* N_PLAT_UNIX */
#endif

#if !defined NWACCESS_H
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

/*
   macro definitions
*/


/*
   this macro will insert code inline to unpack a NWNCPFSEVConsoleInfo
   struct at offset 0 in the byte array 'buf'
*/
#define NWNCP_UNPACK_VCONS_INF(pInf, buf) \
   if (pInf) { NCopyLoHi32(&(pInf)->luServerTime, &(buf)[0]); \
   (pInf)->buConsoleVer = (buf)[4]; \
   (pInf)->buConsoleRev = (buf)[5]; }

/*
   definitions
*/
#define NWNCP_FSE_MAX_STRING_LEN    128    /* Default string lengths */
#define NWNCP_FSE_MAX_OBJECTS       128    /* Default list size */

/* NWNCP123s4GetUserInfo connection types*/
#define NWNCP_FSE_CONN_TYPE_CLIB   1
#define NWNCP_FSE_CONN_TYPE_NCP    2
#define NWNCP_FSE_CONN_TYPE_NLM    3
#define NWNCP_FSE_CONN_TYPE_AFP    4
#define NWNCP_FSE_CONN_TYPE_FTAM   5
#define NWNCP_FSE_CONN_TYPE_ANCP   6

/* NWNCP123s4GetUserInfo connection status */
#define NWNCP_FSE_STAT_LOGGED_IN          0x00000001
#define NWNCP_FSE_STAT_BEING_ABORTED      0x00000002
#define NWNCP_FSE_STAT_AUDITED            0x00000004
#define NWNCP_FSE_STAT_NEEDS_SECURITY     0x00000008
#define NWNCP_FSE_STAT_MAC_STATION        0x00000010
#define NWNCP_FSE_STAT_AUTHENTICATED      0x00000020
#define NWNCP_FSE_STAT_AUDIT_RECORDED     0x00000040
#define NWNCP_FSE_STAT_DSAUDIT_RECORDED   0x00000080

/* NWNCP123s4GetUserInfo write flags */
#define NWNCP_FSE_WFLAG_WRITING         1
#define NWNCP_FSE_WFLAG_ABORTED         2

/* NWNCP123s4GetUserInfo write status */
#define NWNCP_FSE_WSTATE_NOT_WRITING    0
#define NWNCP_FSE_WSTATE_WRITING        1
#define NWNCP_FSE_WSTATE_STOPPING_WRITE 2

/* NWNCP123s8GetCPUInfo bus types */
#define NWNCP_FSE_BUS_TYPE_ISA          0
#define NWNCP_FSE_BUS_TYPE_MICROCHANN   1
#define NWNCP_FSE_BUS_TYPE_EISA         2

/* NWNCP123s11GetNLMInfo Flags */
#define NWNCP_FSE_NLM_FLAG_REENTRANT        0x00000001
#define NWNCP_FSE_NLM_FLAG_MULTIPLE         0x00000002
#define NWNCP_FSE_NLM_FLAG_SYNCHRONIZE      0x00000004
#define NWNCP_FSE_NLM_FLAG_PSUEDOPREEM      0x00000008

/* NWNCP123s11GetNLMInfo Types */
#define NWNCP_FSE_NLM_TYPE_LAN           ((nuint32) 1)
#define NWNCP_FSE_NLM_TYPE_DSK           ((nuint32) 2)
#define NWNCP_FSE_NLM_TYPE_NAM           ((nuint32) 3)
#define NWNCP_FSE_NLM_TYPE_NLM           ((nuint32) 4)
#define NWNCP_FSE_NLM_TYPE_MSL           ((nuint32) 5)
#define NWNCP_FSE_NLM_TYPE_NLM_OS        ((nuint32) 6)
#define NWNCP_FSE_NLM_TYPE_NLM_OS_HIGH   ((nuint32) 7)
#define NWNCP_FSE_NLM_TYPE_HAM           ((nuint32) 8)
#define NWNCP_FSE_NLM_TYPE_CDM           ((nuint32) 9)
#define NWNCP_FSE_NLM_TYPE_FSE           ((nuint32) 10)
#define NWNCP_FSE_NLM_TYPE_NLM_REAL      ((nuint32) 11)
#define NWNCP_FSE_NLM_TYPE_NLM_HIDDEN    ((nuint32) 12)

/* NWNCP123s21GetLANConfigInfo driver config Mode Flags */
#define NWNCP_FSE_ADDR_PHYSICAL        0x0000
#define NWNCP_FSE_ADDR_ILLEGAL         0x4000
#define NWNCP_FSE_ADDR_CANONICAL       0x8000
#define NWNCP_FSE_ADDR_NONCANONICAL    0xC000

/* NWNCP123s21GetLANConfigInfo driver config Driver Flags */
#define NWNCP_FSE_NATIVE               0x0000
#define NWNCP_FSE_ILLEGAL              0x0001
#define NWNCP_FSE_GROUP_WITH_FILTER    0x0002
#define NWNCP_FSE_GROUP_WITHOUT_FILTER 0x0003

/* NWNCP123s30GetMediaObjectInfo object types and types */
#define NWNCP_FSE_OBJECT_ADAPTER          ((nuint32) 0)
#define NWNCP_FSE_OBJECT_CHANGER          ((nuint32) 1)
#define NWNCP_FSE_OBJECT_DEVICE           ((nuint32) 2)
#define NWNCP_FSE_OBJECT_MEDIA            ((nuint32) 4)
#define NWNCP_FSE_OBJECT_PARTITION        ((nuint32) 5)
#define NWNCP_FSE_OBJECT_SLOT             ((nuint32) 6)
#define NWNCP_FSE_OBJECT_HOTFIX           ((nuint32) 7)
#define NWNCP_FSE_OBJECT_MIRROR           ((nuint32) 8)
#define NWNCP_FSE_OBJECT_PARITY           ((nuint32) 9)
#define NWNCP_FSE_OBJECT_VOLUME_SEG       ((nuint32) 10)
#define NWNCP_FSE_OBJECT_VOLUME           ((nuint32) 11)
#define NWNCP_FSE_OBJECT_CLONE            ((nuint32) 12)
#define NWNCP_FSE_OBJECT_MAGAZINE         ((nuint32) 14)
#define NWNCP_FSE_OBJECT_VIRTUAL_DEVICE   ((nuint32) 15)
#define NWNCP_FSE_OBJECT_UNKNOWN          ((nuint32)-1L)

/* NWNCP123s30GetMediaObjectInfo media types */
#define NWNCP_FSE_MEDIA_HARD_DISK         ((nuint32) 0)
#define NWNCP_FSE_MEDIA_CDROM_DISK        ((nuint32) 1)
#define NWNCP_FSE_MEDIA_WORM_DISK         ((nuint32) 2)
#define NWNCP_FSE_MEDIA_TAPE_DEVICE       ((nuint32) 3)
#define NWNCP_FSE_MEDIA_MAG_OPTICAL       ((nuint32) 4)

/* NWNCP123s30GetMediaObjectInfo cartridge types */
#define NWNCP_FSE_CART_FIXED              ((nuint32) 0)
#define NWNCP_FSE_CART_FLOPPY_5_25        ((nuint32) 1)
#define NWNCP_FSE_CART_FLOPPY_3_5         ((nuint32) 2)
#define NWNCP_FSE_CART_OPTICAL_5_25       ((nuint32) 3)
#define NWNCP_FSE_CART_OPTICAL_3_5        ((nuint32) 4)
#define NWNCP_FSE_CART_TAPE_0_5           ((nuint32) 5)
#define NWNCP_FSE_CART_TAPE_0_25          ((nuint32) 6)
#define NWNCP_FSE_CART_TAPE_8MM           ((nuint32) 7)
#define NWNCP_FSE_CART_TAPE_4MM           ((nuint32) 8)
#define NWNCP_FSE_CART_BERNOULLI          ((nuint32) 9)

/* NWNCP123s30GetMediaObjectInfo status bits */
#define NWNCP_FSE_OBJECT_ACTIVATED        (0x00000001)
#define NWNCP_FSE_OBJECT_CREATED          (0x00000002)
#define NWNCP_FSE_OBJECT_SCRAMBLED        (0x00000004)
#define NWNCP_FSE_OBJECT_RESERVED         (0x00000010)
#define NWNCP_FSE_OBJECT_BEING_IDENT      (0x00000020)
#define NWNCP_FSE_OBJECT_MAGAZ_LOADED     (0x00000040)
#define NWNCP_FSE_OBJECT_FAILURE          (0x00000080)
#define NWNCP_FSE_OBJECT_REMOVABLE        (0x00000100)
#define NWNCP_FSE_OBJECT_READ_ONLY        (0x00000200)
#define NWNCP_FSE_OBJECT_IN_DEVICE        (0x00010000)
#define NWNCP_FSE_OBJECT_ACCEPTS_MAG      (0x00020000)
#define NWNCP_FSE_OBJECT_IN_A_CHANGER     (0x00040000)
#define NWNCP_FSE_OBJECT_LOADABLE         (0x00080000)
#define NWNCP_FSE_OBJECT_BEING_LOADED     (0x00080000)
#define NWNCP_FSE_OBJECT_DEVICE_LOCK      (0x01000000)
#define NWNCP_FSE_OBJECT_CHANGER_LOCK     (0x02000000)
#define NWNCP_FSE_OBJECT_REMIRRORING      (0x04000000)
#define NWNCP_FSE_OBJECT_SELECTED         (0x08000000)

/* NWNCP123s30GetMediaObjectInfo function mask */
#define NWNCP_FSE_FUNC_RAND_READ          (0x00000001)
#define NWNCP_FSE_FUNC_RAND_WRITE         (0x00000002)
#define NWNCP_FSE_FUNC_RAND_WRITE_ONCE    (0x00000004)
#define NWNCP_FSE_FUNC_SEQ_READ           (0x00000008)
#define NWNCP_FSE_FUNC_SEQ_WRITE          (0x00000010)
#define NWNCP_FSE_FUNC_RESET_EOT          (0x00000020)
#define NWNCP_FSE_FUNC_SINGL_FILE_MARK    (0x00000040)
#define NWNCP_FSE_FUNC_MULTI_FILE_MARK    (0x00000080)
#define NWNCP_FSE_FUNC_SINGL_SET_MARK     (0x00000100)
#define NWNCP_FSE_FUNC_MULTI_SET_MARK     (0x00000200)
#define NWNCP_FSE_FUNC_SPACE_DATA         (0x00000400)
#define NWNCP_FSE_FUNC_LOCATE_DATA        (0x00000800)
#define NWNCP_FSE_FUNC_POSITION_PART      (0x00001000)
#define NWNCP_FSE_FUNC_POSITION_MEDIA     (0x00002000)

/* NWNCP123s30GetMediaObjectInfo control mask */
#define NWNCP_FSE_CTRL_ACTIVE             (0x00000001)
#define NWNCP_FSE_CTRL_MOUNT              (0x00000002)
#define NWNCP_FSE_CTRL_SELECT             (0x00000003)
#define NWNCP_FSE_CTRL_LOCK               (0x00000008)
#define NWNCP_FSE_CTRL_EJECT              (0x00000010)
#define NWNCP_FSE_CTRL_MOVE               (0x00000020)

/* NWNCP123s44GetProtocolNumByMedia media types */
#define NWNCP_FSE_MEDIA_VIRTUAL        0
#define NWNCP_FSE_MEDIA_APPLETALK      1
#define NWNCP_FSE_MEDIA_ETHERNET_II    2
#define NWNCP_FSE_MEDIA_ETHERNET_802_2 3
#define NWNCP_FSE_MEDIA_TOKEN_RING     4
#define NWNCP_FSE_MEDIA_ETHERNET_802_3 5
#define NWNCP_FSE_MEDIA_ETHERNET_802_4 6
#define NWNCP_FSE_MEDIA_PCN2           7
#define NWNCP_FSE_MEDIA_GNET           8
#define NWNCP_FSE_MEDIA_PRONET         9
#define NWNCP_FSE_MEDIA_ETHERNET_SNAP  10
#define NWNCP_FSE_MEDIA_TOKEN_SNAP     11
#define NWNCP_FSE_MEDIA_LANPAC_II      12
#define NWNCP_FSE_MEDIA_ISDN           13
#define NWNCP_FSE_MEDIA_ARCNET         14
#define NWNCP_FSE_MEDIA_PCN2_802_2     15
#define NWNCP_FSE_MEDIA_PCN2_SNAP      16
#define NWNCP_FSE_MEDIA_OMNINET4       17
#define NWNCP_FSE_MEDIA_3270_COAXA     18
#define NWNCP_FSE_MEDIA_IP             19
#define NWNCP_FSE_MEDIA_FDDI_802_2     20
#define NWNCP_FSE_MEDIA_IVDLAN_802_9   21
#define NWNCP_FSE_MEDIA_DATACO_OSI     22
#define NWNCP_FSE_MEDIA_FDDI_SNAP      23
#define NWNCP_FSE_MEDIA_IBM_SDLC       24
#define NWNCP_FSE_MEDIA_PCO_FDDITP     25
#define NWNCP_FSE_MEDIA_WAIDNET        26
#define NWNCP_FSE_MEDIA_SLIP           27
#define NWNCP_FSE_MEDIA_PPP            27

/* NWNCP123s53GetKnownNetworks network status */
#define NWNCP_FSE_NET_STAT_LOCAL      0x0001
#define NWNCP_FSE_NET_STAT_STAR       0x0002
#define NWNCP_FSE_NET_STAT_RELIABLE   0x0004
#define NWNCP_FSE_NET_STAT_WAN        0x0010


/* NWNCP123s60GetServerSetInfo command types */
#define NWNCP_FSE_SET_TYPE_NUMBER         0
#define NWNCP_FSE_SET_TYPE_BOOLEAN        1
#define NWNCP_FSE_SET_TYPE_TICKS          2
#define NWNCP_FSE_SET_TYPE_BLOCK_SHIFT    3
#define NWNCP_FSE_SET_TYPE_TIME_OFFSET    4
#define NWNCP_FSE_SET_TYPE_STRING         5
#define NWNCP_FSE_SET_TYPE_TRIGGER        6

/* NWNCP123s60GetServerSetInfo trigger types */
#define NWNCP_FSE_SET_TRIGGER_OFF         0x00
#define NWNCP_FSE_SET_TRIGGER_ON          0x01
#define NWNCP_FSE_SET_TRIGGER_PENDING     0x10
#define NWNCP_FSE_SET_TRIGGER_SUCCESS     0x20
#define NWNCP_FSE_SET_TRIGGER_FAILED      0x30

/* NWNCP123s60GetServerSetInfo category types */
#define NWNCP_FSE_CAT_COMMUNICATIONS      0
#define NWNCP_FSE_CAT_MEMORY              1
#define NWNCP_FSE_CAT_FILE_CACHE          2
#define NWNCP_FSE_CAT_DIR_CACHE           3
#define NWNCP_FSE_CAT_FILE_SYSTEM         4
#define NWNCP_FSE_CAT_LOCKS               5
#define NWNCP_FSE_CAT_TRANS_TRACKING      6
#define NWNCP_FSE_CAT_DISK                7
#define NWNCP_FSE_CAT_TIME                8
#define NWNCP_FSE_CAT_NCP                 9
#define NWNCP_FSE_CAT_MISC                10
#define NWNCP_FSE_CAT_ERRORS              11

/* NWNCP123s60GetServerSetInfo command flags */
#define NWNCP_FSE_SET_STARTUP          0x01
#define NWNCP_FSE_SET_HIDE             0x02
#define NWNCP_FSE_SET_ADVANCED         0x04
#define NWNCP_FSE_SET_LATER            0x08
#define NWNCP_FSE_SET_SECURED_CONSOLE  0x10


/*
   structures
*/
/* global response data for all calls */
typedef struct tagNWNCPFSEVConsoleInfo
{
   nuint32                  luServerTime;
   nuint8                   buConsoleVer;
   nuint8                   buConsoleRev;
} NWNCPFSEVConsoleInfo, N_FAR *pNWNCPFSEVConsoleInfo;


/* NWNCP123s1GetCacheInfo call specific structures */
typedef struct tagNWNCPFSECacheCounters
{
   nuint32                  luReadExistingBlockCnt;
   nuint32                  luReadExistingWriteWaitCnt;
   nuint32                  luReadExistingPartialReadCnt;
   nuint32                  luReadExistingReadErrorCnt;
   nuint32                  luWriteBlockCnt;
   nuint32                  luWriteEntireBlockCnt;
   nuint32                  luIntDiskGetCnt;
   nuint32                  luIntDiskGetNeedToAllocCnt;
   nuint32                  luIntDiskGetSomeoneBeatMeCnt;
   nuint32                  luIntDiskGetPartialReadCnt;
   nuint32                  luIntDiskGetReadErrorCnt;
   nuint32                  luAsyncIntDiskGetCnt;
   nuint32                  luAsyncIntDiskGetNeedToAlloc;
   nuint32                  luAsyncIntDiskGetSomeoneBeatMe;
   nuint32                  luErrDoingAsyncReadCnt;
   nuint32                  luIntDiskGetNoReadCnt;
   nuint32                  luIntDiskGetNoReadAllocCnt;
   nuint32                  luIntDiskGetNoReadSomeoneBeat;
   nuint32                  luIntDiskWriteCnt;
   nuint32                  luIntDiskWriteAllocCnt;
   nuint32                  luIntDiskWriteSomeoneBeatMe;
   nuint32                  luWriteErrorCnt;
   nuint32                  luWaitOnSemaphoreCnt;
   nuint32                  luAllocBlockHadToWaitForBeat;
   nuint32                  luAllocBlockCnt;
   nuint32                  luAllocBlockIHadToWaitCnt;
} NWNCPFSECacheCounters, N_FAR *pNWNCPFSECacheCounters;

typedef struct tagNWNCPFSEMemoryCounters
{
   nuint32                  luOriginalNumCacheBuffs;
   nuint32                  luCurNumCacheBuffs;
   nuint32                  luCacheDirtyBlockThreshold;
   nuint32                  luWaitNodeCnt;
   nuint32                  luWaitNodeAllocFailureCnt;
   nuint32                  luMoveCacheNodeCnt;
   nuint32                  luMoveCacheNodeFromAvailCnt;
   nuint32                  luAccelerateCacheNodeWriteCnt;
   nuint32                  luRemoveCacheNodeCnt;
   nuint32                  luRemoveCacheNodeFromAvailCnt;
} NWNCPFSEMemoryCounters, N_FAR *pNWNCPFSEMemoryCounters;

typedef struct tagNWNCPFSETrendCounters
{
   nuint32                  luNumCacheChecks;
   nuint32                  luNumCacheHits;
   nuint32                  luNumCacheDirtyChecks;
   nuint32                  luNumCacheDirtyHits;
   nuint32                  luCacheUsedWhileChecking;
   nuint32                  luWaitTillDirtyBlksDecreaseCnt;
   nuint32                  luAllocBlockFromAvailableCnt;
   nuint32                  luAllocBlockFromLRUCnt;
   nuint32                  luAllocBlockAlreadyWaiting;
   nuint32                  luLRUSittingTime;
} NWNCPFSETrendCounters, N_FAR *pNWNCPFSETrendCounters;

typedef struct tagNWNCPFSECacheInfo
{
   nuint32                  luMaxByteCnt;
   nuint32                  luMinNumCacheBuffs;
   nuint32                  luMinCacheReportThreshold;
   nuint32                  luAllocWaitingCnt;
   nuint32                  luNDirtyBlocks;
   nuint32                  luCacheDirtyWaitTime;
   nuint32                  luCacheMaxConcurrentWrites;
   nuint32                  luMaxDirtyTime;
   nuint32                  luNumDirCacheBuffs;
   nuint32                  luCacheByteToBlockShiftFactor;
} NWNCPFSECacheInfo, N_FAR *pNWNCPFSECacheInfo;


/* NWNCP123s2GetServerInfo call specific structures */
typedef struct tagNWNCPFSEServerInfo
{
   nuint32                  luReplyCancledCnt;
   nuint32                  luWriteHeldOffCnt;
   nuint32                  luWriteHeldOffWithDupRequest;
   nuint32                  luInvalidRequestTypeCnt;
   nuint32                  luBeingAbortedCnt;
   nuint32                  luAlreadyDoingReAllocCnt;
   nuint32                  luDeallocInvalidSlotCnt;
   nuint32                  luDeallocBeingProcessedCnt;
   nuint32                  luDeallocForgedPacketCnt;
   nuint32                  luDeallocStillTransmittingCnt;
   nuint32                  luStartStationErrorCnt;
   nuint32                  luInvalidSlotCnt;
   nuint32                  luBeingProcessedCnt;
   nuint32                  luForgedPacketCnt;
   nuint32                  luStillTransmittingCnt;
   nuint32                  luReExecuteRequestCnt;
   nuint32                  luInvalidSequenceNumCnt;
   nuint32                  luDupIsBeingSentAlreadyCnt;
   nuint32                  luSentPositiveAcknowledgeCnt;
   nuint32                  luSentADupReplyCnt;
   nuint32                  luNoMemoryForStationControlCnt;
   nuint32                  luNoAvailableConnsCnt;
   nuint32                  luReAllocSlotCnt;
   nuint32                  luReAllocSlotCameTooSoonCnt;
} NWNCPFSEServerInfo, N_FAR *pNWNCPFSEServerInfo;

typedef struct tagNWNCPFSEServerCnts
{
   nuint16                  suTooManyHops;
   nuint16                  suUnknownNetwork;
   nuint16                  suNoSpaceForService;
   nuint16                  suNoReceiveBuffers;
   nuint16                  suNotMyNetwork;
   nuint32                  luNetBIOSProgatedCnt;
   nuint32                  luTotalPacketsServiced;
   nuint32                  luTotalPacketsRouted;
} NWNCPFSEServerCnts, N_FAR *pNWNCPFSEServerCnts;


/* NWNCP123s3GetFileSystemsInfo call specific structures */
typedef struct tagNWNCPFSEFileSystemInfo
{
   nuint32                  luFATMovedCnt;
   nuint32                  luFATWriteErrorCnt;
   nuint32                  luSomeoneElseDidItCnt0;
   nuint32                  luSomeoneElseDidItCnt1;
   nuint32                  luSomeoneElseDidItCnt2;
   nuint32                  luIRanOutSomeoneElseDidItCnt0;
   nuint32                  luIRanOutSomeoneElseDidItCnt1;
   nuint32                  luIRanOutSomeoneElseDidItCnt2;
   nuint32                  luTurboFATBuildScrewedUpCnt;
   nuint32                  luExtraUseCountNodeCnt;
   nuint32                  luExtraExtraUseCountNodeCnt;
   nuint32                  luErrReadingLastFATCnt;
   nuint32                  luSomeoneElseUsingThisFileCnt;
} NWNCPFSEFileSystemInfo, N_FAR *pNWNCPFSEFileSystemInfo;

/* NWNCP123s4GetUserInfo call specific structures */
typedef struct tagNWNCPFSEUserInfo
{
   nuint32                  luConnNum;
   nuint32                  luUseCnt;
   nuint8                   buConnServiceType;
   nuint8                   abuLoginTime[7];
   nuint32                  luStatus;
   nuint32                  luExpirationTime;
   nuint32                  luObjType;
   nuint8                   buTransationFlag;
   nuint8                   buLogLockThreshold;
   nuint8                   buRecLockThreshold;
   nuint8                   buFileWriteFlags;
   nuint8                   buFileWriteState;
   nuint8                   buFiller;
   nuint16                  suFileLockCnt;
   nuint16                  suRecordLockCnt;
   nuint8                   abuTotalBytesRead[6];    /* 48 bits */
   nuint8                   abuTotalBytesWritten[6]; /* 48 bits */
   nuint32                  luTotalRequests;
   nuint32                  luHeldRequests;
   nuint8                   abuHeldBytesRead[6];     /* 48 bits */
   nuint8                   abuHeldBytesWritten[6];  /* 48 bits */
} NWNCPFSEUserInfo, N_FAR *pNWNCPFSEUserInfo;


/* NWNCP123s5GetPacketBurstInfo call specific structure */
typedef struct tagNWNCPFSEBurstInfo
{
   nuint32                  luBigInvalidSlotCnt;
   nuint32                  luBigForgedPacketCnt;
   nuint32                  luBigInvalidPacketCnt;
   nuint32                  luBigStillTransmittingCnt;
   nuint32                  luStillDoingTheLastRequestCnt;
   nuint32                  luInvalidControlRequestCnt;
   nuint32                  luControlInvalidMsgNumCnt;
   nuint32                  luControlBeingTornDownCnt;
   nuint32                  luBigRepeatTheFileReadCnt;
   nuint32                  luBigSendExtraCCCnt;
   nuint32                  luBigReturnAbortMsgCnt;
   nuint32                  luBigReadInvalidMsgNumCnt;
   nuint32                  luBigReadDoItOverCnt;
   nuint32                  luBigReadBeingTornDownCnt;
   nuint32                  luPreviousControlPacketCnt;
   nuint32                  luSendHoldOffMsgCnt;
   nuint32                  luBigReadNoDataAvailableCnt;
   nuint32                  luBigReadTryToReadTooMuchCnt;
   nuint32                  luAsyncReadErrorCnt;
   nuint32                  luBigReadPhysicalReadErrorCnt;
   nuint32                  luControlBadACKFragListCnt;
   nuint32                  luControlNoDataReadCnt;
   nuint32                  luWriteDupRequestCnt;
   nuint32                  luShouldntBeACKingHereCnt;
   nuint32                  luWriteInconsistentPktLenCnt;
   nuint32                  luFirstPacketIsntAWriteCnt;
   nuint32                  luWriteTrashedDupRequestCnt;
   nuint32                  luBigWriteInvalidMsgNumCnt;
   nuint32                  luBigWriteBeingTornDownCnt;
   nuint32                  luBigWriteBeingAbortedCnt;
   nuint32                  luZeroACKFragCnt;
   nuint32                  luWriteCurrentlyTransCnt;
   nuint32                  luTryingToWriteTooMuchCnt;
   nuint32                  luWriteOutOfMemForCtrlNodesCnt;
   nuint32                  luWriteDidntNeedThisFragCnt;
   nuint32                  luWriteTooManyBufsChkedOutCnt;
   nuint32                  luWriteTimeOutCnt;
   nuint32                  luWriteGotAnACKCnt0;
   nuint32                  luWriteGotAnACKCnt1;
   nuint32                  luPollerAbortedConnCnt;
   nuint32                  luMaybeHadOutOfOrderWritesCnt;
   nuint32                  luHadAnOutOfOrderWriteCnt;
   nuint32                  luMovedTheACKBitDownCnt;
   nuint32                  luBumpedOutOfOrderWriteCnt;
   nuint32                  luPollerRemovedOutOfOrderCnt;
   nuint32                  luWriteRequestedUnneededACKCnt;
   nuint32                  luWriteTrashedPacketCnt;
   nuint32                  luTooManyACKFragCnt;
   nuint32                  luSavedAnOutOfOrderPacketCnt;
   nuint32                  luConnBeingAbortedCnt;
} NWNCPFSEBurstInfo, N_FAR *pNWNCPFSEBurstInfo;


/* NWNCP123s6GetIPXSPXInfo call specific structures */
typedef struct tagNWNCPFSEIPXInfo
{
   nuint32                  luIPXSendPacketCnt;
   nuint16                  suIPXMalformPacketCnt;
   nuint32                  luIPXGetEcbRequestCnt;
   nuint32                  luIPXGetEcbFailCnt;
   nuint32                  luIPXAesEventCnt;
   nuint16                  suIPXPostponedAesCnt;
   nuint16                  suIPXMaxConfiguredSocketCnt;
   nuint16                  suIPXMaxOpenSocketCnt;
   nuint16                  suIPXOpenSocketFailCnt;
   nuint32                  luIPXListenEcbCnt;
   nuint16                  suIPXEcbCancelFailCnt;
   nuint16                  suIPXGetLocalTargetFailCnt;
} NWNCPFSEIPXInfo, N_FAR *pNWNCPFSEIPXInfo;

typedef struct tagNWNCPFSESPXInfo
{
   nuint16                  suSPXMaxConnsCnt;
   nuint16                  suSPXMaxUsedConns;
   nuint16                  suSPXEstConnReq;
   nuint16                  suSPXEstConnFail;
   nuint16                  suSPXListenConnectReq;
   nuint16                  suSPXListenConnectFail;
   nuint32                  luSPXSendCnt;
   nuint32                  luSPXWindowChokeCnt;
   nuint16                  suSPXBadSendCnt;
   nuint16                  suSPXSendFailCnt;
   nuint16                  suSPXAbortedConn;
   nuint32                  luSPXListenPacketCnt;
   nuint16                  suSPXBadListenCnt;
   nuint32                  luSPXIncomingPacketCnt;
   nuint16                  suSPXBadInPacketCnt;
   nuint16                  suSPXSuppressedPackCnt;
   nuint16                  suSPXNoSesListenEcbCnt;
   nuint16                  suSPXWatchDogDestSesCnt;
} NWNCPFSESPXInfo, N_FAR *pNWNCPFSESPXInfo;


/* NWNCP123s8GetCPUInfo specific structure */
typedef struct tagNWNCPFSECPUInfo
{
   nuint32                  luWeOwnThePageTablesFlag;
   nuint32                  luCPUTypeFlag;
   nuint32                  luNumericCoprocessorFlag;
   nuint32                  luBusTypeFlag;
   nuint32                  luIOEngineFlag;
   nuint32                  luFSEngineFlag;
   nuint32                  luNonDedicatedFlag;
} NWNCPFSECPUInfo, N_FAR *pNWNCPFSECPUInfo;


/* NWNCP123s9GetVolSwitchInfo specific structure */
typedef struct tagNWNCPFSESwitchInfo
{
   nuint32                  luReadFile;
   nuint32                  luWriteFile;
   nuint32                  luDelFile;
   nuint32                  luRenMoveFile;
   nuint32                  luOpenFile;
   nuint32                  luCreateFile;
   nuint32                  luCreateAndOpenFile;
   nuint32                  luCloseFile;
   nuint32                  luScanDelFile;
   nuint32                  luSalvageFile;
   nuint32                  luPurgeFile;
   nuint32                  luMigrateFile;
   nuint32                  luDeMigrateFile;
   nuint32                  luCreateDir;
   nuint32                  luDelDir;
   nuint32                  luDirScans;
   nuint32                  luMapPathToDirNum;
   nuint32                  luModifyDirEntry;
   nuint32                  luGetAccessRights;
   nuint32                  luGetAccessRightsFromIDs;
   nuint32                  luMapDirNumToPath;
   nuint32                  luGetEntryFromPathStrBase;
   nuint32                  luGetOtherNSEntry;
   nuint32                  luGetExtDirInfo;
   nuint32                  luGetParentDirNum;
   nuint32                  luAddTrusteeR;
   nuint32                  luScanTrusteeR;
   nuint32                  luDelTrusteeR;
   nuint32                  luPurgeTrust;
   nuint32                  luFindNextTrustRef;
   nuint32                  luScanUserRestNodes;
   nuint32                  luAddUserRest;
   nuint32                  luDelUserRest;
   nuint32                  luRtnDirSpaceRest;
   nuint32                  luGetActualAvailDskSp;
   nuint32                  luCntOwnedFilesAndDirs;
   nuint32                  luMigFileInfo;
   nuint32                  luVolMigInfo;
   nuint32                  luReadMigFileData;
   nuint32                  luGetVolUsageStats;
   nuint32                  luGetActualVolUsageStats;
   nuint32                  luGetDirUsageStats;
   nuint32                  luNMFileReadsCnt;
   nuint32                  luNMFileWritesCnt;
   nuint32                  luMapPathToDirNumOrPhant;
   nuint32                  luStationHasRightsGranted;
   nuint32                  luGetDataStrmLensFromPathStrBs;
   nuint32                  luCheckAndGetDirEntry;
   nuint32                  luGetDelEntry;
   nuint32                  luGetOrigNameSpace;
   nuint32                  luGetActualFileSize;
   nuint32                  luVerifyNSNum;
   nuint32                  luVerifyDataStrmNum;
   nuint32                  luCheckVolNum;
   nuint32                  luCommitFile;
   nuint32                  luVMGetDirEntry;
   nuint32                  luCreateDMFileEntry;
   nuint32                  luRenameNSEntry;
   nuint32                  luLogFile;
   nuint32                  luReleaseFile;
   nuint32                  luClearFile;
   nuint32                  luSetVolFlag;
   nuint32                  luClearVolFlag;
   nuint32                  luGetOrigInfo;
   nuint32                  luCreateMigratedDir;
   nuint32                  luF3OpenCreate;
   nuint32                  luF3InitFileSearch;
   nuint32                  luF3ContFileSearch;
   nuint32                  luF3RenFile;
   nuint32                  luF3ScanForTrustees;
   nuint32                  luF3ObtainFileInfo;
   nuint32                  luF3ModifyInfo;
   nuint32                  luF3EraseFile;
   nuint32                  luF3SetDirHandle;
   nuint32                  luF3AddTrustees;
   nuint32                  luF3DelTrustees;
   nuint32                  luF3AllocDirHandle;
   nuint32                  luF3ScanSalvagedFiles;
   nuint32                  luF3RecoverSalvagedFiles;
   nuint32                  luF3PurgeSalvageableFile;
   nuint32                  luF3GetNSSpecInfo;
   nuint32                  luF3ModifyNSSpecInfo;
   nuint32                  luF3SearchSet;
   nuint32                  luF3GetDirBase;
   nuint32                  luF3QueryNSInfo;
   nuint32                  luF3GetNSList;
   nuint32                  luF3GetHugeInfo;
   nuint32                  luF3SetHugeInfo;
   nuint32                  luF3GetFullPathStr;
   nuint32                  luF3GetEffectDirRights;
   nuint32                  luParseTree;
   nuint32                  luGetRefCntFromEntry;
   nuint32                  luAllocExtDir;
   nuint32                  luReadExtDir;
   nuint32                  luWriteExtDir;
   nuint32                  luCommitExtDir;
   nuint32                  luClaimExtDir;
   nuint32                  luReturnExtDir;
   nuint32                  luSetOwningNS;
   nuint32                  luRemoveFile;
   nuint32                  luRemoveFileCompletely;
   nuint32                  luGetNSInfo;
   nuint32                  luClearPhantom;
   nuint32                  luGetMaxUserRestric;
   nuint32                  luGetCurrDiskUsedAmt;
   nuint32                  luFlushVol;
   nuint32                  luSetCompressedFileSize;
} NWNCPFSESwitchInfo, N_FAR *pNWNCPFSESwitchInfo;


/* NWNCP123s11GetNLMInfo specific structure */
typedef struct tagNWNCPFSENLMInfo
{
   nuint32                 luIdNum;
   nuint32                 luFlags;
   nuint32                 luType;
   nuint32                 luParentID;
   nuint32                 luMajorVer;
   nuint32                 luMinorVer;
   nuint32                 luRevision;
   nuint32                 luYear;
   nuint32                 luMonth;
   nuint32                 luDay;
   nuint32                 luAllocAvailBytes;
   nuint32                 luAllocFreeCnt;
   nuint32                 luLastGarbCollect;
   nuint32                 luMsgLanguage;
   nuint32                 luNumPublics;
} NWNCPFSENLMInfo, N_FAR *pNWNCPFSENLMInfo;


/* NWNCP123s12GetDirCacheInfo specific structure */
typedef struct tagNWNCPFSEDirCacheInfo
{
   nuint32                 luMinTimeSinceFileDelete;
   nuint32                 luAbsMinTimeSinceFileDelete;
   nuint32                 luMinNumDirCacheBuffs;
   nuint32                 luMaxNumDirCacheBuffs;
   nuint32                 luNumDirCacheBuffs;
   nuint32                 luMinNonReferencedTime;
   nuint32                 luWaitTimeBeforeNewBuff;
   nuint32                 luMaxConcurrentWrites;
   nuint32                 luDirtyWaitTime;
   nuint32                 luDoubleReadFlag;
   nuint32                 luMapHashNodeCnt;
   nuint32                 luSpaceRestrictionNodeCnt;
   nuint32                 luTrusteeListNodeCnt;
   nuint32                 luPercentOfVolUsedByDirs;
} NWNCPFSEDirCacheInfo, N_FAR *pNWNCPFSEDirCacheInfo;


/* NWNCP123s13GetOSVersionInfo specific structure */
typedef struct tagNWNCPFSEOSVerInfo
{
   nuint8                  buMajorVer;
   nuint8                  buMinorVer;
   nuint8                  buRev;
   nuint8                  buAcctVer;
   nuint8                  buVAPVer;
   nuint8                  buQueueVer;
   nuint8                  buSecurityRestLevel;
   nuint8                  buBridgingSupport;
   nuint32                 luMaxNumVols;
   nuint32                 luMaxNumConns;
   nuint32                 luMaxNumUsers;
   nuint32                 luMaxNumNameSpaces;
   nuint32                 luMaxNumLANs;
   nuint32                 luMaxNumMedias;
   nuint32                 luMaxNumStacks;
   nuint32                 luMaxDirDepth;
   nuint32                 luMaxDataStreams;
   nuint32                 luMaxNumOfSpoolPr;
   nuint32                 luServerSerialNum;
   nuint16                 suServerAppNum;
} NWNCPFSEOSVerInfo, N_FAR *pNWNCPFSEOSVerInfo;


/* NWNCP123s15GetNLMResourceTagList specific structure */
/*
   Resource Tag information is a variable length record.
   This structure is provided to assist in parsing each record.
   The RTag name is zero terminated, so the next structure will
   start immediately following the \0 terminator of the name.
*/
typedef struct tagNWNCPFSEResourceTag
{
   nuint32                  luTagNum;
   nuint32                  luSignature;
   nuint32                  luCnt;
   nstr8                    pbstrName[NWNCP_FSE_MAX_STRING_LEN];
} NWNCPFSEResourceTag, N_FAR *pNWNCPFSEResourceTag;


/* NWNCP123s21GetLANConfigInfo specific structure */
typedef struct tagNWNCPFSELANInfo
{
   nuint8                   buConfigMajorVer;
   nuint8                   buConfigMinorVer;
   nuint8                   abuNodeAddress[6];
   nuint16                  suModeFlags;
   nuint16                  suBoardNum;
   nuint16                  suBoardInstance;
   nuint32                  luMaximumSize;
   nuint32                  luMaxRecvSize;
   nuint32                  luRecvSize;
   nuint32                  aluReserved1[3];
   nuint16                  suCardID;
   nuint16                  suMediaID;
   nuint16                  suTransportTime;
   nuint8                   abuReserved[16];
   nuint8                   buMajorVer;
   nuint8                   buMinorVer;
   nuint16                  suDriverFlags;
   nuint16                  suSendRetries;
   nuint32                  luDriverLink;
   nuint16                  suSharingFlags;
   nuint16                  suDriverSlot;
   nuint16                  asuIOPortsAndLengths[4];
   nuint32                  luMemoryDecode0;
   nuint16                  suMemoryLength0;
   nuint32                  luMemoryDecode1;
   nuint16                  suMemoryLength2;
   nuint8                   abuInterrupt[2];
   nuint8                   abuDMAUsage[2];
   nuint32                  aluReserved2[3];
   nstr8                    pbstrLogicalName[18];
   nuint32                  aluLinearMemory[2];
   nuint16                  suChannelNum;
   nuint8                   abuIOReserved[6];
} NWNCPFSELANInfo, N_FAR *pNWNCPFSELANInfo;


/* NWNCP123s22GetLANCommonCounters specific structure */
typedef struct tagNWNCPFSECommonCounters
{
   nuint32                  luNotSupportedMask;
   nuint32                  luTotalTxPacketCnt;
   nuint32                  luTotalRxPacketCnt;
   nuint32                  luNoECBAvailableCnt;
   nuint32                  luPacketTxTooBigCnt;
   nuint32                  luPacketTxTooSmallCnt;
   nuint32                  luPacketRxOverflowCnt;
   nuint32                  luPacketRxTooBigCnt;
   nuint32                  luPacketRxTooSmallCnt;
   nuint32                  luPacketTxMiscErrCnt;
   nuint32                  luPacketRxMiscErrCnt;
   nuint32                  luRetryTxCnt;
   nuint32                  luChecksumErrCnt;
   nuint32                  luHardwareRxMismatchCnt;
   nuint32                  aluReserved[50];
} NWNCPFSECommonCounters, N_FAR *pNWNCPFSECommonCounters;


/* NWNCP123s23GetLANCustomCounters specific structure */
/*
   Custom Counter values are variable length records.
   This structure is provided to assist in parsing each record.
   The name may not be zero terminated, so the next structure will
   start immediately following the n-th character of the name.
*/
typedef struct tagNWNCPFSECustomCounter
{
   nuint32                  luValue;
   nuint8                   buNameLen;
   nstr                     nstrName[NWNCP_FSE_MAX_STRING_LEN];
} NWNCPFSECustomCounter, N_FAR *pNWNCPFSECustomCounter;


/* NWNCP123s25GetLSLInfo specific structure */
typedef struct tagNWNCPFSELSLInfo
{
   nuint32                  luRxBuffers;
   nuint32                  luRxBuffers75;
   nuint32                  luRxBuffersCheckedOut;
   nuint32                  luRxBufferSize;
   nuint32                  luMaxPacketSize;
   nuint32                  luLastTimeRxBufferWasAlloced;
   nuint32                  luMaxNumProtocols;
   nuint32                  luMaxNumMedias;
   nuint32                  luTotalTxPackets;
   nuint32                  luGetECBBuffers;
   nuint32                  luGetECBFailures;
   nuint32                  luAESEvents;
   nuint32                  luPosponedEvents;
   nuint32                  luECBCancelFailures;
   nuint32                  luValidBuffersReused;
   nuint32                  luEnqueuedSends;
   nuint32                  luTotalRxPackets;
   nuint32                  luUnclaimedPackets;
   nuint8                   buStatMajorVer;
   nuint8                   buStatMinorVer;
} NWNCPFSELSLInfo, N_FAR *pNWNCPFSELSLInfo;


/* NWNCP123s30GetMediaObjectInfo specific structure */
typedef struct tagNWNCPFSEMediaInfo
{
   nstr8                    pbstrLabel[64];
   nuint32                  luObjType;
   nuint32                  luObjTimeStamp;
   nuint32                  luMediaType;
   nuint32                  luCartridgeType;
   nuint32                  luUnitSize;
   nuint32                  luBlockSize;
   nuint32                  luCapacity;
   nuint32                  luPreferredUnitSize;
   nstr8                    pbstrName[64];
   nuint32                  luType;
   nuint32                  luStatus;
   nuint32                  luFunctionMask;
   nuint32                  luControlMask;
   nuint32                  luParentCnt;
   nuint32                  luSiblingCnt;
   nuint32                  luChildCnt;
   nuint32                  luSpecificInfoSize;
   nuint32                  luObjUniqueID;
   nuint32                  luMediaSlot;
} NWNCPFSEMediaInfo, N_FAR *pNWNCPFSEMediaInfo;


/* NWNCP123s33GetVolSegmentList specific structures */
#define NWNCP_FSE_MAX_SEGMENTS     42
typedef struct tagNWNCPFSEVolSegment
{
   nuint32                  luDeviceNum;
   nuint32                  luOffset;
   nuint32                  luSize;
} NWNCPFSEVolSegment, N_FAR *pNWNCPFSEVolSegment;


/* NWNCP123s34GetVolInfoByLevel specific structures */
typedef struct tagNWNCPFSEVolInfo1
{
   nuint32                  luVolType;
   nuint32                  luStatusFlag;
   nuint32                  luSectorSize;
   nuint32                  luSectorsPerCluster;
   nuint32                  luVolSizeInClusters;
   nuint32                  luFreedClusters;
   nuint32                  luSubAllocFreeableClusters;
   nuint32                  luFreeableLimboSectors;
   nuint32                  luNonFreeableLimboSectors;
   nuint32                  luNonFreeableSubAllocSectors;
   nuint32                  luNotUsableSubAllocSectors;
   nuint32                  luSubAllocClusters;
   nuint32                  luDataStreams;
   nuint32                  luLimboDataStreams;
   nuint32                  luOldestDeletedFileAgeInTicks;
   nuint32                  luCompressedDataStreams;
   nuint32                  luCompressedLimboDataStreams;
   nuint32                  luUncompressedDataStreams;
   nuint32                  luPrecompressedSectors;
   nuint32                  luCompressedSectors;
   nuint32                  luMigratedFiles;
   nuint32                  luMigratedSectors;
   nuint32                  luClustersUsedByFAT;
   nuint32                  luClustersUsedByDirs;
   nuint32                  luClustersUsedByExtDirs;
   nuint32                  luTotalDirEntries;
   nuint32                  luUnusedDirEntries;
   nuint32                  luTotalExtDirEntries;
   nuint32                  luUnusedExtDirEntries;
   nuint32                  luExtAttrsDefined;
   nuint32                  luExtAttrExtantsUsed;
   nuint32                  luDSObjID;
   nuint32                  luVolLastModifiedDateAndTime;
} NWNCPFSEVolInfo1, N_FAR *pNWNCPFSEVolInfo1;

typedef struct tagNWNCPFSEVolInfo2
{
   nuint32                  luVolActiveCnt;
   nuint32                  luVolUsageCnt;
   nuint32                  luMACRootIDs;
   nuint32                  luVolLastModifiedDataAndTime;
   nuint32                  luVolRefCnt;
   nuint32                  luCompressionLowerLimit;
   nuint32                  luOutstandingIOs;
   nuint32                  luOutstandingCompressionIOs;
   nuint32                  luCompressionIOLimit;
} NWNCPFSEVolInfo2, N_FAR *pNWNCPFSEVolInfo2;

typedef union
{
   NWNCPFSEVolInfo1         Info1;
   NWNCPFSEVolInfo2         Info2;
} NWNCPFSEVolInfo, N_FAR *pNWNCPFSEVolInfo;


/* NWNCP123s40GetActiveProtoStacks specific structures */
typedef struct tagNWNCPFSEStackInfo
{
   nuint32                  luStackNum;
   nuint8                   abuStackShortName[16];
} NWNCPFSEStackInfo, N_FAR *pNWNCPFSEStackInfo;


/* NWNCP123s43GetProtocolCustomInfo specific structure */
/*
   Protocol Information contains variable length records.
   The following structure is provided to assist in parsing
   each record.
   The name may not be zero terminated, so the next structure will
   start immediately following the n-th character of the name.
*/
typedef struct tagNWNCPFSEProtocolInfo
{
   nuint32                  luNum;
   nuint8                   buShortNameLen;
   nstr8                    pbstrShortName[NWNCP_FSE_MAX_STRING_LEN];
} NWNCPFSEProtocolInfo, N_FAR *pNWNCPFSEProtocolInfo;


/* NWNCP123s52GetRoutersInfo specific structures */
typedef struct tagNWNCPFSERouterInfo
{
   nuint8                   abuNode[6];
   nuint32                  luConnectedLAN;
   nuint16                  suRouteHops;
   nuint16                  suRouteTime;
} NWNCPFSERouterInfo, N_FAR *pNWNCPFSERouterInfo;

#define NWNCP_FSE_MAX_ROUTERS          36
typedef struct tagNWNCPFSERouterList
{
   NWNCPFSERouterInfo       aRouters[NWNCP_FSE_MAX_ROUTERS];
} NWNCPFSERouterList, N_FAR *pNWNCPFSERouterList;


/* NWNCP123s53GetKnownNetworks specific structures */
typedef struct tagNWNCPFSENetworkInfo
{
   nuint32                  luNetIDNum;
   nuint16                  suHopsToNet;
   nuint16                  suNetStatus;
   nuint16                  suTimeToNet;
} NWNCPFSENetworkInfo, N_FAR *pNWNCPFSENetworkInfo;

#define NWNCP_FSE_MAX_NETWORKS        51
typedef struct tagNWNCPFSENetList
{
   NWNCPFSENetworkInfo      aNetworks[NWNCP_FSE_MAX_NETWORKS];
} NWNCPFSENetList, N_FAR *pNWNCPFSENetList;


/* NWNCP123s55GetServerSourcesInfo specific structures */
typedef struct tagNWNCPFSESourceInfo
{
   nuint8                   abuServerNode[6];
   nuint32                  luConnectedLAN;
   nuint16                  suSourceHops;
} NWNCPFSESourceInfo, N_FAR *pNWNCPFSESourceInfo;

#define NWNCP_FSE_MAX_SOURCES       42
typedef struct tagNWNCPFSESourceList
{
   NWNCPFSESourceInfo       aSources[NWNCP_FSE_MAX_SOURCES];
} NWNCPFSESourceList, N_FAR *pNWNCPFSESourceList;


/* NWNCP123s56GetKnownServers specific structure */
/*
   Server Information contains variable length records.
   The following structure is provided to assist in parsing
   each record.
   The name may not be zero terminated, so the next structure will
   start immediately following the n-th character of the name.
*/
typedef struct tagNWNCPFSEKnownServerInfo
{
   nuint8                   abuAddress[12];
   nuint16                  suHopsToServer;
   nuint8                   buNameLen;
   nstr8                    pbstrName[NWNCP_FSE_MAX_STRING_LEN];
} NWNCPFSEKnownServerInfo, N_FAR *pNWNCPFSEKnownServerInfo;

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************************************
   Function prototypes
****************************************************************************/

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s1GetCacheInfo
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSECacheCounters   pCacheCounters,
   pNWNCPFSEMemoryCounters  pMemoryCounters,
   pNWNCPFSETrendCounters   pTrendCounters,
   pNWNCPFSECacheInfo       pCacheInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s2GetServerInfo
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluStationsInUseCnt,
   pnuint32                 pluPeakStationsInUse,
   pnuint32                 pluNumNCPRequests,
   pnuint32                 pluServerUtilization,
   pNWNCPFSEServerInfo      pServerInfo,
   pNWNCPFSEServerCnts      pServerCounters
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s3GetFileSystemsInfo
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSEFileSystemInfo  pFileSystemInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s4GetUserInfo
(
   pNWAccess                 pAccess,
   nuint32                  luConnNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSEUserInfo        pUserInfo,
   pnuint8                  pbuUserNameLen,
   pnstr8                   pbstrUserNameB434
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s5GetPacketBurstInfo
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSEBurstInfo       pBurstInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s6GetIPXSPXInfo
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSEIPXInfo         pIPXInfo,
   pNWNCPFSESPXInfo         pSPXInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s7GetGarbCollectInfo
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluFailedAllocReqCnt,
   pnuint32                 pluNumAllocs,
   pnuint32                 pluNoMoreMemAvlCnt,
   pnuint32                 pluNumGarbageCol,
   pnuint32                 pluFoundSomeMem,
   pnuint32                 pluNumChecks
);


#define NWNCP_FSE_MAX_CPU_STRING        17
#define NWNCP_FSE_MAX_CPROC_STRING      49
#define NWNCP_FSE_MAX_BUS_STRING        33

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s8GetCPUInfo
(
   pNWAccess                 pAccess,
   nuint32                  luCPUNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNumCPUs,
   pNWNCPFSECPUInfo         pCPUInfo,
   pnstr8                   pbstrCPUString,
   pnstr8                   pbstrCoprocessorPresentString,
   pnstr8                   pbstrBusString
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s9GetVolSwitchInfo
(
   pNWAccess                 pAccess,
   nuint32                  luStartItemNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluTotalLFSCounters,
   pnuint32                 pluCurLFSCounters,
   pNWNCPFSESwitchInfo      pCounters
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s10GetNLMLoadedList
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuMoreFlag,
   pnuint32                 pluNumNLMsLoaded,
   pnuint32                 pluNLMCount,
   pnuint32                 pluListB128
);


#define NWNCP_FSE_MAX_FILE_NAME     37
#define NWNCP_FSE_MAX_NLM_NAME      129
#define NWNCP_FSE_MAX_CRIGHT_NAME   256

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s11GetNLMInfo
(
   pNWAccess                 pAccess,
   nuint32                  luNLMNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSENLMInfo         pNLMInfo,
   pnuint8                  pbuFileNameLen,
   pnstr8                   pbstrFileName,
   pnuint8                  pbuNLMNameLen,
   pnstr8                   pbstrNLMName,
   pnuint8                  pbuCopyrightLen,
   pnstr8                   pbstrCopyright
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s12GetDirCacheInfo
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSEDirCacheInfo    pInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s13GetOSVersionInfo
(
   pNWAccess                pAccess,
   pNWNCPFSEVConsoleInfo   pVConsoleInfo,
   pnuint16                psuReserved,
   pNWNCPFSEOSVerInfo      pOSVersion
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s14GetActiveConnsByType
(
   pNWAccess                 pAccess,
   nuint32                  luStartConnNum,
   nuint32                  luConnType,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint8                  pbuBitsB512
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s15GetNLMResTagList
(
   pNWAccess                 pAccess,
   nuint32                  luNLMNum,
   nuint32                  luNLMStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluTotalTags,
   pnuint32                 pluCurTags,
   pnuint8                  pbuResources
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s20GetActLANBoardLst
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluMaxNumLANs,
   pnuint32                 pluNumEntries,
   pnuint32                 pluListB64
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s21GetLANConfigInfo
(
   pNWAccess                 pAccess,
   nuint32                  luBoardNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSELANInfo         pInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s22GetLANCommonCounters
(
   pNWAccess                 pAccess,
   nuint32                  luBoardNum,
   nuint32                  luBlockNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint8                  pbuStatMajorVer,
   pnuint8                  pbuStatMinorVer,
   pnuint32                 pluTotalCounters,
   pnuint32                 pluTotalBlocks,
   pnuint32                 pluCustomCounters,
   pnuint32                 pluNextBlock,
   pNWNCPFSECommonCounters  pCounters
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s23GetLANCustomCounters
(
   pNWAccess                 pAccess,
   nuint32                  luBoardNum,
   nuint32                  luStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluCurCounters,
   pnuint8                  pbuCustomCountersB512
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s25GetLSLInfo
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSELSLInfo         pInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s26GetLSLBoardStats
(
   pNWAccess                 pAccess,
   nuint32                  luBoardNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluTotalTxPackets,
   pnuint32                 pluTotalRxPackets,
   pnuint32                 pluUnclaimedPackets,
   pnuint32                 pluReserved
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s30GetMediaObjectInfo
(
   pNWAccess                 pAccess,
   nuint32                  luObjNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pNWNCPFSEMediaInfo       pInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s31GetMediaObjectList
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   nuint32                  luObjType,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNextStartNum,
   pnuint32                 pluObjCnt,
   pnuint32                 pluObjB128
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s32GetMediaChildList
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   nuint32                  luObjType,
   nuint32                  luParentNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNextStartNum,
   pnuint32                 pluChildCnt,
   pnuint32                 pluChildrenB128
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s33GetVolSegmentList
(
   pNWAccess                 pAccess,
   nuint32                  luVolNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNumSegments,
   pNWNCPFSEVolSegment      pListB42
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s34GetVolInfoByLevel
(
   pNWAccess                 pAccess,
   nuint32                  luVolNum,
   nuint32                  luInfoLevel, /* either 1 or 2 */
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluInfoLevel,
   pNWNCPFSEVolInfo         pInfo
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s40GetActiveProtoStacks
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluMaxNumStacks,
   pnuint32                 pluStackCnt,
   pnuint32                 pluNextStartNum,
   pNWNCPFSEStackInfo       pListB24
);


#define NWNCP_FSE_STACK_SHORT_NAME_SIZE  16
#define NWNCP_FSE_STACK_FULL_NAME_SIZE   256

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s41GetProtocolConfig
(
   pNWAccess                 pAccess,
   nuint32                  luStackNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint8                  pbuConfigMajorVer,
   pnuint8                  pbuConfigMinorVer,
   pnuint8                  pbuStackMajorVer,
   pnuint8                  pbuStackMinorVer,
   pnstr8                   pbstrShortStackNameB16,
   pnuint8                  pbuStackFullNameLen,
   pnstr8                   pbstrStackFullName
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s42GetProtocolStats
(
   pNWAccess                 pAccess,
   nuint32                  luStackNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint8                  pbuStatMajorVer,
   pnuint8                  pbuStatMinorVer,
   pnuint16                 psuCommonCounters,
   pnuint32                 pluCounterMask,
   pnuint32                 pluTotalTxPackets,
   pnuint32                 pluTotalRxPackets,
   pnuint32                 pluIgnoredRxPackets,
   pnuint16                 psuCustomCounters
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s43GetProtCustInfo
(
   pNWAccess                 pAccess,
   nuint32                  luStackNum,
   nuint32                  luCustomStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluCustomCount,
   pnuint8                  pInfoB512
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s44GetProtNumByMedia
(
   pNWAccess                 pAccess,
   nuint32                  luMediaType,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNumStacks,
   pnuint32                 pluStacksB128
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s45GetProtNumByBoard
(
   pNWAccess                 pAccess,
   nuint32                  luBoardNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNumStacks,
   pnuint32                 pluStacksB128
);


#define NWNCP_FSE_MAX_MEDIA_NAME       81

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s46GetMediaNameByNum
(
   pNWAccess                 pAccess,
   nuint32                  luMediaNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint8                  pbuNameLen,
   pnstr8                   pbstrMediaNameB81
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s47GetLoadedMediaList
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluMaxNumMedia,
   pnuint32                 pluNumMedias,
   pnuint32                 pluMediasB32
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s50GetRouterAndSapInfo
(
   pNWAccess                 pAccess,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluRIPSocketNum,
   pnuint32                 pluRouterDownFlag,
   pnuint32                 pluTrackOnFlag,
   pnuint32                 pluExtRouterActiveFlag,
   pnuint32                 pluSapSocketNum,
   pnuint32                 pluReplyNearestServerFlag
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s51GetRouterInfo
(
   pNWAccess                 pAccess,
   nuint32                  luNetworkNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNetIDNum,
   pnuint16                 psuHopsToNetCnt,
   pnuint16                 psuNetStatus,
   pnuint16                 psuTimeToNet
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s52GetRoutersInfo
(
   pNWAccess                 pAccess,
   nuint32                  luNetworkNum,
   nuint32                  luStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNumEntries,
   pNWNCPFSERouterList      pList
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s53GetKnownNetworks
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNumEntries,
   pNWNCPFSENetList         pList
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s54GetServerInfo
(
   pNWAccess                 pAccess,
   nuint32                  luServerType,
   nuint8                   buServerNameLen,
   pnstr8                   pbstrServerName,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint8                  pbuServerAddr,
   pnuint16                 psuHopsToServer
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s55GetServerSourcesInfo
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   nuint32                  luServerType,
   nuint8                   buServerNameLen,
   pnstr8                   pbstrServerName,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNumEntries,
   pNWNCPFSESourceList      pSources
);

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s56GetKnownServers
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   nuint32                  luServerType,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNumEntries,
   pnuint8                  pServerInfoB512
);


#define NWNCP_FSE_MAX_COMMAND_SIZE      483
#define NWNCP_FSE_MAX_COMMAND_VALUE     483

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s60GetServerSetInfo
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluTotalNumSetCommands,
   pnuint32                 pluNextStartNum,
   pnuint32                 pluSetCommandType,
   pnuint32                 pluSetCommandCategory,
   pnuint32                 pluSetCommandFlags,
   pnuint8                  pbuCmdNameLen,
   pnstr8                   pbstrSetCmdNameB483,
   pnuint8                  pbuCmdValueLen,
   pnstr8                   pbstrSetCmdValueB483
);


#define NWNCP_FSE_MAX_CAT_NAME      496

N_EXTERN_LIBRARY( NWRCODE )
NWNCP123s61GetServerSetCategory
(
   pNWAccess                 pAccess,
   nuint32                  luStartNum,
   pNWNCPFSEVConsoleInfo    pVConsoleInfo,
   pnuint16                 psuReserved,
   pnuint32                 pluNextStartNum,
   pnuint32                 pluNumCatagories,
   pnuint8                  pbuNameLen,
   pnstr8                   pbstrCategoryNameB496
);

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else /* !N_PLAT_UNIX */
#include <npackoff.h>
#endif /* N_PLAT_UNIX */

#ifdef __cplusplus
}
#endif

#endif /* NCPFSE_H */

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/ncpfse.h,v 1.6 1994/09/26 17:11:25 rebekah Exp $
*/
