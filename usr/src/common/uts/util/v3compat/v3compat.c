/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/v3compat/v3compat.c	1.4"
#ident	"$Header: $"

/*
 * This module contains backward compatibility code for SVR3.2 binaries
 * that read a directory. When this is included the entry point
 * for the read() system call becomes v3read() defined below.
 * Any changes made to fs/vncalls.c read() should also be made here.
 *
 */

#include <acc/dac/acl.h>
#include <acc/mac/cca.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/dirent.h>
#include <fs/fcntl.h>
#include <fs/fifofs/fifonode.h>
#include <fs/file.h>
#include <fs/filio.h>
#include <fs/fs_hier.h>
#include <fs/mode.h>
#include <fs/pathname.h>
#include <fs/s5fs/s5dir.h>
#include <fs/specfs/snode.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/mkdev.h>
#include <io/poll.h>
#include <io/termios.h>
#include <io/ttold.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/exec.h>
#include <proc/proc.h>
#include <proc/resource.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/locking.h>
#include <svc/sco.h>
#include <svc/syscall.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/metrics.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * Read and write.
 */
struct rwa {
	int	fdes;
	void	*cbuf;
	size_t	count;
};

ssize_t	v3read(struct rwa *, rval_t *);

STATIC struct sysent s_v3read = { 3, v3read };

#define	DIRBLKS	1024
#define V3TRUE	1

/*
 * void
 * v3compatinit(void)
 *	Set the entry point for the read() system call to v3read().
 *
 * Calling/Exit State:
 *	No locks are held on entry or on exit.
 */
void
v3compatinit(void)
{
	sysent[SYS_read] = s_v3read;
}

/*
 * STATIC int
 * dircnt(char *buf, int len)
 *
 * Calling/Exit State:
 *	Inode's rwlock held in *shared* mode on entry and exit.
 */
STATIC int
dircnt(char *buf, int len)
{
	int	cnt;

	cnt = 0;
	while (len > 0) {
		struct	dirent	*dp;

		/* LINTED pointer alignment */
		dp = (struct dirent *)buf;
		if (dp->d_ino != 0) {
			cnt++;
		}
		if (dp->d_reclen == 0) {
			return (0);
		}
		buf += dp->d_reclen;
		len -= dp->d_reclen;
	}
	return (cnt);
}

/*
 * STATIC int
 * dir2sysv(char *dir_buf, int dir_len, direct_t *s5)
 *
 * Calling/Exit State:
 *	Inode's rwlock held in *shared* mode on entry and exit.
 */
STATIC int
dir2sysv(char *dir_buf, int dir_len, direct_t *s5)
{
	int	s5cnt;

	s5cnt = 0;
	while (dir_len > 0) {
		struct	dirent	*dp;

		/* LINTED pointer alignment */
		dp = (struct dirent *) dir_buf;
		if (dp->d_ino != 0) {
			s5[s5cnt].d_ino = dp->d_ino;
			{
				int	i;
				char	*p;
				char	*q;

				p = s5[s5cnt].d_name;
				q = dp->d_name;
				for (i = 0; i < 14; i++) {
					if ((p[i] = q[i]) == '\0') {
						break;
					}
				}
				for ( ; i < 14; i++) {
					p[i] = 0;
				}
			}
			s5cnt++;
		}
		if (dp->d_reclen == 0) {
			return (0);
		}
		dir_buf += dp->d_reclen;
		dir_len -= dp->d_reclen;
	}
	return (s5cnt);
}

/*
 * STATIC int
 * dircompat(vnode_t *vp, uio_t *uio, cred_t *cr)
 *
 * Calling/Exit State:
 *	Inode's rwlock held in *shared* mode on entry and exit.
 */
