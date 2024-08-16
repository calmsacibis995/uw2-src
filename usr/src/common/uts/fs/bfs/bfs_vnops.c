/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/bfs/bfs_vnops.c	1.14"
#ident	"$Header: $"

#include <acc/dac/acl.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/bfs/bfs.h>
#include <fs/dirent.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/fs_subr.h>
#include <fs/fs_hier.h>
#include <fs/pathname.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <svc/clock.h>
#include <util/bitmasks.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * BFS routines called only by BFS routines within this file.
 */
STATIC int bfs_extend(inode_t *, off_t, off_t, cred_t *);
STATIC int bfs_direnter(inode_t *, char *, vattr_t *, inode_t **, cred_t *);
STATIC int bfs_dirremove(inode_t *, off_t, char *, inode_t **, cred_t *);
STATIC int bfs_iremove(inode_t *, cred_t *);
STATIC int bfs_truncateup(inode_t *, off_t, cred_t *);
STATIC int bfs_truncatedown(inode_t *, off_t, cred_t *);
STATIC int bfs_filetoend(inode_t *, cred_t *);
STATIC int bfs_zerohole(struct bfs_vfs *, off_t, off_t, cred_t *);
STATIC void bfs_quickcompact(vfs_t *, daddr_t, cred_t *);

/*
 * Boot file system operations vector.
 */
STATIC int bfs_open(vnode_t **, int, cred_t *);
STATIC int bfs_close(vnode_t *, int, boolean_t, off_t, cred_t *);
STATIC int bfs_read(vnode_t *, struct uio *, int, cred_t *);
STATIC int bfs_write(vnode_t *, struct uio *, int, cred_t *);
STATIC int bfs_getattr(vnode_t *, vattr_t *, int, cred_t *);
STATIC int bfs_setattr(vnode_t *, vattr_t *, int, int, cred_t *);
STATIC int bfs_access(vnode_t *, int, int, cred_t *);
STATIC int bfs_lookup(vnode_t *, char *, vnode_t **, pathname_t *, int,
		      vnode_t *, cred_t *);
STATIC int bfs_create(vnode_t *, char *, vattr_t *, enum vcexcl,
			int, vnode_t **, cred_t *);
STATIC int bfs_remove(vnode_t *, char *, cred_t *);
STATIC int bfs_link(vnode_t *, vnode_t *, char *, cred_t *);
STATIC int bfs_rename(vnode_t *, char *, vnode_t *, char *, cred_t *);
STATIC int bfs_readdir(vnode_t *, struct uio *, cred_t *, int *);
STATIC int bfs_fsync(vnode_t *, cred_t *);
STATIC int bfs_fid(vnode_t *, fid_t **);
STATIC void bfs_inactive(vnode_t *, cred_t *);
STATIC int bfs_rwlock(vnode_t *, off_t, int, int, int);
STATIC void bfs_rwunlock(vnode_t *, off_t, int);
STATIC int bfs_seek(vnode_t *, off_t, off_t *);

vnodeops_t bfs_vnodeops = {
	bfs_open,
	bfs_close,
	bfs_read,
	bfs_write,
	(int (*)())fs_nosys,	/* ioctl */
	fs_setfl,
	bfs_getattr,
	bfs_setattr,
	bfs_access,
	bfs_lookup,
	bfs_create,
	bfs_remove,
	bfs_link,
	bfs_rename,
	(int (*)())fs_nosys,	/* mkdir */
	(int (*)())fs_nosys,	/* rmdir */
	bfs_readdir,
	(int (*)())fs_nosys,	/* symlink */
	(int (*)())fs_nosys,	/* readlink */
	bfs_fsync,
	bfs_inactive,
	(void (*)())fs_nosys,	/* release */
	bfs_fid,
	bfs_rwlock,
	bfs_rwunlock,
	bfs_seek,
	fs_cmp,
	(int (*)())fs_nosys,	/* lockf  */
	(int (*)())fs_nosys,	/* realvp */
	(int (*)())fs_nosys,	/* getpage */
	(int (*)())fs_nosys,	/* putpage */
	(int (*)())fs_nosys,	/* mmap */
	(int (*)())fs_nosys,	/* addmap */
	(int (*)())fs_nosys,	/* delmap */
	fs_poll,
	fs_pathconf,
	(int (*)())fs_nosys,	/* getacl */
	(int (*)())fs_nosys,	/* setacl */
	(int (*)())fs_nosys,	/* setlevel */
	(int (*)())fs_nosys,	/* getdvstat */
	(int (*)())fs_nosys,	/* setdvstat */
	(int (*)())fs_nosys,	/* makemld */
	(int (*)())fs_nosys,	/* testmld */
	(int (*)())fs_nosys,	/* stablestore */
	(int (*)())fs_nosys,	/* relstore */
	(int (*)())fs_nosys,	/* getpagelist */
	(int (*)())fs_nosys,	/* putpagelist */
	(int (*)())fs_nosys,	/* msgio */
	(int (*)())fs_nosys,	/* filler[4] ... */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys
};


/*
 * int
 * bfs_open(vnode_t **vpp, int flag, cred_t *cr)
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *
 */
/* ARGSUSED */
STATIC int
bfs_open(vnode_t **vpp, int flag, cred_t *cr)
{
	return 0;
}


/*
 * int
 * bfs_close(vnode_t *vp, int flag, boolean_t lastclose, off_t offset,
 *		 cred_t *cr)
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *
 */
/* ARGSUSED */
STATIC int
bfs_close(vnode_t *vp, int flag, boolean_t lastclose, off_t offset, cred_t *cr)
{
	return 0;
}


/*
 * int
 * bfs_read(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *fcr)
	Read data from an inode.
 *
 * Calling/Exit State:
 *	Callers of this VOP must hold the following locks:
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock in shared mode
 *	   (This is accomplished via VOP_RWLOCK)
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 */
/* ARGSUSED */
STATIC int
bfs_read(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *fcr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)vp->v_vfsp->vfs_data;
	cred_t		*cr = VCURRENTCRED(fcr);	/* refer to vnode.h */
	inode_t		*ip;
	dinode_t	*dip;
	off_t		offset;
	off_t		eoff;
	char		*buf;
	int		chunksize;
	int		error;
	int		buflen;

	ASSERT(KS_HOLD0LOCKS());

	if (uiop->uio_resid == 0)
		return 0;

	if (uiop->uio_offset < 0)
		return EINVAL;

	ip = VTOI(vp);
	dip = &ip->i_diskino;

	if (dip->d_sblock == 0)		/* This file is empty */
		return 0;

	offset = uiop->uio_offset + (dip->d_sblock * BFS_BSIZE);
	eoff = dip->d_eoffset + 1;

	chunksize = MIN(MAXBSIZE, uiop->uio_resid);
	buf = kmem_alloc(chunksize, KM_SLEEP);
	while (uiop->uio_resid && offset < eoff) {
		buflen = MIN(chunksize, uiop->uio_resid);

		/*
		 * Figure out how many bytes to read.
		 */
		if (offset + buflen > eoff)
			buflen = eoff - offset;

		error = vn_rdwr(UIO_READ, bp->bfs_devnode, buf, buflen,
		  offset, UIO_SYSSPACE, 0, 0, cr, (int *)0);
		if (error) {
			kmem_free(buf, chunksize);
			return error;
		}

		error = uiomove(buf, buflen, UIO_READ, uiop);
		if (error) {
			kmem_free(buf, chunksize);
			return error;
		}
		offset += buflen;
	}

	/*
	 * Update the access time of the file
	 */
	if (WRITEALLOWED(vp, cr)) {
		FSPIN_LOCK(&ip->i_mutex); 
		dip->d_fattr.va_atime = hrestime.tv_sec;
		FSPIN_UNLOCK(&ip->i_mutex); 
 			BFS_PUTINODE(bp->bfs_devnode, ip->i_inodoff, dip, cr);
	}
	kmem_free(buf, chunksize);
	return 0;
}


