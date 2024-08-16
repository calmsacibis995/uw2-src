/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/s5fs/s5vnops.c	1.70"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <acc/dac/acl.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/dirent.h>
#include <fs/fbuf.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/flock.h>
#include <fs/fs_subr.h>
#include <fs/pathname.h>
#include <fs/s5fs/s5data.h>
#include <fs/s5fs/s5dir.h>
#include <fs/s5fs/s5filsys.h>
#include <fs/s5fs/s5hier.h>
#include <fs/s5fs/s5inode.h>
#include <fs/s5fs/s5macros.h>
#include <fs/s5fs/s5param.h>
#include <fs/specfs/snode.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/open.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <mem/seg_map.h>
#include <mem/seg_vn.h>
#include <mem/swap.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/resource.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>

extern int	s5_bmappage(vnode_t *, off_t, size_t, page_t **,
			daddr_t *, daddr_t *, int, cred_t *);
extern	int	s5_direnter(inode_t *, char *, enum de_op, inode_t *,
			    inode_t *, vattr_t *, inode_t **, cred_t *);
extern	int	s5_dirlook(inode_t *, char *, inode_t **, int, cred_t *);
extern	int	s5_dirremove(inode_t *, char *, inode_t *, vnode_t *,
			     enum dr_op, cred_t *);
extern	int	s5_iaccess(inode_t *, mode_t, cred_t *);
extern	int	s5_itrunc(inode_t *, uint_t);
extern	int	s5_rdwri(enum uio_rw, inode_t *, caddr_t, int, off_t,
			 enum uio_seg, int, int *);
extern	int	s5_readi(inode_t *, uio_t *, int);
extern	int	s5_writei(inode_t *, uio_t *, int);
extern	int	s5_sync(vfs_t *, int, cred_t *);
extern	int	s5_syncip(inode_t *, int, enum iupmode);
extern	int	s5_idestroy(inode_t *);
extern	void	s5_iput(inode_t *);
extern	void	s5_iupdat(inode_t *);
extern int specpreval(vtype_t, dev_t, cred_t *);
extern void map_addr(vaddr_t *, uint_t, off_t, int);

/*
 * UNIX file system operations vector.
 */
STATIC int	s5_open(vnode_t **, int, cred_t *);
STATIC int	s5_close(vnode_t *, int, boolean_t , off_t, cred_t *);
STATIC int	s5_read(vnode_t *, struct uio *, int, cred_t *);
STATIC int	s5_write(vnode_t *, struct uio *, int, cred_t *);
STATIC int	s5_ioctl(vnode_t *, int, int, int, cred_t *, int *);
STATIC int	s5_getattr(vnode_t *, vattr_t *, int, cred_t *);
STATIC int	s5_setattr(vnode_t *, vattr_t *, int, int, cred_t *);
STATIC int	s5_access(vnode_t *, int, int, cred_t *);
STATIC int	s5_lookup(vnode_t *, char *, vnode_t **, pathname_t *, int,
			  vnode_t *, cred_t *);
STATIC int	s5_create(vnode_t *, char *, vattr_t *, enum vcexcl, int,
			vnode_t **, cred_t *);
STATIC int	s5_remove(vnode_t *, char *, cred_t *);
STATIC int	s5_link(vnode_t *, vnode_t *, char *, cred_t *);
STATIC int	s5_rename(vnode_t *, char *, vnode_t *, char *, cred_t *);
STATIC int	s5_mkdir(vnode_t *, char *, vattr_t *, vnode_t **, cred_t *);
STATIC int	s5_rmdir(vnode_t *, char *, vnode_t *, cred_t *);
STATIC int	s5_readdir(vnode_t *, struct uio *, cred_t *, int *);
STATIC int	s5_symlink(vnode_t *, char *, vattr_t *, char *, cred_t *);
STATIC int	s5_readlink(vnode_t *, struct uio *, cred_t *);
STATIC int	s5_fsync(vnode_t *, cred_t *);
STATIC int	s5_fid(vnode_t *, struct fid **);
STATIC int	s5_seek(vnode_t *, off_t, off_t *);
STATIC int	s5_frlock(vnode_t *, int, struct flock *, int, off_t, cred_t *);
STATIC void	s5_inactive(vnode_t *, cred_t *);
void		s5_release(vnode_t *);
STATIC void	s5_rwunlock(vnode_t *, off_t, int);
STATIC int	s5_rwlock(vnode_t *, off_t, int, int, int);
STATIC int	s5_getpage(vnode_t *, uint_t, uint_t, uint_t *, page_t **,
			uint_t, struct seg *, vaddr_t, enum seg_rw, cred_t *);
STATIC int	s5_putpage(vnode_t *, off_t, uint_t, int, cred_t *);
STATIC int	s5_map(vnode_t *, off_t, struct as *, vaddr_t *, uint_t,
			uint_t, uint_t, uint_t, cred_t *);
STATIC int	s5_addmap(vnode_t *, uint_t, struct as *, vaddr_t, uint_t,
			uint_t, uint_t, uint_t, cred_t *);
STATIC int	s5_delmap(vnode_t *, uint_t, struct as *, vaddr_t, uint_t,
			uint_t, uint_t, uint_t, cred_t *);
STATIC int	s5_stablestore(vnode_t **, off_t *, size_t *, void **, cred_t *);
STATIC int 	s5_relstore(vnode_t *, off_t, size_t, void *, cred_t *);
STATIC int 	s5_getpagelist(vnode_t *, off_t, uint_t, page_t *,
		  void *, int, cred_t *);
STATIC int 	s5_putpagelist(vnode_t *, off_t, page_t *, void *, int, cred_t *);

