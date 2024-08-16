/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/vncalls.c	1.78"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * System call routines for operations on files.  These manipulate
 * the global and per-process file table entries which refer to
 * vnodes, the system generic file abstraction.
 *
 * Many operations take a path name.  After preparing arguments, a
 * typical operation may proceed with:
 *
 *	error = lookupname(name, seg, followlink, &dvp, &vp);
 *
 * where "name" is the path name operated on, "seg" is UIO_USERSPACE
 * or UIO_SYSSPACE to indicate the address space in which the path
 * name resides, "followlink" specifies whether to follow symbolic
 * links, "dvp" is a pointer to a vnode for the directory containing
 * "name", and "vp" is a pointer to a vnode for "name".  (Both "dvp"
 * and "vp" are filled in by lookupname()).  "error" is zero for a
 * successful lookup, or a non-zero errno (from <sys/errno.h>) if an
 * error occurred.
 *
 * lookupname() fetches the path name string into an internal buffer
 * using pn_get() (pathname.c) and extracts each component of the path
 * by iterative application of the file system-specific VOP_LOOKUP
 * operation until the final vnode and/or its parent are found.
 * (There is provision for multiple-component lookup as well.)  If
 * either of the addresses for dvp or vp are NULL, lookupname() assumes
 * that the caller is not interested in that vnode.  Once a vnode has
 * been found, a vnode operation (e.g. VOP_OPEN, VOP_READ) may be
 * applied to it.
 *
 * With few exceptions (made only for reasons of backward compatibility)
 * operations on vnodes are atomic, so that in general vnodes are not
 * locked at this level, and vnode locking occurs at lower levels (either
 * locally, or, perhaps, on a remote machine.  (The exceptions make use
 * of the VOP_RWRDLOCK, VOP_RWWRLOCK, and VOP_RWUNLOCK operations, and
 * include VOP_READ, VOP_WRITE, and VOP_READDIR). The VOP_RWxxLOCK
 * operations were modified to take an offset, range pair and a filemode
 * to allow for finer-grained file locking on I/O requests. Each returns
 * when it is safe for the caller to perform I/O to "at least" the
 * affected range indicated in the call, i.e., the caller will not block
 * on any file/record locks for the affected range and none will appear
 * in the range until the caller releases the rwlock via VOP_RWUNLOCK.
 * This facility can be used to impose finer-grained locking by only
 * locking a portion of the file.  In addition, permission checking is
 * generally done by the specific filesystem, via its VOP_ACCESS
 * operation.  The upper (vnode) layer performs checks involving file
 * types (e.g. VREG, VDIR), since the type is static over the life of
 * the vnode.
 */

#include <acc/audit/audit.h>
#include <acc/audit/auditrec.h>
#include <acc/dac/acl.h>
#ifdef CC_PARTIAL
#include <acc/mac/cca.h>
#endif
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/filio.h>
#include <fs/fs_hier.h>
#include <fs/mode.h>
#include <fs/pathname.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/poll.h>
#include <io/ttold.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/exec.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/resource.h>
#include <proc/signal.h>
#include <proc/unistd.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/locking.h>
#include <svc/sco.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

extern	int	vn_open(char *, uio_seg_t, int, int, vnode_t **, create_t);
extern	int	vn_create(char *, uio_seg_t, vattr_t *, vcexcl_t, int,
			vnode_t **, create_t);
extern	int	vn_link(char *, char *, uio_seg_t);
extern	int	vn_rename(char *, char *, uio_seg_t);
extern	int	vn_remove(char *, uio_seg_t, rm_t);
extern	int	convoff(off_t *, flock_t *, int, off_t);
extern	int	spec_rdchk(vnode_t *, cred_t *, int *);
extern	int	fifo_rdchk(vnode_t *);

/*
 * int
 * copen(char *fname, int filemode, int createmode, rval_t *rvp)
 * 	Common code for open() and creat().  Check permissions, allocate
 * 	an open file structure, and call the device open routine (if any).
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EINVAL	if caller specifies neither FREAD nor FWRITE
 *		EINVAL	if the vnode type of the file to be dup'ed != VCHR
 */

STATIC int
copen(char *fname, int filemode, int createmode, rval_t *rvp)
{
	vnode_t	*vp;
	file_t	*fp;
	int	error;
	int	fd;
	int	dupfd;
	vtype_t	type;

	if ((filemode & (FREAD|FWRITE)) == 0)
		return EINVAL;

	if ((filemode & (FNONBLOCK|FNDELAY)) == (FNONBLOCK|FNDELAY))
		filemode &= ~FNDELAY;

	if (error = falloc((vnode_t *)NULL, filemode & FMASK, &fp, &fd))
		return error;
	
	/*
	 * Last arg is a don't-care term if !(filemode & FCREAT).
	 */
	error = vn_open(fname, UIO_USERSPACE, filemode,
	  (int)((createmode & MODEMASK) & ~u.u_procp->p_cmask), &vp, CRCREAT);
	if (error) {
		setf(fd, NULLFP);
		unfalloc(fp);
	} else if (vp->v_flag & VDUP) {
		/*
		 * The fd-entry is currently not bound. We need to simply
		 * shed the file table entry, and can keep the existing 
		 * fd-entry.
		 */
		unfalloc(fp);
		dupfd = getminor(vp->v_rdev);
		type = vp->v_type;
		vp->v_flag &= ~VDUP;
		VN_RELE(vp);
		if (type != VCHR)
			return EINVAL;
		if (error = getf(dupfd, &fp)) {
			setf(fd, NULLFP);	/* destroy fd-entry on error */
			return error;
		}
		setf(fd, fp);
		rvp->r_val1 = fd;
	} else {
		/* SCO Enhanced Application Compatibility Support */

		extern	int dev_autocad_major; 
		int	major;

		major = getmajor(vp->v_rdev);

		if(major == dev_autocad_major && vp->v_type == VCHR) {
			/*
			 *+ An open was attempted on a disabled driver.
			 */
			cmn_err(CE_WARN, "Trying to open a disabled driver");
			VN_RELE(vp);
			setf(fd, NULLFP);
			unfalloc(fp);
			return (EINVAL);
		}
		/* End  Enhanced Application Compatibility Support */

#ifdef CC_PARTIAL
		MAC_ASSERT (vp, MAC_DOMINATES);	/* open assumed correct */
#endif
		fp->f_vnode = vp;
		setf(fd, fp);			/* need to bind to fd-entry */
		rvp->r_val1 = fd;
	}

	return error;
}

struct opena {
	char	*fname;
	int	fmode;
	int	cmode;
};

/*
 * int
 * open(struct opena *uap, rval_t *rvp)
 *	Open a file.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

int
open(struct opena *uap, rval_t *rvp)
{
	ASSERT(KS_HOLD0LOCKS());
	return copen(uap->fname, (int)(uap->fmode-FOPEN), uap->cmode, rvp);
}

struct creata {
	char	*fname;
	int	cmode;
};

/*
 * int
 * creat(struct creata *uap, rval_t *rvp)
 *	Create a file.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

int
creat(struct creata *uap, rval_t *rvp)
{
	ASSERT(KS_HOLD0LOCKS());

	return copen(uap->fname, FWRITE|FCREAT|FTRUNC, uap->cmode, rvp);
}

struct closea {
	int	fdes;
};

/*
 * int
 * close(struct closea *uap, rval_t *rvp)
 *	Close a file.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
close(struct closea *uap, rval_t *rvp)
{
	ASSERT(KS_HOLD0LOCKS());
	return (closefd(uap->fdes));
}

/*
 * Structure for read/write/pread/pwrite.
 */
struct rwa {
	int	fdes;
	void	*cbuf;
	size_t	count;
	off_t	offset;
};

/*
 * ssize_t
 * read(struct rwa *uap, rval_t *rvp)
 *	Read from file.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	Hold inode's rwlock in *shared* mode when calling VOP_READ.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EBADF	if valid fd not open for reading or writing
 *		EINVAL	if uap->iovcnt <= 0
 *		EINTR	if a signal was caught during execution
 *
 * Description:
 * 	Check permissions; set base, count, and offset;
 * 	switch out to VOP_READ.
 */

ssize_t
read(struct rwa *uap, rval_t *rvp)
{
	vtype_t		type;
	file_t		*fp;
	int		count;
	int		error;
	int		ioflag;
	off_t		toffset;
	pl_t		pl;
	vnode_t		*vp;
	uio_t		auio;
	iovec_t		aiov;
	uio_t		*uio = &auio;

	ASSERT(KS_HOLD0LOCKS());
	MET_READ();
	error = getf_mhold(uap->fdes, &fp);
	if (error) {
		return (error);
	}
	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto error_exit;
	}
	vp = fp->f_vnode;
	aiov.iov_base = (caddr_t)uap->cbuf;
	count = uap->count;
	aiov.iov_len = count;
	uio->uio_resid = aiov.iov_len;
	uio->uio_iov = &aiov;
	uio->uio_iovcnt = 1;
	uio->uio_offset = fp->f_offset;
	toffset = uio->uio_offset;
	uio->uio_segflg = UIO_USERSPACE;
	uio->uio_fmode = fp->f_flag;

	ioflag = 0;
	if (fp->f_flag & FSYNC)
		ioflag |= IO_SYNC;
	error = VOP_RWRDLOCK(vp, toffset, count, uio->uio_fmode);
	if (error) {
		goto error_exit;
	}
	error = VOP_READ(vp, uio, ioflag, fp->f_cred);
	VOP_RWUNLOCK(vp, toffset, count);

	if (error == EINTR && uio->uio_resid != count) {
		error = 0;
	}
	rvp->r_val1 = count - uio->uio_resid;
	ldladd(&u.u_ioch, (ulong)rvp->r_val1);
	type = vp->v_type;
	if (type == VFIFO) {	/* Backward compatibility */
		fp->f_offset = rvp->r_val1;
	} else {
		fp->f_offset = uio->uio_offset;
	}
	MET_READCH( (unsigned)rvp->r_val1 );
	if (vp->v_vfsp != NULL) {
		vp->v_vfsp->vfs_bcount += rvp->r_val1 >> SCTRSHFT;
	}
error_exit:
	GETF_MRELE(fp);
	return (error);
}

/*
 * ssize_t
 * write(struct rwa *uap, rval_t *rvp)
 *	Write on a file.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	Hold inode's rwlock in *exclusive* mode when calling VOP_WRITE.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EBADF	if valid fd not open for reading or writing
 *		EINVAL	if uap->iovcnt <= 0
 *		EINTR	if a signal was caught during execution
 *
 * Description:
 * 	Check permissions; set base, count, and offset;
 * 	switch out to VOP_WRITE.
 */

ssize_t
write(struct rwa *uap, rval_t *rvp)
{
	vtype_t		type;
	file_t		*fp;
	int		count;
	int		error;
	int		ioflag;
	off_t		toffset;
	pl_t		pl;
	vnode_t		*vp;
	uio_t		auio;
	iovec_t		aiov;
	uio_t		*uio = &auio;

	ASSERT(KS_HOLD0LOCKS());
	MET_WRITE();
	error = getf_mhold(uap->fdes, &fp);
	if (error) {
		return (error);
	}
	if ((fp->f_flag & FWRITE) == 0) {
		error = EBADF;
		goto error_exit;
	}
	vp = fp->f_vnode;
	aiov.iov_base = (caddr_t)uap->cbuf;
	count = uap->count;
	aiov.iov_len = count;
	uio->uio_resid = aiov.iov_len;
	uio->uio_iov = &aiov;
	uio->uio_iovcnt = 1;
	uio->uio_offset = fp->f_offset;
	toffset = uio->uio_offset;
	uio->uio_segflg = UIO_USERSPACE;
	uio->uio_fmode = fp->f_flag;
	uio->uio_limit = u.u_rlimits->rl_limits[RLIMIT_FSIZE].rlim_cur;
	ioflag = 0;
	if (fp->f_flag & FAPPEND)
		ioflag |= IO_APPEND;
	if (fp->f_flag & FSYNC)
		ioflag |= IO_SYNC;
	error = VOP_RWWRLOCK(vp, toffset, count, uio->uio_fmode);
	if (error) {
		goto error_exit;
	}
#ifdef CC_PARTIAL
	MAC_ASSERT (vp, MAC_SAME); /* since file is open
		for writing, it's legal to assume that
		labels are same as process */
#endif
	error = VOP_WRITE(vp, uio, ioflag, fp->f_cred);
	VOP_RWUNLOCK(vp, toffset, count);

	if (error == EINTR && uio->uio_resid != count) {
		error = 0;
	}
	rvp->r_val1 = count - uio->uio_resid;
	ldladd(&u.u_ioch, (ulong)rvp->r_val1);
	type = vp->v_type;
	if (type == VFIFO) {	/* Backward compatibility */
		fp->f_offset = rvp->r_val1;
	} else if (((fp->f_flag & FAPPEND) == 0) || type != VREG
		  || uap->count != 0) {  /* POSIX-append with 0 bytes */
		fp->f_offset = uio->uio_offset;
	}
	MET_WRITECH( (unsigned)rvp->r_val1 );
	if (vp->v_vfsp != NULL) {
		vp->v_vfsp->vfs_bcount += rvp->r_val1 >> SCTRSHFT;
	}
error_exit:
	GETF_MRELE(fp);
	return (error);
}

/*
 * ssize_t
 * pread(struct rwa *uap, rval_t *rvp)
 *	Atomic position and read from file.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	Hold inode's rwlock in *shared* mode when calling VOP_READ.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EBADF	if valid fd not open for reading or writing
 *		EINVAL	if uap->iovcnt <= 0
 *		EINTR	if a signal was caught during execution
 *		ESPIPE	if cmd is pread/pwrite and file is a pipe
 *
 * Description:
 * 	Check permissions; set base, count, and offset;
 * 	switch out to VOP_READ.
 */