/*
 * int
 * bfs_write(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *fcr)
 *
 * Calling/Exit State:
 *	Callers of this VOP must hold the following locks:
 *	   - bfs_writelock
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock in exclusive mode.
 *	   (This is accomplished via VOP_RWLOCK)
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 */
STATIC int
bfs_write(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *fcr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)vp->v_vfsp->vfs_data;
	cred_t		*cr = VCURRENTCRED(fcr);
	inode_t		*ip;
	dinode_t	*dip;
	int		buflen;
	void		*buf;
	daddr_t		oeblock;
	off_t		save_eoffset;
	off_t		offset;
	off_t		endfile;
	off_t		newsize;
	off_t		extra;
	int		chunksize;
	int		error;

	if (uiop->uio_resid == 0)
		return 0;

	if (uiop->uio_offset < 0)
		return EINVAL;

	ip = VTOI(vp);
	dip = &ip->i_diskino;

	endfile = BFS_FILESIZE(dip);
	if (ioflag & IO_APPEND) {
		/*
		 * In append mode, start at end of file.
		 */
		uiop->uio_offset = endfile;
	}

	if (uiop->uio_offset + uiop->uio_resid > endfile) {
		/*
		 * We're extending the file, so we may need to allocate
		 * space and/or zero any hole we're skipping over.
		 */
		newsize = MAX(uiop->uio_offset, endfile);
		extra = (uiop->uio_offset + uiop->uio_resid) - newsize;
		if ((error = bfs_extend(ip, newsize, extra, cr)) != 0)
			return error;
		if (dip->d_sblock == 0)
			dip->d_sblock = bp->bfs_eblklastfile + 1;
	}

	offset = dip->d_sblock * BFS_BSIZE + uiop->uio_offset;

	FSPIN_LOCK(&ip->i_mutex); 
	dip->d_fattr.va_mtime = hrestime.tv_sec;
	dip->d_fattr.va_ctime = hrestime.tv_sec;
	FSPIN_UNLOCK(&ip->i_mutex); 

	chunksize = MIN(MAXBSIZE, uiop->uio_resid);
	buf = kmem_alloc(chunksize, KM_SLEEP);
	while (uiop->uio_resid) {
		buflen = MIN(chunksize, uiop->uio_resid);

		/*
		 * Get the user buffer into our space.
		 */
		if ((error = uiomove(buf, buflen, UIO_WRITE, uiop)) != 0) {
			kmem_free(buf, chunksize);
			return error;
		}

		/*
		 * Write to the device.
		 */
		error = vn_rdwr(UIO_WRITE, bp->bfs_devnode, buf, buflen,
		 offset, UIO_SYSSPACE, IO_SYNC, RLIM_INFINITY, cr, 0);
		if (error) {
			kmem_free(buf,chunksize);
			return error;
		}

		save_eoffset = dip->d_eoffset;
		oeblock = dip->d_eblock;

		if ((offset += buflen) <= dip->d_eoffset + 1)
			continue;

		/*
		 * Update the disk inode.
		 */
		dip->d_eoffset = offset - 1;
		dip->d_eblock = dip->d_eoffset / BFS_BSIZE;

		error = BFS_PUTINODE(bp->bfs_devnode, ip->i_inodoff,
				dip, cr);
		if (error) {
			/*
			 * If write of a new file failed when updating
			 * the disk inode the first time through this
			 * loop, then reset in-core inode copy to its
			 * original value.
			 */
			if ((dip->d_eoffset = save_eoffset) == 0)
				dip->d_sblock = 0;
			dip->d_eblock = oeblock;

			kmem_free(buf,chunksize);
			return error;
		}

		if (dip->d_eblock != oeblock) {
			FSPIN_LOCK(&bp->bfs_mutex); 
			if (oeblock == 0)
				bp->bfs_freeblocks -= BFS_FILEBLOCKS(dip);
			else
				bp->bfs_freeblocks -= dip->d_eblock - oeblock;
			FSPIN_UNLOCK(&bp->bfs_mutex); 
		}

		/*
		 * Must update the global variables bfs_lastfilefs,
		 * bfs_sblklastfile and bfs_eblklastfile each time the
		 * disk inode is successfully updated. This way, if a
		 * subsequent write of the inode to disk fails, this
		 * info will not be lost.
		 * NOTE: The update ONLY to be done iff this write is
		 * done to a file at the end of the FS. 
		 */
		if (dip->d_sblock >= bp->bfs_sblklastfile) {
			bp->bfs_lastfilefs = ip->i_inodoff;
			bp->bfs_sblklastfile = dip->d_sblock;
			bp->bfs_eblklastfile = dip->d_eblock;
		}
	}

	kmem_free(buf, chunksize);

	return 0;
}


/*
 * int
 * bfs_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
 * 	Return attrivutes for a vnode.
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *
 *	On exit, all locks acquired by this VOP will be released.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 * Description:
 *	This VOP holds the inode rwlock in shared mode for its entire duration
 *
 */
/* ARGSUSED */
STATIC int
bfs_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
{
	inode_t		*ip;
	dinode_t	*dip;

	ASSERT(KS_HOLD0LOCKS());

	ip = VTOI(vp);

	/*
	 * We get most of the file attributes from the inode.
	 */
	BFS_IRWLOCK_RDLOCK(ip);

	dip = &ip->i_diskino;
	vap->va_type = dip->d_fattr.va_type;
	vap->va_mode = dip->d_fattr.va_mode;
	vap->va_uid = dip->d_fattr.va_uid;
	vap->va_gid = dip->d_fattr.va_gid;
	vap->va_nlink = (nlink_t)dip->d_fattr.va_nlink;

	if (vap->va_mask & (AT_ATIME | AT_MTIME | AT_CTIME)) {
		FSPIN_LOCK(&ip->i_mutex); 
		vap->va_atime.tv_sec = dip->d_fattr.va_atime;
		vap->va_atime.tv_nsec = 0;
		vap->va_mtime.tv_sec = dip->d_fattr.va_mtime;
		vap->va_mtime.tv_nsec = 0;
		vap->va_ctime.tv_sec = dip->d_fattr.va_ctime;
		vap->va_ctime.tv_nsec = 0;
		FSPIN_UNLOCK(&ip->i_mutex);
	}

	vap->va_blksize = BFS_BSIZE;
	vap->va_vcode = 0;
	vap->va_size = BFS_FILESIZE(dip);
	vap->va_nblocks = btod(vap->va_size);
	vap->va_nodeid = dip->d_ino;
	vap->va_fsid = vp->v_vfsp->vfs_dev;
	vap->va_rdev = 0;
	if (vap->va_mask & AT_ACLCNT) {
		vap->va_aclcnt = NACLBASE;
	}

	BFS_IRWLOCK_UNLOCK(ip);
	return 0;
}


/*
 * int
 * bfs_setattr(vnode_t *vp, vattr_t *vap, int flags, int ioflags, cred_t *cr)
 *	Modify/Set inode attributes.
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *	On exit, all locks acquired by this VOP will be released.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 * Description:
 *	This VOP acquires the following locks:
 *	   - bfs_writelock if AT_SIZE is specified.
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock in exclusive mode.
 *
 */
