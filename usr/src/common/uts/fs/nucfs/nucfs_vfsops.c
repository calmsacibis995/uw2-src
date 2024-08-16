/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-nuc:fs/nucfs/nucfs_vfsops.c	1.9.1.21"
#ident	"$Header: /SRCS/esmp/usr/src/nw/uts/fs/nucfs/nucfs_vfsops.c,v 2.61.2.14 1995/02/12 22:29:51 ram Exp $"

/*
**  Netware Unix Client
**
**	MODULE:
**		nucfs_vfsops.c -	The Virtual File System Interface layer
**					(NWfi) vfs operations for UnixWare
**					VFS/VNODE Generic File System.
**
**	ABSTRACT:
**		The nucfs_vfsops.c contains the NetWare UNIX Client File System
**		(NUCFS) vfs operations of the Virtual File System Interface
**		layer (NWfi) for UnixWare VFS/VNODE Architecture Kernels.  This
**		layer binds (interfaces) the portable file system (NWfs) layer
**              of the NUCFS into the UNIX Generic File System as a dependent
**		file system according to the architecture and semantics of the
**		VFS/VNODE.  See NWfiSVr4VfsOpsIntr(3K) for a complete
**		description of these operations.
**
**		The following NWfiVfsOps (struct vfsops) operations are
**		contained in this module:
**
**						NWfiinit()
**		(*vfs_op->vfs_mount)()		NWfiMount()
**		(*vfs_op->vfs_unmount)()	NWfiUnMount()
**		(*vfs_op->vfs_root)()		NWfiGetRootVnode()
**		(*vfs_op->vfs_statvfs)()	NWfiStatVfs()
**		(*vfs_op->vfs_sync)()		NWfiSyncVfs()
**		(*vfs_op->vfs_vget)()		NWfiGetVnodeByHandle()
**		(*vfs_op->vfs_mountroot()	NWfiMountRoot()
**		(*vfs_op->vfs_swapvp)()		NWfiGetSwapVnode()
*/ 

/*
 * XXX: Supress nested inclusion of <net/nuc/ncpconst.h>, which contains
 *	duplicate defintions with <fs/stat.h>. Also, we must surpress the
 *	inclusion of <net/nuc/ncpiopack.h>, for it depends upon
 *	<net/nuc/ncpconst.h>.
 */

#include <util/param.h>
#include <util/types.h>
#include <acc/priv/privilege.h>
#include <acc/mac/mac.h>
#include <svc/systm.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <proc/lwp.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/pathname.h>
#include <fs/stat.h>
#include <io/uio.h>
#include <net/tiuser.h>
#include <util/sysmacros.h>
#include <mem/kmem.h>
#include <mem/faultcatch.h>
#include <mem/vmparam.h>
#include <fs/mount.h>
#include <io/ioctl.h>
#include <fs/statvfs.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <fs/fs_subr.h>
#include <util/mod/moddefs.h>
#include <net/nuc/nwctypes.h>
#include <net/nuc/nuctool.h>
#include <net/nuc/nucerror.h>
#include <net/nuc/nwmp.h>
#include <net/nuc/spilcommon.h>
#include <util/nuc_tools/trace/nwctrace.h>
#include <fs/nucfs/nwfschandle.h>
#include <fs/nucfs/nucfscommon.h>
#include <fs/nucfs/nwficommon.h>
#include <fs/nucfs/nucfsspace.h>
#include <fs/nucfs/nwfsops.h>
#include <fs/nucfs/nucfsglob.h>
#include <fs/nucfs/nwfsname.h>
#include <fs/nucfs/nucfs_tune.h>

#include <fs/nucfs/nwfsvolume.h>
#include <fs/nucfs/nwfsnode.h>
#include <fs/nucfs/nwfidata.h>
#include <fs/nucfs/nwfimacro.h>

#include <net/nuc/requester.h>

#include <proc/proc.h>
#include <proc/user.h>
#include <svc/autotune.h>

#ifdef NUCFS_BOUND
#include <proc/bind.h>
#include <proc/disp.h>
#include <util/engine.h>
#endif /* NUCFS_BOUND */

/*
 * TBD:
 * extern declaration: find suitable header file.
 */
extern sleep_t	NWfiMountVolumeLock;

/*
 * Define the tracing mask.
 */
#define NVLT_ModMask	NVLTM_fs

/*
 * forward declarations
 */
void NWfiinit();
STATIC void NWfideinit();
int NWfi_load();
int NWfi_unload();
	/*
	 * NUCFS VFS operations.
	 */
int	NWfiGetRootVnode ();	
int	NWfiMount ();
int	NWfiStatVfs ();
int	NWfiUnMount ();
int	NWfiSyncVfs ();

/*
 * data declarations
 */
STATIC uint16	volumeIndex = 0;

MOD_FS_WRAPPER(NWfi, NWfi_load, NWfi_unload, "Loadable NetWare File System");

struct	vfsops	nwfiVfsOps = {
        NWfiMount,				/* vfs_mount		*/
        NWfiUnMount,				/* vfs_unmount		*/
        NWfiGetRootVnode,			/* vfs_root		*/
        NWfiStatVfs,				/* vfs_statvfs		*/
        NWfiSyncVfs,				/* vfs_sync		*/
        (int (*)())fs_nosys,			/* vfs_vget		*/
        (int (*)())fs_nosys,			/* vfs_mountroot	*/
        (int (*)())fs_nosys,			/* vfs_swapvp		*/
	(int (*)())fs_nosys,			/* vfs_setceiling	*/
        (int (*)())fs_nosys,			/* filler		*/
        (int (*)())fs_nosys,			/* filler		*/
        (int (*)())fs_nosys,			/* filler		*/
        (int (*)())fs_nosys,			/* filler		*/
        (int (*)())fs_nosys,			/* filler		*/
        (int (*)())fs_nosys,			/* filler		*/
	(int (*)())fs_nosys			/* filler		*/
};

STATIC	struct tune_point nucfs_maxnodes_curve[] = {
	{	8,	400,	TV_LINEAR },
	{	16,	800,	TV_LINEAR },
	{	32,	1600,	TV_LINEAR },
	{	64,	3200,	TV_STEP	  },
};

STATIC	struct tune_point nucfs_volumes_curve[] = {
	{	8,	40,	TV_LINEAR },
	{	16,	100,	TV_LINEAR },
	{	32,	400,	TV_LINEAR },
};

