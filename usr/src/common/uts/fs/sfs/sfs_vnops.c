/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/sfs/sfs_vnops.c	1.128"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <svc/cpu.h>
#include <util/types.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <svc/time.h>
#include <svc/systm.h>
#include <svc/clock.h>
#include <proc/resource.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <proc/disp.h>
#include <fs/buf.h>
#include <fs/dnlc.h>
#include <fs/dow.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/fbuf.h>
#include <fs/file.h>
#include <fs/fcntl.h>
#include <mem/kmem.h>
#include <io/uio.h>
#include <io/conf.h>
#include <proc/mman.h>
#include <fs/pathname.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <fs/sfs/sfs_fs.h>
#include <acc/priv/privilege.h>
#include <acc/dac/acl.h>
#include <acc/mac/mac.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/sfs/sfs_fsdir.h>
#include <fs/sfs/sfs_data.h>
#include <fs/sfs/sfs_hier.h>
#include <fs/specfs/snode.h>
#include <util/inline.h>
#include <fs/sfs/sfs_quota.h>
#include <fs/dirent.h>		/* must be AFTER <fs/sfs/fsdir.h>! */
#include <svc/errno.h>
#include <util/metrics.h>
#include <io/poll.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/as.h>
#include <mem/seg.h>
#include <mem/seg_map.h>
#include <mem/seg_vn.h>
#include <mem/swap.h>
#include <fs/fs_subr.h>
#include <fs/flock.h>
#ifdef CC_PARTIAL
#include <acc/mac/cca.h>
#endif

extern int	sfs_syncip(inode_t *, int, cred_t *);
extern int	sfs_dirlook(inode_t *, char *, inode_t **, int, cred_t *);
extern int      sfs_bmappage(vnode_t *, off_t, size_t, page_t **,
                        page_t **, daddr_t *, daddr_t *, int, cred_t *);
extern int	sfs_bmap_might_realloc(vnode_t *, off_t, size_t);
extern int	sfs_iaccess(inode_t *, mode_t, cred_t *);
extern int	sfs_dirremove(inode_t *, char *, inode_t *, vnode_t *,
			enum dr_op, cred_t *);
extern int	sfs_direnter(inode_t *, char *, enum de_op, inode_t *,
			inode_t *, vattr_t *, inode_t **, cred_t *);
extern void     sfs_dirbad(inode_t *, char *, off_t);

extern int	sfs_free(inode_t *, daddr_t, off_t);
extern void	sfs_iupdat(inode_t *, enum iupmode);
extern void	sfs_iput(inode_t *, cred_t *);
extern int	sfs_itrunc(inode_t *, uint_t, cred_t *);
extern int	sfs_itruncup(inode_t *, size_t);
extern void	map_addr(vaddr_t *, uint_t, off_t, int);
extern int	specpreval(vtype_t, dev_t, cred_t *);
extern int	sfs_quotactl(vnode_t *, int, cred_t *);
extern int	sfs_chkdq(inode_t *, long, int, cred_t *);
extern int	sfs_chkiq(sfs_vfs_t *, inode_t *, uid_t, int, cred_t *);
extern void	sfs_dqrele(dquot_t *, cred_t *);
extern dquot_t	*sfs_getinoquota(inode_t *, cred_t *);
extern int	sfs_idestroy(inode_t *, cred_t *);
extern void	sfs_unlink_flushed(inode_t *, off_t);
#ifdef _SFS_SOFT_DNLC
extern boolean_t sfs_tryhold(vnode_t *);
#endif

STATIC	int sfs_open(vnode_t **, int, cred_t *);
STATIC	int sfs_close(vnode_t *, int, boolean_t , off_t, cred_t *);
STATIC	int sfs_read(vnode_t *, struct uio *, int, cred_t *);
STATIC	int sfs_write(vnode_t *, struct uio *, int, cred_t *);
STATIC	int sfs_ioctl(vnode_t *, int, int, int, cred_t *, int *);
STATIC	int sfs_setfl(vnode_t *, uint_t, uint_t, cred_t *);
STATIC	int sfs_getattr(vnode_t *, vattr_t *, int, cred_t *);
STATIC	int sfs_setattr(vnode_t *, vattr_t *, int, int, cred_t *);
STATIC	int sfs_access(vnode_t *, int, int, cred_t *);
STATIC  int sfs_lookup(vnode_t *, char *, vnode_t **, pathname_t *, int,
			vnode_t *, cred_t *);
STATIC  int sfs_create(vnode_t *, char *, vattr_t *, enum vcexcl,
			int, vnode_t **, cred_t *);
STATIC  int sfs_remove(vnode_t *, char *, cred_t *);
STATIC  int sfs_link(vnode_t *, vnode_t *, char *, cred_t *);
STATIC  int sfs_rename(vnode_t *, char *, vnode_t *, char *, cred_t *);
STATIC  int sfs_mkdir(vnode_t *, char *, vattr_t *, vnode_t **, cred_t *);
STATIC  int sfs_rmdir(vnode_t *, char *, vnode_t *, cred_t *);
STATIC  int sfs_readdir(vnode_t *, struct uio *, cred_t *, int *);
STATIC  int sfs_symlink(vnode_t *, char *, vattr_t *, char *, cred_t *);
STATIC  int sfs_readlink(vnode_t *, struct uio *, cred_t *);
STATIC  int sfs_fsync(vnode_t *, cred_t *);
STATIC  void sfs_inactive(vnode_t *, cred_t *);
STATIC	int sfs_fid(vnode_t *, struct fid **);
STATIC	int sfs_rwlock(vnode_t *, off_t, int, int, int);
STATIC	void sfs_rwunlock(vnode_t *, off_t, int);
STATIC	int sfs_seek(vnode_t *, off_t, off_t *);
STATIC	int sfs_frlock(vnode_t *, int, struct flock *, int, off_t, cred_t *);
STATIC	int sfs_getpage(vnode_t *, uint_t, uint_t, uint_t *, page_t **,
		uint_t, struct seg *, vaddr_t, enum seg_rw, cred_t *);
STATIC	int sfs_putpage(vnode_t *, off_t, uint_t, int, cred_t *);
STATIC	int sfs_map(vnode_t *, off_t, struct as *, vaddr_t *, uint_t,
		uint_t, uint_t, uint_t, cred_t *);
STATIC	int sfs_addmap(vnode_t *, uint_t, struct as *, vaddr_t, uint_t,
		uint_t, uint_t, uint_t, cred_t *);
STATIC	int sfs_delmap(vnode_t *, uint_t, struct as *, vaddr_t, uint_t,
		uint_t, uint_t, uint_t, cred_t *);
STATIC	int sfs_stablestore(vnode_t **, off_t *, size_t *, void **, cred_t *);
STATIC	int sfs_relstore(vnode_t *, off_t, size_t, void *, cred_t *);
STATIC	int sfs_getpagelist(vnode_t *, off_t, uint_t, page_t *,
		  void *, int, cred_t *);
STATIC	int sfs_putpagelist(vnode_t *, off_t, page_t *, void *, int, cred_t *);
STATIC	int sfs_poll(vnode_t *, int, int, short *, struct pollhead **);
STATIC	int sfs_pathconf(vnode_t *, int, u_long *, cred_t *);
STATIC	int sfs_getacl(vnode_t *, long, long *, struct acl *, cred_t *, int *);
STATIC	int sfs_setacl(vnode_t *, long, long, struct acl *, cred_t *);
STATIC	int sfs_setlevel(vnode_t *, lid_t, cred_t *);
STATIC	int sfs_makemld(vnode_t *, char *, vattr_t *, vnode_t **, cred_t *);
STATIC	int sfs_readi(inode_t *, struct uio *, int, cred_t *);
STATIC	int sfs_writei(inode_t *, struct uio *, int, cred_t *);
void sfs_release(vnode_t *);
STATIC  int ufs_lookup(vnode_t *, char *, vnode_t **, pathname_t *, int,
			 vnode_t *, cred_t *);

vnodeops_t sfs_vnodeops = {
	sfs_open,
	sfs_close,
	sfs_read,
	sfs_write,
	sfs_ioctl,
	sfs_setfl,
	sfs_getattr,
	sfs_setattr,
	sfs_access,
	sfs_lookup,
	sfs_create,
	sfs_remove,
	sfs_link,
	sfs_rename,
	sfs_mkdir,
	sfs_rmdir,
	sfs_readdir,
	sfs_symlink,
	sfs_readlink,
	sfs_fsync,
	sfs_inactive,
	sfs_release,
	sfs_fid,
	sfs_rwlock,
	sfs_rwunlock,
	sfs_seek,
	fs_cmp,
	sfs_frlock,
	(int (*)())fs_nosys,	/* realvp */
	sfs_getpage,
	sfs_putpage,
	sfs_map,
	sfs_addmap,
	sfs_delmap,
	sfs_poll,
	sfs_pathconf,
	sfs_getacl,	
	sfs_setacl,	
	sfs_setlevel,	
	(int (*)())fs_nosys, 	/* getdvstat */
	(int (*)())fs_nosys, 	/* setdvstat */
	sfs_makemld,
	(int (*)())fs_nosys,	/* testmld */
	sfs_stablestore,
	sfs_relstore,
	sfs_getpagelist,
	sfs_putpagelist,
	(int (*)())fs_nosys,	/* msgio */
	(int (*)())fs_nosys,	/* filler[4] */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys
};

vnodeops_t ufs_vnodeops = {
	sfs_open,
	sfs_close,
	sfs_read,
	sfs_write,
	sfs_ioctl,
	sfs_setfl,
	sfs_getattr,
	sfs_setattr,
	sfs_access,
	ufs_lookup,
	sfs_create,
	sfs_remove,
	sfs_link,
	sfs_rename,
	sfs_mkdir,
	sfs_rmdir,
	sfs_readdir,
	sfs_symlink,
	sfs_readlink,
	sfs_fsync,
	sfs_inactive,
	sfs_release,
	sfs_fid,
	sfs_rwlock,
	sfs_rwunlock,
	sfs_seek,
	fs_cmp,
	sfs_frlock,
	(int (*)())fs_nosys,	/* realvp */
	sfs_getpage,
	sfs_putpage,
	sfs_map,
	sfs_addmap,
	sfs_delmap,
	sfs_poll,
	sfs_pathconf,
	sfs_getacl,	
	sfs_setacl,	
	sfs_setlevel,	
	(int (*)())fs_nosys, 	/* getdvstat */
	(int (*)())fs_nosys, 	/* setdvstat */
	sfs_makemld,
	(int (*)())fs_nosys,	/* testmld */
	sfs_stablestore,
	sfs_relstore,
	sfs_getpagelist,
	sfs_putpagelist,
	(int (*)())fs_nosys,	/* msgio */
	(int (*)())fs_nosys,	/* filler[4]... */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys
};

/*
 * Don't cache write blocks to files with the sticky bit set.
 * Used to keep swap files from blowing the page cache on a server.
 */
int sfs_stickyhack = 1;

/*
 * int
 * sfs_open(vnode_t **vpp, int flag, cred_t *cr)
 * No special action required for ordinary files.  (Devices are handled
 * through SPECFS.)
 *
 * Calling/Exit State:
 *	Calling LWP holds vnode's r/w sleep lock (v_lock) in *shared* mode
 *	for duration of call. The lock was obtained before calling sfs_open.
 *
 * Description:
 *	We have to hold the inode's rwlock in *exclusive* mode to
 *	synchronize with sfs_close. 
 */
/* ARGSUSED */
STATIC int
sfs_open(vnode_t **vpp, int flag, cred_t *cr)
{
	vfs_t		*vfsp;
	struct sfs_vfs	*sfs_vfsp;

	vfsp = (*vpp)->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	return (0);
}

/*
 * int
 * sfs_close(vnode_t *vp, int flag, boolean_t lastclose,
 *           off_t offset, cred_t *cr)
 *
 * Calling/Exit State:
 *	The caller doesn't hold any locks on the vnode in question.
 *
 * Description:
 *	The inode's rwlock is held to prevent another LWP in the 
 *	same process from establishing a file/record lock on the file.
 */
/*ARGSUSED*/
STATIC int
sfs_close(vnode_t *vp, int flag, boolean_t lastclose, off_t offset, cred_t *cr)
{
	inode_t	*ip;

	ip = VTOI(vp);

	if (vp->v_filocks) {
		SFS_IRWLOCK_WRLOCK(ip);
		cleanlocks(vp, u.u_procp->p_epid, u.u_procp->p_sysid);
		SFS_IRWLOCK_UNLOCK(ip);
	}

	return (0);
}

/*
 * int
 * sfs_readi(inode_t *ip, struct uio *uio, int ioflag, cred_t *cr)
 *	Read data from an inode.
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode's rwlock in at least *shared*
 *	mode. This lock is must be acquired from above the VOP interface
 *	via VOP_RWRDLOCK() (below the VOP interface use sfs_rwlock).
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EINVAL	offset in <uio> is negative.
 *
 * Description:
 *	The VM segmap driver is used to establish a mapping for the
 *	vnode and offset to a kernel address space managed by the segmap
 *	driver. It then calls uiomove() to move data from the kernel
 *	address space to the calling process's buffer. Accessing the
 *	kernel address space causes the a fault which is handled
 *	by the segmap driver. The segmap driver calls VOP_GETPAGE in
 *	response to the fault where the pages are sought first in the
 *	page cache, and if necessary, reads them in from the file's
 *	backing store.
 *
 */
/* ARGSUSED */
STATIC int
sfs_readi(inode_t *ip, struct uio *uio, int ioflag, cred_t *cr)
{
	int	n;
	int	mapon, on;
	fs_t	*fs;
	off_t	offset;
	vnode_t	*vp;
	int	error;
	int	diff;
	uint_t	flags;
	addr_t	base;
	uio_t tmpuio, *olduio;
        iovec_t tmpiov;
        boolean_t isshortlink = B_FALSE;
	pl_t opl;

	vp = ITOV(ip);
	ASSERT((int)vp->v_type == VREG || (int)vp->v_type == VDIR ||
		(int)vp->v_type == VLNK);

	error = 0;
	offset = uio->uio_offset;
	if (offset < 0) {
		return (EINVAL);
	}

	if (uio->uio_resid == 0) {
		return (0);
	}

	if (WRITEALLOWED(vp, cr)) {
		opl = SFS_ILOCK(ip);
		IMARK(ip, IACC);
		SFS_IUNLOCK(ip, opl);
	}

	fs = ip->i_fs;
	/*
         * If the inode is a short symlink, and the target is already
         * being kept in the db list, then just copy the target to the
         * uio structure.  If the target is not in the db list, but could
         * be, then redirect the uio structure so that the uiomove copies
         * the data into the inode.  Once it's in the inode, it will then
         * be copied to the original uio structure.
         */
        if ((vp->v_type == VLNK) && (ip->i_size <= SHORTSYMLINK)) {
                if (ip->i_db[1] != 0)
                        return (uiomove((char *)&ip->i_db[1], ip->i_size,
                                				UIO_READ, uio));

		/*
		 * We need to do io to read in the target. Since we
		 * are going to write it to i_db, we need to upgrade
		 * the rwlock to excl. mode.
		 */
		SFS_IRWLOCK_UNLOCK(ip);
		SFS_IRWLOCK_WRLOCK(ip);
		/*
		 * After we get the lock in excl. mode, we need to
		 * Check again. If someone raced with us and won,
		 * We only need to copy the target and return.
		 */
		if (ip->i_db[1] != 0) {
                        error =  uiomove((char *)&ip->i_db[1], ip->i_size,
                                				UIO_READ, uio);
                        SFS_IRWLOCK_UNLOCK(ip);
                        SFS_IRWLOCK_RDLOCK(ip);
			return error;
                }

                tmpiov.iov_base = (char *)&ip->i_db[1];
                tmpiov.iov_len = ip->i_size;
                tmpuio.uio_iov = &tmpiov;
                tmpuio.uio_iovcnt = 1;
                tmpuio.uio_offset = 0;
                tmpuio.uio_segflg = UIO_SYSSPACE;
                tmpuio.uio_resid = ip->i_size;
                olduio = uio;
                uio = &tmpuio;
                isshortlink = B_TRUE;
        }
	do {
		diff = ip->i_size - offset;

		if (diff <= 0) {
			break;
		}
		mapon = ((blkrounddown(fs, offset) & MAXBOFFSET));
		on = blkoff(fs, offset);

		n = MIN(fs->fs_bsize - on, uio->uio_resid);

		if (diff < n) {
			n = diff;
		}

		base = segmap_getmap(segkmap, vp, offset, n, S_READ,
				     B_FALSE, NULL);

		error = uiomove(base + mapon + on, (long)n, UIO_READ, uio);

		if (error == 0) {
			offset = uio->uio_offset;
			if (isshortlink)
                                flags = SM_INVAL;
			else if ((n + on + mapon) == MAXBSIZE || offset == ip->i_size)
				/*
				 * We read up to a MAXBSIZE boundary. Assume
				 * we don't need the data any time soon.
				 * Notify the segment driver accordingly.
				 */
				flags = SM_NOCACHE;
			else
				flags = 0;
		  	error = segmap_release(segkmap, base, flags);
		} else
			(void)segmap_release(segkmap, base, 0);
	} while (error == 0 && uio->uio_resid > 0);

	if (isshortlink && error == 0) {
                error = uiomove((char *)&ip->i_db[1], ip->i_size, UIO_READ,
                        					olduio);
		SFS_IRWLOCK_UNLOCK(ip);
                SFS_IRWLOCK_RDLOCK(ip);
        }
	return (error);
}

/*
 * int
 * sfs_read(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *fcr)
 *	Transfer data from <vp> to the calling process's address
 *	space.
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode's rwlock in at least
 *	*shared* mode on entry; rwlock remains held on exit. This
 *	lock is usually obtained by a call to
 *	VOP_RWRDLOCK() specifying the same length, offset that's
 *	in <uiop>.
 */
/*ARGSUSED*/
STATIC int
sfs_read(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *fcr)
{
	struct cred     *cr = VCURRENTCRED(fcr);
	inode_t		*ip;
	vfs_t		*vfsp;
	struct sfs_vfs	*sfs_vfsp;
	int		error;

	vfsp = vp->v_vfsp;
	sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		error = EIO;
	} else {
		ip = VTOI(vp);
		error = sfs_readi(ip, uiop, ioflag, cr);
	}
		
	return (error);
}

/*
 * int
 * sfs_write(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *fcr)
 *	Transfer data from the calling process's address space
 *	to <vp>.
 *
 * Calling/Exit State:
 *	The calling LWP holds the inode's rwlock in *exclusive* mode on
 *	entry; it remains held on exit. The rwlock was acquired by calling
 *	VOP_RWWRLOCK specifying the same length, offset pair that's
 *	in <uiop>.
 */
