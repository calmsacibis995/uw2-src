/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/sfs/sfs_quota.c	1.10"
#ident	"$Header: $"

/*
 * Code pertaining to management of the in-core data structures.
 */
#include <util/types.h>
#include <util/param.h>
#include <svc/systm.h>
#include <svc/errno.h>
#include <proc/cred.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <io/uio.h>
#include <fs/sfs/sfs_hier.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/sfs/sfs_quota.h>
#include <fs/sfs/sfs_fs.h>
#include <util/cmn_err.h>
#include <mem/kmem.h>
#include <util/debug.h>
#include <util/inline.h>

extern int sfs_rdwri(uio_rw_t, inode_t *, caddr_t, int, off_t,
		     uio_seg_t, int *, cred_t *);
/*
 * Dquot cache - hash chain headers.
 */
#define	NDQHASH		67	/* Some prime number */
#define	DQHASH(uid, mp) (((unsigned)(mp) + (unsigned)(uid)) % NDQHASH)

typedef	struct	dqhead	{
	dquot_t	*dqh_forw;	/* MUST be first */
	dquot_t	*dqh_back;	/* MUST be second */
} dqhead_t;

/*
 * Declarations for the quota list spin lock and the quota
 * list synchronization variable.
 */
lock_t		quotalist_lock;
sv_t		quota_sv;
lkinfo_t	quotalist_lkinfo;
LKINFO_DECL(quotalist_lkinfo, "FS:QUOTA:quotalist spin lock", 0);

lkinfo_t	dquot_lkinfo;
LKINFO_DECL(dquot_lkinfo, "FS:QUOTA:disk quota I/O sleep lock", 0);

/*
 * Dquot in core hash chain headers.
 */
STATIC dqhead_t sfs_dqhead[NDQHASH];

/*
 * Size of quota table from sfs.cf (NDQUOT)
 */
extern	int sfs_ndquot;

dquot_t	*sfs_dquot, *sfs_dquotNDQUOT;

/*
 * Dquot free list.
 */
STATIC dquot_t sfs_dqfreelist;

#define dqinsheadfree(DQP) {				\
	(DQP)->dq_freef = sfs_dqfreelist.dq_freef;	\
	(DQP)->dq_freeb = &sfs_dqfreelist;		\
	sfs_dqfreelist.dq_freef->dq_freeb = (DQP);	\
	sfs_dqfreelist.dq_freef = (DQP);		\
}

#define dqinstailfree(DQP) {				\
	(DQP)->dq_freeb = sfs_dqfreelist.dq_freeb;	\
	(DQP)->dq_freef = &sfs_dqfreelist;		\
	sfs_dqfreelist.dq_freeb->dq_freef = (DQP);	\
	sfs_dqfreelist.dq_freeb = (DQP);		\
}

#define dqremfree(DQP) {				\
	(DQP)->dq_freeb->dq_freef = (DQP)->dq_freef;	\
	(DQP)->dq_freef->dq_freeb = (DQP)->dq_freeb;	\
}

/*
 * void
 * sfs_qtinit()
 *	Initialize quota caches.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *	Called from ufsinit().
 */
