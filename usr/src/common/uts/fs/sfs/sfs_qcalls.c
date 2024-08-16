/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/sfs/sfs_qcalls.c	1.15"
#ident	"$Header: $"

/*
 * Quota system calls.
 */
#include <acc/priv/privilege.h>
#include <fs/sfs/sfs_data.h>
#include <fs/sfs/sfs_fs.h>
#include <fs/sfs/sfs_hier.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/sfs/sfs_quota.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/types.h>

extern int	sfs_chkdq(inode_t *, long, int, cred_t *);
extern int	sfs_chkiq(sfs_vfs_t *, inode_t *, uid_t, int, cred_t *);
extern int	sfs_dqupdate(dquot_t *, cred_t *);
extern int	sfs_getdiskquota(uid_t, sfs_vfs_t *, int, dquot_t **, cred_t *);
extern void	sfs_dqrele(dquot_t *dqp, cred_t *cr);
extern int	sfs_igrab(inode_t *);
extern int	sfs_syncip(inode_t *, int, enum iupmode);
extern void	sfs_dqinval(dquot_t *);
extern void	sfs_iput(inode_t *, cred_t *);

lock_t	quotalist_lock;		/* Quota List Spin Lock */

/*
 * int
 * sfs_closedq(sfs_vfs_t *sfs_vfsp, cred_t *cr)
 *	Close off disk quotas for a file system.
 *
 * Calling/Exit State:
 *	The vfslist lock will be held on entry and exit when called from
 *	sfs_unmount(); otherwise, no locks are held.
 *
 *	EPERM is returned if the user does not have the P_SYSOPS privilege.
 *
 * Description:
 *	Hold vfs_mutex while disabling quotas (vfs_qflags set to 0).
 *	Search the inode table for disk quotas associated with this fs.
 *	For each one found, NULL out ip->i_dquot and release the disk quota.
 *	De-reference any disk quota structures that are on the disk
 *	quota table for this file system.
 */
int
sfs_closedq(sfs_vfs_t *sfs_vfsp, cred_t *cr)
{
	dquot_t	*dqp;
	inode_t	*ip;
	inode_t	*qip;
	inode_t	*ipx;
	pl_t	qpl;
	pl_t	ipl;
	union	ihead *ih;
	struct inode_marker *mp;

	ipx = NULL;
	if (pm_denied(cr, P_SYSOPS))
		return (EPERM);
	qpl = QLIST_LOCK();
	if (sfs_vfsp->vfs_qinod == NULL) {
		QLIST_UNLOCK(qpl);
		return (0);
	}
	qip = sfs_vfsp->vfs_qinod;
	ASSERT(qip != NULL);
	sfs_vfsp->vfs_qflags |= MQ_DISABLED;	/* disable quotas */
	QLIST_UNLOCK(qpl);

	/*
	 * Run down the inode table and release all dquots assciated with
	 * inodes on this filesystem. Lock the inode table mutex lock so
	 * that sfs_iget() doesn't claim any inodes from the free list.
	 * Also lock the quota list lock so that disk quota structures
	 * cannot be allocated/deallocated. sfs_dqrele() may do a write
	 * but new inode's i_dquot fields will be NULL since quotas are
	 * disabled. This allows us to continue searching from where we
	 * left off.
	 */
	SFS_CREATE_MARKER(mp);
	qpl = QLIST_LOCK();
	for (ih=&sfs_ihead[0];ih < &sfs_ihead[INOHSZ];ih++) {
		ipl = LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
		/*
                * This search runs through the hash chains (rather
                * than the entire inode table) so that we examine
                * inodes that we know are currently valid.
                */
		for (ip = (inode_t *)ih->ih_chain[0]; ip != (inode_t *)ih;
							ip = ipx) {
			ipx = ip->i_forw;
			if (ip->i_state == IMARKER) /* skip markers */
				continue;
			dqp = ip->i_dquot;
			if (dqp == NULL || dqp->dq_sfs_vfsp != sfs_vfsp ||
			   !SFS_IRWLOCK_IDLE(ip)) {
				continue;
			}
			if (sfs_igrab(ip)) {
				SFS_INSERT_MARKER(mp, ipx);
				UNLOCK(&sfs_inode_table_mutex, ipl);
				QLIST_UNLOCK(qpl);
				/*
				 * Check if the inode still has a disk
				 * structure associated with it for this
				 * file system. If so, clear the pointer.
				 * Unlock the inode and release the
				 * quota structure.
				 */
				dqp = ip->i_dquot;
				if (dqp != NULL &&
				    dqp->dq_sfs_vfsp == sfs_vfsp) {
					ip->i_dquot = NULL;
					sfs_dqrele(dqp, cr);
				}
				SFS_IRWLOCK_UNLOCK(ip);
				VN_SOFTRELE(ITOV(ip));
				qpl = QLIST_LOCK();
				ipl = LOCK(&sfs_inode_table_mutex,
					   FS_SFSLISTPL);
				ipx = mp->im_forw;
				SFS_REMOVE_MARKER(mp);
			}
		}
		UNLOCK(&sfs_inode_table_mutex, ipl);
	}
	SFS_DESTROY_MARKER(mp);

	/*
	 * Run down the dquot table and clean and invalidate the
	 * dquots for this file system.
	 */
	for (dqp = sfs_dquot; dqp < sfs_dquotNDQUOT; dqp++) {
		if (dqp->dq_sfs_vfsp == sfs_vfsp) {
			if (dqp->dq_cnt == 0) {
				sfs_dqinval(dqp);
			} else {
				/*
				 * Found a disk quota structure with a
				 * non-zero reference count for that fs.
				 */
				ASSERT(sfs_vfsp->vfs_qcnt != 0);
			}
		}
	}

	/*
	 * If there are any active disk quota structures,
	 * wait for them to be inactivated.
	 */
	while (sfs_vfsp->vfs_qcnt) {
		SV_WAIT(&quota_sv, PRIVFS, &quotalist_lock);
		qpl = QLIST_LOCK();
	}
	sfs_vfsp->vfs_qinod = NULL;
	QLIST_UNLOCK(qpl);

	/*
	 * Release the quota file inode.
	 */
	SFS_IRWLOCK_WRLOCK(qip);
	sfs_iput(qip, cr);

	return (0);
}