/*ARGSUSED*/
STATIC int
sfs_write(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *fcr)
{
	struct cred     *cr = VCURRENTCRED(fcr);
	inode_t		*ip;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;
	int		error;

	ASSERT(vp->v_type == VREG || vp->v_type == VDIR);

	vfsp = vp->v_vfsp;
	sfs_vfsp = (struct sfs_vfs *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	ip = VTOI(vp);

	if (vp->v_type == VREG) {
		error = fs_vcode(vp, &ip->i_vcode);
		if (error)
			return (error);
	}

	if ((ioflag & IO_APPEND) != 0 && (vp->v_type == VREG)) {
		/*
		 * In append mode start at end of file.
		 * NOTE: lock not required around i_size
		 * sample since r/w lock is held exclusive
		 * by calling process.
		 */
		uiop->uio_offset = ip->i_size;
	}

	error = sfs_writei(ip, uiop, ioflag, cr);

	return (error);
}


/*
 * sfs_writei(ip, uio, ioflag, cr)
 *	Write to an inode.
 *
 * Calling/Exit State:
 *	The caller holds the inode's rwlock *exclusive* to
 *	PREparation to change the file's contents/allocation
 *	information.
 *
 * Description:
 *	The details of this routine will change. For now, we
 *	perform a pre-V4 style write through the buf cache.
 *
 *	The VM segmap driver is used to establish a mapping
 *	of the vnode, offset to a kernel address space managed
 *	by the segmap driver. It uses uiomove() to move data
 *	from the user's buffer to the kernel address space.
 */
/* ARGSUSED */
STATIC int
sfs_writei(inode_t *ip, struct uio *uio, int ioflag, cred_t *cr)
{
	int n, mapon, on;
	struct fs *fs;
	struct vnode *vp;
	int type, error;
	rlim_t limit = uio->uio_limit;
	int iupdat_flag;
	long old_blocks;
	long oresid = uio->uio_resid;
	addr_t base;
	uint_t flags, off;
	int bsize;
	boolean_t mustfault;
	off_t lastalloc;
	int isshortlink = 0;
        uio_t tmpuio;
        iovec_t tmpiov;
	pl_t opl;

	vp = ITOV(ip);
	ASSERT(vp->v_type == VREG || vp->v_type == VDIR ||
		vp->v_type == VLNK);

	if (uio->uio_offset < 0) {
		return (EINVAL);
	} else if (uio->uio_resid == 0) {
		return (0);
	}

	fs = ip->i_fs;
	bsize = fs->fs_bsize;
	type = ip->i_mode & IFMT;

	if (type == IFREG && uio->uio_offset >= limit)
		return(EFBIG);
	/*
         * If the inode being written is a short symlink, then create
         * a second uio structure which shows where the target is being
         * copied from.  The target will be copied twice, once using the
         * original uio structure and once using this new one.
         */
        if ((type == IFLNK) && (uio->uio_resid <= SHORTSYMLINK) &&
                        (uio->uio_iovcnt == 1)) {
                ++isshortlink;
                tmpuio = *uio;
                tmpiov = *tmpuio.uio_iov;
                tmpuio.uio_iov = &tmpiov;
        }

	opl = SFS_ILOCK(ip);
	/* don't update access time in getpage */
	ip->i_flag |= INOACC;

	if (ioflag & IO_SYNC) {
		old_blocks = ip->i_blocks;
		ip->i_flag |= ISYNC;
		iupdat_flag = 0;
	}
	SFS_IUNLOCK(ip, opl);

	if (uio->uio_offset > ip->i_size) {
		error = sfs_itruncup(ip, uio->uio_offset);
		if (error)
			goto out;
	}

	do {
		off = uio->uio_offset;
		mapon = ((blkrounddown(fs, off) & MAXBOFFSET));
		on = blkoff(fs, off);
		n = MIN(bsize - on, uio->uio_resid);

		/*
		 * If we are exceeding the ulimit or if we are 
		 * overflowing, clip the io size.
		 */
		if (type == IFREG && off + n >= limit || off + n <= 0) {
			if (off >= limit) {
				error = EFBIG;
				goto out;
			}
			n = limit - off;
		}

		ip->i_iolen = off + n;

		if (off + n > ip->i_size) {
			if (lblkno(fs, ip->i_size) >= NDADDR)
				lastalloc = blkroundup(fs, ip->i_size);
			else
				lastalloc = fragroundup(fs, ip->i_size);

			mustfault = ((lastalloc & PAGEOFFSET) != 0 &&
				     off + n > lastalloc);
		} else
			mustfault = B_FALSE;

		base = segmap_getmap(segkmap, vp, off, n, S_WRITE,
				     mustfault, NULL);

		error = uiomove(base + mapon + on, (long)n, UIO_WRITE, uio);

		ip->i_iolen = 0;

		if (error) {
			off_t noff;
			ASSERT(uio->uio_offset < off + n);

			/*
			 * If we had some sort of error during uiomove,
			 * call segmap_abort_create to have the pages
			 * aborted if we created them.
			 */
			noff = segmap_abort_create(segkmap,
					base, uio->uio_offset,
					(off + n - uio->uio_offset));

			if (noff != -1 && noff < uio->uio_offset) {
				/*
				 * Some pages aborted, need to fix
				 * resid.
				 */
				uio->uio_resid += uio->uio_offset - noff;
			}
			(void) segmap_release(segkmap, base, SM_INVAL);
		} else {
			

			flags = 0;
			/*
			 * Force write back for synchronous write cases.
			 */
			if (ioflag & IO_SYNC) {
			/*
			 * If the sticky bit is set but the
			 * execute bit is not set, we do a
			 * synchronous write back and free
			 * the page when done.  We set up swap
			 * files to be handled this way to
			 * prevent servers from keeping around
			 * the client's swap pages too long.
			 */
				if (ip->i_swapcnt > 0 || IS_STICKY(ip)) {
					flags = SM_WRITE | SM_DONTNEED;
				} else {
					iupdat_flag = 1;
					flags = SM_WRITE;
				}
			} else if (mapon + on + n == MAXBSIZE) {
				/*
				 * Have written to MAXBSIZE boundary.
				 * Mark the buffer to indicate that
				 * it won't be needed again soon.  The
				 * data will be written out either by
				 * fsflush or when the page is recycled
				 * for another purpose.
				 */
				flags = SM_NOCACHE;
			} else if (isshortlink) {
				/*
                                 * if it's a short symlink, then copy the
                                 * the target into the disk block list, and
                                 * release the page.
                                 */
                                if (uiomove((char *)&ip->i_db[1],
                                                tmpuio.uio_resid, UIO_WRITE,
                                                &tmpuio) == 0)
                                        flags = SM_WRITE | SM_ASYNC | SM_INVAL;                               else
                                        bzero((char *)&ip->i_db[1],
                                                SHORTSYMLINK);
			}
			if (ITOV(ip)->v_flag & VNOSYNC)
				flags &= ~(SM_WRITE | SM_ASYNC);
			opl = SFS_ILOCK(ip);
			/*
			 * Fix the isize here to catch the case where
			 * the extending write did not take a fault
			 * because the extension did not go beyond
			 * the existing backing store allocation
			 * boundary (i.e. a fragment).
			 */
			if (off + n > ip->i_size)
				ip->i_size = off + n;
			IMARK(ip, IUPD|ICHG);
			SFS_IUNLOCK(ip, opl);
			/*
                         * Determine if either the setuid-on-exec or setgid-
                         * on-exec bits are set in the file permission bits.
                         */
                        if (((ip->i_mode & (VSGID|(VEXEC>>3))) ==
                             (VSGID|(VEXEC>>3))) || (ip->i_mode & ISUID)) {
                                /*
                                 * If the set[ug]id bits are set, determine if
                                 * the process is privileged.  If not, clear
                                 * both bits unconditionally.
                                 */
                                if (pm_denied(cr, P_OWNER)) {
                                        ip->i_mode &= ~(ISUID);
                                        ip->i_mode &= ~(ISGID);
				}
			}
			error = segmap_release(segkmap, base, flags);
		}
	} while (error == 0 && uio->uio_resid > 0);


	/*
	 * If we are doing synchronous write the only time we should
	 * not be sync'ing the ip here is if we have the sfs_stickyhack
	 * activated, the file is marked with the sticky bit and
	 * no exec bit, the file length has not been changed and
	 * no new blocks have been allocated during this write.
	 */
	if ((ioflag & IO_SYNC) != 0 &&
	    (iupdat_flag != 0 || old_blocks != ip->i_blocks)) {
		sfs_iupdat(ip, IUP_SYNC);
	}

out:
	/*
	 * If we've already done a partial-write, terminate
	 * the write but return no error.
	 */
	if (oresid != uio->uio_resid)
		error = 0;

	return (error);
}

/*
 *
 * int
 * sfs_ioctl(vnode_t *vp, int cmd, int arg, int flag, cred_t *cr, int *rvalp)
 *	Perform an ioctl command on a file. Only Q_QUOTACTL is valid
 *	for UFS file systems.
 *
 * Calling/Exit State:
 *	No file/vnode locks held.
 *	
 */
/* ARGSUSED */
STATIC int
sfs_ioctl(vnode_t *vp, int cmd, int arg, int flag, cred_t *cr, int *rvalp)
{
	vfs_t *vfsp;
	sfs_vfs_t *sfs_vfsp;

	vfsp = vp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	if (UFSIP(VTOI(vp))) {
		if (cmd == Q_QUOTACTL) {
			return (sfs_quotactl(vp, arg, cr));
		} else {
			return (ENOTTY);
		}
	} else
		return (ENOTTY);
}

/*
 * int
 * sfs_setfl(vnode_t *vp, int oflags, int nflags, cred_t *cr)
 *	Indicate whether the file table flags can be set to
 *	<nflags>. SFS always grants permission to do this.
 *
 * Calling/Exit State:
 *	No vnode locks held on entry - does not require
 *	synchronization with other operations. The user programs
 *	performaning this fcntl operation must synchronize correctly,
 *	i.e., the onus is on them.
 */
/* ARGSUSED */
STATIC int
sfs_setfl(vnode_t *vp, uint_t oflags, uint_t nflags, cred_t *cr)
{
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;

	vfsp = vp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	return (fs_setfl(vp, oflags, nflags, cr));
}

/*
 * int
 * sfs_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
 *	Return attributes for a vnode.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 *	The inode's rwlock is held *shared* while copying the inode's
 *	attributes.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EIO	file system containing <vp> has been invalidated
 *			and is unaccessible.
 */
/* ARGSUSED */
STATIC int
sfs_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
{
	inode_t		*ip;
	fs_t		*fsp;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;
	pl_t		opl;

	vfsp = vp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	/*
	 * Generally, we return all of the attributes. We
	 * conditionally return the ACL count and security
	 * level. 
	 */

	fsp = getfs(vfsp);
	ip = VTOI(vp);

	/*
	 * Copy from inode table.
	 */
	SFS_IRWLOCK_RDLOCK(ip);

	vap->va_type   = vp->v_type;
	vap->va_mode   = ip->i_mode & MODEMASK;
	vap->va_uid    = ip->i_uid;
	vap->va_gid    = ip->i_gid;
	vap->va_fsid   = ip->i_dev;
	vap->va_nodeid = ip->i_number;
	vap->va_nlink  = ip->i_nlink;
	vap->va_size   = ip->i_size;
	vap->va_vcode  = ip->i_vcode;

	if (vp->v_type == VCHR || vp->v_type == VBLK || vp->v_type == VXNAM){
		vap->va_rdev = ip->i_rdev;
	} else {
		vap->va_rdev = 0;	/* not a b/xnam/c spec. */
	}

	opl = SFS_ILOCK(ip);
	vap->va_atime.tv_sec = ip->i_atime.tv_sec;
	vap->va_mtime.tv_sec = ip->i_mtime.tv_sec;
	vap->va_ctime.tv_sec = ip->i_ctime.tv_sec;
	vap->va_atime.tv_nsec = ip->i_atime.tv_usec * 1000;
	vap->va_ctime.tv_nsec = ip->i_ctime.tv_usec * 1000;
	vap->va_mtime.tv_nsec = ip->i_mtime.tv_usec * 1000;
	SFS_IUNLOCK(ip, opl);

	switch (ip->i_mode & IFMT) {

	case IFBLK:
	case IFCHR:
		vap->va_blksize = MAXBSIZE;
		break;
	default:
		vap->va_blksize = fsp->fs_bsize;
		break;
	}

	vap->va_nblocks = ip->i_blocks;

	/*
	 * Can only return level and ACL count for SFS file systems.
	 */
	if (vap->va_mask & AT_ACLCNT) {
		if (UFSIP(ip)) {
			vap->va_aclcnt = NACLBASE;
		} else {

			/* 
			 * If there are additional users and/or additional
			 * groups, add the count for USER_OBJ, CLASS_OBJ,
			 * and OTHER_OBJ.  If there are only defaults, add
			 * the count for USER_OBJ, GROUP_OBJ, CLASS_OBJ,
			 * and OTHER_OBJ.
			 */
			if (ip->i_aclcnt > ip->i_daclcnt) {
				vap->va_aclcnt = ip->i_aclcnt + NACLBASE - 1;
			} else {
				vap->va_aclcnt = ip->i_aclcnt + NACLBASE;
			}

		}
	}

	SFS_IRWLOCK_UNLOCK(ip);
	return (0);
}

/*
 * int
 * sfs_setattr(vnode_t *vp, vattr_t *vap, int flags, int ioflags, cred_t *cr)
 *	Modify/Set a inode's attributes.
 *
 * Calling/Exit State:
 *	The caller doesn't hold any inode locks on entry or exit.
 */
/* ARGSUSED */
STATIC int
sfs_setattr(vnode_t *vp, vattr_t *vap, int flags, int ioflags, cred_t *cr)
{
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;
	inode_t		*ip;
	long		mask = vap->va_mask;
	int		issync = 0;
	int		error = 0;
	timestruc_t	timenow;
	pl_t		pl;
	long change;

	vfsp = vp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	/*
	 * Cannot set these attributes.
	 */
	if (mask & AT_NOSET) {
		return (EINVAL);
	}

	ip = VTOI(vp);
	if (ip->i_fs->fs_ronly)
                return EROFS;

	SFS_IRWLOCK_WRLOCK(ip);

	/*
	 * Truncate file.  Must have write permission and not be a directory.
	 */
	if (mask & AT_SIZE) {
		if (vp->v_type == VDIR) {
			error = EISDIR;
			goto out;
		}
		if (vap->va_size >
		    u.u_rlimits->rl_limits[RLIMIT_FSIZE].rlim_cur) {
			error = EFBIG;
			goto out;
		}
		error = sfs_iaccess(ip, IWRITE, cr);
		if (error) {
			goto out;
		}
		if (vp->v_type == VREG) {
			error = fs_vcode(vp, &ip->i_vcode);
			if (error) {
				goto out;
			}
		}

		/*
		 * Check if there is any active mandatory lock on the
		 * range that will be truncated/expanded.
		 */

		if (MANDLOCK(vp, ip->i_mode)) {
			off_t	offset;
			int	length;

			if (ip->i_size < vap->va_size) {
				/*
				 * "Truncate up" case: need to make sure
				 * there is no lock beyond current EOF.
				 */
				offset = ip->i_size;
				length = vap->va_size - offset;
			} else {
				offset = vap->va_size;
				length = ip->i_size - offset;
			}
			error = chklock(vp, FWRITE, offset, length, ioflags,
					ip->i_size);
			if (error) {
				goto out;
			}
		}
		error = sfs_itrunc(ip, vap->va_size, cr);
		if (error) {
			goto out;
		}
		issync++;
	}
	/*
	 * Change file access modes.  Must be owner or privileged.
	 */
	if (mask & AT_MODE) {
		mode_t	nmode;

		if (cr->cr_uid != ip->i_uid && pm_denied(cr, P_OWNER)) {
			error = EPERM;
			goto out;
		}
		/*
		 * If a directory's search accessibility might be
		 * changed, then we need to update its generation count
		 * (i_agen). This is needed by the ufs_lookup optimization.
		 */
		nmode = (ip->i_mode & IFMT) | (vap->va_mode & ~IFMT);
		if (nmode != ip->i_mode) {
			++ip->i_agen;
			WRITE_SYNC();
			ip->i_mode = nmode;
		}
		/*
		 * A non-privileged user can set the sticky bit
		 * on a directory.
		 */
		if (vp->v_type != VDIR) {
			if ((ip->i_mode & ISVTX) && pm_denied(cr, P_OWNER)) {
				ip->i_mode &= ~ISVTX;
			}
		}

		if (!groupmember((uid_t)ip->i_gid, cr) &&
		    pm_denied(cr, P_OWNER)) {
			ip->i_mode &= ~ISGID;
		}
		pl = SFS_ILOCK(ip);
		IMARK(ip, ICHG);
		SFS_IUNLOCK(ip, pl);
	}
	if (mask & (AT_UID|AT_GID)) {
		int checksu = 0;

		/*
		 * To change file ownership, a process not running with
		 * privilege must be running as the owner of the file.
		 */
		if (cr->cr_uid != ip->i_uid) {
			checksu = 1;
		} else {
			if (rstchown) {
				/*
				 * "chown" is restricted.  A process not
				 * running with privilege cannot change the
				 * owner, and can only change the group to a
				 * group of which it's currently a member.
				 */
				if (((mask & AT_UID) &&
				      vap->va_uid != ip->i_uid) ||
				    ((mask & AT_GID) &&
				      !groupmember(vap->va_gid, cr))) {
					checksu = 1;
				}
			}
		}

		if (checksu && pm_denied(cr, P_OWNER)) {
			error = EPERM;
			goto out;
		}

		if (pm_denied(cr, P_OWNER)) {
                       if ((ip->i_mode & (VSGID|(VEXEC>>3))) ==
                           (VSGID|(VEXEC>>3)))
                               ip->i_mode &= ~(ISGID);
                       ip->i_mode &= ~(ISUID);
		}
		if (mask & AT_UID) {
			/*
			 * Remove the blocks, and the file, from the old user's
			 * quota.
			 *
			 * If a directory's search accessibility might be
			 * changed, then we need to update its generation
			 * count (i_agen). This is needed by the ufs_lookup
			 * optimization.
			 *
			 * The change information is also used to speed things
			 * up down below.
			 */
			if (ip->i_uid == vap->va_uid) {
				change = 0;
			} else {
				change = ip->i_blocks;
				++ip->i_agen;
				WRITE_SYNC();
			}
			if (UFSIP(ip) && ip->i_dquot != NULL) {
				dquot_t	*dqp;

				(void) sfs_chkdq(ip, -change, 1, cr);
				(void) sfs_chkiq((sfs_vfs_t *)
			    		((ITOV(ip))->v_vfsp->vfs_data), ip,
			    		(uid_t)ip->i_uid, 1, cr);
				dqp = ip->i_dquot;
				ip->i_dquot = NULL;
				sfs_dqrele(dqp, cr);
			}
			ip->i_uid = vap->va_uid;
		}
		if (mask & AT_GID) {
			/*
			 * If a directory's search accessibility might be
			 * changed, then we need to update its generation
			 * count (i_agen). This is needed by the ufs_lookup
			 * optimization.
			 */
			if (ip->i_gid != vap->va_gid) {
				++ip->i_agen;
				WRITE_SYNC();
				ip->i_gid = vap->va_gid;
			}
		}
		pl = SFS_ILOCK(ip);
		IMARK(ip, ICHG);
		SFS_IUNLOCK(ip, pl);
		if (mask & AT_UID) {
			/*
			 * Add the blocks, and the file, to the old user's
			 * quota.
			 */
			if (UFSIP(ip)) {
				pl = QLIST_LOCK();
				if (sfs_vfsp->vfs_qinod != NULL) {
					QLIST_UNLOCK(pl);
					ip->i_dquot = sfs_getinoquota(ip, cr);
					(void) sfs_chkdq(ip, change, 1, cr);
					(void) sfs_chkiq((sfs_vfs_t *)
						((ITOV(ip))->v_vfsp->vfs_data),
			    			(inode_t *)NULL,
					 	(uid_t)ip->i_uid, 1, cr);
				} else {
					QLIST_UNLOCK(pl);
				}
			}
		}
	}
	/*
	 * Change file access or modified times.
	 */
	if (mask & (AT_ATIME|AT_MTIME)) {
		boolean_t	mtime = B_TRUE;
		boolean_t	atime = B_TRUE;

		if (cr->cr_uid != ip->i_uid && pm_denied(cr, P_OWNER)) {
			if (flags & ATTR_UTIME) {
				error = EPERM;
			} else {
				error = sfs_iaccess(ip, IWRITE, cr);
			}
			if (error) {
				goto out;
			}
		}
		GET_HRESTIME(&timenow);
		pl = SFS_ILOCK(ip);
		if (mask & AT_MTIME) {
			if (flags & (ATTR_UTIME | ATTR_UPDTIME)) {
				if ((flags & ATTR_UPDTIME) &&
					(vap->va_mtime.tv_sec <=
						ip->i_mtime.tv_sec)) {
					mtime = B_FALSE;
				} else {
					ip->i_mtime.tv_sec =
						vap->va_mtime.tv_sec;
					ip->i_mtime.tv_usec =
						vap->va_mtime.tv_nsec/1000;
				}
			} else {
				/*
				 * Make sure that the mod time is unique.
				 */
				if (ip->i_mtime.tv_sec == timenow.tv_sec &&
				    ip->i_mtime.tv_usec >=timenow.tv_nsec/1000){
					timenow.tv_nsec =
						(ip->i_mtime.tv_usec+1) * 1000;
					if (timenow.tv_nsec >= NANOSEC) {
						timenow.tv_sec++;
						timenow.tv_nsec = 0;
					}
				}
				ip->i_mtime.tv_sec = timenow.tv_sec;
				ip->i_mtime.tv_usec = timenow.tv_nsec/1000;
			}
			if (mtime == B_TRUE) {
				ip->i_ctime.tv_sec = timenow.tv_sec;
				ip->i_ctime.tv_usec = timenow.tv_nsec/1000;
				ip->i_flag &= ~(IUPD | ICHG);
				ip->i_flag |= IMODTIME;
			}
		}

		if (mask & AT_ATIME) {
			if (flags & (ATTR_UTIME | ATTR_UPDTIME)) {
				if ((flags & ATTR_UPDTIME) &&
					(vap->va_mtime.tv_sec <=
						ip->i_mtime.tv_sec)) {
					atime = B_FALSE;
				} else {
					ip->i_atime.tv_sec =
						vap->va_atime.tv_sec;
					ip->i_atime.tv_usec =
						vap->va_atime.tv_nsec/1000;
				}
			} else {
				ip->i_atime.tv_sec = timenow.tv_sec;
				ip->i_atime.tv_usec = timenow.tv_nsec/1000;
			}
			if (atime == B_TRUE)
				ip->i_flag &= ~IACC;
		}
		if (mtime == B_TRUE || atime == B_TRUE)
			ip->i_flag |= IMOD;
		SFS_IUNLOCK(ip, pl);
	}
out:
	if (!(flags & ATTR_EXEC) && !error) {
		sfs_iupdat(ip, issync ? IUP_SYNC : IUP_LAZY);
	}

	SFS_IRWLOCK_UNLOCK(ip);
	return (error);
}

/*
 * int
 * sfs_access(vnode_t *vp, int mode, int flags, cred_t *cr)
 *	Determine the accessibility of a file to the calling
 *	process.
 *
 * Calling/Exit State:
 *	No locks held on entry; no locks held on exit.
 *	The inode's rwlock is held *shared* while determining
 *	accessibility of the file to the caller.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		EIO	The file system containing <vp> has been invalidated
 *			and is unaccessible.
 *
 * Description:
 *	If the file system containing <vp> has not been sealed then
 *	obtain the inode shared/exclusive lock in shared mode. Use
 *	sfs_iaccess() to determine accessibility. Return what it does
 *	after releasing the shared/exclusive lock.
 */
/*ARGSUSED*/
STATIC int
sfs_access(vnode_t *vp, int mode, int flags, cred_t *cr)
{
	int		error;
	inode_t		*ip;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;

	vfsp = vp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		error = EIO;
	} else {
		ip = VTOI(vp);
		SFS_IRWLOCK_RDLOCK(ip);
		error = sfs_iaccess(ip, mode, cr);
		SFS_IRWLOCK_UNLOCK(ip);
	}

	return (error);
}

