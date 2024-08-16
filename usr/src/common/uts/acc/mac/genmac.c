/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:acc/mac/genmac.c	1.16"
#ident	"$Header: $"

/*
 * This file contains GENERIC routines for MAC.
 */

#include <acc/mac/mac.h>
#include <acc/mac/mac_hier.h>	
#include <acc/priv/privilege.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <proc/exec.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/ksynch.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <svc/clock.h>	
#ifdef CC_PARTIAL
#include <acc/mac/covert.h>
#include <acc/mac/cc_count.h>
#endif

STATIC vnode_t *mac_lidvp;	/* MAC LID translation file vnode pointer */
STATIC uint_t  mac_rowmask;     /* for computing row index in lid cache */
STATIC int     lvls_zero_ref;	/* No. of levels in cache with zero ref count */
STATIC fspin_t lvls_zero_ref_lk;
timestruc_t	*sort_arryp;
struct lvls_hdr_blk *lvls_hdr_blk;

/*
 * mac_cachel is the 'length' of the levels cache, i.e. # of rows	
 * Its initial value is a tunable.  mac_init will force it to a
 * non-greater power of two -- power of two favors simple hashing.
 *
 */

/*
 * The lvls_cleaner daemon will leave approximately mac_mru_lvls 
 * zero-referenced levels in each row of the cache (if there are that many) 
 * when it is invoked to remove execess zero-referenced entries.
 * mac_mru_lvls is defined by a tunable.
 *
 */
extern  uint_t 		mac_cachel; 	 
extern  uint_t          mac_mru_lvls;
extern  timestruc_t     hrestime;

STATIC int      mac_lvldom(struct mac_cachent *, struct mac_cachent *);
STATIC void     mac_free(struct mac_cachent *);
STATIC int 	mac_lvl_ops(lid_t, struct mac_cachent **, enum ref_mode);

LKINFO_DECL(a_list_lk_lkinfo,"SEC:lvls_list_lock",0);   
LKINFO_DECL(cachent_lock_lkinfo,"SEC:ca_lock",0);

#define T1_LT_T2(t1,t2) ((t2).tv_sec>(t1).tv_sec||((t2).tv_sec==(t1).tv_sec && (t2).tv_nsec>(t1).tv_nsec)) 
#define T1_LE_T2(t1,t2) ((t2).tv_sec>(t1).tv_sec||((t2).tv_sec==(t1).tv_sec && (t2).tv_nsec>=(t1).tv_nsec)) 
#define DEF_CACHEL 8 			 
#define DEF_MRU_LVLS 2

/* 
 * The daemon to remove cache entries
 * will be invoked  when MRU_RATIO*mac_mru_lvls*mac_cachel or more
 * zero-referenced levels are found present. 
 * For time being MRU_RATIO is regarded
 * as private to cache implementation.  Might eventually be 
 * transferred to mac.h, or made a tunable
 *
 */
#define MRU_RATIO	4

int 	lvls_mru_thresh;
event_t lvlsclnr_event;

/*
 * void lvls_clnr(void *lvlsclnr_argp);
 * 	levels cache cleaner daemon
 * Calling/Exit State:
 *	None.
 *
 */
