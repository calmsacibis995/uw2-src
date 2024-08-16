/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/chandleops.c	1.7.1.22"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/chandleops.c,v 2.64.2.18 1995/02/13 07:42:47 stevbam Exp $"

/*
**  Netware Unix Client
**
**	MODULE:
**		chandleops.c -	The NetWare UNIX Client File System client 
**				handle operations.
**
**	ABSTRACT:
**		The chandleops.c contains the NetWare UNIX Clinet File System
**		client handle functions of the NetWare Client File System
**		layer (NWfs).  See NWfsChandleOpsIntro(3k) for a complete
**		description of the client handle operations.
**
**		The following chandleops operations are contained in this 
**		module:
**			...
*/ 

#include <mem/kmem.h>
#include <util/types.h>
#include <util/debug.h>
#include <util/cmn_err.h>

#include <net/nuc/nwctypes.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nucerror.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <net/tiuser.h>
#include <net/nuc/spilcommon.h>
#include <net/nuc/requester.h>
#include <fs/nucfs/nucfs_tune.h>
#include <fs/nucfs/nwfschandle.h>
#include <fs/nucfs/nucfscommon.h>
#include <fs/nucfs/nwfsvolume.h>
#include <fs/nucfs/nwfsname.h>
#include <fs/nucfs/nwfsnode.h>
#include <fs/nucfs/nucfsglob.h>
#include <fs/nucfs/nwfidata.h>
#include <fs/nucfs/nwficommon.h>
#include <fs/nucfs/flock_cache.h>

/*
 * Define the tracing mask.
 */
#define NVLT_ModMask	NVLTM_fs

STATIC void
NWfsPurgeSnodeFromClientHandleNC(NWFS_CLIENT_HANDLE_T *clientHandle,
				 NWFS_SERVER_NODE_T *staleSnode);



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
 * BEGIN_MANUAL_ENTRY( NWfsChandleOpsIntro(3k), \
 *                     ./man/kernel/nucfs/nwfs/ChandleOpsIntro )
 * NAME
 *     NWfsChandleOpsIntro - Introduction to NetWare Client File System (NWfs)
 *                           client handle operations.
 * SYNOPSIS
 *    #include <nwfscommon.h>
 *    #include <nwfsnode.h>
 *    #include <nwfsvolume.h>
 *    #include <nwfschandle.h>
 *    #include <spilcommon.h>
 *
 * DESCRIPTION
 *    This section describes the NUC File System client handle operations of
 *    the NetWare Client File System layer (NWfs).
 *
 *    For each UNIX user ID (UNIX client) referencing a server node 
 *    (NWFS_SERVER_NODE_T), there is a subordinate client handle object 
 *    (NWFS_CLIENT_HANDLE_T) attached to that node.  Thus a client handle object
 *    is exclusively owned by an indivitual UNIX user ID.  There must only be
 *    one client handle per UNIX user ID referencing a NWFS_SERVER_NODE_T 
 *    object.  There is no paradigm in the UNIX Generic File System for a client
 *    handle object.
 *
 *    The NWFS_CLIENT_HANDLE_T objects are dynamically created when a UNIX 
 *    client accesses the NetWare server node (file, directory, etc) for the
 *    first time.  Additional NWFS_CLIENT_HANDLE_T objects are created and 
 *    attached to the NWFS_SERVER_NODE_T object for other UNIX clients which
 *    reference the same NWFS_SERVER_NODE_T object.  Processes of a UNIX client
 *    are mapped to the NWFS_CLIENT_HANDLE_T object, in order to select the
 *    correct client user context.
 *
 *    Service Protocol Interface Layer (SPIL) resource handles are directly
 *    attached to a NWFS_CLIENT_HANDLE_T objects.  All remote operations on a
 *    NWFS_SERVER_NODE_T object must be made on the client handle object of the
 *    effective UNIX user ID, in order to select the correct SPIL resource 
 *    handle context.
 *
 *    When a UNIX client process opens an exsiting NWFS_SERVER_NODE_T object
 *    which is currently not referenced by other process(es) of the UNIX client,
 *    a new NWFS_CLIENT_HANDLE_T object will be created and attached to the 
 *    NWFS_SERVER_NODE_T object in behalf of the client credentials.  A SPIL
 *    resource handle will be opened with the access modes passed to
 *    NWfsOpenNode(3k), and attached to the NWFS_CLIENT_HANDLE_T object.
 *
 *    When a new UNIX client process inherits file descriptor(s) of another
 *    UNIX client (ie. setuid) which reference a NWFS_SERVER_NODE_T object which
 *    is not opened for exclusive access, and the new UNIX client does not
 *    have a NWFS_CLIENT_HANDLE_T object (ie. has not opened the file itself)
 *    attached to the NWFS_SERVER_NODE_T object, a new NWFS_CLIENT_HANDLE_T
 *    object will be created and attached to the NWFS_SERVER_NODE_T object in
 *    behalf of the new UNIX client.  A SPIL resourece handle will be opened
 *    with the access mode necessary to service the operation (ie. reads =
 *    NW_READ, get name space = NW_READ, writes = NW_WRITE, truncates =
 *    NW_WRITE, set name space = NW_WRITE, etc), and attached to the
 *    NWFS_CLIENT_HANDLE_T object.  This is necessary, as it is unclear which
 *    UNIX Generic File System file structure(s), the new UNIX client inherited.
 *    Additional operations will negotiate missing access mode, if necessary,
 *    by opening a new SPIL resource handle with the previous access mode plus
 *    the new needed access mode.  If the new SPIL resource handle is allocated,
 *    the previous SPIL resource handle will be detached and closed and the new
 *    one will be attached to the NWFS_CLIENT_HANDLE_T object.
 *
 *    When a new UNIX client process inherits file descriptor(s) of another
 *    UNIX client (ie. setuid) which reference a NWFS_SERVER_NODE_T object, 
 *    which is opened for exclusive access, the new UNIX client will attempt to
 *    clone a NWFS_CLIENT_HANDLE_T object onto the existing and only 
 *    NWFS_CLIENT_HANDLE_T object for proxied access to the NetWare SPIL 
 *    resource handle.  The access mode necessary to service the operation 
 *    (ie. reads = NW_READ, get name space = NW_READ, writes = NW_WRITE,
 *    truncates = NW_WRITE, set name space = NW_WRITE, etc) is checked with the
 *    NetWare server and if access is granted, the clone NWFS_CLIENT_HANDLE_T is
 *    proxy attached to the existing NWFS_CLIENT_HANDLE_T object.  Additional
 *    operations will negotiate missing access mode, if necessary, by checking 
 *    access with the server and if access is granted, the mode is attached to
 *    the clone NWFS_CLIENT_HANDLE_T object. 
 *
 *    The only case which can cause this, is when a process opens a NetWare
 *    file exclusively and then changes the effective user ID.  Since it is
 *    impossible to have more than one SPIL resource handle opened
 *    simultaneously for exclusive access,  a new NWFS_CLIENT_HANDLE_T object 
 *    can not be created for the new UNIX client.  The new UNIX client however
 *    was instantiated by the holder of the exclusive access, and thus is 
 *    implicitly granted a proxy in this case.  The cloning NWFS_CLIENT_HANDLE_T
 *    object client access is checked with the server before proxy is allowed, 
 *    thus correctly implementing UNIX access semantics.  There will be a clone
 *    client handle for each inheriting UNIX client in this case .  It is
 *    guaranteed that the existing NWFS_CLIENT_HANDLE_T object will have at
 *    least the access needed by any clone since there can only be one UNIX 
 *    Generic File System file structure (ie. file struct) instance, and the 
 *    access flags can not be changed after open.
 *
 * SEE ALSO
 *    NWfsAllocateDirClientHandle(3k), NWfsAllocateFileClientHandle(3k),
 *    NWfsAllocateRootClientHandle(3k),NWfsRemoveClientHandle(3k),
 *    and NWfsLookUpClientHandle(3k).
 *
 * END_MANUAL_ENTRY
 */


/*
 *	Hold a client handle.
 *
 *	The caller already owns at least one hold on the client handle.
 */
void
NWfsHoldClientHandle(NWFS_CLIENT_HANDLE_T *clientHandle)
{
	NWFS_SERVER_NODE_T	*snode;

	NVLT_ENTER(1);

	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));

	snode = clientHandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(snode));

	SNODE_LOCK(snode);
	++clientHandle->holdCount;
	SNODE_UNLOCK(snode);

	NVLT_VLEAVE();
}

/*
 *	Given a server node, find or allocate a client handle structure
 *	corresponding to the specified credentials.
 *
 *	The caller holds the specified ``serverNode'' (either via a soft hold
 *	or hard hold).
 *
 *	The client handle is returned held.
 */
