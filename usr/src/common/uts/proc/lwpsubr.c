/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/lwpsubr.c	1.49"
#ident	"$Header: $"

#include <acc/audit/audit.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <mem/ublock.h>
#include <proc/acct.h>
#include <proc/class.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/proc_hier.h>
#include <proc/resource.h>
#include <proc/signal.h>
#include <proc/ucontext.h>
#include <proc/uidquota.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/syscall.h>
#include <svc/systm.h>
#include <util/bitmasks.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/dl.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

/*
 *+ Governs most of the state in the lwp structure.
 */
LKINFO_DECL(lwplockinfo, "PP::l_mutex", 0);

STATIC boolean_t lwp_mapid(proc_t *, k_lwpid_t *);
extern void copy_ublock(caddr_t , caddr_t );
extern int save(lwp_t *);
extern void parmsret(void);
extern void lwp_setup_f(lwp_t *);
extern void lwp_cleanup_f(lwp_t *);
extern void *lwp_setprivate(void *);
extern void setup_newcontext(dupflag_t, boolean_t, struct user *, 
		             void (*)(void *), void *);
extern void systrap_cleanup(rval_t *, unsigned int, int);
extern void fix_retval(rval_t *);


/*
 *
 * lwp_t *lwp_setup(lwp_t *crlwpp, int cond, proc_t *procp)
 * 	This function allocates the LWP structure and allocates other
 * 	resources such as the u- block and the class specific LWP structure.
 *	This function also initializes all of the state in the created LWP.
 *
 * Calling/Exit State:
 *    Locking:
 * 	No locks held on entry and no locks held on return.
 *	This function can sleep.
 *    Return value:
 * 	This function returns a pointer to the new initialized LWP 
 *	structure; else returns a NULL pointer to indicate failure.
 *
 * Remarks:
 *	The caller is responsible for ensuring that the reference counts
 *	to the credentials and root directory vnode are properly incremented.
 *	This work is not accomplished here for efficiency concerns when this
 *	function is used by fork(2) operations.
 *
 * Arguments:
 *	crlwpp	pointer to the creating LWP
 *	cond	NP_{SYSPROC,FAILOK,FORK1,FORKALL,VFORK}
 * 	procp	pointer to the process containing the new lwp
 */
