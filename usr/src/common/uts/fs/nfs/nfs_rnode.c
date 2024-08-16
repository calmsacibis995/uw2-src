/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_rnode.c	1.17"
#ident	"$Header: $"

/*
 *	nfs_rnode.c, routines which manipulate the rnode.
 *
 */

#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/sysmacros.h>
#include <fs/nfs/rnode.h>
#include <fs/nfs/nfslk.h>
#include <fs/buf.h>
#include <fs/fs_hier.h>

extern	lkinfo_t	rlock_rw_lkinfo;
extern	lkinfo_t	rlock_vm_lkinfo;
extern	lkinfo_t	r_statelock_lkinfo;
extern	lock_t		rpfreelist_lock;
extern	rwsleep_t	nfs_rtable_lock;
extern	struct vnodeops nfs_vnodeops;
extern	int		nrnode;

/*
 * rnode freelist and related numbers
 */
struct	rnode		*rpfreelist = NULL;
int			rreactive, rnfree, rnhash;
int			rnpages, rreuse, rnew, ractive;

/*
 * the rnode hash table
 */
struct	rnode		*rtable[RTABLESIZE];

/*
 * rp_addhash(rp)
 *	Put a rnode in the hash table
 *
 * Calling/Exit State:
 *	Assumes nfs_rtable_lock is write locked on entry.
 *	It is locked on exit too.
 *
 *	Returns a void.
 *
 * Description:
 *	Put a rnode in the hash table
 *
 * Parameters:
 *
 *	rp			# rnode to put
 *
 */
void
rp_addhash(struct rnode *rp)
{
	NFSLOG(0x2000, "rp_addhash: entered\n", 0, 0);

	rp->r_hash = rtable[rtablehash(rtofh(rp))];
	rtable[rtablehash(rtofh(rp))] = rp;
	rnhash++;
}

/*
 * rp_rmhash(rp)
 *	Remove a rnode from the hash table.
 *
 * Calling/Exit State:
 *	Assumes nfs_rtable_lock is write locked on entry.
 *	It is locked on exit too.
 *
 *	Returns a void.
 *
 * Description:
 *	Remove a rnode from the hash table.
 *
 * Parameters:
 *
 *	rp			# rnode to remove 
 *
 */
void
rp_rmhash(struct rnode *rp)
{
	struct	rnode	*rtprev = NULL;
	struct	rnode	*rt;

	NFSLOG(0x2000, "rp_rmhash: entered\n", 0, 0);

	rt = rtable[rtablehash(rtofh(rp))];
	while (rt != NULL) {
		if (rt == rp) {
			if (rtprev == NULL) {
				rtable[rtablehash(rtofh(rp))] = rt->r_hash;
			} else {
				rtprev->r_hash = rt->r_hash;
			}
			rnhash--;
			return;
		}
		rtprev = rt;
		rt = rt->r_hash;
	}
}

/*
 * rp_lastref(rp)
 *	Called when the last reference to the rnode
 *	is going away.
 *
 * Calling/Exit State:
 *	Returns a void.
 *
 *	Called with nfs_rtable_lock held in writer's mode.
 *	It is still locked on exit.
 *
 * Description:
 *	This is called only from nfs_inactive() when the last reference
 *	to the rnode is going away. It removes the rnode from the
 *	hash table if it has no pages, otherwise it simply leaves the
 *	pages (dirty or not) hanging from the rnode. The rnodes with
 *	no pages are put in the front of the freelist, while those with
 *	pages are put at the end.
 *
 *	Note that an rnode can be in one of the following states:
 *
 *	a) only on the hash table.
 *	b) only on the freelist with no pages.
 *	c) on the freelist as well as hash table with pages hanging
 *	   off of it.
 *
 * Parameters:
 *
 *	rp			# rnode to put on free list
 *
 */