NWFS_CLIENT_HANDLE_T *
NWfsGetClientHandle(NWFS_SERVER_NODE_T	*serverNode, NWFS_CRED_T *credentials)
{
	NWFS_CLIENT_HANDLE_T	*baseHandle, *clientHandle;
	NWFS_CLIENT_HANDLE_T	*newClientHandle = NULL;
	uint16			stamp;

	NVLT_ENTER(2);

	baseHandle = &serverNode->clientHandle;

	SNODE_LOCK(serverNode);
	for (;;) {
		/*
		 * Look for a credential match, beginning with the client
		 * handle structure embedded in the snode.
		 */
		clientHandle = baseHandle;
		do {
			NVLT_ASSERT(clientHandle->handleState == SNODE_CHANDLE ||
			       clientHandle->handleState == SNODE_EHANDLE ||
			       clientHandle->handleState == SNODE_MARKER);

			/*
			 * Skip over list markers and an invalid embedded
			 * client handle.
			 */
			if (clientHandle->handleState == SNODE_CHANDLE &&
			    NWFI_CRED_MATCH(&clientHandle->credentials,
					    credentials)) {
				NVLT_ASSERT(clientHandle->holdCount != 0);
				++clientHandle->holdCount;
				SNODE_UNLOCK(serverNode);
				goto done;
			}

			/*
			 * Onto the next client handle.
			 */
			clientHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
				clientToClientChain(clientHandle)));
		} while (clientHandle != baseHandle);

		/*
		 * If the embedded handle is available, then use it.
		 */
		if (baseHandle->handleState == SNODE_EHANDLE) {
			NVLT_ASSERT(baseHandle->holdCount == 0);
			NVLT_ASSERT(NWFI_LIST_EMPTY(&baseHandle->namesList));
			NVLT_ASSERT(baseHandle->snode == serverNode);
			NVLT_ASSERT(baseHandle->cloneVnode == NULL);
			NVLT_ASSERT(baseHandle->resourceHandle == NULL);
			NVLT_ASSERT(baseHandle->resourceHoldCount == 0);
			NVLT_ASSERT(!FLOCK_CACHE_LEN(baseHandle));
			baseHandle->handleState = SNODE_CHANDLE;
			baseHandle->credentials = *credentials;
			baseHandle->holdCount = 1;
			baseHandle->clientHandleFlags = 0;
			++serverNode->clientHandleStamp;
			SNODE_SOFT_HOLD_L(serverNode);
			SNODE_UNLOCK(serverNode);
			clientHandle = baseHandle;
			goto done;
		}

		/*
		 * If we have already allocated a new client handle, then
		 * use it now.
		 */
		if (newClientHandle != NULL)
			break;

		/*
		 * Okay, we need to allocate a new client handle and
		 * initialize it.
		 */
		stamp = serverNode->clientHandleStamp;
		SNODE_UNLOCK(serverNode);
		newClientHandle = kmem_zalloc(sizeof(NWFS_CLIENT_HANDLE_T),
					      KM_SLEEP);
		newClientHandle->handleState = SNODE_CHANDLE;
		NWFI_LIST_INIT(&newClientHandle->namesList);
		NWFI_LIST_INIT(&newClientHandle->flockCacheChain);
		newClientHandle->flockCacheLen = 0;
#if defined(DEBUG) || defined(DEBUG_TRACE)
		newClientHandle->inflated = 0;
#endif
		NWFI_CHANDLE_INIT(newClientHandle);
		newClientHandle->snode = serverNode;
		newClientHandle->credentials = *credentials;
		newClientHandle->holdCount = 1;

		/*
		 * No Racing client handle creaters? Then use the one
		 * we just created.
		 */
		SNODE_LOCK(serverNode);
		if (stamp == serverNode->clientHandleStamp)
			break;
	}

	/*
	 * Add the new client handle to the chain.
	 */
	NWfiInsque(clientToClientChain(newClientHandle),
		   clientToClientChain(baseHandle));
	SNODE_SOFT_HOLD_L(serverNode);
	++serverNode->clientHandleStamp;
	SNODE_UNLOCK(serverNode);

	NVLT_LEAVE((uint_t)newClientHandle);
	return newClientHandle;

done:
	/*
	 * Did we race with another LWP to create this client handle? If
	 * so, just drop the memory we allocated.
	 */
	if (newClientHandle != NULL) {
		NWFI_CHANDLE_DEINIT(newClientHandle);
		kmem_free(newClientHandle, sizeof(NWFS_CLIENT_HANDLE_T));
	}

	NVLT_LEAVE((uint_t)clientHandle);
	return clientHandle;
}

/*
 *	Given a server node, attach to a client handle satisfying the caller's
 *	requirements.
 *
 *		If accessFlag is NW_READ, then the caller needs a
 *		clientHandle which is currently reading.
 *
 *		If accessFlag is NW_WRITE, then the caller needs a
 *		clientHandle which is currently writing.
 *
 *	If a clientHandle is passed into this structure, then an attempt
 *	will be made to return a different (next) one satisfying the
 *	caller's requirements.
 *
 *		Choice 1: first accessFlag match with a resource handle
 *			  (following the passed in client handle)
 *		Choice 2: first accessFlag match without a resource hanlle
 *			  (following the passed in client handle)
 *		Choice 3: first accessFlag match with a resource handle
 *		Choise 4: first accessFlag match without a resource handle
 *
 *	The caller holds the specified ``serverNode'' (either via a soft hold
 *	or hard hold).
 *
 *	The client handle is returned held.
 *
 * Comments:
 *	This function exists solely for the benefit of generic OS
 *	implementations which lose track of the identity of the user who is
 *	reading/writing the data. However, for accounting/auditing reasons,
 *	NetWare needs the true identity of the user doing the I/O. If
 *	multiple users are concurrently reading from the file, we can't
 *	get the identity of the user reading particular bytes. However, we
 *	will get the identity of one of the readers. Similarly for writers.
 */
ccode_t
NWfsAttachClientHandle(NWFS_SERVER_NODE_T *serverNode,
	NWFS_CLIENT_HANDLE_T **retClientHandle, uint32 accessFlag)
{
	NWFS_CLIENT_HANDLE_T	*baseHandle, *clientHandle, *oldClientHandle;
	NWFS_CLIENT_HANDLE_T	*choice1 = NULL;
	NWFS_CLIENT_HANDLE_T	*choice2 = NULL;
	NWFS_CLIENT_HANDLE_T	*choice3 = NULL;
	NWFS_CLIENT_HANDLE_T	*choice4 = NULL;

	NVLT_ENTER(3);

	NVLT_ASSERT(SNODE_REASONABLE(serverNode));
	clientHandle = baseHandle = &serverNode->clientHandle;
	oldClientHandle = *retClientHandle;

	/*
	 * Look for a credential match, beginning with the client
	 * handle structure embedded in the snode.
	 */
	SNODE_LOCK(serverNode);
	do {
		NVLT_ASSERT(clientHandle->handleState == SNODE_CHANDLE ||
		       clientHandle->handleState == SNODE_EHANDLE ||
		       clientHandle->handleState == SNODE_MARKER);

		/*
		 * Skip over list markers and an invalid embedded
		 * client handle.
		 */
		if (clientHandle->handleState != SNODE_CHANDLE)
			goto next;

		/*
		 * If our caller is writing, then we need to choose a user
		 * who has recently dirtied the file. If our caller is
		 * reading, then we need to choose a user who is currently
		 * reading. Otherwise, our caller just wants somebody who
		 * has the file open.
		 */
		if (accessFlag == NW_WRITE) {
			if (!(clientHandle->clientHandleFlags &
					(NWCH_WRITE_FAULT|NWCH_DATA_DIRTY)))
				goto next;
		} else if (accessFlag == NW_READ) {
			if (clientHandle->readersCount == 0)
				goto next;
		} else {
			if (clientHandle->cloneVnode == NULL)
				goto next;
		}

		/*
		 * Okay, we now have a suitable client handle.
		 * Now determine if it is the best choice.
		 */
		if (oldClientHandle == NULL) {
			if (clientHandle->resourceHandle != NULL) {
				choice1 = clientHandle;
				break;
			}
			if (choice2 == NULL)
				choice2 = clientHandle;
		} else {
			if (clientHandle->resourceHandle != NULL) {
				if (choice3 == NULL)
					choice3 = clientHandle;
			} else if (choice4 == NULL)
				choice4 = clientHandle;
		}

		/*
		 * Onto the next client handle.
		 */
next:
		if (clientHandle == oldClientHandle)
			oldClientHandle = NULL;
		clientHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
			clientToClientChain(clientHandle)));
	} while (clientHandle != baseHandle);

	/*
	 * If choice1 is not available, see if some other choice is
	 * available.
	 */
	if (choice1 == NULL) {
		if (choice2 != NULL)
			choice1 = choice2;
		else if (choice3 != NULL)
			choice1 = choice3;
		else if (choice4 != NULL)
			choice1 = choice4;
		else {
			/*
			 * No choice available at all.
			 */
			*retClientHandle = NULL;
			SNODE_UNLOCK(serverNode);
			return  NVLT_LEAVE(FAILURE);
		}
	}

	++choice1->holdCount;
	SNODE_UNLOCK(serverNode);

	NVLT_ASSERT(CHANDLE_REASONABLE(choice1));
	*retClientHandle = choice1;
	return  NVLT_LEAVE(SUCCESS);
}

/*
 *	Release a hold on a client handle. If the client handle is now
 *	fully released, then detach it from its parent server node,
 *	close any associated resource handle, and free it.
 *
 *	The caller possesses a hold on the associated server node (independent
 *	from that exerted by the client handle).
 */