/*
 * int
 * sfs_opendq(sfs_vfs_t *sfs_vfsp, caddr_t addr, cred_t *cr)
 *	Set the quota file up for a particular file system.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *	Returns the following errnos on error:
 *
 *	   EPERM	user does not have the P_SYSOPS privilege
 *	   EROFS	trying to write on a read-only fs
 *	   EACCES	vnode found is not a regular file
 *	   EBUSY	an open or close of the quota file is in progress
 *
 * Description:
 *	Called as the result of an ioctl(fd, Q_QUOTAON) system call.
 *	Hold the quotalist spin lock while checking vfs_qflags and setting
 *	vfs_qinod. Get the disk quota information and hold the quotalist
 *	spin lock while setting vfs_btimelimit, vfs_ftimelimit and vfs_qflags
 *	that denotes quotas are enabled.
 */
STATIC int
sfs_opendq(sfs_vfs_t *sfs_vfsp, caddr_t addr, cred_t *cr)
{
	vnode_t	*vp;
	dquot_t	*dqp;
	int	error;
	pl_t	pl;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (pm_denied(cr, P_SYSOPS)) {
		return (EPERM);
	}
	if (((fs_t *)sfs_vfsp->vfs_bufp->b_addrp)->fs_ronly) {
		return (EROFS);
	}
	error = lookupname(addr, UIO_USERSPACE, FOLLOW,
			  (struct vnode **)NULL, &vp);
	if (error) {
		return (error);
	}
	if ((sfs_vfs_t *)(vp->v_vfsp->vfs_data) != sfs_vfsp ||
	     vp->v_type != VREG) {
		VN_RELE(vp);
		return (EACCES);
	}
	/*
	 * Check to see if a quota structure was already assigned
	 * to the quota file; if so, delete it.
	 */
	dqp = VTOI(vp)->i_dquot;
	if (dqp != NULL) {
		sfs_dqrele(dqp, cr);
		VTOI(vp)->i_dquot = NULL;
	}
	pl = QLIST_LOCK();
	if ((sfs_vfsp->vfs_qflags & MQ_DISABLED) == 0) {
		QLIST_UNLOCK(pl);
		sfs_closedq(sfs_vfsp, cr);
		pl = QLIST_LOCK();
	}
	if (sfs_vfsp->vfs_qinod != NULL) {	/* open/close in progress */
		QLIST_UNLOCK(pl);
		VN_RELE(vp);
		return (EBUSY);
	}
	sfs_vfsp->vfs_qinod = VTOI(vp);
	sfs_vfsp->vfs_qflags = 0;		/* quotas enabled */
	QLIST_UNLOCK(pl);
	/*
	 * The file system time limits are in the uid 0 user dquot.
	 * The time limits set the relative time the other users
	 * can be over quota for this file system.
	 * If it is zero a default is used (see quota.h).
	 */
	error = sfs_getdiskquota((uid_t)0, sfs_vfsp, 1, &dqp, cr);
	if (error == 0) {
		pl = QLIST_LOCK();
		sfs_vfsp->vfs_btimelimit =
		    (dqp->dq_btimelimit? dqp->dq_btimelimit: DQ_BTIMELIMIT);
		sfs_vfsp->vfs_ftimelimit =
		    (dqp->dq_ftimelimit? dqp->dq_ftimelimit: DQ_FTIMELIMIT);
		QLIST_UNLOCK(pl);
		DQUOT_UNLOCK(dqp);
		sfs_dqrele(dqp, cr);
	} else {
		/*
		 * Some sort of I/O error on the quota file.
		 */
		VN_RELE_CRED(ITOV(sfs_vfsp->vfs_qinod), cr);
		pl = QLIST_LOCK();
		sfs_vfsp->vfs_qinod = NULL;
		QLIST_UNLOCK(pl);
	}
	return (error);
}

