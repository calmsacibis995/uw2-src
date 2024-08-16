/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nucfscommon.h	1.11"
#ifndef _FS_NUCFS_NUCFSCOMMON_H
#define _FS_NUCFS_NUCFSCOMMON_H

#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nucfscommon.h,v 2.54.2.5 1995/01/25 00:51:10 stevbam Exp $"

/*
**  Netware Unix Client
**
**	MODULE:
**		nucfscommon.h -	The NetWare UNIX Client File System common
**				definitions used in both the NetWare Client File
**				System layer (NWfs) and Virtual File System
**				layer (NWfi).
**
** +-------------------------------------------------------------+
** |                                                             |
** |             WARNING         WARNING         WARNING         |
** |                                                             |
** |                     !!! COMMENT IS STALE !!!                |
** |                                                             |
** |             WARNING         WARNING         WARNING         |
** |                                                             |
** +-------------------------------------------------------------+
**
**	ABSTRACT:
**		The nucfscommon.h is included with NetWare Client File System 
**		layer (NWfs) and Virtual File System Interface Layer (NWfi) of
**		the NetWare UNIX Client File System layer (NUCfs).  This
**		provides a consistent semantic representation of interface
**		information used between these layers.
**
*/

/*
 * Caching manifest constents of the NWfs layer. 
 */
#define	NWFS_DONOT_USE_CACHING	0	/* Caching is not allowed	*/
#define	NWFS_USE_CACHING	1	/* Caching is allowed		*/

/*
 * Common manifest constents.
 */
#define	NOT_USED		0	/* Variable not used		*/
#define	UNKNOWN_NODE_NUMBER	-1	/* Unknown node number		*/

/*
 * Name space managment manifests.
 */
#define	NWFS_GET_NAME_SPACE_INFO	0	/* Get name space info	*/
#define	NWFS_SET_NAME_SPACE_INFO	1	/* Set name space info	*/

/*
 * NWfs layer list types manifest constents.
 */
#define	NWFS_LOCK_LIST		1	/* Lock list			*/
#define	NWFS_CLIENT_HANDLE_LIST	2	/* Client handle list		*/

/*
 * Block map capabilities manifest constents. 
#define	NWFS_NO_BLOCK_OPS	0	* Disable block operation	*
#define	NWFS_ALLOW_BLOCK_OPS	1	* Enable block operation	*
 */

/*
 * NUC File System volume common manifest constents. 
 */
#define	NUCFS_VOLUME_READ_ONLY   0x01	/* Volume was mounted read only	*/
#define	NWFS_UNIX_NETWARE_NAME_SPACE 0x02 /* Volume in UNIX NetWare mode*/
#define	NWFS_DOS_NAME_SPACE      0x04	/* Volume in DOS name space	*/
#define	NWFS_UNMOUNTING_VOLUME   0x08	/* Volume is being unmounted	*/
#define	NUCFS_INHERIT_PARENT_GID 0x10	/* Inherit parent GID in create	*/
#define	NWFS_UNIX_NFS_NAME_SPACE 0x20	/* Volume in UNIX NFS mode	*/

/*
 * NUC File System's node ID shift value.
 */
#define	NUCFS_SHIFT		0x0000ffff

/*
 * NUC File System file name length.
 */
#define	MAX_UNIX_FILE_NAME	255
#define SMALL_UNIX_FILE_NAME	14
#define MAX_DOS_FILE_NAME	14

/* 
 * Set attributes command manifest constents.
 */
#define	NUCFS_CHANGE_MODE	0	/* chmod command		*/
#define	NUCFS_CHANGE_OWNER	1	/* chown command		*/
#define	NUCFS_CHANGE_TIME	2	/* utime command		*/

/*
 * NUC File System maximum values.
 */
#define	MAX_DIRECTORY_DEPTH		100

#define	MAX_SERVER_NAME_LENGTH		48
#define	MAX_VOLUME_NAME_LENGTH		48
#define	DOT_ENTRIES_LENGTH		32	/* . & .. entries size	*/
#define	MAX_NAME_LENGTH			255	/* Max name length	*/
#define MAX_GENERIC_DIR_ENTRY_SIZE \
	((sizeof (struct dirent) + (MAX_NAME_LENGTH + 1) +  \
	sizeof (uint32)) & ~(sizeof (uint32) - 1 ))

/*
 * Define the NetWare UNIX Client File System version numbers.
 */
#define	NUCFS_MAJOR_VERSION_NUMBER	2
#define NUCFS_MINOR_VERSION_NUMBER	1
#define NUCFS_VERSION_STRING		"2.01"

/*
 * Define the number of buckets for the server volume identity table.
 * Must be a power of 2 for the benefit of the SNODE_HASH function.
 */
#define	NUCFS_NODE_LIST_BUCKETS		128

/*
 * NAME
 *    NUCfsMountOptions - The NWfi layer mount options.
 *
 * DESCRIPTION
 *    This data structure defines the different options to be use for enforcing
 *    security when mounting a server volume.
 *    nucfsAllCanMount  - Flag indicating that anybody can mount a nucfs file
 *                        system.
 *    nucfsRootCanMount - Flag indicating that only root can mount a nucfs file
 *                        system.
 *    nucfsGroupID      - If the user who is mounting a nucfs file system 
 *                        belongs to this group, access is granted to mount the
 *                        server volume.
 */
