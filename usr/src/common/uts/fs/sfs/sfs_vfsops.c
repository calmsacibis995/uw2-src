/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/sfs/sfs_vfsops.c	1.55"
#ident	"$Header: $"

#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/file.h>
#include <fs/fs_subr.h>
#include <fs/mount.h>
#include <fs/pathname.h>
#include <fs/sfs/sfs_hier.h>
#include <fs/sfs/sfs_fs.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/sfs/sfs_data.h>
#include <fs/sfs/sfs_quota.h>
#include <fs/specfs/snode.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <mem/seg.h>
#include <mem/swap.h>
#include <mem/seg_map.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/inline.h>

extern void	sfs_update(cred_t *);
extern void	sfs_flushi(uint_t, cred_t *);
extern void	sfs_iupdat(inode_t *, int);
extern int	sfs_iflush(vfs_t *, inode_t *, cred_t *);
extern int	sfs_iget(vfs_t *, fs_t *, ino_t, inode_t **, int,
			 cred_t *);
extern int	sfs_syncip(inode_t *, int, enum iupmode);
extern int	dnlc_purge_vfsp(vfs_t *, int);
extern int	sfs_closedq(sfs_vfs_t *, cred_t *);
extern int	sfs_iremount(vfs_t *);
extern int	sfs_idestroy(inode_t *, cred_t *);
extern void	sfs_freeblocks_sync(vfs_t *, uint_t);
extern vnode_t	*common_specvp(vnode_t *);

/*
 * Declaration for per-VFS deferred free block list lock info
 */
LKINFO_DECL(sfs_defer_lkinfo, "FS:SFS:deferred free block list mutex", 0);

/*
 * sfs vfs operations.
 */
STATIC int sfs_mount(vfs_t *, vnode_t *, struct mounta *, cred_t *);
STATIC int sfs_mountroot(vfs_t *, enum whymountroot);
STATIC int sfs_setceiling(vfs_t *, lid_t);

int sfs_unmount(vfs_t *, cred_t *);
int sfs_root(vfs_t *, vnode_t **);
int sfs_statvfs(vfs_t *, struct statvfs *);
int sfs_sync(vfs_t *, int, cred_t *);
int sfs_vget(vfs_t *, vnode_t **vpp, struct fid *);

int sfs_mount_cmn(vfs_t *, vnode_t *, struct mounta *, cred_t *, long);
int sfs_mountroot_cmn(vfs_t *, enum whymountroot, long);
STATIC int sfs_mountfs_cmn(vfs_t *, enum whymountroot, dev_t, char *,
		cred_t *, long);


vfsops_t sfs_vfsops = {
	sfs_mount,
	sfs_unmount,
	sfs_root,
	sfs_statvfs,
	sfs_sync,
	sfs_vget,
	sfs_mountroot,
	sfs_setceiling,
	(int (*)())fs_nosys,	/* filler[8] */
	(int (*)())fs_nosys,	
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys
};

void sfs_sbupdate(vfs_t *);

/*
 * STATIC int
 * sfs_mount(struct vfs *vfsp, struct vnode *mvp, struct mounta *uap, \
 *    struct cred *cr)
 *    Do fs specific portion of mount.
 *
 * Calling/Exit State:
 *    The mount point vp->v_lock is locked exclusive on entry and remains
 *    locked at exit. Holding this lock prevents new lookups into the
 *    file system the mount point is in (see the lookup code for more).
 *
 * Description:
 *    Wrapper for sfs_mount_cmn() which does the work.
 *
 */
STATIC int
sfs_mount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr)
{
	return(sfs_mount_cmn(vfsp, mvp, uap, cr, SFS_MAGIC));
}

/*
 * int
 * sfs_mount_cmn(struct vfs *vfsp, struct vnode *mvp, struct mounta *uap, \
 *    struct cred *cr, long magic)
 *    Do fs specific portion of mount.
 *
 * Calling/Exit State:
 *    The mount point vp->v_lock is locked exclusive on entry and remains
 *    locked at exit. Holding this lock prevents new lookups into the
 *    file system the mount point is in (see the lookup code for more).
 *
 * Description:
 *    Common code shared by ufs and sfs.
 *
 *    We insure the moint point is 'mountable', i.e., is a directory
 *    that is neither currently mounted on or referenced, and that
 *    the file system to mount is OK (block special file).
 */
