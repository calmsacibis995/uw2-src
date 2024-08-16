/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/comops.c	1.9.1.23"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/comops.c,v 2.63.2.22 1995/02/13 07:43:14 stevbam Exp $"

/*
**  Netware Unix Client
**
**	MODULE:
**		comops.c -	The NetWare Client File System (NWfs) common
**				operations.
*/ 

#include <util/types.h>
#include <util/cmn_err.h>

#include <net/nuc/nwctypes.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/requester.h>
#include <fs/nucfs/nucfs_tune.h>
#include <net/tiuser.h>
#include <fs/nucfs/nwfschandle.h>
#include <fs/nucfs/nucfscommon.h>
#include <fs/nucfs/nucfsspace.h>
#include <net/nuc/spilcommon.h>
#include <fs/nucfs/nwfsvolume.h>
#include <fs/nucfs/nwfsname.h>
#include <fs/nucfs/nwfsnode.h>
#include <fs/nucfs/nucfsglob.h>
#include <fs/nucfs/nwfimacro.h>
#include <fs/nucfs/nwfidata.h>
#include <fs/nucfs/nwficommon.h>
#include <fs/nucfs/nwfsops.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <util/debug.h>

/*
 * Define the tracing mask.
 */
#define NVLT_ModMask	NVLTM_fs

/*
 * Internal functions.
 */
STATIC void NWfsInvalidateNameCache(NWFS_SERVER_NODE_T *);
STATIC void NWfsWaitForDestroy(NWFS_SERVER_NODE_T *);


int	flstats_nodeattr;
int	flstats_chattr;
int	flstats_namerele;

/*
 *	Deactivate a server node.
 *
 *	Since the
 */
void
NWfsInactiveServerNode(NWFS_SERVER_NODE_T *serverNode)
{
	int			doRelease = FALSE;
	NWFS_CLIENT_HANDLE_T	*delayDeleteClientHandle = NULL;
	NWFS_SERVER_VOLUME_T	*volp;

	NVLT_ENTER(1);

	NVLT_ASSERT(SNODE_REASONABLE(serverNode));

	volp = serverNode->nodeVolume;

	NUCFS_LIST_LOCK();
	SNODE_LOCK(serverNode);
	NVLT_ASSERT (serverNode->hardHoldCount != 0);
	if (--serverNode->hardHoldCount != 0) {
		/*
		 * Server node has been grabbed by someone else.
		 * It is no longer our responsibility to deactivate
		 * it.
		 */
		SNODE_UNLOCK(serverNode);
		NUCFS_LIST_UNLOCK();
		NVLT_VLEAVE();
		return;
	}

	NWFI_GET_CLOCK(volp->activeStamp);

	NVLT_ASSERT(!SNODE_HAS_FLOCK(serverNode));

	switch (serverNode->nodeState) {
	case SNODE_TIMED:
	case SNODE_ACTIVE:
		/*
		 * Remove from active list.
		 */
		NWfiRemque(snodeToVolumeChain(serverNode));
		--volp->activeOrTimedCount;
		--nwfsActiveNodeCount;

		if ((serverNode->softHoldCount == 1) ||
		    (serverNode->delayDeleteClientHandle != NULL)) {
			/*
			 * If any of the following conditions hold:
			 * - nothing is holding this node (other than
			 *   its identity)  
			 * - a delay delete is pending,
			 * then destroy its identity now.
			 */
			NWfiRemque(snodeToIdChain(serverNode));
			serverNode->nodeState = SNODE_STALE;
			doRelease = TRUE;
			++volp->staleCount;
			++nwfsStaleNodeCount;
		} else {
			/*
			 * This server node still has value to us.
			 * So place it on the cache node list.
			 */
			serverNode->nodeState = SNODE_INACTIVE;
			NWfiInsque(snodeToVolumeChain(serverNode),
				   NWFI_PREV_ELEMENT(&(volp->cacheNodeList)));
			/* record the time of enqueing onto cacheNodeList */
			NWFI_GET_CLOCK(serverNode->snodeTimeStamp);
			++volp->cacheCount;
			++nwfsCacheNodeCount;
		}

		/*
		 * Wipe out error state.
		 *
		 * XXX: We suppose that the user is not interested in this
		 *	error anyway.
		 */
		serverNode->asyncError = 0;
		break;
	default:
		NVLT_ASSERT(serverNode->nodeState == SNODE_STALE);
		if (serverNode->softHoldCount == 0) {
			doRelease = TRUE;
			SNODE_SOFT_HOLD_L(serverNode);
		}
		break;
	}

	/*
	 * Save the delay delete client.
	 */
	delayDeleteClientHandle = serverNode->delayDeleteClientHandle;
	serverNode->delayDeleteClientHandle = NULL;
	SNODE_UNLOCK(serverNode);
	NUCFS_LIST_UNLOCK();

	/*
	 * Complete a delayed deletion.
	 *
	 * The node to be deleted can actually become active at this point.
	 * Should this happen, then we are about to make a mistake (as far
	 * as the activating user is concerned). Luckily, the window in which
	 * this can happen is quite small.
	 */
	if (delayDeleteClientHandle != NULL) {
		NWfsStaleNode(serverNode);
		NWfsDeleteById(delayDeleteClientHandle);
		NWfsReleaseClientHandle(delayDeleteClientHandle);
	}

	if (doRelease)
		SNODE_SOFT_RELEASE(serverNode);

	NVLT_VLEAVE();
	return;
}

/*
 * BEGIN_MANUAL_ENTRY( NWfsFreeServerNode(3k), \
 *                     ./man/kernel/nucfs/nwfs/FreeServerNode )
 * NAME
 *    NWfsFreeServerNode - Frees the specified serverNode.
 *
 * SYNOPSIS
 *    void_t 
 *    NWfsFreeServerNode (NWFS_SERVER_NODE_T * serverNode)
 *
 * INPUT
 *    serverNode           - Server node to be freed.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUES
 *    None.
 *
 * DESCRIPTION
 *    The NWfsFreeServerNode frees the specified serverNode.
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
void
NWfsFreeServerNode (NWFS_SERVER_NODE_T *serverNode)
{
	NWFS_SERVER_VOLUME_T	*volp;

	NVLT_ENTER (1);

	NVLT_ASSERT(serverNode->hardHoldCount == 0);
	NVLT_ASSERT(serverNode->softHoldCount == 0);
	NVLT_ASSERT(serverNode->nameCacheSoftHolds == 0);
	NVLT_ASSERT(serverNode->nodeState == SNODE_STALE);

	/*
	 * At this point the server node is privately held by us, unless
	 * somebody is waiting in NWfsDestroyNode.
	 */
	SNODE_LOCK(serverNode);
	if (serverNode->nodeFlags & SNODE_DESTROY) {
		/*
		 * Somebody is awaiting destruction in NWfsDestroyNode.
		 */
		SNODE_UNLOCK(serverNode);
		SNODE_WAKEUP(serverNode);
		NVLT_VLEAVE();
		return;
	}
	SNODE_UNLOCK(serverNode);

        /*
         * De-initialize Snode locks
         */
	NWFI_SNODE_DEINIT(serverNode);

#if defined(DEBUG) || defined(DEBUG_TRACE)
	serverNode->nodeState = SNODE_DEAD;
#endif /* DEBUG || DEBUG_TRACE */

	/*
	 * remember the server volume.
	 */
	volp = serverNode->nodeVolume;

	/*
	 * Free the memory allocated for the serverNode.
	 */
	kmem_free (serverNode, sizeof (NWFS_SERVER_NODE_T));

	/*
	 * Now for some final accounting.
	 */
	NUCFS_LIST_LOCK();
	--volp->staleCount;
	--nwfsStaleNodeCount;
	NUCFS_LIST_UNLOCK();

	NVLT_VLEAVE();
	return;
}


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
 * BEGIN_MANUAL_ENTRY( NWfsAllocateNode(3k), \
 *                     ./man/kernel/nucfs/nwfs/AllocateNode )
 * NAME
 *    NWfsAllocateNode - Allocate and populate a server node object. 
 *
 * SYNOPSIS
 *    ccode_t
 *    NWfsAllocateNode (credentials, nodeVolume, nodeType, nodeName,
 *                      serverNode, diagnostic)
 *    opaque_t       *credentials;
 *    opaque_t       *nodeVolume;      Object is opaque to caller.
 *    uint32         nodeType;
 *    char           *nodeName;
 *    opaque_t       **serverNode;     Object is opaque to caller.
 *    enum  NUC_DIAG *diagnostic;
 *
 * INPUT
 *    credentials - Credentials of the UNIX client allocating the server node.
 *    nodeVolume  - Server volume object representing the volume the serverNode
 *                  object is to be allocated on.
 *    nodeType    - Set to one of the following object types:
 *                  NS_FILE
 *                  NS_DIRECTORY
 *                  NS_CHARACTER_SPECIAL  (Not in DOS Name Space)
 *                  NS_BLOCK_SPECIAL      (Not in DOS Name Space)
 *                  NS_FIFO               (Not in DOS Name Space)
 *                  NS_SYMBOLIC_LINK      (Not in DOS Name Space)
 *                  NS_ROOT
 *    nodeName    - Name of the node.
 *
 * OUTPUT
 *    serverNode  - Newly allocated server node object.
 *    diagnostic  - Set to one of the following if an error occurs:
 *                  NUCFS_ALLOC_MEM_FAILED
 *                  NUCFS_LLIST_OPERATION_FAILED
 *
 * RETURN VALUES
 *    0           - Successful completion.
 *    -1          - Unsuccessful completion, diagnostic contains reason.
 *
 * DESCRIPTION
 *    The NWfsAllocatNode allocates and populates a server node object. 
 *
 * NOTES
 *    The NWfsBindNode function must be called to bind the newly allocated 
 *    server node object with a Generic File System node (INODE, VNODE, etc).
 *
 *    The new server node can be added either to the mounted volume's cache
 *    server node list or the active server node list.
 *
 *    Also refer to the notes in the nwfsnode.h header file.
 *
 * SEE ALSO
 *    NWfsAllocateRootNode(3k), NWfsCreateFileNode(3k), NWfsLookUpNode(3k),
 *    NWfsReadDirNodeEntries(3k), and NWfsMakeDirNode(3k).
 *
 * END_MANUAL_ENTRY
 */

