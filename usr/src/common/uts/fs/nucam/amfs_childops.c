/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucam/amfs_childops.c	1.7"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucam/amfs_childops.c,v 1.5.2.1 1994/12/12 01:07:46 stevbam Exp $"

/*
**  Netware Unix Client Auto Mounter
**
**	MODULE:
**		amfs_childops.c - The NUCAM AMfs node genealogy operations.
**
**	ABSTRACT:
**		amfs_childops.c is resposible for attaching and detaching child
**		AMfs nodes to their parentNode.
**
**		The following amfs_childops operations are contained in this
**		module:
**			AMfsAttachToParentNode ()
**			AMfsDetachFromParentNode ()
**
*/ 

#include <util/types.h>
#include <fs/vnode.h>
#include <util/debug.h>

#include <net/nuc/nwctypes.h>
#include <fs/nucam/nucam_common.h>
#include <net/tiuser.h>
#include <fs/nucam/amfs_node.h>
#include <fs/nucam/amfs_ops.h>


/*
 * BEGIN_MANUAL_ENTRY( AMfsAttachToParentNode(3k), \
 *                     ./man/kernel/nucfs/nwfs/AttachToParentNode )
 * NAME
 *    AMfsAttachToParentNode - Attaches a AMfs node to its parent AMfs node.
 *
 * SYNOPSIS
 *    void
 *    AMfsAttachToParentNode (parentNode, childNode)
 *    AMFS_NODE_T *parentNode;
 *    AMFS_NODE_T *childNode;
 *
 * INPUT
 *    parentNode - Parent AMfs node object to attach to.
 *    childNode  - AMfs node to be attached to its parent AMfs node.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUES
 *    None.
 *
 * MP Locking:
 *    Parent node sleep lock held by caller, across the function.
 *
 * DESCRIPTION
 *    The AMfsAttachToParentNode attaches a AMfs node to its' parent AMfs
 *    node.
 *
 * END_MANUAL_ENTRY
 */
void
AMfsAttachToParentNode (	AMFS_NODE_T	*parentNode,
				AMFS_NODE_T	*childNode )
{
	ASSERT(AMNODE_LOCKOWNED(parentNode));
	ASSERT(childNode->stamp != AMFS_ROOT_NODE_STAMP);
	/*
	 * each child contributes a reference count on the parent,
	 * we must therefore bump the count on behalf of the new child.
	 */
	VN_HOLD(&parentNode->vnode);
	childNode->parentNode = parentNode;
	AMFS_NODE_LIST_ADD(&parentNode->childList, &childNode->parentLink);
}


/*
 * BEGIN_MANUAL_ENTRY( AMfsDetachFromParentNode(3k), \
 *                     ./man/kernel/nucfs/nwfs/DetachFromParentNode )
 * NAME
 *    AMfsDetachFromParentNode - Detaches an AMfs node from its parent AMfs
 *                               node.
 *
 * SYNOPSIS
 *    void
 *    AMfsDetachFromParentNode (childNode)
 *    AMFS_NODE_T *childNode;
 *
 * INPUT
 *    childNode - AMfs node to be detached from its parent AMfs node.
 *
 * OUTPUT
 *    None.
 *
 * MP Locking:
 *    Parent node sleep lock held by caller, across the function.
 *
 * RETURN VALUES
 *    None.
 *
 * DESCRIPTION
 *    The AMfsDetachFromParentNode detaches a AMfs node from its parent
 *    AMfs node.
 *
 * END_MANUAL_ENTRY
 */
void
AMfsDetachFromParentNode (AMFS_NODE_T	*childNode)
{
	AMFS_NODE_T	*parentNode;

	ASSERT(childNode->stamp != AMFS_ROOT_NODE_STAMP);
	ASSERT(AMNODE_LOCKOWNED(childNode->parentNode));
	parentNode = childNode->parentNode;
	AMFS_NODE_LIST_REMOVE(&childNode->parentLink);
	childNode->parentNode = NULL;

	/*
	 * because each child contributes a reference count on the parent,
	 * we must release the count held by this child.
	 */
	VN_RELE(&parentNode->vnode);
}