ssize_t
pread(struct rwa *uap, rval_t *rvp)
{
	vtype_t		type;
	file_t		*fp;
	int		count;
	int		error;
	int		ioflag;
	off_t		toffset;
	pl_t		pl;
	vnode_t		*vp;
	uio_t		auio;
	iovec_t		aiov;
	uio_t		*uio = &auio;

	ASSERT(KS_HOLD0LOCKS());
	MET_READ();
	error = getf_mhold(uap->fdes, &fp);
	if (error) {
		return (error);
	}
	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto error_exit;
	}
	vp = fp->f_vnode;
	type = vp->v_type;
	if (type == VFIFO) {
		error = ESPIPE;
		goto error_exit;
	}
	aiov.iov_base = (caddr_t)uap->cbuf;
	count = uap->count;
	aiov.iov_len = count;
	uio->uio_resid = aiov.iov_len;
	uio->uio_iov = &aiov;
	uio->uio_iovcnt = 1;
	uio->uio_offset = uap->offset;
	toffset = uio->uio_offset;
	uio->uio_segflg = UIO_USERSPACE;
	uio->uio_fmode = fp->f_flag;
	ioflag = 0;
	if (fp->f_flag & FSYNC)
		ioflag |= IO_SYNC;
	error = VOP_RWRDLOCK(vp, toffset, count, uio->uio_fmode);
	if (error) {
		goto error_exit;
	}
	error = VOP_READ(vp, uio, ioflag, fp->f_cred);
	VOP_RWUNLOCK(vp, toffset, count);

	if (error == EINTR && uio->uio_resid != count) {
		error = 0;
	}
	rvp->r_val1 = count - uio->uio_resid;
	ldladd(&u.u_ioch, (ulong)rvp->r_val1);
	MET_READCH( (unsigned)rvp->r_val1 );
	if (vp->v_vfsp != NULL) {
		vp->v_vfsp->vfs_bcount += rvp->r_val1 >> SCTRSHFT;
	}
error_exit:
	GETF_MRELE(fp);
	return (error);
}

/*
 * ssize_t
 * pwrite(struct rwa *uap, rval_t *rvp)
 *	Atomic position and write to file.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	Hold inode's rwlock in *exclusive* mode when calling VOP_WRITE.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EBADF	if valid fd not open for reading or writing
 *		EINVAL	if uap->iovcnt <= 0
 *		EINTR	if a signal was caught during execution
 *		ESPIPE	if cmd is pread/pwrite and file is a pipe
 *
 * Description:
 * 	Check permissions; set base, count, and offset;
 * 	switch out to VOP_WRITE.
 */

ssize_t
pwrite(struct rwa *uap, rval_t *rvp)
{
	vtype_t		type;
	file_t		*fp;
	int		count;
	int		error;
	int		ioflag;
	off_t		toffset;
	pl_t		pl;
	vnode_t		*vp;
	uio_t		auio;
	iovec_t		aiov;
	uio_t		*uio = &auio;

	ASSERT(KS_HOLD0LOCKS());
	MET_WRITE();
	error = getf_mhold(uap->fdes, &fp);
	if (error) {
		return (error);
	}
	if ((fp->f_flag & FWRITE) == 0) {
		error = EBADF;
		goto error_exit;
	}
	vp = fp->f_vnode;
	type = vp->v_type;
	if (type == VFIFO) {
		error = ESPIPE;
		goto error_exit;
	}
	aiov.iov_base = (caddr_t)uap->cbuf;
	count = uap->count;
	aiov.iov_len = count;
	uio->uio_resid = aiov.iov_len;
	uio->uio_iov = &aiov;
	uio->uio_iovcnt = 1;
	uio->uio_offset = uap->offset;
	toffset = uio->uio_offset;
	uio->uio_segflg = UIO_USERSPACE;
	uio->uio_fmode = fp->f_flag;
	uio->uio_limit = u.u_rlimits->rl_limits[RLIMIT_FSIZE].rlim_cur;
	ioflag = 0;
	if (fp->f_flag & FAPPEND)
		ioflag |= IO_APPEND;
	if (fp->f_flag & FSYNC)
		ioflag |= IO_SYNC;
	error = VOP_RWWRLOCK(vp, toffset, count, uio->uio_fmode);
	if (error) {
		goto error_exit;
	}
#ifdef CC_PARTIAL
	MAC_ASSERT (vp, MAC_SAME); /* since file is open
		for writing, it's legal to assume that
		labels are same as process */
#endif
	error = VOP_WRITE(vp, uio, ioflag, fp->f_cred);
	VOP_RWUNLOCK(vp, toffset, count);

	if (error == EINTR && uio->uio_resid != count) {
		error = 0;
	}
	rvp->r_val1 = count - uio->uio_resid;
	ldladd(&u.u_ioch, (ulong)rvp->r_val1);
	MET_WRITECH( (unsigned)rvp->r_val1 );
	if (vp->v_vfsp != NULL) {
		vp->v_vfsp->vfs_bcount += rvp->r_val1 >> SCTRSHFT;
	}
error_exit:
	GETF_MRELE(fp);
	return (error);
}

/*
 * Readv and writev structure.
 */
struct rwva {
	int	fdes;
	iovec_t	*iovp;
	int	iovcnt;
};

#define	MAXIOVCNT	16

/*
 * ssize_t
 * rwv(struct rwva *uap, rval_t *rvp, int mode)
 *	Common code for readv/writev calls.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EBADF	if valid fd not open for reading or writing
 *		EINVAL	if uap->iovcnt <= 0
 *		EFAULT	for an error copying to kernel address space
 *
 * Description:
 * 	Check permissions; set base, count, and offset;
 * 	switch out to VOP_READ or VOP_WRITE operations.
 */

STATIC ssize_t
rwv(struct rwva *uap, rval_t *rvp, int mode)
{
	vtype_t	 type;
	file_t	 *fp;
	int	 count;
	int	 error;
	int	 i;
	int	 ioflag;
	iovec_t	 *iovp;
	dl_t	 dlong;
	off_t	 toffset;
	pl_t	 pl;
	vnode_t	 *vp;
	uio_t	 auio;
	iovec_t	 aiov[MAXIOVCNT];
	uio_t	 *uio = &auio;

	ASSERT(KS_HOLD0LOCKS());
	if (mode == FREAD) {
		MET_READ();
	} else {
		MET_WRITE();
	}
	error = getf_mhold(uap->fdes, &fp);
	if (error) {
		return (error);
	}
	if ((fp->f_flag & mode) == 0) {
		error = EBADF;
		goto error_exit;
	}
	if (uap->iovcnt <= 0 || uap->iovcnt > sizeof(aiov)/sizeof(aiov[0])) {
		error = EINVAL;
		goto error_exit;
	}
	auio.uio_iov = aiov;
	auio.uio_iovcnt = uap->iovcnt;
	if (copyin((caddr_t)uap->iovp, (caddr_t)aiov,
	  (unsigned)(uap->iovcnt * sizeof(iovec_t)))) {
		error = EFAULT;
		goto error_exit;
	}
	uio->uio_resid = 0;
	iovp = uio->uio_iov;
	for (i = 0; i < uio->uio_iovcnt; i++) {
		if (iovp->iov_len < 0) {
			error = EINVAL;
			goto error_exit;
		}
		uio->uio_resid += iovp->iov_len;
		if (uio->uio_resid < 0) {
			error = EINVAL;
			goto error_exit;
		}
		iovp++;
	}
	vp = fp->f_vnode;
	type = vp->v_type;
	count = uio->uio_resid;
	uio->uio_offset = fp->f_offset;
	uio->uio_segflg = UIO_USERSPACE;
	uio->uio_fmode = fp->f_flag;
	uio->uio_limit = u.u_rlimits->rl_limits[RLIMIT_FSIZE].rlim_cur;
	toffset = uio->uio_offset;
	ioflag = 0;
	if (fp->f_flag & FAPPEND)
		ioflag |= IO_APPEND;
	if (fp->f_flag & FSYNC)
		ioflag |= IO_SYNC;

	if (mode == FREAD) {
		error = VOP_RWRDLOCK(vp, toffset, count, uio->uio_fmode);
		if (error) {
			goto error_exit;
		}
		error = VOP_READ(vp, uio, ioflag, fp->f_cred);
	} else {
		error = VOP_RWWRLOCK(vp, toffset, count, uio->uio_fmode);
		if (error) {
			goto error_exit;
		}
#ifdef CC_PARTIAL
		MAC_ASSERT (vp, MAC_SAME); /* since file is open
			for writing, it's legal to assume that
			labels are same as process */
#endif
		error = VOP_WRITE(vp, uio, ioflag, fp->f_cred);
	}
	VOP_RWUNLOCK(vp, toffset, count);

	if (error == EINTR && uio->uio_resid != count) {
		error = 0;
	}
	rvp->r_val1 = count - uio->uio_resid;
	dlong.dl_hop = 0L;
	dlong.dl_lop = (unsigned)rvp->r_val1;
	u.u_ioch = ladd(u.u_ioch, dlong);
	if (type == VFIFO) {	/* Backward compatibility */
		fp->f_offset = rvp->r_val1;
	} else {
		fp->f_offset = uio->uio_offset;
	}
	if (mode == FREAD) {
		MET_READCH( (unsigned)rvp->r_val1 );
	} else {
		MET_WRITECH( (unsigned)rvp->r_val1 );
	}
	if (vp->v_vfsp != NULL) {
		vp->v_vfsp->vfs_bcount += rvp->r_val1 >> SCTRSHFT;
	}
error_exit:
	GETF_MRELE(fp);
	return (error);
}

/*
 * ssize_t
 * readv(struct rwva *uap, rval_t *rvp)
 *	Read from file and place the input data into iovcnt buffers
 *	specified by members of the iovp array.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

ssize_t
readv(struct rwva *uap, rval_t *rvp)
{
	return rwv(uap, rvp, FREAD);
}

/*
 * ssize_t
 * writev(struct rwva *uap, rval_t *rvp)
 *	Write to a file gathering the output data from the iovcnt
 *	buffers specified by members of the iovp array.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

ssize_t
writev(struct rwva *uap, rval_t *rvp)
{
	return rwv(uap, rvp, FWRITE);
}

/*
 * int
 * chdirec(vnode_t *vp)
 *	Update vnode pointer.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 * Description:
 * 	Chdirec() takes as an argument a vnode pointer. If the vnode
 * 	passed in corresponds to a directory for which the user has
 * 	execute permission, then the process' current working directory
 * 	is updated to point to the vnode passed in.
 */

STATIC int
chdirec(vnode_t *vp)
{
	int	error;
	pl_t	pl;
	vnode_t	*oldcvp;
	proc_t *pp;

	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto bad;
	}
	/*
         * Must have MAC search access to the directory.
	 */
	if ((error = MAC_VACCESS(vp, VEXEC, CRED())) != 0)
		goto bad;

#ifdef CC_PARTIAL
	MAC_ASSERT (vp, MAC_DOMINATES);	/* search access implies process
					   must dominate */
#endif
	if (error = VOP_ACCESS(vp, VEXEC, 0, CRED()))
		goto bad;
	pp = u.u_procp;
	/* adt_pathupdate updates p_cdir also */
	if (u.u_lwpp->l_auditp) 
		adt_pathupdate(pp, vp, &oldcvp, NULLVPP, CDIR);
	else {
		pl = CUR_ROOT_DIR_LOCK(pp);
		oldcvp = pp->p_cdir;
		pp->p_cdir = vp;
		CUR_ROOT_DIR_UNLOCK(pp, pl);
	}
	/*
	 * Without holding p_dir_mutex, release the reference on the 
	 * old current directory.
	 */
	VN_RELE(oldcvp);
	return 0;

bad:
	VN_RELE(vp);
	return error;
}

struct chdira {
	char *fname;
};

/*
 * int
 * chdir(struct chdira *uap, rval_t *rvp)
 * 	Change current working directory (".").
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
chdir(struct chdira *uap, rval_t *rvp)
{
	vnode_t	*vp;
	int	error;

	ASSERT(KS_HOLD0LOCKS());
	error = lookupname(uap->fname, UIO_USERSPACE, FOLLOW, NULLVPP, &vp);
	if (error) {
		return error;
	}

	error = chdirec(vp);
	ADT_FREEPATHS(u.u_lwpp->l_auditp);
	return error;
}

struct fchdira {
	int  fd; 
};

/*
 * int
 * fchdir(struct fchdira *uap, rval_t *rvp)
 * 	File-descriptor based version of 'chdir'.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
fchdir(struct fchdira *uap, rval_t *rvp)
{
	file_t	*fp;
	vnode_t	*vp;
	int	error;
	
	ASSERT(KS_HOLD0LOCKS());
	if (error = getf_mhold(uap->fd, &fp))
		return error;
	vp = fp->f_vnode;
	VN_HOLD(vp);
	error = chdirec(vp);
	GETF_MRELE(fp);
	return error;
}

/*
 * int
 * chroot(struct chdira *uap, rval_t *rvp)
 * 	Change notion of root ("/") directory.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
chroot(struct chdira *uap, rval_t *rvp)
{
	int 	error;
	pl_t	pl;
	proc_t *pp;
	lwp_t  *lwpp;
	vnode_t *vp;
	vnode_t	*olwp_rdir;
	vnode_t	*oproc_rdir;

	if (pm_denied(CRED(), P_FILESYS))
		return EPERM;
	ASSERT(KS_HOLD0LOCKS());
	if (error = lookupname(uap->fname, UIO_USERSPACE, 
	    FOLLOW, NULLVPP, &vp))
		return error;
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto bad;
	}
	/*
         * Must have MAC search access to the directory.
	 */
	if ((error = MAC_VACCESS(vp, VEXEC, CRED())) != 0)
		goto bad;