vnodeops_t s5vnodeops = {
	s5_open,
	s5_close,
	s5_read,
	s5_write,
	s5_ioctl,
	fs_setfl,
	s5_getattr,
	s5_setattr,
	s5_access,
	s5_lookup,
	s5_create,
	s5_remove,
	s5_link,
	s5_rename,
	s5_mkdir,
	s5_rmdir,
	s5_readdir,
	s5_symlink,
	s5_readlink,
	s5_fsync,
	s5_inactive,
	s5_release,	/* release */
	s5_fid,
	s5_rwlock,
	s5_rwunlock,
	s5_seek,
	fs_cmp,
	s5_frlock,
	(int (*)())fs_nosys,	/* realvp */
	s5_getpage,
	s5_putpage,
	s5_map,
	s5_addmap,
	s5_delmap,
	(int (*)())fs_poll,
	fs_pathconf,
	(int (*)())fs_nosys, 	/* getacl */
	(int (*)())fs_nosys, 	/* setacl */
	(int (*)())fs_nosys, 	/* setlevel */
	(int (*)())fs_nosys, 	/* getdvstat */
	(int (*)())fs_nosys, 	/* setdvstat */
	(int (*)())fs_nosys, 	/* makemld */
	(int (*)())fs_nosys,	/* testmld */
	s5_stablestore,
	s5_relstore,
	s5_getpagelist,
	s5_putpagelist,
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
int s5_stickyhack = 1;

/*
 * int
 * s5_open(vnode_t **vpp, int flag, cred_t *cr)
 *	Open a file.
 *
 * Calling/Exit State:
 *	No lock is held on entry and exit.
 *
 * Description:
 * 	No special action required for ordinary files.
 *	Devices are handled through the SPECFS.
 */
/* ARGSUSED */
STATIC int
s5_open(vnode_t **vpp, int flag, cred_t *cr)
{
	return 0;
}

/*
 * int 
 * s5_close(vnode_t *vp, int flag, boolean_t lastclose, off_t offset,
 *	    cred_t *cr)
 * 	Close a protocol.
 *
 * Calling/Exit State:
 *	No lock is held on entry and exit.
 *
 * Description:
 *	The inode's rwlock is held to prevent another LWP in the same
 *	process from establishing a file/record lock on the file.
 */
/* ARGSUSED */
STATIC int
s5_close(vnode_t *vp, int flag, boolean_t lastclose, off_t offset, cred_t *cr)
{
	inode_t	*ip;

	ip = VTOI(vp);
	if (vp->v_filocks) {
		S5_IRWLOCK_WRLOCK(ip);
		cleanlocks(vp, u.u_procp->p_epid, u.u_procp->p_sysid);
		S5_IRWLOCK_UNLOCK(ip);
	}

	return 0;
}

/* 
 * int
 * s5_read(vnode_t *vp, uio_t *uio, int ioflag, cred_t *fcr)
 *	Transfer data from <vp> to the calling process's address
 *	space.
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode's rwlock in at least *shared*
 *	mode. This lock is must be acquired from above the VOP interface
 *	via VOP_RWRDLOCK() (below the VOP interface use s5__rwlock).
 *	VOP_RWRDLOCK() specifying the same length, offset that's
 *	in <uiop>.
 *
 *	A return value of 0 indicates success; othwerise a valid errno
 *	is returned. 
 *
 */
/* ARGSUSED */
STATIC int
s5_read(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *fcr)
{
	int	error;
	inode_t *ip;

	ip = VTOI(vp);
	error = s5_readi(ip, uiop, ioflag);
	return error;
}

/*
 * int
 * s5_write(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *fcr)
 *	Transfer data from the calling process's address space
 *	to <vp>.
 *
 * Calling/Exit State:
 *	The calling LWP holds the inode's rwlock in *exclusive* mode on
 *	entry; it remains held on exit. The rwlock was acquired by calling
 *	VOP_RWWRLOCK specifying the same length, offset pair that's
 *	in <uiop>.
 */
/* ARGSUSED */
STATIC int
s5_write(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *fcr)
{
	inode_t	*ip;
	int	error;

	ASSERT(vp->v_type == VREG);

	ip = VTOI(vp);
	if (vp->v_type == VREG
	  && (error = fs_vcode(vp, &ip->i_vcode)))
		return error;
	if ((ioflag & IO_APPEND) && vp->v_type == VREG)
		/*
		 * In append mode start at end of file.
		 * NOTE: lock not required around i_size
		 * sample since r/w lock is held exclusive
		 * by calling process.
		 */
		uiop->uio_offset = ip->i_size;
	error = s5_writei(ip, uiop, ioflag);
	if ((ioflag & IO_SYNC) && vp->v_type == VREG) {
		/*
		 * If synchronous write, update inode now.
		 */
		s5_iupdat(ip);
	}
	return error;
}

/*
 *
 * int
 * s5_ioctl(vnode_t *vp, int cmd, int arg, int flag, cred_t *cr, int *rvalp)
 *	Do nothing. 
 *
 * Calling/Exit State:
 *	No file/vnode locks held.
 *
 */
/* ARGSUSED */
STATIC int
s5_ioctl(vnode_t *vp, int cmd, int arg, int flag, cred_t *cr, int *rvalp)
{
	return ENOTTY;
}

/*
 * STATIC int
 * s5_getsp(vnode_t *vp, u_long *totp)
 *	Compute the total number of blocks allocated to the file, including
 *	indirect blocks as well as data blocks, on the assumption that the
 *	file contains no holes. (It's too expensive to account for holes
 *	since that requires a complete scan of all indirect blocks.)
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 */

STATIC int
s5_getsp(vnode_t *vp, u_long *totp)
{
	s5_fs_t	*s5fsp;
	int		loc, inshift, indir;
	u_long		blocks, tot;

	s5fsp = S5FS(vp->v_vfsp);
	loc = VTOI(vp)->i_size + VBSIZE(vp) - 1;
	blocks = tot = lblkno(s5fsp, loc);
	if (blocks > NDADDR) {
		inshift = s5fsp->fs_nshift;
		indir = s5fsp->fs_nindir;
		tot += ((blocks-NDADDR-1) >> inshift) + 1;
		if (blocks > NDADDR + indir) {
			tot += ((blocks-NDADDR-indir-1) >> (inshift*2)) + 1;
			if (blocks > NDADDR + indir + indir*indir)
				tot++;
		}
	}
	*totp = tot;
	return 0;
}

/*
 * int
 * s5_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
 *	Return attributes for a vnode.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 *	The inode's rwlock is held *shared* while copying the inode's
 *	attributes.
 *
 */
/* ARGSUSED */
STATIC int
s5_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
{
	inode_t	*ip;
	int	error;
	u_long	nlblocks;
	pl_t s;

	error = 0;
	ip = VTOI(vp);
	/*
	 * Return (almost) all the attributes. This should be refined so
	 * that it only returns what's asked for.
	 */
	S5_IRWLOCK_RDLOCK(ip);
	vap->va_type = vp->v_type;
	vap->va_mode = ip->i_mode & MODEMASK;
	vap->va_uid = ip->i_uid;
	vap->va_gid = ip->i_gid;
	vap->va_fsid = ip->i_dev;
	vap->va_nodeid = ip->i_number;
	vap->va_nlink = ip->i_nlink;
	vap->va_size = ip->i_size;
	if (vp->v_type == VCHR || vp->v_type == VBLK || vp->v_type == VXNAM)
		vap->va_rdev = ip->i_rdev;
	else
		vap->va_rdev = 0;	/* not a b/c spec. */
	s = S5_ILOCK(ip);
	vap->va_atime.tv_sec = ip->i_atime;
	vap->va_atime.tv_nsec = 0;
	vap->va_mtime.tv_sec = ip->i_mtime;
	vap->va_mtime.tv_nsec = 0;
	vap->va_ctime.tv_sec = ip->i_ctime;
	vap->va_ctime.tv_nsec = 0;
	S5_IUNLOCK(ip, s);
	vap->va_type = vp->v_type;
	if (vp->v_type == VBLK || vp->v_type == VCHR)
		vap->va_blksize = MAXBSIZE;
	else
		vap->va_blksize = VBSIZE(vp);
	vap->va_vcode = ip->i_vcode;
	if (vap->va_mask & AT_NBLOCKS) {
		error = s5_getsp(vp, &nlblocks);
		if (error == 0) {
			vap->va_nblocks = FsLTOP(S5FS(vp->v_vfsp), nlblocks);
		}
	} else {
		vap->va_nblocks = 0;
	}
	if (vap->va_mask & AT_ACLCNT) {
		vap->va_aclcnt = NACLBASE;
	}
	S5_IRWLOCK_UNLOCK(ip);
	return error;
}

/*
 * int
 * s5_setattr(vnode_t *vp, vattr_t *vap, int flags, int ioflags, cred_t *cr)
 *	Modify/Set a inode's attributes.
 *
 * Calling/Exit State:
 *	The caller doesn't hold any inode locks on entry or exit.
 *
 * Description:
 *	The attributes are set up in the order of AT_SIZE,
 *	AT_MODE, AT_UID, and AT_GID because if the mode is set first
 *	then the permissions were changed and truncate will failed
 *	since the mode is not match with the old one. 
 *	.
 */
/* ARGSUSED */
STATIC int
s5_setattr(vnode_t *vp, vattr_t *vap, int flags, int ioflags, cred_t *cr)
{
	int		error;
	int		newvcode;
	long int	mask;
	inode_t		*ip;
	filsys_t	*fp;
	int		issync;
	timestruc_t	timenow;
	pl_t 		s;

	ip = VTOI(vp);

	fp = getfs(vp->v_vfsp);
	if (fp->s_ronly)
		return EROFS;

	error = newvcode = issync = 0;
	mask = vap->va_mask;
	/*
	 * Cannot set the attributes represented by AT_NOSET.
	 */
	if (mask & AT_NOSET)
		return EINVAL;

	S5_IRWLOCK_WRLOCK(ip);
	/*
	 * Truncate file. Must have write permission and file must not
	 * be a directory.
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
		error = s5_iaccess(ip, IWRITE, cr);
		if (error) {
			goto out;
		}
		if (vp->v_type == VREG && !newvcode) {
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
		error = s5_itrunc(ip, vap->va_size);
		if (error)
			goto out;
		issync++;
	}
	/*
	 * Change file access modes. Must be owner or privileged.
	 */
	if (mask & AT_MODE) {
		if (cr->cr_uid != ip->i_uid && pm_denied(cr, P_OWNER)) {
			error = EPERM;
			goto out;
		}
		ip->i_mode &= IFMT;
		ip->i_mode |= vap->va_mode & ~IFMT;
		/*
		 * A non-privileged user can set the sticky bit
		 * on a directory.
		 */
		if (vp->v_type != VDIR)
			if ((ip->i_mode & ISVTX) && pm_denied(cr, P_OWNER))
				ip->i_mode &= ~ISVTX;
		if (!groupmember(ip->i_gid, cr) && pm_denied(cr, P_OWNER))
			ip->i_mode &= ~ISGID;
		s = S5_ILOCK(ip);
		IMARK(ip, ICHG);
		S5_IUNLOCK(ip, s);
		if (MANDLOCK(vp, vap->va_mode)) {
			if (error = fs_vcode(vp, &ip->i_vcode))
				goto out;
			newvcode = 1;
		}
	}
	/*
	 * Change file ownership; must be the owner of the file
	 * or privileged. If the system was configured with
	 * the "rstchown" option, the owner is not permitted to
	 * give away the file, and can change the group id only
	 * to a group of which he or she is a member.
	 */
	if (mask & (AT_UID|AT_GID)) {
		int checksu = 0;

		if (cr->cr_uid != ip->i_uid)
			checksu = 1;
		else {
			if (rstchown) {
			    if (((mask & AT_UID) && vap->va_uid != ip->i_uid)
			      || ((mask & AT_GID) && !groupmember(vap->va_gid, cr)))
				checksu = 1;
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
		if (mask & AT_UID)
			ip->i_uid = vap->va_uid;
		if (mask & AT_GID)
			ip->i_gid = vap->va_gid;
		s = S5_ILOCK(ip);
		IMARK(ip, ICHG);
		S5_IUNLOCK(ip, s);
	}
	/*
	 * Change file access or modified times.
	 */
	if (mask & (AT_ATIME|AT_MTIME)) {
		boolean_t	mtime = B_TRUE;
		boolean_t	atime = B_TRUE;

		if (cr->cr_uid != ip->i_uid && pm_denied(cr, P_OWNER)) {
			if (flags & ATTR_UTIME)
				error = EPERM;
			else
				error = s5_iaccess(ip, IWRITE, cr);
			if (error)
				goto out;
		}
		GET_HRESTIME(&timenow);
		s = S5_ILOCK(ip);
                if (mask & AT_MTIME) {
                        if (flags & (ATTR_UTIME | ATTR_UPDTIME)) {
                                if ((flags & ATTR_UPDTIME) &&
				    (vap->va_mtime.tv_sec <= ip->i_mtime)) {
						mtime = B_FALSE;
                                } else
                                        ip->i_mtime = vap->va_mtime.tv_sec;
                        } else
                                ip->i_mtime = timenow.tv_sec;

			if (mtime == B_TRUE) {
                                ip->i_ctime = timenow.tv_sec;
				ip->i_flag &= ~(IUPD | ICHG | IACC);
				ip->i_flag |= IMODTIME;
			}
                }
		if (mask & AT_ATIME) {
                        if (flags & (ATTR_UTIME | ATTR_UPDTIME)) {
                                if ((flags & ATTR_UPDTIME) &&
                                    (vap->va_atime.tv_sec <= ip->i_atime)) {
					atime = B_FALSE;
                                } else
                                        ip->i_atime = vap->va_atime.tv_sec;
                        } else
                                ip->i_atime = timenow.tv_sec;
			if (atime == B_TRUE)
				ip->i_flag &= ~IACC;
                }

		if (mtime == B_TRUE || atime == B_TRUE)
			ip->i_flag |= IMOD;
		S5_IUNLOCK(ip, s);
	}
out:
	if ((flags & ATTR_EXEC) == 0) {
		if (issync)
			ip->i_flag |= ISYN;
		s5_iupdat(ip);
	}
	S5_IRWLOCK_UNLOCK(ip);
	return error;
}

/*
 * int
 * s5_access(vnode_t *vp, int mode, int flags, cred_t *cr)
 *	Determine the accessibility of a file to the calling
 *	process.
 *
 * Calling/Exit State:
 *	No locks held on entry; no locks held on exit.
 *	The inode's rwlock is held *shared* while determining
 *	accessibility of the file to the caller.
 *
 * Description:
 *	If the file system containing <vp> has not been sealed then
 *	obtain the inode shared/exclusive lock in shared mode. Use
 *	s5_iaccess() to determine accessibility. Return what it does
 *	after releasing the shared/exclusive lock.
 */
/* ARGSUSED */
STATIC int
s5_access(vnode_t *vp, int mode, int flags, cred_t *cr)
{
	inode_t	*ip;
	int	error;

	ip = VTOI(vp);
	S5_IRWLOCK_RDLOCK(ip);
	error = s5_iaccess(ip, mode, cr);
	S5_IRWLOCK_UNLOCK(ip);
	return error;
}

/*
 * int
 * s5_lookup(vnode_t *dvp, char *nm, vnode_t **vpp, pathname_t *pnp,
 *	     int lookup_flags, vnode_t *rootvp, cred_t *cr)
 *	Check whether a given directory contains a file named <nm>.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		ENOSYS  The file found in the directory is a special file but
 *			SPECFS was unable to create a vnode for it.
 *
 * Description:
 *	We treat null components as a synonym for the directory being
 *	searched. In this case, merely increment the directory's reference
 *	count and return. For all other cases, search the directory via
 *	s5_dirlook(). If we find the given file, indirect to SPECFS if
 *	it's a special file.
 */
STATIC int
s5_lookup(vnode_t *dvp, char *name, vnode_t **vpp, pathname_t *pnp,
	  int lookup_flags, vnode_t *rootvp, cred_t *cr)
{
	inode_t	*ip;
	inode_t	*xip;
	vnode_t *newvp;
	int	error;
	char	nm[DIRSIZ+1];

	/*
	 * Null component name is synonym for directory being searched.
	 */
	if (*name == '\0') {
		VN_HOLD(dvp);
		*vpp = dvp;
		return 0;
	}

	/*
	 * Ensure name is truncated to DIRSIZ characters.
	 */
	*nm = '\0';
	(void) strncat(nm, name, DIRSIZ);

	ip = VTOI(dvp);
	error = s5_dirlook(ip, nm, &xip, 0, cr);
	/*
	 * If we find the file in the directory, the inode is returned
	 * referenced with the rwlock held *shared*.
	 */
	if (error == 0) {
		ip = xip;
		*vpp = ITOV(ip);
		S5_IRWLOCK_UNLOCK(ip);
		/*
		 * If vnode is a device return special vnode instead.
		 */
		if (ISVDEV((*vpp)->v_type)) {
			newvp =
			  specvp(*vpp, (*vpp)->v_rdev, (*vpp)->v_type, cr);
			VN_RELE(*vpp);
			if (newvp == NULL)
				error = ENOSYS;
			else
				*vpp = newvp;
		}
	}
	return error;
}

/*
 * int
 * s5_create(vnode_t *dvp, char *name, vattr_t *vap, enum vcexcl excl,
 *	     int mode, vnode_t **vpp, cred_t *cr)
 *	Create a file in a given directory.
 *
 * Calling/Exit State:
 *	No lock is held on enty or at exit.
 *
 * Description:
 *	If the name of the file to create is null, it is treated as
 *	a synonym for the directory that the file is to be created in.
 *	In this case, the directory's rwlock is obtained in shared mode
 *	to check the calling LWP's access permission to the directory. The
 *	lock is necessary to prevent the permission vector from being
 *	changed.
 *
 *	If name is not null, direnter will create a directory entry
 *	for the new file atomically by holding the directory inode's
 *	rwlock exclusive. The directory is unlocked before returning
 *	to this routine. On success, the new inode is returned with
 *	it's rwlock held exclusive. The lock is held exclusive since
 *	the new entry may require truncation (iff AT_SIZE is specified).
 *
 */
STATIC int
s5_create(vnode_t *dvp, char *name, vattr_t *vap, enum vcexcl excl,
	  int mode, vnode_t **vpp, cred_t *cr)
{
	int	error;
	inode_t	*dip;
	inode_t	*ip;
	vnode_t *newvp;
	char	nm[DIRSIZ+1];

	dip = VTOI(dvp);
	if (*name == '\0') {
		/*
		 * Null component name refers to the directory itself.
		 */
		VN_HOLD(dvp);
		S5_IRWLOCK_RDLOCK(dip);
		error = EEXIST;
		ip = dip;
	} else {
		int i = DIRSIZ;
		char *nmp = nm;

		if (ISVDEV(vap->va_type)) {
			/*
			* Try to catch any specfs problems before writing
			* directory.
			*/
			if (error = specpreval(vap->va_type, vap->va_rdev, cr))
				return error;
		}
		/*
	 	 * Ensure name is truncated to DIRSIZ characters.
		 * *nm = '\0';
		 * (void) strncat(nm, name, DIRSIZ);
	 	 */
		while (((*nmp++ = *name++) != NULL) && (--i > 0))
			;
		*nmp = '\0';
		ip = NULL;
		/*
		 * On success, ip is returned with its rwlock held exclusive.
		 */
		error = s5_direnter(dip, nm, DE_CREATE, (inode_t *) 0,
				   (inode_t *) 0, vap, &ip, cr);
	}

	/*
	 * If the file already exists and this is a non-exclusive create,
	 * check permissions and allow access for non-directories.
	 * Read-only create of an existing directory is also allowed.
	 * We fail an exclusive create of anything which already exists.
	 */
	if (error == EEXIST) {
		if (excl == NONEXCL) {
			if (((ip->i_mode & IFMT) == IFDIR) && (mode & IWRITE))
				error = EISDIR;
			else if (mode)
				error = s5_iaccess(ip, mode, cr);
			else
				error = 0;
		}
		if (error) {
			s5_iput(ip);
			return error;
		} else if (((ip->i_mode & IFMT) == IFREG)
			  && (vap->va_mask & AT_SIZE) && vap->va_size == 0) {
			/*
			 * Truncate regular files, if requested by caller.
			 *
			 * No need to do this if error ! = EEXIST since
                         * s5_direnter made a new inode. Also, no need
                         * to update the version code if error ! = EEXIST
                         * for the same reason.
			 */
			if (MANDLOCK(ITOV(ip), ip->i_mode) &&
                            (ITOV(ip)->v_filocks != NULL)) {
                                s5_iput(ip);
                                error = EAGAIN;
			} else {
				error = s5_itrunc(ip, (u_long)0);
				if (error == 0) {
					error = fs_vcode(ITOV(ip), &ip->i_vcode);
				} else
					S5_IRWLOCK_UNLOCK(ip);
			}

		}
	}
	if (!error) {
		*vpp = ITOV(ip);
		S5_IRWLOCK_UNLOCK(ip);
		/*
	 	* If vnode is a device return special vnode instead.
	 	*/
		if (ISVDEV((*vpp)->v_type)) {
			newvp = specvp(*vpp,(*vpp)->v_rdev, (*vpp)->v_type, cr);
			VN_RELE(*vpp);
			if (newvp == NULL)
				error = ENOSYS;
			else
				*vpp = newvp;
		}

	}
	return error;
}

/*
 * int
 * s5_remove(vnode_t *dvp, char *name, cred_t *cr)
 *	Remove a file from a directory.
 *
 * Calling/Exit State:
 *	The caller holds no inode locks on entry
 *	or exit.
 *
 * Description:
 *	s5_remove uses dirremove to remove the directory
 *	while holding the containing directory's rwlock in
 *	exclusive mode.
 */
STATIC int
s5_remove(vnode_t *dvp, char *name, cred_t *cr)
{
	inode_t	*ip;
	int	error;
	char	nm[DIRSIZ+1];

	ip = VTOI(dvp);
	/*
	 * Ensure name is truncated to DIRSIZ characters.
	 */
	*nm = '\0';
	(void) strncat(nm, name, DIRSIZ);

	error = s5_dirremove(ip, nm, (struct inode *) 0, (struct vnode *) 0,
	  DR_REMOVE, cr);
	return error;
}

/*
 * int
 * s5_link(vnode_t *tdvp, vnode_t *svp, char *tname, cred_t *cr)
 *	Link a file or a directory. Only a privileged user is allowed
 *	to make a link to a directory.
 *
 * Calling/Exit State:
 *	No vnode/inode locks are held on entry or exit.
 *
 * Description:
 *	direnter adds a link to the source file in the
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
s5_link(vnode_t *tdvp, vnode_t *svp, char *tname, cred_t *cr)
{
	inode_t	*tdp;
	inode_t	*sip;
	vnode_t	*realvp;
	int	error;
	char	tnm[DIRSIZ+1];

	if (VOP_REALVP(svp, &realvp) == 0)
		svp = realvp;
	if (svp->v_type == VDIR)
		return EPERM;
	sip = VTOI(svp);
	tdp = VTOI(tdvp);

	/*
	 * Ensure name is truncated to DIRSIZ characters.
	 */
	*tnm = '\0';
	(void) strncat(tnm, tname, DIRSIZ);

	error = s5_direnter(tdp, tnm, DE_LINK, (struct inode *) 0,
		sip, (struct vattr *) 0, (struct inode **) 0, cr);
	return error;
}

/*
 * int
 * s5_rename(vnode_t *sdvp, char *snm, vnode_t *tdvp, char *tname, cred_t *cr)
 * 	Rename a file or directory.
 *
 * Calling/Exit State:
 *	 No lock is held on entry or exit.
 *
 * Description:
 * 	We are given the vnode and entry string of the source and the
 * 	vnode and entry string of the place we want to move the source
 * 	to (the target). The essential operation is:
 *		unlink(target);
 *		link(source, target);
 *		unlink(source);
 * 	but "atomically". Can't do full commit without saving state in
 * 	the inode on disk, which isn't feasible at this time. Best we
 *	 can do is always guarantee that the TARGET exists.
 *
 *	The rename is performed by a combination of dirlook, direnter
 *	and dirremove. dirlook is used to locate the file being
 *	renamed. If the file exists, it is returned with it's rwlock
 *	held shared; this lock is removed before calling direnter.
 *	direnter holds the target directory's rwlock exclusive and
 *		o removes the entry for the target file (if exists) from
 *		  the target directory
 *		o adds and entry for the source file to the target directory
 *		o if the source file is a directory, the source file's
 *		parent directory's link count is adjusted.
 *	This last step introduces the potential for A->B/B->A deadlock since
 *	both the source file's parent directory and the target directory's
 *	rwlock must be held. Deadlock would result if a rename occurs in the
 *	opposite directory, i.e., with the parent directories reversed. To
 *	prevent deadlock, all rename operations where the source file
 *	is a directory and the parent directory changes must be serialized.
 *	Serialization occurs via a per-file system sleep lock.
 */
STATIC int
s5_rename(vnode_t *sdvp, char *sname, vnode_t *tdvp, char *tname, cred_t *cr)
{
	inode_t	*sip;		/* source inode */
	inode_t	*sdp;		/* old (source) parent inode */
	inode_t	*tdp;		/* new (target) parent inode */
	int	error;
	int	have_write_error;
	char	snm[DIRSIZ+1];
	char	tnm[DIRSIZ+1];

	/*
	 * Ensure names are truncated to DIRSIZ characters.
	 */
	*snm = '\0';
	*tnm = '\0';
	(void) strncat(snm, sname, DIRSIZ);
	(void) strncat(tnm, tname, DIRSIZ);

	sdp = VTOI(sdvp);
	tdp = VTOI(tdvp);
	/*
	 * Look up inode of file we're supposed to rename.
	 */
	if (error = s5_dirlook(sdp, snm, &sip, 0, cr))
		return error;
	else {
		have_write_error = s5_iaccess(sip, IWRITE, cr);
		S5_IRWLOCK_UNLOCK(sip);
	}
	/*
	 * Make sure we can delete the source entry. This requires
	 * write permission on the containing directory. If that
	 * directory is "sticky" it further requires (except for the
	 * privileged user) that the user own the directory or the source 
	 * entry, or else have permission to write the source entry.
	 */
	S5_IRWLOCK_RDLOCK(sdp);
	error = s5_iaccess(sdp, IWRITE, cr);
	if (!error) {
		if ((sdp->i_mode & ISVTX) &&
		     cr->cr_uid != sdp->i_uid &&
		     cr->cr_uid != sip->i_uid &&
		     pm_denied(cr, P_OWNER) &&
		     have_write_error) {
			error = have_write_error;
		}
	}
	S5_IRWLOCK_UNLOCK(sdp);
	if (error != 0) {
		VN_RELE_CRED(ITOV(sip), cr);
		return error;
	}
		
	/*
	 * Check for renaming '.' or '..' or alias of '.'
	 */
	if (strcmp(snm, ".") == 0 || strcmp(snm, "..") == 0 || sdp == sip) {
		error = EINVAL;
		goto out;
	}
	/*
	 * Link source to the target.
	 */
	error = s5_direnter(tdp, tnm, DE_RENAME, sdp, sip,
		(struct vattr *) 0, (struct inode **) 0, cr);
	if (error)
		goto out;
	/*
	 * Remove the source entry. dirremove() checks that the entry
	 * still reflects sip, and returns an error if it doesn't.
	 * If the entry has changed just forget about it.  Release
	 * the source inode.
	 */
	if ((error = s5_dirremove(sdp, snm, sip, NULLVP,
	     DR_RENAME, cr)) == ENOENT)
		error = 0;

out:
	/*
	 * Check for special error return which indicates a no-op
	 * rename.
	 */
	if (error == ESAME)
		error = 0;
	VN_RELE_CRED(ITOV(sip), cr);
	return error;
}

/*
 * int
 * s5_mkdir(vnode_t *dvp, char *dirname, vattr_t *vap,
 *	    vnode_t **vpp, cred_t *cr)
 *	Create a directory file.
 *
 * Calling/Exit State:
 *	Caller holds no vnode/inode locks on entry or exit.
 *
 * Description:
 *	direnter creates a directory in <dvp> while holding
 *	the containing directory's rwlock in exclusive mode. On
 *	successful direnter()'s, the newly created directory
 *	is returned with it's rwlock held exclusive.
 */
/*ARGSUSED*/
STATIC int
s5_mkdir(vnode_t *dvp, char *dirname, vattr_t *vap,
	 vnode_t **vpp, cred_t *cr)
{
	inode_t	*ip;
	inode_t	*xip;
	int	error;
	char	dirnm[DIRSIZ+1];

	ASSERT((vap->va_mask & (AT_TYPE|AT_MODE)) == (AT_TYPE|AT_MODE));

	/*
	 * Ensure name is truncated to DIRSIZ characters.
	 */
	*dirnm = '\0';
	(void) strncat(dirnm, dirname, DIRSIZ);

	ip = VTOI(dvp);
	error = s5_direnter(ip, dirnm, DE_MKDIR, (struct inode *) 0,
		(struct inode *) 0, vap, &xip, cr);
	if (error == 0) {
		ip = xip;
		*vpp = ITOV(ip);
		S5_IRWLOCK_UNLOCK(ip);
	} else if (error == EEXIST)
		s5_iput(xip);
	return error;
}

/*
 * int
 * s5_rmdir(vnode_t *vp, char *name, vnode_t *cdir, cred_t *cr)
 *	Remove a diretory.
 *
 * Calling/Exit State:
 *	The caller holds no vnode/inode locks on entry or exit.
 */
/*ARGSUSED*/
STATIC int
s5_rmdir(vnode_t *vp, char *name, vnode_t *cdir, cred_t *cr)
{
	inode_t	*ip;
	int	error;
	char	nm[DIRSIZ+1];

	ip = VTOI(vp);
	/*
	 * Ensure name is truncated to DIRSIZ characters.
	 */
	*nm = '\0';
	(void) strncat(nm, name, DIRSIZ);

	error = s5_dirremove(ip, nm, (struct inode *) 0, cdir, DR_RMDIR, cr);
	return error;
}

#define	DIRBUFSIZE	1048
STATIC void s5_filldir();

/*
 * int
 * s5_readdir(vnode_t *vp, uio_t *uiop, cred_t *fcr, int *eofp)
 *	Read from a directory.
 *
 * Calling/Exit State:
 *	The calling LWP holds the inode's rwlock in *shared* mode. The
 *	rwlock was obtained by a call to VOP_RWRDLOCK.
 *
 *	A return value of not -1 indicates success; otherwise a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		ENOTDIR <vp> is not a directory.
 *		ENXIO
 *
 *	On success, an <*eofp> value of 1 indicates that end-of-file
 *	has been reached, i.e., there are no more directory entries
 *	that may be read.
 */
/* ARGSUSED */
STATIC int
s5_readdir(vnode_t *vp, uio_t *uiop, cred_t *fcr, int *eofp)
{
	inode_t		*ip;
	s5_fs_t	*s5fsp;
	int		bsize;
	int		bmask;
	int		error;
	int		ran_out;
	off_t		diroff;
	off_t		olddiroff = -1;
	int		oresid;
	char		*direntp;
	fbuf_t		*fbp = NULL;
	pl_t 		s;

	ip = VTOI(vp);
	error = ran_out = 0;
	oresid = uiop->uio_resid;
	s5fsp = S5FS(vp->v_vfsp);
	diroff = uiop->uio_offset;
	if (vp->v_type != VDIR) {
		return (ENOTDIR);
	}

	/*
	 * Error if not on directory entry boundary.
	 */
	if (diroff % SDSIZ != 0)
		return ENOENT;
	if (diroff < 0)
		return EINVAL;
	if (WRITEALLOWED(vp, u.u_lwpp->l_cred)) {
		s = S5_ILOCK(ip);
		IMARK(ip, IACC);
		S5_IUNLOCK(ip, s);
	}
	bsize = VBSIZE(vp);
	bmask = s5fsp->fs_bmask;
	/*
	 * Allocate space to hold dirent structures. Use the most common
	 * request size (1048).
	 */
	direntp = kmem_alloc(DIRBUFSIZE, KM_SLEEP);
	/*
	 * In a loop, read successive blocks of the directory,
	 * converting the entries to fs-independent form and
	 * copying out, until the end of the directory is
	 * reached or the caller's request is satisfied.
	 */
	do {
		int blkoff, leftinfile, leftinblock;
		char *directp;
		/* if diroff hasn't been changed terminate operation. */
		if (olddiroff == diroff) {
                        /*
                         *+ A directory on a S5 file system type contained
                         *+ an invalid directory entry.
                         */
                        cmn_err(CE_WARN, "s5_readdir: bad dir, inumber = %d\n",ip->i_number);
                        error = ENXIO;
                        goto out;
                }
		/*
		 * If at or beyond end of directory, we're done.
		 */
		if ((leftinfile = ip->i_size - diroff) <= 0)
			break;
		/*
		 * Map in next block of directory entries.
		 */
		if (error = fbread(vp, diroff & bmask, bsize, S_OTHER, &fbp))
			goto out;
		blkoff = diroff & ~bmask;	/* offset in block */
		leftinblock = MIN(bsize-blkoff, leftinfile);
		directp = fbp->fb_addr + blkoff;
		/*
		 * In a loop, fill the allocated space with fs-independent
		 * directory structures and copy out, until the current
		 * disk block is exhausted or the caller's request has
		 * been satisfied.
		 */
		do {
			int ndirent;	/* nbytes of "dirent" filled */
			int ndirect;	/* nbytes of "direct" consumed */
			int maxndirent;	/* max nbytes of "dirent" to fill */

			maxndirent = MIN(DIRBUFSIZE, uiop->uio_resid);
			s5_filldir(direntp, maxndirent, directp, leftinblock,
				diroff, &ndirent, &ndirect);
			directp += ndirect;
			leftinblock -= ndirect;
			olddiroff = diroff; /* save the old diroff */
			diroff += ndirect;
			if (ndirent == -1) {
				ran_out = 1;
				goto out;
			} else if (ndirent == 0)
				break;
			if (error = uiomove(direntp, ndirent, UIO_READ, uiop))
				goto out;
		} while (leftinblock > 0 && uiop->uio_resid > 0);
		fbrelse(fbp, 0);
		fbp = NULL;
	} while (uiop->uio_resid > 0 && diroff < ip->i_size);
out:
	/*
	 * If we ran out of room but haven't returned any entries, error.
	 */
	if (ran_out && uiop->uio_resid == oresid)
		error = EINVAL;
	if (fbp)
		fbrelse(fbp, 0);
	kmem_free(direntp, DIRBUFSIZE);
	/*
	 * Offset returned must reflect the position in the directory itself,
	 * independent of how much fs-independent data was returned.
	 */
	if (error == 0) {
		uiop->uio_offset = diroff;
		if (eofp)
			*eofp = (diroff >= ip->i_size);
	}
	if (((vp)->v_vfsp->vfs_flag & VFS_RDONLY) == 0){
		s = S5_ILOCK(ip);
		IMARK(ip, IACC);
		S5_IUNLOCK(ip, s);
	}
	return error;
}

/*
 * s5_filldir(char *direntp, int nmax, char *directp, int nleft, off_t diroff,
 *		int *idirentp, int *ndirectp)
 *
 * Calling/Exit State:
 *	Inode's rwlock lock is held in shared mode on entry or exit.
 *
 * Description: 
 * 	Convert fs-specific directory entries (direct_t) to
 * 	fs-independent form (dirent_t) in a supplied buffer.
 * 	Returns, through reference parameters, the number of bytes of
 * 	"struct dirent" with which the buffer was filled and the number of
 * 	bytes of "struct direct" which were consumed. If there was a
 * 	directory entry to convert but no room to hold it, the "number
 * 	of bytes filled" will be -1.
 */
STATIC void
s5_filldir(char *direntp, int nmax, char *directp, int nleft, off_t diroff,
	int *ndirentp, int *ndirectp)
	/* buffer to be filled */
	/* max nbytes to be filled */
	/* buffer of disk directory entries */
	/* nbytes of dir entries left in block */
	/* offset in directory */
	/* nbytes of "struct dirent" filled */
	/* nbytes of "struct direct" consumed */
{
	int	 ndirent;
	int	 ndirect;
	int	 namelen;
	int	 reclen;
	int	 direntsz;
	long	 int ino;
	direct_t *olddirp;
	dirent_t *newdirp;

	ndirent = ndirect = 0;
	/* LINTED pointer alignment */
	newdirp = (struct dirent *) direntp;
	direntsz = (char *) newdirp->d_name - (char *) newdirp;
	/* LINTED pointer alignment */
	for (olddirp = (struct direct *) directp;
	  /* LINTED pointer alignment */
	  olddirp < (struct direct *) (directp + nleft);
	  olddirp++, ndirect += SDSIZ) {
		if ((ino = olddirp->d_ino) == 0)
			continue;
		namelen = (olddirp->d_name[DIRSIZ-1] == '\0') ?
			strlen(olddirp->d_name) : DIRSIZ;
		reclen = (direntsz + namelen + 1 + (NBPW-1)) & ~(NBPW-1);
		if (ndirent + reclen > nmax) {
			if (ndirent == 0)
				ndirent = -1;
			break;
		}
		ndirent += reclen;
		newdirp->d_reclen = (short)reclen;
		newdirp->d_ino = ino;
		newdirp->d_off = diroff + ndirect + SDSIZ;
		bcopy(olddirp->d_name, newdirp->d_name, namelen);
		newdirp->d_name[namelen] = '\0';
		/* LINTED pointer alignment */
		newdirp = (struct dirent *) (((char *) newdirp) + reclen);
	}
	*ndirentp = ndirent;
	*ndirectp = ndirect;
}

/*
 * int
 * s5_symlink(vnode_t *dvp, char *linkname, vattr_t *vap, char *target,
 *	      cred_t *cr)
 *	Create a symbolic link file.
 *
 * Calling/Exit State:
 *	Caller holds no vnode/inode locks on entry or exit.
 *
 * Description:
 *	direnter creates a symbolic link in <dvp> while
 *	holding the containing directory's rwlock in exclusive
 *	mode. If the symbolic link is created, it's inode is
 *	returned with the rwlock held exclusive. This lock is
 *	held while the contents for the symbolic link are
 *	written.
 */
/*ARGSUSED*/
STATIC int
s5_symlink(vnode_t *dvp, char *linkname, vattr_t *vap,
	   char *target, cred_t *cr)
	/* dvp - ptr to parent dir vnode */
	/* linkname - name of symbolic link */
	/* vap - attributes */
	/* target - target path */
	/* cr - user credentials */
{
	inode_t	*ip;
	inode_t	*dip;
	int	error;
	char	linknm[DIRSIZ+1];

	dip = VTOI(dvp);
	/*
	 * Ensure name is truncated to DIRSIZ characters.
	 */
	*linknm = '\0';
	(void) strncat(linknm, linkname, DIRSIZ);

	if ((error = s5_direnter(dip, linknm, DE_CREATE, (struct inode *) 0,
	    (struct inode *) 0, vap, &ip, cr)) == 0) {
		error = s5_rdwri(UIO_WRITE, ip, target, strlen(target),
				(off_t) 0, UIO_SYSSPACE, IO_SYNC, (int *) 0);
		s5_iput(ip);
	} else if (error == EEXIST)
		s5_iput(ip);
	return error;
}

/*
 * int
 * s5_readlink(vnode_t *vp, uio_t *uiop, cred_t *cr)
 *	Read a symbolic link file.
 *
 * Calling/Exit State:
 *	No inode/vnode locks are held on entry or exit.
 *
 *	On success, 0 is returned; otherwise, a valid errno is
 *	returned. Errnos returned directly by this routine are:
 *		EINVAL	The vnode is not a symbolic link file.
 *
 */
/* ARGSUSED */
STATIC int
s5_readlink(vnode_t *vp, uio_t *uiop, cred_t *cr)
{
	inode_t	*ip;
	int	error;

	if (vp->v_type != VLNK)
		return EINVAL;
	ip = VTOI(vp);
	S5_IRWLOCK_RDLOCK(ip);
	error = s5_readi(ip, uiop, 0);
	S5_IRWLOCK_UNLOCK(ip);
	return error;
}

/*
 * s5_fsync(vnode_t *vp, cred_t *cr)
 *	Synchronously flush a file's modified pages to it's
 *	backing store.
 *
 * Calling/Exit State:
 *	No locks held on entry; no locks held on exit.
 *
 * Description:
 *	If the file system containing <vp> has not been sealed then
 *	obtain the inode sleep lock. Use syncip() to flush the
 *	file's pages. If that completes successfully, then
 *	update the inode's times. Return any errors
 *	after releasing the sleep lock.
 */
/* ARGSUSED */
STATIC int
s5_fsync(vnode_t *vp, cred_t *cr)
{
	inode_t	*ip;
	int	error;

	ip = VTOI(vp);
	S5_IRWLOCK_RDLOCK(ip);
	error = s5_syncip(ip, 0, IUP_SYNC); /* Do synchronous writes */
	S5_IRWLOCK_UNLOCK(ip);
	return error;
}

/*
 * s5_inactive(vnode_t *vp, cred_t *cr)
 *	Perform cleanup on an unreferenced inode.
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	s5_inactive is called without applying any locks. s5_iinactive
 *	will check is there is another LWP waiting the inode to become
 *	available, i.e., another LWP holds the inode's rwlock and
 *	is spinning in VN_HOLD. In this situation, the inode is not
 *	released and is given to the waiting LWP. Another interaction
 *	with s5_iget is checked for: s5iget establishes a reference
 *	to an inode by obtaining the inode table lock, searching a hash
 *	list for an inode, and then, atomically enqueuing for the
 *	inode's rwlock while dropping the inode table lock. s5_iinactive
 *	will obtain the inode table lock and check whether any LWPs are
 *	queued on the inode's rwlock. If there are any blocked LWPs,
 *	the inode is not inactivated.
 */
/* ARGSUSED */
STATIC void
s5_inactive(vnode_t *vp, cred_t *cr)
{
	inode_t	*ip = VTOI(vp);
	boolean_t	totally_free;
	struct	inode_marker	*free_list;
	int	flags;


	ASSERT(KS_HOLD0LOCKS());
        ASSERT(getpl() == PLBASE);

	/*
	 * At this point, a new reference to the inode can be still be
	 * generated by one of:
	 * 
	 *	=> s5_iget
	 *
	 *	=> s5_igrab
	 *
	 * In all three cases, the LWP generating the reference holds
	 * either the VN_LOCK or the s5_inode_table_mutex. So, acquire both
	 * locks now. We also acquire the s5_inode_table_mutex so that we
	 * can transition the free/destroy state of the inode atomically with
	 * changing its v_count/v_softcnt fields.
	 */
	(void) LOCK(&s5_inode_table_mutex, FS_S5LISTPL);
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
		VN_UNLOCK(vp);
		UNLOCK(&s5_inode_table_mutex, PLBASE);
		return;
	} 
	/*
	 * The reference count is exactly 1, so that now we can be sure
	 * that we really hold the last hard reference.
	 *
	 * We exchange our hard count (v_count) for a soft count (v_softcnt)
	 * in order to suppress any new references via VN_TRY_HOLD once we
	 * give up the VN_LOCK. Even after we give up the VN_LOCK, we will
	 * still hold the s5_inode_table_mutex, thus inhibiting any
	 * new VN_HOLDs via s5_iget.
	 *
	 * Note: the pages remain visible to the pageout and fsflush daemons.
	 */
	vp->v_count = 0;
	++vp->v_softcnt;
	VN_UNLOCK(vp);

	/*
	 * If we are removing the file, then we
	 * commit to destroying the inode's identity.
	 *
	 * Otherwise, we just free the inode and let the fsflush/pageout
	 * daemons clean the pages/inode.
	 */
	totally_free = (vp->v_softcnt == 1 && vp->v_pages == NULL);
	if (ip->i_nlink <= 0) {
		ip->i_state = (IDENTITY|INVALID|ILCLFREE);
	} else {
		/*
		 * Freeing the inode but not destroying it.
		 */
		if (totally_free) {
			free_list = &s5_totally_free;
			flags = ITFREE;
			MET_INODE_INUSE(MET_S5,-1);
		} else {
			free_list = &s5_partially_free;
			flags = IPFREE;
		}
		S5_FREE_TAIL(free_list, ip, flags);
		ASSERT(ip->i_state == (IDENTITY|flags));
		S5_FREE_CHECK();
		UNLOCK(&s5_inode_table_mutex, PLBASE);
		return;
	}
	UNLOCK(&s5_inode_table_mutex, PLBASE);
	S5_IRWLOCK_WRLOCK(ip);

	/*
	 * At this point, we are committed to destroying the
	 * inode's identity. If destruction fails, then the inode
	 * will go onto the free list. No error handling is needed
	 * here.
	 */
	(void) s5_idestroy(ip);
}

/*
 * void
 * s5_release(vnode_t *vp)
 *	Release the storage for a totally unreferenced vnode.
 *
 * Calling/Exit State:
 *	The user may hold locks. However, no lock is held at FS_S5LISTPL
 *	or above.
 *
 *	This function does not block.
 */
void
s5_release(vnode_t *vp)
{
	inode_t   	*ip = VTOI(vp);
	pl_t		s;

	/*
	 * The inode is privately held at this point.
	 * Therefore, no locking is necesary in order to inspect it.
	 */
	ASSERT(VN_IS_RELEASED(ITOV(ip)));
	ASSERT(ITOV(ip)->v_pages == NULL); 

	if (ip->i_state == (INVALID|ILCLFREE) || ip->i_state == (INVALID|IDENTITY)) {
		ITOV(ip)->v_softcnt = 1;
		ip->i_state = 0;
		s = LOCK(&s5_inode_table_mutex, FS_S5LISTPL);
		S5_FREE_HEAD(&s5_totally_free, ip, ITFREE);
		S5_FREE_CHECK();
		UNLOCK(&s5_inode_table_mutex, s);
		MET_INODE_INUSE(MET_S5,-1);
	} else {
		ASSERT(ip->i_state == INVALID);
	}

	/*
	 * Remember to wake up any waiters
	 */
	if (SV_BLKD(&s5_inode_sv))
		SV_BROADCAST(&s5_inode_sv, 0);

	return;
}


/*
 * int
 * s5_fid(vnode_t *vp, fid_t **fidpp)
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
s5_fid(vnode_t *vp, fid_t **fidpp)
{
	ufid_t	*ufid;

	ufid = (struct ufid *) kmem_zalloc(sizeof(struct ufid), KM_SLEEP);
	if (ufid == NULL)
		return (NULL);
	ufid->ufid_len = sizeof(struct ufid) - sizeof(u_short);
	ufid->ufid_ino = VTOI(vp)->i_number;
	ufid->ufid_gen = VTOI(vp)->i_gen;
	*fidpp = (struct fid *) ufid;
	return (0);
}

/*
 * int
 * s5_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
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
s5_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
{
	inode_t *ip;
	int	error;

	ip = VTOI(vp);
	error = 0;

	if (mode == LOCK_EXCL) {
		S5_IRWLOCK_WRLOCK(ip);
	} else if (mode == LOCK_SHARED) {
		S5_IRWLOCK_RDLOCK(ip);
	} else {
		/*
		 *+ An invalid mode was passed as a parameter to
		 *+ this routine. This indicates a kernel software
		 *+ problem
		 */
		cmn_err(CE_PANIC,"s5__rwlock: invalid lock mode requested");
	}

	if (MANDLOCK(vp, ip->i_mode)) {
		if (fmode) {
			error = chklock(vp, (mode == LOCK_SHARED) ? FREAD :
					FWRITE, off, len, fmode, ip->i_size);
		}
	}

	if (error) {
		S5_IRWLOCK_UNLOCK(ip);
	}

	return (error);
}