void
rp_lastref(struct rnode *rp)
{
	struct	mntinfo	*minfo = (struct mntinfo *)(rtov(rp)->v_vfsp->vfs_data);
	pl_t		opl;


	NFSLOG(0x2000, "rp_lastref: entered\n", 0, 0);

	ASSERT(rtov(rp)->v_count == 1);

	/*
	 * release hold on mntinfo
	 */
	opl = LOCK(&minfo->mi_lock, PLMIN);
	minfo->mi_refct--;
	UNLOCK(&minfo->mi_lock, opl);

	if (rtov(rp)->v_pages == NULL) {
		/*
		 * no pages, remove from hash table and also
		 * free credentials.
		 */
		rp_rmhash(rp);
		opl = LOCK(&rp->r_statelock, PLMIN);
		if (rp->r_cred) {
			crfree(rp->r_cred);
			rp->r_cred = NULL;
		}
		UNLOCK(&rp->r_statelock, opl);

		/*
		 * add and rnode with no pages to the front
		 * of the freelist.
		 */
		rp_addfree(rp, FRONT);
	} else {
		/*
		 * we will not push out the dirty pages here as
		 * it is big overhead.
		 *
		 * add an rnode with pages to the back of
		 * the freelist.
		 */
		rp_addfree(rp, BACK);
	}
}

/*
 * rp_addfree(rp)
 *	Add an rnode to the front of the free list.
 *
 * Calling/Exit State:
 *	Assumes rpfreelist_lock unlocked on entry.
 *
 *	Returns a void.
 *
 * Description:
 *	Add an rnode to the front of the free list.
 *
 * Parameters:
 *
 *	rp			# rnode to add
 */
void
rp_addfree(struct rnode *rp, int front)
{
	pl_t	opl;

	NFSLOG(0x2000, "rp_addfree: entered\n", 0, 0);

	ASSERT((rtov(rp)->v_count == 0) || (rtov(rp)->v_count == 1));

	if (rp->r_freef != NULL) {
		return;
	}

	opl = LOCK(&rpfreelist_lock, PLMIN);
	if (rpfreelist == NULL) {
		rp->r_freef = rp;
		rp->r_freeb = rp;
		rpfreelist = rp;
	} else {
		rp->r_freef = rpfreelist;
		rp->r_freeb = rpfreelist->r_freeb;
		rpfreelist->r_freeb->r_freef = rp;
		rpfreelist->r_freeb = rp;
		if (front) {
			rpfreelist = rp;
		}
	}
	rnfree++;

	ASSERT(rpfreelist == NULL || rpfreelist->r_freeb != NULL);
	ASSERT(rpfreelist == NULL || rpfreelist->r_freef != NULL);

	UNLOCK(&rpfreelist_lock, opl);
}

/*
 * rp_rmfree(rp)
 *	Remove an rnode from the free list.
 *
 * Calling/Exit State:
 *	Assumes rpfreelist_lock locked on entry.
 *
 *	Returns a void.
 *
 * Description:
 *	Remove an rnode from the free list.
 *
 * Parameters:
 *
 *	rp			# rnode to remove
 */
void
rp_rmfree(struct rnode *rp)
{
	NFSLOG(0x2000, "rp_rmfree: entered\n", 0, 0);

	if (rp->r_freef == NULL) {
		return;
	}

	if (rp->r_freef == rp) {
		rpfreelist = NULL;
	} else {
		if (rp == rpfreelist) {
			rpfreelist = rp->r_freef;
		}
		rp->r_freeb->r_freef = rp->r_freef;
		rp->r_freef->r_freeb = rp->r_freeb;
	}
	rp->r_freef = rp->r_freeb = NULL;
	rnfree--;
}

/*
 * rp_free_resources(rp)
 *	Free pages, credentials and acls from rnode.
 *
 * Calling/Exit State:
 *	Both rpfreelist_lock and nfs_hashtable_lock should be
 *	held on entry.
 *
 *	Returns a void.
 *
 * Description:
 *	Free pages, credentials and acls from rnode. Rnode must
 *	have been removed from hash table before this is called.
 *	Called when rnode is going to be reused from the free list.
 *
 * Parameters:
 *
 *	rp			# rnode to free resources from
 *
 */
