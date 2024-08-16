/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nwfschandle.h	1.10"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nwfschandle.h,v 2.55.2.8 1995/01/29 21:31:17 mdash Exp $"

#ifndef _FS_NUCFS_NWFSHANDLE_H
#define _FS_NUCFS_NWFSHANDLE_H

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <fs/nucfs/nwfidata.h>	/* REQUIRED */
#include <net/nw/nwportable.h>	/* REQUIRED */
#include <net/nuc/nucerror.h>	/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/nwfidata.h>	/* REQUIRED */
#include <sys/nwportable.h>	/* REQUIRED */
#include <sys/nucerror.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
**  Netware Unix Client
**
**	MODULE:
**		nwfschandle.h -	Contains the NetWare Client File System layer
**				(NWfs) client handle object structure
**				definition.
**
**	ABSTRACT:
**		The nwfschandle.h is included in the NetWare Client File System 
**		layer (NWfs) and represents a client handle object structure.
**
**	OBJECT THEOREM RULES OF INFERENCE (PRODUCTION):
**         1) A client handle object (NWFS_CLIENT_HANDLE_T) is exclusively
**            owned by a UNIX (user ID, group ID) pair through its parent
**	      server node.
**         2) SPIL resources are directly attached to real client handle
**            objects.
**         3) There must only be one client handle object per
**	      (user ID, group ID) pair referencing the server node object.
*/

/*
 * +-------------------------------------------------------------+
 * |                                                             |
 * |             WARNING         WARNING         WARNING         |
 * |                                                             |
 * |                     !!! COMMENT IS STALE !!!                |
 * |                                                             |
 * |             WARNING         WARNING         WARNING         |
 * |                                                             |
 * +-------------------------------------------------------------+
 *
 * NAME
 *    NWfsClientHandle - The NWfs client handle object structure.
 *
 * DESCRIPTION
 *    This data structure defines the NetWare Client File System layer (NWfs)
 *    client handle object structure.  There is no paradigm in the UNIX Generic
 *    File System for a client handle object.
 *
 *    clientHandleList - Doubly linked list of client handles associated
 *			 with this node.
 *    holdCount	       - Number of holds on the handle. The server node's
 *			 client handle list exerts one hold on the handle.
 *			 Additional holds are exerted when the handle is
 *			 to be used for some operation. A clone file
 *			 handle also asserts a hold on the non-clone.
 *    credentials      - Credentials of the UNIX client associated with this
 *                       client handle object.
 *    clientHandleType - Set to one of the following:
 *                       REAL_CLIENT_HANDLE  - A real client handle object.
 *                       CLONE_CLIENT_HANDLE - A clone client handle object.
 *    resourceHandle   - Pointer to the SPIL resource handle, if the 
 *                       clientHandlType is set to REAL_CLIENT_HANDLE.  Pointer
 *                       to another NWFS_CLIENT_HANDLE_T object, if the
 *                       clientHandleType is set to CLONE_CLIENT_HANDLE.
 *    accessFlags      - Access flags, a NetWare file was opened with.  It
 *                       is set to an inclusive 'OR' of the following:
 *                       NW_READ      - File was opened for reading.
 *                       NW_WRITE     - File was opened for writing.
 *                       NW_EXCLUSIVE - File was opened non shareable.
 *    flockCacheChain  - List head of cached lock requests
 *			 (NUCFS_FLOCK_CACHE_T) honored by the server
 *    flockCacheLen    - Number of elements logically in flockCacheChain.
 *			 Reference counting may result in a lower number of
 *			 physical elements.  This counter is also inflated
 *			 transiently while flock operations are in progress,
 *			 to prevent resource handles from being removed in the
 *			 interval between the time a lock is placed on the
 *			 the server and the time the request is cached.
 *    inflated	       - For debugging; the count by which flockCacheLen has
 *			 been inflated.
 */

/*
 * The following is used independently as a list marker, as the
 * header of a NWfsServerNode structure, or as the header of a
 * NWfsClientHandle structure.
 */
struct NWfsNodeHeader {
	NWFI_LIST_T		chain[2];	/* must come first */
	uint32			headerState;
};

/*
 * Structure to hold credentials
 */
typedef struct NWfsCred {
	uint32			userId;
	uint32			groupId;
} NWFS_CRED_T;

/*
 * Structure to record times when going over the wire to the server.
 */
typedef struct {
	NWFI_TIME_STAMP_T	beforeTime;		/* time before ncp */
	NWFI_TIME_STAMP_T	afterTime;		/* time after ncp */
	int			stale;			/* TRUE => file size */
							/* not commited */
} NWFS_CACHE_INFO_T;