/*
 * int
 * sfs_readlink(vnode_t *vp, struct uio *uiop, cred_t *cr)
 *	Read a symbolic link file.
 *
 * Calling/Exit State:
 *	No inode/vnode locks are held on entry or exit.
 *
 *	On success, 0 is returned; otherwise, a valid errno is
 *	returned. Errnos returned directly by sfs_readlink are:
 *		EINVAL	The vnode is not a symbolic link file.
 *
 *		EIO	The file system containing the symbolic link
 *			has been invalidated.
 */
/* ARGSUSED */
STATIC int
sfs_readlink(vnode_t *vp, struct uio *uiop, cred_t *cr)
{
	inode_t		*ip;
	int		error;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;

	vfsp = vp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		error = EIO;
	} else if (vp->v_type == VLNK) {
		ip = VTOI(vp);
		SFS_IRWLOCK_RDLOCK(ip);
		error = sfs_readi(ip, uiop, 0, cr);
		SFS_IRWLOCK_UNLOCK(ip);
	} else {
		error = EINVAL;
	}

	return (error);
}

/*
 * sfs_fsync(vnode_t *vp, cred_t *cr)
 *	Synchronously flush a file's modified pages to it's
 *	backing store.
 *
 * Calling/Exit State:
 *	No locks held on entry; no locks held on exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		EIO	The file system containing <vp> has been sealed.
 *
 * Description:
 *	If the file system containing <vp> has not been sealed then
 *	obtain the inode sleep lock. Use sfs_syncip() to flush the
 *	file's pages. If that completes successfully, then
 *	update the inode's times. Return any errors
 *	after releasing the sleep lock.
 */
/* ARGSUSED */
STATIC int
sfs_fsync(vnode_t *vp, cred_t *cr)
{
	inode_t		*ip;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;
	int		error;

	if (vp->v_flag & VNOSYNC)
		return (0);

	vfsp = vp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		error = EIO;
	} else {
		ip = VTOI(vp);
		SFS_IRWLOCK_RDLOCK(ip);
		error = sfs_syncip(ip, 0, IUP_SYNC); /* Do synchronous writes */
		SFS_IRWLOCK_UNLOCK(ip);
	}

	return (error);
}

/*
 * sfs_inactive(vnode_t *vp, cred_t *cr)
 *	Perform cleanup on an unreferenced inode.
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 *	This function may block.
 */
STATIC void
sfs_inactive(vnode_t *vp, cred_t *cr)
{
	inode_t   	*ip = VTOI(vp);
	boolean_t	totally_free;
	struct inode_marker *free_list;
	int		flags;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * At this point, a new reference to the inode can be still be
	 * generated by one of:
	 * 
	 *	=> sfs_tryhold
	 *
	 *	=> sfs_iget
	 *
	 *	=> sfs_igrab
	 *
	 *	=> dnlc_lookup (SOFT DNLC case only)
	 *
	 * In all four cases, the LWP generating the reference holds
	 * either the VN_LOCK or the sfs_inode_table_mutex. So, acquire both
	 * locks now. We also acquire the sfs_inode_table_mutex so that we
	 * can transition the free/destroy state of the inode atomically with
	 * changing its v_count/v_softcnt fields.
	 */
	(void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
	ASSERT(ip->i_state == IDENTITY);
	VN_LOCK(vp);
	ASSERT(vp->v_count != 0);
	if (vp->v_count != 1) {
		/*
		 * Someone generated a new reference before we acquired
		 * both locks.
		 *
		 * The new reference is still extant, so that inactivating
		 * the vnode will become someone else's responsibility. Give
		 * up our reference and return.
		 */
		vp->v_count--;
		VN_LOG(vp, "sfs_inactive-1");
		VN_UNLOCK(vp);
		UNLOCK(&sfs_inode_table_mutex, PLBASE);
		return;
	}

	/*
	 * The reference count is exactly 1, so that now we can be sure
	 * that we really hold the last hard reference.
	 *
	 * We exchange our hard count (v_count) for a soft count (v_softcnt)
	 * in order to suppress any new VN_HOLDS via dnlc_lookup once we
	 * give up the VN_LOCK. Even after we give up the VN_LOCK, we will
	 * still hold the sfs_inode_table_mutex, thus inhibiting any
	 * new references via sfs_iget, sfs_igrab, or sfs_tryhold.
	 *
	 * Note: The pages remain visible to the pageout and fsflush
	 *	 daemons, which can generate soft holds on the vnode.
	 *	 Also, dnlc_lookup can still generate soft holds.
	 */
	vp->v_count = 0;
	++vp->v_softcnt;
	VN_LOG(vp, "sfs_inactive-2");
	VN_UNLOCK(vp);

	/*
	 * If we are removing the file, or if we wish to kmem_free the
	 * storage (because we are over the low water mark), then we
	 * commit to destroying the inode's identity.
	 *
	 * Otherwise, we just free the inode and let the fsflush/pageout
	 * daemons clean the pages/inode.
	 *
	 * The unlocked tests of v_softcnt and v_pages are permitted,
	 * as the result is used only for an optimization, not for
	 * correctness.
	 */
	totally_free = (vp->v_softcnt == 1 && vp->v_pages == NULL);
	if (ip->i_nlink <= 0) {
		ip->i_state = (inostats.freeinodes > sfs_inode_lwm) ?
				(IDENTITY|INVALID|IKMFREE) :
				(IDENTITY|INVALID|ILCLFREE);
	} else if (totally_free && inostats.freeinodes > sfs_inode_lwm) {
		ip->i_state = (IDENTITY|INVALID|IKMFREE);
	} else {
		/*
		 * Freeing the inode but not destroying it.
		 */
		if (totally_free) {
			free_list = &sfs_totally_free;
			flags = ITFREE;
			inostats.freeinodes++;
			MET_INODE_INUSE(MET_SFS,-1);
		} else {
			free_list = &sfs_partially_free;
			flags = IPFREE;
		}
		SFS_FREE_TAIL(free_list, ip, flags);
		ASSERT(ip->i_state == (IDENTITY|flags));
		SFS_FREE_CHECK();
		UNLOCK(&sfs_inode_table_mutex, PLBASE);
		return;
	}
	UNLOCK(&sfs_inode_table_mutex, PLBASE);
	SFS_IRWLOCK_WRLOCK(ip);

	/*
	 * At this point, we are committed to destroying the
	 * inode's identity. If destruction fails, then the inode
	 * will go onto the free list. No error handling is needed
	 * here.
	 */
	(void) sfs_idestroy(ip, cr);
}

/*
 * STATIC void
 * sfs_release(vnode_t *vp)
 *	Release the storage for a totally unreferenced vnode.
 *
 * Calling/Exit State:
 *	The user may hold locks. However, no lock is held at FS_SFSLISTPL
 *	or above.
 *
 *	This function does not block.
 */
void
sfs_release(vnode_t *vp)
{
	inode_t   	*ip = VTOI(vp);
	pl_t		s;

	/*
	 * The inode is privately held at this point.
	 * Therefore, no locking is necesary in order to inspect it.
	 */
	ASSERT(VN_IS_RELEASED(ITOV(ip)));
	ASSERT(ITOV(ip)->v_pages == NULL);

	/*
	 * If the inode is marked for kmem_free, then do just that.
	 * Else if inode is destined for the free list, then free it.
	 *
	 * It is possible that the number of inodes has dropped above or
	 * below the low water mark since we decided on how to dispose of
	 * this inode's space. But, since it's just a rough guideline, we
	 * don't check again.
	 */
	if (ip->i_state == (INVALID|IKMFREE)) {
		SFS_DEINIT_INODE(ip);
		if (ip->is_union != NULL)
			kmem_free(ip->is_union, sizeof(union i_secure));
		kmem_free(ip, sizeof(struct inode));
		s = LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
		inostats.totalinodes--;
		UNLOCK(&sfs_inode_table_mutex, s);
		MET_INODE_CURRENT(MET_SFS, -1);
		MET_INODE_INUSE(MET_SFS, -1);
	} else if (ip->i_state == (INVALID|ILCLFREE)) {
		ITOV(ip)->v_softcnt = 1;
		ip->i_state = 0;
		s = LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
		inostats.freeinodes++;
		SFS_FREE_HEAD(&sfs_totally_free, ip, ITFREE);
		SFS_FREE_CHECK();
		UNLOCK(&sfs_inode_table_mutex, s);
		MET_INODE_INUSE(MET_SFS,-1);
	} else {
		ASSERT(ip->i_state == INVALID);
	}

	/*
	 * Remember to wake up any waiters
	 */
	if (SV_BLKD(&sfs_inode_sv))
		SV_BROADCAST(&sfs_inode_sv, 0);

	return;
}

/*
 * Unix file system operations having to do with directory manipulation.
 */

/*
 * int
 * sfs_lookup(vnode_t *dvp, char *nm, vnode_t **vpp, pathname_t *pnp,
 *	      int lookup_flags, vnode_t *rootvp, cred_t *cr)
 *	Check whether a given directory contains a file named <nm>.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 *	A return value of 0 indicates success; otherwise, a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EIO	file system containing directory has been invalidated
 *			and is unaccessible.
 *		ENOSYS	The file found in the directory is a special file but
 *			SPECFS was unable to create a vnode for it.
 * 
 * Description:
 *	We treat null components as a synonym for the directory being
 *	searched. In this case, merely increment the directory's reference
 *	count and return. For all other cases, search the directory via
 *	sfs_dirlook. If we find the given file, indirect to SPECFS if
 *	it's a special file. 
 */
STATIC int
sfs_lookup(vnode_t *dvp, char *nm, vnode_t **vpp, pathname_t *pnp,
	   int lookup_flags, vnode_t *rootvp, cred_t *cr)
{
	inode_t		*dp;
	inode_t		*ip;
	int		error;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;

	vfsp = dvp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	/*
	 * Treat null component names as a synonym for the directory
	 * being searched.
	 */
	if (*nm == '\0') {
		VN_HOLD(dvp);
		*vpp = dvp;
		return (0);
	}

	dp = VTOI(dvp);
	error = sfs_dirlook(dp, nm, &ip, 0, cr);

	/*
	 * If we find the file in the directory, the inode is returned
	 * referenced with the rwlock held *shared*.
	 */
	if (error == 0) {
		*vpp = ITOV(ip);
		SFS_IRWLOCK_UNLOCK(ip);

		/*
		 * If vnode is a device return special vnode instead.
		 */
		if (ISVDEV((*vpp)->v_type)) {
			vnode_t	*newvp;

			newvp = specvp(*vpp, (*vpp)->v_rdev,(*vpp)->v_type, cr);
			VN_RELE(*vpp);
			if (newvp == NULL) {
				error = ENOSYS;
			} else {
				*vpp = newvp;
			}
		}
	}
	return (error);
}

/*
 * int
 * ufs_lookup(vnode_t *dvp, char *nm, vnode_t **vpp, pathname_t *pnp,
 *	      int lookup_flags, vnode_t *rootvp, cred_t *cr)
 *	Check whether a given directory contains a file named <nm>.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 *	A return value of 0 indicates success; otherwise, a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EIO	file system containing directory has been invalidated
 *			and is unaccessible.
 *		ENOSYS	The file found in the directory is a special file but
 *			SPECFS was unable to create a vnode for it.
 * 
 * Description:
 *	Similar to sfs_lookup, but contains a ``fast path'' optimization
 *	for the dnlc_lookup/sfs_iaccess section.
 */
STATIC int
ufs_lookup(vnode_t *dvp, char *nm, vnode_t **vpp, pathname_t *pnp,
	   int lookup_flags, vnode_t *rootvp, cred_t *cr)
{
	inode_t		*dp;
	inode_t		*ip;
	int		error;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;
	vnode_t		*vp, *newvp;
	mode_t		iexec;
	void		*agen;
	boolean_t	softhold;

	/*
	 * Treat null component names as a synonym for the directory
	 * being searched.
	 */
	if (*nm == '\0') {
		VN_HOLD(dvp);
		*vpp = dvp;
		return (0);
	}

	dp = VTOI(dvp);
	vp = dnlc_lookup(dvp, nm, &agen, &softhold, NOCRED);
#ifdef _SFS_SOFT_DNLC
	if (vp && (!softhold || sfs_tryhold(vp))) {
#else	/* !_SFS_SOFT_DNLC */
	if (vp) {
		ASSERT(!softhold);
#endif	/* !_SFS_SOFT_DNLC */
		/*
		 * The following is in inline expansion of a portion of
		 * sfs_iaccess(dp, IEXEC, cr). It doesn't check for P_DACREAD
		 * privledge, or know how to generate ``permission
		 * denied'' error codes. But, it doesn't need to, since we
		 * will call sfs_dirlook should this optimization fail.
		 *
		 * Note that the RW lock on the directory is not held
		 * during the access check. This is okay because:
		 *	(a) Since we do not follow any unmutexed pointers
		 *	    here, we cannot address fault, and
		 *	(b) We will be able to detect permission changes
		 *	    using the access right generation number
		 *	    (i_agen) in the inode.
		 */
		if (cr->cr_uid == dp->i_uid)
			iexec = dp->i_mode & IEXEC;
		else if (groupmember(dp->i_gid, cr))
			iexec = dp->i_mode & (IEXEC >> 3);
		else
			iexec = dp->i_mode & (IEXEC >> 6);

		if (iexec) {
			/*
			 * Make sure the the access right associated with
			 * this inode have not changed since the entry was
			 * made in dnlc.
			 */
			READ_SYNC();
			if ((int) agen == dp->i_agen)
				goto found_it;
		}
		VN_RELE(vp);
	}

	vfsp = dvp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	error = sfs_dirlook(dp, nm, &ip, 0, cr);
	if (error)
		return error;

	/*
	 * If we find the file in the directory, the inode is returned
	 * referenced with the rwlock held *shared*.
	 */
	SFS_IRWLOCK_UNLOCK(ip);
	vp = ITOV(ip);

found_it:
	/*
	 * If vnode is a device return special vnode instead.
	 */
	if (ISVDEV(vp->v_type)) {
		newvp = specvp(vp, vp->v_rdev, vp->v_type, cr);
		VN_RELE(vp);
		if (newvp == NULL)
			return ENOSYS;
		vp = newvp;
	}

	*vpp = vp;

	return 0;
}

/*
 * int
 * sfs_create(vnode_t *dvp, char *name, vattr_t *vap, enum vcexcl excl,
 *            int mode, vnode_t **vpp, cred_t *cr)
 *	Create a file in a given directory.
 *
 * Calling/Exit State:
 *	The vnode's shared/exclusive sleep lock is held on entry
 *	to prevent the file's security level from changing.
 *
 * Description:
 *	If the name of the file to create is null, it is treated as
 *	a synonym for the directory that the file is to be created in.
 *	In this case, the directory's rwlock is obtained in shared mode
 *	to check the calling LWP's access permission to the directory. The
 *	lock is necessary to prevent the permission vector from being
 *	changed.
 *	
 *	If name is not null, sfs_direnter will create a directory entry
 *	for the new file atomically by holding the directory inode's
 *	rwlock exclusive. The directory is unlocked before returning
 *	to this routine. On success, the new inode is returned with
 *	it's rwlock held exclusive. The lock is held exclusive since
 *	the new entry may require truncation (iff AT_SIZE is specified).
 *	
 *
 */
/* ARGSUSED */
STATIC int
sfs_create(vnode_t *dvp, char *name, vattr_t *vap, enum vcexcl excl,
	   int mode, vnode_t **vpp, cred_t *cr)
{
	int		error;
	inode_t		*dp;
	inode_t		*ip;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;

	vfsp = dvp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	/* must be privileged to set sticky bit */
	if ((vap->va_mode & VSVTX) && pm_denied(cr, P_OWNER)) {
		vap->va_mode &= ~VSVTX;	
	}

	dp = VTOI(dvp);
	if (*name == '\0') {
		/*
		 * Null component name refers to the directory itself.
		 */
		VN_HOLD(dvp);
		SFS_IRWLOCK_RDLOCK(dp);
		error = EEXIST;
		ip = dp;
	} else {
		if (ISVDEV(vap->va_type)) {
			/*
			* Try to catch any specfs problems before writing
			* directory.
			*/
			if (error = specpreval(vap->va_type, vap->va_rdev, cr))
				return error;
		}
		/*
		 * On success, ip is returned with it's rwlock held
		 * exclusive
		 */
		ip = NULL;
		error = sfs_direnter(dp, name, DE_CREATE, (inode_t *) 0,
				     (inode_t *) 0, vap, &ip, cr);
	}

	/*
	 * If the file already exists and this is a non-exclusive create,
	 * check permissions and allow access for non-directories.
	 * Read-only create of an existing directory is also allowed.
	 * We fail an exclusive create of anything which already exists.
	 */
	if (error == EEXIST) {
		 struct vnode *vp = ITOV(ip);
                /*
                 * MAC write checks are necessary in sfs dependent
                 * code because the time between lookup and VOP_CREATE
                 * at independent level is rather long.  This
                 * particular check is here because the "mode"
                 * argument is not passed along to sfs_direnter().
                 * Other MAC checks are performed in sfs_direnter().
                 */
                if (vp->v_type != VCHR &&
                    vp->v_type != VBLK &&
                    MAC_VACCESS(vp, mode, cr)) {
                        error = EACCES;
		} else if (excl == NONEXCL) {
			if (((ip->i_mode & IFMT) == IFDIR) && (mode & IWRITE)) {
				error = EISDIR;
			} else if (mode) {
				error = sfs_iaccess(ip, mode, cr);
			} else {
				error = 0;
			}
		}
		if (error) {
			sfs_iput(ip, cr);
		} else if (((ip->i_mode & IFMT) == IFREG)
		    && (vap->va_mask & AT_SIZE) && vap->va_size == 0) {
			/*
			 * Truncate regular files, if requested by caller.
			 * No need to do this if error ! = EEXIST since
			 * sfs_direnter made a new inode. Also, no need
			 * to update the version code if error ! = EEXIST
			 * for the same reason.
			 */
			if (MANDLOCK(vp, ip->i_mode) &&
			    (ITOV(ip)->v_filocks != NULL)) {
				sfs_iput(ip, cr);
				error = EAGAIN;
			} else {
				error = sfs_itrunc(ip, (u_long)0, cr);
				if (error == 0) {
					error = fs_vcode(ITOV(ip),
							 &ip->i_vcode);
				} else
					SFS_IRWLOCK_UNLOCK(ip);
			}
		}
	}

	if (!error) {
		*vpp = ITOV(ip);
		SFS_IRWLOCK_UNLOCK(ip);
		/*
		 * If vnode is a device return special vnode instead.
		 */
		if (ISVDEV((*vpp)->v_type)) {
			vnode_t *newvp;
			newvp = specvp(*vpp,(*vpp)->v_rdev, (*vpp)->v_type, cr);
			VN_RELE(*vpp);
			if (newvp != NULL) {
				*vpp = newvp;
			} else {
				/*
				* Note in this case, directory entry isn't
				* cleaned up. That's why we try to catch
				* specfs problems earlier with call to
				* specpreval().
				*/
				error = ENOSYS;
			}
		}
	}

	return (error);
}

/*
 * int
 * sfs_remove(vnode_t *dvp, char *nm, cred_t *cr)
 *	Remove a file from a directory.
 *
 * Calling/Exit State:
 *	The caller holds no inode locks on entry
 *	or exit.
 *
 * Description:
 *	Sfs_remove uses sfs_dirremove to remove the directory
 *	while holding the containing directory's rwlock in
 *	exclusive mode. 
 */
/*ARGSUSED*/
STATIC int
sfs_remove(vnode_t *dvp, char *nm, cred_t *cr)
{
	inode_t		*ip;
	int		error;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;

	vfsp = dvp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		error = EIO;
	 } else {
		ip = VTOI(dvp);
		error = sfs_dirremove(ip, nm, (inode_t *)0,
				      (vnode_t *)0, DR_REMOVE, cr);
	}

	return (error);
}

/*
 * int
 * sfs_link(vnode_t *tdvp, vnode_t *svp, char *tnm, cred_t *cr)
 *	Link a file or a directory.  Only a privileged user is allowed
 * 	to make a link to a directory.
 *
 * Calling/Exit State:
 *	No vnode/inode locks are held on entry or exit.
 *
 * Description:
 *	Sfs_direnter adds a link to the source file in the
 *	target directory atomically. If the source file is a directory,
 *	the caller must have privelege to perform the operation (this
 *	is determined by pm_denied). The target directory's rwlock is
 *	held exclusive for the udration of the operation to prevent
 *	other LWP's from accessing the target directory. The
 *	source inode's rwlock is held exclusive while it's link count
 *	is incremented.
 *	Hard links to directories are no longer allowed.
 */
/* ARGSUSED */
STATIC int
sfs_link(vnode_t *tdvp, vnode_t *svp, char *tnm, cred_t *cr)
{
	inode_t		*sip;
	inode_t		*tdp;
	int		error;
	vnode_t		*realvp;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;

#ifdef CC_PARTIAL
        MAC_GIVESVAL(svp->v_op->vop_link, sfs_link);
#endif

	vfsp = svp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		error = EIO;
	} else {
		if (VOP_REALVP(svp, &realvp) == 0) {
			svp = realvp;
		}
		if (svp->v_type == VDIR)
			return EPERM;
		sip = VTOI(svp);
		tdp = VTOI(tdvp);
		error = sfs_direnter(tdp, tnm, DE_LINK,
				     (inode_t *) 0, sip,
				     (vattr_t *) 0,
				     (inode_t **)0, cr);
	
	}

	return (error);
}