/* ARGSUSED */
ccode_t
NWfsCreateOrUpdateNode (
	NWFS_CLIENT_HANDLE_T	*parentClientHandle,
	NWFS_NAME_T		*hashName,
	NWFS_CACHE_INFO_T	*cacheInfo,
	NWSI_NAME_SPACE_T	*nameSpaceInfo,
	SPI_HANDLE_T		*resourceHandle,
	uint32			accessFlags,
	NWFS_SERVER_NODE_T 	**serverNode,
	enum	NUC_DIAG	*diagnostic)
{
	NWFS_SERVER_NODE_T	*newSnode = NULL;
	NWFS_SERVER_NODE_T	*parentSnode, *nextSnode;
	NWFS_SERVER_VOLUME_T	*volume;
	NWFS_CLIENT_HANDLE_T	*childClientHandle;
	NWFI_LIST_T		*hashBucket;
	int			allocLoopCount = 0;
	uint32			stamp;
	uint32			activeOrStale;	
	NWFI_LIST_T		*nodeList;

	NVLT_ENTER (8);

	NVLT_ASSERT (CHANDLE_REASONABLE(parentClientHandle));
	NVLT_ASSERT (parentClientHandle->holdCount != 0);
	NVLT_ASSERT (nameSpaceInfo != NULL);

	/*
	 * Get the volume from the parentClientHandle.
	 */
	parentSnode = parentClientHandle->snode;
	NVLT_ASSERT (SNODE_REASONABLE(parentSnode));
	NVLT_ASSERT(parentSnode->hardHoldCount != 0);
	volume = parentSnode->nodeVolume;
	NVLT_ASSERT(volume != NULL);

	/*
	 * Now compute the nodeId of the new server node.
	 */
	hashBucket =
		&volume->idHashTable[SNODE_HASH(nameSpaceInfo->nodeNumber)];
	NUCFS_LIST_LOCK();
retry:
	nextSnode = idChainToSnode(NWFI_NEXT_ELEMENT(hashBucket));

	/*
	 * Search for our snode on the bucket. However, be sure
	 * to skip over list markers.
	 */
	while (nextSnode != idChainToSnode(hashBucket)) {
		if ((nextSnode->nodeState != SNODE_MARKER) &&
		    (nextSnode->nodeNumber == nameSpaceInfo->nodeNumber)) {
			/*
			 * We just found the server node in the
			 * volume's hash table. Note that SNODE_STALE
			 * server nodes are kicked out of the hash table,
			 * so we can't find one here.
			 *
			 * We need to hold the found snode before we drop
			 * the list lock.
			 */
			NVLT_ASSERT(nextSnode->nodeState != SNODE_STALE);
			if (nextSnode->nodeState == SNODE_INACTIVE) {
				/*
				 * Move to active list.
				 */
				NWfiRemque(snodeToVolumeChain(nextSnode));
				NWfiInsque(snodeToVolumeChain(nextSnode),
					   &volume->activeNodeList);
				++nwfsActiveNodeCount;
				++volume->activeOrTimedCount;
				--nwfsCacheNodeCount;
				--volume->cacheCount;
				SNODE_LOCK(nextSnode);
				nextSnode->nodeState = SNODE_ACTIVE; 
				SNODE_UNLOCK(nextSnode);
			}
			NVLT_ASSERT(nextSnode->nodeState == SNODE_ACTIVE ||
				    nextSnode->nodeState == SNODE_TIMED);
			SNODE_HOLD(nextSnode);
			NUCFS_LIST_UNLOCK();

		        /*
			 * Test for mismatching nameSpaceInfo with the
			 * server node we just found. This indicates
			 * a lack of either persistence/uniqueness of
			 * the nodeNumber on the server.
			 *
			 * Also test for sizes which overflow our supported
			 * range.
			 */
			if (nextSnode->nodeType != nameSpaceInfo->nodeType ||
			    (NWFI_OFF_T) nameSpaceInfo->nodeSize < 0) {
				NWfsStaleNode(nextSnode);
				SNODE_RELEASE(nextSnode);
				/*
				 * Since we have already dropped the
				 * NUCFS_LIST_LOCK, we have no choice but
				 * to restart.
				 */
				NUCFS_LIST_LOCK();
				goto retry;
			}

			/*
			 * If we have allocated a new snode, then
			 * free it now.
			 */
			if (newSnode != NULL) {
#if defined(DEBUG) || defined(DEBUG_TRACE)
				newSnode->hardHoldCount = 0;
				newSnode->softHoldCount = 0;
				newSnode->nodeState = SNODE_STALE;
#endif /* DEBUG || DEBUG_TRACE */
				NUCFS_LIST_LOCK();
				++nwfsStaleNodeCount; 
				++newSnode->nodeVolume->staleCount;
				NUCFS_LIST_UNLOCK();
				NWfsFreeServerNode(newSnode);
			}

			/*
			 * We found a pre-existing node. See if we can
			 * update the attribute info within.
			 */
			NWfsUpdateNodeInfo (&parentClientHandle->credentials,
					    nameSpaceInfo, cacheInfo,
					    nextSnode);

			/*
			 * Now try to cache away the resource handle.
			 */
			if (resourceHandle != NULL) {
				childClientHandle =
					NWfsGetClientHandle(nextSnode,
					    &parentClientHandle->credentials);
				NWfsCacheResourceHandle(childClientHandle,
							resourceHandle,
							accessFlags);
				NWfsReleaseClientHandle(childClientHandle);
			}

			*serverNode = nextSnode;
			goto cacheTheName;
		}

		/*
		 * Onto the next snode.
		 */
		nextSnode = idChainToSnode(NWFI_NEXT_ELEMENT(
				snodeToIdChain(nextSnode)));
	}

	/*
	 * If we have already allocated a new snode, then
	 * add it to the hash chain and the active server node list. 
	 */

	if (newSnode != NULL)
		goto addNewSnode;

	/*
	 * We couldn't find the snode. So, allocate a new one.
	 *
	 * But, before we drop the NUCFS_LIST_LOCK, remember the
	 * insertion stamp so that we won't have to research the
	 * hash bucket again if there are no racing creaters.
	 */
	stamp = volume->idHashStamp;

	/*
	 * Allocate a new server node.
	 */
	activeOrStale = nwfsActiveNodeCount + nwfsStaleNodeCount;

	if (activeOrStale >= nwfsMaxNodes  || (allocLoopCount > 5)) {
		/*
		 * Already too many nodes allocated. Fail this
		 * request.
		 */
		NUCFS_LIST_UNLOCK();
		cmn_err(CE_NOTE,"NUCFS: failed to allocate"
				"memory for server nodes");
		*diagnostic =	NUCFS_ALLOCATE_NODE_FAILED;
		return(NVLT_LEAVE(FAILURE));
	} 
	/* 
	 * Else, if the number of active nodes is below the limit, we
	 * allow this allocation to proceed. The periodic timeout function,
	 * nucfs_timeout, knows to wakeup all the volume flush daemons 
	 * that should be woken up to force inactive nodes out of cache.
	 * However, we proceed to set nwfsNodeCacheQuantum to 0, so that any
	 * inactives that happen in the meantime force immediate staling.
	 */
	if ((activeOrStale + nwfsCacheNodeCount) >=
			nwfsMaxNodes) {
		nwfsNodeCacheQuantum = 0;
		nucfsDesperate = 1;
		/* 
		 * try liquidating a cached node from the 
		 * current volume, as a heuristic. 
		 */
		if (volume->cacheCount > 0) {
			/*
			 * PERF: this could be more efficient in 2 respects.
			 *
			 * (1) We could use the recycled snode for the
			 *     required fresh allocation (instead of first
			 *     releasing it and then kmem_alloc'ing a fresh
			 *     copy a short while later, below.
			 * (2) We could make NWfsVolumeCacheRecycle more
			 *     sophisticated when only one node is needed,
			 *     by having it locate a node with no name cache
			 *     holds and then selectively taking that one
			 *     node.
			 */
			NWfsVolumeCacheRecycle(volume, 1);
		}  else {
			NUCFS_RELEASE_WAIT(); /* releases nucfs_list_lock */
			NUCFS_LIST_LOCK();
		}
		++allocLoopCount;
		goto retry;
	}

	NUCFS_LIST_UNLOCK();

	/*
	 * Verify node size.
	 */
	if ((NWFI_OFF_T) nameSpaceInfo->nodeSize < 0) {
		*diagnostic = NUCFS_STALE;
		return NVLT_LEAVE(FAILURE);
	}

	/*
	 * Verify node type.
	 */
	switch (nameSpaceInfo->nodeType) {
	case NS_ROOT:
	case NS_DIRECTORY:
	case NS_FILE:
	case NS_SYMBOLIC_LINK:
		break;
	default:
		*diagnostic = NUCFS_STALE;
		return NVLT_LEAVE(FAILURE);
	}

	/*
	 * XXX: File Systems should test whether they are already
	 * at the discretionary KMA limit, and need to fail 
	 * KMA allocations if the limit is reached.
	 */
	newSnode = kmem_zalloc(sizeof(NWFS_SERVER_NODE_T), KM_SLEEP);

	/*
	 * When and if this server node is actually to be put in use,
	 * it needs to be ACTIVE, hard held, and have 3 soft holds.
	 * The soft holds are allocated for the following purposes:
	 *	=> for the activeList/cacheList/identity-table
	 *	=> for the active clientHandle
	 *	=> for the non-stale attributes.
	 */
	newSnode->nodeState = SNODE_ACTIVE;
	newSnode->hardHoldCount = 1;
	newSnode->softHoldCount = 3;
	newSnode->nameCacheSoftHolds = 0;

	/*
	 * Initialize the new snode using the information passed in
	 * from the caller.
	 */
	newSnode->nodeNumberOfLinks = nameSpaceInfo->nodeNumberOfLinks;
	newSnode->nodeVolume = volume;
	newSnode->nodeType = nameSpaceInfo->nodeType;
	newSnode->nodeNumber = nameSpaceInfo->nodeNumber;
	newSnode->accessTime = nameSpaceInfo->accessTime;
	newSnode->modifyTime = nameSpaceInfo->modifyTime;
	newSnode->changeTime = nameSpaceInfo->changeTime;
	newSnode->nodeSize = nameSpaceInfo->nodeSize;
	newSnode->vmSize =
		newSnode->postExtSize = (NWFI_OFF_T) nameSpaceInfo->nodeSize;
	newSnode->cacheInfo = *cacheInfo;	/* structure copy */
	newSnode->nodeFlags = SNODE_AT_VALID;

	/*
	 * At the moment, the NS_MANDATORY_LOCK_BIT is not used.  Instead, the
	 * UNIX convention of setgid and not group executable is followed.
	 *
	 * Note that we don't blow away any cached pages.  There are two cases
	 * to consider:  (1)  The file is just being opened.  Then there are no
	 * dirty, cached pages.  (2)  The file is already opened, and mandatory
	 * locking is just being turned on.  This is clearly erroneous
	 * application behavior with undefined side-effects, so we don't take
	 * any special action.
	 */
#	define NS_MANDATORY_LOCK_ENABLED(perm) \
		(((perm) & NS_SET_GID_BIT) && !((perm) & NS_GROUP_EXECUTE_BIT))

