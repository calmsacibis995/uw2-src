/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucam/nucam_vfsops.c	1.16"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucam/nucam_vfsops.c,v 1.15.2.1 1994/12/12 01:09:08 stevbam Exp $"

/*
**  Netware Unix Client
**
**	MODULE:
**		nucam_vfsops.c -	The NetWare UNIX Client Auto Mounter
**					Interface layer (AMfi) VFS operations
**					for VFS/VNODE Architecture.
**
**	ABSTRACT:
**		The nucam_vfsops.c contains the NetWare UNIX Client Auto Mounter
**		File System (NUCAM) vfs operations.  This is the NUCAM interface
**		layer (AMfi) for the VFS/VNODE Architecture Kernel.  The
**		AMfi layer is responsible for managing the interface between
**		the UNIX Generic File System and the AMfs layer.  The purpose
**		of the AMfi layer is to map the UNIX file system service
**		requests into the object oriented AMfs layer.  See
**		AMfiVfsOpsIntro(3K) for a complete description of these
**		operations.
**
**		The following AMfiVfsOps (struct vfsops) operations are
**		contained in this module:
**
**						AMfiinit()
**		(*vfs_op->vfs_mount)()		AMfiMount()
**		(*vfs_op->vfs_unmount)()	AMfiUnMount()
**		(*vfs_op->vfs_root)()		AMfiGetRootVnode()
**		(*vfs_op->vfs_statfs)()		AMfiStatVfs()
*/ 

#include <svc/clock.h>
#include <util/param.h>
#include <util/types.h>
#include <acc/priv/privilege.h>
#include <acc/mac/mac.h>
#include <svc/systm.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/pathname.h>
#include <fs/stat.h>
#include <io/uio.h>
#include <fs/mount.h>
#include <io/ioctl.h>
#include <fs/statvfs.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <util/cmn_err.h>

#include <net/nuc/nwctypes.h>
#include <net/nuc/nucerror.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <fs/nucam/nucam_common.h>
#include <fs/nucam/amfs_ops.h>
#include <net/tiuser.h>
#include <fs/nucam/amfs_node.h>
#include <fs/nucam/nucam_glob.h>

#include <util/mod/moddefs.h>

/*
 * nucam File System semaphore.
 */
int	nucamFileSystemSemaphore;

/*
 * AMfs node attribute time.
 */
uint32	amfsNodeTime;

/*
 * Define the tracing mask.
 */
#define NVLT_ModMask	NVLTM_am

/*
 * Reference NUCFS Virtual File System Interface (AMfi) VFS functions accessed
 * via amfiVfsOps.
 */
STATIC	int	AMfiGetRootVnode ();
STATIC	int	AMfiMount ();
STATIC	int	AMfiStatVfs ();
STATIC	int	AMfiUnMount ();


/* 
 *	Loadable driver stuff 
 */

int		AMfi_load();
int		AMfi_unload();

MOD_FS_WRAPPER(AMfi, AMfi_load, AMfi_unload, "Loadable NetWare AM File System");
void AMfiinit (vfssw_t *vfsswEntry);

extern	int	fs_nosys ();

struct	vfsops	amfiVfsOps = {
        AMfiMount,			/* vfs_mount		*/
        AMfiUnMount,			/* vfs_unmount		*/
        AMfiGetRootVnode,		/* vfs_root		*/
        AMfiStatVfs,			/* vfs_statvfs		*/
        fs_nosys,			/* vfs_sync		*/
        fs_nosys,			/* vfs_vget		*/
        fs_nosys,			/* vfs_mountroot	*/
        fs_nosys,			/* vfs_swapvp		*/
	fs_nosys,			/* vfs_setceiling	*/
        fs_nosys,			/* filler		*/
        fs_nosys,			/* filler		*/
        fs_nosys,			/* filler		*/
        fs_nosys,			/* filler		*/
        fs_nosys,			/* filler		*/
        fs_nosys,			/* filler		*/
	fs_nosys			/* filler		*/
};