lwp_t *
lwp_setup(lwp_t *crlwpp, int cond, proc_t *procp)
{
	lwp_t		*newlwpp;
	vaddr_t		ubaddr;
	pl_t		pl;

	ASSERT(KS_HOLD0LOCKS());

	/* Allocate the LWP structure */
	newlwpp = kmem_zalloc(sizeof(lwp_t), KM_SLEEP);

	newlwpp->l_procp = procp;

	/* Allocate the u block */
	ubaddr = ublock_lwp_alloc(newlwpp);
	if (ubaddr == 0) {
		/* Roll back the state */

		if (cond & NP_FAILOK) {
			kmem_free(newlwpp, sizeof(lwp_t));
			MET_LWP_FAIL();
			return (NULL);
		}

		/*
		 *+ Could not allocate a u block when the caller did not 
		 *+ expect failure. No corrective action can be taken by the 
		 *+ the user.
		 */
		cmn_err(CE_PANIC, "lwp_setup(): failed to allocate ublock\n");
	}

	/*
	 * Initialize the l_up field to point to this LWP's uarea.
	 */
	newlwpp->l_up = UBLOCK_TO_UAREA(ubaddr);

	/* Allocate the class specific proc structure */

	newlwpp->l_cllwpp = CL_ALLOCATE(&class[crlwpp->l_cid], newlwpp);
	if (newlwpp->l_cllwpp == NULL) {

		/*roll back the state*/

		if (cond & NP_FAILOK) {
			ublock_lwp_free(newlwpp);
			kmem_free(newlwpp, sizeof(lwp_t));
			MET_LWP_FAIL();
			return(NULL);

		}
		/*
		 *+ Could not allocate the class specific proc structure
		 *+ when the caller did not expect failure.
		 *+ No corrective action can be taken by the the user.
		 */
		cmn_err(CE_PANIC, "lwp_setup(): CL_ALLOCATE() failed\n");
	}

	LOCK_INIT(&newlwpp->l_mutex, LWP_HIER, LWP_MINIPL,
		  &lwplockinfo, KM_SLEEP);
	EVENT_INIT(&newlwpp->l_pollevent);
	EVENT_INIT(&newlwpp->l_slpevent);

	/* Do family-specific setup */
	lwp_setup_f(newlwpp);

	/*
	 * Clone the pertinent state from the context in whose image this 
	 * LWP is being created.
	 */
	ASSERT(KS_HOLD0LOCKS()); 

	/* 
	 * Copy all state from the creating LWP that may be changed by other
	 * contexts while we are doing the copy.  [NOTE: Should include any
	 * other state that should be copied under the protection of l_mutex
	 * here as well.]
	 */
	pl = LOCK(&crlwpp->l_mutex, PLHI);
	newlwpp->l_pri = crlwpp->l_pri;
	newlwpp->l_rq = crlwpp->l_rq;
	UNLOCK(&crlwpp->l_mutex, pl);

	/*
	 * Finish initialization of the LWP with fields that don't
	 * need locking (since the LWP being created is not visible
	 * to any other contexts).
	 */
	newlwpp->l_stat = SIDL;	
	newlwpp->l_cid = crlwpp->l_cid;
	newlwpp->l_clfuncs = crlwpp->l_clfuncs;

	/* initialize the shared state */

	if (cond & (NP_FORK | NP_FORKALL | NP_VFORK)) {
						/* called by a forking client */
		newlwpp->l_cred = newlwpp->l_procp->p_cred;
		newlwpp->l_rdir = u.u_lwpp->l_rdir;
		newlwpp->l_lwpid = crlwpp->l_lwpid;
	} else {				/* called via _lwp_create(2) */
		newlwpp->l_cred = crlwpp->l_cred;
		newlwpp->l_rdir = crlwpp->l_rdir;
	}

	/*
	 * Initialize/inherit signal state.
	 */
	newlwpp->l_sigheld = crlwpp->l_sigheld;

	/* System call trace flags */
	if (procp->p_entrymask)
		newlwpp->l_trapevf |= EVF_PL_SYSENTRY;
	if (procp->p_exitmask)
		newlwpp->l_trapevf |= EVF_PL_SYSEXIT;
	return (newlwpp);
}


/*
 *
 * void lwp_cleanup(lwp_t *lwpp)
 *	Clean up from a failed fork or lwp_create operation begun by
 *	lwp_setup().  This function performs the basic recovery operations and
 *	gets rid of all the data structures allocated by lwp_setup().
 *
 * Calling/Exit State:
 *	No locks held on entry and no locks held on return.
 *
 */
void
lwp_cleanup(lwp_t *lwpp)
{
	ASSERT(KS_HOLD0LOCKS());

	CL_DEALLOCATE(&class[u.u_lwpp->l_cid], lwpp->l_cllwpp);

	ublock_lwp_detach(lwpp);
	ublock_lwp_free(lwpp);

	/* Do family-specific cleanup */
	lwpp->l_up = NULL;
	lwp_cleanup_f(lwpp);

	LOCK_DEINIT(&lwpp->l_mutex);
	/* Don't need to deinit EVENTS (l_pollevent) */

	kmem_free(lwpp, sizeof(*lwpp));
}


/*
 *
 * void lwp_dup(lwp_t *lwpp, lwp_t *newlwpp, dupflag_t dupflags,
 *		void (*funcp)(void *), void *argp) 
 * 	Duplicate the LWP.
 * 	It is assumed that the address space has already been set up.
 * 	This function cannot fail!
 *
 * 	The following actions are performed by this function:
 *
 *		1) copies the u block.
 *		2) arranges for all created LWPs to return appropriately. 
 *
 * Calling/Exit State:
 * 	No locks held on entry and no locks held on return.
 *	This function may sleep.
 *
 *
 * Remarks:
 *	The caller is responsible for ensuring that the reference count
 *	for the resource limits object referenced from u_rlimits is properly
 *	incremented.  This work is not accomplished here for efficiency
 *	concerns when this function is used by fork(2) operations. 
 *
 */