void
NWfsReleaseClientHandle(NWFS_CLIENT_HANDLE_T *clientHandle)
{
	NWFS_SERVER_NODE_T	*snode;

	NVLT_ENTER(1);

	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));

	snode = clientHandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(snode));

	SNODE_LOCK(snode);
	if (--clientHandle->holdCount == 0) {
		NVLT_ASSERT(NWFI_LIST_EMPTY(&clientHandle->namesList));
		NVLT_ASSERT(clientHandle->snode == snode);
		NVLT_ASSERT(clientHandle->cloneVnode == NULL);
		NVLT_ASSERT(clientHandle->resourceHandle == NULL);
		NVLT_ASSERT(clientHandle->resourceHoldCount == 0);
		NVLT_ASSERT(!FLOCK_CACHE_LEN(clientHandle));
		if (NWCH_EMBEDDED(clientHandle)) {
			/*
			 * This is the client handle embedded within the
			 * server node.
			 */
			clientHandle->handleState = SNODE_EHANDLE;
			SNODE_UNLOCK(snode);
		} else {
			NWfiRemque(clientToClientChain(clientHandle));
			SNODE_UNLOCK(snode);
			NWFI_CHANDLE_DEINIT(clientHandle);
#if defined(DEBUG) || defined(DEBUG_TRACE)
			clientHandle->handleState = SNODE_DEAD;
#endif
			kmem_free(clientHandle, sizeof(NWFS_CLIENT_HANDLE_T));
		}
		SNODE_SOFT_RELEASE(snode);
	} else {
		SNODE_UNLOCK(snode);
	}
}

/*
 *	Open a resourceHandle for the given clientHandle, satisfying the
 *	requested access. If a suitable resource handle already exits,
 *	then use it.
 */
ccode_t
NWfsOpenResourceHandle(NWFS_CLIENT_HANDLE_T *clientHandle, uint32 accessFlags,
	int authenticateFlag, int canReplaceFlag, int *newlyCreated,
	enum NUC_DIAG *diagnostic)
{
	NWFS_SERVER_NODE_T	*serverNode;
	int			retries = 0;
	char			*nodeName;
	int32			openFlags;
	NWFS_CACHE_INFO_T	cacheInfo;
	NWSI_NAME_SPACE_T	*nameSpaceInfo;
	nwcred_t		nucCredentials;
	int			gotListLock;

	NVLT_ENTER(6);

	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));

	serverNode = clientHandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(serverNode));

	nameSpaceInfo = kmem_alloc(sizeof(NWSI_NAME_SPACE_T), KM_SLEEP);

	*newlyCreated = FALSE;
	SNODE_LOCK(serverNode);
	for (;;) {
		/*
		 * Don't open a handle for a stale node.
		 */
		if (serverNode->nodeState == SNODE_STALE) {
			SNODE_UNLOCK(serverNode);
			*diagnostic = NUCFS_STALE;
			kmem_free(nameSpaceInfo, sizeof(NWSI_NAME_SPACE_T));
			return NVLT_LEAVE(FAILURE);
		}

		/*
		 * Wait if:
		 *	=> some LWP needs all handles closed, or
		 *	=> the current handle is in transition
		 */
		if (serverNode->closeLockCount != 0 ||
		    (clientHandle->clientHandleFlags & NWCH_RH_INTRANS)) {
			NVLT_ASSERT(
				(clientHandle->clientHandleFlags &
				 NWCH_RH_INTRANS) ? 
					clientHandle->resourceHoldCount >= 1
				      : 1);
			SNODE_WAIT(serverNode);
			SNODE_LOCK(serverNode);
			continue;
		}

		/*
		 * Generate a resource hold. This hold might apply to
		 * either the old handle, or to a handle about to
		 * be created.
		 */
		++clientHandle->resourceHoldCount;

		if (clientHandle->resourceHandle != NULL) {
			/*
			 * If we need write access, and the previous handle
			 * is not writable, then close it out. Note that
			 * this should not harm the reader, since readers
			 * always need to tolerate handle closings.
			 */
			if ((accessFlags & NW_WRITE) &&
			   !(clientHandle->clientHandleFlags & NWCH_RH_WRITE)) {
				if (FLOCK_CACHE_ACTIVE(clientHandle) &&
				    !canReplaceFlag) {

					/*
					 * We cannot destroy the lock cache
					 * unless given permission.
					 */
					SNODE_UNLOCK(serverNode);
					SNODE_WAKEUP(serverNode);
					NWfsReleaseResourceHandle(clientHandle);
					*diagnostic = NUCFS_ACCESS_DENIED;
					kmem_free(nameSpaceInfo,
						  sizeof(NWSI_NAME_SPACE_T));
					return NVLT_LEAVE(FAILURE);
				}
				clientHandle->clientHandleFlags |=
					NWCH_RH_DRAINING;
				SNODE_UNLOCK(serverNode);
				NWfsReleaseResourceHandle(clientHandle);
				SNODE_LOCK(serverNode);
				continue;
			}

			/*
			 * Previous existing handle is suitable. Return it
			 * to the caller.
			 *
			 * If handle was newly created, then return this
			 * to the caller, plus clear the newly created
			 * indicator.
			 */
			if (clientHandle->clientHandleFlags &
						NWCH_RH_NEWLY_CREATED) {
				clientHandle->clientHandleFlags &=
						~NWCH_RH_NEWLY_CREATED;
				*newlyCreated = TRUE;
			}

			SNODE_UNLOCK(serverNode);

			kmem_free(nameSpaceInfo, sizeof(NWSI_NAME_SPACE_T));

			return (NVLT_LEAVE(SUCCESS));
		}

		/*
		 * No previous opened resource handle exits.
		 * So open one now.
		 */
		NVLT_ASSERT(serverNode->closeLockCount == 0);
		NVLT_ASSERT((clientHandle->clientHandleFlags & 
			NWCH_RH_INTRANS) == 0);
		++serverNode->openResourceCount;
		++clientHandle->holdCount;
		NVLT_ASSERT(clientHandle->resourceHoldCount == 1);
		clientHandle->clientHandleFlags |= NWCH_RH_CREATING;
		SNODE_UNLOCK(serverNode);

		/*
		 * NWsiOpenNode returns current nameSpaceInfo.
		 */
		cacheInfo.stale = FALSE;

		/*
		 * Convert NWfs credentials to NUC credentials.
		 */
		NWfiFsToNucCred(&clientHandle->credentials, &nucCredentials);

		for (;;) {
			/*
			 * Set up parameters by node type.
			 */
			if (serverNode->nodeType == NS_ROOT ||
			    serverNode->nodeType == NS_DIRECTORY) {
				openFlags = NOT_USED;
			} else {
				openFlags = accessFlags;
			}
			nodeName = NULL;
			nameSpaceInfo->nodeNumber = serverNode->nodeNumber;
			nameSpaceInfo->nodeType = serverNode->nodeType;

			NWFI_GET_TIME(&cacheInfo.beforeTime);

			/*
			 * NWsiOpenNode will set clientHandle->resourceHandle
			 * to NULL should it return FAILURE.
			 */
			if (NWsiOpenNode (&nucCredentials,
				    serverNode->nodeVolume->spilVolumeHandle,
				    NULL, nodeName, serverNode->nodeType,
				    openFlags, &clientHandle->resourceHandle,
				    nameSpaceInfo, diagnostic) == SUCCESS) {

				/*
				 * Verify node type and node number.
				 *
				 * Notes:
				 *	Attributes are not present for a
				 *	directory.
				 *
				 *	The netware server supports file sizes
				 *	upto 2**32 - 1. The local client's OS
				 *	might not.  If this file is too big
				 *	to be supported by the local OS, then
				 *	we fail the open.
				 */
				if (serverNode->nodeType == NS_ROOT ||
				    serverNode->nodeType == NS_DIRECTORY ||
				    (nameSpaceInfo->nodeType ==
						serverNode->nodeType &&
				     nameSpaceInfo->nodeNumber ==
						serverNode->nodeNumber &&
				     (NWFI_OFF_T) nameSpaceInfo->nodeSize 
						>= 0)) {
					break;
				}
				/*
				 * Verification failure (i.e. we got trash
				 * back from the server, or the generation
				 * count got reused).
				 */
				NWfsStaleNode(serverNode);
				SNODE_LOCK(serverNode);
				NVLT_ASSERT(clientHandle->resourceHoldCount ==
					    1);
				NVLT_ASSERT(!FLOCK_CACHE_LEN(clientHandle));
				clientHandle->clientHandleFlags &=
							~NWCH_RH_CREATING;
				clientHandle->clientHandleFlags |=
							NWCH_RH_DRAINING;
				SNODE_UNLOCK(serverNode);
				SNODE_WAKEUP(serverNode);
				NWfsReleaseResourceHandle(clientHandle);
				*diagnostic = NUCFS_STALE;
				kmem_free(nameSpaceInfo,
					  sizeof(NWSI_NAME_SPACE_T));

				return NVLT_LEAVE(FAILURE);
			}

			switch (*diagnostic) {
			/*
			 * The server is up, but the user has lost either
			 * authentication or connection. So, (re)authenticate.
			 * But, we only try this once.
			 */
			case SPI_AUTHENTICATION_FAILURE:
			case SPI_BAD_CONNECTION:
				if (SNODE_HAS_FLOCK(serverNode)) {
					NWfsFlockStale(serverNode);
					goto fail_common;
				}
				if (++retries > 1 || !authenticateFlag)
					goto fail_common;
				break;
			/*
			 * File or directory has been deleted on the server.
			 */
			case SPI_NO_SUCH_DIRECTORY:
			case SPI_NODE_NOT_FOUND:
			case SPI_INVALID_PATH:
				if (SNODE_HAS_FLOCK(serverNode))
					NWfsFlockStale(serverNode);
				else
					NWfsStaleNode(serverNode);
				*diagnostic = NUCFS_STALE;
				/* FALLTHROUGH */
			default:
				goto fail_common;
			}

			if (NWsiAutoAuthenticate(&nucCredentials, 
				    serverNode->nodeVolume->spilVolumeHandle,
				    NUC_SINGLE_LOGIN_ONLY,
				    diagnostic) != SUCCESS) {
				goto fail_common;
			}
		} /* for */

		/*
		 * Cache attributes returned from the server.
		 *
		 * Note: attributes are not present for a directory.
		 */
		if ((serverNode->nodeType == NS_ROOT) ||
		    (serverNode->nodeType == NS_DIRECTORY)) {

			/* Open done. Adjust clientHandleFlags */
			SNODE_LOCK(serverNode);
			NVLT_ASSERT(clientHandle->resourceHoldCount == 1);
			clientHandle->clientHandleFlags &= ~NWCH_RH_CREATING;
			SNODE_UNLOCK(serverNode);
		} else {
			NWFI_GET_TIME(&cacheInfo.afterTime);
			NWfsUpdateNodeInfo (&clientHandle->credentials,
					    nameSpaceInfo,
					    &cacheInfo, serverNode);
			/*
			 * Open done. Non-root/non-directory nodes
			 * should be enqued on the timed node list 
			 * (unless they are stale). 
			 */
			
			/*
			 * Unlocked peek at nodeState to see whether  we will
			 * likley need the NUCFS_LIST_LOCK, and if so, to get
			 * it now, avoiding a later lock round-trip.
			 */
			gotListLock = 0;
			if (serverNode->nodeState == SNODE_ACTIVE) {
				NUCFS_LIST_LOCK();
				gotListLock = 1;
			}
			SNODE_LOCK(serverNode);
			NVLT_ASSERT(clientHandle->resourceHoldCount == 1);
			clientHandle->clientHandleFlags &= ~NWCH_RH_CREATING;
			if (accessFlags & NW_WRITE) {
				clientHandle->clientHandleFlags |= 
					NWCH_RH_WRITE;
			}

			/*
			 * PERF:  TRYLOCK and out-of-order UNLOCK would be
			 * useful  here, since * we could avoid the lock
			 * release-and-reacquire roundtrips on the snode lock
			 * (in order to get or release  the nucfs  list lock).
			 */
			if (serverNode->nodeState == SNODE_ACTIVE) {
				if (gotListLock == 0) {

					/*
					 * PERF: a TRYLOCK would be useful
					 * here, since we could avoid the lock
					 * release-and-reacquire roundtrip on
					 * the snode lock (in order to  get the
					 * nucfs list lock).
					 */
			   		SNODE_UNLOCK(serverNode);
			   		NUCFS_LIST_LOCK();
			   		SNODE_LOCK(serverNode);
			   		if (serverNode->nodeState == SNODE_ACTIVE) {
						SNODE_ACTIVE_TO_TIMED(serverNode);
			   		} 
				} else {
					SNODE_ACTIVE_TO_TIMED(serverNode);
				}
				SNODE_UNLOCK(serverNode);
		   		NUCFS_LIST_UNLOCK();
			} else if (gotListLock) {
				SNODE_UNLOCK(serverNode);
				NUCFS_LIST_UNLOCK();
			} else {
				SNODE_UNLOCK(serverNode);
			}
			NVLT_ASSERT((serverNode->nodeState == SNODE_TIMED) || 
				    (serverNode->nodeState == SNODE_STALE) );
		} /* else endif */

		SNODE_WAKEUP(serverNode);
		*newlyCreated = TRUE;
		kmem_free(nameSpaceInfo, sizeof(NWSI_NAME_SPACE_T));
		return (NVLT_LEAVE(SUCCESS));

	} /* for */
	/*
	 * No handle could be opened.
	 */
