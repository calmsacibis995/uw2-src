/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucam/amfs_nodeops.c	1.12"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucam/amfs_nodeops.c,v 1.14.2.1 1994/12/12 01:08:11 stevbam Exp $"

/*
**  NetWare Unix Client Auto Mounter File System
**
**	MODULE:
**		amfs_nodeops.c -	NetWare UNIX Client Auto Mounter File
**					System node (AMfs) operations.
**
**	ABSTRACT:
**		The amfs_nodeops.c contains the AMfs node functions of the
**		NUCAM File System.
**
**		The following nodeops operations are contained in this module:
**			AMfsCloseNode ()
**			AMfsLookUpNode ()
**			AMfsOpenNode ()
**			AMfsReadAmfsNodeEntries ()
**
*/ 

#include <util/types.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ipl.h>
#include <util/param.h>
#include <net/nuc/nwctypes.h>
#include <net/tiuser.h>
#include <net/nuc/nwmp.h>
#include <net/nuc/nuctool.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <fs/fs_hier.h>
#include <fs/mount.h>
#include <fs/nucam/nucam_common.h>
#include <fs/nucam/amfs_node.h>
#include <fs/nucam/amfs_ops.h>
#include <fs/nucam/nucam_glob.h>
#include <fs/nucfs/nwficommon.h>
#include <io/NWam/nwam.h>
#include <mem/kmem.h>


/*
 * Define the tracing mask.
 */
#define NVLT_ModMask	NVLTM_am

void	AMfsPrintNode(AMFS_NODE_T *);



/*
 * BEGIN_MANUAL_ENTRY( AMfsNodeOpsIntro(3k), \
 *                     ./man/kernel/nucam/amfs/NodeOpsIntro)
 * NAME
 *    AMfsNodeOpsIntro - Introduction to the node operations of the NetWare
 *                       UNIX Client Auto Mounter File System AMfs layer.
 *
 * SYNOPSIS
 *    #include <nucam_common.h>
 *    #include <amfs_node.h>
 *    #include <amfs_ops.h>
 *
 * DESCRIPTION
 *    The NetWare UNIX Client Auto Mounter (NUCAM) File System is responsible
 *    to automate the mounting of volumes on a NetWare Servers into a directory
 *    on a NUCAM File System.  The NUCAM File System is broken into two layers:
 *    the Auto Mounter Interface (AMfi) layer and the Auto Mounter File System
 *    layer (AMfs).  The amfs_nodeops.c contains the Auto Mounter File System
 *    layer (AMfs) node operations.
 *
 *    The NUCAM File System is installed as a dependent file system which plugs
 *    into the Virtual File System interface of the UNIX Generic File System 
 *    process structure.  The AMfs layer binds the NetWare UNIX Client Auto
 *    Mounter File System, into the UNIX Generic File System according to UNIX
 *    semantics.  
 *
 *
 *    The AMfs node operations are responsible for managing the NUCAM File
 *    System node objects and their user references.  The node object
 *    of the AMfs (AMFS_NODE_T) can have one of the following types: 
 *        AM_ROOT   - Root of the NUCAM File System.  There is only one AMfs
 *                    node of type AM_ROOT per NUCAM.  It is bound to the mount
 *                    structure of the Generic File System to form the mount
 *                    point into the NUCAM File System.  Only one NUCAM File
 *                    System can be mounted to manage the the automation of the
 *                    NUCFS File System mounts.  Therefore we can only have one
 *                    AM_ROOT AMfs node on a workstation.
 *        AM_SERVER - A server AMfs node.  Each server AMfs node under the root
 *                    AMfs node represents a NetWare server on the NetWare LAN.
 *                    Each server AMfs node is paired with a Generic File System
 *                    Node (VNODE, INODE, etc) to form a directory which
 *                    represent the NetWare server.
 *        AM_VOLUME - A volume AMfs node.  Each volume AMfs node under a server
 *                    AMfs node represents a NetWare volume on that NetWare
 *                    server.  Each volume AMfs node is also paired with a
 *                    Generic File System node (VNODE, INODE, etc) to form a
 *                    a directory which represents the NetWare volume.
 *
 *    Each of the above nodes may have multiple UNIX processes operating on them
 *    simultaneously.  There is only one AMfs node for a NetWare server or
 *    volume which is being accessed on a UNIX client system.  
 *
 * SEE ALSO
 *    AMfsAllocateNode(3k), AMfsLookUpNode(3k), AMfsOpenNode(3k),
 *    and AMfsReadAmfsNodeEntries(3k).
 *
 * END_MANUAL_ENTRY
 */


