/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/cdfs/cdfs_vfsops.c	1.13"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/cdfs/cdfs_hier.h>
#include <fs/cdfs/cdfs.h>
#include <fs/cdfs/cdfs_data.h>
#include <fs/cdfs/cdfs_fs.h>
#include <fs/cdfs/cdfs_inode.h>
#include <fs/cdfs/cdfs_susp.h>
#include <fs/cdfs/cdrom.h>
#include <fs/cdfs/iso9660.h>
#include <fs/fbuf.h>
#include <fs/file.h>
#include <fs/fs_subr.h>
#include <fs/mount.h>
#include <fs/pathname.h>
#include <fs/specfs/snode.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <mem/swap.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/ksynch.h>
#include <util/ipl.h>
#include <util/inline.h>
#include <util/cmn_err.h>
#if ((defined CDFS_DEBUG)  && (!defined DEBUG))
#define		DEBUG	YES
#include	<util/debug.h>
#undef		DEBUG
#else
#include	<util/debug.h>
#endif
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>
#include <util/mod/moddefs.h>

STATIC int cdfs_load(void);
STATIC int cdfs_unload(void);

int cdfs_fsflags = 0;		/* to initialize vswp->vsw_flags */

MOD_FS_WRAPPER(cdfs, cdfs_load, cdfs_unload, "Loadable cdfs FS Type");

extern int cdfs_GetSectSize();
extern int cdfs_ReadPvd();

/*
 * Global CDFS kernel constants, strings, and structures:
 */
const uchar_t		CDFS_ISO_STD_ID[] = ISO_STD_ID;
const uchar_t		CDFS_HS_STD_ID[] = HS_STD_ID;

const uchar_t		CDFS_DOT[] = {ISO_DOT, '\0'};
const uchar_t		CDFS_DOTDOT[] = {ISO_DOTDOT, '\0'};

const uchar_t		CDFS_POSIX_DOT[] = ".";
const uchar_t		CDFS_POSIX_DOTDOT[] = "..";

const struct cdfs_fid	CDFS_NULLFID = {0, 0};



/*
 * Global CDFS kernel variables:
 */
int		cdfs_fstype;		/* VFS ID # assigned to CDFS	*/
caddr_t		cdfs_Mem;		/* Inode pool allocated memory	*/
ulong_t		cdfs_MemSz;		/* Amount of memory allocated	*/
uint_t		cdfs_MemCnt;		/* Reference count		*/

cdfs_inode_t	*cdfs_InodeCache;	/* CDFS in-core Inode cache	*/
cdfs_inode_t	*cdfs_InodeFree;	/* CDFS Inode Free List		*/
cdfs_inode_t	**cdfs_InodeHash;	/* CDFS Inode Hash Table	*/

cdfs_drec_t	*cdfs_DrecCache;	/* Multi-extent Dir Rec cache	*/
cdfs_drec_t	*cdfs_DrecFree;		/* Dir Rec cache Free list	*/

caddr_t		cdfs_TmpBufPool;	/* Temp Buf Pool		*/

#ifdef CDFS_DEBUG
uint_t		cdfs_dbflags = DB_ENTER;	/* Flags to select desired DEBUG*/
#endif

/*
 * cdfs vfs operations.
 */
STATIC int cdfs_mount(vfs_t *, vnode_t *, struct mounta *, cred_t *);
STATIC int cdfs_unmount(vfs_t *, cred_t *);
STATIC int cdfs_root(vfs_t *, vnode_t **);
STATIC int cdfs_statvfs(vfs_t *, struct statvfs *);
STATIC int cdfs_sync(vfs_t *, int, cred_t *);
STATIC int cdfs_vget(vfs_t *, vnode_t **vpp, struct fid *);
STATIC int cdfs_mountroot(vfs_t *, enum whymountroot);
STATIC int cdfs_mountfs(vfs_t *, dev_t, struct cdfs_mntargs *, cred_t *);
STATIC int cdfs_unmountfs(vfs_t *, cred_t *);
extern int      dnlc_purge_vfsp(vfs_t *, int);

struct vfsops cdfs_vfsops = {
	cdfs_mount,
	cdfs_unmount,
	cdfs_root,
	cdfs_statvfs,
	cdfs_sync,
	cdfs_vget,
	cdfs_mountroot,
	(int (*)())fs_nosys,		/* vfs_swapvp */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys
};



/*
 * CDFS Entry Point Stub used for debugging.
 */
#ifdef CDFS_DEBUG
void	(*cdfs_DebugPtr)() = cdfs_entry;     /* Default trap CDFS entry points*/

/*
 * void cdfs_entry()
 *
 * Calling/Exit State:
 */
void
cdfs_entry()
{
	return;
}
#endif	/* CDFS_DEBUG */





/*
 * cdfs_AllocMem()
 * 	Allocate and initialize the necessary memory for CDFS.
 *
 * Calling/Exit State:
 * 	No locks held on entry and exits.
 *	It is called at mount time.
 */
