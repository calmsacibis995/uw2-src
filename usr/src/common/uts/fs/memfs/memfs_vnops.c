/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/memfs/memfs_vnops.c	1.52"
#ident	"$Header: $"

#include <acc/dac/acl.h>
#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/dirent.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/flock.h>
#include <fs/fs_subr.h>
#include <fs/memfs/memfs.h>
#include <fs/memfs/memfs_hier.h>
#include <fs/memfs/memfs_mnode.h>
#include <fs/pathname.h>
#include <fs/specfs/snode.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <mem/seg_map.h>
#include <mem/seg_vn.h>
#include <mem/swap.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/resource.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/param.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * Internal static functions
 */
void memfs_delabort_cleanup(void);
STATIC int memfs_trunc_up(mnode_t *, size_t, boolean_t);
STATIC void memfs_trunc_down(mnode_t *, size_t);
STATIC void memfs_relocate(mnode_t *, uchar_t);
STATIC off_t memfs_relocate_page(mnode_t *, off_t, uchar_t, swaploc_t *);
STATIC void memfs_delete_bs(mnode_t *mp);
STATIC void segmemfs_badop(void);
STATIC int segmemfs_kluster(struct seg *, vaddr_t, int);
swaploc_t *memfs_lookup_bs(mnode_t *, off_t);

extern struct mnode *mnode_alloc(mem_vfs_t *, struct vattr *, struct cred *);
extern int specpreval(vtype_t, dev_t, cred_t *);
extern int memfs_taccess(mnode_t *, int, struct cred *);
extern int memfs_dirlook(mnode_t *, char *, mnode_t **, struct cred *);
extern int memfs_direnter(struct mem_vfs *, mnode_t *, char *, mnode_t *,
		struct cred *);
extern int memfs_dirremove(mem_vfs_t *, mnode_t *, mnode_t *, char *,
		struct cred *);
extern int memfs_dirinit(mem_vfs_t *, mnode_t *, mnode_t *, struct cred *);

extern void map_addr(vaddr_t *, uint_t, off_t, int);


STATIC int memfs_open(vnode_t **, int, cred_t *);
STATIC int memfs_close(vnode_t *, int, boolean_t , off_t, cred_t *);
STATIC int memfs_read(vnode_t *, struct uio *, int, cred_t *);
STATIC int memfs_write(vnode_t *, struct uio *, int, cred_t *);
STATIC int memfs_ioctl(vnode_t *, int, int, int, cred_t *, int *);
STATIC int memfs_setfl(vnode_t *, uint_t, uint_t, cred_t *);
STATIC int memfs_getattr(vnode_t *, vattr_t *, int, cred_t *);
STATIC int memfs_setattr(vnode_t *, vattr_t *, int, int, cred_t *);
STATIC int memfs_access(vnode_t *, int, int, cred_t *);
STATIC int memfs_lookup(vnode_t *, char *, vnode_t **, pathname_t *, int,
			vnode_t *, cred_t *);
STATIC int memfs_create(vnode_t *, char *, vattr_t *, enum vcexcl,
			 int, vnode_t **, cred_t *);
STATIC int memfs_remove(vnode_t *, char *, cred_t *);
STATIC int memfs_link(vnode_t *, vnode_t *, char *, cred_t *);
STATIC int memfs_rename(vnode_t *, char *, vnode_t *, char *, cred_t *);
STATIC int memfs_mkdir(vnode_t *, char *, vattr_t *, vnode_t **, cred_t *);
STATIC int memfs_rmdir(vnode_t *, char *, vnode_t *, cred_t *);
STATIC int memfs_readdir(vnode_t *, struct uio *, cred_t *, int *);
STATIC int memfs_symlink(vnode_t *, char *, vattr_t *, char *, cred_t *);
STATIC int memfs_readlink(vnode_t *, struct uio *, cred_t *);
STATIC int memfs_fsync(vnode_t *, cred_t *);
STATIC void memfs_inactive(vnode_t *, cred_t *);
STATIC void memfs_release(vnode_t *);
STATIC void memfs_inactive_common(mnode_t *);
STATIC int memfs_fid(vnode_t *, struct fid **);
STATIC int memfs_rwlock(vnode_t *, off_t, int, int, int);
STATIC void memfs_rwunlock(vnode_t *, off_t, int);
STATIC int memfs_seek(vnode_t *, off_t, off_t *);
STATIC int memfs_frlock(vnode_t *, int, struct flock *, int, off_t, cred_t *);
STATIC int memfs_getpage(vnode_t *, uint_t, uint_t, uint_t *, page_t **,
		uint_t, struct seg *, vaddr_t, enum seg_rw, cred_t *);
STATIC int memfs_getapage(vnode_t *, uint_t, uint_t, uint_t *, page_t *[],
		uint_t, struct seg *, vaddr_t, enum seg_rw, cred_t *);
STATIC int memfs_putpage(vnode_t *, off_t, uint_t, int, cred_t *);
STATIC int memfs_doputpage(vnode_t *, page_t *, int, cred_t *);
STATIC int memfs_map(vnode_t *, off_t, struct as *, vaddr_t *, uint_t,
		uint_t, uint_t, uint_t, cred_t *);
STATIC int memfs_addmap(vnode_t *, uint_t, struct as *, vaddr_t, uint_t,
		uint_t, uint_t, uint_t, cred_t *);
STATIC int memfs_delmap(vnode_t *, uint_t, struct as *, vaddr_t, uint_t,
		uint_t, uint_t, uint_t, cred_t *);


vnodeops_t memfs_vnodeops = {
	memfs_open,
	memfs_close,
	memfs_read,
	memfs_write,
	memfs_ioctl,
	fs_setfl,
	memfs_getattr,
	memfs_setattr,
	memfs_access,
	memfs_lookup,
	memfs_create,
	memfs_remove,
	memfs_link,
	memfs_rename,
	memfs_mkdir,
	memfs_rmdir,
	memfs_readdir,
	memfs_symlink,
	memfs_readlink,
	memfs_fsync,
	memfs_inactive,
	memfs_release,
	memfs_fid,
	memfs_rwlock,
	memfs_rwunlock,
	memfs_seek,
	fs_cmp,
	memfs_frlock,
	(int (*)())fs_nosys,	/* realvp */
	memfs_getpage,
	memfs_putpage,
	memfs_map,
	memfs_addmap,
	memfs_delmap,
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
	(int (*)())fs_nosys,	/* filler[4]... */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys
};

/*
 * seg_memfs is a dummy segment driver. It's only purpose is
 * to allow memfs_relocate_page() to call memfs_getpage() without
 * a genuine segment.
 */
struct seg_ops segmemfs_ops = {
	(int(*)())segmemfs_badop,		/* unmap */
	(void(*)())segmemfs_badop,		/* free */
	(faultcode_t(*)())segmemfs_badop,	/* fault */
	(int(*)())segmemfs_badop,		/* setprot */
	(int(*)())segmemfs_badop,		/* checkprot */
	segmemfs_kluster,			/* kluster */
	(int(*)())segmemfs_badop,		/* sync */
	(int(*)())segmemfs_badop,		/* incore */
	(int(*)())segmemfs_badop,		/* lockop */
	(int(*)())segmemfs_badop,		/* dup */
	(void(*)())segmemfs_badop,		/* childload */
	(int(*)())segmemfs_badop,		/* getprot */
	(off_t(*)())segmemfs_badop,		/* getoffset */
	(int(*)())segmemfs_badop,		/* gettype */
	(int(*)())segmemfs_badop,		/* getvp */
	(void(*)())segmemfs_badop,		/* age */
};

/*
 * STATIC int
 * memfs_open(vnode_t **vpp, int flag, cred_t *cred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	No-op for memfs.
 */
/*ARGSUSED*/
STATIC int
memfs_open(vnode_t **vpp, int flag, cred_t *cred)
{
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	return (0);
}

/*
 * STATIC int
 * memfs_close(vnode_t *vp, int flag, boolean_t lastclose, off_t offset,
 *	cred_t *cred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
/*ARGSUSED*/
STATIC int
memfs_close(vnode_t *vp, int flag, boolean_t lastclose,
    off_t offset, cred_t *cred)
{
	mnode_t *mp = VP_TO_MNODE(vp);

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (!(mp->mno_flags & MEMFS_UNNAMED)) {
		memfs_timestamp(mp);
		cleanlocks(vp, u.u_procp->p_epid, u.u_procp->p_sysid);
	}
	return (0);
}

/*
 * STATIC int
 * memfs_read(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *cred)
 *
 * Calling/Exit State:
 *	inode rwlock is held in shared mode on entry and at exit.
 *
 * Description:
 */
/*ARGSUSED*/
STATIC int
memfs_read(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *cred)
{
	mnode_t *mp = VP_TO_MNODE(vp);
	caddr_t base;
	int oresid = uiop->uio_resid;
	int error = 0, diff;
	uint_t on, n;

	if (uiop->uio_resid == 0)
                return 0;

	if (uiop->uio_offset < 0)
		return EINVAL;

	if (vp->v_type == VDIR)
		return EISDIR;

	ASSERT(vp->v_type == VREG);
	ASSERT(!(mp->mno_flags & MEMFS_UNNAMED));

	do {
		diff = mp->mno_size - uiop->uio_offset;

		if (diff <= 0) 
			break;

		on = uiop->uio_offset & MAXBOFFSET;
		n = MIN(MAXBSIZE - on, uiop->uio_resid);

		if (diff < n)
			n = diff;

		base = segmap_getmap(segkmap, vp, uiop->uio_offset, n,
				     S_READ, B_FALSE, NULL);

		if ((error = uiomove(base + on, n, UIO_READ, uiop)) == 0) {
			int flags;
			/*
			 * If we read a whole block, we won't need this
			 * buffer again soon.
			 */
			if (n + on == MAXBSIZE)
				flags = SM_DONTNEED;
			else
				flags = 0;
			error = segmap_release(segkmap, base, flags);
		} else {
			(void) segmap_release(segkmap, base, 0);
		}
	} while (error == 0 && uiop->uio_resid > 0);

	if (uiop->uio_resid != oresid)
		error = 0;

	MNODE_ACCESSED(mp);

	return (error);
}

/*
 * STATIC int
 * memfs_write(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *cred)
 *
 * Calling/Exit State:
 *	The inode rwlock is held in excl mode on entry and at exit.
 *
 * Description:
 */
/*ARGSUSED*/
STATIC int
memfs_write(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *cred)
{
	mnode_t *mp = VP_TO_MNODE(vp);
	rlim_t limit = uiop->uio_limit;
	addr_t base;
	int error;
	uint_t on, n;
	off_t off;
	long oresid = uiop->uio_resid;
	int flags;

	ASSERT(!(mp->mno_flags & MEMFS_UNNAMED));

	/*
	 * We don't currently support writing to non-regular files
	 */
	if (vp->v_type != VREG)
		return (EINVAL);

	if (ioflag & IO_APPEND) {
		/*
		 * In append mode start at end of file.
		 */
		uiop->uio_offset = mp->mno_size;
	}

	off = uiop->uio_offset;

        if (off < 0)
                return (EINVAL);

	if (uiop->uio_resid == 0)
		return (0);

	if (off > mp->mno_size) {
                error = memfs_trunc_up(mp, off, B_TRUE);
		if (error)
			goto out;
	}

	do {
		on = off & MAXBOFFSET;
		n = MIN(MAXBSIZE - on, uiop->uio_resid);

		/*
                 * If we are exceeding the ulimit or if we are
                 * overflowing, clip the io size.
                 */
                if (off + n >= limit || off + n <= 0) {
                        if (off >= limit) {
                                error = EFBIG;
                                goto out;
                        }
                        n = limit - off;
                }

		base = segmap_getmap(segkmap, vp, off, n, S_WRITE,
						B_FALSE, NULL);

		error = uiomove(base + on, n, UIO_WRITE, uiop);

		if (error == 0) {

			/*
			 * Force write back for synchronous write cases.
			 */
			if (n + on == MAXBSIZE) {
				/*
				 * Have written a whole block.
				 * Mark the buffer to indicate that
				 * it won't be needed again soon.
				 */
				flags = SM_DONTNEED;
			} else
				flags = 0;
			if (off + n > mp->mno_size)
				mp->mno_size = off + n;
			error = segmap_release(segkmap, base, flags);
		} else {
			off_t noff;
			ASSERT(uiop->uio_offset < off + n);

			/*
			 * If we had some sort of error during uiomove,
			 * call segmap_abort_create to have the pages
			 * aborted if we created them.
			 */
			noff = segmap_abort_create(segkmap,
					base, uiop->uio_offset,
					(off + n - uiop->uio_offset));

			if (noff != -1 && noff < uiop->uio_offset) {
				/*
				 * Some pages aborted, need to fix resid.
				 */
				uiop->uio_resid += uiop->uio_offset - noff;
			}

			(void) segmap_release(segkmap, base, 0);
		}
		off = uiop->uio_offset;

	} while (error == 0 && uiop->uio_resid > 0);
out:
	if (oresid != uiop->uio_resid)
		error = 0;

	FSPIN_LOCK(&mp->mno_mutex);
	mp->mno_flag |= (TUPD|TCHG);
	FSPIN_UNLOCK(&mp->mno_mutex);
	return (error);
}

/*
 * STATIC int
 * memfs_ioctl(vnode_t *vp, int cmd, int arg, int flag, cred_t *cr, int *rvalp)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
/*ARGSUSED*/
STATIC int
memfs_ioctl(vnode_t *vp, int cmd, int arg, int flag, cred_t *cr, int *rvalp)
{
	ASSERT(!(VP_TO_MNODE(vp)->mno_flags & MEMFS_UNNAMED));

	return (ENOTTY);
}

/*
 * STATIC int
 * memfs_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Get a mnode attributes.
 */
/*ARGSUSED*/
STATIC int
memfs_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cred)
{
	mnode_t *mp = VP_TO_MNODE(vp);

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(!(mp->mno_flags & MEMFS_UNNAMED));

	RWSLEEP_RDLOCK(&mp->mno_rwlock, PRINOD);
	memfs_timestamp(mp);
	vap->va_type = vp->v_type;
	vap->va_mode = mp->mno_mode & MODEMASK;
	vap->va_uid = mp->mno_uid;
	vap->va_gid = mp->mno_gid;
	vap->va_fsid = mp->mno_fsid;
	vap->va_nodeid = mp->mno_nodeid;
	vap->va_size = mp->mno_size;
	vap->va_nlink = mp->mno_nlink;
	vap->va_atime.tv_sec = mp->mno_atime.tv_sec;
	vap->va_atime.tv_nsec = mp->mno_atime.tv_usec * 1000;
	vap->va_mtime.tv_sec = mp->mno_mtime.tv_sec;
	vap->va_mtime.tv_nsec = mp->mno_mtime.tv_usec * 1000;
	vap->va_ctime.tv_sec = mp->mno_ctime.tv_sec;
	vap->va_ctime.tv_nsec = mp->mno_ctime.tv_usec * 1000;
	vap->va_blksize = PAGESIZE;
	vap->va_rdev = mp->mno_rdev;
	vap->va_vcode = mp->mno_vcode;
	vap->va_nblocks = btodb(ptob(btopr(vap->va_size)));

	/*
	 * Since we don't support ACLs, we only have the base entries
	 * to report.
	 */
	if (vap->va_mask & AT_ACLCNT)
		vap->va_aclcnt = NACLBASE;

	RWSLEEP_UNLOCK(&mp->mno_rwlock);
	return (0);
}

