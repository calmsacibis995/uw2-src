/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nodeops.c	1.10.1.28"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nodeops.c,v 2.65.2.21 1995/02/13 07:43:53 stevbam Exp $"

/*
**  Netware Unix Client
**
**	MODULE:
**		nodeops.c -	Node operations of the NetWare UNIX Client File
**				System (NWfs) layer.
**
**	ABSTRACT:
**		The nodeops.c contains the NetWare UNIX Client File System node
**		functions of the NetWare Client File System layer (NWfs).  See
**		NWfsNodeOpsIntro(3k) for a complete description of the NetWare
**		UNIX Client layer and its node operations.
**
**		The following nodeops operations are contained in this module:
**			NWfsCreateFileNode ()
**			NWfsDeleteFileNode ()
**			NWfsLinkFile ()
**			NWfsLookUpNode ()
**			NWfsMakeDirNode ()
**			NWfsOpenNode ()
**			NWfsReadBytesOnNode ()
**			NWfsReadDirNodeEntries ()
**			NWfsReleaseNode ()
**			NWfsReleaseNodeHold ()
**			NWfsRemoveDirNode ()
**			NWfsSymbolicLink ()
**			NWfsTruncateBytesOnNode ()
**			NWfsWriteBytesOnNode ()
*/ 

#include <util/types.h>
#include <proc/pid.h>
#include <proc/proc.h>

#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>

#include <net/nuc/nwctypes.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/requester.h>
#include <net/tiuser.h>
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
#include <fs/nucfs/nwfslock.h>
#include <fs/nucfs/flock_cache.h>

#include <fs/nucfs/nucfslk.h>
#include <mem/kmem.h>


/*
 * Define the tracing mask.
 */
#define NVLT_ModMask	NVLTM_fs

#if defined(DEBUG) || defined(DEBUG_TOOLS) || defined(DEBUG_TRACE)
#define	SCOPE	
#else
#define	SCOPE	static
#endif

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
 * BEGIN_MANUAL_ENTRY( NWfsNodeOpsIntro(3k), \
 *                     ./man/kernel/nucfs/nwfs/NodeOpsIntro)
 * NAME
 *    NWfsNodeOpsIntro - Introduction to the NetWare Client File System layer 
 *                       (NWfs) node operations.
 *
 * SYNOPSIS
 *    #include <nucfscommon.h>
 *    #include <nwfsnode.h>
 *    #include <nwfschandle.h>
 *    #include <nwfsvolume.h>
 *    #include <nwfsops.h>
 *
 * DESCRIPTION
 *    The NetWare UNIX Client File System (NUCfs) is broken into two layer: the
 *    Virtual File System Interface layer (NWfi) and the NetWare Client File
 *    System layer (NWfs).  The nodeops.c contains the NetWare Client File
 *    System layer (NWfs) node operations.
 *
 *    The NUC File System is installed as a dependent file system which plugs
 *    into the Virtual File System interface of the UNIX Generic File System 
 *    process structure.  The NWfs layer binds the NetWare network operating
 *    system, file system, into the UNIX Generic File System according to UNIX
 *    semantics.  Thus the purpose of NWfs component of the NUC File System is
 *    to emulate a UNIX file system.  The NWfs uses the underlying Service 
 *    protocal Interface layer (SPIL) operations to communicate with the 
 *    NetWare Servers.  NetWare file systems are represented by server volumes
 *    (NWFS_SERVER_VOLUME_T) on NetWare Servers.  
 *
 *    The NWfs node operations are responsible for managing the NUC File System
 *    server node objects (NWFS_SERVER_NODE_T) and their subordinate client 
 *    handle object (NWFS_CLIENT_HANDLE_T) and lock request cache objects
 *    (NUCFS_FLOCK_CACHE_T). It creates objects representations of the real
 *    node objects on NetWare server volumes (NWFS_SERVER_VOLUME_T).
 *
 *    A server node object (NWFS_SERVER_NODE_T) is paired with the UNIX Generic
 *    File System (GFS) node (INODE, VNODE, etc) to form the focal point for a
 *    NetWare file or directory.  It may have multiple UNIX processes operating
 *    on it simultaneously.  There will be only one server node for a NetWare
 *    file or directory which is being accessed on a UNIX client system.  For
 *    each UNIX user ID (UNIX client) referencing a NWFS_SERVER_NODE_T object,
 *    there is a subordinate client handle object (NWFS_CLIENT_HANDLE_T see
 *    NWfsChandleOpsIntro(3k)) .  Also for each lock request honored by a
 *    server, there is a subordinate lock request cache object
 *    (NUCFS_FLOCK_CACHE_T) in the associated client handle, and a count in
 *    nFlocksCached in the server node.  
 *
 * SEE ALSO
 *    NWfsAllocateNode(3k), NWfsBindNode(3k),
 *    NWfsBlockOperationMap(3k), NWfsCheckNodeAccess(3k),
 *    NWfsCreateFileNode(3k), NWfsDeleteFileNode(3k), NWfsDestroyList(3k),
 *    NWfsFileLock(3K), NWfsLinkFile(3k),
 *    NWfsLookUpNode(3k), NWfsMakeDirNode(3k), NWfsManageNameSpaceInfo(3k),
 *    NWfsManageNameSpaceInfoByName(3k), NWfsModifyNameSpaceInfo(3k),
 *    NWfsOpenNode(3k), NWfsReadBlocksOnNode(3k),
 *    NWfsReadBytesOnNode(3k), NWfsReadDirNodeEntries(3k), NWfsReleaseNode(3k),
 *    NWfsReleaseNodeHold(3k), NWfsRemoveDirNode(3k), NWfsSymbolicLink(3k),
 *    NWfsTruncateBytesOnNode(3k), NWfsWriteBlocksOnNode(3k), and
 *    NWfsWriteBytesOnNode(3k).
 *
 * END_MANUAL_ENTRY
 */


STATIC	ccode_t
NWfsFileLockOpenHandle(
	NWFS_CLIENT_HANDLE_T	*chandle,
	uint32			accessFlags,
	int			authenticateFlag,
	int			*newlyCreated,
	enum NUC_DIAG		*diag);

STATIC	void
NWfsFileLockReleaseHandle(
	NWFS_CLIENT_HANDLE_T	*chandle,
	ccode_t			result,
	enum NUC_DIAG		diag);

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
 * BEGIN_MANUAL_ENTRY( NWfsCreateFileNode(3k), \
 *                     ./man/kernel/nucfs/nwfs/CreateFileNode )
 * NAME
 *    NWfsCreateFileNode - Create a non-directory (file) entry in the specified
 *                         parentNode directory.
 *
 * SYNOPSIS
 *    ccode_t 
 *    NWfsCreateFileNode (credentials, parentNode, fileName, nameSpaceInfo,
 *                        exclusive, newFileNode, diagnostic)
 *    opaque_t          *credentials;
 *    opague_t          *parentNode;      Object is opaque to caller.
 *    char              *fileName;
 *    NWSI_NAME_SPACE_T *nameSpaceInfo;
 *    boolean_t         exclusive;
 *    opague_t          **newFileNode;    Object is opaque to caller.
 *    enum  NUC_DIAG    *diagnostic;
 *
 * INPUT
 *    credentials               - Credentials of the UNIX client creating
 *                                the file.
 *    parentNode                - Server node object representing the
 *                                directory to create the server file node in.
 *    fileName                  - Name of the file to be created.
 *    nameSpaceInfo->nodeType   - Set to the following object type:
 *                                NS_FILE
 *                                NS_CHARACTER_SPECIAL   (Not in DOS Name Space)
 *                                NS_BLOCK_SPECIAL       (Not in DOS Name Space)
 *                                NS_FIFO                (Not in DOS Name Space)
 *                                NS_SYMBOLIC_LINK       (Not in DOS Name Space)
 *    nameSpaceInfo->nodePermission - Set to an inclusive OR of the following
 *                                    object mask:
 *                                NS_OTHER_EXECUTE_BIT   (Not in DOS Name Space)
 *                                NS_OTHER_WRITE_BIT     (Not in DOS Name Space)
 *                                NS_OTHER_READ_BIT      (Not in DOS Name Space)
 *                                NS_GROUP_EXECUTE_BIT   (Not in DOS Name Space)
 *                                NS_GROUP_WRITE_BIT     (Not in DOS Name Space)
 *                                NS_GROUP_READ_BIT      (Not in DOS Name Space)
 *                                NS_OWNER_EXECUTE_BIT
 *                                NS_OWNER_WRITE_BIT
 *                                NS_OWNER_READ_BIT
 *                                NS_STICK_BIT           (Not in DOS Name Space)
 *                                NS_SET_GID_BIT         (Not in DOS Name Space)
 *                                NS_SET_UID_BIT         (Not in DOS Name Space)
 *                                NS_FILE_EXECUTABLE_BIT (Not in DOS Name Space)
 *                                NS_MANDATORY_LOCK_BIT  (Not in DOS Name Space)
 *                                NS_HIDDEN_FILE_BIT
 *    nameSpaceInfo->nodeMajorNumber - Set to the Block or Character Device 
 *                                     major number. (Set only when nodeType
 *                                     is NS_CHARACTER_SPECIAL or 
 *                                     NS_BLOCK_SPECIAL).
 *    nameSpaceInfo->nodeMinorNumber - Set to the Block or Character Device 
 *                                     minor number. (Set only when nodeType
 *                                     is NS_CHARACTER_SPECIAL or 
 *                                     NS_BLOCK_SPECIAL).
 *
 *    exclusive                 - If true the file must be created exclusively.
 *    accessFlags               - Flags used to open the newly created file
 *                                with. Set to an inclusive 'OR' of the
 *                                following:
 *                                NW_READ               - Open for reading.
 *                                NW_WRITE              - Open for writing.
 *                                NW_EXCLUSIVE          - Open for exclusively.
 *                                NW_INHERIT_PARENT_GID - Set GID to the parent
 *                                                        GID;
 *                                NW_CREATE             - Create new object.
 * OUTPUT
 *    newFileNode               - Server node object representing the newly
 *                                created file.
 *    diagnostic                - Set to one of the following if an error
 *                                occurs: 
 *                                NUCFS_ALLOCATE_NODE_FAILED
 *                                NUCFS_ALLOC_MEM_FAILED
 *                                NUCFS_DUP_CRED_FAILED
 *                                NUCFS_LLIST_OPERATION_FAILED
 *
 * RETURN VALUES
 *    0                         - Successful completion.
 *    -1                        - Unsuccessful completion, diagnostic contains
 *                                reason.
 *
 * DESCRIPTION
 *    The NWfsCreateFileNode allocates and populates a server node object to
 *    represent the newly created file.  It also creates a NetWare SPIL file 
 *    node object in the SPIL resource handle associated with the specified
 *    parent directory.
 *
 * NOTES
 *    NWfsCreateFileNode allocates a server node object and creates a SPIL
 *    NetWare file node to represent the new file.  The NWfsOpenNode is called
 *    whenever the new SPIL NetWare file node object is to be opened.  The open
 *    (access) flags to open the new SPIL NetWare file node are passed to
 *    NWfsOpenNode.  So the NWfsCreateFileNode is called when the file is
 *    created for the first time, and the NWfsOpenNode is called with the
 *    appropriate open flags every time the file is referenced.
 *
 *    There is a main difference with creating files vs directories.  A file is
 *    really created in two steps, first it is created (NWfsCreateFileNode) and
 *    then it is opened (NWfsOpenNode).  However NWfsOpenNode is not called
 *    when a file is created initially.
 *
 * SEE ALSO
 *     NWfsAllocateNode(3k), NWsiCreateNode(3k), and NWfsOpenNode(3k).
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWfsCreateFileNode (
	NWFS_CRED_T		*credentials,
	NWFS_SERVER_NODE_T	*parentNode,
	char			*fileName,
	NWSI_NAME_SPACE_T	*nameSpaceInfo,
	uint32			accessFlags,
	NWFS_SERVER_NODE_T	**newFileNode,
	enum	NUC_DIAG	*diagnostic)
{
	int			handleTries = 0;
	NWFS_CLIENT_HANDLE_T	*parentClientHandle, *childClientHandle;
	NWFS_NAME_T		*hashName;
	ccode_t			returnCode = FAILURE;
	int			chmodAtClose = FALSE;
	int			needToClose = FALSE;
	NWFS_CACHE_INFO_T	cacheInfo;
	NWSI_NAME_SPACE_T	*localNameSpaceInfo;
	NWFS_SERVER_NODE_T	*childNode;
	int			newlyCreated;
	nwcred_t		nucCredentials;
	SPI_HANDLE_T		*newResourceHandle;
	enum NUC_DIAG		diag;

	NVLT_ENTER (7);

	NVLT_STRING (fileName);
	NVLT_ASSERT (SNODE_REASONABLE(parentNode));
	NVLT_ASSERT (parentNode->nodeType == NS_DIRECTORY ||
		parentNode->nodeType == NS_ROOT);
	NVLT_ASSERT (nameSpaceInfo->nodeType != NS_DIRECTORY &&
		nameSpaceInfo->nodeType != NS_ROOT);

	*newFileNode = NULL;

         /*
          * Hash in the name. The hashed name is returned held.
          */
         hashName = NWfsLookupOrEnterName(fileName);

         if (hashName == NWfsDot || hashName == NWfsDotDot) {
		/*
		 * Can not create a file with '.' or '..' name.
		 */
		*diagnostic = NUCFS_NODE_ALREADY_EXISTS;
		goto done;
	}

	/*
          * Find or allocate a clientHandle structure corresponding
          * to the credentials. The clientHandle is returned held.
          */
         parentClientHandle = NWfsGetClientHandle(parentNode, credentials);

	/*
	 * Note:
	 *	We would like to peek in the name cache. However, in order
	 *	to achieve remove-to-create consistency, we need to go over
	 *	the wire.
	 */

         /*
          * Convert NWfs credentials to NUC credentials.
          */
         NWfiFsToNucCred(credentials, &nucCredentials);

         if (parentNode->nodeVolume->volumeFlags & NUCFS_INHERIT_PARENT_GID)
                 /*
                  * Set the newFileNode GID to that of the parentNode's GID.
                  */
                 accessFlags |= NW_INHERIT_PARENT_GID;

	if (!(nameSpaceInfo->nodePermissions & NS_OWNER_WRITE_BIT)) {
		/*
		 * The owner's write permission bit is not set. Upon close
		 * we make sure a set name space is done to remove w from
		 * owner's permission.
		 */
		chmodAtClose = TRUE;
	}

	localNameSpaceInfo = kmem_alloc(sizeof(NWSI_NAME_SPACE_T), KM_SLEEP);

         for (;;) {
                 /*
                  * Get a NUC resource handle for the parent node.
                  */
		if (NWfsOpenResourceHandle(parentClientHandle, 0, TRUE,
			FALSE, &newlyCreated, diagnostic) != SUCCESS) {
				goto fail_common;
		}

		/*
		 * nameSpaceInfo is a bi-directional parameter to
		 * NWsiCreateNode which can be trashed on an error return.
		 * So therefore, make a copy of it so that we can retry.
		 */
		*localNameSpaceInfo = *nameSpaceInfo;

		cacheInfo.stale = FALSE;
		NWFI_GET_TIME(&cacheInfo.beforeTime);
		if (NWsiCreateNode (&nucCredentials,
				parentNode->nodeVolume->spilVolumeHandle,
				parentClientHandle->resourceHandle, fileName,
				accessFlags, &newResourceHandle,
				localNameSpaceInfo, diagnostic) == SUCCESS) {
			NWFI_GET_TIME(&cacheInfo.afterTime);
			NWfsReleaseResourceHandle(parentClientHandle);
			break;
		}

		switch (*diagnostic) {
		/*
		 * The server is down. So therefore, get rid of our
		 * resource handle and fail the operation.
		 */
		case SPI_SERVER_UNAVAILABLE:
		case SPI_SERVER_DOWN:
			NWfsCloseResourceHandle(parentClientHandle);
			goto fail_common;

		/*
		 * The server is up, but the user has lost authentication.
		 */
		case SPI_AUTHENTICATION_FAILURE:
		case SPI_BAD_CONNECTION:
			NWfsCloseResourceHandle(parentClientHandle);
			if (++handleTries >= NUCFS_RETRY_LIMIT)
				goto fail_common;
			break;
		/*
		 * We received some file or file system specific error.
		 * Return this to the caller.
		 */
		default:
			NWfsReleaseResourceHandle(parentClientHandle);
			goto fail_common;
		}
	}

	/*
	 * Parent access/mod times changed.
	 */
	SNODE_SET_MODIFIED(parentNode);

	/*
	 * The netware server supports file sizes upto 2**32 - 1. The local
	 * client's OS might not. If this file is too big to be supported by
	 * the local OS, then we fail the create (which is really an open).
	 */
	if ((NWFI_OFF_T) localNameSpaceInfo->nodeSize < 0) {
		needToClose = TRUE;
		goto fail_common;
	}