int
sfs_mount_cmn(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr,
	      long magic)
{
	dev_t	dev;
	vnode_t	*bvp;
	vfs_t *dvfsp;
	int	error;
	pl_t	s;
	struct pathname	dpn;
	enum whymountroot	why;
	sfs_vfs_t	*sfs_vfsp;
	enum uio_seg seg;

	if (pm_denied(cr, P_MOUNT))  {
		return (EPERM);
	}

	if (mvp->v_type != VDIR) {
		return (ENOTDIR);
	}

	if ((uap->flags & MS_REMOUNT) == 0 &&
	    (mvp->v_count != 1 || (mvp->v_flag & VROOT))) {
		return (EBUSY);
	}

	/*
	 * Get arguments
	 */
	if (uap->flags & MS_SYSSPACE)
		seg = UIO_SYSSPACE;
	else
		seg = UIO_USERSPACE;
	if (error = pn_get(uap->dir, seg, &dpn))  {
		return error;
	}

	/*
	 * Resolve path name of special file being mounted.
	 */
	error = lookupname(uap->spec, seg, FOLLOW, NULLVPP, &bvp);
	if (error) {
		pn_free(&dpn);
		return error;
	}

	if (bvp->v_type != VBLK) {
		VN_RELE(bvp);
		pn_free(&dpn);
		return (ENOTBLK);
	}

	/*
	 * Find the real device via open.
	 */
	error = VOP_OPEN(&bvp, FREAD, cr);
	if (error) {
		VN_RELE(bvp);
		pn_free(&dpn);
		return (error);
	}
	dev = bvp->v_rdev;
	(void) VOP_CLOSE(bvp, FREAD, 1, 0, cr);
	VN_RELE(bvp);

	/*
	 * Ensure that this device isn't already mounted,
	 * unless this is a REMOUNT request
	 */
	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	dvfsp =  vfs_devsearch(dev);
	SLEEP_UNLOCK(&vfslist_lock);
	if (uap->flags & MS_REMOUNT) {
		/*
                 * Remount requires that the device already be mounted,
                 * and on the same mount point.
                 *
                 * This code really should be done at the generic
                 * level, but since other file system types
                 * perform this check, it is done here as well.
                 */

		if (dvfsp == NULL) {
			pn_free(&dpn);
			return (EINVAL);
		} else if (dvfsp != vfsp) {
			pn_free(&dpn);
			return (EBUSY);
		}
		why = ROOT_REMOUNT;
	} else {
		if (dvfsp != NULL) {
                        pn_free(&dpn);
                        return (EBUSY);
                }
		why = ROOT_INIT;
	}	

	if (getmajor(dev) >= bdevcnt) {
		pn_free(&dpn);
		return (ENXIO);
	}

	/*
	 * If the device is a tape, mount it read only
	 */
	s = LOCK(&vfsp->vfs_mutex, FS_VFSPPL);
	if ((*bdevsw[getmajor(dev)].d_flag & D_TAPE) == D_TAPE)
		vfsp->vfs_flag |= VFS_RDONLY;
	if (uap->flags & MS_RDONLY)
		vfsp->vfs_flag |= VFS_RDONLY;
	UNLOCK(&vfsp->vfs_mutex, s);

	/*
	 * Mount the filesystem.
	 */
	error = sfs_mountfs_cmn(vfsp, why, dev, dpn.pn_path, cr, magic);

	if (error)
		goto err_out;

	if (!(uap->flags & MS_REMOUNT)) {
		SLEEP_LOCK(&vfslist_lock, PRIVFS);
		if (vfs_devsearch(dev) != NULL && why != ROOT_REMOUNT) {
			/* if lost the race then free the private data */
			sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
			SLEEP_DEINIT(&sfs_vfsp->vfs_renamelock);
			kmem_free((caddr_t)sfs_vfsp, sizeof(sfs_vfs_t));
			error = EBUSY;
			SLEEP_UNLOCK(&vfslist_lock);
			goto err_out;
		} else
			vfs_add(mvp, vfsp, uap->flags);
		SLEEP_UNLOCK(&vfslist_lock);
	}

	if (uap->flags & MS_SOFTMNT) {
		SFS_VFS_PRIV(vfsp)->vfs_flags |= SFS_SOFTMOUNT;
		SFS_VFS_PRIV(vfsp)->vfs_fbrel_flags = 0;
	} else {
		SFS_VFS_PRIV(vfsp)->vfs_fbrel_flags = SM_WRITE;
	}
	/*
	 * Enable dow on file system, unless mounted soft or read-only
	 */
        if (!(SFS_VFS_PRIV(vfsp)->vfs_flags & SFS_SOFTMOUNT) &&
			!(vfsp->vfs_flag & VFS_RDONLY))
		SFS_VFS_PRIV(vfsp)->vfs_flags |= SFS_DOWENABLED;
	else
		SFS_VFS_PRIV(vfsp)->vfs_flags &= ~SFS_DOWENABLED;
err_out:
	pn_free(&dpn);
	return (error);
}