/*
 * STATIC void
 * s5_rwunlock(vnode_t *vp, off_t off, int len)
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
s5_rwunlock(vnode_t *vp, off_t off, int len)
{

	inode_t	*ip;

	ip = VTOI(vp);
	S5_IRWLOCK_UNLOCK(ip);
}

/*
 * int
 * s5_seek(vnode_t *vp, off_t ooff, off_t *noffp)
 *	Validate a seek pointer.
 *
 * Calling/Exit State:
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		EINVAL	The new seek pointer is negative.
 *
 * Description:
 *	No locking is necessary since the result of this routine
 *	depends entirely on the value of <*noffp>.
 */
/* ARGSUSED */
STATIC int
s5_seek(vnode_t *vp, off_t ooff, off_t *noffp)
{
	return *noffp < 0 ? EINVAL : 0;
}

/*
 *
 * int
 * s5_frlock(vnode_t *vp, int cmd, struct flock *bfp,
 *	     int flag, off_t offset, cred_t *cr)
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
s5_frlock(vnode_t *vp, int cmd, flock_t *bfp,
	  int flag, off_t offset, cred_t *cr)
{
	inode_t *ip;
	int	error;

	ip = VTOI(vp);

	if (cmd == F_SETLK || cmd == F_SETLKW) {
		S5_IRWLOCK_WRLOCK(ip);
	} else {
		S5_IRWLOCK_RDLOCK(ip);
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
	S5_IRWLOCK_UNLOCK(ip);
	return (error);
}


int s5_ra = 1;

/*
 * int
 * s5_getpageio(vnode_t *vp, off_t off, uint_t len, page_t *pp,
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
s5_getpageio(inode_t *ip, off_t off, uint_t len, daddr_t *io_list,
	page_t *pp, int flag)
{
	vnode_t *devvp;
	s5_fs_t *s5fsp;
	buf_t *bplist[PAGESIZE/NBPSCTR];
	buf_t **bpp;
	int bsize;
	int nio, blkon;
	int dblks = 0;
	off_t curoff;
	int i, blkpp, err = 0, bio_err;

	s5fsp = S5FS(ITOV(ip)->v_vfsp);
        devvp = s5fsp->fs_devvp;
	bsize = VBSIZE(ITOV(ip));

	ASSERT(len != 0);
	ASSERT(len <= MAX(PAGESIZE, bsize));
	ASSERT((off & PAGEOFFSET) == 0);
	ASSERT(pp != NULL);

	if (bsize >= PAGESIZE)
		blkpp = 1;
	else
		blkpp = PAGESIZE/bsize;

	blkon = btodb(blkoff(s5fsp, off));

	ASSERT(blkon == 0 || blkpp == 1);

	nio = pp->p_nio;

	curoff = off & PAGEOFFSET;
	for (bpp = bplist, i = 0; i < blkpp && curoff < ip->i_size;
						i++, curoff += bsize) {
		if (io_list[i] == DB_HOLE)
			continue;

		*bpp = pageio_setup(pp, curoff, MIN(len, bsize), flag | B_READ);

		dblks += btodb((*bpp)->b_bcount);

		(*bpp)->b_edev = ip->i_dev;
		(*bpp)->b_blkno = LTOPBLK(io_list[i], bsize) + blkon;

		(*bdevsw[getmajor(devvp->v_rdev)].d_strategy)(*bpp);
		bpp++;
	}

	ASSERT((bpp - bplist) == nio);

	/* Update the number of dev sized blocks read by this LWP */
	ldladd(&u.u_ior, dblks);