int
cdfs_AllocMem()
{
	uint_t		cache_mem;	/* Size of Inode cache memory	*/
	uint_t		hash_mem;	/* Size of Inode Hash Tbl memory*/
	uint_t		drec_mem;	/* Size of Dir Rec Cache memory	*/

	cdfs_inode_t	*ip;		/* Roving Inode pointer		*/
	cdfs_inode_t	**ipp;		/* Roving "Inode pointer" pntr	*/
	cdfs_drec_t	*drec;		/* Roving Dir Rec pointe 	*/
	int		i;		/* Loop counter			*/
	vnode_t		*vp;

	/*
	 * Compute the amount of memory needed.
	 * - Inode cache.
	 * - Inode hash-table.
	 * - Multi-extent Dir Rec cache.
	 */
	cache_mem = cdfs_InodeCnt * sizeof(*cdfs_InodeCache);
	hash_mem = cdfs_IhashCnt * sizeof(*cdfs_InodeHash);
	drec_mem = cdfs_DrecCnt * sizeof(*cdfs_DrecCache);

	cdfs_MemSz = cache_mem + hash_mem + drec_mem;

	/*
	 * Allocate the required memory and assign it to the
	 * various CDFS resources.
	 */
	cdfs_Mem = kmem_zalloc(cdfs_MemSz, KM_SLEEP); 
	if (cdfs_Mem == NULL) {
		/*
		 *+ Out of memory.
		 *+ Reconfigure the system to consume less memory.
		 */
		cmn_err(CE_PANIC,
			"cdfs_AllocMem(): Unable to allocate 0x%x (%d) bytes of memory.\n",
			cdfs_MemSz, cdfs_MemSz);
		return(ENOMEM);
	}
	/* LINTED pointer alignment */	
	cdfs_InodeCache = (struct cdfs_inode *) cdfs_Mem;
	cdfs_InodeHash = (struct cdfs_inode **) &cdfs_InodeCache[cdfs_InodeCnt];
	cdfs_DrecCache = (struct cdfs_drec *) &cdfs_InodeHash[cdfs_IhashCnt]; 

	/*
	 * Initialize the Inodes in the cache
	 * and add them to the Inode free-list.
	 */
	cdfs_InodeFree = NULL;
	ip = &cdfs_InodeCache[0];
	for (i = 0; i<  cdfs_InodeCnt; i++ , ip++) {
		cdfs_InitInode(ip, 0);
		cdfs_IputFree(ip);
		CDFS_INITINODE_LOCKS(ip);
		vp = ITOV(ip);
		VN_INIT(vp, NULL, 0, 0, 0, KM_SLEEP);
		vp->v_count = 0;
	}
	/*
	 * Initialize the Inode Hash Table.
	 */
	ipp = &cdfs_InodeHash[0];
	for (i=0; i < cdfs_IhashCnt; i++, ipp++) {
		*ipp = NULL;
	}

	/*
	 * Initialize all of the structures in the Multi-extent
	 * Dir Rec cache and add them to the free list.
	 */
	cdfs_DrecFree = NULL;
	drec = &cdfs_DrecCache[0];
	for (i=0; i < cdfs_DrecCnt; i++, drec++) {
		cdfs_DrecPut(&cdfs_DrecFree, drec);
	}
	/* Initialize the inode list lock */
	LOCK_INIT(&cdfs_inode_table_mutex, FS_CDFSLISTHIER, FS_CDFSLISTPL,
                  &cdfs_inode_table_lkinfo, KM_SLEEP);

	return(RET_OK);
}


/*
 * cdfsinit(struct vfssw *vswp)
 *	Allocate all needed memory.
 *	Register the cdfs_vfsops[] table in the vfs structure.
 *
 * Calling/Exit State:
 * 	CDFS initialization routine called only ONCE at system start-up.
 * 	Deals only with global CDFS initialization tasks that are 
 * 	independent of any specific CDFS file-system or file.
 */
void
cdfsinit(struct vfssw *vswp)
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	cdfs_AllocMem();
	/*
	 * Register the cdfs_vfsops[] table in the vfs structure.
	 */
	vswp->vsw_vfsops = &cdfs_vfsops;
	
	return;
}

/*
 * STATIC int
 * cdfs_load(void)
 *      Initialize the cdfs_ops 
 *      and global cdfs synchronization objects.
 *
 * Calling/Exit State:
 *      No locks held on entry and exit.
 *
 * Description:
 *      Should be called when loading a cdfs module.
 *
 * Note : this functions is used when the cdfs file system is loadable
 *      while cdfs_init is used when the cdfs file system is static.
 */
STATIC int
cdfs_load(void)
{
        struct  vfssw   *vswp;

	/*
	 * Register the cdfs_vfsops[] table in the vfs structure.
	 */
        vswp = vfs_getvfssw("cdfs");
        if (vswp == NULL) {
                /*
                 *+ cdfs file system is not registered before
                 *+ attempting to load it.
                 */
                cmn_err(CE_NOTE, "!MOD: CDFS is not registered.");
                return (EINVAL);
        }
	
	cdfs_AllocMem();
	return (0);
}

/*
 * STATIC int
 * cdfs_unload(void)
 *      Deallocate the  inode table lock,
 *      and global memory allocations.. 
 *
 * Calling/Exit State:
 *      No locks held on entry and exit.
 *
 * Description:
 *      Should be called when unloading a cdfs module.
 *
 */
STATIC int
cdfs_unload()
{
	kmem_free((caddr_t)cdfs_Mem, cdfs_MemSz);
	cdfs_MemSz = 0;
        cdfs_Mem = NULL;
        cdfs_InodeCache = NULL;
        cdfs_InodeHash = NULL;
        cdfs_DrecCache = NULL;
        cdfs_InodeFree = NULL;
	LOCK_DEINIT(&cdfs_inode_table_mutex);
        return(0);
}

/*
 * STATIC int
 * cdfs_mount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr)
 * 	Mount a CDFS file system. 
 *
 * Calling/Exit State:
 *	The mount point mvp->v_lock is locked exclusive on entry and remains
 *    	locked at exit. Holding this lock prevents new lookups into the
 *    	file system the mount point is in (see the lookup code for more).
 */