/*
 * Time related data.
 */
clock_t			NWfiStaleTicks;		/* TBD: Move to space.c */
NWFI_TIME_STAMP_T	NWfiBolt;

/*
 * BEGIN_MANUAL_ENTRY(NWfiSVr4_1VfsOpsIntro(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/SVr4_1VfsOpsIntro )
 *
 * NAME
 *    NWfiSVr4_1VfsOpsIntro - Introduction to the Virtual File System Interface
 *                            layer (NWfi) VFS operations.
 *
 * SYNOPSIS
 *    #include <nwctypes.h>
 *    #include <nucerror.h>
 *    #include <nucfscommon.h>
 *    #include <spilcommon.h>
 *    #include <nucfsspace.h>
 *    #include <nwfsops.h>
 *    #include <nucfsglob.h>
 *    #include <nwctrace.h>
 *
 * DESCRIPTION
 *    The NetWare UNIX Client File System (NUCFS) is broken into two layers: the
 *    Virtual File System Interface layer (NWfi) and the NetWare Client File
 *    System layer (NWfs).  The nucfs_vfsops.c contains the Virtual File
 *    System Interface layer (NWfi) VFS operations.
 *
 * SEE ALSO
 *    NWfiGetRootVnode(3K), NWfiMount(3K), NWfiStatVfs(3K), NWfiUnMount(3K)
 *
 * END_MANUAL_ENTRY
 */

/*
 * BEGIN_MANUAL_ENTRY(NWfiGetRootVnode(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/GetRootVnode )
 * NAME
 *    (vfsops_t *)->(*vfs_root)() - Returns the root vnode of the NUCFS file
 *                                  system associated with the specified
 *                                  vfsMountInstance.
 * SYNOPSIS
 *    int
 *    NWfiGetRootVnode (vfs_t *vfsMountInstance, vnode_t **rootVnode)
 * INPUT
 *    vfsMountInstance - Fully populated VFS strucuture to the NUCFS VFS
 *                       NetWare Volume to return root for.
 * OUTPUT
 *    rootVnode        - A fully populated vnode resprsesenting the root node
 *                       of the NUCFS file system.
 * RETURN VALUE
 *    0                - Successful Completion.
 * DESCRIPTION
 *    The 'NWfiGetRootVnode' returns a fully populated vnode representing
 *    root node of the specified VFS NetWare Volume.
 * Calling/Exit State:
 *    No locking assumptions are made.
 * END_MANUAL_ENTRY
 */
int
NWfiGetRootVnode(vfs_t *vfsMountInstance, vnode_t **rootVnode)
{
	NWFS_SERVER_VOLUME_T	*netwareVolume;
	NWFS_SERVER_NODE_T	*rootNode;
	NUCFS_ENGINE_DATA(oldengine);

	NVLT_ENTER (2);
	NUCFS_BIND(oldengine);
	netwareVolume = vfsMountInstance->vfs_data;
	NVLT_ASSERT (netwareVolume != NULL);
	/*
	 * Get the root server node associated with the given netware volume.
	 */
	NWfsGetRootNode(netwareVolume, &rootNode);
	/*
	 * Establish a hold on the vnode associated with the rootNode.
	 * The rootNode and the rootVnode should be a stable pair, since
	 * unmounts are held off in the generic OS before we get here.
	 * Since server nodes cannot go inactive except during unmount,
	 * no locking is necessary to go back and forth between gfsNode
	 * and the server node, for the exceptional case of root.
	 */
	NVLT_ASSERT(rootNode->gfsNode != NULL);
	*rootVnode = NWfiBindVnodeToSnode(rootNode, vfsMountInstance);
	NVLT_ASSERT(rootNode->gfsNode == *rootVnode);
	NVLT_LEAVE (SUCCESS);
	NUCFS_UNBIND(oldengine);
	return(0); 
}

/*
 * int
 * NWfi_load(void)
 *	Dynamically load NWfi module.
 * Calling/Exit State:
 *	No locking assumptions made. Returns error if unable to init.
 * Description:
 *	Dynamically load NWfi module.
 */
int
NWfi_load(void)
{
	struct	vfssw	*vswp;

	NVLT_ENTER(0);

        vswp = vfs_getvfssw("NWfi");
	if (vswp == NULL) {
		/*
		 *+ NWfi file system is not registered before
		 *+ attempting to load it.
		 */
		cmn_err(CE_NOTE, "!MOD: NWfi is not registered.");
		return(EINVAL);
	}
	NWfiinit(vswp);
	if ( nwfiInitialized == FALSE ) {
		return(ENODEV); /* ??? */
	}
	/*
	 * successful initialization.
	 */
	return NVLT_LEAVE(0);
}

/*
 * int
 * NWfi_unload(void)
 *	Dynamically unload NWfi module.
 * Calling/Exit State:
 *	No locking assumptions made. Returns error if unable to unload.
 * Description:
 *	Dynamically unload NWfi module.
 */
int
NWfi_unload(void)
{
	NVLT_ENTER(0);

	/*
	 * Deallocate all dynamically allocated storage:
	 *	- Locks (stat buffers, if allocated)
	 *	- Other.
	 */
	NWfideinit();
	nucfs_lock_deinit();
	NWfsDeInitNameTable();

	return NVLT_LEAVE(0);
}

/*
 * BEGIN_MANUAL_ENTRY(NWfiinit(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/init )
 *
 * NAME
 *    (vfs_init)() - Performs the NUCFS file system initialization.
 *
 * SYNOPSIS
 *    int
 *    NWfiinit (vfssw_t *vfsswEntry)
 *
 * INPUT
 *    vfsswEntry             - Pointer to the File System Type Switch table 
 *                             representing the NetWare Unix Client File System
 *                             entry.
 *    vfsswEntry->vsw_name   - NetWare UNIX Client File System type name string
 *                             (NWfi).
 *    vfsswEntry->vsw_init   - NetWare UNIX Client File System initialization
 *                             routine.
 *    vfsswEntry->vsw_flag   - Flags.
 *
 * OUTPUT
 *    vfsswEntry->vsw_vfsops - NetWare UNIX Client File System VFS operations.
 *
 * RETURN VALUE
 *    None.
 *
 * DESCRIPTION
 *    The NWfiinit is called to initalize NetWare UNIX Client File System.
 *    The following entities are initialized: 
 *		NWfsMountedVolumesList
 *		nucfsType 
 *		vfsswEntry->vsw_vfsops
 *		nwfsActiveNodeCount
 *		nwfsCacheNodeCount
 *		NWfiBolt
 *	  	nucfs_list_lock
 *	  	NWfiMountVolumeLock
 *	  	NWfiTimeLock
 *	  	NWfiNameTableLock
 *	  	nucfs_list_sv
 *		NWfsNameTable
 *		nwfiInitialized
 * Calling/Exit State:
 *	No locking assumptions are made.
 *
 * END_MANUAL_ENTRY
 */
