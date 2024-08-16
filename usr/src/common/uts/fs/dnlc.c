/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/dnlc.c	1.22"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/param.h>
#include <util/ksynch.h>
#include <util/plocal.h>
#include <util/metrics.h>
#include <proc/cred.h>
#include <svc/autotune.h>
#include <svc/systm.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/dnlc.h>
#include <fs/fs_hier.h>
#include <fs/fs_subr.h>		/* temporarily for fs_nosys */
#include <mem/kmem.h>
#include <svc/errno.h>
#include <acc/mac/mac.h>
#ifdef CC_PARTIAL
#include <acc/mac/covert.h>
#include <acc/mac/cc_count.h>
#endif

/*
 * Directory name lookup cache.
 * Based on code originally done by Robert Elz at Melbourne.
 *
 * Names found by directory scans are retained in a cache
 * for future reference.  It is managed LRU, so frequently
 * used names will hang around.  Cache is indexed by hash value
 * obtained from (vp, name) where the vp refers to the
 * directory containing the name.
 *
 * For simplicity (and economy of storage), names longer than
 * some (small) maximum length are not cached; they occur
 * infrequently in any case, and are almost never of interest.
 */

#define NC_REL_SIZE		10	/* chunk removal size */

#define	NC_HASH_VPSHIFT		8
#define	NC_HASH_CALC		4

#define	NC_HASH(namep, namelen, vp)	\
	((namep[0] + namep[namelen-1] + namelen + \
		(((int) vp) >> NC_HASH_VPSHIFT)) & (nchash_size - 1))

/*
 * Macros to insert, remove cache entries from hash, LRU lists.
 */
#define	INS_HASH(ncp, nch)	nc_inshash(ncp, nch)
#define	RM_HASH(ncp)		nc_rmhash(ncp)

#define	INS_LRU(ncp1, ncp2)	nc_inslru((ncache_t *) ncp1, (ncache_t *) ncp2)
#define	RM_LRU(ncp)		nc_rmlru((ncache_t *) ncp)

#define	NULL_HASH(ncp)		(ncp)->hash_next = (ncp)->hash_prev = (ncp)

/*
 * The name cache itself, dynamically allocated at startup.
 */
STATIC ncache_t *ncache;
STATIC int	 nchsize;
extern int ncsize;			/* defined in kernel.cf */
extern int nchash_size;			/* defined in kernel.cf */

/*
 * Hash list of name cache entries for fast lookup.
 */
STATIC struct nc_hash {
	ncache_t *hash_next;
	ncache_t *hash_prev;
};

typedef struct nc_hash nc_hash_t;

/*
 * LRU list of cache entries for aging.
 */
STATIC struct nc_lru {
	ncache_t *hash_next;	/* hash chain, unused */
	ncache_t *hash_prev;
	ncache_t *lru_next;	/* LRU chain */
	ncache_t *lru_prev;
} nc_lru;

typedef struct nc_lru nc_lru_t;

STATIC nc_hash_t *nc_hash;		/* hash table */

STATIC int doingcache = 1;

/*
 * The dnlc_mutex protects the global state of the dnlc. It is
 * implemented as a spin lock.
 */
STATIC lock_t	dnlc_mutex;

/*
 *+
 *+ This lock protects the contents of the DNLC hash list, and
 *+ the DNLC LRU chain.
 */
LKINFO_DECL(dnlc_lkinfo, "FS::dnlc_mutex", 0);

STATIC void	dnlc_rm(ncache_t *);
STATIC ncache_t	*dnlc_search(vnode_t *, char *, int, int, cred_t *);

STATIC void	nc_inshash(ncache_t *, ncache_t *);
STATIC void	nc_rmhash(ncache_t *);
STATIC void	nc_inslru(ncache_t *, ncache_t *);
STATIC void	nc_rmlru(ncache_t *);


extern struct tune_point DNLCSIZEcurve[];
extern int DNLCSIZEcurvesz;
/*
 * void
 * dnlc_init(void)
 *	Initialize the directory cache.
 *
 * Calling/Exit State:
 *	Called from initialization sequent. Thus, there's no
 *	interlock necessary to prevent concurrent dnlc_init()'s.
 *
 * Description:
 *	Initialize the hash links, add each dnlc entry to the LRU list, and
 *	then initialize the lock.
 */
