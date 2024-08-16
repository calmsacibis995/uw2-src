/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/s5fs/s5vfsops.c	1.31"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/dnlc.h>
#include <fs/fbuf.h>
#include <fs/file.h>
#include <fs/fs_subr.h>
#include <fs/mount.h>
#include <fs/s5fs/s5data.h>
#include <fs/s5fs/s5filsys.h>
#include <fs/s5fs/s5hier.h>
#include <fs/s5fs/s5ino.h>
#include <fs/s5fs/s5inode.h>
#include <fs/s5fs/s5macros.h>
#include <fs/s5fs/s5param.h>
#include <fs/specfs/snode.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/open.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <mem/seg.h>
#include <mem/swap.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>


#define	SBSIZE	sizeof(struct filsys)

STATIC int s5_fs_init();


extern	int	s5_iflush(vfs_t *, int);
extern	int	s5_igrab(inode_t *);
extern	int	s5_idestroy(inode_t *);
extern	int	s5_syncip(inode_t *, int, enum iupmode);
extern	void	s5_iupdat(inode_t *);
extern	void	s5_iput(inode_t *);
extern	int	s5_iremount(vfs_t *);
extern vnode_t *common_specvp(vnode_t *vp);

int		s5_flushi(int);
STATIC void	s5_update(void);
STATIC void	s5_flushsb(vfs_t *);

/*
 * UNIX file system VFS operations vector.
 */
STATIC int	s5_mount(vfs_t *, vnode_t *, struct mounta *, cred_t *);
STATIC int	s5_unmount(vfs_t *, cred_t *);
STATIC int	s5_root(vfs_t *, vnode_t **);
STATIC int	s5_statvfs(vfs_t *, struct statvfs *);
STATIC int	s5_sync(vfs_t *, int, cred_t *);
STATIC int	s5_vget(vfs_t *, vnode_t **vpp, struct fid *);
STATIC int	s5_mountroot(vfs_t *, enum whymountroot);

struct vfsops s5_vfsops = {
	s5_mount,
	s5_unmount,
	s5_root,
	s5_statvfs,
	s5_sync,
	s5_vget,
	s5_mountroot,
	(int (*)())fs_nosys,	/* filler */
	(int (*)())fs_nosys,	/* filler */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
};

/*
 * int
 * s5_mountfs(vfs_t *vfsp, whymountroot_t why, dev_t dev, cred_t *cr)
 *	Mount a s5 filesystem.
 *
 * Calling/Exit State:
 *	May be called from various situations: calling context is
 *	given in <why> and <vfsp->vfs_flag>.
 *
 *	If <why> is ROOT_INIT, then this is a 'normal' mount request,
 *	i.e., not a re-mount operation.
 *
 *	If <vfsp->vfs_flag & VFS_REMOUNT> is set, then we're
 *	going to transition the file system from Read-Only to
 *	Read-Write.
 *
 * Description:
 *	For non-remount mounts we must construct an snode for the
 *	device being mounted and open the device before attempting
 *	the actual mount.
 *
 *	For remounts we must wait for any possible fsck I/O rundown
 *	before we transition to remounting. Note that remounts only
 *	work reliable for mounting the root of the *system* since
 *	after bdwait() we don't do anything to prevent I/O from
 *	occuring. The controlled environment of mounting the system's
 *	root file system prevents I/O from occuring after bdwait()
 *	but for non-root file systems we don't have that luxury.
 */
