/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/sfs/sfs_qlims.c	1.4"

/*
 * Routines used in checking limits on file system usage.
 */

#include <util/types.h>
#include <util/param.h>
#include <acc/priv/privilege.h>
#include <svc/time.h>
#include <svc/systm.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <fs/vfs.h>
#include <fs/sfs/sfs_hier.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/sfs/sfs_quota.h>
#include <svc/errno.h>

extern int	sfs_getdiskquota(uid_t, sfs_vfs_t *, int, dquot_t **, cred_t *);
extern void	sfs_dqrele(dquot_t *, cred_t *);
/*
 * dquot_t *
 * sfs_getinoquota(inode_t *ip, cred_t *cr)
 *	Find the dquot structure that should be used in
 *	checking I/O on ip.
 *
 * Calling/Exit State:
 *	The inode's read/write sleep lock will be held exclusively on
 *	entry and exit. Returns NULL if the inode is not on a UFS fs,
 *	if someone is doing I/O to the quota file, or if an error was
 *	returned from sfs_getdiskquota().
 *
 * Description:
 *	Obtain the user's disk quota limits. The disk quota structure
 *	will be locked on successful return of sfs_getdiskquota().
 *	If the file/block soft/hard limits are all NULL, unlock the
 *	disk quota structure, release it and return NULL. Otherwise,
 *	unlock the disk quota structure and return the structure.
 */
dquot_t *
sfs_getinoquota(inode_t *ip, cred_t *cr)
{
	dquot_t		*dqp;
	dquot_t		*xdqp;
	pl_t		pl;
	sfs_vfs_t	*sfs_vfsp;

	if (!UFSIP(ip))
		return (NULL);
	sfs_vfsp = (struct sfs_vfs *)((ITOV(ip))->v_vfsp->vfs_data);
	/*
	 * Check for someone doing I/O to quota file.
	 */
	pl = QLIST_LOCK();
	if (ip == sfs_vfsp->vfs_qinod) {
		QLIST_UNLOCK(pl);
		return (NULL);
	}
	QLIST_UNLOCK(pl);
	if (sfs_getdiskquota((uid_t)ip->i_uid, sfs_vfsp, 0, &xdqp, cr)) {
		return (NULL);
	}
	dqp = xdqp;
	if (dqp->dq_fhardlimit == 0 && dqp->dq_fsoftlimit == 0 &&
	    dqp->dq_bhardlimit == 0 && dqp->dq_bsoftlimit == 0) {
		DQUOT_UNLOCK(dqp);
		sfs_dqrele(dqp, cr);
		dqp = NULL;
	} else {
		DQUOT_UNLOCK(dqp);
	}
	return (dqp);
}

/*
 * int
 * sfs_chkdq(inode_t *ip, long change, int force, cred_t *cr)
 *	Update disk usage, and take corrective action.
 *
 * Calling/Exit State:
 *	The inode's read/write sleep lock is held in exclusive mode
 *	on entry or exit.
 *
 * Description:
 *	Lock the disk quota structure and turn on the modified flag.
 *	Set the current block count field of the dqblk structure, turn
 *	off the block limit warned flag, unlock the disk quota structure
 *	and return if change is less than 0.
 *
 *	Otherwise, disallow allocation if it would bring the current
 *	usage over the hard limit, or if the user is over the current
 *	soft limit and the time limit has expired. Unlock the disk
 *	quota structure and return.
 */
int
sfs_chkdq(inode_t *ip, long change, int force, cred_t *cr)
{
	dquot_t		*dqp;
	sfs_vfs_t	*sfs_vfsp;
	ulong_t		ncurblocks;
	int		error;

	error = 0;
	dqp = ip->i_dquot;
	if (!UFSIP(ip) || change == 0 || dqp == NULL) {
		return (error);
	}
	DQUOT_LOCK(dqp);
	dqp->dq_flags |= DQ_MOD;
	if (change < 0) {
		if ((int)dqp->dq_curblocks + change >= 0)
			dqp->dq_curblocks += change;
		else
			dqp->dq_curblocks = 0;
		if (dqp->dq_curblocks < dqp->dq_bsoftlimit)
			dqp->dq_btimelimit = 0;
		dqp->dq_flags &= ~DQ_BLKS;
		DQUOT_UNLOCK(dqp);
		return (error);
	}

	ncurblocks = dqp->dq_curblocks + change;
	/*
	 * Allocation. Check hard and soft limits.
	 *
	 * Dissallow allocation if it would bring the current usage over
	 * the hard limit or if the user is over his soft limit and his time
	 * has run out.
	 */
	sfs_vfsp = (sfs_vfs_t *)(ITOV(ip))->v_vfsp->vfs_data;
	if (ncurblocks >= dqp->dq_bhardlimit && dqp->dq_bhardlimit && !force) {
		if ((dqp->dq_flags & DQ_BLKS) == 0 &&
		     ip->i_uid == cr->cr_ruid) {
			dqp->dq_flags |= DQ_BLKS;
		}
		error = ENOSPC;
	}
	if (ncurblocks >= dqp->dq_bsoftlimit && dqp->dq_bsoftlimit) {
		if (dqp->dq_curblocks < dqp->dq_bsoftlimit ||
		    dqp->dq_btimelimit == 0) {
			dqp->dq_btimelimit = hrestime.tv_sec +
						sfs_vfsp->vfs_btimelimit;
		} else if (hrestime.tv_sec > dqp->dq_btimelimit && !force) {
			if ((dqp->dq_flags & DQ_BLKS) == 0 &&
			     ip->i_uid == cr->cr_ruid) {
				dqp->dq_flags |= DQ_BLKS;
			}
			error = ENOSPC;
		}
	}
	/*
	 * If an error was encountered, check for privilege.
	 * If the process is privileged, override the error.
	 */
	if (error && !pm_denied(cr, P_FILESYS))
		error = 0;

	if (error == 0)
		dqp->dq_curblocks = ncurblocks;

	DQUOT_UNLOCK(dqp);
	return (error);
}

