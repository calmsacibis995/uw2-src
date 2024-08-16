/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nwfsflushd.c	1.10"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nwfsflushd.c,v 2.1.2.10 1995/01/29 22:48:40 stevbam Exp $"

#include <util/types.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/requester.h>
#include <net/nuc/spilcommon.h>
#include <fs/vnode.h>
#include <fs/buf.h>
#include <fs/nucfs/nwfschandle.h>
#include <fs/nucfs/nucfscommon.h>
#include <fs/nucfs/nwficommon.h>
#include <fs/nucfs/nwfsops.h>
#include <fs/nucfs/nwfidata.h>
#include <fs/nucfs/nwfsvolume.h>
#include <fs/nucfs/nwfsnode.h>
#include <fs/nucfs/nucfsglob.h>
#include <util/ksynch.h>
#include <util/debug.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <proc/user.h>
#include <proc/lwp.h>
/*
 * Define the tracing mask.
 */
#define NVLT_ModMask	NVLTM_fs

void	nuc_pagepushd(NWFI_VOLFLUSH_DATA_T *);
void 	nuc_attflushd(NWFI_VOLFLUSH_DATA_T *);

STATIC	NWFI_BUF_T *NWfsGetNextIOBuf(NWFI_BUF_T *);
STATIC	void
NWfsVolumeAttFlush(struct NWfsServerVolume *, struct NWfsServerNode *);

#define	VOL_ATTFLUSH_WAKE(volP)	\
	NWFI_EV_BROADCAST(&((volP)->volFlushData->attFlushEvent))

#define	VOL_PAGEPUSH_WAKE(volP) \
	NWFI_EV_BROADCAST(&((volP)->volFlushData->pagePushEvent))

NWFI_CLOCK_T	nwfsNodeCacheQuantum, nwfsRHCacheQuantum;
/*
 * Macros to get to volume snode chains.
 */
#define	TIMED_LISTP(x)		(&((x)->timedNodeList))
#define	ACTIVE_LISTP(x)		(&((x)->activeNodeList))
#define	CACHE_LISTP(x)		(&((x)->cacheNodeList))

/*
 * short-hand for snode->snodeTimeStamp and &snode->cacheInfo.afterTime
 */
#define	SNODE_TIMESTAMP(x)	NWFI_CLOCK_READ((x)->snodeTimeStamp)
#define	SNODE_ATTRTIMEP(x)	(&((x)->cacheInfo.beforeTime))



/*
 * void
 * nucfs_timeout()
 *	The purpose of this timeout function is to signal the attribute 
 *	flush daemons for each of the volumes, if the volume has nodes
 *	that are ripe for processing.
 */