void
dnlc_init(void)
{
	ncache_t	*ncp;
	int	i, t_nchsize;

	ncsize = tune_calc(DNLCSIZEcurve, DNLCSIZEcurvesz);
	if (ncsize > 0) {
		ncache = (ncache_t *)kmem_zalloc(ncsize * sizeof(*ncache),
						 KM_SLEEP);

		/*
		 * Calculate the size of the hash table to be approx. 1/4
		 * of ncsize, rounded up to the nearest power of 2.
		 */
		t_nchsize = ncsize / NC_HASH_CALC;
		while (t_nchsize & (t_nchsize - 1))
			t_nchsize = (t_nchsize | (t_nchsize - 1)) + 1;
		nchsize = t_nchsize;
		nc_hash = (nc_hash_t *)kmem_zalloc(nchsize * sizeof(nc_hash_t),
						 KM_SLEEP);
	}

	if (ncsize <= 0 || ncache == (ncache_t *)NULL) {
		doingcache = 0;
		/*
		 *+ The kernel could not allocate the configured
		 *+ size of dnlc. Reconfigure the kernel and/or
		 *+ dnlc to consume less memory. The system will
		 *+ not utilize the name cache until the source
		 *+ of the problem is fixed.
		 */
		cmn_err(CE_NOTE, "No memory for name cache\n");
		return;
	}

	nc_lru.lru_next = (ncache_t *) &nc_lru;
	nc_lru.lru_prev = (ncache_t *) &nc_lru;
	for (i = 0; i < ncsize; i++) {
		ncp = &ncache[i];
		INS_LRU(ncp, &nc_lru);
		NULL_HASH(ncp);
		ncp->dp = ncp->vp = NULL;
	}
	for (i = 0; i < nchash_size; i++) {
		ncp = (ncache_t *) &nc_hash[i];
		NULL_HASH(ncp);
	}
	LOCK_INIT(&dnlc_mutex, FS_DNLCHIER,
		FS_DNLCPL, &dnlc_lkinfo, KM_SLEEP);
	
}

/*
 * dnlc_enter(vnode_t *dp, char *name, vnode_t *vp, void *cookie, cred_t *cred)
 *	Conditionally enter an entry into the dnlc. 
 *
 * Calling/Exit State:
 *	The caller must hold a reference to both the vnode of
 *	the directory (dp) and the vnode (vp). 
 *	
 *	It's not necessary to hold the containing directory locked;
 *	however, entries are usually made into the dnlc after
 *	a directory search operation has been performed and thus,
 *	the directory is usually locked at least *shared*.
 *
 *	cookie contains some data that dnlc will cache on behalf of the
 *	client. dnlc will not interpret this data.
 *
 *	This routine may block; the caller must be prepared for
 *	this.
 *
 * Remarks:
 *	The dnlc_mutex is locked by this routine. The lock
 *	is released before returning to caller. 
 */