/* ARGSUSED */
void
lvls_clnr(void *lvlsclnr_argp)
{
        pl_t    opl_a;  /* priority level when lock acquired on active list */
	struct mac_cachent	*searchp;
	struct mac_cachent	*prep;
	int row;
#ifdef MAC_DEBUG
	int lcl_zero_ref;
#endif 

	u.u_lwpp->l_name = "lvls_clnr";

	EVENT_INIT(&lvlsclnr_event);

	for (;;) {
#ifdef MAC_DEBUG
		int all_removed = 0;
#endif
	
		EVENT_WAIT(&lvlsclnr_event, PRIMEM);
#ifdef MAC_DEBUG
		FSPIN_LOCK(&lvls_zero_ref_lk);
		lcl_zero_ref=lvls_zero_ref;
		ASSERT(lvls_zero_ref>=0); 
		FSPIN_UNLOCK(&lvls_zero_ref_lk);
		/*
		 *+  For MAC cache debugging. 
		 */
		cmn_err(CE_NOTE,"lvls_clnr daemon activated at %d entries.",lcl_zero_ref);
#endif
		for (row = 0; row <= mac_rowmask; row++) { 
			int removed = 0;
			int zero_count = 0;
        		opl_a = RW_WRLOCK(&lvls_hdr_blk[row].lvls_list_lock,
					  PL_LVLS_CACHE); 
			if ((searchp = lvls_hdr_blk[row].lvls_start) == NULL) {
				 RW_UNLOCK(&lvls_hdr_blk[row].lvls_list_lock,
					   opl_a);
				 continue;
			}
			/*
			 * The first pass searches for zero-referenced entries 
			 * in the  cache row, to find the mac_mru_lvls most 
		 	 * recently used, except that for code simplicity on 
			 * this and the second pass it does not examine 
			 * the start entry's last reference time.
			 */
			searchp = lvls_hdr_blk[row].lvls_start->ca_next;
			/*first pass, to find the mac_mru_lvls*/ 
			while (searchp != lvls_hdr_blk[row].lvls_start) { 
				/* 
				 * put the lastref time of the first 
			 	 * mac_mru_lvls number of entries with zero 
				 * reference count into the sort array.
				 */
				if (searchp->ca_count == 0) {
					zero_count++;
					if (zero_count < mac_mru_lvls) 
						sort_arryp[zero_count-1] = searchp->ca_lastref;
					if ((zero_count==mac_mru_lvls)||
						((zero_count>mac_mru_lvls)
						&&(T1_LT_T2(sort_arryp[mac_mru_lvls-1],searchp->ca_lastref)))){
	/*
	 * Bubble sort immediately after finding mac_mru_lvls zero referenced
 	 * entries, or later on, whenever a zero-referenced entry is found
	 * that is less stale than the stalest zero-referenced entry in the
	 * sort array.  In the latter case the newly found entry replaces the
	 * stalest entry in the array prior to bubble sorting.
	 */
						int i;
						timestruc_t tmp;
						boolean_t bubble_more = B_TRUE;
						int pairs = mac_mru_lvls;
						sort_arryp[mac_mru_lvls-1]=searchp->ca_lastref;
						while (bubble_more) {
							--pairs;
							bubble_more = B_FALSE;
							for (i = 0; i < pairs; i++) {
								if (T1_LT_T2(sort_arryp[i],sort_arryp[i+1])) {
									tmp = sort_arryp[i];
									sort_arryp[i] = sort_arryp[i+1];
									sort_arryp[i+1] = tmp;
									bubble_more = B_TRUE;
								}
							}
						}
					/* 
					 * At this point the first entry in 
					 * the sort array has the most 
					 * recently used of the zero-referenced
					 * count entries encountered so far.  
					 * The least recent (oldest) of the 
					 * mac_mru_lvls most recent entries is 
					 * in the mac_mru_lvls-1 position in 
					 * the sort array.  After completion
					 * of the first pass, the value in 
					 * this position is the threshold; 
					 * zero reference count entries with 
					 * lastref time earlier (older) than 
					 * that will be removed from the lvls 
					 * list.
					 */
					}
				}
				searchp = searchp->ca_next;
			}

			/* second pass */
			if (zero_count <= mac_mru_lvls) { 
				/* nothing to remove from this cache row*/
				RW_UNLOCK(&lvls_hdr_blk[row].lvls_list_lock,
					  opl_a);
				continue;
			}	
			
			/*
			 * The second pass removes zero-referenced entries 
			 * from the cache row, leaving at least mac_mru_lvls.  
			 * However, it will always leave one -- i.e., not 
			 * honor mac_mru_lvls == 0.  The start entry is never 
			 * removed but contains a valid lid
			 * which at worst may become and remain unreferenced
			 * for an indefinitely long time.
			 */

			prep = lvls_hdr_blk[row].lvls_start;
			searchp = prep->ca_next;
			while (searchp != lvls_hdr_blk[row].lvls_start) {
				/* 
				 * remove entries with ca_lastref matching as 
				 * well as less than threshold time,
				 * in case coarse time stamping leads to 
				 * repeated ca_lastref values. Acceptable 
				 * consequence is that less tham mac_mru_lvls 
				 * may remain in cache.
				 */

				if ((searchp->ca_count == 0) && 
				    (T1_LE_T2(searchp->ca_lastref, 
				     sort_arryp[mac_mru_lvls-1]))) {
					prep->ca_next = searchp->ca_next;
					LOCK_DEINIT(&searchp->ca_lock);
					kmem_free(searchp, 
						sizeof(struct mac_cachent));
					removed++;
				} else {
					prep = searchp;
				}
				searchp = prep->ca_next;
			}
			if (removed > 0) {
		 		FSPIN_LOCK(&lvls_zero_ref_lk);
		 		lvls_zero_ref -= removed;
		 		ASSERT(lvls_zero_ref>=0); 
				FSPIN_UNLOCK(&lvls_zero_ref_lk);
			}
#ifdef MAC_DEBUG
			all_removed += removed;
#endif
			
		RW_UNLOCK(&lvls_hdr_blk[row].lvls_list_lock,opl_a);
#ifdef MAC_DEBUG
		FSPIN_LOCK(&lvls_zero_ref_lk);
		lcl_zero_ref = lvls_zero_ref;
		ASSERT(lvls_zero_ref>=0); 
		FSPIN_UNLOCK(&lvls_zero_ref_lk);
		/*
		 *+  For MAC cache debugging. 
		 */
		cmn_err(CE_NOTE,"lvls_clnr daemon completed row %d leaving %d entries; %d removed.",row,lcl_zero_ref,removed);
#endif
		} /* eof cleaning of one row */
#ifdef MAC_DEBUG
		FSPIN_LOCK(&lvls_zero_ref_lk);
		lcl_zero_ref = lvls_zero_ref;
		ASSERT(lvls_zero_ref>=0); 
		FSPIN_UNLOCK(&lvls_zero_ref_lk);
		/*
		 *+  For MAC cache debugging. 
		 */
		cmn_err(CE_NOTE,"lvls_clnr daemon completed leaving %d entries; %d removed.",lcl_zero_ref,all_removed);
#endif
	} /* eof perpetual event wait loop */
}