void
sfs_qtinit()
{
	dqhead_t *dhp;
	dquot_t	 *dqp;

	sfs_dquot =
	   (dquot_t *)kmem_zalloc(sfs_ndquot * sizeof(dquot_t), KM_SLEEP);
	if (sfs_dquot == NULL) {
		/*
		 *+ The system is currently short of memory.
		 *+ No quota structures will be initialized.
		 */
		cmn_err(CE_PANIC, 
			"UFS sfs_qtinit: no memory for ufs quota structures");
		return;
	}
	sfs_dquotNDQUOT = sfs_dquot + sfs_ndquot;

	/*
	 * Initialize quota list spin lock and global
	 *  synchronization variable.
	 */
	LOCK_INIT(&quotalist_lock, FS_QLISTHIER, FS_QLISTPL,
		  &quotalist_lkinfo, KM_SLEEP);
	SV_INIT(&quota_sv);

	/*
	 * Initialize the cache between the in-core structures
	 * and the per-file system quota files on disk.
	 */
	for (dhp = &sfs_dqhead[0]; dhp < &sfs_dqhead[NDQHASH]; dhp++) {
		dhp->dqh_forw = dhp->dqh_back = (dquot_t *)dhp;
	}

	/*
	 * Initialize the disk quota structure and put them on
	 * the free list.
	 */
	sfs_dqfreelist.dq_freeb = (dquot_t *)&sfs_dqfreelist;
	sfs_dqfreelist.dq_freef = sfs_dqfreelist.dq_freeb;
	for (dqp = sfs_dquot; dqp < sfs_dquotNDQUOT; dqp++) {
		SLEEP_INIT(&dqp->dq_iolock, (uchar_t) 0,
			   &dquot_lkinfo, KM_SLEEP);
		dqp->dq_forw = dqp->dq_back = dqp;
		dqinsheadfree(dqp);
	}
}

/*
 * int
 * sfs_getdiskquota(uid_t uid, sfs_vfs_t *sfs_vfsp, int force,
 *		dquot_t **dqpp, cred_t *cr)
 *	Obtain the user's on-disk quota limit for file system specified.
 *
 * Calling/Exit State:
 *	No locks are held on entry. Upon success, returns with the disk
 *	quota structure (dqpp) sleep lock exclusively on exit.
 *	On failure, locks are held.
 *
 *	ESRCH is returned if quotas are disabled.
 *	EUSERS is returned if the disk quota freelist table is full.
 *
 * Description:
 *	Lock the quota list lock before checking the cache. If one is
 *	found, lock the disk quota spin lock while incrementing the
 *	dq_cnt field and return.
 *
 *	If it's not in the cache, get the disk quota structure off the
 *	head of the freelist. Lock the quota inode's i_rwlock in shared
 *	mode while reading from the disk.
 */