void
dnlc_enter(vnode_t *dp, char *name, vnode_t *vp, void *cookie, cred_t *cred)
{
	int	namlen;
	ncache_t *ncp;
	int	hash;
	pl_t	pl;
	vnode_t *olddp;
	vnode_t	*oldvp;
	cred_t	*oldcred;

	if (!doingcache) {
		return;
	}

	/*
	 * Long names are not stored in the cache
	 */
	namlen = strlen(name);
	if (namlen > NC_NAMLEN) {
		MET_DNLC_MISSES();
		return;
	}

	hash = NC_HASH(name, namlen, dp);

	/*
	 * Lock the dnlc to prevent other LWPs from accessing
	 * or modifying the dnlc while we're making an entry.
	 */
	pl = LOCK(&dnlc_mutex, FS_DNLCPL);

	/*
	 * Is the entry already in the dnlc? If so, just update the cookie.
	 */
	if ((ncp = dnlc_search(dp, name, namlen, hash, cred)) != NULL) {
		ncp->cookie = cookie;
		UNLOCK(&dnlc_mutex, pl);
		return;
	}

	/*
	 * At this point it's been determined that the entry
	 * would be unique in the dnlc. The head of the LRU list
	 * is inspected for a free entry. If one exists, it's
	 * re-used for this entry. If we're out of entries, then
	 * no entry is made to the dnlc.
	 */
	ncp = nc_lru.lru_next;
	if (ncp == (ncache_t *) &nc_lru) {	/* LRU queue empty */
		UNLOCK(&dnlc_mutex, pl);
		return;
	}
	/*
	 * Remove from LRU, hash chains.
	 */
	RM_LRU(ncp);
	RM_HASH(ncp);

	/*
	 * Save the values of <dp>, <vp> and <cred>. References to
	 * these are dropped after releasing dnlc_mutex in order to
	 * minimize its lock hold time.
	 */
	
	olddp = ncp->dp;
	oldvp = ncp->vp;
	oldcred = ncp->cred;

	/*
	 * Establish references to the vnodes and if specified,
	 * credentials structure here.
	 */

	ncp->dp = dp; VN_DNLC_HOLD(dp);
	ncp->vp = vp; VN_DNLC_HOLD(vp);
	ncp->cookie = cookie;

	ncp->namlen = (char) namlen;
	bcopy(name, ncp->name, (unsigned)namlen);

#ifdef CC_PARTIAL
	ncp->lid = u.u_lwpp->l_cred->cr_lid;
#endif
	ncp->cred = cred;
	if (cred != NOCRED) {
		crhold(cred);
	}

	/*
	 * Insert in LRU, hash chains.
	 */
	INS_LRU(ncp, nc_lru.lru_prev);
	INS_HASH(ncp, (ncache_t *)&nc_hash[hash]);

	/*
	 * The entry has been made into the cache so dnlc_mutex
	 * can be released.
	 */
	UNLOCK(&dnlc_mutex, pl);

	/*
	 * Remove references to resources held by the
	 * previous contents of the dnlc slot.
	 *
	 * Note: olddp != NULL implies oldvp != NULL
	 */

	if (olddp != NULLVP) {
		VN_DNLC_RELE(olddp);
		VN_DNLC_RELE(oldvp);
	}
	if (oldcred != NOCRED) {
		crfree(oldcred);
	}

	return;
}

/*
 * vnode_t *
 * dnlc_lookup(vnode_t *dp, char *name, void **cp, boolean_t *softhold,
 *	       cred_t *cred)
 *	Determine whether the dnlc contains an entry
 *	for the given directory/filename/cred.
 *
 * Calling/Exit State:
 *	If the dnlc contains the requested entry and if VN_TRYHOLD
 *	succeeds against its vnode, then the vnode is returned.
 *
 *	cp is an outarg which receives the cookie value from the dnlc
 *	entry. dnlc caches the cookie on behalf of the client, but does not
 *	interpret it.
 *
 *	softhold is an outarg. If the return value is non-NULL, then
 *	*softhold will be set to true if the returned vnode is soft
 *	held.  Otherwise, the returned vnode is hard held.
 *
 *	The caller is not required to hold the containing directory
 *	locked. However, this routine is most often called while
 *	the containing directory is locked since the caller must
 *	be prepared to perform a directory search if the dnlc does
 *	not contain the requested entry. Also, if the caller does
 *	not interlock dnlc search with dnlc modification, this routine
 *	will return a reference to a vnode being removed. This is probably
 *	not the desired effect the caller wishes, i.e., if a concurrent
 *	directory removal of the name occurs, it's probably also desired
 *	that no other access to the directory 'finds' the file.
 */