/*
 * Define NWfsClientHandle object structure mask.
 */
typedef struct NWfsClientHandle {
	/*
	 * The following structure must come first. The
	 * ``clientHandleList'' and ``handleState'' are mutexed by the
	 * SNODE_LOCK of the associated server node. The ``namesList'' is
	 * mutexed by the CHANDLE_NAME_LOCK.
	 */
	struct NWfsNodeHeader	header;

	/*
	 * The following are mutexed by the nameLock.
	 */
	NWFI_LOCK_T		nameLock;
	uint16			nameStamp;

	/*
	 * The following are mutexed by the SNODE_LOCK of the
	 * associated snode. 
	 *
	 *	The NWCH_DESTROY bit of the clientHandleFlags is
	 *	protected by both the SNODE_LOCK and the nameLock.
	 *	Both locks must be held to write. Either lock can be
	 *	held to read.
	 */
	uint16			holdCount;
	uint16			resourceHoldCount;
	uint16			clientHandleFlags;
	uint16			readersCount;
	NWFI_NODE_T		*cloneVnode;

	/*
	 * The following fields are constant for the valid life of the
	 * NWfsClientHandle structure.
	 */
	struct NWfsServerNode	*snode;
	NWFS_CRED_T		credentials;

	/*
	 * NWsi resource handle information.
	 *
	 * The following are mutexed by the SNODE_LOCK of the
	 * associated snode.
	 */
	struct NWslHandle	*resourceHandle;

	/*
	 * Cached attribute information.
	 *
	 *	The following are mutexed by the SNODE_LOCK lock of the
	 *	associated snode.
	 *
	 *	Note: The userId and groupId stored below represent the
	 *	      client's view of the ownership of the file. This
	 *	      differs from the userId/groupId stored in the
	 *	      credentials (above), which represent the identity of
	 *	      the client.
	 */
	uint32			userId;
	uint32			groupId;
	uint32			nodePermissions;
	NWFS_CACHE_INFO_T	cacheInfo;

	/*
	 * Protected by NWfi-layer locking.  (E.g., VOP_RWWRLOCK in UnixWare
	 * 2.0.  Note that this is implemented using snodeRwLock, but the
	 * code here makes no such assumptions.)
	 */
	NWFI_LIST_T		flockCacheChain;

	/*
	 * Written only while NWfi-layer lock, e.g. VOP_RWWRLOCK, and
	 * SNODE_LOCK are both held.  May be read holding either lock.
	 */
	uint32			flockCacheLen;
#if defined(DEBUG) || defined (DEBUG_TRACE)
	int			inflated;
#endif
} NWFS_CLIENT_HANDLE_T;

/*
 * Convenient names
 */
#define clientHandleList	header.chain[0]
#define namesList		header.chain[1]
#define handleState		header.headerState

/*
 * Macros to Translate Between the client handle structure and
 * client/timer Chains.
 */
#define clientChainToClient(ch)	((NWFS_CLIENT_HANDLE_T *)(ch))
#define clientToClientChain(c)	(&((c)->clientHandleList))

/*
 * clientHandleFlags bits
 */
#define NWCH_AT_VALID		(1 << 0)	/* attributes valid */
#define NWCH_RH_CREATING	(1 << 1)	/* opening resource handle */
#define NWCH_RH_DRAINING	(1 << 2)	/* closing resource handle */
#define NWCH_RH_CMOC		(1 << 3)	/* chmod on close */
#define NWCH_WRITE_FAULT	(1 << 4)	/* write fault occured */
#define NWCH_DATA_DIRTY		(1 << 5)	/* user wrote data */
#define NWCH_RH_WRITE		(1 << 6)	/* handle is write enabled */
#define NWCH_RH_NEWLY_CREATED	(1 << 7)	/* handle created by */
						/* NWsiCreateNode */
#define NWCH_DESTROY		(1 << 10)	/* snode is stale */

#define NWCH_RH_INTRANS		(NWCH_RH_CREATING|NWCH_RH_DRAINING)

/*
 * Is this the client handle embedded in the snode?
 */
#define NWCH_EMBEDDED(hp)	(&((hp)->snode->clientHandle) == (hp))		

/*
 * For convenient ASSERTs.
 */
#define CHANDLE_HELD(hp)	((hp)->holdCount != 0)
#define CHANDLE_REASONABLE(hp)	((hp) != NULL &&			\
				 (hp)->handleState == SNODE_CHANDLE &&	\
				 CHANDLE_HELD(hp))

/*
 * locking operations
 */
