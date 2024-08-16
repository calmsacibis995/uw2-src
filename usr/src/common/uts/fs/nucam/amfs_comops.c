/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucam/amfs_comops.c	1.17"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucam/amfs_comops.c,v 1.12.2.1 1994/12/12 01:07:54 stevbam Exp $"

/*
**  Netware Unix Client Auto Mounter
**
**	MODULE:
**		amfs_comops.c -	The NetWare Unix Client Auto Mounter File System
**				(AMfs) common operations.
**
**	ABSTRACT:
**		The amfs_comops.c contains the AMfs common functions of the
**		NUCAM File System.
**
**		The following comops operations are contained in this module:
**			AMfsAllocateNode ()
**			AMfsFreeAmfsNode ()
**			AMfsSearchChildList ()
**
*/ 

#include <mem/kmem.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <fs/vnode.h>
#include <util/debug.h>

#include <net/tiuser.h>
#include <net/nuc/nwctypes.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <fs/nucam/nucam_common.h>
#include <fs/nucam/amfs_node.h>
#include <fs/nucam/amfs_ops.h>
#include <fs/nucam/nucam_glob.h>
#include <fs/nucfs/nwfimacro.h>

#include <io/NWam/nwam.h>

/*
 * Define the tracing mask.
 */
#define NVLT_ModMask	NVLTM_am
#define	NUCAM_HIER	150
LKINFO_DECL(amfs_slplock_lkinfo, "NUCAM:slplock:amfsNode (per-amfsNode)", 0);



/*
 * BEGIN_MANUAL_ENTRY( AMfsAllocateNode(3k), \
 *                     ./man/kernel/nucam/amfs/AllocateNode )
 * NAME
 *    AMfsAllocateNode - Allocate and populate an AMfs node object. 
 *
 * SYNOPSIS
 *    ccode_t
 *    AMfsAllocateNode (credentials, nodeType, nodeName, nodeAddress,
 *			nodeNumber, vfsMountInstance, amfsNode, diagnostic)
 *    opaque_t     *credentials;
 *    AMFS_TYPE_T  nodeType;
 *    char         *nodeName;
 *    struct netbuf *nodeAddress;
 *    uint32 	   nodeNumber;
 *    vfs_t	   *vfsMountInstance;
 *    AMFS_NODE_T  **amfsNode;
 *    NUCAM_DIAG_T *diagnostic;
 *
 * INPUT
 *    credentials - Credentials of the UNIX client allocating the AMfs node.
 *    nodeType    - Set to one of the following AMfs node types:
 *                  AM_ROOT   - A root AMfs node.
 *                  AM_SERVER - A server AMfs node.
 *                  AM_VOLUME - A volume AMfs node.
 *    nodeName    - Name of the node.
 *    nodeAddress - If not NULL, the address of a netbuf structure 
 *		    containing address information to be encoded into 
 *                  the new AMfs node object.
 *    nodeNumber  - Internal unique identifier to be assigned to the 
 *		    new AMfs node.
 *    vfsMountInstance - Fully populated VFS structure for the NUCAM VFS AMfs
 *                       root node.
 *
 * OUTPUT
 *    amfsNode    - Newly allocated AMfs node object containing a held,
 *		    initialized vnode.
 *    diagnostic  - Set to one of the following if an error occurs:
 *
 * RETURN VALUES
 *    0           - Successful completion.
 *    -1          - Unsuccessful completion, diagnostic contains reason.
 *
 * MP Locking:
 *   Caller ensures that two threads cannot be active in allocating the
 *   same logical node. This happens in one instance by holding the parent
 *   node sleep locked (in AMfsLookUpNodeByName) and in the other case
 *   (i.e., AMfsMount) by permitting only one thread into the code.
 *
 * DESCRIPTION
 *    The AMfsAllocatNode allocates and populates a new root AMFs node, or
 *    a new server AMfs node, or a new volume AMFS node depending on the
 *    specified nodeType.
 *
 * END_MANUAL_ENTRY
 */