fail_common:
	SNODE_LOCK(serverNode);
	NVLT_ASSERT(!(clientHandle->clientHandleFlags & NWCH_RH_DRAINING));
	NVLT_ASSERT(clientHandle->clientHandleFlags & NWCH_RH_CREATING);
	NVLT_ASSERT(clientHandle->resourceHoldCount == 1);
	NVLT_ASSERT(clientHandle->resourceHandle == NULL);
	clientHandle->clientHandleFlags &= ~NWCH_RH_CREATING;
	--serverNode->openResourceCount;
	clientHandle->resourceHoldCount = 0;
	--clientHandle->holdCount;
	SNODE_UNLOCK(serverNode);
	SNODE_WAKEUP(serverNode);
	kmem_free(nameSpaceInfo, sizeof(NWSI_NAME_SPACE_T));

	return NVLT_LEAVE(FAILURE);
}

/*
 *	This function attempts to cache away a resource handle created prior
 *	to the creation of the corresponding server node. In practice, it will
 *	only be called when NWsiCreateFileNode finds a pre-existing file.
 */
void
NWfsCacheResourceHandle(NWFS_CLIENT_HANDLE_T *clientHandle,
	SPI_HANDLE_T *resourceHandle, uint32 accessFlags)
{
	NWFS_SERVER_NODE_T	*serverNode;
	nwcred_t		nucCredentials;
	enum	NUC_DIAG	diagnostic = SUCCESS;

	NVLT_ENTER(3);

	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));

	serverNode = clientHandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(serverNode));

	/*
	 * If somebody is asking for all handles to be closed, then
	 * close ours now!
	 *
	 * If somebody else has already created a handle, then just throw
	 * away the caller's handle.
	 *
	 * Don't cache directory handles.
	 */
	NUCFS_LIST_LOCK();
	SNODE_LOCK(serverNode);
	if (serverNode->closeLockCount != 0 ||
	    serverNode->nodeState == SNODE_STALE ||
	    clientHandle->resourceHandle != NULL ||
	    serverNode->nodeType == NS_ROOT ||
	    serverNode->nodeType == NS_DIRECTORY) {
		SNODE_UNLOCK(serverNode);
		NUCFS_LIST_UNLOCK();

		/*
		 * Convert NWfs credentials to NUC credentials.
		 */
		NWfiFsToNucCred(&clientHandle->credentials, &nucCredentials);

		/*
		 * Close out the handle.
		 */
		NWsiCloseNode(&nucCredentials,
			      serverNode->nodeVolume->spilVolumeHandle,
			      resourceHandle, serverNode->nodeType,
			      &diagnostic);
	} else {
		/*
		 * Store away the handle and set the NWCH_RH_NEWLY_CREATED
		 * bit to indicate to a subsequent open that the handle was
		 * newly created.
		 */
		NVLT_ASSERT(!(clientHandle->clientHandleFlags &
			      NWCH_RH_INTRANS));
		NVLT_ASSERT(clientHandle->resourceHoldCount == 0);
		++clientHandle->holdCount;
		++serverNode->openResourceCount;
		clientHandle->resourceHandle = resourceHandle;
		if (accessFlags & NW_WRITE)
			clientHandle->clientHandleFlags |= NWCH_RH_WRITE;
		clientHandle->clientHandleFlags |=
						NWCH_RH_NEWLY_CREATED;

		/*
		 * Make sure that the this server node gets placed on the
		 * timed list if it is not already there.
		 */
		if (serverNode->nodeState == SNODE_ACTIVE)
			SNODE_ACTIVE_TO_TIMED(serverNode);
		SNODE_UNLOCK(serverNode);
		NUCFS_LIST_UNLOCK();
	}

	NVLT_VLEAVE();
}

/*
 *	Release a reference to a resource handle. If the handle is closing
 *	or marked as stale, and the resource hold count drops to 0, then
 *	close the resource handle.
 *
 *	The caller is required to have a hold on the client handle.
 */
