/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/xnamfs/xnamsubr.c	1.8"

#include <util/types.h>
#include <util/param.h>
#include <svc/systm.h>
#include <fs/buf.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <mem/kmem.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/file.h>
#include <fs/xnamfs/xnamnode.h>
#include <fs/xnamfs/xnamhier.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <svc/clock.h>
#include <util/sysmacros.h>
#include <util/cmn_err.h>
#include <util/mod/moddefs.h>

STATIC dev_t	  xnamdev;
int	xnamfstype;

STATIC int   xnam_load(void);
STATIC int   xnam_unload(void);

unsigned long xnam_fsflags = 0;

MOD_FS_WRAPPER(xnam, xnam_load, xnam_unload, "XNAMFS: XENIX Sem/Sh Mem");

extern void	  xsdinit();
extern void	  xsem_init();
extern vfsops_t   xnam_vfsops;

/*
 * xnamnode hash table lock
 */
lock_t		xnam_table_mutex;
lkinfo_t	xnam_table_lkinfo;
LKINFO_DECL(xnam_table_lkinfo, "FS:XNAMFS:xnamnode hash table mutex", 0);

/*
 * xnamnode lockinfo
 */
lkinfo_t	xnam_mutex_lkinfo;
LKINFO_DECL(xnam_mutex_lkinfo, "FS:XNAMFS:xnamnode mutex", 0);
lkinfo_t	xnam_rwlock_lkinfo;
LKINFO_DECL(xnam_rwlock_lkinfo, "FS:XNAMFS:xnamnode rwlock", 0);

xnamnode_t	  *xnamtable[XNAMTBLSIZE];

/*
 * xnamnode lookup stuff.
 * These routines maintain a table of xnamnodes indexed by dev so
 * that the xnamnode for a name file can be found if it already exists.
 */


/*
 * STATIC int
 * xnam_load(void)
 *	Initialize lock, XENIX semaphore and shared data free lists
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 */
STATIC int
xnam_load(void)
{
	struct	vfssw	*vswp;
	
	vswp = vfs_getvfssw("xnamfs");
	if (vswp == NULL) {
		/*
                 *+ XNAMFS file system is not registered before
                 *+ attempting to load it.
                 */
                cmn_err(CE_NOTE, "!MOD: XNAMFS is not registered.");
                return (EINVAL);
	}
	xnamfstype = vswp - vfssw;
	
	LOCK_INIT(&xnam_table_mutex, FS_XNTBLHIER, FS_XNTBLPL,
		  &xnam_table_lkinfo, KM_SLEEP);

	if ((xnamdev = getudev()) == NODEV)
		xnamdev = 0;

	/*
	 * Initialize XENIX semaphore and shared data free lists
	 */
	xsdinit();
        xsem_init();
        return 0;
}

/*
 * STATIC int
 * xnam_unload(void)
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 */
STATIC int
xnam_unload(void)
{
	LOCK_DEINIT(&xnam_table_mutex);
	return (0);
}

/*
 * STATIC xnamnode_t *
 * xnamfind(dev_t dev, vnode_t *vp)
 *	Lookup an xnamnode by <dev, vp>.
 *
 * Calling/Exit State:
 *	The xnamnode hash table lock is held on entry. If a
 *	If a match is found in the xnamnode hash table, the
 *	xnamnode rwlock is queued for while dropping the hash
 *	table lock atomically. Otherwise, the hash table lock
 *	is held on exit.
 *
 * Description:
 *	While holding the xnamnode lock, an additional
 *	reference is added to the vnode.
 */
STATIC xnamnode_t *
xnamfind(dev_t dev, vnode_t *vp)
{
	vnode_t	   *xvp;
	xnamnode_t *xt;

	xt = xnamtable[dev - 1];
	while (xt != (xnamnode_t *)NULL) {
		xvp = XNAMTOV(xt);
		if (VN_CMP(xt->x_realvp, vp) && (vp != NULL)) {
			/*
			 * Holding this lock prevents xp
			 * from being deleted by xnamdelete().
			 */
			XNODE_RWLOCK(xt, &xnam_table_mutex);
			VN_HOLD(xvp);
			XNODE_RWUNLOCK(xt);
			return (xt);
		}
		xt = xt->x_next;
	}
	return (NULL);
}