         /*
          * Get (possibly allocate) a child node. The child node is returned
          * active and held.
          */
	if (NWfsCreateOrUpdateNode(parentClientHandle, hashName,
			&cacheInfo, localNameSpaceInfo, newResourceHandle,
			accessFlags, &childNode, diagnostic) != SUCCESS) {
		needToClose = TRUE;
		goto fail_common;
	}

	/*
	 * Store away the ``chmod on close'' indicator in the child's
	 * client handle.
	 */
	if (chmodAtClose) {
		childClientHandle = NWfsGetClientHandle(childNode, credentials);
		SNODE_LOCK(childNode);
		childClientHandle->clientHandleFlags |= NWCH_RH_CMOC;
		SNODE_UNLOCK(childNode);
		NWfsReleaseClientHandle(childClientHandle);
	}

	*newFileNode = childNode;
	returnCode = SUCCESS;
fail_common:
	if (needToClose) {
		NWsiCloseNode(&nucCredentials,
			parentNode->nodeVolume->spilVolumeHandle,
			newResourceHandle, nameSpaceInfo->nodeType, &diag);
	}
	NWfsReleaseClientHandle(parentClientHandle);
	kmem_free(localNameSpaceInfo, sizeof(NWSI_NAME_SPACE_T));
done:
	NWFS_NAME_RELEASE(hashName);
	return (NVLT_LEAVE (returnCode));
}


#if defined(DEBUG) || defined(DEBUG_TRACE)
int NWfsFileLockTransparent;	/* Send locks transparently to server. */
#endif

ccode_t
NWfsFileLock (
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	uint32			accessFlags,
	NUCFS_LOCK_T		*lockStruct,
	enum NUC_DIAG		*diagnostic)
{
	ccode_t			result;

	NVLT_ENTER (4);
	NVLT_ASSERT (CHANDLE_REASONABLE(clientHandle));
	NVLT_ASSERT (lockStruct->lockCommand == NWFS_SET_LOCK
			|| lockStruct->lockCommand == NWFS_SET_WAIT_LOCK
			|| lockStruct->lockCommand == NWFS_REMOVE_LOCK);

#if defined(DEBUG) || defined(DEBUG_TRACE)
	if (NWfsFileLockTransparent) {
		result = NWfsDoFileLock(clientHandle, lockStruct, diagnostic);
		return NVLT_LEAVE(result);
	}
#endif
	if (lockStruct->lockCommand == NWFS_REMOVE_LOCK)
		result = NWfsRemoveFileLock(clientHandle, accessFlags,
					    lockStruct, diagnostic);
	else
		result = NWfsSetFileLock(clientHandle, accessFlags,
					 lockStruct, diagnostic);
	return NVLT_LEAVE(result);
}


/*
 * The process named in *lock is giving up all of its lock references in this
 * chandle.
 */
void
NWfsCleanFileLocks(
	NWFS_CLIENT_HANDLE_T	*chandle,
	uint32			accessFlags,
	NUCFS_LOCK_T		*lock)
{
	NWFS_SERVER_NODE_T	*snode;
	NWFI_LIST_T		intersection, *chain;
	NUCFS_FLOCK_CACHE_T	*cache;
	ccode_t			result;
	int			newlyCreated;
	enum NUC_DIAG		diag;
	boolean_t		handleOpened = B_FALSE;

	NVLT_ENTER(3);
	NVLT_ASSERT(CHANDLE_REASONABLE(chandle));
	snode = chandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(snode));
	NVLT_ASSERT(NWFI_LIST_EMPTY(&chandle->flockCacheChain) ==
		    !FLOCK_CACHE_LEN(chandle));

	if (NWfsFileLockOpenHandle(chandle, accessFlags, TRUE, &newlyCreated,
				   &diag) == SUCCESS) {
		handleOpened = B_TRUE;
		result = NWfsRemoveFileLock(chandle, accessFlags, lock, &diag);
	} else
		NWfsFlockStale(snode);
	NWFI_LIST_INIT(&intersection);
	NWfsFlockCacheExtractRange(&chandle->flockCacheChain, lock, B_TRUE,
				   NWFS_NO_LOCK, &intersection);
	SNODE_LOCK(snode);
	chain = NWFI_NEXT_ELEMENT(&intersection);
	while (chain != &intersection) {

		/*
		 * Note that we can't assert that success means that
		 * intersection will be empty, because some locks may
		 * be shared, but in non-error cases, we should have no
		 * remaining references here.
		 */
		cache = CHAIN_TO_CACHE(chain);
		chain = NWFI_NEXT_ELEMENT(chain);
		if (NWfsFlockPidFind(cache, lock->lockPid))
			NWfsChandleCacheRelease_l(chandle, cache,
						  lock->lockPid);
	}
	SNODE_UNLOCK(snode);
	if (!NWFI_LIST_EMPTY(&intersection))
		NWfsFlockCacheSplice(&chandle->flockCacheChain, &intersection);
	if (handleOpened)
		NWfsFileLockReleaseHandle(chandle, result, diag);
	NVLT_ASSERT(NWFI_LIST_EMPTY(&chandle->flockCacheChain) ==
		    !FLOCK_CACHE_LEN(chandle));
	NVLT_VLEAVE();
}


/*
 * Special-purpose code to allow UNIX-style writes through shared locks, which
 * the NetWare server will not directly allow.  Writers of locked (remotely
 * cached) files must call here before attempting a write.  A shared lock in
 * the write range will be upgraded to an exclusive lock.  *lockStruct is a
 * lock describing the range to be written.
 * (I) In error cases, all out parameters except diagnostic are undefined.
 * (II) If there is no error
 *	(A) If no upgrade is needed *upgraded is set to B_FALSE, and all other
 *	    out parameters are undefined.
 *	(B) Otherwise, an upgrade is needed.
 *		(1) *upgraded is set to B_TRUE.
 *		(2) If there is a shared lock that must be restored via a call
 *		    to NWfsRestoreFileLock after attempting to write, then
 *		    *savedCache is updated to point to the cache structure.
 *		(3) Otherwise, *savedCache is NULL.
 */
ccode_t
NWfsUpgradeFileLock(
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	uint32			accessFlags,
	NUCFS_LOCK_T		*lockStruct,
	boolean_t		*upgraded,
	NUCFS_FLOCK_CACHE_T	**savedCache,
	enum NUC_DIAG		*diagnostic)
{
	NWFS_SERVER_NODE_T	*snode;
	NWFI_LIST_T		intersectingRequests;
	NWFI_LIST_T		*chain;
	NUCFS_FLOCK_CACHE_T	*cache;
	int			newlyCreated;
	ccode_t			result = SUCCESS;

	NVLT_ENTER (6);
	NVLT_ASSERT (CHANDLE_REASONABLE(clientHandle));
	snode = clientHandle->snode;
	NVLT_ASSERT (SNODE_REASONABLE(snode));
	NVLT_ASSERT (lockStruct->lockCommand == NWFS_SET_LOCK
			|| lockStruct->lockCommand == NWFS_SET_WAIT_LOCK);
	NVLT_ASSERT (lockStruct->lockType == NWFS_EXCLUSIVE_LOCK);
	NVLT_ASSERT(NWFI_LIST_EMPTY(&clientHandle->flockCacheChain) ==
		    !FLOCK_CACHE_LEN(clientHandle));

	/*
	 * We need to upgrade if there are any shared locks in the range of the
	 * exclusive lock.  Furthermore, if there is a shared lock whose
	 * dimensions exactly match the exclusive lock, the exclusive lock will
	 * overwrite it on the server.  Therefore we save that lock to restore
	 * later.  Note that we pass B_FALSE for matchPid to
	 * NWfsFlockCacheExtractRange, because we are interested in locks held
	 * by any process using this chandle.
	 */
	*upgraded = B_FALSE;
	*savedCache = NULL;
	NWFI_LIST_INIT(&intersectingRequests);
	NWfsFlockCacheExtractRange(&clientHandle->flockCacheChain, lockStruct,
				   B_FALSE, NWFS_SHARED_LOCK,
				   &intersectingRequests);
	if (NWFI_LIST_EMPTY(&intersectingRequests))
		goto done;
	*upgraded = B_TRUE;
	for (chain = NWFI_NEXT_ELEMENT(&intersectingRequests);
			chain != &intersectingRequests;
			chain = NWFI_NEXT_ELEMENT(chain)) {
		cache = CHAIN_TO_CACHE(chain);
		if (cache->cacheOffset == lockStruct->lockOffset &&
		    cache->cacheEnd == lockStruct->lockEnd) {
			*savedCache = cache;
			break;
		}
	}

	/*
	 * Restore the intersecting range, then try to place the exclusive lock
	 * on the server if needed.  We avoid calling NWfsFileLock, and instead
	 * call the lower level interface, NWfsDoFileLock, because we want to
	 * manipulate the flock cache to simplify the interface here and in
	 * NWfsRestoreFileLock.  (We leave the saved lock in the cache, , then
	 * cache the exclusive lock.  This redundancy introduces a transient
	 * inconsistency in the cache, but the caller serializes access via the
	 * snode RW lock, so the inconsistency is invisible externally)
	 */
	NWfsFlockCacheSplice(&clientHandle->flockCacheChain,
			     &intersectingRequests);
	if (!*upgraded)
		goto done;
	if ((result = NWfsFileLockOpenHandle(clientHandle, accessFlags,
			TRUE, &newlyCreated, diagnostic)) != SUCCESS) {
		NWfsFlockStale(snode);
		goto done;
	}
	result = NWfsDoFileLock(clientHandle, lockStruct, diagnostic);
	if (result == SUCCESS)
		NWfsChandleFlockHold(clientHandle, lockStruct);
	NWfsFileLockReleaseHandle(clientHandle, result, *diagnostic);
done:
	NVLT_ASSERT(NWFI_LIST_EMPTY(&clientHandle->flockCacheChain) ==
		    !FLOCK_CACHE_LEN(clientHandle));
	return (NVLT_LEAVE(result));
}


/*
 * Special-purpose code to allow UNIX-style writes through shared locks, which
 * the NetWare server will not directly allow.  Writers of locked files are
 * expected to call here after a write attempt following a successful call to
 * NWfsUpgradeFileLock.  lockStruct is a lock placed by the call to
 * NWfsUpgradeFileLock.  savedCache, if non-NULL, points to a lock that must be
 * restored after the write attempt, as returned by the preceding call to
 * NWfsUpgradeFileLock.  In failure cases, the caller is responsible for
 * assuring that NWfi-layer lock state reflects the existence of lockStruct.
 */
ccode_t
NWfsRestoreFileLock (
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	uint32			accessFlags,
	NUCFS_LOCK_T		*lockStruct,
	NUCFS_FLOCK_CACHE_T	*savedCache,
	enum NUC_DIAG		*diagnostic)
{
	NWFS_SERVER_NODE_T	*snode;
	NUCFS_FLOCK_CACHE_T	*cache;
	ccode_t			result;
	uint16			savedCommand;
	int			newlyCreated;

	NVLT_ENTER (5);
	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
	snode = clientHandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(snode));
	NVLT_ASSERT(lockStruct->lockCommand == NWFS_SET_LOCK ||
		    lockStruct->lockCommand == NWFS_SET_WAIT_LOCK);
	NVLT_ASSERT(lockStruct->lockType == NWFS_EXCLUSIVE_LOCK);
	NVLT_ASSERT(NWFI_LIST_EMPTY(&clientHandle->flockCacheChain) ==
		    !FLOCK_CACHE_LEN(clientHandle));

	if ((result = NWfsFileLockOpenHandle(clientHandle, accessFlags,
			TRUE, &newlyCreated, diagnostic)) != SUCCESS) {
		NWfsFlockStale(snode);
		goto done;
	}
	if (savedCache) {
		NVLT_ASSERT(savedCache->cacheCommand == NWFS_SET_LOCK ||
			    savedCache->cacheCommand == NWFS_SET_WAIT_LOCK);
		NVLT_ASSERT(savedCache->cacheType == NWFS_SHARED_LOCK);
		NVLT_ASSERT(savedCache->cacheOffset == lockStruct->lockOffset &&
			    savedCache->cacheEnd == lockStruct->lockEnd);

		savedCommand = savedCache->cacheCommand;
		savedCache->cacheCommand = NWFS_SET_LOCK;
		result = NWfsDoFileLock(clientHandle, &savedCache->cacheState, 
					diagnostic);
		savedCache->cacheCommand = savedCommand;
		if (result == SUCCESS) {

			/*
			 * We overwrote the exclusive lock on the server.
			 * Reflect this in our cache.
			 */
			cache = NWfsFlockCacheFind(
						&clientHandle->flockCacheChain,
						lockStruct,
						B_TRUE, lockStruct->lockType);
			NVLT_ASSERT(cache);
			NWfsChandleCacheRelease(clientHandle, cache,
						lockStruct->lockPid);
		} else {
			cmn_err(CE_NOTE, "NWfsRestoreFileLock:  cannot restore"
				" saved lock, diag %d", *diagnostic);

			/*
			 * We should never block unlocking.
			 */
			if (NWFS_LOCK_BLOCKED(*diagnostic)) {
				cmn_err(CE_NOTE, "NWfsRestoreFileLock: "
				"blocked unlocking, diag %d", *diagnostic);
				*diagnostic = NUCFS_PROTOCOL_ERROR;
			}

			/*
			 * NWfsUpgradeFileLock did not remove savedCache from
			 * flockCacheChain.
			 */
			NWfsChandleCacheRelease(clientHandle, savedCache,
						lockStruct->lockPid);
		}
		goto releaseHandle;
	}

	/*
	 * There was no saved shared lock.  Drop the exclusive lock.
	 */
	savedCommand = lockStruct->lockCommand;
	lockStruct->lockCommand = NWFS_REMOVE_LOCK;
	result = NWfsDoFileLock(clientHandle, lockStruct, diagnostic);
	lockStruct->lockCommand = savedCommand;
	if (result == SUCCESS) {
		cache = NWfsFlockCacheFind(&clientHandle->flockCacheChain,
					   lockStruct, B_TRUE,
					   lockStruct->lockType);
		NVLT_ASSERT(cache);
		NWfsChandleCacheRelease(clientHandle, cache,
					lockStruct->lockPid);
	} else {
		cmn_err(CE_NOTE, "NWfsRestoreFileLock: cannot remove"
			" exclusive lock");

		/*
		 * We should never block downgrading.
		 */
		if (NWFS_LOCK_BLOCKED(*diagnostic)) {
			cmn_err(CE_NOTE, "NWfsRestoreFileLock: "
			"blocked downgrading, diag %d", *diagnostic);
			*diagnostic = NUCFS_PROTOCOL_ERROR;
		}
	}
releaseHandle:
	NWfsFileLockReleaseHandle(clientHandle, result, *diagnostic);
done:
	NVLT_ASSERT(NWFI_LIST_EMPTY(&clientHandle->flockCacheChain) ==
		    !FLOCK_CACHE_LEN(clientHandle));
	return NVLT_LEAVE(result);
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
 * BEGIN_MANUAL_ENTRY( NWfsLinkFile(3k), \
 *                     ./man/kernel/nucfs/nwfs/LinkFile )
 * NAME
 *    NWfsLinkFile - Links two server file nodes together. 
 *
 * SYNOPSIS
 *    ccode_t
 *    NWfsLinkFile (credentials, existingFile, newLinkParentNode, 
 *                  newLinkFileName, diagnostic) 
 *    opaque_t       *credentials;
 *    opaque_t       *existingFile;         object is opaque to the caller.
 *    opaque_t       *newLinkParentNode;    object is opaque to the caller.
 *    char           *newLinkFileName;
 *    enum  NUC_DIAG *diagnostic;
 *
 * INPUT
 *    credentials       - Credentials of the UNIX client linking the two server
 *                        files.
 *    existingFile      - Server node object representing the existing file to
 *                        link to.
 *    newLinkParentNode - Server node object representing the directory that
 *                        newLinkFileNode is on.
 *    newLinkFileName   - New file name to be linked to the existingFile.
 *
 * OUTPUT
 *    diagnostic        - Set to one of the following if an error occurs:
 *
 * RETURN VALUES
 *    0                 - Successful completion.
 *    -1                - Unsuccessful completion, diagnostic contains reason.
 *
 * DESCRIPTION
 *    The NWfsLinkFile links two file names together by incrementing the
 *    existingFile's number of links and pointing the newLinkFileName in the
 *    newLinkParentNode to the existingFile.  The newLinkFileName object
 *    identifier is the same as the object identifier of the existingFile.
 *
 * NOTES
 *    Only file nodes are linked together in NetWare UNIX Client File System.
 *
 *    Directories are not allowed to be linked due to NUC File System
 *    architecture (The s5 and ufs file system allow this with strange results).
 *
 * SEE ALSO
 *    NWsiLinkFile(3k).
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWfsLinkFile (
	NWFS_CRED_T		*credentials,
	NWFS_SERVER_NODE_T	*existingFile,
	NWFS_SERVER_NODE_T	*newLinkParentNode,
	char			*newLinkFileName,
	enum	NUC_DIAG	*diagnostic)
{
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	NWFS_NAME_T		*hashName;
	ccode_t			returnCode = FAILURE;
	nwcred_t		nucCredentials;
	int			newlyCreated;
	int			handleTries = 0;
	int			delayRetries = 0;

