/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nucfs_comops.c	1.10.1.19"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nucfs_comops.c,v 2.64.2.15 1995/02/12 22:29:38 ram Exp $"

/*
**  Netware Unix Client
**
**	MODULE:
**		nucfs_comops.c -	The SVr4 Virtual File System Interface
**					layer (NWfi) common operations.
**	
** +-------------------------------------------------------------+
** |                                                             |
** |             WARNING         WARNING         WARNING         |
** |                                                             |
** |                     !!! COMMENT IS STALE !!!                |
** |                                                             |
** |             WARNING         WARNING         WARNING         |
** |                                                             |
** +-------------------------------------------------------------+
**
**	ABSTRACT:
**		The nucfs_comops.c contains the NetWare UNIX Client File System
**		common operations of the Virtual File System Interface layer
**		(NWfi) for UnixWare.
**
**		The following nwficomops operations are contained in this
**		module:
**			NWfiAllocateVnode ()
**			NWfiErrorMap ()
**			NWfiFreeCredentials ()
**			NWfiGetCredentials ()
**			NWfiGetNodeMode ()
**			NWfiSetNodeMode ()
*/ 

/*
 * XXX: Surpress nested inclusion of <net/nuc/ncpconst.h>, which contains
 *	duplicate defintions with <fs/stat.h>. Also, we must surpress the
 *	inclusion of <net/nuc/ncpiopack.h>, for it depends upon
 *	<net/nuc/ncpconst.h>.
 */

#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <util/types.h>
#include <fs/dirent.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <svc/systm.h>
#include <svc/clock.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <fs/stat.h>
#include <proc/cred.h>

#include <net/nuc/nwctypes.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nucerror.h>
#include <net/tiuser.h>
#include <net/nuc/spilcommon.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <fs/nucfs/nwfschandle.h>
#include <fs/nucfs/nucfscommon.h>
#include <fs/nucfs/nucfsspace.h>
#include <fs/nucfs/nwficommon.h>
#include <fs/nucfs/nwfsops.h>
#include <fs/nucfs/nucfsglob.h>

#include <proc/proc.h>
#include <fs/nucfs/nucfs_tune.h>
#include <fs/nucfs/nwfsvolume.h>
#include <fs/nucfs/nwfsnode.h>
#include <fs/nucfs/nwfidata.h>

#include <proc/proc.h>
#include <proc/user.h>

#include <fs/nucfs/nucfslk.h>

#include <net/nuc/requester.h>

/*
 * Define the tracing mask.
 */
#define NVLT_ModMask	NVLTM_fs

extern void	itimeout_free(void *co);	/* MR entered on clock.h */

/*
 * Define the NUCFS vnode operations list.
 */
extern	struct	vnodeops	nwfiVnodeOps;

/*
 * Internal prototyes.
 */
STATIC void NWfiDelayWakeup(void *);


/*
 * BEGIN_MANUAL_ENTRY( NWfiComOpsIntro(3k), \
 *                     ./man/kernel/nucfs/nwfi/UnixWare/ComOpsIntro )
 * NAME
 *     NWfiComOpsIntro - Introduction to the Virtual File System Interface
 *                       layer (NWfi) common operations (UnixWare).
 *
 * SYNOPSIS
 *    #include <nucfscommon.h>
 *    #include <nucfsspace.h>
 *    #include <nwfsops.h>
 *    #include <nucfsglob.h" 
 *
 * DESCRIPTION
 *    The NetWare UNIX Client File System (NUCfs) is broken into two layer: the
 *    Virtual File System Interface layer (NWfi) and the NetWare Client File
 *    System layer (NWfs).  The NWfi layer handles vnode and vfs operations
 *    of the NetWare UNIX Client File Sytem.  The nwficomops.c contains the NWfi
 *    common operations used both in vnode and vfs operations (UnixWare).
 *
 * SEE ALSO
 *    NWfiErrorMap(3k), NWfiFreeCredentials(3k), NWfiGetCredentials(3k)
 *
 * END_MANUAL_ENTRY
 */


/*
 * BEGIN_MANUAL_ENTRY( NWfiBindVnodeToSnode(3k), \
 *                     ./man/kernel/nucfs/nwfi/UnixWare/NWfiBindVnodeToSnode )
 * NAME
 *    NWfiBindVnodeToSnode - Hold the vnode associated with a netwareNode.
 *		             If no such vnode exists, then allocate one.
 *                                   
 * SYNOPSIS
 *    #include <sys/vnode.h>
 *    #include <sys/vfs.h>
 *    #include <nucfscommon.h>
 *
 *    vnode_t
 *    NWfiBindVnodeToSnode (NWFS_SERVER_NODE_T * netwareNode,
 *			    vfs_t *vfsp)
 *    NWFS_SERVER_NODE_T *netwareNode;
 *
 * INPUT
 *    netwareNode      - NetWare node, whose vnode is to be returned (hard)
 *			 held. The netwareNode is held at entry.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUES
 *    The held vnode.
 *
 * DESCRIPTION
 *    NWfiBindVnodeToSnode holds the vnode associated with a netwareNode (or
 *    possibly the clone vnode associated with a clientHandle structure).
 *    If no such vnode exists, then one is allocated, and it exerts a
 *    hard hold on the snode (or clientHandle structure).
 *
 *    On entry, the caller holds a hard hold on the snode (or clientHandle
 *    structure). This hold is dropped in this function. 
 *
 * END_MANUAL_ENTRY
 */
