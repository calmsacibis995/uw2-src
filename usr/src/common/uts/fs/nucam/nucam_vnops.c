/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucam/nucam_vnops.c	1.18"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucam/nucam_vnops.c,v 1.20.2.3 1995/01/25 15:39:51 mdash Exp $"

/*
**  NetWare Unix Client Auto Mounter File System
**
**	MODULE:
**		nucam_vnodeops.c -	The NetWare UNIX Client Auto Mounter
**					File System Interface layer (AMfi)
**					node operations for VFS/VNODE
**					Generic File Systems.
**
**	ABSTRACT:
**		The nucam_vnodeops.c contains the NetWare UNIX Client Auto 
**		Mounter File System node operations of the AMfi layer for the 
**		VFS/VNODE Architecture Kernels. This layer binds the 
**		portable file system (AMfs) layer of the NetWare UNIX Client
**		Auto Mounter File System into the UNIX Generic File System as a
**		dependent file system according to the archtiecture and
**		semantics of the VFS/VNODE.  See AMfiVnodeOpsIntr(3K) for
**		a complete description of these operations.
**
**		The following AMfiVnodeOps (struct vnodeops) operations are
**		contained in this module:
**			AMfiCloseNode ()
**			AMfiCheckAccess ()
**			AMfiGetAttributes ()
**			AMfiLookUpNodeByName ()
**			AMfiOpenNode ()
**			AMfiReadDirNodeEntries ()
**			AMfiReleaseNode ()
**			AMfiNucFsMount()
**
*/ 

#include <fs/buf.h>
#include <fs/dirent.h>
#include <fs/mount.h>
#include <fs/stat.h>
#include <fs/fs_hier.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/pathname.h>
#include <fs/file.h>
#include <io/uio.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <mem/kmem.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <fs/fs_subr.h>

#include <net/tiuser.h>
#include <net/nuc/nwctypes.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <fs/nucfs/nwficommon.h>
#include <fs/nucam/nucam_common.h>
#include <fs/nucam/amfs_ops.h>
#include <fs/nucam/nucam_glob.h>
#include <fs/nucam/amfs_node.h>
#include <fs/fs_hier.h>

#include <io/NWam/nwam.h>

/* 
 * Define the tracing mask;
 */
#define NVLT_ModMask	NVLTM_am

STATIC	int	AMfiNucFsMount(AMFS_NODE_T *, cred_t *, nwcred_t *, 
			AMFS_NODE_T **);

/*
 * Reference NUCAM Virtual File System Interface (AMfi) vnode operations.
 */
STATIC	int	AMfiCheckAccess();
STATIC	int	AMfiCloseNode();
STATIC	int	AMfiGetAttributes();
STATIC	int	AMfiLookUpNodeByName();
STATIC	int	AMfiOpenNode();
STATIC	int	AMfiReadDirNodeEntries();
STATIC	void	AMfiReleaseNode();
STATIC	int	AMfiSeek();


/*
 * NUCAM VNODE Operations.
 */
struct	vnodeops	amfiVnodeOps = {
	AMfiOpenNode, 				/* vop_open		*/
	AMfiCloseNode,				/* vop_close		*/
	(int (*)())fs_nosys,			/* vop_read		*/
	(int (*)())fs_nosys,			/* vop_write		*/
	(int (*)())fs_nosys,			/* vop_ioctl		*/
	(int (*)())fs_nosys,			/* vop_setfl		*/
	AMfiGetAttributes,			/* vop_getattr		*/
	(int (*)())fs_nosys,			/* vop_setattr		*/
	AMfiCheckAccess,			/* vop_access		*/
	AMfiLookUpNodeByName,			/* vop_lookup		*/
	(int (*)())fs_nosys,			/* vop_create		*/
	(int (*)())fs_nosys,			/* vop_remove		*/
	(int (*)())fs_nosys,			/* vop_link		*/
	(int (*)())fs_nosys,			/* vop_rename		*/
	(int (*)())fs_nosys,			/* vop_mkdir		*/
	(int (*)())fs_nosys,			/* vop_rmdir		*/
	AMfiReadDirNodeEntries,			/* vop_readdir		*/
	(int (*)())fs_nosys,			/* vop_symlink		*/
	(int (*)())fs_nosys,			/* vop_readlink		*/
	(int (*)())fs_nosys,			/* vop_fsync		*/
	AMfiReleaseNode,			/* vop_inactive		*/
        (void (*)())fs_nosys,                   /* vop_release          */
	(int (*)())fs_nosys,			/* vop_fid		*/
	fs_rwlock,				/* vop_rwlock		*/
	fs_rwunlock,				/* vop_rwunlock		*/
	AMfiSeek,				/* vop_seek		*/
	(int (*)())fs_cmp,			/* vop_cmp		*/
	(int (*)())fs_nosys,			/* vop_frlock		*/
	(int (*)())fs_nosys,			/* vop_realvp		*/
	(int (*)())fs_nosys,			/* vop_getpage		*/
	(int (*)())fs_nosys,			/* vop_putpage		*/
	(int (*)())fs_nosys,			/* vop_map		*/
	(int (*)())fs_nosys,			/* vop_addmap		*/
	(int (*)())fs_nosys,			/* vop_delmap		*/
	(int (*)())fs_nosys,			/* vop_poll		*/
	(int (*)())fs_nosys,			/* vop_pathconf		*/
	(int (*)())fs_nosys,			/* vop_getacl		*/
	(int (*)())fs_nosys,			/* vop_setacl		*/
	(int (*)())fs_nosys,			/* vop_setlevel		*/
	(int (*)())fs_nosys,			/* vop_getdvstat	*/
	(int (*)())fs_nosys,			/* vop_setdvstat	*/
	(int (*)())fs_nosys,			/* vop_makemld		*/
        (int (*)())fs_nosys,                    /* vop_testmld          */
        (int (*)())fs_nosys,                    /* vop_stablestore      */
        (int (*)())fs_nosys,                    /* vop_relstore         */
        (int (*)())fs_nosys,                    /* vop_getpagelist      */
        (int (*)())fs_nosys,                    /* vop_putpagelist      */
        (int (*)())fs_nosys,                    /* vop_msgio            */
        (int (*)())fs_nosys,                    /* filler[4]...         */
        (int (*)())fs_nosys,                    /* filler               */
        (int (*)())fs_nosys,                    /* filler               */
        (int (*)())fs_nosys                     /* filler               */
};


