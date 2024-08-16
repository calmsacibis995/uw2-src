/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/volops.c	1.25"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/volops.c,v 2.59.2.12 1995/02/03 03:31:29 stevbam Exp $"

/*
**  Netware Unix Client
**
**	MODULE:
**		volop.c -	The NetWare Client File System layer (NWfs)
**				volume operations.
**
**	ABSTRACT:
**		The volops.c contains the NetWare UNIX Client File System volume
**		oprations of the NetWare Client File System layer (NWfs).  See
**		NWfsVolOpsIntro(3k) for a complete description of the volume
**		operations.
*/ 

#include <util/types.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/sysmacros.h>

#include <net/nuc/nwctypes.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nucerror.h>
#include <net/tiuser.h>
#include <net/nuc/gtscommon.h>
#include <net/nuc/spilcommon.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <fs/nucfs/nucfs_tune.h>
#include <fs/nucfs/nwfschandle.h>
#include <fs/nucfs/nucfscommon.h>
#include <fs/nucfs/nucfsspace.h>
#include <fs/nucfs/nwfsvolume.h>
#include <fs/nucfs/nwfsnode.h>
#include <fs/nucfs/nucfsglob.h>
#include <net/nuc/requester.h>
#include <net/nuc/nuc_prototypes.h>
#include <fs/nucfs/nwfidata.h>
#include <fs/nucfs/nwfsops.h>
#include <fs/nucfs/nucfslk.h>
#include <fs/nucfs/nwficommon.h>
#include <mem/kmem.h>


/*
 * Define the tracing mask.
 */
#define NVLT_ModMask	NVLTM_fs

#if defined(DEBUG) || defined(DEBUG_TOOLS) || defined (DEBUG_TRACE)
#define	SCOPE	
#else
#define	SCOPE	static
#endif

extern	ccode_t			NWfsAllocateNode ();
extern	ccode_t			NWfsDeleteNodeDLList ();

#define	SERVER_VOLUME_FROM_SERVER_VOLUME_LIST(listptr) 		\
	((NWFS_SERVER_VOLUME_T *)(void *)((char *)(listptr) -		\
	offsetof(NWFS_SERVER_VOLUME_T, serverVolumeList)))

/*
 * The NWfsMountedVolumesList is protected by the global
 * NWfiMountVolumeLock.
 */
NWFI_LIST_T	NWfsMountedVolumesList; 


/*
 * BEGIN_MANUAL_ENTRY( NWfsVolOpsIntro(3k), \
 *                     ./man/kernel/nucfs/nwfs/VolOpsIntro )
 * NAME
 *     NWfsVolOpsIntro - Introduction to NetWare Client File System layer
 *                       (NWfs) volume operations.
 * SYNOPSIS
 *    #include <nucfscommon.h>
 *    #include <nwfsnode.h>
 *    #include <nwfsvolume.h>
 *    #include <nwfschandle.h>
 *
 * DESCRIPTION
 *    The NetWare UNIX Client File System (NUCfs) is broken into two layer: the
 *    Virtual File System Interface layer (NWfi) and the NetWare Client File
 *    System layer (NWfs).  The volops.c contains the NetWare Client File System
 *    layer (NWfs) volume operations.
 *
 *    The NWfs volume functions are responsible for managing the NUC File System
 *    server volume objects (NWFS_SERVER_VOLUME_T).  For each NetWare File 
 *    System mounted on a UNIX client Generic File Sysetm, There is a 
 *    NWFS_SERVER_VOLUME_T object representing the dependent NetWare File 
 *    System.  NetWare file systems are represented by server volumes on NetWare
 *    Servers.  The NWfs binds the server volumes via the mount facility which
 *    mounts the volume onto a directory mount point in the UNIX hierarchical
 *    file system tree. 
 *
 *    The NWFS_SERVER_VOLUME_T data structure has a pointer to a list of 
 *    subordinate server node objects (NWFS_SERVER_NODE_T) associated with the
 *    specific server volume object.  The server volume objects make SPIL calls
 *    to open a NetWare volume on a specific NetWare server.  Server volumes
 *    also have subordinate name space specific (UNIX, DOS, etc) operations list
 *    attached (NWFS_NODE_OPS_T).  When dealing with the DOS name space,  There
 *    is a subordinate active node DOS path tree.  
 *
 *    The NWFS_SERVER_VOLUME_T is paired with the UNIX mount structure and 
 *    associated Generic inode (INODE, VNODE, etc) that the NetWare 
 *    Server:Volume is being mounted on to form the focal point for a NetWare
 *    File System.  This represents the paradigm of a dependent file system in
 *    a UNIX Generic File System mapped to a NetWare Server Volume.
 *
 * SEE ALSO
 *    NWfsAllocateVolume(3k), NWfsGetLogicalBlockSize(3k), NWfsGetRootInode(3k),
 *    NWfsIsServerVolumeOnList(3k), NWfsMountVolume(3k), NWfsUnMountVolume(3k),
 *    and NWfsVolumeStatistics(3k).
 *
 * END_MANUAL_ENTRY
 */