/*
 * int
 * sfs_rename(vnode_t *sdvp, char *snm, vnode_t *tdvp, char *tnm, cred_t *cr)
 *	Rename a file or directory.
 * 
 * Calling/Exit State:
 *	The caller holds no vnode/inode locks on entry or exit.
 *
 * Description:
 *	We are given the vnode and entry string of the source and the
 *	vnode and entry string of the place we want to move the source
 *	to (the target). The essential operation is:
 *		unlink(target);
 *		link(source, target);
 *		unlink(source);
 *	but "atomically".  Can't do full commit without saving state in
 *	the inode on disk, which isn't feasible at this time.  Best we
 *	can do is always guarantee that the TARGET exists.
 *
 *	The rename is performed by a combination of sfs_dirlook, sfs_direnter
 *	and sfs_dirremove. Sfs_dirlook is used to locate the file being
 *	renamed. If the file exists, it is returned with it's rwlock
 *	held shared; this lock is removed before calling sfs_direnter.
 *	Sfs_direnter holds the target directory's rwlock exclusive and
 *		o removes the entry for the target file (if exists) from
 *		  the target directory
 *		o adds and entry for the source file to the target directory
 *		o if the source file is a directory, the source file's
 *		  parent directory's link count is adjusted.
 *	This last step introduces the potential for A->B/B->A deadlock since
 *	both the source file's parent directory and the target directory's
 *	rwlock must be held. Deadlock would result if a rename occurs in the
 *	opposite directory, i.e., with the parent directories reversed. To
 *	prevent deadlock, all rename operations where the source file
 *	is a directory and the parent directory changes must be serialized.
 *	Serialization occurs via a per-file system sleep lock.
 *
 */
/*ARGSUSED*/
STATIC int
sfs_rename(vnode_t *sdvp, char *snm, vnode_t *tdvp, char *tnm, cred_t *cr)
	/* sdvp - old (source) parent vnode */
	/* snm - old (source) entry name */
	/* tdvp - new (target) parent vnode */
	/* tnm - new (target) entry name */
	/* cr - credentials for LWP*/
{
	inode_t		*sip;	/* source inode */
	inode_t		*sdp;	/* old (source) parent inode */
	inode_t		*tdp;	/* new (target) parent inode */
	int		error;
	int		have_write_error;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;


#ifdef CC_PARTIAL
        MAC_GIVESVAL(tdvp->v_op->vop_rename, sfs_rename);
#endif

	vfsp = sdvp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	sdp = VTOI(sdvp);
	tdp = VTOI(tdvp);
	/*
	 * Get inode for source file.
	 */
	error = sfs_dirlook(sdp, snm, &sip, 0, cr);
	if (error) {
		return (error);
	} else {
		have_write_error = sfs_iaccess(sip, IWRITE, cr);
		SFS_IRWLOCK_UNLOCK(sip);
	}

	/*
	 * Make sure we can delete the source file.  This requires
	 * write permission on the containing directory.  If that
	 * directory is "sticky" it further requires (except for a
	 * privileged user) that the user own the directory or the source 
	 * entry, or else have permission to write the source entry.
	 */
	SFS_IRWLOCK_RDLOCK(sdp);
	error = sfs_iaccess(sdp, IWRITE, cr);
	if (!error) {
		if ((sdp->i_mode & ISVTX) &&
		     cr->cr_uid != sdp->i_uid &&
		     cr->cr_uid != sip->i_uid &&
		     pm_denied(cr, P_OWNER) &&
		     have_write_error) {
			error = have_write_error;
		}
	}
	SFS_IRWLOCK_UNLOCK(sdp);

	if (error) {
		VN_RELE_CRED(ITOV(sip), cr);
		return (error);
	}

	/*
	 * Check for renaming '.' or '..' or alias of '.'
	 */
	if (strcmp(snm, ".") == 0 || strcmp(snm, "..") == 0 || sdp == sip) {
		error = EINVAL;
		goto out;
	}

	/*
	 * Link source file to the target directory.
	 */
	error = sfs_direnter(tdp, tnm, DE_RENAME, sdp, sip,
			      (vattr_t *)0, (inode_t **)0, cr);
	if (error) {
		if (error == ESAME) 
			/*
			 * ESAME isn't really an error; this is an attempt
			 * to rename one link of a file to another, or rename
			 * a file to itself. POSIX considers this to be a NOP.
			 * See section 5.5.3.2 of the POSIX 1003.1-1988
			 * standard.
			 */
			error = 0;

		goto out;
	}

	/*
	 * Unlink the source entry. sfs_dirremove() checks that the entry
	 * still reflects <sip>, and returns an error if it doesn't.
	 * If the entry has changed just forget about it. Release
	 * the source inode.
	 */
	error = sfs_dirremove(sdp, snm, sip, (vnode_t *)0, DR_RENAME, cr);
	if (error == ENOENT) {
		error = 0;
	}

out:
	VN_RELE_CRED(ITOV(sip), cr);
	return (error);
}

/*
 * int
 * sfs_mkdir(vnode_t *dvp, char *dirname, vattr_t *vap,
 *           vnode_t **vpp, cred_t *cr)
 *	Create a directory file.
 *
 * Calling/Exit State:
 *	Caller holds no vnode/inode locks on entry or exit.
 *
 * Description:
 *	Sfs_direnter creates a directory in <dvp> while holding
 *	the containing directory's rwlock in exclusive mode. On
 *	successful sfs_direnter()'s, the newly created directory
 *	is returned with it's rwlock held exclusive.
 */
/*ARGSUSED*/
STATIC int
sfs_mkdir(vnode_t *dvp, char *dirname, vattr_t *vap, vnode_t **vpp, cred_t *cr)
{
	inode_t		*dp;
	inode_t		*ip;
	int		error;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;

	vfsp = dvp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	ASSERT((vap->va_mask & (AT_TYPE|AT_MODE)) == (AT_TYPE|AT_MODE));

	dp = VTOI(dvp);
	error = sfs_direnter(dp, dirname, DE_MKDIR, (inode_t *) 0,
			    (inode_t *) 0, vap, &ip, cr);
	if (error == 0) {
		*vpp = ITOV(ip);
		SFS_IRWLOCK_UNLOCK(ip);
	} else if (error == EEXIST) {
		sfs_iput(ip, cr);
	}

	return (error);
}

/*
 * int
 * sfs_rmdir(vnode_t *vp, char *nm, vnode_t *cdir, cred_t *cr)
 *	Remove a diretory.
 *
 * Calling/Exit State:
 *	The caller holds no vnode/inode locks on entry or exit.
 */
/*ARGSUSED*/
STATIC int
sfs_rmdir(vnode_t *vp, char *nm, vnode_t *cdir, cred_t *cr)
{
	inode_t		*ip;
	int		error;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;

	vfsp = vp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		error = EIO;
	} else {
		ip = VTOI(vp);
		error = sfs_dirremove(ip, nm, (inode_t *)0,
				      cdir, DR_RMDIR, cr);
	}

	return (error);
}

/*
 * int
 * sfs_readdir(vnode_t *vp, struct uio *uiop, cred_t *fcr, int *eofp)
 *	Read from a directory.
 *
 * Calling/Exit State:
 *	The calling LWP holds the inode's rwlock in *shared* mode. The
 *	rwlock was obtained by a call to VOP_RWRDLOCK. 
 *
 *	A return value of not -1 indicates success; otherwise a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		EIO	The file system containing <vp> has been
 *			invalidated and is therefore unaccessible.
 *		ENOTDIR	<vp> is not a directory.
 *		ENXIO	
 *
 *	On success, an <*eofp> value of 1 indicates that end-of-file
 *	has been reached, i.e., there are no more directory entries
 *	that may be read.
 */
/* ARGSUSED */
STATIC int
sfs_readdir(vnode_t *vp, struct uio *uiop, cred_t *fcr, int *eofp)
{
	struct cred	*cr = VCURRENTCRED(fcr);
	struct iovec	*iovp;
	inode_t		*ip;
	struct direct	*idp;
	struct dirent	*odp;
	off_t		offset;
	int		incount;
	int		outcount;
	int		remaining;
	uint_t		bytes_wanted;
	uint_t		total_bytes_wanted;
	caddr_t		outbuf;
	size_t		bufsize;
	int		error;
	struct fbuf	*fbp;
	int		direntsz;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;
	pl_t		opl;

	vfsp = vp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	if (vp->v_type != VDIR) {
		return (ENOTDIR);
	}

	iovp = uiop->uio_iov;
	total_bytes_wanted = iovp->iov_len;
	if (total_bytes_wanted == 0) {
		return (0);
	}

	/*
         * Make sure that total_bytes_wanted is at least one
         * dirent struct in size. If it is less than this then
         * the system would hang as sfs_readdir() would get
         * into a loop of fbread() and fbrelse() calls.
         */
        if (total_bytes_wanted < sizeof(struct dirent))
                return(EINVAL);

	ip = VTOI(vp);
	incount = outcount = 0;
	error = 0;

	/*
	 * Force offset to be valid to guard against bogus lseek()
	 * values by rounding down.
	 */
	offset = uiop->uio_offset & ~(DIRBLKSIZ - 1);

	/* Quit if at end of file */
	if (offset >= ip->i_size) {
		if (eofp) {
			*eofp = 1;
		}
		return (0);
	}

	/*
	 * Get space to change directory entries into fs independent format.
	 */
	bufsize = total_bytes_wanted + sizeof (struct dirent) + MAXNAMELEN;
	outbuf = (caddr_t) kmem_alloc(bufsize, KM_SLEEP);
	/* LINTED pointer alignment */
	odp = (struct dirent *)outbuf;
	direntsz = (char *) odp->d_name - (char *) odp;

nextblk:
	bytes_wanted = total_bytes_wanted;

	/* Truncate request to file size */
	if (offset + bytes_wanted > ip->i_size) {
		bytes_wanted = ip->i_size - offset;
	}

	/* Comply with MAXBSIZE boundary restrictions of fbread() */
	if ((offset & MAXBOFFSET) + bytes_wanted > MAXBSIZE) {
		bytes_wanted = MAXBSIZE - (offset & MAXBOFFSET);
	}

	/* Read in the next chunk */
	error = fbread(vp, (long)offset, bytes_wanted, S_OTHER, &fbp);
	if (error) {
		goto out;
	}

	if (WRITEALLOWED(vp, cr)) {
		opl = SFS_ILOCK(ip);
		IMARK(ip, IACC);
		SFS_IUNLOCK(ip, opl);
	}

	incount = 0;
	/* LINTED pointer alignment */
	idp = (struct direct *)fbp->fb_addr;

	/* Transform to file-system independent format */
	while (incount < bytes_wanted) {
		remaining = SFS_DIR_REMAINING(offset);
		if (SFS_DIR_MANGLED(remaining, idp)) {
			sfs_dirbad(ip, "mangled entry", offset);
			fbrelse(fbp, 0);
			error = ENXIO;
			goto out;
		}
		/* Skip to requested offset and skip empty entries */
		if (idp->d_ino != 0 && offset >= uiop->uio_offset) {
			odp->d_ino = idp->d_ino;
			odp->d_reclen = (direntsz + idp->d_namlen + 1 + (NBPW-1)) & ~(NBPW-1);
			odp->d_off = offset + idp->d_reclen;
			strcpy(odp->d_name, idp->d_name);
			outcount += odp->d_reclen;
			/* Got as many bytes as requested, quit */
			if (total_bytes_wanted < outcount) {
				outcount -= odp->d_reclen;
				ASSERT(outcount >= sizeof(struct dirent));
				break;
			}
			odp = (struct dirent *)((int)odp + odp->d_reclen);
		}
		incount += idp->d_reclen;
		offset += idp->d_reclen;
		idp = (struct direct *)((int)idp + idp->d_reclen);
	}
	/* Release the chunk */
	fbrelse(fbp, 0);
	/* Read whole block, but got no entries, read another if not eof */
	if (offset < ip->i_size && !outcount) {
		goto nextblk;
	}

	/* Copy out the entry data */
	error = uiomove(outbuf, (long)outcount, UIO_READ, uiop);
	if (error) {
		goto out;
	}

	uiop->uio_offset = offset;
out:
	kmem_free((void *)outbuf, bufsize);

	if (eofp && error == 0) {
		*eofp = (uiop->uio_offset >= ip->i_size);
	}

	return (error);
}