NWFI_NODE_T *
NWfiBindVnodeToSnode (NWFS_SERVER_NODE_T *snode, vfs_t *vfsp)
{
	vtype_t		unixNodeType;
	ushort_t	flag;
	vnode_t 	*vp, *nvp;

	NVLT_ENTER (2);
	NVLT_ASSERT(SNODE_REASONABLE(snode));
	NVLT_ASSERT(snode->hardHoldCount != 0);

	/*
	 * First, try to hold a pre-existing vnode.
	 *
	 * We must hold the SNODE_LOCK in order to mutex
	 * the gfsNode pointer and the snode hold count.
	 */
	SNODE_LOCK(snode);
	vp = snode->gfsNode;
	if (vp != NULL) {
		/*
		 * Exchange snode hold for a vnode hold. Note that the
		 * vnode and snode already have soft holds on each other.
		 */
		NVLT_ASSERT(vp->v_data == snode);
		NVLT_ASSERT(vp->v_softcnt != 0);
		NVLT_ASSERT(snode->hardHoldCount >= 1);
		NVLT_ASSERT(snode->softHoldCount >= 1);
		VN_LOCK(vp);
		if (vp->v_count == 0) {
			/*
			 * By convention, the vnode exerts a single hard hold
			 * on the snode. We transfer the hard snode hold
			 * (passed in to us by the caller) to become the
			 * hold exerted by the vnode.
			 */
			vp->v_count = 1;
		} else {
			/*
			 * Since the vnode is already exerting a hard hold
			 * on the snode, there is no more need for the snode
			 * hold (passed in to us by the caller). On the other
			 * hand, the caller is requesting a vnode hold.
			 */
			++vp->v_count;
			--snode->hardHoldCount;
		}
		VN_UNLOCK(vp);
		SNODE_UNLOCK(snode);

		NVLT_LEAVE ((uint_t) vp);
		return vp;
	}
	SNODE_UNLOCK(snode);

	/*
	 * Allocate a new vnode. We might have to throw away the
	 * vnode we now allocate if some other LWP is concurrently
	 * executing this code, and is thus racing us for creation
	 * rights.
	 */
	nvp = kmem_zalloc (sizeof (vnode_t), KM_SLEEP);

	/*
	 * Convert Node Type
	 */
	flag = 0;
	switch (snode->nodeType) {
	case NS_FILE:
		unixNodeType = VREG;
		break;
	case NS_ROOT:
		unixNodeType = VDIR;
		flag = VROOT;
		break;
	case NS_DIRECTORY:
		unixNodeType = VDIR;
		break;
	default:
		NVLT_ASSERT(snode->nodeType == NS_SYMBOLIC_LINK);
		unixNodeType = VLNK;
		break;
	}

	/*
	 * Set the rest of the vnode fields.
	 * Note that VN_INIT creates a hard held vnode.
	 * No locking is needed here since nvp is privately held.
	 */
	VN_INIT(nvp, vfsp, unixNodeType, NULL, flag, KM_SLEEP);
	nvp->v_op = &nwfiVnodeOps;
	nvp->v_data = snode;
	nvp->v_softcnt = 1; /* one soft hold for the snode->gfsNode pointer */

	SNODE_LOCK(snode);
	vp = snode->gfsNode;
	if (vp == NULL) {
		/*
		 * We retain the snode (or clientHandle hold) passed in
		 * by the caller, but it is conceptually transferred to
		 * the vnode. However, we need to acquire another soft
		 * hold on the snode for the v_data pointer.
		 */
		vp = nvp;
		snode->gfsNode = nvp;
		SNODE_SOFT_HOLD_L(snode);
		SNODE_UNLOCK(snode);
	} else {
		/*
		 * We lost the creation race. Hold the vnode
		 * created by the other LWP and discard the one
		 * we created.
		 *
		 * Exchange snode hard hold for the vnode hard hold.
		 * Note that the vnode and snode already exert soft holds on
		 * each other.
		 */
		NVLT_ASSERT(vp->v_data == snode);
		NVLT_ASSERT(vp->v_softcnt != 0);
		NVLT_ASSERT(snode->hardHoldCount >= 2);
		NVLT_ASSERT(snode->softHoldCount >= 1);
		VN_LOCK(vp);
		if (vp->v_count == 0) {
			/*
			 * By convention, the vnode exerts a single hard hold
			 * on the snode. We transfer the hard snode hold
			 * (passed in to us by the caller) to become the
			 * hold exerted by the vnode.
			 */
			vp->v_count = 1;
		} else {
			/*
			 * Since the vnode is already exerting a hard hold
			 * on the snode, there is no more need for the snode
			 * hold (passed in to us by the caller). On the other
			 * hand, the caller is requesting a vnode hold.
			 */
			++vp->v_count;
			--snode->hardHoldCount;
		}
		VN_UNLOCK(vp);
		SNODE_UNLOCK(snode);
		VN_DEINIT(nvp);
		kmem_free(nvp, sizeof (vnode_t));
	}

	NVLT_LEAVE ((uint_t) vp);
	return vp;
}

/*
 * The caller passes in a vnode hold on the real vnode, plus
 * a hold on the client handle. Both of these holds are consumed.
 * They might be transferred to the cloneVnode (if it is created now).
 * Otherwise, they will be released.
 */
NWFI_NODE_T *
NWfiBindVnodeToClientHandle (NWFS_CLIENT_HANDLE_T *clientHandle)
{
	NWFS_SERVER_NODE_T	*snode;
	vnode_t 		*vp, *nvp, *rvp;

	NVLT_ENTER (1);

	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));

	snode = clientHandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(snode));

	rvp = snode->gfsNode;
	NVLT_ASSERT(rvp != NULL);
	VN_REASONABLE(rvp);

	/*
	 * First, try to hold a pre-existing vnode.
	 *
	 * We must hold the SNODE_LOCK in order to mutex
	 * the vnode pointer.
	 */
	SNODE_LOCK(snode);
	vp = clientHandle->cloneVnode;
	if (vp != NULL){
		/*
		 * Exchange snode hold (or clientHandle hold) for the vnode
		 * hold. Note that the vnode already holds the clientHandle.
		 */
		NVLT_ASSERT(vp->v_data == clientHandle);
		NVLT_ASSERT(vp->v_softcnt != 0);
		NVLT_ASSERT(clientHandle->holdCount >= 2);
		VN_HOLD(vp);
		--clientHandle->holdCount;
		SNODE_UNLOCK(snode);
		VN_RELE(rvp);

		NVLT_LEAVE ((uint_t) vp);
		return vp;
	}
	SNODE_UNLOCK(snode);

	/*
	 * Allocate a new vnode. We might have to throw away the
	 * vnode we now allocate if some other LWP is concurrently
	 * executing this code, and is thus racing us for creation
	 * rights.
	 */
	nvp = kmem_zalloc (sizeof (vnode_t), KM_SLEEP);

	/*
	 * Set the rest of the vnode fields.
	 * Note that VN_INIT creates a hard held vnode.
	 * No locking is needed here since nvp is privately held.
	 */
	VN_INIT(nvp, rvp->v_vfsp, rvp->v_type, NULL, rvp->v_flag, KM_SLEEP);
	nvp->v_op = &nwfiVnodeOps;
	nvp->v_data = clientHandle;
	nvp->v_softcnt = 1;
		/* one soft hold for the clientHandle->cloneVnode pointer */

	SNODE_LOCK(snode);
	vp = clientHandle->cloneVnode;
	if (vp == NULL) {
		/*
		 * We retain the clientHandle and real vnode holds passed in
		 * by the caller, but they are conceptually transferred to
		 * the clone vnode.
		 */
		vp = clientHandle->cloneVnode = nvp;
		++snode->cloneVnodeCount;
		SNODE_UNLOCK(snode);
	} else {
		/*
		 * We lost the creation race. Hold the vnode
		 * created by the other LWP and discard the one
		 * we created.
		 *
		 * Exchange the clientHandle hold for the vnode
		 * hold. Note that the vnode already holds the clientHandle.
		 * Also drop the callers hold on the real vnode.
		 */
		NVLT_ASSERT(vp->v_data == clientHandle);
		NVLT_ASSERT(vp->v_softcnt != 0);
		NVLT_ASSERT(clientHandle->holdCount >= 2);
		VN_HOLD(vp);
		--clientHandle->holdCount;
		SNODE_UNLOCK(snode);
		VN_RELE(rvp);
		VN_DEINIT(nvp);
		kmem_free(nvp, sizeof (vnode_t));
	}

	NVLT_LEAVE ((uint_t) vp);
	return vp;
}