void
rp_free_resources(struct rnode *rp)
{
	struct	vnode	*vp = rtov(rp);
	pl_t		opl;

	NFSLOG(0x2000, "rp_free_resources: entered\n", 0, 0);

	if (vp->v_pages) {
		opl = LOCK(&rp->r_statelock, PLMIN);
		rp->r_flags &= ~RDIRTY;
		UNLOCK(&rp->r_statelock, opl);
		(void) VOP_PUTPAGE(vp, 0, 0, B_INVAL, rp->r_cred);
	}

	/*
	 * free the credentials, and acls if any
	 */
	opl = LOCK(&rp->r_statelock, PLMIN);
	if (rp->r_cred) {
		crfree(rp->r_cred);
		rp->r_cred = NULL;
	}

#ifdef NFSESV
	if (rp->r_acl) {
		u_int	acl_size;

		acl_size = acl_getmax() * sizeof(struct acl);
		kmem_free((caddr_t)rp->r_acl, acl_size);
	}
#endif

	UNLOCK(&rp->r_statelock, opl);
}


/*
 * rfind(rp)
 *	Lookup a rnode by fhandle.
 *
 * Calling/Exit State:
 *	Returns a pointer to rnode if found, else NULL.
 *
 *	The nfs_rtable_lock must be held in at least reader's mode
 *	on entry.
 *
 * Description:
 *	This routine looks up the rnode in the hash table.
 *
 * Parameters:
 *
 *	fh			# file handle of the rnode
 *	vfsp			# pointer to vfs to which rnode belongs
 *
 */
struct rnode *
rfind(fhandle_t *fh, struct vfs *vfsp)
{
	struct	mntinfo	*minfo;
	struct	rnode	*rt;
	pl_t		opl;

	NFSLOG(0x2000, "rfind: entered\n", 0, 0);

	rt = rtable[rtablehash(fh)];
	while (rt != NULL) {
		if (bcmp((caddr_t)rtofh(rt), (caddr_t)fh, sizeof (*fh)) == 0 &&
			vfsp == rtov(rt)->v_vfsp) {
			/*
			 * found the rnode in hash table, reactivate it.
			 */
			VN_LOCK(rtov(rt));
			if (++rtov(rt)->v_count == 1) {
				VN_UNLOCK(rtov(rt));

				/*
				 * reactivating a free rnode, up vfs ref count
				 * and remove rnode from free list.
				 */
				opl = LOCK(&rpfreelist_lock, PLMIN);
				rp_rmfree(rt);
				UNLOCK(&rpfreelist_lock, opl);

				minfo = (struct mntinfo *)
					(rtov(rt)->v_vfsp->vfs_data);
				opl = LOCK(&minfo->mi_lock, PLMIN);
				minfo->mi_refct++;
				UNLOCK(&minfo->mi_lock, opl);

				rreactive++;
				if (rtov(rt)->v_pages) {
					rnpages++;
				}

				NFSLOG(0x2000, "rfind: reactivating\n", 0, 0);

			} else {
				/*
				 * someone else got to it first,
				 * so assume it is activated
				 */
				VN_UNLOCK(rtov(rt));
				ractive++;

				NFSLOG(0x2000, "rfind: already done\n", 0, 0);
			}

			/*
			 * the rnode shouldn't be on free list now.
			 * Note that we don't need to explicitly remove
			 * the rnode from the free list here (rp_rmfree),
			 * since it was only on the free list if v_count
			 * was 0 when we grabbed this rnode at the top
			 * of this while loop. In that case, we would
			 * have removed the rnode from the free list
			 * above, just after incrementing v_count.
			 */
			ASSERT((rt->r_freef == NULL) && (rt->r_freeb == NULL));
			return (rt);
		}
		rt = rt->r_hash;
	}

	NFSLOG(0x2000, "rfind: returning NULL\n", 0, 0);

	return (NULL);
}