#ifdef CC_PARTIAL
	MAC_ASSERT (vp, MAC_DOMINATES);	/* search access implies process
					   must dominate */
#endif
	if (error = VOP_ACCESS(vp, VEXEC, 0, CRED()))
		goto bad;
	VN_HOLD(vp);
	pp = u.u_procp;
	lwpp = u.u_lwpp;

	/* adt_pathupdate updates l_rdir and p_rdir directories also */
	if (lwpp->l_auditp) 
		adt_pathupdate(pp, vp, &olwp_rdir, &oproc_rdir, RDIR);
	else {
		pl = CUR_ROOT_DIR_LOCK(pp);
		olwp_rdir = lwpp->l_rdir;
		oproc_rdir = pp->p_rdir;
		lwpp->l_rdir = vp;
		pp->p_rdir = vp;
		CUR_ROOT_DIR_UNLOCK(pp, pl);
	}

	if (!SINGLE_THREADED()) {	/* other LWPs to be notified? */
		pl = LOCK(&pp->p_mutex, PLHI);
		/*
		 * Notify all other LWPs in the process that a new root
		 * directory is in effect for the process.
		 */
		trapevproc(pp, EVF_PL_RDIR, B_FALSE);
		UNLOCK(&pp->p_mutex, pl);
	}
	/*
	 * Release the references to the old LWP and process root directory.
	 */
	if (olwp_rdir == oproc_rdir) {
		if(olwp_rdir) {
			VN_RELEN(olwp_rdir, 2);
		}
	} else {
		if(olwp_rdir) {
			VN_RELE(olwp_rdir);
		}
		if(oproc_rdir) {
			VN_RELE(oproc_rdir);
		}
	}
	ADT_FREEPATHS(u.u_lwpp->l_auditp);
	return 0;

bad:
	VN_RELE(vp);
	ADT_FREEPATHS(u.u_lwpp->l_auditp);
	return error;
}

/*
 * int
 * cmknod(int version, char *fname, mode_t fmode, dev_t dev, rval_t *rvp)
 *	Common code for mknod() and xmknod().  Check permissions, set up
 *	desired attributes, get a device and call vn_create().
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *      A return value of 0 indicates success ; otherwise a valid errno
 *      is returned. Errnos returned directly by this routine are:
 *              EPERM  if a FIFO node is not being created or user does
 *			not have P_FILESYS privilege
 *              EINVAL if dev is not a character or block I/O device or
 *			major number associated with that device is invalid
 */

/* ARGSUSED */
STATIC int
cmknod(int version, char *fname, mode_t fmode, dev_t dev, rval_t *rvp)
{
	extern major_t maxmajor;
	extern minor_t maxminor;
	extern major_t udev_emajor;
	extern uint_t udev_nmajors;
	vnode_t	*vp;
	vattr_t	vattr;
	int	error;

	/*
	 * Zero type is equivalent to a regular file.
	 */
	if ((fmode & S_IFMT) == 0)
		fmode |= S_IFREG;

	/*
	 * Must have P_FILESYS privilege unless making a FIFO node.
	 */
	if (((fmode & S_IFMT) != S_IFIFO) 
	/* XENIX Support */
	  && ((fmode & S_IFMT) != S_IFNAM)	/* XENIX Support */
	/* End XENIX Support */
	  && pm_denied(CRED(), P_FILESYS))
		return EPERM;
	/*
	 * Set up desired attributes and vn_create the file.
	 */
	vattr.va_type = IFTOVT(fmode);
	if (vattr.va_type == VNON)
		return(EINVAL);
	vattr.va_mode = (fmode & MODEMASK) & ~u.u_procp->p_cmask;
	vattr.va_mask = AT_TYPE|AT_MODE;
	if (vattr.va_type == VCHR || vattr.va_type == VBLK
	  || vattr.va_type == VXNAM) {
		if (version == _MKNOD_VER && vattr.va_type != VXNAM) {
			if (dev == NODEV || getemajor(dev) == (major_t)NODEV)
				return EINVAL;
		} else {
			/* dev is in old format */
			if ((o_dev_t)dev == O_NODEV ||
			    o_getemajor(dev) == (major_t)NODEV)
				return EINVAL;

			dev = expdev((o_dev_t)dev);
		}
		if (getemajor(dev) >= udev_emajor &&
		    getemajor(dev) < udev_emajor + udev_nmajors)
			dev = udev_getrdev(dev);

		/*
		 * Enforce MAXMAJOR and MAXMINOR tunables, to restrict
		 * device number usage in case of unusual drivers or
		 * applications.
		 */
		if (getemajor(dev) > maxmajor ||
		    geteminor(dev) > maxminor)
			return EOVERFLOW;

		vattr.va_rdev = dev;
		vattr.va_mask |= AT_RDEV;
	}
	if ((error = vn_create(fname, UIO_USERSPACE,
	  &vattr, EXCL, 0, &vp, CRMKNOD)) == 0)
		VN_RELE(vp);
	return error;
}


/* SVR3 mknod arg */
struct mknoda {
	char	*fname;		/* pathname passed by user */
	mode_t	fmode;		/* mode of pathname */
	dev_t	dev;		/* device number - b/c specials only */
};

/*
 * int
 * mknod(struct mknoda *uap, rval_t *rvp)
 *	Create a special file, a regular file, or a directory.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

int
mknod(struct mknoda *uap, rval_t *rvp)
{
	ASSERT(KS_HOLD0LOCKS());

	return cmknod(_R3_MKNOD_VER, uap->fname, uap->fmode, uap->dev, rvp);
}

struct xmknoda {
	int	version;	/* version of this syscall */
	char	*fname;		/* pathname passed by user */
	mode_t	fmode;		/* mode of pathname */
	dev_t	dev;		/* device number - b/c specials only */
};

/*
 * int
 * xmknod(struct xmknoda *uap, rval_t *rvp)
 *	Expanded mknod.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

int
xmknod(struct xmknoda *uap, rval_t *rvp)
{
	ASSERT(KS_HOLD0LOCKS());

	return cmknod(uap->version, uap->fname, uap->fmode, uap->dev, rvp);
}


struct mkdira {
	char *dname;
	int dmode;
};

/*
 * int
 * mkdir(struct mkdira *uap, rval_t *rvp)
 *	Make a directory.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *	Preserve runtime attributes (sticky bit).  MODEMASK is used instead.
 */

/* ARGSUSED */
int
mkdir(struct mkdira *uap, rval_t *rvp)
{
	vnode_t	*vp;
	vattr_t	vattr;
	int	error;

	ASSERT(KS_HOLD0LOCKS());

	vattr.va_type = VDIR;
	vattr.va_mode = (uap->dmode & MODEMASK) & ~u.u_procp->p_cmask;
	vattr.va_mask = AT_TYPE|AT_MODE;
	if ((error = vn_create(uap->dname, UIO_USERSPACE, &vattr,
	  EXCL, 0, &vp, CRMKDIR)) == 0)
		VN_RELE(vp);
	return error;
}

struct linka {
	char	*from;
	char	*to;
};

/*
 * int
 * link(struct linka *uap, rval_t *rvp)
 *	Make a hard link.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
link(struct linka *uap, rval_t *rvp)
{

	ASSERT(KS_HOLD0LOCKS());

	return vn_link(uap->from, uap->to, UIO_USERSPACE);
}

/*
 * Rename or move an existing file.
 */
struct renamea {
	char	*from;
	char	*to;
};

/*
 * int
 * rename(struct renamea *uap, rval_t *rvp)
 *	Rename or move an existing file.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
rename(struct renamea *uap, rval_t *rvp)
{
	ASSERT(KS_HOLD0LOCKS());

	return vn_rename(uap->from, uap->to, UIO_USERSPACE);
}

struct symlinka {
	char	*target;
	char	*linkname;
};

/*
 * int
 * symlink(struct symlinka *uap, rval_t *rvp)
 *	Create a symbolic link.  Similar to link or rename except target
 *	name is passed as string argument, not converted to vnode reference.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	A return value of 0 indicated success; otherwise a valid errno is
 *	returned. Errnos returned directly by this routine are:
 *		EROFS if (dvp->v_vfsp->vfs_flag & VFS_RDONLY)
 *		EACCES if no write access on the parent directory of linkname. 
 */

/* ARGSUSED */
int
symlink(struct symlinka *uap, rval_t *rvp)
{
	vnode_t 	*dvp;
	vattr_t		vattr;
	pathname_t	tpn;
	pathname_t	lpn;
	int		error;
	char            *component;
	size_t          complen,tpnsize;
	char            *tpnbuf;

	if (error = pn_get(uap->linkname, UIO_USERSPACE, &lpn))
		return error;
	if (error = lookuppn(&lpn, NO_FOLLOW, &dvp, NULLVPP)) {
		pn_free(&lpn);
		return error;
	}
	if (dvp->v_vfsp->vfs_flag & VFS_RDONLY) {
		error = EROFS;
		goto error_free1;
	}
	if (error = pn_get(uap->target, UIO_USERSPACE, &tpn))
		goto error_free1;

	if ((error = MAC_CHECKS(dvp, CRED())) != 0)
		goto error_free2;

        /* save the target path name before checking
         * each of it's component length
         */
	tpnsize = tpn.pn_pathlen + 1;
	tpnbuf = kmem_alloc(tpnsize, KM_SLEEP);
	strcpy(tpnbuf, tpn.pn_path);
	while (tpn.pn_pathlen > 0) {
		if (pn_peekchar(&tpn) == '/')
			pn_skipslash(&tpn);

		component = pn_getcomponent(&tpn, &complen);
		if (component == NULL) {
			error = ENAMETOOLONG;
			kmem_free(tpnbuf, tpnsize);
			goto error_free2;
		}
	}

	vattr.va_type = VLNK;
	vattr.va_mode = 0777;
	vattr.va_mask = AT_TYPE|AT_MODE;
	error = VOP_SYMLINK(dvp, lpn.pn_path, &vattr, tpnbuf, CRED());
	kmem_free(tpnbuf, tpnsize);

error_free2:
	pn_free(&tpn);
error_free1:
	ADT_SYMLINK(dvp, &lpn, uap->target, error);
	pn_free(&lpn);
	VN_RELE(dvp);
	return error;
}

struct unlinka {
	char	*fname;
};

/*
 * int
 * unlink(struct unlinka *uap, rval_t *rvp)
 *	Unlink (i.e. delete) a file.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
unlink(struct unlinka *uap, rval_t *rvp)
{
	ASSERT(KS_HOLD0LOCKS());

	return vn_remove(uap->fname, UIO_USERSPACE, RMFILE);
}

struct rmdira {
	char *dname;
};

/*
 * int
 * rmdir(struct rmdira *uap, rval_t *rvp)
 *	Remove a directory.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
rmdir(struct rmdira *uap, rval_t *rvp)
{
	ASSERT(KS_HOLD0LOCKS());

	return vn_remove(uap->dname, UIO_USERSPACE, RMDIRECTORY);
}

struct getdentsa {
	int fd;
	char *buf;
	int count;
};

/*
 * int
 * getdents(struct getdentsa *uap, rval_t *rvp)
 * 	Get directory entries in a fs-independent format.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	Hold inode's rwlock in *shared* mode while calling VOP_READDIR.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		ENOTDIR	if vnode type is not VDIR (not a directory)
 */

int
getdents(struct getdentsa *uap, rval_t *rvp)
{
	vnode_t	*vp;
	file_t	*fp;
	uio_t	auio;
	iovec_t	aiov;
	int	error;
	int	sink;
	int	tcount;		/* save count value */
	off_t	toffset;	/* save offset value */

	ASSERT(KS_HOLD0LOCKS());
	if (error = getf_mhold(uap->fd, &fp))
		return error;
	vp = fp->f_vnode;
	if (vp->v_type != VDIR) {
		error = ENOTDIR;
		goto error_exit;
	}
	aiov.iov_base = uap->buf;
	aiov.iov_len = tcount = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = toffset = fp->f_offset;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_resid = uap->count;
	error = VOP_RWRDLOCK(vp, toffset, tcount, fp->f_flag);
	if (error) {
		goto error_exit;
	}
	error = VOP_READDIR(vp, &auio, fp->f_cred, &sink);
	VOP_RWUNLOCK(vp, toffset, tcount);
	if (!error) {
		rvp->r_val1 = uap->count - auio.uio_resid;
		fp->f_offset = auio.uio_offset;
	}

error_exit:
	GETF_MRELE(fp);
	return (error);
}

struct lseeka {
	int	fdes;
	off_t	off;
	int	sbase;
};

/*
 * int
 * lseek(struct lseeka *uap, rval_t *rvp)
 * 	Move read/write file pointer.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EINVAL	if uap->sbase is not 0 (SEEK_SET), 1 (SEEK_CUR),
 *					  or 2 (SEEK_END).
 */
int
lseek(struct lseeka *uap, rval_t *rvp)
{
	file_t	*fp;
	vnode_t	*vp;
	vattr_t	vattr;
	int	error;

	ASSERT(KS_HOLD0LOCKS());
	if (error = getf_mhold(uap->fdes, &fp))
		return error;
	vp = fp->f_vnode;
	switch (uap->sbase) {
	case 0:
		break;
	case 1:
		uap->off += fp->f_offset;
		break;
	case 2:
		vattr.va_mask = AT_SIZE;
		if (error = VOP_GETATTR(vp, &vattr, 0, CRED()))
			goto error_exit;
		uap->off += vattr.va_size;
		break;
	default:
		error = EINVAL;
		goto error_exit;
	}
	if ((error = VOP_SEEK(vp, fp->f_offset, &uap->off)) == 0)
		rvp->r_off = fp->f_offset = uap->off;

error_exit:
	GETF_MRELE(fp);
	return error;
}

