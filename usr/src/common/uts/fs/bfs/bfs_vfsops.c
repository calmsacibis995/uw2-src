/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/bfs/bfs_vfsops.c	1.10"
#ident	"$Header: $"

#include <acc/priv/privilege.h>
#include <fs/bfs/bfs.h>
#include <fs/buf.h>
#include <fs/file.h>
#include <fs/fs_subr.h>
#include <fs/fs_hier.h>
#include <fs/mount.h>
#include <fs/specfs/snode.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/bitmasks.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>
#include <util/mod/moddefs.h>


extern int dnlc_purge_vfsp(vfs_t *, int);

static void bfs_initlocks(struct bfs_vfs *);
static void bfs_deinitlocks(struct bfs_vfs *);

/*
 * Boot file system VFS operations vector.
 */
STATIC int bfs_mount(vfs_t *, vnode_t *, struct mounta *, cred_t *);
STATIC int bfs_unmount(vfs_t *, cred_t *);
STATIC int bfs_root(vfs_t *, vnode_t **);
STATIC int bfs_statvfs(vfs_t *, struct statvfs *);
STATIC int bfs_sync(vfs_t *, int, cred_t *);
STATIC int bfs_vget(vfs_t *, vnode_t **, struct fid *);

struct vfsops bfs_vfsops = {
	bfs_mount,
	bfs_unmount,
	bfs_root,
	bfs_statvfs,
	bfs_sync,
	bfs_vget,
	(int (*) ())fs_nosys,	/* mountroot */
	(int (*) ())fs_nosys,	/* setceiling */
	(int (*) ())fs_nosys,	/* filler [8] */
	(int (*) ())fs_nosys,
	(int (*) ())fs_nosys,
	(int (*) ())fs_nosys,
	(int (*) ())fs_nosys,
	(int (*) ())fs_nosys,
	(int (*) ())fs_nosys,
	(int (*) ())fs_nosys,
};

int bfs_fstype;

LKINFO_DECL(bfs_inolist_lkinfo, "FS:BFS:BFS inode list lock", 0);
LKINFO_DECL(bfs_writelock_lkinfo, "FS:BFS:BFS write lock", 0);
LKINFO_DECL(bfs_fs_rwlock_lkinfo, "FS:BFS:BFS filesystem rwlock lock", 0);
LKINFO_DECL(bfs_ino_rwlock_lkinfo, "FS:BFS:BFS per inode rwlock lock", 0);

unsigned long bfs_fsflags = 0;
int bfs_load(void);
MOD_FS_WRAPPER(bfs, bfs_load, NULL, "Loadable BFS");


/*
 * int
 * bfs_load(void)
 * 	BFS filesystem global variable initialization.
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *
 */
int
bfs_load(void)
{
	struct vfssw *vswp;

	vswp = vfs_getvfssw("bfs");
	if (vswp == NULL) {
		/*
		 *+ BFS file system is not registered before
		 *+ attempting to load it.
		 */
		cmn_err(CE_NOTE, "!MOD: BFS is not registered.");
		return (EINVAL);
	}
	bfs_fstype = vswp - vfssw;
	
	return(0);
}


/*
 * int
 * bfsinit(struct vfssw *vswp, int fstype)
 * 	BFS filesystem one-time initialization.  Called at system startup time.
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *
 */
/* ARGSUSED */
int
bfsinit(struct vfssw *vswp, int fstype)
{
	bfs_fstype = fstype;
	vswp->vsw_vfsops = &bfs_vfsops;
	return 0;
}


/*
 * int
 * bfs_mount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr)
 *	Mount a BFS filesystem.
 *
 * Calling/Exit State:
 *	The caller must lock the vnode of the mount point in exclusive mode.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 */
