/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucam/amfs_node.h	1.7"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucam/amfs_node.h,v 1.6.2.1 1994/12/12 01:08:03 stevbam Exp $"

#ifndef _AMFS_NODE_H
#define _AMFS_NODE_H

/*
**  Netware Unix Client Auto Mounter File System
**
**	MODULE:
**		amfs_node.h -	Contains the NUCAM File System Amfs node object
**				definitions.
**
**	ABSTRACT:
**		The amfs_node.h is included in the AMfs layer of the NUCAM File
**		system and represents the AMfs node object.
**
*/

/*
 * UIO_OFFSET_EOF == (-2) is a special value used to signal that there are
 * no more directory entries to be read.
 */
#define UIO_OFFSET_EOF		(-2)

/*
 * DIR_SEARCH_ALL == (-1) is a special value for directory search handle, 
 * specifying that all server entries from the beginning of the list 
 * are to be read by the nucam daemon.
 */
#define	DIR_SEARCH_ALL		(-1)

/*
 * DIR_DOT_OFFSET == (0) is a special value for directory search handle,
 * specifying that the "." entry should be the first entry returned.
 */
#define DIR_DOT_OFFSET		(0)

/*
 * DIR_DOTDOT_OFFSET == (-3) is a special value for directory search handle,
 * specifying that the ".." entry should also be returned.
 */
#define DIR_DOTDOT_OFFSET	(-3)

#ifdef _KERNEL

#include <net/xti.h>
#include <util/list.h>
#include <util/sysmacros.h>
#include <util/listasm.h>
#include <fs/vnode.h>
#include <fs/nucam/nucam_nwc_proto.h>
#include <util/ksynch.h>

/*
 * NAME
 *    AMfsNode - The AMfs node object structure.
 * 
 * DESCRIPTION
 *    This data structure defines the NUCAM File System AMfs node object
 *    structure.  It is paired with the UNIX Generic File System Node (VNODE,
 *    INODE, etc) to form the focal point for an AMfs node.
 *
 *    stamp       - AMfs node stamp indicating that the AMfs node is 
 *		    not stale or invalid.  Set to one of the following:
 *                  AMFS_ROOT_NODE_STAMP
 *                  AMFS_SERVER_NODE_STAMP
 *                  AMFS_VOLUME_NODE_STAMP
 *    parentNode  - Pointer back to the parent AMfs node.  If the AMfsNode's
 *                  type is AMFS_ROOT, the parentNode points to the root node.
 *    name        - AMfs node name.
 *    address	  - Address of a netbuf structure for AMFS node. May be NULL.
 *    mountPoint  - Pointer back to the UNIX mount structure.
 *    parentLink  - generic list element in parent node's childList, always
 *		    empty for root node. 
 *		    NOTE:      - An empty parentLink points to itself.
 *    childList   - generic list head of AMfs node objects associated with this
 *		    AMfs node.  If AmfsNode's type is set AM_ROOT, the childList
 *                  points to a list of AM_SERVER nodes.  If AMfsNode's type is
 *                  set to AM_SERVER, the childList points to a list of
 *		    AM_VOLUME nodes.
 *                  NOTE:      - An empty childList points to itself.
 *                             - Each node on the childList holds a reference
 *                               on the vnode of its parent. Therefore, if  
 *                               childList is not empty, then vnode hold count
 *                               cannot be 0.
 *    attributes  - Structure containing following fields:
 *    attributes.number        - Unique AMfs node object identifier.
 *    attributes.type          - AMfsNode type. Set to one of the following:
 *                               AM_ROOT    - Root AMfs node.
 *                               AM_SERVER  - Server AMfs node.
 *                               AM_VOLUME  - Volume AMfs node.
 *                               AM_UNKNOWN - Unknown node.
 *    attributes.permissions   - AMfsNode permissions (see above).
 *    attributes.numberOfLinks - AMfsNode number of links.
 *    attributes.size          - Size in bytes of data space.
 *    attributes.userID        - User identifier of owner.
 *    attributes.groupID       - Group identifier of owner.
 *    attributes.accessTime    - Time of last access.
 *    attributes.modifyTime    - Time of last data modification.
 *    attributes.changeTime    - Time of last name space changed.
 *    slplock	 - sleep lock protecting all fields of the node,
 *		   except vnode, parentNode, and parentLink. 
 *		   parentLink and parentNode are covered
 *		   by the sleep lock in the parent Node.
 *		   A reader-writer sleep lock is not used because the
 *		   primary function of AMFS is to provide a directory
 *		   name lookup service and automounting; thus lookup 
 *		   operations transparently perform structure modification.
 *		   Therefore exclusive locking is useful during lookups; so
 *		   we just work with an exclusive lock per amfs node.
 *    vnode	 - generic file system node
 */

