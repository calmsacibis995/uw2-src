/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gextvol.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"

#include "nwvol.h"
#include "ncpserve.h"

/*manpage*NWGetExtendedVolumeInfo*******************************************
SYNTAX:  NWCCODE N_API NWGetExtendedVolumeInfo
         (
            NWCONN_HANDLE conn,
            nuint16       volNum,
            NWVolExtendedInfo NWPTR volInfo
         )

REMARKS:

ARGS: >  conn
      >  volNum
      <  volInfo   - structure to receive information (see below).
         typedef struct ExtendedVolInfo_tag
         {
            nuint32 volType;
               Differentiate between differant volumes that may be supported
               in the future.
            nuint32 statusFlag;
               Options currently available on this volume.
            nuint32 sectorSize;
               Sector size in bytes
            nuint32 sectorsPerCluster;
               Number of sectors per cluster
            nuint32 volSizeInClusters;
               Size (in clusters) of the volume.
            nuint32 freeClusters;
               Number of clusters currently free for allocation.  This does
               not include space currently available from deleted (limbo)
               files, nor space that could be recalmed from the sub
               allocation file system.
            nuint32 subAllocFreeableClusters;
               Space that could be reclaimed from the sub allocation file
               system.
            nuint32 freeableLimboSectors;
               Disk space, in clusters, that could be freed from deleted
               files.
            nuint32 nonFreeableLimboSectors;
               Disk space, in clusters, currently in deleted files but that
               haven't aged enough to be classified as FreeableLimboClusters.
               These will be migrated to the status of FreeableLimboCluster
               after time.
            nuint32 availSubAllocSectors;
               Space available to the sub allocation file system but cannot
               be freed up and returned as clusters.
            nuint32 nonUsableSubAllocSectors;
               Disk space that is wasted by the suballocation file system.
               These clusters cannot be allocated by the sub allocation
               system or used as regular clusters.
            nuint32 subAllocClusters;
               Disk space being used by the sub allocation file system.
            nuint32 numDataStreams;
               Number of data streams for real files that have data allocated
               to them.
            nuint32 numLimboDataStreams;
               Number of data streams for deleted files tha have data
               allocated to them.
            nuint32 oldestDelFileAgeInTicks;
               The current age of the oldest file in ticks.
            nuint32 numCompressedDataStreams;
               Number of data streams for real files that are compressed.
            nuint32 numCompressedLimboDataStreams;
               Count of data streams for deleted files that are compressed.
            nuint32 numNonCompressableDataStreams;
               Data streams found not compressable (real and deleted).
            nuint32 preCompressedSectors;
               Disk space allocated to all files before they were compressed
               (includes "hole" space).
            nuint32 compressedSectors;
               Disk space used by all compressed files.
            nuint32 nummigratedDataStreams;
               Number of data streams that have been migrated.
            nuint32 migratedSectors;
               Disk space in sectors that have been migrated.
            nuint32 clustersUsedByFAT;
               Disk space in clusters used by the FAT table.
            nuint32 clustersUsedByDirs;
               Disk space in clusters used by directories.
            nuint32 clustersUsedByExtDirs;
               Disk space in clusters used by the extended directory space.
            nuint32 totalDirEntries;
               Total number of directories available on the volume.
            nuint32 unUsedDirEntries;
               Total directory entries unused on volume.
            nuint32 totalExtDirExtants;
               Amount of extended directory space extants (128 byte each)
               available on volume.
            nuint32 unUsedExtDirsExtants;
               Amount of extended directory space extants (128 byte each)
               unused on volume.
            nuint32 extAttrsDefined;
               Number of extended attributes defined on vlume.
            nuint32 extAttrsExtantsUsed;
               Number of extended dir extants used by the extended
               attributes.
            nuint32 directoryServicesObjectID;
            nuint32 volLastModifiedDateAndTime;
         } NWVolExtendedInfo;

         defined volume types:
            VINetWare386    0
            VINetWare286    1
            VINetWare386v30 2
            VINetWare386v31 3

         defined status flag bits:
            VISubAllocEnabledBit    1
            VICompressionEnabledBit 2
            VIMigrationEnabledBit   4
            VIAuditingEnabledBit    8

INCLUDE: nwvol.h

RETURN:

SERVER:  4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     22 51  Get Extended Volume Information

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetExtendedVolumeInfo
(
  NWCONN_HANDLE conn,
  nuint16       suVolNum,
  NWVolExtendedInfo NWPTR volInfo
)
{
   nuint8 abuReserved1B2[2], abuReserved2B16[16];

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP22s51GetExtVolInfo(&access, (nuint8)suVolNum,
               abuReserved1B2, (pNWNCPExtVolInfo)volInfo, abuReserved2B16));

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gextvol.c,v 1.7 1994/09/26 17:46:27 rebekah Exp $
*/