/*
 * STATIC void
 * xnaminsert(xnamnode *xp, xnamnode_t **nxp, vnode_t *vp)
 *	Put a xnamnode in the table.
 *
 * Calling/Exit State:
 *	No lock is held on entry or exit.
 *
 * Description:
 *	The xnamnode hash table lock will be held while adding the
 *	xnamnode to the hash table. Calls xnamfind() to search the
 *	hash table for this xnamnode.  If one is found, someone raced
 *	and won so simply return. Otherwise, add this xnamnode to the
 *	hash table and unlock the hash table lock.
 */
STATIC void
xnaminsert(xnamnode_t *xp, xnamnode_t **nxp, vnode_t *vp)
{
	dev_t	   dev;
	pl_t	   pl;

	dev = xp->x_dev;
	/* Lock hash table */
	pl = XNAMTBL_LOCK();
	*nxp = xnamfind(dev, vp);
	if (*nxp != (xnamnode_t *)NULL) {
		return;
	}
	xp->x_next = xnamtable[XNAMTBLHASH(dev)];
	xnamtable[XNAMTBLHASH(dev)] = xp;
	XNAMTBL_UNLOCK(pl);
}

/*
 * STATIC int
 * xnamdelete(xnamnode *xp)
 *	Remove an xnamnode from the table.
 *
 * Calling/Exit State:
 *	The xnamnode rwlock is held on entry and unlocked at exit.
 *
 * Description:
 *	The xnamnode hash table lock will be held while deleting the
 *	entry from the hash table. It is unlocked before exit.
 */
int
xnamdelete(xnamnode_t *xp)
{
	dev_t	   dev;
	pl_t	   pl;
	xnamnode_t *xt;
	xnamnode_t *xtprev;

	dev = xp->x_dev;
	xtprev = NULL;
	/* Lock hash table */
	pl = XNAMTBL_LOCK();
	xt = xnamtable[XNAMTBLHASH(dev)];
	while (xt != NULL) {
		if (xt == xp) {
			if (xtprev == NULL) {
				xnamtable[XNAMTBLHASH(dev)] = xt->x_next;
			} else {
				xtprev->x_next = xt->x_next;
			}
			break;
		}
		xtprev = xt;
		xt = xt->x_next;
	}
	XNAMTBL_UNLOCK(pl);
	XNODE_RWUNLOCK(xp);
	return (0);
}

/*
 * vnode_t *
 * xnamvp(vnode_t *vp, cred_t *cr)
 *	Return a special XENIX name vnode for the given sub-type.
 *
 * Calling/Exit State:
 *	No lock is held on entry or exit.
 *
 * Description:
 *	If no xnamnode exists for this sub-type, create one and put it
 *	in a table indexed by dev. If the xnamnode for this dev is
 *	already in the table return it (ref count is incremented by
 *	xnamfind). The xnamnode will be flushed from the table when
 *	xnam_inactive calls xnamdelete.
 */