/*
 * BEGIN_MANUAL_ENTRY( NWfiErrorMap(3k), \
 *                     ./man/kernel/nucfs/nwfi/UnixWare/ErrorMap )
 * NAME
 *    NWfiErrorMap - Maps a NUCFS or a SPIL error code to a UNIX SVR4 error 
 *                   code.
 *                                   
 * SYNOPSIS
 *    #include <types.h>
 *    #include <user.h>
 *    #include <errno.h>
 *    #include <nucfscommon.h>
 *
 *    ccode_t
 *    NWfiErrorMap (diagnostic)
 *    int32	diagnostic;
 *
 * INPUT
 *    diagnostic - Specific diagnotic to be mapped.
 *
 * OUTPUT
 *    Node.
 *
 * RETURN VALUES
 *    EACCES  - Permission denied.
 *    EAGAIN  - No more processes.
 *    EBUSY   - Mount device busy.
 *    EEXIST  - File exists.
 *    EFAULT  - Bad address.
 *    EFBIG   - File too large.
 *    EINVAL  - Invalid argument.
 *    EIO     - I/O error.
 *    EISDIR  - Node is a directory.
 *    EMLINK  - Too many links.
 *    ENODEV  - No such device.
 *    ENOENT  - No such file or directory.
 *    ENOLCK  - No record locks available.
 *    ENOLINK - The link has been severed.
 *    ENOMEM  - Not enough memory.
 *    ENOSPC  - No space left on device.
 *    ENOTDIR - Not a Directory.
 *    EPERM   - Not super-user.
 *    EROFS   - Read only file system.
 *    EXDEV   - Crossing device link.
 *
 * DESCRIPTION
 *    NWfiErrorMap maps a specified NetWare UNIX Client File System or SPIL
 *    error code to it's representation in UNIX SVR4 semantic.
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWfiErrorMap (int32 diagnostic)
{
	ccode_t	ccode;

	NVLT_ENTER(1);

	switch (diagnostic) {

	/*
 	 * NUCFS to Generic File System error Mappings.
 	 */

	case NUCFS_ACCESS_DENIED:
		ccode = EACCES;
		break;

	case NUCFS_ALLOC_MEM_FAILED:
		ccode = ENOMEM;
		break;

	case NUCFS_ALLOCATE_NODE_FAILED:
		ccode = ENOMEM;
		break;

	case NUCFS_COPY_IN_FAILED:
		ccode = EFAULT;
		break;

	case NUCFS_COPY_OUT_FAILED:
		ccode = EFAULT;
		break;

	case NUCFS_DIRECTORY_NOT_EMPTY:
		ccode = EEXIST;
		break;

	case NUCFS_DUP_CRED_FAILED:
		ccode = ENOMEM;
		break;

	case NUCFS_INVALID_LOCK:
		ccode = EINVAL;
		break;

	case NUCFS_INVALID_NAME:
		ccode = ENOENT;
		break;

	case NUCFS_INVALID_NODE_TYPE:
		ccode = ENODEV;
		break;

	case NUCFS_INVALID_OFFSET:
		ccode = EINVAL;
		break;

	case NUCFS_INVALID_DATA:
		ccode = EINVAL;
		break;

	case NUCFS_INVALID_SIZE:
		ccode = EINVAL;
		break;

	case NUCFS_NODE_ALREADY_EXISTS:
		ccode = EEXIST;
		break;


	case NUCFS_NODE_NOT_FOUND:
		ccode = ENOENT;
		break;

	case NUCFS_NODE_IS_DIRECTORY:
		ccode = EISDIR;
		break;

	case NUCFS_NOT_A_DIRECTORY:
		ccode = ENOTDIR;
		break;

	case NUCFS_NOT_SAME_VOLUME:
		ccode = EXDEV;
		break;

	case NUCFS_VOLUME_BUSY:
		ccode = EBUSY;
		break;

	case NUCFS_VOLUME_IS_READ_ONLY:
		ccode = EROFS;
		break;

	case NUCFS_VOLUME_NOT_FOUND:
		ccode = ENODEV;
		break;

	case NUCFS_STALE:
		ccode = ESTALE;
		break;

	case NUCFS_NOT_CHILD:
		ccode = ESTALE;
		break;

	case NUCFS_PROTOCOL_ERROR:
		ccode = EPROTO;
		break;

	/*
 	 * SPIL to Generic File System error Mappings.
 	 */

	case SPI_ACCESS_DENIED:
		ccode = EACCES;
		break;

	case SPI_BAD_BYTE_RANGE:
		ccode = EINVAL;
		break;

	case SPI_CLIENT_RESOURCE_SHORTAGE:	
		ccode = ENFILE;
		break;

	case SPI_DIRECTORY_FULL:
	case SPI_FILE_TOO_BIG:
	case SPI_OUT_OF_DISK_SPACE:
		ccode = ENOSPC;
		break;

	case SPI_DIRECTORY_NOT_EMPTY:
		ccode = EEXIST;
		break;

	case SPI_FILE_ALREADY_EXISTS:
		ccode = EEXIST;
		break;

	case SPI_FILE_IN_USE:
		ccode = EBUSY;
		break;

	case SPI_GENERAL_FAILURE:
		ccode = EIO;
		break;

	case SPI_INVALID_OFFSET:
	case SPI_INVALID_MOVE:
		ccode = EINVAL;
		break;

	case SPI_INVALID_PATH:
		ccode = ENOENT;
		break;

	case SPI_LOCK_COLLISION:
		ccode = EACCES;
		break;

	case SPI_LOCK_SHORTAGE:
		ccode = ENOLCK;
		break;

	case SPI_LOCK_TIMEOUT:
		ccode = EACCES;
		break;

	case SPI_MEMORY_EXHAUSTED:
		ccode = ENOMEM;
		break;

	case SPI_MEMORY_FAILURE:
		ccode = ENOMEM;
		break;

	case SPI_NAME_TOO_LONG:
		ccode = ENAMETOOLONG;
		break;

	case SPI_NODE_IS_DIRECTORY:
		ccode = EISDIR;
		break;

	case SPI_NODE_NOT_DIRECTORY:
		ccode = ENOTDIR;
		break;

	case SPI_NODE_NOT_FOUND:
		ccode = ENOENT;
		break;

	case SPI_NO_PERMISSIONS:
		ccode = EACCES;
		break;

	case SPI_NO_SUCH_DIRECTORY:
		ccode = ENOTDIR;
		break;

	case SPI_NO_SUCH_TASK:
		ccode = EACCES;
		break;

	case SPI_SERVER_RESOURCE_SHORTAGE:
		ccode = ENFILE;
		break;

	case SPI_SERVER_UNAVAILABLE:
	case SPI_AUTHENTICATION_FAILURE:
	case SPI_BAD_CONNECTION:
		ccode = ENOLINK;
		break;

	case SPI_SET_NAME_SPACE_DENIED:
		ccode = EPERM;
		break;

	case SPI_TOO_MANY_LINKS:
		ccode = EMLINK;
		break;

	case SPI_USER_MEMORY_FAULT:
		ccode = EFAULT;
		break;

	case SPI_INTERRUPTED:
		ccode = EINTR;
		break;

	default:
		ccode = EIO;
		break;
	}

	return (NVLT_LEAVE(ccode));
}