	if (NS_MANDATORY_LOCK_ENABLED(nameSpaceInfo->nodePermissions))
		newSnode->nodeFlags |= SNODE_REMOTE_CACHE;
	newSnode->nFlocksCached = 0;

        /*
         * initialize Snode locks
         */
	NWFI_SNODE_INIT(newSnode);

	/*
	 * Now initialize the client handle structure on behalf of the
	 * user (i.e. credentials) passed into this function.
	 */
	NWFI_LIST_INIT(&newSnode->clientHandle.clientHandleList);
	newSnode->clientHandle.handleState = SNODE_CHANDLE;
	NWFI_LIST_INIT(&newSnode->clientHandle.namesList);
	NWFI_LIST_INIT(&newSnode->clientHandle.flockCacheChain);
	newSnode->clientHandle.flockCacheLen = 0;
#if defined(DEBUG) || defined(DEBUG_TRACE)
	newSnode->clientHandle.inflated = 0;
#endif
	newSnode->clientHandle.snode = newSnode;
	newSnode->clientHandle.credentials = parentClientHandle->credentials;
	newSnode->clientHandle.nodePermissions = nameSpaceInfo->nodePermissions;
	newSnode->clientHandle.userId = nameSpaceInfo->userID;
	newSnode->clientHandle.groupId = nameSpaceInfo->groupID;
	newSnode->clientHandle.cacheInfo = *cacheInfo;
	if (accessFlags & NW_WRITE)
		newSnode->clientHandle.clientHandleFlags = NWCH_RH_WRITE;

	newSnode->clientHandle.clientHandleFlags |= NWCH_AT_VALID;
	newSnode->clientHandle.holdCount = 1;

	/*
	 * Set up the resource handle state.
	 */
	if (resourceHandle != NULL) {
		newSnode->clientHandle.resourceHandle = resourceHandle;
		++newSnode->clientHandle.holdCount;
		newSnode->openResourceCount = 1;
		newSnode->clientHandle.clientHandleFlags |=
						NWCH_RH_NEWLY_CREATED;
		newSnode->nodeState = SNODE_TIMED;
		NWFI_GET_CLOCK(newSnode->snodeTimeStamp);
	}

	/*
	 * Racing snode creaters running?
	 */
	NUCFS_LIST_LOCK();
	if (stamp != volume->idHashStamp)
		goto retry;

	/*
	 * There are no racing creaters. So must add the newly allocated
	 * server node to the various volume lists.
	 */
addNewSnode:
	++volume->idHashStamp;
	NWfiInsque(snodeToIdChain(newSnode), hashBucket);
	nodeList = (newSnode->nodeState == SNODE_TIMED) ?
				  &volume->timedNodeList
				: &volume->activeNodeList;
	NWfiInsque(snodeToVolumeChain(newSnode), nodeList);
	++volume->activeOrTimedCount;
	++nwfsActiveNodeCount;
	NUCFS_LIST_UNLOCK();
	*serverNode = newSnode;

cacheTheName:
	if (hashName != NULL)
		NWfsCacheName(parentClientHandle, hashName, *serverNode);

	/*
	 * Okay, all done.
	 */
	return (NVLT_LEAVE (SUCCESS));
}

ccode_t
NWfsCreateRootNode (
	NWFS_CRED_T		*credentials,
	NWFS_SERVER_VOLUME_T	*serverVolume,
	NWFS_SERVER_NODE_T 	**serverNode,
	enum	NUC_DIAG	*diagnostic)
{
	NWFS_SERVER_NODE_T	*parentSnode;
	NWFS_CACHE_INFO_T	cacheInfo;
	NWSI_NAME_SPACE_T	*nameSpaceInfo;
	ccode_t			returnCode = FAILURE;
	int			retries = 0;
	nwcred_t		nucCredentials;

	NVLT_ENTER(4);

	/*
	 * Allocate storage for phony parent of the root.
	 */
	parentSnode = kmem_alloc(sizeof(NWFS_SERVER_NODE_T), KM_SLEEP);
	nameSpaceInfo = kmem_alloc(sizeof(NWSI_NAME_SPACE_T), KM_SLEEP);

        /*
         * Convert NWfs credentials to NUC credentials.
         */
        NWfiFsToNucCred(credentials, &nucCredentials);

	for (;;) {
		nameSpaceInfo->nodeType = NS_ROOT;
		nameSpaceInfo->nodeNumber = 0;
		nameSpaceInfo->openFileHandle = NULL;

		cacheInfo.stale = TRUE;
		NWFI_GET_TIME(&cacheInfo.beforeTime);
		if (NWsiGetNameSpaceInfo (&nucCredentials,
			    serverVolume->spilVolumeHandle, NULL, NULL,
			    nameSpaceInfo, diagnostic)== SUCCESS) {
			NWFI_GET_TIME(&cacheInfo.afterTime);
			break;
		}

		if (*diagnostic == SPI_AUTHENTICATION_FAILURE &&
		    ++retries == 1) {
			/*
			 * The server is up, but the user needs either
			 * connection or authentication. Attempt to establish.
			 */
                        if (NWsiAutoAuthenticate(&nucCredentials,
                                    serverVolume->spilVolumeHandle,
				    NUC_SINGLE_LOGIN_ONLY,
				    diagnostic) != SUCCESS) {
				goto done;
			}
		} else {
			goto done;
		}
	}

	/*
	 * Dummy up a parent snode and a parent client handle just
	 * for the purposes of satisfying the NWfsCreateOrUpdateNode
	 * calling sequence.
	 */
	parentSnode->clientHandle.snode = parentSnode;
	parentSnode->nodeVolume = serverVolume;
	parentSnode->clientHandle.credentials = *credentials;
#if defined(DEBUG) || defined(DEBUG_TRACE)
	parentSnode->nodeState = SNODE_ACTIVE;
	parentSnode->clientHandle.handleState = SNODE_CHANDLE;
	parentSnode->clientHandle.holdCount = 1;
	parentSnode->hardHoldCount = 1;
	NWFI_SNODE_INIT(parentSnode);
#endif /* DEBUG || DEBUG_TRACE */

	returnCode = NWfsCreateOrUpdateNode (&parentSnode->clientHandle, NULL,
				&cacheInfo, nameSpaceInfo, NULL, 0,
				serverNode, diagnostic);

#if defined(DEBUG) || defined(DEBUG_TRACE)
	NWFI_SNODE_DEINIT(parentSnode);
#endif /* DEBUG || DEBUG_TRACE */

done:
	kmem_free(parentSnode, sizeof(NWFS_SERVER_NODE_T));
	kmem_free(nameSpaceInfo, sizeof(NWSI_NAME_SPACE_T));
	return NVLT_LEAVE(returnCode);
}

/*
 * BEGIN_MANUAL_ENTRY( NWfsUpdateNodeInfo(3k), \
 *                     ./man/kernel/nucfs/nwfs/UpdateNodeInfo)
 * NAME
 *    NWfsUpdateNodeInfo - Sets the specified serverNode's name space
 *                         information to the specified nameSpaceInfo.
 *
 * SYNOPSIS
 *    ccode_t
 *    NWfsUpdateNodeInfo (serverNode, nameSpaceInfo, diagnostic)
 *    NWFS_SERVER_NODE_T	*serverNode;
 *    NWSI_NAME_SPACE_T	*nameSpaceInfo;
 *    enum	NUC_DIAG 	*diagnostic;
 *
 * INPUT
 *    serverNode                - Server node to be updated.
 *    nameSpaceInfo             - New name space information.
 *
 * OUTPUT
 *   serverNode->lastModifyTime - Set to the specified nameSpaceinfo's modify
 *                                time.
 *   serverNode->nameSpaceInfo  - Set to the specified nameSpaceInfo.
 *
 * RETURN VALUES
 *    None;
 *
 * DESCRIPTION
 *    The NWfsUpdateNodeInfo updates the specified serverNode's name space
 *    information to the specified nameSpaceInfo. 
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */

/*
 * void
 * NWfsUpdateNode (credentials, nameSpaceInfo, resourceHandle,
 *		   accessFlags, serverNode, updateFlags)
 *
 * Calling/Exit State:
 *	Called with no locks held and returns that way.
 */
void
NWfsUpdateNodeInfo (
	NWFS_CRED_T		*credentials,
	NWSI_NAME_SPACE_T	*nameSpaceInfo,
	NWFS_CACHE_INFO_T	*cacheInfo,
	NWFS_SERVER_NODE_T 	*snode)
{
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	NVLT_ENTER(4);

	NVLT_ASSERT (SNODE_REASONABLE(snode));

	/*
	 * XXX: We have received some new attributes from the server. We
	 *	don't really know if these attributes are more recent than
	 *	those we hold locally (since modify time from the
	 *	server is not granular enough to resolve it - plus it can
	 *	be moved backwards as well).
	 *
	 * We use a heuristic. We update the attributes
	 * of servernode only if the new attributes were fetched by
	 * an NCP which departed after the old attributes arrived.
	 */
	SNODE_LOCK(snode);

	/*
	 * If snode is already stale then we don't want to cache any
	 * additional information (in order to prohibit new soft
	 * holds from forming).
	 */
	if (snode->nodeState == SNODE_STALE) {
		SNODE_UNLOCK(snode);
		goto done;
	}

	if (NWFI_GT(&(cacheInfo->beforeTime), 
			&(snode->cacheInfo.afterTime))) {
		if (!(snode->nodeFlags & SNODE_AT_VALID)) {
			snode->nodeFlags |= SNODE_AT_VALID;
			SNODE_SOFT_HOLD_L(snode);
		}
		if (snode->nodeSize != nameSpaceInfo->nodeSize)
			snode->nodeFlags |= SNODE_SV_SIZE_CHANGE;
		if (snode->modifyTime != nameSpaceInfo->modifyTime)
			snode->nodeFlags |= SNODE_SV_MODIFY_CHANGE;
		snode->nodeNumberOfLinks = nameSpaceInfo->nodeNumberOfLinks;
		snode->accessTime = nameSpaceInfo->accessTime;
		snode->modifyTime = nameSpaceInfo->modifyTime;
		snode->changeTime = nameSpaceInfo->changeTime;
		snode->nodeSize = nameSpaceInfo->nodeSize;
		snode->cacheInfo = *cacheInfo;
	}
	SNODE_UNLOCK(snode);

	/*
	 * Now, update the information in the client handle.
	 */
	clientHandle = NWfsGetClientHandle(snode, credentials);
	NWfsUpdateClientAttributes(clientHandle, nameSpaceInfo, cacheInfo);
	NWfsReleaseClientHandle(clientHandle);

done:
	NVLT_VLEAVE();
}