/*
 * BEGIN_MANUAL_ENTRY( AMfsLookUpNode(3k), ./man/kernel/nucam/amfs/LookUpNode )
 * NAME
 *    AMfsLookUpNode - Searches the childList of the specified parentNode,
 *                     looking for an AMfs node with the specified nodeName and 
 *                     parentNode.
 *
 * SYNOPSIS
 *    ccode_t
 *    AMfsLookUpNode (credentials, parentNode, nodeName, foundNode,
 *                    supportedVolume, diagnostic)
 *    opaque_t     *credentials;
 *    AMFS_NODE_T  *parentNode;
 *    char         *nodeName;
 *    opaque_t     **foundNode;
 *    boolean_t    *supportedVolume;
 *    NUCAM_DIAG_T *diagnostic;
 *
 * INPUT
 *    credentials     - The credentials of the UNIX client looking for the
 *                      amfsNode associated with the specified nodeName in
 *                      the specified parentNode.
 *    parentNode      - AMfs node representing the directory containing the
 *                      specified nodeName.
 *    nodeName        - Node name in search of.
 *
 * OUTPUT
 *    foundNode       - Found AMfs node object associated with the specified 
 *                      nodeName in the specified parentNode.  Set to NULL if
 *                      the node is not found.
 *    supportedVolume - Only used if the node is of type AM_VOLUME.  
 *                      It indicates if the volume can be mounted or not.
 *    diagnostic      - Set to one of the following if an error occurs:
 *
 * RETURN VALUES
 *    0           - Successful completion.
 *    -1          - Unsuccessful completion, diagnostic contains reason.
 *
 * MP Locking:
 *   parentNode is held sleep locked. While the lock may be dropped 
 *   in this function, the lock is reacquired before returning.
 *   The looked-for node does not need to be locked, as it is not examined
 *   or manipulated except when it is privately held.
 *
 * DESCRIPTION
 *    The AMfsLookUpNode searches the childList of the specified parentNode,
 *    looking for the an AMfs node with the specified nodeName and the specified
 *    parentNode.  If not found, foundNode is set to NULL.
 *
 * END_MANUAL_ENTRY
 */