/*
 * STATIC int
 * sfs_mountroot(struct vfs *vfsp, enum whymountroot why)
 *    Mount an sfs file system as root.
 *
 * Calling/Exit State:
 *    The global vfslist_lock is locked on entry and remain
 *    locked at exit.
 *
 * Description:
 *	Wrapper for sfs_mountroot_cmn().
 *
 */
STATIC int
sfs_mountroot(vfs_t *vfsp, enum whymountroot why)
{
        /*
         * Call the common SFS/UFS mountroot function.
         */
        return(sfs_mountroot_cmn(vfsp, why, SFS_MAGIC));
}

/*
 * int
 * sfs_mountroot_cmn(struct vfs *vfsp, enum whymountroot why, long magic)
 *    Mount an sfs file system as root.
 *
 * Calling/Exit State:
 *    The global vfslist_lock is locked on entry and remain
 *    locked at exit.
 *
 * Description:
 * "why" is ROOT_INIT on initial call. The root file system is mounted
 * read-only. After root is fsck'ed, this function is called again with
 * ROOT_REMOUNT and root is mounted read-write. Why is "ROOT_UNMOUNT"
 * when this function is called by shutdown to unmount the root file
 * system as part of the system shutdown process.
 *
 */
int
sfs_mountroot_cmn(vfs_t *vfsp, enum whymountroot why, long magic)
{
	fs_t	*fsp;
	int	error;
	vnode_t	*vp;
	cred_t	*cr = u.u_lwpp->l_cred;
	static int rootdone = 0;
	int ovflags = vfsp->vfs_flag;

	error = 0;

	if (why == ROOT_INIT) {
		if (rootdone)
			return (EBUSY);
		if (rootdev == (dev_t)NODEV)
			return (ENODEV);
		vfsp->vfs_flag |= VFS_RDONLY;
	} else if (why == ROOT_REMOUNT) {
		vfsp->vfs_flag |= VFS_REMOUNT;
		vfsp->vfs_flag &= ~VFS_RDONLY;
	} else if (why == ROOT_UNMOUNT) {
		sfs_update(cr);
		fsp = getfs(vfsp);
		if (fsp->fs_state == FSACTIVE) {
			fsp->fs_time = hrestime.tv_sec;
			if (vfsp->vfs_flag & VFS_BADBLOCK)
                                fsp->fs_state = FSBAD;
                        else
				fsp->fs_state = FSOKAY - (long)fsp->fs_time;
			vp = ((sfs_vfs_t *)vfsp->vfs_data)->vfs_devvp;
			sfs_sbupdate(vfsp);
			(void) VOP_CLOSE(vp, FREAD|FWRITE, 1, (off_t)0, cr);
			VN_RELE(vp);
		}
		bflush(NODEV);
		bdwait(NODEV);
		pgwait(NODEV);
		return 0;
	}		
	error = sfs_mountfs_cmn(vfsp, why, rootdev, "/", cr, magic);
	if (error) {
		vfsp->vfs_flag = ovflags;
		return (error);
	}
	if (why == ROOT_INIT)
		vfs_add((struct vnode *)0, vfsp,
			(vfsp->vfs_flag & VFS_RDONLY) ? MS_RDONLY : 0);
	else if (why == ROOT_REMOUNT)
		vfsp->vfs_flag &= ~VFS_REMOUNT;

	/*
	 * Enable dow on file system, unless mounted soft or read-only
	 */
        if (!(SFS_VFS_PRIV(vfsp)->vfs_flags & SFS_SOFTMOUNT) &&
			!(vfsp->vfs_flag & VFS_RDONLY))
		SFS_VFS_PRIV(vfsp)->vfs_flags |= SFS_DOWENABLED;
	else
		SFS_VFS_PRIV(vfsp)->vfs_flags &= ~SFS_DOWENABLED;

	rootdone++;
	return (0);
}

/*
 * STATIC int
 * sfs_mountfs_cmn(vfs_t *vfsp, enum whymountroot why, dev_t dev,
 *             char *path, cred_t *cr, long magic)
 *
 * Calling/Exit State:
 */