/*
 * BEGIN_MANUAL_ENTRY( NWfsIsServerVolumeOnList(3k), \
 *                     ./man/kernel/nucfs/nwfs/IsServerVolumeOnList )
 * NAME
 *    NWfsIsServerVolumeOnList - Check to see if there is a server volume 
 *                               associated with the specified serverName and
 *                               volumeName on the active NetWare UNIX Client
 *                               File System server volume list.
 * SYNOPSIS
 *    ccode_t
 *    NWfsIsServerVolumeOnList(struct netbuf *serverAddress, char *volumeName)
 *
 * INPUT
 *    serverAddress  - address of netbuf structure containing server name.
 *    volumeName     - Name of the volume.
 * OUTPUT
 *    None.
 * RETURN VALUES
 *    SUCCESS        - if the volume is on the mounted volumes list.
 *    FAILURE        - otherwise.
 * Calling/Exit State:
 *    	It is caller's responsibility to assure that the NWfsMountedVolumesList
 *	is stable during the search. Usually, this is assured by holding the
 *	NWfiMountVolumeLock.
 * DESCRIPTION
 *    The NWfsIsServerVolumeOnList searches the NetWare UNIX Client File System
 *    active server volume list looking for a server volume object associated
 *    with both the specified server address and volume name.
 * SEE ALSO
 *    NWfsAllocateRootNode(3k).
 *
 * END_MANUAL_ENTRY
 */
SCOPE ccode_t
NWfsIsServerVolumeOnList(struct netbuf *serverAddress, char *volumeName)
{
	int i;
	NWFS_SERVER_VOLUME_T *volP;
	NWFI_LIST_T	*listP;

	NVLT_ENTER (2);

	for (listP = NWfsMountedVolumesList.flink;
		listP != &NWfsMountedVolumesList;
			listP = NWFI_NEXT_ELEMENT(listP)) {
		volP = SERVER_VOLUME_FROM_SERVER_VOLUME_LIST(listP);

		/*
		 * Got a server volume, is it the one we want?
		 */
		if (serverAddress->len == volP->address->len) {
			for (i = 0; i < serverAddress->len; i++) {
				if (serverAddress->buf[i] != 
						volP->address->buf[i])
					break;
			}
			if ((i == serverAddress->len) &&
			    (strcmp(volumeName, volP->volumeName) == 0)) {
				/*
				 * since both the serverAddress buffers 
				 * and  the volumeNames match, we have a hit.
				 */
				return (NVLT_LEAVE (SUCCESS));
			}
		}
	}
	/*
	 * no match was not found.
	 */
	return (NVLT_LEAVE (FAILURE));
}