/*
 * BEGIN_MANUAL_ENTRY( NWfiGetNodePermissions(3k), \
 *                     ./man/kernel/nucfs/nwfi/UnixWare/GetNodePermissions )
 * NAME
 *    NWfiGetNodePermissions - Populates the UnixWare node permissions 
 *                             according to the NetWare node permissions.
 *                                   
 * SYNOPSIS
 *    #include <sys/vnode.h>
 *    #include <nucfscommon.h>
 *
 *    void_t
 *    NWfiGetNodePermissions (netwareNodePerms, unixNodePerms)
 *    uint32 netwareNodePerms;
 *    mode_t *unixNodePerms;
 *
 * INPUT
 *    netwareNodePerms _ Node permission in NetWare UNIX Client File System 
 *                       semantics.
 *
 * OUTPUT
 *    unixNodePerms    - Node permission in UNIX SVR4 semantics.
 *
 * RETURN VALUES
 *    None.
 *
 * DESCRIPTION
 *    The NWfiGetNodePermissions populates the UNIX SVR4 Generic Inode
 *    permissions according to the NetWare UNIX Client File System node 
 *    node permissions. 
 *
 * END_MANUAL_ENTRY
 */
void
NWfiGetNodePermissions (uint32 netwareNodePerms, mode_t *unixNodePerms)
{
	NVLT_ENTER(2);

	*unixNodePerms = 0;
	if (netwareNodePerms & NS_STICKY_BIT)
		*unixNodePerms |= VSVTX;
	if (netwareNodePerms & NS_SET_UID_BIT)
		*unixNodePerms |= VSUID;
	if (netwareNodePerms & NS_SET_GID_BIT)
		*unixNodePerms |= VSGID;
	if (netwareNodePerms & NS_OWNER_EXECUTE_BIT)
		*unixNodePerms |= VEXEC;
	if (netwareNodePerms & NS_OWNER_WRITE_BIT)
		*unixNodePerms |= VWRITE;
	if (netwareNodePerms & NS_OWNER_READ_BIT)
		*unixNodePerms |= VREAD;
	if (netwareNodePerms & NS_GROUP_EXECUTE_BIT)
		*unixNodePerms |= VEXEC>>3;
	if (netwareNodePerms & NS_GROUP_WRITE_BIT)
		*unixNodePerms |= VWRITE>>3;
	if (netwareNodePerms & NS_GROUP_READ_BIT)
		*unixNodePerms |= VREAD>>3;
	if (netwareNodePerms & NS_OTHER_EXECUTE_BIT)
		*unixNodePerms |= VEXEC>>6;
	if (netwareNodePerms & NS_OTHER_WRITE_BIT)
		*unixNodePerms |= VWRITE>>6;
	if (netwareNodePerms & NS_OTHER_READ_BIT)
		*unixNodePerms |= VREAD>>6;
	NVLT_VLEAVE();
}


/*
 * BEGIN_MANUAL_ENTRY( NWfiSetNodePermissions(3k), \
 *                     ./man/kernel/nucfs/nwfi/UnixWare/SetNodePermissions )
 * NAME
 *    NWfiSetNodePermissions - Populates the NetWare node permissions according
 *                             to the UNIX SVR4 Generic vnode mode permissions.
 *                                   
 * SYNOPSIS
 *    #include <sys/vnode.h>
 *    #include <nwfscommon.h>
 *
 *    void_t
 *    NWfiSetNodePermissions (unixNodePerms, netwareNodePerms)
 *    uint32 unixNodePerms;
 *    uint32 *netwareNodePerms;
 *
 * INPUT
 *    unixNodePerms    - Node permission in UNIX SVR4 semantics.
 *
 * OUTPUT
 *    netwareNodePerms _ Node permission in NetWare UNIX Client File System
 *                       semantics.
 *
 * RETURN VALUES
 *    None.
 *
 * DESCRIPTION
 *    The NWfiSetNodePermissions populates the NetWare UNIX Client File System
 *    node permissions according to the UNIX SVR4 Generic vnode mode permissions
 *    at the request of the NWfi operations.  
 *
 * END_MANUAL_ENTRY
 */