STATIC int
bfs_mount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr)
{
	dev_t		dev;
	vnode_t		*bvp;
	struct bfs_vfs	*bfsp;
	struct bdsuper	*superbuf;
	inode_t		*ip;
	dinode_t	dinode, *dip;
	int		bitmapsize;
	int		rdonly = (uap->flags & MS_RDONLY);
	int		fmode;
	int		error;
	int		i;

	ASSERT(KS_HOLD0LOCKS());

	if (pm_denied(cr, P_MOUNT))
		return EPERM;

	if (mvp->v_type != VDIR)
		return ENOTDIR;

	if (mvp->v_count != 1 || (mvp->v_flag & VROOT))
		return EBUSY;

	/*
	 * Lookup the requested mount device, and get the special vnode.
	 */
	if (error = lookupname(uap->spec, UIO_USERSPACE, FOLLOW, NULLVPP, &bvp))
		return error;

	if (bvp->v_type != VBLK) {
		VN_RELE(bvp);
		return ENOTBLK;
	}
	dev = bvp->v_rdev;
	VN_RELE(bvp);

	/*
	 * Open the special device.  We will keep this vnode around for as
	 * long as the filesystem is mounted.  All subsequent fs I/O is done
	 * on this vnode.
	 */
	bvp = makespecvp(dev, VBLK);
	bvp->v_vfsp = vfsp;

	fmode = rdonly ? FREAD : FREAD|FWRITE;

	if ((error = VOP_OPEN(&bvp, fmode, cr)) != 0) {
		VN_RELE(bvp);
		return error;
	}
	/* use the real device. */
	dev = bvp->v_rdev;
	/*
	 * If device is already mounted, return EBUSY.
	 */
	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	if (vfs_devsearch(dev) != NULL) {
		SLEEP_UNLOCK(&vfslist_lock);
		VOP_CLOSE(bvp, fmode, 1, BFS_INO2OFF(BFSROOTINO), cr);
		VN_RELE(bvp);
		return EBUSY;
	}
	SLEEP_UNLOCK(&vfslist_lock);
	/*
	 * Read only the necessary superblock information from disk.
	 * This includes the header and sanity information.
	 * Note that since a temporary buffer is memory allocated,
	 * care must be taken to dispose of this buffer before
	 * returning to the caller.
	 */
	superbuf = (struct bdsuper *)kmem_alloc(sizeof(struct bdsuper),
						KM_SLEEP);

	error = vn_rdwr(UIO_READ, bvp, superbuf, sizeof(struct bdsuper),
		BFS_SUPEROFF, UIO_SYSSPACE, 0, 0, cr, (int *)0);

	if (!error) {
		if (superbuf->bdsup_bfsmagic != BFS_MAGIC)
			error = EINVAL;

		else if (superbuf->bdcpb_fromblock != -1 &&
			 superbuf->bdcpb_toblock != -1)
			/*
			 * If the last 2 sanity words are not set to "-1",
			 * the file system must be checked before it is mounted.
			 */
			error = ENOSPC;

	}
	if (error) {
		VOP_CLOSE(bvp, fmode, 1, BFS_INO2OFF(BFSROOTINO), cr);
		VN_RELE(bvp);
		kmem_free(superbuf, sizeof(struct bdsuper));
		return error;
	}

	/*
	 * The "bfs_vfs" is constantly referenced for every BFS operation.
	 * It contains all filesystem private info including the device special
	 * vnode.  A pointer to it is contained in the private data field of
	 * the vfs, and is thus passed to every vnodeop and vfsop, even if
	 * indirectly through vnode.v_vfsp.
	 */
	bfsp = (struct bfs_vfs *)kmem_zalloc(sizeof(struct bfs_vfs), KM_SLEEP);

	/*
	 * Save the required VFS info.
	 */
	vfsp->vfs_data = bfsp;	/* Store BFS private data pointer */
	vfsp->vfs_dev = dev;
	vfsp->vfs_fstype = bfs_fstype;
	vfsp->vfs_fsid.val[0] = dev;
	vfsp->vfs_fsid.val[1] = bfs_fstype;
	vfsp->vfs_bsize = BFS_BSIZE;
	vfsp->vfs_bcount = 0;
	if (rdonly)
		vfsp->vfs_flag |= VFS_RDONLY;

	bfsp->bfs_startfs = superbuf->bdsup_start;
	bfsp->bfs_endfs = superbuf->bdsup_end;
	bcopy(superbuf->bdsup_fsname, bfsp->bfs_fsname,
		sizeof(bfsp->bfs_fsname)); 

	kmem_free(superbuf, sizeof(struct bdsuper));

	/*
	 * Start out assuming that we have as much space as the size of the
	 * filesystem, and no free inodes.
	 */
	bfsp->bfs_freeblocks =
		(bfsp->bfs_endfs + 1 - bfsp->bfs_startfs) / BFS_BSIZE;
	bfsp->bfs_freeinodes = 0;

	/*
	 * Store the device special vnode.
	 */
	bfsp->bfs_devnode = bvp;

	bfsp->bfs_inolist = NULL;
	bfsp->bfs_inowlocked = NULL;

	bfs_initlocks(bfsp);

	/*
	 * Get the root inode.
	 */
	if ((error = bfs_iget(vfsp, BFSROOTINO, &ip, B_FALSE, cr)) != 0) {
		VOP_CLOSE(bvp, fmode, 1, BFS_INO2OFF(BFSROOTINO), cr);
		VN_RELE(bvp);
		bfs_deinitlocks(bfsp);
		kmem_free(bfsp, sizeof(struct bfs_vfs));
		return error;
	}
	bfsp->bfs_rootvnode = ITOV(ip);

	BFS_IRWLOCK_UNLOCK(ip);

	/*
	 * Set up the inode bitmap.
	 */
	bfsp->bfs_totalinodes = (bfsp->bfs_startfs - BFS_DINOSTART) /
		sizeof(dinode_t);

	bitmapsize = BITMASK_NWORDS(bfsp->bfs_totalinodes + BFSROOTINO) *
		sizeof(uint_t);

	bfsp->bfs_inobitmap = kmem_zalloc(bitmapsize, KM_SLEEP);
	/*
	 * The first 2 bits of the inode bitmap are never used.
	 * They should be set to 1 so that when looking for the first
	 * available inode in the map (e.g., BITMASKN_FFC()) we will not
	 * get a value less than BFSROOTINO.
	 */
	for (i = 0; i < (int)BFSROOTINO; i++)
		BITMASKN_SET1(bfsp->bfs_inobitmap, i);

	/*
	 * Search through the filesystem, adding to freeinodes each
	 * empty inode slot we find.
	 * For each inode we find, subtract the file size from freeblocks
	 * and set the corresponding bit in the inobitmap.
	 * Also, figure out the lastfile in the file system and its start
	 * and end blocks.
	 */
	dip = &dinode;
	bfsp->bfs_lastfilefs = 0;
	bfsp->bfs_sblklastfile = 0;
	bfsp->bfs_eblklastfile = 0;
 
	for (i = BFSROOTINO; i < bfsp->bfs_totalinodes + BFSROOTINO; i ++) {
		if ((error = BFS_GETINODE(bvp, BFS_INO2OFF(i), dip, cr)) != 0) {
			VN_RELE(ITOV(ip));
			VOP_CLOSE(bvp, fmode, 1, BFS_INO2OFF(BFSROOTINO), cr);
			VN_RELE(bvp);
			bfs_deinitlocks(bfsp);
			kmem_free(bfsp->bfs_inobitmap, bitmapsize);
			kmem_free(bfsp, sizeof(struct bfs_vfs));
			return error;
		}

		if (dip->d_ino == 0)	/* This is an empty slot */
			bfsp->bfs_freeinodes++;
		else {
			BITMASKN_SET1(bfsp->bfs_inobitmap, i);

			bfsp->bfs_freeblocks -= BFS_FILEBLOCKS(dip); 

			if (dip->d_eblock > bfsp->bfs_eblklastfile) {
				bfsp->bfs_sblklastfile = dip->d_sblock;
				bfsp->bfs_eblklastfile = dip->d_eblock;
				bfsp->bfs_lastfilefs = BFS_INO2OFF(i);
			}
		}
	}
	/*
	 * Note that since there is always a non-zero length root directory,
	 * there is always a "lastfile".
	 */
	ASSERT(bfsp->bfs_lastfilefs >= BFS_INO2OFF(BFSROOTINO));
	ASSERT(bfsp->bfs_sblklastfile >= (bfsp->bfs_startfs / BFS_BSIZE));
	ASSERT(bfsp->bfs_eblklastfile >= (bfsp->bfs_startfs / BFS_BSIZE));
	ASSERT(bfsp->bfs_eblklastfile >= bfsp->bfs_sblklastfile);

	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	if (vfs_devsearch(dev) != NULL) {
		/* lost the race */
		SLEEP_UNLOCK(&vfslist_lock);
		VN_RELE(ITOV(ip));
		VOP_CLOSE(bvp, fmode, 1, BFS_INO2OFF(BFSROOTINO), cr);
		VN_RELE(bvp);
		bfs_deinitlocks(bfsp);
		kmem_free(bfsp->bfs_inobitmap, bitmapsize);
		kmem_free(bfsp, sizeof(struct bfs_vfs));
		return EBUSY;
	}
	vfs_add(mvp, vfsp, uap->flags);
	SLEEP_UNLOCK(&vfslist_lock);

	return 0;
}