vnode_t *
dnlc_lookup(vnode_t *dp, char *name, void **cp, boolean_t *softhold,
	    cred_t *cred)
{
	vnode_t	*vp;
	int	namlen;
	int	hash;
	pl_t	pl;
	ncache_t *ncp;

	if (!doingcache) {
		/*
		 * While technically this is a cache miss, there is
		 * no point to calling MET_DNLC_MISSES() here, since
		 * the only way to get here is when the system was
		 * tuned such that the dnlc size is 0.  In that case,
		 * the counters will indicate no dnlc activity.
		 */ 
		return (NULL);
	}

	/*
	 * Long names are not stored ...
	 */
	namlen = strlen(name);
	if (namlen > NC_NAMLEN) {
		MET_DNLC_MISSES();
		return (NULLVP);
	}

	hash = NC_HASH(name, namlen, dp);
	ASSERT(hash <= nchsize);

	/*
	 * Prevent the dnlc from modification during our search.
	 */
	pl = LOCK(&dnlc_mutex, FS_DNLCPL);

	ncp = dnlc_search(dp, name, namlen, hash, cred);
	if (ncp == (ncache_t *)NULL) {
		UNLOCK(&dnlc_mutex, pl);
		MET_DNLC_MISSES();
		return (NULLVP);
	}

	MET_DNLC_HITS();

	/*
	 * Move this slot to the end of LRU
	 * chain.
	 *
	 */
	RM_LRU(ncp);
	INS_LRU(ncp, nc_lru.lru_prev);

	/*
         * If not at the head of the hash chain,
         * move forward so will be found
         * earlier if looked up again.
         */
        if (ncp->hash_prev != (struct ncache *) &nc_hash[hash]) {
                RM_HASH(ncp);
		INS_HASH(ncp, (ncache_t *)&nc_hash[hash]);
        }

	/*
	 * Return the cookie to the caller.
	 */
	*cp = ncp->cookie;

	/*
	 * Try to make a reference to the vnode for the caller.
	 * This must be done while holding dnlc_mutex
	 * since the dnlc entry can become invalid after
	 * releasing the dnlc_mutex.
	 */
	vp = ncp->vp;
	VN_LOCK(vp);
	if (vp->v_count != 0) {
		++vp->v_count;
		*softhold = B_FALSE;
	} else {
		
		++vp->v_softcnt;
		*softhold = B_TRUE;
	}
	VN_UNLOCK(vp);
	UNLOCK(&dnlc_mutex, pl);

	return (vp);

}

/*
 * void
 * dnlc_remove(vnode_t *dp, char *name)
 *	Remove all entries for a given directory/name pair.
 *	
 * Calling/Exit State:
 *	The caller is not required to hold the containing directory
 *	locked although this is most often the case to make the directory
 *	removal interlock with the directory search operations. If the
 *	caller doesn't lock the directory, it must be prepared for
 *	another LWP to 'find' the entry in the directory while it's being
 *	removed.
 *
 *	Also note that unless the caller has interlocked calls to
 *	dnlc_remove with calls to dnlc_enter, this routine is heuristic.
 *	This is because there are windows when dnlc_mutex is not
 *	held and other LWPs could be making dnlc entries. This could
 *	also allow a directory lookup operation to find an entry
 *	that has been removed since the 'directory search' would
 *	be satisfied from the dnlc (instead of searching the actual
 *	directory).
 */
void
dnlc_remove(vnode_t *dp, char *name)
{
	int	namlen;
	int	hash;
	int	relcnt;
	int	i;
	pl_t	pl;
	ncache_t *ncp;
	vnode_t	*relvp[NC_REL_SIZE];

	if (!doingcache) {
		return;
	}

	/*
	 * Long names aren't stored
	 */
	namlen = strlen(name);
	if (namlen > NC_NAMLEN) {
		return;
	}

	hash = NC_HASH(name, namlen, dp);

	/*
	 * The basic operation is to collect some entries, release lock and
	 * then resources, and iterate. A small array is used on the stack
	 * to collect vnodes to release to reduce the number of lock round
	 * trips on dnlc_mutex and also because the references to
	 * the vnodes (both dp and vp) can't be released while holding
	 * dnlc_mutex
	 */
	do {
		relcnt = 0;
		pl = LOCK(&dnlc_mutex, FS_DNLCPL);

		while (ncp = dnlc_search(dp, name, namlen, hash, ANYCRED)) {
			relvp[relcnt] = ncp->vp;
			dnlc_rm(ncp);
			if (++relcnt >= NC_REL_SIZE) {
				break;
			}
		}

		UNLOCK(&dnlc_mutex, pl);

		/*
		 * Release references to vnodes for entries found
		 * in the cache on this iteration. We must release
		 * a reference to the directory for each vnode
		 * reference released
		 */
		for (i = 0; i < relcnt; i++) {
			VN_DNLC_RELE(dp);
			VN_DNLC_RELE(relvp[i]);
		}
	} while (relcnt >= NC_REL_SIZE);
}


