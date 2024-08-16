/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucam/nucam_comops.c	1.13"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucam/nucam_comops.c,v 1.10.2.1 1994/12/12 01:08:51 stevbam Exp $"

/*
**  Netware Unix Client Auto Mounter File System
**
**	MODULE:
**		nucam_comops.c -	The NetWare UNIX Clinet Auto Mounter
**					File System Interface layer (AMfi)
**					common operations for VFS/VNODE
**					Generic File System.
**	
**	ABSTRACT:
**		The nucam_comops.c contains the NetWare UNIX Client Auto
**		Mounter File System common operations of the AMfi layer.
**
**		The following operations are contained in this
**		module:
**			AMfiInitVnode ()
**			AMfiErrorMap ()
**			AMfiGetCredentials ()
*/ 



#include <util/types.h>
#include <fs/dirent.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <fs/vfs.h>
#include <fs/fs_hier.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <fs/stat.h>
#include <proc/cred.h>

#include <net/nuc/nwctypes.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <fs/nucam/nucam_common.h>
#include <net/tiuser.h>
#include <fs/nucam/amfs_node.h>
#include <fs/nucam/amfs_ops.h>
#include <fs/nucam/nucam_glob.h>

#include <proc/proc.h>
#include <proc/user.h>

#include <mem/kmem.h>
#include <net/nuc/requester.h>


/*
 * Define the tracing mask.
 */
#define NVLT_ModMask	NVLTM_am

/*
 * Define the golobal variables used between the Virtual File System 
 * Interface layer (AMfi) and the NetWare Client File System layer (AMfs).
 *
 *    nucamType              - NUCAM File System index into UNIX Generic File
 *                             System Switch table.
 *    amfiInitialized        - If set to TRUE indicates that the Virtual File
 *                             System Interface layer (AMfi) has successfully
 *                             been initialized.
 *    amfsNodeCount          - Total number of active AMfs nodes.
 *    nucamMountCount        - Number of NUCAM File System mounted. 
 */
uint16		nucamType;		
boolean_t	amfiInitialized = B_FALSE;
uint32		amfsNodeCount = 0;
uint8		nucamMountCount = 0;




/*
 * BEGIN_MANUAL_ENTRY( NUCamComOpsIntro(3k), \
 *                     ./man/kernel/nucam/amfi/SVr4_1/ComOpsIntro )
 * NAME
 *     NUCamComOpsIntro - Introduction to the common operations used in the AMfi
 *                        layer.
 *
 * SYNOPSIS
 *    #include <nucam_common.h>
 *    #include <amfs_ops.h>
 *    #include <nucam_glob.h" 
 *
 * DESCRIPTION
 *    The NetWare UNIX Client Auto Mounter File System (NUCAM) is broken into
 *    two layer: the Auto Mounter Interface layer (AMfi) and the Auto Mounter
 *    File System layer (AMfs).  The nucam_comops.c contains the AMfi
 *    common operations used both in VNODE and VFS operations.
 *
 * SEE ALSO
 *    AMfiInitVnode(3k), AMfiErrorMap(3k), AMfiGetCredentials(3k), 
 *
 * END_MANUAL_ENTRY
 */


/*
 * BEGIN_MANUAL_ENTRY( AMfiErrorMap(3k), \
 *                     ./man/kernel/nucam/amfi/SVr4_1/ErrorMap )
 * NAME
 *    AMfiErrorMap - Maps a NUCAM error code to a UNIX error code.
 *                                   
 * SYNOPSIS
 *    ccode_t
 *    AMfiErrorMap (diagnostic)
 *    NUCAM_DIAG_T  diagnostic;
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
 *    ENOINIT - The link has been severed.
 *    ENOMEM  - Not enough memory.
 *    ENOSPC  - No space left on device.
 *    ENOTDIR - Not a Directory.
 *    EPERM   - Not super-user.
 *    EROFS   - Read only file system.
 *    EXDEV   - Crossing device link.
 *
 * DESCRIPTION
 *    AMfiErrorMap maps a specified NUCAM File System error code to its 
 *    representation in UNIX semantic.
 *
 * END_MANUAL_ENTRY
 */