/*
 * BEGIN_MANUAL_ENTRY(AMfiSVr4_1VfsOpsIntro(3K), \
 *		./man/kernel/nucam/amfi/SVr4_1/SVr4_1VfsOpsIntro )
 *
 * NAME
 *    AMfiSVr4_1VfsOpsIntro - Introduction to the NUCAM File System Interface
 *                            layer (AMfi) VFS operations.
 *
 * SYNOPSIS
 *    #include <nwctypes.h>
 *    #include <nucerror.h>
 *    #include <nuctune.h>
 *    #include <nucam_common.h>
 *    #include <amfs_ops.h>
 *    #include <nucam_glob.h>
 *    #include <nwctrace.h>
 *
 * DESCRIPTION
 *    The NetWare UNIX Client Auto Mounter (NUCAM) File System is broken into
 *    two layer: the Auto Mounter Interface layer (AMfi) and the Auto Mounter
 *    File System layer (AMfs).  The nucam_vfsops.c contains the Auto Mounter
 *    Interface layer (AMfi) VFS operations.
 *
 *    The NUCAM File System is designed to automatically mount network File
 *    Systems like NUCFS on directories within the NUCAM file system.  
 *    Directories are the only node type supported by the NUCAM File System.
 *    Each NUCAM File System is a sub-tree of the hierachical AMfs nodes.  The
 *    root of the NUCAM File System (AMFS_ROOT_T) is paired with the UNIX
 *    Generic File System mount structure to form the mount point to the NUCAM
 *    File System. 
 *
 * SEE ALSO
 *    AMfiGetRootVnode(3K),  AMfiMount(3K), AMfiStatVfs(3K), AMfiUnMount(3K).
 *
 * END_MANUAL_ENTRY
 */


/*
 * BEGIN_MANUAL_ENTRY(AMfiGetRootVnode(3K), \
 *		./man/kernel/nucam/amfi/SVr4_1/GetRootVnode )
 *
 * NAME
 *    (struct vfsops *)->(*vfs_root)() - Return root vnode of the specified VFS
 *                                       NUCAM File System.
 *
 * SYNOPSIS
 *    int
 *    AMfiGetRootVnode (vfsMountInstance, rootVnode)
 *    vfs_t	*vfsMountInstance;
 *    vnode_t	**rootVnode;
 *
 * INPUT
 *    vfsMountInstance - Fully populated VFS structure of the NUCAM VFS to
 *                       return root for.
 *
 * OUTPUT
 *    rootVnode        - A fully populated vnode resprsesenting the root vnode
 *                       of the NUCAM File System.
 *
 * RETURN VALUE
 *    0                - Successful Completion.
 *
 * DESCRIPTION
 *    The AMfiGetRootVnode returns a fully populated vnode representing the 
 *    root vnode of the specified VFS associated with the Auto Mounter File
 *    System.
 *
 * SEE ALSO
 *    AMfiVfsOpsIntro(3K), AMfiMount(3K), AMfiUnMount(3K)
 *
 * END_MANUAL_ENTRY
 */
STATIC	int
AMfiGetRootVnode (	vfs_t	*vfsMountInstance,
			vnode_t	**rootVnode )
{
	opaque_t	*amfsRoot;

	NVLT_ENTER (2);

	amfsRoot = (opaque_t *)vfsMountInstance->vfs_data;
	ASSERT (amfsRoot != NULL);
	ASSERT (AMFS_NODE_STAMP(amfsRoot) == AMFS_ROOT_NODE_STAMP);

	/*
	 * Get the root vnode associated with the amfsRoot node.
	 */
	*rootVnode = (struct vnode *)AMFS_VP(amfsRoot);

	/*
	 * Increment rootVnode's reference count.
	 */
	VN_HOLD (*rootVnode);
	return (NVLT_LEAVE (SUCCESS));
}


/*
 * AMfi_load(void)
 *	Dynamically load AMfi module.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns error if unable to init.
 *
 * Description:
 *	Dynamically load AMfi module.
 *
 * Parameters:
 *
 */

int
AMfi_load (void)
{
	struct	vfssw	*vswp;

        vswp = vfs_getvfssw("AMfi");
	if (vswp == NULL) {
		/*
		 *+ AMfi file system is not registered before
		 *+ attempting to load it.
		 */
		cmn_err(CE_NOTE, "!MOD: AMfi is not registered.");
		return (EINVAL);
	}
	AMfiinit(vswp);

	if ( amfiInitialized == FALSE ) {
		/*
		 *	it failed
		 */		
		return ( 1 );
	}

	/*
	 *	it worked!
	 */

	return ( 0 );
}

