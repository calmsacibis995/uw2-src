/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nucfsspace.h	1.7"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nucfsspace.h,v 2.52.2.3 1995/01/18 16:42:02 mdash Exp $"

#ifndef _FS_NUCFS_NUCFSSPACE_H
#define _FS_NUCFS_NUCFSSPACE_H


/*
**  Netware Unix Client
**
**	MODULE:
**		nucfsspace.h -	Contains the NetWare UNIX Client File System
**				space/size structure.
**
**	ABSTRACT:
**		The nucfsspace.h is included in both the NetWare Client File
**		System layer (NWfs) and the Virtual File System Interface layer
**		(NWfi) of the NetWare UNIX Client File System layer (NUCfs).
**		It contains the space/size structure.
*/

/*
 * NAME
 *    nucfsTune - The NetWare UNIX Client File System space/size structure.
 *
 * DESCRIPTION
 *    This data structure contains the NetWare UNIX Client File System 
 *    Interface space/size variables and tunable parameters defined in the
 *    nwcfs_tune.h and space.c files.  The reason an structure is defined is
 *    to pick up tunable parameters at run time VS compile time.
 *
 *    nwfsMaxCacheTime          - Estimated maximum time to keep a server node
 *                                on a cache list.
 *    nwfsMidCacheTime          - Estimated midpoint time to keep a server node
 *                                on a cache list.
 *    nwfsMinCacheTime          - Estimated minimum time to keep a server node
 *                                on a cache list.
 *    nwfsServerNodes           - Estimated maximum number of server node
 *                                objects to be allocated.
 *    nwfsClientHandlesPerNode  - Estimated maximum number of client handle
 *                                objects on a server node object.
 *    nwfsLocksPerNode          - Estimated maximum number of lock objects on a
 *                                server node object.
 *    nwfsServerVolumes         - Estimated maximum number of server volume 
 *                                objects to be mounted.
 *    nucfsDirBuffers           - Maximum number of buffers in the directory
 *                                memory region.
 *    nucfsDirBufferSize        - Maximum number of bytes in a directory buffer.
 *    nucfsDirNodesToBeClosed   - Defines the maximum number of directory
 *                                client handles to close when no more directory
 *                                handles are available.
 *    nwnfFlushTime             - Defines how often the node flusher runs, in
 *                                seconds.
 *    nucfsMaxCachedNodesLimit  - If the total number of cached nodes
 *                                (nwfsCachedServerNodesCount) is greater than
 *                                NUCFS_MAX_CACHED_NODES_LIMIT, the node flusher
 *                                would use the minimum time (EST_NWNF_MIN_CACHE
 *                                _TIME) when aging the cached server nodes.
 *    nucfsMinCachedNodesLimit  - If the total number of cached nodes
 *                                (nwfsCachedServerNodesCount) is greater than
 *                                NUCFS_MIN_CACHED_NODES_LIMIT, but less than
 *                                NUCFS_MAX_CACHED_NODES_LIMIT, the node flusher
 *                                would use the midpoint time (EST_NWNF_MID_
 *                                CACHE_TIME) when aging the cached server
 *                                nodes. And if nwfsCachedServerNodesCount is 
 *                                less than NUCFS_MIN_CACHED_NODES_LIMIT, the
 *                                node flusher uses maximum time (EST_NWFS_MAX_
 *                                CACHE_TIME) when aging the cached server
 *                                nodes.
 *    nucfsFrlockDelayTime	- ticks to delay before retrying frlock request.
 *    nucfsFrlockMaxRetries	- Number of times to retry a frlock request.
 */

/*
 * Define nucfsTune structure.
 */
typedef struct nucfsTune {
	int32	nwfsMaxCacheTime;
	int32	nwfsMidCacheTime;
	int32	nwfsMinCacheTime;
	int32	nwfsServerNodes;
	int32	nwfsClientHandlesPerNode;
	int32	nwfsLocksPerNode;
	int32	nwfsServerVolumes;
	int32	nucfsDirBuffers;
	int32	nucfsDirBufferSize;
	uint32	nucfsDirNodesToBeClosed;
	uint32	nwnfFlushTime;
	uint32	nucfsMaxCachedNodesLimit;
	uint32	nucfsMinCachedNodesLimit;
	uint32	nucfsFrlockDelayTime;
	uint32	nucfsFrlockMaxRetries;
} NUCFS_TUNE_T;

#ifdef ALLOCATE_NWFI_SPACE		/* Defined in space.c		*/

/* 
 * Allocate and initialize nwfiTune structure.
 */
NUCFS_TUNE_T	nucfsTune = {
	EST_NWFS_MAX_CACHE_TIME,		/* Defined in nucfs_tune.h */
	EST_NWFS_MID_CACHE_TIME,		/* Defined in nucfs_tune.h */
	EST_NWFS_MIN_CACHE_TIME,		/* Defined in nucfs_tune.h */
	EST_NWFS_SERVER_NODES,			/* Defined in nucfs_tune.h */
	EST_NWFS_CLIENT_HANDLES_PER_NODE,	/* Defined in nucfs_tune.h */
	EST_NWFS_LOCKS_PER_NODE,		/* Defined in nucfs_tune.h */
	EST_NWFS_SERVER_VOLUMES,		/* Defined in nucfs_tune.h */
	NUCFS_DIR_BUFFERS,			/* Defined in nucfs_tune.h */
	NUCFS_DIR_BUFFER_SIZE,			/* Defined in nucfs_tune.h */
	NUCFS_DIR_NODES_TO_BE_CLOSED,		/* Defined in nucfs_tune.h */
	EST_NWNF_FLUSH_TIME,			/* Defined in nucfs_tune.h */
	NUCFS_MAX_CACHED_NODES_LIMIT,		/* Defined in nucfs_tune.h */
	NUCFS_MIN_CACHED_NODES_LIMIT,		/* Defined in nucfs_tune.h */
	NUCFS_FRLOCK_DELAY_TIME,		/* Defined in nucfs_tune.h */
	NUCFS_FRLOCK_MAX_RETRIES,		/* Defined in nucfs_tune.h */
};

#else

extern	NUCFS_TUNE_T	nucfsTune;

#endif					/* ALLOCATE_NWFI_SPACE		*/

#endif /* _FS_NUCFS_NUCFSSPACE_H */