STATIC int
sfs_mountfs_cmn(vfs_t *vfsp, enum whymountroot why, dev_t dev,
		char *path, cred_t *cr, long magic)
{
	vnode_t *devvp = 0, *cvp;
	fs_t *fsp;
	sfs_vfs_t *sfs_vfsp = 0;
	buf_t *bp = NULL;
	buf_t *tp = 0;
	int error;
	int blks;
	caddr_t base = 0, space = 0;
	int i;
	long size;
	size_t len;
	int needclose = 0;
	inode_t *rip;
	vnode_t *rvp;

	ASSERT(magic == SFS_MAGIC || magic == UFS_MAGIC);
	if (why == ROOT_INIT) {
		/*
		 * Open the device.
		 */
		devvp = makespecvp(dev, VBLK);

		/*
		 * Open block device mounted on.
		 * When bio is fixed for vnodes this can all be vnode
		 * operations.
		 */
		error = VOP_OPEN(&devvp,
		    (vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE, cr);
		if (error) {
			VN_RELE(devvp);
			return (error);
		}
		/* record the real device. */
		dev = devvp->v_rdev;
		needclose = 1;

		/*
		 * Refuse to go any further if this
		 * device is being used for swap.
		 */
		if (devvp->v_flag & VNOMOUNT) {
			VN_RELE(devvp);
			(void) VOP_CLOSE(devvp, (vfsp->vfs_flag & VFS_RDONLY) ?                                 FREAD : FREAD|FWRITE, 1, 0, cr);
			return (EBUSY);
		}
	}
	/*
	 * get dev for already mounted file system since
	 * remount from mountroot is called with rootdev which
	 * sometimes is not the real device.
	 */

	if (vfsp->vfs_flag & VFS_REMOUNT) {
		sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
		devvp = sfs_vfsp->vfs_devvp;
		dev = devvp->v_rdev;
	}
	ASSERT(devvp != 0);
	/* Invalidate pages for this dev */
	cvp = common_specvp(devvp);
        (void) VOP_PUTPAGE(cvp, 0, 0, B_INVAL, cr);
	/*
	 * This is not right to flush out the dirty incore inodes to
	 * overwrite the disk inodes that were just fixed by fsck.
	 * The right way is to read in the disk inodes, compare with
	 * incore inodes and overwrite the incore inodes if it's stale.
	 * The issue is resolved in EA3.
	 */
	if (vfsp->vfs_flag & VFS_REMOUNT) {
		fsp = getfs(vfsp);
		if (!fsp->fs_ronly) {
                        error = EINVAL;
                        goto out;
                }
		if (error = sfs_iremount(vfsp))
			goto out;
	}		
	/*
	 * Read in superblock.
	 */

	/*
	 * In case a copy of the super block is cached in the buffer cache,
	 * mark it stale.
	 */
	tp = getblk(dev, SBLOCK, SBSIZE, BG_NOMISS);
	if (tp) {
		tp->b_flags |= B_STALE|B_AGE;
		brelse(tp);
	}

	tp = bread(dev, SBLOCK, SBSIZE);
	if (tp->b_flags & B_ERROR) {
		goto out;
	}
	fsp = (struct fs *)tp->b_addrp;
	if (fsp->fs_magic != magic || fsp->fs_bsize > MAXBSIZE ||
	    fsp->fs_frag > MAXFRAG || fsp->fs_bsize < sizeof (struct fs) ||
	    fsp->fs_bsize < MINBSIZE) {
		error = EINVAL;	/* also needs translation */
		goto out;
	}
	/*
	 * Allocate VFS private data.
	 */
	if (!(vfsp->vfs_flag & VFS_REMOUNT)) {
		sfs_vfsp = (sfs_vfs_t *)
				kmem_zalloc(sizeof(sfs_vfs_t), KM_SLEEP); 
		vfsp->vfs_bcount = 0;
		vfsp->vfs_data = sfs_vfsp;
		vfsp->vfs_dev = dev;
		vfsp->vfs_flag |= VFS_NOTRUNC;
		vfsp->vfs_fsid.val[0] = dev;
		sfs_vfsp->vfs_devvp = devvp;
		if (magic == SFS_MAGIC) {
                        vfsp->vfs_fsid.val[1] = vfsp->vfs_fstype;
                } else {
                        vfsp->vfs_fsid.val[1] = vfsp->vfs_fstype;
                        sfs_vfsp->vfs_flags |= SFS_UFSMOUNT;
                }
		FSPIN_INIT(&sfs_vfsp->vfs_sbmutex);
		SLEEP_INIT(&sfs_vfsp->vfs_renamelock,
                             (uchar_t) 0, &sfs_renamelock_lkinfo, KM_SLEEP);
		LOCK_INIT(&sfs_vfsp->vfs_defer_lock, KERNEL_HIER_BASE,
			PLMIN, &sfs_defer_lkinfo, KM_SLEEP);
		sfs_vfsp->vfs_defer_blocks = 0;
		sfs_vfsp->vfs_defer_list.im_forw = 
			sfs_vfsp->vfs_defer_list.im_back =
			(inode_t *)&sfs_vfsp->vfs_defer_list;
		sfs_vfsp->vfs_defer_list.im_state = 0;
		SV_INIT(&sfs_vfsp->vfs_defer_idle);
	}
		
	/*
	 * Copy the super block into a buffer in its native size.
	 * Use ngeteblk to allocate the buffer
	 */
	bp = ngeteblk((int)fsp->fs_sbsize);
	bp->b_bcount = fsp->fs_sbsize;
	bcopy((caddr_t)tp->b_un.b_addr, (caddr_t)bp->b_un.b_addr,
		(uint_t)fsp->fs_sbsize);
	tp->b_flags |= B_STALE | B_AGE;
	brelse(tp);
	tp = 0;

	fsp = (struct fs *)bp->b_addrp;
	/*
	 * Currently we only allow a remount to change from
	 * read-only to read-write.
	 */
	if (vfsp->vfs_flag & VFS_RDONLY) {
		ASSERT((vfsp->vfs_flag & VFS_REMOUNT) == 0);
		FSPIN_LOCK(&sfs_vfsp->vfs_sbmutex);
		fsp->fs_ronly = 1;
		fsp->fs_fmod = 0;
		FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);
	} else {
		if (vfsp->vfs_flag & VFS_REMOUNT) {
			if (fsp->fs_state == FSACTIVE) {
				error = EINVAL;
				goto out;
			}
		}

		if ((fsp->fs_state + (long)fsp->fs_time) == FSOKAY)
			fsp->fs_state = FSACTIVE;
		else {
			error = ENOSPC;
			goto out;
		}
		/* write out to disk synchronously */
		tp = ngeteblk((int)fsp->fs_sbsize);
		tp->b_edev = dev;
		tp->b_blkno = SBLOCK;
		tp->b_bcount = fsp->fs_sbsize;
		bcopy((char *)fsp, tp->b_un.b_addr, (uint_t)fsp->fs_sbsize);
		error = bwrite(tp);
		if (error)
			goto out;
		tp = 0;
		FSPIN_LOCK(&sfs_vfsp->vfs_sbmutex);
		fsp->fs_fmod = 1;
		fsp->fs_ronly = 0;
		FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);
	}
	vfsp->vfs_bsize = fsp->fs_bsize;

	/*
	 * Read in cyl group info
	 */
        blks = howmany(fsp->fs_cssize, fsp->fs_fsize);
	base = space = (caddr_t)kmem_alloc((uint_t)fsp->fs_cssize, KM_SLEEP);
	if (space == 0) {
		error = ENOMEM;
		goto out;
	}
	for (i = 0; i < blks; i += fsp->fs_frag) {
		size = fsp->fs_bsize;
		if (i + fsp->fs_frag > blks)
			size = (blks - i) * fsp->fs_fsize;
		tp = bread(dev, (daddr_t)fsbtodb(fsp, fsp->fs_csaddr+i),
			fsp->fs_bsize);
		if (tp->b_flags & B_ERROR) {
			goto out;
		}
		bcopy(tp->b_addrp, (void *)space, (uint_t)size);
		/* LINTED pointer alignment */
		fsp->fs_csp[fragstoblks(fsp, i)] = (struct csum *)space;
		space += size;
		tp->b_flags |= B_AGE;
		brelse(tp);
		tp = 0;
	}
	copystr(path, fsp->fs_fsmnt, sizeof (fsp->fs_fsmnt) - 1, &len);
	bzero(fsp->fs_fsmnt + len, sizeof (fsp->fs_fsmnt) - len);
	if (vfsp->vfs_flag & VFS_REMOUNT) {
		fs_t *tfsp = (fs_t *)(sfs_vfsp->vfs_bufp)->b_addrp;
		kmem_free((caddr_t)tfsp->fs_csp[0], (uint_t)tfsp->fs_cssize);
		bcopy(bp->b_addrp, (void *)tfsp, (uint_t)fsp->fs_sbsize);
		bp->b_flags |= B_AGE;
		brelse(bp);
	} else {
		sfs_vfsp->vfs_bufp = bp;
		if (error = sfs_iget(vfsp, fsp, SFSROOTINO, &rip,
					IG_EXCL|IG_NCREATE|IG_PR_WARN, cr))
			goto out;
		rvp = ITOV(rip);
		rvp->v_flag |= VROOT;
		sfs_vfsp->vfs_root = rvp;
		SFS_IRWLOCK_UNLOCK(rip);
	}
	return (0);