/*
 *	Get attributes from the NetWare server for the specified
 *	server node, using a resource handle. In this way the file size and
 *	modified times returned will not be totally stale.
 */
ccode_t
NWfsGetAttribsById(
	NWFS_SERVER_NODE_T	*serverNode,
	NWFS_CRED_T		*credentials,
	enum NUC_DIAG		*diagnostic)
{
	int			retries = 0;
	NWFS_CLIENT_HANDLE_T	*clientHandle = NULL;
	NWSI_NAME_SPACE_T	*nameSpaceInfo;
	ccode_t			retCode = FAILURE;
	NWFI_TIME_STAMP_T	beforeTime;
	int			authenticateFlag, newlyCreated;
	NWFS_CACHE_INFO_T	cacheInfo;
	nwcred_t		nucCredentials;
	int			handleHeld;

	NVLT_ENTER(3);

	NVLT_ASSERT(SNODE_REASONABLE(serverNode));

	NWFI_GET_TIME(&beforeTime);
retry:
	if (credentials == NULL) {
		/*
		 * If we have no credentials, that means that we
		 * are operating in a system daemon. In that case we
		 * wish to surrogate upon any user who has the file
		 * open.
		 */
		if (NWfsAttachClientHandle(serverNode, &clientHandle, 0)
				!= SUCCESS) {
			*diagnostic = NUCFS_ACCESS_DENIED;
			goto done;
		}
		authenticateFlag = FALSE;
	} else {
		clientHandle = NWfsGetClientHandle(serverNode,
						   credentials);
		authenticateFlag = TRUE;
	}

	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));

	/*
	 * Convert NWfs credentials to NUC credentials (either for the
	 * real user, or possibly for the surrogating user).
	 */
	NWfiFsToNucCred(&clientHandle->credentials, &nucCredentials);

	nameSpaceInfo = kmem_alloc(sizeof(NWSI_NAME_SPACE_T), KM_SLEEP);

	/*
	 * If this is a real file (not a directory or symlink) and if the
	 * file is truely open, then we need a resource handle to get the
	 * true file size. Otherwise, obtaining a resource handle is just
	 * useless work.
	 */
	if (serverNode->nodeType == NS_FILE && NWfiIsOpenFile(clientHandle)) {
		if (NWfsOpenResourceHandle(clientHandle, NW_READ,
				authenticateFlag, FALSE, &newlyCreated,
				diagnostic) != SUCCESS) {
			if (SNODE_HAS_FLOCK(serverNode))
				NWfsFlockStale(serverNode);
			else if (credentials == NULL &&
			    ++retries <= NUCFS_RETRY_LIMIT) {
				NWfsReleaseClientHandle(clientHandle);
				goto retry;
			}
			goto done1;
		}
		nameSpaceInfo->openFileHandle = clientHandle->resourceHandle;
		handleHeld = 1;
		cacheInfo.stale = FALSE;
	} else {
		if (retries > 0) {
			if (!authenticateFlag)
				goto done1;

			/*
			 * Authenticate the user now.
			 */
                        if (NWsiAutoAuthenticate(&nucCredentials,
                                    serverNode->nodeVolume->spilVolumeHandle,
                                    NUC_SINGLE_LOGIN_ONLY,
				    diagnostic) != SUCCESS) {
				goto done1;
			}
		}
		nameSpaceInfo->openFileHandle = NULL;
		handleHeld = 0;
		cacheInfo.stale = TRUE;
	}

	/*
	 * Set up nameSpaceInfo fetch by nodeNumber (unique file id).
	 * We also pass in the open resource handle to get accurate
	 * file size and modify time information.
	 */
	nameSpaceInfo->nodeType = serverNode->nodeType;
	nameSpaceInfo->nodeNumber = serverNode->nodeNumber;

	NWFI_GET_TIME(&cacheInfo.beforeTime);
	if (NWsiGetNameSpaceInfo (&nucCredentials,
				  serverNode->nodeVolume->spilVolumeHandle,
				  NULL, NULL, nameSpaceInfo, diagnostic) !=
					SUCCESS) {
		switch (*diagnostic) {
		/*
		 * File or directory has been deleted on the server.
		 */
		case SPI_NO_SUCH_DIRECTORY:
		case SPI_NODE_NOT_FOUND:
		case SPI_INVALID_PATH:
			NWfsStaleNode(serverNode);
			*diagnostic = NUCFS_STALE;
			/* FALLTHROUGH */

		/*
		 * The server is down, or going down. So therefore,
		 * get rid of out resource handle and fail the
		 * operation.
		 */
		case SPI_SERVER_UNAVAILABLE:
		case SPI_SERVER_DOWN:
			if (handleHeld) {
				if (SNODE_HAS_FLOCK(serverNode))
					NWfsFlockStale(serverNode);
				NWfsCloseResourceHandle(clientHandle);
			}
			goto done1;

		/*
		 * The server is up, but the user has lost either
		 * authentication or connection.
		 */
		case SPI_AUTHENTICATION_FAILURE:
		case SPI_BAD_CONNECTION:
			if (handleHeld) {
				if (SNODE_HAS_FLOCK(serverNode)) {
					NWfsFlockStale(serverNode);
					NWfsCloseResourceHandle(clientHandle);
					goto done1;
				}
				NWfsCloseResourceHandle(clientHandle);
			}
			if (++retries <= NUCFS_RETRY_LIMIT) {
				NWfsReleaseClientHandle(clientHandle);
				goto retry;
			}
			goto done1;

		/*
		 * We got some file or file system specific error.
		 */
		default:
			if (handleHeld)
				NWfsReleaseResourceHandle(clientHandle);
			goto done1;
		}
	}
	if (handleHeld)
		NWfsReleaseResourceHandle(clientHandle);

	NWFI_GET_TIME(&cacheInfo.afterTime);

	/*
	 * Verify node type and node number. Also, verify that file size is
	 * non-negative.
	 */
	if (nameSpaceInfo->nodeType != serverNode->nodeType ||
	     nameSpaceInfo->nodeNumber != serverNode->nodeNumber ||
	     (NWFI_OFF_T) nameSpaceInfo->nodeSize < 0) {
		NWfsStaleNode(serverNode);
		*diagnostic = NUCFS_STALE;
		goto done1;
	}

	/*
	 * Update the attribute information
	 */
	NWfsUpdateNodeInfo(&clientHandle->credentials, nameSpaceInfo,
			   &cacheInfo, serverNode);

	/*
	 * Node already stale?
	 */
	SNODE_LOCK(serverNode);
	if (serverNode->nodeState == SNODE_STALE) {
		SNODE_UNLOCK(serverNode);
		*diagnostic = NUCFS_STALE;
		goto done1;
	}

	/*
	 * Getting the attributes but not actually updating the cached
	 * values can be kind of useless. If somebody else has updated the
	 * attributes, that is fine. But if they can still reflect state from
	 * before we were called then we must retry. Fortunately, this cannot
	 * turn into an infinite loop because ``beforeTime'' is not
	 * advancing.
	 *
	 * PERF: Can the lock be eliminated around the fetch of a
	 *	 NWFI_TIME_STAMP_T value?
	 */
	if (!NWFI_GT(&(serverNode->cacheInfo.beforeTime), &beforeTime) ||
	    !NWFI_GT(&(clientHandle->cacheInfo.beforeTime), &beforeTime)) {
		SNODE_UNLOCK(serverNode);
		NWfsReleaseClientHandle(clientHandle);
		retries = 0;
		goto retry;
	}
	SNODE_UNLOCK(serverNode);
	retCode = SUCCESS;

done1:
	kmem_free(nameSpaceInfo, sizeof(NWSI_NAME_SPACE_T));
	NWfsReleaseClientHandle(clientHandle);
done:
	return NVLT_LEAVE(retCode);
}

/*
 *	Get attributes from the NetWare server for the specified
 *	server node, using a parent resource handle and a name.
 */
ccode_t
NWfsGetAttribsByName(
	NWFS_CRED_T		*credentials,
	NWFS_SERVER_NODE_T	*parentNode,
	NWFS_SERVER_NODE_T	*serverNode,
	char			*nodeName,
	NWSI_NAME_SPACE_T	*nameSpaceInfo,
	enum NUC_DIAG		*diagnostic)
{
	int			retries = 0;
	ccode_t			retCode = FAILURE;
	nwcred_t		nucCredentials;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	int			newlyCreated;

	NVLT_ENTER(6);

	NVLT_ASSERT(SNODE_REASONABLE(parentNode));
	NVLT_ASSERT(SNODE_REASONABLE(serverNode));
	NVLT_ASSERT(credentials != NULL);

	/*
	 * Convert NWfs credentials to NUC credentials (either for the
	 * real user, or possibly for the surrogating user).
	 */
	NWfiFsToNucCred(credentials, &nucCredentials);

	clientHandle = NWfsGetClientHandle(parentNode, credentials);

	/*
	 * Set up nameSpaceInfo.
	 */
retry:
	nameSpaceInfo->nodeType = serverNode->nodeType;
	nameSpaceInfo->nodeNumber = -1;
	nameSpaceInfo->openFileHandle = NULL;

	if (NWfsOpenResourceHandle(clientHandle, 0, TRUE, FALSE,
				   &newlyCreated, diagnostic) != SUCCESS) {
		goto done;
	}

	retCode = NWsiGetNameSpaceInfo (&nucCredentials,
			  serverNode->nodeVolume->spilVolumeHandle,
			  clientHandle->resourceHandle, nodeName,
			  nameSpaceInfo, diagnostic);
	if (retCode == SUCCESS) {
		NWfsReleaseResourceHandle(clientHandle);
		if (nameSpaceInfo->nodeType != serverNode->nodeType ||
		    nameSpaceInfo->nodeNumber != serverNode->nodeNumber) {
			/*
			 * Parent/child relationship is broken. Return an
			 * error to the caller. Also, invalidate both nodes.
			 * This assists the caller in retrying by discarding
			 * possibly stale name cache info.
			 */
			NWfsInvalidateNode(parentNode);
			NWfsInvalidateNode(serverNode);
			*diagnostic = NUCFS_NOT_CHILD;
			retCode = FAILURE;
		}
	} else {
		NWfsCloseResourceHandle(clientHandle);
		switch (*diagnostic) {
		/*
		 * The server is up, but the user has lost either
		 * authentication or connection.
		 */
		case SPI_AUTHENTICATION_FAILURE:
		case SPI_BAD_CONNECTION:
			if (++retries <= NUCFS_RETRY_LIMIT)
				goto retry;
			break;

		/*
		 * The server is down, or going down. Or,
		 * we got some file or file system specific error.
		 */
		default:
			break;
		}
	}

done:
	NWfsReleaseClientHandle(clientHandle);

