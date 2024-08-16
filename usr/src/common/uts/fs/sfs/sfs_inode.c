/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/sfs/sfs_inode.c	1.124"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <acc/dac/acl.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/dnlc.h>
#include <fs/dow.h>
#include <fs/fs_subr.h>
#include <fs/mode.h>
#include <fs/sfs/sfs_dow.h>
#include <fs/sfs/sfs_hier.h>
#include <fs/sfs/sfs_fs.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/sfs/sfs_data.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <mem/swap.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/autotune.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <svc/clock.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/metrics.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>
#include <util/mod/moddefs.h>
#include <fs/sfs/sfs_quota.h>
#ifdef CC_PARTIAL
#include <acc/mac/covert.h>
#include <acc/mac/cc_count.h>
#endif



/*
 * The inode statistics structure and the hash_timestamp are protected
 * by the inode_hash_table_mutex. */
struct inostats inostats;
extern int sfs_timelag;
int hash_timestamp = 0;

STATIC int sfs_load(void);
STATIC int sfs_unload(void);

int sfs_fsflags = 0;    /* initialize vswp->vsw_flags */

MOD_FS_WRAPPER(sfs, sfs_load, sfs_unload, "Loadable SFS FS Type");

extern int sfs_syncip(inode_t *, int, enum iupmode);
extern int sfs_ifree(inode_t *, ino_t, mode_t);
extern int sfs_free(inode_t *, daddr_t, off_t);
extern int	sfs_chkdq(inode_t *, long, int, cred_t *);
extern int	sfs_chkiq(sfs_vfs_t *, inode_t *, uid_t, int, cred_t *);
extern void	sfs_dqrele(dquot_t *, cred_t *);
extern dquot_t	*sfs_getinoquota(inode_t *, cred_t *);
extern void	sfs_release(vnode_t *);
extern int	tune_calc(struct tune_point *, int);
#ifdef _SFS_SOFT_DNLC
extern boolean_t sfs_tryhold(vnode_t *);
#endif
extern struct inode_marker sfs_totally_free;
extern struct inode_marker sfs_partially_free;
extern clock_t sfs_scan_time;

void	sfs_iput(inode_t *, cred_t *);
void	sfs_iupdat(inode_t *, enum iupmode);
void	sfs_remque(inode_t *);
void	sfs_insque(inode_t *, union ihead *);
int	sfs_itrunc(inode_t *, uint_t, cred_t *);
int	sfs_idestroy(inode_t *, cred_t *);

timestruc_t     sfs_iuniqtime;
void		sfskmadv();
STATIC int	sfs_doinit(int);
STATIC boolean_t sfs_free_scan(void);
STATIC long sfs_freeblocks(inode_t *, uint_t, dowid_t);

STATIC mode_t sfs_daccess(inode_t *, mode_t, cred_t *);

/*
 * STATIC int
 * sfs_load(void)
 *	Initialize the inode hash table, the inode table lock
 *	and the synchronized variable.	
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 */
STATIC int
sfs_load(void)
{
	struct	vfssw	*vswp;
	int		error;

	vswp = vfs_getvfssw("sfs");
	if (vswp == NULL) {
		/*
                 *+ SFS file system is not registered before
                 *+ attempting to load it.
                 */
                cmn_err(CE_NOTE, "!MOD: SFS is not registered.");
                return (EINVAL);
	}
	error = sfs_doinit(KM_SLEEP);

	return error;
}