void
NWfiinit(vfssw_t *vfsswEntry)
{
	NVLT_ENTER (1);

	/*
	 * Set the nwfiInitialized global variable to FALSE (default value) 
	 * to indicate that the NWfi layer has not yet been initialized.
	 */
	nwfiInitialized = FALSE;

	NWFI_LIST_INIT(&NWfsMountedVolumesList);

	/*
	 * Set the NetWare UNIX Client File System type to its index value in
	 * the File System Type Switch table (vfssw[]).
	 */
	nucfsType = vfsswEntry - vfssw;

	/*
	 * Set the specified vfsswEntry->vsw_vfsops to the NetWare Unix Client
	 * VFS operations.
	 */
	vfsswEntry->vsw_vfsops = &nwfiVfsOps;

	nwfsActiveNodeCount = 0;
	nwfsCacheNodeCount = 0;
	nwfsStaleNodeCount = 0;
	nwfsMaxNodes = tune_calc(nucfs_maxnodes_curve,
				(sizeof(nucfs_maxnodes_curve) /
					sizeof(struct tune_point)));
	nwfsMaxVolumes = tune_calc(nucfs_volumes_curve,
				(sizeof(nucfs_volumes_curve) /
					sizeof(struct tune_point)));
	nwfsVolumesCount = 0;
	nucfsDesperate = 0;
	nucfsResourceShortage = 0;

	/*
	 * NWfiStaleTicks presumably initialized as a tuned value?
	 */
	NWfiStaleTicks = NUCFS_STALE_TICKS;

	/*
	 * Initialize locks, sv's, etc.:
	 *	nucfs_list_lock
	 *	NWfiMountVolumeLock
	 *	NWfiTimeLock
	 *	NWfiNameTableLock
	 *	nucfs_list_sv
	 */
	nucfs_lock_init();

	/*
	 * Initialize the names table.
	 */
	NWfsInitNameTable();

	/* 
	 * Set the global nwfiInitalized variable to indicated that the NWfi
	 * layer has been initialized successfully.
	 */
	nwfiInitialized = TRUE;

	/*
	 * Start nucfs_timeout for periodically surveying which
	 * volumes are ready for attribute flush, resource handle
	 * closings, etc.
	 */
	nucfs_timeout_start();

	/* 
	 * NWfi init routine messages at the boot time.
	 */
	cmn_err(CE_CONT," (NWfiinit) NetWare UNIX Client File System V"
			NUCFS_VERSION_STRING "\n");

#if defined(DEBUG) || defined(DEBUG_TRACE)
	cmn_err(CE_CONT, "nwfsMaxNodes=%d\tnwfsMaxVolumes=%d\n",
		nwfsMaxNodes, nwfsMaxVolumes);
#endif
	NVLT_VLEAVE();
	return;
}

STATIC void
NWfideinit(void)
{
	NVLT_ENTER(0);

	nucfs_timeout_stop();

	NVLT_VLEAVE();
}