/*
 * AMfi_unload(void)
 *	Dynamically unload AMfi module.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns error if unable to unload.
 *
 * Description:
 *	Dynamically unload AMfi module.
 *
 * Parameters:
 *
 */

int
AMfi_unload (void)

{
	return ( 0 );
}

/*
 * BEGIN_MANUAL_ENTRY(AMfiinit(3K), \
 *		./man/kernel/nucam/amfi/SVr4_1/init )
 *
 * NAME
 *    (vfs_init)() - Performs the NetWare UNIX Client Auto Mounter File System
 *                   initialization.
 *
 * SYNOPSIS
 *    void
 *    AMfiinit (vfsswEntry)
 *    vfssw_t	*vfsswEntry;
 *
 * INPUT
 *    vfsswEntry             - Pointer to the File System Type Switch table 
 *                             representing the NetWare Unix Client Auto Mounter
 *                             File System entry.
 *    vfsswEntry->vsw_name   - NetWare UNIX Client Auto Mounter File System type
 *                             name string (AMfi).
 *    vfsswEntry->vsw_init   - NetWare UNIX Client Auto Mounter File System
 *                             initialization routine.
 *    vfsswEntry->vsw_flag   - Flags.
 *
 * OUTPUT
 *    vfsswEntry->vsw_vfsops - NetWare UNIX Client Auto Mounter File System VFS
 *                             operations.
 *
 * RETURN VALUE
 *    None.
 *
 * DESCRIPTION
 *    The AMfiinit is called to initalize NetWare UNIX Client Auto Mounter File
 *    System.  This operation is called exactly once, when the system is first
 *    started up.  All of NetWare UNIX Client Auto Mounter File System's memory
 *    regions are initialized in this routine.
 *
 * END_MANUAL_ENTRY
 */
void
AMfiinit (	vfssw_t	*vfsswEntry )
{
	NVLT_ENTER (1);

	/*
	 * Set the amfiInitialized global variable to FALSE to indicate that
	 * the AMfi layer momory region has not been initialized yet.
	 */
	amfiInitialized = FALSE;

	/*
	 * Create the nucamFileSystemSemaphore.
	 */
	nucamFileSystemSemaphore = -1;
	if (NWtlCreateAndSetSemaphore (&nucamFileSystemSemaphore, 1)) {
		cmn_err (CE_CONT,
			"AMfiinit: Create nucamFileSystemSemaphore failed.\n");
		nucamFileSystemSemaphore = -1;
		NVLT_LEAVE (FAILURE);
		return;
	}

	/*
	 * Set the NUCAM File System type to its index value in the File System
	 * Type Switch table (vfssw[]).
	 */
	nucamType = vfsswEntry - vfssw;

	/*
	 * Set the specified vfsswEntry->vsw_vfsops to the NUCAM VFS operations.
	 */
	vfsswEntry->vsw_vfsops = &amfiVfsOps;

	/* 
	 * Set the global amfiInitialized variable to indicate that the AMfi
	 * layer has been initialized successfully.
	 */
	amfiInitialized = TRUE;

	/*
	 * Initalize the active AMfs node count.
	 */
	amfsNodeCount = 0;

	/*
	 * Initialize the Number of NUCAM File Systems.  We only allow one 
	 * NUCAM File System to be mounted per machine.
	 */
	nucamMountCount = 0;

	/*
	 * amfsNodeTime is used to set AMfs node attributes times.
	 */
	amfsNodeTime = (uint32)hrestime.tv_sec;

	NVLT_LEAVE (SUCCESS);
	return;
}