void
NWfiSetNodePermissions (mode_t unixNodePerms, uint32 *netwareNodePerms)
{

	NVLT_ENTER(2);

	*netwareNodePerms = 0;
	if (unixNodePerms & VSVTX)
		*netwareNodePerms |= NS_STICKY_BIT;
	if (unixNodePerms & VSUID)
		*netwareNodePerms |= NS_SET_UID_BIT;
	if (unixNodePerms & VSGID)
		*netwareNodePerms |= NS_SET_GID_BIT;
	if (unixNodePerms & VEXEC)
		*netwareNodePerms |= NS_OWNER_EXECUTE_BIT;
	if (unixNodePerms & VWRITE)
		*netwareNodePerms |= NS_OWNER_WRITE_BIT;
	if (unixNodePerms & VREAD)
		*netwareNodePerms |= NS_OWNER_READ_BIT;
	if (unixNodePerms & VEXEC>>3)
		*netwareNodePerms |= NS_GROUP_EXECUTE_BIT;
	if (unixNodePerms & VWRITE>>3)
		*netwareNodePerms |= NS_GROUP_WRITE_BIT;
	if (unixNodePerms & VREAD>>3)
		*netwareNodePerms |= NS_GROUP_READ_BIT;
	if (unixNodePerms & VEXEC>>6)
		*netwareNodePerms |= NS_OTHER_EXECUTE_BIT;
	if (unixNodePerms & VWRITE>>6)
		*netwareNodePerms |= NS_OTHER_WRITE_BIT;
	if (unixNodePerms & VREAD>>6)
		*netwareNodePerms |= NS_OTHER_READ_BIT;
	NVLT_VLEAVE();
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
 * BEGIN_MANUAL_ENTRY( NWfiSaveDirNode(3k), \
 *                     ./man/kernel/nucfs/nwfi/NeXT2_0/SaveDirNode )
 * NAME
 *    NWfiSaveDirNode - Saves a NetWare directory entry in the UNIX semantics
 *                      in the specified IO vectores.
 *                                   
 * SYNOPSIS
 *    #include <sys/vnode.h>
 *    #include <nucfscommon.h>
 *    #include <dir.h>
 *
 *    ccode_t
 *    NWfiSaveDirEntry (struct direct *genericDirEntry,
 *                      sturct uio    *ioArgs,
 *                      char          *entryName,
 *                      int32         nodeID)
 *
 * INPUT
 *    entryName                 - Name of the NetWare directory entry to be
 *                                saved.
 *    nodeID                    - Unique identifier of the entry.
 *
 * OUTPUT
 *    genericDirEntry->d_off    - Entry offset in the data space of a NetWare
 *                                directory.  It is set to -1 because it has
 *                                no significant in the Generic File System.
 *    genericDirEntry->d_ino    - Unique identifier of the entry. Set to
 *                                specified nodeID.
 *    genericDirEntry->d_reclen - Length of this record.
 *    genericDirEntry->d_name   - Name of the entry.  Set to specified 
 *                                entryName.
 *    ioArgs->uio_iov           - pointer to an array of iovec_t structure(s)
 *                                containing the directory entries read.
 *    ioArgs->uio_iov->iov_base - Incremented by the length of this record.
 *    ioArgs->uio_iov->iov_len  - Decremented by the length of this record.;
 *    ioArgs->uio_iovcnt        - Number of iovec_t sturcture(s).
 *    ioArgs->uio_resid         - Residual count.  Number of bytes read into
 *                                the buffer(s) defined by ioArgs->uio_iov and 
 *                                ioArgs->uio_iovcnt.
 *
 * RETURN VALUES
 *    0               - Successful completion.
 *    -1              - Unsuccessful completion, diagnostic contains reason.
 *
 * DESCRIPTION
 *    The NWfiSaveDirNode converts a NetWare directory entry to UNIX semantic
 *    representation of the entry and saves it in the specified ioArgs. 
 *   
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
ccode_t
NWfiSaveDirEntry (struct dirent *genericDirEntry, struct uio *ioArgs,
		  char *entryName, int32 nodeID, off_t offset)
{
	int nameDisSize, nameLen, dirEntryLength;
	ccode_t error;

	NVLT_ENTER(5);

	/*
	 * Calculate the displacement of d_name in sturct direct.
	 */
	nameDisSize = genericDirEntry->d_name - (char *)genericDirEntry;

	/*
	 * Save the entry name.
	 */
	strncpy(genericDirEntry->d_name, entryName, MAX_NAME_LENGTH);
	genericDirEntry->d_name[MAX_NAME_LENGTH] = '\0';

	/*
	 * Calculate the length of the NetWare directory entry name.
	 */
	nameLen = strlen (genericDirEntry->d_name);

	/*
	 * The following rounds the size of a generic UNIX directory
	 * entry up to the next fullword boundary, if it is not already on
	 * a fullword boundary.
	 */
	dirEntryLength = (nameDisSize + (nameLen + 1) + sizeof(uint32)) &
			~(sizeof(uint32) - 1);

	/*
	 * Make sure we have enough space to copy the new entry.
	 */
	if (dirEntryLength > ioArgs->uio_resid)
		return NVLT_LEAVE(EINVAL);

	/*
	 * Finish directory entry creation.
	 */
	genericDirEntry->d_reclen = dirEntryLength;
	genericDirEntry->d_ino = (nodeID & NWFS_NODE_MASK);
	genericDirEntry->d_off = offset;

	/*
	 * Copy the generic directory entry to the user area.
	 */
	error = uiomove(genericDirEntry, dirEntryLength, UIO_READ, ioArgs);

	return NVLT_LEAVE(error);
}


/*
 * BEGIN_MANUAL_ENTRY( NWfiSetNodeType(3k), \
 *                     ./man/kernel/nucfs/nwfi/UnixWare/SetNodeType )
 * NAME
 *    NWfiSetNodeType - Sets the NetWare UNIX Client File System's node type
 *                      according to the specified UNIX SVR4 Generic vnode type.
 *                                   
 * SYNOPSIS
 *    ccode_t
 *    NWfiSetNodeType (unixNodeType, netwareNodeType, diagnostic)
 *    vtype_t unixNodeType;
 *    uint32  *netwareNodeType;
 *    int32   *diagnostic;
 *
 * INPUT
 *    unixNodeType    - Node Type in UNIX SVR4 semantics.
 *
 * OUTPUT
 *    netwareNodeType - Node Type in NetWare UNIX Client File System semantics.
 *    diagnostic      - Set to one of the following if an error occurs:
 *                      NUCFS_INVALID_NODE_TYPE
 *
 * RETURN VALUES
 *    0               - Successful completion.
 *    -1              - Unsuccessful completion, diagnostic contains reason.
 *
 * DESCRIPTION
 *    The NWfiSetNodeType sets the NetWare UNIX Client File System's node type 
 *    according to the specified UNIX SVR4 Generic Inode file type.
 *   
 * END_MANUAL_ENTRY
 */
ccode_t
NWfiSetNodeType (vtype_t unixNodeType, int32 *netwareNodeType,
		 enum NUC_DIAG *diagnostic)
{
	NVLT_ENTER(3);

	switch (unixNodeType) {
	case VREG:
		*netwareNodeType = NS_FILE;
		break;
	case VDIR:
		*netwareNodeType = NS_DIRECTORY;
		break;
	case VLNK:
		*netwareNodeType = NS_SYMBOLIC_LINK;
		break;
	default:
		*diagnostic = NUCFS_INVALID_NODE_TYPE;
		return (FAILURE);
	}

	return NVLT_LEAVE(SUCCESS);
}

/*
 *	Synchronize the attributes/file size of the specified server node
 *	with the server.
 *
 *	The RW lock is held in writer mode by the caller.
 *
 * PERF: We should investigate if it might be possible for this routine
 *	 to not require the RW lock.
 */
ccode_t
NWfiSyncWithServer(NWFS_SERVER_NODE_T *serverNode, int force,
	NWFS_CRED_T *credentials, enum NUC_DIAG *diagnostic)
{
	NWFS_CLIENT_HANDLE_T	*baseHandle, *clientHandle;
	NWFS_CLIENT_HANDLE_T	*myIdClientHandle = NULL;
	int			locallyModified, locallyExtended;
	int			mustInvalidate = 0;
	vnode_t			*vp = NULL;
	ccode_t			retCode = SUCCESS;
	int			error;

	NVLT_ENTER(4);

	NVLT_ASSERT(SNODE_REASONABLE(serverNode));

	/*
	 * If we have a vnode, then hold it. If we are executing in
	 * an async daemon, then we might not have a vnode (because
	 * it may be destroyed before we acquire the SNODE_LOCK).
	 */
	SNODE_LOCK(serverNode);
	if ((vp = serverNode->gfsNode) != NULL) {
		VN_REASONABLE(vp);
		NVLT_ASSERT(!VN_IS_RELEASED(vp));
		NVLT_ASSERT(vp->v_pages ? serverNode->nodeType == NS_FILE : B_TRUE);
		VN_SOFTHOLD(vp);
	}
	SNODE_UNLOCK(serverNode);

	/*
	 * If our caller has credentials, then get the client handle.
	 */
	if (credentials != NULL)
		myIdClientHandle = NWfsGetClientHandle(serverNode, credentials);

	/*
	 * Save the SNODE_MODIFY bit.
	 */
	SNODE_LOCK(serverNode);
	locallyModified = (serverNode->nodeFlags & SNODE_CL_MODIFY);
	locallyExtended = (serverNode->nodeFlags & SNODE_CL_EXTEND);
	serverNode->nodeFlags &= ~(SNODE_CL_MODIFY|SNODE_CL_EXTEND);
	force |= (locallyModified|locallyExtended);

	/*
	 * For regular files we need to clear the pages in order to:
	 *	=> Extend the file on the server to our local length, and
	 *	=> Update the modify time on the server.
	 */
	if (serverNode->nodeType == NS_FILE) {
		/*
		 * Scan the dirty/fault bits in the client handles
		 * associated with with this server node. The fault bits will
		 * be synced into the dirty bits (for the benefit of the page
		 * push operations).
		 */
		baseHandle = &serverNode->clientHandle;
		clientHandle = baseHandle;
		do {
			NVLT_ASSERT(
				clientHandle->handleState == SNODE_CHANDLE ||
				clientHandle->handleState == SNODE_EHANDLE ||
				clientHandle->handleState == SNODE_MARKER);

			/*
			 * Skip over list markers and an invalid embedded
			 * client handle.
			 *
			 * Sync the fault bits to the 
			 */
			if (clientHandle->handleState == SNODE_CHANDLE &&
			    (clientHandle->clientHandleFlags &
					NWCH_WRITE_FAULT)) {
				clientHandle->clientHandleFlags |=
							NWCH_DATA_DIRTY;
				clientHandle->clientHandleFlags &=
							~NWCH_WRITE_FAULT;
				locallyModified = TRUE;
				force = TRUE;
			}

			/*
			 * Onto the next client handle.
			 */
			clientHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
				clientToClientChain(clientHandle)));
		} while (clientHandle != baseHandle);

		SNODE_UNLOCK(serverNode);

		if (vp != NULL) {
			if (locallyModified) {
				/*
				 * We pass sys_cred in to nuc_doputpage
				 * since we don't really know the identity of
				 * the writer to each page. The closest
				 * approximation to this information is
				 * recorded in the NWCH_DATA_DIRTY bits in
				 * the client handles.
				 */
				error = pvn_getdirty_range(nuc_doputpage, vp,
						0, 0, 0, 0, serverNode->vmSize,
						0, sys_cred);

				/*
				 * If the writes failed, then we cannot
				 * take no for an answer (since the system
				 * would deadlock if uncleanable pages were
				 * to accumulate). So try again, and abort
				 * any pages which cannot be pushed.
				 */
				if (error) {
					/*
					 *+ Updated data for a NUCFS file
					 *+ could mnote be written to the
					 *+ server. The error code is one of
					 *+ those defined in <errno.h>. The
					 *+ potential exists for the loss of
					 *+ data. To correct: check for disk
					 *+ errors on the NetWare server, out
					 *+ of space problems on the NetWare
					 *+ server, network link problems, and
					 *+ network routing problems.
					 */
					cmn_err(CE_WARN,
					    "NUCFS write error (%d): volume "
					    "%s node %d\n", error,
					    serverNode->nodeVolume->volumeName,
					    serverNode->nodeNumber &
						    NWFS_NODE_MASK);
					mustInvalidate = 2;
					retCode = EIO;
					goto sync_error;
				}

				/*
				 * If the file is mapped, then we need to
				 * unload translations now (so as to force a
				 * new fault when an LWP touches a page -
				 * thus setting the NWCH_WRITE_FAULT bit).
				 * Theoretically, this could unload a
				 * translation on segmap, causing to to fault
				 * a second time (something which is
				 * illegal). However, since we are holding
				 * the RW lock in writer mode, we know that
				 * this cannot happen.
				 */
				if (serverNode->r_mapcnt != 0)
					pvn_unload(vp);
			}
		}

		/*
		 * Now that all writes are complete to the server, we can
		 * clear the NWCH_DATA_DIRTY bits in the snode.
		 */
		SNODE_LOCK(serverNode);
		clientHandle = baseHandle;
		do {
			NVLT_ASSERT(
				clientHandle->handleState == SNODE_CHANDLE ||
				clientHandle->handleState == SNODE_EHANDLE ||
				clientHandle->handleState == SNODE_MARKER);

			/*
			 * Skip over list markers and an invalid embedded
			 * client handle.
			 */
			if (clientHandle->handleState == SNODE_CHANDLE) {
				clientHandle->clientHandleFlags &=
							~NWCH_DATA_DIRTY;
			}

			/*
			 * Onto the next client handle.
			 */
			clientHandle = clientChainToClient(NWFI_NEXT_ELEMENT(
				clientToClientChain(clientHandle)));
		} while (clientHandle != baseHandle);
	}

	/*
	 * Fetch new attributes from the server if any of the following
	 * are true:
	 *	=> the cached attributes are stale (timed out)
	 *	=> the cached attributes are stale (fetched without handle)
	 *	   and our caller has the file open
	 * 	=> we have pushed data to the server since the last sync
	 *	=> the file was modified in some other way
	 *	=> the caller wants to force the fetch of new attributes
	 */
	while (force || !(serverNode->nodeFlags & SNODE_AT_VALID) ||
	       NUCFS_STALE_ATTRIBS(&serverNode->cacheInfo.beforeTime) ||
	       (serverNode->nodeType == NS_FILE &&
	        serverNode->cacheInfo.stale && myIdClientHandle != NULL &&
		myIdClientHandle->cloneVnode != NULL)) {

		/*
		 * Don't bother with a stale node.
		 */
		if (serverNode->nodeState == SNODE_STALE) {
			SNODE_UNLOCK(serverNode);
			*diagnostic = NUCFS_STALE;
			retCode = FAILURE;
			goto sync_error;
		}

		SNODE_UNLOCK(serverNode);

		if (NWfsGetAttribsById(serverNode, credentials,
					   diagnostic) != SUCCESS) {
			/*
			 * Failed to get the attributes from the
			 * server. Since we have no knowledge of what
			 * is going on over there, we just invalidate
			 * all our pages.
			 */
			mustInvalidate = 2;
			retCode = FAILURE;
			goto sync_error;
		}
		force = FALSE;
		SNODE_LOCK(serverNode);
	}

	/*
	 * XXX: With the information we currently get from the
	 *	server, we are simply unable to determine if a time stamp
	 *	modification is due to writes this client sent to the
	 *	server, or due to writes from some other client. Also, the
	 *	modify time stamp granularity is too coarse (1 second) to
	 *	detect all changes. In the future, some combination of
	 *	callbacks from the server and the file modification
	 *	``generation count'', might help to disambiguate this. But,
	 *	for now, we are simply stuck.
	 *
	 * We could choose to error on the side of safety, and assume that
	 * all changes in modify time were caused by writes from some other
	 * client. However, this would have negative performance consequence,
	 * for it would cause each writing client to abort its own pages!
	 *
	 * So therefore, we chose to error on the side of performance, and
	 * hence we ignore modify time changes on the server if we have been
	 * recently writing. This means that if two clients simultaneously
	 * write to one file, there is some chance that their changes will
	 * not propogate to each other in any reasonable time.
	 */
	if (serverNode->nodeType == NS_FILE) {
		/*
		 * If we don't have the credentials to get the true file size,
		 * and the file is open locally, then just restore the
		 * modified and exteded bits, and allow the authenticated
		 * users to do the job later.
		 */
		if (serverNode->cacheInfo.stale &&
		    serverNode->cloneVnodeCount != 0) {
			if (locallyExtended)
				SNODE_SET_MODIFIED_L(serverNode);
			if (locallyModified)
				SNODE_SET_EXTENDED_L(serverNode);
		} else {
			if ((NWFI_OFF_T) serverNode->nodeSize !=
			    serverNode->vmSize ||
			    ((serverNode->nodeFlags & SNODE_SV_SIZE_CHANGE) &&
			     !locallyExtended) ||
			    ((serverNode->nodeFlags & SNODE_SV_MODIFY_CHANGE) &&
			     !locallyModified)) {
				serverNode->vmSize = serverNode->postExtSize =
					(NWFI_OFF_T) serverNode->nodeSize;
				mustInvalidate = 1;
			}
			serverNode->nodeFlags &=
				~(SNODE_SV_SIZE_CHANGE|SNODE_SV_MODIFY_CHANGE);
		}
	} else {
		serverNode->vmSize = serverNode->postExtSize =
					(NWFI_OFF_T) serverNode->nodeSize;
	}
	SNODE_UNLOCK(serverNode);