/*
 * void mac_init(void)
 * 	This routine initializes the kernel structures necessary for operating
 * 	MAC.  This includes setting up structures for future (kernel internal)
 * 	reads of the LID internal file, and allocating the LID cache.
 *
 * Calling/Exit State:
 * 	This routine is called during the startup phase, after memory
 * 	allocation is operable.  No locks are held at entry and none 
 *	held at exit.
 *
 * Remarks:
 * 	Note that the LID file is not opened for kernel use until init is run.
 * 	Until then, MAC operates in most restrictive mode, i.e. no access is
 * 	allowed for the ordinary user.  MAC can be overridden by the P_MACREAD
 * 	and P_MACWRITE privileges.
 */
void
mac_init(void)
{
	int i;
	int trial;	/* used to make mac_cachel a power of 2 */
	int row;
	ASSERT(KS_HOLD0LOCKS());
	mac_installed = 1; 	/* let rest of kernel know MAC is installed */


	if (mac_cachel < 1 ) {
		/*
		 *+ Warning of inappropriate tunables
		 */
		cmn_err(CE_WARN,"Levels cache set to DEF_CACHEL rows.");
		mac_cachel = DEF_CACHEL;
	}

	if (mac_mru_lvls < DEF_MRU_LVLS ) {
		/*
		 *+ Warning of inappropriate tunables
		 */
		cmn_err(CE_WARN,"Levels cache MRU levels set to DEF_MRU_LVLS.");
		mac_mru_lvls = DEF_MRU_LVLS;
	}

	/* 
	 * whether or not mac_cachel was upsized to DEF_CACHEL, if not a power
	 * of 2 it must be readjusted to a power of 2 <= its current value
	 *
         */

	for (i = 0, trial = 1;( i < NBBY * sizeof(ulong)) && 
	     (2*trial<= mac_cachel); i++, trial += trial)
		;
	if (trial < mac_cachel) {
		mac_cachel = trial;
		/*
		 *+ Warning of inappropriate tunables
		 */
		cmn_err(CE_WARN,"MAC_CACHEL tunable is not a power of 2; levels cache set to %d rows.",mac_cachel);
	}

	/*
	 * Compute the correct row mask from the cache length
	 */

	mac_rowmask = mac_cachel - 1;

	/*
	 * Compute the threshold for invoking the cache cleaner daemon
	 */

	lvls_mru_thresh = MRU_RATIO*mac_mru_lvls*mac_cachel; 
	/*
	 * Allocate and initialize the lvls_hdr_blk's for the mac levels cache
	 */
	if ((lvls_hdr_blk = kmem_zalloc( mac_cachel*sizeof(struct lvls_hdr_blk),KM_NOSLEEP)) == NULL)
		/*
		 *+ Memory should be available without SLEEP
		 *+ during this stage of system initialization
		 */
		cmn_err(CE_PANIC, "mac_init: alloc of levels cache headers failed");
	for (row = 0; row < mac_cachel; row++) {
		lvls_hdr_blk[row].lvls_start = NULL;
		RW_INIT(&lvls_hdr_blk[row].lvls_list_lock,MAC_LIST_HIER,PL_LVLS_CACHE,&a_list_lk_lkinfo,KM_NOSLEEP);
	}

	FSPIN_INIT(&lvls_zero_ref_lk);
	/*
	 * initialize array in which pointer to MRU mac_cachent
	 * will be sorted according to their (most recent reference times)
         *
	 */
	sort_arryp = kmem_zalloc((mac_mru_lvls)*sizeof(timestruc_t),
				  KM_NOSLEEP);

#ifdef CC_PARTIAL
	cc_init();
#endif /* CC_PARTIAL */
}


/*
 * void
 * mac_postroot(void)
 *	Post-root initializations.
 *
 * Calling/Exit State:
 *	Called from main() after mounting root.
 */
void
mac_postroot(void)
{
	extern void lvls_clnr(void *);

	/* Spawn the levels cache cleaner daemon. */
	(void) spawn_lwp(NP_SYSPROC, NULL, LWP_DETACHED, NULL, lvls_clnr, NULL);
}