/*
 * void
 * dnlc_purge_vp(vnode_t *vp)
 *	Purge any cache entries referencing a vnode.
 *
 * Calling/Exit State:
 *	The caller is not required to hold any locks. However,
 *	if this operation is required to interlock with any
 *	other external operations, the caller must provide the
 *	appropriate locking.
 *
 *	This routine is heuristic.
 *
 *	This routine is very similar to dnlc_remove/dnlc_purge_vfsp.
 */
void
dnlc_purge_vp(vnode_t *vp)
{
	ncache_t *ncp;
	int	relcnt;
	int	i;
	pl_t	pl;
	vnode_t	*reldp[NC_REL_SIZE];
	vnode_t	*relvp[NC_REL_SIZE];

	if (!doingcache) {
		return;
	}

	/*
	 * The basic operation is very similar to other dnlc routines
	 * which can remove multiple dnlc entries. The
	 * entire name cache must be searched for dnlc entries whose
	 * vnode pointer (ncp->vp) or directory vnode pointer (ncp->dvp) match
	 * the requested on (parameter vp). 
	 * When either NC_REL_SIZE entries have been gathered
	 * or each dnlc entry has been visited, dnlc_mutex is
	 * released and the references to any vnodes are released.
	 */
	ncp = ncache;
	do {
		relcnt = 0;
		pl = LOCK(&dnlc_mutex, FS_DNLCPL);
		while (ncp < &ncache[ncsize]) {
			if (ncp->dp == vp || ncp->vp == vp) {
				reldp[relcnt] = ncp->dp;
				relvp[relcnt] = ncp->vp;
				dnlc_rm(ncp);
				if (++relcnt >= NC_REL_SIZE) {
					ncp++;
					break;
				}
			}
			ncp++;
		}
		UNLOCK(&dnlc_mutex, pl);
		for (i = 0; i < relcnt; i++) {
			VN_DNLC_RELE(reldp[i]);
			VN_DNLC_RELE(relvp[i]);
		}
	} while (relcnt >= NC_REL_SIZE);

	return;
}

/*
 * int
 * dnlc_purge_vfsp(vfs_t *vfsp, int count)
 *	Purge cache entries referencing a vfsp.
 *
 * Calling/Exit State:
 *	Caller supplies a count of entries to purge; up to that many
 *	will be freed. A count of zero indicates that all such entries
 *	should be purged. Returns the number of entries that were purged.
 *
 *	This routine is typically called during the unmount of a file system.
 *	In such cases, the caller will hold the covered directory's vnode
 *	lock to prevent traversals into the file system being unmounted.
 *	There may still be LWPs making entries into the dnlc for the
 *	file system since they may be in the process of searching a directory
 *	for an entry; if found, an entry into the dnlc may be made.
 *
 *	This routine is heuristic.
 *
 * Remarks:
 *	The file system type may wish this routine to be non-heuristic.
 *	To do so requires the file system type to interlock calls to
 *	dnlc_purge_vfsp with calls to dnlc_lookup/dnlc_enter, i.e.,
 *	block new references from being established for the duration
 *	of this routine.
 */
int
dnlc_purge_vfsp(vfs_t *vfsp, int count)
{
	ncache_t	*ncp;
	int	entries_removed;
	int	relcnt;
	int	i;
	pl_t	pl;
	vnode_t	*relvp[NC_REL_SIZE];
	vnode_t	*reldp[NC_REL_SIZE];

	if (!doingcache) {
		return 0;
	}

	entries_removed = 0;
	ncp = ncache;
	do {
		relcnt = 0;
		pl = LOCK(&dnlc_mutex, FS_DNLCPL);
		while (ncp < &ncache[ncsize]) {
			if (ncp->dp != NULLVP && ncp->dp->v_vfsp == vfsp) {
				entries_removed++;
				relvp[relcnt] = ncp->vp;
				reldp[relcnt] = ncp->dp;
				dnlc_rm(ncp);
				relcnt++;
				if (relcnt >= NC_REL_SIZE ||
				    entries_removed == count) {
					ncp++;
					break;
				}
			}
			ncp++;
		}
		UNLOCK(&dnlc_mutex, pl);
		for (i = 0; i < relcnt; i++) {
			VN_DNLC_RELE(reldp[i]);
			VN_DNLC_RELE(relvp[i]);
		}
	} while (relcnt >= NC_REL_SIZE);


	return (entries_removed);
}