/*
 * BEGIN_MANUAL_ENTRY( NWfsGetRootNode(3k), \
 *                     ./man/kernel/nucfs/nwfs/GetRootNode )
 * NAME
 *    NWfsGetRootNode - Gets the root server node corresponding to a
 *			netware Server volume. The root server node is
 *			returned held.
 *
 * SYNOPSIS
 *    void_t
 *    NWfsGetRootNode(NWFS_SERVER_VOLUME_T *serverVolume, 
 *			NWFS_SERVER_NODE_T **rootNode)
 * INPUT
 *    serverVolume - Server volume containing the root inode. 
 * OUTPUT
 *    rootNode    - Root generic inode.
 * RETURN VALUES
 *    None.
 * DESCRIPTION
 *    The NWfsGetRootNode return the root generic inode of the specified
 *    serverVolume.
 * NOTES
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
void_t
NWfsGetRootNode (NWFS_SERVER_VOLUME_T *serverVolume, 
		 NWFS_SERVER_NODE_T **rootNode)
{
	NVLT_ENTER(2);

	NVLT_ASSERT (serverVolume != NULL);
	*rootNode = serverVolume->rootNode;
	NVLT_ASSERT (SNODE_REASONABLE(*rootNode));
	NVLT_ASSERT((*rootNode)->nodeType == NS_ROOT);
	SNODE_HOLD((*rootNode));

	NVLT_LEAVE(SUCCESS);
	return;
}

/*
 * BEGIN_MANUAL_ENTRY( NWfsMountVolume(3k), \
 *                     ./man/kernel/nucfs/nwfs/MountRootNode )
 * NAME
 *    NWfsMountVolume - Mounts the root server node object of the specified
 *                      serverVolume onto the UNIX Generic File System.
 *
 * SYNOPSIS
 *    ccode_t
 *    NWfsMountVolume (NWFS_CRED_T *credentials, 
 *		struct netbuf *serverAddress, char *volumeName, 
 *		uint32 volumeFlags, NWFS_SERVER_VOLUME_T **serverVolume,
 *              NWFS_SERVER_NODE_T **rootNode, 
 *		NUCFS_VOLUME_STATS_T *volumeStats, enum NUC_DIAG diagnostic)
 * INPUT
 *    credentials                    - Credentials of the UNIX client mounting
 *                                     the specified serverVolume.
 *    serverAddress                  - Address of the netbuf structure 
 *						containing server information.
 *    volumeName                     - Name of the volume to allocate root node
 *                                     for.
 *    volumeFlags                    - Set to the following if volume is to be 
 *                                     allocated for reading only:
 *                                     NUCFS_VOLUME_READ_ONLY
 * OUTPUT
 *    serverVolume                   - Newly allocated server volume object.
 *    rootNode                       - Newly allocated server root node of the 
 *                                     server volume.
 *
 *    volumeStats->logicalBlockSize  - Logical block size of the newly mounted
 *                                     serverVolume.
 *
 *    volumeStats->volumeFlags       - Newly mounted serverVolume's flags.
 *                                     NUCFS_VOLUME_READ_ONLY
 *                                     NWFS_UNIX_NETWARE_NAME_SPACE
 *                                     NWFS_UNIX_NFS_NAME_SPACE
 *                                     NWFS_DOS_NAME_SPACE
 *    diagnostic                     - Set to one of the following if an error
 *                                     occurs:
 *                                     NUCFS_ALLOC_MEM_FAILED
 *                                     NUCFS_LLIST_OPERATION_FAILED
 * RETURN VALUES
 *    0                              - Successful completion.
 *    -1                             - Unsuccessful completion, diagnostic
 *                                     contains reason.
 * DESCRIPTION
 *    The NWfsMountVolume allocates and populates a server volume object.
 *    The root server node object associated with the newly allocated server
 *    volume object is also allocated.  The NWfsMountVolume mounts the newly
 *    allocated root server node object onto the UNIX Generic File System.
 * NOTES
 *    The NWfsMountVolume must be called to mount the root server node
 *    defined by server:volume before it can be accessed.
 * SEE ALSO
 *    NWfsMountVolume(3k), and NWfsUnMountVolume(3k).
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWfsMountVolume (  NWFS_CRED_T *credentials, 
		   struct netbuf *serverAddress,
		   char *volumeName,
		   uint32 volumeFlags,
		   NWFS_SERVER_VOLUME_T **serverVolume,
		   NWFS_SERVER_NODE_T **rootNode,
		   NUCFS_VOLUME_STATS_T *volumeStats,
		   struct NWfiVolumeFlushData *volFlushData,
		   enum NUC_DIAG *diagnostic)
{
	enum	NUC_DIAG   	diag;
	char			rootName[MAX_VOLUME_NAME_LENGTH+2];
	uint32			i;
	SPI_SERVICE_T		*gService;
	nwcred_t		nucCredentials;

	NVLT_ENTER (9);

	/*
	 * Check whether someone else already had the volume 
	 * mounted. The only reason for precluding multiple
	 * mounts of the same volume is that file/record locking
	 * code needs this constraint when performing deadlock
	 * detection on a client machine. 
	 */
	if (NWfsIsServerVolumeOnList(serverAddress, 
			volumeName) == SUCCESS) {
		*diagnostic = NUCFS_VOLUME_BUSY;
		return(NVLT_LEAVE(FAILURE));
	}
	NVLT_ASSERT(serverAddress->len <= serverAddress->maxlen);

	NWfiFsToNucCred(credentials, &nucCredentials);	
	nucCredentials.flags = NWC_OPEN_PUBLIC;

	if (NWslGetService(serverAddress, &gService) != SUCCESS) {
		comargs_t comargs;

		comargs.address = serverAddress;
		comargs.credential = &nucCredentials;
	
		if (NWsiCreateService( &comargs, SPROTO_NCP, 
				NOVELL_IPX, 0) != SUCCESS)  {
			*diagnostic = SPI_GENERAL_FAILURE;
			return (NVLT_LEAVE (FAILURE));
	    	}
		/* TBD:
		 * should we assert something about 
		 * comargs.connectionReference?
		 */
	}

	/*
	 * TBD: Not clear whether serverAddress->len could have changed.
	 */
	if (serverAddress->len > MAX_ADDRESS_SIZE) {
		*diagnostic = NUCFS_ALLOC_MEM_FAILED;
		return (NVLT_LEAVE (FAILURE));
	}

	/*
	 * Allocate memory for the server volume object to be allocated.
	 */
	*serverVolume = kmem_zalloc(sizeof (NWFS_SERVER_VOLUME_T), KM_SLEEP);
	/*
	 * Set the volumeName for newly allocated server volume object.
	 */
	(*serverVolume)->volumeName = kmem_alloc(strlen (volumeName) + 1, 
						KM_SLEEP);
	strcpy ((*serverVolume)->volumeName, volumeName);

	(*serverVolume)->address = kmem_zalloc(sizeof(struct netbuf), 
					KM_SLEEP);
	(*serverVolume)->address->buf = kmem_zalloc(MAX_ADDRESS_SIZE, 
					KM_SLEEP);

	bcopy(serverAddress->buf, (*serverVolume)->address->buf, 
		serverAddress->len);
	(*serverVolume)->address->len = serverAddress->len;
	(*serverVolume)->address->maxlen = MAX_ADDRESS_SIZE;

	/*
	 * Set the volume server flags.
	 */
	(*serverVolume)->volumeFlags = volumeFlags;

	/*
	 * Initialize the lists in the NWserverVolume structure:
	 *	serverVolumeList
	 * 	timedNodeList
	 *	activeNodeList
	 *	cacheNodeList
	 *	the array of idHashTable lists.
	 *
	 * The mutexes protecting these lists are not required at this
	 * point because the serverVolume is not yet publicly referenceable.
	 */
	NWFI_LIST_INIT(&((*serverVolume)->serverVolumeList));

	NWFI_LIST_INIT(&((*serverVolume)->timedNodeList));
	NWFI_LIST_INIT(&((*serverVolume)->activeNodeList));
	NWFI_LIST_INIT(&((*serverVolume)->cacheNodeList));

	(*serverVolume)->activeOrTimedCount = 0;
	(*serverVolume)->cacheCount = 0;
	(*serverVolume)->staleCount = 0;

	for (i=0; i< NUCFS_NODE_LIST_BUCKETS; i++) {
		NWFI_LIST_INIT(&((*serverVolume)->idHashTable[i]));
	}
	/*
	 * Initialize the serverVolume's asyncIoList.
	 * TBD: the deamons. 
	 */
	NWFI_BUF_INIT(&((*serverVolume)->asyncIoList));

	(*serverVolume)->asyncIoList.b_flags = 0;
	(*serverVolume)->idHashStamp = 0;
	(*serverVolume)->volFlushData = volFlushData;

	/*
	 * These fields remain to be initialized: logicalBlockSize,
	 *	rootNode, spilVolumeHandle.
	 *
	 * Open the NetWare volume with the specified volumeName on the NetWare
	 * server with the specified serverAddress.
	 *
	 * NOTE:
	 *    NWsiOpenVolume also creates (if one doesn't already exist) a
	 *    directory in the root directory of the volume (called nuc trash)
	 *    for keeping files or directories that are to be delay deleted.
	 *
	 *    NetWare does not allow deletion of files or directories that are
	 *    open by any other user.  If the volume to be mounted uses the UNIX
	 *    name space, the NetWare server (NUC NLM) manages the delay 
	 *    deletion of files or directories.  However if the volume is using
	 *    the DOS name space, the NUC client has to manage the delay
	 *    deletion concept.  if using DOS name space, we can only delay the
	 *    deletion of files or directories that are opened by the user
	 *    issuing the delete. 
	 */

	if (NWsiOpenVolume (
			&nucCredentials, 
			serverAddress, 
			volumeName, 	
			&((*serverVolume)->spilVolumeHandle), 
			&((*serverVolume)->nucNlmMode),
			&((*serverVolume)->logicalBlockSize), 
			diagnostic) != SUCCESS) {
		/*
		 * Give up and return.
		 */
		kmem_free((*serverVolume)->address->buf, MAX_ADDRESS_SIZE);
		kmem_free((*serverVolume)->address, sizeof(struct netbuf));
		kmem_free ((*serverVolume)->volumeName, 
				     strlen (volumeName) + 1);
		kmem_free (*serverVolume, sizeof (NWFS_SERVER_VOLUME_T));
		return (NVLT_LEAVE (FAILURE));
	}

	if ((*serverVolume)->logicalBlockSize == 0) {
		/*
		 *+ The nucfs volume with the name indicated is being
		 *+ mounted even though the logical block size reported
		 *+ by the server is zero.  Nucfs will substitute the
		 *+ value 512 and continue with the mount attempt.
		 *+ This problem (in and of itself) is not serious, for
		 *+ it only affects volume space statistic reporting. However,
		 *+ it may indicate more serious problems with the server.
		 */
		cmn_err(CE_WARN, "NWfsMountVolume: volume %s has 0 "
				 "logical block size.\n\tUsing 512.\n",
				 volumeName);
		(*serverVolume)->logicalBlockSize = 512;
	}
		
	(*serverVolume)->volumeFlags &= ~(NWFS_UNIX_NFS_NAME_SPACE | 
			NWFS_UNIX_NETWARE_NAME_SPACE| NWFS_DOS_NAME_SPACE);

	/*
	 * Allocate a root server node object for the new server volume object.
	 * The rootNode's nodeName is set to the specified volumeName followed
	 * by a ":".
	 */
	strcpy (rootName, volumeName);
	strcpy (&rootName[strlen(volumeName)], ":");
	volumeStats->logicalBlockSize = (*serverVolume)->logicalBlockSize;
	volumeStats->volumeFlags = (*serverVolume)->volumeFlags;

	/*
	 * TODO:
	 *	Provide a LookUp/Create interface that correctly
	 *	handles the creation of a root serverNode.
	 *
	 * Assumptions below:
	 *	- The lookup/create function for root nodes will
	 *	know to create a client handle and stow the nucCredentials
	 *	in it. 
	 *	- Also, it will talk to the server, in order to
	 *	find the name, and bring up the authenticator as necessary.
	 *	- And it knows to attach the allocated rootNode to the
	 *	serverVolume's activeNodeList or cacheNodeList, as 
	 *	appropriate.
	 *	- Comes back with the rootNode held. An NWfiBindVnodeToSnode
	 *	will happen in our caller (NWfiMount), which will establish
	 *	a vnode with the necessary hold, along with the appropriate
	 *	hard hold on the rootNode.
	 */
	if (NWfsCreateRootNode(
			credentials, 
			(*serverVolume), 
			rootNode, 
			diagnostic) != SUCCESS) {
		NWsiCloseVolume (&nucCredentials, 
			(*serverVolume)->spilVolumeHandle, &diag);
		kmem_free((*serverVolume)->address->buf, MAX_ADDRESS_SIZE);
		kmem_free((*serverVolume)->address, sizeof(struct netbuf));
		kmem_free ((*serverVolume)->volumeName, 
				strlen (volumeName) + 1);
		kmem_free (*serverVolume, sizeof (NWFS_SERVER_VOLUME_T));
		return (NVLT_LEAVE (FAILURE));
	}
	NUCFS_LIST_LOCK();

	(*serverVolume)->rootNode = (*rootNode);

	/* 
	 * Add the new server volume to the list of mounted volumes. 
	 */
	NWfiInsque(&((*serverVolume)->serverVolumeList), 
			&NWfsMountedVolumesList);
	bcopy(&nucCredentials, &(*serverVolume)->privateCredentials,
			sizeof(nwcred_t));
	NUCFS_LIST_UNLOCK();
	NVLT_ASSERT((*serverVolume)->rootNode->nodeType == NS_ROOT);
	return (NVLT_LEAVE (SUCCESS));
}