void
NWfsReleaseResourceHandle(NWFS_CLIENT_HANDLE_T *clientHandle)
{
	NWFS_SERVER_NODE_T	*snode;
	enum	NUC_DIAG	diagnostic = SUCCESS;
	nwcred_t		nucCredentials;

	NVLT_ENTER(1);

	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));

	snode = clientHandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(snode));

	SNODE_LOCK(snode);
	NVLT_ASSERT(clientHandle->resourceHoldCount != 0);
	NVLT_ASSERT(clientHandle->resourceHandle != NULL);
	NVLT_ASSERT(!(clientHandle->clientHandleFlags & NWCH_RH_CREATING));
	if (clientHandle->resourceHoldCount != 1) {
		--clientHandle->resourceHoldCount;
		SNODE_UNLOCK(snode);
		goto done;
	}

	if (snode->nodeState == SNODE_STALE || snode->nodeType == NS_ROOT ||
	    snode->nodeType == NS_DIRECTORY) {
		/*
		 * If server node is already stale, then close its handles.
		 * Also, directory handles are never cached.
		 */
		clientHandle->clientHandleFlags |= NWCH_RH_DRAINING;
	}

	if (clientHandle->clientHandleFlags & NWCH_RH_DRAINING) {
		SNODE_UNLOCK(snode);

		/*
		 * Convert NWfs credentials to NUC credentials.
		 */
		NWfiFsToNucCred(&clientHandle->credentials, &nucCredentials);

		/*
		 * Close out the resource handle.
		 *
		 * Errors are ignored.
		 */
		NWsiCloseNode(&nucCredentials,
			      snode->nodeVolume->spilVolumeHandle,
			      clientHandle->resourceHandle, snode->nodeType,
			      &diagnostic);

		/*
		 * Okay, resource handle is now closed. So, adjust the state
		 * of the snode and the client handle.
		 *
		 * Note that the resourceHandle exerts a hold on the
		 * clientHandle (which we release now). However, since the
		 * caller is required to have its own hold on the
		 * clientHanlde, there is no chance that the client handle
		 * will be released.
		 */
		SNODE_LOCK(snode);
		clientHandle->resourceHandle = NULL;
		NVLT_ASSERT(clientHandle->resourceHoldCount == 1);
		clientHandle->resourceHoldCount = 0;
		--clientHandle->holdCount;
		--snode->openResourceCount;
		clientHandle->clientHandleFlags &=
				~(NWCH_RH_DRAINING|NWCH_RH_NEWLY_CREATED|
				  NWCH_RH_WRITE);
		NVLT_ASSERT(clientHandle->holdCount != 0);
	} else {
		clientHandle->resourceHoldCount = 0;
		NVLT_ASSERT(clientHandle->holdCount > 1);
	}
	SNODE_UNLOCK(snode);
	SNODE_WAKEUP(snode);
done:
	NVLT_VLEAVE();
}

/*
 *	Close a cached resource handle, if it is not otherwise in use.
 *
 *	The caller is required to have a hold on the client handle.
 */
void
NWfsCloseCachedResourceHandle(NWFS_CLIENT_HANDLE_T *clientHandle)
{
	NWFS_SERVER_NODE_T	*snode;

	NVLT_ENTER(1);

	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));

	snode = clientHandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(snode));

	/*
	 * If the user does not have the file open, or otherwise
	 * have the resource handle in use, then close it now.
	 *
	 * Note that directory resource handles are not cached, so there is
	 * nothing to do here.
	 */
	switch (snode->nodeType) {
	case NS_ROOT:
	case NS_DIRECTORY:
		break;
	default:
		SNODE_LOCK(snode);
		if (clientHandle->resourceHandle != NULL &&
		    !(clientHandle->clientHandleFlags & NWCH_RH_INTRANS) &&
		    clientHandle->resourceHoldCount == 0 &&
		    !FLOCK_CACHE_ACTIVE(clientHandle) &&
		    clientHandle->cloneVnode == NULL) {
			clientHandle->clientHandleFlags |= NWCH_RH_DRAINING;
			++clientHandle->resourceHoldCount;
			SNODE_UNLOCK(snode);
			NWfsReleaseResourceHandle(clientHandle);
			SNODE_WAKEUP(snode);
		} else {
			SNODE_UNLOCK(snode);
		}
		break;
	}

	NVLT_VLEAVE();
}

void
NWfsCloseCachedResourceHandles(NWFS_SERVER_NODE_T *snode)
{
	NWFS_CLIENT_HANDLE_T	*baseHandle, *clientHandle, *marker;

	NVLT_ENTER(1);

	NVLT_ASSERT(SNODE_REASONABLE(snode));

	/*
	 * Don't bother with directories, because directory resource handles
	 * are not cached.
	 *
	 * Optimization: Take an unlocked peek at openResourceCount.
	 */
	if (snode->nodeType == NS_ROOT || snode->nodeType == NS_DIRECTORY ||
	    snode->openResourceCount == 0) {
		NVLT_VLEAVE();
		return;
	}

	/*
	 * Create a list marker for the client handle list scan.
	 * The marker allows us to release the snode lock during the
	 * scan and wait.
	 */
	SNODE_CREATE_MARKER(&marker);

	/*
	 * Prepare for the scan.
	 */
	clientHandle = baseHandle = &snode->clientHandle;

	SNODE_LOCK(snode);
		
	do {
		NVLT_ASSERT(clientHandle->handleState == SNODE_CHANDLE ||
		       clientHandle->handleState == SNODE_EHANDLE ||
		       clientHandle->handleState == SNODE_MARKER);
		/*
		 * Skip over list markers, and an invalid embedded
		 * client handle, a client handle without a resource
		 * handle, or a resource handle which is in use.
		 */
		if (clientHandle->handleState != SNODE_CHANDLE ||
		    clientHandle->resourceHandle == NULL ||
		    (clientHandle->clientHandleFlags & NWCH_RH_INTRANS) ||
		    clientHandle->resourceHoldCount != 0 ||
		    FLOCK_CACHE_ACTIVE(clientHandle) ||
		    clientHandle->cloneVnode != NULL) {
			clientHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
				clientToClientChain(clientHandle)));
			continue;
		}

		/*
		 * Hold the client handle, so that it doesn't disappear
		 * while we release the SNODE_LOCK. Also, insert a list
		 * marker to keep our position in the list just in case our
		 * client handle disappears when we release it.
		 */
		clientHandle->clientHandleFlags |= NWCH_RH_DRAINING;
		++clientHandle->resourceHoldCount;
		++clientHandle->holdCount;
		NWfiInsque(clientToClientChain(marker),
			   clientToClientChain(clientHandle));
		SNODE_UNLOCK(snode);

		/*
		 * Invalidate client handle attributes.
		 *
		 * Onto the next client handle.
		 */
		NWfsReleaseResourceHandle(clientHandle);
		NWfsReleaseClientHandle(clientHandle);

		SNODE_LOCK(snode);
		clientHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
			clientToClientChain(marker)));
		NWfiRemque(clientToClientChain(marker));

	} while (clientHandle != baseHandle);

	SNODE_UNLOCK(snode);
	SNODE_WAKEUP(snode);

	SNODE_DESTROY_MARKER(marker);

	NVLT_VLEAVE();
}

/*
 *	Close the resource handle associated with the specified
 *	client handle. If the resource handle is held by another
 *	LWP, then just mark it as draining.
 *
 *	The caller owns a hold for the client handle on entry to this
 *	function and at exit. The caller owns a hold on the associated
 *	resource handle at entry, but gives up this hold before exit.
 */

void
NWfsCloseResourceHandle(NWFS_CLIENT_HANDLE_T *clientHandle)
{
	NWFS_SERVER_NODE_T 	*serverNode;
	NVLT_ENTER(1);

	serverNode = clientHandle->snode;

	SNODE_LOCK(serverNode);
	NVLT_ASSERT(!(clientHandle->clientHandleFlags & NWCH_RH_CREATING));
	if (!FLOCK_CACHE_ACTIVE(clientHandle))
		clientHandle->clientHandleFlags |= NWCH_RH_DRAINING;
	SNODE_UNLOCK(serverNode);
	NWfsReleaseResourceHandle(clientHandle);

	NVLT_VLEAVE();
}

/*
 *	Acquire the handle close lock (closeLockCount) for the specified snode
 *	and then close all resource handles.
 */
ccode_t
NWfsCloseAllHandles(
	NWFS_SERVER_NODE_T	*serverNode,
	enum NUC_DIAG		*diag)
{
	ccode_t			result = SUCCESS;
	NWFS_CLIENT_HANDLE_T	*baseHandle, *clientHandle, *marker;

	NVLT_ENTER(2);

	/*
	 * Create a list marker for the client handle list scan.
	 * The marker allows us to release the snode lock during the
	 * scan and wait.
	 */
	SNODE_CREATE_MARKER(&marker);

	/*
	 * Prepare for the scan.
	 */
	clientHandle = baseHandle = &serverNode->clientHandle;

	/*
	 * Bump up the number of reasons why handles must stay closed.
	 */
	SNODE_LOCK(serverNode);
	++serverNode->closeLockCount;