#ifdef PERF
	mets_fsinfo[MET_S5].pgin++;
	/*
	 * btopr(len) isn't the right number of pages if there are big holes.
	 * But it's probably a good estimate in most cases.
	 */
	mets_fsinfo[MET_S5].pgpgin += btopr(len);
	mets_fsinfo[MET_S5].sectin += dblks;
#endif /* PERF */

	if (flag & B_ASYNC) {
#ifdef PERF
		/*
		 * btopr(len) isn't the right number of pages if there
		 * are big holes.  But it's probably a good estimate
		 * in most cases.
		 */
		mets_fsinfo[MET_S5].rapgpgin += btopr(len);
		mets_fsinfo[MET_S5].rapgpgin += dblks;
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
 * s5_getapage(vnode_t *vp, uint_t off, uint_t len, uint_t protp, page_t *pl[],
 *	 uint_t plsz, struct seg *seg, addr_t addr, enum seg_rw rw, cred_t *cr)
 *
 * Calling/Exit State:
 *	The inode rwlock may or may not be held locked.
 *	The inode getpage lock is locked shared on entry and remains
 *	locked at exit.
 *
 * Description:
 *	Called from pvn_getpages() or s5_getpage() to get a particular page.
 *	When we are called the inode is already locked.
 *
 *	If rw == S_WRITE and block is not allocated, need to alloc block.
 *	If ppp == NULL, async I/O is requested.
 *
 */
/* ARGSUSED */
STATIC int
s5_getapage(vnode_t *vp, uint_t off, uint_t len, uint_t *protp, page_t *pl[],
	uint_t plsz, struct seg *seg, vaddr_t addr, enum seg_rw rw, cred_t *cr)
{
	struct inode *ip;
	page_t *pp, **ppp;
	off_t roff, io_off;
	uint_t io_len;
	page_t *io_pl[MAXBSIZE/PAGESIZE];
	daddr_t	io_list[PAGESIZE/NBPSCTR];
	daddr_t	local_dblist[PAGESIZE/NBPSCTR], *dblist, *dbp;
	off_t lbnoff, curoff;
	int bsize, blksz, i, blkpp, minsize;
	size_t saved_isize;
	int sz, j;
	int err;
	boolean_t page_uninit;

	ip = VTOI(vp);
	bsize = VBSIZE(vp);
	lbnoff = off & ~(bsize - 1);
	roff = off & PAGEMASK;

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

	for (i = blkpp; i-- != 0;)
		local_dblist[i] = io_list[i] = DB_HOLE;

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

		    if (blkpp > PG_DBSIZE) {
			/*
			 * If db doesn't fit in p_dblist, we need to get it
			 * by calling bmappage.
			 */
		    	dblist = local_dblist;
		    	err = s5_bmappage(vp, off, len, &pp, dblist,
		    				io_list, rw, cr);
		    	if (err)
		    		return err;
		    } else {

			/*
			 * If db does fit in p_dblist, we can use it
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

			    	    err = s5_bmappage(vp, off, len, &pp, dblist,
			    			io_list, rw, cr);
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
		    if (pl != NULL) {
			*pl++ = pp;
			*pl = NULL;
		    } else
			page_unlock(pp);

		    return 0;
		}
	} else {
		/*
		 * This is a read-ahead, call page_lazy_create()
		 * which will create the page only if memory is readily
		 * available.
		 */
		pp = page_lazy_create(vp, roff);

		if (pp == NULL)
			return 0;
		saved_isize = ip->i_size;
	}

	ASSERT(pp != NULL);
	ASSERT(PAGE_IS_WRLOCKED(pp));

	if (blkpp > PG_DBSIZE)
		dblist = local_dblist;
	else
		dblist = PG_DBLIST(pp);
	err = s5_bmappage(vp, off, len, &pp, dblist, io_list, rw, cr);

	/*
	 * If bmap fails, pages should have been unlocked by bmap.
	 */
	if (err)
		return (err);

	/*
	 * If page is returned with a reader-lock, we had to drop
	 * the page lock in s5_bmappage() and someone else re-
	 * instantiated the page, we need to go back and
	 * recheck the dblist to see if there is any more work
	 * for us to do.
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
		 * Compute size we really want to get. Excluding pages
		 * past the old isize.
		 */
		if (saved_isize > lbnoff)
			blksz = MIN(roundup(saved_isize, PAGESIZE) -
						lbnoff, bsize);
		else
			blksz = bsize;

		if (bsize > PAGESIZE && off < saved_isize) {
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
			}
		    }
		    ASSERT (npages_ok > 0);
		    /* Adjust io_len if some pages were dropped. */
		    if (npages_ok != npages)
			io_len = io_off + (npages_ok * PAGESIZE);
		} else {
			io_off = roff;
			io_len = ((bsize < PAGESIZE) ? (lbnoff - roff + blksz) :						blksz);
			io_pl[0] = pp;
		}

		pagezero(pp->p_prev, io_len & PAGEOFFSET,
				PAGESIZE - (io_len & PAGEOFFSET));
	} else {
		io_off = roff;
		io_len = PAGESIZE;
		io_pl[0] = pp;
	}

	err = s5_getpageio(ip, io_off, io_len, io_list, pp,
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

/*
 * int
 * s5_getpage(vnode_t *vp, uint_t off, uint_t len, uint_t *protp, page_t *pl[],
 *	uint_t plsz, struct seg *seg, vaddr_t addr, enum seg_rw rw, cred_t *cr)
 *
 * Calling/Exit State:
 *	The inode rwlock may be held shared (if called from readi)
 *	or exclusive (if called from writei) on entry.
 *
 * Description:
 *	Return all the pages from [off..off+len) in given file.
 */

/* ARGSUSED */
STATIC int
s5_getpage(vnode_t *vp, uint_t off, uint_t len, uint_t *protp, page_t *pl[],
	uint_t plsz, struct seg *seg, vaddr_t addr, enum seg_rw rw, cred_t *cr)
{
	inode_t *ip = VTOI(vp);
        uint_t rlen, nextoff;
	int err;
	pl_t s;

#ifdef PERF
	mets_fsinfo[MET_S5].getpage++;
#endif
	ASSERT(!(vp->v_flag & VNOMAP));

	/*
	 * This check for beyond EOF allows the request to extend up to
	 * the page boundary following the EOF.
	 */
	s = S5_ILOCK(ip);
        if (rw != S_OVERWRITE) {
		if (off + len > ip->i_size + PAGEOFFSET) {
			S5_IUNLOCK(ip, s);
			return (EFAULT);	/* beyond EOF */
		} else if (off + len > ip->i_size)
			len = ip->i_size - off;
	}
	S5_IUNLOCK(ip, s);

	if (protp != NULL)
		*protp = PROT_ALL;

	ASSERT(pl == NULL || plsz >= PAGESIZE);
        if (btop(off) == btop(off + len - 1)) {
                err = s5_getapage(vp, off, len, protp, pl, plsz, seg, addr,
                    rw, cr);
	} else
		err = pvn_getpages(s5_getapage, vp, off, len, protp, pl, plsz,
		  seg, addr, rw, cr);
	if (err)
		goto err_out;

	nextoff = ptob(btopr(off + len));
	s = S5_ILOCK(ip);
	if (!err && s5_ra && ip->i_nextr == off &&
				nextoff < ip->i_size && rw != S_OVERWRITE) {
		if (nextoff + PAGESIZE > ip->i_size)
			rlen = ip->i_size - nextoff;
		else
			rlen = PAGESIZE;
		/*
		 * For read-ahead, pass a NULL pl so that if it's
		 * not convenient, io will not be done. And also,
		 * if io is done, we don't wait for it.
		 */
		S5_IUNLOCK(ip, s);
#ifdef PERF
		mets_fsinfo[MET_S5].ra++;
#endif
		err = s5_getapage(vp, nextoff, rlen, NULL, NULL, 0,
				seg, addr, S_OTHER, cr);
	} else
		S5_IUNLOCK(ip, s);

	ip->i_nextr = nextoff;

	/*
	 * If the inode is not already marked for IACC (in readi() for read)
	 * and the inode is not marked for no access time update (in writei()
	 * for write) then update the inode access time and mod time now.
	 */
	if ((ip->i_flag & (IACC | INOACC)) == 0 &&  WRITEALLOWED(vp, cr)) {
		s = S5_ILOCK(ip);
		if (rw != S_OTHER)
			ip->i_flag |= IACC;
		if (rw == S_WRITE )
			ip->i_flag |= IUPD;
		IMARK(ip, ip->i_flag);
		S5_IUNLOCK(ip, s);
	}

err_out:
	return(err);
}

/*
 * int
 * s5_getpagelist(vnode_t *vp, off_t off, page_t *pp, uint_t plsz,
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
s5_getpagelist(vnode_t *vp, off_t off, uint_t len, page_t *pp,
		  void *bmapp, int flags, cred_t *cr)
{
	struct inode *ip = VTOI(vp);
#ifdef DEBUG
	page_t *pp2;
#endif
	s5_fs_t *s5fsp;
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

	s5fsp = S5FS(ITOV(ip)->v_vfsp);
	bsize = VBSIZE(ITOV(ip));

	if (bsize >= PAGESIZE)
		blkpp = 1;
	else
		blkpp = PAGESIZE/bsize;

	plist = pp;
	do {

		pp = plist;
		io_list = ((daddr_t *)bmapp + (off >> s5fsp->fs_bshift)); 

		io_off = off;
		io_len = 0;
		io_pl = NULL;
		do {
			io_len += PAGESIZE;
			page_sub(&plist, pp);
			page_sortadd(&io_pl, pp);
			pp->p_nio = (uchar_t)blkpp;
			pp = plist;
		} while (((io_off += PAGESIZE) < off + bsize) && pp != NULL);

		ASSERT(io_pl != NULL);
		err = s5_getpageio(ip, off, io_len, io_list, io_pl, 0);

		if (err)
			break;

		off = io_off;

	} while (plist != NULL);

	return err;
}

/*
 * int
 * s5_putpageio(inode_t *ip, off_t off, size_t len, page_t *pp,
 *	daddr_t *io_list, int flags)
 *
 * Calling/Exit State:
 *	The inode rwlock may or may not be held locked on entry.
 *
 * Description:
 * 	Set up for page io and call the driver strategy routine to
 *	write the pages out. Pages are linked by p_next. If flag is
 *	B_ASYNC, don't wait for io to complete.
 *	Flags are composed of {B_ASYNC, B_INVAL, B_DONTNEED}
 */
STATIC int
s5_putpageio(inode_t *ip, off_t off, size_t len, page_t *pp,
	daddr_t *io_list, int flags)
{
	vnode_t *devvp;
	s5_fs_t *s5fsp;
	size_t bsize;
	buf_t *bplist[PAGESIZE/NBPSCTR];
	buf_t **bpp;
	int i, blkpp, nio, blkon;
	int dblks = 0;
	off_t curoff;
	int bio_err, err = 0;

	ASSERT(len != 0);

	s5fsp = S5FS(ITOV(ip)->v_vfsp);
        devvp = s5fsp->fs_devvp;
	bsize = VBSIZE(ITOV(ip));

	if (bsize >= PAGESIZE)
		blkpp = 1;
	else
		blkpp = PAGESIZE/bsize;

	blkon = btodb(blkoff(s5fsp, off));

	/*
	 * Need to calculate nio before invoking
	 * the driver strategy routine because
	 * io could be async.
	 */
	pp->p_nio = 0;
	curoff = off & PAGEOFFSET;
	for (i = 0; i < blkpp && curoff < off + len;
						i++, curoff += bsize) {
		if (io_list[i] == DB_HOLE)
			continue;
		pp->p_nio++;
	}

	curoff = off & PAGEOFFSET;
	for (bpp = bplist, i = 0; i < blkpp && curoff < off + len;
						i++, curoff += bsize) {
		if (io_list[i] == DB_HOLE)
			continue;

		*bpp = pageio_setup(pp, curoff, MIN(len, bsize),
				    B_WRITE | flags);

		dblks += btodb((*bpp)->b_bcount);

		(*bpp)->b_edev = ip->i_dev;
		(*bpp)->b_blkno = LTOPBLK(io_list[i], bsize) + blkon;

		(*bdevsw[getmajor(devvp->v_rdev)].d_strategy)(*bpp);
		bpp++;
	}

	/* Update the number of dev sized blocks written by this LWP */
	ldladd(&u.u_iow, dblks);
#ifdef PERF
	mets_fsinfo[MET_S5].pgout++;
	mets_fsinfo[MET_S5].pgpgout += btopr(len);
	mets_fsinfo[MET_S5].sectout += dblks;
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
 * s5_putpage(vnode_t *vp, off_t off, uint_t len, int flags, cred_t *cr)
 *
 * Calling/Exit State:
 *	The inode rwlock may or may not be held locked on entry.
 *
 * Description:
 *	Flags are composed of {B_ASYNC, B_INVAL, B_FREE, B_DONTNEED, B_FORCE}
 *	If len == 0, do from off to EOF.
 *
 *	The normal cases should be len == 0 & off == 0 (entire vp list),
 *	len == MAXBSIZE (from segmap_release actions), and len == PAGESIZE
 *	(from pageout).
 *
 */
/* ARGSUSED */
STATIC int
s5_putpage(vnode_t *vp, off_t off, uint_t len, int flags, cred_t *cr)
{
	inode_t *ip;
	off_t kl_off;
	uint_t kl_len;
	int error;
	STATIC int s5_doputpage(vnode_t *, page_t *, int, cred_t *);

	ASSERT(!(vp->v_flag & VNOMAP));

#ifdef PERF
	mets_fsinfo[MET_S5].putpage++;
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

	if (len != 0) {
		s5_fs_t *s5fsp;

		s5fsp = S5FS(vp->v_vfsp);

		/* Do klustering to bsize boundaries */
		kl_off = (off & s5fsp->fs_bmask);
		kl_len = ((off + len + s5fsp->fs_bsize - 1) & s5fsp->fs_bmask) -
			  kl_off;
	} else {
		kl_off = off;
		kl_len = 0;
	}

	error = pvn_getdirty_range(s5_doputpage, vp, off, len, kl_off, kl_len,
				   ip->i_size, flags, cr);

	if (off == 0 && !error && (len == 0 || len >= ip->i_size)) {
		/*
		 * We have just sync'ed back all the pages
		 * on the inode; turn off the IMODTIME flag.
		 */
		(void) S5_ILOCK(ip);
		ip->i_flag &= ~IMODTIME;
		S5_IUNLOCK(ip, PLBASE);
	}

	return error;
}


/*
 * STATIC int
 * s5_doputpage(vnode_t *vp, page_t *dirty, int flags, cred_t *cr)
 *	workhorse for s5_putpage
 *
 * Calling/Exit State:
 *	The inode rwlock may or may not be held locked on entry.
 *
 *	A list of dirty pages, prepared for I/O (in pageout state),
 *	is passed in dirty.  Other parameters are passed through from
 *	s5_putpage.
 */
STATIC int
s5_doputpage(vnode_t *vp, page_t *dirty, int flags, cred_t *cr)
{
	struct inode *ip;
	struct page *pp, *pp2;
	page_t *pl_list;
	uint_t io_off, io_len, io_rlen;
	daddr_t lbn;
	daddr_t	local_dblist[PAGESIZE/NBPSCTR], *dblist;
	uint_t lbn_off;
	uint_t isize;
	int bsize;
	s5_fs_t *s5fsp;
	int fs_bshift;
	int blkpp;
	int err = 0;
	boolean_t io_short;

	ip = VTOI(vp);

	if (vp->v_vfsp == 0)
		/* Stale vnode. */
		return 0;

	bsize = VBSIZE(vp);

	s5fsp = S5FS(vp->v_vfsp);
	fs_bshift = s5fsp->fs_bshift;

	/*
	 * Now pp will have the list of kept dirty pages marked for
	 * write back.  All the pages on the pp list need to still
	 * be dealt with here.  Verify that we can really can do the
	 * write back to the filesystem and if not and we have some
	 * dirty pages, return an error condition.
	 */
	if ((vp->v_vfsp->vfs_flag & VFS_RDONLY) && dirty != NULL)
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

	if (bsize >= PAGESIZE)
		blkpp = 1;
	else
		blkpp = PAGESIZE/bsize;

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
                lbn = io_off >> fs_bshift;

		page_sub(&dirty, pp);
		pl_list = pp;
		io_len = PAGESIZE;
		lbn_off = lbn << fs_bshift;

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
			 * total the block size that are within file
			 * size limit.
			 */
			while (lbn_off + bsize < isize)
				bsize += VBSIZE(vp);
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

		/*
		 * If db info doesn't fit in pp, need to call bmappage to
		 * get the db info.
		 */
		if (blkpp > PG_DBSIZE) {
			dblist = local_dblist;
			err = s5_bmappage(vp, io_off, io_rlen, &pl_list, dblist,
						dblist, S_OTHER, cr);
			if (err)
				break;
		} else
			dblist = PG_DBLIST(pp);

		err = s5_putpageio(ip, io_off, io_rlen, pp, dblist, flags);

		if (err)
			break;
	}

	if ((err || io_short) && dirty != NULL)
		pvn_fail(dirty, B_WRITE | flags);
	return (err);
}

/*
 * int
 * s5_putpagelist(vnode_t *vp, off_t off, page_t *pp, void *bmapp,
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
s5_putpagelist(vnode_t *vp, off_t off, page_t *pp, void *bmapp,
	int flags, cred_t *cr)
{
	inode_t *ip = VTOI(vp);
	s5_fs_t *s5fsp;
	daddr_t *io_list;
	page_t *plist, *io_pl;
	int bsize;
	uchar_t blkpp;
	uint_t io_len;
	off_t io_off;
	int error;

	ASSERT(vp->v_type == VREG);
	ASSERT((off & PAGEOFFSET) == 0);

	s5fsp = S5FS(ITOV(ip)->v_vfsp);
	bsize = VBSIZE(ITOV(ip));

	if (bsize >= PAGESIZE)
		blkpp = 1;
	else
		blkpp = PAGESIZE / bsize;

	plist = pp;
	do {
		pp = plist;

		ASSERT(PAGE_IS_LOCKED(pp));

		io_list = ((daddr_t *)bmapp + (off >> s5fsp->fs_bshift)); 
		io_off = off;
		io_len = 0;
		io_pl = NULL;
		do {
			io_len += PAGESIZE;
			pp->p_nio = blkpp;
			page_sub(&plist, pp);
			page_sortadd(&io_pl, pp);
			pp = plist;
		} while (((io_off += PAGESIZE) < off + bsize) && pp != NULL);

		ASSERT(io_pl != NULL);

		error = s5_putpageio(ip, off, io_len, io_pl, io_list, flags);

		if (error)
			break;

		off = io_off;

	} while (plist != NULL);

	return error;
}

/*
 * int
 * s5_stablestore(vnode_t **vpp, off_t off, size_t len, void **bmapp,
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
STATIC int
s5_stablestore(vnode_t **vpp, off_t *off, size_t *len, void **bmapp,
		  cred_t *cr)
{
	inode_t *ip = VTOI(*vpp);
	s5_fs_t *s5fsp;
	daddr_t	dummy_iolist[PAGESIZE/NBPSCTR];
	daddr_t *daddrp;
	uint_t nblks, bsize, blkpp;
	off_t curoff, roff, eoff;
	size_t rlen, bmap_size;
	int err;
	pl_t s;


	ASSERT((*vpp)->v_type == VREG);
	ASSERT(*len != 0);

	S5_IRWLOCK_WRLOCK(ip);

	if ((*off + *len) > ip->i_size) {
		S5_IRWLOCK_UNLOCK(ip);
		return EINVAL;
	}

	s5fsp = S5FS((*vpp)->v_vfsp);
        bsize = VBSIZE(*vpp);

	/* Allocate space for disk block addresses. */
        roff = blkroundup(s5fsp, *off);
	rlen = *len - (roff - *off);
	nblks = pageroundup(rlen) >> s5fsp->fs_bshift;
        /* 0 blocks after rounding? */
        if (nblks == 0) {
                S5_IRWLOCK_UNLOCK(ip);
                return EINVAL;
        }

	daddrp = *bmapp =
		(daddr_t *)kmem_zalloc(nblks * sizeof(daddr_t), KM_SLEEP);

        eoff = roff + rlen;
        if (bsize >= PAGESIZE)
                blkpp = 1;
        else
                blkpp = PAGESIZE/bsize;
        for (curoff = roff; curoff < eoff; curoff += bmap_size,
                                                daddrp += blkpp) {
		bmap_size = MAX((PAGESIZE - (curoff&PAGEOFFSET)),
				(bsize - (curoff&(s5fsp->fs_bsize-1))));
		if (curoff + bmap_size > eoff)
                        bmap_size = eoff - curoff;
		err = s5_bmappage(*vpp, curoff, bmap_size, NULL, daddrp,
					dummy_iolist, S_WRITE, cr);
                if (err)
                        break;
        }

	if (!err) {
		s = S5_ILOCK(ip);
		ip->i_swapcnt++;
		S5_IUNLOCK(ip, s);
		*off = roff;
		*len = rlen;
		*bmapp = (daddr_t *)*bmapp + ((roff & PAGEOFFSET) >> s5fsp->fs_bshift);
	} else {
		kmem_free(daddrp, nblks * sizeof(daddr_t));
		*bmapp = NULL;
	}

	S5_IRWLOCK_UNLOCK(ip);
	return err;
}