struct accessa {
	char	*fname;
	int	fmode;
};

/*
 * int
 * access(struct accessa *uap, rval_t *rvp)
 * 	Determine accessibility of a file.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EINVAL	if bit pattern in uap->fmode is not EFF_ONLY_OK,
 *			 EX_OK, R_OK, W_OK or X_OK
 *		EACCES	if permission bits of file mode don't permit access
 */

/* ARGSUSED */
int
access(struct accessa *uap, rval_t *rvp)
{
	vattr_t	vattr;
	vnode_t *vp;
	cred_t	*tmpcr;
	cred_t	*savecr;
	int	error;
	int	mode;
	int	eok;
	int	exok;

	if (uap->fmode & ~(EFF_ONLY_OK|EX_OK|R_OK|W_OK|X_OK))
		return EINVAL;

	mode = ((uap->fmode & (R_OK|W_OK|X_OK)) << 6);
	eok = (uap->fmode & EFF_ONLY_OK);
	exok = (uap->fmode & EX_OK);

	if (!eok) {
		savecr = CRED();
		CRED() = crdup(savecr);
		CRED()->cr_uid = savecr->cr_ruid;
		CRED()->cr_gid = savecr->cr_rgid;
		CRED()->cr_ruid = savecr->cr_uid;
		CRED()->cr_rgid = savecr->cr_gid;
		/*
		 * uid-based privilege mechanism only: recalculate
		 * privileges based on "new" effective user-ID.
		 * Has no effect for a file-based privilege mechanism.
		 */
		pm_recalc(CRED());
	}

	if (error = lookupname(uap->fname, UIO_USERSPACE,
	  FOLLOW, NULLVPP, &vp)) {
		if (!eok) {
			tmpcr = CRED();
			CRED() = savecr;
			crfree(tmpcr);
		}
		return error;
	}

	if (mode || exok) {
		/*
		 * Must have the requested MAC access to the vnode,
		 * before checking discretionary access, or getting
		 * the file attributes. Note that we need MAC read
		 * access in addition to the requested discretionary
		 * access if we are doing new "exec" check, since
		 * we will be calling vop_getattr.
		 * If type is VFIFO then always check for WR
		 */

		if (vp->v_type == VFIFO)
			error = MAC_VACCESS(vp, VWRITE, CRED());
		else
			error = MAC_VACCESS(vp, (exok ? mode | R_OK : mode), CRED());
	}
	if (!error && mode) {
#ifdef CC_PARTIAL
		/* MAC READ,EXEC, or WRITE implies at least dominates.*/
		MAC_ASSERT (vp, MAC_DOMINATES);
#endif
		error = VOP_ACCESS(vp, mode, 0, CRED());
	}

	if (!error && exok) {
#ifdef CC_PARTIAL
		/* MAC READ,EXEC, or WRITE implies at least dominates.*/
		MAC_ASSERT (vp, MAC_DOMINATES);
#endif
		vattr.va_mask = AT_MODE;
		error = VOP_GETATTR(vp, &vattr, 0, CRED());
		if ((!error) && ((vp->v_type != VREG) ||
			((vattr.va_mode & (VEXEC|(VEXEC>>3)|(VEXEC>>6))) 
			== 0))) 
			error = EACCES;
	}
	
	if (!eok) {
		tmpcr = CRED();
		CRED() = savecr;
		crfree(tmpcr);
	}

	VN_RELE(vp);
	return error;
}

/*
 * int
 * cstat(vnode_t *vp, struct stat *ubp, cred_t *cr)
 * 	Common code for stat(), lstat(), and fstat().
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

STATIC int
cstat(vnode_t *vp, struct stat *ubp, cred_t *cr)
{
	struct	stat sb;
	vattr_t	vattr;
	int	error;

        /*
         * Must have MAC read access to the vnode.
	 */
	if ((error = MAC_VACCESS(vp, VREAD, cr)) != 0)
		goto out;
#ifdef CC_PARTIAL
	MAC_ASSERT (vp, MAC_DOMINATES);	/* MAC read access implies dominates */
#endif

	vattr.va_mask = AT_STAT;
	error = VOP_GETATTR(vp, &vattr, 0, cr);
	if (error)
		goto out;
	/*
	 * Check for large values.
	 */
	if (vattr.va_uid > USHRT_MAX || vattr.va_gid > USHRT_MAX
	  || vattr.va_nodeid > USHRT_MAX || vattr.va_nlink > SHRT_MAX )
		return (EOVERFLOW);
	/*
	 * Need to convert expanded dev to old dev format.
	 */
	sb.st_dev = cmpdev(vattr.va_fsid);
	sb.st_rdev = cmpdev(vattr.va_rdev);

	/*
	 * Check if cmpdev() returned O_NODEV.
	 */
	if (sb.st_dev == O_NODEV || sb.st_rdev == O_NODEV)
		return (EOVERFLOW);

	sb.st_mode = (o_mode_t) (VTTOIF(vattr.va_type) | vattr.va_mode);
	sb.st_uid = (o_uid_t) vattr.va_uid;
	sb.st_gid = (o_gid_t) vattr.va_gid;
	sb.st_ino = (o_ino_t) vattr.va_nodeid;
	sb.st_nlink = (o_nlink_t) vattr.va_nlink;
	sb.st_size = vattr.va_size;
	sb.st_atime = vattr.va_atime.tv_sec;
	sb.st_mtime = vattr.va_mtime.tv_sec;
	sb.st_ctime = vattr.va_ctime.tv_sec;

	if (copyout((caddr_t)&sb, (caddr_t)ubp, sizeof(sb)))
		error = EFAULT;
out:
	return (error);
}

/*
 * int
 * xcstat(vnode_t *vp, struct xstat *ubp, cred_t *cr)
 *	Common code for xstat(), lxstat(), and fxstat().
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

STATIC int
xcstat(vnode_t *vp, struct xstat *ubp, cred_t *cr)
{
	struct	xstat sb;
	vattr_t	vattr;
	int	error;
	vfssw_t	*vswp;

        /*
         * Must have MAC read access to the vnode.
	 */
	if ((error = MAC_VACCESS(vp, VREAD, cr)) != 0)
		goto out;

#ifdef CC_PARTIAL
	MAC_ASSERT (vp, MAC_DOMINATES);	/* MAC read access implies dominates */
#endif

	struct_zero(&vattr, sizeof(vattr));
	vattr.va_mask = AT_XSTAT;
	if (error = VOP_GETATTR(vp, &vattr, 0, cr))
		goto out;

	struct_zero((caddr_t)&sb, sizeof(sb));
	sb.st_aclcnt = vattr.va_aclcnt;
	sb.st_mode = VTTOIF(vattr.va_type) | vattr.va_mode;
	sb.st_uid = vattr.va_uid;
	sb.st_gid = vattr.va_gid;
	sb.st_dev = vattr.va_fsid;
	sb.st_ino = vattr.va_nodeid;
	sb.st_nlink = vattr.va_nlink;
	sb.st_size = vattr.va_size;
	sb.st_atime = vattr.va_atime;
	sb.st_mtime = vattr.va_mtime;
	sb.st_ctime = vattr.va_ctime;
	sb.st_rdev = vattr.va_rdev;
	sb.st_blksize = vattr.va_blksize;
	sb.st_blocks = vattr.va_nblocks;
	if (vp->v_vfsp) {
		vswp = &vfssw[vp->v_vfsp->vfs_fstype];
		if (vswp->vsw_name && *vswp->vsw_name)
			strcpy(sb.st_fstype, vswp->vsw_name);

	}
	sb.st_level = vp->v_lid;
	if (mac_installed && vp->v_macflag & VMAC_ISMLD)
		sb.st_flags |= S_ISMLD;

	/* For block/char special files check if dev is mounted.
         * This supports a semantic available in ustat(2) where
         * user level code can determine if the device is mounted.
         * This implementation assumes both the block/char major
         * number map to the same driver entry point.
         */
	if (vattr.va_type == VBLK || vattr.va_type == VCHR) {
		SLEEP_LOCK(&vfslist_lock, PRIVFS);
		if (vfs_devsearch(vattr.va_rdev))
			sb.st_flags |= _S_ISMOUNTED;
		SLEEP_UNLOCK(&vfslist_lock);
	}

	if (copyout((caddr_t)&sb, (caddr_t)ubp, sizeof(sb)))
		error = EFAULT;
out:
	return error;
}

struct stata {
	char	*fname;
	struct stat *sb;
};

/*
 * int
 * stat(struct stata *uap, rval_t *rvp)
 * 	Get file attribute information through a file name.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
stat(struct stata *uap, rval_t *rvp)
{
	vnode_t *vp;
	int	error;

	ASSERT(KS_HOLD0LOCKS());
	if (error = lookupname(uap->fname, UIO_USERSPACE, FOLLOW, NULLVPP, &vp))
		return error;
	error = cstat(vp, uap->sb, CRED());
	VN_RELE(vp);
	return error;
}

struct xstatarg {
	int version;
	char *fname;
	struct xstat *sb;
};

/*
 * int
 * xstat(struct xstatarg *uap, rval_t *rvp)
 * 	Get file attribute information through a file name.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EINVAL	if uap->version is not SVR4 stat or SVR3 stat
 */

/* ARGSUSED */
int
xstat(struct xstatarg *uap, rval_t *rvp)
{
	vnode_t *vp;
	int	error;

	ASSERT(KS_HOLD0LOCKS());
	if (error = lookupname(uap->fname, UIO_USERSPACE, FOLLOW, NULLVPP, &vp))
		return error;

	/*
	 * Check version.
	 */
	switch (uap->version) {

	case _STAT_VER:
		/* SVR4 stat */
		error = xcstat(vp, uap->sb, CRED());
		break;

	case _R3_STAT_VER:
		/* SVR3 stat */
		error = cstat(vp, (struct stat *)uap->sb, CRED());
		break;

	default:
		error = EINVAL;
	}

	VN_RELE(vp);
	return error;
}

struct lstata {
	char	*fname;
	struct stat *sb;
};

/*
 * int lstat(struct stata *uap, rval_t *rvp)
 * 	Get file attribute information from a symbolic link.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
lstat(struct lstata *uap, rval_t *rvp)
{
	vnode_t *vp;
	int	error;

	ASSERT(KS_HOLD0LOCKS());
	if (error = lookupname(uap->fname, UIO_USERSPACE, NO_FOLLOW, NULLVPP, &vp))
		return error;
	error = cstat(vp, uap->sb, CRED());
	VN_RELE(vp);
	return error;
}

/*
 * int
 * lxstat(struct xstatarg *uap, rval_t *rvp)
 * 	Get file attribute information.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EINVAL	if uap->version is not SVR4 stat or SVR3 stat
 */

/* ARGSUSED */
int
lxstat(struct xstatarg *uap, rval_t *rvp)
{
	vnode_t *vp;
	int	error;

	ASSERT(KS_HOLD0LOCKS());
	if (error = lookupname(uap->fname, UIO_USERSPACE,
	  NO_FOLLOW, NULLVPP, &vp))
		return error;

	/*
	 * Check version.
	 */
	switch (uap->version) {

	case _STAT_VER:
		/* SVR4 stat */
		error = xcstat(vp, uap->sb, CRED());
		break;

	case _R3_STAT_VER:
		/* SVR3 stat */
		error = cstat(vp, (struct stat *) uap->sb, CRED());
		break;

	default:
		error = EINVAL;
	}

	VN_RELE(vp);
	return error;
}

struct fstata {
	int	fdes;
	struct stat *sb;
};

/*
 * int
 * fstat(struct fstata *uap, rval_t *rvp)
 * 	Get file attribute information through a file descriptor.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
fstat(struct fstata *uap, rval_t *rvp)
{
	file_t *fp;
	vnode_t *vp;
	int	error;

	ASSERT(KS_HOLD0LOCKS());
	error = getf_mhold(uap->fdes, &fp);
	if (!error) {
		vp = fp->f_vnode;
		error = cstat(vp, uap->sb, CRED());
		ADT_GETF(vp);
		GETF_MRELE(fp);
	}
	return (error);
}

struct fxstatarg {
	int	version;
	int	fdes;
	struct xstat *sb;
};

/*
 * int
 * fxstat(struct fxstatarg *uap, rval_t *rvp)
 * 	Get file attribute information.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EINVAL	if uap->version is not SVR4 stat or SVR3 stat
 */

/* ARGSUSED */
int
fxstat(struct fxstatarg *uap, rval_t *rvp)
{
	file_t *fp;
	vnode_t *vp;
	int	error;

	ASSERT(KS_HOLD0LOCKS());
	/*
	 * Check version number.
	 */
	switch (uap->version) {
	case _STAT_VER:
		break;
	default:
		return EINVAL;
	}

	if (error = getf_mhold(uap->fdes, &fp))
		return error;
	vp = fp->f_vnode;

	switch (uap->version) {
	case _STAT_VER:
		/* SVR4 stat */
		error = xcstat(vp, uap->sb, CRED());
		break;

	case _R3_STAT_VER:
		/* SVR3 stat */
		error = cstat(vp, (struct stat *) uap->sb, CRED());
		break;

	default:
		error = EINVAL;
	}

	ADT_GETF(vp);
	GETF_MRELE(fp);
	return error;
}

#if defined(__STDC__)
STATIC int      cpathconf(vnode_t *, int, rval_t *, cred_t *);
#else
STATIC int      cpathconf();
#endif