	do {
		NVLT_ASSERT(clientHandle->handleState == SNODE_CHANDLE ||
		       clientHandle->handleState == SNODE_EHANDLE ||
		       clientHandle->handleState == SNODE_MARKER);

		/*
		 * Skip over list markers, unused embedded
		 * client handle, and clientHandles which don't
		 * possess and are not about to possess a resource 
		 * handle.
		 */
		if (clientHandle->handleState != SNODE_CHANDLE ||
			(!(clientHandle->clientHandleFlags & NWCH_RH_INTRANS)
	    		  && (clientHandle->resourceHandle == NULL) )) {
			clientHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
				clientToClientChain(clientHandle)));
			continue;
		}

		/*
		 * Hold the client handle, so that it doesn't disappear while
		 * we release the lock. Also, insert a list marker to keep our
		 * position in the list just in case our client handle
		 * disappears when we release it.
		 */
		++clientHandle->holdCount;
		NWfiInsque(clientToClientChain(marker),
			   clientToClientChain(clientHandle));

		do {
			switch (clientHandle->clientHandleFlags &
				NWCH_RH_INTRANS) {
			case NWCH_RH_CREATING:
				NVLT_ASSERT(clientHandle->resourceHoldCount ==
					    1);
				/* FALLTHROUGH */
			case NWCH_RH_DRAINING:
				NVLT_ASSERT(clientHandle->resourceHoldCount >=
					    1);
				/*
				 * We need to wait for the in-progress
				 * operation (open or close) to finish.
				 * Make sure we hold the clientHandle
				 * while we wait.
				 */
				SNODE_WAIT(serverNode);
				break;

			default:
				/*
				 * Resource handle is not in transition.
				 * So therefore, force it to close unless
				 * it covers flocks or there is a flock
				 * op in progress.
				 *
				 * TODO:  Better synchronization with flock
				 * activity would avoid unnecessary failures of
				 * rename and unlink operations.  (Consider the
				 * case that FLOCK_CACHE_ACTIVE is true only
				 * because a single lock is being removed.)
				 */
				NVLT_ASSERT(!(clientHandle->clientHandleFlags &
					NWCH_RH_INTRANS));
				NVLT_ASSERT(clientHandle->resourceHandle 
					!= NULL);
				if (FLOCK_CACHE_ACTIVE(clientHandle)) {
					result = FAILURE;
					*diag = SPI_FILE_IN_USE;
					goto endWhile;
				}
				clientHandle->clientHandleFlags |=
							NWCH_RH_DRAINING;
				++clientHandle->resourceHoldCount;
				SNODE_UNLOCK(serverNode);
				NWfsReleaseResourceHandle(clientHandle);
			}
			SNODE_LOCK(serverNode);
		} while (clientHandle->resourceHandle != NULL);

	endWhile:

		NVLT_ASSERT((clientHandle->clientHandleFlags &
			NWCH_RH_INTRANS) == 0);

		/*
		 * Ok, done with this client handle.
		 */
		SNODE_UNLOCK(serverNode);
		NWfsReleaseClientHandle(clientHandle);

		/*
		 * Onto the next client handle.
		 */
		SNODE_LOCK(serverNode);
		clientHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
			clientToClientChain(marker)));
		NWfiRemque(clientToClientChain(marker));

	} while (clientHandle != baseHandle);

#if defined(DEBUG) || defined(DEBUG_TRACE)
	clientHandle = baseHandle = &serverNode->clientHandle;	
	do {
		NVLT_ASSERT(clientHandle->handleState == SNODE_CHANDLE ||
		       clientHandle->handleState == SNODE_EHANDLE ||
		       clientHandle->handleState == SNODE_MARKER);
		if (clientHandle->handleState == SNODE_CHANDLE &&
		    !FLOCK_CACHE_ACTIVE(clientHandle)) {
			NVLT_ASSERT(clientHandle->resourceHandle == NULL);
			NVLT_ASSERT(!(clientHandle->clientHandleFlags & 
						NWCH_RH_INTRANS));
			NVLT_ASSERT(clientHandle->resourceHoldCount == 0);
		}
		clientHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
					clientToClientChain(clientHandle)));
	} while (clientHandle != baseHandle);
#endif
	if (result != SUCCESS)
		--serverNode->closeLockCount;
	NVLT_ASSERT(result != SUCCESS || serverNode->openResourceCount == 0);
	SNODE_UNLOCK(serverNode);

	SNODE_DESTROY_MARKER(marker);

	return NVLT_LEAVE(result);
}

/*
 *
 */
void
NWfsAllowResourceHandlesToOpen(NWFS_SERVER_NODE_T *serverNode)
{
	NVLT_ENTER(1);

	SNODE_LOCK(serverNode);
	NVLT_ASSERT(serverNode->openResourceCount == 0);
	NVLT_ASSERT(serverNode->closeLockCount != 0);
	--serverNode->closeLockCount;
	SNODE_UNLOCK(serverNode);
	SNODE_WAKEUP(serverNode);

	NVLT_VLEAVE();
}

/*
 *	Enter a name into the name cache for the associated clientHandle.
 */

void
NWfsCacheName(
	NWFS_CLIENT_HANDLE_T *clientHandle, 
	NWFS_NAME_T *hashName,
	NWFS_SERVER_NODE_T *childNode)
{
	NWFS_SERVER_NODE_T	*foundNode = NULL;
	NWFS_CHILD_NAME_T	*anchorName, *nextName, *newName = NULL;
	uint16			stamp;
	int			doClientHandleHold = FALSE;

	NVLT_ENTER(3);

	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));

	if (NWFI_DONT_CACHE_NAME(clientHandle)) {
		NVLT_VLEAVE();
		return;
	}

	anchorName = (NWFS_CHILD_NAME_T *)(&(clientHandle->namesList));

	CHANDLE_NAME_LOCK(clientHandle);
	for (;;) {

		nextName = (NWFS_CHILD_NAME_T *) anchorName->childNames.flink;
		while (nextName != anchorName) {
			if (nextName->hashedName == hashName) {
				/*
				 * Okay, we found a name. Update the
				 * time.
				 */
				NWFI_GET_CLOCK(nextName->cacheTime);

				/* 
				 * But does this name have the right child
				 * server node attached? If not fix things
				 * right.
				 */
				foundNode = nextName->childNode;
				NVLT_ASSERT(SNODE_REASONABLE(foundNode));
				if (foundNode != childNode) {
					nextName->childNode = childNode;
					CHANDLE_NAME_UNLOCK(clientHandle);
					SNODE_CHILDNAME_SOFT_HOLD(childNode);
					SNODE_CHILDNAME_SOFT_RELEASE(foundNode);
				} else {
					CHANDLE_NAME_UNLOCK(clientHandle);
				}
				goto done;
			}
			nextName = (NWFS_CHILD_NAME_T *)
						nextName->childNames.flink;
		}

		/*
		 * If we have already allocated a new name, then
		 * use it now.
		 */
		if (newName != NULL)
			break;

		/*
		 * Okay, we need to allocate a new child name structure
		 * and initialize it.
		 */
		stamp = clientHandle->nameStamp;
		CHANDLE_NAME_UNLOCK(clientHandle);
		newName = kmem_zalloc(sizeof(NWFS_CHILD_NAME_T), KM_SLEEP);

		newName->hashedName = hashName;
		newName->childNode = childNode;
		NWFI_GET_CLOCK(newName->cacheTime);

		/*
		 * No racing name handle creaters? Then use the one
		 * we just created.
		 */
		CHANDLE_NAME_LOCK(clientHandle);
		if (stamp == clientHandle->nameStamp)
			break;
	}

	/*
	 * Don't cache a name if either the parent or the child
	 * is stale.
	 */
	SNODE_LOCK(childNode);
	if ((clientHandle->clientHandleFlags & NWCH_DESTROY) ||
	    childNode->nodeState == SNODE_STALE) {
		SNODE_UNLOCK(childNode);
		CHANDLE_NAME_UNLOCK(clientHandle);
		goto done;
	}

	/*
	 * Add a new hash name to the chain.
	 */
	if (NWFI_LIST_EMPTY(&anchorName->childNames))
		doClientHandleHold = TRUE;
	NWfiInsque(&newName->childNames, &anchorName->childNames);
	++clientHandle->nameStamp;
	SNODE_CHILDNAME_SOFT_HOLD_L(childNode);
	SNODE_UNLOCK(childNode);
	CHANDLE_NAME_UNLOCK(clientHandle);

	/*
	 * Now take holds on other objects as necessary.
	 */
	NWFS_NAME_HOLD(hashName);
	if (doClientHandleHold)
		NWfsHoldClientHandle(clientHandle);

	NVLT_VLEAVE();
	return;

done:
	/*
	 * Did we race with another LWP to create this child name? If
	 * so, just drop the memory we allocated.
	 */
	if (newName != NULL)
		kmem_free(newName, sizeof(NWFS_CHILD_NAME_T));

	NVLT_VLEAVE();
	return;
}

/*
 *	Find the child node for the associated clientHandle and name (if
 *	one exists).
 *
 *	Returns the foundNode soft held.
 */
NWFS_SERVER_NODE_T *
NWfsSearchByName(NWFS_CLIENT_HANDLE_T *clientHandle, NWFS_NAME_T *hashName)
{
	NWFS_SERVER_NODE_T	*foundNode = NULL;
	NWFS_CHILD_NAME_T	*anchorName, *nextName;
	int			doRelease;

	NVLT_ENTER(2);

	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
	anchorName = (NWFS_CHILD_NAME_T *) (&(clientHandle->namesList));

	CHANDLE_NAME_LOCK(clientHandle);

	nextName = (NWFS_CHILD_NAME_T *) anchorName->childNames.flink;
	while (nextName != anchorName) {
		if (nextName->hashedName == hashName) {
			foundNode = nextName->childNode;
			NVLT_ASSERT(SNODE_REASONABLE(foundNode));
			/*
			 * Name timed out? If so delete it.
			 */
			if (NUCFS_CLOCK_STALE(nextName->cacheTime))
				goto stale;

			/*
			 * Okay, we found an up-to-date name in the
			 * cache. Return the referenced server
			 * node soft held.
			 */
			SNODE_SOFT_HOLD(foundNode);
			CHANDLE_NAME_UNLOCK(clientHandle);
			NVLT_LEAVE((uint_t)foundNode);
			return foundNode;
		}
		nextName = (NWFS_CHILD_NAME_T *) nextName->childNames.flink;
	}
	CHANDLE_NAME_UNLOCK(clientHandle);

	return (NWFS_SERVER_NODE_T *) NVLT_LEAVE(NULL);

	/*
	 * Okay, the name is stale. So get rid of it now.
	 */
stale:
	NWfiRemque(&nextName->childNames);
	doRelease = NWFI_LIST_EMPTY(&anchorName->childNames);
	CHANDLE_NAME_UNLOCK(clientHandle);
	if (doRelease)
		NWfsReleaseClientHandle(clientHandle);
	NWFS_FREE_CHILD_NAME(nextName);

	return (NWFS_SERVER_NODE_T *) NVLT_LEAVE(NULL);
}