STATIC int
s5_mountfs(vfs_t *vfsp, whymountroot_t why, dev_t dev, cred_t *cr)
{
	vnode_t		*devvp, *cvp;
	filsys_t	*fsp, *oldfsp;
	buf_t		*bp;
	int		error;
	inode_t		*rip;
	vnode_t		*rvp;
	s5_fs_t	*s5fsp;

	devvp = NULLVP;
	oldfsp = (filsys_t *)NULL;
	bp = (buf_t *)NULL;
	s5fsp = (s5_fs_t *)NULL;
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
		/* recorded the real device */
		dev = devvp->v_rdev;
		/*
	 	 * Allocate VFS private data.
	 	 */
		if ((s5fsp = (s5_fs_t *)
		     kmem_alloc(sizeof(s5_fs_t), KM_SLEEP)) == NULL) {
			VN_RELE(devvp);
			return (ENOMEM);
		}
		vfsp->vfs_bcount = 0;
		vfsp->vfs_data = (caddr_t)s5fsp;
		vfsp->vfs_op = &s5_vfsops;
		vfsp->vfs_dev = dev;
		vfsp->vfs_fsid.val[0] = dev;
		vfsp->vfs_fsid.val[1] = vfsp->vfs_fstype;
		SLEEP_INIT(&s5fsp->fs_sblock,
		   (uchar_t) 0, &s5_sblock_lkinfo, KM_SLEEP);	
		SLEEP_INIT(&s5fsp->fs_renamelock,
		   (uchar_t) 0, &s5_renamelock_lkinfo, KM_SLEEP);	
		s5fsp->fs_bufp = geteblk();
		s5fsp->fs_devvp = devvp;
	}
	/* check for dev already mounted on */
	if (vfsp->vfs_flag & VFS_REMOUNT) {
		s5fsp = (s5_fs_t *)vfsp->vfs_data;
		devvp = s5fsp->fs_devvp;
		dev = devvp->v_rdev;
	}
	ASSERT(devvp != 0);
	/* Invalidate pages for this dev */
        cvp = common_specvp(devvp);
        (void) VOP_PUTPAGE(cvp, 0, 0, B_INVAL, cr);

	if (why == ROOT_REMOUNT) {
		fsp = getfs(vfsp);
		if (!fsp->s_ronly) {
			error =  EINVAL;
			goto closeout;
		}
		if (error = s5_iremount(vfsp))
			goto closeout;
	}

	/*
	 * Read the superblock.  We do this in the remount case as well
	 * because it might have changed (if fsck was applied, for example).
	 */
	bp = getblk(dev, SUPERB, SBSIZE, BG_NOMISS); 
	if (bp) {
		bp->b_flags |= B_STALE|B_AGE;
		brelse(bp);	
	} 
	bp = bread(dev, SUPERB, SBSIZE);
	if (bp->b_flags & B_ERROR) {
		error = geterror(bp);
		brelse(bp);
		goto closeout;
	}
	fsp = getfs(vfsp); 
	if (why == ROOT_REMOUNT) {
		/*
		 * Save the contents of the in-core superblock so it
		 * can be restored if the mount fails.
		 */
		oldfsp = (struct filsys *)kmem_alloc(SBSIZE, KM_SLEEP);
		bcopy((caddr_t)fsp, (caddr_t)oldfsp, SBSIZE);
	}
	bcopy((caddr_t)bp->b_un.b_addr, (caddr_t)fsp, SBSIZE);

	if (fsp->s_magic != FsMAGIC) {
		brelse(bp);
		error = EINVAL;
		goto closeout;
	}
	fsp->s_ilock = 0;
	fsp->s_flock = 0;
	fsp->s_ninode = 0;
	fsp->s_inode[0] = 0;
	if (vfsp->vfs_flag & VFS_RDONLY) {
		ASSERT((vfsp->vfs_flag & VFS_REMOUNT) == 0);
		fsp->s_fmod = 0;
		fsp->s_ronly = 1;
		brelse(bp);
	} else {
		if (vfsp->vfs_flag & VFS_REMOUNT) {
			if (fsp->s_state == FsACTIVE) {
				error = EINVAL;
				brelse(bp);
				goto closeout;
			}
		}
		if (fsp->s_state + (long)fsp->s_time == FsOKAY) {
			fsp->s_state = FsACTIVE;
			bcopy((caddr_t)fsp, bp->b_un.b_addr, SBSIZE);
			bwrite(bp);
		} else {
			brelse(bp);
			error = ENOSPC;
			goto closeout;
		}
		fsp->s_fmod = 1;
		fsp->s_ronly = 0;
	}

	/*
	 * Determine blocksize.
	 */
	vfsp->vfs_bsize = FsBSIZE(fsp->s_bshift);
	if (vfsp->vfs_bsize < FsMINBSIZE || vfsp->vfs_bsize > MAXBSIZE) {
		error = EINVAL;
		goto closeout;
	}

	s5_fs_init(s5fsp, vfsp->vfs_bsize);

	if (why == ROOT_INIT) {
		if (error = s5_iget(vfsp, fsp, S5ROOTINO, IG_EXCL, &rip))
			goto closeout;
		rvp = ITOV(rip);
		rvp->v_flag |= VROOT;
		s5fsp->fs_root = rvp;
		S5_IRWLOCK_UNLOCK(rip);
	}

	return 0;