out:
	if (error == 0)
		error = EIO;

	if (base)
		kmem_free((void *)base, (uint_t)fsp->fs_cssize);

	if (bp) {
		bp->b_flags |= B_AGE;
		brelse(bp);
	}

	if (tp) {
		tp->b_flags |= B_AGE;
		brelse(tp);
	}

	if (!(vfsp->vfs_flag & VFS_REMOUNT)) {
		if (sfs_vfsp)  {
			SLEEP_DEINIT(&sfs_vfsp->vfs_renamelock);
			kmem_free((caddr_t)sfs_vfsp, sizeof(sfs_vfs_t));
		}
	}

	if (needclose) {
		(void) VOP_CLOSE(devvp, (vfsp->vfs_flag & VFS_RDONLY) ?
		                 FREAD : FREAD|FWRITE, 1, 0, cr);
		binval(dev);
		VN_RELE(devvp);
	}

	return (error);
}

/*
 * int
 * sfs_unmount(vfs_t *vfsp, cred_t *cr)
 *    Do the fs specific portion of the unmount.
 *
 * Calling/Exit State:
 *    The mount point vp->v_lock is locked exclusive on entry and remains
 *    locked at exit.
 *
 * Description:
 *    Purges dnlc cache entries and flushes inodes while holding the
 *    vfs list locked so that NFS cannot establish any new references
 *    to any files in this file system. Also, the root inode and
 *    in-core superblock are sync'ed back to disk.
 */