/*
 * STATIC int
 * memfs_setattr(vnode_t *vp, vattr_t *vap, int flags, int ioflags, cred_t *cr)
 *      Modify/Set an mnode's attributes.
 *
 * Calling/Exit State:
 *      The caller holds no mnode locks and no spin LOCKs on entry to
 *	this function. None are held upon return.
 */
STATIC int
memfs_setattr(vnode_t *vp, vattr_t *vap, int flags, int ioflags,
	cred_t *cr)
{
	mnode_t *mp = VP_TO_MNODE(vp);
	int error = 0;
	long mask = vap->va_mask;
	timestruc_t timenow;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Cannot set these attributes
	 */
	if (mask & AT_NOSET)
		return (EINVAL);

        RWSLEEP_WRLOCK(&mp->mno_rwlock, PRINOD);

        /*
         * Truncate file.  Must have write permission and not be a directory.
         */
        if (mask & AT_SIZE) {
		ASSERT(!(mp->mno_flags & MEMFS_FIXEDSIZE));
                if (vap->va_size > LONG_MAX || vap->va_size >
			    u.u_rlimits->rl_limits[RLIMIT_FSIZE].rlim_cur) {
                        error = EFBIG;
                } else if (vap->va_size > mp->mno_size) {
			error = memfs_trunc_up(mp, vap->va_size, B_TRUE);
		} else if (vap->va_size < mp->mno_size) {
			memfs_trunc_down(mp, vap->va_size);
			error = 0;
		}
		if (error == 0) {
			FSPIN_LOCK(&mp->mno_mutex);
			mp->mno_flag |= TUPD|TCHG;
			FSPIN_UNLOCK(&mp->mno_mutex);
			memfs_timestamp(mp);
		}
	}

	/*
	 * Change file access modes. Must be owner or super-user.
	 */
	if (mask & AT_MODE) {
		if (cr->cr_uid != mp->mno_uid && pm_denied(cr, P_OWNER)) {
                        error = EPERM;
                        goto out;
                }
		mp->mno_mode &= S_IFMT;
		mp->mno_mode |= vap->va_mode & ~S_IFMT;
		/*
                 * A non-privileged user can set the sticky bit
                 * on a directory.
                 */
                if (vp->v_type != VDIR) {
                        if ((mp->mno_mode & ISVTX) && pm_denied(cr, P_OWNER)) {
                                mp->mno_mode &= ~ISVTX;
                        }
                }

                if (!groupmember((uid_t)mp->mno_gid, cr) &&
                    pm_denied(cr, P_OWNER)) {
                        mp->mno_mode &= ~ISGID;
                }
		GET_HRESTIME(&timenow);
		FSPIN_LOCK(&mp->mno_mutex);
		mp->mno_flag |= TCHG;
		FSPIN_UNLOCK(&mp->mno_mutex);
		memfs_timestamp(mp);
	}
	if (mask & (AT_UID | AT_GID)) {
		int checksu = 0;
		if (cr->cr_uid != mp->mno_uid)
			checksu = 1;
		else {
			if (rstchown) {
				if (((mask & AT_UID) &&
				    vap->va_uid != mp->mno_uid) ||
				    ((mask & AT_GID) &&
				    !groupmember(vap->va_gid, cr)))
					checksu = 1;
			}
		}
		if (checksu && pm_denied(cr, P_OWNER)) {
                        error = EPERM;
                        goto out;
                }
		if (pm_denied(cr, P_OWNER)) {
                       if ((mp->mno_mode & (VSGID|(VEXEC>>3))) ==
                           (VSGID|(VEXEC>>3)))
                               mp->mno_mode &= ~(ISGID);
                       mp->mno_mode &= ~(ISUID);
                }
		if (mask & AT_UID)
			mp->mno_uid = vap->va_uid;
		if (mask & AT_GID)
			mp->mno_gid = vap->va_gid;
		FSPIN_LOCK(&mp->mno_mutex);
		mp->mno_flag |= TCHG;
		FSPIN_UNLOCK(&mp->mno_mutex);
		memfs_timestamp(mp);
	}
	if (mask & (AT_ATIME | AT_MTIME)) {
		boolean_t	mtime = B_TRUE;
		boolean_t	atime = B_TRUE;

		if (cr->cr_uid != mp->mno_uid && pm_denied(cr, P_OWNER)) {
			if (flags & ATTR_UTIME) {
				error = EPERM;
			} else {
				error = memfs_taccess(mp, IWRITE, cr);
			}
			if (error) {
				goto out;
			}
		}
		GET_HRESTIME(&timenow);
                FSPIN_LOCK(&mp->mno_mutex);
		if (mask & AT_MTIME) {
			if (flags & (ATTR_UTIME | ATTR_UPDTIME)) {
				if ((flags & ATTR_UPDTIME) &&
					(vap->va_mtime.tv_sec <=
						mp->mno_mtime.tv_sec)) {
					mtime = B_FALSE;
				} else {
					mp->mno_mtime.tv_sec =
						vap->va_mtime.tv_sec;
					mp->mno_mtime.tv_usec =
						vap->va_mtime.tv_nsec/1000;
				}
			} else {
				/*
				 * Make sure that the mod time is unique.
				 */
				if (mp->mno_mtime.tv_sec == timenow.tv_sec &&
				    mp->mno_mtime.tv_usec >=timenow.tv_nsec/1000){
					timenow.tv_nsec =
						(mp->mno_mtime.tv_usec+1) * 1000;
					if (timenow.tv_nsec >= NANOSEC) {
						timenow.tv_sec++;
						timenow.tv_nsec = 0;
					}
				}
				mp->mno_mtime.tv_sec = timenow.tv_sec;
				mp->mno_mtime.tv_usec = timenow.tv_nsec/1000;
			}
			if (mtime == B_TRUE) {
				mp->mno_ctime.tv_sec = timenow.tv_sec;
				mp->mno_ctime.tv_usec = timenow.tv_nsec/1000;
				mp->mno_flag &= ~(TUPD | TCHG);
			}
		}

		if (mask & AT_ATIME) {
			if (flags & (ATTR_UTIME | ATTR_UPDTIME)) {
				if ((flags & ATTR_UPDTIME) &&
					(vap->va_mtime.tv_sec <=
						mp->mno_mtime.tv_sec)) {
					atime = B_FALSE;
				} else {
					mp->mno_atime.tv_sec =
						vap->va_atime.tv_sec;
					mp->mno_atime.tv_usec =
						vap->va_atime.tv_nsec/1000;
				}
			} else {
				mp->mno_atime.tv_sec = timenow.tv_sec;
				mp->mno_atime.tv_usec = timenow.tv_nsec/1000;
			}
			if (atime == B_TRUE)
				mp->mno_flag &= ~TACC;
		}
                FSPIN_UNLOCK(&mp->mno_mutex);
	}

out:
	RWSLEEP_UNLOCK(&mp->mno_rwlock);
	return (error);
}

/*
 * STATIC int
 * memfs_access(vnode_t *vp, int mode, int flags, cred_t *cred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
/*ARGSUSED*/
STATIC int
memfs_access(vnode_t *vp, int mode, int flags, cred_t *cred)
{
	mnode_t *mp = VP_TO_MNODE(vp);
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(!(mp->mno_flags & MEMFS_UNNAMED));

	RWSLEEP_RDLOCK(&mp->mno_rwlock, PRINOD);
	error = memfs_taccess(mp, mode, cred);
	RWSLEEP_UNLOCK(&mp->mno_rwlock);
	return (error);
}

/*
 * STATIC int
 * memfs_lookup(vnode_t *dvp, char *nm, vnode_t **vpp, pathname_t *pnp,
 *		int lookup_flags, vnode_t *rootvp, cred_t *cred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
/* ARGSUSED */
STATIC int
memfs_lookup(vnode_t *dvp, char *nm, vnode_t **vpp, pathname_t *pnp,
	     int lookup_flags, vnode_t *rootvp, cred_t *cred)
{
	mnode_t *mp = VP_TO_MNODE(dvp);
	mnode_t *nmp = NULL;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(!(mp->mno_flags & MEMFS_UNNAMED));

	if (*nm == '\0') {
		VN_HOLD(dvp);
		*vpp = dvp;
		return (0);
	}

	ASSERT(mp);
	RWSLEEP_RDLOCK(&mp->mno_rwlock, PRINOD);
	error = memfs_dirlook(mp, nm, &nmp, cred);
	RWSLEEP_UNLOCK(&mp->mno_rwlock);
	if (error == 0) {
		ASSERT(nmp);
		*vpp = MNODE_TO_VP(nmp);
		/*
		 * If vnode is a device return special vnode instead
		 */
		if (ISVDEV((*vpp)->v_type)) {
			vnode_t *newvp;

			newvp = specvp(*vpp, (*vpp)->v_rdev, (*vpp)->v_type,
									cred);
			VN_RELE(*vpp);
			*vpp = newvp;
		}
	}
	return (error);
}

/*
 * STATIC int
 * memfs_create(vnode_t *dvp, char *nm, vattr_t *vap, enum vcexcl exclusive,
 *	int mode, vnode_t **vpp, cred_t *cred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
/*ARGSUSED*/
STATIC int
memfs_create(vnode_t *dvp, char *nm, vattr_t *vap, enum vcexcl exclusive,
	int mode, vnode_t **vpp, cred_t *cred)
{
	mnode_t *parent = VP_TO_MNODE(dvp);
	mem_vfs_t *mem_vfsp = VTOTM(dvp);
	mnode_t *self;
	int error;
	mnode_t *oldmp = NULL;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(!(parent->mno_flags & MEMFS_UNNAMED));

	switch (vap->va_type) {
	case VBLK:
	case VCHR:
		/*
		 * Only super-user can create non-FIFO special devices.
		 */
		if (error = specpreval(vap->va_type, vap->va_rdev, cred))                                return error;
		else
			break;

	case VREG:
		if (pm_denied(cred, P_OWNER))
			vap->va_mode &= ~VSVTX;
		break;

	case VDIR:
		/*
		 * memfs_mkdir is used to create directories.
		 */
		return (EISDIR);
	}

	RWSLEEP_WRLOCK(&parent->mno_rwlock, PRINOD);
	error = memfs_dirlook(parent, nm, &oldmp, cred);

	if (error == 0) {	/* name found */
		ASSERT(oldmp);
		RWSLEEP_UNLOCK(&parent->mno_rwlock);
		if (exclusive == EXCL)
			error = EEXIST;
		else if ((oldmp->mno_mode & S_IFMT) == S_IFDIR)
			error = EISDIR;
		else {
			error = memfs_taccess(oldmp, mode, cred);
		}
		if (error) {
			MNODE_RELE(oldmp);
			return (error);
		}
		*vpp = MNODE_TO_VP(oldmp);
		if ((*vpp)->v_type == VREG && (vap->va_mask & AT_SIZE) &&
		    vap->va_size == 0) {
			if (oldmp->mno_size > 0) {
				memfs_trunc_down(oldmp, (u_long)0);
			}
		}
		if (ISVDEV((*vpp)->v_type)) {
			vnode_t *newvp;

			newvp = specvp(*vpp, (*vpp)->v_rdev, (*vpp)->v_type, cred);
			VN_RELE(*vpp);
			if (newvp == NULL)
				return (ENOSYS);
			*vpp = newvp;
		}
		return (0);
	}
	if (error != ENOENT) {
		RWSLEEP_UNLOCK(&parent->mno_rwlock);
		return (error);
	}
	self = mnode_alloc(mem_vfsp, vap, cred);
	if (self == NULL) {
		RWSLEEP_UNLOCK(&parent->mno_rwlock);
		return (ENOSPC);
	}
	error = memfs_direnter(mem_vfsp, parent, nm, self, cred);
	RWSLEEP_UNLOCK(&self->mno_rwlock);
	MNODE_MODIFIED(parent);
	RWSLEEP_UNLOCK(&parent->mno_rwlock);

	if (error) {
		MNODE_RELE(self);
		return (error);
	}
	*vpp = MNODE_TO_VP(self);
	if (!error && ISVDEV((*vpp)->v_type)) {
		vnode_t *newvp;

		newvp = specvp(*vpp, (*vpp)->v_rdev, (*vpp)->v_type, cred);
		VN_RELE(*vpp);
		if (newvp == NULL)
			return (ENOSYS);
		*vpp = newvp;
	}
	return (0);
}

/*
 * STATIC int
 * memfs_remove(vnode_t *dvp, char *nm, cred_t *cred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Remove entry "nm" from directory "dvp".
 */
STATIC int
memfs_remove(vnode_t *dvp, char *nm, cred_t *cred)
{
	mnode_t *parent = VP_TO_MNODE(dvp);
	mem_vfs_t *mem_vfsp = VTOTM(dvp);
	int error;
	mnode_t *mp = NULL;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(!(parent->mno_flags & MEMFS_UNNAMED));

	/*
	 * get the write lock now since we know we'll probably be
	 * updating the parent to remove the file
	 */
	RWSLEEP_WRLOCK(&parent->mno_rwlock, PRINOD);
	error = memfs_dirlook(parent, nm, &mp, cred);
	if (error) {
		RWSLEEP_UNLOCK(&parent->mno_rwlock);
		return (error);
	}
	ASSERT(mp);
	if (((mp->mno_mode & S_IFMT) == S_IFDIR) &&
		pm_denied(CRED(), P_FILESYS)) {
		error = EPERM;
		goto done;
	}
	error = memfs_dirremove(mem_vfsp, parent, mp, nm, cred);
	if (!error)
		MNODE_MODIFIED(parent);
done:
	RWSLEEP_UNLOCK(&parent->mno_rwlock);
	MNODE_RELE(mp);
	return (error);
}

/*
 * STATIC int
 * memfs_link(vnode_t *dvp, vnode_t *srcvp, char *tnm, cred_t *cred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
STATIC int
memfs_link(vnode_t *dvp, vnode_t *srcvp, char *tnm, cred_t *cred)
{
	mnode_t *parent = VP_TO_MNODE(dvp);
	mnode_t *from;
	mem_vfs_t *mem_vfsp = VTOTM(dvp);
	int error;
	mnode_t *found = NULL;
	vnode_t *rvp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(!(parent->mno_flags & MEMFS_UNNAMED));

	if (srcvp->v_type == VDIR && pm_denied(cred, P_FILESYS))
		return (EPERM);

	/*
	 * Get the write lock now because we'll probably be
	 * entering a new file.
	 */
	RWSLEEP_WRLOCK(&parent->mno_rwlock, PRINOD);
	error = memfs_dirlook(parent, tnm, &found, cred);

	if (error == 0) {
		ASSERT(found);
		MNODE_RELE(found);
		RWSLEEP_UNLOCK(&parent->mno_rwlock);
		return (EEXIST);
	}

	if (error != ENOENT) {
		RWSLEEP_UNLOCK(&parent->mno_rwlock);
		return (error);
	}

	if (VOP_REALVP(srcvp, &rvp) == 0)
		srcvp = rvp;
	from = VP_TO_MNODE(srcvp);

	RWSLEEP_WRLOCK(&from->mno_rwlock, PRINOD);
	error = memfs_direnter(mem_vfsp, parent, tnm, from, cred);
	MNODE_MODIFIED(from);
	RWSLEEP_UNLOCK(&from->mno_rwlock);

	MNODE_MODIFIED(parent);
	RWSLEEP_UNLOCK(&parent->mno_rwlock);

	return (error);
}