/*
 * BEGIN_MANUAL_ENTRY(AMfiMount(3K), \
 *		./man/kernel/nucam/amfi/SVr4_1/Mount )
 *
 * NAME
 *    (struct vfsops *)->(*vfs_mount)() - Mounts a NUCAM root AMfs node as a
 *                                        VFS on a directory mount point.
 *
 * SYNOPSIS
 *    int
 *    AMfiMount (vfsMountInstance, vnodeMountPoint, mountArgs, unixCredentials)
 *    vfs_t		*vfsMountInstance;
 *    vnode_t		*vnodeMountPoint;
 *    struct	mounta	*mountArgs;
 *    cred_t		*unixCredentials;
 *
 * INPUT
 *    vfsMountInstance                   - Refers to a vfs_t structure that is
 *                                         being initialized by this operation; 
 *                                         upon successful completion of a mount
 *                                         it will be linked by the Generic File
 *                                         System into the VFS list.
 *    For REMOUNTS: (vfsMountInstance->vfs_flag & VFS_REMOUNT)
 *                                       - Fully populated VFS strucuture to the
 *                                         NUCAM VFS being remounted.
 *    For New MOUNTS:
 *    vfsMountInstance->vfs_next         - Initialized to NULL, not linked into
 *                                         mount list.
 *    vfsMountInstance->vfs_op           - Set to amfiVfsOps.
 *    vfsMountInstance->vfs_flag         - Initialized to 0.
 *    vfsMountInstance->vfs_data         - Initialized to NULL.
 *    vfsMountInstance->vfs_nsubmounts   - Set to 0.
 *    vnodeMountPoint                    - Fully populated vnode of the 
 *                                         directory mounting onto.
 *    mountArgs                          - Pointer to a mount argument sturcture
 *                                         containing the arguments to the 
 *                                         mount(2) system call.
 *    mountArgs->spec                    - Pointer to user address space path
 *                                         name string of the block device to
 *                                         mount.  
 *    mountArgs->dir                     - Pointer to user address space path
 *                                         name string of the directory
 *                                         'vnodeMountPoint' corresponds to.
 *    mountArgs->flags                   - Set to an inclusive OR of the
 *                                         following:
 *                                         MS_RDONLY  - Read only File System.
 *                                         MS_FSS     - Old 4 arguments mount.
 *                                         MS_DATA    - 6 arguments mount.
 *                                         MS_NOSUID  - Setuid disallowed.
 *                                         MS_REMOUNT - Reinitialze VFS.
 *                                         MS_NOTRUNC - Do not truncate
 *                                                      Long File Names.
 *    mountArgs->fstype                  - Pointer to user area address space
 *                                         name string of the NetWare Client
 *                                         Auto Mounter File System ("NUCAM").
 *    mountArgs->dataptr                 - Not used with NUCAM.
 *    mountArgs->datalen                 - Not Used with NUCAM.
 *    unixCredentials->cr_uid            - The effective user id of the process
 *                                         making the request.  This represents
 *                                         the credentials of the NetWare Client
 *                                         User.
 *    unixCredentials->cr_gid            - The effective group id of the process
 *                                         making the request.  This represents
 *                                         the UNIX group the NetWare Client
 *                                         User is using.
 *
 * OUTPUT (Not Applicable to MS_REMOUNT case)
 *    vfsMountInstance->vfs_bcount       - Set to 0.
 *    vfsMountInstance->vfs_bsize        - Set to 1024.
 *    vfsMountInstance->vfs_data         - Set to the NUCAM root AMfs node
 *                                         object associated with the VFS Mount
 *                                         instance.
 *    vfsMountInstance->vfs_dev          - NUCAM fabricated device representing
 *                                         the mount device.
 *    vfsMountInstance->vfs_flag         - Generic Managed.
 *    vfsMountInstance->vfs_fsid         - Set to the unique identifier (ie.
 *                                         internal identifier of this NUCAM
 *                                         File System). Comprised of the 
 *                                         concatenation of `vfs_dev' and
 *                                         'vfs_fstype'.
 *    vfsMountInstance->vfs_fstype       - Set to the NUCAM File System Type
 *                                         Index (nucamType).
 *    vfsMountInstance->vfs_next         - Generic Managed.
 *    vfsMountInstance->vfs_op           - Set to NUCAM VFS operations list.
 *    vfsMountInstance->vfs_vnodecovered - Generic Managed.
 *
 * RETURN VALUES
 *    0          - Successful Completion.
 *    [EFAULT]   - The 'mountArgs' are not in user address space.
 *    [EINVAL]   - The 'mountArgs' are improperly formatted.
 *    [ENOTDIR]  - The 'vnodeMountPoint' is not of type VDIR.
 *    [EPERM]    - Not super user, permission denied.
 *
 * DESCRIPTION
 *    The AMfiMount mounts a NUCAM File System root AMfs node as a VFS in the
 *    UNIX Generic File System.  The NUCAM root AMfs node is then available to
 *    UNIX processes which traverse the mount point.
 *
 * NOTES
 *    The MS_REMOUNT flag is requesting that the file system be brough back
 *    to an initial state on the same VFS it was originally mounted on.  The
 *    NUCAM File System simply returns, as everyting is idem-potent on the 
 *    RAM.
 *
 * SEE ALSO
 *    AMfiVfsOpsIntro(3K), AMfiUnMount(3K)
 *
 * END_MANUAL_ENTRY
 */