	NVLT_ENTER (5);
	NVLT_STRING (newLinkFileName);

	NVLT_ASSERT (SNODE_REASONABLE(existingFile));
	NVLT_ASSERT (existingFile->nodeType != NS_SYMBOLIC_LINK);
	NVLT_ASSERT (SNODE_REASONABLE(newLinkParentNode));
	NVLT_ASSERT ((newLinkParentNode->nodeType == NS_DIRECTORY) ||
		(newLinkParentNode->nodeType == NS_ROOT));

	/*
	 * Make sure the existingFile is not a directory.
	 */
	if (existingFile->nodeType == NS_DIRECTORY ||
			existingFile->nodeType == NS_ROOT) {
		*diagnostic = NUCFS_NODE_IS_DIRECTORY;
		goto done;
	}
	
	/*
	 * Make sure that the files to be linked are on the same server volume.
	 */
	if (existingFile->nodeVolume != newLinkParentNode->nodeVolume) {
		*diagnostic = NUCFS_NOT_SAME_VOLUME;
		goto done;
	}

         /*
          * Hash in the name. The hashed name is returned held.
	 * Check for ``.'' and ``..'' (which the server knows nothing about).
          */
         hashName = NWfsLookupOrEnterName(newLinkFileName);
         if (hashName == NWfsDot || hashName == NWfsDotDot) {
                 *diagnostic = NUCFS_NODE_IS_DIRECTORY;
                 goto done1;
         }

	/*
	 * Find or allocate a clientHandle structure corresponding
	 * to the credentials. The clientHandle is returned held.
	 */
	clientHandle = NWfsGetClientHandle(newLinkParentNode, credentials);

	/*
	 * Cannot generate a hard link to an open file. So close all of its
	 * resource handles.
	 */
	if (NWfsCloseAllHandles(existingFile, diagnostic) != SUCCESS)
		goto done1;

	/*
	 * Convert NWfs credentials to NUC credentials.
	 */
	NWfiFsToNucCred(credentials, &nucCredentials);

        /*
         * Convert NWfs credentials to NUC credentials.
         */
        NWfiFsToNucCred(credentials, &nucCredentials);

         for (;;) {
                 /*
                  * Get a NUC resource handle for the parent node.
                  */
		if (NWfsOpenResourceHandle(clientHandle, 0, TRUE,
				FALSE, &newlyCreated, diagnostic) !=
				SUCCESS) {
			goto done2;
		}

		if (NWsiLinkFile (&nucCredentials,
				newLinkParentNode->nodeVolume->spilVolumeHandle,
				NULL, NULL, existingFile->nodeNumber,
				clientHandle->resourceHandle,
				newLinkFileName, diagnostic) == SUCCESS) {
			NWfsReleaseResourceHandle(clientHandle);
			NWfsCacheName(clientHandle, hashName, existingFile);
			break;
		}

		switch (*diagnostic) {
		/*
		 * The server is down. So therefore, get rid of our
		 * resource handle and fail the operation.
		 */
		case SPI_SERVER_UNAVAILABLE:
		case SPI_SERVER_DOWN:
			NWfsCloseResourceHandle(clientHandle);
			if(SNODE_HAS_FLOCK(existingFile))
				NWfsFlockStale(existingFile);
			goto done2;

                /*
                 * The file is open on the NetWare server.
                 * So delay one second and retry.
                 */
                case SPI_FILE_IN_USE:
			NWfsReleaseResourceHandle(clientHandle);
                        if (++delayRetries <= NUCFS_DELAY_TIME) {
                                NWfiDelay(HZ);
                                break;
                        }
                        goto done2;

		/*
		 * The server is up, but the user has lost authentication.
		 */
		case SPI_AUTHENTICATION_FAILURE:
		case SPI_BAD_CONNECTION:
			NWfsCloseResourceHandle(clientHandle);
			if(SNODE_HAS_FLOCK(existingFile)) {
				NWfsFlockStale(existingFile);
				goto done2;
			}
			if (++handleTries >= NUCFS_RETRY_LIMIT)
				goto done2;
			break;

                /*
                 * We received some file or file system specific error.
                 * Return this to the caller.
                 */
                default:
                        NWfsReleaseResourceHandle(clientHandle);
                        goto done2;
		}
	}

        /*
         * Parent access/mod times changed.
	 * Child link count has changed - and its create time should have
	 * changed as well.
	 *
	 * Since we don't possess updated name space info for the new parent,
	 * nor for the child, we simply mark the attributes as stale.
         */
	SNODE_SET_MODIFIED(newLinkParentNode);
	SNODE_SET_MODIFIED(existingFile);

	returnCode = SUCCESS;

	/*
	 * Release resources
	 */
done2:
	NWfsAllowResourceHandlesToOpen(existingFile);
	NWfsReleaseClientHandle(clientHandle);
done1:
	NWFS_NAME_RELEASE(hashName);
done:
	return (NVLT_LEAVE (returnCode));
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
 * BEGIN_MANUAL_ENTRY( NWfsLookUpNode(3k), \
 *                     ./man/kernel/nucfs/nwfs/LookUpNode )
 * NAME
 *    NWfsLookUpNode - Searches the server volume's active or the cache lists,
 *                     the specified parentNode directory object is on, looking
 *                     for a server node with the specified nodeName and 
 *                     parentNode.
 *
 * SYNOPSIS
 *    ccode_t
 *    NWfsLookUpNode (credentials, parentNode, nodeName, foundNode, diagnostic)
 *    NWFS_CRED_T		*credentials;
 *    NWFS_SERVER_NODE_T	*parentNode;
 *    char			*nodeName;
 *    NWFS_SERVER_NODE_T	**foundNode;
 *    enum NUC_DIAG		*diagnostic;
 *
 * INPUT
 *    credentials - The credentials of the UNIX client looking for the server
 *                  node associated with the specified nodeName in the specified
 *                  parentNode.
 *    parentNode  - Server node object representing the directory containing the
 *                  nodeName.
 *    nodeName    - Node name in search of.
 *
 * OUTPUT
 *    foundNode   - Found server node object associated with the specified
 *                  nodeName in the specified parentNode.  Set to NULL if the
 *                  node is not found.
 *    diagnostic  - Set to one of the following if an error occurs:
 *
 * RETURN VALUES
 *    0           - Successful completion.
 *    -1          - Unsuccessful completion, diagnostic contains reason.
 *
 * DESCRIPTION
 *    The NWfsLookUpNode searches the server volume's active or cache lists, the
 *    specified parentNode is on, looking for the a server node with the
 *    specified nodeName and the specified parentNode.  If not found, foundNode
 *    is set to NULL.
 *
 * NOTE
 *    If the server node in search of if on one of the cache list buckets, it
 *    is moved to the same bucket number on the active lists.
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWfsLookUpNode (
	NWFS_CRED_T		*credentials,
	NWFS_SERVER_NODE_T	*parentNode,
	char			*nodeName,
	NWFS_SERVER_NODE_T	**foundNode,
	enum	NUC_DIAG	*diagnostic)
{
	int32			handleTries = 0;
	NWFS_NAME_T		*hashName;
	NWFS_CLIENT_HANDLE_T	*clientHandle, *childClientHandle;
	NWFS_SERVER_NODE_T	*childNode;
	nwcred_t		nucCredentials;
	NWSI_NAME_SPACE_T	*nameSpaceInfo;
	NWFS_CACHE_INFO_T	cacheInfo;
	NWFS_SERVER_VOLUME_T	*volp;
	int			newlyCreated;

	NVLT_ENTER (5);
	NVLT_STRING (nodeName);

	NVLT_ASSERT (SNODE_REASONABLE(parentNode));
	NVLT_ASSERT (parentNode->nodeType == NS_DIRECTORY ||
		parentNode->nodeType == NS_ROOT);
	NVLT_ASSERT (nodeName != NULL);

	/*
	 * Get volume pointer.
	 */
	volp = parentNode->nodeVolume;

	/*
	 * Hash in the name. The hashed name is returned held.
	 */
	hashName = NWfsLookupOrEnterName(nodeName);

	if (hashName == NWfsDot ||
	    (hashName == NWfsDotDot && parentNode->nodeType == NS_ROOT)) {
		/*
		 * Self referencing parentNode.
		 */
		SNODE_HOLD(parentNode);
		childNode = parentNode;
		goto done;
	}

	/*
	 * Find or allocate a clientHandle structure corresponding
	 * to the credentials. The clientHandle is returned held.
	 */
	clientHandle = NWfsGetClientHandle(parentNode, credentials);

	/*
	 * Search for the name in the client handle. If it is found,
	 * NWfsSearchByName will return the childNode soft held.
	 */
	if ((childNode = NWfsSearchByName(clientHandle, hashName)) != NULL) {
		NUCFS_LIST_LOCK();
		switch (childNode->nodeState) {
		case SNODE_INACTIVE:
			/*
			 * Move to active list.
			 */
			NWfiRemque(snodeToVolumeChain(childNode));
			NWfiInsque(snodeToVolumeChain(childNode),
				   &volp->activeNodeList);
			childNode->nodeState = SNODE_ACTIVE; 
			--volp->cacheCount;
			--nwfsCacheNodeCount;
			++volp->activeOrTimedCount;
			++nwfsActiveNodeCount;
			/* FALLTHROUGH */
		case SNODE_ACTIVE:
		case SNODE_TIMED:
			SNODE_LOCK(childNode);
			--childNode->softHoldCount;
			++childNode->hardHoldCount;
			SNODE_UNLOCK(childNode);
			NUCFS_LIST_UNLOCK();
			goto done1;
		default:
			NVLT_ASSERT(childNode->nodeState == SNODE_STALE);
			NUCFS_LIST_UNLOCK();
			SNODE_SOFT_RELEASE(childNode);
			break;
		}
	}

	/*
	 * We didn't find a cached name. So therefore, we need to
	 * go over the wire to the NetWare server to find it.
	 */

	/*
	 * Convert NWfs credentials to NUC credentials.
	 */
	NWfiFsToNucCred(&clientHandle->credentials, &nucCredentials);

	nameSpaceInfo = kmem_alloc(sizeof(NWSI_NAME_SPACE_T), KM_SLEEP);

	NWFI_GET_TIME(&cacheInfo.beforeTime);
	for (;;) {
		if (hashName == NWfsDotDot) {
			/*
			 * Since NetWare servers don't maintain "..", not
			 * even when the UNIX name space is loaded, we need a
			 * special interface to get information about our
			 * parent.
			 */
			nameSpaceInfo->nodeNumber = parentNode->nodeNumber;
			nameSpaceInfo->nodeType = parentNode->nodeType;
			nameSpaceInfo->openFileHandle = NULL;
			if (NWsiGetParentNameSpaceInfo(&nucCredentials,
				    volp->spilVolumeHandle,
				    nameSpaceInfo, diagnostic) == SUCCESS) {
				break;
			}

			switch (*diagnostic) {
			/*
			 * The server is up, but the user has lost either
			 * lost the connection or authentication. Attempt
			 * to re-establish.
			 */
			case SPI_AUTHENTICATION_FAILURE:
			case SPI_BAD_CONNECTION:
				if (++handleTries >= NUCFS_RETRY_LIMIT)
					goto fail_common;
				break;

			/*
			 * The server is down, or going down, or we got
			 * some file, file system, or server specific error.
			 * Fail the operation now.
			 */
			default:
				goto fail_common;
			}

			/*
			 * Okay, (re)authenticate the user.
			 */
			if (NWsiAutoAuthenticate(&nucCredentials,
				    parentNode->nodeVolume->spilVolumeHandle,
				    NUC_SINGLE_LOGIN_ONLY,
				    diagnostic) != SUCCESS) {
				goto fail_common;
			}
		} else  {
			/*
			 * Get a NUC resource handle for the parent node.
			 */
			if (NWfsOpenResourceHandle(clientHandle, 0, TRUE,
					FALSE, &newlyCreated,
					diagnostic) != SUCCESS) {
				goto fail_common;
			}

			nameSpaceInfo->nodeType = NS_UNKNOWN;
			nameSpaceInfo->nodeNumber = -1;
			nameSpaceInfo->openFileHandle = NULL;
			if (NWsiGetNameSpaceInfo (&nucCredentials,
				    parentNode->nodeVolume->spilVolumeHandle,
				    clientHandle->resourceHandle, nodeName,
				    nameSpaceInfo, diagnostic)== SUCCESS) {
				NWfsReleaseResourceHandle(clientHandle);
				break;
			}

			switch (*diagnostic) {
			/*
			 * The server is down. So therefore, get rid of our
			 * resource handle and fail the operation.
			 */
			case SPI_SERVER_UNAVAILABLE:
			case SPI_SERVER_DOWN:
				NWfsCloseResourceHandle(clientHandle);
				goto fail_common;

			/*
			 * The server is up, but the user has lost either
			 * lost the connection or authentication. Attempt
			 * to re-establish.
			 */
			case SPI_AUTHENTICATION_FAILURE:
			case SPI_BAD_CONNECTION:
				NWfsCloseResourceHandle(clientHandle);
				if (++handleTries >= NUCFS_RETRY_LIMIT)
					goto fail_common;
				break;

			/*
			 * We received some server or file system specific
			 * error. Return this to the caller.
			 */
			default:
				NWfsReleaseResourceHandle(clientHandle);
				goto fail_common;
			}
		}
	}
	NWFI_GET_TIME(&cacheInfo.afterTime);
	cacheInfo.stale = TRUE;

	/*
	 * Get (possibly allocate) a child node. The child node is returned
	 * active and held. In addition, the nameSpaceInfo information is
	 * cached in the child node, plus the associated clientHandle.
	 */
	if (NWfsCreateOrUpdateNode(clientHandle, hashName,
			&cacheInfo, nameSpaceInfo, NULL, 0, &childNode,
			diagnostic) != SUCCESS)
		goto fail_common;
	
	/*
	 * If the child is a directory, then cache ".." for it.
	 */
	if (childNode->nodeType == NS_DIRECTORY && hashName != NWfsDotDot) {
		childClientHandle = NWfsGetClientHandle(childNode, credentials);
		NWfsCacheName(childClientHandle, NWfsDotDot, parentNode);
		NWfsReleaseClientHandle(childClientHandle);
	}

	kmem_free(nameSpaceInfo, sizeof(NWSI_NAME_SPACE_T));
done1:
	NWfsReleaseClientHandle(clientHandle);
done:
	NWFS_NAME_RELEASE(hashName);
	*foundNode = childNode;
	return (NVLT_LEAVE (SUCCESS));

fail_common:
	kmem_free(nameSpaceInfo, sizeof(NWSI_NAME_SPACE_T));
	NWfsReleaseClientHandle(clientHandle);
	NWFS_NAME_RELEASE(hashName);
	*foundNode = NULL;