ccode_t
AMfiErrorMap (	NUCAM_DIAG_T	diagnostic )
{
	ccode_t	ccode;

	switch (diagnostic) {
	/*
 	 * NUCAM to Generic File System error Mappings.
 	 */
	case NUCAM_ACCESS_DENIED:
		ccode = EACCES;
		break;

	case NUCAM_ALLOC_MEM_FAILED:
		ccode = ENOMEM;
		break;

	case NUCAM_ALLOCATE_NODE_FAILED:
		ccode = ENOMEM;
		break;

	case NUCAM_COPY_IN_FAILED:
		ccode = EFAULT;
		break;

	case NUCAM_COPY_OUT_FAILED:
		ccode = EFAULT;
		break;

	case NUCAM_DOT_NODE_NAME:
		ccode = ENOENT;
		break;

	case NUCAM_INVALID_LOCK:
		ccode = EINVAL;
		break;

	case NUCAM_INVALID_NAME:
		ccode = ENOENT;
		break;

	case NUCAM_INVALID_NODE_TYPE:
		ccode = ENODEV;
		break;

	case NUCAM_INVALID_OFFSET:
		ccode = EINVAL;
		break;

	case NUCAM_INVALID_DATA:
		ccode = EINVAL;
		break;

	case NUCAM_INVALID_SIZE:
		ccode = EINVAL;
		break;

	case NUCAM_MOUNT_FAILED:
		ccode = ENOLINK;
		break;

	case NUCAM_NODE_ALREADY_EXISTS:
		ccode = EEXIST;
		break;

	case NUCAM_NODE_NOT_FOUND:
		ccode = ENOENT;
		break;

	case NUCAM_NODE_IS_DIRECTORY:
		ccode = EISDIR;
		break;

	case NUCAM_NOT_A_DIRECTORY:
		ccode = ENOTDIR;
		break;

	case NUCAM_NOT_SAME_VOLUME:
		ccode = EXDEV;
		break;

	case NUCAM_BUSY:
		ccode = EBUSY;
		break;

	case NUCAM_VOLUME_IS_READ_ONLY:
		ccode = EROFS;
		break;

	case NUCAM_VOLUME_NOT_FOUND:
		ccode = ENODEV;
		break;

	default:
		ccode = EIO;
		break;

	}

	return (ccode);
}


/*
 * BEGIN_MANUAL_ENTRY( AMfiGetCredentials(3k), \
 *                     ./man/kernel/nucam/amfi/SVr4_1/GetCredentials )
 * NAME
 *    AMfiGetCredentials - Populates the credential sturcture with the UNIX
 *                         user ID and the UNIX group ID of the UNIX client 
 *                         user by calling the generic NUC tool credential
 *                         routines.
 *
 * SYNOPSIS
 *    void
 *    AMfiGetCredentials (unixCredentials, nucCredentials)
 *    cred_t	*unixCredentials,
 *    nwcred_t	*nucCredentials)
 *
 * INPUT
 *    unixCredentials->cr_uid - The effective user id of the process making a 
 *                              request.  This represents the credentials of
 *                              the AMfs Client User.
 *    unixCredentials->cr_gid - The effective group id of a process making the
 *                              request.  This represents the UNIX group the 
 *                              AMfs Client User is using.
 *
 * OUTPUT
 *    nucCredentials          - The NUC tool credential handle of the UNIX 
 *                              clinet requesting NetWare UNIX Client Auto 
 *                              Mounter File System operations.
 *
 * DESCRIPTION
 *    The AMfiGetCredentials populates the generic NUC tool credential structure
 *    with the UNIX user ID and UNIX group ID of the UNIX client user requesting
 *    a node or volume operation at the request of the AMfi operations.
 *
 * END_MANUAL_ENTRY
 */