/*
 * int
 * s5_relstore(vnode_t *vp, off_t off, size_t len, void *bmapp,
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
STATIC int
s5_relstore(vnode_t *vp, off_t off, size_t len, void *bmapp,
		  cred_t *cr)
{
	inode_t *ip = VTOI(vp);
	s5_fs_t *s5fsp;
	pl_t s;

	ASSERT(vp->v_type == VREG);
	ASSERT(len != 0);

	if ((off + len) > ip->i_size)
		return EINVAL;

	s5fsp = S5FS((vp)->v_vfsp);

	ASSERT(blkoff(s5fsp, off) == 0);

	len -= (off - blkroundup(s5fsp, off));
	kmem_free(((daddr_t *)bmapp - ((off & PAGEOFFSET) >> s5fsp->fs_bshift)),
		    (pageroundup(len) >> s5fsp->fs_bshift) * sizeof(daddr_t));
	s = S5_ILOCK(ip);
	ip->i_swapcnt--;
	S5_IUNLOCK(ip, s);

	return 0;
}

/*
 * int
 * s5_map(vnode_t *vp, off_t off, struct as *as, vaddr_t *addrp, uint_t len,
 *	  uint_t prot, uint_t maxprot, uint_t flags, cred_t *fcr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 */
/* ARGSUSED */
STATIC int
s5_map(vnode_t *vp, off_t off, struct as *as, vaddr_t *addrp, uint_t len,
	uint_t prot, uint_t maxprot, uint_t flags, cred_t *fcr)
{
	cred_t *cr = VCURRENTCRED(fcr);
	struct segvn_crargs vn_a;
	int error = 0;
	inode_t *ip = VTOI(vp);

	if (vp->v_flag & VNOMAP)
		return ENOSYS;

	if (vp->v_type != VREG)
		return ENODEV;

	if ((int)off < 0 || (int)(off + len) < 0)
		return EINVAL;

	/*
	 * If file has active mandatory lock, disallow mmap.
	 */
	if (vp->v_filocks != NULL && MANDLOCK(vp, ip->i_mode))
		return EAGAIN;

	S5_IRWLOCK_WRLOCK(ip);

	as_wrlock(as);

	if ((flags & MAP_FIXED) == 0) {
		map_addr(addrp, len, (off_t)off, 0);
		if (*addrp == NULL) {
			as_unlock(as);
			S5_IRWLOCK_UNLOCK(ip);
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
	S5_IRWLOCK_UNLOCK(ip);
	return error;
}

/*
 * int
 * s5_addmap(vnode_t *vp, uint_t off, struct as *as, vaddr_t addr, uint_t len,
 *	     uint_t prot, uint_t maxprot, uint_t flags, cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 */
/* ARGSUSED */
STATIC int
s5_addmap(vnode_t *vp, uint_t off, struct as *as, vaddr_t addr, uint_t len,
	  uint_t prot, uint_t maxprot, uint_t flags, cred_t *cr)
{
	inode_t *ip;
	pl_t s;
	
	ASSERT(!(vp->v_flag & VNOMAP));

	ip = VTOI(vp);
	s = S5_ILOCK(ip);
	ip->i_mapcnt += btopr(len);
	S5_IUNLOCK(ip, s);

	return 0;
}

/*
 * int
 * s5_delmap(vnode_t *vp, uint_t off, struct as *as, vaddr_t addr, uint_t len,
 *	     uint_t prot, uint_t maxprot, uint_t flags, cred_t *cr)
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
/* ARGSUSED */
STATIC int
s5_delmap(vnode_t *vp, uint_t off, struct as *as, vaddr_t addr, uint_t len,
	  uint_t prot, uint_t maxprot, uint_t flags, cred_t *cr)
{
	inode_t *ip;
	pl_t s;

	if (vp->v_flag & VNOMAP)
		return (ENOSYS);

	ip = VTOI(vp);
	if (((flags & MAP_TYPE) == MAP_SHARED) && (maxprot & PROT_WRITE)) {
		/*
		 * For shared writable mappings, make sure that the timestamps
		 * of the file are updated if any changes were made to this
		 * file through the mapping.  Call pvn_syncsdirty to see if the
		 * file has been modified, and, if so, call IMARK to update
		 * the timestamps.
		 */
		pvn_syncsdirty(vp);
		s = S5_ILOCK(ip);
		if (vp->v_flag & VMOD)
			IMARK(ip, ip->i_flag);
	} else {
		/*
		 * Read-only mapping, so no file modifications
		 * occurred through this mapping; no need to
		 * call pvn_syncsdirty here.  Just lock the inode.
		 */
		s = S5_ILOCK(ip);
	}
	ip->i_mapcnt -= btopr(len); 	/* Count released mappings */
	ASSERT(ip->i_mapcnt >= 0);
	S5_IUNLOCK(ip, s);
	return 0;
}