closeout:
	/* 
	 * Clean up on error.
	 */
	if (oldfsp) {
		bcopy((caddr_t)fsp, (caddr_t)oldfsp, SBSIZE);
		kmem_free((caddr_t)oldfsp, SBSIZE);
	}
	if (why == ROOT_REMOUNT)
		return error;
	(void) VOP_CLOSE(devvp,
	     (vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE, 1, 0, cr);
	brelse(s5fsp->fs_bufp);

	ASSERT(error);
	SLEEP_DEINIT(&s5fsp->fs_sblock);
	SLEEP_DEINIT(&s5fsp->fs_renamelock);
	kmem_free((caddr_t) s5fsp, sizeof(s5_fs_t));
	VN_RELE(devvp);
	return error;
}
/*
 * s5_mount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr)
 *    Do fs specific portion of mount.
 *
 * Calling/Exit State:
 *    The mount point vp->v_lock is locked exclusive on entry and remains
 *    locked at exit. Holding this lock prevents new lookups into the
 *    file system the mount point is in (see the lookup code for more).
 *
 * Description:
 *    We insure the moint point is 'mountable', i.e., is a directory
 *    that is neither currently mounted on or referenced, and that
 *    the file system to mount is OK (block special file).
 */
STATIC int
s5_mount(vfs_t *vfsp, vnode_t *mvp, struct mounta *uap, cred_t *cr)
{
	dev_t		dev;
	vfs_t		*dvfsp;
	s5_fs_t		*s5fsp;
	int		error;
	vnode_t		*bvp;
	enum		whymountroot	why;
	filsys_t	*fp;
	enum uio_seg seg;

	if (pm_denied(cr, P_MOUNT))
		return EPERM;
	if (mvp->v_type != VDIR)
		return ENOTDIR;
	if ((uap->flags & MS_REMOUNT) == 0 &&
	    (mvp->v_count > 1 || (mvp->v_flag & VROOT)))
		return EBUSY;

        /*
         * Get arguments
         */
        if (uap->flags & MS_SYSSPACE)
                seg = UIO_SYSSPACE;
        else
                seg = UIO_USERSPACE;

	/*
	 * Resolve path name of special file being mounted.
	 */
	if (error = lookupname(uap->spec, seg, FOLLOW, NULLVPP, &bvp))
		return error;

	if (bvp->v_type != VBLK) {
		VN_RELE(bvp);
		return ENOTBLK;
	}
	/*
	 *  Find the real device via open().
	 */
	error = VOP_OPEN(&bvp, FREAD, cr);
	if (error)
	{
		VN_RELE(bvp);
		return (error);
	}
	dev = bvp->v_rdev;
	(void) VOP_CLOSE(bvp, FREAD, 1, 0, cr);
	VN_RELE(bvp);

	/*
	 * Ensure that this device isn't already mounted, unless this is
	 * a remount request.
	 */
	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	dvfsp = vfs_devsearch(dev);
	if (dvfsp != NULL) {
		if (uap->flags & MS_REMOUNT) {
			fp = getfs(vfsp);
                        if (!fp->s_ronly) {
				SLEEP_UNLOCK(&vfslist_lock);
                                return EINVAL;
			}
			why = ROOT_REMOUNT;
			vfsp->vfs_flag |= VFS_REMOUNT;
		} else {
			SLEEP_UNLOCK(&vfslist_lock);
			return (EBUSY);
		}
	} else {
		if (uap->flags & MS_REMOUNT) {
			SLEEP_UNLOCK(&vfslist_lock);
                        return EINVAL;
		}
		why = ROOT_INIT;
	}
	SLEEP_UNLOCK(&vfslist_lock);
	if (uap->flags & MS_RDONLY){
		vfsp->vfs_flag |= VFS_RDONLY;
	}
	/*
	 * Mount the filesystem.
	 */
	error = s5_mountfs(vfsp, why, dev, cr);
	if ((error == 0) && (!(uap->flags & MS_REMOUNT))) {
		SLEEP_LOCK(&vfslist_lock, PRIVFS);
		if (vfs_devsearch(dev) != NULL && why != ROOT_REMOUNT){
			/* if lost the race free up the private data */
			s5fsp = S5FS(vfsp);
			SLEEP_DEINIT(&s5fsp->fs_sblock);
			SLEEP_DEINIT(&s5fsp->fs_renamelock);
			kmem_free((caddr_t)s5fsp, sizeof(s5_fs_t));
			error = EBUSY;
		} else
			vfs_add(mvp, vfsp, uap->flags);
		SLEEP_UNLOCK(&vfslist_lock);
	}
	return error;
}