STATIC int
bfs_setattr(vnode_t *vp, vattr_t *vap, int flags, int ioflags, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)vp->v_vfsp->vfs_data;
	inode_t		*ip;
	dinode_t	*dip;
	struct bfsvattr *battrs;
	int		error = 0;
	int		new_error;
	long		filesize;
	long		mask = vap->va_mask;

	ASSERT(KS_HOLD0LOCKS());

	if (mask & AT_NOSET) {
		/* Cannot set these attributes. */
		return EINVAL;
	}

	if (mask & AT_SIZE) {
		/* Truncate a file. Must not be a directory. */
		if (vp->v_type == VDIR)
			return EISDIR;

		SLEEP_LOCK(&bp->bfs_writelock, PRIVFS);
	}
	
	RWSLEEP_RDLOCK(&bp->bfs_fs_rwlock, PRIVFS);

	ip = VTOI(vp);

	BFS_IRWLOCK_WRLOCK(ip);

	if (mask & AT_SIZE) {
		/* Truncate file.  Must have write permission */
		if ((error = bfs_iaccess(ip, VWRITE, cr)) != 0)
			goto out2;
	}

	dip = &ip->i_diskino;
	battrs = &dip->d_fattr;

	/*
	 * Change file access modes.  Must be owner or privileged.
	 */
	if (mask & AT_MODE) {
		if (cr->cr_uid != battrs->va_uid && pm_denied(cr, P_OWNER)) {
			error = EPERM;
			goto out2;
		}

		battrs->va_mode = vap->va_mode;

		/*
		 * A non-privileged user can set the sticky bit
		 * on a directory.
		 */
		if (vp->v_type != VDIR)
			if ((battrs->va_mode & VSVTX) && pm_denied(cr, P_OWNER))
				battrs->va_mode &= ~VSVTX;
		if (!groupmember(battrs->va_gid, cr) && pm_denied(cr, P_OWNER))
			battrs->va_mode &= ~VSGID;
	}
	/*
	 * Change file ownership; must be the owner of the file
	 * or privileged.  If the system was configured with
	 * the "rstchown" option, the owner is not permitted to
	 * give away the file, and can change the group id only
	 * to a group of which he or she is a member.
	 */
	if (mask & (AT_UID|AT_GID)) {
		boolean_t	checksu = B_FALSE;

		if (rstchown) {
		       if (((mask & AT_UID) && vap->va_uid != battrs->va_uid) ||
			   ((mask & AT_GID) && !groupmember(vap->va_gid, cr)))
				checksu = B_TRUE;
		} else if (cr->cr_uid != battrs->va_uid)
			checksu = B_TRUE;

		if (checksu && pm_denied(cr, P_OWNER)) {
			error = EPERM;
			goto out;
		}

		if ((battrs->va_mode & (VSUID|VSGID)) && pm_denied(cr, P_OWNER))
			battrs->va_mode &= ~(VSUID|VSGID);
		if (mask & AT_UID)
			battrs->va_uid = vap->va_uid;
		if (mask & AT_GID)
			battrs->va_gid = vap->va_gid;
	}

	/*
	 * Change file access or modified times.
	 */
	if (mask & (AT_ATIME|AT_MTIME)) {
		if (cr->cr_uid != battrs->va_uid && pm_denied(cr, P_OWNER)) {
			if (flags & ATTR_UTIME)
				error = EPERM;
			else
				error = bfs_iaccess(ip, VWRITE, cr);
			if (error)
				goto out;
		}

		FSPIN_LOCK(&ip->i_mutex); 
		if (mask & AT_ATIME)
			battrs->va_atime = vap->va_atime.tv_sec;
		if (mask & AT_MTIME) {
			battrs->va_mtime = vap->va_mtime.tv_sec;
			battrs->va_ctime = hrestime.tv_sec;
		}
		FSPIN_UNLOCK(&ip->i_mutex); 
	}

	/*
	 * Truncate file.  Must have write permission and not be a directory.
	 * 
	 */
	if (mask & AT_SIZE) {
		filesize = BFS_FILESIZE(dip);
		if (vap->va_size < filesize) {
			error = bfs_truncatedown(ip, vap->va_size, cr);
			if (error)
				goto out;
		}

		else if (vap->va_size > filesize) {
			error = bfs_truncateup(ip, vap->va_size, cr);
			if (error)
				goto out;
		}

	}
out:
	new_error = BFS_PUTINODE(bp->bfs_devnode, ip->i_inodoff, dip, cr);
	if (!error) 
		error = new_error;
out2:
	BFS_IRWLOCK_UNLOCK(ip);
	RWSLEEP_UNLOCK(&bp->bfs_fs_rwlock);
	if (mask & AT_SIZE)
		SLEEP_UNLOCK(&bp->bfs_writelock);
	return error;
}


/*
 * int
 * bfs_access(vnode_t *vp, int mode, int flags, cred_t *cr)
 *	Determine the accessibility of a file to the calling proccess
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *	On exit, the lock acquired by this VOP will be released.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 * Description:
 *	This VOP holds the inode rwlock in shared mode for its entire duration
 *
 */
/* ARGSUSED */
STATIC int
bfs_access(vnode_t *vp, int mode, int flags, cred_t *cr)
{
	inode_t *ip;
	int	error;

	ASSERT(KS_HOLD0LOCKS());

	ip = VTOI(vp);
	BFS_IRWLOCK_RDLOCK(ip);
	error = bfs_iaccess(ip, mode, cr);
	BFS_IRWLOCK_UNLOCK(ip);
	return error;
}


/*
 * int
 * bfs_lookup(vnode_t *dvp, char *nm, vnode_t **vpp, pathname_t *pnp,
 *	      int lookup_flags, vnode_t *rootvp, cred_t *cr);
 *	Check whether the directory (root) contains a file named <nm>.
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *	On exit, all locks acquired by this VOP will be released.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 * Description:
 *	This VOP holds the following locks for its entire duration:
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock of the root directory in shared mode
 *
 */
/* ARGSUSED */
STATIC int
bfs_lookup(vnode_t *dvp, char *nm, vnode_t **vpp,  pathname_t *pnp,
	   int lookup_flags, vnode_t *rootvp, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)dvp->v_vfsp->vfs_data;
	inode_t		*ip;
	inode_t		*dp;
	off_t		offset;
	int		error;

	ASSERT(KS_HOLD0LOCKS());

	if (dvp->v_type != VDIR)	/* We can only read root */
		return ENOTDIR;

	RWSLEEP_RDLOCK(&bp->bfs_fs_rwlock, PRIVFS);

	dp = VTOI(dvp);

	BFS_IRWLOCK_RDLOCK(dp);

	if ((error = bfs_iaccess(dp, VEXEC, cr)) != 0)
		goto out;

	/*
	 * The null name means the current directory.
	 */
	if (*nm == '\0') {
		*vpp = dvp;
		VN_HOLD(dvp);
		error = 0;
		goto out;
	}

	/*
	 * Handle '..' even though it doesn't exist.
	 */
	if (nm[0] == '.' && nm[1] == '.' && nm[2] == '\0') {
		*vpp = bp->bfs_rootvnode;
		VN_HOLD(*vpp);
		error = 0;
		goto out;
	}

	/*
	 * Search through the flat directory for the file.
	 */
	if ((offset = bfs_searchdir(dp, nm, cr)) == 0) {
		error = ENOENT;
		goto out;
	}

	if ((error = bfs_iget(dvp->v_vfsp, BFS_OFF2INO(offset),
			&ip, B_FALSE, cr)) != 0)
		goto out;

	*vpp = ITOV(ip);

	BFS_IRWLOCK_UNLOCK(ip);

out:
	BFS_IRWLOCK_UNLOCK(dp);
	RWSLEEP_UNLOCK(&bp->bfs_fs_rwlock);
	return error;
}


/*
 * int
 * bfs_create(vnode_t *dvp, char *fname, vattr_t *vap, enum vcexcl excl,
 *	Create a file in the root directory. Root is the only directory in BFS.
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *	On exit, all locks acquired by this VOP will be released.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 * Description:
 *	This VOP acquires the following locks:
 *	   - bfs_writelock if AT_SIZE is specified.
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock of the root directory in exclusive mode
 *	   - inode rwlock of file to be created via bfs_iget()
 *
 */