/*
 * STATIC int
 * sfs_unload(void)
 *	Deinitialize the inode table lock
 *	and the synchronized variable.	
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 */
STATIC int
sfs_unload(void)
{
	inode_t *ip;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Free all dynamic inode storage.
	 */
	for (;;) {
		(void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);

		/*
		 * Since all sfs file systems have been unmounted at
		 * this time, all inodes have lost identity. Therefore,
		 * they are either on the free list, are SOFTHELD, or
		 * both.
		 */
		ASSERT(SFS_LIST_ISEMPTY(&sfs_partially_free));
		while ((ip = SFS_LIST_HEAD(&sfs_totally_free)) != NULL) {
			ASSERT(ip->i_state == ITFREE);
			ASSERT(ITOV(ip)->v_softcnt >= 1);
			SFS_FREE_REMOVE(ip);
			inostats.freeinodes--;
			ip->i_state = (INVALID|IKMFREE);
			SFS_FREE_CHECK();
			UNLOCK(&sfs_inode_table_mutex, PLBASE);
			VN_SOFTRELE(ITOV(ip));
			(void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
		}

		/*
		 * If all inodes have been freed, then we are done. Else,
		 * other inodes may be out there some place, VN_SOFTHELD by
		 * some LWPs. Wait for them to free up as well.
		 */
		if (inostats.totalinodes == 0)
			break;
		SV_WAIT(&sfs_inode_sv, PRINOD, &sfs_inode_table_mutex);
	}
	UNLOCK(&sfs_inode_table_mutex, PLBASE);

	LOCK_DEINIT(&sfs_inode_table_mutex);
	SLEEP_DEINIT(&sfs_updlock);

	return(0);
}

/*
 * sfskmadv()
 *	Call kmem_advise() for SFS inode data structure.
 *
 * Calling/Exit State:
 *	Called at sysinit time while still single threaded.
 */
void
sfskmadv()
{
	kmem_advise(sizeof(inode_t));
}

extern struct tune_point SFSNINODEcurve[];
extern int SFSNINODEcurvesz;
/*
 * int
 * sfs_doinit(int sleepflag)
 *	Initialize hash links for inodes and build inode table.
 *	Initialize the inode table lock, the update sleep lock,
 *	and the synchronization variable.
 *
 * Calling/Exit State:
 *	Should only be called at file system initialization time, i.e.,
 *	once per module load (or once per system lifetime if not
 *	dynamically loadable).
 *
 *	Returns an error code on failure or 0  on sucess.
 *
 * Remarks:
 *	Called from sfsinit() and sfs_load().
 */
int
sfs_doinit(int sleepflag)
{
	int		i;
	union ihead	*ih;
	inode_t		*ip;

	/* check tunable flush time parameter */
	if (sfs_tflush > v.v_autoup) {
		/*
		 *+ Invalid flush time parameter
		 */
		cmn_err(CE_NOTE,
			"SFSFLUSH is invalid. It should be less than NAUTOUP");
		return (EINVAL);
	}

        sfs_ninode = tune_calc(SFSNINODEcurve, SFSNINODEcurvesz);

	ih = sfs_ihead;
	for (i = INOHSZ; --i >= 0; ih++) {
		ih->ih_head[0] = ih;
		ih->ih_head[1] = ih;
	}

	ip = (inode_t *)(&sfs_totally_free);
	ip->i_freef = ip->i_freeb = ip;
	ip = (inode_t *)(&sfs_partially_free);
	ip->i_freef = ip->i_freeb = ip;
	sfs_scan_time = lbolt;
	SV_INIT(&sfs_inode_sv);
	LOCK_INIT(&sfs_inode_table_mutex, FS_SFSLISTHIER, FS_SFSLISTPL,
		  &sfs_inode_table_lkinfo, KM_NOSLEEP);
	SLEEP_INIT(&sfs_updlock, (uchar_t) 0, &sfs_updlock_lkinfo, sleepflag);

	MET_INODE_MAX(MET_SFS, sfs_ninode);

	inostats.totalinodes = 0;
	inostats.freeinodes = 0;
#ifdef DEBUG
	inostats.lockedlist = 0;
 	inostats.lhelp = 0;
#ifndef _SFS_SOFT_DNLC
	inostats.purgedlist = 0;
	inostats.lnp = 0;
	inostats.phelp = 0;
	inostats.lnph = 0;
#endif	/* !_SFS_SOFT_DNLC */
#endif	/* DEBUG */

	return 0;
}

/*
 * sfsinit(vswp)
 *	Initialize SFS structures.
 *
 * Calling/Exit State:
 *	Should only be called when initializing SFS. Currently, this
 *	happens at system initialization time when sfs is static.
 */
void
sfsinit(struct vfssw *vswp)
{
	if (sfs_doinit(KM_NOSLEEP))
		return;

	/*
	 * Associate vfs operations
	 */
	vswp->vsw_vfsops = &sfs_vfsops;

	return;
}


/*
 * inode_t *
 * sfs_search_ilist(union ihead *ih, ino_t ino, vfs_t *vfsp)
 *	Determine whether a specific inode belongs to a
 *	given hash bucket, i.e., has the inode been hashed
 *	already?
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode table lock.
 *
 *	On success, a pointer to an *unlocked* inode
 *	is returned. Since the calling LWP has the
 *	inode table lock, this is ok.
 *
 *	If NULL is returned, than a matching inode
 *	could not be found.
 *
 * Description:
 *	Simply search the hash bucket for a inode matching
 *	<vfsp->vfs_dev, ino>.
 */
inode_t *
sfs_search_ilist(union ihead *ih, ino_t ino, vfs_t *vfsp)
{
	inode_t *ip;
#ifdef DEBUG
	inode_t *last_ip = (inode_t *)ih;
#endif
	ASSERT(LOCK_OWNED(&sfs_inode_table_mutex));

	for (ip = ih->ih_chain[0]; ip != (inode_t *)ih; ip = ip->i_forw) {
		SFS_ASSERT(ip, (ip->i_state & IDENTITY) ||
			       ip->i_state == IMARKER);
		SFS_ASSERT(ip, ip->i_back == last_ip);
		/*
		 * Test for matching identity.
		 *
		 * N.B. Test of i_fs must come first due to the implicit
		 *	union with ``markers''. Markers do not have
		 *	i_number and i_dev fields.
		 */
		if (ip->i_fs != NULL &&
		    ino == ip->i_number && vfsp->vfs_dev == ip->i_dev)
			return (ip);
#ifdef DEBUG
		last_ip = ip;
#endif
	}
	SFS_ASSERT(last_ip, ip->i_back == last_ip);

	/*
	 * Didn't find it...
	 */
	return (NULL);
}

/*
 * inode_t *
 * sfs_getdynamic(void)
 *	Create a new inode slot by getting the memory from KMA.
 *
 * Calling/Exit State:
 *	The table_mutex is unlocked throughout.
 *
 * Description:
 *	Gets space from KMA, sets it up to be an inode.
 *
 */
inode_t *
sfs_getdynamic(void)
{
	inode_t *ip;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	ip = kmem_zalloc(sizeof(struct inode), KM_SLEEP);
	ASSERT(ip != NULL);

	MET_INODE_CURRENT(MET_SFS, 1);
	/* Assume we get an inode because we're going to use it */
	MET_INODE_INUSE(MET_SFS, 1);
	SFS_INIT_INODE(ip, NULL, 0, 0);
	return(ip);
}


/* 
 * int
 * sfs_getfree_inode(inode_t **ipp, union ihead *ih, ino_t ino,
 *		     fs_t *fs, vfs_t *vfsp, cred_t *cr)
 *	Remove an inode from the free list (if there are
 *	any available) for re-use, or create one. May cause dnlc purges.
 *
 * Calling/Exit State:
 *	Calling LWP holds the inode table locked. The inode table lock will
 *	be released before this function returns.
 *
 *	This function may block.
 *	
 *	If 0 is returned, then the inode removed from the free list or 
 *	newly allocated is placed in <*ipp>. The rwlock of <*ipp> is held 
 *	exclusive in this case. 
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		ENFILE	The maximum number of SFS inodes are in use.
 *			<*ipp> is NULL in this case.
 *
 * Description:
 *	Try to take an inode from the free list. If the inode has pages
 *	associated with it, dynamically allocate a new inode unless the 
 *	high water mark has been reached.  If the free list is empty then 
 * 	purge some dnlc entries from the file system that the inode will 
 *	live on. Return ENFILE if there aren't any inodes available.
 *
 *	Insert the new inode into the proper place on the hash list. If an
 *	inode with pages was recycled, clean any pages associated with the 
 *	inode.
 *
 *	Must be careful to re-check the hash list for the requested inode
 *	if we release the lists lock since another LWP could have entered
 *	the inode in the list.
 */
int
sfs_getfree_inode(inode_t **ipp, union ihead *ih, ino_t ino,
		  fs_t *fs, vfs_t *vfsp, cred_t *cr)
{
	inode_t *ip;
	inode_t	*dupip = NULL;
	vnode_t	*vp;
	int	count = 0;
	int	save_timestamp;
	int	error = 0;
	int	softcnt;

	ASSERT(KS_HOLD1LOCK());
	ASSERT(LOCK_OWNED(&sfs_inode_table_mutex));
	ASSERT(getpl() == FS_SFSLISTPL);

	/*
	 * Save the hash timestamp before dropping the inode table lock.
	 */
	save_timestamp = hash_timestamp;
	*ipp = NULL;
again:

#ifdef DEBUG
	if (count > 5)
		inostats.lockedlist++;
#endif DEBUG

	/*
	 * First, try to grab a inode off the totally free list.
	 *
	 * Second, try to shake some inodes off the partially free list and
	 * try to grab one.
	 */
	if (!SFS_LIST_ISEMPTY(&sfs_totally_free) || SFS_PFREE_SCAN()) {
		ASSERT(!SFS_LIST_ISEMPTY(&sfs_totally_free));
		ip = SFS_LIST_HEAD(&sfs_totally_free);
		ASSERT(ip->i_state == (IDENTITY|ITFREE) ||
		       ip->i_state == ITFREE);
		if (ip->i_state == ITFREE) {
			SFS_FREE_REMOVE(ip);
			ip->i_state = (INVALID|ILCLFREE);
			inostats.freeinodes--;
			SFS_FREE_CHECK();
			MET_INODE_INUSE(MET_SFS, 1);
			goto i_initialize;
		}

		/*
		 * Be careful about not looping too long on an
		 * inode whose identity cannot be destroyed.
		 */
		if (count < 5) {
			inostats.freeinodes--;
			MET_INODE_INUSE(MET_SFS, 1);
			goto i_destroy;
		}
	}

	/*
	 * Take from the head of the partially free list if the pages
	 * have ``aged'' sufficiently long.
	 */
	if ((ip = SFS_LIST_HEAD(&sfs_partially_free)) != NULL &&
	    lbolt - ip->i_ftime > sfs_timelag) {
		goto i_destroy;
	}

	/*
	 * Now, if we are below the high water mark, try to allocate
	 * a new inode from KMA.
	 */
	if (inostats.totalinodes < sfs_ninode) {
		UNLOCK(&sfs_inode_table_mutex, PLBASE);
		ip = sfs_getdynamic();
		(void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
		if (ip != NULL) {
			inostats.totalinodes++;
#ifdef DEBUG
			if (count > 5)
				inostats.lhelp++;
#endif
			goto i_initialize;
		}
	}

#ifndef _SFS_SOFT_DNLC
	/*
	 * now either we're over the high water mark, or we're
	 * below but kmem_alloc failed.  Purge some DNLC
	 * entries for this mounted file system in an attempt to
	 * reclaim some inodes.
	 * dnlc_purge_vfsp might go to sleep. So we have to check the
	 * hash chain again to handle possible race condition.
	 */
	UNLOCK(&sfs_inode_table_mutex, PLBASE);
	(void)dnlc_purge_vfsp(vfsp, 10);
#ifdef DEBUG
	inostats.purgedlist++;
	if (count > 5)
		inostats.lnp++;
#endif	/* DEBUG */
	(void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
#endif	/* _SFS_SOFT_DNLC */

	/*
	 * The sfs_inode_table_mutex was dropped, so recheck the totally
	 * free list.
	 */
	if ((ip = SFS_LIST_HEAD(&sfs_totally_free)) != NULL) {
#if	defined(DEBUG) && !defined(_SFS_SOFT_DNLC)
		inostats.phelp++;
		if (count > 5)
			inostats.lnph++;
#endif
		ASSERT(ip->i_state == (IDENTITY|ITFREE) ||
		       ip->i_state == ITFREE);
		if (ip->i_state == ITFREE) {
			SFS_FREE_REMOVE(ip);
			ip->i_state = (INVALID|ILCLFREE);
			inostats.freeinodes--;
			SFS_FREE_CHECK();
			MET_INODE_INUSE(MET_SFS, 1);
			goto i_initialize;
		}
		inostats.freeinodes--;
		MET_INODE_INUSE(MET_SFS, 1);
		goto i_destroy;
	}

	/*
	 * Now either we're over the high water mark, or we're
	 * below but kmem_alloc failed. Try to destroy the identity
	 * of some inode from the partially list.
	 */
	if (SFS_LIST_ISEMPTY(&sfs_partially_free)) {
		UNLOCK(&sfs_inode_table_mutex, PLBASE);
		return (ENFILE);
	}

#if	defined(DEBUG) && !defined(_SFS_SOFT_DNLC)
	inostats.phelp++;
	if (count > 5)
		inostats.lnph++;
#endif

	ip = SFS_LIST_HEAD(&sfs_partially_free);
	ASSERT(ip->i_state == (IDENTITY|IPFREE));

	/*
	 * Remove the inode from the free list.
	 */
i_destroy:
	SFS_FREE_REMOVE(ip);
	ASSERT(ip->i_state == IDENTITY);
	ip->i_state = (IDENTITY|INVALID);
	SFS_FREE_CHECK();
	UNLOCK(&sfs_inode_table_mutex, PLBASE);

	/*
	 * Gather more inode metrics (these are per-processor so
	 * we don't need the lock). v_pages doesn't need locking,
	 * as metrics don't need to be exact.
	 */
	if (ITOV(ip)->v_pages != NULL) {
		MET_INO_W_PG(MET_SFS, 1);
	} else {
		MET_INO_WO_PG(MET_SFS, 1);
	}

	/*
	 * The inode has an old identity which must now be destroyed.
	 */
	SFS_IRWLOCK_WRLOCK(ip);
	error = sfs_idestroy(ip, cr);
	if (error) {
		if (++count > 10) /* too many tries */
			return (ENFILE);
		(void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
		goto again;
	}

	/*
	 * Wait for all SOFTHOLDs to disappear on the old identity.
	 */
	vp = ITOV(ip);
	for (;;) {
		(void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
		VN_LOCK(vp);
		softcnt = vp->v_softcnt;
		VN_UNLOCK(vp);
		if (softcnt == 0)
			break;
		SV_WAIT(&sfs_inode_sv, PRINOD, &sfs_inode_table_mutex);
	}
	ip->i_state = INVALID|ILCLFREE;
	vp->v_softcnt = 1; /* no locking needed, vp is privately held */

i_initialize:
	/*
	 * Make sure that someone else didn't enter the inode into
	 * the list while we dropped the table lock. If they
	 * did, then, must release our inode (<ip>) and start over.
	 */
	ASSERT(ip->i_state == (INVALID|ILCLFREE) ||
	       ip->i_state == (INVALID|IKMFREE));
	vp = ITOV(ip);

	/*
 	 * Make sure there is or isn't space for an ACL, as appropriate.
	 * Also, set v_op as appropriate.
	 */

	if (UFSVFSP(vfsp)) {
		if (ip->is_union != NULL) {
			kmem_free(ip->is_union, sizeof(union i_secure));
			ip->is_union = NULL;
		}
		vp->v_op = &ufs_vnodeops;
	} else { /* SFS */
		if (ip->is_union == NULL) {
			UNLOCK(&sfs_inode_table_mutex, PLBASE);
			ip->is_union = kmem_alloc(sizeof(union i_secure),
						  KM_SLEEP);
			(void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
		} 
		vp->v_op = &sfs_vnodeops;
	}

	if (save_timestamp != hash_timestamp) {
		dupip = sfs_search_ilist(ih, ino, vfsp);
	}
	if (dupip != NULL) {
		UNLOCK(&sfs_inode_table_mutex, PLBASE);
		/* no locking needed, vp is privately held */
		vp->v_softcnt = 0;
		sfs_release(vp);
		return (0);
	}

	/*
	 * Clear any flags that may have been set.
	 */
	ip->i_flag = 0;
#ifdef _SFS_SOFT_DNLC
	vp->v_flag = VSOFTDNLC;
#else /* !_SFS_SOFT_DNLC */
	vp->v_flag = 0;
#endif /* !_SFS_SOFT_DNLC */
	vp->v_macflag = 0;

	ASSERT(vp->v_pages == NULL);
	ASSERT(vp->v_count == 0);
	ASSERT(vp->v_softcnt == 1);

	/*
	 * Put the inode on the chain for its new (ino, dev) pair and
	 * destroy the mapping, if any, in effect.
	 */
	sfs_insque(ip, ih);
	hash_timestamp++;

	ip->i_dev = vfsp->vfs_dev;
	ip->i_fs = fs;
	ip->i_number = ino;
	UNLOCK(&sfs_inode_table_mutex, PLBASE);

	/*
	 * The inode is now visible to sfs_iget via the table.
	 * However, it is marked INVALID, so that any potential
	 * LWP looking for this inode will block in sfs_iget.
	 */

	ip->i_devvp = ((sfs_vfs_t *)vfsp->vfs_data)->vfs_devvp;
	ip->i_diroff = 0;
	ip->i_dirofflid = cr->cr_lid;
	ip->i_nextr = 0;
	ip->i_vcode = 0;
	ip->i_mapcnt = 0;
	ip->i_swapcnt = 0;
	ip->i_iolen = 0;

	/*
	 * Return the inode write locked.
	 */
	ASSERT(SFS_IRWLOCK_IDLE(ip));
	SFS_IRWLOCK_WRLOCK(ip);

	*ipp = ip;

	return (error);
}

/*
 * int
 * sfs_init_inode(inode_t *ip, struct fs *fs, ino_t ino, int lkmode,
 *		  cred_t *cr)
 *	Initialize an inode from its on-disk copy to complete
 *	the initialization of an inode.
 *
 * Calling/Exit State:
 *	The calling LWP holds the inode's rwlock *exclusive* on 
 *	entry and exit. The inode has IDENTITY, is INVALID,
 *	vp->v_count == 0, and vp->v_softcnt == 1.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. There are no errnos returned directly
 *	by this routine.
 *
 *	This function may block.
 *
 * Description:
 *	The inode is initialized from the on-disk copy. If there's
 *	an error, the inode number (ip->i_number) is set to 0
 *	so any LWPs blocked on the inode do not use it (see sfs_iget).
 */
int
sfs_init_inode(inode_t *ip, fs_t *fs, vfs_t *vfsp, ino_t ino,
	       int lkmode, cred_t *cr)
{
	int	error;
	buf_t	*bp;
	int	ftype;
	struct dinode *dp;
	vnode_t	*vp = ITOV(ip);
	sfs_vfs_t *sfs_vfsp;

	ASSERT(ip->i_state == (IDENTITY|INVALID|IKMFREE) ||
	       ip->i_state == (IDENTITY|INVALID|ILCLFREE));
	ASSERT(vp->v_count == 0);
	ASSERT(vp->v_softcnt == 1);
	ASSERT(vp->v_pages == NULL);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(!SFS_ITRYRDLOCK(ip));

	if (ip->i_dquot != NULL) {
		sfs_dqrele(ip->i_dquot, cr);
	}
	bp = bread(ip->i_dev,
		   (daddr_t)fsbtodb(fs, itod(fs, ino)),
		   fs->fs_bsize);
	/*
	 * Check I/O errors and get vcode
	 */
	error = (bp->b_flags & B_ERROR) ? EIO : 0;
	if (error != 0) {
		goto error_exit;
	}

	dp = (struct dinode *)bp->b_addrp;
	dp += itoo(fs, ino);
	ip->i_ic = dp->di_ic;			/* structure assignment */

	if ((lkmode & IG_NCREATE) != 0 && (ip->i_nlink == 0 ||
							ip->i_mode == 0)) {
		if ((lkmode & IG_PR_WARN) != 0) {
		    /*
		     *+ An inode has a directory entry pointing to it,
		     *+ yet it's link count or mode is 0. This indicates
		     *+ an inconsistency between an inode's state and
		     *+ the directory entry. We have to fail the lookup.
		     *+ The problem can be fixed by running fsck.
		     */
		    cmn_err(CE_NOTE,
		     "UFS/SFS sfs_init_inode: in-use inode %s/%d, mode = 0%o, nlink = %d\n",
		     ip->i_fs->fs_fsmnt, ip->i_number, ip->i_mode, ip->i_nlink);
		}

		error = ENOENT;
		goto error_exit;
	}
	if (IFTOVT(ip->i_mode) == VREG) {
		error = fs_vcode(vp, &ip->i_vcode);
		if (error != 0) {
			goto error_exit;
		}
	}

	if (!UFSVFSP(vfsp)) {
		ip->is_ic = (dp + 1)->di_ic;	/* alternate inode */
		vp->v_macflag |= VMAC_SUPPORT;
		vp->v_lid = ip->i_lid;
		if (ip->i_sflags & ISD_MLD) {
			vp->v_macflag |= VMAC_ISMLD;
		}
	} else
                /*
                 * When mac_installed, this isn't really necessary, since
                 * lookuppn() takes care of setting the level on
                 * non-VMAC_SUPPORT files.  But when !mac_installed, lookuppn()
		 * doesn't touch levels, so we clear the lid here.
                 */
                vp->v_lid = 0;

	brelse(bp);

	if (ip->i_eftflag != (ulong)EFT_MAGIC) {
		ip->i_mode = ip->i_smode;
		ip->i_uid = ip->i_suid;
		ip->i_gid = ip->i_sgid;
		ftype = ip->i_mode & IFMT;

		if (ftype == IFBLK || ftype == IFCHR) {
			ip->i_rdev = expdev(ip->i_oldrdev);
		}
		ip->i_eftflag = (u_long)EFT_MAGIC;
	}
	ip->i_agen = 0;

	/*
	 * Fill in the rest.
	 */
	vp->v_vfsp = vfsp;
	vp->v_stream = NULL;
	vp->v_filocks = NULL;
	vp->v_type = IFTOVT(ip->i_mode);
	vp->v_rdev = ip->i_rdev;
	if (ino == (ino_t)SFSROOTINO) {
		vp->v_flag |= VROOT;
	}

	sfs_vfsp = (sfs_vfs_t *)vp->v_vfsp->vfs_data;
	if (ip->i_mode != 0 && sfs_vfsp->vfs_qinod != NULL) {
		ip->i_dquot = sfs_getinoquota(ip, cr);
	} else {
		ip->i_dquot = NULL;
	}
	if (sfs_vfsp->vfs_flags & SFS_SOFTMOUNT)
		vp->v_flag |= VNOSYNC;


	/*
	 * Now, we wish to make the inode visible to the rest of the world.
	 * Remember to wakeup any potential waiters.
	 *
	 * As soon as we release the VN_LOCK, the vnode can be grabbed
	 * by the VN_TRY_HOLD macro. Therefore, initialization must be
	 * complete at this time.
	 */
	(void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
	ip->i_state = IDENTITY;
	VN_LOCK(vp);
	ASSERT(vp->v_count == 0);
	ASSERT(vp->v_softcnt == 1);
	vp->v_count = 1;
	vp->v_softcnt = 0;
	VN_LOG(vp, "sfs_init_inode");
	VN_UNLOCK(vp);
	UNLOCK(&sfs_inode_table_mutex, PLBASE);
	if (SV_BLKD(&sfs_inode_sv))
		SV_BROADCAST(&sfs_inode_sv, 0);

	return (0);

error_exit:
	brelse(bp);

	/*
	 * Initialization failed. So destroy the identity now.
	 */
	(void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
	sfs_remque(ip);
	UNLOCK(&sfs_inode_table_mutex, PLBASE);
	SFS_IRWLOCK_UNLOCK(ip);

	/*
	 * At this point, we hold a soft count on the vnode,
	 * but not a hard count. All we need to do is release our
	 * soft count, and the vnode will go away.
	 */
	VN_SOFTRELE(vp);

	return (error);
}


/*
 * int
 * sfs_iget(vfs_t *vfsp, fs_t *fs, ino_t ino,
 *           inode_t **ipp, int lkmode, cred_t *cr)
 *	Look up an inode by device, i-number.
 *
 * Calling/Exit State:
 *	On success 0 is returned, <*ipp> is returned locked according to
 *	lkmode. lkmode is composed of:
 *
 *	    at most one of the following:
 *		IG_SHARE	returned read locked
 *		IG_EXCL		returned write locked
 *	    plus the following bits:
 *		IG_NCREATE	do not allocate, just look it up
 *		IG_PR_WARN	print warning if disk inode is not sane
 *
 *	For IG_NCREATE case, if the specified inode is free,
 *	then a ENOENT error is returned.
 *
 *	On failure, an errno value is returned.
 *
 *	This function may block.
 *
 * Description:
 *	Search for the inode on the hash list. If it's there, honor
 *	the locking protocol. If it's not there, try to get an inode
 *	slot by either dynamically allocating one, or by removing an
 *	inode from the free list. If we can, than initialize the
 *	inode from its specified device. Before changing the identity
 *	of an inode, free any pages it has.
 */
int
sfs_iget(vfs_t *vfsp, fs_t *fs, ino_t ino, inode_t **ipp, int lkmode,
	 cred_t *cr)
{
	inode_t		*ip;
	vnode_t		*vp;
	union ihead	*ih;
	int		error = 0;

	MET_IGET(MET_SFS);

	ASSERT(getfs(vfsp) == fs);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	ih = &sfs_ihead[INOHASH(vfsp->vfs_dev, ino)];
loop:

	(void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
	ip = sfs_search_ilist(ih, ino, vfsp);
	if (ip != NULL) {
		/*
		 * If some LWP is creating or destroying the identity of
		 * this inode, then wait till it is finished.
		 */
		ASSERT(ip->i_state & IDENTITY);
		if (ip->i_state & INVALID) {
			SV_WAIT(&sfs_inode_sv, PRINOD, &sfs_inode_table_mutex);
			goto loop;
		}
		/*
		 * Remove the inode from the free list (if necessary).
		 */
		vp = ITOV(ip);
		switch(ip->i_state) {
		case IDENTITY|ITFREE:
			inostats.freeinodes--;
			MET_INODE_INUSE(MET_SFS, 1);
			/* FALLTHROUGH */
		case IDENTITY|IPFREE:
			SFS_FREE_REMOVE(ip);
			VN_LOCK(vp);
			++vp->v_count;
			--vp->v_softcnt;
			VN_LOG(vp, "sfs_iget");
			VN_UNLOCK(vp);
			break;
		default:
			ASSERT(ip->i_state == IDENTITY);
			VN_HOLD(vp);
		}
		SFS_FREE_CHECK();
		UNLOCK(&sfs_inode_table_mutex, PLBASE);

		if ((lkmode & IG_SHARE) != 0) {
			SFS_IRWLOCK_RDLOCK(ip);
		} else if ((lkmode & IG_EXCL) != 0) {
			SFS_IRWLOCK_WRLOCK(ip);
		}

		*ipp = ip;

		return (0);
	}

	/*
	 * Take a free inode and re-assign it to our inode.
	 *
	 * We hold the inode table lock at this point.
	 * sfs_getfree_inode() drops the lock in all cases.
	 *
	 * When sfs_getfree_inode returns an inode, the new inode has been
	 * added to the table, is no longer free, and has both the INVALID
	 * and IDENTITY bits set.
	 */
	error = sfs_getfree_inode(&ip, ih, ino, fs, vfsp, cr);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	if (error != 0) {
		if (error == ENFILE) {
			MET_INODE_FAIL(MET_SFS);
			/*
			 *+ The inode table has filled up.
			 *+ Corrective action: reconfigure
			 *+ the kernel to increase the inode
			 *+ table size.
			 */
			cmn_err(CE_WARN,
				"UFS/SFS sfs_iget: inode table overflow");
		}
		return (error);
	}

	if (ip == NULL) {
		goto loop;
	}

	error = sfs_init_inode(ip, fs, vfsp, ino, lkmode, cr);
	if (error)
		return (error);

	/*
	 * The inode is always returned write locked by sfs_getfree_inode.
	 * Therefore, we might need to downgrade the lock, depending upon
	 * the requirements of the caller.
	 */
	if ((lkmode & IG_SHARE) != 0) {
		SFS_IRWLOCK_UNLOCK(ip);
		SFS_IRWLOCK_RDLOCK(ip);
	} else if ((lkmode & IG_EXCL) == 0) {
		SFS_IRWLOCK_UNLOCK(ip);
	}

	ASSERT(ip->i_number == ino);
	*ipp = ip;

	return (0);
}

/*
 * void
 * sfs_iput(inode_t *ip, cred_t *cr)
 *	Unlock inode and vrele associated vnode
 *
 * Calling/Exit State:
 *	The caller must hold the inode's rwlock in *exclusive* mode
 *	on entry.  On exit, the calling LWP's reference to the inode
 *	is removed and the caller may not reference the inode anymore.
 */
void
sfs_iput(inode_t *ip, cred_t *cr)
{
	vnode_t		*vp = ITOV(ip);
	boolean_t	totally_free;
	struct inode_marker *free_list;
	int		flags;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * At this point, a new reference to the inode can be still be
	 * generated by one of:
	 * 
	 *	=> sfs_tryhold
	 *
	 *	=> sfs_iget
	 *
	 *	=> dnlc_lookup (SOFT DNLC case)
	 *
	 * In all three cases, the LWP generating the reference holds
	 * either the VN_LOCK or the sfs_inode_table_mutex. So, acquire both
	 * locks now. We also acquire the sfs_inode_table_mutex so that we
	 * can transition the free/destroy state of the inode atomically with
	 * changing its v_count/v_softcnt fields.
	 */
	(void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
	ASSERT(ip->i_state == IDENTITY);
	VN_LOCK(vp);
	ASSERT(vp->v_count != 0);
	if (vp->v_count != 1) {
		/*
		 * Someone generated a new reference before we acquired
		 * both locks.
		 *
		 * The new reference is still extant, so that inactivating
		 * the vnode will become someone else's responsibility. Give
		 * up our reference and return.
		 */
		vp->v_count--;
		VN_LOG(vp, "sfs_put-1");
		VN_UNLOCK(vp);
		UNLOCK(&sfs_inode_table_mutex, PLBASE);
		SFS_IRWLOCK_UNLOCK(ip);
		return;
	}

	/*
	 * The reference count is exactly 1, so that now we can be sure
	 * that we really hold the last hard reference.
	 *
	 * We exchange our hard count (v_count) for a soft count (v_softcnt)
	 * in order to suppress any new references via VN_TRY_HOLD once we
	 * give up the VN_LOCK. Even after we give up the VN_LOCK, we will
	 * still hold the sfs_inode_table_mutex, thus inhibiting any
	 * new VN_HOLDs via sfs_iget or sfs_tryhold.
	 *
	 * Note: the pages remain visible to the pageout and fsflush daemons.
	 */
	vp->v_count = 0;
	++vp->v_softcnt;
	VN_LOG(vp, "sfs_put-2");
	VN_UNLOCK(vp);

	/*
	 * If we are removing the file, or if we wish to kmem_free the
	 * storage (because we are over the low water mark), then we
	 * commit to destroying the inode's identity.
	 *
	 * Otherwise, we just free the inode and let the fsflush/pageout
	 * daemons clean the pages/inode.
	 */
	totally_free = (vp->v_softcnt == 1 && vp->v_pages == NULL);
	if (ip->i_nlink <= 0) {
		ip->i_state = (inostats.freeinodes > sfs_inode_lwm) ?
				(IDENTITY|INVALID|IKMFREE) :
				(IDENTITY|INVALID|ILCLFREE);
	} else if (totally_free && inostats.freeinodes > sfs_inode_lwm) {
		ip->i_state = (IDENTITY|INVALID|IKMFREE);
	} else {
		/*
		 * Freeing the inode but not destroying it.
		 */
		if (totally_free) {
			free_list = &sfs_totally_free;
			flags = ITFREE;
			inostats.freeinodes++;
			MET_INODE_INUSE(MET_SFS,-1);
		} else {
			free_list = &sfs_partially_free;
			flags = IPFREE;
		}
		SFS_FREE_TAIL(free_list, ip, flags);
		ASSERT(ip->i_state == (IDENTITY|flags));
		SFS_FREE_CHECK();
		UNLOCK(&sfs_inode_table_mutex, PLBASE);
		SFS_IRWLOCK_UNLOCK(ip);
		return;
	}
	UNLOCK(&sfs_inode_table_mutex, PLBASE);

	/*
	 * At this point, we are committed to destroying the
	 * inode's identity. If destruction fails, then the inode
	 * will go onto the free list. No error handling is needed
	 * here.
	 */
	(void) sfs_idestroy(ip, cr);
}

/*
 * sfs_igrab(inode_t *ip)
 *	Attempt to lock and SOFTHOLD an inode that caller thinks is currently 
 *	referenced but unlocked.
 *
 * Calling/Exit State:
 *	If the inode does not have IDENTITY, is INVALID, or cannot be writ
 *	locked, then return (0), else return (1) [with the inode locked
 *	and with an added soft reference].
 *
 *	Called with the sfs_inode_table_mutex held and returns that
 *	way.
 */
int
sfs_igrab(inode_t *ip)
{
	vnode_t	*vp;

	ASSERT(LOCK_OWNED(&sfs_inode_table_mutex));

	/*
	 * Race analysis:
	 *
	 * The caller needs to know that upon successful return, the
	 * IDENTITY of the ``grabbed'' vnode will not go away. This is
	 * guaranteed by the fact that sfs_idestroy is the only function which
	 * can destroy identity, and that sfs_idestroy needs to acquire the
	 * inode write lock.
	 *
	 * The second problem is that the caller needs to safely unlock
	 * the inode without the storage disappearing beneath the unlock.
	 * This is guaranteed by the SOFTHOLD.
	 *
	 * In order to release the ``grabbed inode'', the caller must first
	 * unlock the inode, and then VN_SOFTRELE the vnode.
	 */

	vp = ITOV(ip);
	switch (ip->i_state) {
	case IDENTITY:
	case IDENTITY|ITFREE:
	case IDENTITY|IPFREE:
		if (SFS_ITRYWRLOCK(ip)) {
			VN_SOFTHOLD(vp);
			return (1);
		}
		break;
	}

	return (0);
}

#ifdef _SFS_SOFT_DNLC
/*
 * boolean_t
 * sfs_tryhold(vnode_t *vp)
 *	Attempt to trade a soft hold on an sfs vnode for a hard hold.
 *
 * Calling/Exit State:
 *	vp is held via a softhold.
 *
 *	On success, vp is return hard held and B_TRUE is returned.
 *	Otherwise, B_FALSE is returned.
 *
 *	In all cases, the soft hold which the caller passed in is
 *	released.
 *
 *	This function does not block.
 */
boolean_t
sfs_tryhold(vnode_t *vp)
{
	inode_t   	*ip = VTOI(vp);
	pl_t		s;

	/*
	 * The following unlocked test of v_softcnt relies upon
	 * ATOMIC access to ints.
	 */
	ASSERT(vp->v_softcnt != 0);

	s = LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
	if (ip->i_state & INVALID) {
		UNLOCK(&sfs_inode_table_mutex, s);
		VN_SOFTRELE(vp);
		return B_FALSE;
	}

	/*
	 * 1. Remove the inode from the free list (if necessary).
	 *
	 * 2. Release the soft hold associated with being on the free list.
	 *
	 * 3. Obtain a hard hold.
	 *
	 * 5. Release the soft hold we were called with.
	 */
	switch(ip->i_state) {
	case IDENTITY|ITFREE:
		inostats.freeinodes--;
		MET_INODE_INUSE(MET_SFS, 1);
		/* FALLTHROUGH */
	case IDENTITY|IPFREE:
		SFS_FREE_REMOVE(ip);
		VN_LOCK(vp);
		++vp->v_count;
		vp->v_softcnt -= 2;
		VN_LOG(vp, "sfs_tryhold-1");
		VN_UNLOCK(vp);
		break;
	default:
		ASSERT(ip->i_state == IDENTITY);
		VN_LOCK(vp);
		++vp->v_count;
		--vp->v_softcnt;
		VN_LOG(vp, "sfs_tryhold-2");
		VN_UNLOCK(vp);
	}
	SFS_FREE_CHECK();
	UNLOCK(&sfs_inode_table_mutex, s);

	return B_TRUE;
}
#endif /* _SFS_SOFT_DNLC */

/*
 * int
 * sfs_sec_cleanup(inode_t *ip)
 *
 * Calling/Exit State:
 *	The inode passed in is being destroyed. The security
 *	information for it is cleaned up here. The inode's
 *	rwlock is held *exclusive* on entry and exit.
 *
 */
int
sfs_sec_cleanup(inode_t *ip)
{
	vnode_t   	*vp = ITOV(ip);
	vfs_t     	*vfsp = vp->v_vfsp;
	sfs_vfs_t	*sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	fs_t      	*fsp = ip->i_fs;
	struct  aclhdr  *ahdrp;
	long            aclcnt = 0;
	uint            bsize;
	daddr_t         aclblk = (daddr_t)0;
	daddr_t         nxt_aclblk;
	buf_t		*bp;

#ifdef CC_PARTIAL
	if (MAC_ACCESS(MACEQUAL, ip->i_lid, CRED()->cr_lid)) {
		CC_COUNT(CC_SPEC_UNLINK, CCBITS_SPEC_UNLINK);
	}
#endif /* CC_PARTIAL */

	/*
	 * The inode level (i_lid) is not
	 * cleared. It is used in determining
	 * a covert channel event in
	 * sfs_dirmakeinode().
	 */

	ip->i_sflags = 0;

	if (ip->i_aclcnt > NACLI) {
		aclcnt = ip->i_aclcnt - NACLI;
		aclblk = ip->i_aclblk;
	}
	while (aclcnt) {
		ASSERT(aclblk != (daddr_t) 0);
		bsize = fragroundup(fsp,
		    aclcnt * sizeof(struct acl) + sizeof(struct aclhdr));
		if (bsize > fsp->fs_bsize) {
			bsize = fsp->fs_bsize;
		}
		bp = pbread(ip->i_dev, NSPF(fsp) * aclblk, bsize);
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			break;
		}

		ahdrp = (struct aclhdr *) (bp->b_addrp);
		aclcnt -= (aclcnt > ahdrp->a_size) ? ahdrp->a_size : aclcnt;
		nxt_aclblk = ahdrp->a_nxtblk;
		ASSERT ((ahdrp->a_size * sizeof(struct acl) +
		    sizeof(struct aclhdr)) <= fsp->fs_bsize);
		brelse(bp);
		if (((sfs_free(ip, aclblk, bsize) != 0)) &&
		    (sfs_vfsp->vfs_flags & SFS_FSINVALID)) {
			return (1);
		}

		aclblk = nxt_aclblk;
	}	/* end "while (aclcnt) " */
	ip->i_aclblk = (daddr_t)0;
	ip->i_aclcnt = ip->i_daclcnt = 0;
	return (0);
}

/*
 * int
 * sfs_idestroy(inode_t *ip, cred_t *cr)
 *	Destroy an inode's identity.
 *
 * Calling/Exit State:
 *	The inode is INVALID, has IDENTITY, is not free, and has a zero
 *	v_count. The caller holds a SOFTHOLD on the vnode. The callers
 *	holds the SFS_IRWLOCK_WRLOCK At exit, both thethe SOFTHOLD and the
 *	SFS_IRWLOCK_WRLOCK are released.
 *
 * 	No SPIN locks are held on entry or exit from this function.
 *
 *	Returns 0 on success and an error code on failure.
 */
int
sfs_idestroy(inode_t *ip, cred_t *cr)
{
	vnode_t		*vp = ITOV(ip);
	vfs_t     	*vfsp = vp->v_vfsp;
	sfs_vfs_t	*sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	mode_t		mode;
	int		err;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(vp->v_count == 0);
	ASSERT(vp->v_softcnt != 0);
	ASSERT(ip->i_state == (INVALID|IDENTITY|IKMFREE) ||
	       ip->i_state == (INVALID|IDENTITY|ILCLFREE) ||
	       ip->i_state == (INVALID|IDENTITY));

	/*
	 * Since the INVALID bit is set, and since vp->v_count == 0,
	 * neither fs_iget nor VN_TRY_HOLD can grab a hard hold on
	 * this inode. However, fsflush/pageout are still able to grab
	 * the pages, and hence can grab a softhold.
	 */
	if (!ip->i_fs->fs_ronly && !(sfs_vfsp->vfs_flags & SFS_FSINVALID)) {
		if (ip->i_nlink <= 0) {
			/* free inode */
			ip->i_gen++;
			ASSERT(ip->i_swapcnt == 0);
			err = sfs_itrunc(ip, 0, cr);
			if (err != 0 &&
			    (sfs_vfsp->vfs_flags & SFS_FSINVALID)) {
				/* 
				 * File system invalidated during
				 * sfs_itrunc. Skip freeing file
				 * blocks & acl blocks
				 */
				goto fs_invalid;
			}
			mode = ip->i_mode;
			ip->i_mode = 0;
			ip->i_smode = 0;
			ip->i_rdev = 0;
			ip->i_oldrdev = 0;
			ip->i_flag |= IUPD|ICHG;
			ip->i_eftflag = 0;
			if (!UFSIP(ip) && (sfs_sec_cleanup(ip) != 0))
				goto fs_invalid;

			if (sfs_vfsp->vfs_flags & SFS_FSINVALID)
				goto fs_invalid;

			sfs_ifree(ip, ip->i_number, mode);
			sfs_iupdat(ip, IUP_FORCE_DELAY);
		} else {
			/*
			 * The unlocked reference to vpages is legal here
			 * because no LWP could have instantiated a
			 * page since the INVALID bit was set (i.e.
			 * v_pages is being mutexed by the INVALID
			 * pseudo lock). Since pages might be in
			 * the process of being aborted, it does assume
			 * ATOMIC reference to pointers.
			 */
			if (vp->v_pages != NULL) {
				/*
				 * Synchronously write and abort the pages
				 * of the vnode and put the pages on the free
				 * list when we are done. This action will
				 * cause all the pages to be written back for
				 * the file now and will allow sfs_update()
				 * to skip over inodes that are on the free
				 * list.
				 */
				pvn_unload(vp);
				err = sfs_syncip(ip, B_INVAL, IUP_FORCE_DELAY);
				if (err)
					goto backout;
			} else {
				sfs_iupdat(ip, IUP_FORCE_DELAY);
			}
		}

		if (UFSIP(ip) && ip->i_dquot != NULL) {
			(void) sfs_chkiq((sfs_vfs_t *)
					ITOV(ip)->v_vfsp->vfs_data,
					ip, (uid_t)ip->i_uid, 0, cr);
			sfs_dqrele(ip->i_dquot, cr);
			ip->i_dquot = NULL;
		}
	} else {
                /*
                 * We need to call pvn_abort_range() to invalidate
                 * all pages before the inode is destroyed even for
		 * a read-only mount or an invalidated file system.
                 */
                pvn_abort_range(vp, 0, 0);
        }

fs_invalid:
	/* Remove from hash. */
	(void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
	sfs_remque(ip);
	UNLOCK(&sfs_inode_table_mutex, PLBASE);

	/*
	 * Unlock the inode.
	 */
	SFS_IRWLOCK_UNLOCK(ip);

#ifdef _SFS_SOFT_DNLC
	/*
	 * Remove the associated vnode from DNLC if necessary.
	 *
	 * The following unlocked reference to
	 * v_softcnt assumes ATOMIC reference to
	 * ints, permitted under the ES/MP memory
	 * model. It relies on the fact that no LWP
	 * is making new entries in DNLC at this
	 * time due to the INVALID bit (i.e. v_softcnt
	 * is being mutexed by the INVALID pseudo lock).
	 */
	if (vp->v_softcnt != 1)
		dnlc_purge_vp(vp);
#endif	/* _SFS_SOFT_DNLC */

	VN_SOFTRELE(vp);

	return 0;

backout:
	/*
	 * Take an extra SOFTHOLD on the vnode. We need two SOFTHOLDs here:
	 *
	 *	(1) To protect the vnode storage during the
	 *	    SFS_IRWLOCK_UNLOCK below.
	 *	(2) For the free list.
	 */
	VN_SOFTHOLD(vp);
	ASSERT(vp->v_softcnt >= 2);

	/*
	 * Recover from a failure to sync the pages of an inode we
	 * were attempting to destroy. Since the inode still has
	 * identity, we just put in on the free list.
	 */
	(void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
	ip->i_state = IDENTITY;
	SFS_FREE_TAIL(&sfs_partially_free, ip, IPFREE);
	SFS_FREE_CHECK();
	UNLOCK(&sfs_inode_table_mutex, PLBASE);

	/*
	 * Unlock the inode. This must be done after sending it to the
	 * free list, in order to prevent races with sfs_igrab.
	 */
	SFS_IRWLOCK_UNLOCK(ip);
	VN_SOFTRELE(vp);

	/*
	 * Remember to wake up anybody waiting.
	 */
	if (SV_BLKD(&sfs_inode_sv))
		SV_BROADCAST(&sfs_inode_sv, 0);

	return err;
}

/*
 * STATIC boolean_t
 * sfs_free_scan(void)
 *	Scan the partially-free list in hopes of rebuild the
 *	totally-free list.
 *
 * Calling/Exit State:
 *	The sfs_inode_table_mutex is held on entry to this function,
 *	throughout execution, and at the exit.
 *
 * Remarks:
 *	Called from the SFS_SCAN_FREE macro at most once in every
 *	period of (sfs_timelag / 4) time.
 *
 *	This routine can take the number of totally-free inodes
 *	over the low water mark (sfs_inode_lwm). This is acceptable,
 *	since the low water mark is only a guideline. This condition
 *	is expected to be rare.
 */
STATIC boolean_t
sfs_free_scan(void)
{
	inode_t *ip, *nip;
	vnode_t *vp;
	boolean_t ret = B_FALSE;

	ASSERT(LOCK_OWNED(&sfs_inode_table_mutex));

	ip = ((inode_t *)(&sfs_partially_free))->i_freeb;
	while (ip != (inode_t *)(&sfs_partially_free)) {
		ASSERT(ip != NULL);
		ASSERT(ip->i_state == (IDENTITY|IPFREE));
		nip = ip->i_freeb;
		ASSERT(nip != NULL);
		vp = ITOV(ip);
		/*
		 * The following unlocked tests of the vnode fields
		 * are legal, since the division of inodes between the
		 * partially and totally free lists is only an optimization.
		 */
		if (vp->v_pages == NULL && vp->v_softcnt == 1) {
			SFS_FREE_REMOVE(ip);
			MET_INODE_INUSE(MET_SFS, -1);
			inostats.freeinodes++;
			SFS_FREE_TAIL(&sfs_totally_free, ip, ITFREE);
			ret = B_TRUE;
		}
		ip = nip;
	}
	sfs_scan_time = lbolt;

	SFS_FREE_CHECK();

	return ret;
}

/*
 * void
 * sfs_iupdat(inode_t *ip, enum iupmode mode)
 *	Check accessed and update flags on an inode structure.
 *	If any are on, update the inode with the (unique) current time.
 *	If waitfor is given, insure I/O order so wait for write to complete.
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode's rwlock in *exclusive* mode.
 *
 * Description:
 * The inode is copied to the buffer cache depending on the value of
 * iuptype:
 *
 *      If iuptype is IUP_SYNC, then the data is copied to the buffer cache
 *      and then bwrite is invoked to release the buffer.
 *
 *      If iuptype is IUP_DELAY, then the data is copied to the buffer cache
 *      and then btwrite is invoked to release the buffer.
 *
 *      If iuptype is IUP_LAZY, then blookup is used to look in the buffer
 *      cache to see if the buffer is present and available.  If the buffer
 *      is present, then the inode is copied to the buffer and the buffer is
 *      released via btwrite.  If the buffer is not present, then no work is
 *      done.
 *
 * Normally, soft mount inodes (VNOSYNC) are skipped over with no action
 * taken. The exception is IUP_FORCE_DELAY, which is always treated as
 * IUP_DELAY.
 *
 * Note that IUP_LAZY should not be used when the incore copy of the inode
 * is about to be destroyed.
 *
 * This gets called at least once during each FS hardening interval, since
 * sfs_sync calls sfs_flushi(SYNCATTR) which in turns calls
 * sfs_iupdat(IUP_DELAY) if the inode has changed.
 *
 * If the IDOW flag is set in the inode, then the buffer should be written
 * via a delayed ordered write; the b_writestrat field of the buffer should
 * be set to dow_strategy_buf.
 */
void
sfs_iupdat(inode_t *ip, enum iupmode mode)
{
	buf_t		*bp;
	fs_t		*fp;
	struct dinode	*dp;
	buf_t *(*lookup)();
	pl_t opl;

	/*
	 * No updates are done for soft mount files, except in the
	 * ``FORCE'' case.
	 */
	if (mode == IUP_FORCE_DELAY)
		mode = IUP_DELAY;
	else if (ITOV(ip)->v_flag & VNOSYNC)
		return;

	fp = ip->i_fs;
	if ((ip->i_flag & (IUPD|IACC|ICHG|IMOD)) != 0) {
		if (fp->fs_ronly) {
			return;
		}
		/*
         	* Note that we leave the IMOD bit on for now.  If the
         	* blookup below returns NULL, then the IMOD bit will remain
         	* on in the inode so that future calls to sfs_iupdat will
         	* continue to try to copy the inode to the buffer cache.
         	* However, if blookup finds the buffer (or bread is called),
         	* and the inode data is copied to the buffer cache, then the
         	* IMOD bit is cleared when the buffer is released via btwrite
         	* or bwrite.
         	*/

		/*
		 * Update the inode's times if necessary
		 */
		opl = SFS_ILOCK(ip);
		IMARK(ip, ip->i_flag);
		SFS_IUNLOCK(ip, opl);

		ip->i_smode = ip->i_mode;

		if (ip->i_uid > 0xffff) {
			ip->i_suid = UID_NOBODY;
		} else {
			ip->i_suid = ip->i_uid;
		}

		if (ip->i_gid > 0xffff) {
			ip->i_sgid = GID_NOBODY;
		} else {
			ip->i_sgid = ip->i_gid;
		}
		if (ip->i_stamp == 0)
                        ip->i_stamp = lbolt;
		if (mode == IUP_LAZY)
                        lookup = blookup;
                else
                        lookup = bread;
                bp = (*lookup)(ip->i_dev,
                        (daddr_t)fsbtodb(fp, itod(fp, ip->i_number)),
                        (int)fp->fs_bsize);
                if (bp == NULL)
                        return;
		if (bp->b_flags & B_ERROR) {
			brelse(bp);
			return;
		}
		dp = (struct dinode *)bp->b_addrp + itoo(fp, ip->i_number);
		dp->di_ic = ip->i_ic;		/* structure assignment */

		if (!UFSIP(ip)) {
			(dp + 1)->di_ic = ip->is_ic;	/* alternate inode */
		}
	/*
         * If this inode is a sym link, and the target is being kept in
         * the i_db array, make sure we zero it out in the disk inode
         * structure before writing the inode to disk.
         */
                if (((ip->i_mode & IFMT) == IFLNK) &&
                                (ip->i_db[1] != 0) &&
                                (ip->i_size <= SHORTSYMLINK)) {
                        bzero((char *)&dp->di_db[1], ip->i_size);
                }

	/*
	 * If the inode has ordering dependencies, then ensure that
	 * it is written via dow_strategy_buf
	 */
		if (ip->i_flag & IDOW) {
			extern void dow_strategy_buf(buf_t *);

			bp->b_writestrat = dow_strategy_buf;
		}
		if (mode == IUP_SYNC) {
			bwrite(bp);
		} else {
			btwrite(bp, ip->i_stamp);
		}
		opl = SFS_ILOCK(ip);
		ip->i_flag &= ~(IMOD | IDOW);
                ip->i_stamp = 0;
		SFS_IUNLOCK(ip, opl);
	}

	return;
}

#define	SINGLE	0	/* index of single indirect block */
#define	DOUBLE	1	/* index of double indirect block */
#define	TRIPLE	2	/* index of triple indirect block */

/*
 * long
 * sfs_truncindir(inode_t *ip, daddr_t bn, daddr_t lastbn, int level)
 *	Release blocks associated with the inode ip and
 *	stored in the indirect block bn.
 *
 * Calling/Exit State:
 *	May be called recursively.
 *
 *	The calling LWP hold's the inode's rwlock in *exclusive* mode
 *	on entry and exit.
 *
 * Description:
 *	Blocks are free'd in LIFO order up to (but not including) <lastbn>.
 *	If level is greater than SINGLE, the block is an indirect
 *	block and recursive calls to sfs_indirtrunc must be used to
 *	cleanse other indirect blocks.
 */
STATIC long
sfs_truncindir(inode_t *ip, daddr_t bn, daddr_t lastbn, int level)
{
	int	i;
	buf_t	*bp;
	buf_t	*copy;
	daddr_t *bap;
	fs_t	*fs = ip->i_fs;
	daddr_t	nb;
	daddr_t	last;
	long	factor;
	int	blocksreleased = 0;
	int	nblocks;
	int	err;

	/*
	 * Calculate index in current block of last
	 * block to be kept.  -1 indicates the entire
	 * block so we need not calculate the index.
	 */
	factor = 1;
	for (i = SINGLE; i < level; i++) {
		factor *= NINDIR(fs);
	}
	last = lastbn;
	if (lastbn > 0) {
		last /= factor;
	}

	nblocks = btodb(fs->fs_bsize);

	/*
	 * Get buffer of block pointers, zero those
	 * entries corresponding to blocks to be free'd,
	 * and update on disk copy first.
	 */
	copy = ngeteblk(fs->fs_bsize);
	bp = bread(ip->i_dev, (daddr_t)fsbtodb(fs, bn), (int)fs->fs_bsize);
	if (bp->b_flags & B_ERROR) {
		brelse(copy);
		brelse(bp);
		return (0);
	}
	bap = bp->b_un.b_daddr;
	bcopy((caddr_t)bap, (caddr_t)copy->b_un.b_daddr, (u_int)fs->fs_bsize);
	bzero((caddr_t)&bap[last + 1],
	    (u_int)(NINDIR(fs) - (last + 1)) * sizeof (daddr_t));
	bwrite(bp);
	bp = copy;
	bap = bp->b_un.b_daddr;

	/*
	 * Recursively free totally unused blocks.
	 */
	for (i = NINDIR(fs) - 1; i > last; i--) {
		nb = bap[i];
		if (nb == 0) {
			continue;
		}
		if (level > SINGLE) {
			blocksreleased +=
			    sfs_truncindir(ip, nb, (daddr_t)-1, level - 1);
		}
		err = sfs_free(ip, nb, (off_t)fs->fs_bsize);
		if (err != 0) {
			return (err);
		}
		blocksreleased += nblocks;
	}

	/*
	 * Recursively free last partial block.
	 */
	if (level > SINGLE && lastbn >= 0) {
		last = lastbn % factor;
		nb = bap[i];
		if (nb != 0) {
			blocksreleased +=
			    sfs_truncindir(ip, nb, last, level - 1);
		}
	}
	brelse(bp);
	return (blocksreleased);
}

/*
 * int
 * sfs_itruncup(inode_t *ip, size_t nsize)
 *	Truncate the inode ip up to nsize size.
 *
 * Calling/Exit State:
 *	The calling LWP holds the inode's rwlock
 *	exclusive on entry.
 */
int
sfs_itruncup(inode_t *ip, size_t nsize)
{
	vnode_t *vp = ITOV(ip);
	fs_t	*fs = ip->i_fs;
	size_t	osize;
	size_t	zsize;
	daddr_t llbn, lbn;
	int	err = 0;
	pl_t opl;

	osize = ip->i_size;
	ASSERT(nsize > osize);

	llbn = lblkno(fs, osize);
	lbn = lblkno(fs, nsize);
	if (lblkno(fs, osize) >= NDADDR || llbn < lbn)
		zsize = blkroundup(fs, osize);
	else
		zsize = fragroundup(fs, nsize);

	if (zsize > nsize)
		zsize = nsize;
	else if (zsize & PAGEOFFSET)
		page_find_unload(vp, zsize & PAGEMASK);

	/*
	 * Call pvn_trunczero to zero till end of
	 * block. As a result of faulting the pages
	 * in for zeroing, it will also cause 
	 * the fragment to be extended to a full
	 * block.
	 */
	ip->i_iolen = zsize;

	err = pvn_trunczero(vp, osize, zsize - osize);

	ip->i_iolen = 0;

	if (err) {
		/*
		 * Need to restore old isize because
		 * isize may have changed.
		 */
		opl = SFS_ILOCK(ip);
		if (ip->i_size != osize) {
			ip->i_size = osize;
		}
		SFS_IUNLOCK(ip, opl);
		return (err);
	}

	/*
	 * If there are no errors, update the inode.
	 */
	opl = SFS_ILOCK(ip);
	ip->i_size = nsize;
	IMARK(ip, IUPD|ICHG);
	SFS_IUNLOCK(ip, opl);
	sfs_iupdat(ip, IUP_LAZY);

	return 0;
}

/*
 * int
 * sfs_itrunc(inode_t *oip, int length, cred_t *cr)
 *	Truncate the inode ip to at most length size.
 *	Free affected disk blocks -- the blocks of the
 *	file are removed in reverse order.
 *
 * Calling/Exit State:
 *	The calling LWP holds the inode's rwlock
 *	exclusive on entry.
 *
 * Description:
 *	In the truncate down case, use delayed ordered writes to
 *	set up an ordering between two actions: the writing of
 *	the inode as being truncated, and moving the inode's blocks
 *	to the free list.  This routine calls sfs_dow_iupdat to
 *	write the inode with delayed ordered writes, and then calls
 *	sfs_freeblocks to set up the function call to move the
 *	blocks to the free list once the inode is written to disk.
 *
 * Remarks:
 *	Note that the freeing of blocks is only delayed if we're
 *	truncating to zero length.
 */
/* ARGSUSED */
int
sfs_itrunc(inode_t *oip, uint_t length, cred_t *cr)
{
	fs_t	*fs = oip->i_fs;
	inode_t	*ip;
	daddr_t	lastblock;
	off_t	osize;
	daddr_t	lastiblock[NIADDR];
	int	level;
	long	blocksreleased = 0;
	int	type;
	inode_t	tip;
	int	i;
	int	err;
	dowid_t	ipdow;
	pl_t	opl;

	/*
	 * We only allow truncation of regular files and directories
	 * to arbritary lengths here.  In addition, we allow symbolic
	 * links to be truncated only to zero length.  Other inode
	 * types cannot have their length set here disk blocks are
	 * being dealt with - especially device inodes where
	 * ip->i_rdev is actually being stored in ip->i_db[1]!
	 */
	type = oip->i_mode & IFMT;

	if (type == IFIFO) {
		return (0);
	}

	osize = oip->i_size;

        /*
         * We only allow truncation of regular files and directories
         * to arbritary lengths here.  In addition, we allow symbolic
         * links to be truncated only to zero length.  Other inode
         * types cannot have their length set here disk blocks are
         * being dealt with - especially device inodes where
         * ip->i_rdev is actually being stored in ip->i_db[1]!
         *
         * the behaviour on Char special devices (actually terminal devices)
         * and on FIFOs is described in POSIX.1-90 page 89 lines 231-232
         *
         * O_TRUNC .... O_TRUNC shall have no effect on FIFO special files
         *           or terminal device files.
         */

	if (type != IFREG && type != IFDIR && !(type == IFLNK && length == 0)) {
		return (0);
	} else if (type == IFLNK) {
                if (length != 0)
                        return (EINVAL);
                if ((osize <= SHORTSYMLINK) && (oip->i_db[1] != 0))
                        bzero((char *)&oip->i_db[1], osize);
        }

	/*
	 * If file size is not changing, mark it as changed for
	 * POSIX. Since there's no space change, no need to write
	 * inode synchronously.
	 */
	if (length == osize) {
		opl = SFS_ILOCK(oip);
		IMARK(oip, ICHG|IUPD);
		SFS_IUNLOCK(oip, opl);
		sfs_iupdat(oip, IUP_LAZY);
		return (0);
	}

	if (length > osize) {
		err = sfs_itruncup(oip, length);
		return (err);
	}

	/* Truncate-down case. */

	/*
	 * If file is currently in use for swap, disallow truncate-down
	 */

	if (oip->i_swapcnt > 0) {
		return (EBUSY);
	}


	/*
	 * Update the pages of the file.  If the file is not being
	 * truncated to a page boundary, the contents of the
	 * pages following the end of the file must be zero'ed
	 * in case it is looked at through mmap'ed address.
	 */
	if ((length & PAGEOFFSET) != 0) {
		int zbytes;
		zbytes = MIN(pageroundup(length), blkroundup(fs, length))
								- length;
		zbytes = MIN(osize, zbytes);
		err = pvn_trunczero(ITOV(oip), length, zbytes);
		if (err)
			return (err);
	}

	/*
	 * Calculate index into inode's block list of
	 * last direct and indirect blocks (if any)
	 * which we want to keep.  Lastblock is -1 when
	 * the file is truncated to 0.
	 */
	lastblock = lblkno(fs, length + fs->fs_bsize - 1) - 1;
	lastiblock[SINGLE] = lastblock - NDADDR;
	lastiblock[DOUBLE] = lastiblock[SINGLE] - NINDIR(fs);
	lastiblock[TRIPLE] = lastiblock[DOUBLE] - NINDIR(fs) * NINDIR(fs);

	/*
	 * The freeing of the inode blocks will be deferred if the file
	 *	is being truncated to zero length and if kmem_alloc is
	 * 	able to allocate a copy of the inode.
	 */
	ip = &tip;

	/*
	 * Fix the file size before abort pages past new EOF.
	 */
	opl = SFS_ILOCK(oip);

	/*
	 * Update file and block pointers
	 * on disk before we start freeing blocks.
	 * If we crash before free'ing blocks below,
	 * the blocks will be returned to the free list.
	 * lastiblock values are also normalized to -1
	 * for calls to sfs_truncindir below.
	 */
	*ip = *oip;		/* structure copy */

	for (level = TRIPLE; level >= SINGLE; level--) {
		if (lastiblock[level] < 0) {
			oip->i_ib[level] = 0;
			lastiblock[level] = -1;
		}
	}
	for (i = NDADDR - 1; i > lastblock; i--) {
		oip->i_db[i] = 0;
	}

	oip->i_size = length;

	IMARK(oip, ICHG|IUPD);
	SFS_IUNLOCK(oip, opl);

	ipdow = sfs_dow_iupdat(oip);	/* delayed ordered iupdat */

	/*
	 * Call pvn_abort_range to abort all pages past the new eof.
	 */
	pvn_abort_range(ITOV(oip), length, 0);

	/*
	 * If the new eof falls in the middle of a page, we need
	 * to wait for any pending io to complete before freeing
	 * any backing store for the portion of the page past eof.
	 */
	if ((length & PAGEOFFSET) != 0)
		page_find_iowait(ITOV(oip), length);

	if (type == IFDIR)
		dow_abort_range(ITOV(oip), length, ip->i_size - length);
#ifdef	DEBUG
	if (type == IFDIR) {
		uint_t mask, size;
		off_t off;

		if (ip->i_fs->fs_bsize < PAGESIZE) {
			mask = ip->i_fs->fs_bmask;
			size = ip->i_fs->fs_bsize;
		} else {
			mask = PAGEMASK;
			size = PAGESIZE;
		}
		off = (length + size - 1) & mask;
		while (off < ip->i_size) {
			ASSERT(!DOW_EXISTS((long)(ITOV(oip)), (long)off));
			off += size;
		}
	}
#endif

	blocksreleased = sfs_freeblocks(ip, length, ipdow);

	dow_rele(ipdow);

	oip->i_blocks -= blocksreleased;

	if (oip->i_blocks < 0 || (length == 0 && oip->i_blocks != 0)) {
		/* sanity */
		/*
		 *+ The number of blocks assigned to a file fell below
		 *+ 0. The kernel corrected the problem and reset the
		 *+ number of blocks to 0.
		 */
		cmn_err(CE_NOTE,
		    "UFS/SFS sfs_itrunc: %s/%d new size = %d, blocks = %d\n",
		    fs->fs_fsmnt, oip->i_number, oip->i_size,
		    oip->i_blocks);
		oip->i_blocks = 0;

	}
	opl = SFS_ILOCK(oip);
	IMARK(oip, ICHG);
	SFS_IUNLOCK(oip, opl);
	if (oip->i_dquot) {
		(void) sfs_chkdq(oip, -blocksreleased, 0, cr);
	}
	return (0);
}

/*
 * int
 * sfs_iflush(vfs_t *vfsp, inode_t *iq, cred_t *cr)
 *	Remove any inodes in the inode cache belonging to dev
 *
 * Calling/Exit State:
 *	The calling LWP does not hold any relevant SFS locks.
 *
 * Description:
 *	There should not be any active ones, return -1 if any are found but
 *	still invalidate others. If <iq> is not NULL (quota file inode
 *	exists), that inode is not removed from the inode cache.
 *
 * Remarks:
 *	This is called from sfs_umount1()/sfs_vfsops.c when dev is being
 *	unmounted and from sfs_mountfs() when a REMOUNT occurs.
 *
 */
/* ARGSUSED */
int
sfs_iflush(vfs_t *vfsp, inode_t *iq, cred_t *cr)
{
	inode_t	*ip, *ipx = NULL;
	union 	ihead *ih;
	int	open = 0;
	vnode_t	*vp;
	vnode_t	*rvp;
	dev_t	dev = vfsp->vfs_dev;
	sfs_vfs_t	*sfs_vfsp;
	struct inode_marker *mp;
	int	error;
	boolean_t lock_dropped;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	SFS_CREATE_MARKER(mp);
	sfs_vfsp = (sfs_vfs_t *)vfsp->vfs_data;
	rvp = sfs_vfsp->vfs_root;

rescan:
	(void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
	lock_dropped = B_FALSE;
	for (ih = &sfs_ihead[0]; ih < &sfs_ihead[INOHSZ]; ih++) {
		for (ip = (inode_t *)ih->ih_chain[0]; ip != (inode_t *)ih;
							ip = ipx) {
			ipx = ip->i_forw;
			/*
			 * Test of i_fs comes first for the ``markers''.
			 */
			if (ip->i_fs != NULL && ip != iq && ip->i_dev == dev) {
			    vp = ITOV(ip);

			    if (vp == rvp && vp->v_count == 2)
				continue;

			    /*
			     * Try to grab the inode. If we fail to grab it,
			     * then one of several things is happening:
			     *
			     * (a) fslush (i.e. sfs_flushi) has its
			     *     hooks on it, or
			     *
			     * (b) an LWP is destroying the identity
			     *     for us in sfs_idestroy, or
			     *
			     * (c) an LWP is creating the identity
			     *     in sfs_getfree_inode.
			     *
			     * (d) an LWP is closing the file in
			     *     sfs_inactive or sfs_iput.
			     *
			     * In cases (b) or (c), the inode is
			     * INVALID, so it suffices to wait on
			     * sfs_inode_sv. In cases (a) and (d),
			     * it suffices to take a soft hold and
			     * wait on the SFS_IRWLOCK_WRLOCK.
			     */
			    lock_dropped = B_TRUE;
			    SFS_INSERT_MARKER(mp, ip);
			    if (!sfs_igrab(ip)) {
				if (ip->i_state & INVALID) {
					SV_WAIT(&sfs_inode_sv, PRINOD,
						&sfs_inode_table_mutex);
				} else {
					VN_SOFTHOLD(vp);
					UNLOCK(&sfs_inode_table_mutex, PLBASE);
					SFS_IRWLOCK_WRLOCK(ip);
					SFS_IRWLOCK_UNLOCK(ip);
					VN_SOFTRELE(vp);
				}
				(void) LOCK(&sfs_inode_table_mutex,
					    FS_SFSLISTPL);
				ipx = mp->im_forw;
				SFS_REMOVE_MARKER(mp);
				continue;
			    }

			    /*
			     * If the file is truly open, then set error
			     * indicator, but sync the file and
			     * continue to move on. However, the root
			     * vnode is expected to have a count of 2.
			     */
			    VN_LOCK(vp);
			    if (vp->v_count != 0) {
				open = -1;
				VN_UNLOCK(vp);
				UNLOCK(&sfs_inode_table_mutex, PLBASE);
				sfs_syncip(ip, 0, IUP_FORCE_DELAY);
				(void) LOCK(&sfs_inode_table_mutex,
					    FS_SFSLISTPL);
				/*
				 * skip over the file
				 */
				ASSERT(ip->i_state & IDENTITY);
				ipx = ip->i_forw;
				SFS_REMOVE_MARKER(mp);
				SFS_INSERT_MARKER(mp, ipx);
				UNLOCK(&sfs_inode_table_mutex, PLBASE);
				SFS_IRWLOCK_UNLOCK(ip);
				VN_SOFTRELE(vp);
			    } else {
				VN_UNLOCK(vp);
				/*
				 * Remove the inode from the free list (if
				 * necessary).
				 */
				switch(ip->i_state) {
				case IDENTITY|ITFREE:
					inostats.freeinodes--;
					MET_INODE_INUSE(MET_SFS, 1);
					/* FALLTHROUGH */
				case IDENTITY|IPFREE:
					SFS_FREE_REMOVE(ip);
					VN_SOFTRELE(vp);
					break;
				}
				SFS_FREE_CHECK();
				ASSERT(ip->i_state == IDENTITY);

				/*
				 * Okay, destroy the identity.
				 */
				ip->i_state =
				    (inostats.freeinodes > sfs_inode_lwm) ?
					    (IDENTITY|INVALID|IKMFREE) :
					    (IDENTITY|INVALID|ILCLFREE);
				UNLOCK(&sfs_inode_table_mutex, PLBASE);

				/*
				 * sfs_idestroy will release both the
				 * inode RWLOCK and the SOFTHOLD.
				 */
				error = sfs_idestroy(ip, sys_cred);
				if (error)
				    open = -1;
			   }
			   (void) LOCK(&sfs_inode_table_mutex, FS_SFSLISTPL);
			   ipx = mp->im_forw;
			   SFS_REMOVE_MARKER(mp);
			}
		}
	}
	UNLOCK(&sfs_inode_table_mutex, PLBASE);

	/*
	 * While we were scanning the table, somebody could have exchanged
	 * a hold ahead of the scan for one behind the scan. So therefore,
	 * we must rescan the table if the lock was dropped.
	 *
	 * PERF: The lock hold time associated with this approach is not
	 *	 good. However, it is better to hold the lock than to
	 *	 allow vnodes to be owned by an unmounted file system -
	 *	 which will lead to strange memory stepping phenomena.
	 */
	if (open == 0 && lock_dropped)
		goto rescan;

	SFS_DESTROY_MARKER(mp);

	return (open);
}

/*
 * int
 * sfs_iaccess(inode_t *ip, mode_t mode, cred_t *cr)
 * 	Check mode permission on an inode.
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode's rwlock in 
 *	*at least* shared mode (it may be held in exclusive mode).
 *
 * Description:
 *	Mode is IREAD, IWRITE, or IEXEC. For IWRITE, the read-only
 *	status of the file system is checked first. The only types
 *	of files from a read-only file system that may be written
 *	to are: fifos, block special, and character special devices.
 *
 *	The mode permission is actually checked by sfs_daccess.
 */
int
sfs_iaccess(inode_t *ip, mode_t mode, cred_t *cr)
{
	mode_t	denied_mode;

	if (mode & IWRITE) {
		/*
		 * Disallow write attempts on read-only
		 * file systems, unless the file is a block
		 * or character device or a FIFO.
		 */
		if (ip->i_fs->fs_ronly != 0) {
			if ((ip->i_mode & IFMT) != IFCHR &&
			    (ip->i_mode & IFMT) != IFBLK &&
			    (ip->i_mode & IFMT) != IFIFO) {
				return (EROFS);
			}
		}
	}

	/*
         *      Perform Discretionary Access Check
         */
	denied_mode = sfs_daccess(ip, mode, cr);
	if (denied_mode == 0) {
		return (0);
	}

	if ((long)denied_mode < 0) {
		/* it is assumed sfs_daccess returns -1 for EIO */
		return (EIO);
	} else {
		if ((denied_mode & (IREAD|IEXEC)) && pm_denied(cr, P_DACREAD)) {
			return (EACCES);
		}
		if ((denied_mode & IWRITE) && pm_denied(cr, P_DACWRITE)) {
			return (EACCES);
		}
		return (0);
	}
}

/*
 * mode_t
 * sfs_daccess(inode_t *ip, mode_t mode, cred_t *cr)
 *	Check discretionary access permissions of an inode.
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode's rwlock in *at least*
 *	shared mode (it may be exclusive mode). 
 *
 * Description:
 *      processing -    if effective uid == owner of file, use file owner bits
 *                      if no ACL entries & effective gid == owning group,
 *                              use file group bits.
 *                      scan ACL looking for a matching user or group,
 *                              and use matching entry permissions.
 *                              Use total permissions of all matching group
 *                              entries, until ACL is exhausted.
 *                      file group bits mask permissions allowed by ACL.
 *                      if not owner, owning group, or matching entry in ACL,
 *                              use file other bits.
 *
 *      output -        0 if permission granted, otherwise denied permissions.
 *			-1 indicates we received an I/O error from pbread().
 *
 *			NOTE: sfs_iaccess expects the denied permissions
 *			to be contained in the 1st octal permissions bit
 *			(e.g. 0400, 0200) for comparison with IREAD,IWRITE,
 *			or IEXEC prior to privilege checking.
 *			So we must make sure to shift the denied access
 *			to the correct position before returning it.
 *
 */
/* ARGSUSED */
STATIC mode_t
sfs_daccess(inode_t *ip, mode_t mode, cred_t *cr)
{
	mode_t         workmode = 0;
	mode_t         reqmode;
	mode_t         basemode;
	buf_t          *bp = NULL;
	fs_t           *fsp = ip->i_fs;
	struct acl     *aclp;
	int            cnt, i;
	struct aclhdr  *ahdrp;
	daddr_t        aclblk;
	long           bsize;
	int            idmatch = 0;
	long           entries = 0;


	/*
         *      check if effective uid == owner of file
         */

	if (cr->cr_uid == ip->i_uid) {
		if ((workmode = (ip->i_mode & mode)) == mode) {
			return (0);
		} else {
			return (mode & ~workmode);
		}
	}
	mode >>= 3;
	/*
         *      If there's no ACL, check only the group &
         *      other permissions (Just like the "Good Old Days")
         */
	if ((UFSIP(ip)) || dac_installed == 0 ||
	    ((entries = (long)(ip->i_aclcnt - ip->i_daclcnt)) == 0)) {
		if (groupmember(ip->i_gid, cr)) {
			if ((workmode = (ip->i_mode & mode)) == mode) {
				return (0);
			} else {
				return ((mode & ~workmode) << 3);
			}
		} else {
			goto other_ret;
		}
	}

	/*      set up requested & base permissions */
	reqmode = (mode >> 3) & 07;
	basemode = (ip->i_mode >> 3) & 07;

	/*
         *      Check the ACL for a matching entry
         *      first, the entries in the inode are checked
         */

	aclp = ip->i_acl;
	cnt = (entries > NACLI) ? NACLI : entries;
	entries -= cnt;
	aclblk = ip->i_aclblk;

	while (cnt) {
		for (i = cnt; i > 0; i--, aclp++) {
			switch (aclp->a_type) {
			case USER:
				if (cr->cr_uid == aclp->a_id) {
					if ((workmode = ((aclp->a_perm & 
					    reqmode) & basemode)) == 
					    reqmode) {
						/*
						 * Matching USER entry found,
						 * access granted
						 */
						if (bp) {
							brelse(bp);
						}
						return (0);
					} else {
						/*
						 * Matching USER entry found,
						 * access not granted
						 */
						if (bp) {
							brelse(bp);
						}
						return ((reqmode & ~workmode)
							<< 6);
					}
				}
				break;
			case GROUP_OBJ:
				if (groupmember(ip->i_gid, cr)) {
					if ((workmode |= (aclp->a_perm
					    & reqmode)) == reqmode) {
						goto match;
					} else {
						idmatch++;
					}
				}
				break;
			case GROUP:
				if (groupmember(aclp->a_id, cr)) {
					if ((workmode |= (aclp->a_perm
					    & reqmode)) == reqmode) {
						goto match;
					} else {
						idmatch++;
					}
				}
				break;
			}       /* end switch statement */
		}       /* end for statement */

		/*
                 *      next, check entries in each disk block
                 */

		if (bp) {
			brelse(bp);
		}
		if (entries) {
			ASSERT(aclblk != 0);
			/* compute size of all remaining ACL entries */
			bsize = fragroundup(fsp, entries * sizeof(struct acl) +
					    sizeof(struct aclhdr));
			/* if bigger than a logical block, just read a block */
			if (bsize > fsp->fs_bsize) {
				bsize = fsp->fs_bsize;
			}
			bp = pbread(ip->i_dev, NSPF(fsp) * aclblk, bsize);
			if (bp->b_flags & B_ERROR) {
				brelse(bp);
				return ((mode_t)-1);
			}
			ahdrp = (struct aclhdr *)(bp->b_addrp);
			cnt =  (entries > ahdrp->a_size) ?
				ahdrp->a_size : entries;
			entries -= cnt;
			aclblk = ahdrp->a_nxtblk;

			aclp = (struct acl *)(ahdrp + 1);
		} else {
			cnt = 0;
		}
	}       /* end while statement */
	if (idmatch) {
		/*
                 *      Matching GROUP or GROUP_OBJ entries
                 *      were found, but  did not grant the access.
                 */
		return ((reqmode & ~(workmode & basemode)) << 6);
	}

other_ret:

	/*
         *      Not the file owner, and either
         *      no ACL, or no match in ACL.
         *      Now, check the file permissions.
         */

	mode >>= 3;
	if ((workmode = (ip->i_mode & mode)) == mode) {
		return (0);
	} else {
		return ((mode & ~workmode) << 6);
	}

match:
	/*
         *      Access granted by GROUP_OBJ or GROUP ACL entry or entries
         */

	if (bp) {
		brelse(bp);
	}

	/*
         *      File Group Class Bits mask access,
         *      so determine whether matched entries
         *      should really grant access.
         */
	if ((workmode & basemode) == reqmode) {
		return (0);
	} else {
		return ((reqmode & ~(workmode & basemode)) << 6);
	}
}

/*
 * void
 * sfs_remque(inode_t *ip)
 *	Remove an inode from the hash chain it's on.
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode table lock.
 */
void
sfs_remque(inode_t *ip)
{
	ASSERT(LOCK_OWNED(&sfs_inode_table_mutex));
	ASSERT(ip->i_state & IDENTITY);
	ASSERT(!(ip->i_state & (ITFREE|IPFREE)));

	ip->i_back->i_forw      = ip->i_forw;
	ip->i_forw->i_back      = ip->i_back;
	ip->i_state &= ~IDENTITY;
	return;
}

/*
 * void
 * sfs_insque(inode_t *ip, union ihead *hip)
 *	Insert an inode into a hash chain.
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode table lock.
 */
void
sfs_insque(inode_t *ip, union ihead *hip)
{
	ASSERT(LOCK_OWNED(&sfs_inode_table_mutex));
	ASSERT(!(ip->i_state & (IDENTITY|ITFREE|IPFREE)));

	hip->ih_chain[0]->i_back = ip;
	ip->i_forw               = hip->ih_chain[0];
	hip->ih_chain[0]         = ip;
	ip->i_back               = (inode_t *) hip;
	ip->i_state |= IDENTITY;

	return;
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)

#ifdef	DEBUG
extern void print_vnode(const vnode_t *vp);
#endif	/* DEBUG */

/*
 * void
 * print_sfs_inode(const inode_t *ip)
 *	Print an sfs inode.
 *
 * Calling/Exit State:
 *	No locking.
 *
 * Remarks:
 *	Intended for use from a kernel debugger.
 */
void
print_sfs_inode(const inode_t *ip)
{
	if (ip->i_state == IMARKER) {
		debug_printf("ip = 0x%x (INODE MARKER)\n", ip);
		return;
	}
	debug_printf("ip = 0x%x, iflag = %8x, istate = %8x, inum = %d\n",
		     ip, ip->i_flag, ip->i_state, ip->i_number);
	debug_printf("\tlinks = %d, imode = %06o, isize = %d, iblocks = %d\n",
		     ip->i_nlink, ip->i_mode, ip->i_size, ip->i_blocks);
#ifdef	DEBUG
	print_vnode(ITOV(ip));
#else
	debug_printf("\tvnode = %lx, vcount = %d, vsoftcnt = %d\n",
		     ITOV(ip), ITOV(ip)->v_count, ITOV(ip)->v_softcnt);
#endif	/* DEBUG */
}

/*
 * void
 * print_ufs_inode(const inode_t *ip)
 *	Print a ufs inode.
 *
 * Calling/Exit State:
 *	No locking.
 *
 * Remarks:
 *	Intended for use from a kernel debugger.
 */
void
print_ufs_inode(const inode_t *ip)
{
	print_sfs_inode(ip);
}

/*
 * void
 * print_sfs_hash(void)
 *	Dumps the inode cache.
 *
 * Calling/Exit State:
 *	No locking.
 */
void
print_sfs_hash(void)
{
	inode_t	*ip, *ipx = NULL;
	union 	ihead *ih;

	for (ih = &sfs_ihead[0]; ih < &sfs_ihead[INOHSZ]; ih++) {
		for (ip = (inode_t *)ih->ih_chain[0]; ip != (inode_t *)ih;
							ip = ipx) {
			ipx = ip->i_forw;
			print_sfs_inode(ip);
			if (debug_output_aborted())
				return;
		}
	}
}

/*
 * void
 * print_sfs_freelist(void)
 *	Dumps the inode free list.
 *
 * Calling/Exit State:
 *	No locking.
 */
void
print_sfs_freelist(void)
{
	inode_t	*ip;

	
	debug_printf("===== totally free list:\n");

	ip = ((inode_t*)(&sfs_totally_free))->i_freef;
	while (ip != (inode_t*)(&sfs_totally_free)) {
		print_sfs_inode(ip);
		if (debug_output_aborted())
			return;
		ip = ip->i_freef;
	}

	debug_printf("===== partially free list:\n");

	ip = ((inode_t*)(&sfs_partially_free))->i_freef;
	while (ip != (inode_t*)(&sfs_partially_free)) {
		print_sfs_inode(ip);
		if (debug_output_aborted())
			return;
		ip = ip->i_freef;
	}
}

#endif /* DEBUG || DEBUG_TOOLS */

#ifdef DEBUG

/*
 * STATIC int
 * sfs_indcount(inode_t *ip, daddr_t bn, daddr_t lastbn, int level)
 *	Count ind. blocks allocated.
 *
 * Calling/Exit State:
 *	No locking.
 */
STATIC int
sfs_indcount(inode_t *ip, daddr_t bn, daddr_t lastbn, int level)
{
	int	i;
	buf_t	*bp;
	daddr_t *bap;
	fs_t	*fs = ip->i_fs;
	daddr_t	nb;
	daddr_t	last;
	long	factor;
	int	bcount = 0;
	int	nblocks;

	/*
	 * Calculate index in current block of last
	 * block to be kept.  -1 indicates the entire
	 * block so we need not calculate the index.
	 */
	factor = 1;
	for (i = SINGLE; i < level; i++) {
		factor *= NINDIR(fs);
	}
	last = lastbn;
	if (lastbn > 0) {
		last /= factor;
	}

	nblocks = btodb(fs->fs_bsize);

	/*
	 * Get buffer of block pointers, zero those
	 * entries corresponding to blocks to be free'd,
	 * and update on disk copy first.
	 */
	bp = bread(ip->i_dev, (daddr_t)fsbtodb(fs, bn), (int)fs->fs_bsize);
	if (bp->b_flags & B_ERROR) {
		brelse(bp);
		return (0);
	}
	bap = bp->b_un.b_daddr;

	/*
	 * Recursively count ind blocks.
	 */
	for (i = 0; i < NINDIR(fs) - 1; i++) {
		nb = bap[i];
		if (nb == 0) {
			continue;
		}
		if (level > SINGLE) {
			bcount +=
			    sfs_indcount(ip, nb, (daddr_t)-1, level - 1);
		}
		bcount += nblocks;
	}

	brelse(bp);
	return (bcount);
}

/*
 * void
 * sfs_bcheck(inode_t *)
 *	Checks file size against bd.
 *
 * Calling/Exit State:
 *	No locking.
 */
void
sfs_bcheck(inode_t *ip)
{
	fs_t	*fs = ip->i_fs;
	daddr_t	lastblock;
	off_t	bsize;
	daddr_t	bn;
	daddr_t	lastiblock[NIADDR];
	int	level;
	long	nblocks;
	long	bcount = 0;
	int	i;

	/*
	 * Calculate index into inode's block list of
	 * last direct and indirect blocks (if any)
	 * which we want to keep.  Lastblock is -1 when
	 * the file is truncated to 0.
	 */
	lastblock = lblkno(fs, ip->i_size + fs->fs_bsize - 1) - 1;
	lastiblock[SINGLE] = lastblock - NDADDR;
	lastiblock[DOUBLE] = lastiblock[SINGLE] - NINDIR(fs);
	lastiblock[TRIPLE] = lastiblock[DOUBLE] - NINDIR(fs) * NINDIR(fs);
	nblocks = btodb(fs->fs_bsize);

	/*
	 * Indirect blocks first.
	 */
	for (level = TRIPLE; level >= SINGLE; level--) {
		bn = ip->i_ib[level];
		if (bn != 0) {
			bcount += sfs_indcount(ip, bn, lastiblock[level], level);
			bcount += nblocks;
		}
	}

	/*
	 * All direct blocks or frags.
	 */
	for (i = 0; i <= lastblock && i < NDADDR; i++) {
		bn = ip->i_db[i];
		if (bn == 0)
			continue;
		bsize = (off_t)blksize(fs, ip->i_size, i);
		bcount += btodb(bsize);
	}

	if (ip->i_blocks != bcount) {
		cmn_err(CE_NOTE,
		  "UFS/SFS sfs_bcheck: %s/0x%x size = 0x%x, blocks = 0x%x, bcount = 0x%x\n",
		  fs->fs_fsmnt, ip->i_number, ip->i_size, ip->i_blocks, bcount);
	}
	return;
}

/*
 * int
 * sfs_assfail(inode_t *ip, const char *a, const char *f, int l)
 *	Panic when an SFS_ASSERT fails, but also capture ip.
 * 
 * Calling/Exit State:
 *	None.
 */
int
sfs_assfail(inode_t *ip, const char *a, const char *f, int l)
{
	cmn_err(CE_WARN, "INODE FAILURE at 0x%x\n", (long)ip);
	assfail(a, f, l);
	/* NOTREACHED */
}

/*
 * void
 * sfs_free_check(void)
 *	Perform basic checks on the free list.
 *
 * Calling/Exit State:
 *	The sfs_inode_table_mutex is held.
 */
void
sfs_free_check(void)
{
	inode_t	*ip, *fip, *lip;
	int count = 0;

	ASSERT(LOCK_OWNED(&sfs_inode_table_mutex));

	fip = lip = ip = (inode_t *)(&sfs_totally_free);
	ip = ip->i_freef;
	while (ip != fip) {
		ASSERT(ip->i_state == ITFREE ||
		       ip->i_state == (IDENTITY|ITFREE));
		ASSERT(ip->i_freeb == lip);
		lip = ip;
		ip = ip->i_freef;
		++count;
	}
	ASSERT(ip->i_freeb == lip);

	ASSERT(inostats.freeinodes == count);

	fip = lip = ip = (inode_t *)(&sfs_partially_free);
	ip = ip->i_freef;
	while (ip != fip) {
		ASSERT(ip->i_state == (IDENTITY|IPFREE));
		ASSERT(ip->i_freeb == lip);
		lip = ip;
		ip = ip->i_freef;
		++count;
	}
	ASSERT(ip->i_freeb == lip);

	ASSERT(count <= inostats.totalinodes);
}

#endif /* DEBUG */

/*
 * STATIC long
 * sfs_freeblocks_now(inode_t *ip, uint_t length)
 *	Free the blocks used by inode ip, truncating it to the specified
 *	length.
 *
 * Calling/Exit State:
 *	The calling LWP holds the inode's rwlock
 *	exclusive on entry.
 *
 *	Returns the number of blocks actually freed.
 *
 * Remarks:
 *	ip is actually a stale copy of the inode whose blocks
 *	are to be freed; it drives the process of freeing the blocks.
 *
 *	Called from sfs_freeblocks and sfs_freeblocks_delayed.
 */
STATIC long
sfs_freeblocks_now(inode_t *ip, uint_t length)
{
	fs_t	*fs = ip->i_fs;
	daddr_t	lastblock;
	off_t	bsize;
	daddr_t	bn;
	daddr_t	lastiblock[NIADDR];
	int	level;
	long	nblocks;
	long	blocksreleased = 0;
	int	i;
	int	err;

	/*
	 * We only allow truncation of regular files and directories
	 * to arbritary lengths here.  In addition, we allow symbolic
	 * links to be truncated only to zero length.  Other inode
	 * types cannot have their length set here disk blocks are
	 * being dealt with - especially device inodes where
	 * ip->i_rdev is actually being stored in ip->i_db[1]!
	 */
	ASSERT(((ip->i_mode & IFMT) == IFREG) ||
		((ip->i_mode & IFMT) == IFDIR) ||
		(((ip->i_mode & IFMT) == IFLNK) && (length == 0)));
	ASSERT(ip->i_swapcnt <= 0);

	/*
	 * Calculate index into inode's block list of
	 * last direct and indirect blocks (if any)
	 * which we want to keep.  Lastblock is -1 when
	 * the file is truncated to 0.
	 */
	lastblock = lblkno(fs, length + fs->fs_bsize - 1) - 1;
	lastiblock[SINGLE] = lastblock - NDADDR;
	lastiblock[DOUBLE] = lastiblock[SINGLE] - NINDIR(fs);
	lastiblock[TRIPLE] = lastiblock[DOUBLE] - NINDIR(fs) * NINDIR(fs);
	nblocks = btodb(fs->fs_bsize);
	for (level = TRIPLE; level >= SINGLE; level--)
		if (lastiblock[level] < 0)
			lastiblock[level] = -1;

	/*
	 * Indirect blocks first.
	 */
	for (level = TRIPLE; level >= SINGLE; level--) {
		bn = ip->i_ib[level];
		if (bn != 0) {
			blocksreleased +=
			    sfs_truncindir(ip, bn, lastiblock[level], level);
			if (lastiblock[level] < 0) {
#ifdef DEBUG
				ip->i_ib[level] = 0;
#endif
				err = sfs_free(ip, bn, (off_t)fs->fs_bsize);
				if (err != 0) {
					return (err);
				}
				blocksreleased += nblocks;
			}
		}
		if (lastiblock[level] >= 0)
			goto done;
	}

	/*
	 * All whole direct blocks or frags.
	 */
	for (i = NDADDR - 1; i > lastblock; i--) {
		bn = ip->i_db[i];
		if (bn == 0)
			continue;
#ifdef DEBUG
		ip->i_db[i] = 0;
#endif
		bsize = (off_t)blksize(fs, ip->i_size, i);
		err = sfs_free(ip, bn, bsize);
		if (err != 0) {
			return (err);
		}
		blocksreleased += btodb(bsize);
	}
	if (lastblock < 0) {
		goto done;
	}

	/*
	 * Finally, look for a change in size of the
	 * last direct block; release any frags.
	 */
	bn = ip->i_db[lastblock];
	if (bn != 0) {
		off_t oldspace, newspace;

		/*
		 * Calculate amount of space we're giving
		 * back as old block size minus new block size.
		 */
		oldspace = blksize(fs, ip->i_size, lastblock);
		ip->i_size = length;
		newspace = blksize(fs, ip->i_size, lastblock);
		ASSERT(newspace != 0);
		if (oldspace - newspace > 0) {
			/*
			 * Block number of space to be free'd is
			 * the old block # plus the number of frags
			 * required for the storage we're keeping.
			 */
			bn += numfrags(fs, newspace);
			err = sfs_free(ip, bn, oldspace - newspace);
			if (err != 0) {
				return (err);
			}
			blocksreleased += btodb(oldspace - newspace);
		}
	}
done:
	return (blocksreleased);
}

/*
 * STATIC void
 * fip_copyin(freeblkinfo_t *fip, inode_t *ip)
 *	Copy data into a freeblkinfo_t from an inode.
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
fip_copyin(freeblkinfo_t *fip, inode_t *ip, uint_t length)
{
	int i;

	fip->fi_forw = fip->fi_back = (inode_t *)fip;
	fip->fi_freef = fip->fi_freeb = (inode_t *)fip;
	fip->fi_state = 0;
	fip->fi_fs = ip->i_fs;
	fip->fi_vfsp = ITOV(ip)->v_vfsp;
	fip->fi_dev = ip->i_dev;
	fip->fi_number = ip->i_number;
	for (i = 0 ; i < NDADDR ; ++i)
		fip->fi_db[i] = ip->i_db[i];
	for (i = 0 ; i < NIADDR ; ++i)
		fip->fi_ib[i] = ip->i_ib[i];
	fip->fi_osize = ip->i_size;
	fip->fi_nsize = length;
	fip->fi_blocks = ip->i_blocks;
#ifdef	DEBUG
	fip->fi_mode = ip->i_mode;
	fip->fi_swapcnt = ip->i_swapcnt;
#endif
}


/*
 * STATIC void
 * fip_copyout(freeblkinfo_t *fip, inode_t *ip, uint_t *lengthp)
 *	Copy data out of a freeblkinfo_t into an inode.  Also
 *	store the length in <*lengthp>.
 *
 * Calling/Exit State:
 *	None
 */
STATIC void
fip_copyout(freeblkinfo_t *fip, inode_t *ip, uint_t *lengthp)
{
	int i;

	ip->i_fs = fip->fi_fs;
	ITOV(ip)->v_vfsp = fip->fi_vfsp;
	ip->i_dev = fip->fi_dev;
	ip->i_number = fip->fi_number;
	for (i = 0 ; i < NDADDR ; ++i)
		ip->i_db[i] = fip->fi_db[i];
	for (i = 0 ; i < NIADDR ; ++i)
		ip->i_ib[i] = fip->fi_ib[i];
	ip->i_size = fip->fi_osize;
	ip->i_blocks = fip->fi_blocks;
	*lengthp = fip->fi_nsize;
#ifdef	DEBUG
	ip->i_mode = fip->fi_mode;
	ip->i_swapcnt = fip->fi_swapcnt;
#endif
}


/*
 * STATIC void
 * sfs_freeblocks_delayed(freeblkinfo_t *fip)
 *	Free the blocks specified by the freeblkinfo_t.
 *
 * Calling/Exit State:
 *	Called from PLBASE, no spin locks held.
 *	ip is a copy of an inode which is on the deferred free list.
 *
 * Description:
 *	Free all the blocks in the freeblkinfo_t.  Also do bookkeeping
 *	on the deferred free block list, and kmem_free the freeblkinfo_t.
 *
 * Remarks:
 *	Called through delayed ordered writes after the corresponding
 *	inode was written to disk.
 */
STATIC void
sfs_freeblocks_delayed(freeblkinfo_t *fip)
{
	sfs_vfs_t *sfs_vfsp;
	pl_t pl;
	inode_t inode;
	uint_t length;

	sfs_vfsp = SFS_VFS_PRIV(fip->fi_vfsp);
	pl = SFS_DEFLOCK(sfs_vfsp);
	((freeblkinfo_t *)fip->fi_forw)->fi_back = fip->fi_back;
	((freeblkinfo_t *)fip->fi_back)->fi_forw = fip->fi_forw;
	sfs_vfsp->vfs_defer_blocks -= fip->fi_blocks;
	++sfs_vfsp->vfs_defer_list.im_state;
	SFS_DEFUNLOCK(sfs_vfsp, pl);
	fip_copyout(fip, &inode, &length);
	(void)sfs_freeblocks_now(&inode, length);
	kmem_free(fip, sizeof(*fip));
	pl = SFS_DEFLOCK(sfs_vfsp);
	if (--sfs_vfsp->vfs_defer_list.im_state == 0)
		SFS_DEFSIGNAL(sfs_vfsp);
	ASSERT(sfs_vfsp->vfs_defer_list.im_state >= 0);
	SFS_DEFUNLOCK(sfs_vfsp, pl);
}

/*
 * STATIC boolean_t
 * sfs_freeblocks_setup(freeblkinfo_t *fip, dowid_t ipdow)
 *	Arrange for the blocks of an inode to be freed after the
 *	inode is written to disk.
 *
 * Calling/Exit State:
 *	Holds inode write locked on entry and exit.
 *
 * Description:
 *	Use delayed ordered writes to:
 *		(1) create a dow for a function to free the inode's blocks.
 *		(2) have the function called after the inode gets written
 *			to disk (ipdow is a dow for the inode buffer)
 *
 *	If any of the setup fails, then the whole thing gets handled
 *	synchronously, i.e., the inode will get written out and then the
 *	blocks will be freed.
 *
 * Remarks:
 *	See sfs_dow.c to see how some of the recovery from setup failures
 *	takes place.
 */
STATIC boolean_t
sfs_freeblocks_setup(freeblkinfo_t *fip, dowid_t ipdow)
{
	dowid_t fdow;
	sfs_vfs_t *sfs_vfsp;
	freeblkinfo_t *headp;
	pl_t pl;

	fdow = dow_create_func(sfs_freeblocks_delayed, fip, DOW_NO_RESWAIT,
		B_TRUE);
	sfs_dow_order(fdow, ipdow);
	if (fdow == DOW_NONE)
		return (B_FALSE);
	sfs_vfsp = SFS_VFS_PRIV(fip->fi_vfsp);
	headp = (freeblkinfo_t *)&sfs_vfsp->vfs_defer_list;
	pl = SFS_DEFLOCK(sfs_vfsp);
	fip->fi_forw = (inode_t *)headp;
	fip->fi_back = headp->fi_back;
	headp->fi_back = (inode_t *)fip;
	((freeblkinfo_t *)fip->fi_back)->fi_forw = (inode_t *)fip;
	sfs_vfsp->vfs_defer_blocks += fip->fi_blocks;
	SFS_DEFUNLOCK(sfs_vfsp, pl);
	dow_setmod(fdow, sfs_hardening.sfs_remove);
	dow_rele(fdow);
	return (B_TRUE);
}

/*
 * STATIC long
 * sfs_freeblocks(inode_t *ip, uint_t length, dowid_t ipdow)
 *	Free the blocks used by inode ip, truncating it to the specified
 *	length.
 *
 * Calling/Exit State:
 *	The calling LWP holds the inode's rwlock
 *	exclusive on entry.
 *
 *	Returns the number of blocks actually freed.
 *
 * Remarks:
 *	ip is actually a stale copy of the inode whose blocks
 *	are to be freed; it drives the process of freeing the blocks.
 *
 *	Called from sfs_itrunc.
 */
STATIC long
sfs_freeblocks(inode_t *ip, uint_t length, dowid_t ipdow)
{
	freeblkinfo_t *fip;
	long blocksreleased;

	if ((length == 0) && (ipdow != DOW_NONE) &&
			((fip = kmem_alloc(sizeof(*fip), KM_NOSLEEP))
			!= NULL)) {
		fip_copyin(fip, ip, length);
		if (sfs_freeblocks_setup(fip, ipdow))
			blocksreleased = ip->i_blocks;
		else {
			kmem_free(fip, sizeof(*fip));
			blocksreleased = sfs_freeblocks_now(ip, length);
		}
	} else {
		dow_handle_sync(ipdow);
		blocksreleased = sfs_freeblocks_now(ip, length);
	}
	return (blocksreleased);
}

/*
 * boolean_t
 * sfs_freeblocks_reclaim(vfs_t *vfsp)
 *	Try to reclaim blocks from the deferred free block list.
 *
 * Calling/Exit State:
 *	Holds no locks
 *
 *	Returns B_TRUE if blocks may have been reclaimed; B_FALSE
 *	if there are no blocks left on the deferred free block list.
 *
 * Remarks:
 *	Called from block allocation path when there are no more free
 *	blocks.
 */
boolean_t
sfs_freeblocks_reclaim(vfs_t *vfsp)
{
	sfs_vfs_t *sfs_vfsp = SFS_VFS_PRIV(vfsp);
	freeblkinfo_t *headp, *fip;
	pl_t pl;
	dowid_t fdow;

	headp = (freeblkinfo_t *)&sfs_vfsp->vfs_defer_list;
	pl = SFS_DEFLOCK(sfs_vfsp);
	if (sfs_vfsp->vfs_defer_blocks == 0) {
		SFS_DEFUNLOCK(sfs_vfsp, pl);
		return (B_FALSE);
	}
	fip = (freeblkinfo_t *)headp->fi_forw;
	while ((fip != headp) && (fip->fi_state == IMARKER))
		fip = (freeblkinfo_t *)fip->fi_forw;
	SFS_DEFUNLOCK(sfs_vfsp, pl);
	if (fip != headp) {
		fdow = dow_create_func(sfs_freeblocks_delayed, fip,
			DOW_NO_RESWAIT, B_TRUE);
		/*
		 * Using dow_handle_async followed by dow_iowait reduces
		 * the probability of stack overflow, because it forces
		 * sfs_freeblocks_delayed to be executed in another context.
		 */
		dow_handle_async(fdow);
		dow_iowait(fdow);
		dow_rele(fdow);
	}
	return (B_TRUE);
}

/*
 * void
 * sfs_freeblocks_sync(vfs_t *vfsp, uint_t flags)
 *	Sync blocks from the deferred free block list back to disk.
 *
 * Calling/Exit State:
 *	Holds no locks
 *
 * Remarks:
 *	Called from sfs_update and sfs_umount.
 */
void
sfs_freeblocks_sync(vfs_t *vfsp, uint_t flags)
{
	sfs_vfs_t *sfs_vfsp = SFS_VFS_PRIV(vfsp);
	freeblkinfo_t *headp, *fip, *mp;
	inode_t *ipx;
	pl_t pl;
	dowid_t fdow;

	SFS_CREATE_MARKER(mp);
	headp = (freeblkinfo_t *)&sfs_vfsp->vfs_defer_list;
	pl = SFS_DEFLOCK(sfs_vfsp);
	if (sfs_vfsp->vfs_defer_blocks != 0) {
		fip = (freeblkinfo_t *)headp->fi_forw;
		while (fip != headp) {
			if (fip->fi_state == IMARKER)
				fip = (freeblkinfo_t *)fip->fi_forw;
			else {
				ipx = fip->fi_forw;
				SFS_INSERT_MARKER(mp, ipx);
				SFS_DEFUNLOCK(sfs_vfsp, pl);
				fdow = dow_create_func(sfs_freeblocks_delayed,
					fip, DOW_NO_RESWAIT, B_TRUE);
				/*
				 * Using dow_handle_async here, even for the
				 * synchronous case, reduces the probability
				 * of stack overflow, by forcing
				 * sfs_freeblocks_delayed to be executed in
				 * another context.  Ensure synchrony by
				 * calling dow_iowait.
				 */
				dow_handle_async(fdow);
				if (!(flags & B_ASYNC))
					dow_iowait(fdow);
				dow_rele(fdow);
				pl = SFS_DEFLOCK(sfs_vfsp);
				fip = (freeblkinfo_t *)mp->fi_forw;
				SFS_REMOVE_MARKER(mp);
			}
		}
	}
	SFS_DEFUNLOCK(sfs_vfsp, pl);
	SFS_DESTROY_MARKER(mp);
}