/*
 * ccode_t 
 * NWfsPrepareToUnMountVolume (NWFS_SERVER_NODE_T *rootNode, 
 *		enum NUC_DIAG *diagnostic)
 * INPUT
 *    rootNode    - Root server node of the server volume to be unmounted.
 * OUTPUT
 *    diagnostic - Set to one of the following if an error occurs:
 * RETURN VALUES
 *    0          - Successful completion.
 *    -1         - Unsuccessful completion, diagnostic contains reason.
 * DESCRIPTION
 *    The NWfsUnMountVolume unmounts the server volume object associated with 
 *    the specified rootNode from the UNIX Generic File System.  The specified
 *    root server node is also release.
 * NOTES
 *    When NWfsUnMountVolume is called the only active node on the server 
 *    volume of the specified rootNode is the root server node.
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWfsPrepareToUnMountVolume (NWFS_SERVER_NODE_T *rootNode, 
			enum NUC_DIAG *diagnostic)
{
	NWFS_SERVER_VOLUME_T	*volP = NULL; 
	NWFS_SERVER_NODE_T	*srvrNode;
	NWFS_SERVER_NODE_T	*marker;
	int i;

	NVLT_ENTER (2);

	NVLT_ASSERT (SNODE_REASONABLE(rootNode));
	volP = rootNode->nodeVolume;
	NVLT_ASSERT(volP != NULL);
	NVLT_ASSERT(volP->rootNode == rootNode);

	NUCFS_LIST_LOCK();
	if ((volP->staleCount + volP->activeOrTimedCount) > 1) {
		*diagnostic = NUCFS_VOLUME_BUSY;
		NUCFS_LIST_UNLOCK();
		return(NVLT_LEAVE(FAILURE));
	} 
	NUCFS_LIST_UNLOCK();

	/*
	 * First, release as many softholds from cached information
	 * as possible.
	 */
	SNODE_CREATE_MARKER(&marker);
	for (i = 0; i < NUCFS_NODE_LIST_BUCKETS; ++i) {
		NWfsForEachSnode(
			&volP->idHashTable[i],
			NWfsReleaseStaleCachingHolds,
			marker);
	}
	SNODE_DESTROY_MARKER(marker);

	NUCFS_LIST_LOCK();
	/*
	 * Make each cacheNode stale, and then destroy it. Since we have 
	 * to drop the  nucfs_list_lock each time through the loop, we should
	 * verify that no new activeNodes or timedNodes got generated,
	 * before proceeding to next iteration.
	 */
	while (volP->cacheCount > 0) {
		NVLT_ASSERT(!NWFI_LIST_EMPTY(&(volP->cacheNodeList)));
		srvrNode = volumeChainToSnode(volP->cacheNodeList.flink);
		NVLT_ASSERT(srvrNode != 
			volumeChainToSnode(&volP->cacheNodeList));
		while (srvrNode->nodeState == SNODE_MARKER) {
			srvrNode = volumeChainToSnode(NWFI_NEXT_ELEMENT(
				snodeToVolumeChain(srvrNode)));
			NVLT_ASSERT(srvrNode !=
				volumeChainToSnode(&volP->cacheNodeList));
		}
		SNODE_DESTROY_IDENTITY(srvrNode, volP, cacheCount);
		--nwfsCacheNodeCount;
		NUCFS_LIST_UNLOCK();
		/* 
		 * When the identity was destroyed, we inherited the
		 * softHold on the srvrNode. So we can safely call
		 * NWfsDestroyNode.
		 */
		NWfsDestroyNode(srvrNode); 
		NUCFS_LIST_LOCK();
		/*
		 * Recheck the other counts before proceeding further.
		 */
		if ((volP->activeOrTimedCount + volP->staleCount) > 1) {
			NUCFS_LIST_UNLOCK();
			*diagnostic = NUCFS_VOLUME_BUSY;
			return (NVLT_LEAVE(FAILURE));
		}
	}