#define CHANDLE_NAME_LOCK(chp)		NWFI_LOCK(&(chp)->nameLock)
#define CHANDLE_NAME_UNLOCK(chp)	NWFI_UNLOCK(&(chp)->nameLock)

#if defined(DEBUG) || defined(DEBUG_TRACE)

#define FLOCK_CACHE_LEN_INFLATE(chp)			\
	((void)(					\
		SNODE_LOCK((chp)->snode),		\
		(chp)->inflated++,			\
		(chp)->flockCacheLen++,			\
		SNODE_UNLOCK((chp)->snode)		\
	))
#define FLOCK_CACHE_LEN_DEFLATE(chp)			\
	((void)(					\
		SNODE_LOCK((chp)->snode),		\
		(chp)->inflated--,			\
		(chp)->flockCacheLen--,			\
		SNODE_UNLOCK((chp)->snode)		\
	))
#define FLOCK_CACHE_LEN(chp)	((chp)->flockCacheLen - (chp)->inflated)

#else

#define FLOCK_CACHE_LEN_INFLATE(chp)			\
	((void)(					\
		SNODE_LOCK((chp)->snode),		\
		(chp)->flockCacheLen++,			\
		SNODE_UNLOCK((chp)->snode)		\
	))
#define FLOCK_CACHE_LEN_DEFLATE(chp)			\
	((void)(					\
		SNODE_LOCK((chp)->snode),		\
		(chp)->flockCacheLen--,			\
		SNODE_UNLOCK((chp)->snode)		\
	))
#define FLOCK_CACHE_LEN(chp)	((chp)->flockCacheLen)

#endif

#define FLOCK_CACHE_ACTIVE(chp)	((chp)->flockCacheLen)

/*
 * Function prototypes.
 */
struct NWfsServerNode;
struct NWfsName;
struct NWnameSpace;
struct NWfsServerNode;
struct NWslHandle;
void NWfsHoldClientHandle(NWFS_CLIENT_HANDLE_T *);
NWFS_CLIENT_HANDLE_T * NWfsGetClientHandle(struct NWfsServerNode *,
	NWFS_CRED_T *);
ccode_t NWfsAttachClientHandle(struct NWfsServerNode *, NWFS_CLIENT_HANDLE_T **,
	 uint32);
void NWfsReleaseClientHandle(NWFS_CLIENT_HANDLE_T *);
ccode_t NWfsOpenResourceHandle(NWFS_CLIENT_HANDLE_T *, uint32, int, int, int *,
	 enum NUC_DIAG *);
void NWfsCacheResourceHandle(NWFS_CLIENT_HANDLE_T *, struct NWslHandle *,
	uint32);
void NWfsReleaseResourceHandle(NWFS_CLIENT_HANDLE_T *);
void NWfsCloseCachedResourceHandle(NWFS_CLIENT_HANDLE_T *);
void NWfsCloseCachedResourceHandles(struct NWfsServerNode *);
void NWfsCloseResourceHandle(NWFS_CLIENT_HANDLE_T *);
ccode_t NWfsCloseAllHandles(struct NWfsServerNode *, enum NUC_DIAG *);
void NWfsAllowResourceHandlesToOpen(struct NWfsServerNode *);
void NWfsCacheName(NWFS_CLIENT_HANDLE_T *, struct NWfsName *,
	struct NWfsServerNode *);
struct NWfsServerNode * NWfsSearchByName(NWFS_CLIENT_HANDLE_T *,
	struct NWfsName *);
void NWfsDiscardChildNames(NWFS_CLIENT_HANDLE_T *);
void NWfsUpdateClientAttributes(NWFS_CLIENT_HANDLE_T *, struct NWnameSpace *,
	 NWFS_CACHE_INFO_T *);
boolean_t NWfsInvalidateChandles(struct NWfsServerNode *);
void NWfsStaleChandles(struct NWfsServerNode *);
void NWfsPurgeSnodeFromNC(struct NWfsServerNode *, struct NWfsServerNode *);

struct NUCfsLock;
struct NUCfsFlockCache;
struct NUCfsFlockCache *NWfsChandleFlockHold(NWFS_CLIENT_HANDLE_T *,
					     struct NUCfsLock *);
void NWfsChandleCacheRelease(NWFS_CLIENT_HANDLE_T *, struct NUCfsFlockCache *,
			     uint32);
void NWfsChandleCacheRelease_l(NWFS_CLIENT_HANDLE_T *, struct NUCfsFlockCache *,
			       uint32);

#if defined(__cplusplus)
	}
#endif

#endif /* _FS_NUCFS_NWFSHANDLE_H */