	return NVLT_LEAVE(retCode);
}

/*
 *	Attempt to delete a file, using the file's nodeNumber.
 *
 *	No errors are returned to the caller.
 */
void
NWfsDeleteById(NWFS_CLIENT_HANDLE_T *clientHandle)
{
	int			retries = 0;
	int			delayRetries = 0;
	NWFS_SERVER_NODE_T	*serverNode;
	nwcred_t		nucCredentials;
	enum NUC_DIAG		diagnostic = SUCCESS;

	NVLT_ENTER(1);

	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
	serverNode = clientHandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(serverNode));

	/*
	 * Convert NWfs credentials to NUC credentials.
	 */
	NWfiFsToNucCred(&clientHandle->credentials, &nucCredentials);

retry:
	NVLT_ASSERT(!SNODE_HAS_FLOCK(serverNode));	/* inactive/symlink */
	if (NWsiDeleteNode (&nucCredentials,
			    serverNode->nodeVolume->spilVolumeHandle,
			    NULL, NULL, serverNode->nodeType,
			    serverNode->nodeNumber,
			    &diagnostic) != SUCCESS) {
		switch (diagnostic) {
		/*
		 * The server is up, but the user has lost either
		 * authentication or connection.
		 */
		case SPI_AUTHENTICATION_FAILURE:
		case SPI_BAD_CONNECTION:
			if (++retries > NUCFS_RETRY_LIMIT ||
					SNODE_HAS_FLOCK(serverNode))
                                break;
                        if (NWsiAutoAuthenticate(&nucCredentials,
                                    serverNode->nodeVolume->spilVolumeHandle,
                                    NUC_SINGLE_LOGIN_ONLY,
				    &diagnostic) == SUCCESS) {
				goto retry;
			}
			break;

		/*
		 * The file is open on the NetWare server.
		 * So delay one second and retry.
		 */
		case SPI_FILE_IN_USE:
			if (++delayRetries <= NUCFS_DELAY_TIME) {
				NWfiDelay(HZ);
				goto retry;
			}
			break;

		/*
		 * The server is down, or going down, or we got some
		 * file, file system, or server specific error.
		 * Fail the operation now.
		 */
		default:
			break;
		}
	}

	NVLT_VLEAVE();

	return;
}

/*
 *	Delete a file, using the name.
 */
ccode_t
NWfsDeleteByName(
	NWFS_CLIENT_HANDLE_T	*parentClientHandle,
	char			*fileName,
	NWFS_SERVER_NODE_T	*fileNode,
	enum NUC_DIAG		*diagnostic)
{
	int			retries = 0;
	int			delayRetries = 0;
	NWFS_SERVER_NODE_T	*parentNode;
	int			newlyCreated;
	ccode_t			retCode;
	uint32			nodeNumber;
	uint32			nodeType;
	nwcred_t		nucCredentials;

	NVLT_ENTER(4);
	NVLT_STRING(fileName);

	NVLT_ASSERT(CHANDLE_REASONABLE(parentClientHandle));
	parentNode = parentClientHandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(parentNode));
	NVLT_ASSERT(parentNode->nodeType == NS_DIRECTORY ||
	       parentNode->nodeType == NS_ROOT);
	NVLT_ASSERT(SNODE_REASONABLE(fileNode));

	/*
	 * Convert NWfs credentials to NUC credentials.
	 */
	NWfiFsToNucCred(&parentClientHandle->credentials, &nucCredentials);

	nodeType = fileNode->nodeType;
	nodeNumber = (uint32) -1;

retry:
	retCode = NWfsOpenResourceHandle(parentClientHandle, 0, TRUE, FALSE,
			   &newlyCreated, diagnostic);
	if (retCode != SUCCESS)
                goto done;

	retCode = NWsiDeleteNode (&nucCredentials,
			    parentNode->nodeVolume->spilVolumeHandle,
			    parentClientHandle->resourceHandle, fileName,
			    nodeType, nodeNumber, diagnostic);
	if (retCode == SUCCESS) {
		NWfsReleaseResourceHandle(parentClientHandle);
		goto done;
	}

	NWfsCloseResourceHandle(parentClientHandle);
	switch (*diagnostic) {
	/*
	 * The server is down, or going down. So therefore,
	 * get rid of out resource handle and fail the
	 * operation.
	 */
	case SPI_SERVER_UNAVAILABLE:
	case SPI_SERVER_DOWN:
		goto done;

	/*
	 * The server is up, but the user has lost either
	 * authentication or connection.
	 */
	case SPI_AUTHENTICATION_FAILURE:
	case SPI_BAD_CONNECTION:
		if (++retries <= NUCFS_RETRY_LIMIT)
			goto retry;
		break;

	/*
	 * The file is open on the NetWare server.
	 * So delay one second and retry.
	 */
	case SPI_FILE_IN_USE:
		if (++delayRetries <= NUCFS_DELAY_TIME) {
			NWfiDelay(HZ);
			goto retry;
		}
		break;

	/*
	 * The server is down, or going down, or we got some
	 * file, file system, or server specific error.
	 * Fail the operation now.
	 */
	default:
		break;
	}

done:
	return NVLT_LEAVE(retCode);
}

/*
 * This function will get time off of the server.
 *
 * PERF: This routine is generally called only for rare cases.  The only
 *       possible exception to this is as a result of the ustat(NULL)
 *       system call.  Since it's fairly expensive to go to the server
 *       to get the time, it might be reasonable to implement some sort
 *       of caching here, but ustat(NULL) is probably not used with enough
 *       frequency to matter.
 */
ccode_t
NWfsGetServerTime(
	NWFS_CRED_T		*credentials,
	NWFS_SERVER_VOLUME_T	*volume,
	int32			*timePtr,
	enum NUC_DIAG		*diagnostic)
{
	nwcred_t		nucCredentials;
	ccode_t			retCode;
	int32			retries = 0;
	int32			serverTime, localTime;
	int32			serverSeconds, localSeconds;
	int32			adjust;
	const int32		fifteenMinutes = 15 * 60;
	const int32		thirtyMinutes = 30 * 60;

	NVLT_ENTER(4);

	/*
	 * Convert NWfs credentials to NUC credentials.
	 */
	NWfiFsToNucCred(credentials, &nucCredentials);

retry:
	retCode = NWsiGetServerTime(&nucCredentials,
				    volume->spilVolumeHandle,
				    &serverTime,
				    diagnostic);
	if (retCode == SUCCESS) {
		/*
		 * The server returns time adjusted to its local time zone,
		 * so that the half-hours may be wrong. We have no way to
		 * query the server for its time zone. Therefore, we adjust
		 * the half-hours to the locally maintained value.
		 */
		localTime = hrestime.tv_sec;
		serverSeconds = serverTime % thirtyMinutes;
		localSeconds = localTime % thirtyMinutes;
		adjust = serverSeconds - localSeconds;
		if (serverSeconds > localSeconds) {
			if (serverSeconds > localSeconds + fifteenMinutes)
				adjust -= thirtyMinutes;
		} else {
			if (localSeconds > serverSeconds + fifteenMinutes)
				adjust += thirtyMinutes;
		}
		*timePtr = localTime + adjust;
		goto done;
	}

	switch (*diagnostic) {
	/*
	 * The server is up, but the user has lost either
	 * authentication or connection. Attempt to re-establish.
	 */
	case SPI_AUTHENTICATION_FAILURE:
	case SPI_BAD_CONNECTION:
		if (++retries <= NUCFS_RETRY_LIMIT &&
		    NWsiAutoAuthenticate(&nucCredentials,
					 volume->spilVolumeHandle,
					 NUC_SINGLE_LOGIN_ONLY,
					 diagnostic) == SUCCESS)
			goto retry;
		break;

	/*
	 * The server is down, or going down, or we got some
	 * file, file system, or server specific error.
	 * Fail the operation now.
	 */
	default:
		break;
	}

done:

	return NVLT_LEAVE(retCode);
}

/*
 * Place a server node in the ``stale'' state, indicating that it
 * is being destroyed.
 */
void
NWfsStaleNode(NWFS_SERVER_NODE_T *snode)
{
	NWFS_SERVER_VOLUME_T	*volp;

	NVLT_ENTER(1);

	NVLT_ASSERT(SNODE_REASONABLE(snode));

	volp = snode->nodeVolume;

	/*
	 * First, make sure that the snode is in the stale state.
	 */
	NUCFS_LIST_LOCK();
	switch (snode->nodeState) {
	case SNODE_ACTIVE:
	case SNODE_TIMED:
		SNODE_DESTROY_IDENTITY(snode, volp, activeOrTimedCount);
		--nwfsActiveNodeCount;
		NUCFS_LIST_UNLOCK();
		SNODE_SOFT_RELEASE(snode);
		break;
	case SNODE_INACTIVE:
		SNODE_DESTROY_IDENTITY(snode, volp, cacheCount);
		--nwfsCacheNodeCount;
		NUCFS_LIST_UNLOCK();
		SNODE_SOFT_RELEASE(snode);
		break;
	default:
		NVLT_ASSERT(snode->nodeState == SNODE_STALE);
		NUCFS_LIST_UNLOCK();
		break;
	}

	/*
	 * Stale out client handles.
	 */
	NWfsStaleChandles(snode);
	NWfsInvalidateNode(snode);

	/*
	 * Tell the NWfi layer to stale out any state it is holding.
	 */
	NWfiStaleNode(snode, FALSE);

	NVLT_VLEAVE();
}

void
NWfsInvalidateNode(NWFS_SERVER_NODE_T *snode)
{
	boolean_t purgeSnodeFromNameCache = B_TRUE;

	NVLT_ENTER(1);

	NVLT_ASSERT(SNODE_REASONABLE(snode));

	/*
	 * First, invalidate the attributes in the snode
	 */
	SNODE_LOCK(snode);
	if (snode->nodeFlags & SNODE_AT_VALID) {
		snode->nodeFlags &= ~SNODE_AT_VALID;
		--snode->softHoldCount;
		NVLT_ASSERT(SNODE_HELD_L(snode));
	}
	if ((snode->nameCacheSoftHolds == 0) && 
			(snode->nodeState == SNODE_STALE)) {
		/*
		 * there are no references to this snode as a "child" from
		 * the name cache; and this node is stale, so no new
		 * references can form. So we can bypass the name cache
		 * purge for this snode.
		 */
		purgeSnodeFromNameCache = B_FALSE;
	}	
	
	SNODE_UNLOCK(snode);

	/*
	 * Now invalidate the client handles (attributes and names).
	 */
	(void)NWfsInvalidateChandles(snode);
	if (purgeSnodeFromNameCache) {
		/*
		 * Now clear out all cached names referencing 
		 * ``snode'' (as a child).
		 */
		NWfsInvalidateNameCache(snode);
	}

	NVLT_VLEAVE();
}