vnode_t *
xnamvp(vnode_t *vp, cred_t *cr)
{
	xnamnode_t *xp, *nxp;
	vnode_t    *xvp;
	dev_t	   dev;
	pl_t	   pl;
	vattr_t	   va;

	dev = vp->v_rdev;
	if (vp == NULL || (dev-1) >= XNAMTBLSIZE)
		return (NULL);

	pl = XNAMTBL_LOCK();
	xp = xnamfind(dev, vp);
	if (xp == NULL) {
		XNAMTBL_UNLOCK(pl);
		xp = (xnamnode_t *)kmem_zalloc(sizeof(*xp), KM_SLEEP);
		XNAMTOV(xp)->v_op = &xnam_vnodeops;

		/*
		 * Init the times in the xnamnode to those in the vnode.
		 */
		va.va_mask = (AT_TIMES|AT_FSID|AT_MODE|AT_UID|AT_GID);
		if (VOP_GETATTR(vp, &va, 0, cr) == 0) {
			xp->x_atime = va.va_atime.tv_sec;
			xp->x_mtime = va.va_mtime.tv_sec;
			xp->x_ctime = va.va_ctime.tv_sec;
			xp->x_fsid  = va.va_fsid;
			xp->x_mode  = va.va_mode;
			xp->x_uid   = va.va_uid;
			xp->x_gid   = va.va_gid;
		} else {
			xp->x_fsid  = xnamdev;
		}
		xp->x_realvp = vp;
		xvp = XNAMTOV(xp);

		/* initialize vnode */

		VN_INIT(xvp, vp->v_vfsp, VXNAM, dev, 0, KM_SLEEP);
		xvp->v_data = (caddr_t)xp;

		xvp->v_macflag |= (VMAC_DOPEN | VMAC_SUPPORT);
		/*
		 * Set the level on the vnode.  Do this whether or not
		 * mac_installed is set, since you should be able to see the
		 * level on a file even if the kernel is not enforcing MAC
		 * access restritions.
		 * If the file system does not support labeling,
		 * vnode will be labelled with the level floor of
		 * of the mounted file system if vfsp is NON_NULL.
		 */
		if (vp->v_macflag & VMAC_SUPPORT) {
			xvp->v_lid = vp->v_lid;
		} else if (vp->v_vfsp) {
			xvp->v_lid = vp->v_vfsp->vfs_macfloor;
		}

		VN_HOLD(vp);

		xp->x_dev = dev;
		xp->x_flag |= XNAMEINVAL;
		SV_INIT(&xp->x_sv);
		LOCK_INIT(&xp->x_mutex, XNAM_HIER, FS_XNAMPL,
			  &xnam_mutex_lkinfo, KM_SLEEP);
		SLEEP_INIT(&xp->x_rwlock, (uchar_t) 0, &xnam_rwlock_lkinfo,
			     KM_SLEEP);

		xnaminsert(xp, &nxp, vp);
		if (nxp != (xnamnode_t *)NULL) {
			/*
			 * Lost the race in creating an xnamnode to
			 * another LWP; destroy ours.
			 */
			VN_RELE(vp);
			VN_DEINIT(xvp);
			SLEEP_DEINIT(&xp->x_rwlock);
			LOCK_DEINIT(&xp->x_mutex);
			kmem_free((caddr_t)xp, sizeof(xnamnode_t));
			return (XNAMTOV(nxp));
		}
	}
	return (XNAMTOV(xp));
}


/*
 * void
 * xnammark(xnamnode_t *xp, int flags)
 *	Mark the times fields in an xnamnode with the current time.
 *
 * Calling/Exit State:
 *	No lock is held on entry or exit.
 *
 * Description:
 *	The xnamnode spin lock is held while updating the times fields.
 */
void
xnammark(xnamnode_t *xp, int flags)
{
	pl_t		pl;
	timestruc_t	ltime;

	GET_HRESTIME(&ltime);
	pl = XNODE_LOCK(xp);
	xp->x_flag |= flags;
	if (flags & XNAMACC)
		xp->x_atime = ltime.tv_sec;
	if (flags & XNAMUPD)
		xp->x_mtime = ltime.tv_sec;
	if (flags & XNAMCHG)
		xp->x_ctime = ltime.tv_sec;
	XNODE_UNLOCK(xp, pl);
}

/*
 * void
 * xnaminit(vfssw_t *vswp, int fstype)
 *	Initialize xnamfs file system.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	Save filesystem type, initialize vfs operations vector,
 *	get unique device number for XNAMFS. Initialize the
 *	semaphores and shared data free lists.
 */
void
xnaminit(vfssw_t *vswp, int fstype)
{
	dev_t	dev;

	LOCK_INIT(&xnam_table_mutex, FS_XNTBLHIER, FS_XNTBLPL,
		  &xnam_table_lkinfo, KM_SLEEP);
	/*
	 * Associate vfs operations.
	 */
	vswp->vsw_vfsops = &xnam_vfsops;
	xnamfstype = fstype;
	if ((xnamdev = getudev()) == NODEV)
		xnamdev = 0;

	/*
	 * Initialize XENIX semaphore and shared data free lists
	 */
	xsdinit();
	xsem_init();
}

/*
 * int
 * xnampreval(vtype_t type, dev_t dev, cred_t *cr)
 *	Pre-validate attributes that will be passed to a
 *	subsequent xnamvp() call.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 */
/*ARGSUSED*/
int
xnampreval(vtype_t type, dev_t dev, cred_t *cr)
{
	if ((dev-1) >= XNAMTBLSIZE) {
		return (EINVAL);
	} else {
		return (0);
	}
}