#if defined(DEBUG) || defined(DEBUG_TRACE)
	NVLT_ASSERT(NWFI_LIST_EMPTY(&(volP->cacheNodeList)));
	{
		int i, count = 0;
		NWFI_LIST_T *listp;

		for (i=0; i< NUCFS_NODE_LIST_BUCKETS; i++) {
			listp = &(volP->idHashTable[i]);
			if (!NWFI_LIST_EMPTY(listp)) {
				NVLT_ASSERT(idChainToSnode(NWFI_NEXT_ELEMENT(
					listp)) == rootNode);
				NVLT_ASSERT(idChainToSnode(NWFI_NEXT_ELEMENT(
					listp)) == rootNode);
				++count;
			}
		}
		NVLT_ASSERT(volP->activeOrTimedCount + volP->staleCount == 1);
		NVLT_ASSERT(volP->activeOrTimedCount == 1 ? count == 1 : count == 0);
	}
	NVLT_ASSERT(NWFI_BUF_EMPTY(&(volP->asyncIoList)));
#endif /* DEBUG || DEBUG_TRACE */
	NUCFS_LIST_UNLOCK();
	return (NVLT_LEAVE(SUCCESS));
}

ccode_t
NWfsDoUnMountVolume(NWFS_SERVER_NODE_T *rootNode)
{
	NWFS_SERVER_VOLUME_T *volP;

	NVLT_ENTER(1);

	/*
	 * Eventhough we are not holding the nucfs_list_lock, we
	 * can safely dereference it to get the volume pointer.
	 */
	volP = rootNode->nodeVolume;
	NVLT_ASSERT(volP->rootNode == rootNode);
	NUCFS_LIST_LOCK();

	/*
	 * Note that the rootVnode and the root server node are
	 * yet to be released; but there are no stray contexts
	 * holding references on the rootVnode. 
	 */
	NWfiRemque(&(volP->serverVolumeList));

	/*
	 * We will be leaving this function with a softhold on the
	 * root server node, so that we can release the rootVnode in
	 * the NWfi layer, and then come back for releasing the
	 * root server node.
	 */
	SNODE_SOFT_HOLD(rootNode);
	NUCFS_LIST_UNLOCK();
	return (NVLT_LEAVE(SUCCESS));
}

