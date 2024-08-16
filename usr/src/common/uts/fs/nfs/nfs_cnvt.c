/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_cnvt.c	1.9"
#ident	"$Header: $"

/*
 *	nfs_cnvt.c, routines for conversion of file handle to open file
 *	descriptor and vice versa.
 */

#include <util/param.h>
#include <util/types.h>
#include <acc/priv/privilege.h>
#include <svc/systm.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/file.h>
#include <proc/proc.h>
#include <util/debug.h>
#include <net/rpc/types.h>

#define NFSSERVER

#include <fs/nfs/nfs.h>
#include <fs/nfs/nfssys.h>

/*
 * nfs_cnvt(arg, rvp)
 *	Convert and nfs file handle to descriptor.
 *
 * Calling/Exit State:
 *	Returns 0 on success, error on failure.
 *
 * Description:
 *	Given a pointer to an fhandle_t and a mode, this routine opens
 *	the file corresponding to the fhandle_t with the given mode and
 *	returns a file descriptor.
 *
 * Parameters:
 *
 *	arg			# args to this routine
 *	rvp			# rval struct to return stuff in
 *
 */
/*ARGSUSED*/
int
nfs_cnvt(struct nfs_cnvt_args *arg, rval_t *rvp)
{
	struct file		*fp;
	struct vnode		*vp;
	struct nfs_cnvt_args	a;
	struct nfs_cnvt_args	*ap;
	fhandle_t		tfh;
	int			filemode;
	int			mode;
	int			fd;
	int			error;
	struct	vnode		*myfhtovp();

	NFSLOG(0x10, "nfs_cnvt: entered\n", 0, 0);

	/*
	 * must have owner privelege
	 */
	if (pm_denied(u.u_lwpp->l_cred, P_OWNER))
		return (EPERM);

	/*
	 * copyin the args and file handle
	 */
	if (copyin((caddr_t) arg, (caddr_t) &a, sizeof(a))) {
		return (EFAULT);
	} else {
		ap = &a;
	}
	if (copyin((caddr_t) ap->fh, (caddr_t) &tfh, sizeof(tfh))) {
		return (EFAULT);
	}

	/*
	 * must have FCREAT in file mode
	 */
	filemode = ap->filemode - FOPEN;
	if (filemode & FCREAT)
		return (EINVAL);

	mode = 0;
	if (filemode & FREAD)
		mode |= VREAD;
	if (filemode & (FWRITE | FTRUNC))
		mode |= VWRITE;

	/*
	 * allocate a file struct, adapted from copen().
	 */
	error = falloc((struct vnode *)NULL, filemode & FMASK, &fp, &fd);
	if (error)
		return error;

	/*
	 * this takes the place of lookupname in copen. Note that
	 * we can't use the normal fhtovp function because we want
	 * this to work on files that may not have been exported.
	 */
	if ((vp = myfhtovp(&tfh)) == (struct vnode *) NULL) {

		NFSLOG(0x10, "nfs_cnvt: stale handle\n", 0, 0);
	
		error = ESTALE;
		goto out;
	}

	/*
	 * adapted from vn_open().
	 */
	if (filemode & (FWRITE | FTRUNC)) {
		struct vattr vattr;

		if (vp->v_type == VDIR) {
			error = EISDIR;
			goto out;
		}
		if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
			error = EROFS;
			goto out;
		}

		/*
		 * can't truncate files on which mandatory locking
		 * is in effect.
		 */
		if ((filemode & FTRUNC) && vp->v_filocks != NULL) {
			vattr.va_mask = AT_MODE;
			if ((error = VOP_GETATTR(vp, &vattr, 0,
				u.u_lwpp->l_cred)) == 0 &&
					MANDLOCK(vp, vattr.va_mode))
				error = EAGAIN;
		}

		if (error)
			goto out;
	}

	/*
	 * check permissions.
	 * must have read and write permission to open
	 * a file for private access.
	 */
	if (error = VOP_ACCESS(vp, mode, 0, u.u_lwpp->l_cred)) {

		NFSLOG(0x10, "nfs_cnvt: access problems\n", 0, 0);

		goto out;
	}

	/*
	 * open the file
	 */
	error = VOP_OPEN(&vp, filemode, u.u_lwpp->l_cred);
	if ((error == 0) && (filemode & FTRUNC)) {
		struct vattr vattr;

		vattr.va_size = 0;
		vattr.va_mask = AT_SIZE;
		if ((error = VOP_SETATTR(vp, &vattr, 0, filemode,
					u.u_lwpp->l_cred)) != 0) {
			(void)VOP_CLOSE(vp, filemode, 1, 0, u.u_lwpp->l_cred);
		}
	}
	if (error)
		goto out;

	/*
	 * copyout the descriptor. also bind to the fd entry.
	 */
	fp->f_vnode = vp;
	setf(fd, fp);
	if (copyout((caddr_t) &fd, (caddr_t) ap->fd, sizeof(fd))) {
		error = EFAULT;
	} else {
		NFSLOG(0x10, "nfs_cnvt: no errors\n", 0, 0);

		return (0);
	}

out:
	if (fp) {
		setf(fd, NULLFP);
		unfalloc(fp);
	}
	if (vp)
		VN_RELE(vp);

	NFSLOG(0x10, "nfs_cnvt: returning error %d\n", error, 0);

	return (error);
}

/*
 * myfhtovp(fh)
 *	Convert file handle to a vnode.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Description:
 *	Returns pointer to vnode on success, null on failure.
 *	We require this version of fhtovp that simply converts
 *	an fhandle_t to a vnode without any ancillary checking
 *	(e.g., whether it's exported).
 *
 * Parameters:
 *
 *	fh			# file handle to convert
 *
 */
struct vnode *
myfhtovp(fhandle_t *fh)
{
	struct	vnode	*vp;
	struct	vfs	*vfsp;
	int		error;

	NFSLOG(0x10, "myfhtovp: entered\n", 0, 0);

	/*
	 * get pointer to vfs first
	 */
	vfsp = getvfs(&fh->fh_fsid);
	if (vfsp == (struct vfs *) NULL) {
		return ((struct vnode *) NULL);
	}

	/*
	 * now get the vnode
	 */
	error = VFS_VGET(vfsp, &vp, (struct fid *)&(fh->fh_len));
	if (error || vp == (struct vnode *) NULL) {
		return ((struct vnode *) NULL);
	}

	return (vp);
}