	return (NVLT_LEAVE (FAILURE));
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
 * BEGIN_MANUAL_ENTRY( NWfsMakeDirNode(3k), \
 *                     ./man/kernel/nucfs/nwfs/MakeDirNode )
 * NAME
 *    NWfsMakeDirNode - Creates a directory in the specified server parent
 *                      directory node.
 *
 * SYNOPSIS
 *    ccode_t
 *    NWfsMakeDirNode (credentials, parentNode, dirName, accessFlags,
 *                     nameSpaceInfo, newDirNode, diagnostic)
 *    opaque_t          *credentials;
 *    opaque_t          *parentNode;      Object node is opaque to the caller.
 *    char              *dirName;   
 *    uint32            accessFlags;
 *    NWSI_NAME_SPACE_T *nameSpaceInfo;
 *    opaque_t          **newDirNode;     Object node is opaque to the caller.
 *    enum  NUC_DIAG    *diagnostic;
 *
 * INPUT
 *    credentials               - Credentials of the UNIX client making the 
 *                                directory.
 *    parentNode                - Parent server directory node object to make
 *                                the new directory in.
 *    dirName                   - Directory name to be created.
 *    accessFlags               - If the NW_INHERIT_PARENT_GID bit is set, new 
 *                                directory's GID is set to parent GID.
 *    nameSpaceInfo->nodeType   - Set to the following object type:
 *                                NS_DIRECTORY
 *    nameSpaceInfo->nodePermission - Set to an inclusive OR of the following
 *                                    object mask:
 *                                NS_OTHER_EXECUTE_BIT   (Not in DOS Name Space)
 *                                NS_OTHER_WRITE_BIT     (Not in DOS Name Space)
 *                                NS_OTHER_READ_BIT      (Not in DOS Name Space)
 *                                NS_GROUP_EXECUTE_BIT   (Not in DOS Name Space)
 *                                NS_GROUP_WRITE_BIT     (Not in DOS Name Space)
 *                                NS_GROUP_READ_BIT      (Not in DOS Name Space)
 *                                NS_OWNER_EXECUTE_BIT
 *                                NS_OWNER_WRITE_BIT
 *                                NS_OWNER_READ_BIT
 *                                NS_STICKY_BIT          (Not in DOS Name Space)
 *                                NS_HIDDEN_FILE_BIT
 *
 * OUTPUT
 *    nameSpaceInfo->nodeNumber - Set to the unique object identifier in the
 *                                server volume.  This is not set for DOS
 *                                name space.
 *    newDirNode                - Server node object associated with the newly
 *                                created directory.
 *    diagnostic                - Set to one of the following if an error
 *                                occurs: 
 *                                NUCFS_ALLOCATE_NODE_FAILED
 *
 * RETURN VALUES
 *    0                         - Successful completion.
 *    -1                        - Unsuccessful completion, diagnostic contains
 *                                reason.
 *
 * DESCRIPTION
 *    The NWfsMakeDirNode allocates and populates a server directory node object
 *    and creates a SPIL NetWare directory node to represent the new directory.
 *    A directory client handle object is also allocated which contains the SPIL
 *    resource handle associated with the newly created SPIL NetWare directory
 *    node.
 *
 * NOTES
 *    Directory creation is the only case where a directory node has its parent
 *    linked in the NetWare UNIX Client File System (nucfs).
 *
 * SEE ALSO
 *    NWfsAllocateNode(3k), NWsiCreateNode(3k), NWsiOpenNode(3k).
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWfsMakeDirNode (
	NWFS_CRED_T		*credentials,
	NWFS_SERVER_NODE_T	*parentNode,
	char			*dirName,   
	NWSI_NAME_SPACE_T	*nameSpaceInfo,
	NWFS_SERVER_NODE_T	**newDirNode,
	enum	NUC_DIAG	*diagnostic)
{
	NWFS_CLIENT_HANDLE_T	*parentClientHandle, *childClientHandle;
	uint32			accessFlags = 0;
	ccode_t			returnCode = FAILURE;
	NWFS_CACHE_INFO_T	cacheInfo;
	NWSI_NAME_SPACE_T	*localNameSpaceInfo;
	uint32			handleTries = 0;
	NWFS_NAME_T		*hashName;
	NWFS_SERVER_NODE_T	*childNode;
        nwcred_t		nucCredentials;
	int			newlyCreated;
	SPI_HANDLE_T		*newResourceHandle;

	NVLT_ENTER (6);
	NVLT_STRING (dirName);

	NVLT_ASSERT (SNODE_REASONABLE(parentNode));
	NVLT_ASSERT (parentNode->nodeType == NS_DIRECTORY ||
		parentNode->nodeType == NS_ROOT);
	NVLT_ASSERT (nameSpaceInfo->nodeType == NS_DIRECTORY);

	*newDirNode = NULL;

        /*
         * Hash in the name. The hashed name is returned held.
         */
        hashName = NWfsLookupOrEnterName(dirName);

        if (hashName == NWfsDot || hashName == NWfsDotDot) {
                /*
                 * Can not create a directory with '.' or '..' name.
                 */
                *diagnostic = NUCFS_NODE_ALREADY_EXISTS;
                goto done;
	}

	if (parentNode->nodeVolume->volumeFlags & NUCFS_INHERIT_PARENT_GID)
		/*
		 * Set the newDirNode GID to that of the parentNode's GID.
		 */
		accessFlags = NW_INHERIT_PARENT_GID;

        /*
         * Find or allocate a clientHandle structure corresponding
         * to the credentials. The clientHandle is returned held.
         */
        parentClientHandle = NWfsGetClientHandle(parentNode, credentials);

	localNameSpaceInfo = kmem_alloc(sizeof(NWSI_NAME_SPACE_T), KM_SLEEP);

        /*
         * Note:
         *      We would like to peek in the name cache. However, in order
         *      to achieve remove-to-create consistency, we need to go over
         *      the wire.
         */

        /*
         * Convert NWfs credentials to NUC credentials.
         */
        NWfiFsToNucCred(credentials, &nucCredentials);

        for (;;) {
                /*
                 * Get a NUC resource handle for the parent node.
                 */
		if (NWfsOpenResourceHandle(parentClientHandle, 0, TRUE,
			FALSE, &newlyCreated, diagnostic) != SUCCESS) {
				goto fail_common;
		}

                /*
                 * nameSpaceInfo is a bi-directional parameter to
                 * NWsiCreateNode which can be trashed on an error return.
                 * So therefore, make a copy of it so that we can retry.
                 */
                *localNameSpaceInfo = *nameSpaceInfo;

                cacheInfo.stale = FALSE;
                NWFI_GET_TIME(&cacheInfo.beforeTime);
                if (NWsiCreateNode (&nucCredentials,
                                parentNode->nodeVolume->spilVolumeHandle,
                                parentClientHandle->resourceHandle, dirName,
                                accessFlags, &newResourceHandle,
                                localNameSpaceInfo, diagnostic) == SUCCESS) {
			NVLT_ASSERT(newResourceHandle == NULL);
                        NWFI_GET_TIME(&cacheInfo.afterTime);
                        NWfsReleaseResourceHandle(parentClientHandle);
                        break;
                }

                switch (*diagnostic) {
                /*
                 * The server is down. So therefore, get rid of our
                 * resource handle and fail the operation.
                 */
                case SPI_SERVER_UNAVAILABLE:
                case SPI_SERVER_DOWN:
                        NWfsCloseResourceHandle(parentClientHandle);
                        goto fail_common;

                /*
                 * The server is up, but the user has lost authentication.
                 */
                case SPI_AUTHENTICATION_FAILURE:
                case SPI_BAD_CONNECTION:
                        NWfsCloseResourceHandle(parentClientHandle);
                        if (++handleTries >= NUCFS_RETRY_LIMIT)
                                goto fail_common;
                        break;
                /*
                 * We received some file or file system specific error.
                 * Return this to the caller.
                 */
                default:
                        NWfsReleaseResourceHandle(parentClientHandle);
                        goto fail_common;
                }
        }

        /*
         * Parent access/mod times changed.
         */
        SNODE_SET_MODIFIED(parentNode);

        /*
         * Get (possibly allocate) a child node. The child node is returned
         * active and held.
         */
        if (NWfsCreateOrUpdateNode(parentClientHandle, hashName,
                        &cacheInfo, localNameSpaceInfo, newResourceHandle,
			0, &childNode, diagnostic) != SUCCESS) {
                goto fail_common;
        }

	/*
	 * Now cache ".." for child node.
	 */
	childClientHandle = NWfsGetClientHandle(childNode, credentials);
	NWfsCacheName(childClientHandle, NWfsDotDot, parentNode);
	NWfsReleaseClientHandle(childClientHandle);

	*newDirNode = childNode;
	returnCode = SUCCESS;

fail_common:
	NWfsReleaseClientHandle(parentClientHandle);
	kmem_free(localNameSpaceInfo, sizeof(NWSI_NAME_SPACE_T));
done:
	NWFS_NAME_RELEASE(hashName);
	return (NVLT_LEAVE (returnCode));
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
 * BEGIN_MANUAL_ENTRY( NWfsManageNameSpaceInfo(3k), \
 *                     ./man/kernel/nucfs/nwfs/ManageNameSpaceInfo )
 * NAME
 *    NWfsManageNameSpaceInfo - Gets/sets the name space information associated 
 *                              with the specified serverNode.
 *
 * SYNOPSIS
 *    ccode_t
 *    NWfsManageNameSpaceInfo (credentials, serverNode, nameSpaceInfo,
 *                             operationFlag, diagnostic)
 *    opaque_t          *credentials;
 *    opaque_t	        *serverNode;          Object is opaque to the caller.
 *    NWSI_NAME_SPACE_T *nameSpaceInfo;
 *    boolean_t         operationFlag;
 *    enum  NUC_DIAG    *diagnostic;
 *
 * INPUT
 *    credentials                - Credentials of the UNIX client geting/setting
 *                                 the name space information.
 *    serverNode                 - Server node object to get/set the name space
 *                                 information of.
 *    operationFlag              - Indicates whether to set or get the name 
 *                                 space information.  Set to one of the 
 *                                 following:
 *                                 NWFS_SET_NAME_SPACE_INFO
 *                                 NWFS_GET_NAME_SPACE_INFO
 *
 * OUTPUT if get  INPUT if set.
 *    nameSpaceInfo->nodeNumber - Set to the unique object identifier in the
 *                                server volume.  This is not set for DOS
 *                                name space.
 *    nameSpaceInfo->nodeType   - Set to the following object type:
 *                                NS_FILE
 *                                NS_DIRECTORY
 *                                NS_CHARACTER_SPECIAL   (Not in DOS Name Space)
 *                                NS_BLOCK_SPECIAL       (Not in DOS Name Space)
 *                                NS_FIFO                (Not in DOS Name Space)
 *                                NS_SYMBOLIC_LINK       (Not in DOS Name Space)
 *    nameSpaceInfo->nodePermission - Set to an inclusive OR of the following
 *                                    object mask:
 *                                NS_OTHER_EXECUTE_BIT   (Not in DOS Name Space)
 *                                NS_OTHER_WRITE_BIT     (Not in DOS Name Space)
 *                                NS_OTHER_READ_BIT      (Not in DOS Name Space)
 *                                NS_GROUP_EXECUTE_BIT   (Not in DOS Name Space)
 *                                NS_GROUP_WRITE_BIT     (Not in DOS Name Space)
 *                                NS_GROUP_READ_BIT      (Not in DOS Name Space)
 *                                NS_OWNER_EXECUTE_BIT
 *                                NS_OWNER_WRITE_BIT
 *                                NS_OWNER_READ_BIT
 *                                NS_STICKY_BIT          (Not in DOS Name Space)
 *                                NS_SET_GID_BIT         (Not in DOS Name Space)
 *                                NS_SET_UID_BIT         (Not in DOS Name Space)
 *                                NS_FILE_EXECUTABLE_BIT (Not in DOS Name Space)
 *                                NS_MANDATORY_LOCK_BIT  (Not in DOS Name Space)
 *                                NS_HIDDEN_FILE_BIT
 *    nameSpaceInfo->nodeNumberOfLinks - Set to number of names linked to the
 *                                       data space.
 *    nameSpaceInfo->nodeSize   - Set to size in bytes of the data space.
 *    nameSpaceInfo->nodeMajorNumber - Set to the Block or Character Device 
 *                                     major number. (Set only when nodeType
 *                                     is NS_CHARACTER_SPECIAL or 
 *                                     NS_BLOCK_SPECIAL).
 *    nameSpaceInfo->nodeMinorNumber - Set to the Block or Character Device 
 *                                     minor number. (Set only when nodeType
 *                                     is NS_CHARACTER_SPECIAL or 
 *                                     NS_BLOCK_SPECIAL).
 *    nameSpaceInfo->userID     - Set to the user identifier of the object
 *                                owner.
 *    nameSpaceInfo->groupID    - Set to the group identifier of the object
 *                                owner.
 *    nameSpaceInfo->accessTime - Set to the time of last access in seconds
 *                                since 00:00:00 GMT 1/1/70.
 *    nameSpaceInfo->modifyTime - Set to the time of last data space
 *                                modification in seconds since 00:00:00
 *                                GMT 1/1/70.
 *    nameSpaceInfo->changeTime - Set to the time of last name space
 *                                modification in seconds since 00:00:00
 *                                GMT 1/1/70.
 *    diagnostic                - Set to one of the following if an error
 *                                occurs:
 *
 * RETURN VALUES
 *    0           - Successful completion.
 *    -1          - Unsuccessful completion, diagnostic contains reason.
 *
 * DESCRIPTION
 *    The NWfsManageNameSpaceInfo gets or sets the name space information of the
 *    specified serverNode.
 *
 * SEE ALSO
 *    NWfsManageNameSpaceInfoByName(3k), and NWfsModifyNameSpaceInfo(3k).
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWfsSetNameSpaceInfo (
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	NWSI_NAME_SPACE_T	*nameSpaceInfo,
	enum	NUC_DIAG	*diagnostic)
{
	NWFS_SERVER_NODE_T	*serverNode;
	nwcred_t		nucCredentials;
	ccode_t			returnCode = FAILURE;
	int			handleTries = 0;

	NVLT_ENTER (3);

	/* 
	 * Get the serverNode from the client handle.
	 */
	NVLT_ASSERT(nameSpaceInfo != NULL);
	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
	serverNode = clientHandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(serverNode));

	/*
	 * Convert NWfs credentials to NUC credentials.
	 */
	NWfiFsToNucCred(&clientHandle->credentials, &nucCredentials);

	/*
	 * Set the name space information of the specified serverNode.
	 */
	while (NWsiSetNameSpaceInfo (&nucCredentials,
		       serverNode->nodeVolume->spilVolumeHandle, NULL,
		       NULL, nameSpaceInfo, diagnostic) != SUCCESS) {
		switch (*diagnostic) {
			/*
			 * The server is up, but the user has lost either
			 * lost the connection or authentication. Attempt
			 * to re-establish.
			 */
			case SPI_AUTHENTICATION_FAILURE:
			case SPI_BAD_CONNECTION:
				if (SNODE_HAS_FLOCK(serverNode)) {
					NWfsFlockStale(serverNode);
					goto fail;
				}
				if (++handleTries >= NUCFS_RETRY_LIMIT)
					goto fail;
				break;

			/*
			 * The server is down, or going down, or we got
			 * some file, file system, or server specific error.
			 * Fail the operation now.
			 */
			default:
				goto fail;
		}

		/*
		 * Okay, (re)authenticate the user.
		 */
		if (NWsiAutoAuthenticate(&nucCredentials,
			    serverNode->nodeVolume->spilVolumeHandle,
			    NUC_SINGLE_LOGIN_ONLY,
			    diagnostic) != SUCCESS) {
			goto fail;
		}
	}

	if (nameSpaceInfo->nodePermissions != (uint32) -1 &&
	    nameSpaceInfo->nodePermissions & NS_OWNER_WRITE_BIT) {
		SNODE_LOCK(serverNode);
		/*
		 * If the NWCH_RH_CMOC then this file was supposed to be
		 * created without the w permission for the owner. However,
		 * the underlying layers granted w anyway (in order to enable
		 * the server to write). Now chmod(2) is changing it to w for
		 * owner. Thus we should not cancel owner's w permission at
		 * close time.
		 */
		NVLT_ASSERT((clientHandle->clientHandleFlags & NWCH_RH_CMOC) ?
			    serverNode->nodeType == NS_FILE : 1);
		clientHandle->clientHandleFlags &= ~NWCH_RH_CMOC;
		SNODE_UNLOCK(serverNode);
	}

	returnCode = SUCCESS;

fail:
	return (NVLT_LEAVE (returnCode));
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
 * BEGIN_MANUAL_ENTRY( NWfsMoveNode(3k), \
 *                     ./man/kernel/nucfs/nwfs/MoveNode )
 * NAME
 *    NWfsMoveNode - Moves the specified oldServerNode to the specified
 *                   newParentServerNode with the specified newName.
 *
 * SYNOPSIS
 *    ccode_t
 *    NWfsMoveNode (credentials, oldParentNode, oldNode, newParentNode,
 *                  newName, &diagnostic)
 *    opaque_t           *credentials;
 *    NWFS_SERVER_NODE_T *oldParentNode;
 *    NWFS_SERVER_NODE_T *oldNode;
 *    NWFS_SERVER_NODE_T *newParentNode;
 *    char               *newName;
 *    enum  NUC_DIAG     *diagnostic;
 *
 * INPUT
 *    credentials   - Credentials of the UNIX client moving the server node.
 *    oldparentNode - Server node object representing the directory where the
 *                    old server node resides in.
 *    oldName       - Old name of the server node to be moved.
 *    newParentNode - Server node object representing the directory where the
 *                    new server node must reside in.
 *    newName       - New name to be given to the server node after move.
 *
 * OUTPUT
 *    diagnostic    - Set to one of the following if an error occurs: 
 *
 * RETURN VALUES
 *    0             - Successful completion.
 *    -1            - Unsuccessful completion, diagnostic contains reason.
 *
 * DESCRIPTION
 *    The NWfsMoveNode moves the node assciated with specified oldName to the
 *    specified newParentNode dirctory with the given newName.  It does this
 *    by removing the oldNode and creating a new server node to represent the
 *    new server node.
 *
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWfsMoveNode (
	NWFS_CRED_T		*credentials,
	NWFS_SERVER_NODE_T	*oldParentNode,
	char			*oldName,
	NWFS_SERVER_NODE_T	*sourceNode,
	NWFS_SERVER_NODE_T	*newParentNode,
	char			*newName,
	enum	NUC_DIAG	*diagnostic)
{
	ccode_t			returnCode = FAILURE;
	NWFS_NAME_T		*oldHashName, *newHashName;
	NWSI_NAME_SPACE_T	*nameSpaceInfo;

	NVLT_ENTER (7);
	NVLT_STRING (oldName);
	NVLT_STRING (newName);