void
NWfsDestroyVolumeAndRootNode(NWFS_CRED_T *nwfsCredentials,
			     NWFS_SERVER_NODE_T *rootNode)
{
	NWFS_SERVER_VOLUME_T	*volP;
	enum NUC_DIAG		diagnostic = SUCCESS;
	nwcred_t		nucCredentials;

	NVLT_ENTER(2);

	/*
	 * save the volume pointer, before destroying the root node.
	 */
	volP = rootNode->nodeVolume;
	NVLT_ASSERT(volP->rootNode == rootNode); /* sanity test */
	/*
	 * destroy  the root server node.
	 */
	NWfsDestroyNode(rootNode);

	/*
	 * Close the SPIL NetWare Volume object. Even if the operation
	 * fails, we will go ahead and unmount anyway.
	 */
	NWfiFsToNucCred(nwfsCredentials, &nucCredentials);
	NWsiCloseVolume (&nucCredentials, volP->spilVolumeHandle, 
			 &diagnostic);

	/*	
	 * Release the data structure/s associated with volume.
	 * At this point, we should be the only one to know anything
	 * about the volume data structures.
	 */
	kmem_free (volP->address->buf, MAX_ADDRESS_SIZE);
	kmem_free (volP->address, sizeof(struct netbuf));
	kmem_free (volP->volumeName,
			strlen (volP->volumeName) + 1);
	/*
	 * Delete the volume.
	 */
	kmem_free (volP, sizeof (NWFS_SERVER_VOLUME_T));
	NVLT_VLEAVE();
	return;
}