/*
 * int
 * sfs_rdwri(enum uio_rw rw, inode_t *ip, caddr_t base, int len,
 *	SFS specific routine used to do sfs io.
 *
 * Calling/Exit State:
 *	The caller must hold the inode's rwlock in at least *shared*
 *	mode if doing a read; *exclusive* mode  must be specified
 *	when doing a write.
 */
int
sfs_rdwri(enum uio_rw rw, inode_t *ip, caddr_t base, int len,
	  off_t offset, enum uio_seg seg, int *aresid, cred_t *cr)
{
	struct uio	auio;
	struct iovec	aiov;
	int		error;

	aiov.iov_base = base;
	aiov.iov_len = len;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = offset;
	auio.uio_segflg = (short)seg;
	auio.uio_limit = u.u_rlimits->rl_limits[RLIMIT_FSIZE].rlim_cur;
	auio.uio_resid = len;
	if (rw == UIO_WRITE) {
		auio.uio_fmode = FWRITE;
		error = sfs_writei(ip, &auio, 0, cr);
	} else {
		auio.uio_fmode = FREAD;
		error = sfs_readi(ip, &auio, 0, cr);
	}

	if (aresid) {
		*aresid = auio.uio_resid;
	} else if (auio.uio_resid) {
		error = EIO;
	}

	return (error);
}

/*
 * int
 * sfs_symlink(vnode_t *dvp, char *linkname, vattr_t *vap,
 *	Create a symbolic link file.
 *
 * Calling/Exit State:
 *	Caller holds no vnode/inode locks on entry or exit.
 *
 * Description:
 *	Sfs_direnter creates a symbolic link in <dvp> while
 *	holding the containing directory's rwlock in exclusive
 *	mode. If the symbolic link is created, it's inode is
 *	returned with the rwlock held exclusive. This lock is
 *	held while the contents for the symbolic link are
 *	written.
 */
/*ARGSUSED*/
STATIC int
sfs_symlink(vnode_t *dvp, char *linkname, vattr_t *vap,
	    char *target, cred_t *cr)
	/* dvp - ptr to parent dir vnode */
	/* linkname - name of symbolic link */
	/* vap - attributes */
	/* target - target path */
	/* cr - user credentials */
{
	inode_t		*ip;
	inode_t		*dip;
	int		error;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;

	vfsp = dvp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	vap->va_type = VLNK;
	vap->va_rdev = 0;

	ip = (inode_t *)0;
	dip = VTOI(dvp);
	error = sfs_direnter(dip, linkname, DE_CREATE, (inode_t *)0,
			     (inode_t *)0, vap, &ip, cr);
	if (error == 0) {
		error = sfs_rdwri(UIO_WRITE, ip, target, (int)strlen(target),
				  (off_t)0, UIO_SYSSPACE, (int *)0, cr);

		sfs_iput(ip, cr);
	} else if (error == EEXIST) {
		sfs_iput(ip, cr);
	}

	return (error);
}


/*
 * int
 * sfs_fid(vnode_t *vp, struct fid **fidpp)
 *	Return a unique identifier for the given file.
 *
 * Calling/Exit State:
 *	No inode/vnode locks are held on entry or exit.
 *
 * Description:
 *	The file identifier for the vnode is generated from the invariant
 *	fields of the inode. Thus, the fields are simply copied without
 *	any locking.
 */
/* ARGSUSED */
STATIC int
sfs_fid(vnode_t *vp, fid_t **fidpp)
{
	struct ufid	*ufid;
	int		error;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;

	vfsp = vp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		error = EIO;
	} else {
		error = 0;
		ufid = (struct ufid *)
				kmem_zalloc(sizeof(struct ufid), KM_SLEEP);
		ufid->ufid_len = sizeof (struct ufid) - sizeof (ushort);
		ufid->ufid_ino = VTOI(vp)->i_number;
		ufid->ufid_gen = VTOI(vp)->i_gen;
		*fidpp = (fid_t *)ufid;
	}
	return (error);
}

/*
 * int
 * sfs_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
 *	Obtain, if possible, the inode's rwlock according to <mode>.
 *
 * Calling/Exit State:
 *	A return value of 0 indicates success.
 *
 *	On success, the rwlock of the inode is held according to
 *	mode. It is also guaranteed that the caller will not block
 *	on I/O operations to the range indicated by <off, len>
 *	while holding the rwlock (i.e., until a subsequent 
 *	VOP_RWUNLOCK() is performed).
 *
 *	On failure, the rwlock of the inode is *not* held.
 *
 * Description:
 *	Acquire the inode's rwlock in the requested mode and then
 *	if mandatory locking is enabled for the file, check whether
 *	there are any file/record locks which would cause the LWP
 *	to block on a subsequent I/O operation. If there are, the
 *	caller will block in chklock() if <fmode> indicates it's
 *	OK to do so.
 */
/* ARGSUSED */
STATIC int
sfs_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
{
	inode_t	*ip;
	int	error;

	ip = VTOI(vp);
	error = 0;

	if (mode == LOCK_EXCL) {
		SFS_IRWLOCK_WRLOCK(ip);
	} else if (mode == LOCK_SHARED) {
		SFS_IRWLOCK_RDLOCK(ip);
	} else {
		/*
		 *+ An invalid mode was passed as a parameter to
		 *+ this routine. This indicates a kernel software
		 *+ problem
		 */
		cmn_err(CE_PANIC,
			"UFS/SFS sfs_rwlock: invalid lock mode requested");
	}

	if (MANDLOCK(vp, ip->i_mode)) {
		if (fmode) {
			error = chklock(vp, (mode == LOCK_SHARED) ? FREAD :
					FWRITE, off, len, fmode, ip->i_size);
		}
	} 

	if (error) {
		SFS_IRWLOCK_UNLOCK(ip);
	}

	return (error);
}

/*
 * STATIC void
 * sfs_rwunlock(vnode_t *vp, off_t off, int len)
 *	Release the inode's rwlock.
 *
 * Calling/Exit State:
 *	On entry, the calling LWP must hold the inode's rwlock
 *	in either *shared* or *exclusive* mode. On exit, the
 *	caller's hold on the lock is released.
 *
 * Remarks:
 *	Currently, <off> and <len> are ignored. In the future,
 *	for example, they might be used for a finer grained locking
 *	scheme.
 */
/* ARGSUSED */
STATIC void
sfs_rwunlock(vnode_t *vp, off_t off, int len)
{
	inode_t	*ip;

	ip = VTOI(vp);
	SFS_IRWLOCK_UNLOCK(ip);

	return;
}
			
/*
 * int
 * sfs_seek(vnode_t *vp, off_t ooff, off_t *noffp)
 *	Validate a seek pointer.
 *
 * Calling/Exit State:
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		EIO	<vp> is on a file system that has been
 *			invalidated and is currently inaccessible.
 *		EINVAL	The new seek pointer is negative.
 *
 * Description:
 *	No locking is necessary since the result of this routine
 *	depends entirely on the value of <*noffp>.
 */
/* ARGSUSED */
STATIC int
sfs_seek(vnode_t *vp, off_t ooff, off_t *noffp)
{
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;

	vfsp = vp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	return ((*noffp < 0) ? EINVAL : 0);
}
			
/*
 * 
 * int
 * sfs_frlock(vnode_t *vp, int cmd, struct flock *bfp,
 *            int flag, off_t offset, cred_t *cr)
 *	Establish or interrogate the state of an advisory or mandatory
 *	lock on the vnode.
 *
 * Calling/Exit State:
 *	No vnode/inode locks are held on entry or exit.
 *
 * Description:
 *	To set a file/record lock (i.e., cmd is F_SETLK or
 *	F_SETLKW), the inode's rwlock must be held in exclusive
 *	mode before checking whether mandatory locking is enabled
 *	for the vnode to interact correctly with LWPs performing I/O
 *	to or from the file.
 */
/* ARGSUSED */
STATIC int
sfs_frlock(vnode_t *vp, int cmd, struct flock *bfp,
	   int flag, off_t offset, cred_t *cr)
{
	inode_t		*ip;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;
	int		error;

	vfsp = vp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	ip = VTOI(vp);

	if (cmd == F_SETLK || cmd == F_SETLKW) {
		SFS_IRWLOCK_WRLOCK(ip);
	} else {
		SFS_IRWLOCK_RDLOCK(ip);
	}

	/*
	 * If file is being mapped, disallow frlock.
	 */
	if (ip->i_mapcnt > 0 && MANDLOCK(vp, ip->i_mode)) {
		error = EAGAIN;
		goto out;
	}

	error = fs_frlock(vp, cmd, bfp, flag, offset, cr, ip->i_size);
out:
	SFS_IRWLOCK_UNLOCK(ip);
	return (error);
}

/*
 * int
 * sfs_getpageio(vnode_t *vp, off_t off, uint_t len, page_t *pp,
 *	daddr_t *io_list, int flag)
 *
 * Calling/Exit State:
 *	The inode rwlock may or may not be held locked on entry.
 *
 * Description:
 * 	Set up for page io and call the driver strategy routine to
 *	fill the pages. Pages are linked by p_next. If flag is
 *	B_ASYNC, don't wait for io to complete.
 */
STATIC int
sfs_getpageio(inode_t *ip, off_t off, uint_t len, daddr_t *io_list,
	page_t *pp, int flag)
{
	vnode_t *devvp;
	fs_t *fs = ip->i_fs;
	buf_t *bplist[PAGESIZE/NBPSCTR];
	buf_t **bpp;
	int bsize;
	int nio, blkon;
	int dblks = 0;
	off_t curoff;
	uint_t curlen;
	int i, blkpp, err = 0, bio_err;

	bsize = fs->fs_bsize;

	ASSERT(len != 0);
	ASSERT(len <= MAX(PAGESIZE, bsize));
	ASSERT(pp != NULL);

	devvp = ip->i_devvp;
	if (bsize >= PAGESIZE)
		blkpp = 1;
	else
		blkpp = PAGESIZE/bsize;

	blkon = btodb(blkoff(fs, off));

	ASSERT(blkon == 0 || blkpp == 1);

	nio = pp->p_nio;
	ASSERT(nio >= 1);

	curoff = off & PAGEOFFSET;
	curlen = len;
	for (bpp = bplist, i = 0; i < blkpp && curoff < ip->i_size;
					i++, curoff += bsize, curlen -= bsize) {
		if (io_list[i] == DB_HOLE)
			continue;

		*bpp = pageio_setup(pp, curoff, MIN(curlen, bsize),
							flag | B_READ);

		dblks += btodb((*bpp)->b_bcount);

		(*bpp)->b_edev = ip->i_dev;
		(*bpp)->b_blkno = fsbtodb(fs, io_list[i]) + blkon;

		(*bdevsw[getmajor(devvp->v_rdev)].d_strategy)(*bpp);
		bpp++;
	}

	ASSERT((bpp - bplist) == nio);

	/* Update the number of dev sized blocks read by this LWP */
	ldladd(&u.u_ior, dblks);
#ifdef PERF
	mets_fsinfo[MET_SFS].pgin++;
	/*
	 * btopr(len) isn't the right number of pages if there are big holes.
	 * But it's probably a good estimate in most cases.
	 */
	mets_fsinfo[MET_SFS].pgpgin += btopr(len);
	mets_fsinfo[MET_SFS].sectin += dblks;
#endif /* PERF */

	if (mac_installed && (ip->i_swapcnt == 0)) {
		uint_t i; 
		page_t *pp2 = pp;

		for (i = btop(len); i-- != 0; pp2 = pp2->p_next)
			pp2->p_lid = CRED()->cr_lid;
	}

	if (flag & B_ASYNC) {
#ifdef PERF
		/*
		 * btopr(len) isn't the right number of pages if there
		 * are big holes.  But it's probably a good estimate
		 * in most cases.
		 */
		mets_fsinfo[MET_SFS].rapgpgin += btopr(len);
		mets_fsinfo[MET_SFS].rasectin += dblks;
#endif /* PERF */
		return 0;
	}

	for (bpp = bplist; nio-- > 0; bpp++) {
		bio_err = biowait(*bpp);
		if (bio_err)
			err = bio_err;
		pageio_done(*bpp);
	}

	return(err);
}

/*
 * int
 * sfs_getapage(vnode_t *vp, uint_t off, u_int len, u_int *protp,
 *	page_t *pl[], uint_t plsz, struct seg *seg,
 *	vaddr_t addr, enum seg_rw rw, cred_t *cr)
 *
 * Calling/Exit State:
 *	The inode rwlock may or may not be held locked.
 *
 * Description:
 *
 *	Called from pvn_getpages or sfs_getpage to get a particular page.
 *
 *	If rw == S_WRITE or rw == S_OVERWRITE and block is not allocated,
 *	bmap will be caled to allocate backing store.
 *
 *	If pl == NULL, async I/O is requested and I/O is done only if
 *	if it's convenient.
 *
 */