/*
 * Define AMfsNode object structure mask.
 */
typedef struct AMfsNode {
	uint32			stamp;
	struct AMfsNode		*volatile parentNode;
	char			*name;
	struct netbuf		address;
	opaque_t		*mountPoint;
	list_t			parentLink;
	list_t			childList;
	NUCAM_ATTRIBUTES_T	attributes;
        sleep_t			slplock;
	vnode_t			vnode;
} AMFS_NODE_T;

/*
 * Default AMFS node size == 512.
 */
#define	AMFS_DEFAULT_NODE_SIZE	512

/*
 * Macros for getting server AMfs node fields.
 */
#define	AMFS_NODE_STAMP(n)	((AMFS_NODE_T *)(n))->stamp
#define	AMFS_NODE_NAME(n)	((AMFS_NODE_T *)(n))->name
#define	AMFS_GET_CHILD_LIST(n)	((AMFS_NODE_T *)(n))->childList
#define	AMFS_PARENT_NODE(n)	((AMFS_NODE_T *)(n))->parentNode
#define	AMFS_PARENT_LINK_P(n)	&(((AMFS_NODE_T *)(n))->parentLink)
#define	AMFS_VP(n)		&(((AMFS_NODE_T *)(n))->vnode)
#define	AMFS_NODE_ATTR(n)	((AMFS_NODE_T *)(n))->attributes
#define	AMFS_NODE_ID(n)		((AMFS_NODE_T *)(n))->attributes.number
#define	AMFS_NODE_TYPE(n)	((AMFS_NODE_T *)(n))->attributes.type
#define	AMFS_NODE_SIZE(n)	((AMFS_NODE_T *)(n))->attributes.size
#define	AMFS_NODE_PERMISSION(n)	((AMFS_NODE_T *)(n))->attributes.permissions
#define	AMFS_NODE_LINKS(n)	((AMFS_NODE_T *)(n))->attributes.numberOfLinks
#define	AMFS_NODE_UID(n)	((AMFS_NODE_T *)(n))->attributes.userID
#define	AMFS_NODE_GID(n)	((AMFS_NODE_T *)(n))->attributes.groupID
#define	AMFS_SLPLOCK_P(n)	&(((AMFS_NODE_T *)(n))->slplock)

#define AMFS_NODE_FROM_PARENT_LINK(pl)	\
	((AMFS_NODE_T *)((char *)(pl) - offsetof(AMFS_NODE_T, parentLink)))
#define AMFS_NODE_FROM_CHILD_LIST(cl)	\
	((AMFS_NODE_T *)((char *)(cl) - offsetof(AMFS_NODE_T, childList)))

#define AMFS_NODE_LIST_INIT(l)		\
	((void)((l)->flink = (l), (l)->rlink = (l)))
#define AMFS_NODE_LIST_IS_EMPTY(l)	\
	((l)->flink == (l) ? (ASSERT((l)->rlink == (l)), B_TRUE) : B_FALSE)
#define AMFS_NODE_LIST_ADD(lhead, l) 	\
	(ASSERT(AMFS_NODE_LIST_IS_EMPTY(l)), insque((void *)(l), (void *)lhead))
#define AMFS_NODE_LIST_REMOVE(l)	\
	(remque((void *)(l)), AMFS_NODE_LIST_INIT(l))

#ifdef  NUCAM_KMEM_PARANOID
#define	NUCAM_DEBUG_PATTERN	KMA_MAGIC
#define	NUCAM_KMEM_FREE(addr, size)  do { int *taddr; \
	taddr = (int *)addr; \
	while (taddr < (int *)((char *)addr + size)) { \
		*taddr = NUCAM_DEBUG_PATTERN; \
		taddr++; \
	} \
	kmem_free(addr, size); \
} while (0)

#else
#define	NUCAM_KMEM_FREE(addr, size)	kmem_free(addr, size)
#endif

#define	NUCAM_KMEM_ALLOC(a, b)	kmem_zalloc(a, b)

#endif	/* _KERNEL */
#endif					/* _AMFS_NODE_H		*/
