/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucam/amfs_rootops.c	1.12"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucam/amfs_rootops.c,v 1.13.2.1 1994/12/12 01:08:31 stevbam Exp $"

/*
**  Netware Unix Client Auto Mounter File System
**
**	MODULE:
**		amfs_root.c -	The NetWare UNIX Client Auto Mounter File System
**                               (AMfs)	root operations.
**
**	ABSTRACT:
**		The amfs_root.c contains the NUCAM File System root operations.
**		The following root_ops operations are contained in this module:
**			AMfsMount ()
**			AMfsUnMount ()
*/ 

#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>
#include <fs/vnode.h>

#include <net/nuc/nwctypes.h>
#include <net/nuc/nuctool.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <fs/nucam/nucam_common.h>
#include <net/tiuser.h>
#include <net/nuc/nwmp.h>
#include <fs/nucam/amfs_node.h>
#include <fs/nucam/amfs_ops.h>
#include <fs/nucam/nucam_glob.h>
#include <fs/nucfs/nwfimacro.h>
#include <io/NWam/nwam.h>
#include <mem/kmem.h>

/*
 * Define the tracing mask.
 */
#define NVLT_ModMask		NVLTM_am



/*
 * BEGIN_MANUAL_ENTRY( AMfsRootOpsIntro(3k), \
 *                     ./man/kernel/nucam/amfs/RootOpsIntro )
 * NAME
 *     AMfsRootOpsIntro - Introduction to NetWare UNIX Client Auto Mounter File
 *                       System (AMfs) root operations.
 * SYNOPSIS
 *    #include <nucam_common.h>
 *    #include <amfs_root.h>
 *    #include <amfs_volume.h>
 *    #include <amfs_volume.h>
 *    #include <amfs_chandle.h>
 *
 * DESCRIPTION
 *    The NetWare UNIX Client Auto Mounter (NUCAM) File System is broken into to
 *    layer: the Auto Mounter Interface (AMfi) layer and the Auto Mounter File
 *    System (AMfs) layer.  The amfs_root.c contains the root operations which
 *    are responsible for mounting and unmounting a NUCAM File System.
 *
 *    The root AMfs node represents the root of the NUCAM File System and is 
 *    paired with the VFS Generic File System mount structure to form the mount
 *    point to the NUCAM File System.
 *
 * SEE ALSO
 *    AMfsMount(3k), and AMfsUnMount(3k).
 *
 * END_MANUAL_ENTRY
 */


/*
 * BEGIN_MANUAL_ENTRY( AMfsAddNucfsMnttabEntry(3k), \
 *                     ./man/kernel/nucam/amfs/AddNucfsMnttabEntry )
 * NAME
 *    AMfsAddNucfsMnttabEntry - Calls the NUC Auto Mounter daemon to add an
 *                              entey to /etc/mnttab file for the newly added 
 *                              NUCFS file system.
 *
 * SYNOPSIS
 *    ccode_t
 *    AMfsAddNucfsMnttabEntry (credentials, serverName, volumeName, diagnostic)
 *    opaque-t		*credentials;
 *    char		*serverName;
 *    char		*volumeName;
 *    NUCAM_DIAG_T	*diagnostic;
 *
 * INPUT
 *    credentials - Credentials of the UNIX client adding the nucfs mount entry.
 *    serverName  - Name of the volume containing the volume that was mounted.
 *    volumeName  - Name of the mounted volume.
 *
 * OUTPUT
 *    diagnostic  - Set to one of the following if an error occurs:
 *
 * RETURN VALUES
 *    0           - Successful completion.
 *    -1          - Unsuccessful completion, diagnostic contains reason.
 *
 * MP Locking:
 *    No nodes are dereferenced herein, so no lock protection appears 
 *    necessary. The nwamRequestReply structure is protected because of
 *    the use of the nucamFileSystemSemaphore.
 *
 * DESCRIPTION
 *    The AMfsAddNucfsMnttabEntry calls the NetWare Unix Client Auto Mounter
 *    daemon to add an entry to the /etc/mnttab file for the newly mounted NUCFS
 *    file system.
 *
 * END_MANUAL_ENTRY
 */