/*
 * int mac_openlid(char *fname)
 * 	mac_openlid() opens the LID internal file for kernel use.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, mac_openlid returns zero.  On failure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 * 	This routine is called from secsys().
 *
 * Remarks:
 *	The LID file remains open for kernel use as long
 *	as the system in running.  Also, note that this file can be 
 *	opened more than once but likelyhood of happening this is not
 *	that great.  And it does not create any problem except that
 *	the reference on a vnode is more than one as long as it
 *	is the same file. 
 */
int
mac_openlid(char *fname)
{

	int error;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	if (mac_lidvp)
		return EBUSY;

	error = vn_open(fname, UIO_USERSPACE, FREAD, 0, &mac_lidvp,
			(create_t)0);
	return error;
}

struct lcmpa {
	lid_t *lid1;
	lid_t *lid2;
};

/*
 * int lvldom(struct lcmpa *uap, rval_t *rvp)
 * 	This is the system call entry point to determine if one level (lid1)
 * 	dominates another. 
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, lvldom returns zero.  On failure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 *
 * Descriptions:
 * 	The levels are retrieved, then compared. This
 * 	call returns 1 if lid1 dominates lid2, 0 otherwise.
 * 	Note that if either lid is invalid, return is EINVAL.
 */
int
lvldom(struct lcmpa *uap, rval_t *rvp)
{
	struct mac_cachent *level1p, *level2p;
	lid_t lid1, lid2;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

#ifdef CC_PARTIAL
	CC_COUNT(CC_CACHE_MACLVL, CCBITS_CACHE_MACLVL);
#endif

	/*
	 * copy in the lids from user space. If that went OK, validate the 1st
	 * lid & translate it to a level structure. If that succeeds, see if
	 * the *lids* are equal. If so, drop out to return 1. If not,
	 * continue to translate the 2nd lid to a level and do the
	 * domination check. If lid1 dominates lid2, return 1, if not, return
	 * 0. Return -1 and errno on other errors.
	 */

	if ((copyin((caddr_t)uap->lid1, (caddr_t)&lid1, sizeof(lid_t)))
	   || (copyin((caddr_t)uap->lid2, (caddr_t)&lid2, sizeof(lid_t)))) 
		return EFAULT;

	if (mac_lvl_ops(lid1, &level1p,INCR))
		return EINVAL;

	if (MAC_ACCESS(MACEQUAL, lid1, lid2) == 0) {
		rvp->r_val1 = 1;
		mac_free(level1p);
		return 0;
	}

	if (mac_lvl_ops(lid2, &level2p,INCR)) {
		mac_free(level1p);
		return EINVAL;
	}

	if (mac_lvldom(level1p, level2p) == 0)
		rvp->r_val1 = 1;

	mac_free(level1p);
	mac_free(level2p);
	return 0;
}


/*
 * int lvlequal(struct lcmpa *uap, rval_t *rvp)
 * 	This is the system call entry point to determine if one level (lid1)
 * 	equals another. 
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, lvlequal returns zero.  On failure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 *
 * Description:
 *	Under the current implementation, it is a simple check for equality 
 * 	on the LIDs. But, we must make sure that it is a valid lid, so
 * 	call mac_lvl_ops to validate it.  If the lids are equal and valid,
 * 	1 is returned, if the lids are not equal, 0 is returned.  If 
 * 	there was a problem validating a lid, return EINVAL.
 */

int
lvlequal(struct lcmpa *uap, rval_t *rvp)
{
	lid_t lid1, lid2;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	if ((copyin((caddr_t)uap->lid1, (caddr_t)&lid1, sizeof(lid_t)))
	   || (copyin((caddr_t)uap->lid2, (caddr_t)&lid2, sizeof(lid_t))))
		return EFAULT;

	if (mac_valid(lid2))
		return EINVAL;

	/*
	 * For performance, equality check is performed before 
	 * validating other lid.
	 */
	if (MAC_ACCESS(MACEQUAL, lid1, lid2) == 0) {
		rvp->r_val1 = 1;
		return 0;
	}

	/*
	 * lids are not equal, but if either specified lid is invalid,
	 * return EINVAL.
	 */
	if (mac_valid(lid1))
		return EINVAL;
	
	/* lids are valid, but not equal */
	return 0;
}


struct mldmodea {
	int mldmode;
};
/*
 * int mldmode(struct mldmodea *uap, rval_t *rvp)
 * 	This system call manipulates the MLD mode of the current
 * 	process.
 *
 * Calling/Exit State:
 *	No locks to be held on entry and none held on return.
 *	On success, mldmode returns zero.  On failure, it returns
 *	the appropriate errno.  This function must be called at PLBASE.
 */