/*
 * int
 * cpathconf(vnode_t *vp, int cmd, rval_t *rvp, cred_t *cr)
 *	Common code for pathconf(), fpathconf() system calls
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

STATIC int
cpathconf(vnode_t *vp, int cmd, rval_t *rvp, cred_t *cr)
{
	int	error;
	u_long	val;

	if ((error = VOP_PATHCONF(vp, cmd, &val, cr)) == 0)
		rvp->r_val1 = val;

	return error;
}

/* fpathconf/pathconf interfaces */

struct fpathconfa {
	int	fdes;
	int	name;
};

/*
 * int
 * fpathconf(struct fpathconfa *uap, rval_t *rvp)
 *	Get configurable pathname variables from file descriptor.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
fpathconf(struct fpathconfa *uap, rval_t *rvp)
{
	file_t *fp;
	int	error;

	ASSERT(KS_HOLD0LOCKS());

	if ((error = getf_mhold(uap->fdes, &fp)) == 0) {
		error= cpathconf(fp->f_vnode, uap->name, rvp, CRED());
		GETF_MRELE(fp);
	}
	return error;
}

struct pathconfa {
	char	*fname;
	int	name;
};

/*
 * int
 * pathconf(struct pathconfa *uap, rval_t *rvp)
 *	Get configurable pathname variables from pathname.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
pathconf(struct pathconfa *uap, rval_t *rvp)
{
	vnode_t *vp;
	int	error;

	ASSERT(KS_HOLD0LOCKS());

	if (error = lookupname(uap->fname, UIO_USERSPACE,
	  FOLLOW, NULLVPP, &vp))
		return error;
	error = cpathconf(vp, uap->name, rvp, CRED());
	VN_RELE(vp);
	return error;
}


struct readlinka {
	char	*name;
	char	*buf;
	int	count;
};

/*
 * int
 * readlink(struct readlinka *uap, rval_t *rvp)
 *	Read the contents of a symbolic link.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *      A return value of 0 indicates success; otherwise a valid errno
 *      is returned. Errnos returned directly by this routine are:
 *              EINVAL if (vp->v_type != VLNK)
 */

int
readlink(struct readlinka *uap, rval_t *rvp)
{
	vnode_t	*vp;
	iovec_t	aiov;
	uio_t	auio;
	int	error;

	if (error = lookupname(uap->name, UIO_USERSPACE,
	  NO_FOLLOW, NULLVPP, &vp))
		return error;

	if (vp->v_type != VLNK) {
		error = EINVAL;
		goto out;
	}

        /*
         * Must have MAC read access to the symbolic link.
	 */
	if ((error = MAC_VACCESS(vp, VREAD, CRED())) != 0)
		goto out;

#ifdef CC_PARTIAL
	MAC_ASSERT (vp, MAC_DOMINATES);	/* MAC read access implies dominates */
#endif
	aiov.iov_base = uap->buf;
	aiov.iov_len = uap->count;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = 0;
	auio.uio_segflg = UIO_USERSPACE;
	auio.uio_resid = uap->count;
	error = VOP_READLINK(vp, &auio, CRED());

	rvp->r_val1 = uap->count - auio.uio_resid;
out:
	VN_RELE(vp);
	return error;
}

/*
 * int
 * namesetattr(char *fnamep, symfollow_t followlink, vattr_t *vap, int flags)
 *	Common routine for modifying attributes of named files.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *	
 *	A return value of 0 indicated success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EROFS if (vp->v_vfsp->vfs_flag & VFS_RDONLY)
 */

STATIC int
namesetattr(char *fnamep, symfollow_t followlink, vattr_t *vap, int flags)
{
	vnode_t *vp;
	int	error;

	if (error = lookupname(fnamep, UIO_USERSPACE, followlink,
	  NULLVPP, &vp))
		return error;	
	if (vp->v_vfsp->vfs_flag & VFS_RDONLY)
		error = EROFS;
	else {
              	/*
                 * Must have MAC write access to the file before calling
                 * VOP_SETATTR.
                 */
                error = MAC_VACCESS(vp, VWRITE, CRED());
		if (!error)
		{
#ifdef CC_PARTIAL
			MAC_ASSERT(vp, MAC_SAME); /* MAC write implies
						   same labels */
#endif
                        error = VOP_SETATTR(vp, vap, flags, 0, CRED());
		}
	}
	VN_RELE(vp);
	return error;
}

/*
 * int
 * fdsetattr(int fd, vattr_t *vap)
 *	Common routine for modifying attributes of files referenced
 *	by descriptor.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *      A return value of 0 indicated success; otherwise a valid errno
 *      is returned. Errnos returned directly by this routine are:
 *              EROFS if (vp->v_vfsp->vfs_flag & VFS_RDONLY)
 */

static int
fdsetattr(int fd, vattr_t *vap)
{
	file_t	*fp;
	vnode_t	*vp;
	int	error;
	u_int	ioflags;

	error = getf_mhold(fd, &fp);
	if (error == 0) {
		vp = fp->f_vnode;
		ioflags = fp->f_flag;
		if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
			error = EROFS;
			goto error_exit;
		}
		/*
                 * Must have MAC write access to the file before calling
                 * VOP_SETATTR.
                 */
                error = MAC_VACCESS(vp, VWRITE, CRED());
		if (!error) {
#ifdef CC_PARTIAL
			MAC_ASSERT(vp, MAC_SAME); /* MAC write implies
						   same labels */
#endif
                        error = VOP_SETATTR(vp, vap, 0, ioflags, CRED());
		}
error_exit:
		ADT_GETF(vp);
		GETF_MRELE(fp);
	}
	return error;
}

struct chmoda {
	char	*fname;
	int	fmode;
};

/*
 * int
 * chmod(struct chmoda *uap, rval_t *rvp)
 *	Change mode of file given path name.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
chmod(struct chmoda *uap, rval_t *rvp)
{
	vattr_t vattr;

	ASSERT(KS_HOLD0LOCKS());

	vattr.va_mode = uap->fmode & MODEMASK;
	vattr.va_mask = AT_MODE;
	return namesetattr(uap->fname, FOLLOW, &vattr, 0);
}

struct fchmoda {
	int	fd;
	int	fmode;
};

/*
 * int
 * fchmod(struct fchmoda *uap, rval_t *rvp)
 *	Change mode of file given file descriptor.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
fchmod(struct fchmoda *uap, rval_t *rvp)
{
	vattr_t vattr;

	ASSERT(KS_HOLD0LOCKS());

	vattr.va_mode = uap->fmode & MODEMASK;
	vattr.va_mask = AT_MODE;
	return fdsetattr(uap->fd, &vattr);
}

struct chowna {
	char	*fname;
	int	uid;
	int	gid;
};

/*
 * int
 * chown(struct chowna *uap, rval_t *rvp)
 *	Change ownership of file given file name. If file is a
 *	symbolic link, chown() changes ownership of the file or
 *	directory to which the symbolic link refers.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	A return value of 0 if success; otherwise a valid errno is
 *	returned. Errnos returned directly by this routines is:
 *		EINVAL if uid or gid is out of range.
 */

/* ARGSUSED */
int
chown(struct chowna *uap, rval_t *rvp)
{
	vattr_t vattr;

	ASSERT(KS_HOLD0LOCKS());

	if (uap->uid < (uid_t)-1 || uap->uid > MAXUID
	  || uap->gid < (gid_t)-1 || uap->gid > MAXUID)
		return EINVAL;
	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	vattr.va_mask = 0;
	if (vattr.va_uid != (uid_t)-1)
		vattr.va_mask |= AT_UID;
	if (vattr.va_gid != (gid_t)-1)
		vattr.va_mask |= AT_GID;
	return namesetattr(uap->fname, FOLLOW, &vattr, 0);
}

/*
 * int
 * lchown(struct chowna *uap, rval_t *rvp)
 *	Change ownership of file given file name. If file is a
 *      symbolic link, lchown() changes ownership of the symbolic
 *	link file itself.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *	
 * 	A return value of 0 if success; otherwise a valid errno is
 *	returned. Errnos returned directly by this routine is:
 *		EINVAL if uid or gid is out of range.
 */

/* ARGSUSED */
int
lchown(struct chowna *uap, rval_t *rvp)
{
	vattr_t vattr;

	ASSERT(KS_HOLD0LOCKS());

	if (uap->uid < (uid_t)-1 || uap->uid > MAXUID
	  || uap->gid < (gid_t)-1 || uap->gid > MAXUID)
		return EINVAL;
	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	vattr.va_mask = 0;
	if (vattr.va_uid != (uid_t)-1)
		vattr.va_mask |= AT_UID;
	if (vattr.va_gid != (gid_t)-1)
		vattr.va_mask |= AT_GID;
	return namesetattr(uap->fname, NO_FOLLOW, &vattr, 0);
}

/*
 * Change ownership of file given file descriptor.
 */
struct fchowna {
	int	fd;
	int	uid;
	int	gid;
};

/*
 * int
 * fchown(struct fchowna *uap, rval_t *rvp)
 *	Change ownership of file given file descriptor.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *      A return value of 0 if success; otherwise a valid errno is
 *      returned. Errnos returned directly by this routine is:
 *              EINVAL if uid or gid is out of range.
 */

/* ARGSUSED */
int
fchown(struct fchowna *uap, rval_t *rvp)
{
	vattr_t vattr;

	ASSERT(KS_HOLD0LOCKS());

	if (uap->uid < (uid_t)-1 || uap->uid > MAXUID
	  || uap->gid < (gid_t)-1 || uap->gid > MAXUID)
		return EINVAL;
	vattr.va_uid = uap->uid;
	vattr.va_gid = uap->gid;
	vattr.va_mask = 0;
	if (vattr.va_uid != (uid_t)-1)
		vattr.va_mask |= AT_UID;
	if (vattr.va_gid != (gid_t)-1)
		vattr.va_mask |= AT_GID;
	return fdsetattr(uap->fd, &vattr);
}

struct trunca {
	char	*fname;
	off_t	len;
};

/*
 * int
 * truncate(struct trunca *uap, rval_t *rvp)
 *	Truncate a file to a specified length.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
truncate(struct trunca *uap, rval_t *rvp)
{
	vattr_t vattr;

	ASSERT(KS_HOLD0LOCKS());
	if (uap->len < 0)
		return (EINVAL);
	vattr.va_size = uap->len;
	vattr.va_mask = AT_SIZE;
	return namesetattr(uap->fname, FOLLOW, &vattr, 0);
}

struct ftrunca {
	int	fd;
	off_t	len;
};

/*
 * int
 * ftruncate(struct ftrunca *uap, rval_t *rvp)
 *	Truncate a file to a specified length given a file descriptor.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
ftruncate(struct ftrunca *uap, rval_t *rvp)
{
	vattr_t vattr;

	ASSERT(KS_HOLD0LOCKS());
	if (uap->len < 0)
		return (EINVAL);
	vattr.va_size = uap->len;
	vattr.va_mask = AT_SIZE;
	return fdsetattr(uap->fd, &vattr);
}

struct chsizea {
	int fdes;
	int size;
};

/* XENIX Support */

/*
 * int
 * chsize(struct chsizea *uap, rval_t *rvp)
 *	Change file size.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	A return value of 0 if success; otherwise a valid errno is
 *	returned. Errnos returned directly by this routine are:
 *		EFBIG if file size is less than 0 or greater than rlimit.
 *		EBADF if file descriptor is not open for writing
 *		EINVAL if (vp->v_type != VREG)
 */

/* ARGSUSED */
int
chsize(struct chsizea *uap, rval_t *rvp)
{
	vnode_t	*vp;
	vattr_t	vattr;
	int	error;
	file_t	*fp;

	ASSERT(KS_HOLD0LOCKS());
	if (uap->size < 0L ||
	    uap->size > u.u_rlimits->rl_limits[RLIMIT_FSIZE].rlim_cur) {
		return EFBIG;
	}
	error = getf_mhold(uap->fdes, &fp);
	if (error) {
		return error;
	}
	if ((fp->f_flag & FWRITE) == 0) {
		error = EBADF;
		goto error_exit;
	}
	vp = fp->f_vnode;
	if (vp->v_type != VREG) {
		error = EINVAL;
		goto error_exit;
	}
	vattr.va_size = uap->size;
	vattr.va_mask = AT_SIZE;
	error = VOP_SETATTR(vp, &vattr, 0, fp->f_flag, CRED());

error_exit:
	GETF_MRELE(fp);
	return (error);
}

struct rdchka {
	int fdes;
};

/*
 * int
 * rdchk(struct rdchka *uap, rval_t *rvp)
 *	Read check.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	A return value of 0 if success; otherwise a valid errno is
 *	returned. Errnos returned directly by this routine is:
 *		EBADF if file descriptor is not open for reading
 */

/* ARGSUSED */
int
rdchk(struct rdchka *uap, rval_t *rvp)
{
	vnode_t	*vp;
	file_t	*fp;
	vattr_t	vattr;
	int	error;

	ASSERT(KS_HOLD0LOCKS());
	error = getf_mhold(uap->fdes, &fp);
	if (error) {
		return error;
	}
	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto error_exit;
	}
	vp = fp->f_vnode;
	if (vp->v_type == VCHR) {
		error = spec_rdchk(vp, CRED(), &rvp->r_val1);
	} else if (vp->v_type == VFIFO) {
		vattr.va_mask = AT_SIZE;
		error = VOP_GETATTR(vp, &vattr, 0, CRED());
		if (error) {
			goto error_exit;
		}
		if (vattr.va_size > (u_long) 0 || fifo_rdchk(vp) <= 0 ||
		    fp->f_flag & (FNDELAY|FNONBLOCK)) {
			rvp->r_val1 = 1;
		} else {
			rvp->r_val1 = 0;
		}
	} else {
		rvp->r_val1 = 1;
	}

