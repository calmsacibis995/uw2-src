/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/xnamfs/xnamvnops.c	1.8"
#ident	"$Header: $"

#include <util/types.h>
#include <util/param.h>
#include <svc/systm.h>
#include <fs/buf.h>
#include <util/cmn_err.h>
#include <io/conf.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <svc/time.h>
#include <fs/fcntl.h>
#include <fs/flock.h>
#include <fs/file.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <proc/mman.h>
#include <io/open.h>
#include <mem/swap.h>
#include <util/sysmacros.h>
#include <io/uio.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/poll.h>
#include <io/stream.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <proc/session.h>
#include <fs/xnamfs/xnamnode.h>
#include <fs/xnamfs/xnamhier.h>
#include <mem/seg.h>
#include <mem/seg_map.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/seg_dev.h>
#include <mem/seg_vn.h>
#include <fs/fs_subr.h>

extern void	xsd_destroy(xnamnode_t *);
extern void	xsem_unalloc(xnamnode_t *);
extern int	xnamdelete(xnamnode_t *);

STATIC int	xnam_open(vnode_t **, int, cred_t *);
STATIC int	xnam_close(vnode_t *, int, boolean_t, off_t, cred_t *);
STATIC int	xnam_getattr(vnode_t *, vattr_t *, int, cred_t *);
STATIC int	xnam_setattr(vnode_t *, vattr_t *, int, int, cred_t *);
STATIC int	xnam_access(vnode_t *, int, int, cred_t *);
STATIC int	xnam_fsync(vnode_t *, cred_t *);
STATIC void	xnam_inactive(vnode_t *, cred_t *);
STATIC int	xnam_fid(vnode_t *, fid_t **);
STATIC int	xnam_realvp(vnode_t *, vnode_t **);
STATIC int	xnam_rwlock(vnode_t *, off_t, int, int, int);
STATIC void	xnam_rwunlock(vnode_t *, off_t, int);

