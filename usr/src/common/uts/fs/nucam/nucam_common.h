/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucam/nucam_common.h	1.8"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucam/nucam_common.h,v 1.7.2.1 1994/12/12 01:08:40 stevbam Exp $"

#ifndef _NUCAM_COMMON_H
#define _NUCAM_COMMON_H

/*
**  Netware Unix Client Auto Mounter File System
**
**	MODULE:
**		nucamcommon.h -	The NetWare UNIX Client Auto Mounter File System
**				common definitions used in both the AMfi and
**				AMfs layers.
**
**	ABSTRACT:
**		The nucamcommon.h is included with both the AMfi and AMfs layers
**		of the NUCAM.  This provides a consistent semantic
**		representation of interface information used between these
**		layers.
**
*/

#ifdef _KERNEL_HEADERS
#include <util/sysmacros_f.h>
#else
#include <sys/sysmacros_f.h>
#endif _KERNEL_HEADERS

/*
 * NUCAM sleep priority.
 */
#define	NUCAM_SLEEP_PRIORITY	primed	/* NUCAM sleep priority		*/

/*
 * NUC File System maximum values.
 */
#define	AM_DOT_ENTRIES_LENGTH	32	/* . & .. entries size		*/
#define	AM_MAX_NAME_LENGTH	255	/* Max name length		*/

/*
 * Define the NetWare UNIX Client Auto Mounter File System logical block size.
 */
#define	NUCAM_LOGICAL_BLOCK_SIZE	1024

#define	AMFS_UNKNOWN_NODE_NUMBER	-1

/*
 * Node object permissions.
 */
#define AM_NO_PERMISSION	0x00000L	/* No permissions granted */
#define	AM_OTHER_EXECUTE_BIT	0x00001L	/* Other execute perm bit */
#define	AM_OTHER_WRITE_BIT	0x00002L	/* Other write perm bit	  */
#define	AM_OTHER_READ_BIT	0x00004L	/* Other read perm bit	  */
#define	AM_GROUP_EXECUTE_BIT	0x00008L	/* Group execute perm bit */
#define	AM_GROUP_WRITE_BIT	0x00010L	/* Group write perm bit	  */
#define	AM_GROUP_READ_BIT	0x00020L	/* Group read perm bit	  */
#define	AM_OWNER_EXECUTE_BIT	0x00040L	/* Owner execute perm bit */
#define	AM_OWNER_WRITE_BIT	0x00080L	/* Owner write perm bit	  */
#define	AM_OWNER_READ_BIT	0x00100L	/* Owner read perm bit	  */

/*
 * The Amfsnode types.
 *	AM_ROOT              - Root AMfs node.
 *	AM_SERVER            - Server AMfs node.
 *	AM_VOLUME            - Volume AMfs node.
 *	AM_UNKNOWN           - Unknown node type.
 */
typedef	enum	amfsNodeType	{
		AM_ROOT			= 1,
		AM_SERVER		= 2,
		AM_VOLUME		= 3,
		AM_UNKNOWN		= 4
} AMFS_TYPE_T;

/*
 * The AMfs node stamps.
 */
#define	AMFS_ROOT_NODE_STAMP	0xA726E		/* A R(oot) N(ode)	*/
#define	AMFS_SERVER_NODE_STAMP	0xA736E		/* A S(erver) N(ode)	*/
#define	AMFS_VOLUME_NODE_STAMP	0xA766E		/* A V(olume) N(ode)	*/

#define NUCAM_RANGE 		100		/* NUCAM error codes	*/

/*
 *	NetWare UNIX Client Auto Mounter File System Diagnostics
 */
typedef enum nucamDiag {
	NUCAM_ACCESS_DENIED = NUCAM_RANGE,
	NUCAM_ALLOC_MEM_FAILED,
	NUCAM_ALLOCATE_NODE_FAILED,
	NUCAM_BUSY,
	NUCAM_COPY_IN_FAILED,
	NUCAM_COPY_OUT_FAILED,
	NUCAM_DOT_NODE_NAME,
	NUCAM_INVALID_BLOCK_NUMBER,
	NUCAM_INVALID_DATA,
	NUCAM_INVALID_LOCK,
	NUCAM_INVALID_NAME,
	NUCAM_INVALID_NODE_TYPE,
	NUCAM_INVALID_OFFSET,
	NUCAM_INVALID_SIZE,
	NUCAM_MOUNT_FAILED,
	NUCAM_NODE_ALREADY_EXISTS,
	NUCAM_NODE_IS_DIRECTORY,
	NUCAM_NODE_NOT_FOUND,
	NUCAM_NOT_A_DIRECTORY,
	NUCAM_NOT_SAME_VOLUME,
	NUCAM_OPEN_FAILED,
	NUCAM_VOLUME_IS_READ_ONLY,
	NUCAM_VOLUME_NOT_FOUND
} NUCAM_DIAG_T;