/*
 *	func does not block waiting for softHoldCount to drain to 0.
 */
void
NWfsForEachSnode(NWFI_LIST_T *list, void (*func)(), void *arg)
{
	NWFS_SERVER_NODE_T	*snode, *marker;

	NVLT_ENTER(3);

	SNODE_CREATE_MARKER(&marker);

	NUCFS_LIST_LOCK();
	snode = idChainToSnode(NWFI_NEXT_ELEMENT(list));
	while (snode != idChainToSnode((NWFS_SERVER_NODE_T *) list)) {
		/*
		 * Skip over list markers
		 */
		if (snode->nodeState == SNODE_MARKER) {
			snode = idChainToSnode(NWFI_NEXT_ELEMENT(
					snodeToIdChain(snode)));
			continue;
		}
		NVLT_ASSERT(SNODE_REASONABLE(snode));
		NVLT_ASSERT(snode->nodeState != SNODE_STALE);
		SNODE_SOFT_HOLD(snode);
		NWfiInsque(snodeToIdChain(marker), snodeToIdChain(snode));
		NUCFS_LIST_UNLOCK();

		/*
		 * Invoke the thunk on the snode, passing in the
		 * argument.
		 */
		(*func)(snode, arg);

		/*
		 * Onto the next snode.
		 */
		SNODE_SOFT_RELEASE(snode);
		NUCFS_LIST_LOCK();
		snode = idChainToSnode(NWFI_NEXT_ELEMENT(
					snodeToIdChain(marker)));
		NWfiRemque(snodeToIdChain(marker));
	}
	NUCFS_LIST_UNLOCK();

	SNODE_DESTROY_MARKER(marker);

	NVLT_VLEAVE();
}

STATIC void
NWfsInvalidateNameCache(NWFS_SERVER_NODE_T *snode)
{
	NWFS_SERVER_VOLUME_T *volp;
	int i;

	NVLT_ENTER(1);

	NVLT_ASSERT(SNODE_REASONABLE(snode));

	volp = snode->nodeVolume;

	NUCFS_LIST_LOCK();
	if (NWFI_VOLUME_UNMOUNTING(volp->volFlushData)) {
		/* 
		 * fast path for the unmount case. the name cache
		 * is already fully purged, and no new names are
		 * being added due to the VOL_UNMOUNTING flag.
		 */
		NUCFS_LIST_UNLOCK();
		NVLT_VLEAVE();
		return;
	}
	NUCFS_LIST_UNLOCK();

	for (i = 0; i < NUCFS_NODE_LIST_BUCKETS; ++i) {
		NWfsForEachSnode(&volp->idHashTable[i],
				 NWfsPurgeSnodeFromNC, snode);
	}

	NVLT_VLEAVE();
}

/*
 * Calling/Exit State:
 *	The caller has removed snode from one of the server node volume lists,
 *	and transitioned it to the stale state (under the cover of the
 *	NUCFS_LIST_LOCK).
 */

void
NWfsDestroyNode(NWFS_SERVER_NODE_T *snode)
{
	NVLT_ENTER(1);

	/*
	 * Synchronously wait for node usage to drain down to 0.
	 */
	NWfsWaitForDestroy(snode);

	/*
	 * Bye Bye Node.
	 */
	NWfsFreeServerNode(snode);

	NVLT_VLEAVE();
}

/*
 * The caller has a single soft hold on ``snode''. Snode is in
 * state SNODE_STALE. The caller transitioned it into this state
 * via the SNODE_DESTROY_IDENTITY macro.
 */
void
NWfsWaitForDestroy(NWFS_SERVER_NODE_T *snode)
{
	NVLT_ENTER(1);

	NVLT_ASSERT(SNODE_REASONABLE(snode));

	/*
	 * Drop as many of the reasons for hold as we can.
	 */
	NWfsStaleNode(snode);
	NVLT_ASSERT(snode->nodeState == SNODE_STALE);

	/*
	 * Atomically drop the caller's soft hold and begin waiting for
	 * for all remaining uses to drain down.
	 *
	 * Note: We need to be able to assert that two LWPs do not conduct 
	 *	 a fight to destroy a single node. By convention, an LWP
	 *	 is permitted to destroy a server node only if it has
	 *	 has removed it from one of the server volume lists.
	 *	 Since this can only be done once (under the cover of the
	 *	 NUCFS_LIST_LOCK), it follows that such fights are
	 *	 impossible.
	 */
	SNODE_LOCK(snode);
	NVLT_ASSERT(!(snode->nodeFlags & SNODE_DESTROY));
	NVLT_ASSERT(snode->softHoldCount != 0);
	snode->nodeFlags |= SNODE_DESTROY;
	--snode->softHoldCount;
	while (snode->softHoldCount + snode->hardHoldCount != 0) {
		SNODE_WAIT(snode);
		SNODE_LOCK(snode);
	}
	snode->nodeFlags &= ~SNODE_DESTROY;
	SNODE_UNLOCK(snode);

	NVLT_VLEAVE();
}

/*
 * Obtain the RW lock in WRITER mode for each snode on a NULL terminated
 * list of server nodes (passed in as an array). In order to establish a
 * deadlock free property, the servernodes are locked in machine address
 * order. Duplicates (if any) are removed from the list.
 */
void
NWfsLockSnodes(NWFS_SERVER_NODE_T *snodeArray[])
{
	NWFS_SERVER_NODE_T	*snode1, *snode2;
	NWFS_SERVER_NODE_T	**snode1p, **snode2p, **snode3p;
	boolean_t flipped;

	NVLT_ENTER(1);

	do {
		flipped = FALSE;
		snode1p = &snodeArray[0];
		snode1 = *snode1p;
		snode2p = snode1p + 1;
		while ((snode2 = *snode2p) != NULL) {
			NVLT_ASSERT(*snode1p == snode1);
			NVLT_ASSERT(snode2p == snode1p + 1);
			if (snode1 == snode2) {
				snode3p = snode2p + 1;
				while ((*snode2p = *snode3p) != NULL) {
					++snode2p;
					++snode3p;
				}
				snode2p = snode1p + 1;
			} else {
				if (snode1 > snode2) {
					*snode1p = snode2;
					*snode2p = snode1;
					flipped = TRUE;
				} else {
					snode1 = snode2;
				}
				++snode1p;
				++snode2p;
			}
		}
	} while (flipped);

	snode1p = &snodeArray[0];
	while ((snode1 = *snode1p++) != NULL)
		SNODE_WR_LOCK(snode1);
	
	NVLT_VLEAVE();
}

/*
 * Unlock a list of server nodes (previously locked by NWfsLockSnodes)
 */
void
NWfsUnLockSnodes(NWFS_SERVER_NODE_T *snodeArray[])
{
	NWFS_SERVER_NODE_T	*snode;
	NWFS_SERVER_NODE_T	**snodep;

	NVLT_ENTER(1);

	snodep = &snodeArray[0];
	while ((snode = *snodep++) != NULL)
		SNODE_RW_UNLOCK(snode);
	
	NVLT_VLEAVE();
}

static char base36Digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
static char renameTemplate[] = ".XXXXXXXX.UXC";

/*
 *	Instead of deleting an open file, rename it to a hidden file.
 *	The chosen name has the unique node number of the file encoded
 *	into base 36, plus the suffix ".UXC" for "Unix Client".
 * 
 *	If necessary, this of sort name can be used in the DOS name space
 *	if it didn't have the leading dot.
 */
ccode_t
NWfsHideNode(
	NWFS_CRED_T		*credentials,
	NWFS_SERVER_NODE_T	*parentNode,
	NWFS_SERVER_NODE_T	*snode,
	enum NUC_DIAG		*diagnostic)
{
	NWFS_CLIENT_HANDLE_T	*parentClientHandle;
	nwcred_t		nucCredentials;
	NWSI_NAME_SPACE_T	*nameSpaceInfo;
	char			*fileName;
	int32			nodeNumber;
	char			*p;
	int			retries = 0;
	int			newlyCreated;
	int			delayRetries = 0;
	ccode_t			returnCode = FAILURE;

	NVLT_ENTER(4);

	NVLT_ASSERT(SNODE_REASONABLE(parentNode));
	NVLT_ASSERT(SNODE_REASONABLE(snode));
	NVLT_ASSERT(snode->delayDeleteClientHandle == NULL);

	/*
	 * Formulate the new name for the file.
	 */
	fileName = kmem_alloc(sizeof(renameTemplate), KM_SLEEP);
	strcpy(fileName, renameTemplate);
	p = &fileName[8];
	nodeNumber = snode->nodeNumber;
	while (p >= &fileName[1]) {
		*p-- = base36Digits[nodeNumber % 36];
		nodeNumber /= 36;
	}

	parentClientHandle = NWfsGetClientHandle(parentNode, credentials);

        /*
         * Convert NWfs credentials to NUC credentials.
         */
retry:
	NVLT_ASSERT(!SNODE_HAS_FLOCK(snode));
        NWfiFsToNucCred(credentials, &nucCredentials);

	/*
	 * Set up for call to NWsiRenameNode.
	 */
	nameSpaceInfo = kmem_alloc(sizeof(NWSI_NAME_SPACE_T), KM_SLEEP);
	nameSpaceInfo->nodeNumber = snode->nodeNumber;
	nameSpaceInfo->linkNodeNumber = (uint32) -1;
	nameSpaceInfo->nodeType = snode->nodeType;
	nameSpaceInfo->nodePermissions = (uint32) -1;
	nameSpaceInfo->nodeNumberOfLinks = (uint32) -1;
	nameSpaceInfo->nodeSize = (uint32) -1;
	nameSpaceInfo->userID = (uint32) -1;
	nameSpaceInfo->groupID = (uint32) -1;
	nameSpaceInfo->accessTime = (uint32) -1;
	nameSpaceInfo->modifyTime = (uint32) -1;
	nameSpaceInfo->changeTime = (uint32) -1;
	nameSpaceInfo->openFileHandle = NULL;
	nameSpaceInfo->searchSequence = (uint32) -1;

	if (NWfsOpenResourceHandle(parentClientHandle, 0,
			TRUE, FALSE, &newlyCreated, diagnostic) !=
			SUCCESS) {
		goto done;
	}