vnodeops_t xnam_vnodeops = {
	xnam_open,
	xnam_close,
	(int (*)())fs_nosys,	/* read */
	(int (*)())fs_nosys,	/* write */
	(int (*)())fs_nosys,	/* ioctl */
	fs_setfl,
	xnam_getattr,
	xnam_setattr,
	xnam_access,
	(int (*)())fs_nosys,	/* lookup */
	(int (*)())fs_nosys,	/* create */
	(int (*)())fs_nosys,	/* remove */
	(int (*)())fs_nosys,	/* link */
	(int (*)())fs_nosys,	/* rename */
	(int (*)())fs_nosys,	/* mkdir */
	(int (*)())fs_nosys,	/* rmdir */
	(int (*)())fs_nosys,	/* readdir */
	(int (*)())fs_nosys,	/* symlink */
	(int (*)())fs_nosys,	/* readlink */
	xnam_fsync,
	xnam_inactive,
	(void (*)())fs_nosys,	/* release */
	xnam_fid,
	xnam_rwlock,
	xnam_rwunlock,
	(int (*)())fs_nosys,	/* seek */
	fs_cmp,
	(int (*)())fs_nosys,	/* frlock */
	xnam_realvp,
	(int (*)())fs_nosys,	/* getpage */
	(int (*)())fs_nosys,	/* putpage */
	(int (*)())fs_nosys,	/* map */
	(int (*)())fs_nosys,	/* addmap */
	(int (*)())fs_nosys,	/* delmap */
	(int (*)())fs_nosys,	/* poll */
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
 * STATIC int
 * xnam_open(vnode_t **vpp, int flag, cred_t *cr)
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Returns EISNAM since semaphore and shared data files
 *	should not be opened.
 */
/*ARGSUSED*/
STATIC int
xnam_open(vnode_t **vpp, int flag, cred_t *cr)
{
	return (EISNAM);
}

/*
 * STATIC int
 * xnam_close(vnode_t *vp, int flag, boolean_t lastclose,
 *	      off_t offset, cred_t *cr)
 *	Close an xnamnode.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Hold the xnamnode's mutex lock while decrementing x_count.
 */
/* ARGSUSED */
STATIC int
xnam_close(vnode_t *vp, int flag, boolean_t lastclose,
	   off_t offset, cred_t *cr)
{
	pl_t	   pl;
	xnamnode_t *xp;

	if (!lastclose) {
		return 0;
	}

	xp = VTOXNAM(vp);
	pl = XNODE_LOCK(xp);
	xp->x_count--;		/* one fewer open reference */
	XNODE_UNLOCK(xp, pl);

	return 0;
}


/*
 * STATIC int
 * xnam_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
 *	Return the attributes for a vnode.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Calls VOP_GETATTR to get the attributes of the real vnode.
 *	Holds the xnamnode's mutex lock while setting the AT_TIMES
 *	attributes from the xnamnode.
 */
STATIC int
xnam_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
{
	int	   error;
	pl_t	   pl;
	xnamnode_t *xp;

	xp = VTOXNAM(vp);
	error = VOP_GETATTR(xp->x_realvp, vap, flags, cr);
	if (error) {
		return (error);
	}

	pl = XNODE_LOCK(xp);
	vap->va_atime.tv_sec = xp->x_atime;
	vap->va_atime.tv_nsec = 0;
	vap->va_mtime.tv_sec = xp->x_mtime;
	vap->va_mtime.tv_nsec = 0;
	vap->va_ctime.tv_sec = xp->x_ctime;
	vap->va_ctime.tv_nsec = 0;
	XNODE_UNLOCK(xp, pl);
	return (0);
}

/*
 * STATIC int
 * xnam_setattr(vnode_t *vp, vattr_t *vap, int flags, int ioflags, cred_t *cr)
 *	Sets the attributes for a vnode.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Calls VOP_SETATTR to set the attributes of the real vnode.
 *	If the times fields were changed, hold the xnamnode's mutex
 *	lock while setting the AT_TIMES fields of the xnamnode.
 */
STATIC int
xnam_setattr(vnode_t *vp, vattr_t *vap, int flags, int ioflags, cred_t *cr)
{
	int	   chtime;
	int	   error;
	pl_t	   pl;
	xnamnode_t *xp;

	chtime = 0;
	xp = VTOXNAM(vp);
	error = VOP_SETATTR(xp->x_realvp, vap, flags, ioflags, cr);
	if (error == 0) {
		/*
		 * If times were changed, update xnamnode.
		 */
		pl = XNODE_LOCK(xp);
		if (vap->va_mask & AT_ATIME) {
			xp->x_atime = vap->va_atime.tv_sec;
			chtime++;
		}
		if (vap->va_mask & AT_MTIME) {
			xp->x_mtime = vap->va_mtime.tv_sec;
			chtime++;
		}
		if (chtime) {
			xp->x_ctime = hrestime.tv_sec;
			xp->x_mode  = vap->va_mode;
			xp->x_uid   = vap->va_uid;
			xp->x_gid   = vap->va_gid;
		}
		XNODE_UNLOCK(xp, pl);
	}
	return (error);
}

/*
 * STATIC int
 * xnam_access(vnode_t *vp, int mode, int flags, cred_t *cr)
 *	Perform access checks for the vnode.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Calls VOP_ACCESS for access checks. Holds the xnamnode's
 *	shared/exclusive sleep lock in shared mode during this call.
 */
STATIC int
xnam_access(vnode_t *vp, int mode, int flags, cred_t *cr)
{
	int	   error;
	xnamnode_t *xp;

	xp = VTOXNAM(vp);
	SLEEP_LOCK(&xp->x_rwlock, PRINOD);
	error = VOP_ACCESS(xp->x_realvp, mode, flags, cr);
	SLEEP_UNLOCK(&xp->x_rwlock);

	return (error);
}

/*
 * STATIC int
 * xnam_fsync(vnode_t *vp, cred_t *cr)
 *	Perform a synchronous write on the vnode.
 *
 * Calling/Exit State:
 *	No locking is held on entry or exit.
 *
 * Description:
 *	Holds the xnamnode mutex lock while checking if the times
 *	has changed or not. Sets the time stamps in the real vnode
 *	to the greater of the access/modification times fields from
 *	the xnamnode or the real vnode.
 */
STATIC int
xnam_fsync(vnode_t *vp, cred_t *cr)
{
	pl_t	   pl;
	xnamnode_t *xp;
	vattr_t	   va;
	vattr_t	   vatmp;

	xp = VTOXNAM(vp);

	pl = XNODE_LOCK(xp);
	/* If times didn't change, don't flush anything. */
	if ((xp->x_flag & (XNAMACC|XNAMUPD|XNAMCHG)) == 0) {
		XNODE_UNLOCK(xp, pl);
		return 0;
	}
	XNODE_UNLOCK(xp, pl);

	vatmp.va_mask = AT_TIMES;
	if (VOP_GETATTR(xp->x_realvp, &vatmp, 0, cr) == 0) {
		pl = XNODE_LOCK(xp);
		if (vatmp.va_atime.tv_sec > xp->x_atime)
			va.va_atime = vatmp.va_atime;
		else {
			va.va_atime.tv_sec = xp->x_atime;
			va.va_atime.tv_nsec = 0;
		}
		if (vatmp.va_mtime.tv_sec > xp->x_mtime)
			va.va_mtime = vatmp.va_mtime;
		else {
			va.va_mtime.tv_sec = xp->x_mtime;
			va.va_mtime.tv_nsec = 0;
		}
		XNODE_UNLOCK(xp, pl);

		va.va_mask = AT_TIMES;
		(void) VOP_SETATTR(xp->x_realvp, &va, 0, 0, cr);
	}
	(void) VOP_FSYNC(xp->x_realvp, cr);
	return 0;
}

/*
 * STATIC void
 * xnam_inactive(vnode_t *vp, cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or exit.
 *
 * Description:
 *	Locks the xnamnode rwlock exclusive. Calls xnamdelete to
 *	remove the xnamnode from the hash table. Returns if this
 *	fails. Sync's the file system and frees the xnamnode.
 */
STATIC void
xnam_inactive(vnode_t *vp, cred_t *cr)
{
	xnamnode_t *xp;

	xp = VTOXNAM(vp);

	SLEEP_LOCK(&xp->x_rwlock, PRINOD);
	VN_LOCK(vp);
	if (vp->v_count != 1) {
		vp->v_count--;
		VN_UNLOCK(vp);
		XNODE_RWUNLOCK(xp);
		return;
	}
	vp->v_count = 0;
	VN_UNLOCK(vp);
	ASSERT(vp->v_count == 0);

	if (xnamdelete(xp) != 0) {
		return;
	}

	if (vp->v_type == VXNAM) {
		if (vp->v_rdev == XNAM_SEM && xp->x_sem) {
			xsem_unalloc(xp);
		} else {
			if (vp->v_rdev == XNAM_SD && xp->x_sd)
				xsd_destroy(xp);
		}
	}
	(void) xnam_fsync(vp, cr);
	VN_RELE(xp->x_realvp);
	xp->x_realvp = NULL;

	VN_DEINIT(vp);
	SLEEP_DEINIT(&xp->x_rwlock);
	LOCK_DEINIT(&xp->x_mutex);
	kmem_free((caddr_t)xp, sizeof (xnamnode_t));
}

/*
 * STATIC int
 * xnam_fid(vnode_t *vp, fid_t **fidpp)
 *	Generate a unique identifier for the vnode.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Calls VOP_FID if shadowing a vnode; returns EINVAL if not.
 */
STATIC int
xnam_fid(vnode_t *vp, fid_t **fidpp)
{
	xnamnode_t *xp;

	xp = VTOXNAM(vp);
	if (xp->x_realvp) {
		return (VOP_FID(xp->x_realvp, fidpp));
	} else {
		return (EINVAL);
	}
}

/*
 * STATIC int
 * xnam_realvp(vnode_t *vp, vnode_t **vpp)
 *
 * Calling/Exit State:
 *      No locks are held on entry or exit.
 *
 * Description:
 *	If there is a realvp associated with vp, return it.
 */
STATIC int
xnam_realvp(vnode_t *vp, vnode_t **vpp)
{
	xnamnode_t *xp;
	vnode_t	   *rvp;

	xp = VTOXNAM(vp);
	vp = xp->x_realvp;
	if (vp && VOP_REALVP(vp, &rvp) == 0)
		vp = rvp;
	*vpp = vp;
	return (0);
}

/*
 * int
 * xnam_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
 *	Lock a xnamnode.
 *
 * Calling/Exit State:
 *	No lock is held on entry; the xnamnode rwlock is held in
 * 	shared or exclusive mode depending on "mode" at exit.
 *
 */
/* ARGSUSED */
STATIC int
xnam_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
{
	xnamnode_t *xp;

	xp = VTOXNAM(vp);
	SLEEP_LOCK(&xp->x_rwlock, PRINOD);
	return (0);
}

/*
 * void
 * xnam_rwunlock(vnode_t *vp, off_t off, int len)
 *	Unlock a xnamnode.
 *
 * Calling/Exit State:
 *	The xnamnode rwlock is held on entry and unlocked at exit.
 */
/* ARGSUSED */
STATIC void
xnam_rwunlock(vnode_t *vp, off_t off, int len)
{
	xnamnode_t *xp;

	xp = VTOXNAM(vp);
	XNODE_RWUNLOCK(xp);
}