/*
 * nfs_inval_vfs(vfsp)
 *	Invalidate all vnodes for this vfs.
 *
 * Calling/Exit State:
 *	Assumes nfs_rtable_lock is not locked on entry.
 *
 *	Returns a void.
 *
 * Description:
 *	This routine invalidates all the vnodes (rnodes)
 *	for a file system.
 *
 *	NOTE: assumes vnodes have been written already via nfs_flush_vfs().
 *	Called by nfs_unmount in an attempt to get the "mount info count"
 *	back to zero. This routine will be filled in if we ever have
 *	an LRU rnode cache.
 *
 * Parameters:
 *
 *	vfsp			# vfs to invalidate rnodes of
 *
 */
void
nfs_inval_vfs(struct vfs *vfsp)
{
	struct	rnode	*rp;
	pl_t		opl1;

	NFSLOG(0x2000, "nfs_inval_vfs: entered\n", 0, 0);

	if (rpfreelist == NULL) {
		return;
	}

	/*
	 * must write lock the hash table
	 */
	RWSLEEP_WRLOCK(&nfs_rtable_lock, PRINOD);

repeat:

	/*
	 * start from the top of the free list
	 */
	opl1 = LOCK(&rpfreelist_lock, PLMIN);
	rp = rpfreelist;

	do {
		/*
		 * lock the vnode to check count
		 */
		VN_LOCK(rtov(rp));
		if (rtov(rp)->v_vfsp == vfsp && rtov(rp)->v_count == 0) {
			VN_UNLOCK(rtov(rp));

			/*
			 * no references, remove from hash list
			 */
			rp_rmhash(rp);

			if (rtov(rp)->v_pages) {
				/*
				 * there are pages hanging on this rnode,
				 * invalidate them. this must mean that
				 * the rnode credentials are intact.
				 *
				 * free up rnode resources. must drop the
				 * freelist lock first.
				 */
				UNLOCK(&rpfreelist_lock, opl1);

				ASSERT(rp->r_cred != NULL);

				rp_free_resources(rp);

				/*
				 * must repeat as we dropped the freelist
				 * lock before VOP_PUTPAGE
				 */
				goto repeat;
			}
		} else {
			VN_UNLOCK(rtov(rp));
		}
		rp = rp->r_freef;
	} while (rp != rpfreelist);

	UNLOCK(&rpfreelist_lock, opl1);
	RWSLEEP_UNLOCK(&nfs_rtable_lock);
}

/*
 * nfs_flush_vfs(vfsp)
 *	Flush all vnodes for this or every vfs.
 *
 * Calling/Exit State:
 *	Assumes nfs_rtable_lock is not locked, and sync_busy_lock
 *	is locked on entry.
 *
 *	Returns a void.
 *
 * Description:
 *	This routine flushes all the vnodes (rnodes)
 *	for one or all nfs file systems. Used by
 *	nfs_sync and by nfs_unmount.
 *
 * Parameters:
 *
 *	vfsp			# vfs to flush, is NULL to flush all
 *
 */
void
nfs_flush_vfs(struct vfs *vfsp)
{
	struct	rnode	**rpp, *rp;
	struct	vnode	*vp;

	NFSLOG(0x2000, "nfs_flush_vfs: entered\n", 0, 0);

	/*
	 * only need to read lock the hash table
	 * as we will not modify it
	 */
	RWSLEEP_RDLOCK(&nfs_rtable_lock, PRINOD);
	for (rpp = rtable; rpp < &rtable[RTABLESIZE]; rpp++) {
		for (rp = *rpp; rp != (struct rnode *)NULL; rp = rp->r_hash) {
			vp = rtov(rp);
			/*
			 * don't bother sync'ing a vp if it is part of
			 * virtual swap device or if vfs is read-only
			 */
			if ((rp->r_swapcnt > 0) ||
			   ((vp->v_vfsp->vfs_flag & VFS_RDONLY) != 0))
				continue;
			if (vfsp == (struct vfs *)NULL || vp->v_vfsp == vfsp) {
				(void) VOP_PUTPAGE(vp, 0, 0, B_ASYNC,
					rp->r_cred);
			}
		}
	}
	RWSLEEP_UNLOCK(&nfs_rtable_lock);
}