STATIC int
bfs_create(vnode_t *dvp, char *fname, vattr_t *vap, enum vcexcl excl,
		int mode, vnode_t **vpp, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)dvp->v_vfsp->vfs_data;
	inode_t		*rip;
	inode_t		*ip;
	dinode_t	*dip;
	off_t		offset;
	int		error = 0;
	off_t		filesize;

	ASSERT(KS_HOLD0LOCKS());

	if (*fname == '\0')
		/*
		 * Null component name refers to the directory itself.
		 */
		return EISDIR;

	/*
	 * Can only create into root.
	 */
	if (dvp->v_type != VDIR)
		return ENOTDIR;

	if (vap->va_type != VREG)
		return ENOSYS;

	if (vap->va_mask & AT_SIZE)
		SLEEP_LOCK(&bp->bfs_writelock, PRIVFS);
	
	RWSLEEP_RDLOCK(&bp->bfs_fs_rwlock, PRIVFS);

	rip = VTOI(dvp);
	BFS_IRWLOCK_WRLOCK(rip);

	offset = bfs_searchdir(rip, fname, cr);
	if (offset == 0) {
		error = bfs_direnter(rip, fname, vap, &ip, cr);
		if (!error)
			*vpp = ITOV(ip);
		goto out;

	} else if (excl == NONEXCL) {
		error = bfs_iget(dvp->v_vfsp, BFS_OFF2INO(offset),
			 	 &ip, B_TRUE, cr);
		if (error)
			goto out;
		/*
		 * If user has access, truncate if requested
		 */
		if (mode && (error = bfs_iaccess(ip, mode, cr)) == 0) {
			dip = &ip->i_diskino;
			filesize = BFS_FILESIZE(dip);

			if (vap->va_mask & AT_SIZE) {
				if (vap->va_size < filesize)
					error =
					 bfs_truncatedown(ip, vap->va_size, cr);
				else if (vap->va_size > filesize)
					error =
					   bfs_truncateup(ip, vap->va_size, cr);
			}
			BFS_IRWLOCK_UNLOCK(ip);
			if (error) {
				VN_RELE(ITOV(ip));
				goto out;
			}
		
			*vpp = ITOV(ip);
		} else {
			BFS_IRWLOCK_UNLOCK(ip);
			VN_RELE(ITOV(ip));
			goto out;
		}
	} else 
		error = EEXIST;
out:
	BFS_IRWLOCK_UNLOCK(rip);
	RWSLEEP_UNLOCK(&bp->bfs_fs_rwlock);
	if (vap->va_mask & AT_SIZE)
		SLEEP_UNLOCK(&bp->bfs_writelock);
	return error;
}


/*
 * int
 * bfs_remove(vnode_t *dvp, char *nm, cred_t *cr)
 *	Remove a regular file from the root directory.
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *	On exit, all locks acquired by this VOP will be released.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 * Description:
 *	This VOP acquires the following locks:
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock of the root directory in exclusive mode.
 *
 */
STATIC int
bfs_remove(vnode_t *dvp, char *nm, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)dvp->v_vfsp->vfs_data;
	inode_t		*dp;
	inode_t		*ip;
	off_t		offset;
	int		error;

	ASSERT(KS_HOLD0LOCKS());

	RWSLEEP_RDLOCK(&bp->bfs_fs_rwlock, PRIVFS);

	dp = VTOI(dvp);

	BFS_IRWLOCK_WRLOCK(dp);

	/*
	 * Since BFS is a flat file system, only need to check permissions
	 * of the ROOT directory.
	 */
	if ((error = bfs_iaccess(dp, VEXEC|VWRITE, cr)) != 0)
		goto out;

	/*
	 * Search through the flat directory for the file.
	 */
	if ((offset = bfs_searchdir(dp, nm, cr)) == 0){
		error = ENOENT;
		goto out;
	}

	error = bfs_dirremove(dp, offset, nm, &ip, cr);
out:
	BFS_IRWLOCK_UNLOCK(dp);
	RWSLEEP_UNLOCK(&bp->bfs_fs_rwlock);
	if (!error) {
		BFS_IRWLOCK_UNLOCK(ip);
		VN_RELE(ITOV(ip));
	}
	return error;
}


/*
 * int
 * bfs_link(vnode_t *tdvp, vnode_t *svp, char *tnm, cred_t *cr)
 *	Link a regular file. 
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *	On exit, all locks acquired by this VOP will be released.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 * Description:
 *	This VOP holds the following locks for its entire duration:
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock of the root directory in exclusive mode
 *	   - inode rwlock of the source file in exclusive mode
 *
 */
STATIC int
bfs_link(vnode_t *tdvp, vnode_t *svp, char *tnm, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)tdvp->v_vfsp->vfs_data;
	dinode_t	*dip;
	inode_t		*tdip;
	inode_t		*sip;
	bfs_ino_t	sino;
	int		error;

	ASSERT(KS_HOLD0LOCKS());

	if (tdvp->v_type != VDIR) 
		return ENOTDIR;

	if (svp->v_type != VREG) 
		return ENOSYS;

	RWSLEEP_RDLOCK(&bp->bfs_fs_rwlock, PRIVFS);

	tdip = VTOI(tdvp);
	BFS_IRWLOCK_WRLOCK(tdip);

	if ((error = bfs_iaccess(tdip, VWRITE, cr)) != 0) {
		BFS_IRWLOCK_UNLOCK(tdip);
		RWSLEEP_UNLOCK(&bp->bfs_fs_rwlock);
		return error;
	}

	sip = VTOI(svp);
	BFS_IRWLOCK_WRLOCK(sip);

	dip = &sip->i_diskino;
	sino = dip->d_ino;

	if ((error = bfs_addirent(tdip, tnm, sino, cr)) != 0) {
		BFS_IRWLOCK_UNLOCK(tdip);
		BFS_IRWLOCK_UNLOCK(sip);
		RWSLEEP_UNLOCK(&bp->bfs_fs_rwlock);
		return error;
	}

	/*
	 * Update the source inode 
	 */
	dip->d_fattr.va_nlink++;

	FSPIN_LOCK(&sip->i_mutex); 
	dip->d_fattr.va_ctime = hrestime.tv_sec;
	FSPIN_UNLOCK(&sip->i_mutex);

	error = BFS_PUTINODE(bp->bfs_devnode, sip->i_inodoff, dip, cr);

	BFS_IRWLOCK_UNLOCK(tdip);
	BFS_IRWLOCK_UNLOCK(sip);
	RWSLEEP_UNLOCK(&bp->bfs_fs_rwlock);
	return error;
}


/*
 *
 * int
 * bfs_rename(vnode_t *sdvp, char *snm, vnode_t *tdvp, char *tnm, cred_t *cr)
 *	Rename a regular file.
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *	On exit, all locks acquired by this VOP will be released.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 * Description:
 *	This VOP acquires the following locks:
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock of the root directory in exclusive mode
 *	   - inode rwlock of the source file (via bfs_iget())
 *
 */
STATIC int
bfs_rename(vnode_t *sdvp, char *snm, vnode_t *tdvp, char *tnm, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)sdvp->v_vfsp->vfs_data;
	inode_t		*sdp;
	inode_t		*sip;
	inode_t		*tip;
	off_t		soffset, toffset = 0;
	int		error, newerror;

	ASSERT(KS_HOLD0LOCKS());

	/*
	 * Make sure we can delete the source entry.
	 * Source and target directory must be same.
	 */
	if (sdvp != tdvp)
		return EINVAL;

	RWSLEEP_RDLOCK(&bp->bfs_fs_rwlock, PRIVFS);

	sdp = VTOI(sdvp);

	BFS_IRWLOCK_WRLOCK(sdp);

	if ((error = bfs_iaccess(sdp, VWRITE, cr)) != 0)
		goto out;

	/*
	 * Check for renaming '.' or '..'.
	 */
	if (strcmp(snm, ".") == 0 || strcmp(snm, "..") == 0) {
		error = EINVAL;
		goto out;
	}

	/*
	 * search for the source file in the root directory.
	 * If the file does not exist, stop here.
	 */
	if ((soffset = bfs_searchdir(sdp, snm, cr)) <= 0) {
		error = ENOENT;
		goto out;
	}

	/*
	 * search for the target file name in the root directory.
	 * If the file exists, remove it from the directory and decrement 
	 * the inode count.
	 */
	if ((toffset = bfs_searchdir(sdp, tnm, cr)) != 0) {

		if ((error = bfs_dirremove(sdp, toffset, tnm, &tip, cr)) != 0)
			goto out;
	}

	if ((newerror = bfs_iget(sdvp->v_vfsp, BFS_OFF2INO(soffset),
				 &sip, B_TRUE, cr)) != 0)
		goto out;

	newerror = bfs_rendirent(sdp, snm, tnm, cr);

	BFS_IRWLOCK_UNLOCK(sip);
	VN_RELE(ITOV(sip));
out:
	BFS_IRWLOCK_UNLOCK(sdp);
	RWSLEEP_UNLOCK(&bp->bfs_fs_rwlock);
	if (!error && toffset != 0) {
		error = newerror;
		BFS_IRWLOCK_UNLOCK(tip);
		VN_RELE(ITOV(tip));
	}
	return error;
}


#define RECBUFSIZE	1048