	NVLT_ASSERT (SNODE_REASONABLE(oldParentNode));
	NVLT_ASSERT (oldParentNode->nodeType == NS_DIRECTORY ||
		oldParentNode->nodeType == NS_ROOT);
	NVLT_ASSERT (SNODE_REASONABLE(newParentNode));
	NVLT_ASSERT (newParentNode->nodeType == NS_DIRECTORY ||
		newParentNode->nodeType == NS_ROOT);

	oldHashName = NWfsLookupOrEnterName(oldName);
	newHashName = NWfsLookupOrEnterName(newName);

	if (oldHashName == NWfsDot || newHashName == NWfsDotDot) {
		/*
		 * Can not move the current directory or the parent directory.
		 */
		*diagnostic = NUCFS_INVALID_DATA;
		goto done;
	}

	if (newHashName == NWfsDot || newHashName == NWfsDotDot) {
		/*
		 * Cannot create a "." or ".." file on the server.
		 */
		*diagnostic = NUCFS_INVALID_DATA;
		goto done;
	}

	if (sourceNode->nodeType == NS_ROOT) {
		/*
		 * Cannot rename the root of the file system.
		 */
		*diagnostic = NUCFS_VOLUME_BUSY;
		goto done;
	}

	/*
	 * Close all child client handles.
	 */
	if (sourceNode->nodeType != NS_DIRECTORY) {
		if (NWfsCloseAllHandles(sourceNode, diagnostic) != SUCCESS)
			goto done;
	}

	nameSpaceInfo = kmem_alloc(sizeof(NWSI_NAME_SPACE_T), KM_SLEEP);

	/*
	 * Open regular files get special handling when moving to another
	 * directory because we wish to avoid having the NetWare server
	 * assign a new nodeNumber, which would stale out open file
	 * descriptors.
	 *
	 * XXX: Note that we are holding the WRITE lock on the oldparentNode
	 *	directory, thus inhibiting new opens through that
	 *	directory. However, new opens can still occur through hard
	 *	links from other directories, resulting in possible stale
	 *	file descriptors.
	 *
	 * XXX: We would like to do the same thing with directories, but
	 *	there doesn't seem to be any way to go about it.
	 */
	if (sourceNode->nodeType == NS_FILE &&
	    oldParentNode != newParentNode && NWfiActiveNode(sourceNode, 2)) {

		if (NWfsGetAttribsByName(credentials, oldParentNode,
				sourceNode, oldName, nameSpaceInfo,
				diagnostic) != SUCCESS) {
			/*
			 * Parent/child relationship is broken. Return an
			 * error to the caller.
			 */
			goto done1;
		}

		if (nameSpaceInfo->nodeNumber !=
		    nameSpaceInfo->linkNodeNumber) {
			/*
			 * We are renaming a hard link. Since this cannot
			 * change file identity, we can just do a normal
			 * rename.
			 */
			goto normalRename;
		}

		if (sourceNode->delayDeleteClientHandle != NULL) {
			/*
			 * Delay delete is already pending.
			 */
			*diagnostic = NUCFS_NODE_NOT_FOUND;
			goto done1;
		}

		/*
		 * To avoid changing file identity, we build a hard link
		 * and hide the real file (setting the real file up for a
		 * delayed delete).
		 */
		if (NWfsLinkFile (credentials, sourceNode, newParentNode,
				  newName, diagnostic) != SUCCESS) {
			goto done1;
		}

		/*
		 * Now set up the node for delayed delete.
		 */
		if (NWfsHideNode(credentials, oldParentNode, sourceNode,
				 diagnostic) != SUCCESS) {
			goto done1;
		}

		returnCode = SUCCESS;
		goto done1;
	}

normalRename:
	if (NWfsRenameNode(credentials, oldParentNode, newParentNode,
			   sourceNode, oldName, newName,
			   diagnostic) != SUCCESS) {
		goto done1;
	}

	returnCode = SUCCESS;

        /*
         * Allow resource handles to (re)open on the renamed file.
         */
done1:
	if (sourceNode->nodeType != NS_DIRECTORY)
		NWfsAllowResourceHandlesToOpen(sourceNode);
	kmem_free(nameSpaceInfo, sizeof(NWSI_NAME_SPACE_T));

done:
        NWFS_NAME_RELEASE(newHashName);
        NWFS_NAME_RELEASE(oldHashName);
	return (NVLT_LEAVE (returnCode));
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
 * BEGIN_MANUAL_ENTRY( NWfsOpenNode(3k), \
 *                     ./man/kernel/nucfs/nwfs/OpenNode )
 * NAME
 *    NWfsOpenNode - Opens a SPIL NetWare node object.
 *
 * SYNOPSIS
 *    ccode_t
 *    NWfsOpenNode (credentials, serverNode, accessFlags, diagnostic)
 *    opaque_t       *credentials;
 *    opaque_t       *serverNode;    Object is opaque to caller. 
 *    uint32         accessFlags;
 *    enum  NUC_DIAG *diagnostic;
 *
 * INPUT
 *    credentials - Credentials of the UNIX client opening the SPIL NetWare
 *                  node object.
 *    serverNode  - Server node object to attach the client handle object of 
 *                  the SPIL resource handle to.
 *    accessFlags - The access flags the serverNode file was opened with.  The
 *                  accessFlags are ignored when dealing with directories.  It
 *                  is set to an inclusive 'OR' of the following:
 *                  NW_READ      - File was opened for reading.
 *                  NW_WRITE     - File was opened for writing.
 *                  NW_EXCLUSIVE - File was opened non shareable.
 *
 * OUTPUT
 *    nameSpaceInfo->nodeNumber - Set to the unique object identifier in the
 *                                server volume.  This is not set for DOS
 *                                name space.
 *    nameSpaceInfo->nodeType   - Set to the following object type:
 *                                NS_FILE
 *                                NS_DIRECTORY
 *                                NS_CHARACTER_SPECIAL   (Not in DOS Name Space)
 *                                NS_BLOCK_SPECIAL       (Not in DOS Name Space)
 *                                NS_FIFO                (Not in DOS Name Space)
 *                                NS_SYMBOLIC_LINK       (Not in DOS Name Space)
 *    nameSpaceInfo->nodePermission - Set to an inclusive OR of the following
 *                                    object mask:
 *                                NS_OTHER_EXECUTE_BIT   (Not in DOS Name Space)
 *                                NS_OTHER_WRITE_BIT     (Not in DOS Name Space)
 *                                NS_OTHER_READ_BIT      (Not in DOS Name Space)
 *                                NS_GROUP_EXECUTE_BIT   (Not in DOS Name Space)
 *                                NS_GROUP_WRITE_BIT     (Not in DOS Name Space)
 *                                NS_GROUP_READ_BIT      (Not in DOS Name Space)
 *                                NS_OWNER_EXECUTE_BIT
 *                                NS_OWNER_WRITE_BIT
 *                                NS_OWNER_READ_BIT
 *                                NS_STICKY_BIT          (Not in DOS Name Space)
 *                                NS_SET_GID_BIT         (Not in DOS Name Space)
 *                                NS_SET_UID_BIT         (Not in DOS Name Space)
 *                                NS_FILE_EXECUTABLE_BIT (Not in DOS Name Space)
 *                                NS_MANDATORY_LOCK_BIT  (Not in DOS Name Space)
 *                                NS_HIDDEN_FILE_BIT
 *    nameSpaceInfo->nodeNumberOfLinks - Set to number of names linked to the
 *                                       data space.
 *    nameSpaceInfo->nodeSize   - Set to size in bytes of the data space.
 *    nameSpaceInfo->nodeMajorNumber - Set to the Block or Character Device 
 *                                     major number. (Set only when nodeType
 *                                     is NS_CHARACTER_SPECIAL or 
 *                                     NS_BLOCK_SPECIAL).
 *    nameSpaceInfo->nodeMinorNumber - Set to the Block or Character Device 
 *                                     minor number. (Set only when nodeType
 *                                     is NS_CHARACTER_SPECIAL or 
 *                                     NS_BLOCK_SPECIAL).
 *    nameSpaceInfo->userID     - Set to the user identifier of the object
 *                                owner.
 *    nameSpaceInfo->groupID    - Set to the group identifier of the object
 *                                owner.
 *    nameSpaceInfo->accessTime - Set to the time of last access in seconds
 *                                since 00:00:00 GMT 1/1/70.
 *    nameSpaceInfo->modifyTime - Set to the time of last data space
 *                                modification in seconds since 00:00:00
 *                                GMT 1/1/70.
 *    nameSpaceInfo->changeTime - Set to the time of last name space
 *                                modification in seconds since 00:00:00
 *                                GMT 1/1/70.
 *    diagnostic - Set to one of the following if an error occurs:
 *
 * RETURN VALUES
 *    0          - Successful completion.
 *    -1         - Unsuccessful completion, diagnostic contains reason.
 *
 * DESCRIPTION
 *    The NWfsOpenNode opens a SPIL NetWare node object.  
 *
 * NOTE
 *    There are two posibilities when opening a node:
 *        1) The node to be opened is a file.  The specified accessFlags are 
 *           used only when openning file (NS_FILE, NS_SPECIAL_CHARACTER, etc),
 *           and specifies the flags to open the file with.  NWfsCreateFileNode
 *           is called when the file is created initially, which allocates a
 *           server node (NWFS_SERVER_NODE_T) and creates a SPIL NetWare file 
 *           node to represent the new file.  NWfsCreateFileNode does not set
 *           the cached accessFlags in the new server node object, because the 
 *           access (open) flags are not passed to NWfsCreateFileNode.  The
 *           server file node's accessFlags is set when the NWfsOpenNode is
 *           called.  Server node objects have a universe access mode 
 *           represented as an inclusive 'OR' of all access modes (ie. the 
 *           maximal access on the node of all opened UNIX client 
 *           instantiations, and the associated maximal access on the node of
 *           all UNIX Generic File System file structures (ie. struct file)).
 *           NWfsOpenNode also allocates a file client handle object, if one
 *           is not allocated for the UNIX client opening the file.
 *        2) The node to be opened is a directory.  The specified accessFlags
 *           are ignored when openning directoris (NS_DIRECORY, NS_ROOT). 
 *           NWfsMakeDirNode is called when the directory is created initially,
 *           which allocated a server node and creates a NetWare directory node
 *           on the NetWare server to represent the new directory.
 *    
 * SEE ALSO
 *    NWfsCreateFileNode(3k), NWfsMakeDirNode(3k), and NWfsAllocateRootNode(3k).
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWfsOpenNode (
	NWFS_CRED_T		*nwfscred,
	NWFS_SERVER_NODE_T	*serverNode,
	NWFS_CLIENT_HANDLE_T	**retClientHandle,
	uint32			accessFlags,
	int			*handleIsNew,
	enum	NUC_DIAG	*diagnostic)
{
	NWFS_CLIENT_HANDLE_T	*clientHandle;

	NVLT_ENTER (6);

	NVLT_ASSERT(SNODE_REASONABLE(serverNode));

	/*
	 * Get a client handle corresponding to the credentials.
	 */
	clientHandle = NWfsGetClientHandle(serverNode, nwfscred);

	/*
	 * Open a new resource for this client handle, or attach to
	 * a pre-existing one.
	 */
	if (NWfsOpenResourceHandle(clientHandle, accessFlags, TRUE, FALSE,
				   handleIsNew, diagnostic) != SUCCESS) {
		if (SNODE_HAS_FLOCK(serverNode))
			NWfsFlockStale(serverNode);
		goto fail;
	}

	/*
	 * NWfsOpenResourceHandle gave us a hold on the resource handle.
	 * This is a hold we don't need any longer.
	 */
	NWfsReleaseResourceHandle(clientHandle);

	*retClientHandle = clientHandle;
	return (NVLT_LEAVE (SUCCESS));

fail:
	NWfsReleaseClientHandle(clientHandle);
	*retClientHandle = NULL;
	return (NVLT_LEAVE (FAILURE));
}


/*
 * BEGIN_MANUAL_ENTRY( NWfsReadBlocksOnNode(3k), \
 *                     ./man/kernel/nucfs/nwfs/ReadBlockOnNode )
 * NAME
 *    NWfsReadBlocksOnNode - Reads logical blocks of data from a SPIL NetWare
 *                           file node.
 *
 * SYNOPSIS
 *    ccode_t
 *    NWfsReadBlocksOnNode (credentials, fileNode, ioArgs, diagnostic)
 *    opaque_t        *credentials;
 *    opaque_t        *fileNode;      Object is opaque to caller. 
 *    NUCFS_IO_ARGS_T *ioArgs;
 *    enum  NUC_DIAG   *diagnostic;
 *
 * INPUT
 *    credentials                - Credentials of the UNIX client reading the
 *                                 data space of a file.
 *    fileNode                   - Server node object associated with the file
 *                                 to read data from.
 *    ioArgs->granuleOffset      - Byte offset from beginning of file's data
 *                                 space to start the read from.
 *    ioArgs->granulesRequested  - Number of blocks to read.
 *    ioArgs->memoryTypeFlag     - same as iobuf_t memoryType.
 *    ioArgs->ioBuffer           - Pointer to a contiguous block to read the
 *                                 bytes of data into.
 *
 * OUTPUT
 *    ioArgs->ioBuffer           - Is loaded with block(s) of data.
 *    ioArgs->granulesReturned   - Number of blocks read.
 *    diagnostic                 - Set to one of the following if an error 
 *                                 occurs:
 *                                 NUCFS_ACCESS_DENIED
 *                                 NUCFS_INVALID_OFFSET
 *                                 NUCFS_INVALID_SIZE
 *
 * RETURN VALUES
 *    0                          - Successful completion.
 *    -1                         - Unsuccessful completion, diagnostic contains
 *                                 reason.
 *
 * DESCRIPTION
 *    The NWfsReadBlocksOnNode reads logical blocks of data space from a file,
 *    and returns the number of blocks read.
 *
 * NOTES
 *    The only valid file type is NS_FILE.  NS_FIFO, NS_SPECIAL_CHARACTER,
 *    NS_SPECIAL_BLOCK, and NS_SYMBOLIC_LINK not supported currently.
 *
 *    Need to worry about locks.
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */


/*
 * BEGIN_MANUAL_ENTRY( NWfsReadBytesOnNode(3k), \
 *                     ./man/kernel/nucfs/nwfs/ReadBytesOnNode )
 * NAME
 *    NWfsReadBytesOnNode - Reads bytes of data from a SPIL NetWare file node
 *                          object.
 *
 * SYNOPSIS
 *    ccode_t
 *    NWfsReadBytesOnNode (credentials, filrNode, ioArgs, diagnostic)
 *    opaque_t        *credentials;
 *    opaque_t        *fileNode;     Object is opaque to caller. 
 *    NUCFS_IO_ARGS_T *ioArgs;
 *    enum  NUC_DIAG  *diagnostic;
 *
 * INPUT
 *    credentials                - Credentials of the UNIX client reading from
 *                                 a file.
 *    fileNode                   _ Server node object representing the file to
 *                                 read bytes of data from. 
 *    ioArgs->granuleOffset      - Byte offset from beginning of file's data
 *                                 space to start the read from.
 *    ioArgs->granulesRequested  - Number of bytes to read.
 *    ioArgs->memoryTypeFlag     - Same as iobuf_t memoryType.
 *    ioArgs->ioBuffer           - Pointer to a contiguous block to read the
 *                                 bytes of data into.
 *
 * OUTPUT
 *    ioArgs->ioBuffer           - Is loaded with bytes of data.
 *    ioArgs->granulesReturned   - Number of bytes read.
 *    diagnostic                 - Set to one of the following if an error 
 *                                 occurs:
 *                                 NUCFS_ACCESS_DENIED
 *                                 NUCFS_INVALID_OFFSET
 *                                 NUCFS_INVALID_SIZE
 *
 * RETURN VALUES
 *    0                          - Successful completion.
 *    -1                         - Unsuccessful completion, diagnostic
 *                                 contains reason.
 *
 * DESCRIPTION
 *    The NWfsReadBytesOnNode reads bytes of data from a SPIL NetWare file node
 *    object, and returns the number of bytes read.
 *
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
 * NOTES
 *    The only valid file type is NS_FILE.  NS_FIFO, NS_SPECIAL_CHARACTER,
 *    NS_SPECIAL_BLOCK, and NS_SYMBOLIC_LINK not supported currently.
 *
 *    Need to worry about locks.
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */

ccode_t
NWfsReadBytesOnNode (
	NWFS_CRED_T		*credentials,
	NWFS_SERVER_NODE_T	*fileNode,
	NUCFS_IO_ARGS_T		*ioArgs,
	enum	NUC_DIAG	*diagnostic)
{
	NWFS_CLIENT_HANDLE_T	*fileClientHandle = NULL;
	NUC_IOBUF_T		*spilIoArgs = NULL;
	nwcred_t		nucCredentials;
	int			newlyCreated;
	ccode_t			returnCode = FAILURE;
	int			retries = 0;

	NVLT_ENTER (4);