void
lwp_dup(lwp_t *lwpp,		/* pointer to the calling context */
	lwp_t *newlwpp,		/* the LWP being created */
	dupflag_t dupflags,	/* describes cloning */
	void (*funcp)(void *),	/* func to be executed by the new context */
	void *argp)		/* arg for the function	*/
{
	user_t		*cusr_adr;
	user_t 		*pusr_adr;
	lwp_t		*pwalkp;
	lwp_t		*cwalkp;

	ASSERT(KS_HOLD0LOCKS());

	
	if (dupflags != DUP_FORKALL) {
		pwalkp = u.u_lwpp;
		cwalkp = newlwpp;
	} else {
		pwalkp = lwpp->l_procp->p_lwpp; 
		cwalkp = newlwpp->l_procp->p_lwpp;
	}
	do {
		pusr_adr = pwalkp->l_up;
		cusr_adr = cwalkp->l_up;
		/*
		 * There is a possibility that the LWPs in the parent
		 * that have rendezvoused may not have fully switched out.
		 * This is a very unlikely scenario, but we have to allow
		 * for it.  Since swtch() is called holding l_mutex, we can
		 * close the window by acquiring l_mutex even when we don't
		 * need it for the call to save().
		 */
		(void)LOCK(&pwalkp->l_mutex, PLHI);
		if (pwalkp == lwpp)
			(void)save(lwpp);
		UNLOCK(&pwalkp->l_mutex, PLBASE);

		/* 
		 * Copy relevant portion of the u block.
		 */ 
		copy_ublock((caddr_t)pusr_adr, (caddr_t)cusr_adr);
		cusr_adr->u_lwpp = cwalkp;
		cusr_adr->u_procp = cwalkp->l_procp;
		cusr_adr->u_acflag = AFORK;
		cusr_adr->u_ior = dl_zero;
               	cusr_adr->u_iow = dl_zero;
               	cusr_adr->u_ioch = dl_zero;
		cusr_adr->u_italarm[0] = NULL;
		cusr_adr->u_italarm[1] = NULL;

		/*
		 * NOTE: The resource limits are _always_ set
		 *	 to those of the calling LWP, and NOT
		 *	 the LWP in the parent.  This is most
		 *	 expedient for the resource limit object
		 *	 management of proc_setup().
		 */
		cusr_adr->u_rlimits = u.u_rlimits;

		/*
		 * Initialize the saved sate and the stack state for the
		 * new context.
		 */
		setup_newcontext(dupflags, 
		 	((cwalkp->l_procp->p_flag & P_SYS) ? B_TRUE : B_FALSE), 
			           cusr_adr, funcp, argp);

		cwalkp = cwalkp->l_next;
		pwalkp = pwalkp->l_next;
	} while ((cwalkp != NULL) && (dupflags == DUP_FORKALL));
}


/*
 *
 * k_lwpid_t lwp_dirslot(proc_t *procp, lwp_t *newlwpp) 
 * 	This function allocates an unused LWP-ID and LWP directory slot
 *	for the calling process, and initializes the corresponding LWP
 *	directory with the given passed-in LWP pointer.
 *
 * Calling/Exit State:
 * 	No spin locks can be held on entry.  If successful, this function
 *	returns the non-zero value of the newly allocated LWP-ID, with the
 *	p_mutex lock of the calling process held.  If unsuccessful, a
 *	value of zero is returned, and the p_mutex lock is not held.
 *
 * Remarks:
 * 	This function may have to sleep to allocate memory.  This function
 *	assumes that no user-ID in the system (without exception) can
 *	create another LWP when there are already N or more LWPs with the
 *	same user-ID, where N is the minimum of:
 *	    1.	the maximum value of the data type used to contain p_nlwpdir,
 *	    2.	the maximum value of a k_lwpid_t data type.
 *
 */

k_lwpid_t
lwp_dirslot(proc_t *procp, lwp_t *newlwpp)