/*
 *
 * sfs_chkiq(sfs_vfs_t *sfs_vfsp, inode_t *ip, uid_t uid, int force,
 *	     cred_t *cr)
 *	Check the inode limit, applying corrective action.
 *
 * Calling/Exit State:
 *	The inode's read/write sleep lock is held in exclusive mode
 *	on entry or exit.
 *
 * Description:
 *	If the inode has a disk quota associated with it, lock the
 *	disk quota structure, turn on the modified flag, decrement
 *	the current allocated files field, turn off the file limit
 *	warned flag, unlock the disk quota structure and return.
 *
 *	Otherwise, get the user's disk quota limit (disk quota structure
 *	will be locked on successful return). If the soft/hard file limits
 *	are both NULL, unlock the disk quota structure, release the disk
 *	quota structure and return. Otherwise, turn on the modified flag.
 *	Disallow allocation if it would bring the current usage over the
 *	hard limit, or if the user is over the current soft limit and the
 *	time limit has expired. Unlock/release the disk quota structure
 *	and return.
 */
int
sfs_chkiq(sfs_vfs_t *sfs_vfsp, inode_t *ip, uid_t uid, int force, cred_t *cr)
{
	dquot_t	*dqp;
	dquot_t	*xdqp;
	u_long	ncurfiles;
	int	error;

	error = 0;
	/*
	 * Free.
	 */
	if (ip != NULL) {
		dqp = ip->i_dquot;
		if (dqp == NULL) {
			return (error);
		}
		DQUOT_LOCK(dqp);
		dqp->dq_flags |= DQ_MOD;
		if (dqp->dq_curfiles) {
			dqp->dq_curfiles--;
		}
		if (dqp->dq_curfiles < dqp->dq_fsoftlimit) {
			dqp->dq_ftimelimit = 0;
		}
		dqp->dq_flags &= ~DQ_FILES;
		DQUOT_UNLOCK(dqp);
		return (error);
	}

	/*
	 * Allocation. Get dquot for uid. Disk quota structure
	 * will be locked on successful return.
	 */
	error = sfs_getdiskquota(uid, sfs_vfsp, 0, &xdqp, cr);
	if (error) {
		return (error);
	}
	dqp = xdqp;
	if (dqp->dq_fsoftlimit == 0 && dqp->dq_fhardlimit == 0) {
		DQUOT_UNLOCK(dqp);
		sfs_dqrele(dqp, cr);
		return (error);
	}
	dqp->dq_flags |= DQ_MOD;
	ncurfiles = dqp->dq_curfiles + 1;
	/*
	 * Dissallow allocation if it would bring the current usage over
	 * the hard limit or if the user is over his soft limit and his time
	 * has run out.
	 */
	if (ncurfiles >= dqp->dq_fhardlimit && dqp->dq_fhardlimit && !force) {
		if ((dqp->dq_flags & DQ_FILES) == 0 && uid == cr->cr_ruid) {
			dqp->dq_flags |= DQ_FILES;
		}
		error = ENOSPC;
	} else if (ncurfiles >= dqp->dq_fsoftlimit && dqp->dq_fsoftlimit) {
		if (ncurfiles == dqp->dq_fsoftlimit || dqp->dq_ftimelimit==0) {
			dqp->dq_ftimelimit = hrestime.tv_sec +
						sfs_vfsp->vfs_ftimelimit;
		} else if (hrestime.tv_sec > dqp->dq_ftimelimit && !force) {
			if ((dqp->dq_flags & DQ_FILES) == 0 &&
			     uid == cr->cr_ruid) {
				dqp->dq_flags |= DQ_FILES;
			}
			error = ENOSPC;
		}
	}
	/*
	 * If an error was encountered, check for privilege.
	 * If the process is privileged, override the error.
	*/
	if (error && !pm_denied(cr, P_FILESYS))
		error = 0;

	if (error == 0)
		dqp->dq_curfiles++;

	DQUOT_UNLOCK(dqp);
	sfs_dqrele(dqp, cr);
	return (error);
}