/*
 * nfs_mmap_sync()
 *	Validate caches of all mmap'ed nfs files.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns a void.
 *
 * Description:
 *	Validate caches of all mmap'ed nfs files.
 *
 * Parameters:
 *
 *	None.
 */
void
nfs_mmap_sync()
{
	struct	rnode	**rpp, *rp;
	struct	vnode	*vp;
	struct	vattr	va;
	pl_t		opl;

	NFSLOG(0x2000, "nfs_mmap_sync: entered\n", 0, 0);

	/*
	 * only need to read lock the hash table
	 * as we will not modify it
	 */
	RWSLEEP_RDLOCK(&nfs_rtable_lock, PRINOD);

	for (rpp = rtable; rpp < &rtable[RTABLESIZE]; rpp++) {
		for (rp = *rpp; rp != (struct rnode *)NULL; rp = rp->r_hash) {
			vp = rtov(rp);
			/*
			 * TODO: remember when a file is being used for swap
			 * 	at VOP_STABLESTORE time
			 */

			/*
			 * check rnode mapcnt. do not need a lock here
			 * as we are just interested in a snapshot
			 */
			if (rp->r_mapcnt) {
				/*
				 * get the statelock to check if 
				 * cached attibutes are valid
				 */
				opl = LOCK(&rp->r_statelock, PLMIN);
				if (hrestime.tv_sec < rp->r_attrtime.tv_sec) {
					/*
					 * cache attrbutes are valid
					 */
					UNLOCK(&rp->r_statelock, opl);
				} else {
					/*
					 * cached attributes are invalid.
					 * we will need to go to the server,
					 * so drop the nfs_rtable_lock.
					 */
					UNLOCK(&rp->r_statelock, opl);
					RWSLEEP_UNLOCK(&nfs_rtable_lock);
					(void) nfs_getattr_otw(vp, &va,
							rp->r_cred);

					/*
					 * re-acquire nfs_rtable_lock again.
					 * the rtable may have changed by now
					 * but we go ahead and complete this
					 * run
					 */
					RWSLEEP_RDLOCK(&nfs_rtable_lock,
							PRINOD);
				}
			}
		}
	}

	RWSLEEP_UNLOCK(&nfs_rtable_lock);
}

/*
 * makenfsnode(fh, attr, vfsp)
 *	Return a vnode for the given fhandle.
 *
 * Calling/Exit State:
 *
 * Description:
 *	This routine returns an rnode for the given fhandle.
 *	If no rnode exists for this fhandle, it creates one and
 *	puts it	in a table hashed by fh_fsid and fs_fid. If the
 *	rnode for this fhandle is already in the table it is
 *	returned. Its ref count is incremented by rfind. The
 *	rnode will be flushed from the table when nfs_inactive
 *	is called.
 *
 * Parameters:
 *
 *	fh			# file handle to get rnode for
 *	attr			# attr to use in rnode creation
 *	vfsp			# vfs to which file belongs
 *
 */