/*
 * int
 * dnlc_purge_fs(int fstype, int count)
 *	Purge cache entries referencing given file system type.
 *
 * Calling/Exit State:
 *	Caller supplies a count of entries to purge; up to that many
 *	will be freed. A count of zero indicates that all such entries
 *	should be purged. Returns the number of entries that were purged.
 */
int
dnlc_purge_fs(int fstype, int count)
{
	ncache_t	*ncp;
	int	entries_removed;
	int	relcnt;
	int	i;
	pl_t	pl;
	vnode_t	*relvp[NC_REL_SIZE];
	vnode_t	*reldp[NC_REL_SIZE];

	if (!doingcache) {
		return 0;
	}

	entries_removed = 0;
	ncp = ncache;
	do {
		relcnt = 0;
		pl = LOCK(&dnlc_mutex, FS_DNLCPL);
		while (ncp < &ncache[ncsize]) {
			if (ncp->dp != NULLVP &&
			    ncp->dp->v_vfsp->vfs_fstype == fstype) {
				entries_removed++;
				relvp[relcnt] = ncp->vp;
				reldp[relcnt] = ncp->dp;
				dnlc_rm(ncp);
				relcnt++;
				if (relcnt >= NC_REL_SIZE ||
				    entries_removed == count) {
					ncp++;
					break;
				}
			}
			ncp++;
		}
		UNLOCK(&dnlc_mutex, pl);
		for (i = 0; i < relcnt; i++) {
			VN_DNLC_RELE(reldp[i]);
			VN_DNLC_RELE(relvp[i]);
		}
	} while (relcnt >= NC_REL_SIZE);

	return (entries_removed);
}

/*
 * STATIC void
 * dnlc_rm(ncache_t *ncp)
 *	Obliterate a cache entry.
 *
 * Calling/Exit State:
 *	The caller must hold dnlc_mutex since
 *	the dnlc is modified here.
 */
STATIC void
dnlc_rm(ncache_t *ncp)
{
	/*
	 * Move the entry to the head of the LRU list
	 * since it's no longer needed.
	 */
	RM_LRU(ncp);
	INS_LRU(ncp, &nc_lru);

	/*
	 * Null reference to vnodes and release reference (if any)
	 * to the credentials.
	 */
	ncp->dp = NULL;
	ncp->vp = NULL;
	if (ncp->cred != NOCRED) {
		crfree(ncp->cred);
		ncp->cred = NOCRED;
	}

	/*
	 * Remove from the hash list and make a dummy
	 * hash chain.
	 */
	RM_HASH(ncp);
	NULL_HASH(ncp);

	return;
}

/*
 * STATIC ncache_t *
 * dnlc_search(vnode_t *dp, char *name, int namlen, int hash, cred_t *cred)
 *	Utility routine to search for a cache entry.
 *
 * Calling/Exit State:
 *	The caller must hold dnlc_mutex.
 *
 *	If an entry matching <dp, name, cred> is found on the specified
 *	hash bucket, a pointer to the dnlc entry is returned.
 *
 *	If the dnlc doesn't contain the requested entry, NULL is returned.
 */