int
sfs_getdiskquota(uid_t uid, sfs_vfs_t *sfs_vfsp, int force,
		 dquot_t **dqpp, cred_t *cr)
{
	dquot_t	 *dqp;
	dqhead_t *dhp;
	inode_t	 *qip;
	int	 error;
	pl_t	 pl;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Check for quotas enabled.
	 */
	pl = QLIST_LOCK();
	if ((sfs_vfsp->vfs_qinod == NULL ||
	    (sfs_vfsp->vfs_qflags & MQ_DISABLED) && !force)) {
		QLIST_UNLOCK(pl);
		return (ESRCH);
	}
	qip = sfs_vfsp->vfs_qinod;
	QLIST_UNLOCK(pl);

	/*
	 * Check the cache first.
	 */
	dhp = &sfs_dqhead[DQHASH(uid, sfs_vfsp)];
	pl = QLIST_LOCK();

	for (dqp = dhp->dqh_forw; dqp != (dquot_t *)dhp; dqp = dqp->dq_forw) {
		if (dqp->dq_uid != uid || dqp->dq_sfs_vfsp != sfs_vfsp) {
			continue;
		}
		/*
		 * Check again to see if quotas are still enabled.
		 */
		if (sfs_vfsp->vfs_qflags & MQ_DISABLED) {
			QLIST_UNLOCK(pl);
			return (ESRCH);
		}
		/*
		 * Now that we have a quota structure, lock it in
		 * exclusive mode.
		 */
		DQUOT_LOCK_RELLOCK(dqp);
		pl = QLIST_LOCK();
		/*
		 * Cache hit with no references. Take the structure
		 * off the free list, and increment the total number
		 * of active disk quota structures field. This field
		 * is protected by the quota list spin lock.
		 */
		if (dqp->dq_cnt == 0) {
			dqremfree(dqp);
			sfs_vfsp->vfs_qcnt++;
		}
		dqp->dq_cnt++;
		*dqpp = dqp;
		QLIST_UNLOCK(pl);
		return (0);
	}
	/*
	 * Not in cache.
	 * Get dquot at head of free list.
	 */
	dqp = sfs_dqfreelist.dq_freef;
	if (dqp == &sfs_dqfreelist) {
		QLIST_UNLOCK(pl);
		/*
		 *+ The disk quota freelist table is full.
		 */
		cmn_err(CE_WARN, "dquot table full");
		return (EUSERS);
	}
	DQUOT_LOCK_RELLOCK(dqp);
	pl = QLIST_LOCK();

	/*
	 * Obtained a disk quota structure from the free list
	 * that had an active reference.
	 */
	ASSERT(dqp->dq_cnt == 0 && dqp->dq_flags == 0);

	/*
	 * Check to see if quotas are still enabled.
	 */
	if (sfs_vfsp->vfs_qflags & MQ_DISABLED) {
		QLIST_UNLOCK(pl);
		DQUOT_UNLOCK(dqp);
		return (ESRCH);
	}

	/*
	 * Increment the total number of active disk quota structures
	 * field (protected by the quota list spin lock).
	 * Take it off the free list, and off the hash chain it was on.
	 */
	sfs_vfsp->vfs_qcnt++;
	dqremfree(dqp);
	remque(dqp);

	/*
	 * Set the fields in the disk quota structure and
	 * unlock the quota list lock.
	 */
	dqp->dq_cnt = 1;
	dqp->dq_uid = uid;
	dqp->dq_flags = NULL;
	dqp->dq_sfs_vfsp = sfs_vfsp;
	QLIST_UNLOCK(pl);

	SFS_IRWLOCK_RDLOCK(qip);
	if (dqoff(uid) < qip->i_size) {
		/*
		 * Read quota info off disk.
		 */
		error = sfs_rdwri(UIO_READ, qip, (caddr_t)&dqp->dq_dqb,
		    (int)sizeof (struct dqblk), dqoff(uid), UIO_SYSSPACE,
		    (int *)NULL, cr);
		SFS_IRWLOCK_UNLOCK(qip);
		if (error) {
			/*
			 * I/O error in reading quota file.
			 * Put dquot at the head of the free list and
			 * reflect the problem to caller.
			 */
			dqp->dq_cnt = 0;
			dqp->dq_sfs_vfsp = NULL;
			dqp->dq_forw = dqp;
			dqp->dq_back = dqp;
			dqinsheadfree(dqp);
			DQUOT_UNLOCK(dqp);
			return (EIO);
		}
	} else {
		SFS_IRWLOCK_UNLOCK(qip);
		bzero((caddr_t)&dqp->dq_dqb, sizeof (struct dqblk));
	}
	/*
	 * Put the disk quota structure on the correct hash list
	 * where others can find it; done after successful I/O to
	 * eliminate having to do a remque() on failure.
	 */
	pl = QLIST_LOCK();
	insque(dqp, dhp);
	QLIST_UNLOCK(pl);
	*dqpp = dqp;
	return (0);
}

/*
 * int
 * sfs_dqupdate(dquot_t *dqp, cred_t *cr)
 *	Update on disk quota info.
 *
 * Calling/Exit State:
 *	The disk quota sleep lock is held on entry
 *	and unlocked on exit.
 *
 * Description:
 *	Lock the disk quota structure. If the quota has been modified,
 *	mark the quota as busy. Lock the inode's i_rwlock in exclusive
 *	mode while writing the disk quota structure to disk. Unlock the
 *	i_rwlock and disk quota lock and return.
 */