struct vnode *
makenfsnode(fhandle_t *fh, struct nfsfattr *attr, struct vfs *vfsp)
{
	struct	rnode	*rp;
	struct	vnode	*vp;
	struct	mntinfo	*minfo;
	int		newnode = 0;
#ifdef NFSESV
	int		acl_size;
#endif
	pl_t		opl;

	NFSLOG(0x2000, "makenfsnode: entered\n", 0, 0);

	/*
	 * look for the rnode in the rnode table.
	 *
	 * note that we only need the nfs_rtable_lock in reader mode
	 * to do this. however, we will modify the hash table if we
	 * do not find the rnode in the hash table and re-use from
	 * freelist or allocate a new one. so to avoid having to
	 * upgrade to writer mode, we get it in writer mode in the
	 * first place.
	 */
	RWSLEEP_WRLOCK(&nfs_rtable_lock, PRINOD);

	if ((rp = rfind(fh, vfsp)) == NULL) {
redo:
		/*
		 * rnode not found, get one from the freelist
		 */
		opl = LOCK(&rpfreelist_lock, PLMIN);
		if (rpfreelist &&
		   (rtov(rpfreelist)->v_pages == NULL || rnew >= nrnode)) {
			/*
			 * already over max, reuse from top of list
			 */
			rp = rpfreelist;
			rpfreelist = rpfreelist->r_freef;

			ASSERT(rtov(rp)->v_count == 0);

			rp_rmfree(rp);
			rreuse++;
			UNLOCK(&rpfreelist_lock, opl);

			/*
			 * remove from hash table, in case it is one of
			 * those which is both on the free list as well
			 * as on the hash list.
			 */
			rp_rmhash(rp);

			/*
			 * now flush and free up the pages, credentials,
			 * acls etc.
			 */
			rp_free_resources(rp);

			vp = rtov(rp);

			ASSERT(vp->v_softcnt >= 0);

			VN_LOCK(vp);
			if (vp->v_softcnt > 0) {
				/*
				 * this is the unlikely case where
				 * we are racing with pageout/fsflush.
				 * put the rnode back on the freelist
				 * (at the back), and go for another.
				 */
				VN_UNLOCK(vp);
				rp_addfree(rp, BACK);
				goto redo;
			}
			VN_UNLOCK(vp);

			/*
			 * at this point, pageout/fsflush cannot get
			 * a softhold on the vnode as there are no
			 * pages hanging off the rnode.
			 *
			 * deinitialize all locks.
			 */
			LOCK_DEINIT(&rp->r_statelock);
			RWSLEEP_DEINIT(&rp->r_rwlock);
			VN_DEINIT(vp);

			/*
			 * now zero out the rnode
			 */
			bzero((caddr_t)rp, sizeof (*rp));
		} else {
			struct rnode *trp;

			UNLOCK(&rpfreelist_lock, opl);

			/*
			 * can create a new rnode
			 */
			rp = (struct rnode *)kmem_zalloc(sizeof(*rp),KM_SLEEP);

			/*
			 * there may be a race condition here if someone else
			 * alloc's the same rnode while we're asleep, so we
			 * check again and recover if found
			 */
			if ((trp = rfind(fh, vfsp)) != NULL) {
				kmem_free((caddr_t)rp, sizeof (*rp));
				rp = trp;
				goto rnode_found;
			} else {
				opl = LOCK(&rpfreelist_lock, PLMIN);
				rnew++;
				UNLOCK(&rpfreelist_lock, opl);
			}

			vp = rtov(rp);
		}

#ifdef NFSESV
		acl_size = acl_getmax() * sizeof(struct acl);
		rp->r_acl = (struct acl *) kmem_alloc(acl_size, KM_SLEEP);
#endif

		/*
		 * initialize rnode locks
		 */
		LOCK_INIT(&rp->r_statelock, NFS_HIERSTATE, PLMIN,
					&r_statelock_lkinfo, KM_SLEEP);
		RWSLEEP_INIT(&rp->r_rwlock, NFS_HIERRW,
					&rlock_rw_lkinfo, KM_SLEEP);

		/*
		 * VN_INIT() will leave a hold on the vnode
		 */
		VN_INIT(vp, vfsp, 0, 0, 0, KM_SLEEP);

		rp->r_fh = *fh;
		vp->v_lid = (lid_t)0;
		vp->v_op = &nfs_vnodeops;
		if (attr) {
			/*
			 * set the type here
			 */
			vp->v_type = n2v_type(attr);

			/*
			 * also set the file size here for new
			 * rnodes. this is needed as we call
			 * nfs_cache_attr below, which does not
			 * set file size.
			 */
			rp->r_size = attr->na_size;

			/*
			 * a translation here seems to be necessary
			 * because this function can be called
			 * with `attr' that has come from the wire,
			 * and been operated on by vattr_to_nattr().
			 */
			if ((attr->na_rdev & 0xffff0000) == 0)
				vp->v_rdev = expdev(attr->na_rdev);
			else
				vp->v_rdev = n2v_rdev(attr);
		}

		vp->v_data = (caddr_t)rp;
		vp->v_vfsp = vfsp;

		/*
		 * now add to the hash table
		 */
		rp_addhash(rp);
		minfo = (struct mntinfo *)vfsp->vfs_data;

		/*
		 * hold on to the mntinfo
		 */
		opl = LOCK(&minfo->mi_lock, PLMIN);
		minfo->mi_refct++;
		UNLOCK(&minfo->mi_lock, opl);
		newnode++;
	}

rnode_found:

	RWSLEEP_UNLOCK(&nfs_rtable_lock);
	if (attr) {
		struct	vattr	va;
		struct	vnode	*vp = rtov(rp);

		if (!newnode) {
			timestruc_t	mtime;

			/*
			 * for rnodes grabbed from the hash table
			 * flush the caches if they are not valid
			 */
			mtime.tv_sec = attr->na_mtime.tv_sec;
			mtime.tv_nsec = attr->na_mtime.tv_usec*1000;
			nfs_cache_check(vp, mtime, attr->na_size, rp->r_size);
		}

		/*
		 * set the attr cache
		 */
		nattr_to_vattr(vp, attr, &va);
		nfs_cache_attr(vp, &va);
	}

	return (rtov(rp));
}