/*
 * int
 * bfs_unmount(vfs_t *vfsp, cred_t *cr)
 *
 * Calling/Exit State:
 *	The caller must lock the vnode of the mount point in exclusive mode.
 *
 * Description:
 * 	Unmount a BFS filesystem.  Release the device special vnode and free up
 *	all pages that are in core.
 *
 *	This routine may block, so no spin locks may be held on entry.
 */
STATIC int
bfs_unmount(vfs_t *vfsp, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)vfsp->vfs_data;
	vnode_t		*bvp;
	inode_t		*ip;
	int		bitmapsize;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (pm_denied(cr, P_MOUNT))
		return EPERM;

	/*
	 * Grab the vfslist lock to prevent new references
	 * established by other agents, e.g. NFS.
	 */
	SLEEP_LOCK(&vfslist_lock, PRIVFS);

	/*
	 * If anyone is establishing references to files in this
	 * file system, fail the unmount now.
	 */
	(void)LOCK(&vfsp->vfs_mutex, FS_VFSPPL);
	if (vfsp->vfs_count != 0) {
		UNLOCK(&vfsp->vfs_mutex, PLBASE);
		SLEEP_UNLOCK(&vfslist_lock);
		return EBUSY;
	}
	UNLOCK(&vfsp->vfs_mutex, PLBASE);

	(void)LOCK(&bp->bfs_inolist_mutex, FS_BFSLISTPL);
	/*
	 * The root inode must be the only inode in the list and
	 * its reference count must be equal to two. Otherwise FS is busy.
	 */
	ip = bp->bfs_inolist;
	ASSERT(ip->i_diskino.d_ino == BFSROOTINO);
	if (ip->i_next != NULL || ITOV(ip)->v_count > 2) {
		UNLOCK(&bp->bfs_inolist_mutex, PLBASE);
		SLEEP_UNLOCK(&vfslist_lock);
		return EBUSY;
	}
	UNLOCK(&bp->bfs_inolist_mutex, PLBASE);


	/* Remove vfs from vfs list. */
	vfs_remove(vfsp);

	SLEEP_UNLOCK(&vfslist_lock);

	ASSERT(ITOV(ip)->v_count == 2);
	BFS_DEINIT_INODE(ip);
	kmem_free(ip, sizeof(inode_t));

	bvp = bp->bfs_devnode;
	VOP_CLOSE(bvp, (vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE, 1,
	          BFS_INO2OFF(BFSROOTINO), cr);
	VN_RELE(bvp);

	bitmapsize = BITMASK_NWORDS(bp->bfs_totalinodes + BFSROOTINO) *
		sizeof(uint_t);
	kmem_free(bp->bfs_inobitmap, bitmapsize);

	bfs_deinitlocks(bp);
	kmem_free(bp, sizeof(struct bfs_vfs));
	binval(vfsp->vfs_dev);
	return 0;
}