        if (NWsiRenameNode (&nucCredentials,
                            parentNode->nodeVolume->spilVolumeHandle,
                            NULL, NULL, nameSpaceInfo,
			    parentClientHandle->resourceHandle,
			    fileName, diagnostic) != SUCCESS) {

		NWfsCloseResourceHandle(parentClientHandle);

                switch (*diagnostic) {
                /*
                 * The server is up, but the user has lost either
                 * authentication or connection.
                 */
                case SPI_AUTHENTICATION_FAILURE:
                case SPI_BAD_CONNECTION:
                        if (++retries > NUCFS_RETRY_LIMIT)
                                break;
                        if (NWsiAutoAuthenticate(&nucCredentials,
                                    parentNode->nodeVolume->spilVolumeHandle,
                                    NUC_SINGLE_LOGIN_ONLY,
				    diagnostic) == SUCCESS) {
                                goto retry;
                        }
                        break;

                /*
                 * The file is open on the NetWare server.
                 * So delay one second and retry.
                 */
                case SPI_FILE_IN_USE:
                        if (++delayRetries <= NUCFS_DELAY_TIME) {
                                NWfiDelay(HZ);
                                goto retry;
                        }
                        break;

                /*
                 * The server is down, or going down, or we got some
                 * file, file system, or server specific error.
                 * Fail the operation now.
                 */
                default:
                        break;
                }
        } else {
		NWfsReleaseResourceHandle(parentClientHandle);
		returnCode = SUCCESS;

		/*
		 * Now set the child node up for a delayed delete.
		 */
		snode->delayDeleteClientHandle =
				 NWfsGetClientHandle(snode, credentials);

		/*
		 * One of the child node's names has been invalidated.
		 * The parent directory has been modified.
		 */
		NWfsInvalidateNode(snode);
		SNODE_SET_MODIFIED(parentNode);
	}
done:
	NWfsReleaseClientHandle(parentClientHandle);
	kmem_free(fileName, sizeof(renameTemplate));
	kmem_free(nameSpaceInfo, sizeof(NWSI_NAME_SPACE_T));
	return NVLT_LEAVE(returnCode);
}

/*
 * Rename a file.
 */
ccode_t
NWfsRenameNode(
	NWFS_CRED_T		*credentials,
	NWFS_SERVER_NODE_T	*oldParentNode,
	NWFS_SERVER_NODE_T	*newParentNode,
	NWFS_SERVER_NODE_T	*sourceNode,
	char			*oldName,
	char			*newName,
	enum	NUC_DIAG	*diagnostic)
{
	ccode_t			returnCode = FAILURE;
	NWFS_CLIENT_HANDLE_T	*oldParentClientHandle, *newParentClientHandle;
	int			newlyCreated;
	NWSI_NAME_SPACE_T	*nameSpaceInfo;
	nwcred_t		nucCredentials;
	int			handleTries = 0;
	int			delayRetries = 0;

	NVLT_ENTER(7);
	NVLT_STRING (oldName);
	NVLT_STRING (newName);

	NVLT_ASSERT (SNODE_REASONABLE(oldParentNode));
	NVLT_ASSERT (oldParentNode->nodeType == NS_DIRECTORY ||
		oldParentNode->nodeType == NS_ROOT);
	NVLT_ASSERT (SNODE_REASONABLE(newParentNode));
	NVLT_ASSERT (newParentNode->nodeType == NS_DIRECTORY ||
		newParentNode->nodeType == NS_ROOT);
	NVLT_ASSERT (SNODE_REASONABLE(sourceNode));

	/*
	 * Get client handle objects (which are returned held).
	 */
	oldParentClientHandle = NWfsGetClientHandle(oldParentNode, credentials);
	if (oldParentNode == newParentNode) {
		newParentClientHandle = oldParentClientHandle;
	} else {
		NVLT_ASSERT(oldParentNode->nodeVolume ==
			    newParentNode->nodeVolume);
		newParentClientHandle = NWfsGetClientHandle(newParentNode,
							    credentials);
	}

	nameSpaceInfo = kmem_alloc(sizeof(NWSI_NAME_SPACE_T), KM_SLEEP);

	/*
         * Move the NetWare node to its new directory with the specified
	 * newName.
	 *
	 * NOTE:
	 *    If successful, NWsiRenameNode will create the newName in the
	 *    specified parent directory on the NetWare server.
	 */
	for (;;) {
		/*
                 * Get NUC resource handles for the parents.
                 */

		/*
		 * can't rename flocked file
		 */
		NVLT_ASSERT(!SNODE_HAS_FLOCK(sourceNode));

                if (NWfsOpenResourceHandle(oldParentClientHandle, 0,
                                TRUE, FALSE, &newlyCreated, diagnostic) !=
                                SUCCESS) {
                        goto done;
                }
		if (oldParentNode != newParentNode &&
		    NWfsOpenResourceHandle(newParentClientHandle, 0, TRUE,
					FALSE, &newlyCreated, diagnostic) !=
					SUCCESS) {
			NWfsReleaseResourceHandle(oldParentClientHandle);
			goto done;
		}

		nameSpaceInfo->nodeNumber = -1;
		nameSpaceInfo->linkNodeNumber = -1;
		nameSpaceInfo->nodeType = sourceNode->nodeType;
		nameSpaceInfo->nodePermissions = (uint32) -1;
		nameSpaceInfo->nodeNumberOfLinks = (uint32) -1;
		nameSpaceInfo->nodeSize = (uint32) -1;
		nameSpaceInfo->userID = (uint32) -1;
		nameSpaceInfo->groupID = (uint32) -1;
		nameSpaceInfo->accessTime = (uint32) -1;
		nameSpaceInfo->modifyTime = (uint32) -1;
		nameSpaceInfo->changeTime = (uint32) -1;
		nameSpaceInfo->openFileHandle = NULL;
		nameSpaceInfo->searchSequence = (uint32) -1;

		/*
		 * Convert NWfs credentials to NUC credentials.
		 */
		NWfiFsToNucCred(credentials, &nucCredentials);

		returnCode = NWsiRenameNode (&nucCredentials, 
				oldParentNode->nodeVolume->spilVolumeHandle,
				oldParentClientHandle->resourceHandle, oldName,
				nameSpaceInfo,
				newParentClientHandle->resourceHandle, newName,
				diagnostic);

		if (returnCode == SUCCESS) {
			/*
			 * Release (both) parent client handles.
			 * (Both) parents have now been modified.
			 */
                        NWfsReleaseResourceHandle(oldParentClientHandle);
			if (oldParentNode != newParentNode) {
				NWfsReleaseResourceHandle(
						newParentClientHandle);
				SNODE_SET_MODIFIED(newParentNode);
			}

			/*
			 * At the very least, one of the child node's names
			 * has been invalidated. Perhaps its very identity
			 * has been destroyed (if it was moved to another
			 * directory). In either case it sufficies to
			 * invalidate the cached information.
			 */
			NWfsInvalidateNode(sourceNode);
			break;
		}

		NWfsCloseResourceHandle(oldParentClientHandle);
		if (oldParentNode != newParentNode)
			NWfsCloseResourceHandle(newParentClientHandle);

		switch (*diagnostic) {
                case SPI_SERVER_UNAVAILABLE:
                case SPI_SERVER_DOWN:
                        goto done;

                /*
                 * The server is up, but the user has lost authentication.
                 */
                case SPI_AUTHENTICATION_FAILURE:
                case SPI_BAD_CONNECTION:
                        if (++handleTries <= NUCFS_RETRY_LIMIT)
                                break;
                        goto done;

		/*
		 * The file is open on the NetWare server.
		 * So delay one second and retry.
		 */
		case SPI_FILE_IN_USE:
			if (++delayRetries <= NUCFS_DELAY_TIME) {
				NWfiDelay(HZ);
				break;
			}
			goto done;

                /*
		 * The server is down. OR, we received some file or file
		 * system specific error. Return this to the caller.
                 */
                default:
                        goto done;
                }
	}

done:
	/*
	 * Release client handles.
	 */
	NWfsReleaseClientHandle(oldParentClientHandle);
	if (oldParentNode != newParentNode)
		NWfsReleaseClientHandle(newParentClientHandle);
	kmem_free(nameSpaceInfo, sizeof(NWSI_NAME_SPACE_T));

	return NVLT_LEAVE(returnCode);
}

#if defined(DEBUG) || defined(DEBUG_TRACE)
/*
 * Some DEBUG/DEBUG_TRACE only functions
 */
boolean_t
NWfsNodeIsHeld(NWFS_SERVER_NODE_T *sNode)
{
	boolean_t answer;
	NUCFS_LIST_LOCK();
	answer = (sNode->hardHoldCount > 0);
	NUCFS_LIST_UNLOCK();
	return(answer);
}
boolean_t
NWfsNodeIsSoftHeld(NWFS_SERVER_NODE_T *sNode)
{
	boolean_t answer;
	NUCFS_LIST_LOCK();
	answer = (sNode->softHoldCount > 0);
	NUCFS_LIST_UNLOCK();
	return(answer);
}
#endif /* DEBUG || DEBUG_TRACE */

/*
 * void
 * NWfsReleaseCachingSoftHolds(struct NWfsServerNode *snode,
 *	struct NWfsServerNode *marker)
 *
 * Release stale attributes from a server node and its client
 * handles. Also release stale child names from the client
 * handles. Adjust holds with releases of the attributes and
 * the names cached.
 *
 * marker is an LWP private, pre-initialized marker that is
 * being passed in as an optimization.
 */
void
NWfsReleaseStaleCachingHolds(struct NWfsServerNode *snode,
	struct NWfsServerNode *marker)
{
	NWFS_CLIENT_HANDLE_T *clientHandle;
	NWFS_CLIENT_HANDLE_T *baseHandle;
	NWFS_CHILD_NAME_T discardList;
	NWFS_CHILD_NAME_T *anchorName; 
	NWFS_CHILD_NAME_T *nextChildName;
	NWFS_CHILD_NAME_T *childName;
	int force = 0;
	int dropClientHolds;

/*
 * Convenient macros for referencing child names
 */
#define NAME_STALE(cnamep)		\
	NUCFS_CLOCK_STALE((cnamep)->cacheTime)
#define NAME_NEXT(cnamep)		\
	(NWFS_CHILD_NAME_T *)NWFI_NEXT_ELEMENT(&((cnamep)->childNames))
#define NAME_INSERT(cname1, cname2)	\
	NWfiInsque(&((cname1)->childNames), &((cname2)->childNames))
#define NAME_REMOVE(cnamep)		\
	NWfiRemque(&((cnamep)->childNames))
	

	NVLT_ENTER(2);

	/*
	 * Caller must have a softhold on this snode.
	 */
	NVLT_ASSERT(snode->softHoldCount >= 1);

	if (snode->nodeState == SNODE_STALE) {
		NVLT_VLEAVE();
		return;
	}