int
mldmode(struct mldmodea *uap, rval_t *rvp)
{
	cred_t *tmpcrp;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	switch (uap->mldmode) {
	case MLD_REAL:
		/*
		 * If not already in real mode, set real mode
		 */
		if (!(CRED()->cr_flags & CR_MLDREAL)) {
			tmpcrp = crdup2(CRED());
			tmpcrp->cr_flags |= CR_MLDREAL;
			crinstall(tmpcrp);
		}
		break;

	case MLD_VIRT:
		/*
		 * If not already in virtual mode, set virtual mode
		 */
		if (CRED()->cr_flags & CR_MLDREAL) {
			tmpcrp = crdup2(CRED());
			tmpcrp->cr_flags &= ~CR_MLDREAL;
			crinstall(tmpcrp);
		}
		break;

	case MLD_QUERY:
		/*
		 * Return the current MLD mode. If we're in real mode,
		 * return MLD_REAL (1). Otherwise, return MLD_VIRT (0).
		 */
		if (CRED()->cr_flags & CR_MLDREAL) 
			rvp->r_val1 = 1;
		break;

	default:
		return EINVAL;
	}

	return 0;
}


/*
 * int mac_liddom(lid_t lid1, lid_t lid2)
 *	Perform lid domination check.
 * 
 * Calling/Exit State:
 *	On success, mac_liddom returns zero.  On failure, it returns
 *	the appropriate errno.  
 *
 * Description:
 * 	Unfortunately, a function is necessary to allocate local 
 * 	variables level1p and level2p.
 *
 * Remarks:
 * 	Note that this routine is not geared towards performance.
 * 	There is no reason to speed up performance for the failure case.
 * 	Performance is achieved by calling the MAC_ACCESS() macro.
 */
int
mac_liddom(lid_t lid1, lid_t lid2)
{
	struct mac_cachent *level1p;
	struct mac_cachent *level2p;
	int error;

	if (mac_lvl_ops(lid1, &level1p,INCR))
		return EACCES;

	if (mac_lvl_ops(lid2, &level2p,INCR)) {
		mac_free(level1p);
		return EACCES;
	}

	error = mac_lvldom(level1p, level2p);

	mac_free(level1p);
	mac_free(level2p);

	if (error)
		return EACCES;
	return 0;
}


/*
 *	mac_lvl_ops(lid_t lid, struct  mac_cachent **levelpp, enum ref_mode mode)
 *
 * 	This routine retrieves a pointer to the full level structure for the 
 * 	LID given by "lid", and places it in the mac_level structure pointer
 * 	referenced by "levelpp".
 *
 * Calling/Exit State:
 *	On success, mac_lvl_ops returns zero.  On failure, it returns
 *	the appropriate errno.  
 *	WARNING: Depending on mode, levelpp may not be valid when accessed by the
 *      caller, since the cache entry to which it points may have been removed
 *      in the interim.  It is intended that it be accessed only following
 *      a call with mode = INCR.  The caller must maintain the discipline. 
 */