/*
 * int
 * bfs_root(vfs_t *vfsp, vnode_t **vpp)
 *	Return a pointer to the root vnode.
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *
 */
STATIC int
bfs_root(vfs_t *vfsp, vnode_t **vpp)
{
	struct bfs_vfs *bp = (struct bfs_vfs *)vfsp->vfs_data;

	*vpp = bp->bfs_rootvnode;
	VN_HOLD(*vpp);
	return 0;
}


/*
 * int
 * bfs_sync(vfs_t *vfsp, int flag, cred_t *cr)
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *
 * Description:
 *	Flush pending I/O for BFS file system.  Nothing to do, really,
 *	since all BFS I/O is synchronous anyway.
 */
/* ARGSUSED */
STATIC int
bfs_sync(vfs_t *vfsp, int flag, cred_t *cr)
{
	return 0;
}


/*
 * int
 * bfs_vget(vfs_t *vfsp, vnode_t **vpp, struct fid *fidp)
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 * Description:
 *	Given an fid, create or find a vnode. In BFS, we can build a vnode given
 *	the disk inode offset, which is the only thing described by fid.
 */
STATIC int
bfs_vget(vfs_t *vfsp, vnode_t **vpp, struct fid *fidp)
{
	inode_t		*ip;
	bfs_ino_t	ino;

	ASSERT(KS_HOLD0LOCKS());

	ino = BFS_OFF2INO(((struct bfs_fid_overlay *)(void *)fidp)->o_offset);
	if (bfs_iget(vfsp, ino, &ip, B_FALSE, CRED())) {
		*vpp = NULL;
		return 0;
	}

	BFS_IRWLOCK_UNLOCK(ip);
	*vpp = ITOV(ip);
	return 0;
}