sync_error:
	if (vp) {
		if (mustInvalidate) {
			/*
			 * Some other client has apparently modified the
			 * data. So therefore, we invalidate all the pages we
			 * hold.
			 *
			 * XXX: The use of B_INVAL is inconsistent with
			 *	mlock(2) guarantees under the UW2.0 kernel.
			 *	This problem was introduced with ES/MP.
			 *	Previous (SRV4.0 derivative) kernels had
			 *	the reverse problem (i.e. a single mlock(2)ing
			 *	process would cause all other readers to view
			 *	a stale image). No kernel has yet to get this
			 *	right!
			 */
			pvn_getdirty_range(nuc_doputpage, vp, 0, 0, 0, 0,
					   serverNode->vmSize, B_INVAL,
					   sys_cred);
		}
		VN_SOFTRELE(vp);
	}
	if (mustInvalidate == 2)
		NWfsInvalidateNode(serverNode);
	if (myIdClientHandle != NULL)
		NWfsReleaseClientHandle(myIdClientHandle);

	return NVLT_LEAVE(retCode);
}

/*
 *	Truncate a file down to size.
 *
 *	Called with the RW lock held in WRITER mode and returns that way.
 */
ccode_t
NWfiTruncateBytesOnNode (NWFS_CRED_T *credentials,
			 NWFS_SERVER_NODE_T *fileNode, off_t truncateOffset,
			 enum NUC_DIAG *diagnostic)
{
	NWFS_CLIENT_HANDLE_T *clientHandle;
	off_t nextPage;
	size_t zeroSize, oldSize;
	int error;
	vnode_t *vp;