/* ARGSUSED */
STATIC int
sfs_getapage(vnode_t *vp, uint_t off, u_int len, u_int *protp,
	page_t *pl[], uint_t plsz, struct seg *seg,
	vaddr_t addr, enum seg_rw rw, cred_t *cr)
{
	struct inode *ip;
	fs_t *fs;
	page_t *pp, **ppp;
	off_t roff, io_off;
	uint_t io_len;
	page_t *io_pl[MAXBSIZE/PAGESIZE];
	daddr_t	io_list[PAGESIZE/NBPSCTR];
	daddr_t local_dblist[PAGESIZE/NBPSCTR], *dblist, *dbp;
	daddr_t lbn;
	off_t lbnoff, curoff;
	int bsize, blksz, i, blkpp, minsize;
	size_t saved_isize;
	int sz, j;
	int err;
	boolean_t page_uninit;

	ip = VTOI(vp);
	fs = ip->i_fs;
	lbn = lblkno(fs, off);
	lbnoff = off & fs->fs_bmask;
	roff = off & PAGEMASK;
	bsize = fs->fs_bsize;
	/*
	 * Save the old file size. This is necessary because
	 * if we are extending file size, bmap is going to
	 * change the file size. We need the old file size
	 * to determine how much io is needed to bring in
	 * file data.
	 */
	saved_isize = ip->i_size;

	if (bsize >= PAGESIZE) {
		blkpp = 1;
		minsize = PAGESIZE;
	} else {
		blkpp = PAGESIZE/bsize;
		minsize = bsize;
	}

	/*
	 * If we are not doing read-ahead, we call page_lookup_or_create
	 * to look up the page in the page cache or have it created. If
	 * the page is in the page cache, the page is returned read-locked.
	 * If the page is created, it is write-locked.
	 */
	if (pl != NULL) {
		pp = page_lookup_or_create(vp, roff);
		ASSERT(pp != NULL);

		/*
		 * If we find the page in the page cache
		 * and we are not changing the backing store,
		 * we can return the page to the caller.
		 */
		if (PAGE_IS_RDLOCKED(pp)) {

		    if (blkpp > PGPRVSZ) {
		        /*
		         * If db doesn't fit in dblist, we need to get it
		         * by calling bmappage.
		         */
		    	dblist = local_dblist;
		    	err = sfs_bmappage(vp, off, len, &pp, NULL, dblist,
		    				io_list, rw, cr);
		    	if (err)
		    		return err;
		    } else {

			/*
			 * If db does fit in dblist, we can use it
			 * to determine if we need to call bmap to
			 * allocate backing store within the "write" range.
			 */
		    	dblist = PG_DBLIST(pp);

			if (rw == S_WRITE || rw == S_OVERWRITE) {
			    curoff = roff + PAGESIZE;

			    dbp = dblist + blkpp - 1;
			    for (i = blkpp; i-- != 0; dbp--) {
			    	curoff -= minsize;
			    	if (saved_isize <= (off + len) ||
			    	(*dbp == DB_HOLE &&
			    	 off < curoff + minsize &&
			    	 off + len > curoff)) {

			    	    err = sfs_bmappage(vp, off, len, &pp, NULL,
						dblist, io_list, rw, cr);
			    	    if (err)
			    		return err;
				    saved_isize = ip->i_size;
			    	    if (PAGE_IS_WRLOCKED(pp))
			    		goto do_io;
			    	    break;
			    	}
			    }
			}
		    }


existing_page:
		    if (protp) {
		        curoff = roff;
			dbp = dblist;
		        for (i = 0; curoff < saved_isize && i < blkpp;
						i++, curoff += bsize, dbp++) {
			    if (*dbp == DB_HOLE) {
				*protp &= ~PROT_WRITE;
				break;
			    }
		        }
		    }

		    ASSERT(PAGE_IS_RDLOCKED(pp));
		    *pl++ = pp;
		    *pl = NULL;

		    return 0;
		}
	} else {
		/*
		 * This is a read-ahead, call page_lazy_create()
		 * which will create the page only if memory is readily
		 * available and page is not already in the page cache.
		 */
		pp = page_lazy_create(vp, roff);

		if (pp == NULL)
			return 0;

		saved_isize = ip->i_size;
	}

	ASSERT(pp != NULL);
	ASSERT(PAGE_IS_WRLOCKED(pp));

	if (blkpp > PGPRVSZ)
                dblist = local_dblist;
        else
                dblist = PG_DBLIST(pp);

	err = sfs_bmappage(vp, off, len, &pp, NULL, dblist, io_list, rw, cr);

	/*
	 * If bmap fails, pages should have been unlocked by bmap.
	 */
	if (err)
		return (err);

        /*
         * If page is returned with a reader-lock, page is out-of-order
         * we had to drop it and reacquire it in sfs_bmappage(). It might
	 * be a different page now. We need to go back and recheck the
	 * dblist to see if there is any more work for us to do.
         */
        if (PAGE_IS_RDLOCKED(pp))
                goto existing_page;
do_io:
	/*
         * If the file size changed while we were creating the page,
         * and the page offset is now past the end of the new file
         * size, drop the page and return.
         */
        if (pp->p_offset > saved_isize) {
                pvn_fail(pp, B_READ);
                return EFAULT;
        }

	/*
	 * If file is currently mapped or it's a directory, we need
	 * to turn off the S_OVERWRITE optimization.
	 */
	if (rw == S_OVERWRITE && (ip->i_mapcnt > 0 || vp->v_type == VDIR))
		rw = S_WRITE;
	
	pp->p_nio = 0;
	page_uninit = B_FALSE;
	curoff = roff;
	dbp = dblist;
	for (i = 0; i < blkpp; i++, curoff += bsize, dbp++) {

		/*
		 * If we are overwriting the whole block, we
		 * don't need to read it in. But we need to
		 * mark the page as uninitialized and return
		 * it to the caller writer-locked.
		 */
		if (rw == S_OVERWRITE && curoff >= off &&
		    (curoff + minsize) <= off + len) {
			io_list[i] = DB_HOLE;
			page_uninit = B_TRUE;
			continue;
		}

		if (*dbp == DB_HOLE && protp && curoff < saved_isize)
			*protp &= ~PROT_WRITE;

		if (io_list[i] == DB_HOLE) {
			pagezero(pp, i * bsize, MIN(bsize, PAGESIZE));
		} else
			pp->p_nio++;

	}

	if (pp->p_nio == 0) {
		/*
		 * All holes or past EOF, so we don't have to do any I/O.
		 */
		ASSERT(PAGE_IS_WRLOCKED(pp));
		if (pl != NULL) {
			if (!page_uninit)
				page_downgrade_lock(pp);
			*pl++ = pp;
			*pl = NULL;
		} else {
			page_unlock(pp);	/* drop page writer lock */
		}

		return 0;
	}

	for (i = 0; i < MAXBSIZE/PAGESIZE; i++)
                io_pl[i] = NULL;

	if (rw != S_OVERWRITE) {
		/*
		 * Compute size we really want to get.
		 */
		if (lbn < NDADDR) {
			/*
			 * Direct block.
			 */
			if (bsize >= PAGESIZE) {
				blksz =
				    ((saved_isize >= ((lbn) + 1) <<
				    (fs)->fs_bshift) ? bsize :
				    (fragroundup(fs, blkoff(fs, saved_isize))));
			} else {
				blksz =
				    ((saved_isize >= roff + PAGESIZE) ?
				    PAGESIZE :
				    (fragroundup(fs, saved_isize) - roff));
			}
		} else {
			/*
			 * Indirect block, round up to smaller of
			 * page boundary or file system block size.
			 */
			if (bsize >= PAGESIZE) {
				blksz = MIN(roundup(saved_isize, PAGESIZE) -
						lbnoff, bsize);
			} else {
				blksz =
				    ((saved_isize >= roff + PAGESIZE) ?
				    PAGESIZE :
				    (blkroundup(fs, saved_isize) - roff));
			}
		}
		if (bsize > PAGESIZE && off < lbnoff + blksz) {
		    page_t *center_pp = pp;
		    page_t *pp_cur, *pp_next;
		    int npages, npages_ok = 0;
		    boolean_t abort_rest = B_FALSE;

		    pp_cur = pp = pvn_kluster(vp, off, seg, addr,
			&io_off, &io_len, lbnoff, blksz, pp);

		    saved_isize = ip->i_size;

		    /*
		     * If the file size changed and the center page is beyond
		     * the new file size, we abort the io and return EFAULT.
		     */
		    if (center_pp->p_offset > saved_isize) {
			pvn_fail(pp, B_READ);
			return (EFAULT);
		    }

		    /*
		     * At least some pages, includeing the center page, are
		     * within the file size range. We'll loop through them
		     * and drop the ones that are out of bound.
		     */
		    npages = io_len >> PAGESHIFT;
		    if ((io_len & PAGEOFFSET) != 0)
			npages++;
		    ASSERT(npages > 0);
		    ppp = io_pl;

		    for (i=0; i < npages; i++) {
			if ((pp_cur->p_offset > saved_isize) || abort_rest) {
			    pp_next = pp_cur->p_next;
			    page_sub(&pp, pp_cur);
			    page_abort(pp_cur);
			    pp_cur = pp_next;
			    abort_rest = B_TRUE;
			} else {
			    npages_ok++;
			    *ppp++ = pp_cur;
			    if (pp_cur != center_pp) {
				/*
				 * Fill in the dblist of pages that
				 * we didn't do bmap for.
				 */
				PG_DBLIST(pp_cur)[0] =
					PG_DBLIST(center_pp)[0];
				pp_cur->p_nio = 1;
			    }
			    pp_cur = pp_cur->p_next;
			}
		    }

		    ASSERT (npages_ok > 0);
		    /* Adjust io_len if some pages were dropped. */
		    if (npages_ok != npages)
			    io_len = io_off + (npages_ok * PAGESIZE);
		} else {
			io_off = roff;
			io_len = ((bsize < PAGESIZE) ? (lbnoff - roff + blksz) :
						blksz);
			io_pl[0] = pp;
		}

		pagezero(pp->p_prev, io_len & PAGEOFFSET,
				PAGESIZE - (io_len & PAGEOFFSET));
	} else {
		io_off = roff;
		io_len = PAGESIZE;
		io_pl[0] = pp;
	}

	err = sfs_getpageio(ip, io_off, io_len, io_list, pp,
					pl == NULL ? B_ASYNC : 0);

	/*
	 * If we encountered any I/O error, the pages should have been
	 * aborted by pvn_done() and we need not downgrade the page
	 * lock, nor should we reference any of the pages in the
	 * page list.
	 */
	if (pl == NULL || err)
		return err;

	/*
	 * Otherwise, load the pages in the page list to return to caller.
	 */
	if (plsz >= io_len) {
		/*
		 * Everything fits, set up to load up all the pages.
		 */
		i = 0;
		sz = io_len;
	} else {
		/*
		 * Not everything fits. Set up to load plsz worth
		 * starting at the needed page.
		 */
		for (i = 0; io_pl[i]->p_offset != off; i++) {
			ASSERT(i < btopr(io_len) - 1);
		}
		sz = plsz;
		if (ptob(i) + sz > io_len)
                        i = btopr(io_len - sz);
		ASSERT(!page_uninit);
	}

	j = i;
	ppp = pl;
	do {
		*ppp = io_pl[j++];
		ASSERT(PAGE_IS_WRLOCKED(*ppp));
		if (!page_uninit)
			page_downgrade_lock(*ppp);
		ppp++;
	} while ((sz -= PAGESIZE) > 0);
	*ppp = NULL;		/* terminate list */

	/* Unlock pages we're not returning to our caller. */
	/* Need to Fix S5 */
	if (j >= btopr(io_len))
		j = 0;
	while (j != i) {
		if (io_pl[j] != NULL) {
			ASSERT(PAGE_IS_WRLOCKED(io_pl[j]));
			page_unlock(io_pl[j]);
		}
		j++;
		if (j >= btopr(io_len))
			j = 0;
	}

	return 0;
}

STATIC int sfs_ra = 1;

/*
 * int
 * sfs_getpage(vnode_t *vp, uint_t off, u_int len, u_int *protp, page_t *pl[],
 *	uint_t plsz, struct seg *seg, vaddr_t addr, enum seg_rw rw, cred-t *cr)
 *
 * Calling/Exit State:
 *	The inode rwlock may be held shared (if called from readi)
 *	or exclusive (if called from writei) on entry.
 *
 * Description:
 *	 Reads one or more pages from a contiguous range of file space starting
 *	 at the specified <vnode, offset>.  VOP_GETPAGE() is responsible for
 *	 finding/creating the necessary pages with this <vnode, offset>
 *	 identity, and returning these pages, reader or writer locked, to
 *	 the caller. For the S_OVERWRITE case, some pages may be returned
 *	 writer locked if they are not entirely initialized.
 *
 *	 The pages are returned in a NULL-terminated array whose space is
 *	 provided by the caller.
 */
STATIC int
sfs_getpage(vnode_t *vp, uint_t off, u_int len, u_int *protp, page_t *pl[],
	uint_t plsz, struct seg *seg, vaddr_t addr, enum seg_rw rw, cred_t *cr)
{
	inode_t *ip = VTOI(vp);
	fs_t *fs = ip->i_fs;
	off_t roff;
	uint_t rlen, nextoff;
	page_t *rpl[MAXBSIZE/PAGESIZE + 1], **ppp;
	daddr_t dummy_iolist[PAGESIZE/NBPSCTR];
	daddr_t dummy_dblist[PAGESIZE/NBPSCTR];
	int err;
	pl_t	opl;

#ifdef PERF
	mets_fsinfo[MET_SFS].getpage++;
#endif
	if (vp->v_flag & VNOMAP)
		return (ENOSYS);

	/*
	 * This check for beyond EOF allows the request to extend up to
	 * the page boundary following the EOF.
	 *
	 * Also, since we may be called as a side effect of a bmap or
	 * dirsearch() using fbread() when the blocks might be being
	 * allocated and the size has not yet been up'ed.  In this case
	 * we disable the check and always allow the getpage to go through
	 * if the segment is seg_map, since we want to be able to return
	 * zeroed pages if bmap indicates a hole in the non-write case.
	 * For ufs, we also might have to read some frags from the disk
	 * lbn in sfs_bmap().
	 */
	opl = SFS_ILOCK(ip);
        if (rw != S_OVERWRITE) {
		if (off + len > ip->i_size + PAGEOFFSET) {
			SFS_IUNLOCK(ip, opl);
			return (EFAULT);	/* beyond EOF */
		} else if (off + len > ip->i_size)
			len = ip->i_size - off;
	}

	if (protp != NULL)
		*protp = PROT_ALL;

	if (rw == S_OVERWRITE &&
	    ((rlen = sfs_bmap_might_realloc(vp, off, len)) != 0)) {

		SFS_IUNLOCK(ip, opl);
		/*
		 * If we are extending file size from EOF and there is
		 * potential for realloc, we need to bring in all the
		 * pages with existing data before we (possibly) assign
		 * new backing store.
		 */
		ASSERT((rlen & PAGEOFFSET) == 0);
		roff = MIN(pagerounddown(off), blkrounddown(fs, off));

		err = pvn_getpages(sfs_getapage, vp, roff, rlen,
			NULL, rpl, rlen,
			seg, addr - (off - roff), S_OTHER, cr);
		if (err) {
			ppp = rpl;
			while (*ppp != NULL)
				page_unlock(*ppp++);
			goto err_out;
		}

		/* Allocate the backing store up-front. */
		err = sfs_bmappage(vp, off, len, NULL, rpl, dummy_dblist,
							dummy_iolist, rw, cr);
		if (err)
			goto err_out;

	} else
		SFS_IUNLOCK(ip, opl);

	ASSERT(pl == NULL || plsz >= PAGESIZE);
	if (btop(off) == btop(off + len - 1)) {
		err = sfs_getapage(vp, off, len, protp, pl, plsz, seg, addr,
		    rw, cr);
	} else
		err = pvn_getpages(sfs_getapage, vp, off, len, protp, pl, plsz,
		    seg, addr, rw, cr);

	if (err)
		goto err_out;

	nextoff = ptob(btopr(off + len));
	opl = SFS_ILOCK(ip);
	if (sfs_ra && ip->i_nextr == off &&
		nextoff < ip->i_size && rw != S_OVERWRITE && rw != S_WRITE) {
		if (nextoff + PAGESIZE > ip->i_size)
			rlen = ip->i_size - nextoff;
		else
			rlen = PAGESIZE;
		/*
		 * For read-ahead, pass a NULL pl so that if it's
		 * not convenient, io will not be done. And also,
		 * if io is done, we don't wait for it.
		 */
#ifdef PERF
		mets_fsinfo[MET_SFS].ra++;
#endif
		SFS_IUNLOCK(ip, opl);
		err = sfs_getapage(vp, nextoff, rlen, NULL, NULL, 0,
				seg, addr, S_OTHER, cr);
	} else
		SFS_IUNLOCK(ip, opl);

	ip->i_nextr = nextoff;

	/*
	 * If the inode is not already marked for IACC (in readi() for read)
	 * and the inode is not marked for no access time update (in writei()
	 * for write) then update the inode access time and mod time now.
	 */
	if ((ip->i_flag & (IACC | INOACC)) == 0 && WRITEALLOWED(vp, cr)) {
                opl = SFS_ILOCK(ip);
                if (rw != S_OTHER)
                        ip->i_flag |= IACC;
                if (rw == S_WRITE)
                        /* The S_OVERWRITE case is handled by sfs_writei(). */
                        ip->i_flag |= IUPD;
                IMARK(ip, ip->i_flag);
                SFS_IUNLOCK(ip, opl);
	}

err_out:

	return (err);
}

/*
 * int
 * sfs_getpagelist(vnode_t *vp, off_t off, page_t *pp, uint_t plsz,
 * 		void *bmapp, int flags, cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	Fills in data for each of the pages in the pp list from
 *	consecutive logical offsets starting from off. Note that these
 *	offsets are logical offsets relative to the base offset passed
 *	into a prior call to VOP_STABLESTORE(). The logical offsets
 *	are translated into the backing store offsets.
 *
 */
/*ARGSUSED*/
STATIC int
sfs_getpagelist(vnode_t *vp, off_t off, uint_t len, page_t *pp,
		  void *bmapp, int flags, cred_t *cr)
{
	struct inode *ip = VTOI(vp);
#ifdef DEBUG
	page_t *pp2;
#endif
	fs_t *fs;
	daddr_t *io_list;
	int bsize;
	short blkpp;
	page_t *plist, *io_pl;
	uint_t io_len;
	off_t io_off;
	int err;

	ASSERT(vp->v_type == VREG);

#ifdef DEBUG
	{
	uint_t n;

	for (pp2 = pp, n = btop(len); n-- != 0;  pp2 = pp2->p_next) {
		ASSERT(PAGE_IS_WRLOCKED(pp2));
	}
	ASSERT(pp2 == pp);
	}
#endif

	ASSERT((off & PAGEOFFSET) == 0);
	ASSERT((len >= PAGESIZE) && ((len & PAGEOFFSET) == 0));
	ASSERT(off + len <= ip->i_size);

	fs = ip->i_fs;
	bsize = fs->fs_bsize;

	if (bsize >= PAGESIZE)
		blkpp = 1;
	else
		blkpp = PAGESIZE/bsize;

	plist = pp;
	do {

		pp = plist;
		io_list = ((daddr_t *)bmapp + (off >> fs->fs_bshift)); 

		io_off = off;
		io_len = 0;
		io_pl = NULL;
		do {
			io_len += PAGESIZE;
			page_sub(&plist, pp);
			page_sortadd(&io_pl, pp);
			pp->p_nio = (uchar_t)blkpp;
			pp = plist;
		} while (((io_off += PAGESIZE) < blkroundup(fs, off))
							&& pp != NULL);

		ASSERT(io_pl != NULL);
		err = sfs_getpageio(ip, off, io_len, io_list, io_pl, 0);

		if (err)
			break;

		off = io_off;

	} while (plist != NULL);

	return err;
}

/*
 * int
 * sfs_putpageio(inode_t *ip, off_t off, size_t len, page_t *pp,
 *	daddr_t *io_list, int flags)
 *
 * Calling/Exit State:
 *	The inode rwlock may or may not be held locked on entry.
 *
 * Description:
 * 	Set up for page I/O and call the driver strategy routine to
 *	write the pages out. Pages are linked by p_next. If flag is
 *	B_ASYNC, don't wait for I/O to complete.
 *	Flags are composed of {B_ASYNC, B_INVAL, B_DONTNEED}
 */
STATIC int
sfs_putpageio(inode_t *ip, off_t off, size_t len, page_t *pp,
	daddr_t *io_list, int flags)
{
	vnode_t *devvp;
	fs_t *fs;
	size_t bsize;
	buf_t *bplist[PAGESIZE/NBPSCTR];
	buf_t **bpp;
	int i, blkpp, nio, blkon;
	int dblks = 0;
	off_t curoff;
	size_t curlen;
	int bio_err, err = 0, mask;

	ASSERT(len != 0);

	PAGEIO_LOG_PAGES(pp, flags, "sfs_putpageio");

	fs = ip->i_fs;
	devvp = ip->i_devvp;
	bsize = fs->fs_bsize;

	if (bsize >= PAGESIZE) {
		blkpp = 1;
		mask = PAGEMASK;
	} else {
		blkpp = PAGESIZE/bsize;
		mask = fs->fs_bmask;
	}

	blkon = btodb(blkoff(fs, off));

	/*
	 * Need to calculate nio before invoking
	 * the driver strategy routine because
	 * io could be async.
	 */
	pp->p_nio = 0;
	curoff = off;
	for (i = 0; i < blkpp && curoff < off + len;
					i++, curoff += bsize) {
		if (io_list[i] == DB_HOLE)
			continue;

		pp->p_nio++;
	}

	curoff = off;
	curlen = len;
	for (bpp = bplist, i = 0; i < blkpp && curoff < off + len;
					i++, curoff += bsize, curlen -= bsize) {
		if (io_list[i] == DB_HOLE)
			continue;

		*bpp = pageio_setup(pp, curoff & PAGEOFFSET, MIN(curlen, bsize),
				    B_WRITE | flags);

		dblks += btodb((*bpp)->b_bcount);

		(*bpp)->b_edev = ip->i_dev;
		(*bpp)->b_blkno = fsbtodb(fs, io_list[i]) + blkon;

		/*
		 * call dow_strategy_page for directory pages only - they're
		 * the only pages with ordering requirements at present
		 */
		if ((ip->i_mode & IFMT) == IFDIR) {
			sfs_unlink_flushed(ip, curoff);
			dow_strategy_page(bpp, ITOV(ip), curoff & mask,
				MIN(PAGESIZE, bsize));
		} else
			(*bdevsw[getmajor(devvp->v_rdev)].d_strategy)(*bpp);
		bpp++;
	}

	/* Update the number of dev sized blocks written by this LWP */
	ldladd(&u.u_iow, dblks);
#ifdef PERF
	mets_fsinfo[MET_SFS].pgout++;
	mets_fsinfo[MET_SFS].pgpgout += btopr(len);
	mets_fsinfo[MET_SFS].sectout += dblks;
#endif

	/*
	 * If async, assume that pvn_done will handle the pages
	 * when I/O is done.
	 */
	if (flags & B_ASYNC)
		return 0;

	nio = pp->p_nio;
	for(bpp = bplist; nio-- > 0; bpp++) {
		bio_err = biowait(*bpp);
		if (bio_err)
			err = bio_err;
		pageio_done(*bpp);
	}

	return (err);
}