{
	k_lwpid_t	lwpid;
	lwp_t		**dirp;
	size_t		new_dirsize;
	size_t		dircopysize;
	size_t		mapcopysize;
	uint_t		*lwpid_bitmap;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	(void)LOCK(&procp->p_mutex, PLHI);

	/* 
	 * Check to see if there is free slot in the existing LWP directory
	 * and LWP-ID bit map.
	 */
	if (lwp_mapid(procp, &lwpid)) {		/* Available free slot found */
		if (procp->p_nlwpdir == 1) {
			/* Need to allocate the directory the first time */
			UNLOCK(&procp->p_mutex, PLBASE);
			dirp = kmem_zalloc(NBITPW * sizeof(lwp_t *), KM_SLEEP);
			(void)LOCK(&procp->p_mutex, PLHI);
			procp->p_nlwpdir = NBITPW;
			dirp[u.u_lwpp->l_lwpid - 1] = u.u_lwpp;
			dirp[lwpid - 1] = newlwpp;
			procp->p_lwpdir = dirp;
		} else {
			procp->p_lwpdir[lwpid - 1] = newlwpp;
		}
		return (lwpid);			/* Return with p_mutex held */
	}
	
	/* 
	 * No LWP-ID/dirslot is available in the existing maps; need to
	 * grow the LWP directory and LWP-ID bit-map structures used to
	 * maintain the LWP directory/LWP-ID state.
	 */

	UNLOCK(&procp->p_mutex, PLBASE);

	/* 
	 * Each time we grow the LWP directory, we will grow it by 
	 * NBITPW (number of bits in a uint_t).
	 * The LWP-ID-bitmap will be grown accordingly. 
	 */

	for(;;) {
		/*
		 * Compute the next higher LWP directory and bit map size.
		 * Make sure however that we do not go over USHRT_MAX, since
		 * both p_nlwpdir and k_lwpid_t are declared as ushort_t.
		 */
		new_dirsize = (size_t)procp->p_nlwpdir + NBITPW;
		if (new_dirsize > USHRT_MAX)
			return (0);

		dirp = kmem_zalloc(new_dirsize * sizeof(lwp_t *), KM_SLEEP);
		lwpid_bitmap = kmem_zalloc(new_dirsize / NBBY, KM_SLEEP);
		/* 
	 	 * Since we may have slept, a slot may have been freed;
	 	 * check before using the newly allocated structures.
	 	 */
        	(void)LOCK(&procp->p_mutex, PLHI);
        	if (lwp_mapid(procp, &lwpid)) {

                	/* 
		 	 * Raced with somebody releasing an ID.
		 	 * Init the directory and release the memory allocated.
		 	 */

                	procp->p_lwpdir[lwpid - 1] = newlwpp;
			/* Release the memory we have allocated */
			kmem_free(dirp, new_dirsize * sizeof(lwp_t *));
			kmem_free(lwpid_bitmap, new_dirsize / NBBY);
                	return (lwpid); 	/* Return with p_mutex held */
        	}

		/*
	 	 * Before we use the newly allocated data structures, 
	 	 * handle the infrequent case where we raced with other
		 * LWP create operations, and what we have allocated is no
		 * longer sufficient to accomodate the existing LWP data
		 * structures of the calling process.
	 	 */

		if (new_dirsize > (size_t)procp->p_nlwpdir) {

			/* We can safely copy from the existing structures */

			dircopysize = procp->p_nlwpdir * sizeof(lwp_t *);
			bcopy(procp->p_lwpdir, dirp, dircopysize); 
			kmem_free(procp->p_lwpdir, dircopysize);
			if (procp->p_nlwpdir <= NBITPW) {
				/* The map was in p_small_lwpidmap */
				lwpid_bitmap[0] = procp->p_small_lwpidmap;
			} else {
				mapcopysize = procp->p_nlwpdir / NBBY;
				bcopy(procp->p_large_lwpidmap,
				      lwpid_bitmap, mapcopysize);
				kmem_free(procp->p_large_lwpidmap, 
					  mapcopysize);
			}

			procp->p_nlwpdir = (u_short)new_dirsize;
			procp->p_lwpdir = dirp;
			procp->p_large_lwpidmap = lwpid_bitmap;

			/* Allocate the id */
        		if (lwp_mapid(procp, &lwpid)) {
                		procp->p_lwpdir[lwpid - 1] = newlwpp;
				/*
				 * Return with p_mutex held.
				 */
                		return (lwpid);		
        		} else {
				/*
			 	 *+ The LWP directory is in an impossible
				 *+ state.  The user cannot take any corrective
				 *+ action.
			 	 */
				cmn_err(CE_PANIC, "lwp_dirslot(): bad state\n");
			}
		}

		/*
	 	 * While we were sleeping the number of LWPs in the process has
	 	 * increased so much that the data structures we have allocated
	 	 * for the directory and the map are not big enough!  Relesae
	 	 * what we have and go for the next size. 
	         * As the number of LWPs a process may have is fixed, and since
		 * by the time this routine is called, we should have staked
		 * our claim, this loop will terminate!
	 	 */

        	UNLOCK(&procp->p_mutex, PLBASE);
		kmem_free(dirp, new_dirsize * sizeof(lwp_t *));
		kmem_free(lwpid_bitmap, new_dirsize / NBBY);
	}
}