STATIC int
cdfs_mount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr)
	/* VFS struct to be used		*/
	/* Vnode of mount-point			*/
	/* Mount arguments from mount(2)	*/
	/* User's credential struct		*/
{
	vnode_t		*devvp;		/* Vnode of the device-node	*/
	dev_t		devnum;		/* Device # (Maj/Min) of device */
	cdfs_vfs_t 	*cdfs_vfsp;	/* Private VFS data: CDFS struct*/
	cdfs_mntargs_t	mntargs;	/* Mount flags from CDFS-mount()*/	
	boolean_t 	mntargs_valid;	/* Indicates validity of mntargs*/
	int 		retval;		/* Return value of called routines*/
	vfs_t		*dvfsp;


	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	/*
	 * Validate pre-conditions:
	 * - Mount-point is a directory.
	 * - Reference count of mount-point must be 1 (Except REMOUNT).
	 * - Mount-point is not 'root' of another file-system (Except REMOUNT).
	 * - Mount read-only.
	 */
	if (pm_denied(cr, P_MOUNT))
		return EPERM;
	if (mvp->v_type != VDIR) {
		return(ENOTDIR);
	}
	if (((uap->flags & MS_REMOUNT) == 0) &&
		(mvp->v_count > 1 || (mvp->v_flag & VROOT))) {
		return(EBUSY);
	}

	if ((uap->flags & MS_RDONLY) == 0)
		return(EROFS);
	/*
	 * Get the device # (major/minor) of the device being mounted.
	 * - Get and verify the device's Vnode via its pathname.
	 * - Get and verify the device number from the Vnode.
	 * - Release the Vnode.
	 */
	retval = lookupname(uap->spec, UIO_USERSPACE, FOLLOW, NULLVPP, &devvp);
	if (retval != 0) {
		return(retval);
	}

	if (devvp->v_type != VBLK) {
		VN_RELE(devvp);
		return(ENOTBLK);
	}

	devnum = devvp->v_rdev;	
	if (getmajor(devnum) >= bdevcnt) {
		VN_RELE(devvp);
		return(ENXIO);
	}
	VN_RELE(devvp);

	/*
	 * Verify that the state of the device (mounted v.s. unmounted)
	 * matches the type of mount request (REMOUNT v.s. non-REMOUNT).
	 * - Search the current list of allocated VFS structures to see
	 *   if the device is being referenced (i.e. mounted).
	 * - If its already mounted, this had better be a REMOUNT request.
	 */
	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	dvfsp = vfs_devsearch(devnum);
	if (dvfsp == NULL) {
		if ((uap->flags & MS_REMOUNT) == 0) {
			vfsp->vfs_flag &= ~VFS_REMOUNT;
		} else {
			SLEEP_UNLOCK(&vfslist_lock);
			return(EINVAL);
		}
	} else {
		if ((uap->flags & MS_REMOUNT) != 0) {
			if (dvfsp != vfsp) {
				SLEEP_UNLOCK(&vfslist_lock);
				return(EBUSY);
			}
			/* This is REMOUNT case -- return */
			SLEEP_UNLOCK(&vfslist_lock);
			return(0);
		} else {
			SLEEP_UNLOCK(&vfslist_lock);
			return(EBUSY);
		}
	}
	SLEEP_UNLOCK(&vfslist_lock);

	/*
	 * Mount the file system as Read-Only if specified by user.
	 */
	if (uap->flags & MS_RDONLY)
		vfsp->vfs_flag |= VFS_RDONLY;

	/*
	 * Get the CDFS-specific mount arguments passed in from
	 * the CDFS-specific mount(1M) command.
	 */
	if (((uap->flags & MS_DATA) != 0) &&
		(uap->dataptr != NULL) &&
		(uap->datalen >= sizeof(mntargs)) &&
		(copyin(uap->dataptr, (caddr_t)&mntargs, sizeof(mntargs)) == 0)) {;
		mntargs_valid = B_TRUE;
	} else {
		mntargs_valid = B_FALSE;
	}

	/*
	 * Mount the file system.
	 */
	if (mntargs_valid == B_TRUE) {
		retval = cdfs_mountfs(vfsp, devnum, &mntargs, cr);
	} else {
		retval = cdfs_mountfs(vfsp, devnum, NULL, cr);
	}
	/*
	 * Get and store mount-point and device node pathnames.
	 */
	cdfs_vfsp = (struct cdfs *)vfsp->vfs_data;

	if ((retval == RET_OK) && (!(uap->flags & MS_REMOUNT))) {
		SLEEP_LOCK(&vfslist_lock, PRIVFS);
                if (vfs_devsearch(devnum) != NULL) {
                        /* if lost the race then free the all private data */
                        kmem_free((caddr_t)cdfs_vfsp, sizeof(cdfs_vfs_t));
                        retval = EBUSY;
                } else
                        vfs_add(mvp, vfsp, uap->flags);
                SLEEP_UNLOCK(&vfslist_lock);
		if (retval != 0)
			return(retval);
	} else {
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		return(retval);
	}


	if (pn_get(uap->dir, UIO_USERSPACE, &cdfs_vfsp->cdfs_MntPnt) != 0) {
		/*
		 *+ Cannot get pathname for mount point.
		 */
		cmn_err(CE_NOTE,
			"cdfs_mount(): Unable to obtain pathname of mount-point.\n");
		cdfs_vfsp->cdfs_MntPnt.pn_buf[0] = '\0';
		cdfs_vfsp->cdfs_MntPnt.pn_pathlen = 0;
	}

	if (pn_get(uap->spec, UIO_USERSPACE, &cdfs_vfsp->cdfs_DevNode) != 0 ) {
		/*
		 *+ Cannot get pathname for mount point.
		 */
		cmn_err(CE_NOTE,
			"cdfs_mount(): Unable to obtain pathname of device-file.\n");
		cdfs_vfsp->cdfs_DevNode.pn_buf[0] = '\0';
		cdfs_vfsp->cdfs_DevNode.pn_pathlen = 0;
	}

	return(retval);
}