/*
 * NAME
 *    NUCAMattributes - The NUCAM File System name space structure.
 * 
 * DESCRIPTION
 *    This data structure defines the NUCAM File System name space interface
 *    used between AMfi and AMfs layers.
 *
 *    number        - Unique object identifier.
 *    type          - Object type (see above).
 *    permissions   - Object permissions (see above).
 *    numberOfLinks - Object number of links.
 *    size          - Size in bytes of data space.
 *    userID        - User identifier of owner.
 *    groupID       - Group identifier of owner.
 *    accessTime    - Time of last access.
 *    modifyTime    - Time of last data modification.
 *    changeTime    - Time of last name space changed.
 */

/*
 * Define NUCAMattributes structure mask.
 */
typedef struct NUCAMattributes {
	uint32		number;
	AMFS_TYPE_T	type;
	uint32		permissions;
	uint32		numberOfLinks;
	uint32		size;
	uint32		userID;
	uint32		groupID;
	uint32		accessTime;
	uint32		modifyTime;
	uint32		changeTime;
} NUCAM_ATTRIBUTES_T;


/* 
 * NAME
 *    NUCamDirIoArgs - The NUCAM File System directory I/O argument structure.
 *
 * DESCRIPTION
 *    This data structure defines the NetWare UNIX Client Auto Mounter File
 *    System directory I/O argument structre.  This is the interface structure
 *    for use with  AMfsReadDirNodeEntries(3k) to acquire directory entries. 
 *    This data is used in both the AMfi and the AMfs layers of the NUCAM File
 *    System.
 *
 *    buffer       - Pointer to a contiguous buffer to read the directory
 *                   entries into.
 *    bufferLength - Size of nucamIoBuf.buffer.
 *    memoryType   - Set to IOMEM_KERNEL to indicate that the buffer is resident
 *                   in kernel space or IOMEM_USER to indicate that the buffer
 *                   is resident in user space..
 *    searchHandle - Search handle. This is an opaque value that is passed
 *		     to the nucam daemon. It is actually a unique index 
 *		     at which the daemon would begin its next search.
 */

/*
 * Define NUCamIoArgs structure mask.
 */
typedef struct NUCamDirIoArgs {
	char	*buffer;
	int32	bufferLength;
	int32	memoryType;
	int32	searchHandle;
} NUCAM_DIR_IO_ARGS_T;

/* 
 * NAME
 *    AMfsNodeID - The identifier of an object in the NetWare UNIX Client
 *                 Auto Mounter File System.
 *
 * DESCRIPTION
 *    This data structure defines the NetWare UNIX Client Auto Mounter File
 *    System AMfs node object identifier.
 *
 *    internalName - Unique object identifier set to the AMfs node object ID.
 *    externalName - Node name of the AMfs node object.
 */

/*
 * Define AMfsNodeID structure mask.
 */
typedef struct AMfsNodeID {
	int32	internalName;
	char	*externalName;
} AMFS_NODE_ID_T;

extern 	lock_t			*nucam_lockp;
extern	pl_t			savenucpl;
#define	NUCAMPL			plbase
extern	struct vnodeops		amfiVnodeOps;
#define	LOCK_NUCAM_SPIN()	(savenucpl = LOCK(nucam_lockp, NUCAMPL))
#define	UNLOCK_NUCAM_SPIN()	UNLOCK(nucam_lockp, savenucpl)
#define LOCK_AMNODE(aN) 	SLEEP_LOCK(AMFS_SLPLOCK_P((aN)), PRINOD)
#define	UNLOCK_AMNODE(aN) 	SLEEP_UNLOCK(AMFS_SLPLOCK_P((aN)))
#define	LOCK_VP_AMNODE(vp) 	LOCK_AMNODE((vp)->v_data)
#define UNLOCK_VP_AMNODE(vp) 	UNLOCK_AMNODE((vp)->v_data);
#define	TRYLOCK_AMNODE(aN) 	SLEEP_TRYLOCK(AMFS_SLPLOCK_P((aN))) 
#define	TRYLOCK_VP_AMNODE(vp)	TRYLOCK_AMNODE((vp)->v_data) 
#define	AMNODE_LOCKOWNED(aN)	SLEEP_LOCKOWNED(AMFS_SLPLOCK_P((aN)))

#define NUCAM_DIRENT_ALIGN(p)		LALIGN(p)
#define NUCAM_IS_DIRENT_ALIGNED(p)	((char *)(p) == LALIGN(p))

/*
 * Reference NUCAM Virtual File System Interface (AMfi) common
 * operations.
 */
extern	ccode_t	AMfiErrorMap();
extern	void	AMfiGetCredentials();
extern	ccode_t	AMfiGetNodeType();
extern	void	AMfiGetNodePermissions();
extern	ccode_t	AMfsSearchChildList();

#endif				/* _NUCAM_COMMON_H			*/