/*
 *
 * int lwp_reapid(proc_t *procp, k_lwpid_t lwpid) 
 * 	This function checks to see if the specified ID can be released
 *	for reuse.  An ID can be reused if the corresponding LWP has 
 * 	exited.  This function manages its own concurrency control.
 *
 * Calling/Exit State:
 * 	The p_mutex lock of the containing process should not be held on 
 *	entry.  Following are the return values and locking state.
 *
 *	DIR_ACTIVE: This indicates that the specified ID is active.  In this 
 *		    case the p_mutex lock of the calling context is held
 *		    on return.
 *	
 *	DIR_ESEARCH: The specified LWP ID is not known to the system.
 *		     In This case no locks are held on return.
 *
 *	DIR_EXITED:  The specified LWP has exited.  In this case the 
 *		     ID and other resources are released.  No locks are
 *		     held upon return.
 *
 *	DIR_INVAL:   The specified ID is in a non-waitable state as it is
 *		     presently detached.  No locks are held upon return.
 * 
 */
int
lwp_reapid(proc_t *procp, lwpid_t lwpid)
{
        pl_t            pl;
	lwp_t		*lwpp;
	uint_t		*lwpidmap;

	pl = LOCK(&procp->p_mutex, PLHI);

        /*
         * Check the validity of the ID passed in.
         */
        if ((lwpid < 1) || (lwpid > (lwpid_t)procp->p_nlwpdir)) {
		UNLOCK(&procp->p_mutex, pl);
                return (DIR_ESEARCH);
	}

	if ((lwpp = procp->p_lwpdir[lwpid - 1]) != NULL) {
		if (lwpp->l_flag & L_DETACHED) {
			/*
			 * The LWP is detached and so cannot be waited for.
			 */
			UNLOCK(&procp->p_mutex, pl); 
			return (DIR_INVAL);
		}
		/*
		 * The LWP exists, is not detached, and is not a zombie.
		 * Return with p_mutex held.
		 */
		return (DIR_ACTIVE);
	}

	if (procp->p_nlwpdir <= NBITPW)
		lwpidmap = &procp->p_small_lwpidmap;
	else
		lwpidmap = procp->p_large_lwpidmap;

	if (BITMASKN_TEST1(lwpidmap, lwpid - 1)) {
		/*
		 * The LWP exists as a zombie.
		 * Reap it.
		 */
		BITMASKN_CLR1(lwpidmap, lwpid - 1);
		freelwp(procp);
		UNLOCK(&procp->p_mutex, pl); 
		return (DIR_EXITED);
	}

	/*
	 * The LWP does not exist at all, in any state.
	 */
	UNLOCK(&procp->p_mutex, pl); 
	return (DIR_ESEARCH);
}

		
/*
 *
 * int lwp_reapany(proc_t *procp, lwpid_t *idp)
 * 	The lwp_reapany() function checks all the LWPs within a process
 *	for zombies. 
 *
 * Calling/Exit State: 
 *	The p_mutex lock of the containing process cannot be held on entry. 
 *	Following are the return values (and lock states):
 *
 *      DIR_ACTIVE: This indicates that all LWPs in the process are active.
 *		    In this case, the p_mutex lock of the calling context is
 *		    held on return.
 *
 *      DIR_EXITED: A zombie LWP has been found and has been cleaned up.
 *		    The ID of the zombie is returned in the out parameter.
 *		    In this case no locks are held upon return. 
 *
 *      DIR_INVAL:  The calling context is the only context in the process. 
 *                  No locks are held upon return.
 */