void
NWfsDiscardChildNames(NWFS_CLIENT_HANDLE_T *clientHandle)
{
	NWFS_CHILD_NAME_T	*firstName, *childName, *nextChildName;

	NVLT_ENTER(1);

	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));

	CHANDLE_NAME_LOCK(clientHandle);
	if (!NWFI_LIST_EMPTY(&clientHandle->namesList)) {
		/*
		 * By removing the list anchor, we effectively
		 * unlink the entire list of child names at once.
		 */
		firstName = childName =
			(NWFS_CHILD_NAME_T *) clientHandle->namesList.flink;
		NWfiRemque(&clientHandle->namesList);
		NWFI_LIST_INIT(&clientHandle->namesList);
		CHANDLE_NAME_UNLOCK(clientHandle);

		/*
		 * Now, discard each child name from the list.
		 * The entire list of child names is now privately
		 * held.
		 */
		do {
			nextChildName = (NWFS_CHILD_NAME_T *)
						childName->childNames.flink;
			NWFS_FREE_CHILD_NAME(childName);
			childName = nextChildName;
		} while (childName != firstName);

		/*
		 * The list is now empty, so release the hold on the
		 * client handle.
		 */
		NWfsReleaseClientHandle(clientHandle);
	} else {
		CHANDLE_NAME_UNLOCK(clientHandle);
	}

	NVLT_VLEAVE();
}


void
NWfsUpdateClientAttributes(NWFS_CLIENT_HANDLE_T *clientHandle,
	NWSI_NAME_SPACE_T *nameSpaceInfo, NWFS_CACHE_INFO_T *cacheInfo)
{
	NWFS_SERVER_NODE_T	*snode;
	int			otherStale = FALSE;
	NWFS_CLIENT_HANDLE_T	*baseHandle, *scanHandle, *marker;

	NVLT_ENTER(3);

	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
	snode = clientHandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(snode));

	/*
	 * Add attributes to the client handle.
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
			&(clientHandle->cacheInfo.afterTime)) ) {
		if (!(clientHandle->clientHandleFlags & NWCH_AT_VALID)) {
			clientHandle->clientHandleFlags |= NWCH_AT_VALID;
			++clientHandle->holdCount;
		}
		if (clientHandle->nodePermissions !=
					nameSpaceInfo->nodePermissions ||
		    clientHandle->userId != nameSpaceInfo->userID ||
		    clientHandle->groupId != nameSpaceInfo->groupID) {
			otherStale = TRUE;
			clientHandle->nodePermissions =
					nameSpaceInfo->nodePermissions;
			clientHandle->userId = nameSpaceInfo->userID;
			clientHandle->groupId = nameSpaceInfo->groupID;
		}
		clientHandle->cacheInfo = *cacheInfo;	/* structure copy */
	}
	SNODE_UNLOCK(snode);

	/*
	 * Do we need need to invalidate the attributes in the other handles?
	 * If our user's attributes changes, then they may have also changed
	 * for other users.
	 */
	if (!otherStale)
		goto done;

	/*
	 * Create a list marker for the client handle list scan.
	 * The marker allows us to release the snode lock during the
	 * scan.
	 */
	SNODE_CREATE_MARKER(&marker);

	/*
	 * invalidate other child handles.
	 */
	baseHandle = &snode->clientHandle;
	scanHandle = baseHandle;
	SNODE_LOCK(snode);
	do {
		NVLT_ASSERT(scanHandle->handleState == SNODE_CHANDLE ||
		       scanHandle->handleState == SNODE_EHANDLE ||
		       scanHandle->handleState == SNODE_MARKER);

		/*
		 * Skip over:
		 *	=> the clientHandle we just updated
		 *	=> list markers
		 *	=> an invalid embedded handle
		 *	=> a handle with invalid attributes
		 */
		if (scanHandle == clientHandle ||
		    scanHandle->handleState != SNODE_CHANDLE ||
		    !(scanHandle->clientHandleFlags & NWCH_AT_VALID)) {
			scanHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
				clientToClientChain(scanHandle)));
			continue;
		}

		/*
		 * Insert a list marker to keep our position in the list
		 * just in case our client handle disappears when we release
		 * it.
		 */
		NWfiInsque(clientToClientChain(marker),
			   clientToClientChain(scanHandle));

		/*
		 * Invalidate the attributes
		 */
		scanHandle->clientHandleFlags &= ~NWCH_AT_VALID;

		/*
		 * Ok, done with this client handle.
		 */
		SNODE_UNLOCK(snode);
		NWfsReleaseClientHandle(scanHandle);

		/*
		 * Onto the next client handle.
		 */
		SNODE_LOCK(snode);
		scanHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
			clientToClientChain(marker)));
		NWfiRemque(clientToClientChain(marker));

	} while (scanHandle != baseHandle);

	SNODE_UNLOCK(snode);

	SNODE_DESTROY_MARKER(marker);
done:
	NVLT_VLEAVE();
}

/*
 * Returns B_TRUE iff all chandles are invalidated.
 */
boolean_t
NWfsInvalidateChandles(NWFS_SERVER_NODE_T *snode)
{
	NWFS_CLIENT_HANDLE_T	*baseHandle, *clientHandle, *marker;
	int			doClose;
	boolean_t		result = B_TRUE;

	NVLT_ENTER(1);

	/*
	 * Create a list marker for the client handle list scan.
	 * The marker allows us to release the snode lock during the
	 * scan and wait.
	 */
	SNODE_CREATE_MARKER(&marker);

	/*
	 * Prepare for the scan.
	 */
	clientHandle = baseHandle = &snode->clientHandle;

	SNODE_LOCK(snode);
	do {
		NVLT_ASSERT(clientHandle->handleState == SNODE_CHANDLE ||
		       clientHandle->handleState == SNODE_EHANDLE ||
		       clientHandle->handleState == SNODE_MARKER);

		/*
		 * Skip over list markers and an invalid embedded
		 * client handle.
		 */
		if (clientHandle->handleState != SNODE_CHANDLE) {
			clientHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
				clientToClientChain(clientHandle)));
			continue;
		}

		/*
		 * Insert a list marker to keep our position in the list
		 * just in case clientHandle disappears when we release it.
		 */
		NWfiInsque(clientToClientChain(marker),
			   clientToClientChain(clientHandle));

		/*
		 * Get rid of a cached resource handle.
		 */
		doClose = FALSE;
		if (FLOCK_CACHE_ACTIVE(clientHandle)) {
			result = B_FALSE;
		} else if (!(clientHandle->clientHandleFlags &
			     NWCH_RH_INTRANS) &&
		    clientHandle->resourceHoldCount == 0 &&
		    clientHandle->resourceHandle != NULL) {
			clientHandle->clientHandleFlags |= NWCH_RH_DRAINING;
			++clientHandle->resourceHoldCount;
			doClose = TRUE;
		}

		/*
		 * Invalidate client handle attributes.
		 *
		 * The clientHandle must remain held through the
		 * NWfsReleaseResourceHandle() operation. So, we take an
		 * extra hold if the attributes are already invalid.
		 * In either case, we will possess a single hold to drop
		 * after the NWfsReleaseResourceHandle().
		 */
		if (clientHandle->clientHandleFlags & NWCH_AT_VALID) {
			clientHandle->clientHandleFlags &= ~NWCH_AT_VALID;
		} else {
			++clientHandle->holdCount;
		}
		SNODE_UNLOCK(snode);

		/*
		 * Invalidate name cache for this client handle.
		 */
		NWfsDiscardChildNames(clientHandle);

		/*
		 * Onto the next client handle.
		 */
		if (doClose)
			NWfsReleaseResourceHandle(clientHandle);
		NWfsReleaseClientHandle(clientHandle);
		SNODE_LOCK(snode);
		clientHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
			clientToClientChain(marker)));
		NWfiRemque(clientToClientChain(marker));

	} while (clientHandle != baseHandle);

	SNODE_UNLOCK(snode);

	SNODE_DESTROY_MARKER(marker);

	return NVLT_LEAVE(result);
}