/*
 * BEGIN_MANUAL_ENTRY(NWfiMount(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/Mount )
 *
 * NAME
 *    (vfsops_t *)->(*vfs_mount)() - Mounts a NetWare Volume as a VFS on a
 *                                   directory mount point.
 *
 * SYNOPSIS
 *    int
 *    NWfiMount (vfs_t *vfsMountInstance, vnode_t *vnodeMountPoint, 
 *			struct mounta *mountArgs, cred_t *unixCredentials)
 * INPUT
 *    vfsMountInstance                   - Refers to a vfs_t structure that is 
 *                                         being initialized by this operation;
 *                                         upon successful completion of a mount
 *                                         it will be linked by the Generic 
 *                                         File System into the VFS list.
 *    vfsMountInstance->vfs_next         - Initialized to NULL, not linked into
 *                                         mount list.
 *    vfsMountInstance->vfs_op           - Set to nwfiVfsOps.
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
 *                                         mount.  Has no meaning with NetWare,
 *                                         the Server:Volume is passed in
 *                                         'mountArgs->dataptr'.
 *    mountArgs->dir                     - Pointer to user address space path
 *                                         name string of the directory
 *                                         'vnodeMountPoint' corresponds to.
 *					   Not referenced within this function.
 *    mountArgs->flags                   - Set to an inclusive OR of the
 *                                         following:
 *                                         MS_RDONLY  - Read only File System.
 *                                         MS_FSS     - Old 4 arguments mount.
 *                                         MS_DATA    - 6 arguments mount.
 *                                         MS_NOSUID  - Setuid disallowed.
 *                                         MS_REMOUNT - Reinitialze VFS.
 *                                         MS_NOTRUNC - Do not truncate
 *                                                      Long File Names.
 *                                         MS_NUCAM   - The mountArgs->dataptr
 *                                                      was allocated in the
 *                                                      kernel.  Use bcopy to
 *                                                      copy NWFI_MOUNT_ARGS_T.
 *    mountArgs->fstype                  - Pointer to user area address space
 *                                         name string of the NetWare Client
 *                                         File System (ie. "NUCFS").
 *    mountArgs->dataptr                 - Pointer to user address space 
 *                                         (NWFI_MOUNT_ARGS_T) structure
 *                                         containing the server and volume to
 *                                         be mounted:
 *    (NWFI_MOUNT_ARGS_T *)(mountArgs->dataptr)->address 
 *					   A netbuf structure, containing the
 *					   buffer in which server name is
 *					   stored. address.maxlen contains the
 *					   maximum length of the buffer, while
 *					   address.len contains the actual
 *					   length of the buffer. address.buf
 *					   has the pointer to the buffer.
 *    (NWFI_MOUNT_ARGS_T *)(mountArgs->dataptr)->volumeName _
 *                                         NetWare volume name to be mounted.
 *    (NWFI_MOUNT_ARGS_T *)(mountArgs->dataptr)->mountFlags -
 *                                         Mount flags. Set to exclusive OR of 
 *                                         the following:
 *                                         NWFI_USE_UID - Use the user ID in the
 *                                                        credStruct for mount.
 *                                         NWFI_USE_GID - Use the group ID in 
 *                                                        the credStruct for
 *                                                        mount.
 *    (NWFI_MOUNT_ARGS_T *)(mountArgs->dataptr)->credStruct -
 *                                         Credentials structure of the user
 *                                         mounting the NUCFS File System.
 *    mountArgs->datalen                 - Set to sizeof (NWFI_MOUNT_ARGS_T);
 *    unixCredentials->cr_uid            - The effective user id of the process
 *                                         making the request.  This represents
 *                                         the credentials of the NetWare Client
 *                                         User.
 *    unixCredentials->cr_gid            - The effective group id of the process
 *                                         making the request.  This represents
 *                                         the UNIX group the NetWare Client
 *                                         User is using.
 *
 * OUTPUT: 
 *    vfsMountInstance->vfs_bcount       - Set to 0.
 *    vfsMountInstance->vfs_bsize        - Set to logical blocks size of the
 *                                         NetWare Server Volume (ie. negotiated
 *                                         wire block size).
 *    vfsMountInstance->vfs_data         - Set to the NUCfs (NWfs) serverVolume
 *                                         object associated with the VFS Mount
 *                                         instance.
 *    vfsMountInstance->vfs_dev          - NUCfs fabricated device representing
 *                                         the mount device.
 *    vfsMountInstance->vfs_flag         - Generic Managed.
 *    vfsMountInstance->vfs_fsid         - Set to the unique identifier (ie.
 *                                         internal identifier of this NetWare
 *                                         active Volume. Comprised of the 
 *                                         concatenation of `vfs_dev' and
 *                                         'vfs_fstype'.
 *    vfsMountInstance->vfs_fstype       - Set to the File System Type Index
 *                                         of the NUCfsType.
 *    vfsMountInstance->vfs_next         - Generic Managed.
 *    vfsMountInstance->vfs_op           - Set to NUCFS VFS operations list.
 *    vfsMountInstance->vfs_vnodecovered - Generic Managed.
 *
 * RETURN VALUES
 *    0          - Successful Completion.
 *    [EFAULT]   - The 'mountArgs' are not in user address space.
 *    [EINVAL]   - The 'mountArgs' are improperly formatted.
 *    [ENOLINK]  - NetWare Server connection has been lost.
 *    [ENOTDIR]  - The 'vnodeMountPoint' is not of type VDIR.
 *    [EPERM]    - Not super user, permission denied.
 *
 * DESCRIPTION
 *    The 'NWfiMount' mounts a distributed NetWare Volume on a NetWare Server
 *    as a VFS mount point in the UNIX Generic File System.  The NetWare Volume
 *    is then available to UNIX processes which traverse the mount point.
 *
 * NOTES
 *    1. The MS_REMOUNT option is not supported for NUCFS.
 *    2. The MS_NOTRUNC flag is implied in NetWare, as we do not truncate 
 *       long file names and will always return ENAMETOOLONG.
 * SEE ALSO
 *    NWfiIntroduction(3K), NWfiGetRootVnode(3K), NWfiMountRoot(3K),
 *    NWfiUnMount(3K)
 * Calling/Exit State:
 *    No locking assumptions are made.
 *
 * END_MANUAL_ENTRY
 */