/*
 * BEGIN_MANUAL_ENTRY( NWfsVolumeStatistics(3k), \
 *                     ./man/kernel/nucfs/nwfs/VolumeStatistics )
 * NAME
 *    NWfsVolumeStatistics - Returns the server volume statictics associated
 *                           with the specified serverNode object.
 *
 * SYNOPSIS
 *    ccode_t NWfsVolumeStatistics (NWFS_SERVER_NODE_T *serverNode, 
 *		NUCFS_VOLUME_STATS_T *volumeStats, NWFS_CRED_T *credentials,
 *		enum NUC_DIAG diagnostic)
 * INPUT
 *    serverNode                    - A server node on the server volume
 *                                    question.
 *    credentials		    - The credentials of the caller.
 *
 * OUTPUT
 *    volumeStats->logicalBlockSize - Server volume logical block size.
 *    volumeStats->volumeFlags      - Is set to an inclusive 'OR' of the
 *                                    following:
 *                                    NUCFS_VOLUME_READ_ONLY
 *                                    NWFS_UNIX_NETWARE_NAME_SPACE
 *                                    NWFS_UNIX_NFS_NAME_SPACE
 *                                    NWFS_DOS_NAME_SPACE
 *    volumeStats->totalBlocks      - Maximum number of blocks associated with
 *                                    the server volume.
 *    volumeStats->totalFreeBlocks  - Number of unused blocks on the server
 *                                    volume.
 *    volumeStats->totalNodes       - Total number of server node objects, 
 *                                    server volume cam accommodate.
 *    volumeStats->totalFreeNodes   - Number of unused server node objects.
 *    volumeStats->volumeName       - Server volume name.
 *    volumeStats->serverName       - Server name, volume is on.
 *    diagnostic                    - Set to one of the following if an error
 *                                    occurs:
 * RETURN VALUES
 *    0                            - Successful completion.
 *    -1                           - Unsuccessful completion, diagnostic
 *                                   contains reason.
 * DESCRIPTION
 *    The NWfsVolumeStatistics loads the NetWare UNIX Client File System volume
 *    statistics sturcture with the server volume information.
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWfsVolumeStatistics ( NWFS_SERVER_NODE_T *serverNode, 
	NUCFS_VOLUME_STATS_T *volumeStats, NWFS_CRED_T *credentials,
	enum NUC_DIAG *diagnostic)
{
	NWSI_VOLUME_STATS_T	spilVolumeStats;
	nwcred_t		nucCredentials;

	NVLT_ENTER (4);

	NVLT_ASSERT (SNODE_REASONABLE(serverNode));

	/*
	 * convert callers credentials to NUC credentials.
	 */
	NWfiFsToNucCred(credentials, &nucCredentials);

	/*
	 * Get the SPIL NetWare volume statistics.
	 *
	 * We try both the caller's credentials and the mounter's
	 * credentials.
	 */
	if ((NWsiVolumeStats (
			&(serverNode->nodeVolume->privateCredentials),
			serverNode->nodeVolume->spilVolumeHandle,
			&spilVolumeStats, diagnostic) != SUCCESS) &&
	    (NWsiVolumeStats (
			&nucCredentials,
			serverNode->nodeVolume->spilVolumeHandle,
			&spilVolumeStats, diagnostic) != SUCCESS)) {
		return (NVLT_LEAVE (FAILURE));
	}

	/*
	 * Set the NUCFS volume statistics.
	 */
	NUCFS_LIST_LOCK();
	volumeStats->logicalBlockSize = 
			serverNode->nodeVolume->logicalBlockSize;
	volumeStats->volumeFlags = serverNode->nodeVolume->volumeFlags;
	volumeStats->totalBlocks = spilVolumeStats.totalBlocks;
	volumeStats->totalFreeBlocks = spilVolumeStats.totalFreeBlocks;
	volumeStats->totalNodes = spilVolumeStats.totalNodes;
	volumeStats->totalFreeNodes = spilVolumeStats.totalFreeNodes;
	strcpy (volumeStats->volumeName,
			serverNode->nodeVolume->volumeName);
	volumeStats->serverAddress.len = serverNode->nodeVolume->address->len;
	bcopy(serverNode->nodeVolume->address->buf,
			volumeStats->serverAddress.buf, 
			serverNode->nodeVolume->address->len);
	NUCFS_LIST_UNLOCK();
	return (NVLT_LEAVE (SUCCESS));
}