/*
 * int
 * bfs_readdir(vnode_t *vp, struct uio *uiop, cred_t *fcr, int *eofp)
 *	Read from the root directory.
 *
 * Calling/Exit State:
 *	Callers of this VOP must hold the following locks:
 *	- bfs_fs_rwlock in shared mode
 *	- inode rwlock in shared mode
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 */
STATIC int
bfs_readdir(vnode_t *vp, struct uio *uiop, cred_t *fcr, int *eofp)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)vp->v_vfsp->vfs_data;
	cred_t		*cr = VCURRENTCRED(fcr);
	struct bfs_ldirs	*ld;
	off_t		diroff = uiop->uio_offset;
	inode_t		*ip;
	dinode_t	*dip;
	struct dirent	*drp;
	void		*drent;
	void		*buf;
	off_t		offset;
	off_t		i;
	int		error = 0;
	int		buflen;
	int		chunksize;
	int		reclen, namesz;
	int		indrent = 0;
	int		fixsz;
	long		len;

	ASSERT(KS_HOLD0LOCKS());

	/*
	 * Check for a valid offset into the "directory".
	 */
	if (diroff % sizeof(struct bfs_ldirs) != 0)
		return ENOENT;

	if (diroff < 0)
		return EINVAL;
	
	ip = VTOI(vp);
	dip = &ip->i_diskino;
	len = BFS_FILESIZE(dip) - diroff;
	if (len < 0) {
		return ENOENT;
	}
	chunksize = MIN(len, DIRBUFSIZE);
	buf = kmem_alloc(chunksize, KM_SLEEP);

	drent = kmem_alloc(RECBUFSIZE, KM_SLEEP);
	drp = (struct dirent *)drent;
	fixsz = (char *)drp->d_name - (char *)drp;

	for (offset = diroff + (dip->d_sblock * BFS_BSIZE);
	     uiop->uio_resid > 0 && len > 0; len -= buflen, offset += buflen) {	

		buflen = MIN(chunksize, len);
		/*
		 * Get the list of entries stored in the ROOT directory
		 */
		error = BFS_GETDIRLIST(bp->bfs_devnode, offset, buf, buflen,cr);
		if (error) {
			kmem_free(buf, chunksize);
			kmem_free(drent, RECBUFSIZE);
			return error;
		}
		ld = (struct bfs_ldirs *)buf;
		for (i = 0; i < buflen && uiop->uio_resid >= indrent;
		     i += sizeof(struct bfs_ldirs), ld++) {
			if (ld->l_ino == 0)
				continue;
			namesz = (ld->l_name[BFS_MAXFNLEN - 1] == '\0') ?
				 strlen(ld->l_name) : BFS_MAXFNLEN;
			reclen = (fixsz + namesz + 1 + (NBPW - 1)) & ~(NBPW -1);

			if (RECBUFSIZE - indrent < reclen || 
			    uiop->uio_resid < indrent + reclen) {
				uiomove(drent, indrent, UIO_READ, uiop);
				if (uiop->uio_resid < reclen) {
					offset += i;
					indrent = -1;
					break;
				}
				drp = (struct dirent *) drent;
				indrent = 0;
			}
			if (uiop->uio_resid >= indrent + reclen) {
				drp->d_ino = ld->l_ino;
				drp->d_off = offset + i +
				   sizeof(struct bfs_ldirs) -
				   (dip->d_sblock * BFS_BSIZE);
				drp->d_reclen = (short)reclen;
				strncpy(drp->d_name, ld->l_name, BFS_MAXFNLEN);
				drp->d_name[namesz] = '\0';
				drp = (struct dirent *)
					(void *)((char *)drp + reclen);
				indrent += reclen;
			}
		}
		if (indrent == - 1)
			break;
	}

	if (indrent > 0 && uiop->uio_resid >= indrent)
		uiomove(drent, indrent, UIO_READ, uiop);

	/*
	 * The offset must be changed manually to reflect the filesystem
	 * DEPENDENT directory size.
	 */
	if (error == 0) {
		uiop->uio_offset = offset - (dip->d_sblock * BFS_BSIZE);
		if (eofp)
			*eofp = (offset >= dip->d_eoffset);
	}

	kmem_free(buf, chunksize);
	kmem_free(drent, RECBUFSIZE);
	return error;
}


/*
 * int
 * bfs_fsync(vnode_t *vp, cred_t *cr)
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *
 */
/* ARGSUSED */
STATIC int
bfs_fsync(vnode_t *vp, cred_t *cr)
{
	return 0;
}


/*
 * void
 * bfs_inactive(vnode_t *vp, cred_t *cr)
 *	Perform cleanup on an unreferenced inode.
 *
 * Calling/Exit State:
 *	On exit all locks acquired by this VOP will be released.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 * Description:
 *	If this vnode has no more references, remove it from the inode list
 *	and free up the space.
 *
 *	If the vnode has no more references and the inode count is 0, this
 *	VOP will acquire the following locks before calling bfs_iremove():
 *		- bfs_writelock
 *		- bfs_fs_rdwrlock in shared mode
 */
/* ARGSUSED */
STATIC void
bfs_inactive(vnode_t *vp, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)vp->v_vfsp->vfs_data;
	inode_t		*ip, *pip, *np;

	ASSERT(KS_HOLD0LOCKS());

	ip = VTOI(vp);
	BFS_IRWLOCK_WRLOCK(ip);

	VN_LOCK(vp);
	if (vp->v_count != 1) {
		vp->v_count--;
		VN_UNLOCK(vp);
		BFS_IRWLOCK_UNLOCK(ip);
		return;
	}
	else {
		vp->v_count = 0;
	}
	VN_UNLOCK(vp);
	ASSERT(vp->v_count == 0);

	(void)LOCK(&bp->bfs_inolist_mutex, FS_BFSLISTPL);

	if (BFS_IRWLOCK_LOCKBLKD(ip)) {
		UNLOCK(&bp->bfs_inolist_mutex, PLBASE);
		VN_LOCK(vp);
		VN_UNLOCK(vp);
		BFS_IRWLOCK_UNLOCK(ip);
		return;
	}

	/*
	 * Delete the inode from the inode list.
	 */
	bfs_searchlist(bp, ip->i_diskino.d_ino, &np, &pip);
	ASSERT(ip == np);
	if (ip != pip) {
		ASSERT(pip->i_next == ip);
		pip->i_next = ip->i_next;
	}
	else
		bp->bfs_inolist = NULL;

	UNLOCK(&bp->bfs_inolist_mutex, PLBASE);

	BFS_IRWLOCK_UNLOCK(ip);

	if (ip->i_diskino.d_fattr.va_nlink == 0) {
		SLEEP_LOCK(&bp->bfs_writelock, PRIVFS);
	
		RWSLEEP_RDLOCK(&bp->bfs_fs_rwlock, PRIVFS);

		bfs_iremove(ip, cr);

		RWSLEEP_UNLOCK(&bp->bfs_fs_rwlock);

		SLEEP_UNLOCK(&bp->bfs_writelock);
	}
	/*
	 * Free up any buffer associated with the inode and vnode locks.
	 */
	BFS_DEINIT_INODE(ip);

	kmem_free(ip, sizeof(inode_t));

	return;
}


/*
 * int
 * bfs_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
 *	Obtains the inode's rwlock and other BFS locks according to <mode>.
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *	On exit, all locks acquired by this VOP will be held.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 * Description:
 *	This VOP acquires the following locks:
 *	   - bfs_writelock if mode is set ot LOCK_EXCL
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock in exclusive mode if mode is set ot LOCK_EXCL
 *	     otherwise in shared mode.
 *
 */
