/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/flock_cache.c	1.5"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/flock_cache.c,v 2.1.2.4 1995/01/29 20:32:03 mdash Exp $"

/*
 * Definitions for the NUCFS lock request cache.  Caller is responsible for
 * mutual exclusion and initialization of all lists.  To handle simple, common
 * cases easily, we use only an unsorted list of cache entries.  This data
 * structure supports efficent searches of small lock sets, the common case.
 * If large sets turn out to be interesting, a new data structure may be
 * used.
 *
 * NOTE:  In all cases, argument lists must be initialized by callers.
 */

#include <util/types.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <mem/kmem.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/requester.h>
#include <net/nuc/spilcommon.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <fs/nucfs/nucfs_tune.h>
#include <fs/nucfs/nwfschandle.h>
#include <fs/nucfs/nucfscommon.h>
#include <fs/nucfs/nucfsspace.h>
#include <fs/nucfs/nwfsvolume.h>
#include <fs/nucfs/nwfsnode.h>
#include <fs/nucfs/nucfsglob.h>
#include <fs/nucfs/nwfimacro.h>
#include <fs/nucfs/nwfsname.h>
#include <fs/nucfs/nwfsops.h>
#include <fs/nucfs/nwfidata.h>
#include <fs/nucfs/nwficommon.h>
#include <fs/nucfs/flock_cache.h>
#include <fs/nucfs/nwfslock.h>

#define NVLT_ModMask	NVLTM_fs

/*
 * Let end = base + len.  Then extent1 intersects extent2 iff
 *	!(base1 >= end2 || base2 >= end1)
 *		==
 *	base1 < end2 && base2 < end1
 */
#define INTERSECTS(b1, e1, b2, e2)	((b1) < (e2) && (b2) < (e1))

/*
 * Update destinationList with elements from sourceList that intersect the
 * range described by lock.  Further match criteria: (1) matchPid == B_TRUE for
 * matching on lockPid, and (2) lockType == type of lock to match on, or
 * NWFS_NO_LOCK to ignore type.  Elements added to destinationList are removed
 * from sourceList.  Takes no locks.
 */
void
NWfsFlockCacheExtractRange(
	NWFI_LIST_T	*sourceList,
	NUCFS_LOCK_T	*lock,
	boolean_t	matchPid,
	uint16		lockType,
	NWFI_LIST_T	*destinationList)
{
	NWFI_LIST_T		*chain;
	NUCFS_FLOCK_CACHE_T	*cache;

	NVLT_ENTER(5);
	chain = NWFI_NEXT_ELEMENT(sourceList);
	while (chain != sourceList) {
		cache = CHAIN_TO_CACHE(chain);
		chain = NWFI_NEXT_ELEMENT(chain);
		if (!INTERSECTS(cache->cacheOffset, cache->cacheEnd,
				lock->lockOffset, lock->lockEnd) ||
		    matchPid && !NWfsFlockPidFind(cache, lock->lockPid) ||
		    lockType != NWFS_NO_LOCK && cache->cacheType != lockType)
			continue;
		NWfiRemque(&cache->cacheChain);
		NWfiInsque(&cache->cacheChain, destinationList);
	}
	NVLT_VLEAVE();
}

/*
 * For all elements in intersectingRequests that overhang (extend beyond) the
 * range described by lock, establish a new lock covering the overhang.  Does
 * not update intersectingRequests or its contents.  If
 * NWfs(Set|Remove)FileLock call here, recursion must be limited by first
 * extracting intersectingRequests.  Updates chandle->flockCacheChain and
 * nFlocksCached in the associated server node.
 */
ccode_t
NWfsFlockCacheClip(
	NWFS_CLIENT_HANDLE_T	*chandle,
	uint32			accessFlags,
	NUCFS_LOCK_T		*lock,
	NWFI_LIST_T		*intersectingRequests,
	enum NUC_DIAG		*diag)
{
	NUCFS_FLOCK_CACHE_T	*cache;
	NWFI_LIST_T		overhangList, *chain;
	NUCFS_LOCK_T		newLock;
	ccode_t			result = SUCCESS;
	enum NUC_DIAG		sinkDiag;

	NVLT_ENTER(5);
	NVLT_ASSERT(CHANDLE_REASONABLE(chandle));

	NWFI_LIST_INIT(&overhangList);
	chain = NWFI_NEXT_ELEMENT(intersectingRequests);
	while (chain != intersectingRequests) {
		cache = CHAIN_TO_CACHE(chain);
		NVLT_ASSERT(NWfsFlockPidFind(cache, lock->lockPid));
		chain = NWFI_NEXT_ELEMENT(chain);
		if (cache->cacheOffset < lock->lockOffset) {
			newLock = cache->cacheState;	/* struct copy */
			newLock.lockEnd = lock->lockOffset;
			result = NWfsSetFileLock(chandle, accessFlags,
						 &newLock, diag);
			if (result != SUCCESS)
				break;
			NWfsFlockCacheHold(&overhangList, &newLock);
		}
		if (cache->cacheEnd > lock->lockEnd) {
			newLock = cache->cacheState;
			newLock.lockOffset = lock->lockEnd;
			result = NWfsSetFileLock(chandle, accessFlags,
						 &newLock, diag);
			if (result != SUCCESS)
				break;
			NWfsFlockCacheHold(&overhangList, &newLock);
		}
	}
	if (result != SUCCESS && NWFS_LOCK_BLOCKED(*diag)) {

		/*
		 * We should never block now because we already have the range
		 * locked.
		 */
		result = FAILURE;
		*diag = NUCFS_PROTOCOL_ERROR;
		cmn_err(CE_NOTE, "NWfsFlockCacheOverhangLock: "
			"blocked relocking, diag %d", diag);
	}