ccode_t
AMfsLookUpNode (	opaque_t	*credentials,
			AMFS_NODE_T	*parentNode,
			char		*nodeName,
			AMFS_NODE_T	**foundNode,
			boolean_t	*supportedVolume,
			NUCAM_DIAG_T	*diagnostic )
{
	AMFS_NODE_ID_T		nodeID;
	AMFS_TYPE_T		nodeType;
	uint32			nodeNumber;

	NVLT_ENTER (6);
	NVLT_STRING (parentNode->name);
	NVLT_STRING (nodeName);

	/* 
	 * Validate the specified parentNode.
	 */
	ASSERT (parentNode != NULL);
	ASSERT ((parentNode->stamp == AMFS_ROOT_NODE_STAMP) ||
		(parentNode->stamp == AMFS_SERVER_NODE_STAMP) ||
		(parentNode->stamp == AMFS_VOLUME_NODE_STAMP));
	ASSERT ((parentNode->attributes.type == AM_ROOT) ||
		(parentNode->attributes.type == AM_SERVER) ||
		(parentNode->attributes.type == AM_VOLUME));
	ASSERT (nodeName != NULL);
	ASSERT (*nodeName != '\0');
	
	/*
	 * parentNode sleep locked by caller.
	 */
	ASSERT(AMNODE_LOCKOWNED(parentNode));

	/*
	 * Set the supportedVolume flag to the default value.
	 */
	*supportedVolume = FALSE;

	if (strcmp (nodeName, ".") == 0) {
		/*
		 * Self referencing parentNode.
		 */
		*foundNode = parentNode;
		VN_HOLD(&(*foundNode)->vnode);	
		return (NVLT_LEAVE (SUCCESS));
	}

	if (strcmp (nodeName, "..") == 0) {
		/*
		 * Get the parent of the specified parentNode.
		 * Since a child node has an implicit hold on
 		 * its parent node, we can safely dereference
		 * the parent of "parentNode".
		 * 
		 * NOTE:
		 *    Parent of the rootNode is set to itself.
		 */
		ASSERT (parentNode->parentNode != NULL);
		*foundNode = parentNode->parentNode;
		ASSERT((*foundNode)->vnode.v_count != 0);
		VN_HOLD(&(*foundNode)->vnode);
		return (NVLT_LEAVE (SUCCESS));
	}

retry:
	/* 
	 * Does an AMfs node already exist with the specified nodeName
	 * in specified parentNode's childList?
	 */

	/* 
	 * by convention, we are assigning AMFS_UNKNOWN_NODE_NUMBER
	 * to the internal name, to suggest that the search must
	 * be done by external name.
	 */
	nodeID.internalName = AMFS_UNKNOWN_NODE_NUMBER;

	nodeID.externalName = nodeName;
	if (AMfsSearchChildList (parentNode, &nodeID, foundNode,
			diagnostic) == SUCCESS) {
		/*
		 * Found the wanted AMfs node.  
		 */
		VN_HOLD(&(*foundNode)->vnode);
		return (NVLT_LEAVE (SUCCESS));
	} 

	/* 
	 * Need to call the Auto Mounter daemon to make sure 
	 * the specified nodeName exists.  If the parentNode's
	 * type is AM_ROOT, the the specified nodeName must
	 * represent a NetWare server name.  And if the
	 * parentNode's type is AM_SERVER, the specified
	 * nodeName must represent a volume name of the NetWare
	 * server.  
	 *
	 * NOTE:
	 *    Make sure the Auto Mounter daemon is running 
	 *    before issuing the request.
	 *
	 *    Also Make sure the nucamFileSystemSemaphore is not
	 *    locked.  If so, wait untill it is available.
	 */
	*foundNode = (AMFS_NODE_T *)NULL;
	*diagnostic = NUCAM_NODE_NOT_FOUND;
	if (nwamDeviceOpen == FALSE) {
		/*
		 * The nucam daemon is not running.
		 */
		return (NVLT_LEAVE (FAILURE));
	}

	/*
	 * Let go of the parent lock, since we are about to block (possibly)
	 * for the nucamFileSystemSemaphore. We will need to verify that
	 * the node we are looking for did not materialise while we were
	 * waiting either for the semaphore or for a reply from the nucam
	 * daemon.
	 */
	UNLOCK_AMNODE(parentNode);

	if (NWtlPSemaphore (nucamFileSystemSemaphore))  {
		LOCK_AMNODE(parentNode);
		return (NVLT_LEAVE (FAILURE));
	}
	
	nwamRequestReply.type = NWAM_LOOK_UP_ENTRY;
	NWtlGetCredUserID (credentials, (uint32 *)&nwamRequestReply.userID);
	nwamRequestReply.data.lookUpEntry.parentNodeType = 
			parentNode->attributes.type;
	strcpy ( nwamRequestReply.data.lookUpEntry.parentNodeName,
			parentNode->name);
	strcpy ( nwamRequestReply.data.lookUpEntry.searchName,
			nodeName);

	/*
	 * Unlock the nwamDriverSemaphore which would wake the
	 * NWamioctl routine up to get the reply from the nucamd
	 * daemon.
	 */
	ResponseAvailable = B_FALSE;
	if (NWtlVSemaphore_l (nwamDriverSemaphore)) {
		NWtlVSemaphore_l (nucamFileSystemSemaphore);
		LOCK_AMNODE(parentNode);
		return (NVLT_LEAVE (FAILURE));
	}

	/*
	 * Wait for the reply.
 	 */
	if (NucamWaitForResponse()) {
		NWtlVSemaphore_l (nucamFileSystemSemaphore);
		LOCK_AMNODE(parentNode);
		return (NVLT_LEAVE (FAILURE));
	}

	/*
	 * Okay, now we reobtain the parent lock, and decide whether we
	 * raced with someone else. 
	 */
	if (!TRYLOCK_AMNODE(parentNode)) {
		/*
		 * Possible race. Try again in a short while.
		 */
		delay(HZ/10);
		if (!TRYLOCK_AMNODE(parentNode)) {
			/*
			 * Okay, give up and try again from the top.
			 * We let go of the semaphore this time around.
			 */
			NWtlVSemaphore_l(nucamFileSystemSemaphore);	
			delay(HZ/10);
			LOCK_AMNODE(parentNode);
			goto retry;
		}
	}

	/* 
	 * by convention, we are assigning AMFS_UNKNOWN_NODE_NUMBER
	 * to the internal name, to suggest that the search must
	 * be done by external name.
	 */
	nodeID.internalName = AMFS_UNKNOWN_NODE_NUMBER;

	nodeID.externalName = nodeName;
	if (AMfsSearchChildList (parentNode, &nodeID, foundNode,
			diagnostic) == SUCCESS) {
		/*
		 * we raced with someone, and did find the node
		 * we were looking for. so we can leave here.
		 */
		NWtlVSemaphore_l (nucamFileSystemSemaphore);
		/*
		 * Found the wanted AMfs node.  
		 */
		VN_HOLD(&(*foundNode)->vnode);
		return (NVLT_LEAVE (SUCCESS));
	} 
	  
	if (nwamDeviceOpen == FALSE) {
		/*
		 * The process was awakened by NWamclose
		 * rountine because the nucam daemon was closed.
		 */
		NWtlVSemaphore_l (nucamFileSystemSemaphore);
		return (NVLT_LEAVE (FAILURE));
	}

	/*
	 * Got the reply. Specified nodeName is a valid name.
	 */
	*diagnostic = nwamRequestReply.diagnostic;
	if (*diagnostic != SUCCESS) {
		/*
		 * Node not found.
		 */
		NWtlVSemaphore_l (nucamFileSystemSemaphore);
		return (NVLT_LEAVE (FAILURE));
	}
	if (nwamRequestReply.data.lookUpEntry.address.len
			> MAX_ADDRESS_SIZE) {
		NWtlVSemaphore_l (nucamFileSystemSemaphore);
		*diagnostic = NUCAM_INVALID_NAME; /* okay? */
		return (NVLT_LEAVE (FAILURE));
	}
	nodeNumber = nwamRequestReply.data.lookUpEntry.foundNodeNumber;

	/*
	 * Set the netbuf buf pointer to the location of the data space in the
	 * RequestReply structure, which contains the address data.
	 */
	nwamRequestReply.data.lookUpEntry.address.buf =
		nwamRequestReply.data.lookUpEntry.buffer;

	/*
	 * Set the supportedVolume to indicate if this volume can be mounted
	 * or not.
	 */
	*supportedVolume = nwamRequestReply.data.lookUpEntry.supportedVolume;

	/*
 	 * Since we did not find the wanted AMfs node in the parentNode's
	 * childList, allocate a new AMFS node.  Set the specified 
	 * attributes's type to the appropriate AMfs node type.
	 */
	switch (parentNode->attributes.type) {
		case AM_ROOT:
			nodeType = AM_SERVER;
			break;
		case AM_SERVER:
			nodeType = AM_VOLUME;
			break;
		default:
			/*
			 * This should never happen.
			 */
			cmn_err (CE_PANIC,
			    "AMfsLookUpNode: Invalid parentNode type.");
			break;
	}
	/*
	 * It is okay to look at parentNode's vfsp without locking the
	 * vnode, because it cannot change under us.
	 */
	*foundNode = (AMFS_NODE_T *)NULL;
	if (AMfsAllocateNode(credentials, nodeType, nodeName,
		&nwamRequestReply.data.lookUpEntry.address,
		nodeNumber, parentNode->vnode.v_vfsp, foundNode, diagnostic)
		!= SUCCESS) {
		/*
	 	 * Could not allocate a new AMfs node object.
	 	 */
		(void)NWtlVSemaphore_l(nucamFileSystemSemaphore);
		return (NVLT_LEAVE (FAILURE));
	} 
	ASSERT((*foundNode) != (AMFS_NODE_T *)NULL);
	ASSERT((AMFS_VP(*foundNode))->v_count == 1);
	/*
	 * Unlock the nucamFileSystemSemaphore to allow requests
	 * to be issued.
	 */
	if (NWtlVSemaphore_l (nucamFileSystemSemaphore)) {
		AMfsFreeAmfsNode(*foundNode);
		*foundNode = NULL;
		*diagnostic = NUCAM_NODE_NOT_FOUND;
		return (NVLT_LEAVE (FAILURE));
	}
	ASSERT((*foundNode)->parentNode == NULL);
	ASSERT(AMFS_NODE_LIST_IS_EMPTY(&(*foundNode)->parentLink));
	return (NVLT_LEAVE (SUCCESS));

} /* end AMfsLookUpNode */