/* ARGSUSED */
STATIC int
bfs_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)vp->v_vfsp->vfs_data;
	inode_t	*ip;

	ASSERT(KS_HOLD0LOCKS());

	ip = VTOI(vp);

	ASSERT(mode == LOCK_EXCL || mode == LOCK_SHARED);
	if (mode == LOCK_EXCL) {
		SLEEP_LOCK(&bp->bfs_writelock, PRIVFS);
	
		RWSLEEP_RDLOCK(&bp->bfs_fs_rwlock, PRIVFS);

		BFS_IRWLOCK_WRLOCK(ip);

		/*
		 * Must set this global variable to remember that we
		 * have locked the inode for writing and therefore also
		 * are holding the bfs_writelock. This is done so that
		 * when the upper layer calls bfs_rwunlock(), we can
		 * determine if the bfs_writelock needs to be unlocked.
		 */
		bp->bfs_inowlocked = ip;

	} else { /* if mode == LOCK_SHARED */
		RWSLEEP_RDLOCK(&bp->bfs_fs_rwlock, PRIVFS);

		BFS_IRWLOCK_RDLOCK(ip);
	}

	return 0;
}


/*
 * int
 * bfs_rwunlock(vnode_t *vp, off_t off, int len)
 *	Releases the inode's rwlock and other BFS locks.
 *
 * Calling/Exit State:
 *	This VOP releases all lockes acquired by bfs_rwlock()
 *
 */
/* ARGSUSED */
STATIC void
bfs_rwunlock(vnode_t *vp, off_t off, int len)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)vp->v_vfsp->vfs_data;
	inode_t		*ip;

	ip = VTOI(vp);

	BFS_IRWLOCK_UNLOCK(ip);

	RWSLEEP_UNLOCK(&bp->bfs_fs_rwlock);

	/*
	 * If the inode had been locked for writing, then we are also
	 * holding the bfs_writelock and must release it.
	 */
	if (bp->bfs_inowlocked == ip) {
		bp->bfs_inowlocked = NULL;
		SLEEP_UNLOCK(&bp->bfs_writelock);
	}

	return;
}


/*
 * int
 * bfs_fid(vnode_t *vp, fid_t **fidpp)
 *	Returns a unique identifier for the given file.
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *
 *	It is the caller's responsibility to free the allocated space.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 * Description:
 *	Convert a vnode to a BFS file id.  In BFS a fid is just the inode
 *	disk offset.
 */
STATIC int
bfs_fid(vnode_t *vp, fid_t **fidpp)
{
	struct bfs_fid_overlay	*overlay;
	inode_t		*ip;

	ASSERT(KS_HOLD0LOCKS());

	overlay = (struct bfs_fid_overlay *)
	  kmem_alloc(sizeof(struct bfs_fid_overlay), KM_SLEEP);
	overlay->o_len = sizeof(struct bfs_fid_overlay) - sizeof(ushort);
	ip = VTOI(vp);
	overlay->o_offset = ip->i_inodoff;

	*fidpp = (struct fid *)overlay;
	return 0;
}


/*
 * int
 * bfs_seek(vnode_t *vp, off_t ooff, off_t *noffp)
 *
 * Calling/Exit State:
 *	No locks are required on entry.
 *
 */
/* ARGSUSED */
STATIC int
bfs_seek(vnode_t *vp, off_t ooff, off_t *noffp)
{
	return *noffp < 0 ? EINVAL : 0;
}


/*
 *
 * int
 * bfs_extend(inode_t *ip, off_t newsize, off_t extra, cred_t *cr)
 *
 * Calling/Exit State:
 *	On entry the following locks must be held:
 *	   - bfs_writelock
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock in exclusive mode.
 *
 *	If compaction is needed, this routine will:
 *	   - release the bfs_fs_rwlock
 *	   - release the inode rwlock.
 *	   - acquire the bfs_fs_rwlock in exclusive mode.
 *	   - call the compaction routine.
 *	   - release the bfs_fs_rwlock.
 *	   - acquire the bfs_fs_rwlock in shared mode
 *	   - acquire the inode rwlock in exclusive mode.
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 */
STATIC int
bfs_extend(inode_t *ip, off_t newsize, off_t extra, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)ITOV(ip)->v_vfsp->vfs_data;
	dinode_t	*dip = &ip->i_diskino;
	daddr_t		spaceateof, spaceneeded;
	daddr_t		oeblock;
	off_t		save_eoffset;
	off_t		newoffset;
	int		error;

	ASSERT(KS_HOLD0LOCKS());

	save_eoffset = dip->d_eoffset;

	/* Handle extension within current EOF block first */
	if (dip->d_sblock != 0) {
		ASSERT(newsize >= BFS_NZFILESIZE(dip));
		if (newsize > BFS_NZFILESIZE(dip)) {
			off_t	incr = newsize - BFS_NZFILESIZE(dip);
			newoffset = roundup(dip->d_eoffset + 1, BFS_BSIZE) - 1;
			if (newoffset - dip->d_eoffset > incr)
				newoffset = dip->d_eoffset + incr;

			if ((error = bfs_zerohole(bp, dip->d_eoffset, newoffset,
						 cr)) != 0)
				return error;

			dip->d_eoffset = newoffset;

			if ((error = BFS_PUTINODE(bp->bfs_devnode,
						ip->i_inodoff, dip, cr)) != 0) {
				dip->d_eoffset = save_eoffset;
				return error;
			}
			save_eoffset = dip->d_eoffset;
		}
	}

	spaceneeded = ((newsize + extra + BFS_BSIZE - 1) / BFS_BSIZE) -
			BFS_FILEBLOCKS(dip);

	if (spaceneeded == 0)
		return 0;

	if (spaceneeded > bp->bfs_freeblocks)
		return ENOSPC;

	spaceateof = ((bp->bfs_endfs +1) / BFS_BSIZE) - 
		      (bp->bfs_eblklastfile + 1);

	/* If file is in the middle, move it to the end */
	if (bp->bfs_lastfilefs != ip->i_inodoff && dip->d_sblock != 0) {

		if (bp->bfs_freeblocks < BFS_FILEBLOCKS(dip))
			return ENOSPC;

		if (spaceateof < BFS_FILEBLOCKS(dip))
			BFS_COMPACT(bp, ip,cr);

		if ((error = bfs_filetoend(ip, cr)) != 0 )
			return error;
		/*
		 * reset "save_eoffset", since the inode information could have
		 * been changed and writen out to disk by bfs_compact() and
		 * definitely changed and writen out to disk by bfs_filetoend().
		 */
		save_eoffset = dip->d_eoffset;

		spaceateof = ((bp->bfs_endfs + 1) / BFS_BSIZE) -
			      (bp->bfs_eblklastfile + 1);
	}

	/*
	 * If not enough space at end of the filesystem, compact.
	 * After this there must be enough space, since we checked
	 * the total space above.
	 */
	if (spaceateof < spaceneeded) {
		BFS_COMPACT(bp, ip, cr);
		/*
		 * reset "save_eoffset", since the inode information could have
		 * been changed and writen out to disk by bfs_compact().
		 */
		save_eoffset = dip->d_eoffset;
	}

	/*
	 * If we are allocating space for a currently empty file, start
	 * filling in the inode.
	 */
	if (dip->d_sblock == 0) {
		if (newsize == 0)
			return 0;
		dip->d_sblock = bp->bfs_eblklastfile + 1;
		dip->d_eoffset = (dip->d_sblock * BFS_BSIZE) - 1; 
		oeblock = dip->d_sblock - 1;
	} else
		oeblock = dip->d_eblock;

	newoffset = (dip->d_sblock * BFS_BSIZE) + newsize - 1;
	if (newoffset > dip->d_eoffset) {
		error = bfs_zerohole(bp, dip->d_eoffset, newoffset, cr);
		if (error != 0) {
			/* restore inode values */
			if ((dip->d_eoffset = save_eoffset) == 0)
				dip->d_sblock = 0;
			return error;
		}
		/* update the inode */
		dip->d_eoffset = newoffset;
		dip->d_eblock = newoffset / BFS_BSIZE;
		error = BFS_PUTINODE(bp->bfs_devnode, ip->i_inodoff, dip, cr);
		if (error != 0) {
			/* restore inode values */
			if ((dip->d_eoffset = save_eoffset) == 0)
				dip->d_sblock = 0;
			dip->d_eblock = save_eoffset / BFS_BSIZE;
			return error;
		}

		/* Update "lastfile information */
		bp->bfs_lastfilefs = ip->i_inodoff;
		bp->bfs_sblklastfile = dip->d_sblock;
		bp->bfs_eblklastfile = dip->d_eblock;

		FSPIN_LOCK(&bp->bfs_mutex);
		bp->bfs_freeblocks -= dip->d_eblock - oeblock;
		FSPIN_UNLOCK(&bp->bfs_mutex);
	}
	return 0;
}