int
NWfiMount(vfs_t *vfsMountInstance, vnode_t *vnodeMountPoint, 
		struct mounta *mountArgs, cred_t *unixCredentials)
{
	NWFI_MOUNT_ARGS_T	nucfsMountArgs;
	NUCFS_VOLUME_STATS_T	volumeStats;
	NWFS_CRED_T		nwfsCredentials;	
	NWFS_SERVER_VOLUME_T	*netwareVolume;
	NWFS_SERVER_NODE_T	*netwareRootNode; 
	NWFI_VOLFLUSH_DATA_T	*volFlushData;
	vnode_t			*rootVnode;
	uint32			volumeFlags = 0;
	enum NUC_DIAG		diagnostic = SUCCESS;
	int32			returnCode;
	struct netbuf 		address;
	char 			addressBuffer[MAX_ADDRESS_SIZE];
	cred_t			dummyUnixCredentials;
	iovec_t			nwiov;
	uio_t			nwUio = {
				  NULL, 		/*   *uio_iov */
				  1,			/*   uio_iovcnt */
				  0,			/*   uio_offset */
				  0,			/*   uio_segflg */
				  UIO_WRITE,		/*   uio_fmode */
				  0,			/*   uio_limit */
				  0			/*   uio_resid */
				};
	int			nbytesToRead;
	uint_t			catch_flags;
	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);
	NVLT_ENTER (4);

	nwUio.uio_iov = &nwiov;

	/*
	 * Verify that this is not an old-style (4 arguments) mount.
	 */
	if ((mountArgs->flags & (MS_FSS|MS_DATA)) != MS_DATA) {
		returnCode = ENOSYS;
		goto done;
	}
	if (vfsMountInstance->vfs_flag & VFS_REMOUNT) {
		/*
		 * Remounting. For now ignore remount option.
		 */
		returnCode = ENOSYS;
		goto done;
	}


	if (vnodeMountPoint->v_type != VDIR) {
		/*
		 * Attempting to mount onto a file.
		 */
		returnCode = ENOTDIR;
		goto done;
	}

	/*
	 * TBV-
	 * What really protects the inference that it is okay to mount
	 * on the vnodeMountPoint based on its v_count being 1 at this
	 * point in time, -- subsequently?
	 */
	if ((vnodeMountPoint->v_count != 1) || 
			(vnodeMountPoint->v_flag & VROOT)) {
		/*
		 * Directory mounting onto is either busy or not marked as
		 * root.
		 */
		returnCode = EBUSY;
		goto done;
	}

	/*
	 * Verify NUCFS mount arguments.
	 */
	if (mountArgs->datalen != sizeof (NWFI_MOUNT_ARGS_T)) {
		/*
		 * Invalid datalen.
		 */
		returnCode = EINVAL;
		goto done;
	}

	/*
	 * Bring in the necessary data for mounts:
	 *
	 * 1. Get the NUCFS mount arguments.
	 */
	nbytesToRead = sizeof(NWFI_MOUNT_ARGS_T);
	nwiov.iov_base = mountArgs->dataptr;
	nwUio.uio_offset = 0;
	nwUio.uio_resid = nwiov.iov_len = nbytesToRead;
	nwUio.uio_limit = nbytesToRead;

	/*
	 * Unlike NFS mounts, NUCFS mounts originating from nucam
	 * are initiated in context of ordinary (non-root) processes.
	 * Therefore, root privilege is waived for nucfs mounts that
	 * originate from the kernel.
	 */
	if (mountArgs->flags & MS_SYSSPACE) {
		/*
		 * Set up for system space arguments.
		 */
		nwUio.uio_segflg = UIO_SYSSPACE;
		catch_flags = CATCH_ALL_FAULTS;

		/*
		 * Verify that the arguments data is truely in system space.
		 */
		if (!KADDR(nwiov.iov_base) ||
		    !KADDR(nwiov.iov_base + nwUio.uio_resid)) {
			returnCode = EINVAL;
			goto done;
		}
	} else {
		nwUio.uio_segflg = UIO_USERSPACE;
		catch_flags = CATCH_KERNEL_FAULTS;

		/*
		 * Only root can mount this NUC File System.
		 */
		if (pm_denied(unixCredentials, P_MOUNT)) {
			returnCode = EACCES;
			goto done;
		}
	}

	if ((returnCode = uiomove_catch(&nucfsMountArgs, nbytesToRead,
				  UIO_WRITE, &nwUio, catch_flags)) != 0) {
		goto done;
	}

	if ( nucfsMountArgs.address.len > MAX_ADDRESS_SIZE ) {
		returnCode = NWfiErrorMap(SPI_NAME_TOO_LONG);
		goto done;
	}

	/*
	 * 2. Copy the data from the buffer pointed to by 
	 * nucfsMountArgs.address.buf into the addressBuffer[]
	 * provided on stack. 
	 */
	nbytesToRead = nucfsMountArgs.address.len;
	nwiov.iov_base = nucfsMountArgs.address.buf;
	nwUio.uio_offset = 0;
	nwUio.uio_resid = nwiov.iov_len = nbytesToRead;
	nwUio.uio_limit = nbytesToRead;

	/*
	 * Verify that system space arguments are truely in system space.
	 */
	if ((mountArgs->flags & MS_SYSSPACE) && (!KADDR(nwiov.iov_base) ||
	    !KADDR(nwiov.iov_base + nwUio.uio_resid))) {
		returnCode = EINVAL;
		goto done;
	}

	if ((returnCode = uiomove_catch(addressBuffer,
					nucfsMountArgs.address.len,
					UIO_WRITE, &nwUio,
					catch_flags)) != 0) {
		goto done;
	}

	/*
	 * Finished reading in the server and volume name data, required
	 * for the mount to occur.
	 */

	/*
	 * Prepare "address", so that it describes the addressBuffer.
	 * It will be passed down to NWfsMountVolume, later below.
	 */
	address.maxlen = MAX_ADDRESS_SIZE;
	address.len = nucfsMountArgs.address.len;
	address.buf = addressBuffer;

	/*
	 * Fixup the credentials, taking NWFI_USE_UID, NWFI_USE_GID into
	 * account.
	 */
	bcopy(unixCredentials, &dummyUnixCredentials, sizeof(cred_t));
	dummyUnixCredentials.cr_uid =
		((nucfsMountArgs.mountFlags & NWFI_USE_UID) ?
			nucfsMountArgs.credStruct.cr_uid :
			u.u_lwpp->l_cred->cr_uid);
	dummyUnixCredentials.cr_gid =
		((nucfsMountArgs.mountFlags & NWFI_USE_GID) ?
			nucfsMountArgs.credStruct.cr_gid :
			u.u_lwpp->l_cred->cr_gid);

	/*
	 * now convert to the NUCFS credentials.
	 */
	NWfiUnixToFsCred(&dummyUnixCredentials, &nwfsCredentials);

	/*
	 * prepare volumeFlags.
	 */
	if (nucfsMountArgs.mountFlags & NWFI_INHERIT_PARENT_GID) {
		/*
		 * When creating new nodes, set the new node's GID to the parent
		 * directory's GID.
		 */
		volumeFlags |= NUCFS_INHERIT_PARENT_GID;
	}
	if (mountArgs->flags & MS_RDONLY) {
		/* 
		 * NUC File System being mounted is a read only file system.
		 */
		volumeFlags |= NUCFS_VOLUME_READ_ONLY;
	}

        SLEEP_LOCK(&NWfiMountVolumeLock, PRIVFS);

	NUCFS_LIST_LOCK();
	if (nwfsVolumesCount >= nwfsMaxVolumes) {
		NUCFS_LIST_UNLOCK();
		SLEEP_UNLOCK(&NWfiMountVolumeLock);
		cmn_err(CE_NOTE, "NWfiMount: Reached maximum of %d mounted"
			"volumes. Mount attempt failed",
			nwfsMaxVolumes);
		goto fail;
	} else {
		++nwfsVolumesCount;
		NUCFS_LIST_UNLOCK();
	}


	if (NWfiFlushInit(&volFlushData, volumeIndex)) {
		NUCFS_LIST_LOCK();
		--nwfsVolumesCount;
		NUCFS_LIST_UNLOCK();
		SLEEP_UNLOCK(&NWfiMountVolumeLock);
		returnCode = ENOMEM;
		goto fail;
	}
	
	/*
	 * Allocate a NetWare volume object for the new NUC File System to be
	 * mounted.  The root NetWare node is also allocated.
	 */
	if (NWfsMountVolume(
		&nwfsCredentials, 		/* credentials */
		&address,			/* serverAddress */
		nucfsMountArgs.volumeName, 	/* volumeName */
		volumeFlags, 			/* volumeFlags */
		&netwareVolume, 		/* (serverVolume) */
		&netwareRootNode, 		/* (rootNode) */
		&volumeStats, 			/* volumeStats */
		volFlushData,			/* flush daemons data */
		&diagnostic			/* diagnostic */
			) != SUCCESS) {
		returnCode = NWfiErrorMap (diagnostic);
        	SLEEP_UNLOCK(&NWfiMountVolumeLock);
		NWfiFlushDeInit(volFlushData);
		NUCFS_LIST_LOCK();
		--nwfsVolumesCount;
		NUCFS_LIST_UNLOCK();
		goto fail;
	}
       	SLEEP_UNLOCK(&NWfiMountVolumeLock);

	NWfiFlushActivate(netwareVolume, volFlushData);

	/*
	 * Set the vfsMountInstance fields.
	 */
	vfsMountInstance->vfs_bsize = volumeStats.logicalBlockSize;
	vfsMountInstance->vfs_fstype = nucfsType;
	vfsMountInstance->vfs_data = netwareVolume;
	if ((vfsMountInstance->vfs_dev = getudev()) == NODEV) {
                /*
                 *+ Could not get a unique device number for processor fs.
                 */
                cmn_err(CE_WARN, "NWfiMount: cannot get unique device number");
                vfsMountInstance->vfs_dev = 0;
        }
	NUCFS_LIST_LOCK();
	++volumeIndex;
	NUCFS_LIST_UNLOCK();
	vfsMountInstance->vfs_fsid.val[0] = vfsMountInstance->vfs_dev;
	vfsMountInstance->vfs_fsid.val[1] = nucfsType;
	vfsMountInstance->vfs_bcount = 0;
	vfsMountInstance->vfs_count = 0;	/* TBV */
	/*
	 * Allocate a vnode and bind it to the netwareRootNode.
	 */
	rootVnode = NWfiBindVnodeToSnode (netwareRootNode, vfsMountInstance);
	NVLT_ASSERT(rootVnode != NULL);
        SLEEP_LOCK(&vfslist_lock, PRIVFS);
	/*
	 * pass the mountArgs->flags into vfs_add, which will take care
	 * of merging in any additional flags (MS_NOSUID, MS_RDONLY)
	 * into vfsMountInstance->vfs_flag, as appropriate.
	 */
        vfs_add(vnodeMountPoint, vfsMountInstance, mountArgs->flags);
        SLEEP_UNLOCK(&vfslist_lock);
