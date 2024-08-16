/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwvol.h	1.4"
/*--------------------------------------------------------------------------
   Copyright (c) 1991 by Novell, Inc. All Rights Reserved.
--------------------------------------------------------------------------*/
#ifndef NWVOL_INC
#define NWVOL_INC

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

/* define volume types  */

#define VINetWare386    0
#define VINetWare286    1
#define VINetWare386v30 2
#define VINetWare386v31 3

/*    define the extended volume information status flag bits  */

#define NWSubAllocEnabledBit    0x01
#define NWCompressionEnabledBit 0x02
#define NWMigrationEnabledBit   0x04
#define NWAuditingEnabledBit    0x08
#define NWReadOnlyEnabledBit    0x10

typedef struct
{
  nuint32 objectID;
  nuint32 restriction;
} NWOBJ_REST;

typedef struct
{
  nuint8  numberOfEntries;
  struct
  {
    nuint32 objectID;
    nuint32 restriction;
  } resInfo[12];
} NWVolumeRestrictions;

typedef struct
{
  nuint8  numberOfEntries;
  struct
  {
    nuint32 objectID;
    nuint32 restriction;
  } resInfo[16];
} NWVOL_RESTRICTIONS;

typedef struct
{
  nint32    systemElapsedTime;
  nuint8    volumeNumber;
  nuint8    logicalDriveNumber;
  nuint16   sectorsPerBlock;
  nuint16   startingBlock;
  nuint16   totalBlocks;
  nuint16   availableBlocks;
  nuint16   totalDirectorySlots;
  nuint16   availableDirectorySlots;
  nuint16   maxDirectorySlotsUsed;
  nuint8    isHashing;
  nuint8    isCaching;
  nuint8    isRemovable;
  nuint8    isMounted;
  nstr8     volumeName[16];
} VOL_STATS;


typedef struct ExtendedVolInfo_tag
{
  nuint32 volType;
  nuint32 statusFlag;
  nuint32 sectorSize;
  nuint32 sectorsPerCluster;
  nuint32 volSizeInClusters;
  nuint32 freeClusters;
  nuint32 subAllocFreeableClusters;
  nuint32 freeableLimboSectors;
  nuint32 nonfreeableLimboSectors;
  nuint32 availSubAllocSectors;            /* non freeable */
  nuint32 nonuseableSubAllocSectors;
  nuint32 subAllocClusters;
  nuint32 numDataStreams;
  nuint32 numLimboDataStreams;
  nuint32 oldestDelFileAgeInTicks;
  nuint32 numCompressedDataStreams;
  nuint32 numCompressedLimboDataStreams;
  nuint32 numNoncompressibleDataStreams;
  nuint32 precompressedSectors;
  nuint32 compressedSectors;
  nuint32 numMigratedDataStreams;
  nuint32 migratedSectors;
  nuint32 clustersUsedByFAT;
  nuint32 clustersUsedByDirs;
  nuint32 clustersUsedByExtDirs;
  nuint32 totalDirEntries;
  nuint32 unusedDirEntries;
  nuint32 totalExtDirExtants;
  nuint32 unusedExtDirExtants;
  nuint32 extAttrsDefined;
  nuint32 extAttrExtantsUsed;
  nuint32 DirectoryServicesObjectID;
  nuint32 volLastModifiedDateAndTime;
} NWVolExtendedInfo;


NWCCODE N_API NWGetDiskUtilization
(
  NWCONN_HANDLE   conn,
  nuint32         objID,
  nuint8          volNum,
  pnuint16        usedDirectories,
  pnuint16        usedFiles,
  pnuint16        usedBlocks
);

NWCCODE N_API NWGetObjDiskRestrictions
(
  NWCONN_HANDLE   conn,
  nuint8          volNumber,
  nuint32         objectID,
  pnuint32        restriction,
  pnuint32        inUse
);

NWCCODE N_API NWScanVolDiskRestrictions
(
  NWCONN_HANDLE   conn,
  nuint8          volNum,
  pnuint32        iterhandle,
  NWVolumeRestrictions N_FAR * volInfo
);

NWCCODE N_API NWScanVolDiskRestrictions2
(
  NWCONN_HANDLE   conn,
  nuint8          volNum,
  pnuint32        iterhandle,
  NWVOL_RESTRICTIONS N_FAR * volInfo
);

NWCCODE N_API NWRemoveObjectDiskRestrictions
(
  NWCONN_HANDLE   conn,
  nuint8          volNum,
  nuint32         objID
);

NWCCODE N_API NWSetObjectVolSpaceLimit
(
  NWCONN_HANDLE   conn,
  nuint16         volNum,
  nuint32         objID,
  nuint32         restriction
);

NWCCODE N_API NWGetVolumeInfoWithHandle
(
  NWCONN_HANDLE   conn,
  NWDIR_HANDLE    dirHandle,
  pnstr8          volName,
  pnuint16        totalBlocks,
  pnuint16        sectorsPerBlock,
  pnuint16        availableBlocks,
  pnuint16        totalDirEntries,
  pnuint16        availableDirEntries,
  pnuint16        volIsRemovableFlag
);

NWCCODE N_API NWGetVolumeInfoWithNumber
(
  NWCONN_HANDLE   conn,
  nuint16         volNum,
  pnstr8          volName,
  pnuint16        totalBlocks,
  pnuint16        sectorsPerBlock,
  pnuint16        availableBlocks,
  pnuint16        totalDirEntries,
  pnuint16        availableDirEntries,
  pnuint16        volIsRemovableFlag
);

NWCCODE N_API NWGetVolumeName
(
  NWCONN_HANDLE   conn,
  nuint16         volNum,
  pnstr8          volName
);

NWCCODE N_API NWGetVolumeNumber
(
  NWCONN_HANDLE   conn,
  pnstr8          volName,
  pnuint16        volNum
);

NWCCODE N_API NWGetVolumeStats
(
  NWCONN_HANDLE   conn,
  nuint8          volNum,
  VOL_STATS N_FAR * volInfo
);

NWCCODE N_API NWGetExtendedVolumeInfo
(
  NWCONN_HANDLE   conn,
  nuint16         volNum,
  NWVolExtendedInfo N_FAR * volInfo
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
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwvol.h,v 1.6 1994/06/08 23:33:30 rebekah Exp $
*/