/*
 * int
 * bfs_direnter(inode_t *dp, char *fname, vattr_t *vap, inode_t **ipp,
 *		cred_t *cr)
 *
 * Calling/Exit State:
 *	The caller of this routine must hold the following locks:
 *	   - bfs_writelock
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock of the root directory in exclusive mode.
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 */
STATIC int
bfs_direnter(inode_t *dp, char *fname, vattr_t *vap, inode_t **ipp, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)dp->i_vnode.v_vfsp->vfs_data;
	inode_t		*ip;
	dinode_t	diskino, *dip;
	bfs_ino_t	ino;
	uint_t		nwords;
	int		error;
	
	ASSERT(KS_HOLD0LOCKS());

	/*
	 * Must have write permissions in this directory
	 */
	if ((error = bfs_iaccess(dp, VWRITE, cr)) != 0)
		return error;

	if (bp->bfs_freeinodes == 0)
		return ENFILE;

	/*
	 * Find the first available inode in the inobitmap
	 */
	nwords = BITMASK_NWORDS(bp->bfs_totalinodes + BFSROOTINO);
	ino = BITMASKN_FFC(bp->bfs_inobitmap, nwords);
	ASSERT(ino > BFSROOTINO);

	/*
	 * Initialize the disk inode.
	 */
	dip = &diskino;
	if ((error = BFS_GETINODE(bp->bfs_devnode, BFS_INO2OFF(ino),
			dip, cr)) != 0)
		return error;
	ASSERT(dip->d_ino == 0);

	/*
	 * Get most attributes from argument list.
	 */
	dip->d_ino = ino;
	dip->d_sblock = 0;
	dip->d_eblock = 0;
	dip->d_eoffset = 0;
	dip->d_fattr.va_type = vap->va_type;
	dip->d_fattr.va_mode = vap->va_mode;
	dip->d_fattr.va_uid = cr->cr_uid;
	dip->d_fattr.va_gid = cr->cr_gid;
	dip->d_fattr.va_nlink = 1;
	dip->d_fattr.va_atime = hrestime.tv_sec;
	dip->d_fattr.va_mtime = hrestime.tv_sec;
	dip->d_fattr.va_ctime = hrestime.tv_sec;

	/*
	 * Write the new disk inode to the disk inode slot.
	 */
	error = BFS_PUTINODE(bp->bfs_devnode, BFS_INO2OFF(ino), dip, cr);
	if (error)
		return error;

	/*
	 * bfs_iget() will get the requested inode in core
	 * and will perform the in-core inode initialization
	 */
	if ((error = bfs_iget(ITOV(dp)->v_vfsp, ino, &ip, B_TRUE, cr)) != 0)
		return error;

	/*
	 * Add the directory entry in the root directory
	 */
	if ((error = bfs_addirent(dp, fname, ino, cr)) != 0) {
		BFS_IRWLOCK_UNLOCK(ip);
		VN_RELE(ITOV(ip));
		return error;
	}

	FSPIN_LOCK(&bp->bfs_mutex); 
	bp->bfs_freeinodes--;
	FSPIN_UNLOCK(&bp->bfs_mutex); 

	/*
	 * Set the corresponding inode bit to indicate that inode
	 * is not available.
	 */
	BITMASKN_SET1(bp->bfs_inobitmap, ino);

	BFS_IRWLOCK_UNLOCK(ip);
	*ipp = ip;
	return 0;
}


/*
 * int
 * bfs_dirremove(inode_t *dp, off_t offset, char *nm, inode_t **ipp, cred_t *cr)
 *	Removes entry from root directory and decrements link count of file.
 *
 * Calling/Exit State:
 *	Callers of this routine must hold the following locks:
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock of the root directory in exclusive mode.
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 * Description:
 *	This routine will acquire the inode rwlock of the file in exclusive
 *	mode via bfs_iget(); It will then remove the directory entry from
 *	the root directory and decrement the link count of the file.
 *
 *	The inode will be returned locked. It is the responsibility of the
 *	caller to unlock the inode and release the vnode.
 */
STATIC int
bfs_dirremove(inode_t *dp, off_t offset, char *nm, inode_t **ipp, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)ITOV(dp)->v_vfsp->vfs_data;
	dinode_t	*dip;
	inode_t		*ip;
	int 		error;

	ASSERT(KS_HOLD0LOCKS());

	if ((error = bfs_iget(dp->i_vnode.v_vfsp, BFS_OFF2INO(offset),
				&ip, B_TRUE, cr)) != 0)
		return error;

	if (ip->i_vnode.v_type == VDIR) {
		error = EISDIR;
		goto out;
	}

	/*
	 * Zero the ino field in the root directory.
	 */
	error = bfs_rmdirent(dp, nm, cr);
	if (error)
		goto out;

	dip = &ip->i_diskino;
	dip->d_fattr.va_nlink--;

	FSPIN_LOCK(&ip->i_mutex); 
	dip->d_fattr.va_ctime = hrestime.tv_sec;
	FSPIN_UNLOCK(&ip->i_mutex);

	error = BFS_PUTINODE(bp->bfs_devnode, ip->i_inodoff, dip, cr);
	if (!error) {
		*ipp = ip;
		return 0;
	}else {
		dip->d_fattr.va_nlink++;
		error = bfs_addirent(dp, nm, dip->d_ino, cr);
	}
out:
	BFS_IRWLOCK_UNLOCK(ip);
	VN_RELE(ITOV(ip));
	return error;
}


/*
 * int
 * bfs_iremove(inode_t *ip, cred_t *cr)
 *
 * Calling/Exit State:
 *	Caller of this routine (bfs_inactive()) must hold the following locks:
 *	   - bfs_writelock
 *	   - bfs_fs_rwlock in shared mode
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 *
 */
STATIC int
bfs_iremove(inode_t *ip, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)ITOV(ip)->v_vfsp->vfs_data;
	dinode_t	*dip;
	daddr_t		blocks_freed;
	int 		error;

	ASSERT(KS_HOLD0LOCKS());

	dip = &ip->i_diskino;

	ASSERT(dip->d_fattr.va_nlink == 0);
	ASSERT(dip->d_ino != 0);

	/*
	 * Zero the ino entry (the link count is 0).
	 */
	dip->d_ino = 0;

	error = BFS_PUTINODE(bp->bfs_devnode, ip->i_inodoff, dip, cr);
	if (error) {
		dip->d_ino = BFS_OFF2INO(ip->i_inodoff);
		return error;
	}

	blocks_freed = BFS_FILEBLOCKS(dip);

	/*
	 * Must free up disk space and the inode.
	 */
	FSPIN_LOCK(&bp->bfs_mutex); 
	bp->bfs_freeblocks += blocks_freed;
	bp->bfs_freeinodes++;
	FSPIN_UNLOCK(&bp->bfs_mutex); 

	/*
	 * Clear the corresponding inode bit to indicate that inode
	 * is now available.
	 */
	BITMASKN_CLR1(bp->bfs_inobitmap, BFS_OFF2INO(ip->i_inodoff));

	/*
	 * If this is the file furthest into the disk, we must reset the
	 * value of the global variables:
	 * bfs_lastfilefs, bfs_sblklastfile and bfs_eblklastfile.
	 */
	if (bp->bfs_lastfilefs == ip->i_inodoff)
		return bfs_resetglbvars(bp, cr);

	return 0;
}


/*
 * int
 * bfs_quickcompact(vfs_t *vfsp, daddr_t gapsize, cred_t *cr)
 *
 * Calling/Exit State:
 *	Caller of this routine must hold the following locks:
 *	   - bfs_writelock
 *	   - bfs_fs_rwlock in shared mode
 *
 *	All locks held on entry will be held on exit.
 *
 *	On exit, the inode rwlock acquired via bfs_iget() will be released.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 *
 */