#ifdef NFSESV

/*
 * makeesvnfsnode(fh, attr, vfsp)
 *	Return a vnode for the given fhandle.
 *
 * Calling/Exit State:
 *
 * Description:
 *	This routine returns a vnode (rnode) for the given fhandle.
 *	If no rnode exists for this fhandle, it creates one and
 *	puts it	in a table hashed by fh_fsid and fs_fid. If the
 *	rnode for this fhandle is already in the table it is
 *	returned. Its ref count is incremented by rfind. The
 *	rnode will be flushed from the table when nfs_inactive
 *	is called.
 *
 *	This is used by the extended (esv) protocol.
 *
 * Parameters:
 *
 *	fh			# file handle to get rnode for
 *	attr			# attr to use in rnode creation
 *	vfsp			# vfs to which file belongs
 *
 */
struct vnode *
makeesvnfsnode(fhandle_t *fh, struct nfsesvfattr *attr, struct vfs *vfsp)
{
	struct	rnode	*rp;
	struct	vnode	*vp;
	struct	mntinfo	*minfo;
	int		newnode = 0;
	pl_t		opl;

	NFSLOG(0x2000, "makeesvnfsnode: entered", 0, 0);

	/*
	 * look for the rnode in the rnode table
	 */
	if ((rp = rfind(fh, vfsp)) == NULL) {
		/*
		 * rnode not found, get one from the freelist
		 */
		if (rpfreelist &&
			(rtov(rpfreelist)->v_pages == NULL || rnew >= nrnode)) {
			/*
			 * already over max, reuse from top of list
			 */
			rp = rpfreelist;
			rpfreelist = rpfreelist->r_freef;
			rp_rmfree(rp);

			rp_rmhash(rp);
			rp_free_resources(rp);
			rreuse++;
			vp = rtov(rp);
		} else {
			struct rnode *trp;
			int acl_size;

			/*
			 * can create a new rnode
			 */
			rp = (struct rnode *)kmem_alloc(sizeof (*rp), KM_SLEEP);
			acl_size = acl_getmax() * sizeof(struct acl);
			rp->r_acl = (struct acl *)
					kmem_alloc(acl_size, KM_SLEEP);

			/*
			 * there may be a race condition here if someone else
			 * alloc's the same rnode while we're asleep, so we
			 * check again and recover if found
			 */
			if ((trp = rfind(fh, vfsp)) != NULL) {
				kmem_free((caddr_t)rp->r_acl, acl_size);
				kmem_free((caddr_t)rp, sizeof (*rp));
				rp = trp;
				goto esvrnode_found;
			} else {
				rp->r_aclcnt = 0;
				rnew++;
			}

			/*
			 * init the rnode locks
			 */
			LOCK_INIT(&rp->r_statelock, NFS_HIERSTATE, PLMIN,
					&r_statelock_lkinfo, KM_SLEEP);
			RWSLEEP_INIT(&rp->r_rwlock, NFS_HIERRW,
					&rlock_rw_lkinfo, KM_SLEEP);

			vp = rtov(rp);

			RWSLEEP_INIT(&(vp)->v_lock, (uchar_t) 0,
					&vlock_lkinfo, KM_SLEEP);
			LOCK_INIT(&(vp)->v_filocks_mutex, FS_VPFRLOCKHIER,
					PLBASE, &vmutex_lkinfo, KM_SLEEP);
			FSPIN_INIT(&(vp)->v_mutex);
		}

		/*
		 * init the rnode
		 */
		bzero((caddr_t)rp, sizeof (*rp));
		rp->r_fh = *fh;
		vp = rtov(rp);

		/*
		 * need to protect to mod v_lid etc
		 */
		VN_HOLD(vp);
		vp->v_lid = (lid_t)0;
		vp->v_macflag |= VMAC_SUPPORT;
		vp->v_op = &nfs_vnodeops;
		if (attr) {
			/*
			 * a translation here seems to be necessary
			 * because this function can be called
			 * with `attr' that has come from the wire,
			 * and been operated on by vattr_to_nattr().
			 * See nfsrootvp()->VOP_GETTATTR()->nfsgetattr()
			 * ->nfs_getattr_otw()->rfscall()->vattr_to_nattr()
			 * ->makesvenfsnode().
			 */
			vp->v_type = n2v_type(attr);
			if ((attr->na_rdev & 0xffff0000) == 0)
				vp->v_rdev = expdev(attr->na_rdev);
			else
				vp->v_rdev = n2v_rdev(attr);
		}

		/*
		 * now add it to the hash table
		 */
		vp->v_data = (caddr_t)rp;
		vp->v_vfsp = vfsp;
		rp_addhash(rp);
		minfo = (struct mntinfo *)vfsp->vfs_data;

		/*
		 * hold on to the mountinfo
		 */
		opl = LOCK(&minfo->mi_lock, PLMIN);
		minfo->mi_refct++;
		UNLOCK(&minfo->mi_lock, opl);
		newnode++;
	}

esvrnode_found:

	if (attr) {
		if (!newnode) {
			timestruc_t	mtime;

			/*
			 * flush the cahes if they are
			 * not valid for existing rnodes
			 */
			mtime.tv_sec = attr->na_mtime.tv_sec;
			mtime.tv_nsec = attr->na_mtime.tv_usec*1000;
			nfs_esvcache_check(rtov(rp), mtime, attr->na_size);
		}

		/*
		 * set the cahes
		 */
		nfs_esvattrcache(rtov(rp), attr);
		if (NA_TSTMLD(attr) && rtov(rp)->v_type == VDIR)
			rp->r_flags |= RMLD;
	}

	return (rtov(rp));
}

#endif