/*
 * int
 * sfs_putpage(vnode_t *vp, off_t off, uint_t len, int flags, cred_t *cr)
 *
 * Calling/Exit State:
 *	The inode rwlock may or may not be held locked on entry.
 *
 * Description:
 *
 *	Flags are composed of {B_ASYNC, B_INVAL, B_DONTNEED, B_FORCE}
 *	If len == 0, do from off to EOF.
 *
 *	The normal cases should be len == 0 & off == 0 (entire vp list),
 *	len == MAXBSIZE (from segmap_release actions), and len == PAGESIZE
 *	(from pageout).
 *
 *	Note that for sfs it is possible to have dirty pages beyond
 *	roundup(ip->i_size, PAGESIZE).  This can happen if the file
 *	length is long enough to involve indirect blocks (which are
 *	always fs->fs_bsize'd) and PAGESIZE < bsize while the length
 *	is such that roundup(blkoff(fs, ip->i_size), PAGESIZE) < bsize.
 *
 * Remarks:
 *	Directory pages are not clustered, because pages which are
 *	written using delayed ordered writes cannot be clustered.
 *	The reason for this is that clustering could cause cycles,
 *	if one of the pages being clustered must be written before
 *	a particular buffer, and another of the pages must be written
 *	after the buffer.
 */
/* ARGSUSED */
STATIC int
sfs_putpage(vnode_t *vp, off_t off, uint_t len, int flags, cred_t *cr)
{
	inode_t *ip;
	off_t kl_off;
	uint_t kl_len;
	int error;
	pl_t	opl;
	STATIC int sfs_doputpage(vnode_t *, page_t *, int, cred_t *);

	ASSERT(!(vp->v_flag & VNOMAP));

#ifdef PERF
	mets_fsinfo[MET_SFS].putpage++;
#endif

	ip = VTOI(vp);

	/*
	 * The following check is just for performance
	 * and therefore doesn't need to be foolproof.
	 * The subsequent code will gracefully do nothing
	 * in any case.
	 */
	if (vp->v_pages == NULL || off >= ip->i_size)
		return (0);

	if ((ip->i_mode & IFMT) == IFDIR) {
		kl_off = off;	/* No clustering of directory pages - see */
		kl_len = len;	/* Remarks above */
	} else if (len != 0) {
		fs_t *fs = ip->i_fs;

		/* Do klustering to bsize boundaries */
		kl_off = (off & fs->fs_bmask);
		kl_len = ((off + len + fs->fs_bsize - 1) & fs->fs_bmask) -
			  kl_off;
	} else {
		kl_off = off;
		kl_len = 0;
	}

	error = pvn_getdirty_range(sfs_doputpage, vp, off, len, kl_off, kl_len,
				   ip->i_size, flags, cr);

	if (off == 0 && !error && (len == 0 || len >= ip->i_size)) {
		/*
		 * We have just sync'ed back all the pages
		 * on the inode; turn off the IMODTIME flag.
		 */
		opl = SFS_ILOCK(ip);
		ip->i_flag &= ~IMODTIME;
		SFS_IUNLOCK(ip, opl);
	}

	return error;
}

/*
 * STATIC int
 * sfs_doputpage(vnode_t *vp, page_t *dirty, int flags, cred_t *cr)
 *	workhorse for sfs_putpage
 *
 * Calling/Exit State:
 *	The inode rwlock may or may not be held locked on entry.
 *
 *	A list of dirty pages, prepared for I/O (in pageout state),
 *	is passed in dirty.  Other parameters are passed through from
 *	sfs_putpage.
 */
STATIC int
sfs_doputpage(vnode_t *vp, page_t *dirty, int flags, cred_t *cr)
{
	inode_t *ip;
	page_t *pp, *pp2;
	fs_t *fs;
	page_t *pl_list;
	uint_t io_off, io_len, io_rlen;
	daddr_t lbn;
	daddr_t local_dblist[PAGESIZE/NBPSCTR], *dblist;
	uint_t lbn_off;
	int bsize, blkpp;
	uint_t isize;
	int err;
	boolean_t io_short;

	PAGEIO_LOG_PAGES(dirty, flags, "sfs_doputpage");
	ip = VTOI(vp);
	fs = ip->i_fs;

	/*
	 * Now we have a list of locked dirty pages marked for
	 * write back.  All the pages on the dirty list need to still
	 * be dealt with here.  Verify that we can really can do the
	 * write back to the filesystem and if not and we have some
	 * dirty pages, return an error condition.
	 */
	if (fs->fs_ronly)
		err = EROFS;
	else
		err = 0;

	/*
	 * Since we are not holding any locks that stabilize the file
	 * size, the file size might change while we are pushing the
	 * pages out. We need to take a snapshot of the size and use
	 * it to determine how much IO we really need to do.
	 */
	isize = ip->i_size;

	/*
	 * This is an attempt to clean up loose ends left by
	 * applications that store into mapped files.  It's
	 * insufficient, strictly speaking, for ill-behaved
	 * applications, but about the best we can do.
	 */
	if ((flags & B_FORCE) && (VN_CLRMOD(vp))) {
		/*
		 * sync the inode time stamp from msyc(), as was done 
		 * is various versions of SVR4. this would retain a semantic
		 * that an msync record a new modification time on a mapped
		 * file that is modified since last modification time update. 
		 */
		IMARK(ip, IUPD);
		if (vp->v_type == VREG) {
			err = fs_vcode(vp, &ip->i_vcode);
		}
	} else if ((ip->i_flag & IMODTIME) == 0) {
		/*
		 * DO NOT update the modification timestamp. applications
		 * storing into mapped files can cause modification time
		 * for the file to creep forword, as modified pages are
		 * discovered and written out from here, otherwise.
		 */
		if (vp->v_type == VREG) {
			err = fs_vcode(vp, &ip->i_vcode);
		}
	} 


	bsize = fs->fs_bsize;

	if (bsize >= PAGESIZE)
		blkpp = 1;
	else
		blkpp = PAGESIZE / bsize;

	io_short = B_FALSE;
	/*
	 * Handle all the dirty pages not yet dealt with.
	 */
	while (err == 0 && io_short == B_FALSE && (pp = dirty) != NULL) {
		io_off = pp->p_offset;

		/*
		 * We have some dirty page(s) in hand but io length
		 * rounded down to file size doesn't cover the whole
		 * range. This means file size changed between the
		 * time we started the putpage() process and the time
		 * we took a snapshot of the file size at the beginning of
		 * this function (most likely by someone doing an itrunc).
                 * If all pages in the dirty list is beyond the new file
		 * size, we abort the io.
                 */
		if (io_off >= isize) {
			pvn_fail(dirty, B_WRITE|flags);
			return 0;
		}

		/*
		 * Pull off a contiguous chunk that fixes in one lbn
		 */
		lbn = lblkno(fs, io_off);
		bsize = blksize(fs, isize, lbn);

		page_sub(&dirty, pp);
		pl_list = pp;
		io_len = PAGESIZE;
		lbn_off = lbn << fs->fs_bshift;

		if (blkpp == 1) {
			/*
			 * If there are multiple dirty pages in
			 * the same block, we try to kluster them.
			 */
			while (dirty != NULL &&
			    dirty->p_offset < lbn_off + bsize &&
			    dirty->p_offset == io_off + io_len) {
				pp = dirty;
				page_sub(&dirty, pp);
				page_sortadd(&pl_list, pp);
				io_len += PAGESIZE;
			}
		} else {
			/*
			 * If there are multiple blocks per page, we
			 * total the bsize of the blocks within file
			 * size limit.
			 */
			while (lbn_off + bsize < isize) {
				lbn++;
				bsize += blksize(fs, isize, lbn);
			}
		}

		/*
		 * Check for page length rounding problems.
		 */
		if (io_off + io_len > lbn_off + bsize)
			io_rlen = lbn_off + bsize - io_off;
		else
			io_rlen = io_len;

		/*
		 * File size changed and some dirty pages are past
		 * the new size. We need to trim the io length to
		 * cover only pages in the new size range.
		 */
		if (pageroundup(io_rlen) < io_len) {
			io_short = B_TRUE;
			for (pp2 = pl_list; pp2 != pl_list; pp2 = pp) {
				pp = pp2->p_next;
				if (pp2->p_offset > lbn_off + io_rlen) {
					page_sub(&pl_list, pp2);
					page_sortadd(&dirty, pp2);
				} 
			}
		}

		pp = pl_list;

		if (blkpp > PG_DBSIZE) {
			dblist = local_dblist;
			err = sfs_bmappage(vp, io_off, io_rlen, &pl_list, NULL,
					   dblist, dblist, S_OTHER, cr);
			if (err)
				break;
		} else
			dblist = PG_DBLIST(pp);

		err = sfs_putpageio(ip, io_off, io_rlen, pp, dblist, flags);
	}

	if ((err || io_short) && dirty != NULL)
		pvn_fail(dirty, B_WRITE | flags);

	return (err);
}

/*
 * int
 * sfs_putpagelist(vnode_t *vp, off_t off, page_t *pp, void *bmapp,
 *	int flags, cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	Fills in data for each of the pages in the pp list from
 *	consecutive logical offsets starting from off. Note that these
 *	offsets are logical offsets relative to the base offset passed
 *	into a prior call to VOP_STABLESTORE(). The logical offsets
 *	are translated into the backing store offsets.
 *
 */
/*ARGSUSED*/
STATIC int
sfs_putpagelist(vnode_t *vp, off_t off, page_t *pp, void *bmapp,
	int flags, cred_t *cr)
{
	inode_t *ip = VTOI(vp);
	fs_t *fs;
	daddr_t *io_list;
	page_t *plist, *io_pl;
	int bsize;
	uchar_t blkpp;
	uint_t io_len;
	off_t io_off;
	int error;

	ASSERT(vp->v_type == VREG);
	ASSERT((off & PAGEOFFSET) == 0);

	fs = ip->i_fs;
	bsize = fs->fs_bsize;

	if (bsize >= PAGESIZE)
		blkpp = 1;
	else
		blkpp = PAGESIZE / bsize;

	plist = pp;
	do {
		pp = plist;

		ASSERT(PAGE_IS_LOCKED(pp));

		io_list = ((daddr_t *)bmapp + (off >> fs->fs_bshift)); 
		io_off = off;
		io_len = 0;
		io_pl = NULL;
		do {
			io_len += PAGESIZE;
			pp->p_nio = blkpp;
			page_sub(&plist, pp);
			page_sortadd(&io_pl, pp);
			pp = plist;
		} while (((io_off += PAGESIZE) < blkroundup(fs, off))
								&& pp != NULL);

		ASSERT(io_pl != NULL);

		error = sfs_putpageio(ip, off, io_len, io_pl, io_list, flags);

		if (error)
			break;

		off = io_off;

	} while (plist != NULL);

	return error;
}

/*
 * int
 * sfs_stablestore(vnode_t **vpp, off_t *off, size_t *len, void **bmapp,
 *			cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	Reserve storage associated with the vnode pointed to by *vpp
 *	starting at offset "off" for the length of "len" bytes. *bmapp
 *	points to a backing store mapping data structure which is
 *	filled in by this vnode operation and returned to the caller.
 *	
 */
/*ARGSUSED*/
int
sfs_stablestore(vnode_t **vpp, off_t *off, size_t *len, void **bmapp,
		  cred_t *cr)
{
	inode_t *ip = VTOI(*vpp);
	fs_t *fs;
	daddr_t dummy_iolist[PAGESIZE/NBPSCTR];
        daddr_t *daddrp;
	uint_t nblks, bsize, blkpp;
	off_t curoff, roff, eoff;
	size_t rlen, bmap_size;
	int err;
	pl_t	opl;


	ASSERT((*vpp)->v_type == VREG);
	ASSERT(len != 0);

	SFS_IRWLOCK_WRLOCK(ip);

	if ((*off + *len) > ip->i_size) {
		SFS_IRWLOCK_UNLOCK(ip);
		return EINVAL;
	}

	fs = ip->i_fs;

	/* Allocate space for disk block addresses. */
        roff = blkroundup(fs, *off);
	rlen = *len - (roff - *off);
	nblks = blkroundup(fs, rlen) >> fs->fs_bshift;
	/* 0 blocks after rounding? */
	if (nblks == 0) {
		SFS_IRWLOCK_UNLOCK(ip);
		return EINVAL;
	}
	daddrp = *bmapp =
		(daddr_t *)kmem_zalloc(nblks * sizeof(daddr_t), KM_SLEEP);

        eoff = roff + rlen;
        bsize = VBSIZE(*vpp);
        if (bsize >= PAGESIZE)
                blkpp = 1;
        else
                blkpp = PAGESIZE/bsize;
        bmap_size = MAX(PAGESIZE, bsize);
        for (curoff = roff; curoff < eoff; curoff += bmap_size,
                                                daddrp += blkpp) {
		if (curoff + bmap_size > eoff)
			bmap_size = eoff - curoff;
		err = sfs_bmappage(*vpp, curoff, bmap_size, NULL, NULL, daddrp,
					dummy_iolist, S_WRITE, cr);
                if (err)
                        break;
        }

	if (!err) {
		opl = SFS_ILOCK(ip);
		ip->i_swapcnt++;
		SFS_IUNLOCK(ip, opl);
		*off = roff;
		*len = rlen;
	} else {
		kmem_free(daddrp, nblks * sizeof(daddr_t));
		*bmapp = NULL;
	}

	SFS_IRWLOCK_UNLOCK(ip);
	return err;
}

/*
 * int
 * sfs_relstore(vnode_t *vp, off_t off, size_t len, void *bmapp,
 *		cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	Release storage associated with the vnode pointed to by *vpp
 *	starting at offset "off" for the length of "len" bytes. *bmapp
 *	points to a backing store mapping data structure which is
 *	filled in by this vnode operation and returned to the caller.
 *	
 */
/*ARGSUSED*/
int
sfs_relstore(vnode_t *vp, off_t off, size_t len, void *bmapp,
		  cred_t *cr)
{
	inode_t *ip = VTOI(vp);
	fs_t *fs = ip->i_fs;
	pl_t	opl;

	ASSERT(vp->v_type == VREG);
	ASSERT(len != 0);
	ASSERT(blkoff(fs, off) == 0);

	if ((off + len) > ip->i_size)
		return EINVAL;

	kmem_free(bmapp,
		  (blkroundup(fs, len) >> fs->fs_bshift) * sizeof(daddr_t));
	opl = SFS_ILOCK(ip);
	ip->i_swapcnt--;
	SFS_IUNLOCK(ip, opl);

	return 0;
}

/*
 * int
 * sfs_map(vnode_t *vp, off_t off, struct as *as, vaddr_t *addrp, *uint_t len,
 *	uint_t prot, uint_t maxprot, uint_t flags, cred_t *fcr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	Map the specified <vp, offset> at address "addrp" of address
 *	space "as". The inode rwlock is held shared to prevent file
 *	size from chnaging while the mapping is established. The
 *	address space is locked exclusive before calling as_map()
 *	to prevent multiple lwp's from extablishing or relinquish
 *	mappings concurrently.
 *	
 */
/* ARGSUSED */
STATIC int
sfs_map(vnode_t *vp, off_t off, struct as *as, vaddr_t *addrp, uint_t len,
	uint_t prot, uint_t maxprot, uint_t flags, cred_t *fcr)
{
	struct cred *cr = VCURRENTCRED(fcr);
	struct segvn_crargs vn_a;
	int error;
	struct vfs *vfsp = vp->v_vfsp;
	inode_t *ip = VTOI(vp);
	sfs_vfs_t *sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
		return EIO;

	if (vp->v_flag & VNOMAP)
		return (ENOSYS);

	if (vp->v_type != VREG)
		return (ENODEV);

	if ((int)off < 0 || (int)(off + len) < 0)
		return (EINVAL);

 
	/*
	 * If file is being locked, disallow mapping.
	 */
	if (vp->v_filocks != NULL && MANDLOCK(vp, ip->i_mode))
		return EAGAIN;

	SFS_IRWLOCK_WRLOCK(ip);

	as_wrlock(as);

	if ((flags & MAP_FIXED) == 0) {
		map_addr(addrp, len, (off_t)off, 0);
		if (*addrp == NULL) {
			as_unlock(as);
			SFS_IRWLOCK_UNLOCK(ip);
			return (ENOMEM);
		}
	} else {
		/*
		 * User specified address - blow away any previous mappings
		 */
		(void) as_unmap(as, *addrp, len);
	}       

	vn_a.vp = vp;
	vn_a.offset = off;
	vn_a.type = flags & MAP_TYPE;
	vn_a.prot = (uchar_t)prot;
	vn_a.maxprot = (uchar_t)maxprot;
	vn_a.cred = cr;

	error = as_map(as, *addrp, len, segvn_create, &vn_a);

	as_unlock(as);
	SFS_IRWLOCK_UNLOCK(ip);
	return error;
}

/*
 * int
 * sfs_addmap(vnode_t *vp, uint_t off, as_t *as, vaddr_t addr, uint_t len,
 *	uchar_t prot, uchar_t maxprot, uint_t flags, cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	Bump up the count of active mappings of "vp" by the number
 *	of pages of mapping being established.
 *	
 */