typedef struct NUCfsMountOptions {
	unsigned	nucfsAllCanMount:1;
	unsigned	nucfsRootCanMount:1;
	int32		nucfsGroupID;
} NUCFS_MOUNT_OPTIONS_T;

/*
 * NAME
 *    NUCfsVolumeStats - The NUC File System Server Volume statistics structure.
 * 
 * DESCRIPTION
 *    This data structure defines the Server Volume statistics interface
 *    structure.  This is the interface structure for use with
 *    NWfsVolumeStatistics(3k) and NWfsMountRootNode(3k) to acquire volume
 *    statistic information.  This data structure is used in both the Virtual
 *    File System Interface layer (NWfi) and the NetWare Client File System 
 *    layer (NWfs) of the NetWare UNIX Client File System (NUCfs).
 *
 *    logicalBlockSize - Maximum number of bytes in a logical block.
 *    volumeFlags      - Flags the volume was mounted with.
 *    totalBlocks      - Total number of blocks available on a specific volume.
 *    totalFreeBlocks  - Total number of free blocks available on a specific
 *                       volume.
 *    totalNodes       - Total number of nodes available on a specific volume.
 *    totalFreeNodes   - Total number of free nodes available on a specific
 *                       volume.
 *    serverName       - Name of the NetWare server the volume is on.
 *    volumeName       - Name of the volume.
 */

#define MAX_ADDRESS_SIZE 30
/*
 * Define NUCfsVolumeStats structure mask.
 */
typedef	 struct	 NUCfsVolumeStats {
	uint32	logicalBlockSize;
	uint16	volumeFlags;
	uint32	totalBlocks;
	uint32	totalFreeBlocks;
	uint32	totalNodes;
	uint32	totalFreeNodes;
	struct netbuf serverAddress;
	char	buffer[MAX_ADDRESS_SIZE];
	char    volumeName[MAX_VOLUME_NAME_LENGTH];
} NUCFS_VOLUME_STATS_T;

/* 
 * NAME
 *    NUCfsIoArgs - The NUC File System I/O argument structure.
 *
 * DESCRIPTION
 *    This data structure defines the NetWare UNIX Client File System I/O
 *    argument structre.  This is the interface structure for use with
 *    NWfsReadBlocksOnNode(3k), NWfsReadBytesOnNode(3k),
 *    NWfsWriteBlocksOnNode(3k), and NWfsWriteBytesOnNode(3k) to acquire I/O
 *    operations.  This data structure is used in both the Virtual File System
 *    Interface layer (NWfi) and the NetWare Client File System layer (NWfs) of
 *    the NetWare UNIX Client File System (NUCfs).
 *
 *    granuleRequested   - Number of bytes/blocks to read/write.
 *    ioBuffer           - Pointer to a contiguous buffer to read/write the 
 *                         bytes/blocks of data to/from a server node object
 *                         file.
 *    memoryTypeFlag     - Same as NUC_IOBUF_T memoryType.
 *    granuleOffset      - Byte offser from the beginning of file data space
 *                         to start the read/write from.
 *    granulesReturned   - Size in bytes copied into the ioBuffer.
 */

/*
 * Define NUCfsIoArgs structure mask.
 */
typedef struct NUCfsIoArgs {
	NUC_IOBUF_T	nucIoBuf;
	int32		granuleOffset;
	int32		granulesReturned;
} NUCFS_IO_ARGS_T;

#define	granulesRequested	nucIoBuf.bufferLength
#define	ioBuffer		nucIoBuf.buffer
#define	memoryTypeFlag		nucIoBuf.memoryType

/* 
 * NAME
 *    NUCfsDirIoArgs - The NUC File System directory I/O argument structure.
 *
 * DESCRIPTION
 *    This data structure defines the NetWare UNIX Client File System directory
 *    I/O argument structre.  This is the interface structure for use with
 *    NWfsReadDirNodeEntries(3k) to acquire directory entries.  This data
 *    structure is used in both the Virtual File System Interface layer (NWfi)
 *    and the NetWare Client File System layer (NWfs) of the NetWare UNIX
 *    Client File System (NUCfs).
 *
 *    nucIoBuf.bufferLength   - Size of nucIoBuf.buffer.
 *    nucIoBuf.buffer         - Pointer to a contiguous buffer to read the 
 *                              directory entries into.
 *    nucIoBuf.kernelResident - Always Set to TRUE to indicate that the 
 *                              nucIoBuf.buffer is resident in kernel space.
 *    dirSearchHandle         - SPIL search thread context handle.
 */

/*
 * Define NUCfsIoArgs structure mask.
 */
typedef struct NUCfsDirIoArgs {
	NUC_IOBUF_T	nucIoBuf;
	uint32		dirSearchHandle;
} NUCFS_DIR_IO_ARGS_T;

#endif /* _FS_NUCFS_NUCFSCOMMON_H */