/*
 * s5_unmount(vfs_t *vfsp, cred_t *cr)
 *	Do the fs specific portion of the unmount.
 *
 * Calling/Exit State:
 *	The mount point vp->v_lock is locked exclusive on entry and remains
 *	locked at exit.
 *
 * Description:
 *	Flushes inodes while holding the
 *	vfs list locked so that NFS cannot establish any new references
 *	to any files in this file system. Also, the root inode and
 *	in-core superblock are sync'ed back to disk.
 */
STATIC int
s5_unmount(vfs_t *vfsp, cred_t *cr)
{
	dev_t		dev;
	vnode_t		*bvp, *rvp;
	filsys_t	*fp;
	inode_t		*rip;
	s5_fs_t	*s5fsp;
	int		error;
	pl_t		s;
	buf_t		*bp;
	boolean_t	writable;

	s5fsp = S5FS(vfsp);
	ASSERT(vfsp->vfs_op == &s5_vfsops);

	fp = getfs(vfsp);
	writable = !fp->s_ronly;

	if (pm_denied(cr, P_MOUNT))
		return EPERM;
	/* Grab vfs list lock to prevent NFS establishes
 	 * a new reference to it via fhtovp.
 	*/
	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	/* if NFS wins the race fails the unmount */
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

	dev = vfsp->vfs_dev;
	if ((error = s5_iflush(vfsp, 0)) < 0) {
		/*
                 * Do the best job of syncing as we can without
                 * actually unmounting.
                 */
		if (writable ) {
			s5_flushsb(vfsp);
			bflush(dev);
			bdwait(dev);
		}
		SLEEP_UNLOCK(&vfslist_lock);
		return EBUSY;
	}

	/*
	 * Flush root inode to disk.
	 */
	rvp = s5fsp->fs_root;
	ASSERT(rvp != NULL);
	rip = VTOI(rvp);
	S5_IRWLOCK_WRLOCK(rip);
	if ((error = s5_syncip(rip, B_INVAL, IUP_SYNC)) != 0) {
		S5_IRWLOCK_UNLOCK(rip);
		SLEEP_UNLOCK(&vfslist_lock);
		return(error);
	}
	/* Remove vfs from vfs list. */
	vfs_remove(vfsp);
	SLEEP_UNLOCK(&vfslist_lock);
	/*
	 * At this point there should be no active files on the
	 * file system, and the super block should not be locked.
	 * write in-core superblock to disk 
	 */
	bvp = s5fsp->fs_devvp;
	if (writable) {
		bflush(dev);
		SLEEP_LOCK(&s5fsp->fs_sblock, PRIVFS);
		fp->s_time = hrestime.tv_sec;
		if (vfsp->vfs_flag & VFS_BADBLOCK)
                        fp->s_state = FsBAD;
                else
			fp->s_state = FsOKAY - (long)fp->s_time;
		SLEEP_UNLOCK(&s5fsp->fs_sblock);
		bp = getblk(dev, SUPERB, SBSIZE, BG_NOMISS); 
		if (bp) {
			bp->b_flags |= B_STALE|B_AGE;
			brelse(bp);
		}
		bp = bread(dev, SUPERB, SBSIZE);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			S5_IRWLOCK_UNLOCK(rip);
			return error;
		}
		bcopy((caddr_t)fp, (caddr_t)bp->b_un.b_addr, SBSIZE);
		bwrite(bp);
	}
	error = VOP_CLOSE(bvp, 
		(vfsp->vfs_flag & VFS_RDONLY) ? FREAD : FREAD|FWRITE,
		 1, (off_t) 0, cr);
	if (error) {
		S5_IRWLOCK_UNLOCK(rip);
		return error;
	}
	VN_RELE(bvp);
	/*
	 * We must destroy rip's identity. It is now privately held, except
	 * for some possible SOFTHOLDs from some LWPs out there somewhere.
	 * Therefore, we can access i_state, i_flag, and v_page without
	 * locking. Note that the s5_idestroy below must succeed because the
	 * s5_syncip above succeeded!
	 */
	ASSERT(rip->i_state == IDENTITY);
	ASSERT(!(rip->i_flag & (IUPD|IACC|ICHG|IMOD)));
	ASSERT(rvp->v_pages == NULL);
	rip->i_state = (IDENTITY|INVALID|ILCLFREE);
	VN_LOCK(rvp);
	ASSERT(rvp->v_count == 2);
	rvp->v_count = 0;
	++rvp->v_softcnt;
	VN_UNLOCK(rvp);
	(void) s5_idestroy(rip);
	bflush(dev);
	binval(dev);
	brelse(s5fsp->fs_bufp);
	SLEEP_DEINIT(&s5fsp->fs_sblock);
	SLEEP_DEINIT(&s5fsp->fs_renamelock);
	kmem_free((caddr_t)s5fsp, sizeof(s5_fs_t));
	/*
         * If not mounted read only then call bdwait()
         * to wait for async I/O to complete.
         */
        if (writable) {
                bdwait(dev);
		pgwait(dev);
	}
	return 0;
}