STATIC ncache_t *
dnlc_search(vnode_t *dp, char *name, int namlen, int hash, cred_t *cred)
{
	nc_hash_t	*nhp;
	ncache_t	*ncp;

	nhp = &nc_hash[hash];
	for (ncp = nhp->hash_next; ncp != (struct ncache *) nhp;
	  ncp = ncp->hash_next) {
		if (ncp->dp == dp && ncp->namlen == namlen
		  && (cred == ANYCRED || ncp->cred == cred)
		  && *ncp->name == *name	/* fast chk 1st chr */
		  && bcmp(ncp->name, name, namlen) == 0) {
#ifdef CC_PARTIAL
			lid_t		lid;

			/*
			 * The following section of code is added to
			 * treat a potential covert channel threat in the
			 * use of the dnlc cache to transmit covert
			 * information.
			 *
			 * When an entry is entered to the cache,
			 * the level of the calling process is
			 * registered in the entry's lid field (in
			 * dnlc_enter()).  If at this time the caller's
			 * level is not the same as the level of the
			 * entry, count it as a covert channel event.
			 * Note that once counted, this cache entry
			 * need not be counted again.  The lid field is
			 * zeroed to indicate this fact.
			 *
			 * NOTE: MAC_ACCESS is guaranteed to *not* block
			 * since we are checking process levels ids (i.e.,
			 * we're not checking file level ids) which are
			 * always present in the cache. Even if the process
			 * that made the entry exits (which would remove
			 * the level id from the cache) this usage of
			 * MAC_ACCESS will not block since the operation
			 * being performed is checking for equality (not
			 * domination).
			 *
			 */
			lid = ncp->lid;
			if (lid && MAC_ACCESS(MACEQUAL, lid, CRED()->cr_lid)) {
				ncp->lid = (lid_t)0;
				CC_COUNT(CC_CACHE_DNLC, CCBITS_CACHE_DNLC);
			}
#endif /* CC_PARTIAL */
			return (ncp);
		}
	}
	return ((ncache_t *)NULL);
}

/*
 * STATIC void
 * nc_inshash(ncache_t *ncp, ncache_t *hp)
 *	Insert an entry into the dnlc hash list.
 *
 * Calling/Exit State:
 *	The caller must hold dnlc_mutex. in
 */
STATIC void
nc_inshash(ncache_t *ncp, ncache_t *hp)
{
	ncp->hash_next = hp->hash_next;
	ncp->hash_prev = hp;
	hp->hash_next->hash_prev = ncp;
	hp->hash_next = ncp;
}

/*
 * STATIC void
 * nc_rmhash(ncache_t *ncp)
 *	Remove an entry from the hash list.
 *
 * Calling/Exit State:
 *	The caller must hold dnlc_mutex.
 */
STATIC void
nc_rmhash(ncache_t *ncp)
{
	ncp->hash_prev->hash_next = ncp->hash_next;
	ncp->hash_next->hash_prev = ncp->hash_prev;
}

/*
 * STATIC void
 * nc_inslru(ncache_t *ncp2, ncache_t *ncp1)
 *	Add a dnlc entry to the LRU list.
 *
 * Calling/Exit State:
 *	The caller must hold dnlc_mutex.
 */
STATIC void
nc_inslru(ncache_t *ncp2, ncache_t *ncp1)
{
	ncache_t *ncp3;

	ncp3 = ncp1->lru_next;
	ncp1->lru_next = ncp2;
	ncp2->lru_next = ncp3;
	ncp3->lru_prev = ncp2;
	ncp2->lru_prev = ncp1;
}

/*
 * STATIC void
 * nc_rmlru(ncache_t *ncp)
 *	Remove a dnlc entry from the LRU list.
 *
 * Calling/Exit State:
 *	The caller must hold dnlc_mutex.
 */
STATIC void
nc_rmlru(ncache_t *ncp)
{
	ncp->lru_prev->lru_next = ncp->lru_next;
	ncp->lru_next->lru_prev = ncp->lru_prev;
}


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_dnlc_hash(void)
 *	Dump the dnlc cache.
 *
 * Calling/Exit State:
 *	Called on user request from kernel debuggers.
 *	Prints results on console.
 */
void
print_dnlc_hash(void)
{
	int i;
	nc_hash_t	*nhp;
	ncache_t	*ncp;
	char		ncname[NC_NAMLEN + 1];

	for (i = 0; i < nchash_size; i++) {
		nhp = &nc_hash[i];
		for (ncp = nhp->hash_next; ncp != (struct ncache *)nhp;
		  ncp = ncp->hash_next) {
			bcopy(ncp->name, ncname, NC_NAMLEN);
			ncname[ncp->namlen] = '\0';
			debug_printf("dp = 0x%x, vp = 0x%x, name = %s\n",
				     ncp->dp, ncp->vp, ncname);
			if (debug_output_aborted())
				return;
		}
	}
}