void
NWfsVolumeSync(NWFS_SERVER_VOLUME_T *volume)
{
	void NWfsSyncSnode(NWFS_SERVER_NODE_T *snode, void *args);
	int i;

	NVLT_ENTER(1);

	for (i = 0; i < NUCFS_NODE_LIST_BUCKETS; i++) {
		NWfsForEachSnode(&volume->idHashTable[i], NWfsSyncSnode, NULL);
	}

	NVLT_VLEAVE();
}

/* ARGSUSED */
STATIC void
NWfsSyncSnode(NWFS_SERVER_NODE_T *snode, void *arg)
{
	enum NUC_DIAG diagnostic;
	int error;

	NVLT_ENTER(2);

	NVLT_ASSERT(SNODE_REASONABLE(snode));

	/*
	 * Only real files need to be synced.
	 */
	if (snode->nodeType != NS_FILE)
		return;

	/*
	 * Our caller does not possess credentials for this snode.
	 * So we tell NWfiSyncWithServer to surrogate upon somebody
	 * who may have the file open (by passing in credentials of NULL).
	 *
	 * We don't wish to attempt to sync an inactive snode because such a
	 * sync attempt could generate an invalidation should no
	 * credentials be available with which to contact the server. Such
	 * invalidating is functionally harmless, but it has the negative
	 * performance side effect of throwing away the pages of cached
	 * snodes. Therefore, we only sync active nodes.
	 *
	 * XXX: Since the caller is only holding a soft hold on the
	 *	snode, the snode can become stale, leading to a
	 *	possible invalidation. But, even an active node can
	 *	lack the necesary credentials is the file is not actually
	 *	in use by a user (opened or mapped).
	 */
	SNODE_LOCK(snode);
	switch (snode->nodeState) {
	case SNODE_TIMED:
	case SNODE_ACTIVE:
		SNODE_UNLOCK(snode);
		SNODE_WR_LOCK(snode);
		error = NWfiSyncWithServer(snode, FALSE, NULL, &diagnostic);
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
		SNODE_RW_UNLOCK(snode);
		break;
	default:
		SNODE_UNLOCK(snode);
		break;
	}

	NVLT_VLEAVE();
}