int
lwp_reapany(proc_t *procp, lwpid_t *idp)
{
	int		idx, lwpid;
	int		base_lwpid;
	uint_t		lwpidmap_word;
	uint_t		*lwpidmap;
	uint_t		nlwpidmap_words;
	boolean_t	deadlock = B_TRUE;
	pl_t		pl;

	base_lwpid = 1;
	pl = LOCK(&procp->p_mutex, PLHI);
	nlwpidmap_words = BITMASK_NWORDS((uint_t)procp->p_nlwpdir);
	if (nlwpidmap_words <= 1)
		lwpidmap = &procp->p_small_lwpidmap;
	else
		lwpidmap = procp->p_large_lwpidmap;
	do {
		lwpidmap_word = *lwpidmap++;
		while ((idx = BITMASK1_FFSCLR(&lwpidmap_word)) != -1) {
			lwpid = base_lwpid + idx;
			if ((procp->p_lwpdir[lwpid-1]) == NULL) {
				/*
				 * Reap the found zombie LWP.
				 * Clear the LWP-ID bit in the LWP-ID bitmap.
				 */
				BITMASK1_CLR1(lwpidmap - 1, idx);
				freelwp(procp);
				UNLOCK(&procp->p_mutex, pl);
				*idp = (lwpid_t)lwpid;
				return (DIR_EXITED);
			}
			if ((deadlock) &&
			    (lwpid != u.u_lwpp->l_lwpid)) {
				deadlock = B_FALSE;
			}
		}
		base_lwpid += NBITPW;
	} while (--nlwpidmap_words != 0);

	/* No zombies found; check if we are deadlocking */

	if (deadlock) {
		UNLOCK(&procp->p_mutex, pl);
		return (DIR_INVAL);
	}

	/* All ID's active; return with p_mutex held */
	return (DIR_ACTIVE);
}

		
/*
 *
 * boolean_t lwp_mapid(proc_t *procp, k_lwpid_t *idp)
 *	Allocate an available LWP-ID from the process LWP-ID bit map.
 *
 * Calling/Exit State:
 *	The p_mutex lock of the process containing the calling LWP must be
 *	locked upon entry.  This lock remains held upon return.
 *	If a free LWP-ID is found, it will be marked as busy and returned at
 *	(*idp) with this function returning B_TRUE.  Otherwise, B_FALSE
 *	is returned to indicate that no free LWP-IDs exist in the current
 *	LWP-ID bit map.
 */
STATIC boolean_t
lwp_mapid(proc_t *procp, k_lwpid_t *idp)
{
	uint_t		*map;
	int		retval;

	/* Check the entry condition */

	ASSERT(LOCK_OWNED(&procp->p_mutex));
	
	/*
	 * Check the specified map for the first bit that is 0; set retval
	 * to the bit number, and set the bit.  If no bit was 0, set retval
	 * to -1.
	 */
	if (procp->p_nlwpdir <= NBITPW) {
		map = &procp->p_small_lwpidmap;
		retval = BITMASK1_FFCSET(map);
	} else {
		map = procp->p_large_lwpidmap;
		retval = BITMASKN_FFCSET(map,
				BITMASK_NWORDS((uint_t)procp->p_nlwpdir));
	}

	if (retval == -1)
		return (B_FALSE);

	*idp = (k_lwpid_t) retval + 1;
	return (B_TRUE);
}


/*
 * void freelwp(proc_t *pp)
 *      This function coordinates the final destruction of the
 *      LWP with the per-user quotas mechanism.
 *
 * Calling/Exit State:
 *      The p_mutex lock of the containing process must be held on entry.
 *	This lock remains held upon return.
 *
 * Remarks:
 *	The p_mutex lock must be held by the caller to prevent races with
 *	a new LWP being created with the same LWP-ID.  In addition, p_mutex
 *	must be held to get a stable reading of the current uidquota
 *	object assigned to the process.
 */