	NVLT_ASSERT (SNODE_REASONABLE(fileNode));
	NVLT_ASSERT (fileNode->nodeType != NS_DIRECTORY);

	ioArgs->granulesReturned = 0;
	if (credentials == NULL) {
retry:
		/*
		 * We have no credentials (because we were called
		 * from the system). Therefore, surrogate on some
		 * reader for this file.
		 */
		if (NWfsAttachClientHandle(fileNode, &fileClientHandle,
				NW_READ) != SUCCESS) {
			*diagnostic = NUCFS_ACCESS_DENIED;
			goto done;
		}
	} else {
		fileClientHandle = NWfsGetClientHandle(fileNode, credentials);
	}

	/*
	 * Open a resource handle. However, do not authenticate the user
	 * at this point (for we may have pages locked) and thus are not
	 * willing to wait for such a thing to happen.
	 */
	if (NWfsOpenResourceHandle(fileClientHandle, NW_READ, FALSE, FALSE,
				   &newlyCreated, diagnostic) != SUCCESS) {
		NWfsReleaseClientHandle(fileClientHandle);
		/*
		 * Try another user.
		 */
		if (SNODE_HAS_FLOCK(fileNode))
			NWfsFlockStale(fileNode);
		else if (credentials == NULL && ++retries <= NUCFS_RETRY_LIMIT)
			goto retry;
		goto done;
	}

	/*
	 * Convert NWfs credentials to NUC credentials.
	 */
	NWfiFsToNucCred(&fileClientHandle->credentials, &nucCredentials);

	/*
	 * Set the SPIL IO args structure.
	 */
	spilIoArgs = kmem_alloc(sizeof(NUC_IOBUF_T), KM_SLEEP);
	spilIoArgs->buffer = ioArgs->ioBuffer;
	spilIoArgs->bufferLength = ioArgs->granulesRequested;
	spilIoArgs->memoryType = ioArgs->memoryTypeFlag;

	if (NWsiReadFile (&nucCredentials,
			fileNode->nodeVolume->spilVolumeHandle,
			fileClientHandle->resourceHandle,
			ioArgs->granuleOffset, spilIoArgs, diagnostic)
				!= SUCCESS) {
		switch (*diagnostic) {
		/*
		 * Beyond EOF. This is not an error, for our file size
		 * synchronization with the server is imprecise. However,
		 * it is an indication that our file size might be
		 * stale. So, just invalidate attributes at this time.
		 */
		case SPI_INVALID_OFFSET:
			NWfsInvalidateNode(fileNode);
			goto success;
                /*
                 * The server is up, but the user has lost either
                 * authentication or connection. If surrogating, then
		 * try another user.
                 */
                case SPI_AUTHENTICATION_FAILURE:
                case SPI_BAD_CONNECTION:
                        NWfsCloseResourceHandle(fileClientHandle);
			NWfsReleaseClientHandle(fileClientHandle);
			if (SNODE_HAS_FLOCK(fileNode))
				NWfsFlockStale(fileNode);
			else if (credentials == NULL &&
			    ++retries <= NUCFS_RETRY_LIMIT) {
				goto retry;
			}
                        goto done;
                /*
                 * The server is down, or going down, or we got
                 * some file or file system specific error.
                 */
                default:
                        goto done1;
                }
        }

	/*
	 * Total bytes read.
	 */
	ioArgs->granulesReturned = spilIoArgs->bufferLength;

success:
	returnCode = SUCCESS;
done1:
	NWfsReleaseResourceHandle(fileClientHandle);
	NWfsReleaseClientHandle(fileClientHandle);
done:
	if (spilIoArgs != NULL)
		kmem_free(spilIoArgs, sizeof(NUC_IOBUF_T));

	return (NVLT_LEAVE (returnCode));
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
 * BEGIN_MANUAL_ENTRY( NWfsReadDirNodeEntries(3k)i, \
 *                     ./man/kernel/nucfs/nwfs/ReadDirNodeEntries )
 * NAME
 *    NWfsReadDirNodeEntries - Reads directory entries from a SPIL NetWare 
 *                             directroy node.
 *
 * SYNOPSIS
 *    ccode_t
 *    NWfsReadDirNodeEntries (credentials, dirNode, entryFlag, dirIoArgs,
 *				diagnostic)
 *    opaque_t            *credentials;
 *    opaque_t	          *dirNode;
 *    uint8               entryFlag;
 *    NUCFS_DIR_IO_ARGS_T *dirIoArgs;
 *    enum  NUC_DIAG      *diagnostic;
 *
 * INPUT
 *    credentials                        - Credentials of the UNIX client
 *                                         reading the directory entries.
 *    dirNode                            - Directory server node to read the
 *                                         entries from.
 *    entryFlag                          - Can be set to one of the followings:
 *                                         GET_FIRST - First N entries.
 *                                         GET_MORE  - Next N entries.
 *    dirIoArgs->nucIoBuf.bufferLength   - Size of nucIoBuf.buffer.
 *    dirIoArgs->nucIoBuf.buffer         - Pointer to a contiguous buffer to 
 *                                         read the directory entries into.
 *    dirIoArgs->nucIoBuf.memoryType     - Always Set to IOMEM_KERNEL.
 *    dirIoArgs->dirSearchHandle         - SPIL search thread context handle.
 *
 * OUTPUT
 *    dirIoArgs->nucIoBuf.bufferLength   - Number of bytes saved in the 
 *                                         nucIoBuf.buffer.
 *    dirIoArgs->dirSearchHandle         - Next SPIL search thread context 
 *                                         handle.
 *    dirIoArgs->nucIoBuf.buffer         - Is loaded with a contiguous directory
 *                                         buffer of formatted directory entries
 *                                         according to NWSI_DIRECTORY_T.  As
 *                                         many entries as posssible are 
 *                                         formatted, or until end of directory
 *                                         is reached.
 *    diagnostic                         - Set to one of the following if an
 *                                         error occurs:
 *
 * RETURN VALUES
 *    0                                  - Successful completion.
 *    -1                                 - Unsuccessful completion, diagnostic
 *                                         contains reason.
 *
 * DESCRIPTION
 *    The NWfsReadDirNodeEntries read the specified NetWare directory's
 *    entries and fills the dirIoArgs->nucIoBuf.buffer argument with the UNIX
 *    generic directory entries.  The number of entries to read is 
 *    limited by an end of directory or the NetWare block size saved
 *    in the ioArgs->granulesRequested argument. 
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWfsReadDirNodeEntries (
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	NUCFS_DIR_IO_ARGS_T	*dirIoArgs,
	enum	NUC_DIAG	*diagnostic)
{
	NWFS_SERVER_NODE_T	*dirNode, *childNode;
	NWFS_CLIENT_HANDLE_T	*childClientHandle;
	nwcred_t		nucCredentials;
	NWSI_DIRECTORY_T	*spilDirPtr;
	char			*scanPtr;
	uint32			bytesScanned = 0;
	ccode_t			returnCode = FAILURE;
	ccode_t			error;
	int			newlyCreated;
	NWFS_CACHE_INFO_T	cacheInfo;
	int			handleTries = 0;
	NWFS_NAME_T		*hashName;

	NVLT_ENTER (3);

	NVLT_ASSERT (CHANDLE_REASONABLE(clientHandle));
	dirNode = clientHandle->snode;
	NVLT_ASSERT (SNODE_REASONABLE(dirNode));
	NVLT_ASSERT (dirNode->nodeType == NS_DIRECTORY ||
		dirNode->nodeType == NS_ROOT);

	/*
	 * Convert NWfs credentials to NUC credentials.
	 */
	NWfiFsToNucCred(&clientHandle->credentials, &nucCredentials);

	/*
	 * Get some directory entries.
	 */
	for (;;) {
		/*
		 * Open a NUC resource handle.
		 */
		if (NWfsOpenResourceHandle(clientHandle, 0, TRUE,
			FALSE, &newlyCreated, diagnostic) != SUCCESS) {
				goto fail_common;
		}

		cacheInfo.stale = TRUE;
		NWFI_GET_TIME(&cacheInfo.beforeTime);
		error = NWsiGetDirectoryEntries (&nucCredentials, 
				dirNode->nodeVolume->spilVolumeHandle,
				clientHandle->resourceHandle, GET_MORE,
				&(dirIoArgs->dirSearchHandle),
				&(dirIoArgs->nucIoBuf), diagnostic);
		NWFI_GET_TIME(&cacheInfo.afterTime);

		if (error == SUCCESS) {
			NWfsReleaseResourceHandle(clientHandle);
			break;
		}

		NWfsCloseResourceHandle(clientHandle);

		switch (*diagnostic) {
		/*
		 * The server is up, but the user has lost authentication.
		 */
		case SPI_AUTHENTICATION_FAILURE:
		case SPI_BAD_CONNECTION:
			if (++handleTries >= NUCFS_RETRY_LIMIT)
				goto fail_common;
			break;
		/*
		 * The server is down, or we received some file system
		 * specific error. Return this to the caller.
		 */
		default:
			goto fail_common;
		}
	}

	if (dirIoArgs->nucIoBuf.bufferLength == 0)
		/*
		 * No more entries left.
		 */
		return (NVLT_LEAVE (SUCCESS));

	/*
	 * For every entry returned, we wish to cache the name space
	 * information (for some period of time).
	 * 
	 * NOTE:
	 *    This architectural enhancement was added to cache the name
	 *    space information of each directory entry to speed up
	 *    commands like the directory listing (ls). Otherwise, another
	 *    wire call per listed file would be needed when the
	 *    application invoked stat(2).
	 *
	 *    New server nodes might be allocated for the purpose of
	 *    caching the information just obtained (if the nodes do not
	 *    already exist). If not otherwise busy, these server nodes
	 *    will end up on the inactive (cached) list.
	 */
	scanPtr = (char *)(dirIoArgs->nucIoBuf.buffer);
	
	NVLT_ASSERT(dirIoArgs->nucIoBuf.bufferLength > 0);

	while (bytesScanned < dirIoArgs->nucIoBuf.bufferLength) {

		spilDirPtr = (NWSI_DIRECTORY_T *)(void *) scanPtr;
		NVLT_ASSERT((((unsigned int) scanPtr) %
			     sizeof (char *)) == 0);

		/*
		 * Onto the next entry.
		 */
		bytesScanned += spilDirPtr->structLength;
		scanPtr += spilDirPtr->structLength;

		/*
		 * Trash returned from the server?
		 */
		if (spilDirPtr->structLength <
				offsetof(NWSI_DIRECTORY_T, name) + 2 ||
		    spilDirPtr->name[0] == '\0' ||
		    bytesScanned > dirIoArgs->nucIoBuf.bufferLength) {
			cmn_err(CE_NOTE,
				"NWfsReadDirNodeEntries: bad entries "
				"obtained for dir node %d in volume %s\n,",
				dirNode->nodeNumber & NWFS_NODE_MASK,
				dirNode->nodeVolume->volumeName);
			*diagnostic = NUCFS_EIO;
			goto fail_common;
		}

		/*
		 * Guarantee that name is NULL terminated.
		 */
		spilDirPtr->name[spilDirPtr->structLength -
				 offsetof(NWSI_DIRECTORY_T, name) - 1] = '\0';

		/*
		 * Hash in the name. The hashed name is returned held.
		 */
		hashName = NWfsLookupOrEnterName(spilDirPtr->name);

		/*
		 * Get (possibly allocate) a child node. The child node is
		 * returned active and held. The effect of this is to
		 * cache the nameSpaceInfo and the child name.
		 */
		if (NWfsCreateOrUpdateNode(clientHandle, hashName,
				&cacheInfo, &spilDirPtr->nameSpaceInfo,
				NULL, 0, &childNode, diagnostic) != SUCCESS) {
			NWFS_NAME_RELEASE(hashName);
			goto fail_common;
		}

		/*
		 * If the child is a directory, then cache ".." for it.
		 */
		if (childNode->nodeType == NS_DIRECTORY) {
			NVLT_ASSERT(SNODE_REASONABLE(dirNode));
			childClientHandle = NWfsGetClientHandle(childNode,
						&clientHandle->credentials);
			NWfsCacheName(childClientHandle, NWfsDotDot,
					clientHandle->snode);
			NWfsReleaseClientHandle(childClientHandle);
		}

		/*
		 * We don't want to hold the child node active.
		 */
		SNODE_RELEASE(childNode);
		NWFS_NAME_RELEASE(hashName);
	}

	returnCode = SUCCESS;

fail_common:
	return (NVLT_LEAVE (returnCode));
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
 * BEGIN_MANUAL_ENTRY( NWfsRemoveDirNode(3k), \
 *                     ./man/kernel/nucfs/nwfs/RemoveDirNode )
 * NAME
 *    NWfsRemoveDirNode - Permanently removes the specified server directory
 *                        node object.
 *
 * SYNOPSIS
 *    ccode_t
 *    NWfsRemoveDirNode (credentials, parentNode, dirNode, diagnostic)
 *    opaque_t       *credentials;
 *    opaque_t       *parentNode;        Object is opaque to the caller.
 *    opaque_t       *dirNode;           Object is opaque to the caller.
 *    enum  NUC_DIAG *diagnostic;
 *
 * INPUT
 *    credentials - Credentials of the UNIX client deleting the file.
 *    parentNode  - Parent server node of the specified fileNode to be removed.
 *    dirNode     - Server directory node to be deleted.
 *
 * OUTPUT
 *    diagnostic  - Set to one of the following if an error occurs:
 *                  NUCFS_NOT_A_DIRECTORY
 *                  NUCFS_DIRECTORY_NOT_EMPTY
 *                  NUCFS_ACCESS_DENIED
 *
 * RETURN VALUES
 *    0           - Successful completion.
 *    -1          - Unsuccessful completion, diagnostic contains reason.
 *
 * DESCRIPTION
 *    The NWfsRemoveDirNode permanently removes the specified server directory
 *    node object.
 *
 * NOTES
 *    If the parent server directory node is writable and has the sticky bit
 *    set, server directories within the directory can be removed only if one or
 *    more of the following is true:
 *        The UNIX client is the super-user.
 *        The UNIX client owns the directory to be deleted.
 *        The UNIX client owns the parent directory.
 *        The directory to be deleted is writable by the UNIX client.
 *    
 *    In UNIX semantics when a directory is to be deleted, the link count is 
 *    decremented by one on the generic inode itself, and the directory name in
 *    its parent directory is removed.  However the generic inode object is not
 *    really deleted if the link count is still greater than zero, or the 
 *    reference count is still greater than zero.  In a distributed file system
 *    such as NetWare, the actual atomic directory object is managed by the
 *    Server, and the name of the object is mapped to a resource handle the
 *    client system accesses the object through.  Thus, the name of the object
 *    is deleted from its parent directory by the client system, and it is up
 *    to the server to actually delete the distributed directory object.  
 *    However to preserve local UNIX semantics, the name must be removed from
 *    search by the local UNIX processes, but the object deletion request to the
 *    Server itself must be delayed until the last UNIX process referece is
 *    released, at which time the NWfsReleaseNode(3K) will detect the directory
 *    object is to be deleted, and will delete the directory object on behalf of
 *    the UNIX client who requested deletion via NWfsDeleteFileNode(3K) via a
 *    proxy of their credentials.
 *
 *    The directory to be deleted will be marked as hidden file.  This will
 *    prevent it from being advertised when listing its parent directory
 *    entries.  If the server or the client systems crash prior to receiving 
 *    the delay deletion request, the directory object will inadvertntly not be
 *    deleted on the server.
 *
 * SEE ALSO
 *    NWfsReleaseNode(3k), NWfsDeleteFileNode(3k), NWfsManageNameSpaceInfo(3k),
 *    and NWsiCheckNodeAccess(3k).
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWfsDeleteNode (
	NWFS_CRED_T		*credentials,
	NWFS_SERVER_NODE_T	*parentNode,
	char			*childName,
	NWFS_SERVER_NODE_T	*childNode,
	enum	NUC_DIAG	*diagnostic)
{
	NWFS_CLIENT_HANDLE_T	*parentClientHandle;
	ccode_t			returnCode = FAILURE;
	NWFS_NAME_T		*hashName;
	NWSI_NAME_SPACE_T	*nameSpaceInfo;

	NVLT_ENTER (5);

	NVLT_ASSERT (SNODE_REASONABLE(parentNode));
	NVLT_ASSERT (SNODE_REASONABLE(childNode));
	NVLT_ASSERT (parentNode->nodeType == NS_DIRECTORY ||
		     parentNode->nodeType == NS_ROOT);

	if (childNode->nodeType == NS_ROOT) {
		/*
		 * Can not delete the root of the file system.
		 */
		*diagnostic = NUCFS_ACCESS_DENIED;
		goto done;
	}

	/*
	 * Hash in the name. The hashed name is returned held.
	 */
	hashName = NWfsLookupOrEnterName(childName);

        /*
         * return error when deleting . and ..
         */
        if (hashName == NWfsDot) {
                *diagnostic = NUCFS_INVALID_DATA;
                goto done1;
        }
        if (hashName == NWfsDotDot) {
                *diagnostic = NUCFS_NODE_ALREADY_EXISTS;
                goto done1;
        }