/*
 * cdfs_mountroot(vfs_t *vfsp, enum whymountroot why)
 * 	This is NO-OP since cdfs cannot be a root file system.	
 *
 * Calling/Exit State:
 *
 */
/* ARGSUSED */
STATIC int
cdfs_mountroot(vfs_t *vfsp, enum whymountroot why)
{
	return(ENOSYS);
}



/*
 * STATIC int
 * cdfs_mountfs(vfs_t *vfsp, dev_t devnum, struct cdfs_mntargs *mntargs,
 *	cred_t *cr)
 * 	Mount the file system stored on the device.
 *
 * Calling/Exit State:
 *	May be called from various situations but for now why is ROOT_INIT
 *	only.
 *
 * Description:
 * 	- Open the device.
 * 	- Get and process the super-block.
 * 	- Initialize the VFS and CDFS structures
 * 	- Get the Vnode for the file system root directory. 
 */
STATIC int
cdfs_mountfs(vfs_t *vfsp, dev_t devnum, struct cdfs_mntargs *mntargs,
	 cred_t *cr)
		/* VFS struct of file system	*/
		/* Device # (Maj/Min) of device */
		/* CDFS-specific mount arguments*/
		/* User's credential structure	*/
{
	vnode_t		*devvp;		/* Vnode of device to be mounted*/
	cdfs_vfs_t	*cdfs;		/* CDFS structure		*/
	cdfs_iobuf_t	pvd_buf;	/* PVD I/O buffer structure	*/
	cdfs_iobuf_t	drec_buf;	/* Dir Rec I/O buffer structure	*/
	cdfs_fid_t	pvd_fid;	/* PVD Root-Dir-Rec FID		*/
	cdfs_inode_t	*pvd_ip;	/* PVD Root-Dir-Rec Inode	*/
	cdfs_inode_t	*root_ip; 	/* Root Inode			*/
	u_int		secsize;	/* Logical sector size		*/
	enum cdfs_type	fstype;		/* CDFS type: ISO_9660/HiSierra	*/
	u_int		count;		/* Miscellanious counter.	*/
	int		retval;		/* Return value for various calls*/
	int		cleanup_cnt;	/* # of items to be cleanup */
	
	/*
	 * Initialize allocated resources in order to aid
	 * in the cleanup task following an error.
	 */
	cleanup_cnt = 0;
	devvp = NULLVP;
	cdfs = (cdfs_vfs_t *)NULL;

	/*
	 * If this is a REMOUNT, the Vnode is already known.
	 */
	if ((vfsp->vfs_flag & VFS_REMOUNT) == 1) {
		return(0);
	}
	/*
	 * This is an initial mount request (not a REMOUNT) so the
	 * device must be opened and verified:
	 * - Build a Vnode for the device using the device number. 
	 * - Open the device.
	 * - Verify that the device is not being used for SWAP space.
	 * - Invalidate all pages associated with the device.
	 *
	 * Note: Even though the Vnode for the device was obtained in
	 * cdfs_mount(), this routine must be able to build a Vnode
	 * using only the device number.  The cdfs_mountroot() routine,
	 * which also calles this routine, only has the device number of
	 * the root device, there is no pathname to pass to 'lookupname()'.
	 *
	 * - When bio is fixed for vnodes, this can all be vnode operations.
	 */
	devvp = makespecvp(devnum, VBLK);
	cleanup_cnt++;

	retval = VOP_OPEN(&devvp, FREAD, cr);
	if (retval != 0) {
		goto Cleanup;
	}
	cleanup_cnt++;

	/*
	 * If the device is being used for swap space,
	 * failed the mount. 
	 */
	if (devvp->v_flag & VNOMOUNT) {
		/*
		 *+ It's no map device. Cannot use this device.
		 */
		cmn_err(CE_WARN,
			"cdfs_mountfs(): Device currently used as a swap device.\n");
		retval = EBUSY;
		goto Cleanup;
	}

	/*
	 * Initialize the VFS and CDFS structures with the currently
	 * known data.
	 * - Allocate memory for the CDFS structure.
	 * - Init the known VFS fields.
	 * - Init the know CDFS fields.
	 * 
	 * NOTE: This is done early so that some of the lower-layer,
	 * which depend on certain VFS and CDFS values, can be used
	 * at this point in time.
	 */
	cdfs = (struct cdfs *)kmem_zalloc(sizeof(struct cdfs), KM_SLEEP);
	if (cdfs == NULL) {
		/*
		 *+ No memory. Reconfigure the system to consume less memory.
		 */
		cmn_err(CE_WARN,
			"cdfs_mountfs(): Unable to allocate CDFS structure memory.\n");
		retval = ENOMEM;
		goto Cleanup;
	}
	cleanup_cnt++;

	vfsp->vfs_dev = devnum;
	vfsp->vfs_fsid.val[0] = devnum;
	vfsp->vfs_fsid.val[1] = vfsp->vfs_fstype;
	vfsp->vfs_flag |= VFS_NOTRUNC;
	vfsp->vfs_data = (caddr_t) cdfs;
	vfsp->vfs_bcount = 0;
	
	if (mntargs != NULL) {
		cdfs->cdfs_Flags = mntargs->mnt_Flags & CDFS_ALL_FLAGS;
	} else {
		cdfs->cdfs_Flags = CDFS_SUSP | CDFS_RRIP;
	}
	cdfs->cdfs_DevVnode = devvp;

	/*
	 * Initialize XCDR and RRIP default and mapping structures.
	 */
	cdfs->cdfs_Dflts.def_uid = cdfs_InitialUID;
	cdfs->cdfs_Dflts.def_gid = cdfs_InitialGID;
	cdfs->cdfs_Dflts.def_fperm = cdfs_InitialFPerm;
	cdfs->cdfs_Dflts.def_dperm = cdfs_InitialDPerm;
	cdfs->cdfs_Dflts.dirsperm = cdfs_InitialDirSearch;
	cdfs->cdfs_NameConv = cdfs_InitialNmConv;
	for (count = 0; count < CD_MAXUMAP; count ++) {
		cdfs->cdfs_UidMap[count].from_uid = CDFS_UNUSED_MAP_ENTRY;
		cdfs->cdfs_UidMap[count].to_uid = CDFS_UNUSED_MAP_ENTRY;
	}
	cdfs->cdfs_UidMapCnt = 0;

	for (count = 0; count < CD_MAXGMAP; count ++) {
		cdfs->cdfs_GidMap[count].from_gid = CDFS_UNUSED_MAP_ENTRY;
		cdfs->cdfs_GidMap[count].to_gid = CDFS_UNUSED_MAP_ENTRY;
	}
	cdfs->cdfs_GidMapCnt = 0;

	for (count = 0; count < CD_MAXDMAP; count ++) {
		cdfs->cdfs_DevMap[count].fileid = CDFS_NULLFID;
		cdfs->cdfs_DevMap[count].to_num = NODEV;
	}
	cdfs->cdfs_DevMapCnt = 0;

	/*
	 * Get the Logical Sector size for this CDFS.
	 * - If the user specified it on the command line, then use that value.
	 * - Otherwise, it has to be computed.
	 * - Validate the Logical Sector size.
	 */
	if ((mntargs != NULL) &&
		((mntargs->mnt_Flags & CDFS_USER_BLKSZ) != 0)) {
		secsize = mntargs->mnt_LogSecSz;
	} else {
		retval = cdfs_GetSectSize(devnum, &secsize);
		if (retval != RET_OK) {
			/*
			 *+ Cannot find the logical sector size.
			 */
			cmn_err(CE_WARN,
				"cdfs_mountfs(): Unable to determine logical sector size of media.\n");
			if (retval < RET_OK) {
				retval = EINVAL;
			}
			goto Cleanup;
		}
	}

	if (secsize > MAXBSIZE) {
		/*
		 *+  Incorrect Logical Sector size.
		 */
		cmn_err(CE_WARN,
			"cdfs_mountfs(): Invalid Logical Sector: 0x%x", secsize);
		cmn_err(CE_CONT,
			"Maximum logical sector size is 0x%x\n\n", MAXBSIZE);
		retval = EINVAL;
		goto Cleanup;
	}

	cdfs->cdfs_LogSecSz = secsize;
	cdfs->cdfs_LogSecMask = secsize-1;
	for (count=0; (ulong_t)(1 << count) <= (ulong_t)MAXBSIZE; count++) {
		if ((1 << count) == secsize) {
			break;
		}; 
	}

	if ((1 << count) != secsize) {
		/*
		 *+  Incorrect Logical Sector size.
		 *+ It must be power of two.
		 */
		cmn_err(CE_WARN,
			"cdfs_mountfs(): Invalid Logical Sector size: 0x%x", secsize);
		cmn_err(CE_CONT,
			"Not an integral power-of-two multiple\n\n");
		retval = EINVAL;
		goto Cleanup;
	}
	cdfs->cdfs_LogSecShift = count;
		
	/*
	 * Read the Primary Volume Descriptor (Super-Block) from the device.
	 * - Setup the PVD I/O Buffer.
	 */
	CDFS_SETUP_IOBUF(&pvd_buf, CDFS_BUFIO);
	pvd_buf.sb_dev = CDFS_DEV(vfsp);
	pvd_buf.sb_sect = ISO_VD_LOC;
	cleanup_cnt++;

	retval = cdfs_ReadPvd(vfsp, &pvd_buf, &fstype);
	if (retval != RET_OK) {
		/* 
		 *+ Cannot read super block.
		 *+ Fix the super block before retry. 
		 */
		cmn_err(CE_WARN,
			"cdfs_mountfs(): Unable to locate Primary Volume Descriptor.\n");
		if (retval < RET_OK) {
			retval = EIO;
		}
		goto Cleanup;
	}

	cdfs->cdfs_PvdLoc = pvd_buf.sb_sect;
	cdfs->cdfs_Type = fstype;

	retval = cdfs_ConvertPvd(cdfs, (union media_pvd *)pvd_buf.sb_ptr, fstype);
	if (retval != RET_OK) {
		/*
		 *+ Cannot convert the super block to appropriate media.
		 */ 
		cmn_err(CE_WARN,
			"cdfs_mountfs(): Invalid Primary Volume Descriptor.\n");
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		goto Cleanup;
	}

	vfsp->vfs_bsize = cdfs->cdfs_LogBlkSz;

	cdfs->cdfs_LogBlkMask = cdfs->cdfs_LogBlkSz - 1;
	for (count=0; (ulong_t)(1 << count) <= (ulong_t)MAXBSIZE; count++) {
		if ((1 << count) == cdfs->cdfs_LogBlkSz) {
			break;
		}; 
	}

	if ((1 << count) != cdfs->cdfs_LogBlkSz) {
		/*
		 *+ Invalid Logical Sector size.
		 */
		cmn_err(CE_WARN,
			"cdfs_mount(): Invalid Logical Sector size: 0x%x", secsize);
		cmn_err(CE_CONT,
			"Not an integral power-of-two multiple\n\n");
		retval = EINVAL;
		goto Cleanup;
	}
	cdfs->cdfs_LogBlkShift = count;
		
	/*
	 * Setup the FID for the Root Inode.
	 */
	pvd_fid.fid_SectNum = pvd_buf.sb_sect;
	pvd_fid.fid_Offset = pvd_buf.sb_offset + cdfs->cdfs_RootDirOff;

	CDFS_SETUP_IOBUF(&drec_buf, CDFS_BUFIO);
	drec_buf.sb_ptr = pvd_buf.sb_ptr + cdfs->cdfs_RootDirOff;
	drec_buf.sb_offset = pvd_buf.sb_offset + cdfs->cdfs_RootDirOff;
	drec_buf.sb_sect = pvd_buf.sb_sect;
	drec_buf.sb_sectoff = pvd_buf.sb_sectoff;
	drec_buf.sb_start = drec_buf.sb_ptr;
	drec_buf.sb_end = drec_buf.sb_ptr + cdfs->cdfs_RootDirSz;
	cleanup_cnt++;

	CDFS_FLAGS(vfsp) &= ~(CDFS_SUSP_PRESENT | CDFS_RRIP_ACTIVE);
	CDFS_FLAGS(vfsp) |= CDFS_BUILDING_ROOT;

	/*
	 * Build the primary volume descriptor root-directory-record inode.
 	 */
	retval = cdfs_GetInode(vfsp, &pvd_fid, &drec_buf, &pvd_ip);
	if (retval != RET_OK) {
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		goto Cleanup;
	}
	cleanup_cnt++;

	/*
	 * If requested by the user, try to locate the SUSP and/or RRIP
	 * extensions within the Root Dir Rec.
	 */
	CDFS_FS_ILOCK(cdfs);
	CDFS_FLAGS(vfsp) &= ~CDFS_SUSP_PRESENT;
	CDFS_SUSPOFF(vfsp) = 0;

	if (((CDFS_FLAGS(vfsp) & CDFS_SUSP) != 0) &&
		((CDFS_FLAGS(vfsp) & CDFS_RRIP) != 0)) {
		CDFS_FLAGS(vfsp) |= CDFS_RRIP_ACTIVE;
	}
	CDFS_FS_IUNLOCK(cdfs);

	retval = cdfs_DirLookup(vfsp, pvd_ip, (uchar_t *)CDFS_DOT, &root_ip, cr);
	if (retval != RET_OK) {
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		goto Cleanup;
	}
	cleanup_cnt++;
	
	root_ip->i_ParentFid = root_ip->i_Fid;
	ITOV(root_ip)->v_flag |= VROOT;

	cdfs->cdfs_RootInode = root_ip;
	cdfs->cdfs_RootFid = root_ip->i_Fid; 
	CDFS_FS_ILOCK(cdfs);
	CDFS_FLAGS(vfsp) &= ~(CDFS_BUILDING_ROOT);

	/*
	 * If the RRIP extensions do not exist for the Root Dir Rec,
	 * then don't bother looking for them in other Dir Recs.
	 */
	if ((CDFS_FLAGS(vfsp) & CDFS_RRIP_PRESENT) == 0) {
		CDFS_FLAGS(vfsp) &= ~(CDFS_RRIP_ACTIVE);
	}

	CDFS_FS_IUNLOCK(cdfs);
	/*
	 * Since we're done, we can release SOME of the allocated resources.
	 * - Unlock and release the Inode built from the PVD.
	 * - Unlock the Root Inode.  The Root Inode is not released
	 *	 so that cdfs_root() can just use it without having to
	 *	 look it up and/or build it on every call.  The Root Inode
	 *	 is released by cdfs_unmountfs().
	 * - Release the Dir Rec and PVD I/O buffers.
	 */
	VN_RELE(ITOV(pvd_ip));

	CDFS_RELEASE_IOBUF(&drec_buf);
	CDFS_RELEASE_IOBUF(&pvd_buf);

	return(RET_OK);

Cleanup:
	switch (cleanup_cnt) {
		case 7: {
			/*
			 * Unlock and release the Root Inode.
			 */
			VN_RELE(ITOV(root_ip));
		} /* FALLTHRU */

		case 6: {
			/*
			 * Unlock and release the PVD Inode.
			 */
			VN_RELE(ITOV(pvd_ip));
		} /* FALLTHRU */

		case 5: {
			/*
			 * Release the Dir Rec I/O buffer structure.
			 */
			CDFS_RELEASE_IOBUF(&drec_buf);
		} /* FALLTHRU */

		case 4: {
			/*
			 * Release the PVD I/O buffer structure.
			 */
			CDFS_RELEASE_IOBUF(&pvd_buf);
		} /* FALLTHRU */

		case 3: {
			/*
			 * Release the CDFS data structure memory.
			 */
			kmem_free((caddr_t)cdfs, sizeof(struct cdfs));
		} /* FALLTHRU */

		case 2: {
			/*
			 * Close the device and invalidate its buf headers.
			 * Note: Don't do this on a REMOUNT.
			 */
			if ((vfsp->vfs_flag & VFS_REMOUNT) == 0) {
				(void) VOP_CLOSE(devvp, FREAD, 1, 0, cr);
				binval(devnum);
			}
		} /* FALLTHRU */

		case 1: {
			/*
			 * Release the Vnode of the device.
			 * Note: Don't do this on a REMOUNT.
			 */
			if ((vfsp->vfs_flag & VFS_REMOUNT) == 0) {
				VN_RELE(devvp);
			}
		} /* FALLTHRU */

		case 0: {
			break;
		}

		default: {
			/*
			 *+ Unknown cleanup level so just leave things alone.
			 */
			cmn_err(CE_WARN,
				"cdfs_mountfs(): Unknown cleanup level (%d)", cleanup_cnt);
			/*
			 *+ No cleanup action taken.
			 */
			cmn_err(CE_NOTE,
				"No cleanup action taken.\n\n");
		} /* FALLTHRU */
	}
	return(retval);
}