/*
 * int
 * bfs_statvfs(vfs_t *vfsp, struct statvfs *sp)
 *	Return filesystem independent information about this VFS.
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *
 */
STATIC int
bfs_statvfs(vfs_t *vfsp, struct statvfs *sp)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)vfsp->vfs_data;

	bzero(sp, sizeof(*sp));
	sp->f_bsize = sp->f_frsize = BFS_BSIZE;
	sp->f_blocks = (bp->bfs_endfs + 1) / BFS_BSIZE;
	sp->f_files = bp->bfs_totalinodes;

	FSPIN_LOCK(&bp->bfs_mutex); 
	sp->f_bfree = sp->f_bavail = bp->bfs_freeblocks;
	sp->f_ffree = sp->f_favail = bp->bfs_freeinodes;
	FSPIN_UNLOCK(&bp->bfs_mutex); 

	sp->f_fsid = vfsp->vfs_dev;
	strcpy(sp->f_basetype, vfssw[vfsp->vfs_fstype].vsw_name);
	sp->f_flag = vf_to_stf(vfsp->vfs_flag);
	sp->f_namemax = BFS_MAXFNLEN;
	strncpy(sp->f_fstr, bp->bfs_fsname, sizeof(bp->bfs_fsname));
	return 0;
}


/*
 * void
 * bfs_initlocks(struct bfs_vfs *bp)
 *	Initialize bfs global locks.
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *
 */
void
bfs_initlocks(struct bfs_vfs *bp)
{
	RWSLEEP_INIT(&bp->bfs_fs_rwlock, 0, &bfs_fs_rwlock_lkinfo, KM_SLEEP);

	SLEEP_INIT(&bp->bfs_writelock, 0, &bfs_writelock_lkinfo, KM_SLEEP);

	LOCK_INIT(&bp->bfs_inolist_mutex, FS_BFSLISTHIER, FS_BFSLISTPL,
		&bfs_inolist_lkinfo, KM_SLEEP);

	FSPIN_INIT(&bp->bfs_mutex);
}


/*
 * void
 * bfs_deinitlocks(struct bfs_vfs *bp)
 *	De-initialize bfs global locks.
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *
 */
void
bfs_deinitlocks(struct bfs_vfs *bp)
{
	RWSLEEP_DEINIT(&bp->bfs_fs_rwlock);

	SLEEP_DEINIT(&bp->bfs_writelock);

	LOCK_DEINIT(&bp->bfs_inolist_mutex);
}