int
sfs_dqupdate(dquot_t *dqp, cred_t *cr)
{
	inode_t	*qip;
	int	error;
	pl_t	pl;

	ASSERT(SLEEP_LOCKOWNED(&dqp->dq_iolock));

	error = 0;
	if (dqp->dq_flags & DQ_MOD) {
		pl = QLIST_LOCK();
		qip = dqp->dq_sfs_vfsp->vfs_qinod;
		QLIST_UNLOCK(pl);
		ASSERT(qip != NULL);
		dqp->dq_flags &= ~DQ_MOD;
		SFS_IRWLOCK_WRLOCK(qip);
		error = sfs_rdwri(UIO_WRITE, qip, (caddr_t)&dqp->dq_dqb,
			(int) sizeof(dqblk_t), dqoff(dqp->dq_uid),
			UIO_SYSSPACE, (int *)NULL, cr);
		SFS_IRWLOCK_UNLOCK(qip);
	}
	DQUOT_UNLOCK(dqp);
	return (error);
}

/*
 * void
 * sfs_dqinval(dquot_t *dqp)
 *	Invalidate a dquot.
 *
 * Calling/Exit State:
 *	The quota list spin lock is held on entry and exit.
 *
 * Description:
 *	Take the dquot off its hash list and put it on a private,
 *	unfindable hash list. Also, put it at the head of the free list.
 */
void
sfs_dqinval(dquot_t *dqp)
{
	ASSERT(KS_HOLD1LOCK());
	ASSERT(dqp->dq_cnt == 0);  
	ASSERT((dqp->dq_flags & (DQ_MOD)) == 0);

	dqp->dq_flags = 0;
	remque(dqp);
	dqremfree(dqp);
	dqp->dq_sfs_vfsp = NULL;
	dqp->dq_forw = dqp;
	dqp->dq_back = dqp;
	dqinsheadfree(dqp);
}

/*
 * void
 * sfs_dqrele(dquot_t *dqp, cred_t *cr)
 *	Release a dquot.
 *
 * Calling/Exit State:
 *	The quota inode's rwsleep lock is held in exclusive mode
 *	on entry and on exit.
 *
 * Description:
 *	Lock the quota list lock before checking the disk quota
 *	structure. Check the count of active references; if 1,
 *	decrement the count and signal any LWPs waiting on the
 *	quota global synchronization variable.
 */
void
sfs_dqrele(dquot_t *dqp, cred_t *cr)
{
	sfs_vfs_t	*sfs_vfsp;
	pl_t		pl;

	if (dqp == NULL) {
		return;
	}
	pl = QLIST_LOCK();
	ASSERT(dqp->dq_cnt != 0);
	/*
	 * Releasing a disk quota structure with
	 * a zero active reference count.
	 */
	if (dqp->dq_cnt == 1) {
		DQUOT_LOCK_RELLOCK(dqp);
		if (dqp->dq_flags & DQ_MOD) {
			sfs_dqupdate(dqp, cr);
		} else {
			DQUOT_UNLOCK(dqp);
		}
		pl = QLIST_LOCK();
		/*
		 * Make sure reference count is still 1
		 * after sleeping for I/O.
		 */
		if (dqp->dq_cnt == 1) {
			dqinstailfree(dqp);
			dqp->dq_cnt--;
			sfs_vfsp = dqp->dq_sfs_vfsp;
			if ((--sfs_vfsp->vfs_qcnt == 0) && SV_BLKD(&quota_sv)) {
				SV_BROADCAST(&quota_sv, 0);
			}
			QLIST_UNLOCK(pl);
			return;
		}
	}
	dqp->dq_cnt--;
	QLIST_UNLOCK(pl);
}

/*
 * void
 * sfs_deinitqt()
 *	Deinitialize quota caches.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *	Called from sfs_unload().
 */
void
sfs_deinitqt()
{
	dquot_t	 *dqp;

	LOCK_DEINIT(&quotalist_lock);
	for (dqp = sfs_dquot; dqp < sfs_dquotNDQUOT; dqp++) {
		SLEEP_DEINIT(&dqp->dq_iolock);
		dqp->dq_forw = dqp->dq_back = dqp;
	}
	kmem_free(sfs_dquot, sfs_ndquot*sizeof(dquot_t));
}