int
sfs_unmount(vfs_t *vfsp, cred_t *cr)
{
	dev_t	dev = vfsp->vfs_dev;
	fs_t	*fs;
	int	stillopen;
	sfs_vfs_t	*sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	boolean_t	writable;
	vnode_t	*bvp, *rvp;
	inode_t	*rip;
	inode_t	*qip;
	buf_t	*bp;
	int	err;
	pl_t	pl;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (pm_denied(cr, P_MOUNT))
		return (EPERM);

	if (UFSVFSP(vfsp)) {
		pl = QLIST_LOCK();
		qip = sfs_vfsp->vfs_qinod;
		QLIST_UNLOCK(pl);
	} else {
		qip = sfs_vfsp->vfs_qinod;
	}
	fs = getfs(vfsp);
	bp = sfs_vfsp->vfs_bufp;
	bvp = sfs_vfsp->vfs_devvp;
	writable = !fs->fs_ronly;

	/* Grab the vfslist lock to prevent new references
	 * established by NFS via fhtovp.
	 */
	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	/*
	 * If NFS is establishing references to files in this
	 * file system, fail the unmount now.
	 */
	(void) LOCK(&vfsp->vfs_mutex, FS_VFSPPL);
	if (vfsp->vfs_count != 0) {
		UNLOCK(&vfsp->vfs_mutex, PLBASE);
		SLEEP_UNLOCK(&vfslist_lock);
		return EBUSY;
	}
	/*
	 * turn off delayed ordered writes on this file system (do
	 *	this here since we have vfs_mutex)
	 */
	sfs_vfsp->vfs_flags &= ~SFS_DOWENABLED;
	UNLOCK(&vfsp->vfs_mutex, PLBASE);

	/* dnlc_purge moved here from upper level.
	 * It is done after the vfslist is locked
	 * because only then can we be sure that
	 * there will be no more cache entries
	 * established via vget by NFS.
	 */
	dnlc_purge_vfsp(vfsp, 0);

	stillopen = sfs_iflush(vfsp, qip, cr);
	sfs_freeblocks_sync(vfsp, 0);
	if (stillopen < 0) {
		/*
		 * Do the best job of syncing as we can without
		 * actually unmounting.
		 */
		if (writable && !(sfs_vfsp->vfs_flags & SFS_FSINVALID)) {
			sfs_sbupdate(vfsp);
			bflush(dev);
			bdwait(dev);
		}
		SLEEP_UNLOCK(&vfslist_lock);
		return (EBUSY);
	}
	if (UFSVFSP(vfsp) && qip != NULL) {
		(void) sfs_closedq(sfs_vfsp, cr);
		/*
	 	 * Here we have to sfs_iflush again to get rid of the quota
		 * inode.
	 	 */
		if (sfs_iflush(vfsp, (struct inode *)NULL, cr) < 0) {
			SLEEP_UNLOCK(&vfslist_lock);
			return (EBUSY);
		}
		sfs_sbupdate(vfsp);
	}

	/* Flush root inode to disk */
	rvp = sfs_vfsp->vfs_root;
	ASSERT(rvp != NULL);
	rip = VTOI(rvp);
	SFS_IRWLOCK_WRLOCK(rip);
	if ((err = sfs_syncip(rip, B_INVAL, IUP_FORCE_DELAY)) != 0) {
		SLEEP_UNLOCK(&vfslist_lock);
		return (err);
	}

	/* Remove vfs from vfs list. */
	vfs_remove(vfsp);
	SLEEP_UNLOCK(&vfslist_lock);

	/* Write in-core superblock to disk. */
	kmem_free((caddr_t)fs->fs_csp[0], (uint_t)fs->fs_cssize);
	if (writable && !(sfs_vfsp->vfs_flags & SFS_FSINVALID)) {
		bflush(dev);
		FSPIN_LOCK(&sfs_vfsp->vfs_sbmutex);
		fs->fs_time = hrestime.tv_sec;
		if (vfsp->vfs_flag & VFS_BADBLOCK)
                        fs->fs_state = FSBAD;
                else
			fs->fs_state = FSOKAY - (long)fs->fs_time;
		FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);
                bcopy((char *)fs, bp->b_un.b_addr, (long)fs->fs_sbsize);
                bp->b_edev = dev;
                bp->b_bcount = fs->fs_sbsize;
                bp->b_blkno = SBLOCK;
                bwrite(bp);
	}
	else {
		bp->b_flags |= B_AGE;
		brelse(bp);
	}
	(void) VOP_CLOSE(bvp, writable, 1, 0, cr);
	VN_RELE(bvp);

	/*
	 * We must destroy rip's identity. It is now privately held, except
	 * for some possible SOFTHOLDs from some LWPs out there somewhere.
	 * Therefore, we can access i_state, i_flag, and v_page without
	 * locking. Note that the sfs_idestroy below must succeed because the
	 * sfs_syncip above succeeded!
	 */
	ASSERT(rip->i_state == IDENTITY);
	ASSERT(!(rip->i_flag & (IUPD|IACC|ICHG|IMOD)));
	ASSERT(rvp->v_pages == NULL);
	rip->i_state = (IDENTITY|INVALID|IKMFREE);
	VN_LOCK(rvp);
	ASSERT(rvp->v_count == 2);
	rvp->v_count = 0;
	++rvp->v_softcnt;
	VN_UNLOCK(rvp);
	(void) sfs_idestroy(rip, cr);
	bflush(dev);
	binval(dev);
	SLEEP_DEINIT(&sfs_vfsp->vfs_renamelock);
	kmem_free((caddr_t)sfs_vfsp, sizeof(sfs_vfs_t));

	/*
         * If not mounted read only then call bdwait()
         * to wait for async I/O to complete.
         */
        if (writable) {
                bdwait(dev);
                pgwait(dev);
	}

	return (0);
}