/* 
 * int 
 * s5_root(vfs_t *vfsp, vnode_t **vpp)
 *
 * Calling/Exit State:
 */
STATIC int
s5_root(vfs_t *vfsp, vnode_t **vpp)
{
	s5_fs_t	*s5fsp;
	vnode_t		*vp;

	s5fsp = S5FS(vfsp);
	vp = s5fsp->fs_root;
	VN_HOLD(vp);
	*vpp = vp;
	return 0;
}

/* int
 * s5_statvfs(vfs_t *vfsp, statvfs_t *sp)
 * 	Return file system specifics for a given s5
 *	file system.
 *
 * Calling/Exit State:
 *	No locking on entry or exit.
 *
 */
STATIC int
s5_statvfs(vfs_t *vfsp, statvfs_t *sp)
{
	filsys_t *fp;
	s5_fs_t	*s5fsp;
	int i;
	char *cp;

	s5fsp = S5FS(vfsp);
	fp = getfs(vfsp);
	if (fp->s_magic != FsMAGIC)
		return EINVAL;

	bzero((caddr_t)sp, sizeof(*sp));
	sp->f_bsize = sp->f_frsize = vfsp->vfs_bsize;
	sp->f_blocks = fp->s_fsize;
	sp->f_files = (fp->s_isize - 2) * s5fsp->fs_inopb;
	SLEEP_LOCK(&s5fsp->fs_sblock, PRIVFS);
	sp->f_ffree = sp->f_favail = fp->s_tinode;
	sp->f_bfree = sp->f_bavail = fp->s_tfree;
	SLEEP_UNLOCK(&s5fsp->fs_sblock);
	sp->f_fsid = vfsp->vfs_dev;
	strcpy(sp->f_basetype, vfssw[vfsp->vfs_fstype].vsw_name);
	sp->f_flag = vf_to_stf(vfsp->vfs_flag);
	sp->f_namemax = DIRSIZ;
	cp = &sp->f_fstr[0];
	for (i=0; i < sizeof(fp->s_fname) && fp->s_fname[i] != '\0'; i++,cp++)
		*cp = fp->s_fname[i];
	*cp++ = '\0';
	for (i=0; i < sizeof(fp->s_fpack) && fp->s_fpack[i] != '\0'; i++,cp++)
		*cp = fp->s_fpack[i];
	*cp = '\0';

	return 0;
}