/*
 * cdfs_unmount(vfs_t *vfsp, cred_t *cr)
 *	Unmount a file system.
 *
 * Calling/Exit State:
 *	The mount point vp->v_lock is locked exclusive on entry and remains
 *	locked at exit.
 *
 */
int
cdfs_unmount(vfs_t *vfsp, cred_t *cr)
	/* VFS struct of file system	*/
	/* credential structure		*/
{
	int		retval;		/* Return value of various calles*/

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	retval = cdfs_unmountfs(vfsp, cr);
	if (retval != RET_OK) {
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		return(retval);
	}

	return(0);
}





/*
 * cdfs_unmountfs(vfs_t *vfsp, cred_t *cr)
 * 	Common code to unmount a CDFS file system.
 *
 * Calling/Exit State:
 *	The mount point vp->v_lock is locked exclusive on entry and remains
 *	locked at exit.
 *
 * Description:
 *	The vfs list lock is held so that no one cannot establish any
 *	new references to any files in this file system. Also, the root
 *	inode and in-core superblock are sync'ed back to disk.
 */
STATIC int
cdfs_unmountfs(vfs_t *vfsp, cred_t *cr)
	/* VFS struct of file system	*/
	/* credential structure		*/
{
	cdfs_vfs_t	*cdfs;		/* CDFS struct of file system	*/
	dev_t		devnum;		/* Device # (major/minor)	*/
	vnode_t		*devvp;		/* 'specfs' Vnode of device	*/
	vnode_t		*rootvp;	/* Vnode of Root directory	*/
	int		retval;		/* Return value of various calles*/
	pl_t		s;

	ASSERT(vfsp->vfs_op == &cdfs_vfsops);

	if (pm_denied(cr, P_MOUNT))
		return EPERM;

	/* Grab the vfslist lock to prevent new references
         * established by NFS via fhtovp.
         */
	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	/*
         * If NFS is establishing references to files in this
         * file system, fail the unmount now.
         */
	s = LOCK(&vfsp->vfs_mutex, FS_VFSPPL);
	if (vfsp->vfs_count != 0) {
		UNLOCK(&vfsp->vfs_mutex, s);
                SLEEP_UNLOCK(&vfslist_lock);
                return EBUSY;
	}
	UNLOCK(&vfsp->vfs_mutex, s);

	/*
         * dnlc_purge moved here from upper level.
         * It is done after the vfslist is locked
         * because only then can we be sure that
         * there will be no more cache entries
         * established via vget by NFS.
         */
        dnlc_purge_vfsp(vfsp, 0);

	/*
	 * Free all inactive Inodes/Vnodes associated with the device.
	 * If an Inode is still active, the device can not be unmounted.
	 */
	retval = cdfs_FlushInodes(vfsp);
	if (retval != RET_OK) {
		SLEEP_UNLOCK(&vfslist_lock);
		return(EBUSY);
	}

	rootvp = CDFS_ROOT(vfsp);
	cdfs = (struct cdfs *)vfsp->vfs_data;
	devvp = cdfs->cdfs_DevVnode;


	/*
	 * Release and cleanup the Root Inode and put it
	 * at the head of the Free list.
	 */
	ASSERT(rootvp != NULL);
	
	rootvp->v_count = 1;
	VN_RELE(rootvp);
	
	/* Remove vfs from vfs list. */
	vfs_remove(vfsp);
        SLEEP_UNLOCK(&vfslist_lock);

	/*
	 * Release the memory allocated for the CDFS structure.
	 */
	if (cdfs->cdfs_MntPnt.pn_buf != NULL) {
		pn_free(&cdfs->cdfs_MntPnt);
	}

	if (cdfs->cdfs_DevNode.pn_buf != NULL) {
		pn_free(&cdfs->cdfs_DevNode);
	}

	kmem_free((caddr_t)cdfs, sizeof(struct cdfs));
	vfsp->vfs_data = NULL;

	/*
	 * Close the device, invalidate the pages associated with the
	 * device and, release the Vnode for the device.
	 */
	devnum = CDFS_DEV(vfsp);
	(void) VOP_CLOSE(devvp, FREAD, 1, 0, cr);
	binval(devnum);
	VN_RELE(devvp);

	return(RET_OK);
}