error_exit:
	GETF_MRELE(fp);
	return (error);
}

struct lockinga {
	int  fdes;
	int  mode;
	long size;
};

/*
 * int
 * locking(struct lockinga *uap, rval_t *rvp)
 *	XENIX system call.
 *	Locks or unlocks a file region for reading or writing.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 * Description:
 *	Locking() is a system call subtype called through the cxenix
 *	sysent entry.
 *
 *	The following is a summary of how locking() calls map onto fcntl():
 *
 *	locking()	new fcntl()	acts like fcntl()	with flock
 *	 'mode'		  'cmd'		     'cmd'		 'l_type'
 *	---------	-----------	-----------------	-------------
 *
 *	LK_UNLCK	F_LK_UNLCK	F_SETLK			F_UNLCK
 *	LK_LOCK		F_LK_LOCK	F_SETLKW		F_WRLCK
 *	LK_NBLCK	F_LK_NBLCK	F_SETLK			F_WRLCK
 *	LK_RLCK		F_LK_RLCK	F_SETLKW		F_RDLCK
 *	LK_NBRLCK	F_LK_NBRLCK	F_SETLW			F_RDLCK
 */

/* ARGSUSED */
int
locking(struct lockinga *uap, rval_t *rvp)
{
	file_t	*fp;
	flock_t	bf;
	int	error;
	int	cmd;
	int	scolk;

	ASSERT(KS_HOLD0LOCKS());
	error = getf_mhold(uap->fdes, &fp);
	if (error) {
		return (error);
	}

	scolk = 0;
	/*
	 * Map the locking() mode onto the fcntl() cmd.
	 */
	switch (uap->mode) {
	case LK_UNLCK:
		cmd = F_SETLK;
		bf.l_type = F_UNLCK;
		break;
	case LK_LOCK:
		cmd = F_SETLKW;
		bf.l_type = F_WRLCK;
		break;
	case LK_NBLCK:
		cmd = F_SETLK;
		bf.l_type = F_WRLCK;
		break;
	case LK_RLCK:
		cmd = F_SETLKW;
		bf.l_type = F_RDLCK;
		break;
	case LK_NBRLCK:
		cmd = F_SETLK;
		bf.l_type = F_RDLCK;
		break;
	/* XENIX Support */
	case F_O_GETLK:
	case F_SETLK:
	case F_SETLKW:
		/*
		 * Kludge to some SCO fcntl/lockf x.outs (they
		 * map onto locking, instead of onto fcntl...).
		 */
/* Enhanced Application Compatibility Support */
		if (VIRTUAL_XOUT) {
/* End Enhanced Application Compatibility Support */
			cmd = uap->mode;
			scolk++;
			break;
		}
		else
			return EINVAL;
	/* End XENIX Support */
	default:
		GETF_MRELE(fp);
		return (EINVAL);
	}

	if (scolk == 0) {
		bf.l_whence = 1;
		if (uap->size < 0) {
			bf.l_start = uap->size;
			bf.l_len = -(uap->size);
		} else {
			bf.l_start = 0L;
			bf.l_len = uap->size;
		}
	} else {
		/* SCO fcntl/lockf */
		if (copyin((caddr_t)uap->size, (caddr_t)&bf, sizeof(o_flock_t))) 
			return (EFAULT);
		else
			bf.l_type = XMAP_TO_LTYPE(bf.l_type);	
	}

#ifdef _SECURE_CCA
	{
	    vnode_t *tvp;

	    tvp = fp->f_vnode;
	    /*
	     * This shouldn't be necessary, but coding the call
	     * this way allows the CCA tool to interpret the
	     * filesystem-specific code with only vnodes
	     * appropriate to each file system.
	     */
	    error = VOP_FRLOCK(tvp, cmd, &bf, fp->f_flag, fp->f_offset, CRED());
	}
#else
	error = VOP_FRLOCK(fp->f_vnode, cmd, &bf, fp->f_flag, fp->f_offset,
			   CRED());
#endif

	if (error == 0 && uap->mode != LK_UNLCK) {
		VN_LOCK(fp->f_vnode);
		fp->f_vnode->v_flag |= VXLOCKED;
		VN_UNLOCK(fp->f_vnode);
	}
	GETF_MRELE(fp);
	return (error);
}
/* End XENIX Support */

struct utimea {
	char	*fname;
	time_t	*tptr;
};

/*
 * int
 * utime(struct utimea *uap, rval_t *rvp)
 *	Set access/modify times on named file.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	A returned value of 0 if success; otherwise a valid errno is
 *	returned. Errnos directly returned by this routine is :
 *		EFAULT if pathname points outside of the process's
 *			allocated address space
 */

/* ARGSUSED */
int
utime(struct utimea *uap, rval_t *rvp)
{
	time_t	tv[2];
	vattr_t vattr;
	int	flags;

	ASSERT(KS_HOLD0LOCKS());

	flags = 0;
	if (uap->tptr != NULL) {
		if (copyin((caddr_t)uap->tptr,(caddr_t)tv, sizeof(tv)))
			return EFAULT;
		flags |= ATTR_UTIME;
	} else {
		tv[0] = hrestime.tv_sec;
		tv[1] = tv[0];
	}
	vattr.va_atime.tv_sec = tv[0];
	vattr.va_atime.tv_nsec = 0;
	vattr.va_mtime.tv_sec = tv[1];
	vattr.va_mtime.tv_nsec = 0;
	vattr.va_mask = AT_ATIME|AT_MTIME;
	return namesetattr(uap->fname, FOLLOW, &vattr, flags);
}


struct fsynca {
	int fd;
};

/*
 * int
 * fsync(struct fsynca *uap, rval_t *rvp)
 *	Flush output pending for file.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *	A returned value of 0 if success; otherwise a valid errno is
 *	returned. Errnos directly retured by this routine is:
 *		EBADF if file descriptor is not open for writing
 */

/* ARGSUSED */
int
fsync(struct fsynca *uap, rval_t *rvp)
{
	file_t	*fp;
	int	error;
	
	ASSERT(KS_HOLD0LOCKS());

        error = getf_mhold(uap->fd, &fp);
        if (error == 0) {
		if ((fp->f_flag & FWRITE) == 0)
		{
			error = EBADF;
			goto error_exit;
		} else {
#ifdef _SECURE_CCA
			vnode_t *tvp;

			tvp = fp->f_vnode;
			/*
			 * This shouldn't be necessary, but coding the
			 * call this way allows the CCA tool to
			 * interpret the filesystem-specific code
			 * with only vnodes appropriate
			 * to each file system.
			 */
			error = VOP_FSYNC(tvp, CRED());
#else
			error = VOP_FSYNC(fp->f_vnode, CRED());
#endif
		}
	} else {
		return error;
	}
error_exit:
	GETF_MRELE(fp);
	return error;
}

struct fcntla {
	int fdes;
	int cmd;
	int arg;
};

/*
 * int
 * fcntl(struct fcntla *uap, rval_t *rvp)
 *	File control operations.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 *
 *      A returned value of 0 if success; otherwise a valid errno is
 *      returned. Errnos directly retured by this routine is:
 *              EINVAL if fdes is less than 0 or greater than the limit. 
 */

int
fcntl(struct fcntla *uap, rval_t *rvp)
{
	file_t	  *fp;
	int	  i;
	int	  error;
	vnode_t	  *vp;
	off_t	  offset;
	int	  fd;
	uint_t	  flag;
	flock_t	  bf;
	o_flock_t obf;
	char	  flags;
	vattr_t	  vattr;
	/* XENIX Support */
	uint_t	  virt_xout = 0;
	/* End XENIX Support */

	ASSERT(KS_HOLD0LOCKS());

	error = getf_mhold(uap->fdes, &fp);
	if (error) {
		return (error);
	}
	vp = fp->f_vnode;
	flag = fp->f_flag;
	offset = fp->f_offset;

	switch (uap->cmd) {

	case F_DUPFD:
		if ((i = uap->arg) < 0 ||
		    i >= u.u_rlimits->rl_limits[RLIMIT_NOFILE].rlim_cur)
			error = EINVAL;
		else if ((error = fddup(uap->fdes, i, ~FCLOSEXEC, &fd)) == 0)
			rvp->r_val1 = fd;
		break;

	case F_DUP2:
		fd = uap->fdes;
		i = uap->arg;
		if (i < 0 ||
		    i >= u.u_rlimits->rl_limits[RLIMIT_NOFILE].rlim_cur) {
			error = EINVAL;
		} else if ((error = fddup2(fd, ~FCLOSEXEC, i)) == 0) {
			rvp->r_val1 = i;
		}
		break;

	case F_GETFD:
		error = getpof(uap->fdes, &flags);
		if (error != 0) {
			break;
		}
		rvp->r_val1 = flags;
		break;

	case F_SETFD:
		error = setpof(uap->fdes, (char)uap->arg);
		break;

	case F_GETFL:
		rvp->r_val1 = fp->f_flag+FOPEN;
		break;

	case F_SETFL:
		if ((uap->arg & (FNONBLOCK|FNDELAY)) == (FNONBLOCK|FNDELAY)) {
			uap->arg &= ~FNDELAY;
		}
		error = VOP_SETFL(vp, flag, uap->arg, fp->f_cred);
		if (error == 0) {
			flag &= (FREAD|FWRITE);
			flag |= ((uap->arg & FMASK) - FOPEN) & ~(FREAD|FWRITE);

			/* NOTE: Relying on atomic memory access */
			fp->f_flag = flag;
		}
		break;

	case F_SETLK:
	case F_SETLKW:
		/*
		 * Must have MAC write access to the vnode in question.
		 * Should this be after the copyin()?
		 */
		error = MAC_VACCESS(vp, VWRITE, CRED());
		if (error) {
			break;
		}

#ifdef CC_PARTIAL
		MAC_ASSERT(vp, MAC_SAME); /* MAC write implies same labels */
#endif
		/* FALLTHRU */
	case F_GETLK:
	case F_O_GETLK:
		/*
		 * Copy in input fields only.
		 */
		if (copyin((caddr_t)uap->arg, (caddr_t)&bf, sizeof obf)) {
			error = EFAULT;
			break;
		/* XENIX Support */
		} else {
			virt_xout = VIRTUAL_XOUT;
			/* Map lock type for XENIX binaries */
			if (virt_xout) 
				bf.l_type = XMAP_TO_LTYPE(bf.l_type);	
			/* Indicate to VOP_FRLOCK() that it was called by
			 * fcntl(), instead of from chklock(), etc.
			 * This info is needed to support XENIX behavior
			 * in VOP_FRLOCK().
			 */
			RENV |= UB_FCNTL; 
		}
		/* End XENIX Support */
		if ((uap->cmd == F_SETLK || uap->cmd == F_SETLKW) &&
		     bf.l_type != F_UNLCK) {
			getpof(uap->fdes, &flags);
			setpof(uap->fdes, flags|UF_FDLOCK);
		}
		error = VOP_FRLOCK(vp, uap->cmd, &bf, flag, offset, CRED());
		if (error) {
			/*
			 * Translation for backward compatibility.
			 */
			/* XENIX Support */
			if ((error == EAGAIN) && !(virt_xout))
			/* End XENIX Support */
				error = EACCES;
			break;
		/* XENIX Support */
		} else {
			if (virt_xout) {
				/* Turn on lock enforcement bit */
				if (uap->cmd == F_SETLK || uap->cmd == F_SETLKW)
				{
					VN_LOCK(vp);
					vp->v_flag |= VXLOCKED;
					VN_UNLOCK(vp);
				}
				/* Map lock type for XENIX binaries */
				if (uap->cmd != F_SETLKW)
					bf.l_type = XMAP_FROM_LTYPE(bf.l_type);
			}
		}
		/* End XENIX Support */

		/*
		 * If command is GETLK and no lock is found, only
		 * the type field is changed.
		 */
		if ((uap->cmd == F_O_GETLK || uap->cmd == F_GETLK)
		  && bf.l_type == F_UNLCK) {
			if (copyout((caddr_t)&bf.l_type,
			   (caddr_t)&((flock_t *)uap->arg)->l_type,
			    sizeof(bf.l_type))) {
				error = EFAULT;
			}
			break;
		}

		if (uap->cmd == F_O_GETLK) {
			/*
			 * Return an SVR3 flock structure to the user.
			 */
			obf.l_type = bf.l_type;
			obf.l_whence = bf.l_whence;
			obf.l_start = bf.l_start;
			obf.l_len = bf.l_len;
			if (bf.l_sysid > SHRT_MAX || bf.l_pid > SHRT_MAX) {
				/*
				 * One or both values for the above fields
				 * is too large to store in an SVR3 flock
				 * structure.
				 */
				error = EOVERFLOW;
				break;
			}
			obf.l_sysid = (short) bf.l_sysid;
			obf.l_pid = (o_pid_t) bf.l_pid;
			if (copyout((caddr_t)&obf, (caddr_t)uap->arg,
			    sizeof obf)) {
				error = EFAULT;
			}
		} else if (uap->cmd == F_GETLK) {
			/*
			 * Copy out SVR4 flock.
			 */
			int i;

			for (i = 0; i < 4; i++)
				bf.l_pad[i] = 0;
		    	if (copyout((caddr_t)&bf, (caddr_t)uap->arg, sizeof bf))
			  	error = EFAULT;
		}
		/* XENIX Support */
		if (virt_xout)
			RENV &= ~UB_FCNTL;
		/* End XENIX Support */
		break;

	case F_RSETLK:
	case F_RSETLKW:
		/*
		 * Must have MAC write access to the vnode in question.
		 * Should this be after the copyin()?
		 */
		if ((error = MAC_VACCESS(vp, VWRITE, CRED())) != 0)
			break;

#ifdef CC_PARTIAL
		MAC_ASSERT(vp, MAC_SAME); /* MAC write implies same labels */
#endif
		/* FALLTHRU */
	case F_RGETLK:
		/*
		 * EFT only interface, applications cannot use
		 * this interface when _STYPES is defined.
		 * This interface supports an expanded
		 * flock struct--see fcntl.h.
		 */
		if (copyin((caddr_t)uap->arg, (caddr_t)&bf, sizeof bf)) {
			error = EFAULT;
			break;
		/* XENIX Support */
		} else {
			virt_xout = VIRTUAL_XOUT;
			/* Map lock type for XENIX binaries */
			if (virt_xout) 
				bf.l_type = XMAP_TO_LTYPE(bf.l_type);	
			/* Indicate to VOP_FRLOCK() that it was called by
			 * fcntl(), instead of from chklock(), etc.
			 * This info is needed to support XENIX behavior
			 * in VOP_FRLOCK().
			 */
			RENV |= UB_FCNTL; 
		}
		/* End XENIX Support */
		error = VOP_FRLOCK(vp, uap->cmd, &bf, flag, offset, CRED());
		if (error) {
			/*
			 * Translation for backward compatibility.
			 */
		/* XENIX Support */
			if ((error == EAGAIN) && !(virt_xout))
		/* End XENIX Support */
				error = EACCES;
			break;
		/* XENIX Support */
		} else {
			if (virt_xout) {
				/* Turn on lock enforcement bit */
				if (uap->cmd == F_RSETLK ||
				    uap->cmd == F_RSETLKW) {
					VN_LOCK(vp);
					vp->v_flag |= VXLOCKED;
					VN_UNLOCK(vp);
				}
				/* Map lock type for XENIX binaries */
				if (uap->cmd != F_RSETLKW)
					bf.l_type = XMAP_FROM_LTYPE(bf.l_type);
			}
		}
		/* End XENIX Support */
		if (uap->cmd == F_RGETLK
		  && copyout((caddr_t)&bf, (caddr_t)uap->arg, sizeof bf)) {
			  error = EFAULT;
		}
		/* XENIX Support */
		if (virt_xout)
			RENV &= ~UB_FCNTL;
		/* End XENIX Support */
		break;

	case F_ALLOCSP:
	case F_FREESP:
		if ((flag & FWRITE) == 0) {
			error = EBADF;
		} else if (vp->v_type != VREG) {
			error = EINVAL;
		/*
		 * For compatibility we overlay an SVR3 flock on an SVR4
		 * flock.  This works because the input field offsets 
		 * in "flock_t" were preserved.
		 */
		} else if (copyin((caddr_t)uap->arg, (caddr_t)&bf,
			   sizeof obf)) {
				error = EFAULT;
		} else {
			/*
			 * Must have MAC write access to the vnode in
			 * question.
			 */
			error = MAC_VACCESS(vp, VWRITE, CRED());
			if (error) {
				break;
			} else {
#ifdef CC_PARTIAL
				/* MAC write implies same labels */
				MAC_ASSERT(vp, MAC_SAME);
#endif
				vattr.va_mask = AT_SIZE;
				error = VOP_GETATTR(vp, &vattr, 0, CRED());
				if (error) {
					break;
				}
				error = VOP_RWWRLOCK(vp, offset, vattr.va_size,
						     flag);
				if (error) {
					break;
				}
				error = convoff((off_t *)vattr.va_size, &bf, 0,
						 offset);
				VOP_RWUNLOCK(vp, offset, vattr.va_size);
				if (error == 0) {
					vattr.va_size = bf.l_start;
					vattr.va_mask = AT_SIZE;
					error = VOP_SETATTR(vp, &vattr, 0, flag,
							    CRED());
				}
			}
		}
		break;

	default:
		error = EINVAL;
		break;
	}

	ADT_GETF(vp);
	GETF_MRELE(fp);	/* release execution reference */
	return (error);
}