ccode_t
AMfsAddNucfsMnttabEntry (	opaque_t	*credentials,
				char		*serverName,
				char		*volumeName,
				NUCAM_DIAG_T	*diagnostic )
{
	NVLT_ENTER (4);

	/* 
	 * Need to call the Auto Mounter daemon to add an entry in /etc/mnttab
	 * file for the newly mounted NUCFS file system.  
	 *
	 * NOTE:
	 *    Make sure the nucamFileSystemSemaphore is not locked.  If so, wait
	 *    untill it is available.
	 *
	 *    Make sure the Auto Mounter daemon is running before issuing the
	 *    request.
	 */
	if (nwamDeviceOpen == FALSE) {
		/*
		 * The nucam daemon is not running.
		 */
		*diagnostic = FAILURE;
		return (NVLT_LEAVE (FAILURE));
	}

	if (NWtlPSemaphore (nucamFileSystemSemaphore))
		return (NVLT_LEAVE (FAILURE));

	nwamRequestReply.type = NWAM_ADD_NUCFS_MNTTAB_ENTRY;
	NWtlGetCredUserID (credentials, (uint32 *)&nwamRequestReply.userID);
	strcpy (nwamRequestReply.data.mnttabEntry.serverName, serverName);
	strcpy (nwamRequestReply.data.mnttabEntry.volumeName, volumeName);

	/*
	 * Unlock the nwamDriverSemaphore which would wake the NWamioctl routine
	 * up to get the reply from the nucamd daemon.
	 */
	ResponseAvailable = B_FALSE;
	if (NWtlVSemaphore_l (nwamDriverSemaphore)) {
		NWtlVSemaphore_l (nucamFileSystemSemaphore);
		return (NVLT_LEAVE (FAILURE));
	}

	/*
	 * Wait for the reply.
 	 */
	if (NucamWaitForResponse()) {
		NWtlVSemaphore_l (nucamFileSystemSemaphore);
		return (NVLT_LEAVE (FAILURE));
	}

	if (nwamDeviceOpen == FALSE) {
		/*
		 * The process was awakened by NWamclose rountine because the 
		 * nucam daemon was closed.
		 */
		*diagnostic = FAILURE;
		NWtlVSemaphore_l (nucamFileSystemSemaphore);
		return (NVLT_LEAVE (FAILURE));
	}

	/*
	 * Got the reply. The specified nodeName is a valid name.
	 */
	*diagnostic = nwamRequestReply.diagnostic;
	if (*diagnostic != SUCCESS) {
		/*
		 * Node not found.
		 */
		NWtlVSemaphore_l (nucamFileSystemSemaphore);
		return (NVLT_LEAVE (FAILURE));
	}

	/*
	 * Unlock the nucamFileSystemSemaphore to allow requests
	 * to be issued.
	 */
	if (NWtlVSemaphore_l (nucamFileSystemSemaphore))
		return (NVLT_LEAVE (FAILURE));

	return (NVLT_LEAVE (SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY( AMfsMount(3k), ./man/kernel/nucam/amfs/Mount )
 * NAME
 *    AMfsMount - Mounts the root AMfs node object onto the UNIX Generic File
 *                System.
 *
 * SYNOPSIS
 *    ccode_t
 *    AMfsMount (credentials, rootName, mountPoint, rootNode, diagnostic)
 *    opaque_t     *credentials;
 *    char         *rootName;
 *    opaque_t     *mountPoint;
 *    AMFS_NODE_T  **rootNode;
 *    NUCAM_DIAG_T *diagnostic;
 *
 * INPUT
 *    credentials  - Credentials of the UNIX client mounting the NUCAM File
 *                   System.
 *    rootName     - Name of the root AMfs node.
 *    mountPoint   - Pointer back to the UNIX Generic mount structure, the
 *                   rootNode is paired with.
 *
 * OUTPUT
 *    rootNode     - Newly allocated root AMfs node of the NUCAM File System.
 *    diagnostic   - Set to one of the following if an error occurs:
 *                   NUCAM_ALLOC_MEM_FAILED
 *
 * RETURN VALUES
 *    0            - Successful completion.
 *    -1           - Unsuccessful completion, diagnostic contains reason.
 *
 * MP Locking:
 *    Only one thread of control will be active in this function, or 
 *    examining the data structures of interest.
 *
 * DESCRIPTION
 *    The AMfsMount allocates and populates a root AMfs node.  The AMfsMount
 *    mounts the newly allocated root AMfs node object onto the UNIX Generic
 *    File System.
 *
 * SEE ALSO
 *    AMfsUnMount(3k).
 *
 * END_MANUAL_ENTRY
 */
ccode_t
AMfsMount (	opaque_t	*credentials,
		char		*rootName,
		opaque_t	*mountPoint,
		AMFS_NODE_T	**rootNode,
		NUCAM_DIAG_T	*diagnostic )
{
	NVLT_ENTER (5);

	/*
	 * Because only one thread of control can be in here, we don't
	 * have to worry about protecting the allocation function for
	 * the rootnode in an MP environment.
	 */

	/*
	 * Allocate the new root AMfs node.
	 */
	if (AMfsAllocateNode(credentials, AM_ROOT, rootName,
			(struct netbuf *)NULL, 1, (vfs_t *)mountPoint,
			rootNode, diagnostic) != SUCCESS)
		return (NVLT_LEAVE (FAILURE));

	/*
	 * Set the rootNode->parentNode to itself.
	 */
	(*rootNode)->parentNode = *rootNode;

	/*
	 * Set the pointer back to the UNIX Generic mount structure, the newly
	 * allocated root AMfs node is paired with.
	 */
	(*rootNode)->mountPoint = mountPoint;
	(*rootNode)->vnode.v_flag |= VROOT;
	return (NVLT_LEAVE (SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY( AMfsUnMount(3k), \
 *                     ./man/kernel/nucam/amfs/UnMount )
 * NAME
 *    AMfsUnMount - Unmounts the root AMfs node from the UNIX Generic File
 *                  System.
 *
 * SYNOPSIS
 *    ccode_t
 *    AMfsUnMount (credentials, rootNode, diagnostic)
 *    opaque_t     *credentials;
 *    AMFS_NODE_T  *rootNode;
 *    NUCAM_DIAG_T *diagnostic;
 *
 * INPUT
 *    credentials - Credentials of the UNIX client unmounting the server volume.
 *    rootNode    - Root AMfs node to be unmounted.
 *
 * OUTPUT
 *    diagnostic - Set to one of the following if an error occurs:
 *
 * RETURN VALUES
 *    none
 *
 * MP Locking:
 *   The root node is handed in locked. This function unlocks it before
 *   getting rid of it. 
 *
 * DESCRIPTION
 *    The AMfsUnMount unmounts the root AMfs node from the UNIX Generic File
 *    System.
 *
 * END_MANUAL_ENTRY
 */
void
AMfsUnMount (	AMFS_NODE_T	*rootNode )
{
	NVLT_ENTER (2);

	/*
	 * The reference count for our unbusy root vnode is 2 because
	 * the calling umount holds one reference, and the vfsp another.
	 */
	ASSERT (rootNode != NULL);
	ASSERT (rootNode->attributes.type == AM_ROOT);
	ASSERT (rootNode->stamp == AMFS_ROOT_NODE_STAMP);
	ASSERT (rootNode->vnode.v_count == 2);
	ASSERT(AMFS_NODE_LIST_IS_EMPTY(&rootNode->childList));

	/*
	 * Free the specified rootNode. 
	 *
	 * NOTE:
	 *    The AMfsFreeAmfsNode() is not called to release the rootNode.
	 *    Only AMfsUnMount routine can free the rootNode.
	 *
	 * Free the memory allocated for the rootNode name.
	 */
	NUCAM_KMEM_FREE (rootNode->name, strlen (rootNode->name)+1);

	rootNode->parentNode = (AMFS_NODE_T *)NULL;	
	VN_DEINIT(&rootNode->vnode);
	if (rootNode->address.buf) {
		NUCAM_KMEM_FREE(rootNode->address.buf, rootNode->address.len);
	}
	SLEEP_DEINIT(AMFS_SLPLOCK_P(rootNode));
	NUCAM_KMEM_FREE (rootNode, sizeof (AMFS_NODE_T));

	/*
	 * Decrement the total AMfs node count.
	 */
	LOCK_NUCAM_SPIN();
	amfsNodeCount--;
	ASSERT (amfsNodeCount == 0);

	/*
	 * Decrement the total number of NUCAM mount count.
	 */
	nucamMountCount--;
	ASSERT (nucamMountCount == 0);
	UNLOCK_NUCAM_SPIN();
}