	if (childNode == parentNode) {
		/*
		 * Name cache contains junk (probably stale).
		 */
		*diagnostic = NUCFS_NOT_CHILD;
		goto done1;
	}

	/*
	 * Close all child client handles.
	 */
	if (NWfsCloseAllHandles(childNode, diagnostic) != SUCCESS)
		goto done1;

	nameSpaceInfo = kmem_alloc(sizeof(NWSI_NAME_SPACE_T), KM_SLEEP);

	/*
	 * Open regular files get special handling because we wish to
	 * implement UNIX like semantics here, wherein the delete is
	 * delayed until file deactivation time.
	 *
	 * Directories are ignored under the following grounds:
	 *	=> They must be empty to be deleted, so there is really no
	 *	   state to preserve.
	 *	=> If we let a deleted directory live under a hidden name,
	 *	   and should the user create files in it, we might not
	 *	   be able to delete it at deactivation time.
	 *
	 * XXX: Note that we are holding the WRITE lock on the oldparentNode
	 *	directory, thus inhibiting new opens through that
	 *	directory. However, new opens can still occur through hard
	 *	links from other directories, resulting in possible stale
	 *	file descriptors.
	 *
	 * XXX: We ignore directories; there doesn't seem to be any way to
	 *	handle them.
	 *
	 */
	if (childNode->nodeType == NS_FILE && NWfiActiveNode(childNode, 1)) {

		if (NWfsGetAttribsByName(credentials, parentNode,
				childNode, childName, nameSpaceInfo,
				diagnostic) != SUCCESS) {
			/*
			 * Parent/child relationship is broken. Return an
			 * error to the caller.
			 */
			goto done2;
		}

		if (nameSpaceInfo->nodeNumber !=
		    nameSpaceInfo->linkNodeNumber) {
			/*
			 * We are deleting a hard link. Since this cannot
			 * change file identity, we can just do a normal
			 * delete.
			 */
			goto normalDelete;
		}

		if (childNode->delayDeleteClientHandle != NULL) {
			/*
			 * Delay delete is already pending.
			 */
			*diagnostic = NUCFS_NODE_NOT_FOUND;
			goto done2;
		}

		/*
		 * To avoid changing file identity, we hide the file under
		 * an assumed name, setting it for a delayed delete.
		 */
		if (NWfsHideNode(credentials, parentNode, childNode,
				 diagnostic) != SUCCESS) {
			goto done2;
		}

		returnCode = SUCCESS;
		goto done2;
	}
normalDelete:

	/*
	 * Get a client handle for the parent.
	 */
	parentClientHandle = NWfsGetClientHandle(parentNode, credentials);

	/*
	 * Now, attempt to remove the node ``by name''.
	 */
	returnCode = NWfsDeleteByName(parentClientHandle, childName, childNode,
				 	  diagnostic);
	if (returnCode == SUCCESS) {
		/*
		 * Both nodes (if they still exist) have now been modified.
		 * The childNode may no longer exist (if we removed the last
		 * link) but we don't really know that, we don't wish to
		 * guess, and we will find out at our next access if that
		 * should occur. So just invalidate its caches for now.
		 */
		NWfsInvalidateNode(childNode);
		SNODE_SET_MODIFIED(parentNode);
	}

	NWfsReleaseClientHandle(parentClientHandle);

        /*
         * Allow resource handles to (re)open on the renamed file.
         */
done2:
        NWfsAllowResourceHandlesToOpen(childNode);
	kmem_free(nameSpaceInfo, sizeof(NWSI_NAME_SPACE_T));
done1:
	NWFS_NAME_RELEASE(hashName);
done:
	return (NVLT_LEAVE (returnCode));
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
 * BEGIN_MANUAL_ENTRY( NWfsSymbolicLink(3k), \
 *                     ./man/kernel/nucfs/nwfs/SymbolicLink )
 * NAME
 *    NWfsSymbolicLink - Creates a symbolic link file.
 *
 * SYNOPSIS
 *    ccode_t
 *    NWfsSymbolicLink (credentials, newLinkParentNode, newLinkFileName,
 *                      newLinkNameSpaceInfo, relativeExistingNodePath,
 *                      newLinkNode, diagnostic)
 *    opaque_t           *credentials;
 *    opaque_t           *newLinkParentNode;       Object is opaque to caller.
 *    char               *newLinkFileName;
 *    NWSI_NAME_SPACE_T  *newLinkNameSpaceInfo;
 *    char               *relativeExistingNodePath;
 *    opaque_t           **newLinkNode;           Object is opaque to caller.
 *    enum  NUC_DIAG     *diagnostic;
 *
 * INPUT
 *    credentials              - Credentials of the UNIX client creating the
 *                               symbolic link file.
 *    newLinkParentNode        - The new link parent directory.
 *    newLinkFilename          - The new link file name.
 *    newLinkNameSpaceInfo     - The new link file name space information.
 *    relativeExistingNodePath - The relative existing node path.
 *    
 * OUTPUT
 *    newLinkNode              - Server node of the newly created symbolic link
 *                               file node.
 *    diagnostic               - Set to one of the following if an error occurs:
 *
 * RETURN VALUES
 *    0                        - Successful completion.
 *    -1                       - Unsuccessful completion, diagnostic contains 
 *                               reason.
 *
 * DESCRIPTION
 *    The NWfsSymbolicLink creates a symbolic link file which contains
 *    the relative path of where the existing link file is.
 *
 * EXAMPLE
 *     ln -s relativeExistingNodePath newLinkFile
 *
 * SEE ALSO
 *     NWfsCreateFileNode(3k), NWfsOpenNode(3k), NWfsWriteBytesOnNode(3k).
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWfsSymbolicLink (
	NWFS_CRED_T		*credentials,
	NWFS_SERVER_NODE_T      *newLinkParentNode,
	char			*newLinkFileName,
	NWSI_NAME_SPACE_T	*newLinkNameSpaceInfo,
	char			*relativeExistingNodePath,
	enum	NUC_DIAG	*diagnostic)
{
	NUCFS_IO_ARGS_T		ioArgs;
	NWFS_SERVER_NODE_T	*newLinkNode;
	ccode_t			returnCode = SUCCESS;
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	enum NUC_DIAG		diag;

	NVLT_ENTER (6);

	NVLT_STRING (newLinkFileName);
	NVLT_ASSERT (SNODE_REASONABLE(newLinkParentNode));
	NVLT_ASSERT (newLinkNameSpaceInfo->nodeType == NS_SYMBOLIC_LINK);

	/*
	 * Create a new link server file node.
	 */
	if (NWfsCreateFileNode (credentials, newLinkParentNode, newLinkFileName,
			newLinkNameSpaceInfo,
			NW_READ|NW_WRITE|NW_FAIL_IF_NODE_EXISTS,
			&newLinkNode, diagnostic) != SUCCESS) {
		returnCode = FAILURE;
		goto done;
	}

	/*
	 * XXX: There is no way to totally prevent some user from seeing
	 *	a NULL string for the symbolic link at this point. While
	 *	we could prevent it on this client by taking the parent's
	 *	RW write lock, and by forcing NWfsLookupNode to take the
	 *	parent's RW read lock, we chose not to do this. Instead, we
	 *	cause all reads of a NULL and young link to backoff and
	 *	re-read. This solution has the benefit of working on all
	 *	clients almost all of the time.
	 *
	 * Write the specified relativeExistingNodePath in the newly created
	 * symbolic link file.
	 */
	ioArgs.granuleOffset = 0;
	ioArgs.granulesRequested = strlen (relativeExistingNodePath);
	ioArgs.memoryTypeFlag = IOMEM_KERNEL;
	ioArgs.ioBuffer = (opaque_t *)relativeExistingNodePath;
	clientHandle = NWfsGetClientHandle(newLinkNode, credentials);
	if (NWfsWriteBytesOnNode (credentials, newLinkNode, &ioArgs, 
			diagnostic) != SUCCESS) {
		/*
		 * Do a best effort to delete. But, we are forced to
		 * ignore errors here.
		 */
		if (NWfsCloseAllHandles(newLinkNode, &diag) == SUCCESS) {
			NWfsDeleteById(clientHandle);
			NWfsAllowResourceHandlesToOpen(newLinkNode);
		}
		returnCode = FAILURE;
	}

	/*
	 * Both nodes have now been modified.
	 */
	SNODE_SET_MODIFIED(newLinkParentNode);
	SNODE_SET_MODIFIED(newLinkNode);

	/*
	 * Close any handles we used to write the data and release
	 * the node.
	 */
	NWfsCloseCachedResourceHandle(clientHandle);
	NWfsReleaseClientHandle(clientHandle);
	SNODE_RELEASE(newLinkNode);
done:
	return (NVLT_LEAVE (returnCode));
}


ccode_t
NWfsReadSymbolicLink(
	NWFS_SERVER_NODE_T	*netwareNode, 
	NUCFS_IO_ARGS_T		*netwareIoArgs,	
	int			maxBytesToRead,
	int			*bytesRead,
	NWFS_CRED_T		*nwfsCred, 
	enum NUC_DIAG		*diagnostic) 
{
	int32			currentServerTime;
	int			retries;
	int			returnCode = SUCCESS;

	NVLT_ENTER (6);

	NVLT_ASSERT(maxBytesToRead <= MAXPATHLEN);
	netwareIoArgs->memoryTypeFlag = IOMEM_KERNEL;
	netwareIoArgs->granuleOffset = 0;
	netwareIoArgs->granulesRequested = maxBytesToRead;
	netwareIoArgs->granulesReturned = 0;

	*bytesRead = 0;

	for (retries = 0; retries < NUCFS_RETRY_LIMIT; retries++) {

		NVLT_ASSERT (SNODE_REASONABLE(netwareNode));
		if (netwareNode->nodeState == SNODE_STALE) {
			*diagnostic = NUCFS_STALE;	
			returnCode = FAILURE;
			break;
		}

		/*
		 * Reading the data.
		 */
		if ((returnCode = NWfsReadBytesOnNode (
			nwfsCred, 
			netwareNode,
			netwareIoArgs, 
			diagnostic)) != SUCCESS) {
			break;
		}
		*bytesRead = netwareIoArgs->granulesReturned;
		NVLT_ASSERT(*bytesRead >= 0);
		if (*bytesRead > 0) {
			break;
		}
		/*
		 * we read 0 bytes of data. Either it could be the
		 * special case of an implied symlink to the parent
		 * directory or it could be a race with a symlink
		 * create operation. Check whether the symbolic link
		 * is a young symbolic link, and retry reading it if
		 * is; otherwise, decide that it is a valid symbolic
		 * link that refers to the containing directory.
		 */
		if ((returnCode = NWfsGetServerTime (
			nwfsCred,
			netwareNode->nodeVolume,
			&currentServerTime,
			diagnostic)) != SUCCESS) {
			break;
		}
		if ((currentServerTime - netwareNode->modifyTime) >=
			NWfiStaleTicks)	{
			/*
	 	 	 * We could not have raced with a symlink creation 
			 * operation that happened recently. So we 
			 * return a zero length symlink (identifying ".").
			 */
			break;
		}
		/*
		 * we may have raced with a recent symlink operation.
		 * retry after a small interval.
		 */
		NWfiDelay(HZ);
	}
	NVLT_LEAVE (returnCode);
	return returnCode;
}

/*
 * BEGIN_MANUAL_ENTRY( NWfsTruncateBytesOnNode(3k), \
 *                     ./man/kernel/nucfs/nwfs/TruncateBytesOnNode )
 * NAME
 *    NWfsTruncateBytesOnNode - Truncates bytes on a NetWare node object
 *                              file.
 * SYNOPSIS
 *    ccode_t
 *    NWfsTruncateBytesOnNode (credentials, fileNode, truncateOffset,
 *                             diagnostic)
 *    NWFS_CRED_T        *credentials;
 *    NWFS_SERVER_NODE_T *fileNode;
 *    uint32             truncateOffset;
 *    enum  NUC_DIAG     *diagnostic;
 *
 * INPUT
 *    credentials    - Credentials of the UNIX client truncating the specified
 *                     fileNode.
 *    fileNode       - Server file node to be truncated.
 *    truncateOffset - Byte offset form the beginning of the file to start 
 *                     truncation.
 *    
 * OUTPUT
 *    diagnostic     - Set to one of the following if an error occurs:
 *
 * RETURN VALUES
 *    0              - Successful completion.
 *    -1             - Unsuccessful completion, diagnostic contains reason.
 *
 * DESCRIPTION
 *    The NWfsTruncateBytesOnNode truncates the SPIL NetWare file node's data
 *    space associated with the specified fileNode from the specified 
 *    truncateOffset to the end of the file.
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 *
 * Calling/Exit State:
 *	Called with the RW lock held in WRITER mode and returns that
 *	way.
 */
ccode_t
NWfsTruncateBytesOnNode (
	NWFS_CRED_T		*credentials,
	NWFS_SERVER_NODE_T	*fileNode,
	uint32			truncateOffset,
	enum	NUC_DIAG	*diagnostic)
{
	NWFS_CLIENT_HANDLE_T	*clientHandle;
	ccode_t			retCode = FAILURE;
	int			handleIsNew;
	nwcred_t		nucCredentials;
	int			retries = 0;

	NVLT_ENTER (4);

	NVLT_ASSERT (SNODE_REASONABLE(fileNode));
	NVLT_ASSERT (fileNode->nodeType == NS_FILE);

        /*
         * Get a client handle corresponding to the credentials.
         */
        clientHandle = NWfsGetClientHandle(fileNode, credentials);

        /*
         * Open a new resource for this client handle, or attach to
         * a pre-existing one.
         */
retry:
	if (NWfsOpenResourceHandle(clientHandle, NW_READ|NW_WRITE, TRUE, FALSE,
				   &handleIsNew, diagnostic) != SUCCESS) {
		if (SNODE_HAS_FLOCK(fileNode))
			NWfsFlockStale(fileNode);
		goto done;
	}

        /*
         * Convert NWfs credentials to NUC credentials.
         */
        NWfiFsToNucCred(credentials, &nucCredentials);

	if (NWsiTruncateFile (&nucCredentials,
			fileNode->nodeVolume->spilVolumeHandle,
			clientHandle->resourceHandle, truncateOffset,
			diagnostic) != SUCCESS) {
		switch (*diagnostic) {
		/*
		 * The server is down, or going down. So therefore,
		 * get rid of our resource handle and fail the
		 * operation.
		 */
		case SPI_SERVER_UNAVAILABLE:
		case SPI_SERVER_DOWN:
			NWfsCloseResourceHandle(clientHandle);
			if (SNODE_HAS_FLOCK(fileNode))
				NWfsFlockStale(fileNode);
			 goto done;

		/*
		 * The server is up, but the user has lost either
		 * authentication or connection.
		 */
		case SPI_AUTHENTICATION_FAILURE:
		case SPI_BAD_CONNECTION:
			NWfsCloseResourceHandle(clientHandle);
			if (SNODE_HAS_FLOCK(fileNode))
				NWfsFlockStale(fileNode);
			else if (++retries <= NUCFS_RETRY_LIMIT)
				goto retry;
			goto done;

                /*
                 * We got some file or file system specific error.
                 */
                default:
                        NWfsReleaseResourceHandle(clientHandle);
                        goto done;
		}
	}

	retCode = SUCCESS;

	NWfsReleaseResourceHandle(clientHandle);
done:
	NWfsReleaseClientHandle(clientHandle);
	return NVLT_LEAVE(retCode);
}


/*
 * BEGIN_MANUAL_ENTRY( NWfsWriteBytesOnNode(3k), \
 *                     ./man/kernel/nucfs/nwfs/WriteBytesOnNode )
 * NAME
 *    NWfsWriteBytesOnNode - Writes bytes of data to a SPIL NetWare file node
 *                           object.
 *
 * SYNOPSIS
 *    ccode_t
 *    NWfsWriteBytesOnNode (credentials, fileNode, ioArgs, diagnostic)
 *    opaque_t        *credentials;
 *    opaque_t        *fileNode;     Object is opaque to caller. 
 *    NUCFS_IO_ARGS_T *ioArgs;
 *    enum  NUC_DIAG   *diagnostic;
 *
 * INPUT
 *    credentials                - Credentials of the UNIX client reading from
 *                                 a file.
 *    fileNode                   _ Server node object representing the file to
 *                                 read bytes of data from. 
 *    ioArgs->granuleOffset      - Byte offset from beginning of file's data
 *                                 space to start the read from.
 *    ioArgs->granulesRequested  - Number of bytes to read.
 *    ioArgs->memoryTypeFlag     - same as iobuf_t memoryType.
 *    ioArgs->ioBuffer           - Pointer to a contiguous block to read the
 *                                 bytes of data into.
 *
 * OUTPUT
 *    ioArgs->ioBuffer           - Is loaded with bytes of data.
 *    ioArgs->granulesReturned   - Number of bytes read.
 *    diagnostic                 - Set to one of the following if an error 
 *                                 occurs:
 *                                 NUCFS_ACCESS_DENIED
 *                                 NUCFS_INVALID_OFFSET
 *                                 NUCFS_INVALID_SIZE
 *
 * RETURN VALUES
 *    0                          - Successful completion.
 *    -1                         - Unsuccessful completion, diagnostic
 *                                 contains reason.
 *
 * DESCRIPTION
 *    The NWfsWriteBytesOnNode reads bytes of data from a SPIL NetWare file node
 *    object, and returns the number of bytes read.
 *
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
 * NOTES
 *    The only valid file type is NS_FILE.  NS_FIFO, NS_SPECIAL_CHARACTER,
 *    NS_SPECIAL_BLOCK, and NS_SYMBOLIC_LINK not supported currently.
 *
 *    Need to worry about locks.
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */

ccode_t
NWfsWriteBytesOnNode (
	NWFS_CRED_T		*credentials,
	NWFS_SERVER_NODE_T	*fileNode,
	NUCFS_IO_ARGS_T		*ioArgs,
	enum	NUC_DIAG	*diagnostic)
{
	NWFS_CLIENT_HANDLE_T	*fileClientHandle = NULL;
	NUC_IOBUF_T		*spilIoArgs = NULL;
	ccode_t			returnCode = FAILURE;
	int			newlyCreated;
	nwcred_t		nucCredentials;
	int			retries = 0;