/*
 * BEGIN_MANUAL_ENTRY( AMfsReadAmfsNodeEntries(3k)i, \
 *                     ./man/kernel/nucam/amfs/ReadAmfsNodeEntries )
 * NAME
 *    AMfsReadAmfsNodeEntries - Reads directory entries from a SPIL NetWare 
 *                             directory node.
 *
 * SYNOPSIS
 *    ccode_t
 *    AMfsReadAmfsNodeEntries (credentials, amfsNode, dirIoArgs, diagnostic)
 *    opaque_t			*credentials;
 *    AMFS_NODE_T		*amfsNode;
 *    NUCAM_DIR_IO_ARGS_T	*dirIoArgs;
 *    NUCAM_DIAG_T		*diagnostic;
 *
 * INPUT
 *    credentials             - Credentials of the UNIX client reading the
 *                              directory entries.
 *    amfsNode                - Directory AMfs node to read the entries from.
 *    dirIoArgs->bufferLength - Size of nucIoBuf.buffer.
 *    dirIoArgs->buffer       - Pointer to a contiguous buffer to read the
 *                              amfsNode entries into.
 *    dirIoArgs->memoryType   - Set to IOMEM_KERNEL for kernel resident buffers
 *                              or IOMEM_USER for user resident buffers.
 *    dirIoArgs->searchHandle - Next read handle.
 *
 * OUTPUT
 *    dirIoArgs->bufferLength - Number of bytes saved in the buffer.
 *    dirIoArgs->buffer       - Is loaded with a contiguous formatted entries
 *                              according to "struct dirent".  As many entries
 *                              as posssible are formatted, or until end of
 *                              directory is reached.
 *    dirIoArgs->searchHandle - Next read handle.
 *    diagnostic              - Set to one of the following if an error occurs:
 *
 * RETURN VALUES
 *    0                       - Successful completion.
 *    -1                      - Unsuccessful completion, diagnostic contains
 *                              reason.
 *
 * MP Locking:
 *   The amfsNode is held locked by caller. The lock may be dropped and
 *   reacquired within this function.
 *
 * DESCRIPTION
 *    The AMfsReadAmfsNodeEntries fills the dirIoArgs->buffer argument with the
 *    AMfs node entries.  The number of entries to read is limited by an end of
 *    entries or the block size in the ioArgs->granulesRequested argument. 
 *
 * END_MANUAL_ENTRY
 */
