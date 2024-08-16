/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/fs_subr.c	1.13"
#ident	"$Header: $"

/*
 * Generic vnode operations.
 */
#include <acc/mac/mac.h>
#include <fs/fcntl.h>
#include <fs/flock.h>
#include <fs/statvfs.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/poll.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/unistd.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/types.h>
#include <util/param.h>

/*
 * int
 * fs_nosys(void)
 *	The associated operation is not supported by the file system.
 *
 * Calling/Exit State:
 *	No locks should be held on entry or exit.
 *
 *	Returns ENOSYS when called.
 */
int
fs_nosys(void)
{
	return (ENOSYS);
}

/*
 * int
 * fs_sync(vfs_t *vfsp, int flag, cred_t *cr)
 *	The file system has nothing to sync to disk.  However, the
 *	VFS_SYNC operation must not fail.
 *
 * Calling/Exit State:
 *	No locks should be held on entry or exit.
 */
/* ARGSUSED */
int
fs_sync(vfs_t *vfsp, int flag, cred_t *cr)
{
	return (0);
}

/*
 * int
 * fs_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
 *	Read/write lock.  Does nothing.
 *
 * Calling/Exit State:
 *	No locks should be held on entry or exit.
 */
/* ARGSUSED */
int
fs_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
{
	return (0);
}

/*
 * fs_rwunlock(vnode_t *vp, off_t off, int len)
 *	Read/write unlock.
 *
 * Calling/Exit State:
 *	No locks should be held on entry or exit.
 */
/* ARGSUSED */
void
fs_rwunlock(vnode_t *vp, off_t off, int len)
{
	return;
}

/*
 * int
 * fs_cmp(vnode_t *vp1, vnode_t *vp2)
 *	Compare two vnodes.
 *
 * Calling/Exit State:
 *	No locks should be held on entry or exit.
 *
 *	Returns result of comparison.
 */
int
fs_cmp(vnode_t *vp1, vnode_t *vp2)
{
	return (vp1 == vp2);
}

/*
 * int
 * fs_frlock(vnode_t *vp, int cmd, flock_t *bfp, int flag, off_t offset,
 *	     cred_t *cr, off_t size)
 *	File and record locking.
 *
 * Calling/Exit State:
 *	The inode's rwlock is held exclusive or shared (depending
 *	on cmd) on entry and on exit.
 */
/* ARGSUSED */
int
fs_frlock(vnode_t *vp, int cmd, flock_t *bfp, int flag, off_t offset,
	  cred_t *cr, off_t size)
{
	int	frcmd;

	switch (cmd) {

	case F_GETLK: 
	case F_O_GETLK: 
		bfp->l_pid = u.u_procp->p_epid;
		bfp->l_sysid = u.u_procp->p_sysid;
		frcmd = 0;
		break;

	case F_RGETLK: 
		frcmd = RCMDLCK;
		break;

	case F_SETLK: 
		bfp->l_pid = u.u_procp->p_epid;
		bfp->l_sysid = u.u_procp->p_sysid;
		frcmd = SETFLCK;
		break;

	case F_RSETLK: 
		frcmd = SETFLCK|RCMDLCK;
		break;

	case F_SETLKW: 
		bfp->l_pid = u.u_procp->p_epid;
		bfp->l_sysid = u.u_procp->p_sysid;
		frcmd = SETFLCK|SLPFLCK;
		break;

	case F_RSETLKW: 
		frcmd = SETFLCK|SLPFLCK|RCMDLCK;
		break;
		
	default:
		return EINVAL;
	}
	return (reclock(vp, bfp, frcmd, flag, offset, size));
}

/*
 * int
 * fs_setfl(vnode_t *vp, u_int oflags, u_int nflags, cred_t *cr)
 *	Allow any flags.
 *
 * Calling/Exit State:
 *	No locks should be held on entry or exit.
 */
/* ARGSUSED */
int
fs_setfl(vnode_t *vp, u_int oflags, u_int nflags, cred_t *cr)
{
	return (0);
}

/*
 * int
 * fs_poll(vnode_t *vp, int events, int anyyet, short *reventsp,
 *	   struct pollhead **phpp)
 *	Return the answer requested to poll() for non-device files.
 *
 * Calling/Exit State:
 *	No locks should be held on entry or exit.
 *
 *	Only POLLIN, POLLRDNORM, and POLLOUT are recognized.
 */