/* 
 * int
 * s5_sync(vfs_t *vfsp, int flag, cred_t *cr)
 *	Flush all the pages associated with an inode using the given flags,
 *	then force inode information to be written back using the given flags.
 *
 * Calling/Exit State:
 *	The inode's rwlock lock must be held on entry and exit.
 * 
 * Remarks:
 *	The s5_update() function only flush s5 files.
 */
/* ARGSUSED */
STATIC int
s5_sync(vfs_t *vfsp, int flag, cred_t *cr)
{
	static int flushtime = 0;

	if (flag & SYNC_ATTR) {
		if (++flushtime < s5_tflush)
			return (0);
		flushtime =0;
		s5_flushi(SYNC_ATTR);
	} else
		s5_update();
	return 0;
}

/*
 * void
 * s5_update()
 *	Performs the S5 component of 'sync'.
 *
 * Calling/Exit State:
 *	Called from s5_sync(). No locks are held on entry and exit.
 *
 * * Description:
 *	We go through the disk queues to initiate sandbagged I/O. It
 *	goes through the list of inodes and writes all modified ones.
 *	Modified superblocks of s5 file systems are written to disk.
 *	
 */
STATIC void
s5_update()
{
	vfs_t	*vfsp;

	/*
	 * Avoid performing a sync if there's one in progress
	 */
	if (SLEEP_TRYLOCK(&s5_updlock) != B_TRUE) {
		return;
	}
	SLEEP_LOCK(&vfslist_lock, PRIVFS);
	for (vfsp = rootvfs; vfsp != NULL; vfsp = vfsp->vfs_next)
		if (vfsp->vfs_op == &s5_vfsops)
			s5_flushsb(vfsp);
	SLEEP_UNLOCK(&vfslist_lock);
	SLEEP_UNLOCK(&s5_updlock);

	s5_flushi(0);
	bflush(NODEV);
}

/*
 * int
 * s5_flushi(int flag)
 *	Each modified inode is written to disk.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	We traverse the inode table looking for idle,
 *	modified inodes. If we find one, we try to
 *	grab it. If we do grab it, we flush the inode
 *	to disk (if flag & SYNC_ATTR) or both the
 *	inode and it's pages to disk.
 */