void
NWfsStaleChandles(NWFS_SERVER_NODE_T *serverNode)
{
	NWFS_CLIENT_HANDLE_T	*baseHandle, *clientHandle, *marker;

	NVLT_ENTER(1);

	/*
	 * Create a list marker for the client handle list scan.
	 * The marker allows us to release the snode lock during the
	 * scan and wait.
	 */
	SNODE_CREATE_MARKER(&marker);

	/*
	 * Prepare for the scan.
	 */
	clientHandle = baseHandle = &serverNode->clientHandle;

	SNODE_LOCK(serverNode);
	do {
		NVLT_ASSERT(clientHandle->handleState == SNODE_CHANDLE ||
		       clientHandle->handleState == SNODE_EHANDLE ||
		       clientHandle->handleState == SNODE_MARKER);

		/*
		 * Skip over list markers and an invalid embedded
		 * client handle.
		 */
		if (clientHandle->handleState != SNODE_CHANDLE) {
			clientHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
				clientToClientChain(clientHandle)));
			continue;
		}

		/*
		 * Hold the client handle, so that it doesn't disappear
		 * while we release the SNODE_LOCK. Also, insert a list
		 * marker to keep our position in the list just in case our
		 * client handle disappears when we release it.
		 *
		 * PERF: The following code sequence could benefit from a
		 *	 trylock style optimization.
		 */
		++clientHandle->holdCount;
		NWfiInsque(clientToClientChain(marker),
			   clientToClientChain(clientHandle));
		SNODE_UNLOCK(serverNode);

		/*
		 * Set the destroy bit in the client handle. For this
		 * operation we need the client handle name lock.
		 */
		CHANDLE_NAME_LOCK(clientHandle);
		SNODE_LOCK(serverNode);
		clientHandle->clientHandleFlags |= NWCH_DESTROY;
		SNODE_UNLOCK(serverNode);
		CHANDLE_NAME_UNLOCK(clientHandle);

		/*
		 * Onto the next client handle.
		 */
		NWfsReleaseClientHandle(clientHandle);
		SNODE_LOCK(serverNode);
		clientHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
			clientToClientChain(marker)));
		NWfiRemque(clientToClientChain(marker));

	} while (clientHandle != baseHandle);

	SNODE_UNLOCK(serverNode);

	SNODE_DESTROY_MARKER(marker);

	NVLT_VLEAVE();
}

/*
 * Purge a stale snode from the name cache of the specified parent snode.
 */
void
NWfsPurgeSnodeFromNC(NWFS_SERVER_NODE_T *snode,
		     NWFS_SERVER_NODE_T *staleSnode)
{
	NWFS_CLIENT_HANDLE_T	*baseHandle, *clientHandle, *marker;

	NVLT_ENTER(2);

	NVLT_ASSERT(SNODE_REASONABLE(snode));

	/*
	 * Only a directory can have child names.
	 */
	if (snode->nodeType != NS_ROOT && snode->nodeType != NS_DIRECTORY) {
		NVLT_VLEAVE();
		return;
	}

	SNODE_CREATE_MARKER(&marker);

	/*
	 * Prepare for the scan.
	 */
	clientHandle = baseHandle = &snode->clientHandle;

	SNODE_LOCK(snode);
	do {
		NVLT_ASSERT(clientHandle->handleState == SNODE_CHANDLE ||
		       clientHandle->handleState == SNODE_EHANDLE ||
		       clientHandle->handleState == SNODE_MARKER);

		/*
		 * Skip over list markers and an invalid embedded
		 * client handle. Also, skip over clientHandles whose
		 * names list is already empty.
		 */
		if (clientHandle->handleState != SNODE_CHANDLE ||
		    NWFI_LIST_EMPTY(&clientHandle->namesList)) {
			clientHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
				clientToClientChain(clientHandle)));
			continue;
		}

		/*
		 * We hold the clientHandle to prevent deallocation in another
		 * LWP following NWfsReleaseClientHandle().
		 *
		 * Insert a list marker to keep our position in the list (since
		 * the clientHandle can be deallocated in another LWP).
		 */
		++clientHandle->holdCount;
		NWfiInsque(clientToClientChain(marker),
			   clientToClientChain(clientHandle));
		SNODE_UNLOCK(snode);

		/*
		 * Now purge the stale Snode from the name cache of the
		 * specified client handle.
		 */
		NWfsPurgeSnodeFromClientHandleNC(clientHandle, staleSnode);

		/*
		 * Ok, done with this client handle.
		 */
		NWfsReleaseClientHandle(clientHandle);

		/*
		 * Onto the next client handle.
		 */
		SNODE_LOCK(snode);
		clientHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
			clientToClientChain(marker)));
		NWfiRemque(clientToClientChain(marker));


	} while (clientHandle != baseHandle);

	SNODE_UNLOCK(snode);

	SNODE_DESTROY_MARKER(marker);

	NVLT_VLEAVE();
}

/* Caller must hold snode RW lock. */
NUCFS_FLOCK_CACHE_T	*
NWfsChandleFlockHold(
	NWFS_CLIENT_HANDLE_T	*chandle,
	NUCFS_LOCK_T		*lock)
{
	NWFS_SERVER_NODE_T	*snode;
	NUCFS_FLOCK_CACHE_T	*result;

	NVLT_ENTER(2);
	NVLT_ASSERT(chandle->handleState == SNODE_CHANDLE);
	NVLT_ASSERT(CHANDLE_REASONABLE(chandle));
	snode = chandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(snode));
	result = NWfsFlockCacheHold(&chandle->flockCacheChain, lock);
	SNODE_LOCK(snode);
	snode->nFlocksCached++;
	chandle->flockCacheLen++;
	SNODE_UNLOCK(snode);
	NVLT_LEAVE(SUCCESS);
	return result;
}

/* Caller must hold snode RW lock. */
void
NWfsChandleCacheRelease(
	NWFS_CLIENT_HANDLE_T	*chandle,
	NUCFS_FLOCK_CACHE_T	*cache,
	uint32			pid)
{
	NWFS_SERVER_NODE_T	*snode;

	NVLT_ENTER(3);
	NVLT_ASSERT(chandle->handleState == SNODE_CHANDLE);
	NVLT_ASSERT(CHANDLE_REASONABLE(chandle));
	snode = chandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(snode));
	NWfsFlockCacheRelease(cache, pid);
	SNODE_LOCK(snode);
	snode->nFlocksCached--;
	chandle->flockCacheLen--;
	SNODE_UNLOCK(snode);
	NVLT_VLEAVE();
}

/* Caller must hold snode RW lock and snode lock. */
void
NWfsChandleCacheRelease_l(
	NWFS_CLIENT_HANDLE_T	*chandle,
	NUCFS_FLOCK_CACHE_T	*cache,
	uint32			pid)
{
	NWFS_SERVER_NODE_T	*snode;

	NVLT_ENTER(3);
	NVLT_ASSERT(chandle->handleState == SNODE_CHANDLE);
	NVLT_ASSERT(CHANDLE_REASONABLE(chandle));
	snode = chandle->snode;
	NWfsFlockCacheRelease(cache, pid);
	snode->nFlocksCached--;
	chandle->flockCacheLen--;
	NVLT_VLEAVE();
}

STATIC void
NWfsPurgeSnodeFromClientHandleNC(NWFS_CLIENT_HANDLE_T *clientHandle,
				 NWFS_SERVER_NODE_T *staleSnode)
{
	NWFS_CHILD_NAME_T	*childName, *nextChildName;
	NWFS_CHILD_NAME_T	discardList;
	int			doRelease = FALSE;

	NVLT_ENTER(2);

	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
	NVLT_ASSERT(SNODE_REASONABLE(staleSnode));

	NWFI_LIST_INIT(&discardList.childNames);

	CHANDLE_NAME_LOCK(clientHandle);
	if (!NWFI_LIST_EMPTY(&clientHandle->namesList)) {
		childName = (NWFS_CHILD_NAME_T *) clientHandle->namesList.flink;
		while (childName !=
			(NWFS_CHILD_NAME_T *) &(clientHandle->namesList)) {

			nextChildName = (NWFS_CHILD_NAME_T *)
						childName->childNames.flink;
			if (childName->childNode == staleSnode) {
				NWfiRemque(&childName->childNames);
				NWfiInsque(&childName->childNames,
					   &discardList.childNames);
			}
			childName = nextChildName;
		}

		if (NWFI_LIST_EMPTY(&clientHandle->namesList))
			doRelease = TRUE;

		CHANDLE_NAME_UNLOCK(clientHandle);

		/*
		 * Any names remaining in the client handle?
		 */
		if (doRelease)
			NWfsReleaseClientHandle(clientHandle);

		/*
		 * Now, discard each child name from the list.
		 * The entire list of discard names is now privately
		 * held.
		 */
		while (!NWFI_LIST_EMPTY(&discardList.childNames)) {
			childName = (NWFS_CHILD_NAME_T *)
						discardList.childNames.flink;
			NWfiRemque(&childName->childNames);
			NWFS_FREE_CHILD_NAME(childName);
		}
	} else {
		CHANDLE_NAME_UNLOCK(clientHandle);
	}

	NVLT_VLEAVE();
}

#ifdef NOTYET

NWfsTimeOutResourceHandles(volume)

/*
 *	For a given server node, free all stale client handles.
 *	Also free stale associated names.
 *
 *	If the serverNode is marked ESTALE, then all resources are considered
 *	to be stale, and are thus released as soon as possible.
 *
 *	serverNode is soft held by the caller.
 */
void
NWfsFreeStaleClientHandles(serverNode)

#endif /* NOTYET */