	NVLT_ENTER(4);

	/*
	 * Hopefully, there is nothing to do!
	 */
	if (truncateOffset == (oldSize = fileNode->vmSize))
		return NVLT_LEAVE(SUCCESS);

	/*
	 * Before sending the new size to the server, we do some local
	 * bookkeeping prior. First, adjust the local file size. This
	 * will have the effect of stopping pages beyond EOF from being
	 * pushed to the server (when truncating down). When truncating
	 * up, it will allow pages to be created beyond old EOF.
	 */
	SNODE_LOCK(fileNode);
	fileNode->vmSize = fileNode->postExtSize = truncateOffset;

	/*
	 * Mark the file as locally extended.
	 */
	SNODE_SET_EXTENDED_L(fileNode);

	/*
	 * Abort pages beyond new EOF.
	 */
	if ((vp = fileNode->gfsNode) != NULL) {
		NVLT_ASSERT(!VN_IS_RELEASED(vp));
		VN_SOFTHOLD(vp);
		SNODE_UNLOCK(fileNode);

		if (oldSize > truncateOffset)
			pvn_abort_range(vp, truncateOffset, 0);

		/*
		 * Clear trailing bytes in the new last page of the file.
		 * This works in the trunc down case because the nucfs
		 * getpage routine never extends the file size.
		 * pvn_trunczero() is kind of overkill here, since the the
		 * backing store (the server) doesn't need any help in
		 * zeroing bytes beyond EOF.
		 *
		 * In the trunc-up case, the pvn_trunczero() cannot fail,
		 * since it will never actually read from the server. In the
		 * trunc-down case we can safely ignore errors because the
		 * server doesn't need our help in zeroing its backing store.
		 */
		nextPage = ptob(btopr(truncateOffset));
		zeroSize = nextPage - truncateOffset;
		if (zeroSize != PAGESIZE && zeroSize != 0) {
			/*
			 * We record that we are a reader in the client
			 * handle. This is for the benefit of our getpage
			 * routine, which is presented with ``sys_cred'' by
			 * segmap, but which needs to present the NetWare
			 * server with the credentials of some actual user.
			 * See function NWfsAttachClientHandle() to see how
			 * this is dealt with inside the fault.
			 *
			 * We can assert that clientHandle->readersCount is 0
			 * because we hold the RW lock in writer mode.
			 *
			 * We also set the NWCH_DATA_DIRTY bit for the
			 * benefit of the putpage routine (which is also
			 * presented with sys_cred) and needs to spot the
			 * writing user.
			 */
			clientHandle = NWfsGetClientHandle(fileNode,
							   credentials);
			SNODE_LOCK(fileNode);
			NVLT_ASSERT(clientHandle->readersCount == 0);
			clientHandle->readersCount = 1;
			clientHandle->clientHandleFlags |= NWCH_DATA_DIRTY;
			SNODE_UNLOCK(fileNode);

			pvn_trunczero(vp, truncateOffset, zeroSize);

			SNODE_LOCK(fileNode);
			NVLT_ASSERT(clientHandle->readersCount == 1);
			clientHandle->readersCount = 0;
			SNODE_SET_MODIFIED_L(fileNode);
			SNODE_UNLOCK(fileNode);
			NWfsReleaseClientHandle(clientHandle);

			/*
			 * The dirty page we just created will extend the file
			 * for us. It is the caller's responsibility to push
			 * it out to the server.
			 */
			if (truncateOffset > oldSize)
				return NVLT_LEAVE(SUCCESS);
		}

		/*
		 * We need to force all pending writes out to the server,
		 * so that a previously queued extending write does not
		 * clobber the the file size we are about to send in
		 * NWfsTruncateBytesOnNode.
		 */
		error = pvn_getdirty_range(nuc_doputpage, vp, 0, 0, 0,
				0, fileNode->vmSize, 0, sys_cred);
		if (error) {
			*diagnostic = NUCFS_EIO;
			return NVLT_LEAVE(FAILURE);
		}

		VN_SOFTRELE(vp);
	} else {
		SNODE_UNLOCK(fileNode);
	}

