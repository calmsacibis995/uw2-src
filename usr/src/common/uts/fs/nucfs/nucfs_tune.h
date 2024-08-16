/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nucfs_tune.h	1.9"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nucfs_tune.h,v 2.52.2.6 1995/01/29 20:33:00 mdash Exp $"

#ifndef _FS_NUCFS_NUCFS_TUNE_H
#define _FS_NUCFS_NUCFS_TUNE_H

/*
**  Netware Unix Client
**
**	MODULE:
**		nucfs_tune.h -	The NetWare UNIX Client File System tunable
**				parameters. 
**
*/

#ifdef _KERNEL_HEADERS

#include <util/param.h>  /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/param.h>       /* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * NAME
 *    nucfs_tune - The NetWare UNIX Client File System tunable parameters.
 * 
 * DESCRIPTION
 *
 * +-------------------------------------------------------------+
 * |                                                             |
 * |             WARNING         WARNING         WARNING         |
 * |                                                             |
 * |       !!! SOME OF THESE TUNABLES ARE NO LONGER USED  !!!    |
 * |                                                             |
 * |             WARNING         WARNING         WARNING         |
 * |                                                             |
 * +-------------------------------------------------------------+
 *
 *    Following define the tunable parameter used in the NetWare UNIX Client
 *    File System layer (NWfi & NWfs):
 *
 *    EST_NWFS_MAX_CACHE_TIME          - Defines the estimatied maximum time, a
 *                                       server node stays on a cache list.
 *    EST_NWFS_MID_CACHE_TIME          - Defines the estimatied midpoint time,
 *                                       a server node stays on a cache list.
 *    EST_NWFS_MIN_CACHE_TIME          - Defines the estimatied minimum time, a 
 *                                       server node stays on a cache list.
 *    EST_NWFS_SERVER_NODES            - Defines the estimatied maximum number
 *                                       of active/cache server node objects.
 *                                       The needed memory for the server nodes
 *                                       on both the active and the cache lists
 *                                       of a specific server volume object are
 *                                       allocated from the NUC File System 
 *                                       memory region.
 *    EST_NWFS_CLIENT_HANDLES_PER_NODE - Defines the estimated maximum number of
 *                                       client handle objects attached to a
 *                                       server node object.
 *    EST_NWFS_LOCKS_PER_NODE          - Defines the estimatied maximum number
 *                                       of lock objects attached to a server
 *                                       node object.
 *    EST_NWFS_SERVER_VOLUMES          - Defines the estimated maximum number
 *                                       of volume objects to be mounted.
 *    NUCFS_DIR_BUFFERS                - Defines the maximum number of buffers
 *                                       that can be allocated from the NetWare
 *                                       UNIX Client File System directory  
 *                                       memory region.
 *    NUCFS_DIR_BUFFER_SIZE            - Defines the maximum number of bytes in
 *                                       a directory buffer.
 *    NUCFS_DIR_NODES_TO_BE_CLOSED     - Defines the maximum number of directory
 *                                       client handles to close when no more
 *                                       directory handles are available.
 *    EST_NWNF_FLUSH_TIME              - Defines how often the node flusher
 *                                       runs, in seconds.
 *    NUCFS_MAX_CACHED_NODES_LIMIT     - If the total number of cached nodes
 *                                       (nwfsCachedServerNodesCount) is greater
 *                                       than NUCFS_MAX_CACHED_NODES_LIMIT, the
 *                                       node flusher would use the minimum time
 *                                       (EST_NWNF_MIN_CACHE_TIME) when aging
 *                                       the cached server nodes.
 *    NUCFS_MIN_CACHED_NODES_LIMIT     - If the total number of cached nodes
 *                                       (nwfsCachedServerNodesCount) is greater
 *                                       than NUCFS_MIN_CACHED_NODES_LIMIT, but
 *                                       less than NUCFS_MAX_CACHED_NODES_LIMIT,
 *                                       the node flusher would use the midpoint
 *                                       time (EST_NWNF_MID_CACHE_TIME) when
 *                                       aging the cached server nodes. And if
 *                                       nwfsCachedServerNodesCount is less than
 *                                       NUCFS_MIN_CACHED_NODES_LIMIT, the node
 *                                       flusher uses maximum time (EST_NWFS_
 *                                       MAX_CACHE_TIME) when aging the cached
 *                                       server nodes.
 *    NUCFS_FRLOCK_DELAY_TIME          - ticks to delay before retrying frlock
 *					 request.
 *    NUCFS_FRLOCK_MAX_RETRIES	       - Number of times to retry a frlock
 *					 request.
 */

#define	EST_NWFS_MAX_CACHE_TIME			60
#define	EST_NWFS_MID_CACHE_TIME			45
#define	EST_NWFS_MIN_CACHE_TIME			30
#define	EST_NWFS_SERVER_NODES			2048
#define	EST_NWFS_CLIENT_HANDLES_PER_NODE	6
#define	EST_NWFS_LOCKS_PER_NODE			3
#define	EST_NWFS_SERVER_VOLUMES			10
#define	NUCFS_DIR_BUFFERS			10
#define	NUCFS_DIR_BUFFER_SIZE			4096
#define	NUCFS_DIR_NODES_TO_BE_CLOSED		5
#define	EST_NWNF_FLUSH_TIME			2
#define	NUCFS_MAX_CACHED_NODES_LIMIT		1024
#define	NUCFS_MIN_CACHED_NODES_LIMIT		512
#define NUCFS_FRLOCK_DELAY_TIME			(2 * HZ)
#define NUCFS_FRLOCK_MAX_RETRIES		10

/*
 * error handling
 */
#define NUCFS_RETRY_LIMIT			4
#define	NUCFS_DELAY_TIME			20	/* seconds */
#define NUCFS_STALE_TICKS			(15 * HZ)

#endif /* _FS_NUCFS_NUCFS_TUNE_H */