void
AMfiGetCredentials (	cred_t		*unixCredentials,
			nwcred_t	*nucCredentials )
{
	/*
	 * Set the user ID component of the credential structure.
	 */
	NWtlSetCredUserID (nucCredentials, unixCredentials->cr_uid);

	/*
	 * Set the group ID component of the credential structure.
	 */
	NWtlSetCredGroupID (nucCredentials, unixCredentials->cr_gid);

	/*
	 *	Set the Process ID 
	 */
	NWtlSetCredPid (nucCredentials, u.u_procp->p_epid );

        nucCredentials->flags = NWC_OPEN_PUBLIC;

	return;
}


/*
 * BEGIN_MANUAL_ENTRY( AMfiGetNodePermissions(3k), \
 *                     ./man/kernel/nucfs/amfi/SVr4_1/GetNodePermissions )
 * NAME
 *    AMfiGetNodePermissions - Populates the UNIX SVr4 node permissions 
 *                             according to the NetWare node permissions.
 *                                   
 * SYNOPSIS
 *    void
 *    AMfiGetNodePermissions (amfsNodePerms, unixNodePerms)
 *    uint32 amfsNodePerms;
 *    mode_t *unixNodePerms;
 *
 * INPUT
 *    amfsNodePerms - Node permission in NetWare UNIX Client File System 
 *                    semantics.
 *
 * OUTPUT
 *    unixNodePerms - Node permission in UNIX SVR4.X semantics.
 *
 * RETURN VALUES
 *    None.
 *
 * DESCRIPTION
 *    The AMfiGetNodePermissions populates the UNIX SVR4.X Generic Node
 *    permissions according to the NUCAM File System node node permissions. 
 *
 * END_MANUAL_ENTRY
 */
void
AMfiGetNodePermissions (	uint32	amfsNodePerms,
				mode_t	*unixNodePerms )
{
	*unixNodePerms = 0;
	if (amfsNodePerms & AM_OWNER_EXECUTE_BIT)
		*unixNodePerms |= VEXEC;
	if (amfsNodePerms & AM_OWNER_WRITE_BIT)
		*unixNodePerms |= VWRITE;
	if (amfsNodePerms & AM_OWNER_READ_BIT)
		*unixNodePerms |= VREAD;
	if (amfsNodePerms & AM_GROUP_EXECUTE_BIT)
		*unixNodePerms |= VEXEC>>3;
	if (amfsNodePerms & AM_GROUP_WRITE_BIT)
		*unixNodePerms |= VWRITE>>3;
	if (amfsNodePerms & AM_GROUP_READ_BIT)
		*unixNodePerms |= VREAD>>3;
	if (amfsNodePerms & AM_OTHER_EXECUTE_BIT)
		*unixNodePerms |= VEXEC>>6;
	if (amfsNodePerms & AM_OTHER_WRITE_BIT)
		*unixNodePerms |= VWRITE>>6;
	if (amfsNodePerms & AM_OTHER_READ_BIT)
		*unixNodePerms |= VREAD>>6;
	return;
}


/*
 * BEGIN_MANUAL_ENTRY( AMfiGetNodeType(3k), \
 *                     ./man/kernel/nucfs/amfi/SVr4_1/GetNodeType )
 * NAME
 *    AMfiGetNodeType - Sets the UNIX SVR4.X Generic Node node type according to
 *                      the specified NetWare node type.
 *                                   
 * SYNOPSIS
 *    ccode_t
 *    AMfiGetNodeType (amfsNodeType, unixNodeType, diagnostic)
 *    uint32  amfsNodeType;
 *    vtype_t *unixNodeType;
 *    int32   *diagnostic;
 *
 * INPUT
 *    amfsNodeType - Node Type in NUCAM File System semantics.
 *
 * OUTPUT
 *    unixNodeType - Node Type in UNIX SVR4.X semantics.
 *    diagnostic   - Set to one of the following if an error occurs: 
 *                   NUCAM_INVALID_NODE_TYPE
 *
 * RETURN VALUES
 *    0            - Successful completion.
 *    -1           - Unsuccessful completion, diagnostic contains reason.
 *
 * DESCRIPTION
 *    The AMfiGetNodeType sets the UNIX SVR4.X Generic Node file type according
 *    to the specified NUCAM node type at the request of the AMfi operations.
 *
 * END_MANUAL_ENTRY
 */