/*
 * STATIC int
 * memfs_rename(vnode_t *odvp, char *onm, vnode_t *ndvp, char *nnm,
 * cred_t *cred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
STATIC int
memfs_rename(vnode_t *odvp, char *onm, vnode_t *ndvp, char *nnm, cred_t *cred)
{
	mnode_t *fromparent = VP_TO_MNODE(odvp);
	mnode_t *toparent = VP_TO_MNODE(ndvp);
	mnode_t *frommp = NULL;
	mnode_t *tomp = NULL;
	mem_vfs_t *mem_vfsp = VTOTM(odvp);
	int error, doingdirectory;
	int samedir = 0;	/* set if odvp == ndvp */

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(!(fromparent->mno_flags & MEMFS_UNNAMED));
	ASSERT(!(toparent->mno_flags & MEMFS_UNNAMED));

	/*
	 * Check for renaming '.' or '..'
	 * This implies that frommp != fromparent
	 */
	if ((strcmp(onm, ".") == 0) || (strcmp(onm, "..") == 0))
		return (EINVAL);

	/*
	 * The locking strategy is different here if we're
	 * renaming a file within the same directory.
	 * A deadlock can occur since we try to acquire the
	 * write lock on the destination directory after acquiring
	 * the same on the source directory.
	 */
	samedir = (fromparent == toparent);

	/*
	 * Enter the rename mutex for this file system ... sigh.
	 */
	SLEEP_LOCK(&memfs_renamelock, PRINOD);

	/*
	 * Make sure we can delete the old (source) entry.
	 */
	RWSLEEP_WRLOCK(&fromparent->mno_rwlock, PRINOD);
	error = memfs_taccess(fromparent, VWRITE, cred);
	if (error) {
		RWSLEEP_UNLOCK(&fromparent->mno_rwlock);
		SLEEP_UNLOCK(&memfs_renamelock);
		return (error);
	}

	/*
	 * Make sure we can rename into the new (destination) directory.
	 */
	if (!samedir) {
		RWSLEEP_WRLOCK(&toparent->mno_rwlock, PRINOD);
		error = memfs_taccess(fromparent, VWRITE, cred);
		if (error) {
			RWSLEEP_UNLOCK(&toparent->mno_rwlock);
			RWSLEEP_UNLOCK(&fromparent->mno_rwlock);
			SLEEP_UNLOCK(&memfs_renamelock);
			return (error);
		}
	}

	/*
	 * Lookup source
	 */
	error = memfs_dirlook(fromparent, onm, &frommp, cred);
	if (error || frommp == toparent ||
			(fromparent == toparent && strcmp(onm, nnm) == 0)) {
		if (!error)
			error = EINVAL;
		RWSLEEP_UNLOCK(&fromparent->mno_rwlock);
		if (!samedir)
			RWSLEEP_UNLOCK(&toparent->mno_rwlock);
		SLEEP_UNLOCK(&memfs_renamelock);
		return (error);
	}
	RWSLEEP_WRLOCK(&frommp->mno_rwlock, PRINOD);

	if ((fromparent->mno_mode & S_ISVTX) && pm_denied(cred, P_OWNER) &&
	    cred->cr_uid != fromparent->mno_uid &&
	    frommp->mno_uid != cred->cr_uid &&
	    memfs_taccess(frommp, VWRITE, cred) != 0) {
		error = EPERM;
		goto done;
	}

	doingdirectory = ((frommp->mno_mode & S_IFMT) == S_IFDIR);

	/*
	 * At this point we have the writer version of the contents lock
	 * on the source, destination and the file we're renaming
	 */

	if (doingdirectory) {
		mnode_t *parent_mp = NULL;
		mnode_t *curr_mp = toparent;

		/*
		 * POSIX requires write permission on a directory in order
		 * to rename it
		 */
		if ((error = memfs_taccess(frommp, VWRITE, cred)) != 0)
			goto done;

		/*
		 * Check that we don't move a parent directory down
		 * to one of its children, or we'd end up removing
		 * this subtree from the directory hierarchy.
		 */
		VN_HOLD(MNODE_TO_VP(curr_mp));
		while (frommp != curr_mp) {
			/*
			 * Only lock curr_mp if we know we're
			 * not going to deadlock.  We already have
			 * the locks on the src and dest dir
			 */
			if (curr_mp != fromparent && curr_mp != toparent)
				RWSLEEP_WRLOCK(&curr_mp->mno_rwlock, PRINOD);
			error = memfs_dirlook(curr_mp, "..", &parent_mp, cred);
			if (curr_mp != fromparent && curr_mp != toparent)
				RWSLEEP_UNLOCK(&curr_mp->mno_rwlock);
			if (error) {
				MNODE_RELE(curr_mp);
				goto done;
			}
			/*
			 * We're okay if we traverse the directory tree up to
			 * the root directory and don't run into the
			 * parent directory.
			 */
			if (curr_mp == parent_mp) { /* at roomemfs_dir */
				MNODE_RELE(curr_mp);
				MNODE_RELE(parent_mp);
				goto nomparent;
			}
			MNODE_RELE(curr_mp);
			curr_mp = parent_mp;
		}
		MNODE_RELE(curr_mp);
		error = EINVAL;
		goto done;
	}

nomparent:
	/*
	 * Look up the target
	 */
	error = memfs_dirlook(toparent, nnm, &tomp, cred);
	/* 
	 * check if the from parent mnode and the target mnode are the same.
	 */
	if (fromparent == tomp) {
		error = EISDIR;
		goto done;
	}
	/*
	 * entry of name already exists
	 */
	if (error == 0) {
		RWSLEEP_WRLOCK(&tomp->mno_rwlock, PRINOD);
		if ((tomp->mno_mode & S_IFMT) == S_IFDIR) {
			if (!doingdirectory)
				error = EISDIR;
			if (MNODE_TO_VP(tomp)->v_count > 2)
				error = ENOTEMPTY;	/* EEXIST ? */
			else
				error = EEXIST;
			RWSLEEP_UNLOCK(&tomp->mno_rwlock);
			goto done;
		} else if (doingdirectory) {
			error = ENOTDIR;
			RWSLEEP_UNLOCK(&tomp->mno_rwlock);
			goto done;
		}

		if (tomp == frommp) {	/* same file */
			RWSLEEP_UNLOCK(&tomp->mno_rwlock);
			goto done;
		}

		/*
		 * Unlink old target
		 */
		error = memfs_dirremove(mem_vfsp, toparent, tomp, nnm, cred);
		RWSLEEP_UNLOCK(&tomp->mno_rwlock);
		if (error)
			goto done;
	} else if (error != ENOENT)
		goto done;

	/*
	 * Link source to new target
	 */
	error = memfs_direnter(mem_vfsp, toparent, nnm, frommp, cred);
	if (error) {
		/*
		 * Re-enter old target into directory if necessary
		 */
		if (tomp) {
			RWSLEEP_UNLOCK(&tomp->mno_rwlock);
			if (memfs_direnter(mem_vfsp, toparent, nnm, tomp, cred))
			    /*
			     *+ Re-entering old target into directory failed.
			     *+ This indicates either a software bug or memfs
			     *+ file system corruption.
			     *+ Corrective action - reboot.
			     */
			    cmn_err(CE_PANIC,
			    "memfs_rename: error reentering old target %s\n",
									nnm);
		}
		goto done;
	}

	/*
	 * If renaming a directory moves it into a different parent directory,
	 * rename the .. entry to point at new parent.
	 */
	if (doingdirectory) {
		if (!samedir) {
			mnode_t *tmpmp;

			error = memfs_dirlook(frommp, "..", &tmpmp, cred);
			if (error) {
				/*
				 *+ When renaming a directory, lookup of the
				 *+ parent directory of the source failed.
				 *+ This indicates either a software bug or
				 *+ memfs file system corruption.
				 *+ Corrective action - reboot.
				 */
				cmn_err(CE_PANIC,
				    "memfs_rename:err %d in finding '..' in %x\n",
					error, frommp);
				MNODE_RELE(tmpmp);
				goto done;
			}

			ASSERT(tmpmp == fromparent);
			error = memfs_dirremove(mem_vfsp, frommp, tmpmp, "..", cred);
			if (error)
				/*
				 *+ When renaming a directory, removing the
				 *+ ".." entry of the source directory failed.
				 *+ This indicates either a software bug or
				 *+ memfs file system corruption.
				 *+ Corrective action - reboot.
				 */
				cmn_err(CE_PANIC, "memfs_rename: memfs_dirremove '..' 0x%x 0x%x\n",
				    frommp, tmpmp);
			MNODE_RELE(tmpmp);
			error = memfs_direnter(mem_vfsp, frommp, "..", toparent, cred);
			if (error)
				/*
				 *+ When renaming a directory, adding the
				 *+ ".." entry of the source directory failed.
				 *+ This indicates either a software bug or
				 *+ memfs file system corruption.
				 *+ Corrective action - reboot.
				 */
				cmn_err(CE_PANIC, "memfs_rename: memfs_direnter '..' 0x%x 0x%x\n",
				    frommp, toparent);
		}
	}
	MNODE_MODIFIED(toparent);

	/*
	 * Unlink source
	 */
	error = memfs_dirremove(mem_vfsp, fromparent, frommp, onm, cred);
	MNODE_MODIFIED(fromparent);

done:
	RWSLEEP_UNLOCK(&frommp->mno_rwlock);
	if (!samedir)
		RWSLEEP_UNLOCK(&toparent->mno_rwlock);
	RWSLEEP_UNLOCK(&fromparent->mno_rwlock);
	MNODE_RELE(frommp);
	if (tomp)
		MNODE_RELE(tomp);
	SLEEP_UNLOCK(&memfs_renamelock);
	return (error);
}

/*
 * STATIC int
 * memfs_mkdir(vnode_t *dvp, char *nm, vattr_t *va, vnode_t **vpp, cred_t *cred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
STATIC int
memfs_mkdir(vnode_t *dvp, char *nm, vattr_t *va, vnode_t **vpp, cred_t *cred)
{
	mnode_t *parent = VP_TO_MNODE(dvp);
	mnode_t *self = NULL;
	mem_vfs_t *mem_vfsp = VTOTM(dvp);
	vnode_t *vp;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(!(parent->mno_flags & MEMFS_UNNAMED));

	RWSLEEP_WRLOCK(&parent->mno_rwlock, PRINOD);
	/*
	 * Make sure we have write permissions on the current directory
	 * before we allocate anything
	 */
	if (error = memfs_taccess(parent, VWRITE, cred)) {
		RWSLEEP_UNLOCK(&parent->mno_rwlock);
		return (error);
	}

	/*
	 * We don't need to grab the memfs_list_lck because we know that
	 * all those who modify mno_rwlock also hold the RW lock (in
	 * addition to the memfs_list_lck).
	 */
	if (parent->mno_nlink == 0) {
		RWSLEEP_UNLOCK(&parent->mno_rwlock);
		return (ENOENT);
	}

	error = memfs_dirlook(parent, nm, &self, cred);
	if (error == 0) {
		ASSERT(self);
		RWSLEEP_UNLOCK(&parent->mno_rwlock);
		MNODE_RELE(self);
		return (EEXIST);
	}
	if (error != ENOENT) {
		RWSLEEP_UNLOCK(&parent->mno_rwlock);
		return (error);
	}
	self = mnode_alloc(mem_vfsp, va, cred);
	if (self == NULL) {
		RWSLEEP_UNLOCK(&parent->mno_rwlock);
		return (ENOSPC);
	}
	vp = MNODE_TO_VP(self);
	/*
	 * use memfs_dirinit to initialize directory since
	 * memfs_direnter would unnecessarily check permissions
	 * for '.' and '..'
	 */
	if (error = memfs_dirinit(mem_vfsp, parent, self, cred)) {
		RWSLEEP_UNLOCK(&self->mno_rwlock);
		VN_LOCK(vp);
		--vp->v_count;
		VN_UNLOCK(vp);
		memfs_inactive_common(self);
		RWSLEEP_UNLOCK(&parent->mno_rwlock);
		return (error);
	}
	error = memfs_direnter(mem_vfsp, parent, nm, self, cred);
	if (error) {
		if (memfs_dirremove(mem_vfsp, self, parent, "..", cred))
			/*
			 *+ When making a directory, entering the name into
			 *+ its parent directory failed. When trying to
			 *+ back out and remove the ".." entry in the new
			 *+ directory, it failed.
			 *+ This indicates either a software bug or memfs
			 *+ file system corruption. Corrective action - reboot.
			 */
			cmn_err(CE_PANIC, "memfs_mkdir: memfs_dirremove '..'");
		if (memfs_dirremove(mem_vfsp, self, self, ".", cred))
			/*
			 *+ When making a directory, entering the name into
			 *+ its parent directory failed. When trying to
			 *+ back out and remove the "." entry for the new
			 *+ directory, it failed.
			 *+ This indicates either a software bug or memfs
			 *+ file system corruption. Corrective action - reboot.
			 */
			cmn_err(CE_PANIC, "memfs_mkdir: memfs_dirremove '.'");
		RWSLEEP_UNLOCK(&self->mno_rwlock);
		VN_LOCK(vp);
		--vp->v_count;
		VN_UNLOCK(vp);
		memfs_inactive_common(self);
		RWSLEEP_UNLOCK(&parent->mno_rwlock);

		return (error);
	}
	RWSLEEP_UNLOCK(&self->mno_rwlock);
	MNODE_MODIFIED(parent);
	RWSLEEP_UNLOCK(&parent->mno_rwlock);
	*vpp = vp;
	return (0);
}

/*
 * STATIC int
 * memfs_rmdir(vnode_t *dvp, char *nm, vnode_t *cdir, cred_t *cred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
STATIC int
memfs_rmdir(vnode_t *dvp, char *nm, vnode_t *cdir, cred_t *cred)
{
	mnode_t *parent = VP_TO_MNODE(dvp);
	mnode_t *self = NULL;
	mem_vfs_t *mem_vfsp = VTOTM(dvp);
	int error = 0;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(!(parent->mno_flags & MEMFS_UNNAMED));

	/*
	 * return error when removing . and ..
	 */
	if (strcmp(nm, ".") == 0)
		return (EINVAL);
	if (strcmp(nm, "..") == 0)
		return (EEXIST);
	RWSLEEP_WRLOCK(&parent->mno_rwlock, PRINOD);
	error = memfs_dirlook(parent, nm, &self, cred);
	if (error) {
		RWSLEEP_UNLOCK(&parent->mno_rwlock);
		return (error);
	}
	RWSLEEP_WRLOCK(&self->mno_rwlock, PRINOD);
	if ((parent->mno_mode & S_ISVTX) && pm_denied(cred, P_OWNER) &&
	    cred->cr_uid != parent->mno_uid && self->mno_uid != cred->cr_uid &&
	    memfs_taccess(self, VWRITE, cred) != 0) {
		error = EPERM;
		goto done;
	}
	 
	if (MNODE_TO_VP(self) == dvp || MNODE_TO_VP(self) == cdir) {
		error = EINVAL;
		goto done;
	}
	if ((self->mno_mode & S_IFMT) != S_IFDIR) {
		error = ENOTDIR;
		goto done;
	}

	/*
	 * Check for the size of an empty directory
	 * i.e. only includes entries for "." and ".."
	 */
	if (self->mno_size > MEMFS_DIRENT_SIZE(1) + MEMFS_DIRENT_SIZE(2)) {
		error = EEXIST;
		goto done;
	}

	error = memfs_dirremove(mem_vfsp, parent, self, nm, cred);
	if (error == 0) {
		/*
		 * Release the reference from memfs_dirlook().
		 */
		memfs_dirtrunc(mem_vfsp, self, B_FALSE);
		MNODE_MODIFIED(parent);
	}
	RWSLEEP_UNLOCK(&parent->mno_rwlock);
	return (error);