void
freelwp(proc_t *p)
{
	static uidres_t uidresobject;

	ASSERT(LOCK_OWNED(&p->p_mutex));
	/*
	 * Decrement the LWP count maintained in the uidquota structure.
	 */
	uidresobject.ur_lwpcnt = 1;
	uidquota_decr(p->p_uidquotap, &uidresobject);
	MET_LWP_INUSE(-1);
	/*
	 * Decrement the total LWP count. This is the count of active LWPs
	 * plus the number of waitable LWPs that have exited, but have not been 
	 * waited for.
	 */
	--p->p_ntotallwp;
}

/*
 * void lwp_attrupdate(void)
 *	Update any out-of-date LWP versions of the shared process attributes,
 *	as well as handle delayed LWP attribute updates for the calling LWP.
 *
 * Calling/Exit State:
 *	No spin locks held upon entry or exit.
 */
void
lwp_attrupdate(void)
{
	lwp_t		*lwpp = u.u_lwpp;	/* calling LWP */
	proc_t		*p = u.u_procp;		/* calling process */
	cred_t		*oldcrp = NULL;
	vnode_t		*oldrvp = NULL;
	rlimits_t	*oldrlp = NULL;
	pl_t		pl;

	if (lwpp->l_trapevf & EVF_PL_CRED) {	/* credentials updated */
		oldcrp = lwpp->l_cred;

		pl = LOCK(&p->p_mutex, PLHI);
		(void)LOCK(&lwpp->l_mutex, PLHI);
		lwpp->l_trapevf &= ~EVF_PL_CRED;
		UNLOCK(&lwpp->l_mutex, PLHI);

		crhold(p->p_cred);
		lwpp->l_cred = p->p_cred;

		UNLOCK(&p->p_mutex, pl);
	}
	if (lwpp->l_trapevf & EVF_PL_RDIR) {	/* root directory updated */
		oldrvp = lwpp->l_rdir;

		pl = LOCK(&p->p_mutex, PLHI);
		(void)LOCK(&lwpp->l_mutex, PLHI);
		lwpp->l_trapevf &= ~EVF_PL_RDIR;
		UNLOCK(&lwpp->l_mutex, PLHI);
		UNLOCK(&p->p_mutex, pl);

		pl = CUR_ROOT_DIR_LOCK(p);
		VN_HOLD(p->p_rdir);
		lwpp->l_rdir = p->p_rdir;
 		if (lwpp->l_auditp) {
 			adtpath_t *rdp = lwpp->l_auditp->al_rdp;
 			if (p->p_auditp->a_rdp) {
 				lwpp->l_auditp->al_rdp = p->p_auditp->a_rdp;
 				p->p_auditp->a_rdp->a_ref++;
 			} else
 				lwpp->l_auditp->al_rdp = NULL;
 			if (rdp)
				CPATH_FREE(p->p_auditp, rdp, 1);
 		}
		CUR_ROOT_DIR_UNLOCK(p, pl);
	}
	if (lwpp->l_trapevf & EVF_PL_RLIMIT) {	/* resource limits updated */
		oldrlp = u.u_rlimits;

		pl = LOCK(&p->p_mutex, PLHI);
		(void)LOCK(&lwpp->l_mutex, PLHI);
		lwpp->l_trapevf &= ~EVF_PL_RLIMIT;
		UNLOCK(&lwpp->l_mutex, PLHI);

		rlhold(p->p_rlimits);
		u.u_rlimits = p->p_rlimits;

		UNLOCK(&p->p_mutex, pl);
	}

	/* Check for scheduling parameters update */
	if (lwpp->l_trapevf & EVF_L_SCHEDPARM) {
		parmsret();			/* clears EVF_L_SCHEDPARM */
	}

	/*
	 * Without holding any locks, free up the old copies of the common
	 * process attributes.
	 */
	if (oldcrp != NULL) {
		crfree(oldcrp);
	}
	if (oldrvp != NULL) {
		VN_RELE(oldrvp);
	}
	if (oldrlp != NULL) {
		rlfree(oldrlp);
	}
}

/*
 * void complete_lwpcreate(void)
 *	Do the final processing for a user level lwp create.
 *
 * Calling/Exit State:
 *	No locks can be held on entry.
 */