/*
 * cdfs_root(vfs_t *vfsp, vnode_t **vpp)
 * 	Get the Vnode of the file system's root directory.
 *
 * Calling/Exit State:
 *	 No locks held on entry or exit.
 */
STATIC int
cdfs_root(vfs_t *vfsp, vnode_t **vpp)
	/* VFS strucut of file system	*/
	/* Addr of Vnode Pntr to be set */
{
	cdfs_inode_t	*rootip; 

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	/*
	 * Get the root Vnode from the CDFS structure.
	 * Also, make sure the Vnode is not released.
	 */
	rootip = CDFS_ROOT_INODE(vfsp);
	*vpp = ITOV(rootip);
	VN_HOLD(*vpp);

	return(0);
}




/*
 * cdfs_statvfs(vfs_t *vfsp, struct statvfs *sp)
 * 	Get various file system statistics.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC int
cdfs_statvfs(vfs_t *vfsp, struct statvfs *sp)
	/* File system's VFS struct		*/
	/* Generic FS statistics struct */
{
	cdfs_vfs_t 	*cdfs;		/* File system's CDFS struct	*/
	int		cnt;		/* Byte count for string copy	*/
	
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	/*
	 * Clear all fields in the generic file-system statistics structure.
	 */
	(void)bzero((caddr_t)sp, (int)sizeof(*sp));

	/*
	 * Locate and validate the CDFS structure.
	 */
	cdfs = (struct cdfs *) vfsp->vfs_data;
	if ((cdfs->cdfs_Type != CDFS_ISO_9660) &&
		(cdfs->cdfs_Type != CDFS_HIGH_SIERRA)) {
		return (EINVAL);
	}

	/*
	 * Specify various file system  block counts:
	 * - Block size in bytes.
	 * - Fragment size.
	 * - Total # of blocks.
	 * - Total # of free blocks.
	 * - Total # of free block for non-privaledged users. 
	 */
	sp->f_bsize = CDFS_SECTSZ(vfsp);
	sp->f_frsize = CDFS_BLKSZ(vfsp);
	sp->f_blocks = cdfs->cdfs_VolSpaceSz;
	sp->f_bfree = 0; 
	sp->f_bavail = 0;

	/*
	 * Specify various Inode data:
	 * - Total # of inodes.
	 * - Total # of free Inode.
	 * - Total # of free Inodes available for non-privaliged users.
	 */
	sp->f_files = 0xffffffff;
	sp->f_ffree = 0xffffffff;
	sp->f_favail = 0xffffffff;

	/*
	 * Other miscellaneous data:
	 * - File system ID.
	 * - Flags.
	 * - File system name.
	 * - Maximum length of a file name.
	 * - Volume (file sysetm) ID string.
	 */
	sp->f_fsid = CDFS_DEV(vfsp);
	sp->f_flag = vf_to_stf(vfsp->vfs_flag);

	cnt = sizeof(sp->f_basetype) - 1;
	(void) strncpy(&sp->f_basetype[0], vfssw[vfsp->vfs_fstype].vsw_name, cnt);
	sp->f_basetype[cnt] = '\0';
	CDFS_FS_ILOCK(cdfs);
	if ((CDFS_FLAGS(vfsp) & CDFS_RRIP_ACTIVE) == 0) {
		sp->f_namemax = CDFS_MAX_NAME_LEN;
	} else {
		sp->f_namemax = MAXNAMELEN - 1;
	}
	CDFS_FS_IUNLOCK(cdfs);

	cnt = MIN(sizeof(sp->f_fstr), sizeof(cdfs->cdfs_VolID)) - 1;
	(void) strncpy(&sp->f_fstr[0], (caddr_t)&cdfs->cdfs_VolID[0], cnt);
	sp->f_fstr[cnt] = '\0';

	return(0);
}