done:
	RWSLEEP_UNLOCK(&self->mno_rwlock);
	RWSLEEP_UNLOCK(&parent->mno_rwlock);
	MNODE_RELE(self);
	return (error);
}

/*
 * STATIC int
 * memfs_readdir(vnode_t *vp, struct uio *uiop, cred_t *cred, int *eofp)
 *
 * Calling/Exit State:
 *	The inode rwlock is held in shared mode on entry and at exit.
 *
 * Description:
 */
/*ARGSUSED*/
STATIC int
memfs_readdir(vnode_t *vp, struct uio *uiop, cred_t *cred, int *eofp)
{
	mnode_t *mp = VP_TO_MNODE(vp);
	struct memfs_dirent *tdp;
	int error, direntsz, namelen;
	struct dirent *dp;
	uint_t offset;
	uint_t total_bytes_wanted;
	int outcount = 0;
	int bufsize;
	void * outbuf;

	ASSERT(!(mp->mno_flags & MEMFS_UNNAMED));

	if (uiop->uio_iovcnt != 1)
		return (EINVAL);

	RWSLEEP_RDLOCK(&mp->mno_rwlock, PRINOD);
	if ((mp->mno_mode & S_IFMT) != S_IFDIR) {
		RWSLEEP_UNLOCK(&mp->mno_rwlock);
		return (ENOTDIR);
	}

	/*
	 * We don't need to grab the memfs_list_lck because we know that
	 * all those who modify mno_rwlock also hold the RW lock (in
	 * addition to the memfs_list_lck).
	 */
	if (mp->mno_nlink == 0) {
		RWSLEEP_UNLOCK(&mp->mno_rwlock);
		if (eofp) {
			*eofp = 1;
		}
		return 0;
	}

	if (mp->mno_dir == NULL)
		/*
		 *+ An mnode of VDIR type does not have any directory
		 *+ entries. 
		 *+ This indicates either a software bug or memfs
		 *+ file system corruption. Corrective action - reboot.
		 */
		cmn_err(CE_PANIC, "empty directory 0x%x", mp);

	/*
	 * Get space for multiple directory entries
	 */
	total_bytes_wanted = uiop->uio_iov->iov_len;
	bufsize = total_bytes_wanted + sizeof (struct dirent);
	outbuf = kmem_zalloc(bufsize, KM_SLEEP);
	dp = (struct dirent *)outbuf;
	direntsz = (char *)dp->d_name - (char *)dp;

	offset = 0;
	tdp = mp->mno_dir;
	while (tdp) {
		offset = tdp->td_offset;
		if (offset >= uiop->uio_offset) {
			namelen = tdp->td_namelen;
			dp->d_reclen = (direntsz + namelen + 1 + (NBPW - 1))
			    & ~(NBPW - 1);
			if (outcount + dp->d_reclen > total_bytes_wanted)
				break;
			ASSERT(tdp->td_mnode != NULL);
			dp->d_ino = (u_long)tdp->td_mnode->mno_nodeid;
			dp->d_off = tdp->td_offset + 1;
			bcopy(tdp->td_name, dp->d_name, namelen);
			dp->d_name[namelen] = '\0';
			outcount += dp->d_reclen;
			ASSERT(outcount <= bufsize);
			dp = (struct dirent *)((int)dp + dp->d_reclen);
		}
		tdp = tdp->td_next;
	}
	error = uiomove((char *)outbuf, (long)outcount, UIO_READ, uiop);
	if (!error) {
		uiop->uio_offset = offset;
		if (eofp)
			if (tdp == NULL) {
				*eofp = 1;
				uiop->uio_offset += 1;
			} else
				*eofp = 0;
	}
	MNODE_ACCESSED(mp);
	kmem_free((void *)outbuf, bufsize);
	RWSLEEP_UNLOCK(&mp->mno_rwlock);
	return (error);
}

/*
 * STATIC int
 * memfs_symlink(vnode_t *dvp, char *lnm, vattr_t *tva, char *tnm, cred_t *cred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
/*ARGSUSED*/
STATIC int
memfs_symlink(vnode_t *dvp, char *lnm, vattr_t *tva, char *tnm, cred_t *cred)
{
	mnode_t *parent = VP_TO_MNODE(dvp);
	mnode_t *self = NULL;
	mem_vfs_t *mem_vfsp = VTOTM(dvp);
	char *cp = NULL;
	vnode_t *vp;
	int error, len;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(!(parent->mno_flags & MEMFS_UNNAMED));

	RWSLEEP_WRLOCK(&parent->mno_rwlock, PRINOD);
	error = memfs_dirlook(parent, lnm, &self, cred);

	if (error == 0) {
		/*
		 * The entry already exists
		 */
		RWSLEEP_UNLOCK(&parent->mno_rwlock);
		MNODE_RELE(self);
		return (EEXIST);	/* was 0 */
	}
	if (error != ENOENT) {
		RWSLEEP_UNLOCK(&parent->mno_rwlock);
		if (self != NULL)
			MNODE_RELE(self);
		return (error);
	}
	self = mnode_alloc(mem_vfsp, tva, cred);
	if (self == NULL) {
		RWSLEEP_UNLOCK(&parent->mno_rwlock);
		return (ENOSPC);
	}
	vp = MNODE_TO_VP(self);
	len = strlen(tnm);
	cp = (char *)memfs_kmemalloc(mem_vfsp, (uint_t)len, KM_SLEEP);
	if (cp == NULL) {
		RWSLEEP_UNLOCK(&self->mno_rwlock);
		VN_LOCK(vp);
		--vp->v_count;
		VN_UNLOCK(vp);
		memfs_inactive_common(self);
		RWSLEEP_UNLOCK(&parent->mno_rwlock);
		return (ENOSPC);
	}
	(void) strncpy(cp, tnm, len);
	self->mno_symlink = cp;
	self->mno_size = len;
	error = memfs_direnter(mem_vfsp, parent, lnm, self, cred);
	MNODE_MODIFIED(parent);
	RWSLEEP_UNLOCK(&parent->mno_rwlock);
	RWSLEEP_UNLOCK(&self->mno_rwlock);
	if (error) {
		VN_LOCK(vp);
		--vp->v_count;
		VN_UNLOCK(vp);
		memfs_inactive_common(self);
		return (error);
	}
	MNODE_RELE(self);
	return (error);
}

/*
 * STATIC int
 * memfs_readlink(vnode_t *vp, struct uio *uiop, cred_t *cred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
/*ARGSUSED*/
STATIC int
memfs_readlink(vnode_t *vp, struct uio *uiop, cred_t *cred)
{
	mnode_t *mp = VP_TO_MNODE(vp);
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(!(mp->mno_flags & MEMFS_UNNAMED));

	if (vp->v_type != VLNK)
		return (EINVAL);
	RWSLEEP_RDLOCK(&mp->mno_rwlock, PRINOD);
	error = uiomove(mp->mno_symlink, (int)mp->mno_size,
	    UIO_READ, uiop);
	MNODE_ACCESSED(mp);
	RWSLEEP_UNLOCK(&mp->mno_rwlock);
	if (error)
		return (error);
	return (0);
}

/*
 * STATIC int
 * memfs_fsync(vnode_t *vp, cred_t *cred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
/*ARGSUSED*/
STATIC int
memfs_fsync(vnode_t *vp, cred_t *cred)
{
	ASSERT(!(VP_TO_MNODE(vp)->mno_flags & MEMFS_UNNAMED));

	return (0);
}

/*
 * STATIC void
 * memfs_inactive(vnode_t *vp, struct cred *cred)
 *	The VOP_INACTIVE function for memefs.
 *
 * Calling/Exit State:
 *	Called at PLBASE with no spin LOCKs held and returns that way.
 *	Upon return, the vnode is inactivated and the memfs file is
 *	destroyed.
 */