/*
 * BEGIN_MANUAL_ENTRY(AMfiVnodeOpsIntro(3K), \
 *		./man/kernel/nucam/amfi/SVR4.X/VnodeOpsIntro )
 * NAME
 *     AMfiVnodeOpsIntro - Introduction to the AMfi layer node operations.
 *
 * SYNOPSIS
 *    #include <cmn_err.h>
 *    #include <nwctypes.h>
 *    #include <nucam_common.h>
 *    #include <amfs_ops.h>
 *    #include <nucam_glob.h>
 *    #include <nwctrace.h>
 *
 * DESCRIPTION
 *    The NetWare UNIX Client Auto Mounter File System (NUCAM) is broken into
 *    two layer: the Auto Mounter Interface (AMfi) layer and the Auto Mounter
 *    File System (AMfs) layers.  The nucam_vnodeops.c contains the AMfi vnode
 *    operations.
 *
 *    The NUCAM File System is designed to automatically mount network File
 *    Systems such a NUCFS on directories within the NUCAM File System.
 *    Directories are the only node type supported by the NUCAM File System.
 *    Each NUCAM File System is a sub-tree of the hierachical AMfs nodes.  The
 *    root of the NUCAM File System (AMFS_NODE_T) is paired with the UNIX
 *    Generic File System mount structure to form the mount point to the NUCAM
 *    File System.
 *
 * SEE ALSO
 *    AMfiCheckAccess(), AMfiCloseNode(), AMfiGetAttributes(),
 *    AMfiLookUpNodeByName(), AMfiOpenNode(), AMfiReadDirNodeEntries(),
 *    and AMfiReleaseNode().
 *
 * END_MANUAL_ENTRY
 */