struct dupa {
	int	fdes;
};

/*
 * int
 * dup(struct dupa *uap, rval_t *rvp)
 *	Duplicate a file descriptor.
 *
 * Calling/Exit State:
 *	This function can block.  No locks should be
 *	held upon entry, none are held on exit.
 */

int
dup(struct dupa *uap, rval_t *rvp)
{
	int error;
	int fd;

	ASSERT(KS_HOLD0LOCKS());
	if ((error = fddup(uap->fdes, 0, ~FCLOSEXEC, &fd)) == 0)
		rvp->r_val1 = fd;
	return (error);
}

struct ioctla {
	int fdes;
	int cmd;
	int arg;
};

/*
 * int
 * ioctl(struct ioctla *uap, rval_t *rvp)
 *	I/O control on devices and STREAMS.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

int
ioctl(struct ioctla *uap, rval_t *rvp)
{
	file_t	*fp;
	int	error;
	vnode_t	*vp;
	vattr_t	vattr;
	off_t	offset;
	int	flag;
	pl_t	pl;

	ASSERT(KS_HOLD0LOCKS());
	if (error = getf_mhold(uap->fdes, &fp))
		return error;

	if ((uap->cmd != FIONREAD) && (uap->cmd != FIONBIO)) {
#ifdef _SECURE_CCA
		{
		    vnode_t *tvp;
	
		    tvp = fp->f_vnode;
		    /*
		     * This shouldn't be necessary, but coding the call
		     * this way allows the CCA tool to interpret the
		     * filesystem-specific code with only vnodes appropriate
		     * to each file system.
		     */
		    error = VOP_IOCTL(tvp, uap->cmd, uap->arg,
			fp->f_flag, CRED(), &rvp->r_val1);
		}
#else
		error = VOP_IOCTL(fp->f_vnode, uap->cmd, uap->arg,
		    fp->f_flag, CRED(), &rvp->r_val1);
#endif
		ADT_GETFIO(fp);
		GETF_MRELE(fp);
		return error;
	}

	/*
	 * Handle these two ioctls for regular files and
	 * directories.  All others will usually be failed
	 * with ENOTTY by the VFS-dependent code.  System V
	 * always failed all ioctls on regular files, but SunOS
	 * supported these.
	 */

	vp = fp->f_vnode;
	if ((vp->v_type == VREG) || (vp->v_type == VDIR)) {
		switch ((uint_t) uap->cmd) {
		case FIONREAD:
			vattr.va_mask = AT_SIZE;
			if (error = VOP_GETATTR(vp, &vattr, 0, CRED()))
				goto error_exit;
			offset = vattr.va_size - fp->f_offset;
			if (copyout((caddr_t)&offset, (caddr_t)uap->arg,
			  sizeof(offset))) {
				error = EFAULT;
				goto error_exit;
			}
			goto error_exit;
	
		case FIONBIO:
			if (copyin((caddr_t)uap->arg, (caddr_t)&flag, 
			  sizeof(int))) {
				error = EFAULT;
				goto error_exit;
			}
			pl = FTE_LOCK(fp);
			if (flag)
				fp->f_flag |= FNDELAY;
			else
				fp->f_flag &= ~FNDELAY;
			FTE_UNLOCK(fp, pl);
			goto error_exit;
	
		default:
			break;
		}
	}
#ifdef _SECURE_CCA
	{
	    vnode_t *tvp;

	    tvp = fp->f_vnode;
	    /*
	     * This shouldn't be necessary, but coding the call
	     * this way allows the CCA tool to interpret the
	     * filesystem-specific code with only vnodes appropriate
	     * to each file system.
	     */
	    error = VOP_IOCTL(tvp, uap->cmd, uap->arg,
		fp->f_flag, CRED(), &rvp->r_val1);
	}
#else
	error = VOP_IOCTL(fp->f_vnode, uap->cmd, uap->arg,
	    fp->f_flag, CRED(), &rvp->r_val1);
#endif
	if (error == 0) {
		switch ((uint_t) uap->cmd) {
		case FIONBIO:
			if (copyin((caddr_t)uap->arg, (caddr_t)&flag,
			  sizeof(int))) {
				error = EFAULT;
				break;
			}
			pl = FTE_LOCK(fp);
			if (flag)
				fp->f_flag |= FNDELAY;
			else
				fp->f_flag &= ~FNDELAY;
			FTE_UNLOCK(fp, pl);
			break;

		default:
			break;
	    }
	}
error_exit:
	ADT_GETFIO(fp);
	GETF_MRELE(fp);
	return error;
}

struct sgttya {
	int	fdes;
	int	arg;
};

/*
 * int
 * stty(struct sgttya *uap, rval_t *rvp)
 *	Set terminal I/O options.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
stty(struct sgttya *uap, rval_t *rvp)
{
	return (EIO);
}

/*
 * int
 * gtty(struct sgttya *uap, rval_t *rvp)
 *	Get terminal I/O options.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
gtty(struct sgttya *uap, rval_t *rvp)
{
	return (EIO);
}

lock_t	pollcompat;	/* for compatibility with older drivers */
fspin_t	pollgen_fspin;	/* to protect pollgen */
unsigned long pollgen;	/* generation count to solve polladd/pollwakeup race */

/*
 * void
 * polltime(register lwp_t *lwpp)
 *	This function is placed in the callout table to time out a process
 *	waiting on poll.  If the poll completes, this function is removed
 *	from the table.
 *
 * Calling/Exit State:
 *	Assumes l_mutex not held.
 */
STATIC void
polltime(register lwp_t *lwpp)
{
	pl_t pl;

	pl = LOCK(&lwpp->l_mutex, PLHI);
	lwpp->l_pollflag &= ~SPOLLTIME;
	UNLOCK(&lwpp->l_mutex, pl);
	/* Don't need to hold l_mutex for EVENT_SIGNAL - in fact, you can't */
	EVENT_SIGNAL(&lwpp->l_pollevent, 0);
}

/*
 * void
 * pollrun(register lwp_t *lwpp)
 *	This function is called to inform a process that
 *	an event being polled for has occurred.
 *
 * Calling/Exit State:
 *	Assumes l_mutex not held.
 */

STATIC void
pollrun(register lwp_t *lwpp)
{
	/* Don't need to hold l_mutex for EVENT_SIGNAL - in fact, you can't */
	EVENT_SIGNAL(&lwpp->l_pollevent, 0);
}


/*
 * void
 * polladd(register struct pollhead *php, short events, void (*fn)(), long arg,
 *	   register struct polldat *pdp)
 *	This function allocates a polldat structure, fills in the given
 *	data, and places it on the given pollhead list.
 *
 * Calling/Exit State:
 *	Assumes that ph_mutex (or pollcompat) is not held on entry.  No
 *	locks held on exit.
 */

STATIC void
polladd(register struct pollhead *php, short events, void (*fn)(), long arg,
	register struct polldat *pdp)
{
	register lock_t *lockp;
	pl_t pl;

	pdp->pd_events = events;
	pdp->pd_fn = fn;
	pdp->pd_arg = arg;
	pdp->pd_headp = php;
	if (php->ph_type == PHCOMPAT)
		lockp = &pollcompat;
	else
		lockp = php->ph_mutex;
	pl = LOCK(lockp, PLHI);
	if (php->ph_list) {
		pdp->pd_next = php->ph_list;
	} else {
		pdp->pd_next = NULL;
	}
	php->ph_list = pdp;
	php->ph_events |= events;
	UNLOCK(lockp, pl);
}

/*
 * void
 * polldel(register struct pollhead *php, register struct polldat *pdp)
 *	This function removes a polldat structure from a pollhead.
 *	Recalculate the events after removal of the specified polldat.
 *
 * Calling/Exit State:
 *	Assumes ph_mutex (or pollcompat) not held.
 */

STATIC void
polldel(register struct pollhead *php, register struct polldat *pdp)
{
	register struct polldat *p;
	register struct polldat *op;
	lock_t *lockp;
	pl_t pl;

	op = NULL;
	if (php->ph_type == PHCOMPAT)
		lockp = &pollcompat;
	else
		lockp = php->ph_mutex;
	pl = LOCK(lockp, PLHI);
	php->ph_events = 0;
	for (p = php->ph_list; p; p = p->pd_next) {
		if (p == pdp) {
			if (op)
				op->pd_next = p->pd_next;
			else
				php->ph_list = p->pd_next;
		} else {
			php->ph_events |= p->pd_events;
			op = p;
		}
	}
	UNLOCK(lockp, pl);
}

struct polla {
	struct pollfd *fdp;
	unsigned long nfds;
	long	timo;
};

/*
 * int
 * poll(struct polla *uap, rval_t *rvp)
 *	Poll file descriptors for interesting events.
 *
 * Calling/Exit State:
 *	No locks should be held upon entry, none are held on exit.
 */