STATIC int
mac_lvl_ops(lid_t lid, struct mac_cachent **levelpp, enum ref_mode mode)
{
	pl_t	opl_e;	/* priority level when lock on cache entry */
	pl_t	opl_a; 	/* priority level when lock acquired on active list */
	struct mac_cachent	*searchp;	/* running search pointer   */
	struct mac_cachent	*cachentp;	/* entry examined	    */
	struct mac_cachent	*tmpentp=NULL;	/* tmp ptr to kma'd entry   */
	struct mac_level	levelbuf;	/* place to read level into */
        struct uio lid_uio;             /* for VOP_READ of LID xlation file */
        struct iovec lid_aiov;          /* for VOP_READ of LID xlation file */
	int row;
        int error = 0;
	enum { FIRST=1,SECOND } cache_pass;

	row = lid & mac_rowmask;
	cache_pass = FIRST;
	opl_a = RW_RDLOCK(&lvls_hdr_blk[row].lvls_list_lock, PL_LVLS_CACHE);

search: 
	/* a SECOND pass past this point occurs if no hit on FIRST pass, 
         * to assure that a race does not multiply enter a lid/level into
         * the levels cache.
         */

	if ((cache_pass == FIRST) && (lvls_hdr_blk[row].lvls_start == NULL))
		goto readin_lvl;	/* levels list empty */
	if ((cache_pass == SECOND) && (lvls_hdr_blk[row].lvls_start == NULL))
		goto enter_entry;	/* levels list empty */
	searchp = lvls_hdr_blk[row].lvls_start;
	do {
		/*
		 * examine one entry ahead so searchp can be retained
		 * as pointer to entry pointing to entry under;
		 * future chnages to this code may exploit or remove
		 * this mechanism.
		 */
		cachentp = searchp->ca_next;
		if (lid == cachentp->ca_lid) { 	/*hit*/
			boolean_t event_flag = B_FALSE;
			ASSERT(mode != DECR ? 1 :(cachentp->ca_count > 0));
			if (cache_pass == SECOND) {
				cachentp->ca_lastref = hrestime;
				cachentp->ca_count += mode;
				if ((mode == INCR) && 
				    (cachentp->ca_count == 1)) {
					FSPIN_LOCK(&lvls_zero_ref_lk);
					lvls_zero_ref--;
					FSPIN_UNLOCK(&lvls_zero_ref_lk);
				} else if ((mode == DECR) && 
					   (cachentp->ca_count == 0)) {
					FSPIN_LOCK(&lvls_zero_ref_lk);
					lvls_zero_ref++;
					if (lvls_zero_ref > lvls_mru_thresh) 
						event_flag = B_TRUE;
					FSPIN_UNLOCK(&lvls_zero_ref_lk);
				}
				/*
				 * Another lwp won the race to insert the
				 * level so deallocate  the kma'd entry
				 */
				RW_UNLOCK(&lvls_hdr_blk[row].lvls_list_lock,opl_a);
				ASSERT(tmpentp != NULL);
				kmem_free(tmpentp, sizeof(struct mac_cachent));
			} else {  /* FIRST pass */
				opl_e = LOCK (&cachentp->ca_lock,PL_LVLS_CACHE);
				cachentp->ca_lastref = hrestime;
				cachentp->ca_count += mode;
				if ((mode == INCR) && (cachentp->ca_count == 1)){
					FSPIN_LOCK(&lvls_zero_ref_lk);
					lvls_zero_ref--;
					FSPIN_UNLOCK(&lvls_zero_ref_lk);
				} else if ((mode == DECR) && (cachentp->ca_count== 0)) {
					FSPIN_LOCK(&lvls_zero_ref_lk);
					lvls_zero_ref++;
					if (lvls_zero_ref > lvls_mru_thresh) 
						event_flag=B_TRUE;
					FSPIN_UNLOCK(&lvls_zero_ref_lk);
				}
				UNLOCK (&cachentp->ca_lock,opl_e);
				RW_UNLOCK (&lvls_hdr_blk[row].lvls_list_lock,opl_a);
			}
			if (event_flag)
				EVENT_SIGNAL(&lvlsclnr_event,0);
			*levelpp = cachentp;
			return 0;
		}
		searchp = cachentp;
	} while (searchp != lvls_hdr_blk[row].lvls_start);

	/*
    	 * Arrival here means no hit, whether on first or second pass.
	 *
	 * If it's the SECOND copy the level from the buffer into the kma'd
    	 * entry, and insert that entry into the list, and return.
	 *
	 */
enter_entry:
	if (cache_pass == SECOND) { 
		boolean_t event_flag = B_FALSE;
			if (lvls_hdr_blk[row].lvls_start != NULL) {
				tmpentp->ca_next = lvls_hdr_blk[row].lvls_start->ca_next;
				lvls_hdr_blk[row].lvls_start->ca_next = tmpentp;
			} else {
				lvls_hdr_blk[row].lvls_start = tmpentp->ca_next =tmpentp;
			}
		bcopy(&levelbuf,&tmpentp->ca_level, sizeof(struct mac_level));
		tmpentp->ca_lastref = hrestime;
		tmpentp->ca_lid = lid;
		ASSERT(mode!=DECR);
		tmpentp->ca_count = mode;
		if (mode == NO_CHANGE) {
			/* 
			 * A new entry was entered on behalf of a
			 * a mac_valid call.  The ca_count should
			 * be zero, and this entry becomes a candidate for
			 * removal from the cache.  
			 *
			 */
			FSPIN_LOCK(&lvls_zero_ref_lk);
			lvls_zero_ref++;
			if (lvls_zero_ref > lvls_mru_thresh)
				event_flag = B_TRUE;
			FSPIN_UNLOCK(&lvls_zero_ref_lk);
		}
		RW_UNLOCK(&lvls_hdr_blk[row].lvls_list_lock,opl_a);
		if (event_flag)
			EVENT_SIGNAL(&lvlsclnr_event,0);
		*levelpp = tmpentp;
		return 0;
	}

	/*
	 * No hit after first pass - readin the level into kma'd space 
	 *
 	 */


readin_lvl:
	RW_UNLOCK(&lvls_hdr_blk[row].lvls_list_lock,opl_a);

	/*
	 * If lid.internal is not yet open, return invalid LID always.
	 * This check is minor considering the read that will
	 * occur next.  This check is done as late as possible to
	 * avoid an extra check in most cases.
	 */

	if (mac_lidvp == (struct vnode *)NULL) {
		*levelpp = NULL;
		return EINVAL;
	}

	/*
	 * We didn't find the LID in the cache. Read the corresponding
	 * level into a buffer. We don't read directly into the cache
	 * just in case we get a null record. If the read was not a null
	 * record, it will subsequently be moved 
 	 * to the level portion of the 'replacement'
	 * cache entry that we were keeping track of in the row search,
	 * even if the record is not active (so the access checking
	 * routines will work on non-active lids).  Then the lid,
 	 * reference time, and reference count will be set, and the return
   	 * pointer set.
	 */

	lid_uio.uio_segflg = UIO_SYSSPACE;
	lid_uio.uio_limit = LONG_MAX;
	lid_uio.uio_fmode = FREAD;
	lid_uio.uio_resid = lid_aiov.iov_len = sizeof(struct mac_level);
	lid_uio.uio_iov = &lid_aiov;
	lid_uio.uio_iovcnt = 1;
	lid_aiov.iov_base = (caddr_t)&levelbuf; /*target address*/
	lid_uio.uio_offset = lid * sizeof(struct mac_level); /*file offset*/
	VOP_RWRDLOCK(mac_lidvp,0,0,lid_uio.uio_fmode); 
	/* 
	 * NB the 0's in VOPs above and below must be replaced 
	 * by proper values for off,len,fmode 
	 */
	error = VOP_READ(mac_lidvp, &lid_uio, 0, CRED());
	VOP_RWUNLOCK(mac_lidvp, 0, 0);
	if (error) {
		*levelpp = NULL;
		return error;
	}

	/*
	 * if resid is >0, we went past end of file. return EINVAL
	 */
	if (lid_uio.uio_resid) {
		*levelpp = NULL;
		return EINVAL;
	}
	/*
	 * if the validity flag is null, this lid was never assigned
	 * a value. Return EINVAL.
	 */
	if (levelbuf.lvl_valid == (unchar)0){
		*levelpp = NULL;
		return EINVAL;
	}
	/*
	 * Is either a valid active record, or an inactive (deleted) record.
	 * Prepare an entry for it in the active list.
	 */

	/* Allocate a kma entry */
	tmpentp = kmem_zalloc(sizeof(struct mac_cachent),KM_SLEEP);
	LOCK_INIT(&tmpentp->ca_lock,MAC_ENTRY_HIER,PL_LVLS_CACHE,&cachent_lock_lkinfo,KM_NOSLEEP);
	cache_pass = SECOND;
	opl_a = RW_WRLOCK(&lvls_hdr_blk[row].lvls_list_lock,PL_LVLS_CACHE);
	goto search;
}