/* ARGSUSED */
STATIC void
memfs_inactive(vnode_t *vp, struct cred *cred)
{
	mnode_t *mp = VP_TO_MNODE(vp);
 
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * We need to remove this mnode from the mnode list in order to
	 * inactivate it. However, it is possible that another LWP has a
	 * VN_HOLD on it. In that case, it will be truly inactivated when the
	 * that LWP finally performs VN_RELE operation.
	 */
	(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
	VN_LOCK(vp);
	if (vp->v_count != 1) {
		/*
		 * We lose. Another LWP has its claws on it.
		 */
		--vp->v_count;
		VN_UNLOCK(vp);
		UNLOCK(&memfs_list_lck, PLBASE);
		return;
	}

	/*
	 * Normal case, mnode is truly being inactivated.
	 */
	vp->v_count = 0;
	VN_UNLOCK(vp);

	/*
	 * If the mnode is named, and if nlink is non-zero, then
	 * just let it stay on the active list.
	 */
	if (!(mp->mno_flags & MEMFS_UNNAMED) && mp->mno_nlink != 0) {
		UNLOCK(&memfs_list_lck, PLBASE);
		return;
	}

	/*
         * It is possible that the vnode we are inactivating uses
         * backing store on a swap device that is being, or is about
         * to be, deleted. For this reason, we add the mnode to the
         * ``inactive'' list. The swap delete operation uses the
         * inactive list to wait for mnode inactivations to complete.
         */
	MNODE_LIST_DELETE(MNODE_TO_MNODEH(mp));
        MNODE_LIST_INSERT(MNODE_TO_MNODEH(mp), &mnode_ianchor);
	UNLOCK(&memfs_list_lck, PLBASE);

	/*
	 * If the caller inactivates the mnode when sleeping is not
	 * permitted (specified by the MEMFS_NSINACT flag), then attempt to
	 * abort all pages now (because once we call memfs_trunc() we are
	 * committed to the abort succeeding). If we succeed with the abort
	 * then we will complete the inactivation now. Otherwise, it will be
	 * necessary to turn this mnode into a MEMFS_DELABORT case. This
	 * latter case won't happen very often if the caller clean the pages
	 * before inactivating the mnode. However, even then,
	 * pvn_abort_range_nosleep() can still fail to kmem_alloc a marker
	 * page, or the page daemon may have grabbed one of the the mnodes's
	 * pages before the last ublock_lock().
	 */
	if (mp->mno_size != 0 && (mp->mno_flags & MEMFS_NSINACT)
	    && !pvn_abort_range_nosleep(MNODE_TO_VP(mp), 0, 0)) {
		(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
		mp->mno_flags =
			(mp->mno_flags & ~MEMFS_NSINACT) | MEMFS_DELABORT;
		++memfs_ndelabort;
		UNLOCK(&memfs_list_lck, PLBASE);
		return;
	}

	memfs_inactive_common(mp);

}

/*
 * STATIC void
 * memfs_inactive_common(mnode_t *mp)
 *	Complete the inactivation of an mnode.
 *
 * Calling/Exit State:
 *	Called at PLBASE with no spin LOCKs held and returns that way.
 *	Upon return, the vnode is inactivated and the memfs file is
 *	destroyed.
 */

STATIC void
memfs_inactive_common(mnode_t *mp)
{

	vnode_t *vp = MNODE_TO_VP(mp);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Abort all pages and free the backing store.
	 */
	if (mp->mno_size != 0) {
		if (vp->v_type != VLNK)
			memfs_trunc_down(mp, 0);
		else {
			memfs_kmemfree(VTOTM(vp), mp->mno_symlink,
							mp->mno_size);
			mp->mno_size = 0;
		}
	}

	if (!(mp->mno_flags & MEMFS_UNNAMED)) {
		mnode_free(VTOTM(vp), mp);
	}

	/*
	 * Now, delete the mnode from the inactive list, waking up any
	 * potential swap delete waiters.
	 */
	(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
	MNODE_LIST_DELETE(MNODE_TO_MNODEH(mp));
	UNLOCK(&memfs_list_lck, PLBASE);
	if (SV_BLKD(&memfs_sv))
		SV_BROADCAST(&memfs_sv, 0);

        VN_SOFTRELE(vp);

}

/*
 * STATIC void
 * memfs_release(vnode_t *vp)
 *      Release the storage for a totally unreferenced vnode.
 *
 * Calling/Exit State:
 *      The user may hold locks. However, no lock is held at FS_SFSLISTPL
 *      or above.
 *
 *      This function does not block.
 */
STATIC void
memfs_release(vnode_t *vp)
{
        mnode_t         *mp = VP_TO_MNODE(vp);
        uint_t nbytes;

        /*
         * The mnode is privately held at this point.
         * Therefore, no locking is necesary in order to inspect it.
         */
        ASSERT(VN_IS_RELEASED(vp));
	ASSERT(vp->v_pages == NULL);

	/*
	 * Finally, de-initialize the mnode and free it.
	 */
	MNODE_DEINIT_COMMON(mp);
	if (mp->mno_flags & MEMFS_FIXEDSIZE) {
		nbytes = btop(mp->mno_bsize) * sizeof(memfs_bs_t);
	} else {
		/*
		 * Free the backing store tree.
		 */
		if (!(mp->mno_flags & MEMFS_FIXEDSIZE))
			memfs_delete_bs(mp);
		nbytes = MEMFS_BNODE_BYTES;
	}

	if (mp->mno_flags & MEMFS_UNNAMED) {
		MNODE_DEINIT_UNNAMED(mp);
		nbytes += sizeof(mnode_unnamed_t);
	} else {
		MNODE_DEINIT_NAMED(mp);
		nbytes += sizeof(mnode_t);
	}
	kmem_free(mp, nbytes);
}

/*
 * STATIC int
 * memfs_fid(vnode_t *vp, struct fid **fidpp)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
STATIC int
memfs_fid(vnode_t *vp, struct fid **fidpp)
{
	mnode_t *mp = (mnode_t *)VP_TO_MNODE(vp);
	struct tfid *tfid;

	ASSERT(!(mp->mno_flags & MEMFS_UNNAMED));

	tfid = (struct tfid *)kmem_zalloc(sizeof (struct tfid), KM_SLEEP);
	tfid->tfid_len = sizeof (struct tfid) - sizeof (u_short);

	RWSLEEP_WRLOCK(&mp->mno_rwlock, PRINOD);
	tfid->tfid_ino = mp->mno_nodeid;
	tfid->tfid_gen = mp->mno_gen;
	RWSLEEP_UNLOCK(&mp->mno_rwlock);

	*fidpp = (struct fid *)tfid;
	return (0);
}

/*
 * size_t
 * memfs_map_size(vnode_t *)
 *	Compute the minimal size of a mapping for the entirety of the
 *	memfs file given by ``vp''.
 *
 * Calling/Exit State:
 *	The mnode corresponding to the argument vnode is of fixed size
 *	(MEMFS_FIXEDSIZE).
 *
 *	Returns the minimal size of a mapping (in bytes) which can
 *	map the entire memfs file given by ``vp''.
 */

size_t
memfs_map_size(vnode_t *vp)
{
	return (ptob(btopr(VP_TO_MNODE(vp)->mno_size)));
}

/*
 * void
 * memfs_expand_bs(mnode_t *mp, size_t size)
 *	Expand the backing store tree for a given mnode to where it is
 *	large enought to accomodate a file of the specified size.
 *
 * Calling/Exit State:
 *	Either the mnode is privately held, or the rw lock is held in
 *	WRITEr mode.
 *
 *	Called with no LOCKs held and returns that way.
 */

void
memfs_expand_bs(mnode_t *mp, size_t size)
{
	memfs_bs_t *nrootp, *bsnp, *nbsnp;
	int shift;
	off_t off;

	ASSERT(!(mp->mno_flags & MEMFS_FIXEDSIZE));

	/*
	 * First, grow the tree vertically, developing enough
	 * levels to satsify the request.
	 */
	while (memfs_maxbsize[mp->mno_maxlevel] < size) {
		nrootp = kmem_zalloc(MEMFS_BNODE_BYTES, KM_SLEEP);
		nrootp[0].mbs_ptr = mp->mno_bsnp;
		FSPIN_LOCK(&mp->mno_realloclck);
		mp->mno_bsnp = nrootp;
		++mp->mno_maxlevel;
		FSPIN_UNLOCK(&mp->mno_realloclck);
	}

	/*
	 * Now, fill out the levels.
	 */
	ASSERT(mp->mno_bsize >= size || mp->mno_maxlevel > 0);
	ASSERT(mp->mno_maxlevel <= MEMFS_MAXLEVEL);
	while (mp->mno_bsize < size) {
		off = mp->mno_bsize >> PAGESHIFT;
		bsnp = mp->mno_bsnp;
		shift = MEMFS_SHIFT(mp->mno_maxlevel);
		do {
			nbsnp = bsnp + ((off >> shift) & MEMFS_LVL_MASK);
			if ((bsnp = nbsnp->mbs_ptr) == NULL) {
				bsnp = nbsnp->mbs_ptr =
					kmem_alloc(MEMFS_BNODE_BYTES, KM_SLEEP);
				if (shift == MEMFS_LVL_SHIFT)
					bcopy(memfs_empty_node, bsnp,
					      MEMFS_BNODE_BYTES);
				else
					bzero(bsnp, MEMFS_BNODE_BYTES);
			}
			shift -= MEMFS_LVL_SHIFT;
		} while (shift > 0);
		mp->mno_bsize += MEMFS_KLUSTER_SIZE;
	}
}

/*
 * STATIC void
 * memfs_delete_bs(mnode_t *mp)
 *	Delete the backing store tree for a given mnode.
 *
 * Calling/Exit State:
 *	The mnode is on the inactive list. The mnode is not one of fixed size.
 */

STATIC void
memfs_delete_bs(mnode_t *mp)
{
	int level, shift, minshift, maxshift;
	size_t inc;
	off_t off, off1;
	memfs_bs_t *bsnp;

	ASSERT(!(mp->mno_flags & MEMFS_FIXEDSIZE));

	/*
	 * Delete each level from the tree, starting from the leaves.
	 */
	maxshift = MEMFS_SHIFT(mp->mno_maxlevel);
	for (level = 0; level <= mp->mno_maxlevel; ++level) {
		inc = memfs_maxbsize[level];
		minshift = MEMFS_SHIFT(level);

		/*
		 * Delete each node at this level
		 */
		for (off = 0; off < mp->mno_bsize; off += inc) {
			bsnp = mp->mno_bsnp;
			shift = maxshift;
			off1 = off >> PAGESHIFT;
			while (shift > minshift) {
				bsnp += (off1 >> shift) & MEMFS_LVL_MASK;
				bsnp = bsnp->mbs_ptr;
				ASSERT(bsnp != NULL);
				shift -= MEMFS_LVL_SHIFT;
			}
			/*
			 * Be careful not to delete the node tacked onto
			 * the initial mnode.
			 */
			if (level > 0 || off != 0)
				kmem_free(bsnp, MEMFS_BNODE_BYTES);
		}
	}
}

/*
 * swaploc_t *
 * memfs_lookup_bs(mnode_t *mp, off_t off)
 *	Given an mnode and the offset, find the corresponding location
 *	in the backing store tree.
 *
 * Calling/Exit State:
 *	The caller carries a VN_HOLD for the corresponding vnode
 *	[i.e. for MNODE_TO_VP(mp)].
 *
 *	A pointer to the backing store descriptor is returned.
 *
 * Remarks:
 *	The backing store descriptors being pointed by the return
 *	value are protected by the pseudo MEM_T_LOCK. The MEM_T_LOCK
 *	does not need to be held for this routine to be called.
 */

STATIC swaploc_t *
memfs_lookup_bs(mnode_t *mp, off_t off)
{
	memfs_bs_t *bsnp;
	swaploc_t *swapp;
	int shift, level;

	off >>= PAGESHIFT;
	FSPIN_LOCK(&mp->mno_realloclck);
	bsnp = mp->mno_bsnp;
	level = mp->mno_maxlevel;
	FSPIN_UNLOCK(&mp->mno_realloclck);

	ASSERT(bsnp != NULL);
	if (level > 0) {
		shift = MEMFS_SHIFT(level);
		do {
			bsnp += (off >> shift) & MEMFS_LVL_MASK;
			bsnp = bsnp->mbs_ptr;
			ASSERT(bsnp != NULL);
			shift -= MEMFS_LVL_SHIFT;
		} while (shift > 0);
		off &= MEMFS_LVL_MASK;
	}

	swapp = &bsnp[off].mbs_swaploc;
	ASSERT(swapp != NULL);

	return (swapp);
}

/*
 * STATIC void
 * segmemfs_badop(void)
 *	Illegal operation.
 *
 * Calling/Exit State:
 *	Always panics.
 */
STATIC void
segmemfs_badop(void)
{
	/*
	 *+ A segment operation was invoked using the segmemfs_ops vector
	 *+ other than SOP_KLUSTER.  This indicates a kernel software problem.
	 */
	cmn_err(CE_PANIC, "segmemfs_badop");
	/*NOTREACHED*/
}

/*
 * STATIC int
 * segmemfs_kluster(struct seg *seg, vaddr_t addr, int delta)
 *	SOP_KLUSTER routine for segmemfs.
 *
 * Calling/Exit State:
 *	Always succeeds.
 */

/* ARGSUSED */
STATIC int
segmemfs_kluster(struct seg *seg, vaddr_t addr, int delta)
{
	return (0);
}

/*
 * STATIC void
 * memfs_trunc_down(mnode_t *mp, size_t size)
 *	Decrease the size of a memfs file to ``size''.
 *
 * Calling/Exit State:
 *	The caller iether holds the rw lock in WRITEr mode, or holds the
 *	mnode privately.
 *
 *	Called at PLBASE with no spin LOCKS held and returns that way.
 */
STATIC void
memfs_trunc_down(mnode_t *mp, size_t size)
{
	uint_t extra;
	size_t old_size;
	int klen;
	off_t off, delta;
	swaploc_t *bsp, *kbsp;
	boolean_t fixed_size;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(size < mp->mno_size);

	/*
	 * Clear trailing bytes in the current last page of the file (before
	 * we reduce the file size), but not beyond the old file size.
	 */
	old_size = mp->mno_size;
	off = ptob(btopr(size));
	if (size < off)
		pvn_trunczero(MNODE_TO_VP(mp),
			      size, MIN(old_size, off) - size);

	/*
	 * Now, shrink the file size.
	 */
	mp->mno_size = size;

	/*
	 * Abort the pages beyond the new end of file.
	 *
	 * If inactivating a file when it is not permitted to sleep,
	 * then the pages have already aborted the pages in memfs_inactive().
	 */
	if (!(mp->mno_flags & MEMFS_NSINACT)) {
		pvn_abort_range(MNODE_TO_VP(mp), size, 0);
	}

	/*
	 * Now, free up any backing store assignments beyond the new
	 * end of file.
	 */
	if (off < old_size) {
		fixed_size = (mp->mno_flags & MEMFS_FIXEDSIZE);
		bsp = memfs_lookup_bs(mp, off);
		for (;;) {
			/*
			 * Try to gather up a kluster of contiguous backing
			 * store to free in a single call to swap_free().
			 */
			kbsp = bsp + 1;
			off += PAGESIZE;
			if (SWAPLOC_HAS_BACKING(bsp)) {
				klen = 1;
				delta = PAGESIZE;
				while (((off & MEMFS_KLUSTER_OFF) != 0) &&
				       off < old_size &&
				       SWAPLOC_CONTIG(bsp, delta, kbsp)) {

					off += PAGESIZE;
					delta += PAGESIZE;
					++klen;
					SWAPLOC_MAKE_EMPTY(kbsp);
					++kbsp;
				}
				swap_free(klen, bsp, B_FALSE);
				SWAPLOC_MAKE_EMPTY(bsp);
			}
			if (off >= old_size)
				break;
			bsp = kbsp;
			if (!fixed_size && (off & MEMFS_KLUSTER_OFF) == 0)
				bsp = memfs_lookup_bs(mp, off);
		}
	}

	/*
	 * Finally, give up the memory reservations for the swap
	 * backing store.
	 */
	extra = btopr(old_size) - btopr(size);
	mem_unresv(extra, M_SWAP);

	if (!(mp->mno_flags & MEMFS_UNNAMED)) {
		struct mem_vfs *mem_vfsp;

		mem_vfsp = VTOTM(MNODE_TO_VP(mp));

		(void) LOCK(&mem_vfsp->mem_contents, FS_MEMFS_PL);
		mem_vfsp->mem_swapmem -= extra;
		UNLOCK(&mem_vfsp->mem_contents, PLBASE);
	}
}

/*
 * int
 * memfs_trunc_up(mnode_t *mp, size_t size, boolean_t do_zero)
 *	Increase the size of a memfs file to ``size''.
 *
 * Calling/Exit State:
 *	The caller either holds both the rw lock in WRITEr mode, or
 *	hold the mnode privately.
 *
 *	If do_zero is equal to B_TRUE, then we zero the trailing bytes in
 *	the current last page of the file, but not beyond the new end of
 *	file.
 *
 *	Called at PLBASE with no spin LOCKS held and returns that way.
 */

int
memfs_trunc_up(mnode_t *mp, size_t size, boolean_t do_zero)
{
	uint_t needed;
	size_t osize, epsize;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(size > mp->mno_size);

	/*
	 * get some more swap reservations
	 */
	needed = btopr(size) - btopr(mp->mno_size);
	if (!mem_resv(needed, M_SWAP))
		return (EAGAIN);

	if (!(mp->mno_flags & MEMFS_UNNAMED)) {
		struct mem_vfs *mem_vfsp;

		mem_vfsp = VTOTM(MNODE_TO_VP(mp));

		(void) LOCK(&mem_vfsp->mem_contents, FS_MEMFS_PL);
		if ((mem_vfsp->mem_swapmem += needed)
					 > mem_vfsp->mem_swapmax) {
			mem_vfsp->mem_swapmem -= needed;
			UNLOCK(&mem_vfsp->mem_contents, PLBASE);
			mem_unresv(needed, M_SWAP);

			/*
			 *+ Mounted memfs file system is overflowing its swap
			 *+ space limit.
			 *+ Corrective action - remove unused file/directories.
			 */
			cmn_err(CE_WARN,
			    "%s: File system full, memory limit exceeded\n",
			    mem_vfsp->mem_mntpath);
			return (ENOSPC);
		}
		UNLOCK(&mem_vfsp->mem_contents, PLBASE);
	}

	/*
	 * Now, expand the backing store tree, if required.
	 */
	if (mp->mno_bsize < size)
		memfs_expand_bs(mp, size);

	/*
	 * Now expand the file size.
	 */
	osize = mp->mno_size;
	mp->mno_size = size;

	/*
	 * Clear trailing bytes in the current last page of the
	 * file (before we increase its size), but not beyond the
	 * new end of file.
	 */
	if (do_zero) {
		epsize = ptob(btopr(osize));
		if (osize < epsize)
			pvn_trunczero(MNODE_TO_VP(mp),
				      osize, MIN(epsize, size) - osize);
	}

	return (0);
}

/*
 * STATIC int 
 * memfs_getpage(vnode_t *vp, uint_t off, uint_t len, uint_t *promp,
 *		 page_t *pl[], uint_t plsz, struct seg *seg, vaddr_t addr,
 *		 enum seg_rw rw, cred_t *cred)
 *	Implement the VOP_GETPAGE function for memfs.
 *
 * Calling/Exit State:
 *	Called at PLBASE with no spin LOCKs held, and returns the same way.
 *	The caller is prepared to block.
 *
 * Remarks:
 *	PERF: Read-ahead is not supported by memfs, primarily because we
 *	expect memfs files to be used primarily for short lived files, and
 *	thus to have high memory residency.
 */

STATIC int 
memfs_getpage(vnode_t *vp, uint_t off, uint_t len, uint_t *protp, page_t *pl[],
	     uint_t plsz, struct seg *seg, vaddr_t addr, enum seg_rw rw,
	     cred_t *cred)
{
	int err;
	mnode_t *mp;
	size_t msize;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(pl != NULL);

	/*
	 * Convert vnode pointer to mnode pointer.
	 */
	mp = VP_TO_MNODE(vp);
	ASSERT(!(mp->mno_flags & MEMFS_UNNAMED) || (addr & PAGEOFFSET) == 0);

	/*
	 * Check file size.
	 *
	 * If this is a fixed size file, then the users (System V Shared
	 * Memory, SHARED /dev/zero, and xsd) will never read past the end of
	 * the file. Or, the user may have promised not to read past the end
	 * of the file by creating it with ``MEMFS_NPASTEOF''. In either
	 * case, there is no need to check the file length.
	 */
	if (!(mp->mno_flags & (MEMFS_FIXEDSIZE|MEMFS_NPASTEOF))) {
		/*
		 * If this is an S_OVERWRITE case, then we already hold the
		 * RW lock, so that it is legal to change the file size.
		 */
		if (rw == S_OVERWRITE) {
			ASSERT(!(mp->mno_flags & MEMFS_UNNAMED));
			if (off + len > mp->mno_size) {
				err = memfs_trunc_up(mp, off + len, B_FALSE);
				if (err)
					return (err);
			}
		} else {
			/*
			 * Not an S_OVERWRITE case.
			 *
			 * Check the file size. If the file needs more
			 * pages to satisfy the request, then return an
			 * error. If the file needs more bytes to
			 * satisfy the request, then simply trim back the
			 * length.
			 */
			READ_SYNC();
			msize = mp->mno_size;
			if (btopr(off + len) > btopr(msize)) {
				return (EFAULT);
			} else if (off + len > msize) {
				len = mp->mno_size - off;
			}
		}
	} else {
#ifdef DEBUG
		READ_SYNC();
		ASSERT(btopr(off + len) <= btopr(mp->mno_size));
#endif
	}

	/*
	 * Memfs allows the client to write to pages without backing store,
	 * which is why it is called "memfs".
	 */
	if (protp != NULL)
		*protp = PROT_ALL;

	ASSERT(plsz >= PAGESIZE);
	if (btop(off) == btop(off + len - 1)) {
		err = memfs_getapage(vp, off, len, protp, pl, plsz, seg, addr,
				    rw, cred);
	} else {
		err = pvn_getpages(memfs_getapage, vp, off, len, protp, pl,
				   plsz, seg, addr, rw, cred);
	}

	return (err);
}

/*
 * STATIC int 
 * memfs_getapage(vnode_t *vp, uint_t off, uint_t len, uint_t *protp,
 *		 page_t *pl[], uint_t plsz, struct seg *seg, vaddr_t addr,
 *		 enum seg_rw rw, cred_t *cred)
 *	The internals of the memfs_getpage.
 *
 * Calling/Exit State:
 *	Called at PLBASE with no spin LOCKs held, and returns the same way.
 *	The caller is prepared to block.
 */

/* ARGSUSED */
STATIC int
memfs_getapage(vnode_t *vp, uint_t off, uint_t len, uint_t *protp, page_t *pl[],
	      uint_t plsz, struct seg *seg, vaddr_t addr, enum seg_rw rw,
	      cred_t *cred)
{
	mnode_t *mp;
	off_t roff, swoffset, vp_off, kloff, io_off;
	size_t vp_len, ret_len, diff, io_len;
	page_t *pp, *scanpp, *scan2pp, *rpp;
	page_t **ppp, **eppp;
	page_t *extrapl[MEMFS_KLUSTER_NUM + 1];
	swaploc_t *bsp, *kbsp;
	vnode_t *stvp;
	swapinfo_t *swp;
	void *storeobj;
	int err;

	ASSERT(pl != NULL);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	mp = VP_TO_MNODE(vp);

#ifndef NO_RDMA
	ASSERT((MEMFS_PAGE_FLAGS(mp) == P_DMA) ==
	       ((mp->mno_flags & MEMFS_DMA) != 0));
#endif

	/*
	 * If we find the page in the page cache no need to do any I/O.
	 */
	roff = off & PAGEMASK;
	pp = page_lookup_or_create3(vp, roff, MEMFS_PAGE_FLAGS(mp));
	if (roff >= mp->mno_size) {
		page_abort(pp);
		return (EFAULT);
	}

	if (PAGE_IS_RDLOCKED(pp)) {
		pl[0] = pp;
		pl[1] = NULL;

		return (0); 
	}

	/*
	 * S_OVERWRITE optimization
	 *
	 * Just return the page WRITEr locked. We can skip calling
	 * page_setmod(), since the client is going to write into the page
	 * through a kernel visible mapping.
	 */
	if (rw == S_OVERWRITE && roff == off && len == PAGESIZE) {
		ASSERT(!(VP_TO_MNODE(vp)->mno_flags &
			 (MEMFS_UNNAMED|MEMFS_FIXEDSIZE|MEMFS_NPASTEOF)));
		pl[0] = pp;
		pl[1] = NULL;

		return (0);
	}

	/*
	 * Get backing store information.
	 */
	bsp = memfs_lookup_bs(mp, roff);

	/*
	 * If an I/O error previously occured in memfs_getpage(), when the
	 * swap file was being deleted, then we need to return an EIO now.
	 */
	if (SWAPLOC_IS_IOERR(bsp)) {
		page_abort(pp);
		return (EIO);
	}

	/*
	 * If no backing store is allocated, then this is the first
	 * user access to this page.
	 */
	if (SWAPLOC_IS_EMPTY(bsp)) {
		/*
		 * This is the first user access to this page.
		 * For normal users, we zero the page and mark it dirty.
		 * However, we can skip this for users who do not care
		 * about the initial contents of the page.
		 */
		if (!(mp->mno_flags & MEMFS_NPGZERO)) {
			pagezero(pp, 0, PAGESIZE);
			page_setmod(pp);
		}

		/*
		 * Downgrade the lock and return the page.
		 */
		page_downgrade_lock(pp);
		pl[0] = pp;
		pl[1] = NULL;

		return (0); 
	}

	/*
	 * The page is not in core so we need to bring it back from swap.
	 * We attempt to kluster it in with adjacent pages on the swap
	 * file. Since we do not hold the MEM_T_LOCK(s) on the adjacent
	 * pages, the following scan of the backing store structure
	 * only gives an approximate answer for what we can kluster in.
	 *
	 * Scan for continguous backing store for vnode offsets lower
	 * than pp.
	 */
	vp_len = PAGESIZE;
	vp_off = roff;
	kbsp = bsp - 1;
	while ((vp_off & MEMFS_KLUSTER_OFF) != 0 &&
	       SWAPLOC_CONTIG(kbsp, vp_len, bsp)) {

		vp_len += PAGESIZE;
		vp_off -= PAGESIZE;
		--kbsp;
	}

	/*
	 * Now, scan for continguous backing store for vnode offsets higher
	 * than pp.
	 */
	kloff = roff + PAGESIZE;
	diff = PAGESIZE;
	kbsp = bsp + 1;
	while ((kloff & MEMFS_KLUSTER_OFF) != 0 &&
	       SWAPLOC_CONTIG(bsp, diff, kbsp)) {

		kloff += PAGESIZE;
		diff += PAGESIZE;
		++kbsp;
	}
	vp_len += diff - PAGESIZE;

	if (rw != S_OVERWRITE)
		rpp = pvn_kluster(vp, roff, seg, addr, &io_off, &io_len, vp_off,
			  vp_len, pp);
	else {
		io_off = roff;
		io_len = PAGESIZE;
		rpp = pp;
	}
	ASSERT(rpp != NULL);
	ASSERT(pp->p_offset >= io_off);
	ASSERT(pp->p_offset < io_off + io_len);

	/*
	 * Now, find all pages in the rpp list which no longer have
	 * contiguous backing store with the center page ``pp''. NOW that we
	 * hold all of the pages on the rpp list WRITEr locked, we know that
	 * the backing store situation is stabilized, so that we can now make
	 * a definitive determination of backing store klustering situation.
	 * There is an almost 100% chance that the io_off/io_len pair
	 * returned by pvn_kluster is the definitive answer. However, there
	 * is a miniscule chance that the backing store for one of the
	 * corresponding virtual pages of the file was freed by
	 * memfs_relocate(), but the attempt to read the backing store
	 * failed. Perhaps even more remotely, there is a chance that the
	 * backing store was freed, new backing store was allocated, and then
	 * the page was aborted. This might have happened, for example, while
	 * we were preemptively suspended and swapped out!
	 *
	 * These strange contigencies make it impossible for us to assert
	 * that the pages on the rpp list even have backing store!
	 *
	 * First, a backwards scan from the center page (pp).
	 */
	scanpp = pp->p_prev;
	kbsp = bsp - 1;
	vp_len = PAGESIZE;
	while (scanpp->p_offset < pp->p_offset) {
		ASSERT(scanpp->p_offset >= io_off);
		ASSERT(scanpp->p_offset < io_off + io_len);
		ASSERT(scanpp->p_offset + vp_len == pp->p_offset);
		if (SWAPLOC_CONTIG(kbsp, vp_len, bsp)) {
			--kbsp;
			vp_len += PAGESIZE;
			scanpp = scanpp->p_prev;
		} else {
			do {
				ASSERT(scanpp != pp);
				scan2pp = scanpp->p_prev;
				/*
				 * We need to increment swapdoubles here
				 * because it will be decremented by the
				 * page_abort() via routine memfs_hashout().
				 */
				if (SWAPLOC_HAS_BACKING(kbsp)) {
					(void) LOCK(&swap_lck, VM_SWAP_IPL);
					++swapdoubles;
					UNLOCK(&swap_lck, PLBASE);
				}
				page_sub(&rpp, scanpp);
				page_abort(scanpp);
				scanpp = scan2pp;
				--kbsp;
			} while (scanpp->p_offset < pp->p_offset);
			break;
		}
	}
	vp_off = roff + PAGESIZE - vp_len;

	/*
	 * Now, a forwards scan from the center page (pp).
	 */
	scanpp = pp->p_next;
	kbsp = bsp + 1;
	diff = PAGESIZE;
	while (scanpp->p_offset > pp->p_offset) {
		ASSERT(scanpp->p_offset >= io_off);
		ASSERT(scanpp->p_offset < io_off + io_len);
		ASSERT(pp->p_offset + diff == scanpp->p_offset);
		/*
		 * Recheck the file size, aborting any pages now beyond
		 * the end of file.
		 */
		if (scanpp->p_offset < mp->mno_size &&
		    SWAPLOC_CONTIG(bsp, diff, kbsp)) {
			++kbsp;
			diff += PAGESIZE;
			scanpp = scanpp->p_next;
		} else {
			do {
				ASSERT(scanpp != pp);
				scan2pp = scanpp->p_next;
				/*
				 * We need to increment swapdoubles here
				 * because it will be decremented by the
				 * page_abort() via routine memfs_hashout().
				 */
				if (SWAPLOC_HAS_BACKING(kbsp)) {
					(void) LOCK(&swap_lck, VM_SWAP_IPL);
					++swapdoubles;
					UNLOCK(&swap_lck, PLBASE);
				}
				page_sub(&rpp, scanpp);
				page_abort(scanpp);
				scanpp = scan2pp;
				++kbsp;
			} while (scanpp->p_offset > pp->p_offset);
			break;
		}
	}
	ret_len = (vp_len += diff - PAGESIZE);

	/*
	 * If we have more than plsz bytes on the rpp list, then we will
	 * be unable to return all of the pages to the client. The surplus
	 * pages will be klustered in from the swap file, but will be
	 * immediately freed following the I/O.
	 *
	 * First, scan for pages before the center page which cannot
	 * be returned.
	 */
	scanpp = scan2pp = rpp;
	eppp = extrapl;
	while (ret_len > plsz && scanpp != pp) {
		*eppp++ = scanpp;
		scanpp = scanpp->p_next;
		ret_len -= PAGESIZE;
	}

	/*
	 * Return plsz pages to the client (in the pl list).
	 * Any pages beyond that need to be freed following the I/O.
	 */
	ppp = pl;
	do {
		if (plsz != 0) {
			*ppp++ = scanpp;
			plsz -= PAGESIZE;
		} else {
			*eppp++ = scanpp;
		}
		scanpp = scanpp->p_next;
	} while (scanpp != scan2pp);
	*eppp = *ppp = NULL;

	/*
	 * We are creating swapdoubles. If the I/O fails, then
	 * memfs_hashout() will destroy the doubles we are creating.
	 */
	(void) LOCK(&swap_lck, VM_SWAP_IPL);
	swapdoubles += btop(vp_len);
	UNLOCK(&swap_lck, PLBASE);

	/*
	 * Let the file system managing the swap file bring in pages
	 * from the backing store.
	 */
	swoffset = SWAPLOC_TO_OFF(bsp) - (pp->p_offset - vp_off);
	swp = swaptab[SWAPLOC_TO_IDX(bsp)];
	stvp = swp->si_stvp;
	storeobj = swp->si_obj;

	err = VOP_GETPAGELIST(stvp, swoffset, vp_len, scan2pp, storeobj,
			      0, cred);	
	if (err == 0) {
		/*
		 * unlock pages not being returned to the caller
		 */
		eppp = extrapl;
		while ((pp = *eppp++) != NULL)
			page_unlock(pp);

		/*
		 * downgrade the locks on pages being returned to the
		 * caller
		 */
		ppp = pl;
		while ((pp = *ppp++) != NULL)
			page_downgrade_lock(pp);
	}

	return (err);
}

/*
 * int
 * memfs_putpage(vnode_t *vp, off_t off, uint_t len, int flags, cred_t *cr)
 *
 * Calling/Exit State:
 *	Called at PLBASE with no spin LOCKs held and returns the same way.
 *
 * Description:
 *	Flags are composed of B_ASYNC and B_PAGEOUT.
 *
 *	This routine is only called by pageout(). Flags will be B_ASYNC and
 *	B_PAGEOUT. len will equal PAGESIZE.
 *
 *	This routine is not called from segmap_release(), segvn_sync(),
 *	fsflush(), memfs_sync(), or from any other file syncing routine. This
 *	is in line with the "sync never" policy of memfs.
 */

/* ARGSUSED */
STATIC int
memfs_putpage(vnode_t *vp, off_t off, uint_t len, int flags, cred_t *cr)
{
	mnode_t *mp = VP_TO_MNODE(vp);
	off_t kloff, baseoff;
	size_t kllen, diff;
	swaploc_t *bsp, *kbsp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT((off & PAGEOFFSET) == 0);
	ASSERT(len == PAGESIZE);
	ASSERT(flags == (B_PAGEOUT|B_ASYNC));
	ASSERT(off < mp->mno_bsize);

	/*
	 * Get backing store information.
	 */
	bsp = memfs_lookup_bs(mp, off);

	/*
	 * Since we do not hold MEM_T_LOCK(s) on any of the pages at this
	 * time, our judgement of klustering is just an approximation.
	 * However, since pageout() is the only agent allocating backing
	 * store, and since frees of backing store are expected to be rare,
	 * there is a very good chance that the backing store information
	 * will remain stable until the point that we get the MEM_T_LOCK(s)
	 * in routine memfs_doputpage().
	 *
	 * If the center page has no backing store allocated, then we try
	 * to extend the kluster to adjacent pages whick also have no backing
	 * store. If the center page has backing store allocated, then we try
	 * to extend the kluster to adjacent pages with adjacent backing
	 * store.
	 *
	 * First, scan for contiguous backing store (or lack thereof) for
	 * pages with vnode offsets less than the center page.
	 */
	kllen = PAGESIZE;
	kloff = off;
	kbsp = bsp - 1;
	while ((kloff & MEMFS_KLUSTER_OFF) != 0 &&
	       SWAPLOC_EQUAL(kbsp, kllen, bsp)) {

		kllen += PAGESIZE;
		kloff -= PAGESIZE;
		--kbsp;
	}

	/*
	 * Now, scan for continguous backing store (or lack thereof) for
	 * vnode offsets greater than the center page.
	 */
	baseoff = off + PAGESIZE;
	diff = PAGESIZE;
	kbsp = bsp + 1;
	while ((baseoff & MEMFS_KLUSTER_OFF) != 0 &&
	       SWAPLOC_EQUAL(bsp, diff, kbsp)) {
	
		baseoff += PAGESIZE;
		diff += PAGESIZE;
		++kbsp;
	}
	kllen += diff - PAGESIZE;

	/*
	 * The real work will be done by a combination of pvn_getdirty_range()
	 * and memfs_doputpage().
	 *
	 * The unlocked read of ``mp->mno_size'' is acceptible here because
	 * the filesize is only used as advice by pvn_getdirty_range().
	 * An accurate value is not required.
	 */
	return (pvn_getdirty_range(memfs_doputpage, vp, off, len, kloff, kllen,
				   mp->mno_size, flags, cr));
}

/*
 * STATIC int
 * memfs_doputpage(vnode_t *vp, page_t *dirty, int flags, cred_t *cr)
 *	workhorse for memfs_putpage
 *
 * Calling/Exit State:
 *	A list of dirty pages, prepared for I/O (in pageout state),
 *	is passed in dirty.  Other parameters are passed through from
 *	memfs_putpage.
 *
 *	PERF: the call to memfs_lookup_bs() could be eliminated by storing
 *	      a pointer to the swaploc_t in the page private data.
 */

/* ARGSUSED */
STATIC int
memfs_doputpage(vnode_t *vp, page_t *dirtyp, int flags, cred_t *cr)
{
	mnode_t *mp = VP_TO_MNODE(vp);
	off_t io_off;
	uint_t io_len;
	off_t swoff;
	swaploc_t *bsp, *kbsp;
	swaploc_t swaploc, swaploc2;
	page_t *kpp, *io_listp, *pp, *lpp;
	uint_t totpgs, pgs, pgs2;
	swapinfo_t *sip;
	boolean_t last_pass = B_FALSE;
#ifdef DEBUG
	off_t pg_offset;
#endif
#ifdef _MEMFS_HIST
	off_t sw_offset;
#endif

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(flags == (B_PAGEOUT|B_ASYNC));

	while (dirtyp != NULL) {
		/*
		 * Look to see how big a chunk with contiguous backing
		 * store, or with no backing store, we can pull off
		 * the dirty list.
		 *
		 * First, get backing store information.
		 */
		ASSERT(dirtyp->p_vnode == vp);
		ASSERT(dirtyp->p_pageout == 1);
		ASSERT(dirtyp->p_activecnt);
		io_off = dirtyp->p_offset;
		bsp = memfs_lookup_bs(mp, io_off);
		io_len = PAGESIZE;
		kbsp = bsp + 1;
		kpp = dirtyp->p_next;
#ifdef DEBUG
		pg_offset = dirtyp->p_offset;
#endif /* DEBUG */
#ifdef _MEMFS_HIST
		sw_offset = dirtyp->p_offset;
#endif /* _MEMFS_HIST */

		/*
		 * Now, look for additional pages to kluster together.
		 */
		while (kpp != dirtyp && SWAPLOC_EQUAL(bsp, io_len, kbsp) &&
		       dirtyp->p_offset + io_len == kpp->p_offset) {
			ASSERT(kpp->p_vnode == vp);
			ASSERT(kpp->p_pageout == 1);
			ASSERT(kpp->p_activecnt);
			io_len += PAGESIZE;
			kpp = kpp->p_next;
			++kbsp;
		}

		/*
		 * If no backing store is currently assigned, then we will
		 * try to allocate some now. This activity might fail if
		 * the swap space is fully allocated.
		 */
		totpgs = pgs = btop(io_len);
		if (!SWAPLOC_HAS_BACKING(bsp)) {
			/*
			 * See if we can allocate some swap space.
			 */
			(void) LOCK(&swap_lck, VM_SWAP_IPL);

			if (nswappgfree == 0) {
				UNLOCK(&swap_lck, PLBASE);
				if (swapdoubles)
					page_swapreclaim(B_TRUE);
				io_listp = NULL;
				while (pgs-- > 0) {
					pp = dirtyp;
					ASSERT(pp->p_vnode == vp);
					ASSERT(pp->p_pageout == 1);
					ASSERT(pp->p_activecnt);
					ASSERT(pp->p_offset == pg_offset);
#ifdef DEBUG
					pg_offset += PAGESIZE;
#endif
					page_sub(&dirtyp, pp);
					page_add(&io_listp, pp);
				}
				pvn_fail(io_listp, B_WRITE | flags);
				continue;
			}
			
			if (pgs > nswappgfree)
				pgs = nswappgfree;

			while (!(swap_alloc(pgs, &swaploc))) {
				ASSERT(pgs != 1);
				pgs = pgs / 2;
			}

			nswappgfree -= pgs;
			swapdoubles += pgs;

			io_len = ptob(pgs);

			/*
			 * Assign the backing store locations into the
			 * backing store tree. Note that we hold the
			 * swap lock through this activity for the
			 * benefit of memfs_relocate().
			 */
			pgs2 = pgs;
			kbsp = bsp;
			swaploc2 = swaploc;
			do {
				*kbsp = swaploc2;
				SWAPLOC_ADD_OFFSET(&swaploc2, PAGESIZE);
				MEMFS_LOG(mp, sw_offset, "allocated");
#ifdef _MEMFS_HIST
				sw_offset += PAGESIZE;
#endif /* _MEMFS_HIST */
				++kbsp;
			} while (--pgs2 != 0);

			UNLOCK(&swap_lck, PLBASE);
		} else {
			swaploc = *bsp;
		}

		/*
		 * Remove the group of pages from the dirtyp list and
		 * tack them onto the I/O list.
		 *
		 * Optimization: do not copy the list if all pages on the
		 *		 source list are going to be copied
		 */
		io_listp = dirtyp;
		if (pgs != totpgs || kpp != dirtyp) {
			ASSERT(dirtyp->p_vnode == vp);
			ASSERT(dirtyp->p_pageout == 1);
			ASSERT(dirtyp->p_activecnt);
			ASSERT(dirtyp->p_offset == pg_offset);
#ifdef DEBUG
			pg_offset += PAGESIZE;
#endif
			page_sub(&dirtyp, io_listp);
			while (--pgs != 0) {
				pp = dirtyp;
				page_sub(&dirtyp, pp);
				lpp = io_listp;
				page_add(&lpp, pp);
				ASSERT(pp->p_vnode == vp);
				ASSERT(pp->p_pageout == 1);
				ASSERT(pp->p_activecnt);
				ASSERT(pp->p_offset == pg_offset);
#ifdef DEBUG
				pg_offset += PAGESIZE;
#endif
			}
		} else {
			last_pass = B_TRUE;
		}

		/*
		 * Now, let the underlying file system do the I/O.
		 */
		sip = swaptab[SWAPLOC_TO_IDX(&swaploc)];
		ASSERT(sip != NULL);
		swoff = SWAPLOC_TO_OFF(&swaploc);
		(void) VOP_PUTPAGELIST(sip->si_stvp, swoff, io_listp,
				       sip->si_obj, flags, cr);	

		if (last_pass)
			break;
	}

	return (0);
}


/*
 * void
 * memfs_delabort_cleanup(void)
 *	Complete the inactivation of MEMFS_DELABORT mnodes.
 *
 * Calling/Exit State:
 *	Called at PLBASE with with no spin LOCKs held and returns that
 *	way.
 */

void
memfs_delabort_cleanup(void)
{
	mnode_header_t *markerp, *mnhp;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	/*
	 * allocate a list marker.
	 */
	markerp = kmem_alloc(sizeof(mnode_header_t), KM_SLEEP);
	markerp->mnh_flags = MEMFS_MARKER;

	(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
	mnhp = mnode_ianchor.mnh_nextp;

	while (memfs_ndelabort > 0) {
		/*
		 * Ignore markers.
		 */
		if (mnhp->mnh_flags & MEMFS_MARKER)
			mnhp = mnhp->mnh_nextp;
		else if (mnhp->mnh_flags & MEMFS_ANCHOR)
			break;
		else if (MNODEH_TO_MNODE(mnhp)->mno_flags & MEMFS_DELABORT) {
			/*
			 * We have found a delayed abort mnode. Complete its
			 * inactivation now.
			 */
			MNODE_LIST_INSERT(markerp, mnhp);
			MNODEH_TO_MNODE(mnhp)->mno_flags &= ~MEMFS_DELABORT;
			--memfs_ndelabort;
			UNLOCK(&memfs_list_lck, PLBASE);
			memfs_inactive_common(MNODEH_TO_MNODE(mnhp));

			/*
			 * Delete our marker.
			 */
			(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
			mnhp = markerp->mnh_nextp;
			MNODE_LIST_DELETE(markerp);
		} else 
		 	mnhp = mnhp->mnh_nextp;
	}
	UNLOCK(&memfs_list_lck, PLBASE);

	/*
	 * Free up the marker.
	 */
	kmem_free(markerp, sizeof(mnode_header_t));
}

/*
 * int
 * memfs_map(vnode_t *vp, off_t off, struct as *as, vaddr_t *addrp, *uint_t len,
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
memfs_map(vnode_t *vp, off_t off, struct as *as, vaddr_t *addrp, uint_t len,
	uint_t prot, uint_t maxprot, uint_t flags, cred_t *fcr)
{
	struct cred *cr = VCURRENTCRED(fcr);
	struct segvn_crargs vn_a;
	int error;
        mnode_t *mp = VP_TO_MNODE(vp);

	ASSERT(!(mp->mno_flags & MEMFS_UNNAMED));

	if (vp->v_flag & VNOMAP)
		return (ENOSYS);

	if (vp->v_type != VREG)
		return (ENODEV);

	if ((int)off < 0 || (int)(off + len) < 0)
		return (EINVAL);

 
	/*
	 * If file is being locked, disallow mapping.
	 */
	if (vp->v_filocks != NULL && MANDLOCK(vp, mp->mno_mode))
		return EAGAIN;

	RWSLEEP_WRLOCK(&mp->mno_rwlock, PRINOD);

	as_wrlock(as);

	if ((flags & MAP_FIXED) == 0) {
		map_addr(addrp, len, (off_t)off, 0);
		if (*addrp == NULL) {
			as_unlock(as);
			RWSLEEP_UNLOCK(&mp->mno_rwlock);
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
	RWSLEEP_UNLOCK(&mp->mno_rwlock);
	return error;
}

/*
 * int
 * memfs_addmap(vnode_t *vp, uint_t off, as_t *as, vaddr_t addr, uint_t len,
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
memfs_addmap(vnode_t *vp, uint_t off, struct as *as, vaddr_t addr, uint_t len,
	uint_t prot, uint_t maxprot, uint_t flags, cred_t *cr)
{
        mnode_t *mp = VP_TO_MNODE(vp);
	int error;

	if (vp->v_flag & VNOMAP)
		error = ENOSYS;
	else {
		error = 0;
		if ((mp->mno_flags & MEMFS_UNNAMED) == 0) {
			FSPIN_LOCK(&mp->mno_mutex);
			mp->mno_mapcnt += btopr(len);
			FSPIN_UNLOCK(&mp->mno_mutex);
		}
	}

	return (error);
}

/*
 * int
 * memfs_delmap(vnode_t *vp, uint_t off, struct as *as, vaddr_t addr, uint_t len,
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
 *	
 */
/*ARGSUSED*/
STATIC int
memfs_delmap(vnode_t *vp, uint_t off, struct as *as, vaddr_t addr, uint_t len,
	uint_t prot, uint_t maxprot, uint_t flags, cred_t *cr)
{
        mnode_t *mp = VP_TO_MNODE(vp);
	int error;

	if (vp->v_flag & VNOMAP)
		error = ENOSYS;
	else {
		error = 0;
		if ((mp->mno_flags & MEMFS_UNNAMED) == 0) {
			ASSERT(mp->mno_mapcnt >= 0);
			if (((flags & MAP_TYPE) == MAP_SHARED) &&
					(maxprot & PROT_WRITE)) {
				/*
				 * For shared writable mappings, make sure that
				 * the timestamps of the file are updated if
				 * any changes were made to this file through
				 * the mapping.  Call pvn_syncsdirty to see if
				 * the file has been modified, and then call
				 * memfs_timestamp to update the timestamps.
				 */
				pvn_syncsdirty(vp);
				memfs_timestamp(mp);
			}
			FSPIN_LOCK(&mp->mno_mutex);
			mp->mno_mapcnt -= btopr(len);
			FSPIN_UNLOCK(&mp->mno_mutex);
		}
	}
	return (error);
}

/*
 * boolean_t
 * memfs_extend(vnode_t *vp, size_t len)
 *      Increase the size of a memfs file.
 *
 * Calling/Exit State:
 *      The caller holds no mnode locks and no spin LOCKs on entry to
 *	this function. None are held upon return.
 *
 * Description:
 *	This special purpose interface is provided for ub_memfs_alloc(),
 *	which needs to circumvent the check against
 *	u.u_rlimits->rl_limits[RLIMIT_FSIZE]. This check cannot be made
 *	when executing in under p0_init.
 */

int
memfs_extend(vnode_t *vp, size_t len)
{
        mnode_t *mp = VP_TO_MNODE(vp);
	boolean_t ret = B_TRUE;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(len > mp->mno_size);
	ASSERT(!(mp->mno_flags & MEMFS_FIXEDSIZE));
	ASSERT(mp->mno_flags & MEMFS_UNNAMED);

	/*
	 * Acquire the rw lock in WRITEr mode to inhibit concurrent
	 * changes to the file.
	 */
        RWSLEEP_WRLOCK(&mp->mno_rwlock, PRINOD);

	if (len > LONG_MAX)
		ret = B_FALSE;
	else if (memfs_trunc_up(mp, len, B_FALSE) != 0)
		ret = B_FALSE;

	RWSLEEP_UNLOCK(&mp->mno_rwlock);
	return (ret);
}

#ifdef DEBUG
/*
 * void
 * memfs_truncate(vnode_t *vp, size_t len)
 *      Truncate the size of a memfs file.
 *
 * Calling/Exit State:
 *      The caller holds no mnode locks and no spin LOCKs on entry to
 *	this function. None are held upon return.
 *
 * Description:
 *	This special purpose interface is provided for flavors kvn tests.
 */

void
memfs_truncate(vnode_t *vp, size_t len)
{
        mnode_t *mp = VP_TO_MNODE(vp);
	boolean_t ret = B_TRUE;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(len <= mp->mno_size);
	ASSERT(!(mp->mno_flags & MEMFS_FIXEDSIZE));
	ASSERT(mp->mno_flags & MEMFS_UNNAMED);

	/*
	 * Acquire the rw lock in WRITEr mode to inhibit concurrent
	 * changes to the file.
	 */
        RWSLEEP_WRLOCK(&mp->mno_rwlock, PRINOD);

	memfs_trunc_down(mp, len);

	RWSLEEP_UNLOCK(&mp->mno_rwlock);
}

#endif

/*
 * STATIC int
 * memfs_frlock(vnode_t *vp, int cmd, struct flock *bfp, int flag, off_t offset,
 *	cred_t *cred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
/*ARGSUSED*/
STATIC int
memfs_frlock(vnode_t *vp, int cmd, struct flock *bfp, int flag, off_t offset,
	cred_t *cred)
{
	ASSERT(!(VP_TO_MNODE(vp)->mno_flags & MEMFS_UNNAMED));

	return (fs_frlock(vp, cmd, bfp, flag, offset, cred,
					(VP_TO_MNODE(vp))->mno_size));
}

/*
 * STATIC int
 * memfs_seek(vnode_t *vp, off_t ooff, off_t *noffp)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
/*ARGSUSED*/
STATIC int
memfs_seek(vnode_t *vp, off_t ooff, off_t *noffp)
{
	ASSERT(!(VP_TO_MNODE(vp)->mno_flags & MEMFS_UNNAMED));

	return (*noffp < 0 ? EINVAL : 0);
}

/*
 * STATIC int
 * memfs_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
/*ARGSUSED*/
STATIC int
memfs_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
{
	mnode_t *mp = VP_TO_MNODE(vp);

	ASSERT(!(mp->mno_flags & MEMFS_UNNAMED));

 	if (mode == LOCK_EXCL) {
 		RWSLEEP_WRLOCK(&mp->mno_rwlock, PRINOD);
 	} else if (mode == LOCK_SHARED) {
 		RWSLEEP_RDLOCK(&mp->mno_rwlock, PRINOD);
 	} else {
		/*
                 *+ An invalid mode was passed as a parameter to
                 *+ this routine. This indicates a kernel software
                 *+ problem
                 */
                cmn_err(CE_PANIC,"memfs_rwlock: invalid lock mode requested");
        }
        return 0;
}

/*
 * STATIC void
 * memfs_rwunlock(vnode_t *vp, off_t off, int len)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 */
/*ARGSUSED*/
STATIC void
memfs_rwunlock(vnode_t *vp, off_t off, int len)
{
	mnode_t *mp = VP_TO_MNODE(vp);

	ASSERT(!(mp->mno_flags & MEMFS_UNNAMED));

	RWSLEEP_UNLOCK(&mp->mno_rwlock);
}


/*
 * void
 * memfs_hashout(page_t *pp)
 *	Callout function from page_hashout informing memfs that one of
 *	its pages (pp) is being aborted.
 *
 * Calling/Exit State:
 *	Called with the page uselocked and returns that way. Since the
 *	page is being destroyed, the caller holds an effective MEM_T_LOCK
 *	on the backing store descriptor.
 *
 * Description:
 *	This function has two purposes:
 *		(1) To keep the swapdoubles counts in order, and
 *		(2) To mark IOERRs in backing store tree for the
 *		    swap delete case.
 */

void
memfs_hashout(page_t *pp)
{
	mnode_t *mp = VP_TO_MNODE(pp->p_vnode);
	swaploc_t *bsp;
	pl_t ospl;

	/*
	 * If this page still has a swap component, then we are throwing
	 * away its incore portion. Adjust swapdoubles to reflect this
	 * change.
	 *
	 * If the swap space is being deleted, then nobody is ever going to
	 * be able to read it again. We need to record this in the backing
	 * store tree, especially for the case were the abort was caused by
	 * an I/O error within memfs_relocate().
	 */
	bsp = memfs_lookup_bs(mp, pp->p_offset);

	if (SWAPLOC_HAS_BACKING(bsp)) {
		if (pp->p_invalid &&
		    (swaptab[SWAPLOC_TO_IDX(bsp)]->si_flags & ST_INDEL)) {
			swap_free(1, bsp, B_TRUE);
			SWAPLOC_MAKE_IOERR(bsp);
			MEMFS_LOG(mp, pp->p_offset, "memfs_hashout");
		} else {
			ospl = LOCK(&swap_lck, VM_SWAP_IPL);
			--swapdoubles;
			UNLOCK(&swap_lck, ospl);
		}
	}
}

/*
 * void
 * memfs_freeswap(page_t *pp)
 *	Free the swap space for memfs page pp.
 *
 * Calling/Exit State:
 *	The caller holds a PAGE_USELOCK on pp, and has verified that
 *	p_invalid and p_pageout are both clear. Thus, the caller holds
 *	an effective MEM_T_LOCK on the backing store descriptor.
 *
 * Description:
 *	Called by page_swapreclaim() when double association is being
 *	broken.
 *
 *	PERF: the call to memfs_lookup_bs() could be eliminated by storing
 *	      a pointer to the swaploc_t in the page private data.
 */

void
memfs_freeswap(page_t *pp)
{
	mnode_t *mp = VP_TO_MNODE(pp->p_vnode);
	swaploc_t *bsp = memfs_lookup_bs(mp, pp->p_offset);

	swap_free(1, bsp, B_TRUE);
	SWAPLOC_MAKE_EMPTY(bsp);
	MEMFS_LOG(mp, pp->p_offset, "memfs_freeswap");
}

/*
 * void
 * memfs_has_swap(page_t *pp)
 *	Determine if memfs page pp has backing store allocated.
 *
 * Calling/Exit State:
 *	The caller holds a PAGE_USELOCK on pp, and has verified that
 *	p_invalid and p_pageout are both clear. Thus, the caller holds
 *	an effective MEM_T_LOCK on the backing store descriptor.
 *
 * Description:
 *	Called by page_swapreclaim() to determine if double association
 *	needs to be broken.
 *
 *	PERF: the call to memfs_lookup_bs() could be eliminated by storing
 *	      a pointer to the swaploc_t in the page private data.
 */

boolean_t
memfs_has_swap(page_t *pp)
{
	mnode_t *mp = VP_TO_MNODE(pp->p_vnode);

	if (SWAPLOC_HAS_BACKING(memfs_lookup_bs(mp, pp->p_offset)))
		return (B_TRUE);
	
	return (B_FALSE);
}

/*
 * void
 * memfs_swapdel(uchar_t idx)
 *	Instruct memfs to stop using the swap device given by ``idx''.
 *
 * Calling/Exit State:
 *	Called at PLBASE with with no spin LOCKs held and returns that
 *	way.
 *
 * Description:
 *	Called by swapdel() when deleting a swap device.
 */

void
memfs_swapdel(uchar_t idx)
{
	mnode_header_t *markerp, *mnhp;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	/*
	 * allocate a list marker.
	 */
	markerp = kmem_alloc(sizeof(mnode_header_t), KM_SLEEP);
	markerp->mnh_flags = MEMFS_MARKER;

	/*
	 * Now relocate the swap space for all mnodes on the active
	 * mnode list.
	 */
	(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
	mnhp = mnode_anchor.mnh_nextp;
	for (;;) {
		/*
		 * Locate the next mnode to relocate. We need to skip
		 * over all markers on the mnode list.
		 *
		 * Note that any newly created mnodes will end up behind
		 * our marker on the active mnode list. Thus, we will ignore
		 * them. This is proper, since such mnodes cannot use swap
		 * space from a swap file which is being deleted.
		 */
		while (mnhp->mnh_flags & MEMFS_MARKER)
			mnhp = mnhp->mnh_nextp;

		/*
		 * If we see the anchor, then we are done with the active
		 * mnode list.
		 */
		if (mnhp->mnh_flags & MEMFS_ANCHOR)
			break;

		/*
		 * Insert the marker ahead of the mnode we are relocating.
		 */
		MNODE_LIST_INSERT(markerp, mnhp);

		/*
		 * Soft hold the vnode.
		 */
		VN_SOFTHOLD(MNODE_TO_VP(MNODEH_TO_MNODE(mnhp)));
		UNLOCK(&memfs_list_lck, PLBASE);

		/*
		 * Now, traverse the backing store tree, looking for
		 * swap space on swap file ``idx''.
		 */
		memfs_relocate(MNODEH_TO_MNODE(mnhp), idx);

		/*
		 * Release the vnode we are holding. Note that it may have
		 * already been deleted from the active mnode list.
		 * It is for exactly this reason that we have inserted a
		 * marker into the list.
		 */
		VN_SOFTRELE(MNODE_TO_VP(MNODEH_TO_MNODE(mnhp)));

		/*
		 * Resume the scan from the current marker position,
		 * deleting our marker from the list.
		 */
		(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
		mnhp = markerp->mnh_nextp;
		MNODE_LIST_DELETE(markerp);
	}

	/*
	 * Now, we wait for the inactive mnode list to drain down.
	 * We are only interested in mnodes which are inactive at this
	 * time. Any mnodes which become inactive at some later time
	 * have already been relocated, and so are of no concern.
	 */
	mnhp = mnode_ianchor.mnh_nextp;

	for (;;) {
		/*
		 * Ignore other markers.
		 */
		while (mnhp->mnh_flags & MEMFS_MARKER)
			mnhp = mnhp->mnh_nextp;

		/*
		 * If we see the anchor, then we are done with the inactive
		 * mnode list.
		 */
		if (mnhp->mnh_flags & MEMFS_ANCHOR)
			break;

		/*
		 * Insert our marker into the list.
		 */
		MNODE_LIST_INSERT_PREV(markerp, mnhp);

		/*
		 * If we see a delayed abort mnode, then complete its
		 * inactivation now. Else, wait for somebody to finish
		 * inactivating an mnode.
		 */
		if (MNODEH_TO_MNODE(mnhp)->mno_flags & MEMFS_DELABORT) {
			MNODEH_TO_MNODE(mnhp)->mno_flags &= ~MEMFS_DELABORT;
			--memfs_ndelabort;
			UNLOCK(&memfs_list_lck, PLBASE);
			memfs_inactive_common(MNODEH_TO_MNODE(mnhp));
		} else {
			SV_WAIT(&memfs_sv, PRINOD, &memfs_list_lck);
		}

		/*
		 * Delete our marker.
		 */
		(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
		mnhp = markerp->mnh_nextp;
		MNODE_LIST_DELETE(markerp);
	}
	UNLOCK(&memfs_list_lck, PLBASE);

	/*
	 * Free up the marker.
	 */
	kmem_free(markerp, sizeof(mnode_header_t));
}

/*
 * STATIC void
 * memfs_relocate(mnode_t *mp, uchar_t idx)
 *	Routine to release all swap space on the swap device given by ``idx''
 *	that might be held by the memfs file given by mp.
 *
 * Calling/Exit State:
 *	When this function returns, the the file will use no space on the
 *	specified swap device.
 *
 *	Called at PLBASE with no spin LOCKs held and returns that way.
 */

STATIC void
memfs_relocate(mnode_t *mp, uchar_t idx)
{
	swaploc_t *bsp;
	off_t off;
	size_t msize;
	uint_t vcount;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	MEMFS_LOG(mp, -1, "memfs_relocate-1");

	/*
	 * Now, force this mnode to relocate off of swap file ``idx''.
	 *
	 * First, grab the rwlock in READer mode in order to stabilize the
	 * backing store tree and the file size. Acquiring this lock also
	 * guarantees that all backing store beyond the end of file has
	 * been freed.
	 *
	 * Since we are only holding a VN_SOFTHOLD on the mnode's vnode,
	 * we do not inhibit the mnode from being inactivated. Since
	 * inactivation does not acquire the rwlock, it follows that
	 * there might be a racing truncation. But, in that case is of
	 * no concern, as the data is being discarded anyway.
	 */
	RWSLEEP_RDLOCK(&mp->mno_rwlock, PRINOD);

	/*
	 * Sample the file size.
	 */
	msize = mp->mno_bsize;

	/*
	 * Traverse the backing store tree.
	 *
	 * Note that the underlying storage for the backing store tree is
	 * protected by the VN_SOFTHOLD which our caller has acquired.
	 */
	for (off = 0; off < msize; ) {
		/*
		 * If the file is being inactivated, then our caller
		 * [memfs_swapdel()] will wait until the inactivation is
		 * complete, thus guaranteeing that the backing store is
		 * no longer in use.
		 */
		(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
		VN_LOCK(MNODE_TO_VP(mp));
		vcount = MNODE_TO_VP(mp)->v_count +
			 ((mp->mno_flags & MEMFS_UNNAMED) ? 0 : mp->mno_nlink);
		VN_UNLOCK(MNODE_TO_VP(mp));
		UNLOCK(&memfs_list_lck, PLBASE);
		if (vcount == 0) {
			MEMFS_LOG(mp, off, "memfs_relocate-2");
			break;
		}

		MEMFS_LOG(mp, off, "memfs_relocate-3");
		bsp = memfs_lookup_bs(mp, off);

		/*
		 * The following unlocked read(s) of *bsp are acceptable
		 * because:
		 *
		 * 1) memfs_relocate_page() is tolerant of the case where the
		 *    page is not actually on the specified swap file.
		 *
		 * 2) It is unacceptable to skip the call to
		 *    memfs_relocate_page() for a page which is on the
		 *    specified swap file.
		 *
		 * 3) The swap file specified by idx has been marked as
		 *    ST_INDEL by our caller swapdel(). swapdel() held the
		 *    swap_lck while it wrote the ST_INDEL flag.
		 *
		 * 4) The swap_lck is held in memfs_putpage(), protecting
		 *    both the swap allocation and the assignment of the
		 *    swap location to *bsp.
		 *
		 * 5) Consequently, anyone who is writing to *bsp must have
		 *    already freed the space, or must have observed that
		 *    the space was free and is allocating new space in
		 *    another swap file.
		 */
		if (SWAPLOC_IDX_EQUAL(bsp, idx))
			off = memfs_relocate_page(mp, off, idx, bsp);
		else
			off += PAGESIZE;
	}

	/*
	 * Release the rwlock.
	 */
	RWSLEEP_UNLOCK(&mp->mno_rwlock);
}

/*
 * STATIC off_t
 * memfs_relocate_page(mnode_t *mp, off_t off, uchar_t idx, swaploc_t *bsp)
 *	Relocate one page off of the swap file specified by idx.
 *
 * Calling/Exit State:
 *	The caller holds the rw lock on the mp. When this function returns,
 *	the page at off ``off'' will not be located on the specified swap
 *	device.
 *
 *	Called at PLBASE with no spin LOCKs held and returns that way.
 *
 *	Returns an the offset of the next page yet to be processed by
 *	memfs_relocate_page().
 */

STATIC off_t
memfs_relocate_page(mnode_t *mp, off_t off, uchar_t idx, swaploc_t *bsp)
{
	page_t *pl[MEMFS_KLUSTER_NUM + 1];
	vnode_t *vp = MNODE_TO_VP(mp);
	page_t *pp;
	page_t **ppp;
	off_t rtoff;
	int err;
	uint_t vcount;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	MEMFS_LOG(mp, off, "memfs_relocate_page-1");

	/*
	 * Get memfs_getpage to kluster in as many as MEMFS_KLUSTER_NUM
	 * pages. We make use of the dummy memfs_seg segment to facilitate
	 * this operation.
	 */
	err = VOP_GETPAGE(vp, off, PAGESIZE, NULL, pl, MEMFS_KLUSTER_SIZE,
			  &memfs_seg, off, S_READ, sys_cred);
	if (err) {
		(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
		VN_LOCK(vp);
		vcount = vp->v_count +
			 ((mp->mno_flags & MEMFS_UNNAMED) ? 0 : mp->mno_nlink);
		VN_UNLOCK(vp);
		UNLOCK(&memfs_list_lck, PLBASE);
		if (vcount == 0) {
			MEMFS_LOG(mp, off, "memfs_relocate_page-2");
			/*
			 * File is being or has been deleted.
			 */
			return NULL;
		}

		/*
		 * I/O error case: the backing store has been marked as such.
		 * All future VOP_GETPAGE() attempts to that page will
		 * fail with EIO.
		 *
		 * We would like to simply ASSERT that SWAPLOC_IS_IOERR(bsp).
		 * However, the swap space for the page might have
		 * reallocated before VOP_GETPAGE() obtained the MEM_T_LOCK
		 * lock.
		 */
		ASSERT(SWAPLOC_TO_IDX(bsp) != idx || SWAPLOC_IS_IOERR(bsp));
		MEMFS_LOG(mp, off, "memfs_relocate_page-2");
		return (off + PAGESIZE);
	}

	/*
	 * Since the entire kluster returned by memfs_getpage must have
	 * contiguous swap storage, it suffices to test the first page
	 * to determine if the file is using storage on the device being
	 * deleted.
	 */
	if (SWAPLOC_IDX_EQUAL(bsp, idx)) {
		ppp = pl;
		pp = *ppp;
		ASSERT(pp->p_offset == off);
		do {
			/*
			 * Wait for any in progress pageouts to complete.
			 * Note that we are actually obtaining a MEM_T_LOCK
			 * on the backing store info here. In addition,
			 * page_setmod_iowait() has the beneficial side
			 * effect of setting the page dirty bit (which is of
			 * course required).
			 *
			 * We would like to batch up multiple pages into the
			 * call to swap_free. However, that would require
			 * TRYLOCKing on the PAGE_USELOCK, something which
			 * is a bit inappropriate at this level.
			 */
			page_setmod_iowait(pp);
			PAGE_USELOCK(pp);
			swap_free(1, bsp, B_TRUE);
			ASSERT(bsp == memfs_lookup_bs(mp, pp->p_offset));
			SWAPLOC_MAKE_EMPTY(bsp);
			MEMFS_LOG(mp, pp->p_offset, "memfs_relocate_page-3");
			++bsp;
			PAGE_USEUNLOCK(pp);
			pp = *(++ppp);
		} while (pp != NULL);
	}

	/*
	 * Almost all done now. Just unlock the pages and return.
	 */
	ppp = pl;
	pp = *ppp;
	rtoff = pp->p_offset;
	do {
		page_unlock(pp);
		pp = *(++ppp);
	} while (pp != NULL);
	rtoff += ptob(ppp - pl);

	return (rtoff);
}