	/*
	 * Entries on the overhang list now also contain analogs on the server
	 * and in the chandle lock state.  Dispose of the overhang list.  If
	 * we had an error, try also to delete the lock from the server and
	 * from the chandle.
	 */
	chain = NWFI_NEXT_ELEMENT(&overhangList);
	while (chain != &overhangList) {
		cache = CHAIN_TO_CACHE(chain);
		chain = NWFI_NEXT_ELEMENT(chain);
		if (result != SUCCESS) {
			cache->cacheCommand = NWFS_REMOVE_LOCK;
			(void)NWfsRemoveFileLock(chandle, accessFlags,
						 &cache->cacheState, &sinkDiag);
		}
		NWfsFlockCacheRelease(cache, lock->lockPid);
	}
	return NVLT_LEAVE(result);
}

/*
 * Move the contents of sourceList to destinationList.
 */
void
NWfsFlockCacheSplice(
	NWFI_LIST_T	*destinationList,
	NWFI_LIST_T	*sourceList)
{
	NVLT_ENTER(2);

	/*
	 * There doesn't seem to be an efficient way to do this without peeking
	 * at the list or otherwise observing the special case of list empty.
	 */
	if (NWFI_LIST_EMPTY(sourceList)) {
		NVLT_VLEAVE();
		return;
	}
	NWFI_NEXT_ELEMENT(NWFI_PREV_ELEMENT(sourceList)) =
		NWFI_NEXT_ELEMENT(destinationList);
	NWFI_PREV_ELEMENT(NWFI_NEXT_ELEMENT(destinationList)) =
		NWFI_PREV_ELEMENT(sourceList);
	NWFI_PREV_ELEMENT(NWFI_NEXT_ELEMENT(sourceList)) = destinationList;
	NWFI_NEXT_ELEMENT(destinationList) = NWFI_NEXT_ELEMENT(sourceList);
	NWFI_LIST_INIT(sourceList);
	NVLT_VLEAVE();
}

/*
 * Release pid's hold on cache.  It is an error to call with a pid that
 * does not have a hold.
 */
void
NWfsFlockCacheRelease(
	NUCFS_FLOCK_CACHE_T	*cache,
	uint32			pid)
{
	NUCFS_FLOCK_PID_T	*flockPid;
	NWFI_LIST_T		*chain;

	NVLT_ENTER(2);
	NVLT_ASSERT(cache->cachePidCount);
	NVLT_ASSERT((cache->cachePidCount == 1) ==
		     NWFI_LIST_EMPTY(&cache->cachePidChain));
	if (cache->cachePidCount == 1) {
		NVLT_ASSERT(cache->cachePid == pid);
		NWfiRemque(&cache->cacheChain);
		kmem_free(cache, sizeof(*cache));
	} else {
		for (chain = NWFI_NEXT_ELEMENT(&cache->cachePidChain);
		     chain != &cache->cachePidChain;
		     chain = NWFI_NEXT_ELEMENT(chain)) {
			flockPid = CHAIN_TO_PID(chain);
			if (flockPid->pid == pid)
				break;
		}
		NVLT_ASSERT(chain != &cache->cachePidChain &&
			    flockPid->pid == pid);
		NWfiRemque(&flockPid->chain);
		kmem_free(flockPid, sizeof(*flockPid));
		if (--cache->cachePidCount == 1) {
			chain = NWFI_NEXT_ELEMENT(&cache->cachePidChain);
			flockPid = CHAIN_TO_PID(chain);
			cache->cachePid = flockPid->pid;
			NVLT_ASSERT(cache->cachePid != pid);
			NWfiRemque(&flockPid->chain);
			kmem_free(flockPid, sizeof(*flockPid));
		}
		NVLT_ASSERT((cache->cachePidCount == 1) ==
			     NWFI_LIST_EMPTY(&cache->cachePidChain));
	}
	NVLT_VLEAVE();
}

/*
 * Establish on behalf of lock->lockPid a hold on a NUCFS_FLOCK_CACHE_T
 * structure recording lock in list.  Redundant claims are ignored.
 */