/* ARGSUSED */
void 
nucfs_timeout(void *arg)
{
	NWFS_SERVER_VOLUME_T 	*volP;
	NWFI_LIST_T		*listP;
	NWFS_SERVER_NODE_T	*snode;
	NWFI_LIST_T		*timedListAnchor, *nextLink;
	uint32			allocated;
	int			nucfsReleaseWake = 1;


	/*
	 * NUCFS_LIST_LOCK provides the guarantee that the 
	 * NWfsMountedVolumeList chain is stable, and that
	 * server nodes on timedNode chains of each volume can
	 * be examined.
	 */
	NUCFS_LIST_LOCK();
	/* 
	 * compute the nwfsNodeCacheQuantum time (in ticks).
	 *
	 * XXX: We could make the set of nwfsNodeCacheQuantum values 
	 *	tunable, but *	a simpler approach may be to pick one 
	 *	of these four values, based on allocated and permitted 
	 *	node counts:
	 *	(1) INT_MAX		if allocated < (permitted * 1/2)
	 *	(2) NWfiStaleTicks/2	if allocated < (permitted * 3/4)
	 *      (3) NWfiStaleTicks/4    if allocated < permitted
	 *	(4) 0			otherwise.
	 */
	nwfsNodeCacheQuantum = NWfiStaleTicks;
	nucfsDesperate = 0;
	allocated = nwfsActiveNodeCount + nwfsStaleNodeCount 
			+ nwfsCacheNodeCount;
	if (allocated >= (nwfsMaxNodes/2)) {
		nwfsNodeCacheQuantum /= 2;
		if (allocated >= (nwfsMaxNodes * 3)/4) {
			nucfsDesperate = 1;
			nwfsNodeCacheQuantum /= 2;
			if (allocated >= nwfsMaxNodes) {
				/* recycle any inactive nodes! */
				nwfsNodeCacheQuantum = 0;
			}
		}
	} else {
		nwfsNodeCacheQuantum = INT_MAX;
	}

	/*
	 * compute the resource handle cache aging interval
	 *
	 * PERF: For now, we allow the resource handle caching
	 * quantum to be either 0 or NWfiStaleTicks, depending
	 * on whether a resource shortage was noticed recently.
	 * We may want to have a more gradual variation in this
	 * caching quantum, driven by the degree of resource
	 * handle consumption.
	 */
	nwfsRHCacheQuantum = NWfiStaleTicks;	

	if (nucfsResourceShortage) {
		/*
		 * close out any cached resource handles possible.
		 */
		nwfsRHCacheQuantum = 0;		
		nucfsResourceShortage = 0;
	}
	

	for (listP = NWFI_NEXT_ELEMENT(&NWfsMountedVolumesList);
			listP != &NWfsMountedVolumesList;
			listP = NWFI_NEXT_ELEMENT(listP)) {
		volP = chainToServerVolume(listP);

		if (nucfsDesperate && volP->cacheCount) {
			NVLT_PRINTF(
				"\nnucfs_timeout: nucfsDesperate, volflush wake"
				" volume = 0x%x, volume index = %d\n",
				volP, volP->volFlushData->volumeIndex, 0);
			VOL_ATTFLUSH_WAKE(volP);
			nucfsReleaseWake = 0;
			continue;
		}
		timedListAnchor = TIMED_LISTP(volP);
		if (NWFI_LIST_EMPTY(timedListAnchor)) {
			continue;
		}

		/*
	 	 * The timed node list is maintained in FIFO order. So we
		 * examine the first node off the list, and see whether
		 * its timestamp is old enough.
		 */
		snode = volumeChainToSnode(NWFI_NEXT_ELEMENT(timedListAnchor));

		/* 
		 * Locate the first non-marker snode on the timed-chain.
		 */
		if (snode->nodeState == SNODE_MARKER) {
			while ((nextLink = NWFI_NEXT_ELEMENT(
				 	snodeToVolumeChain(snode))) 
					!= timedListAnchor) {
				snode = volumeChainToSnode(nextLink);
				if (snode->nodeState != SNODE_MARKER)
					break;
			}
			
			if (nextLink == timedListAnchor) {
				continue;
			}
		}
		NVLT_ASSERT(snode->nodeState == SNODE_TIMED);

		if (NWFI_CLOCK_AGED(SNODE_TIMESTAMP(snode), nwfsRHCacheQuantum) 
		|| NUCFS_CLOCK_STALE(volP->volFlushData->lastStaleCheckTime)) {
			NVLT_PRINTF(
				"\nnucfs_timeout: age/stale, volflush wake"
				" volume = 0x%x, volume index = %d\n",
				volP, volP->volFlushData->volumeIndex, 0);
			VOL_ATTFLUSH_WAKE(volP);
			nucfsReleaseWake = 0;
		}
	}
	NUCFS_LIST_UNLOCK();

	if (nucfsReleaseWake) {
		/* 
		 * We did not wakeup any volume's recycling daemon.
		 * Issue a general wakeup to all LWPs waiting for
		 * resource recycling to happen, so that they may 
		 * retry anyway, instead of waiting indefinitely.
		 */
		NUCFS_RELEASE_WAKE();
	}

}

/* 
 * STATIC NWFI_BUF_T *
 * NWfsGetNextIOBuf(NWFI_BUF_T *aioList)
 *	Deque the next iobuf from the asyncIoList chain that is passed
 *	in, and return that buffer. If the chain is empty, then return
 *	NULL.
 * Calling/Exit State:
 *	The caller ensures that the chain is protected as necessary.
 */