/*
 * BEGIN_MANUAL_ENTRY(AMfiCheckAccess(3K), \
 *		./man/kernel/nucfs/nwfi/SVr4_1/CheckAccess )
 *
 * NAME
 *    (vnodeops_t *)->(*vop_access)() - Validates permission to perform the
 *                                      UNIX generic operation on the specified
 *                                      vnode.
 *
 * SYNOPSIS
 *    int
 *    AMfiCheckAccess (vnode, mode, flags, unixCredentials)
 *    vnode_t *vnode;
 *    int     mode;
 *    int     flags;
 *    cred_t  *unixCredentials;
 *
 * INPUT
 *    vnode                   - Vnode representing the NetWare node in question.
 *    mode                    - Set to one of the following:
 *                              VREAD  - Can UNIX client user read the NetWare
 *                                       node object?
 *                              VWRITE - Can UNIX client user write to the 
 *                                       NetWare node object?
 *                              VEXEC  - Can UNIX client user execute the
 *                                       NetWare node object?
 *    flags                   - Bit-mask of additional information.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.  This represents the credentials of
 *                              the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUE
 *    0                       - Successful completion, permission is granted.
 *    [EACCES]                - Permission denied.
 *
 * DESCRIPTION
 *    AMfiCheckAccess checks access permissions for the specified vnode.
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
AMfiCheckAccess (	vnode_t	*vnode,
			int	mode,
			int	flags,
			cred_t 	*unixCredentials )
{
	NVLT_ENTER (4);

	if (mode & VWRITE)
		/*
		 * NUCAM File System is a read only file system.
		 */
		return (NVLT_LEAVE (EROFS));

	/*
	 * Access is always granted for read and change directory. because
	 * permissions are alwasy 0555.
	 */
	return (NVLT_LEAVE (SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(AMfiCloseNode(3K), \
 *		./man/kernel/nucam/amfi/SVR4.X/CloseNode )
 *
 * NAME
 *    (vnodeops_t *)->(*vn_close)() - Closes the AMfs node object associated
 *                                    with the specified vnode.
 *
 * SYNOPSIS
 *    int
 *    AMfiCloseNode (vnode, openFlags, lastClose, offset, unixCredentials)
 *    vnode_t		*vnode;
 *    int		openFlags;
 *    boolean_t		lastClose;
 *    off_t		offset;
 *    struct	ucred	*unixCredentials;
 *
 * INPUT
 *    vnode                   - Vnode representing the AMfs node to be closed.
 *    openFlags               - Current File table flags.  Set to an exclusive
 *                              'OR' of the following:
 *                              FREAD  - Opened for reading.
 *                              FWRITE - Opened for writing.
 *                              FEXCL  - Opened exclusively.
 *    lastClose               - Whether this is the last close.
 *    offset                  - Current file offset in the Generic File System
 *                              file structure (ie. struct file).
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *
 * DESCRIPTION
 *    The AMfiCloseNode closes the AMfs node object associated with the 
 *    specified vnode at the request of the Generic File System.
 *
 * SEE ALSO
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
AMfiCloseNode (	vnode_t		*vnode,
		int		openFlags,
		boolean_t	lastClose,
		off_t		offset,
		cred_t		*unixCredentials )
{
	NVLT_ENTER (5);
	ASSERT(vnode->v_type == VDIR);
	return (NVLT_LEAVE (SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(AMfiGetAttributes(3K), \
 *		./man/kernel/nucam/amfi/SVR4.X/GetAttributes )
 *
 * NAME
 *    (vnodeops_t *)->(*vn_getattr)() - Populates the specified attributes with
 *                                      the information describing the AMfs node
 *                                      associated with the specified vnode.
 *
 * SYNOPSIS
 *    int
 *    AMfiGetAttributes (vnode, unixAttributes, flags, unixCredentials)
 *    vnode_t	*vnode,
 *    vattr_t	*attributes, 
 *    int	flags;
 *    cred_t	*unixCredentials)
 *
 * INPUT
 *    vnode                   - Vnode representing the AMfs node in question.
 *    attributes->va_mask     - Bit mask of the attributes of which the caller
 *                              is interested in.  Set to an exclusive 'OR' of
 *                              the following:
 *                                 AT_TYPE    - Type attribute.
 *                                 AT_MODE    - Mode attribute.
 *                                 AT_UID     - User ID attribute.
 *                                 AT_GID     - Group ID attribute.
 *                                 AT_FSID    - File System ID attribute.
 *                                 AT_NODEID  - Node ID attribute.
 *                                 AT_NLINK   - Number of link attribute.
 *                                 AT_SIZE    - Size attribute.
 *                                 AT_ATIME   - Access time attribute.
 *                                 AT_MTIME   - Modification time  attribute.
 *                                 AT_CTIME   - Creatation time attribute.
 *                                 AT_RDEV    - Device attribute.
 *                                 AT_BLKSIZE - Block size attribute.
 *                                 AT_NBLOCKS - Number of blocks attribute.
 *                                 AT_VCODE   - Version code attribute.
 *                                 AT_ALL     - All attributes.
 *    flags                    - Not used.
 *    unixCredentials->cr_uid  - The effective user id of the process making
 *                               the request.
 *    unixCredentials->cr_gid  - The effective group id of the process making
 *                               the request.
 *
 * OUTPUT
 *    attributes->va_type      - Set to one of the following: 
 *                                 VDIR  - Directory file.
 *                                 VBAD  - Bad file type.
 *    attributes->va_mode      - Node access mode.
 *    attributes->va_uid       - Owner user ID.
 *    attributes->va_gid       - Owner group ID.
 *    attributes->va_fsid      - Directory system ID.
 *    attributes->va_nodeid    - node ID.
 *    attributes->va_nlink     - Number of references to node.
 *    attributes->va_blksize   - I/O block size.
 *    attributes->va_vcode     - version code.
 *    attributes->va_size      - Node size in bytes.
 *    attributes->va_atime     - Time of last access.
 *    attributes->va_mtime     - Time of last modification.
 *    attributes->va_ctime     - Time node created. 
 *    attributes->va_rdev      - Device the file represents.
 *    attributes->va_nblocks   - Number of blocks allocated.
 *    attributes->va_aclcnt    - count of Access Control List entries.
 *
 * RETURN VALUE
 *    0                        - Successful completion.
 *
 * DESCRIPTION
 *    The AMfiGetAttributes loads the specified attributes structure with the
 *    requested AMfs node information virtualized into UNIX semantics.
 *
 * NOTES
 *    The AMfiGetAttributes operation may return more attributes than the
 *    caller requested (if it is convenient or cheap to do so), but provides
 *    at least those that were requested.  It is illegal for the caller to 
 *    refer subsequently to attributes that were not requested.
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
AMfiGetAttributes (	vnode_t	*vnode,
			vattr_t	*unixAttributes,
			int	flags,
			cred_t	*unixCredentials )
{
	NUCAM_ATTRIBUTES_T	amfsAttributes;
	NUCAM_DIAG_T		diagnostic;
	AMFS_NODE_T		*amfsNode;

	NVLT_ENTER (4);

	/*
	 * Get the AMfs node associated with the specified vnode.
	 */
	amfsNode = (AMFS_NODE_T *)(vnode->v_data);
	ASSERT (amfsNode != (AMFS_NODE_T *)NULL);
	LOCK_AMNODE(amfsNode);

	/*
	 * Read in the name space information of the AMfs node associated 
	 * with the specified vnode.
	 */
	amfsAttributes = AMFS_NODE_ATTR (amfsNode);

	/*
	 * Load the specified attributes with the AMfs node's name space
	 * information virtualized into semantics.
	 */
	if (AMfiGetNodeType (amfsAttributes.type, &(unixAttributes->va_type),
			&diagnostic) != SUCCESS)
		return ((int)NVLT_LEAVE (AMfiErrorMap (diagnostic)));

	AMfiGetNodePermissions (amfsAttributes.permissions, 
			&(unixAttributes->va_mode)); 
	unixAttributes->va_uid = (uid_t)amfsAttributes.userID;
	unixAttributes->va_gid = (gid_t)amfsAttributes.groupID;
	unixAttributes->va_fsid = (dev_t)vnode->v_vfsp->vfs_dev;
	unixAttributes->va_nodeid = (ino_t)amfsAttributes.number;
	unixAttributes->va_nlink = 
			(nlink_t)amfsAttributes.numberOfLinks;
	unixAttributes->va_size = (u_long)amfsAttributes.size;
	unixAttributes->va_atime.tv_sec = 
			(time_t)amfsAttributes.accessTime;
	unixAttributes->va_atime.tv_nsec = 0;
	unixAttributes->va_mtime.tv_sec =
			(time_t)amfsAttributes.modifyTime;
	unixAttributes->va_mtime.tv_nsec = 0;
	unixAttributes->va_ctime.tv_sec = 
			(time_t)amfsAttributes.changeTime;
	unixAttributes->va_ctime.tv_nsec = 0;
	unixAttributes->va_rdev = (dev_t)vnode->v_vfsp->vfs_dev;
	unixAttributes->va_blksize = (u_long)vnode->v_vfsp->vfs_bsize;
	unixAttributes->va_nblocks = 2;
	unixAttributes->va_vcode = 1;   /* NUCAM_MAJOR_VERSION_NUMBER? */
	unixAttributes->va_aclcnt = 0; 	/* or, should this be NACLBASE */
					/*	     like nfs_getattr? */


	UNLOCK_AMNODE(amfsNode);
	return ((int)NVLT_LEAVE (SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(AMfiOpenNode(3K), \
 *		./man/kernel/nucam/amfi/SVR4.X/OpenNode )
 *
 * NAME
 *    (vnodeops_t *)->(*vn_open)() - Opens the specified AMfs node with the
 *                                   specified openFlags.
 *
 * SYNOPSIS
 *    int
 *    AMfiOpenNode (vnode, openFlags, unixCredentials)
 *    vnode_t	**vnode,
 *    int	openFlags,
 *    cred_t	*unixCredentials)
 *
 * INPUT
 *    vnode                   - Vnode representing the AMfs node to be opened.
 *    openFlags               - This function ensures that the FWRITE flag,
 *                              prescribing that the open is to be performed
 *                              for writing, is not effective. For AMfi,
 *				the FEXL, FAPPEND, FNDELAY, or FSYNC are not
 *				meaningful.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.
 *
 * OUTPUT
 *    There is no outarg for this function.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *
 * DESCRIPTION
 *    The AMfiOpenNode opens the AMfs node associated with the specified
 *    vnode with the specified openFlags. 
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
AMfiOpenNode (	vnode_t	**vnode,
		int	openFlags,
		cred_t	*unixCredentials )
{
	NVLT_ENTER (3);
	ASSERT((*vnode)->v_type == VDIR);
	ASSERT(amfiInitialized);	/* else could not lookup */
	if (openFlags & FWRITE) 
		/*
		 * Can not open nodes for write in NUCAM File System.
		 */
		return (NVLT_LEAVE (EACCES));
	return (NVLT_LEAVE (SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(AMfiReadDirectoryNodeEntries(3K), \
 *		./man/kernel/nucam/amfi/SVR4.X/ReadDirectoryNodeEntries )
 *
 * NAME
 *    (vnodeops_t *)->(*vn_readdir)() - Reads AMfs directory entries
 *                                           from the specified vnode 
 *                                           representing the AMfs directory
 *                                           to be read.
 *
 * SYNOPSIS
 *    int
 *    AMfiReadDirNodeEntries (dirVnode, ioArgs, unixCredentials, eofFlag)
 *    struct  vnode *dirVnode;
 *    uio_t         *ioArgs;
 *    cred_t        *unixCredentials;
 *    int           *eofFlag;
 *
 * INPUT
 *    dirVnode                - Vnode representing the directory to be read.
 *    ioArgs->uio_iov         - pointer to an array of iovec_t structure(s),
 *                              to read the entries into.
 *    ioArgs->uio_iovcnt      - Number of iovec_t structure(s).
 *    ioArgs->uio_offset      - Offset of the sequential directoy entry to
 *                              be read.  If set to zero indicates that the 
 *                              first entries must be read.
 *    ioArgs->uio_segflg      - Address space (kernel or user).  Set to TRUE for
 *                              buffers resident in kernel space, and FALSE for
 *                              buffers resident in user space.
 *    ioArgs->uio_fmode       - Not used.
 *    ioArgs->uio_limit       - U-limit (maximum "block" offset).
 *    ioArgs->uio_resid       - Residual count.  Number of bytes to read into
 *                              the buffer(s) defined by ioArgs->uio_iov and 
 *                              ioArgs->uio_iovcnt.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.
 *
 * OUTPUT
 *    ioArgs->uio_iov         - pointer to an array of iovec_t structure(s),
 *                              containing the directory entries read.
 *    ioArgs->uio_iovcnt      - Number of iovec_t structure(s).
 *    ioArgs->uio_offset      - Offset of the sequential directoy entry to
 *                              be read. 
 *    ioArgs->uio_resid       - Residual count.  Number of bytes read into
 *                              the buffer(s) defined by ioArgs->uio_iov and 
 *                              ioArgs->uio_iovcnt.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *    [EFAULT]                - The read buffer is not within the writable
 *                              address space of the user space.
 *    [EINVAL]                - Buffer size argument is not large enough for
 *                              one directory entry.
 *    [EIO]                   - Directory file is no longer available.
 *    [ENOENT]                - The current file pointer for the directory is
 *                              not located at a valid entry.
 *    [ENOLINK]               - Unrecoverable network error.
 *    [EPROTO]                - Miscommunication with user-level daemon.
 *
 * DESCRIPTION
 *    The AMfiReadDirNodeEntries reads AMfs directory entries in a Generic
 *    File System format (struct direct) from the specified AMfs directory.
 *    The number of entries read is limited by either buffer exhaustion or 
 *    directory entry exhaustion.
 *
 * END_MANUAL_ENTRY
 */
int
AMfiReadDirNodeEntries (	vnode_t	*dirVnode,
				uio_t	*ioArgs,
				cred_t	*unixCredentials,
				int	*eofFlag )
{
	nwcred_t		nucCredentials;
	NUCAM_DIR_IO_ARGS_T	dirIoArgs;
	NUCAM_DIAG_T		diagnostic;
	int			returnCode, bufferSize;
	struct	dirent		*tmpEntry;
	char			*entriesPtr, *tmpBuffer, *endBuffer;
	AMFS_NODE_ID_T		nodeID;
	AMFS_NODE_T		*foundNode, *amfsDirNode;
	boolean_t		fixOffset = B_FALSE;
#	define AMDOTSIZE \
	 ((size_t)NUCAM_DIRENT_ALIGN(offsetof(struct dirent, d_name) + 2))
#	define AMDOTDOTSIZE \
	 ((size_t)NUCAM_DIRENT_ALIGN(offsetof(struct dirent, d_name) + 3))
	/*
	 * We need a dirent structure padded at the end to support a
	 * ".." name string.
	 */
	union {
		struct dirent dirent;
		char tmpbuffer[AMDOTDOTSIZE];
	}			un;
	struct dirent		*dotEntriesp = &un.dirent;

	NVLT_ENTER (4);
	NVLT_PRINTF ("AMfiReadDirNodeEntries: uio_offset = %x	%d\n",
			ioArgs->uio_offset, ioArgs->uio_offset, 0);

	*eofFlag = 0;

	if (ioArgs->uio_resid <= 0)
		/*
		 * Invalid size.
		 */
		return (NVLT_LEAVE (EINVAL));

	/*
	 * Do we need to read more entries or have we read them all?
	 */
	if (ioArgs->uio_offset == UIO_OFFSET_EOF) {
		/*
		 * All the entries have already been read, so set eofFlag  
		 * to inform the Generic File System that there are no more 
		 * entries left to read.
		 */
		*eofFlag = 1;
		return (NVLT_LEAVE (SUCCESS));
	}

	ASSERT(dirVnode->v_count != 0);

	/*
	 * Get the AMfs node associated with the specified dirVnode.
	 */
	amfsDirNode = (AMFS_NODE_T *)dirVnode->v_data;
	ASSERT (amfsDirNode != NULL);

	/*
	 * Get the generic NUC credentials.
	 */
	AMfiGetCredentials(unixCredentials, &nucCredentials);

	/*
	 * To Be Fixed (doshi):
	 *	Reduce the scope for AMNODE sleep lock to the minimum
	 *	required duration in this function.
	 */

	LOCK_AMNODE(amfsDirNode);
	if (ioArgs->uio_offset == DIR_DOT_OFFSET) {
		/*
		 * Need to return the "." entry.
		 */
		if (ioArgs->uio_resid < AMDOTSIZE) {
			/*
			 * I/O buffer not big enough.
			 */
			UNLOCK_AMNODE(amfsDirNode);
			return(NVLT_LEAVE(EINVAL));
		}

		/*
		 * Add the "." entry.
		 */
		strcpy (dotEntriesp->d_name, ".");
		dotEntriesp->d_ino = AMFS_NODE_ID (amfsDirNode);
		dotEntriesp->d_off = (AMFS_NODE_ID (amfsDirNode)) + 1; 
		dotEntriesp->d_reclen = AMDOTSIZE;

		if ((returnCode = uiomove((caddr_t)dotEntriesp,
				dotEntriesp->d_reclen, UIO_READ, ioArgs))
				!= 0) {
			UNLOCK_AMNODE(amfsDirNode);
			return(NVLT_LEAVE(returnCode));
		}
		fixOffset = B_TRUE;
		ioArgs->uio_offset = DIR_DOTDOT_OFFSET;
	}
	if (ioArgs->uio_offset == DIR_DOTDOT_OFFSET) {
		/*
		 * Need to return the ".." entry.
		 */
		if (ioArgs->uio_resid < AMDOTDOTSIZE) {
			/*
			 * I/O buffer not big enough.
			 */
			UNLOCK_AMNODE(amfsDirNode);
			if (fixOffset)
				/*
				 * We read something.
				 */
				return(NVLT_LEAVE(SUCCESS));

			/*
			 * We read nothing.
			 */
			return(NVLT_LEAVE(EINVAL));
		}
		/*
		 * Add the ".." entry.
		 */
		strcpy (dotEntriesp->d_name, "..");
		dotEntriesp->d_ino = 
			AMFS_NODE_ID (AMFS_PARENT_NODE(amfsDirNode));
		dotEntriesp->d_off = 
			AMFS_NODE_ID (AMFS_PARENT_NODE(amfsDirNode));
		dotEntriesp->d_reclen = AMDOTDOTSIZE;
		if((returnCode = uiomove((caddr_t)dotEntriesp,
				dotEntriesp->d_reclen, UIO_READ, ioArgs))
				!= 0) {
			UNLOCK_AMNODE(amfsDirNode);
			return(NVLT_LEAVE(returnCode));
		}
		fixOffset = B_TRUE;
		ioArgs->uio_offset = DIR_SEARCH_ALL;
	}

	bufferSize = MIN(ioArgs->uio_resid, NWAM_MAXBUF);
	tmpBuffer = (char *)NUCAM_KMEM_ALLOC (bufferSize, KM_SLEEP);
	ASSERT(NUCAM_IS_DIRENT_ALIGNED(tmpBuffer));

	/*
	 * Set the I/O argument structure.
	 */
	ASSERT(ioArgs->uio_offset != 0);	/* . and .. handled above */
	if (fixOffset)
		ioArgs->uio_offset = DIR_SEARCH_ALL;
	dirIoArgs.searchHandle = ioArgs->uio_offset;
	dirIoArgs.bufferLength = bufferSize;
	dirIoArgs.memoryType = IOMEM_KERNEL;
	dirIoArgs.buffer = tmpBuffer;

	/*
	 * Read the directory entries.
	 */
	if (AMfsReadAmfsNodeEntries(&nucCredentials, amfsDirNode, &dirIoArgs,
			&diagnostic) != SUCCESS) {
		UNLOCK_AMNODE(amfsDirNode);
		NUCAM_KMEM_FREE (tmpBuffer, bufferSize);
		if (fixOffset)
			/*
			 * We have an error, but got at least "." or ".."
			 * above.
			 */
			return (NVLT_LEAVE(SUCCESS));

		return (NVLT_LEAVE(AMfiErrorMap(diagnostic)));
	}

	/*
	 * Make sure the number of bytes read is not more than number
	 * of bytes requested.
	 */
	if (dirIoArgs.bufferLength > bufferSize) {
		UNLOCK_AMNODE(amfsDirNode);
		NUCAM_KMEM_FREE (tmpBuffer, bufferSize);
		cmn_err (CE_WARN,
			"AMfiReadDirNodeEntries: read too much data");
		return (NVLT_LEAVE(EPROTO));
	}
	if (dirIoArgs.bufferLength == 0) {

		/*
		 * No more directory entries left to be read.
		 */
		UNLOCK_AMNODE(amfsDirNode);
		NUCAM_KMEM_FREE (tmpBuffer, bufferSize);
		ioArgs->uio_offset = UIO_OFFSET_EOF;
		*eofFlag = 1;
		return(NVLT_LEAVE (SUCCESS));
	}

	/*
	 * NWfsReadAmfsNodeEntries fills the dirIoArgs.buffer with
	 * directory entries in "dirent" structure format.  Scan the
	 * returned entries to validate them and get the correct node number.
	 *
	 * To Be Fixed (mdash):  All nodes have well-defined numbers when
	 * created.  Why set them again?
	 */

	/*
	 * The alignment checks following may seem overly strict, but how are
	 * we otherwise to tell that we are not getting entries that straddle
	 * the end of the buffer, etc?
	 */

	endBuffer = tmpBuffer + dirIoArgs.bufferLength;
	if (!NUCAM_IS_DIRENT_ALIGNED(endBuffer)) {
		UNLOCK_AMNODE(amfsDirNode);
		NUCAM_KMEM_FREE (tmpBuffer, bufferSize);
		cmn_err (CE_WARN,
			"AMfiReadDirNodeEntries: misaligned buffer end");
		return (NVLT_LEAVE(EPROTO));
	}
	for (entriesPtr = tmpBuffer; entriesPtr < endBuffer;
			entriesPtr += tmpEntry->d_reclen) {

		if (!NUCAM_IS_DIRENT_ALIGNED(entriesPtr)) {
			UNLOCK_AMNODE(amfsDirNode);
			NUCAM_KMEM_FREE (tmpBuffer, bufferSize);
			cmn_err (CE_WARN,
				"AMfiReadDirNodeEntries: misaligned entries");
			return (NVLT_LEAVE(EPROTO));
		}
		tmpEntry = (struct dirent *)entriesPtr;

		/*
		 * If there is an AMfs node representing this node
		 * entry, make sure the unique node identifier is set
		 * to the number returned by nucamd's get directory
		 * entries. 
		 *
		 * To Be Fixed (mdash):  All nodes have well-defined numbers
		 * when created.  Why set them again?
		 */
		nodeID.internalName = AMFS_UNKNOWN_NODE_NUMBER;
		nodeID.externalName = tmpEntry->d_name;
		if (AMfsSearchChildList (amfsDirNode, &nodeID, &foundNode,
				&diagnostic) == SUCCESS)
			/*
			 * Set the foundNode's node number.
			 */
			foundNode->attributes.number = tmpEntry->d_ino;
	}

	/*
	 * Did the last entry end on the logical buffer boundary?
	 */
	if (entriesPtr != endBuffer) {
		UNLOCK_AMNODE(amfsDirNode);
		NUCAM_KMEM_FREE (tmpBuffer, bufferSize);
		cmn_err (CE_WARN,
			"AMfiReadDirNodeEntries: entry straddling buffer end");
		return (NVLT_LEAVE(EPROTO));
	}

	if((returnCode = uiomove((caddr_t)tmpBuffer, dirIoArgs.bufferLength,
			UIO_READ, ioArgs)) != 0) {
		UNLOCK_AMNODE(amfsDirNode);
		NUCAM_KMEM_FREE (tmpBuffer, bufferSize);
		return(NVLT_LEAVE(returnCode));
	}

	/*
	 * Save the offset of the next directory entry to read.
	 */
	ioArgs->uio_offset = (off_t)dirIoArgs.searchHandle;

	if (ioArgs->uio_offset == UIO_OFFSET_EOF) {
		/*
		 * We have already gotten all of the entries.
		 */
		*eofFlag = 1;
	}

	NVLT_PRINTF ("AMfiReadDirNodeEntries: uio_offset = %x	%d\n",
			ioArgs->uio_offset, ioArgs->uio_offset, 0);
	UNLOCK_AMNODE(amfsDirNode);
	NUCAM_KMEM_FREE (tmpBuffer, bufferSize);
	return(NVLT_LEAVE(SUCCESS));
}




/*
 * BEGIN_MANUAL_ENTRY(AMfiLookUpNodeByName(3K), \
 *              ./man/kernel/nucam/amfi/SVR4.X/LookUpNodeByName )
 *
 * NAME
 *    (vnodeops_t *)->(*vn_lookup)() - Looks up the vnode associated with the
 *                                     specified nodeName in the specified
 *                                     parentVnode.
 *
 * SYNOPSIS
 *    int
 *    AMfiLookUpNodeByName (parentVnode, nodeName, foundVnode, pathName, flags,
 *                          rootVnode, unixCredentials)
 *    vnode_t           *parentVnode;
 *    char              *nodeName;
 *    vnode_t           **foundVnode;
 *    pathname_t        *pathName;
 *    int               flags;
 *    vnode_t           *rootVnode;
 *    cred_t            *unixCredentials;
 *
 * INPUT
 *    parentVnode             - Vnode representing the AMfs parent directory
 *                              to look for the specified nodeName.
 *    nodeName                - Name of the node in search of.
 *    pathName                - Not used.
 *    flags                   - Additional information.
 *                              LOOKUP_DIR - Look for the Vnode of the parent
 *                                           directory.
 *    rootVnode               - Vnode representing the NUCAM File System root
 *                              node.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.
 *
 * OUTPUT
 *    foundVnode              - Vnode representing the found AMfs node
 *                              associated with the specified nodeName in the
 *                              parentVnode directory.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *
 * DESCRIPTION
 *    The AMfiLookUpNodeByName looks up the specified component name (nodeName)
 *    in the specified parentVnode directory and returns the foundVnode.
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
AMfiLookUpNodeByName (	vnode_t         *parentVnode,
			char            *nodeName,
			vnode_t         **foundVnode,
			pathname_t      *pathName,
			int             flags,
			vnode_t         *rootVnode,
			cred_t          *unixCredentials )
{
	nwcred_t		nucCredentials;
	AMFS_NODE_T		*amfsParentNode;
	AMFS_NODE_T		*foundAmfsNode;
	NUCAM_DIAG_T		diagnostic;
	ccode_t			returnCode;
	boolean_t		supportedVolume = FALSE;

	NVLT_ENTER (7);

	ASSERT (parentVnode->v_count != 0);

	if (*nodeName == '\0') {
		/*
		 * Null component is synonym for directory being searched.
		 */
		VN_HOLD (parentVnode);
		*foundVnode = parentVnode;
		return (NVLT_LEAVE (SUCCESS));
	}

	/*
	 * Get the AMfs node associated with the speicfied parentVnode.
	 */
	amfsParentNode = (opaque_t *)parentVnode->v_data;
	ASSERT (amfsParentNode != NULL);

	/*
	 * Get the generic NUC credentials.
	 */
	AMfiGetCredentials(unixCredentials, &nucCredentials); 

	/*
	 * Lock parent for search.
	 */
	
	LOCK_VP_AMNODE (parentVnode);

	/*
	 * Look up the node.  The vnode associated with the returned
	 * node is held for us by AMfsLookUpNode.
	 */
	if (AMfsLookUpNode (&nucCredentials, amfsParentNode, nodeName,
			&foundAmfsNode, &supportedVolume, &diagnostic)
			!= SUCCESS) {
		/*
		 * Node not found.
		 */
		UNLOCK_VP_AMNODE (parentVnode);
		returnCode = AMfiErrorMap (diagnostic);
		*foundVnode = NULLVP;
		return ((int)NVLT_LEAVE (returnCode));
	}

	if (AMFS_NODE_LIST_IS_EMPTY (&foundAmfsNode->parentLink)
			&& (foundAmfsNode->stamp != AMFS_ROOT_NODE_STAMP)) {

		/*
		 * This is a newly allocated node.  If it is a volume node,
		 * take care of mounting now.
		 */
		if (foundAmfsNode->attributes.type == AM_VOLUME) {

			/*
			 * Can we mount this volume?
			 */
			if (supportedVolume) {

				/*
				 * This volume can be mounted.
				 * To cover races on multiple mounts,
				 * AMfiNucFsMount will attach the parent
				 * to the child.
				 */
				if ((returnCode = AMfiNucFsMount(amfsParentNode,
						unixCredentials,
						&nucCredentials,
						&foundAmfsNode)) != 0) {
					UNLOCK_VP_AMNODE (parentVnode);
					AMfsFreeAmfsNode(foundAmfsNode);
					*foundVnode = NULLVP;
					return ((int)NVLT_LEAVE (returnCode));
				}
			} else {

				/*
				 * This volume can not be mounted.  The
				 * permissions for this node are set to 
				 * AM_NO_PERMISSION to deny access for all.
				 */
				foundAmfsNode->attributes.permissions =
						AM_NO_PERMISSION;
				AMfsAttachToParentNode(amfsParentNode,
						foundAmfsNode);
			}
		} else {

			/*
			 * We have a fully initialized, newly allocated node
			 * that is not a volume.  Make it findable before
			 * unlocking the parent.
			 */
			AMfsAttachToParentNode(amfsParentNode, foundAmfsNode);
		}
	}
	UNLOCK_VP_AMNODE (parentVnode);

	/* Safe to examine vp without locking foundAmfsNode */
	*foundVnode = AMFS_VP(foundAmfsNode);

	return (NVLT_LEAVE (SUCCESS));
}




/*
 * BEGIN_MANUAL_ENTRY(AMfiReleaseNode(3K), \
 *              ./man/kernel/nucam/amfi/SVR4.X/ReleaseNode )
 *
 * NAME
 *    (vnodeops_t *)->(*vn_inactive)() - Releases the AMfs node associated with
 *                                       the specified vnode.
 *
 * SYNOPSIS
 *    void
 *    AMfiReleaseNode (vnode, unixCredentials)
 *    struct vnode *vnode;
 *    cred_t       *unixCredentials;
 *
 * INPUT
 *    vnode                   - Vnode representing the AMfs node to be released.
 *    unixCredentials->cr_uid - The effective user id of the process making the
 *                              request.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.
 *
 * OUTPUT
 *    none.
 *
 * RETURN VALUE
 *    0                       - Successful completion.
 *
 * DESCRIPTION
 *    The AMfiReleaseNode operation is called when the Generic File System no
 *    longer holds any references to the AMfs file associated with the
 *    specified vnode; this allows this function to either release the AMfs
 *    node and all of the resources associated with the AMfs node or caches
 *    the node for a later release.
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
void
AMfiReleaseNode (	vnode_t *vnode,
			cred_t  *unixCredentials )
{
        opaque_t        *amfsNode;
	opaque_t	*parentNode;
	vnode_t		*parentVnode;

        NVLT_ENTER (2);

        /*
         * Vnode's reference count should be at least 1 here.
         */
        ASSERT (vnode->v_count >= 1);

        /*
         * Do not worry about releasing the AMfs node associated with the
         * root vnode, AMfsUnMountVolume releases the node.
         */
        if (vnode->v_flag & VROOT) {
                NVLT_LEAVE (SUCCESS);
                return;
        }

        VN_LOCK(vnode);
	for(;;) {
		if (vnode->v_count > 1) {
                /*
                 * someone generated a new reference before we acquired
                 * the vnode lock. the vnode has now become someone else's
                 * responsibility. give up our reference and return.
                 */
               		vnode->v_count--;
                	VN_UNLOCK(vnode);
			NVLT_LEAVE (SUCCESS);
                	return;
        	}
        	ASSERT(vnode->v_count == 1);
        	/*
	         * Get the AMfs node associated with the specified vnode.
       		 */
        	amfsNode = (opaque_t *)(vnode->v_data);
        	ASSERT (amfsNode != NULL);
		parentNode = (opaque_t *)AMFS_PARENT_NODE(amfsNode);
		/*
		 * Since we can never have a situation in which parent-less
		 * amfsNodes can stay around (since a parent can never go
		 * away while one of its children persist), we should be
		 * able to assert that parentNode is not NULL.
		 */
		ASSERT(parentNode != NULL);
		parentVnode = AMFS_VP(parentNode);
		ASSERT(parentVnode->v_count != 0);
		/*
		 * We must first detach this node from parent,
		 * before we can free it.
		 */
		ASSERT(!AMFS_NODE_LIST_IS_EMPTY(AMFS_PARENT_LINK_P(amfsNode)));

		if (TRYLOCK_AMNODE(parentNode)) {
			VN_UNLOCK(vnode);
			/*
			 * Obtain an extra hold on the parent, so that
			 * we do not have to worry about recursively
			 * releasing parent while we are not done
			 * releasing the vnode we came here to free.
			 */
			VN_HOLD(parentVnode);
			/*
			 * AMfsDetachFromParentNode() below will drop
			 * the reference count held by this amfsNode on
			 * its parent. But because of the extra hold we
			 * have obtained above, we know that the parent
			 * will not be freed, as a result of the detach.
			 * This will allow us to safely dispose of the
			 * child, before having to free the parent as well.
			 * This means we do not have to worry about lock
			 * acquisition deadlocks between child and parent
			 * nodes.
			 */ 
			AMfsDetachFromParentNode(amfsNode);
			UNLOCK_AMNODE(parentNode);
       			/*
		         * Now liquidate the AMfs node associated 
			 * with the specified vnode.
		         */
       			AMfsFreeAmfsNode(amfsNode);
			/*
			 * And drop the hold on the parent.
			 */
			VN_RELE(parentVnode);
       			NVLT_LEAVE (SUCCESS);
			return;
		}

		/* 
		 * parentNode is busy. Try again, transparently,
		 * after a brief delay.
		 */
		VN_UNLOCK(vnode);
		delay(HZ/10);
		VN_LOCK(vnode);

	} /* for(;;) */
}


/*
 * Mount the appropriate netware volume on foundAmfsNode, which is an in/out
 * parameter to allow for races on resolving the name.  The races are admitted
 * because we drop the amfsParentNode lock (which must be held on entry and is
 * also held on exit) to allow for operations that may block indefinitely.  If
 * we lose such a race, it is handled transparently by freeing *foundAmfsNode,
 * and substituting the node provided by the winner of the race.  The returned
 * vnode is held.
 */
STATIC	int
AMfiNucFsMount (	AMFS_NODE_T	*parentNode,
			cred_t		*unixCredentials,
			nwcred_t	*nucCredentials,
			AMFS_NODE_T	**foundAmfsNode )
{
	AMFS_NODE_T		*dupAmfsNode;
	char			*tempPtr;
	vfs_t			*vfsp;
	vfssw_t			*vfsswp;
	NWFI_MOUNT_ARGS_T	nucfsMountArgs;
	struct mounta		mountArgs;
	int			returnCode = 0;
	NUCAM_DIAG_T		diagnostic;
	AMFS_NODE_ID_T		nodeID;
	vnode_t			*rootNucfsVnode;

	NVLT_ENTER (3);

	/*
	 * Note:  In case of errors, we do not check to see if the error can
	 * be discarded because some other thread of control has successfully
	 * mounted the node.  We decline to do so (1) for simplicity's sake
	 * and (2) because such races are not discoverable from user level.
	 */

	UNLOCK_AMNODE (parentNode);

	/*
	 * Derive the nucfsType for NUCFS File System from its
	 * ordinal position in the vfssw[].
	 */
	if ((vfsswp = vfs_getvfssw("nucfs")) != NULL)
		nucfsType = vfsswp - vfssw;
	else {
		LOCK_AMNODE (parentNode);
		return (NVLT_LEAVE(EINVAL));
	}

	/*
	 * Now initialize a new vfs structure to mount the NUCFS
	 * volume with.
	 */
	vfsp = (vfs_t *)NUCAM_KMEM_ALLOC(sizeof(vfs_t), KM_SLEEP);
	VFS_INIT(vfsp, vfssw[nucfsType].vsw_vfsops, (caddr_t) NULL);

	/*
	 * Now automount the NUCFS Server:Volume root vnode onto
	 * the NUCAM volume vnode before returning its vnode
	 * pointer.
	 *
	 * Note:
	 *    Need to take out the .nws extension of the server
	 *    name and .nwv extension of the volume name.
	 */
	nucfsMountArgs.address.maxlen = parentNode->address.maxlen;
	nucfsMountArgs.address.len = parentNode->address.len;
	nucfsMountArgs.address.buf = parentNode->address.buf;
	strcpy (nucfsMountArgs.volumeName, AMFS_NODE_NAME (*foundAmfsNode));
	for (tempPtr = nucfsMountArgs.volumeName; *tempPtr; tempPtr++) {
		if (*tempPtr == '.') {
			*tempPtr = '\0';
			break;
		}
	}

	nucfsMountArgs.mountFlags = 0;
	mountArgs.flags = (MS_SYSSPACE | MS_NOSUID | MS_DATA);
	mountArgs.datalen = sizeof (NWFI_MOUNT_ARGS_T);
	mountArgs.dataptr = (char *)&nucfsMountArgs;
	if ((returnCode = VFS_MOUNT(vfsp, &(*foundAmfsNode)->vnode, &mountArgs,
			unixCredentials)) != 0) {
		LOCK_DEINIT(&vfsp->vfs_mutex); 
		NUCAM_KMEM_FREE ((caddr_t) vfsp, sizeof(struct vfs));
		LOCK_AMNODE (parentNode);
		return (NVLT_LEAVE(returnCode));
	}

	/*
	 * Set the nucfs file system root vnode's level
	 * to that of the mount point.  Also set the
	 * ceiling level and the floor level of mounted
	 * file system to that of mount point.
	 */
	if (VFS_ROOT(vfsp, &rootNucfsVnode) == 0) {
		rootNucfsVnode->v_lid = 
		    (*foundAmfsNode)->vnode.v_vfsp->vfs_macfloor;
		vfsp->vfs_macceiling = 
		    (*foundAmfsNode)->vnode.v_vfsp->vfs_macfloor;
		vfsp->vfs_macfloor = 
		    (*foundAmfsNode)->vnode.v_vfsp->vfs_macfloor;
		VN_RELE(rootNucfsVnode);
	}

	LOCK_AMNODE (parentNode);

	/*
	 * Under protection of the parent lock, check to see if another thread
	 * of control has won the race on mounting this volume.
	 */
	nodeID.internalName = AMFS_UNKNOWN_NODE_NUMBER;
	nodeID.externalName = AMFS_NODE_NAME (*foundAmfsNode);
	if (AMfsSearchChildList(parentNode, &nodeID, &dupAmfsNode, &diagnostic)
			== SUCCESS) {
		/*
		 * We lost the race.  Substitute the newly found node for the
		 * one we were passed, and get rid of the one we were passed.
		 */
		(void)VFS_UNMOUNT(vfsp, unixCredentials);
		LOCK_DEINIT(&vfsp->vfs_mutex); 
		NUCAM_KMEM_FREE ((caddr_t) vfsp, sizeof(struct vfs));
		AMfsFreeAmfsNode(*foundAmfsNode);
		VN_HOLD(&dupAmfsNode->vnode);
		*foundAmfsNode = dupAmfsNode;
		return (NVLT_LEAVE (SUCCESS));
	}

	/*
	 * After making the new node publicly accessible, add a mnttab entry
	 * for the newly mounted NUCFS file system.  Drop the lock again
	 * because this operation may be long-running, but first hold the
	 * vnode, to avoid races with unmount.
	 */
	VN_HOLD(&(*foundAmfsNode)->vnode);
	AMfsAttachToParentNode(parentNode, *foundAmfsNode);
	UNLOCK_AMNODE (parentNode);
	if (AMfsAddNucfsMnttabEntry (nucCredentials,
			AMFS_NODE_NAME (parentNode),
			AMFS_NODE_NAME (*foundAmfsNode),
			&diagnostic) != SUCCESS)
		/*
		 * The entry was not added to the mnttab file.
		 */
		cmn_err (CE_NOTE, "AMfiNucFsMount: add mnttab failed.");

	LOCK_AMNODE (parentNode);
	return (NVLT_LEAVE(SUCCESS));
}

int
AMfiSeek (	vnode_t	*vnode,
		off_t	oldOffset,
		off_t	*newOffset)
{
	NVLT_ENTER (3);
	NVLT_PRINTF ("AMfiSeek: oldOffset = 0x%x, newOffset = 0x%x\n",
			oldOffset, *newOffset, 0);

	if (*newOffset < 0)
		return (NVLT_LEAVE (EINVAL));
	else
		return (NVLT_LEAVE (SUCCESS));
}