NUCFS_FLOCK_CACHE_T	*
NWfsFlockCacheHold(
	NWFI_LIST_T		*list,
	NUCFS_LOCK_T		*lock)
{
	NUCFS_FLOCK_CACHE_T	*cache;
	NUCFS_FLOCK_PID_T	*flockPid;

	NVLT_ENTER(2);

	/*
	 * Search for an equivalent lock to add the reference to before
	 * deciding to allocate a new one.
	 */
	cache = NWfsFlockCacheFind(list, lock, B_FALSE, lock->lockType);
	if (!cache) {
		cache = kmem_alloc(sizeof(*cache), KM_SLEEP);
		cache->cachePidCount = 1;
		NWFI_LIST_INIT(&cache->cachePidChain);
		cache->cacheState = lock[0];
		NWfiInsque(&cache->cacheChain, list);
	} else if (!NWfsFlockPidFind(cache, lock->lockPid)) {
		NVLT_ASSERT(cache->cachePidCount);
		NVLT_ASSERT((cache->cachePidCount == 1) ==
			    NWFI_LIST_EMPTY(&cache->cachePidChain));
		if (cache->cachePidCount == 1) {

			/*
			 * Have to convert the original lockPid.
			 */
			flockPid = kmem_alloc(sizeof(*flockPid), KM_SLEEP);
			flockPid->pid = cache->cachePid;
			NWfiInsque(&flockPid->chain, &cache->cachePidChain);
		}
		cache->cachePidCount++;
		flockPid = kmem_alloc(sizeof(*flockPid), KM_SLEEP);
		flockPid->pid = lock->lockPid;
		NWfiInsque(&flockPid->chain, &cache->cachePidChain);
	}
	NVLT_LEAVE(SUCCESS);
	return cache;
}

/*
 * Return value == element in list with the same offset, length as lock, or
 * NULL.  Further match criteria: (1) matchPid == B_TRUE for matching on
 * lockPid, and (2) lockType == type of lock to match on, or NWFS_NO_LOCK to
 * ignore type.  Element is not removed from list.
 */
NUCFS_FLOCK_CACHE_T	*
NWfsFlockCacheFind(
	NWFI_LIST_T	*list,
	NUCFS_LOCK_T	*lock,
	boolean_t	matchPid,
	uint16		lockType)
{
	NUCFS_FLOCK_CACHE_T	*cache;
	NWFI_LIST_T		*chain;

	NVLT_ENTER(4);

	for (chain = NWFI_NEXT_ELEMENT(list); chain != list;
	     chain = NWFI_NEXT_ELEMENT(chain)) {
		cache = CHAIN_TO_CACHE(chain);
		if (cache->cacheOffset != lock->lockOffset ||
		    cache->cacheEnd != lock->lockEnd ||
		    matchPid && !NWfsFlockPidFind(cache, lock->lockPid) ||
		    lockType != NWFS_NO_LOCK && cache->cacheType != lockType)
			continue;
		NVLT_LEAVE(SUCCESS);
		return cache;
	}
	NVLT_LEAVE(FAILURE);
	return NULL;
}

boolean_t
NWfsFlockPidFind(
	NUCFS_FLOCK_CACHE_T	*cache,
	uint32			pid)
{
	NUCFS_FLOCK_PID_T	*flockPid;
	NWFI_LIST_T		*chain;
	boolean_t		result = B_FALSE;

	NVLT_ENTER(2);
	NVLT_ASSERT(cache->cachePidCount);
	NVLT_ASSERT((cache->cachePidCount == 1) ==
		    NWFI_LIST_EMPTY(&cache->cachePidChain));
	if (cache->cachePidCount == 1) {
		if (cache->cachePid == pid)
			result = B_TRUE;
	} else {
		for (chain = NWFI_NEXT_ELEMENT(&cache->cachePidChain);
		     chain != &cache->cachePidChain;
		     chain = NWFI_NEXT_ELEMENT(chain)) {
			flockPid = CHAIN_TO_PID(chain);
			if (flockPid->pid == pid) {
				result = B_TRUE;
				break;
			}
		}
	}
	return NVLT_LEAVE(result);
}

/*
 * If *nextPid is NULL, return the first pid in the list of pids with
 * references to cache.  Otherwise, return the pid of the element in the list
 * denoted by *nextPid.  Updates *nextPid with the next element, or NULL if
 * there is none.
 */
uint32
NWfsFlockPidNext(
	NUCFS_FLOCK_CACHE_T	*cache,
	NWFI_LIST_T		**nextPid)
{
	uint32			pid;
	NWFI_LIST_T		*chain;
	NUCFS_FLOCK_PID_T	*flockPid;

	NVLT_ENTER(2);
	if (cache->cachePidCount == 1) {
		pid = cache->cachePid;
		*nextPid = NULL;
	} else {
		chain = *nextPid;
		if (!chain)
			chain = NWFI_NEXT_ELEMENT(&cache->cachePidChain);
		flockPid = CHAIN_TO_PID(chain);
		pid = flockPid->pid;
		chain = NWFI_NEXT_ELEMENT(chain);
		if (chain == &cache->cachePidChain)
			*nextPid = NULL;
		else
			*nextPid = chain;
	}
	return NVLT_LEAVE(pid);
}