int
s5_flushi(int flag)
{
	inode_t	*ip, *ipx;
	int	i, cheap;
	struct	inode_marker *mp;
	struct hinode	*ih;

	cheap = flag & SYNC_ATTR;
	S5_CREATE_MARKER(mp);
	for (i = 0; i < NHINO; i++) {
		(void) LOCK(&s5_inode_table_mutex, FS_S5LISTPL);
		ih = &hinode[i];
		for (ip = ih->ih_forw; ip != (inode_t *)ih; ip = ipx) {
			ipx = ip->i_forw;
			if (ip->i_state == IMARKER)	/* skip */
				continue;
			if ((((ITOV(ip))->v_pages == NULL) &&
			    (ip->i_flag & (IACC|IUPD|ICHG|IMOD)) == 0) ||
				(ITOV(ip)->v_flag & VNOSYNC) || /* not mod'd */
				!S5_IRWLOCK_IDLE(ip)) {		/* or locked */
					continue;		/* ignore    */
			}
			if (s5_igrab(ip)) {	
				S5_INSERT_MARKER(mp, ipx);
				UNLOCK(&s5_inode_table_mutex, PLBASE);
				/* 
 				 * the inode is locked exclusively 
				 */
				if (cheap || ip->i_swapcnt > 0 || IS_STICKY(ip))
					s5_iupdat(ip);
				else
					(void) s5_syncip(ip, B_ASYNC, IUP_DELAY);
				S5_IRWLOCK_UNLOCK(ip);
				VN_SOFTRELE(ITOV(ip));
				(void)LOCK(&s5_inode_table_mutex, FS_S5LISTPL);
				ipx = mp->im_forw;
				S5_REMOVE_MARKER(mp);
			}
		}
		UNLOCK(&s5_inode_table_mutex, PLBASE);
	}
	S5_DESTROY_MARKER(mp);

	return 0;
}

/* 
 * void
 * s5_flushsb(vfs_t *vfsp)
 *	update the superblock.
 *
 * Calling/Exit State:
 *	No locks are held on entry and exit.
 */
STATIC void
s5_flushsb(vfs_t *vfsp)
{
	filsys_t *fp;
	vnode_t *vp;
	s5_fs_t	*s5fsp;
	buf_t *bp;

	fp = getfs(vfsp);
	s5fsp = S5FS(vfsp);
	SLEEP_LOCK(&s5fsp->fs_sblock, PRIVFS);
	if (fp->s_fmod == 0 || fp->s_ronly != 0) {
		SLEEP_UNLOCK(&s5fsp->fs_sblock);
		return;
	}
	fp->s_fmod = 0;
	fp->s_time = hrestime.tv_sec;
	SLEEP_UNLOCK(&s5fsp->fs_sblock);
	vp = S5FS(vfsp)->fs_devvp;
	VN_HOLD(vp);
	bp = getblk(vfsp->vfs_dev, SUPERB, SBSIZE, BG_NOMISS);
	if (bp) {
		bp->b_flags |= B_STALE|B_AGE;
		brelse(bp);
	} 
	bp = bread(vfsp->vfs_dev, SUPERB, SBSIZE);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		goto out;
	} else {
		bcopy((caddr_t)fp, bp->b_un.b_addr, SBSIZE);
		bwrite(bp);
	}
out:
	VN_RELE(vp);
}

/*
 * int
 * s5_vget(vfs_t *vfsp, vnode_t **vpp, fid_t *fidp)
 *	Given a file identifier, return a vnode for
 *	the file possible.
 *
 * Calling/Exit State:
 *	The file system that we're going to retrieve the inode
 *	from is protected against unmount by getvfs() -- see vfs.c
 */
/* ARGSUSED */
STATIC int
s5_vget(vfs_t *vfsp, vnode_t **vpp, fid_t *fidp)
{
	ufid_t	*ufid;
	inode_t	*ip;

	/* LINTED pointer alignment*/
	ufid = (struct ufid *) fidp;
	if (s5_iget(vfsp, getfs(vfsp), ufid->ufid_ino, IG_NCREATE, &ip)) {
		*vpp = NULL;
		return 0;
	}
	if (ip->i_gen != ufid->ufid_gen) {
		VN_RELE(ITOV(ip));
		*vpp = NULL;
		return 0;
	}
	*vpp = ITOV(ip);
	return 0;
}