/* ARGSUSED */
int
fs_poll(vnode_t *vp, int events, int anyyet,
	short *reventsp, struct pollhead **phpp)
{
	*reventsp = 0;
	if (events & POLLIN)
		*reventsp |= POLLIN;
	if (events & POLLRDNORM)
		*reventsp |= POLLRDNORM;
	if (events & POLLOUT)
		*reventsp |= POLLOUT;
	*phpp = (struct pollhead *)NULL;
	return (0);
}

/*
 * int
 * fs_vcode(vnode_t *vp, u_long *vcp)
 *	vcp is an in/out parameter.  Updates *vcp with a version code
 *	suitable for the va_vcode attribute, possibly the value passed in.
 *
 * Calling/Exit State:
 *	The va_vcode attribute is intended to support cache coherency
 *	and IO atomicity for file servers that provide traditional
 *	UNIX file system semantics.  The vnode of the file object
 *	whose va_vcode is being updated must be held locked when
 *	this function is evaluated.
 *
 *	Returns 0 for success, a nonzero errno for failure.
 */
/* ARGSUSED */
int
fs_vcode(vnode_t *vp, u_long *vcp)
{
#ifdef RFS
	static u_long		vcode;
	u_long		error = 0;

	extern int		rf_state;
	/*
	 * RFS hooks here.
	 */

	if (*vcp == 0 || rf_state) {
		if (vcode == (u_long)~0) {
			/*
			 *+ version code attribute overflow
			 */
			cmn_err(CE_WARN, "fs_vcode: vcode overflow\n");
			error = ENOMEM;
		} else {
			register u_long	tvcode;

			*vcp = tvcode = ++vcode;
			if (rf_state) {
				extern void rfc_inval();
				
				rfc_inval(vp, tvcode);
			}
		}
	}
	return error;
#else /* FS_RFS */
	return (0);
#endif
}

/*
 * int
 * fs_pathconf(vnode_t *vp, int cmd, u_long *valp, cred_t *cr)
 *	POSIX pathconf() support.
 *
 * Calling/Exit State:
 *	No locks should be held on entry or exit.
 */
/* ARGSUSED */
int
fs_pathconf(vnode_t *vp, int cmd, u_long *valp, cred_t *cr)
{
	u_long val;
	int error = 0;
	struct statvfs vfsbuf;
	extern short maxlink;
	extern int fifoblksize;

	switch (cmd) {

	case _PC_LINK_MAX:
		val = maxlink;
		break;

	case _PC_MAX_CANON:
		val = MAX_CANON;
		break;

	case _PC_MAX_INPUT:
		val = MAX_INPUT;
		break;

	case _PC_NAME_MAX:
		struct_zero((caddr_t)&vfsbuf, sizeof(vfsbuf));
		if (error = VFS_STATVFS(vp->v_vfsp, &vfsbuf))
			break;
		val = vfsbuf.f_namemax;
		break;

	case _PC_PATH_MAX:
		val = MAXPATHLEN;
		break;

	case _PC_PIPE_BUF:
		val = fifoblksize;
		break;

	case _PC_NO_TRUNC:
		if (vp->v_vfsp->vfs_flag & VFS_NOTRUNC)
			val = 1;	/* NOTRUNC is enabled for vp */
		else
			val = (u_long)-1;
		break;

	case _PC_VDISABLE:
		val = _POSIX_VDISABLE;
		break;

	case _PC_CHOWN_RESTRICTED:
		if (rstchown)
			val = rstchown;		/* chown restricted enabled */
		else
			val = (u_long)-1;
		break;

	default:
		error = EINVAL;
		break;
	}

	if (error == 0)
		*valp = val;
	return (error);
}

/*
 * fs_itoh(lid_t lid, char *str)
 *	Translate a lid to the corresponding character hexidecimal
 *	representation, and return it (null terminated) in the string
 *	referenced by "str".
 *
 * Calling/Exit State:
 *	No locks should be held on entry or exit.
 *
 *	This is called when deflecting through an MLD to find the
 *	effective directory corresponding to the lid on the process.
 *	Note that the string referenced by "str" must be at least of
 *	MLD_SZ size.
 */
/* ARGSUSED */
void
fs_itoh(lid_t lid, char *str)
{
	char x[MLD_SZ];
	char *charp = &x[MLD_SZ];
	*charp = '\0';
	if (lid == 0) {
		*--charp = '0';
	} else {
		do {
			*--charp = "0123456789ABCDEF"[lid & 0x0000000f];
		} while (lid >>= 4);
	}

	while (*str++ = *charp++)
		continue;
}