/*
 * int
 * sfs_root(vfs_t *vfsp, vnode_t **vpp)
 *
 * Calling/Exit State:
 */
int
sfs_root(vfs_t *vfsp, vnode_t **vpp)
{
	sfs_vfs_t *sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	struct vnode *vp = sfs_vfsp->vfs_root;

	VN_HOLD(vp);
	*vpp = vp;
	return(0);
}

/*
 * int
 * sfs_statvfs(vfs_t *vfsp, struct statvfs *sp)
 *	Return file system specifics for a given
 *	file system.
 *
 * Calling/Exit State:
 *	No relevant locking on entry or exit.
 */
int
sfs_statvfs(vfs_t *vfsp, struct statvfs *sp)
{
	fs_t *fsp;
	int blk, i;
	long bavail;
	sfs_vfs_t *sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	pl_t pl;
	struct inode_marker *headp;

	fsp = getfs(vfsp);
	if ((fsp->fs_magic != UFS_MAGIC) && (fsp->fs_magic != SFS_MAGIC))
		return (EINVAL);

	(void)bzero((caddr_t)sp, (int)sizeof(*sp));
	sp->f_bsize = fsp->fs_bsize;
	sp->f_frsize = fsp->fs_fsize;
	sp->f_blocks = fsp->fs_dsize;
	/*
	 * lock deferred free block list, wait for deferred free block list
	 *	to be quiescent, and get count of deferred free blocks
	 */
	headp = &sfs_vfsp->vfs_defer_list;
	pl = SFS_DEFLOCK(sfs_vfsp);
	while (headp->im_state != 0) {
		ASSERT(headp->im_state > 0);
		if (!SFS_DEFWAIT(sfs_vfsp)) {
			return (EINTR);
		}
		pl = SFS_DEFLOCK(sfs_vfsp);
	}
	sp->f_bfree = sfs_vfsp->vfs_defer_blocks / btodb(fsp->fs_fsize);

	/* Lock superblock to insure data consistency. */
	FSPIN_LOCK(&sfs_vfsp->vfs_sbmutex);
	/* 
	 * For latency, we perform some calculations after releasing
	 * the fast spin lock and will just collect the data to
	 * be manipulated here.
	 */
	sp->f_bfree += fsp->fs_cstotal.cs_nbfree * fsp->fs_frag +
		       fsp->fs_cstotal.cs_nffree;

	/*
	 * avail = MAX(max_avail - used, 0)
	 */


	if (!UFSVFSP(vfsp)) 
		bavail = sp->f_bfree;
	else
		bavail = (fsp->fs_dsize * (100 - fsp->fs_minfree) / 100) -
		         (fsp->fs_dsize - sp->f_bfree);

	sp->f_ffree = sp->f_favail = fsp->fs_cstotal.cs_nifree;

	FSPIN_UNLOCK(&sfs_vfsp->vfs_sbmutex);

	/*
	 * release lock on deferred free block list
	 */
	SFS_DEFUNLOCK(sfs_vfsp, pl);

	sp->f_bavail = bavail < 0 ? 0 : bavail;

	sp->f_files =  fsp->fs_ncg * fsp->fs_ipg;
	if (!UFSVFSP(vfsp)) {
		/*
	 	 * inodes - divide for alternate inodes
	 	 */
		sp->f_files /=  NIPFILE;
	}


	sp->f_fsid = vfsp->vfs_dev;

	(char *) strcpy(sp->f_basetype, vfssw[vfsp->vfs_fstype].vsw_name);

	sp->f_flag = vf_to_stf(vfsp->vfs_flag);
	sp->f_namemax = (MAXNAMELEN-1);
	
	if (fsp->fs_cpc != 0) {
		blk = fsp->fs_spc * fsp->fs_cpc / NSPF(fsp);
	
		for (i = 0; i < blk; i += fsp->fs_frag)
			/* do nothing */;
		i  -= fsp->fs_frag;
		blk = i / fsp->fs_frag;
		bcopy((char *)&(fsp->fs_rotbl[blk]), sp->f_fstr, 14);
	}

	return (0);
}