STATIC int
dircompat(vnode_t *vp, uio_t *uio, cred_t *cr)
{
	void	*dir_buf;	/* buffer of dirent structures */
	caddr_t	s5_buf;
	int	dir_index;	/* index of first dirent struct in dir_buf */
	int	dir_len;	/* length of buffer of dirent structures */
	int	eof;
	int	error;
	int	trueflg = 1;
	iovec_t	dir_iov[1];
	uio_t	dir_uio;

	error = 0;
	if (uio->uio_resid <= 0)
		return (NULL);

	/* allocate buffer for block of dirent entries */
	dir_buf = kmem_alloc(DIRBLKS, KM_SLEEP);
	/*
	 * allocate buffer for block of s5 entries;
	 * each dirent takes at least 8 bytes (actually, I think min is 12),
	 * so expansion to s5 format can't use more than 2x space.
	 */
	s5_buf = kmem_alloc (2 * DIRBLKS, KM_SLEEP);

	/* fs-dependent offset is updated in loop by VOP_READDIR */
	dir_uio.uio_offset = 0;
	/* index of first entry is updated in loop */
	dir_index = 0;
	while (trueflg) {
		int	cur_index;

		dir_iov[0].iov_base = dir_buf;
		dir_iov[0].iov_len = DIRBLKS;
		dir_uio.uio_iov = dir_iov;
		dir_uio.uio_iovcnt = 1;
		dir_uio.uio_resid = DIRBLKS;
		dir_uio.uio_segflg = UIO_SYSSPACE;
		dir_uio.uio_fmode = uio->uio_fmode;
		dir_uio.uio_limit = uio->uio_limit;
		error = VOP_READDIR(vp, &dir_uio, cr, &eof);
		if (error) {
			goto out;
		}
		dir_len = DIRBLKS - dir_uio.uio_resid;
		if (dir_len <= 0) {
			goto out;
		}
		cur_index = dircnt(dir_buf, dir_len);
		if (cur_index <= 0) {
			continue;
		}
		cur_index += dir_index;
		if ((cur_index << 4) >= uio->uio_offset) {
			break;
		}
		dir_index = cur_index;
	}

	while (trueflg)
	{
		int	cur_cnt; /* number of entries in the current buffer */
		int	cur_off; /* offset into buffer */
		int	cur_len; /* bytes to copy */

		/*
		 * copy (portion of) this buffer to user
		 */
		/* LINTED pointer alignment */
		cur_cnt = dir2sysv(dir_buf, dir_len, (direct_t *)s5_buf);
		if (cur_cnt > 0) {
			/*
			 * offset into buffer of 1st byte to copy out
			 */
			cur_off = uio->uio_offset - (dir_index << 4);
			/*
			 * number of bytes to copy from this buffer
			 */
			cur_len = (cur_cnt << 4) - cur_off;
			if (cur_len > uio->uio_resid) {
				cur_len = uio->uio_resid;
			}
			error = uiomove(&(s5_buf[cur_off]), cur_len,
					 UIO_READ, uio);
			if (error || uio->uio_resid <= 0) {
				break;
			}
			dir_index += cur_cnt;
		}
		/* read next block */
		dir_iov[0].iov_base = dir_buf;
		dir_iov[0].iov_len = DIRBLKS;
		dir_uio.uio_iov = dir_iov;
		dir_uio.uio_iovcnt = 1;
		dir_uio.uio_resid = DIRBLKS;
		dir_uio.uio_segflg = UIO_SYSSPACE;
		dir_uio.uio_fmode = uio->uio_fmode;
		dir_uio.uio_limit = uio->uio_limit;
		error = VOP_READDIR(vp, &dir_uio, cr, &eof);
		if (error) {
			goto out;
		}
		dir_len = DIRBLKS - dir_uio.uio_resid;
		if (dir_len <= 0) {
			goto out;
		}
	}
out:
	kmem_free(dir_buf, DIRBLKS);
	kmem_free(s5_buf, 2 * DIRBLKS);
	return (error);
}


/*
 * ssize_t
 * read(struct rwa *uap, rval_t *rvp)
 *	SVR3.2 binary read of a directory.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *	Hold inode's rwlock in *shared* mode when calling VOP_READ.
 *
 *	A return value of 0 indicates success; otherwise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EBADF	if valid fd is not open for reading
 *		EINVAL	if count argument passed in is less than 0
 */
ssize_t
v3read(struct rwa *uap, rval_t *rvp)
{
	dl_t	dlong;
	file_t	*fp;
	int	count;
	int	error;
	iovec_t	aiov;
	off_t	toffset;
	pl_t	pl;
	uio_t	auio;
	uio_t	*uio = &auio;
	vnode_t	*vp;

	ASSERT(KS_HOLD0LOCKS());

	error = getf_mhold(uap->fdes, &fp);
	if (error) {
		return (error);
	}
	if ((fp->f_flag & FREAD) == 0) {
		error = EBADF;
		goto error_exit;
	}
	if (uap->count < 0) {
		error = EINVAL;
		goto error_exit;
	}

	MET_READ();
	aiov.iov_base = (caddr_t)uap->cbuf;
	count = uap->count;
	aiov.iov_len = count;
	uio->uio_resid = aiov.iov_len;
	uio->uio_iov = &aiov;
	uio->uio_iovcnt = 1;
	vp = fp->f_vnode;
	uio->uio_fmode = fp->f_flag;
	uio->uio_offset = fp->f_offset;
	uio->uio_segflg = UIO_USERSPACE;
	toffset = uio->uio_offset;

	/* don't need to keep track of uio_limit as not increasing file
	 * size
	 */
	error = VOP_RWRDLOCK(vp, toffset, count, uio->uio_fmode);
	if (error) {
		goto error_exit;
	}
	if ((vp->v_type == VDIR) && ((RENV & RE_RENVMASK) != RE_ISELF)) {
		error = dircompat(vp, uio, fp->f_cred);
	} else {
		error = VOP_READ(vp, uio, 0, fp->f_cred);
	}
	VOP_RWUNLOCK(vp, toffset, count);

	if (error == EINTR && uio->uio_resid != count) {
		error = 0;
	}
	rvp->r_val1 = count - uio->uio_resid;
	dlong.dl_hop = 0L;
	dlong.dl_lop = (unsigned)rvp->r_val1;
	u.u_ioch = ladd(u.u_ioch, dlong);

	if (vp->v_type == VFIFO) {	/* Backward compatibility */
		fp->f_offset = rvp->r_val1;
	} else {
		fp->f_offset = uio->uio_offset;
	}
	MET_READCH((unsigned)rvp->r_val1);
	if (vp->v_vfsp != NULL) {
		vp->v_vfsp->vfs_bcount += rvp->r_val1 >> SCTRSHFT;
	}
error_exit:
	GETF_MRELE(fp);
	return (error);
}