STATIC int
AMfiMount (	vfs_t		*vfsMountInstance,
		vnode_t		*vnodeMountPoint,
		struct	mounta	*mountArgs,
		cred_t		*unixCredentials )
{
	nwcred_t	nucCredentials;
	opaque_t	*amfsRoot = NULL;
	NUCAM_DIAG_T	diagnostic;

	NVLT_ENTER (4);

	if (nucamMountCount != 0) {
		cmn_err (CE_NOTE,"AMfiMount: One NUCAM File System allowed.\n");
		return (NVLT_LEAVE (EACCES));
	}

	/* Verify this process has the mount privilege.
	 */
	if( pm_denied(unixCredentials, P_MOUNT) ){
		return( NVLT_LEAVE(EACCES) );
	}

	if (vfsMountInstance->vfs_flag & VFS_REMOUNT)
		/*
		 * Remounting, nothing to do here, we are already mounted.
		 */
		return (NVLT_LEAVE (SUCCESS));

	if (vnodeMountPoint->v_type != VDIR)
		/*
		 * Attempting to mount onto a file.
		 */
		return (NVLT_LEAVE (ENOTDIR));

	if (vnodeMountPoint->v_count != 1 || vnodeMountPoint->v_flag & VROOT)
		/*
		 * Directory mounting onto is either busy or is marked as
		 * root.
		 */
		return (NVLT_LEAVE (EBUSY));

	/*
	 * Get a generic NUC credentials structure. 
	 */
	AMfiGetCredentials(unixCredentials, &nucCredentials); 
	
	/*
	 * Allocate a NUCAM root AMfs node for the new NUCAM File System to be
	 * mounted.
	 */
	LOCK_NUCAM_SPIN();
	if (nucamMountCount != 0) {
		UNLOCK_NUCAM_SPIN();
		/*
		 * Can only mount one NUCAM File System per NetWare UNIX Client.
		 */
		return (NVLT_LEAVE (FAILURE));
	}
	++nucamMountCount;
	UNLOCK_NUCAM_SPIN();

	if (AMfsMount (&nucCredentials, "nucam", vfsMountInstance, 
			&amfsRoot, &diagnostic) != SUCCESS) {
		LOCK_NUCAM_SPIN();
		--nucamMountCount;
		ASSERT(nucamMountCount == 0);
		UNLOCK_NUCAM_SPIN();
		return (NVLT_LEAVE (AMfiErrorMap (diagnostic)));
	}

	/*
	 * Set the vfsMountInstance fields.
	 */
	vfsMountInstance->vfs_bsize = NUCAM_LOGICAL_BLOCK_SIZE;
	vfsMountInstance->vfs_data = (caddr_t)amfsRoot;
	vfsMountInstance->vfs_fstype = nucamType;

	vfsMountInstance->vfs_dev = getudev();

	vfsMountInstance->vfs_fsid.val[0] = vfsMountInstance->vfs_dev;
	vfsMountInstance->vfs_fsid.val[1] = nucamType;
	vfsMountInstance->vfs_bcount = 0;

	/*
	 * Note that the root vnode will have its reference count set to one
	 * as part of initialization, and no further hold is needed here.
	 */

        SLEEP_LOCK(&vfslist_lock, PRIVFS);
        vfs_add(vnodeMountPoint, vfsMountInstance, mountArgs->flags);
        SLEEP_UNLOCK(&vfslist_lock);
	return (NVLT_LEAVE (SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(AMfiStatVfs(3K), \
 *		./man/kernel/nucam/amfi/SVr4_1/StatVfs )
 *
 * NAME
 *    (struct vfsops *)->(*vfs_statvfs)() - Returns statistics of a NUCAM
 *                                          File System.
 *
 * SYNOPSIS
 *    int
 *    AMfiStatVfs (vfsMountInstance, vfsStats)
 *    vfs_t	*vfsMountInstance;
 *    statfs_t	*vfsStats;
 *
 * INPUT
 *    vfsMountInstance   - Fully populated VFS strucuture to the NUCAM File
 *                         System to return statistics on.
 *
 * OUTPUT
 *    vfsStats->f_basetype - Name of the NetWare UNIX Client Auto Mounter set to
 *                           vfssw[vfsMountInstance->vfs_fstype].vsw_name
 *                           = "nucam".
 *    vfsStats->f_bsize    - Set to the NUCAM_LOGICAL_BLOCK_SIZE.
 *    vfsStats->f_bfree    - Total number of free blocks on the NUCAM File
 *                           System.
 *    vfsStats->f_bavail   - Same as f_bfree since there is no distinction
 *                           of quotas between Supervisor and others.
 *    vfsStats->f_blocks   - Total number of blocks on the NUCAM File System.
 *    vfsStats->f_files    - Total number of AMfs nodes on the NUCAM File
 *                           System.
 *    vfsStats->f_favail   - Same as f_ffree since there is no distinction
 *                           of quotas between Supervisor and others.
 *    vfsStats->f_ffree    - Total number of f_files on the NUCAM File System
 *                           which are free for use.
 *    vfsStats->f_files    - Total number of file objects possible on the
 *                           NUCAM File System.
 *    vfsStats->f_flag     - Set to an inclusive OR of the following flags:
 *                           ST_RDONLY  - Set if the NUCAM File System was
 *                                        mounted with the read only argument.
 *                           ST_NOSUID  - Set if the NUCAM File System was
 *                                        mounted with the nosuid argument.
 *                           ST_NOTRUNC - No truncation on long file names.
 *    vfsStats->f_frsize   - Same as f_bsize.  Since the fundamental size is 
 *                           abstracted to logical wire transfer the fragment
 *                           size is synonymous with block size.
 *    vfsStats->f_fsid     - Set to 'vfsMountInstance->vfs_fsid', which is the
 *                           unique identifier of the active NUCAM nodes.
 *    vfsStats->f_fstr     - Set to the "netware".
 *    vfsStats->f_namemax  - Maximum file name length in NUCAM File System.
 *
 * RETURN VALUE
 *    0          - Successful Completion.
 *
 * DESCRIPTION
 *    The AAMiStatVfs returns statistics about a NUCAM Volume to the Generic
 *    UNIX File System.
 *
 * END_MANUAL_ENTRY
 */
STATIC	int
AMfiStatVfs (	vfs_t		*vfsMountInstance,
		statvfs_t	*vfsStats )
{
	NVLT_ENTER (2);

	/*
	 * Make sure vfsMountInstance is not NULL. 
	 */
	ASSERT (vfsMountInstance != NULL);

	/*
	 * Populate the vfsStats Generic File System status sturcture with the
	 * information describing the NUCAM File System.
	 */
	vfsStats->f_bsize = NUCAM_LOGICAL_BLOCK_SIZE;
	vfsStats->f_frsize = 0;
	vfsStats->f_blocks = 0;
	vfsStats->f_bfree = 0;
	vfsStats->f_bavail = vfsStats->f_bfree;
	vfsStats->f_files = est_amfs_nodes;
	/*
	 * We do not bother with the nucam spin lock when reading
	 * the amfsNodeCount, below, since this can change anyway
	 * after this function returns the stats.
	 */
	vfsStats->f_ffree = vfsStats->f_files - amfsNodeCount;
	vfsStats->f_favail = vfsStats->f_files - amfsNodeCount;
	vfsStats->f_fsid = vfsMountInstance->vfs_dev;	
	strncpy (vfsStats->f_basetype,
		vfssw[vfsMountInstance->vfs_fstype].vsw_name, FSTYPSZ);
	vfsStats->f_namemax = AM_MAX_NAME_LENGTH;
	vfsStats->f_flag |= (ST_RDONLY|ST_NOTRUNC);
	strcpy (vfsStats->f_fstr, "NUCAM");

	return (NVLT_LEAVE (SUCCESS));
}


/*
 * BEGIN_MANUAL_ENTRY(AMfiUnMount(3K), \
 *		./man/kernel/nucam/amfi/SVr4_1/UnMount )
 *
 * NAME
 *    (struct vfsops *)->(*vfs_unmount)() - Un-Mounts a NUCAM File System from
 *                                          the UNIX Generic Files System.
 *
 * SYNOPSIS
 *    int
 *    AMfiUnMount (vfsMountInstance, unixCredentials)
 *    vfs_t	*vfsMountInstance;
 *    cred_t	*unixCredentials;
 *
 * INPUT
 *    vfsMountInstance        - Fully populated VFS strucuture to the NUCAM File
 *                              System to unmount.
 *    unixCredentials->cr_uid - The effective user id of the process making
 *                              the request.  This represents the credentials
 *                              of the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 *
 *
 * OUTPUT
 *    None.
 *
 * RETURN VALUE
 *    0                       - Successful Completion. 
 *    [EBUSY]                 - Vnodes are active in this VFS.
 *    [EPERM]                 - Not super user, permission denied.
 *
 * DESCRIPTION
 *    The AMfiUnMount unmounts a NUCAM File System from its associated VFS
 *    mount instance in the local UNIX Generic File System.  The NUCAM File
 *    System is no longer available to UNIX proess on the Client after 
 *    completion of the un-mount.
 *
 * NOTES
 *    The vfsMountInstance is removed from the vfs list by the Generic File
 *    System.
 *
 * SEE ALSO
 *    AMfiMount(3K)
 *
 * END_MANUAL_ENTRY
 */
STATIC	int
AMfiUnMount (vfsMountInstance, unixCredentials)
vfs_t	*vfsMountInstance;
cred_t	*unixCredentials;
{
	vnode_t		*rootVnode;
	nwcred_t	nucCredentials;
	AMFS_NODE_T	*amfsRootNode;
	ccode_t		returnCode;

	NVLT_ENTER (2);

	/*
	 * Get a generic NUCAM credentials structure.
	 */
	AMfiGetCredentials(unixCredentials, &nucCredentials);
	
	/*
	 * Get the root vnode of the specified vfsMountInstance.
	 */
	(void)AMfiGetRootVnode (vfsMountInstance, &rootVnode);
	VN_RELE (rootVnode);

	/*
	 * The reference count for our unbusy root vnode is 2 because
	 * the calling umount holds one reference, and the vfsp another.
	 */
	if (rootVnode->v_count > 2) {
		/*
		 * Other process(es) are referencing the rootVnode.
		 */
		cmn_err(CE_CONT, "NUCAM: Cannot unmount busy root node.\n"); 
		returnCode = EBUSY;
		goto fail;
	}

	/*
	 * The reference count to the rootVnode better be one at this
	 * point.
	 */
	ASSERT (rootVnode->v_count == 2);

	/*
	 * Get the NUCAM root AMfs node.
	 */
	amfsRootNode = (AMFS_NODE_T *)(rootVnode->v_data);

	/* 
	 * Unmount the NUCAM File System.
	 *
	 * NOTE:
	 *    The AMfsUnMount also releases the memory allocated for the
	 *    rootVnode.
	 */
	AMfsUnMount (amfsRootNode);

        /*
         * remove this vfs from list of vfs
         */
        SLEEP_LOCK(&vfslist_lock, PRIVFS);
        vfs_remove(vfsMountInstance);
        SLEEP_UNLOCK(&vfslist_lock);

	putudev(vfsMountInstance->vfs_dev);

	returnCode = SUCCESS;

fail:
	return ((int)NVLT_LEAVE (returnCode));
}