STATIC void
bfs_quickcompact(vfs_t *vfsp, daddr_t gapsize, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)vfsp->vfs_data;
	inode_t		*ip;
	char		*buffp;

	ASSERT(KS_HOLD0LOCKS());

	if (bfs_iget(vfsp, BFS_OFF2INO(bp->bfs_lastfilefs), &ip, B_TRUE, cr))
		return;

	buffp = kmem_alloc(BFSBUFSIZE, KM_SLEEP);

	bfs_shiftfile(bp, ip, gapsize, buffp, cr);

	kmem_free(buffp, BFSBUFSIZE);

	BFS_IRWLOCK_UNLOCK(ip);
	VN_RELE(ITOV(ip));

	return;
}


/*
 * int
 * bfs_truncatedown(inode_t *ip, off_t newsize, cred_t *cr)
 *
 * Calling/Exit State:
 *	Callers of this routine must hold the following locks:
 *	   - bfs_writelock 
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock in exclusive mode.
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 */
STATIC int
bfs_truncatedown(inode_t *ip, off_t newsize, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)ip->i_vnode.v_vfsp->vfs_data;
	dinode_t	*dip;
	daddr_t		oeblock;
	daddr_t		blocks_freed;
	int		error;

	ASSERT(KS_HOLD0LOCKS());

	dip = &ip->i_diskino;

	ASSERT(BFS_FILESIZE(dip) != 0);

	oeblock = dip->d_eblock;
	if (newsize == 0) {
		blocks_freed = BFS_FILEBLOCKS(dip);
		dip->d_sblock = dip->d_eblock = 0;
		dip->d_eoffset = 0;
	} else {
		dip->d_eoffset = (dip->d_sblock * BFS_BSIZE) + newsize - 1;
		dip->d_eblock = dip->d_eoffset / BFS_BSIZE;
		blocks_freed = oeblock - dip->d_eblock;
	}

	error = BFS_PUTINODE(bp->bfs_devnode, ip->i_inodoff, dip, cr);
	if (error)
		return error;

	FSPIN_LOCK(&bp->bfs_mutex); 
	bp->bfs_freeblocks += blocks_freed;
	FSPIN_UNLOCK(&bp->bfs_mutex); 

	/*
	 * If the new size of this file is 0 and this is the lastfile
	 * on the file system, must reset the global variables:
	 * bfs_lastfile, bfs_sblklastfile and bfs_eblklastfile.
	 */
	if (newsize == 0 && bp->bfs_lastfilefs == ip->i_inodoff) {
		BFS_IRWLOCK_UNLOCK(ip);
		error = bfs_resetglbvars(bp, cr);
		BFS_IRWLOCK_WRLOCK(ip);
		return error;
	}

	/*
	 * If the new size of this file is > 0 and this is the lastfile
	 * on the file system, must reset the global variable:
	 * bfs_eblklastfile.
	 */
	if (bp->bfs_lastfilefs == ip->i_inodoff)
		bp->bfs_eblklastfile = dip->d_eblock;

	/*
	 * If the truncated file (or truncated chunk of the file)
	 * is a BIGFILE and it is immediately followed by
	 * the lastfile in the file system and this lastfile
	 * is an SMALL file, do a quickcompaction.
	 */
	else if (blocks_freed >= BIGFILE &&
	    bp->bfs_sblklastfile == oeblock + 1 &&
	    bp->bfs_eblklastfile - bp->bfs_sblklastfile + 1 <= SMALLFILE)

		bfs_quickcompact(ITOV(ip)->v_vfsp, blocks_freed, cr);

	return error;
}


/*
 * int
 * bfs_truncateup(inode_t *ip, off_t newsize, cred_t *cr)
 *
 * Calling/Exit State:
 *	Callers of this routine must hold the following locks:
 *	   - bfs_writelock 
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock in exclusive mode.
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 */
/* ARGSUSED */
STATIC int
bfs_truncateup(inode_t *ip, off_t newsize, cred_t *cr)
{
	return bfs_extend(ip, newsize, (off_t)0, cr);
}


/*
 * int
 * bfs_filetoend(inode_t *ip, cred_t *cr)
 *	Move file to the end of the file system.
 *
 * Calling/Exit State:
 *	Callers of this routine must hold the following locks:
 *	   - bfs_writelock 
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock in exclusive mode.
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 */
STATIC int
bfs_filetoend(inode_t *ip, cred_t *cr)
{
	struct bfs_vfs	*bp = (struct bfs_vfs *)ip->i_vnode.v_vfsp->vfs_data;
	dinode_t	*dip;
	off_t		newblock;
	off_t		dstoffset, srcoffset;
	off_t		size;
	int		chunksize;
	int		error;
	off_t		j;
	void		*buf;
	
	ASSERT(KS_HOLD0LOCKS());

	dip = &ip->i_diskino;

	ASSERT(BFS_FILESIZE(dip) != 0);

	newblock = bp->bfs_eblklastfile + 1;

	dstoffset = newblock * BFS_BSIZE;
	srcoffset = dip->d_sblock * BFS_BSIZE;

	size = dip->d_eoffset + 1 - srcoffset;

	ASSERT(dstoffset + size <= bp->bfs_endfs + 1);

	/*
	 * Move the file in large chunks.
	 */
	chunksize = min(size, MAXBSIZE);
	buf = kmem_alloc(chunksize, KM_SLEEP);

	do {
		/*
		 * Move either the whole file or chunksize, whichever is
		 * smaller.
		 */
		j = min(dip->d_eoffset - srcoffset + 1, chunksize);

		(void) vn_rdwr(UIO_READ, bp->bfs_devnode, buf,
		  j, srcoffset, UIO_SYSSPACE, 0, 0, cr, 0);

		srcoffset += j;

		(void) vn_rdwr(UIO_WRITE, bp->bfs_devnode, buf,
				j, dstoffset, UIO_SYSSPACE, IO_SYNC,
				RLIM_INFINITY, cr, (int *)0);

		dstoffset += j;
	} while (srcoffset < dip->d_eoffset + 1);

	kmem_free(buf, chunksize);

	dip->d_eoffset = dstoffset - 1;
	dip->d_sblock = newblock;
	dip->d_eblock = dip->d_eoffset / BFS_BSIZE;

	/*
	 * Write the updated inode entry.
	 */
	error = BFS_PUTINODE(bp->bfs_devnode, ip->i_inodoff, dip, cr);
	if (error) {
		dip->d_eoffset = srcoffset - 1;
		dip->d_eblock = dip->d_eoffset / BFS_BSIZE;
		dip->d_sblock = (srcoffset - size) / BFS_BSIZE;
		return error;
	}

	/*
	 * Must reset the value of the global variables:
	 * bfs_lastfilefs, bfs_sblklastfile and bfs_eblklastfile.
	 */
	bp->bfs_lastfilefs = ip->i_inodoff;
	bp->bfs_sblklastfile = dip->d_sblock;
	bp->bfs_eblklastfile = dip->d_eblock;

	return 0;
}


/*
 * int
 * bfs_zerohole(struct bfs_vfs *bp, off_t offset, off_t newoffset, cred_t *cr)
 *
 * Calling/Exit State:
 *	Callers of this routine must hold the following locks:
 *	   - bfs_writelock 
 *	   - bfs_fs_rwlock in shared mode
 *	   - inode rwlock of file in exclusive mode.
 *
 *	All locks held on entry will be held on exit.
 *
 *	This routine may block, so no spin locks may be held on entry.
 *
 */
STATIC int
bfs_zerohole(struct bfs_vfs *bp, off_t offset, off_t newoffset, cred_t *cr)
{
	off_t		holesz;
	void		*p;
	int		count;
	int		error = 0;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(newoffset > offset);

	/*
	 * zero everything between endfile and newoffset
	 */
	holesz = newoffset - offset;
	p = kmem_zalloc(BFS_BSIZE, KM_SLEEP);
	ASSERT(p);
	while (holesz > 0) {
		count = MIN(BFS_BSIZE, holesz);
		error = vn_rdwr(UIO_WRITE, bp->bfs_devnode, p, count, offset,
			        UIO_SYSSPACE, IO_SYNC, RLIM_INFINITY, cr, 0);
		/* 
		 * if (error)
		 *	clean up and return.
		 */
		holesz -= count;
		offset += count;
	}
	kmem_free(p, BFS_BSIZE);
	return error;
}