/*
 * void
 * print_dnlc_lru(void)
 *	Dump the dnlc lru chain.
 *
 * Calling/Exit State:
 *	Called on user request from kernel debuggers.
 *	Prints results on console.
 */
void
print_dnlc_lru(void)
{
	ncache_t *ncp;
	char ncname[NC_NAMLEN + 1];

	ncp = nc_lru.lru_next;
	while (ncp != (ncache_t *)&nc_lru) {
		bcopy(ncp->name, ncname, NC_NAMLEN);
		ncname[ncp->namlen] = '\0';
		debug_printf("dp = 0x%x, vp = 0x%x, name = %s\n",
			     ncp->dp, ncp->vp, ncname);
		if (debug_output_aborted())
			return;
		ncp = ncp->lru_next;
	}
}

/*
 * void
 * find_dnlc_by_name(char *nm)
 *	Dump the dnlc cache entries for a particular name.
 *
 * Calling/Exit State:
 *	Called on user request from kernel debuggers.
 *	Prints results on console.
 */
void
find_dnlc_by_name(char *nm)
{
	int i, nmlen;
	nc_hash_t	*nhp;
	ncache_t	*ncp;

	nmlen = strlen(nm);
	for (i = 0; i < nchash_size; i++) {
		nhp = &nc_hash[i];
		for (ncp = nhp->hash_next; ncp != (struct ncache *)nhp;
		  ncp = ncp->hash_next) {
			if (ncp->namlen != nmlen ||
			    bcmp(ncp->name, nm, nmlen) != 0)
				continue;
			debug_printf("dp = 0x%x, vp = 0x%x\n",
				     ncp->dp, ncp->vp);
			if (debug_output_aborted())
				return;
		}
	}
}

/*
 * void
 * find_dnlc_by_vp(vnode_t *vp)
 *	Dump the dnlc cache entries for a particular target vnode.
 *
 * Calling/Exit State:
 *	Called on user request from kernel debuggers.
 *	Prints results on console.
 */
void
find_dnlc_by_vp(vnode_t *vp)
{
	int i;
	nc_hash_t	*nhp;
	ncache_t	*ncp;
	char		ncname[NC_NAMLEN + 1];

	for (i = 0; i < nchash_size; i++) {
		nhp = &nc_hash[i];
		for (ncp = nhp->hash_next; ncp != (struct ncache *)nhp;
		  ncp = ncp->hash_next) {
			if (ncp->vp != vp)
				continue;
			bcopy(ncp->name, ncname, NC_NAMLEN);
			ncname[ncp->namlen] = '\0';
			debug_printf("dp = 0x%x, vp = 0x%x, name = %s\n",
				     ncp->dp, ncp->vp, ncname);
			if (debug_output_aborted())
				return;
		}
	}
}

/*
 * void
 * find_dnlc_by_dp(vnode_t *dp)
 *	Dump the dnlc cache entries for a particular directory vnode.
 *
 * Calling/Exit State:
 *	Called on user request from kernel debuggers.
 *	Prints results on console.
 */
void
find_dnlc_by_dp(vnode_t *dp)
{
	int i;
	nc_hash_t	*nhp;
	ncache_t	*ncp;
	char		ncname[NC_NAMLEN + 1];

	for (i = 0; i < nchash_size; i++) {
		nhp = &nc_hash[i];
		for (ncp = nhp->hash_next; ncp != (struct ncache *)nhp;
		  ncp = ncp->hash_next) {
			if (ncp->dp != dp)
				continue;
			bcopy(ncp->name, ncname, NC_NAMLEN);
			ncname[ncp->namlen] = '\0';
			debug_printf("dp = 0x%x, vp = 0x%x, name = %s\n",
				     ncp->dp, ncp->vp, ncname);
			if (debug_output_aborted())
				return;
		}
	}
}

#endif /* DEBUG || DEBUG_TOOLS */