ccode_t
AMfsReadAmfsNodeEntries (	nwcred_t		*credentials,
				AMFS_NODE_T		*amfsNode,
				NUCAM_DIR_IO_ARGS_T	*dirIoArgs,
				NUCAM_DIAG_T		*diagnostic )
{
	NVLT_ENTER (4);
	NVLT_STRING (amfsNode->name);

	ASSERT(AMNODE_LOCKOWNED(amfsNode));
	/*
	 * Validate the specified amfsNode.
	 */
	ASSERT (amfsNode != NULL);
	ASSERT ((amfsNode->stamp == AMFS_ROOT_NODE_STAMP) ||
		(amfsNode->stamp == AMFS_SERVER_NODE_STAMP) ||
		(amfsNode->stamp == AMFS_VOLUME_NODE_STAMP));

	/*
	 * Validate the node type.
	 */
	ASSERT ((amfsNode->attributes.type == AM_ROOT) ||
		(amfsNode->attributes.type == AM_SERVER) ||
		(amfsNode->attributes.type == AM_VOLUME));

	/*
	 * Need to call the Auto Mounter daemon to get AMfs node entries .  If
	 * the amfsNode's type is AM_ROOT, the the returned entries are names of
	 * NetWare servers.  And if the amfsNode's type is AM_SERVER, the
	 * returned entries are names of volume on the specified AMfs server
	 * node.
	 *
	 * NOTE
	 *    Lock the nucamFileSystemSemaphore to stop other processes from
	 *    using the  nwamRequestReply structure.  If in use, wait until it
	 *    is unlocked.
	 *
	 *    Make sure the Auto Mounter daemon is running before issuing the
	 *    request.
	 */
retry:
	*diagnostic = FAILURE;
	if (nwamDeviceOpen == FALSE)
		/*
		 * The nucam daemon is not running.
		 */
		return (NVLT_LEAVE (FAILURE));

	UNLOCK_AMNODE(amfsNode);

	if (NWtlPSemaphore (nucamFileSystemSemaphore)) {
		LOCK_AMNODE(amfsNode); /* doshi -- bug fix */
		return (NVLT_LEAVE (FAILURE));
	}

	nwamRequestReply.type = NWAM_GET_NODE_ENTRIES;
	NWtlGetCredUserID (credentials, (uint32 *)&nwamRequestReply.userID);
	nwamRequestReply.data.getEntries.parentNodeType = amfsNode->attributes.type;

	/*
	 * To Get the volumes list, we need the NetWare Server name.
	 */
	strcpy (nwamRequestReply.data.getEntries.parentNodeName,
			amfsNode->name);
	nwamRequestReply.data.getEntries.bufferLength = dirIoArgs->bufferLength;
	nwamRequestReply.data.getEntries.searchHandle = dirIoArgs->searchHandle;
	nwamRequestReply.data.getEntries.kernelBuffer = dirIoArgs->buffer;

	/*
	 * Unlock the nwamDriverSemaphore, which will wake the NWamioctl routine
	 * up to get the reply from the nucamd daemon.
	 */
	ResponseAvailable = B_FALSE;
	if (NWtlVSemaphore_l (nwamDriverSemaphore)) {
		NWtlVSemaphore_l (nucamFileSystemSemaphore);
		LOCK_AMNODE(amfsNode);
		return (NVLT_LEAVE (FAILURE));
	}

	/*
	 * Wait for the reply.
	 */
	if (NucamWaitForResponse()) {
		NWtlVSemaphore_l (nucamFileSystemSemaphore);
		LOCK_AMNODE(amfsNode);
		return (NVLT_LEAVE (FAILURE));
	}

	/*
	 * Okay, now we reobtain the parent lock, and decide whether we
	 * raced with someone else. 
	 */
	if (!TRYLOCK_AMNODE(amfsNode)) {
		/*
		 * Possible race. Try again in a short while.
		 */
		delay(HZ/10);
		if (!TRYLOCK_AMNODE(amfsNode)) {
			/*
			 * Okay, give up and try again from the top.
			 * We let go of the semaphore this time around.
			 */
			NWtlVSemaphore_l(nucamFileSystemSemaphore);	
			delay(HZ/10);
			LOCK_AMNODE(amfsNode);
			goto retry;
		}
	}


	if (nwamDeviceOpen == FALSE) {
		/*
		 * The process was awakened by NWamclose rountine because the 
		 * nucam daemon was closed.
		 */
		NWtlVSemaphore_l (nucamFileSystemSemaphore);
		return (NVLT_LEAVE (FAILURE));
	}

	/*
	 * Got the reply.  Set the dirIoArgs accordingly.
	 */
	*diagnostic = nwamRequestReply.diagnostic;
	if (*diagnostic != SUCCESS) {
		/*
		 * Something went wrong.
		 */
		NWtlVSemaphore_l (nucamFileSystemSemaphore);
		return (NVLT_LEAVE (FAILURE));
	}

	ASSERT(dirIoArgs->buffer == 
		nwamRequestReply.data.getEntries.kernelBuffer);
	dirIoArgs->searchHandle = 
		nwamRequestReply.data.getEntries.searchHandle;
	dirIoArgs->bufferLength = 
		nwamRequestReply.data.getEntries.bufferLength;

	/*
	 * Unlock the nucamFileSystemSemaphore to allow more request to be
	 * issued.
	 */
	if (NWtlVSemaphore_l (nucamFileSystemSemaphore)) {
		*diagnostic = FAILURE;
		return (NVLT_LEAVE (FAILURE));
	}
	return (NVLT_LEAVE (SUCCESS));
}

void
AMfsPrintNode (	AMFS_NODE_T *np )
{
	cmn_err(CE_CONT, "np 0x%x stamp 0x%x parentNode %x\n",
		     np, np->stamp, np->parentNode);
	cmn_err(CE_CONT, "address 0x%x mountPoint 0x%x parentLink 0x%x 0x%x\n",
		     np->address, np->mountPoint, np->parentLink.flink,
		     np->parentLink.rlink);
	cmn_err(CE_CONT, "childList 0x%x 0x%x &attributes 0x%x\n",
		     np->childList.flink, np->childList.rlink, &np->attributes);
	cmn_err(CE_CONT, "slplock 0x%x &vnode 0x%x\n", &np->slplock, 
		     &np->vnode);
	cmn_err(CE_CONT, "name (address of) = 0x%x\n %s\n",
		&(np->name[0]), np->name);
}