	NWFI_LIST_INIT(&(discardList.childNames));

	/* following unprotected check is okay */
	force = NWFI_VOLUME_UNMOUNTING(snode->nodeVolume->volFlushData);

	SNODE_LOCK(snode);

	/*
	 * Release hold from stale attributes upon the snode
	 */
	if ((snode->nodeFlags & SNODE_AT_VALID) &&
	    (NUCFS_STALE_ATTRIBS(&snode->cacheInfo.beforeTime) || force)) {
		snode->nodeFlags &= ~SNODE_AT_VALID;
		--snode->softHoldCount;
		NVLT_ASSERT(snode->softHoldCount >= 1);
		++flstats_nodeattr;
	}  


	/*
	 * Release stale attributes on client handles.
	 */
	clientHandle = baseHandle = &snode->clientHandle;

	do {
		NVLT_ASSERT(clientHandle->handleState == SNODE_CHANDLE ||
		       clientHandle->handleState == SNODE_EHANDLE ||
		       clientHandle->handleState == SNODE_MARKER);
		/*
		 * Skip over list markers, invalid embedded
		 * client handle.
		 */
		if (clientHandle->handleState != SNODE_CHANDLE) {
			clientHandle = clientChainToClient(
				clientToClientChain(clientHandle)->flink);
			continue;
		}
		 
		/*
		 * Place the marker, since we will need to drop the 
		 * snode lock.
		 */
		NWfiInsque(clientToClientChain(marker),
			clientToClientChain(clientHandle));
		/*
		 * PERF: It is *very* desirable to avoid the following
		 * lock drop forced by hierarchy considerations. To be
		 * substituted by a TRYLOCK when we implement the
		 * NWFI_TRYLOCK facility!
		 *	This is all the more useful here, because in the
		 * majority of cases, when we avoid dropping the SNODE_LOCK,
		 * we would also not have to insert the marker for the
		 * client chain, since we expect to release client handle
		 * holds inline most of the time.
		 */

		/*
		 * PERF: As an optimization, when we invalidate
		 * client handle attributes, we use the soft hold that
		 * is reaped from that invalidation to cover the
		 * traversal of the name list.
		 */
		if ((clientHandle->clientHandleFlags & NWCH_AT_VALID) &&
		    (NUCFS_STALE_ATTRIBS(&clientHandle->cacheInfo.beforeTime)
				|| force)) {
			clientHandle->clientHandleFlags &= ~NWCH_AT_VALID;
			++flstats_chattr;
		} else {
			++clientHandle->holdCount;
		}
		dropClientHolds = 1;
		SNODE_UNLOCK(snode);

		CHANDLE_NAME_LOCK(clientHandle);
		anchorName = (NWFS_CHILD_NAME_T *)(&(clientHandle->namesList));
		childName = NAME_NEXT(anchorName);
		if (childName != anchorName) {
			do {
				nextChildName = NAME_NEXT(childName);
				if (NAME_STALE(childName) || force) {
					NAME_REMOVE(childName);
					NAME_INSERT(childName, &discardList);
					++flstats_namerele;
				}
				childName = nextChildName;
			} while (childName != anchorName);
			if (NWFI_LIST_EMPTY(&(clientHandle->namesList))) {
				++dropClientHolds;
			}
		}
		CHANDLE_NAME_UNLOCK(clientHandle);

		SNODE_LOCK(snode);
		/*
		 * We have completed processing this client handle
		 * for stale names or attributes. Adjust the hold
		 * count as appropriate.
		 */
		if (clientHandle->holdCount > (uint16)dropClientHolds) {
			clientHandle->holdCount -= (uint16)dropClientHolds;	
			clientHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
				clientToClientChain(marker)));	
			NWfiRemque(clientToClientChain(marker));
			continue;
		}

		NVLT_ASSERT((dropClientHolds == 1) || (dropClientHolds == 2));
		NVLT_ASSERT(clientHandle->holdCount == dropClientHolds);

		if (dropClientHolds == 2) {
			--clientHandle->holdCount;
			/* 
			 * The last remaining hold will be dropped after 
			 * we have released the snode lock.
			 */
		}
		SNODE_UNLOCK(snode);
		/* 
		 * Release the last remaining hold, that we inherited on 
		 * the clienthandle. 
		 */
		NWfsReleaseClientHandle(clientHandle);
		SNODE_LOCK(snode);
		clientHandle = clientChainToClient(
				clientToClientChain(marker)->flink);	
		NWfiRemque(clientToClientChain(marker));

	} while (clientHandle != baseHandle);
	SNODE_UNLOCK(snode);

	/*
	 * now let go of the accumulated, discardable childnames.
	 */
	while (!NWFI_LIST_EMPTY(&(discardList.childNames))) {
		childName = NAME_NEXT(&discardList);
		NAME_REMOVE(childName);
		NWFS_FREE_CHILD_NAME(childName);
	}

	NVLT_VLEAVE();
	return;
}

/*
 * Release all name cache holds upon childnodes that are already
 * inactive or stale.
 */
void
NWfsNameCacheRelease(struct NWfsServerNode *snode,
	struct NWfsServerNode *marker)
{
	NWFS_CLIENT_HANDLE_T *clientHandle;
	NWFS_CLIENT_HANDLE_T *baseHandle;
	NWFS_CHILD_NAME_T discardList;
	NWFS_CHILD_NAME_T *anchorName; 
	NWFS_CHILD_NAME_T *nextChildName;
	NWFS_CHILD_NAME_T *childName;
	uint32 childState;
	int dropClientHolds;
/*
 * Convenient macros for referencing child names
 */
#define NAME_NEXT(cnamep)		\
	(NWFS_CHILD_NAME_T *)NWFI_NEXT_ELEMENT(&((cnamep)->childNames))
#define NAME_INSERT(cname1, cname2)	\
	NWfiInsque(&((cname1)->childNames), &((cname2)->childNames))
#define NAME_REMOVE(cnamep)		\
	NWfiRemque(&((cnamep)->childNames))
	
	NVLT_ENTER(2);

	/*
	 * Caller must have a softhold on this snode.
	 */
	NVLT_ASSERT(snode->softHoldCount >= 1);

	NWFI_LIST_INIT(&(discardList.childNames));

	SNODE_LOCK(snode);

	/*
	 * Release stale attributes on client handles.
	 */
	clientHandle = baseHandle = &snode->clientHandle;

	do {
		NVLT_ASSERT(clientHandle->handleState == SNODE_CHANDLE ||
		       clientHandle->handleState == SNODE_EHANDLE ||
		       clientHandle->handleState == SNODE_MARKER);
		/*
		 * Skip over list markers, invalid embedded
		 * client handle, or a client handle without 
		 * a child name list. Of these, it is safe to
		 * perform the last test without the CHANDLE_NAME_LOCK
		 * cover, because the consequence of failing to purge
		 * existing names (in some rare race conditions) is only 
		 * that some snodes will fail to get their names purged
		 * now, and we will need to issue the more expensive
		 * calls to functions that purge name cache soft holds
		 * for one snode at a time.
		 */
		if (clientHandle->handleState != SNODE_CHANDLE ||
		    NWFI_LIST_EMPTY(&(clientHandle->namesList)) ) {
			clientHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
				clientToClientChain(clientHandle)));
			continue;
		}
		/*
		 * Place the marker, since we will need to drop the 
		 * snode lock.
		 */
		NWfiInsque(clientToClientChain(marker),
			clientToClientChain(clientHandle));
		++clientHandle->holdCount;
		dropClientHolds = 1;
		SNODE_UNLOCK(snode);
		CHANDLE_NAME_LOCK(clientHandle);
		anchorName = (NWFS_CHILD_NAME_T *)(&(clientHandle->namesList));
		childName = NAME_NEXT(anchorName);
		if (childName != anchorName) {
			do {
				nextChildName = NAME_NEXT(childName);
				/*
				 * It is safe to examine child node state
				 * without holding the child snode locked,
				 * because the only consquence of an
				 * incorrect reading of the state is 
				 * additional overhead either in 
				 * reestablishing a purged name, or a later
				 * call to NWfsInvalidateNameCache for an
				 * snode that we failed to handle here. 
				 */
				childState = childName->childNode->nodeState;
				if (childState == SNODE_INACTIVE ||
						childState == SNODE_STALE) {
					NAME_REMOVE(childName);
					NAME_INSERT(childName, &discardList);
				}
				childName = nextChildName;
			} while (childName != anchorName);
			if (NWFI_LIST_EMPTY(&(clientHandle->namesList))) {
				++dropClientHolds;
			}
		}
		CHANDLE_NAME_UNLOCK(clientHandle);
		SNODE_LOCK(snode);
		/*
		 * Adjust the hold count as appropriate.
		 */
		if (clientHandle->holdCount > (uint16)dropClientHolds) {
			clientHandle->holdCount -= (uint16)dropClientHolds;	
			clientHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
				clientToClientChain(marker)));	
			NWfiRemque(clientToClientChain(marker));
			continue;
		}
		NVLT_ASSERT((dropClientHolds == 1) || (dropClientHolds == 2));
		NVLT_ASSERT(clientHandle->holdCount == dropClientHolds);
		if (dropClientHolds == 2) {
			--clientHandle->holdCount;
			/* 
			 * The last remaining hold will be dropped after 
			 * we have released the snode lock.
			 */
		}
		SNODE_UNLOCK(snode);
		/* 
		 * Release the last remaining hold, that we inherited on 
		 * the clienthandle. 
		 */
		NWfsReleaseClientHandle(clientHandle);
		SNODE_LOCK(snode);
		clientHandle = clientChainToClient(
				clientToClientChain(marker)->flink);	
		NWfiRemque(clientToClientChain(marker));

	} while (clientHandle != baseHandle);
	SNODE_UNLOCK(snode);
	/*
	 * now let go of the accumulated, discardable childnames.
	 */
	while (!NWFI_LIST_EMPTY(&(discardList.childNames))) {
		childName = NAME_NEXT(&discardList);
		NAME_REMOVE(childName);
		NWFS_FREE_CHILD_NAME(childName);
	}
	NVLT_VLEAVE();
	return;
}

/*
 * Blow away an snode that has flocks on a fatal error.  Since this cannot be a
 * locked call, if the preceding check misses an lwp setting an flock, the lwp
 * will presumably get its own error.  The wakeup is to nudgs lwps waiting for
 * resource handles to drain, e.g.
 */
void
NWfsFlockStale(NWFS_SERVER_NODE_T *snode)
{
	NVLT_ENTER(1);
	NWfsStaleNode(snode);
	SNODE_WAKEUP(snode);
	NVLT_VLEAVE();
}