ccode_t
AMfsAllocateNode (	opaque_t	*credentials,
			AMFS_TYPE_T	nodeType,
			char		*nodeName,
			struct netbuf	*nodeAddress,
			uint32		nodeNumber,
			vfs_t		*vfsMountInstance,
			AMFS_NODE_T	**amfsNode,
			NUCAM_DIAG_T	*diagnostic )
{
	vtype_t		unixNodeType;

	NVLT_ENTER (8);
	NVLT_STRING (nodeName);

	ASSERT (nodeName != NULL);

	if (AMfiGetNodeType(nodeType, &unixNodeType, diagnostic) != SUCCESS) {
	        return (NVLT_LEAVE (FAILURE));
	}

	LOCK_NUCAM_SPIN();
	amfsNodeCount++;
	UNLOCK_NUCAM_SPIN();

	*amfsNode = (AMFS_NODE_T *)NUCAM_KMEM_ALLOC (sizeof (AMFS_NODE_T),
			KM_SLEEP);
	AMFS_NODE_LIST_INIT(&((*amfsNode)->childList));
	AMFS_NODE_LIST_INIT(&((*amfsNode)->parentLink));

	/*
	 * Stamp the newly allocated amfsNode with its object stamp.
	 */
	switch (nodeType) {
	case AM_ROOT:
		(*amfsNode)->stamp = AMFS_ROOT_NODE_STAMP;
		break;
	case AM_SERVER:
		(*amfsNode)->stamp = AMFS_SERVER_NODE_STAMP;
		break;
	case AM_VOLUME:
		(*amfsNode)->stamp = AMFS_VOLUME_NODE_STAMP;
		break;
	}

	/*
	 * Set the amfsNode type. 
	 */
	(*amfsNode)->attributes.type = nodeType;

	/* 
	 * Set the amfsNode name.
	 */
	(*amfsNode)->name = (char *)NUCAM_KMEM_ALLOC ((strlen(nodeName))+1,
			KM_SLEEP);
	strcpy ((*amfsNode)->name, nodeName);

	/* 
	 * Set the amfsNode address.
	 */
	if (nodeAddress && nodeAddress->len) {
		(*amfsNode)->address.buf =
			(char *)NUCAM_KMEM_ALLOC (nodeAddress->len, KM_SLEEP);
		(*amfsNode)->address.maxlen = nodeAddress->len;
		(*amfsNode)->address.len = nodeAddress->len;
		ASSERT(nodeAddress->buf != NULL);
		bcopy(nodeAddress->buf, (*amfsNode)->address.buf, 
			nodeAddress->len);
	} else {
		(*amfsNode)->address.buf = NULL;
		(*amfsNode)->address.maxlen = 0;
		(*amfsNode)->address.len = 0;
	}

	/*
	 * Initialize the amfsNode's attributes.
	 */
	(*amfsNode)->attributes.number = nodeNumber;
	(*amfsNode)->attributes.numberOfLinks = 2;
	(*amfsNode)->attributes.permissions = AM_OWNER_READ_BIT |
			AM_OWNER_EXECUTE_BIT | AM_GROUP_READ_BIT |
			AM_GROUP_EXECUTE_BIT | AM_OTHER_READ_BIT |
			AM_OTHER_EXECUTE_BIT;
	(*amfsNode)->attributes.size = AMFS_DEFAULT_NODE_SIZE;
	(*amfsNode)->attributes.accessTime = amfsNodeTime;
	(*amfsNode)->attributes.modifyTime = amfsNodeTime;
	(*amfsNode)->attributes.changeTime = amfsNodeTime;
	NWtlGetCredUserID (credentials, &((*amfsNode)->attributes.userID));
	NWtlGetCredGroupID (credentials, &((*amfsNode)->attributes.groupID));

	/*
	 * Clear the amfsNode elements.
	 */
	/* (*amfsNode)->flags = 0; removed */
	(*amfsNode)->mountPoint = (opaque_t *)NULL;
	(*amfsNode)->parentNode = (AMFS_NODE_T *)NULL;

        SLEEP_INIT(AMFS_SLPLOCK_P(*amfsNode), NUCAM_HIER,
                                &amfs_slplock_lkinfo, KM_SLEEP);

	/*
	 * AMfiInitVnode initializes and holds the vnode.
	 */
	AMfiInitVnode(vfsMountInstance, *amfsNode, unixNodeType);

	return (NVLT_LEAVE (SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY( AMfsFreeAmfsNode(3k), \
 *                     ./man/kernel/nucfs/nwfs/FreeAmfsNode )
 * NAME
 *    AMfsFreeAmfsNode - Frees the specified amfsNode.  Caller is responsible
 *			 for ensuring that the node is removed from its parent's
 *			 childList.
 *
 * SYNOPSIS
 *    void 
 *    AMfsFreeAmfsNode (amfsNode)
 *    AMFS_NODE_T *amfsNode;
 *
 * INPUT
 *    amfsNode  - AMfs node to be freed.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUES
 *    None.
 *
 * MP Locking:
 *    Trivially ensured by calling this function ONLY FOR PRIVATELY held nodes.
 *
 * DESCRIPTION
 *    The AMfsFreeAmfsNode frees the specified amfsNode.  Caller is responsible
 *    for ensuring that the node is removed from its parent's childList.
 *
 * END_MANUAL_ENTRY
 */
void
AMfsFreeAmfsNode (	AMFS_NODE_T	*amfsNode )
{
	NVLT_ENTER (1);
	NVLT_STRING (amfsNode->name);

	ASSERT(AMFS_NODE_LIST_IS_EMPTY(&amfsNode->childList));
	ASSERT(AMFS_NODE_LIST_IS_EMPTY(&amfsNode->parentLink));
	ASSERT(!amfsNode->parentNode);
	ASSERT(amfsNode->vnode.v_count == 1);
	ASSERT(amfsNode->stamp != AMFS_ROOT_NODE_STAMP);

	/*
	 * Prevent other processes access.
	 *
	 * Previously:
	 *	amfsNode->flags |= AMFS_HOLD_NODE_BIT;
	 * Now:
	 *	alternative means to ensure that there is no race 
	 *	with a node delete operation. Only callers of this 
	 *	function are AMfsAllocateNode, AMfsLookupNode, and
	 *	AMfiReleaseNode.
	 *
	 *	AMfsAllocateNode is called only from AMfsLookUpNode and
	 *	AMfsMount.  AMfsLookUpNode is called only from AMfiLookUpNode.
	 *	which is VOP_LOOKUP.  Both AMfsAllocateNode and AMfsLookupNode
	 *	call here when an error happens after initializing the node.
	 *		
	 *	AMfiReleaseNode is VOP_INACTIVE, so the call here is as a
	 *	result of normal inactivation.
	 * So:
	 *	The protection from delete should basically be that we 
	 *	destroy identity under lock cover (assuming all identity 
	 *	searching/establishing is done under the same lock cover). 
	 * Note:
	 *	The AMFS root node is freed explicitly in AMfsUnMount.
	 *
	 */

	VN_DEINIT(&amfsNode->vnode);

	/*
	 * Free the memory allocated for the node name.
	 */
	NUCAM_KMEM_FREE (amfsNode->name, strlen (amfsNode->name)+1);

	if (amfsNode->address.buf)
		NUCAM_KMEM_FREE(amfsNode->address.buf, amfsNode->address.len);

	SLEEP_DEINIT (AMFS_SLPLOCK_P(amfsNode));
	NUCAM_KMEM_FREE (amfsNode, sizeof (AMFS_NODE_T));
	LOCK_NUCAM_SPIN();
	amfsNodeCount--;
	UNLOCK_NUCAM_SPIN();
	NVLT_LEAVE (SUCCESS);
}


/*
 * BEGIN_MANUAL_ENTRY( AMfsSearchChildList(3k), \
 *                     ./man/kernel/nucfs/nwfs/SearchChildList)
 * NAME
 *    AMfsSearchChildList - Looks for the AMfs node associated with the
 *                          specified node identifier in the specified
 *                          parentNode's childList
 *
 * SYNOPSIS
 *    ccode_t
 *    AMfsSearchChildList (parentNode, nodeID, foundNode, diagnostic)
 *    AMFS_NODE_T	*parentNode;
 *    AMFS_NODE_ID_T	*nodeID;
 *    AMFS_NODE_T	**foundNode;
 *    NUCAM_DIAG_T	*diagnostic;
 *
 * INPUT
 *    parentNode           - Parent AMfs node with the childList.
 *    nodeID->internalName - Unique node identifier of the AMfs node in search
 *                           of (used when not set to AMFS_UNKNOWN_NODE_NUMBER).
 *    nodeID->externalName - Node name of the Amfs node in search of (used 
 *                           when internalName is set to AMFS_UNKNOWN_NODE_
 *                           NUMBER).
 *
 * OUTPUT
 *    foundNode            - AMfs node in search of.
 *    diagnostic           - Set to one of the following if an error occurs:
 *                           NUCAM_NODE_NOT_FOUND
 *
 * RETURN VALUES
 *    0                    - Successful completion.
 *    -1                   - Unsuccessful completion, diagnostic contains
 *                           reason.
 *
 * DESCRIPTION
 *    The AMfsSearchChildList looks in the specified parentNode's childList, for
 *    an AMfs node with the either the internal or external name.
 *
 * END_MANUAL_ENTRY
 */
ccode_t
AMfsSearchChildList (	AMFS_NODE_T	*parentNode,
			AMFS_NODE_ID_T	*nodeID,
			AMFS_NODE_T	**foundNode,
			NUCAM_DIAG_T	*diagnostic )
{
	list_t	*volatile nextElement;

	NVLT_ENTER (4);

	/*
	 * the parentNode is held locked by caller. Ordinarily, when
	 * referencing attributes of a childNode (during search below,
	 * for example), we require a lock on the childNode to stabilize
	 * attributes. However, because the attributes in question for
	 * this search are only the internal and external names of the
	 * nodes, we do not need to lock the child nodes during search.
	 * readdir operations can change these attributes, and they are 
	 * held off due to parent being locked. child nodes are not
	 * removed either, because the parent is held locked. 
	 */

	ASSERT(AMNODE_LOCKOWNED(parentNode));

	/*
	 * This assertion is an XOR.
	 */
	ASSERT((nodeID->internalName == AMFS_UNKNOWN_NODE_NUMBER) !=
	       (nodeID->externalName == NULL));

	if (nodeID->externalName != NULL)
		NVLT_STRING (nodeID->externalName);

	for (nextElement = parentNode->childList.flink;
	     nextElement != &parentNode->childList;
	     nextElement = nextElement->flink) {
		*foundNode = AMFS_NODE_FROM_PARENT_LINK(nextElement);
		ASSERT((*foundNode)->parentNode == parentNode);
		if (nodeID->internalName != AMFS_UNKNOWN_NODE_NUMBER) {
			/*
			 * Looking by nodeID.
			 */
			if ((*foundNode)->attributes.number ==
					nodeID->internalName)
				/*
				 * This is the one.  
				 */
				return (NVLT_LEAVE (SUCCESS));
		} else {

			/*
			 * Looking by name.
			 */
			if (strcmp((*foundNode)->name,
					nodeID->externalName) == 0)
				/*
				 * Found an AMfs node with the specified
				 * external name.  This must be one we are in
				 * search of since none of the node names can
				 * be the same in a childList.
				 */
				return (NVLT_LEAVE (SUCCESS));
		}
	}

	*diagnostic = NUCAM_NODE_NOT_FOUND;
	*foundNode = (AMFS_NODE_T *)NULL;
	return (NVLT_LEAVE (FAILURE));
}