/*
 * STATIC int mac_lvldom(struct mac_cachent *level1p, 
 * 			 struct mac_cachent *level2p)
 * 	This routine determines whether the level in the mac_level structure
 * 	referenced by "level1p" dominates the level in the mac_level structure 
 * 	referenced by "level2p". 
 *
 * Calling/Exit State:
 *	It returns 0 if levellp dominates level2p, EACCES otherwise.
 */

STATIC int
mac_lvldom(struct mac_cachent *level1p, struct mac_cachent *level2p)
{
	ushort *catsig1p, *catsig2p;
	ulong *cat1p, *cat2p;
	int catsig1, catsig2;

	/*
	 * If the second level's classification is greater than the first
	 * we know the domination check fails.
	 */
	if (level2p->ca_level.lvl_class > level1p->ca_level.lvl_class)
		return EACCES;

	/*
	 * Set up the pointers to the category significance arrays, and the
	 * category bit arrays for both levels.
	 */

	catsig1p = level1p->ca_level.lvl_catsig;
	catsig2p = level2p->ca_level.lvl_catsig;
	cat1p = level1p->ca_level.lvl_cat;
	cat2p = level2p->ca_level.lvl_cat;

	/*
	 * The last entry of the category significance array  *always* contains
	 * a null, so we have to make only one check (no check for exceeding
	 * size of lvl_catsig[]).
	 */

	/* while there are more categories in each level */
	while (*catsig1p != 0 && *catsig2p != 0) {
		/*lessen indirection*/
		catsig1 = *catsig1p;
		catsig2 = *catsig2p;
		/*
		 * if 1st level has a chunk of category bits the second
		 * level doesn't, skip them.
		 */
		if (catsig1 < catsig2) {
			catsig1p++;
			cat1p++;
			/*
		 	 * if the significance of the currently referenced 
			 * category bits is the same...
		 	 */
		} else if (catsig1 == catsig2) {
			/*
			 * see if level 1's bits are a superset of
			 * level 2's bits
			 */
			if (!(*cat2p & ~*cat1p)) {
				/* 
				 * Since they are, point to the next 
				 * set of bits and significance values.
				 */
				catsig1p++;
				catsig2p++;
				cat1p++;
				cat2p++;
			} else 
				break;
		} else 
			break;
	}

	/*
	 * When we get to this point, as long as we've exhausted the *second*
	 * level's category bits, we have domination.
	 */

	if (*catsig2p == 0)
		return 0;
	return EACCES;
}