#if defined(DEBUG) || defined(DEBUG_TRACE)
	cmn_err(CE_CONT,"\nNWfiMount: SUCCESS: rootVnode=0x%x vcnt=%d ",
		rootVnode, rootVnode->v_count);
#endif /* DEBUG || DEBUG_TRACE */

	returnCode = 0;
fail:
done:
	NVLT_LEAVE(returnCode);
	NUCFS_UNBIND(oldengine);
	return(returnCode);
}


/*
 * BEGIN_MANUAL_ENTRY(NWfiStatVfs(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/StatVfs )
 *
 * NAME
 *    (vfsops_t *)->(*vfs_statvfs)() - Returns statistics for a NetWare
 *                                     Volume file system.
 *
 * SYNOPSIS
 *    int
 *    NWfiStatVfs (vfs_t *vfsMountInstance, statvfs_t *vfsStats)
 * INPUT
 *    vfsMountInstance     - Fully populated VFS strucuture to the NUCfs VFS
 *                           NetWare Volume to return statistics on.
 * OUTPUT
 *    vfsStats->f_basetype - Name of the NetWare Client File System set to
 *                           vfssw[vfsMountInstance->vfs_fstype].vsw_name
 *                           = "NUCFS".
 *    vfsStats->f_bsize    - Set to the logical block size (Negotiated wire
 *                           transfer size) of the NetWare Volume.
 *    vfsStats->f_bfree    - Total number of f_frsize blocks on the NetWare 
 *                           volume free for use.
 *    vfsStats->f_bavail   - Same as f_bfree since there is no distinction
 *                           of quotas between Supervisor and others.
 *    vfsStats->f_blocks   - Total number of f_frsize blocks on the NetWare
 *                           volume.
 *    vfsStats->f_favail   - Same as f_ffree since there is no distinction
 *                           of quotas between Supervisor and others.
 *    vfsStats->f_ffree    - Total number of f_files on the NetWare volume
 *                           which are free for use.
 *    vfsStats->f_files    - Total number of file objects possible on the
 *                           NetWare volume.
 *    vfsStats->f_flag     - Set to an inclusive OR of the following flags:
 *                           ST_RDONLY  - Set if the NetWare Volume was mounted
 *                                        with the read only argument, or if the
 *                                        NetWare Volume is read only.
 *                           ST_NOSUID  - Set if the NetWare Volume was mounted
 *                                        with the nosuid argument.
 *                           ST_NOTRUNC - Always set, NetWare does truncate
 *                                        file names.
 *    vfsStats->f_frsize   - Same as f_bsize.  Since the fundamental size is 
 *                           abstracted to logical wire transfer the fragment
 *                           size is synonymous with block size.
 *    vfsStats->f_fsid     - Set to 'vfsMountInstance->vfs_fsid', which is the
 *                           unique identifier of the active NetWare Volume.
 *    vfsStats->f_fstr     - Set to the name of the volume the VFS is mounted
 *			     on.  This is stored as a null-terminated string.
 *			     The sizeof(unsigned long) space at the end of the
 *			     array is used to hold the number of seconds since
 *			     the last time there were any active references to
 *			     any files in this filesystem (besides the root);
 *			     this must be copied into/from a properly-aligned
 *			     unsigned long variable; it must not be accessed
 *			     directly.  XXX - not sure if this works on non-
 *			     byte-oriented machines.
 *    vfsStats->f_namemax  - Maximum file name length in NetWare Volume. Set to
 *                           14 for DOS Name Space or 255 for UNIX Name Space.
 * RETURN VALUE
 *    0          - Successful Completion.
 *    [ENOLINK]  - NetWare Server connection has been lost.
 * DESCRIPTION
 *    The 'NWfiStatVfs' returns statistics about a NetWare Volume to the
 *    Generic UNIX File System.
 * SEE ALSO
 *    NWfiIntroduction(3K), NWfiMount(3K), NWfiMountRoot(3K)
 * Calling/Exit State:
 *    No locking assumptions are made.
 *
 * END_MANUAL_ENTRY
 */