/* ARGSUSED */
STATIC int
sfs_addmap(vnode_t *vp, uint_t off, struct as *as, vaddr_t addr, uint_t len,
	uint_t prot, uint_t maxprot, uint_t flags, cred_t *cr)
{
	inode_t *ip;
	int error;
	pl_t	opl;
	
	if (vp->v_flag & VNOMAP)
		error = ENOSYS;
	else {
		error = 0;
		ip = VTOI(vp);
		opl = SFS_ILOCK(ip);
		ip->i_mapcnt += btopr(len);
		SFS_IUNLOCK(ip, opl);
	}

	return (error);
}

/*
 * int
 * sfs_delmap(vnode_t *vp, uint_t off, struct as *as, vaddr_t addr, uint_t len,
 *	uint_t prot, uint_t maxprot, uint_t flags, cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	Decrement the count of active mappings of "vp" by the number
 *	of pages of mapping being relinquished.  If the mapping was
 *	shared and writable, and file has been modified through this
 *	mapping, update the file's timestamps.
 */
/*ARGSUSED*/
STATIC int
sfs_delmap(vnode_t *vp, uint_t off, struct as *as, vaddr_t addr, uint_t len,
	uint_t prot, uint_t maxprot, uint_t flags, cred_t *cr)
{
	inode_t *ip;
	int error;
	pl_t	opl;

	if (vp->v_flag & VNOMAP)
		error = ENOSYS;
	else {
		error = 0;
		ip = VTOI(vp);

		if (((flags & MAP_TYPE) == MAP_SHARED) &&
				(maxprot & PROT_WRITE)) {
			/*
			 * For shared writable mappings, make sure that
			 * the timestamps of the file are updated if any
			 * changes were made to this file through the
			 * mapping.  Call pvn_syncsdirty to see if the
			 * file has been modified, and, if so, call IMARK
			 * to update the timestamps.
			 */
			pvn_syncsdirty(vp);
			opl = SFS_ILOCK(ip);
			if (vp->v_flag & VMOD)
                        	IMARK(ip, ip->i_flag);
		} else {
			/*
			 * Read-only mapping, so no file modifications
			 * occurred through this mapping; no need to
			 * call pvn_syncsdirty here.  Just lock the inode.
			 */
			opl = SFS_ILOCK(ip);
		}
		ip->i_mapcnt -= btopr(len); 	/* Count released mappings */
		ASSERT(ip->i_mapcnt >= 0);
		SFS_IUNLOCK(ip, opl);
	}
	return (error);
}

/*
 * int
 * sfs_poll(vnode_t *vp, int events, int anyyet,
 *	    short *reventsp, struct pollhead **phpp)
 *
 * Calling/Exit State:
 *
 */
/* ARGSUSED */
STATIC int
sfs_poll(vnode_t *vp, int events, int anyyet,
	 short *reventsp, struct pollhead **phpp)
{
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;

	vfsp = vp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	return (fs_poll(vp, events, anyyet, reventsp, phpp));
}

/*
 * int
 * sfs_pathconf(vnode_t *vp, int cmd, u_long *valp, cred_t *cr)
 *
 * Calling/Exit State:
 *	No vnode/inode locks are held on entry or exit.
 *
 * Description:
 *	No locking is necessary since the information returned
 *	by this routine is invariant. Fs_pathconf is called to
 *	obtain the information to be returned.
 */
/* ARGSUSED */
STATIC int
sfs_pathconf(vnode_t *vp, int cmd, u_long *valp, cred_t *cr)
{
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;

	vfsp = vp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	return (fs_pathconf(vp, cmd, valp, cr));
}

/*
 * int
 * sfs_getacl(vnode_t *vp, long nentries, long *dentriesp,
 *            struct acl *aclbufp, cred_t *cr, int *rvalp)
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
/*ARGSUSED*/
STATIC int
sfs_getacl(vnode_t *vp, long nentries, long *dentriesp,
	   struct acl *aclbufp, cred_t *cr, int *rvalp)
{
	inode_t	*ip = VTOI(vp);
	int 		error = 0;
	struct vfs	*vfsp = vp->v_vfsp;
	struct sfs_vfs	*sfs_vfsp;
	struct acl 	base_user  = {USER_OBJ, (uid_t) 0, (ushort) 0};
	struct acl 	base_group = {GROUP_OBJ, (uid_t) 0, (ushort) 0};
	struct acl 	base_class = {CLASS_OBJ, (uid_t) 0, (ushort) 0};
	struct acl 	base_other = {OTHER_OBJ, (uid_t) 0, (ushort) 0};
	struct acl	*tgt_aclp;	/* ptr to target ACL */
	int		rval;

	sfs_vfsp  = (struct sfs_vfs *)vfsp->vfs_data;
	if (UFSIP(ip)) {
		return (ENOSYS);
	}

	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	SFS_IRWLOCK_RDLOCK(ip);
	ASSERT(ip->i_aclcnt >= ip->i_daclcnt);

	rval = ip->i_aclcnt > ip->i_daclcnt ?
			ip->i_aclcnt + NACLBASE - 1 : ip->i_aclcnt + NACLBASE;

	if (rval > nentries) {
		error = ENOSPC;
		goto out;
	}


	/* 
	 * get USER_OBJ, CLASS_OBJ, & OTHER_OBJ entry permissions from file 
	 * owner class, file group class, and file other permission bits 
	 */
	base_user.a_perm = (ip->i_mode >> 6) & 07;
	base_class.a_perm = (ip->i_mode >> 3) & 07;
	base_other.a_perm = ip->i_mode & 07;

	tgt_aclp = aclbufp;

	/* copy USER_OBJ into caller's buffer */
	bcopy((caddr_t)&base_user, (caddr_t)tgt_aclp, sizeof(struct acl));
	tgt_aclp++;

	if (ip->i_aclcnt == ip->i_daclcnt) {
		/* 
		 * No Actual non-default ACL entries stored.
		 * Set GROUP_OBJ entry permissions same as CLASS_OBJ,
		 * and copy GROUP_OBJ, CLASS_OBJ, & OTHER_OBJ into
		 * caller's buffer.
		 */
		base_group.a_perm = base_class.a_perm;
		bcopy((caddr_t)&base_group, (caddr_t)tgt_aclp,
		      sizeof(struct acl));
		tgt_aclp++;
		bcopy((caddr_t)&base_class, (caddr_t)tgt_aclp,
		      sizeof(struct acl));
		tgt_aclp++;
		bcopy((caddr_t)&base_other, (caddr_t)tgt_aclp,
		      sizeof(struct acl));
		tgt_aclp++;
		
		/* copy default entries if any */
		if (ip->i_daclcnt && (error = sfs_aclget(ip, tgt_aclp, 0)))
			goto out;
		
		*dentriesp = ip->i_daclcnt;
		*rvalp = rval;
		goto out;
	}

	/*
	 * There are non-default entries.
	 */
	if (ip->i_daclcnt) {
		char *tmpbuf;		/* tmp buffer to store ACL */
		struct acl *tmpaclp;	/* ptr to current ACL entry */
		int tmpsize;		/* size of allocated tmp buffer */
		long aentries;		/* non-default entries */

		/*
		 * There are default entries.
		 * Allocate temporary buffer to contain stored
		 * ACL entries.  Copy the non-default entries first,
		 * followed by the CLASS_OBJ and OTHER_OBJ entries,
		 * followed by the default entries.
		 * Free the temporary buffer.
		 */
		tmpsize = ip->i_aclcnt * sizeof(struct acl);
		tmpbuf = kmem_alloc(tmpsize, KM_SLEEP);
		/* LINTED pointer alignment */
		tmpaclp = (struct acl *)tmpbuf;
		error = sfs_aclget(ip, tmpaclp, 0);
		if (error) {
			kmem_free(tmpbuf, tmpsize);
			goto out;
		}
		
		aentries = ip->i_aclcnt - ip->i_daclcnt;
		bcopy((caddr_t)tmpaclp, (caddr_t)tgt_aclp,
		      aentries * sizeof(struct acl));
		tmpaclp  += aentries;
		tgt_aclp += aentries;
		bcopy((caddr_t)&base_class, (caddr_t)tgt_aclp,
		      sizeof(struct acl));
		tgt_aclp++;
		bcopy((caddr_t)&base_other, (caddr_t)tgt_aclp,
		      sizeof(struct acl));
		tgt_aclp++;
		bcopy((caddr_t)tmpaclp, (caddr_t)tgt_aclp,
			ip->i_daclcnt * sizeof(struct acl));

		kmem_free(tmpbuf, tmpsize);

		*dentriesp = ip->i_daclcnt;
		*rvalp = rval;
		goto out;
	}

	/*
	 * There are no default entries.
	 * Copy stored ACL entries directly, followed by
	 * the CLASS_OBJ and OTHER_OBJ entries.
	 */
	error = sfs_aclget(ip, tgt_aclp, 0);
	if (!error) {
		tgt_aclp += ip->i_aclcnt;
		bcopy((caddr_t)&base_class, (caddr_t)tgt_aclp,
		      sizeof(struct acl));
		tgt_aclp++;
		bcopy((caddr_t)&base_other, (caddr_t)tgt_aclp,
		      sizeof(struct acl));
		
		*dentriesp = 0;
		*rvalp = rval;
	}
out:
	SFS_IRWLOCK_UNLOCK(ip);
	return (error);
}


/*
 * int
 * sfs_setacl(vnode_t *vp, long nentries, long dentries,
 * 	   struct acl *aclbufp, cred_t *cr)
 *	Set the ACL information for a vnode.
 *						  
 * Calling/Exit State:
 *	Input:	 Pointer to the file's inode  	  
 *		 Number of ACL entries to save
 *		 Pointer to number of default ACL entries
 *		 Pointer to user's ACL entries  
 *		 Pointer to process's credentials
 *
 * Description:
 *	The inode's rwlock is held *exclusive* while updating the
 *	inode's ACL information. The lock is held exclusive to prevent
 *	other accesses to the file's ACL information. The actual
 *	information is updated in sfs_aclstore. When all of the
 *	ACL information has been updated, the inode is updated 
 *	synchronously and atomically with respect to the new ACL
 *	information in sfs_iupdat.
 *						  
 */
/* ARGSUSED */
STATIC int
sfs_setacl(vnode_t *vp, long nentries, long dentries,
	   struct acl *aclbufp, cred_t *cr)
{
	register inode_t	*ip = VTOI(vp);
	register struct acl	*src_aclp;
	register uint 		bsize;
	register int 		i;
	struct vfs		*vfsp = vp->v_vfsp;
	sfs_vfs_t		*sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	struct buf 		*bp;
	struct fs		*fsp = ip->i_fs;
	struct aclhdr 		*ahdrp;
	struct acl 		*tmpaclp = NULL;
	daddr_t 		laclblk;
	daddr_t 		oldaclblk;
	long			aentries;	/* non-default ACL entries */
	long 			entries;	/* entries to store */
	uint 			error = 0;
	int 			oldaclcnt;
	int 			olddefcnt;
	dev_t 			dev = ip->i_dev;
	mode_t 			mode;
	struct acl		oldacls[NACLI];

	if (UFSIP(ip)) {
		return (ENOSYS);
	}
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	SFS_IRWLOCK_WRLOCK(ip);
	if (cr->cr_uid != ip->i_uid && pm_denied(cr, P_OWNER)) {
		SFS_IRWLOCK_UNLOCK(ip);
		return (EPERM);
	}

	oldaclblk = ip->i_aclblk;
	oldaclcnt = ip->i_aclcnt;
	olddefcnt = ip->i_daclcnt;
	for (i = 0; i < (oldaclcnt > NACLI ? NACLI : oldaclcnt); i++) {
		oldacls[i] = ip->i_acl[i];
	}
	aentries = nentries - dentries;		/* compute non-default count */
	mode = (aclbufp->a_perm & 07) << 6;	/* save owner perms */
	src_aclp = aclbufp + aentries - 1;	/* point at OTHER_OBJ */
	mode |= src_aclp->a_perm & 07;		/* save other perms */
	src_aclp--;				/* point at CLASS_OBJ */
	mode |= (src_aclp->a_perm & 07) << 3;	/* save file group class perms */

	if (aentries == NACLBASE) {
		/* No additional USER or GROUP entries */

		if (dentries == 0) {
			/* if no DEFAULT entries, go update the inode */
			ip->i_aclcnt = 0;
			ip->i_aclblk = (daddr_t)0;
			ip->i_daclcnt = 0;
			goto upd_inode;
		} else {
			/* else only defaults will be stored */
			entries = dentries;   /* store default entries */
			src_aclp += 2;        /* point past CLASS_OBJ & */
					      /* OTHER_OBJ at default entries */
		}
	} else {
		/*	additional USER or GROUP entries	*/
		entries = nentries - NACLBASE + 1; /* omit owner/class/other */
		aentries -= (NACLBASE - 1);
		src_aclp = aclbufp + 1;		   /* point past USER_OBJ */

		/*	
		 *	additional USER or GROUP entries,and default 
		 *	entries.  Allocate a kernel buffer so we can copy
		 *	all USER, GROUP_OBJ, GROUP, & DEFAULT entries 
		 *	in for storage in the file system in a reasonable 
		 *	fashion.
		 */
		if (dentries > 0) {
			tmpaclp = (struct acl *)kmem_alloc(entries * 
						sizeof(struct acl), KM_SLEEP);
			/* copy USER, GROUP_OBJ, & GROUP entries */
			bcopy((caddr_t)src_aclp, (caddr_t)tmpaclp, 
				aentries * sizeof(struct acl));
			/* skip past above entries, CLASS_OBJ, & OTHER_OBJ */
			src_aclp += aentries + 2;
			/* copy default entries */
			bcopy((caddr_t)src_aclp, (caddr_t)(tmpaclp + aentries), 
				dentries * sizeof(struct acl));
			src_aclp = tmpaclp;
		}	/* end else */
	}	/* end else */

	/* go store the entries on the file */
	error = sfs_aclstore(ip, src_aclp, entries, dentries, cr);
	if (error != 0) {
		ip->i_aclblk = oldaclblk;
		ip->i_aclcnt = oldaclcnt;
		ip->i_daclcnt = olddefcnt;
		for (i = 0; i < (oldaclcnt > NACLI ? NACLI : oldaclcnt); i++) {
			ip->i_acl[i] = oldacls[i];
		}
		if (tmpaclp) {
			kmem_free(tmpaclp, entries * sizeof (struct acl));
		}
		goto out;
	}

upd_inode:
	ip->i_mode &= ~(ushort)PERMMASK;
	ip->i_mode |= mode; 
	ip->i_flag |= (ISYNC | ICHG);
	sfs_iupdat(ip, 1);

	/* Release temporary buffer */
	if (tmpaclp)
		kmem_free(tmpaclp, entries * sizeof (struct acl));

	/* Release all old ACL blocks now */
	entries = (oldaclcnt > NACLI) ? (oldaclcnt - NACLI) : 0;

	while (oldaclblk) {
		ASSERT(entries > 0);
		bsize = fragroundup(fsp, (entries * sizeof(struct acl)) +
					sizeof(struct aclhdr));
		if (bsize > fsp->fs_bsize) 
			bsize = fsp->fs_bsize;
		bp = pbread(dev, NSPF(fsp) * oldaclblk, bsize);
		if (bp->b_flags & B_ERROR) {
			sfs_free(ip, oldaclblk, bsize);
			brelse(bp);
			goto out;
		}
		ahdrp = (struct aclhdr *)(bp->b_addrp);
		laclblk = ahdrp->a_nxtblk;
		entries -= ahdrp->a_size;
		sfs_free(ip, oldaclblk, bsize);
		brelse(bp);
		if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
			goto out;
		oldaclblk = laclblk;
	}
out:
	SFS_IRWLOCK_UNLOCK(ip);
	return (error);
}

/*
 * int
 * sfs_setlevel(vnode_t *vp, lid_t level, cred_t *credp)
 *	Set the security level of a vnode.
 *
 * Calling/Exit State:
 *	None.
 *
 */
/* ARGSUSED */
STATIC int
sfs_setlevel(vnode_t *vp, lid_t level, cred_t *credp)
{
	inode_t		*ip;
	int		error;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;
	pl_t		opl;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	ip = VTOI(vp);
	if (UFSIP(ip)) {
		return (ENOSYS);
	}

	vfsp = vp->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	if (sfs_vfsp->vfs_flags & SFS_FSINVALID) {
		return (EIO);
	}

	error = 0;

	SFS_IRWLOCK_WRLOCK(ip);

	/*
	 * Make sure calling process is owner or has override privilege.
	 */
	
	if (ip->i_uid != credp->cr_uid && pm_denied(credp, P_OWNER)) {
		error = EPERM;
		goto out;
	}

	/*
	 * The root vnode of a file system is considered "non-tranquil".
	 */

	if (vp->v_flag & VROOT) {
		error = EBUSY;
		goto out;
	}

	/*
	 * If MAC is installed, then we can only change LID when
	 * there is no subject in the system that has a handle on
	 * the vp object.
	 */

	VN_LOCK(vp);
#ifdef _SFS_SOFT_DNLC
	if (mac_installed && vp->v_count > 1) {
#else	/* _SFS_SOFT_DNLC */
	/*
	 * v_softcnt is incremented to indicate that v_count is comming
	 * from other than the subject has handle on it.
	 */
	if (mac_installed && ((vp->v_count - vp->v_softcnt) > 1)) {
#endif	/* _SFS_SOFT_DNLC */
		VN_UNLOCK(vp);
		error = EBUSY;
	} else {
		vp->v_lid = ip->i_lid = level;
		VN_UNLOCK(vp);
		opl = SFS_ILOCK(ip);
		IMARK(ip, ICHG);
		SFS_IUNLOCK(ip, opl);
		sfs_iupdat(ip, 0);
	}

out:
	SFS_IRWLOCK_UNLOCK(ip);
	return (error);
}

/*
 * int
 * sfs_makemld(vnode_t *dvp, char *dirname, vattr_t *vap,
 *             vnode_t **vpp, cred_t *credp)
 *
 * Calling/Exit State:
 *	No inode locks held on entry or exit.
 */
/* ARGSUSED */
STATIC int
sfs_makemld(vnode_t *dvp, char *dirname, vattr_t *vap,
	    vnode_t **vpp, cred_t *credp)
{
	inode_t *ip, *dp;
	int error;

	ASSERT((vap->va_mask & (AT_TYPE|AT_MODE)) == (AT_TYPE|AT_MODE));

	dp = VTOI(dvp);
	if (!UFSIP(dp)) {
		error = sfs_direnter(dp, dirname, DE_MKMLD, (inode_t *) 0,
		    (inode_t *) 0, vap, &ip, credp);
		if (error == 0) {
			*vpp = ITOV(ip);
			SFS_IRWLOCK_UNLOCK(ip);
		} else if (error == EEXIST)
			sfs_iput(ip, credp);
	} else {
		error = ENOSYS;
	}

	return error;
}