/*
 * int
 * sfs_sync(vfs_t *vfsp, int flag, cred_t *cr)
 *	Flush any pending I/O to file system vfsp.
 * Calling/Exit State:
 *
 * Remarks:
 * The sfs_update() routine will only flush *all* sfs files.
 */
/*ARGSUSED*/
int
sfs_sync(vfs_t *vfsp, int flag, cred_t *cr)
{
	static int flushtime = 0;

	if (flag & SYNC_ATTR) {
		if (++flushtime < sfs_tflush)
			return (0);
		flushtime = 0;
		sfs_flushi(SYNC_ATTR, cr);
	} else {
		sfs_update(cr);
	}
	return (0);
}

/*
 * void
 * sfs_sbupdate(vfs_t *vfsp)
 * 
 * Calling/Exit State:
 */
void
sfs_sbupdate(vfs_t *vfsp)
{
	fs_t	*fs = getfs(vfsp);
	buf_t	*bp;
	int	blks;
	caddr_t	space;
	long	size;
	int 	i;

        bp = ngeteblk(fs->fs_sbsize);
        bp->b_edev = vfsp->vfs_dev;
        bp->b_blkno = SBLOCK;
        bp->b_bcount = fs->fs_sbsize;
        bcopy((caddr_t)fs, bp->b_un.b_addr, (uint)fs->fs_sbsize);
        bwrite(bp);

        blks = howmany(fs->fs_cssize, fs->fs_fsize);
        space = (caddr_t)fs->fs_csp[0];
        for (i = 0; i < blks; i += fs->fs_frag) {
                size = fs->fs_bsize;
                if (i + fs->fs_frag > blks)
                        size = (blks -i) * fs->fs_fsize;
                bp = getblk(vfsp->vfs_dev,
                        (daddr_t)(fsbtodb(fs, fs->fs_csaddr+i)),
                        fs->fs_bsize, 0);
                bcopy(space, bp->b_un.b_addr, (uint_t)size);
                space += size;
                bp->b_bcount = size;
                bwrite(bp);

        }
}


/*
 * int
 * sfs_vget(vfs_t *vfsp, vnode_t **vpp, struct fid *fidp)
 *	Given a file identifier, return a vnode for
 *	the file possible.
 *
 * Calling/Exit State:
 *	The file system that we're going to retrieve the inode
 *	from is protected against unmount by getvfs() -- see vfs.c
 */
/* ARGSUSED */
int
sfs_vget(vfs_t *vfsp, vnode_t **vpp, struct fid *fidp)
{
	struct ufid *ufid;
	fs_t	*fs = getfs(vfsp);
	inode_t	*ip;

	/* LINTED pointer alignment*/
	ufid = (struct ufid *)fidp;
	if (sfs_iget(vfsp, fs, ufid->ufid_ino, &ip, IG_NCREATE,
		     u.u_lwpp->l_cred)) {
		*vpp = NULL;
		return (0);
	}
	if (ip->i_gen != ufid->ufid_gen) {
		VN_RELE(ITOV(ip));
		*vpp = NULL;
		return (0);
	}
	*vpp = ITOV(ip);

	return (0);
}

/*
 * STATIC int
 * sfs_setceiling(vfs_t *vfsp, lid_t level)
 *
 * Calling/Exit State:
 *      This routine sets the ceiling of the mounted file system
 *       checks are done at fs independent level to make sure
 *      that that new level dominates the floor of the mounted
 *      file system.  Note that the level of the root vnode is the
 *      same as the level of floor ceiling of the file system
 *      the level of the root vnode cannot change while it is mounted
 */
/* ARGSUSED */
STATIC int
sfs_setceiling(vfs_t *vfsp, lid_t level)
{
	vfsp->vfs_macceiling = level;
	return (0);
}