	/*
	 * Now, tell the server to truncate the file to the desired size.
	 */
	if (NWfsTruncateBytesOnNode (credentials, fileNode, truncateOffset,
				     diagnostic) != SUCCESS) {
		return NVLT_LEAVE(FAILURE);
	}

	/*
	 * If truncating the file down in size, then we must close the
	 * the file in order to have the truncate really take effect on
	 * the NetWare server.
	 *
	 * XXX: If another client has the file open, then the truncate
	 *	won't really occur.
	 */
	if (truncateOffset < oldSize) {
		if (NWfsCloseAllHandles(fileNode, diagnostic) != SUCCESS)
			return NVLT_LEAVE(FAILURE);
		NWfsAllowResourceHandlesToOpen(fileNode);
	}

	/*
	 * It will be the responsibility of the caller to sync
	 * attributes with the server.
	 */

	return (NVLT_LEAVE (SUCCESS));
}

void
NWfiStaleNode(NWFS_SERVER_NODE_T *snode, int cansleep)
{
	vnode_t	*vp;
	int n;

	NVLT_ENTER(2);

	SNODE_LOCK(snode);
	if ((vp = snode->gfsNode) != NULL) {
		VN_SOFTHOLD(vp);
		SNODE_UNLOCK(snode);
		n = 1;

		if (cansleep)
			pvn_abort_range(vp, 0, 0);
		else
			pvn_abort_range_nosleep(vp, 0, 0);

		/*
		 * If possible, unbind the vnode form the server node.
		 */
		SNODE_LOCK(snode);
		VN_LOCK(vp);
		if (vp->v_pages == NULL && vp->v_count == 0) {
			NVLT_ASSERT(vp->v_softcnt >= 2);
			vp->v_data = NULL;
			snode->gfsNode = NULL;
			n = 2;
		}
		VN_UNLOCK(vp);
		SNODE_UNLOCK(snode);
		VN_SOFTRELE(vp);
		if (n == 2) {
			VN_SOFTRELE(vp);
			SNODE_SOFT_RELEASE(snode);
		}
	} else {
		SNODE_UNLOCK(snode);
	}
}

/*
 * Return an indication of whether a server node is active in the
 * generic file system.
 *
 * Thus function is designed to be called form unlink(2) and rmdir(2),
 * wherein the generic file system own no holds on the vnode, but the
 * NWfi layer owns 1 hold.
 */

int
NWfiActiveNode(NWFS_SERVER_NODE_T *snode, int count)
{
	vnode_t	*vp;
	int	ret = 0;

	NVLT_ENTER(2);

	NVLT_ASSERT(SNODE_REASONABLE(snode));

	SNODE_LOCK(snode);
	vp = snode->gfsNode;
	if (vp != NULL) {
		NVLT_ASSERT(vp->v_data == snode);
		VN_LOCK(vp);
		ret = (vp->v_count > count);
		VN_UNLOCK(vp);
	}
	SNODE_UNLOCK(snode);

	return NVLT_LEAVE(ret);
}

/*
 * Return an indication of whether a client handle is open by a user
 * of the generic file system.
 */

int
NWfiIsOpenFile(NWFS_CLIENT_HANDLE_T *clientHandle)
{
	NWFS_SERVER_NODE_T	*snode;
	int			ret;

	NVLT_ENTER(1);

	NVLT_ASSERT(CHANDLE_REASONABLE(clientHandle));
	snode = clientHandle->snode;
	NVLT_ASSERT(SNODE_REASONABLE(snode));

	SNODE_LOCK(snode);
	ret = (clientHandle->cloneVnode != NULL);
	SNODE_UNLOCK(snode);

	return NVLT_LEAVE(ret);
}

/*
 * boolean_t
 * NWfiDelay(long ticks)
 *
 * 	Sleep for a certain number of ticks, but be prepared to
 *	receive a signal.
 *
 * Calling/Exit State:
 *
 *	Caller must not be holding locks, as we sleep.
 *	Returns B_TRUE if the specified interval has been slept for or
 *	B_FALSE if premature wakeup was generated due to a signal.
 *
 * Description:
 *
 *	We allocate an event, set a callout against this event and
 *	sleep on the event.  The callout will fire and wake us up.
 *	Since we allocate space while sleeping, we snapshot the target
 *	wakeup-time before sleeping and set the callout for the appropriate
 *	number of ticks left after we've allocated all the necessary
 *	resources.
 */
boolean_t
NWfiDelay(long ticks)
{
	event_t *eventp;
	clock_t sample;
	long diff;
	void *co;
	toid_t timeOutId;
	boolean_t retValue;

	NVLT_ENTER(1);

	if (ticks <= 0)
		return NVLT_LEAVE(B_TRUE);

	sample = lbolt;
	eventp = EVENT_ALLOC(KM_SLEEP);
	co = itimeout_allocate(KM_SLEEP);

	TIME_LOCK();

	/*
	 * Diff is the number of ticks which have passed while we waited
	 * for memory.
	 */
	diff = lbolt - sample;
	if (diff < 0) {
		/*
		 * Lbolt rolled over while we were allocating.
		 */
		diff = -diff;
	}

	if (ticks > diff) {
		/*
		 * Haven't delayed greater than ticks allocating the
		 * necessary memory.
		 */
		timeOutId = itimeout_l_a(NWfiDelayWakeup, eventp,
					 ticks - diff, PLBASE, co);
		TIME_UNLOCK();
		retValue = EVENT_WAIT_SIG(eventp, PRIVFS);
		if (!retValue)
			untimeout(timeOutId);
	} else {
		/*
		 * Delayed greater then ticks just getting memory.  Don't
		 * have to worry about doing the timeout (i.e. we blew the
		 * time allocating memory).
		 */
		TIME_UNLOCK();
		itimeout_free(co);
		retValue = B_TRUE;
	}

	EVENT_DEALLOC(eventp);

	return NVLT_LEAVE(retValue);
}

/*
 * void NWfiDelayWakeup(void *eventp)
 *
 * 	Activate someone doing a delay.
 *
 * Calling/Exit State:
 *
 *	Called from callout dispatch.  Simply signals the event the
 *	waiter is blocked on.
 */
STATIC void
NWfiDelayWakeup(void *eventp)
{
	NVLT_ENTER(1);

	EVENT_SIGNAL((event_t *)eventp, 0);

	NVLT_VLEAVE();

	return;
}