int
NWfiStatVfs(vfs_t *vfsMountInstance, statvfs_t *vfsStats)
{
	NUCFS_VOLUME_STATS_T	volumeStats;
	enum NUC_DIAG		diagnostic = SUCCESS;
	NWFS_SERVER_NODE_T	*netwareRootNode;
	NWFS_SERVER_VOLUME_T	*volume;
	NWFI_CLOCK_T		curTime;
	unsigned long		timeSinceActive;
	uint_t			maxName, len;
	int32			returnCode;
	NWFS_CRED_T		nwfsCredentials;
	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER (2);

	volumeStats.serverAddress.buf = volumeStats.buffer;
	volumeStats.serverAddress.maxlen = MAX_ADDRESS_SIZE;

	/*
	 * Make sure vfsMountInstance is not NULL. 
	 */
	NVLT_ASSERT (vfsMountInstance != NULL);

	/*
	 * Get the NUCFS root node.
	 */
	NVLT_ASSERT(vfsMountInstance->vfs_data != NULL);
	netwareRootNode = ((NWFS_SERVER_VOLUME_T *)
				(vfsMountInstance->vfs_data))->rootNode;
	NVLT_ASSERT (SNODE_REASONABLE(netwareRootNode));
	volume = netwareRootNode->nodeVolume;

	/*
	 * Obtain the caller's credentials.
	 */
	NWfiUnixToFsCred(CRED(), &nwfsCredentials);

	/*
	 * Load the volume statistics structure with the NetWare file system
	 * volume information.
	 */
	if (NWfsVolumeStatistics (
			netwareRootNode, 
			&volumeStats, 
			&nwfsCredentials,
			&diagnostic
				) != SUCCESS) {
		returnCode = NWfiErrorMap (diagnostic);
		NVLT_LEAVE (returnCode);
		NUCFS_UNBIND(oldengine);
		return(returnCode);
	}

	/*
	 * Populate the vfsStats generic file system status sturcture with the
	 * information describing the NUC file system volume.
	 */
	vfsStats->f_bsize = volumeStats.logicalBlockSize;
	vfsStats->f_frsize = volumeStats.logicalBlockSize;
	vfsStats->f_blocks = volumeStats.totalBlocks;
	vfsStats->f_bfree = volumeStats.totalFreeBlocks;
	vfsStats->f_bavail = volumeStats.totalFreeBlocks;
	vfsStats->f_files = volumeStats.totalNodes;
	vfsStats->f_ffree = volumeStats.totalFreeNodes;
	vfsStats->f_favail = volumeStats.totalFreeNodes;

	vfsStats->f_fsid = vfsMountInstance->vfs_dev; /* TBV */

	strncpy (vfsStats->f_basetype,
		vfssw[vfsMountInstance->vfs_fstype].vsw_name, FSTYPSZ);

	if (!(volumeStats.volumeFlags & NWFS_DOS_NAME_SPACE))
		vfsStats->f_namemax = MAX_UNIX_FILE_NAME;
	else
		vfsStats->f_namemax = MAX_DOS_FILE_NAME;

	/*
	 * Set the f_flag field:
	 */
	vfsStats->f_flag = vf_to_stf(vfsMountInstance->vfs_flag);
	if (volumeStats.volumeFlags & NUCFS_VOLUME_READ_ONLY)
		vfsStats->f_flag |= ST_RDONLY;
	vfsStats->f_flag |= ST_NOTRUNC;			/* TBV ? */

	/*
	 * Compute the amount of space available in f_fstr for the
	 * visible fs-dependent string.  We subtract off sizeof
	 * timeSinceActive since we will be stuffing it into the end
	 * of the f_fstr array.  maxName includes the null byte
	 * terminator, which we will always place on the string;
	 * it also is used for the offset for timeSinceActive.
	 */
	maxName = sizeof(vfsStats->f_fstr) - sizeof(timeSinceActive);

	/*
	 * Stick the volume name into f_fstr, but clip it if it's too long.
	 */
	len = strlen(volumeStats.volumeName);
	if (len >= maxName)
		len = maxName - 1;
	bcopy(volumeStats.volumeName, vfsStats->f_fstr, len);
	vfsStats->f_fstr[len] = '\0';

	/*
	 * Compute the time since last activity and stuff it into the
	 * end of the f_fstr array.
	 */
	timeSinceActive = 0;
	NUCFS_LIST_LOCK();
	if (volume->activeOrTimedCount + volume->staleCount == 1) {
		NWFI_CLOCK_T activeStamp = volume->activeStamp;

		NUCFS_LIST_UNLOCK();
		NWFI_GET_CLOCK(curTime);
		timeSinceActive = (curTime - activeStamp) / HZ;
	} else
		NUCFS_LIST_UNLOCK();
	bcopy(&timeSinceActive,
	      vfsStats->f_fstr + maxName,
	      sizeof(timeSinceActive));

	NVLT_LEAVE (SUCCESS);
	NUCFS_UNBIND(oldengine);
	return(0);
}
/*
 * BEGIN_MANUAL_ENTRY(NWfiSyncVfs(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/SyncVfs )
 * NAME
 *    (vfsops_t *)->(*vfs_sync)() - Flushed old cached NUCFS vnodes. 
 *
 * SYNOPSIS
 *    NWfiSyncVfs (vfs_t *vfsp, int flag, cred_t *unixCredentials)
 * INPUT
 *    vfsp                    - Set to NULL to indicate that all mounted NUCFS
 *                              file systems should flush their old vnodes.
 *    flag                    - Set to SYNC_ATTR or SYNC_CLOSE.
 *    unixCredentials->cr_uid - The effective user id of the process making
 *                              the request.  This represents the credentials
 *                              of the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 * OUTPUT
 *    None.
 * RETURN VALUE
 *    0        - Always returns success. 
 * DESCRIPTION
 *    The 'NWfiSyncVfs' function flushes the old cached NUCFS vnodes.
 *
 * END_MANUAL_ENTRY
 */
/* ARGSUSED */
int
NWfiSyncVfs (vfs_t *vfsp, int flag, cred_t *unixCredentials)
{
	STATIC void NWfiSyncAll(void);

	NUCFS_ENGINE_DATA(oldengine);

	if (flag & SYNC_ATTR) {
		/*
		 * NUCFS takes care of its own attribute syncing.
		 * We don't need to do anything here.
		 */
		return 0;
	}

	NVLT_ENTER (3);

	NUCFS_BIND(oldengine);

	if (vfsp == NULL)
		NWfiSyncAll();
	else {
		NVLT_ASSERT(vfsp->vfs_data != NULL);

		NWfsVolumeSync((NWFS_SERVER_VOLUME_T *)vfsp->vfs_data);
	}

	NUCFS_UNBIND(oldengine);

	return NVLT_LEAVE(0);
}