/*
 * cdfs_sync(vfs_t *vfsp, int flag, cred_t *cr)
 * 	Synchronize media with in-core data structures.
 *
 * Calling/Exit State:
 *
 * Note: CDFS is currently Read-Only, so this is successful NO-OP.
 */
/* ARGSUSED */
STATIC int
cdfs_sync(vfs_t *vfsp, int flag, cred_t *cr)
	/* VFS struct of the file system */
	/* Sync flags			*/
	/* credential structure		*/
{
	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	return(0);	/* Always Read-Only: Nothing to do*/
 }

/*
 * cdfs_vget(vfs_t *vfsp, vnode-t **vpp, fid-t *fidp)
 * 	Find the Vnode associated with the unique file ID structure.
 *
 * Calling/Exit State:
 *	The file system that we're going to retrieve the inode
 *	from is protected against unmount by getvfs() -- see vfs.c
 */
STATIC int
cdfs_vget(vfs_t *vfsp, vnode_t **vpp, fid_t *fidp)
	/* VFS struct of file system	*/
	/* Return addr for Vnode pointer*/
	/* A unique file ID structure	*/
{
	cdfs_fid_t	*cdfs_fid;	/* Unique CDFS file ID structure*/
	cdfs_inode_t	*ip;
	int		retval;

	/*
	 * CDFS Entry Point.
	 */
	DB_CODE(DB_ENTER, {
		(*cdfs_DebugPtr)();
	});

	/*
	 * Get the CDFS FID from the generic FID structure.
	 */
	/* LINTED pointer alignment */
	cdfs_fid = (struct cdfs_fid *) &(fidp->fid_data[0]);

	/*
	 * Locate the Inode that corresponds to the unique file ID.
	 */
	retval = cdfs_GetInode(vfsp, cdfs_fid, NULL, &ip);
	if (retval != RET_OK) {
		if (retval < RET_OK) {
			retval = EINVAL;
		}
		*vpp = NULL;
		return(retval);
	}

	*vpp = ITOV(ip);
	return(0);
}