/*
 * int
 * s5_mountroot(vfs_t *vfsp, whymountroot_t why) 
 * 	Mount an s5 file system as root.
 *
 * Calling/Exit State:
 *	The global vfslist_lock is locked on entry and exit.
 *
 * Description:
 * 	"why" is ROOT_INIT on initial call, ROOT_REMOUNT if called to
 * 	remount the root file system, and ROOT_UNMOUNT if called to
 * 	unmount the root (e.g., as part of a system shutdown).
 */
/* ARGSUSED */
STATIC int
s5_mountroot(vfs_t *vfsp, whymountroot_t why)
{
	filsys_t	*fp;
	vnode_t		*vp;
	int		error;
	buf_t		*bp;
	cred_t		*cr;
	int ovflags = vfsp->vfs_flag;

	cr = u.u_lwpp->l_cred;
	switch (why) {
	case ROOT_INIT:
		if (rootdev == (dev_t)NODEV)
			return (ENODEV);
		vfsp->vfs_flag |= VFS_RDONLY;
		break;

	case ROOT_REMOUNT:
		vfsp->vfs_flag |= VFS_REMOUNT;
		vfsp->vfs_flag &= ~VFS_RDONLY;
		break;

	case ROOT_UNMOUNT:
		s5_update();
		fp = getfs(vfsp);
		if (fp->s_state == FsACTIVE) {
			fp->s_time = hrestime.tv_sec;
			fp->s_state = FsOKAY - (long)fp->s_time;
			vp = S5FS(vfsp)->fs_devvp;
			bp = getblk(vfsp->vfs_dev, SUPERB, SBSIZE, BG_NOMISS);
			if (bp) {
				bp->b_flags |= B_STALE|B_AGE;
				brelse(bp);
			}				
			bp = bread(vfsp->vfs_dev, SUPERB, SBSIZE);
			if (bp->b_flags & B_ERROR){ 
				brelse(bp);
				return (error=geterror(bp));
			}
			bcopy((caddr_t)fp, bp->b_un.b_addr, SBSIZE);
			bwrite(bp);
			(void) VOP_CLOSE(vp, FREAD|FWRITE, 1, (off_t)0, cr);
			VN_RELE(vp);
		}
		bdwait(NODEV);
		pgwait(NODEV);
		return 0;

	default:
		return EINVAL;
	}
	error = s5_mountfs(vfsp, why, rootdev, cr);	
	if (error) {
		vfsp->vfs_flag = ovflags;
		return (error);
	}

	/* The routine is called at system boot and at this time
	 * it still in UP state and no one has the access to the fs
	 * therefore no need to lock vfslist when adding the vfs.
	 */
	if (why == ROOT_INIT)
		vfs_add(NULLVP, vfsp,
			(vfsp->vfs_flag & VFS_RDONLY) ? MS_RDONLY : 0);
	else if (why == ROOT_REMOUNT)
                vfsp->vfs_flag &= ~VFS_REMOUNT;

	return (0);
}

/* 
 * int
 * s5_fs_init(s5_fs_t *fsp, int bsize);
 *	Initialize the s5vfs structure.
 *
 * Calling/Exit State:
 *	Should only be called when initializing s5.
 */
STATIC int
s5_fs_init(s5_fs_t *fsp, int bsize)
{
	int i;

	for (i = bsize, fsp->fs_bshift = 0; i > 1; i >>= 1)
		fsp->fs_bshift++;
	fsp->fs_nindir = bsize / sizeof(daddr_t);
	fsp->fs_inopb = bsize / sizeof(struct dinode);
	fsp->fs_bsize = bsize;
	fsp->fs_bmask = ~(bsize - 1);
	fsp->fs_nmask = fsp->fs_nindir - 1;
	for (i = bsize/512, fsp->fs_ltop = 0; i > 1; i >>= 1)
		fsp->fs_ltop++;
	for (i = fsp->fs_nindir, fsp->fs_nshift = 0; i > 1; i >>= 1)
		fsp->fs_nshift++;
	for (i = fsp->fs_inopb, fsp->fs_inoshift = 0; i > 1; i >>= 1)
		fsp->fs_inoshift++;
	return 0;
}