STATIC void
NWfiSyncAll(void)
{
	NWFS_SERVER_VOLUME_T	*volume;
	NWFI_LIST_T		*listP;

	NVLT_ENTER(0);

	SLEEP_LOCK(&NWfiMountVolumeLock, PRIVFS);
	for (listP = NWFI_NEXT_ELEMENT(&NWfsMountedVolumesList);
			listP != &NWfsMountedVolumesList;
			listP = NWFI_NEXT_ELEMENT(listP)) {
		volume = chainToServerVolume(listP);
		NWfsVolumeSync(volume);
	}
	SLEEP_UNLOCK(&NWfiMountVolumeLock);

	NVLT_VLEAVE();
}

/*
 * BEGIN_MANUAL_ENTRY(NWfiUnMount(3K), \
 *		./man/kernel/nucfs/nwfi/UnixWare/UnMount )
 * NAME
 *    (vfsops_t *)->(*vfs_unmount)() - Un-Mounts a NetWare Volume file system
 *                                     from UNIX Generic Files System.
 * SYNOPSIS
 *    int
 *    NWfiUnMount (vfs_t *vfsMountInstance, cred_t *unixCredentials)
 * INPUT
 *    vfsMountInstance        - Fully populated VFS structure to the NUCfs
 *                              VFS NetWare Volume to unmount.
 *    unixCredentials->cr_uid - The effective user id of the process making
 *                              the request.  This represents the credentials
 *                              of the NetWare Client User.
 *    unixCredentials->cr_gid - The effective group id of the process making
 *                              the request.  This represents the UNIX group
 *                              the NetWare Client User is using.
 * OUTPUT
 *    None.
 * RETURN VALUE
 *    0        - Successful Completion. 
 *    [EBUSY]  - Vnodes are active in this VFS.
 *    [EPERM]  - Not super user, permission denied.
 * DESCRIPTION
 *    The 'NWfiUnMount' unmounts a NetWare Volume for its associated VFS
 *    mount instance in the local UNIX kernel.  The volume is no longer
 *    available to UNIX proess on the Client after completion of the un-mount.
 * SEE ALSO
 *    NWfiIntroduction(3K), NWfiMount(3K), NWfiMountRoot(3K)
 * Calling/Exit State:
 *    The generic layer ensures that no new traversals will be permitted
 *    beyond the mounted-upon vnode, into the file system being unmounted.
 *    No other locking assumptions are made. 
 *
 * END_MANUAL_ENTRY
 */
int
NWfiUnMount (vfs_t *vfsp, cred_t *unixCredentials)
{
	vnode_t			*rootVnode;
	NWFS_CRED_T		nwfsCredentials; 
	NWFS_SERVER_NODE_T	*rootNode;
	int32			returnCode;
	enum NUC_DIAG		diagnostic = SUCCESS;
	NUCFS_ENGINE_DATA(oldengine);

	NUCFS_BIND(oldengine);

	NVLT_ENTER (2);

	if (pm_denied (unixCredentials, P_MOUNT)) {
		/*
		 * Process does not have the mount privilege.
		 */
		returnCode = EPERM;
		goto bye;
	}
	NWfiUnixToFsCred(unixCredentials, &nwfsCredentials);
	rootNode = ((NWFS_SERVER_VOLUME_T *) (vfsp->vfs_data))->rootNode;
	rootVnode = rootNode->gfsNode;
	SLEEP_LOCK(&NWfiMountVolumeLock, PRIVFS);
	if (rootVnode->v_count > 2) {
		/*
		 * Other process(es) are referencing the rootVnode.
		 */
		SLEEP_UNLOCK(&NWfiMountVolumeLock);
		returnCode = EBUSY;
		goto bye;
	}
	/* 
	 * Unmount the NetWare UNIX Client File System volume.
	 *
	 * NOTE:
	 *    NWfsUnMountVolume does not release the memory allocated 
	 *    for the rootVnode or its snode. This is done, separately,
	 *    by NWfsUnMountCleanUp(), that is called if the unmount
	 *    returns successfully.
	 */
	
	NUCFS_LIST_LOCK();
	rootNode->nodeVolume->volFlushData->flags |= VOL_UNMOUNTING;
	NUCFS_LIST_UNLOCK();

	if ((NWfsPrepareToUnMountVolume (rootNode, &diagnostic) != SUCCESS)) {
		returnCode = NWfiErrorMap (diagnostic);

		NUCFS_LIST_LOCK();
		rootNode->nodeVolume->volFlushData->flags &= ~VOL_UNMOUNTING;
		NUCFS_LIST_UNLOCK();

		SLEEP_UNLOCK(&NWfiMountVolumeLock);
		goto bye;
	} 

	if (rootVnode->v_count > 2|| NWfsDoUnMountVolume(rootNode) != SUCCESS) {
		returnCode = EBUSY;

		NUCFS_LIST_LOCK();
		rootNode->nodeVolume->volFlushData->flags &= ~VOL_UNMOUNTING;
		NUCFS_LIST_UNLOCK();

		SLEEP_UNLOCK(&NWfiMountVolumeLock);
		goto bye;
	}

	SLEEP_UNLOCK(&NWfiMountVolumeLock);
	/*
	 * now we are committed to destroying the rootVnode, and the
	 * root snode. Since the root snode cannot go stale, we are
	 * guaranteed to be able to destroy it. 
	 *
	 * NWfsDoUnMountVolume deposited a soft hold on the rootNode, so that
	 * we can release the rootVnode completely, at this point.
	 */
	VN_RELEN(rootVnode, 2);
	NVLT_ASSERT(!NWfsNodeIsHeld(rootNode));
	NVLT_ASSERT(NWfsNodeIsSoftHeld(rootNode));
	NWfiFlushDeInit(rootNode->nodeVolume->volFlushData);
	NUCFS_LIST_LOCK();
	--nwfsVolumesCount;
	NUCFS_LIST_UNLOCK();
	NWfsDestroyVolumeAndRootNode(&nwfsCredentials, rootNode);

        /*
         * remove this vfs from list of vfs
         */
        SLEEP_LOCK(&vfslist_lock, PRIVFS);
        vfs_remove(vfsp);
        SLEEP_UNLOCK(&vfslist_lock);
	if (vfsp->vfs_dev != 0)
		putudev(vfsp->vfs_dev);

	returnCode = 0;
bye:
	NVLT_LEAVE (returnCode);
	NUCFS_UNBIND(oldengine);
	return(returnCode);
}