void
complete_lwpcreate(void)
{
	lwp_t	*lwpp = u.u_lwpp;
	rval_t	rval;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(lwpp->l_ucp != NULL);

	u.u_privatedatap = lwpp->l_ucp->uc_privatedatap;
	restorecontext(lwpp->l_ucp);
	(void)lwp_setprivate(lwpp->l_ucp->uc_privatedatap);
	kmem_free(lwpp->l_ucp, sizeof(ucontext_t));
	lwpp->l_ucp = NULL;
	/*
	 * systrap_cleanup will overwrite return value registers
	 * with r_val1 and r_val2.  Fix them so that the user really
	 * gets the context that was specified.
	 */
	fix_retval(&rval);
	systrap_cleanup(&rval, SYS_lwpcreate, 0);
}


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_lwp(const lwp_t *lwpp)
 * 	Formatted dump of an LWP structure
 *
 * Calling/Exit State:
 * 	None. I'll try to dump whatever you pass to me.
 */
void
print_lwp(const lwp_t *lwpp)
{
	debug_printf("LWP structure dump at 0x%x (proc 0x%x,%d):\n\n",
		     lwpp, lwpp->l_procp, lwpp->l_lwpid);

	debug_printf("l_stat=0x%x l_flag=0x%x l_trapevf=0x%x\n",
		     lwpp->l_stat, lwpp->l_flag, lwpp->l_trapevf);
	debug_printf("l_up=0x%x &l_ubinfo=0x%x l_ucp=0x%x\n",
		     lwpp->l_up, &lwpp->l_ubinfo, lwpp->l_ucp);
	debug_printf("l_cred=0x%x l_rdir=0x%x\n",
		     lwpp->l_cred, lwpp->l_rdir);
	debug_printf("l_realtimer=0x%x l_artid=%d l_rtid=%d l_clktim=0x%x\n",
		     lwpp->l_realtimer, lwpp->l_artid, lwpp->l_rtid,
		     lwpp->l_clktim);
	debug_printf("l_utime=%d l_stime=%d l_start=%d l_slptime=%d\n",
		     lwpp->l_utime, lwpp->l_stime, lwpp->l_start,
		     lwpp->l_slptime);
	debug_printf("l_pri=%d l_origpri=%d l_cid=%d, l_cllwpp=0x%x\n",
		     lwpp->l_pri, lwpp->l_origpri, lwpp->l_cid,
		     lwpp->l_cllwpp);
	debug_printf("l_clfuncs=0x%x l_qparmsp=0x%x l_rq=0x%x l_eng=0x%x\n",
		     lwpp->l_clfuncs, lwpp->l_qparmsp, lwpp->l_rq,
		     lwpp->l_eng);
	debug_printf("l_kbind=0x%x l_xbind=0x%x l_ubind=0x%x l_lastran=%d\n",
		     lwpp->l_kbind, lwpp->l_xbind, lwpp->l_ubind,
		     lwpp->l_lastran);
	debug_printf("l_ublocktype=%d l_sq=0x%x l_boost=0x%x l_waiterp=0x%x\n",
		     lwpp->l_ublocktype, lwpp->l_sq, lwpp->l_boost,
		     lwpp->l_waiterp);
	debug_printf("l_pollflag=%d &l_pollevent=0x%x"
		      " l_whystop=%d l_whatstop=%d\n",
		     lwpp->l_pollflag, &lwpp->l_pollevent,
		     lwpp->l_whystop, lwpp->l_whatstop);
	debug_printf("l_trace=0x%x l_curflt=%d l_sigwait=%d l_cursig=%d\n",
		     lwpp->l_trace, lwpp->l_curflt, lwpp->l_sigwait,
		     lwpp->l_cursig);
	debug_printf("&l_cursigst=0x%x &l_sigs=0x%x l_siginfo=0x%x\n",
		     &lwpp->l_cursigst, lwpp->l_sigs, lwpp->l_siginfo);
	debug_printf("l_sep=0x%x &l_slpevent=0x%x l_notrt=%d l_beingpt=%d\n",
		     lwpp->l_sep, &lwpp->l_slpevent, lwpp->l_notrt,
		     lwpp->l_beingpt);
	debug_printf("\n");
}

#endif /* DEBUG || DEBUG_TOOLS */