	NVLT_ENTER (4);

	NVLT_ASSERT (SNODE_REASONABLE(fileNode));
	NVLT_ASSERT (fileNode->nodeType != NS_DIRECTORY);

        ioArgs->granulesReturned = 0;
        if (credentials == NULL) {
retry:
                /*
                 * We have no credentials (because we were called
                 * from the system). Therefore, surrogate on some
                 * valid writer for this file.
                 */
                if (NWfsAttachClientHandle(fileNode, &fileClientHandle,
                                NW_WRITE) != SUCCESS) {
                        *diagnostic = NUCFS_ACCESS_DENIED;
                        goto done;
                }
        } else {
                fileClientHandle = NWfsGetClientHandle(fileNode, credentials);
        }

        /*
         * Open a resource handle. However, do not authenticate the user
	 * at this point (for we may have pages locked and thus are not
	 * willing to wait for such a thing to happen).
	 */
	if (NWfsOpenResourceHandle(fileClientHandle, NW_READ|NW_WRITE, FALSE,
				   FALSE, &newlyCreated, diagnostic) !=
					SUCCESS) {
		NWfsReleaseClientHandle(fileClientHandle);
		/*
		 * Try another user.
		 */
		if (SNODE_HAS_FLOCK(fileNode))
			NWfsFlockStale(fileNode);
		else if (credentials == NULL && ++retries <= NUCFS_RETRY_LIMIT)
			goto retry;
		goto done;
	}

        /*
         * Convert NWfs credentials to NUC credentials.
         */
        NWfiFsToNucCred(&fileClientHandle->credentials, &nucCredentials);

	/*
	 * Set the SPIL IO args structure.
	 */
	spilIoArgs = kmem_alloc(sizeof(NUC_IOBUF_T), KM_SLEEP);
	spilIoArgs->buffer = ioArgs->ioBuffer;
	spilIoArgs->bufferLength = ioArgs->granulesRequested;
	spilIoArgs->memoryType = ioArgs->memoryTypeFlag;

	if (NWsiWriteFile (&nucCredentials,
			fileNode->nodeVolume->spilVolumeHandle,
			fileClientHandle->resourceHandle, ioArgs->granuleOffset,
			spilIoArgs, diagnostic) != SUCCESS) {
		switch (*diagnostic) {
                /*
                 * The server is up, but the user has lost either
                 * authentication or connection. If surrogating, then
                 * try another user.
                 */
		case SPI_AUTHENTICATION_FAILURE:
		case SPI_BAD_CONNECTION:
			NWfsCloseResourceHandle(fileClientHandle);
			NWfsReleaseClientHandle(fileClientHandle);
			if (SNODE_HAS_FLOCK(fileNode))
				NWfsFlockStale(fileNode);
			else if (credentials == NULL &&
			    ++retries <= NUCFS_RETRY_LIMIT)
				goto retry;
			goto done;
		/*
		 * The server is down, or going down, or we got
		 * some file or file system specific error.
		 */
		default:
			goto done1;
		}
	}

	/*
	 * Total bytes written.
	 */
	ioArgs->granulesReturned = spilIoArgs->bufferLength;
	returnCode = SUCCESS;

done1:
	NWfsReleaseResourceHandle(fileClientHandle);
	NWfsReleaseClientHandle(fileClientHandle);
done:
	if (spilIoArgs != NULL)
		kmem_free(spilIoArgs, sizeof(NUC_IOBUF_T));
	return (NVLT_LEAVE (returnCode));
}


ccode_t
NWfsSetFileLock(
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	uint32			accessFlags,
	NUCFS_LOCK_T		*lockStruct,
	enum NUC_DIAG		*diagnostic)
{
	NWFS_SERVER_NODE_T	*snode;
	ccode_t			result = SUCCESS;
	NUCFS_FLOCK_CACHE_T	*cache;
	int			newlyCreated;
	boolean_t		accessFlagsUpdated = B_FALSE;

	NVLT_ENTER (4);
	NVLT_ASSERT (CHANDLE_REASONABLE(clientHandle));
	snode = clientHandle->snode;
	NVLT_ASSERT (SNODE_REASONABLE(snode));
	NVLT_ASSERT (lockStruct->lockCommand == NWFS_SET_LOCK
			|| lockStruct->lockCommand == NWFS_SET_WAIT_LOCK);

	/*
	 * See if the process already holds the lock.  If so, we are done.
	 */
	cache = NWfsFlockCacheFind(&clientHandle->flockCacheChain, lockStruct,
				   B_TRUE, lockStruct->lockType);
	if (cache)
		goto done;

	/*
	 * On placing the first lock, try to get a resource handle that has
	 * write permissions, even if the user did not open with write
	 * permissions.  This anticipates that another, writeable file
	 * desciptor with the same credentials might later want a writeable
	 * resource handle.  If that were to happen, and we did not do this
	 * here, but successfully placed a lock, the request for the writeable
	 * resource handle would fail, because a resource handle for a client
	 * handle that has flocks cannot be closed.
	 */
	if (NWFI_LIST_EMPTY(&clientHandle->flockCacheChain) &&
	    !(accessFlags & NW_WRITE)) {
		accessFlagsUpdated = B_TRUE;
		accessFlags |= NW_WRITE;
	}

	/*
	 * Grab the handle now even if we don't go remote, so we can do the
	 * bookkeeping.
	 */
	if ((result = NWfsFileLockOpenHandle(clientHandle, accessFlags,
			TRUE, &newlyCreated, diagnostic)) != SUCCESS) {
		if (accessFlagsUpdated) {
			accessFlags &= ~NW_WRITE;
			result = NWfsFileLockOpenHandle(clientHandle,
							accessFlags,
							TRUE, &newlyCreated,
							diagnostic);
		}
		if (result != SUCCESS) {
			NWfsFlockStale(snode);
			goto done;
		}
	}

	/*
	 * See if another process with the same cred holds a lock of the same
	 * size.  We don't have to worry about  a conflicting lock, because we
	 * have been through fs_frlock.  If there is a matching lock within our
	 * cred, just hold it and leave, since all processes in our cred look
	 * like a single DOS machine to the server.
	 */
	cache = NWfsFlockCacheFind(&clientHandle->flockCacheChain, lockStruct,
				   B_FALSE, lockStruct->lockType);
	if (cache) {
		NVLT_ASSERT(lockStruct->lockType == NWFS_SHARED_LOCK);
		NWfsChandleFlockHold(clientHandle, lockStruct);
		goto releaseHandle;
	}

	/*
	 * Lock request is not redundant.  Issue it and cache it.
	 */
	result = NWfsDoFileLock(clientHandle, lockStruct, diagnostic);
	if (result == SUCCESS) {

		/*
		 * Did the process hold another lock of the same size?  If so,
		 * it cannot be the same type as the current lock, and must not
		 * be shared (from checks made above).  Give up the hold on the
		 * old lock. Place the new one unconditionally.
		 */
		cache = NWfsFlockCacheFind(&clientHandle->flockCacheChain,
					   lockStruct, B_TRUE, NWFS_NO_LOCK);
		if (cache) {
			NVLT_ASSERT(lockStruct->lockType != cache->cacheType);
			NVLT_ASSERT(cache->cachePidCount == 1);
			NWfsChandleCacheRelease(clientHandle, cache,
						lockStruct->lockPid);
		}
		NWfsChandleFlockHold(clientHandle, lockStruct);
	}
releaseHandle:
	NWfsFileLockReleaseHandle(clientHandle, result, *diagnostic);
done:
	return NVLT_LEAVE(result);
}


ccode_t
NWfsRemoveFileLock(
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	uint32			accessFlags,
	NUCFS_LOCK_T		*lockStruct,
	enum NUC_DIAG		*diagnostic)
{
	NWFS_SERVER_NODE_T	*snode;
	ccode_t			result = SUCCESS;
	NWFI_LIST_T		intersectingRequests, *chain;
	NUCFS_FLOCK_CACHE_T	*cache;
	uint16			saveCommand;
	int			newlyCreated;

	NVLT_ENTER(4);
	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
	snode = clientHandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(snode));
	NVLT_ASSERT(lockStruct->lockCommand == NWFS_REMOVE_LOCK);

	NWFI_LIST_INIT(&intersectingRequests);

	/*
	 * Find locks held by this process in the range.
	 */
	NWfsFlockCacheExtractRange(&clientHandle->flockCacheChain, lockStruct,
				   B_TRUE, NWFS_NO_LOCK, &intersectingRequests);
	if (NWFI_LIST_EMPTY(&intersectingRequests))
		goto done;

	if ((result = NWfsFileLockOpenHandle(clientHandle, accessFlags,
			TRUE, &newlyCreated, diagnostic)) != SUCCESS) {
		NWfsFlockStale(snode);
		goto splice;
	}

	/*
	 * In preparation for dropping locks, relock the portions of
	 * intersecting requests that extend beyond the range of the unlock
	 * request.
	 */
	result = NWfsFlockCacheClip(clientHandle, accessFlags, lockStruct,
				    &intersectingRequests, diagnostic);
	/*
	 * If we did not get rid of all overhangs, put the intersecting
	 * requests back into the cache.
	 */
	if (result != SUCCESS)
		goto releaseHandle;

	/*
	 * We got rid of overhangs.  For intersecting cache entries shared with
	 * other pids, just give up our holds.  For unshared ones, unlock on
	 * the server first, then release locally.
	 */
	chain = NWFI_NEXT_ELEMENT(&intersectingRequests);
	while (chain != &intersectingRequests) {
		cache = CHAIN_TO_CACHE(chain);
		chain = NWFI_NEXT_ELEMENT(chain);
		NVLT_ASSERT(NWfsFlockPidFind(cache, lockStruct->lockPid));
		if (cache->cachePidCount != 1) {
			NWfsChandleCacheRelease(clientHandle, cache,
						lockStruct->lockPid);
			continue;
		}
		saveCommand = cache->cacheCommand;
		cache->cacheCommand = NWFS_REMOVE_LOCK;
		result = NWfsDoFileLock(clientHandle, &cache->cacheState,
					diagnostic);
		cache->cacheCommand = saveCommand;
		if (result != SUCCESS) {
			if (NWFS_LOCK_BLOCKED(*diagnostic)) {
				*diagnostic = NUCFS_PROTOCOL_ERROR;
				cmn_err(CE_NOTE, "NWfsRemoveFileLock:  blocked"
					" diag %d", *diagnostic);
			}
			goto releaseHandle;
		}
		NWfsChandleCacheRelease(clientHandle, cache,
					lockStruct->lockPid);
	}
releaseHandle:
	NWfsFileLockReleaseHandle(clientHandle, result, *diagnostic);
splice:
	/*
	 * Restore the remainder of the list.
	 */
	if (!NWFI_LIST_EMPTY(&intersectingRequests))
		NWfsFlockCacheSplice(&clientHandle->flockCacheChain,
				     &intersectingRequests);
done:
	return NVLT_LEAVE(result);
}


ccode_t
NWfsDoFileLock (
	NWFS_CLIENT_HANDLE_T	*clientHandle,
	NUCFS_LOCK_T		*lockStruct,
	enum NUC_DIAG		*diagnostic)
{
	NWSI_LOCK_T		spilLockStruct;
	nwcred_t		nucCredentials;
	NWFS_SERVER_NODE_T	*serverNode;
	ccode_t			result;

	NVLT_ENTER(3);
	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
	serverNode = clientHandle->snode;
	NVLT_ASSERT(serverNode != NULL);
	NVLT_ASSERT(SNODE_REASONABLE(serverNode));
	NVLT_ASSERT(lockStruct->lockCommand == NWFS_SET_LOCK ||
		    lockStruct->lockCommand == NWFS_SET_WAIT_LOCK ||
		    lockStruct->lockCommand == NWFS_REMOVE_LOCK);
	NVLT_ASSERT(clientHandle->resourceHoldCount);
	NVLT_ASSERT(FLOCK_CACHE_ACTIVE(clientHandle));

	NWfiFsToNucCred(&clientHandle->credentials, &nucCredentials);
	spilLockStruct.offset = lockStruct->lockOffset;

	/*
	 * TODO:  Is the lost of information due to storing lockEnd important?
	 */
	if (lockStruct->lockEnd == NUCFS_LOCK_EOF)
		spilLockStruct.size = NUCFS_LOCK_EOF;
	else
		spilLockStruct.size =
			lockStruct->lockEnd - lockStruct->lockOffset;
	spilLockStruct.timeout = 0;
	switch (lockStruct->lockCommand) {
	case NWFS_SET_LOCK:
	case NWFS_SET_WAIT_LOCK:
		NVLT_ASSERT (lockStruct->lockType == NWFS_SHARED_LOCK
				|| lockStruct->lockType == NWFS_EXCLUSIVE_LOCK);
		switch (lockStruct->lockType) {
		case NWFS_SHARED_LOCK:
			spilLockStruct.mode = READ_LOCK;
			break;
		case NWFS_EXCLUSIVE_LOCK:
			spilLockStruct.mode = WRITE_LOCK;
			break;
		}
		result = NWsiSetFileLock(&nucCredentials,
				serverNode->nodeVolume->spilVolumeHandle,
				clientHandle->resourceHandle,
				&spilLockStruct, diagnostic);
		break;
	case NWFS_REMOVE_LOCK:
		result = NWsiRemoveFileLock (&nucCredentials, 
				serverNode->nodeVolume->spilVolumeHandle,
				clientHandle->resourceHandle, &spilLockStruct,
				diagnostic);
		break;
	}
	return (NVLT_LEAVE(result));
}

STATIC	ccode_t
NWfsFileLockOpenHandle(
	NWFS_CLIENT_HANDLE_T	*chandle,
	uint32			accessFlags,
	int			authenticateFlag,
	int			*newlyCreated,
	enum NUC_DIAG		*diag)
{
	ccode_t			result;
	int			canReplaceFlag;

	NVLT_ENTER(5);

	/*
	 * Inflate flock count to make sure the handle does not get ripped
	 * away.
	 */
	canReplaceFlag = !FLOCK_CACHE_LEN(chandle);
	FLOCK_CACHE_LEN_INFLATE(chandle);
	result = NWfsOpenResourceHandle(chandle, accessFlags, authenticateFlag,
					canReplaceFlag, newlyCreated, diag);
	if (result != SUCCESS)
		FLOCK_CACHE_LEN_DEFLATE(chandle);
	return NVLT_LEAVE(result);
}

STATIC	void
NWfsFileLockReleaseHandle(
	NWFS_CLIENT_HANDLE_T	*chandle,
	ccode_t			result,
	enum NUC_DIAG		diag)
{
	NVLT_ENTER(3);

	/*
	 * Undo inflated flock count from NWfsFileLockOpenHandle.
	 */
	FLOCK_CACHE_LEN_DEFLATE(chandle);
	if (result != SUCCESS) {
		switch (diag) {
		case SPI_SERVER_UNAVAILABLE:
		case SPI_SERVER_DOWN:
		case SPI_AUTHENTICATION_FAILURE:
		case SPI_BAD_CONNECTION:
			/*
			 * The server is down or going down, or the user has
			 * lost authentication or connection.  It would be
			 * possible to be more forgiving in the latter case, if
			 * there are no locks yet set, but that seems like too
			 * fine a point.
			 */
			NWfsCloseResourceHandle(chandle);
			NWfsFlockStale(chandle->snode);
			break;
		default:
			/*
			 * We got a semantic error.
			 */
			NWfsReleaseResourceHandle(chandle);
			break;
		}
	} else {
		NWfsReleaseResourceHandle(chandle);
	}
	NVLT_VLEAVE();
}