/*
 * STATIC void mac_free(struct mac_cachent *cachentp)
 * 	This routine decrements cache reference count for the given entry.
 *
 * Calling/Exit State:
 *	This call should only be made following an associated mac_lvl_ops
 *	call made with mode = INCR, and which returned the cachentp value
 *      provided as an argument to this call. This discipline is the 
 *	caller's responsibility.
 *
 *	mac_lvl_ops(..,..,DECR) is an alternative, but mac_free avoids
 *      the search.
 *
 */
/* ARGSUSED */
STATIC void
mac_free(struct mac_cachent *cachentp)
{
	pl_t	opl_e;	/* priority level when lock on cache entry */
	pl_t	opl_a; 	/* priority level when lock acquired on active list */
	int row = mac_rowmask & cachentp->ca_lid; 
	boolean_t event_flag = B_FALSE;

	opl_a = RW_RDLOCK(&lvls_hdr_blk[row].lvls_list_lock,PL_LVLS_CACHE);
	opl_e = LOCK (&cachentp->ca_lock,PL_LVLS_CACHE);
	ASSERT(cachentp->ca_count > 0);
	cachentp->ca_lastref = hrestime;
	if ((--cachentp->ca_count) == 0) {
		FSPIN_LOCK(&lvls_zero_ref_lk);
		lvls_zero_ref++;
		if (lvls_zero_ref > lvls_mru_thresh) 
			event_flag=B_TRUE;
		FSPIN_UNLOCK(&lvls_zero_ref_lk);
	}
 	UNLOCK(&cachentp->ca_lock,opl_e);
	RW_UNLOCK(&lvls_hdr_blk[row].lvls_list_lock,opl_a);
 	if (event_flag)
		EVENT_SIGNAL(&lvlsclnr_event,0);
}


/*
 * int mac_lid_ops(lid_t lid, enum ref_mode mode)
 *
 *      This routine will enter the level into the cache if
 *      it is not present. Depending on the value of mode, 
 *      the reference count of the level, once in the cache will
 *
 *      DECR	   Decremented by one
 *	NO_CHANGE  unchanged, or be zero if level is newly entered
 *      INCR       Incremented by one
 *
 * Calling/Exit State:
 *	Return 0 on success and EINVAL on failure.
 */

int
mac_lid_ops(lid_t lid, enum ref_mode mode)
{
	struct mac_cachent *cachentp;
	if (mac_lvl_ops(lid, &cachentp, mode))
		return EINVAL;
	return 0;
}

#ifdef DEBUG

/*
 * void
 * print_mac_lidcache(void)
 * 	Formatted dump of the mac_cachent structures
 *
 * Calling/Exit State:
 * 	None.
 */
void
print_mac_lidcache(void)
{
	struct mac_cachent	*searchp, *cachentp;
	int row;
	ushort *catsigp;	/* ptr to category significance array entry */
	ulong *catp, i;		/* ptr to category array entry */
	int firstcat = 1;

	debug_printf("             LAST REFERENCE TIME FL\n");
	debug_printf("ROW       LID     (SEC)   (NSEC) AG CLASS CATEGORIES\n\n");
	for (row = 0; row < mac_rowmask; row++) {
		if ((searchp = lvls_hdr_blk[row].lvls_start) == NULL)
			 continue;
		searchp = lvls_hdr_blk[row].lvls_start->ca_next;
		do {
			cachentp = searchp->ca_next;
			debug_printf("[%2d] %8d %9d %9d %c  %3d  ",
				row,
				cachentp->ca_lid,
				cachentp->ca_lastref.tv_sec,
				cachentp->ca_lastref.tv_nsec,
				cachentp->ca_level.lvl_valid,
				cachentp->ca_level.lvl_class);

			/*
			 * Print out the category numbers in effect.
			 */
			for (catsigp = &cachentp->ca_level.lvl_catsig[0],
				catp = &cachentp->ca_level.lvl_cat[0];
			     *catsigp != 0; catsigp++, catp++) {
				for (i = 0; i < NB_LONG; i++) {
					if (*catp & (((ulong)1 << (NB_LONG - 1)) >> i)) {
						if (!firstcat)
							debug_printf(",");
						/* print the category # */
						debug_printf("%d",
							((ulong) (*catsigp-1) << CAT_SHIFT)
							+ i + 1);
						firstcat = 0;
					} /* end-if catp */
				} /* end-for till CATSIZE */
			} /* end-for catsig */
			debug_printf("\n");
			searchp = cachentp;
		}while (searchp != lvls_hdr_blk[row].lvls_start);
	}
}

#endif