/*
 * int
 * sfs_setquota(int cmd, uid_t uid, sfs_vfs_t *sfs_vfsp, caddr_t addr,
 *		cred_t *cr)
 *	Set various fields of the dqblk according to the command.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *	EPERM is returned if the user does not have the P_FILESYS privilege.
 *
 * Description:
 *	Q_SETQUOTA denotes assigning an entire dqblk structure.
 *	Q_SETQLIM denotes assigning a dqblk structure except for the usage.
 *
 *	The disk quota structure is locked on successful return from
 *	sfs_getdiskquota(). The vfs_mutex lock is held while setting the
 *	vfs_btimelimit/vfs_ftimelimit fields of the sfs_vfs structure.
 *	The disk quota structure is release before return.
 */
STATIC int
sfs_setquota(int cmd, uid_t uid, sfs_vfs_t *sfs_vfsp, caddr_t addr, cred_t *cr)
{
	dquot_t	*dqp;
	dquot_t	*xdqp;
	dqblk_t	newlim;
	pl_t	pl;
	int	error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (pm_denied(cr, P_FILESYS)) {
		return (EPERM);
	}
	error = copyin(addr, (caddr_t)&newlim, sizeof(dqblk_t));
	if (error) {
		return (error);
	}
	error = sfs_getdiskquota(uid, sfs_vfsp, 0, &xdqp, cr);
	if (error) {
		return (error);
	}
	dqp = xdqp;
	/*
	 * Don't change disk usage on Q_SETQLIM
	 */
	if (cmd == Q_SETQLIM) {
		newlim.dqb_curblocks = dqp->dq_curblocks;
		newlim.dqb_curfiles = dqp->dq_curfiles;
	}
	dqp->dq_dqb = newlim;
	/*
	 * The following check is NOT a privilege check but rather a
	 * conditional check to determine if the uid value passed as
	 * an argument to this routine can perform this function.
	 * It should NEVER be changed to a pm_denied() call!!
	*/
	pl = QLIST_LOCK();
	if (uid == 0) {
		/*
		 * Timelimits for the value uid == 0 set the relative time
		 * the other users can be over quota for this file system.
		 * If it is zero a default is used (see quota.h).
		 */
		sfs_vfsp->vfs_btimelimit =
		    newlim.dqb_btimelimit? newlim.dqb_btimelimit: DQ_BTIMELIMIT;
		sfs_vfsp->vfs_ftimelimit =
		    newlim.dqb_ftimelimit? newlim.dqb_ftimelimit: DQ_FTIMELIMIT;
	} else {
		/*
		 * If the user is now over quota, start the timelimit.
		 * The user will not be warned.
		 */
		if (dqp->dq_curblocks >= dqp->dq_bsoftlimit &&
		    dqp->dq_bsoftlimit && dqp->dq_btimelimit == 0) {
			dqp->dq_btimelimit =
				hrestime.tv_sec + sfs_vfsp->vfs_btimelimit;
		} else {
			dqp->dq_btimelimit = 0;
		}
		if (dqp->dq_curfiles >= dqp->dq_fsoftlimit &&
		    dqp->dq_fsoftlimit && dqp->dq_ftimelimit == 0) {
			dqp->dq_ftimelimit =
				hrestime.tv_sec + sfs_vfsp->vfs_ftimelimit;
		} else {
			dqp->dq_ftimelimit = 0;
		}
		dqp->dq_flags &= ~(DQ_BLKS|DQ_FILES);
	}
	QLIST_UNLOCK(pl);
	dqp->dq_flags |= DQ_MOD;
	/*
	 * The disk quota sleep lock will be unlocked
	 * by sfs_dqupdate().
	 */
	sfs_dqupdate(dqp, cr);
	sfs_dqrele(dqp, cr);
	return (0);
}

/*
 * int
 * sfs_getquota(uid_t uid, sfs_vfs_t *sfs_vfsp, caddr_t addr, cred_t *cr)
 *	Q_GETQUOTA - return current values in a dqblk structure.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *	EPERM is returned if the user does not have the P_OWNER privilege.
 *	ESRCH is returned if all the block/file hard/soft limits are NULL.
 *
 * Description:
 *	The disk quota structure is locked on successful return of
 *	sfs_getdiskquota(). Mark the disk quota structure busy while
 *	copying the structure out. Unlock the disk quota structure
 *	before return.
 */