ccode_t
AMfiGetNodeType (	AMFS_TYPE_T	amfsNodeType,
			vtype_t		*unixNodeType,
			NUCAM_DIAG_T	*diagnostic )
{
	*unixNodeType = 0;
	switch (amfsNodeType) {
	case AM_ROOT:
	case AM_SERVER:
	case AM_VOLUME:
		*unixNodeType = VDIR;
		break;
	case AM_UNKNOWN:
		*unixNodeType = VBAD;
		break;
	default:
		*diagnostic = NUCAM_INVALID_NODE_TYPE;
		return (FAILURE);
	}

	return (SUCCESS);
}


/*
 * BEGIN_MANUAL_ENTRY( AMfiInitVnode(3k), \
 *                     ./man/kernel/nucam/amfi/SVr4_1/AllocateVnode )
 * NAME
 *    AMfiInitVnode - Initializes a new vnode.
 *                                   
 * SYNOPSIS
 *    void
 *    AMfiInitVnode (vfsMountInstance, amfsNode, nodeType)
 *    vfs_t		*vfsMountInstance;
 *    opaque_t		*amfsNode;
 *    vtype_t		nodeType;
 *
 * INPUT
 *    vfsMountInstance - Fully populated VFS structure for the NUCAM VFS AMfs
 *                       root node.
 *    amfsNode         - AMfs node to be bound to the newly allocated vnode
 *                       object.
 *    nodeType         - Set to one of the following:
 *                       VNON  - No type.
 *                       VDIR  - Directory file.
 *
 * OUTPUT
 *    newVnode         - Newly allocated VNODE object.
 *
 * MP Locking:
 *    Any coverage needed for the amfsNode or the component vnode is
 *    arranged by the caller.
 *
 * DESCRIPTION
 *    AMfiInitVnode initializes a new vnode object on the specified 
 *    vfsMountInstance representing (and contained in) the specified amfsNode.
 *
 * NOTE
 *    The only allowed vnode type is VDIR.  The NUCAM File System only needs to
 *    manage directories to represent NetWare servers, and NetWare volumes.
 *
 * END_MANUAL_ENTRY
 */
void
AMfiInitVnode (	vfs_t		*vfsMountInstance,
		AMFS_NODE_T	*amfsNode,
		vtype_t		nodeType )
{
	NVLT_ENTER(3);

	ASSERT(nodeType == VDIR || nodeType == VNON);

	/*
	 * Among other things, VN_INIT sets v_count to 1.
	 */
	VN_INIT (&amfsNode->vnode, vfsMountInstance, nodeType, 
			NULL, 0, KM_SLEEP);
	amfsNode->vnode.v_op = &amfiVnodeOps;
	amfsNode->vnode.v_data = (caddr_t)amfsNode;
	amfsNode->vnode.v_vfsmountedhere = (struct vfs *)NULL;
	amfsNode->vnode.v_filocks = (struct filock *)NULL;
	amfsNode->vnode.v_flag = 0;
	amfsNode->vnode.v_type = VDIR;
	amfsNode->vnode.v_macflag = 0;
	if (nodeType == VNON) {
		amfsNode->vnode.v_flag |= VROOT;
	}
	NVLT_LEAVE(SUCCESS);
	return;
}