STATIC	NWFI_BUF_T *
NWfsGetNextIOBuf(NWFI_BUF_T *aioList)
{
	NWFI_BUF_T *bufP;

	NVLT_ENTER(1);

	if (NWFI_BUF_EMPTY(aioList)) 
		return NULL;
	bufP = (NWFI_BUF_T *)NWFI_BUF_FORW(aioList);

	NWFI_BUF_REMOVE(bufP);
	NVLT_LEAVE((uint_t)bufP);
	return(bufP);
}

STATIC	char asciiTable[]={'0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};

STATIC void
nuc_itoa(int i, char *str)
{
	int j;
	int strpos = 5;
	
	str[strpos--] = 0;

	j = i;
	while (strpos >= 0) {
		str[strpos--] = asciiTable[(j % 10)];
		j /= 10;
	}
}

void
nuc_pagepushd(NWFI_VOLFLUSH_DATA_T *vfdP)
{
	NWFS_SERVER_VOLUME_T	*volP;
	NWFI_BUF_T *bp;
	char myIndex[40];
	char *lwpName = NULL;
	
	NVLT_ENTER(1);

	if ((lwpName = kmem_alloc(LWP_NAMESTRING_LEN, KM_NOSLEEP)) != NULL) {
		nuc_itoa(vfdP->volumeIndex, myIndex);
		strcpy(lwpName, "nuc_pgout");
		strcat(lwpName, myIndex);
		u.u_lwpp->l_name = lwpName;
	} else {
		u.u_lwpp->l_name = "nuc_pgout";
	}
	

	/*
	 * after creation, the daemon should wait until activated.
	 */
	NWFI_EV_WAIT(&vfdP->pagePushEvent);
	volP = vfdP->serverVolume;
	for(;;) {
		if (vfdP->flags & VOL_FLUSH_DEINIT) {
			NVLT_ASSERT(volP == NULL ||
				    NWFI_BUF_EMPTY(&(volP->asyncIoList)));
			NVLT_VLEAVE();
			if (lwpName) {
				u.u_lwpp->l_name = "nuc_pgout";
				kmem_free(lwpName,  LWP_NAMESTRING_LEN);
			}
			lwp_exit();
		}
		NUCFS_LIST_LOCK();
		while ((bp = NWfsGetNextIOBuf(&(volP->asyncIoList))) 
				!= NULL) {
			NUCFS_LIST_UNLOCK();
			(void)do_nuc_bio((struct buf *)bp);
			NUCFS_LIST_LOCK();
		}
		NUCFS_LIST_UNLOCK();
		NWFI_EV_WAIT(&vfdP->pagePushEvent);
	}
}


/*
 * void
 * nuc_attflushd(NWFI_VOLFLUSH_DATA_T *vfdP)
 *
 */
void
nuc_attflushd(NWFI_VOLFLUSH_DATA_T *vfdP)
{
	NWFS_SERVER_VOLUME_T	*volP;
	NWFS_SERVER_NODE_T	*privateMarker;
	int i;
	char myIndex[40];
	char *lwpName = NULL;
	
	NVLT_ENTER(1);

	if ((lwpName = kmem_alloc(LWP_NAMESTRING_LEN, KM_NOSLEEP)) != NULL) {
		nuc_itoa(vfdP->volumeIndex, myIndex);
		strcpy(lwpName, "nuc_flush");
		strcat(lwpName, myIndex);
		u.u_lwpp->l_name = lwpName;
	} else {
		u.u_lwpp->l_name = "nuc_flush";
	}

	/*
	 * after creation, the daemon should wait until activated.
	 */
	NWFI_EV_WAIT(&vfdP->attFlushEvent);
	volP = vfdP->serverVolume;

	SNODE_CREATE_MARKER(&privateMarker);

	for(;;) {
		if (vfdP->flags & VOL_FLUSH_DEINIT) {
			NVLT_VLEAVE();
			SNODE_DESTROY_MARKER(privateMarker);
			if (lwpName) {
				u.u_lwpp->l_name = "nuc_flush";
				kmem_free(lwpName,  LWP_NAMESTRING_LEN);
			}
			lwp_exit();
		}
		if (NUCFS_CLOCK_STALE(vfdP->lastStaleCheckTime)) {
			/* Release stale attributes and names */
			for (i = 0; i < NUCFS_NODE_LIST_BUCKETS; ++i) {
				NWfsForEachSnode(
					&volP->idHashTable[i],
					NWfsReleaseStaleCachingHolds,
					privateMarker);
			}
			NWFI_GET_CLOCK(vfdP->lastStaleCheckTime); 
		}

		NWfsVolumeAttFlush(volP, privateMarker);
		/*
		 * next, harvest old cached nodes 
		 */
		if (nwfsNodeCacheQuantum != INT_MAX) {
			NUCFS_LIST_LOCK();
			NWfsVolumeCacheRecycle(volP, volP->cacheCount);
			NUCFS_LIST_UNLOCK();
		}
		NWFI_EV_WAIT(&vfdP->attFlushEvent);
	}/* for (;;) */

}

void
NWfsVolumeAttFlush(struct NWfsServerVolume *volP, 
		struct NWfsServerNode *privateMarker)
{
	NWFI_LIST_T *timedListAnchor, *nextLink;
	struct NWfsServerNode *snode;
	enum NUC_DIAG diagnostic;
	int error;

	NVLT_ENTER(2);

	timedListAnchor = TIMED_LISTP(volP);
	NUCFS_LIST_LOCK();
	nextLink = NWFI_NEXT_ELEMENT(timedListAnchor);
	while (nextLink != timedListAnchor) {
		snode = volumeChainToSnode(nextLink);
		/*
		 * Skip markers, etc.
		 */
		if (snode->nodeState != SNODE_TIMED) {
			NVLT_ASSERT(snode->nodeState == SNODE_MARKER);
			nextLink = NWFI_NEXT_ELEMENT(nextLink);
			continue;
		}
		SNODE_LOCK(snode);
		NVLT_ASSERT(snode->hardHoldCount != 0);
		NVLT_ASSERT((snode->nodeType != NS_ROOT) &&
			(snode->nodeType != NS_DIRECTORY));
		/*
		 * Is this snode aged enough?
		 */
		if (!NWFI_CLOCK_AGED(SNODE_TIMESTAMP(snode), 
					nwfsRHCacheQuantum) ) {
			/* 
			 * this node has not expired either of its
			 * attribute flush or resource handle caching
			 * intervals, so we need do nothing. 
			 *
			 * assuming the timedNode list is built up 
			 * in time order, we can stop processing the
			 * list at this point.
			 */
			SNODE_UNLOCK(snode);
			break;
		}
		/*
		 * either one or both of the attribute caching and
		 * resource handle caching intervals are up. We will
		 * process the node at this point. Even if the node
		 * has only expired the resource handle caching interval,
		 * we will flush attributes if it is mmapp'ed since
		 * we can do so at a relatively less cost before closing
		 * the resource handles.
		 */
		SNODE_SOFT_HOLD_L(snode);
		/*
		 * insert marker
		 */
		NWfiInsque(snodeToVolumeChain(privateMarker),
			snodeToVolumeChain(snode));
		/*
		 * take the snode back to the active list. we will
		 * bring it to timed list, if necessary, later.
		 */
		SNODE_TIMED_TO_ACTIVE(snode);

		/*
		 * Sync any dirty data to the disk before closing the
		 * file handles.
		 */
		SNODE_UNLOCK(snode);
		NUCFS_LIST_UNLOCK();
		SNODE_WR_LOCK(snode);
		error = NWfiSyncWithServer(
			snode, 
			0, 
			NULL, /* credentials */
			&diagnostic);
		SNODE_RW_UNLOCK(snode);
		if (error) {
			/*
			 * Can't just throw away the error.
			 * So store it in the snode for later return
			 * to the user.
			 */
			SNODE_LOCK(snode);
			if (snode->asyncError != 0)
				snode->asyncError = error;
			SNODE_UNLOCK(snode);
		}

		/*		
		 * following unprotected check of openResourceCount
		 * is okay, because if we find the openResourceCount
		 * to be equal to 0 but it is not really 0 because
		 * we just happened to race with an openResourceHandle(),
		 * then it means that the handle is very very young 
		 * anyway. If there is really an acute resource shortage,
		 * we will wind up processing this node a short while later
		 * during the next activation of the daemon.
		 *
		 * If on the other hand we mistakenly conclude that there
		 * are open resource handles when there are not any, we will
		 * only wind up closing cached client handles. This should
		 * be rare anyway, so performance is not an issue.
		 */
		if (snode->openResourceCount > 0 &&
		    NWfsCloseAllHandles(snode, &diagnostic) == SUCCESS)
			NWfsAllowResourceHandlesToOpen(snode);

		SNODE_LOCK(snode);
		NVLT_ASSERT( (snode->hardHoldCount > 0) || 
				( (snode->r_mapcnt == 0) &&
			  	  (snode->nodeState != SNODE_TIMED) &&
			  	  (snode->nodeState != SNODE_ACTIVE) ));

		if (snode->nodeState == SNODE_ACTIVE) {
                        if (snode->r_mapcnt ||
				(snode->openResourceCount != 0)) {

				/*
				 * mapped files will need to be resync'ed
				 * again and again. Alternatively, if the
				 * snode has open resource handles, we may
				 * need to process it again. So place it at
				 * the tail of the timedNodeList.
				 */
				SNODE_ACTIVE_TO_TIMED(snode);
			}
		}
		SNODE_UNLOCK(snode);
		SNODE_SOFT_RELEASE(snode); 
		NUCFS_LIST_LOCK();
		nextLink = NWFI_NEXT_ELEMENT(snodeToVolumeChain(privateMarker));
		NWfiRemque(snodeToVolumeChain(privateMarker));
	} /* for (;;) : the timedNode list */
	NUCFS_LIST_UNLOCK();
	NVLT_VLEAVE();
	return;
}

void
NWfsVolumeCacheRecycle(struct NWfsServerVolume *volP, int howMany)
{
	NWFI_LIST_T *nextLink, *cacheListAnchor;
	struct NWfsServerNode *snode;
	NWFS_SERVER_NODE_T *marker;
	int i;
	int recycled = 0;

	NVLT_ENTER(2);

	cacheListAnchor = CACHE_LISTP(volP);
start:
	nextLink = NWFI_NEXT_ELEMENT(cacheListAnchor);
	NVLT_ASSERT((nextLink != cacheListAnchor) || (volP->cacheCount == 0));
	while ((nextLink != cacheListAnchor) && (recycled < howMany)) {
		snode = volumeChainToSnode(nextLink);
		if (snode->nodeState == SNODE_MARKER) {
			nextLink = NWFI_NEXT_ELEMENT(nextLink);
			continue;
		}
		NVLT_ASSERT(snode->nodeState == SNODE_INACTIVE);
		if (!NWFI_CLOCK_AGED(SNODE_TIMESTAMP(snode), 
				nwfsNodeCacheQuantum)) {
			break;
		}
		/*
	 	 * this node is old enough to recycle.
		 */
		if (snode->nameCacheSoftHolds > 0) {
			NUCFS_LIST_UNLOCK();
			SNODE_CREATE_MARKER(&marker);
			for (i = 0; i < NUCFS_NODE_LIST_BUCKETS; ++i) {
				NWfsForEachSnode(
					&volP->idHashTable[i],
					NWfsNameCacheRelease,
					marker);
			}
			SNODE_DESTROY_MARKER(marker);
			/*
			 * yield to others, since the above name cache
			 * releases may have taken much time.
			 */
			delay((HZ + (100 -1))/100);
			NUCFS_LIST_LOCK();
			goto start;
		}
		SNODE_DESTROY_IDENTITY(snode, volP, cacheCount);
		--nwfsCacheNodeCount;
		NUCFS_LIST_UNLOCK();
		/* 
		 * When the identity was destroyed, we 
		 * inherited the softHold on the snode. 
		 */
		NWfsDestroyNode(snode);
		NUCFS_LIST_LOCK();
		nextLink = NWFI_NEXT_ELEMENT(cacheListAnchor);
	} 
	NUCFS_RELEASE_WAKE();

	NVLT_VLEAVE();
	return;
}