/* ARGSUSED */
int
poll(register struct polla *uap, rval_t *rvp)
{
	register int i;
	pl_t pl;
	register fdcnt;
	struct pollfd *pollp;
	time_t t;
	int rem;
	toid_t id;
	int size;
	int psize;
	int dsize;
	int xsize;
	struct polldat *darray;
	struct polldat *curdat;
	struct pollx *xarray;
	struct pollhead *php;
	int error;
	lwp_t *lwpp;
	unsigned long opgen;
	unsigned long tmp;

	if (uap->nfds < 0 || uap->nfds > u.u_rlimits->rl_limits[RLIMIT_NOFILE].rlim_cur)
		return EINVAL;
	t = lbolt;
	error = 0;
	fdcnt = 0;
	lwpp = u.u_lwpp;
	darray = curdat = NULL; /* PCS */

	/*
	 * Allocate space for the pollfd array and space for the
	 * polldat structures and pollx structures.  Then copy in
	 * the pollfd array from user space.
	 */
	if (uap->nfds != 0) {
		/* allocate all we need in one shot */
		psize = uap->nfds * sizeof(struct pollfd);
		dsize = uap->nfds * sizeof(struct polldat);
		xsize = uap->nfds * sizeof(struct pollx);
		size = psize + dsize + xsize;
		if ((pollp = (struct pollfd *)kmem_zalloc(size, KM_NOSLEEP)) == NULL)
			return EAGAIN;
		/* LINTED pointer alignment */
		darray = (struct polldat *) ((char *) pollp + psize);
		/* LINTED pointer alignment */
		xarray = (struct pollx *) ((char *) darray + dsize);
		curdat = darray;
		if (copyin((caddr_t)uap->fdp, (caddr_t)pollp, psize)) {
			kmem_free((caddr_t)pollp, size);
			return EFAULT;
		}

		EVENT_CLEAR(&lwpp->l_pollevent);
		lwpp->l_pollflag = 0;

		/* convert fds to fps */
		fdgetpollx(pollp, xarray, uap->nfds);
	}

retry:		
	FSPIN_LOCK(&pollgen_fspin);
	opgen = pollgen;
	FSPIN_UNLOCK(&pollgen_fspin);
	/*
	 * Retry scan of fds until an event is found or until the
	 * timeout is reached.
	 */

	/*
	 * Polling the fds is a relatively long process.
	 * Clean up from last iteration.
	 */
	if (uap->nfds != 0) {
		while (--curdat >= darray) {
			polldel(curdat->pd_headp, curdat);
		}
		curdat = darray;
	}
	for (i = 0; i < uap->nfds; i++) {
		if (pollp[i].fd < 0) 
			pollp[i].revents = 0;
		else if (xarray[i].px_fp == NULL)
			pollp[i].revents = POLLNVAL;
		else {
			xarray[i].px_php = NULL;
#ifdef _SECURE_CCA
			{
			    vnode_t *tvp;

			    tvp = xarray[i].px_fp->f_vnode;
			    /*
			     * This shouldn't be necessary, but coding the
			     * call this way allows the CCA tool to
			     * interpret the filesystem-specific code
			     * with only vnodes appropriate
			     * to each file system.
			     */
			    error = VOP_POLL(tvp, pollp[i].events, fdcnt,
				&pollp[i].revents, &xarray[i].px_php);
			}
#else
			error = VOP_POLL(xarray[i].px_fp->f_vnode, pollp[i].events, fdcnt,
			    &pollp[i].revents, &xarray[i].px_php);
#endif
			if (error) {
				goto pollout;
			}
		}
		if (pollp[i].revents)
			fdcnt++;
		else if (fdcnt == 0 && xarray[i].px_php) {
			polladd(xarray[i].px_php, pollp[i].events, pollrun,
			  (long)lwpp, curdat++);
		}
	}
	if (fdcnt) 
		goto pollout;

	/*
	 * This is moderately tricky logic.  If the poll generation number is
	 * different than when we started, then a pollwakeup has fired.  It
	 * may or may not have been for a pollhead that we're interested in.
	 * Since we got here, fdcnt is 0, so nothing was found above and we
	 * would drop down to the EVENT_WAIT_SIG.  If the pollwakeup was
	 * done after the polladd, then there is an event that we could
	 * consume, which would kick us back up to retry.  If we raced and
	 * the pollwakeup happened before the polladd, then there is no event
	 * and we would block and lose the fact that the pollwakeup occurred.
	 * At pollwakeup time, the pollhead is known and a new generation
	 * number is put in to mark it.  If we find a pollhead with a newer
	 * generation number than we saved, it's pollwakeup has fired.  Note
	 * that this catches both cases - when we raced and when we didn't.
	 * The only distinction is whether or not an event is pending.
	 * Rescanning the fds will be successul and on exit (pollout), the
	 * pending events are cleared.  Also note that the logic of a "later"
	 * generation works even if pollgen wraps - an odd characteristic of
	 * unsigned math.  By scaling the sample points, the comparison is
	 * valid for both wrap and non-wrap cases.  In particular, we are
	 * interested in the case where opgen <= ph_gen < pollgen, so we
	 * scale by opgen.
	 */
	FSPIN_LOCK(&pollgen_fspin);
	tmp = pollgen - opgen;
	if (tmp != 0) {
		/* might have missed a pollwakeup */
		for (i = 0; i < uap->nfds; i++) {
			php = xarray[i].px_php;
			if (php && ((php->ph_gen - opgen) < tmp)) {
				/* A pollwakeup we care about has happened */
				FSPIN_UNLOCK(&pollgen_fspin);
				goto retry;
			}
		}
	}
	FSPIN_UNLOCK(&pollgen_fspin);

	/*
	 * If you get here, the poll of fds was unsuccessful.
	 * First make sure your timeout hasn't been reached.
	 * If not then sleep and wait until some fd becomes
	 * readable, writeable, or gets an exception.
	 */
	rem = uap->timo < 0 ? 1 : uap->timo - ((lbolt - t)*1000)/HZ;
	if (rem <= 0)
		goto pollout;

	if (uap->timo > 0) {
		/*
		 * Turn rem into milliseconds and round up.
		 */
		rem = ((rem/1000) * HZ) + ((((rem%1000) * HZ) + 999) / 1000);
		pl = LOCK(&lwpp->l_mutex, PLHI);
		lwpp->l_pollflag |= SPOLLTIME;
		UNLOCK(&lwpp->l_mutex, pl);
		id = itimeout((void(*)())polltime, (caddr_t)lwpp, rem, PLHI);
		if (id == 0) {
			error = EAGAIN;
			goto pollout;
		}
	}

	/*
	 * The lwp will usually be awakened either by this poll's timeout 
	 * (which will have cleared SPOLLTIME), or by the pollwakeup function 
	 * called from either the VFS, the driver, or the stream head.  If
	 * the event happened between the return from VOP_POLL and now, the
	 * EVENT_WAIT_SIG won't block.
	 */
	if (EVENT_WAIT_SIG(&lwpp->l_pollevent, PRIMED) == B_FALSE) {
		if (uap->timo > 0)
			untimeout(id);
		error = EINTR;
		goto pollout;
	}

	/*
	 * If SPOLLTIME is still set, you were awakened because an event
	 * occurred (data arrived, can write now, or exceptional condition).
	 * If so go back up and poll fds again. Otherwise, you've timed
	 * out so you will fall through and return.
	 */
	if (uap->timo > 0) {
		pl = LOCK(&lwpp->l_mutex, PLHI);
		if (lwpp->l_pollflag & SPOLLTIME) {
			lwpp->l_pollflag &= ~SPOLLTIME;
			UNLOCK(&lwpp->l_mutex, pl);
			untimeout(id);
			goto retry;
		}
		UNLOCK(&lwpp->l_mutex, pl);
	} else
		goto retry;

pollout:

	/*
	 * Poll cleanup code.
	 */
	if (error == 0) {
		/*
		 * Copy out the events and return the fdcnt to the user.
		 */
		rvp->r_val1 = fdcnt;
		if (uap->nfds != 0)
			if (copyout((caddr_t)pollp, (caddr_t)uap->fdp, psize))
				error = EFAULT;
	}
	lwpp->l_pollflag = 0;
	EVENT_CLEAR(&lwpp->l_pollevent);
	if (uap->nfds != 0) {
		/* get rid of accumulated polldats */
		while (--curdat >= darray) {
			polldel(curdat->pd_headp, curdat);
		}
		for (i = 0; i < uap->nfds; i++) {
			if (xarray[i].px_fp)
				GETF_MRELE(xarray[i].px_fp);
		}
		kmem_free((caddr_t)pollp, size);
	}
	return error;
}

struct filepriva {
	char	*fname;
	int	cmd;
	priv_t	*privp;
	int	count;
};

/*
 * int filepriv(struct filepriva *uap, rval_t *rvp)
 * 	set or get privileges of the named file.
 *	Takes four parameters:
 *		path -> pointer to file name.
 *		cmd ->  command type.
 *			Depends on privilege mechanism in use.
 *		privp ->  pointer to an array of PRIDs which are
 *			  a list of longs.
 *		count ->  number of PRIDs contained in privp.
 * Calling/Exit State:
 * 	No locks are held on entry and none held on exit.
 * 	This syscall returns error in case of failure and 0 for succes.
 *
 */

/* ARGSUSED */
int
filepriv(struct filepriva *uap, rval_t *rvp)
{
	vnode_t	*vp;
	vattr_t	vattr;
	int	error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (error = lookupname(uap->fname, UIO_USERSPACE, FOLLOW, NULLVPP, &vp))
		return error;

	vattr.va_mask = AT_STAT;
	if (!(error = VOP_GETATTR(vp, &vattr, 0, CRED()))) 
		error = pm_file(uap->cmd, vp, &vattr, rvp, CRED(), 
			uap->privp, uap->count);

	VN_RELE(vp);
	return error;
}


/*
 *
 * STATIC int clvlfile(vnode_t *vp, int cmd, lid_t *usrlevel)
 * 	This is the common routine which handles the processing for both the
 * 	lvlfile() and flvlfile system calls. It returns 0 on success, EACCES
 * 	if the process does not have the appropriate MAC access, and EPERM
 * 	if the process does not have the appropriate privilege to set the level.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, clvlfile returns zero.  On faliure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 *
 */
STATIC int
clvlfile(vnode_t *vp, int cmd, lid_t *usrlevel)
{
	lid_t	lid;
	int	error;

	switch (cmd) {
	case MAC_SET:
		/* 
		 * Check for read-only fs.  No need to lock vfs_flag 
		 * because it is set once at mount time only. 
		 */
		if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
			error = EROFS;
			break;
		}

		if (copyin((caddr_t)usrlevel, (caddr_t)&lid, sizeof(lid_t))) {
			error = EFAULT;
			break;
		}

		if (mac_valid(lid)) {
			error = EINVAL;
			break;
		}

		/*
		 * Device special files are handled separately by
		 * SPECFS when VOP_SETLEVEL is called.
		 */
		if (vp->v_type != VCHR && vp->v_type != VBLK) {
			/* Must have MAC write access. */
			if ((error = MAC_VACCESS(vp, VWRITE, CRED())) != 0) 
				break;

#ifdef CC_PARTIAL
                        MAC_ASSERT(vp, MAC_SAME); /* MAC write implies 
						     same labels */
#endif
			/*
			 * Must either have P_SETFLEVEL privilege, or
			 * (if new level dominates old) P_MACUPGRADE
			 * privilege.
			 */
			if (error = pm_denied(CRED(), P_SETFLEVEL)) {
			    	if (MAC_ACCESS(MACDOM, lid, vp->v_lid) == 0) {
					if (error = pm_denied(CRED(), 
					    P_MACUPGRADE))
						break;
			    	} else 
					break;
			}
		}
		/*
		 * The file system dependent code is responsible
		 * for updating the vnode and inode level.
		 */
		error = VOP_SETLEVEL(vp, lid, CRED());
		break;

	case MAC_GET:
		/*
		 * Must have MAC read access to the file.
		 */
		if ((error = MAC_VACCESS(vp, VREAD, CRED())) != 0)
			break;
#ifdef CC_PARTIAL
                MAC_ASSERT (vp, MAC_DOMINATES); /* MAC read access implies
                                                 * dominates */
#endif
		if (copyout((caddr_t)&vp->v_lid, (caddr_t)usrlevel, 
		    sizeof(lid_t)) == -1)
			error = EFAULT;
		break;
		
	default:
		error = EINVAL;
		break;
	} 

	return error;
}


struct lvlfilea {
	char	*path;
	int	cmd;
	lid_t	*lidp;
};
/*
 *
 * int lvlfile(struct lvlfilea *uap, rval_t *rvp)
 * 	This system call gets or sets the MAC level of a file.
 * 	With the MAC module installed, processing is handled within
 * 	the MAC module by clvlfile().  Without the MAC module
 * 	installed, a process with P_SETFLEVEL privilege can set the
 * 	level of a file on a file system which supports labels.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, lvlfile returns zero.  On faliure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 *
 */
/* ARGSUSED */
int
lvlfile(struct lvlfilea *uap, rval_t *rvp)
{
	vnode_t	*vp;
	int	error;

	ASSERT(getpl() == PLBASE);

	if (!(error = lookupname(uap->path, UIO_USERSPACE, FOLLOW, 
	    NULLVPP, &vp))) {
		/* lidp is copyin by clvlfile(). */
		error = clvlfile(vp, uap->cmd, uap->lidp);
		VN_RELE(vp);
	}
	return error;
}



struct flvlfilea {
	int	fildes;
	int	cmd;
	lid_t	*lidp;
};


/*
 *
 * int flvlfile(struct flvlfilea *uap, rval_t *rvp)
 * 	This is the system call entry point to get or set the level of a 
 *	file by passing a file descriptor. The pathname is resolved to 
 *	a vnode and a common routine is called to handle the 
 *	actual processing.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, flvlfile returns zero.  On faliure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 *
 */
/* ARGSUSED */
int
flvlfile(struct flvlfilea *uap, rval_t *rvp)
{
	file_t	*fp;
	int	error;
	vnode_t	*vp;


	ASSERT(getpl() == PLBASE);
	if (error = getf_mhold(uap->fildes, &fp))
		return error;
	vp = fp->f_vnode;

	/*
	 * Can only set level on a block or character special device.
	 * Otherwise,  return EINVAL.
	 */

	if (uap->cmd == MAC_SET && vp->v_type != VCHR && vp->v_type != VBLK) {
		error = ENODEV;
		goto out;
	}

	/* lidp is copyin by clvlfile(). */

	error = clvlfile(vp, uap->cmd, uap->lidp);
out:
	ADT_GETF(vp);
	GETF_MRELE(fp);
	return error;
}