STATIC int
sfs_getquota(uid_t uid, sfs_vfs_t *sfs_vfsp, caddr_t addr, cred_t *cr)
{
	dquot_t	*dqp;
	dquot_t	*xdqp;
	int	error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (uid != cr->cr_ruid && pm_denied(cr, P_OWNER)) {
		return (EPERM);
	}
	error = sfs_getdiskquota(uid, sfs_vfsp, 0, &xdqp, cr);
	if (error) {
		return (error);
	}
	dqp = xdqp;
	if (dqp->dq_fhardlimit == 0 && dqp->dq_fsoftlimit == 0 &&
	    dqp->dq_bhardlimit == 0 && dqp->dq_bsoftlimit == 0) {
		error = ESRCH;
	} else {
		error = copyout((caddr_t)&dqp->dq_dqb, addr, sizeof(dqblk_t));
	}
	DQUOT_UNLOCK(dqp);
	sfs_dqrele(dqp, cr);
	return (error);
}

/*
 * int
 * sfs_qsync(sfs_vfs_t *sfs_vfsp, cred_t *cr)
 *	Q_SYNC - sync quota files to disk.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *	ESRCH is returned if quotas are not enabled.
 *
 * Description:
 *	Search the disk quota table for quotas that have been modified
 *	while holding both the disk quota lock and the quota list lock.
 *	When found, increment its active reference count and unlock both
 *	the quota list lock and disk quota lock. Then write the disk
 *	quota structure to disk.
 */
STATIC int
sfs_qsync(sfs_vfs_t *sfs_vfsp, cred_t *cr)
{
	dquot_t	*dqp;
	pl_t	pl;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (sfs_vfsp != NULL && (sfs_vfsp->vfs_qflags & MQ_DISABLED)) {
		return (ESRCH);
	}
	/*
	 * Flush any disk quota structures for this device out to disk.
	 * The quota list lock is not locked since new entries added while
	 * the sync was in progress could have happened after the sync.
	 */
	for (dqp = sfs_dquot; dqp < sfs_dquotNDQUOT; dqp++) {
		DQUOT_LOCK(dqp);
		pl = QLIST_LOCK();
		if ((dqp->dq_flags & DQ_MOD) == 0 ||
		    (sfs_vfsp && dqp->dq_sfs_vfsp != sfs_vfsp)) {
			QLIST_UNLOCK(pl);
			DQUOT_UNLOCK(dqp);
			continue;
		}
		++dqp->dq_cnt;
		QLIST_UNLOCK(pl);
		/*
	 	 * The disk quota sleep lock will be unlocked
	 	 * by sfs_dqupdate().
	 	 */
		sfs_dqupdate(dqp, cr);
		sfs_dqrele(dqp, cr);
	}
	return (0);
}

/*
 * int
 * sfs_quotactl(vnode_t *vp, int arg, cred_t *cr)
 *	System call to allow users to find out quota information,
 *	and to allow privileged users to alter it.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *	EINVAL is returned for an invalid operation.
 *
 * Description:
 *	Called via ioctl(fd, Q_QUOTACTL, arg) only for a UFS.
 *	Call the appropriate funtion based on the operation passed in.
 */
int
sfs_quotactl(vnode_t *vp, int arg, cred_t *cr)
{
	struct	quotctl	quot;
	sfs_vfs_t	*sfs_vfsp;
	int		error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (copyin((caddr_t)arg, (caddr_t)&quot, sizeof(struct quotctl))) {
		return (EFAULT);
	}

	if (quot.uid < 0)
		quot.uid = cr->cr_ruid;
	if (quot.op == Q_SYNC && vp == NULL) {
		sfs_vfsp = NULL;
	} else if (quot.op != Q_ALLSYNC) {
		sfs_vfsp = (sfs_vfs_t *)(vp->v_vfsp->vfs_data);
	}

	switch (quot.op) {
	case Q_QUOTAON:
		error = sfs_opendq(sfs_vfsp, quot.addr, cr);
		break;

	case Q_QUOTAOFF:
		error = sfs_closedq(sfs_vfsp, cr);
		break;

	case Q_SETQUOTA:
	case Q_SETQLIM:
		error = sfs_setquota(quot.op, (uid_t)quot.uid, sfs_vfsp,
				     quot.addr, cr);
		break;

	case Q_GETQUOTA:
		error = sfs_getquota((uid_t)quot.uid, sfs_vfsp, quot.addr, cr);
		break;

	case Q_SYNC:
		error = sfs_qsync(sfs_vfsp, cr);
		break;

	case Q_ALLSYNC:
		error = sfs_qsync(NULL, cr);
		break;

	default:
		error = EINVAL;
		break;
	}
	return (error);
}
