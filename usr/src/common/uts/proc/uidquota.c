/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/uidquota.c	1.18"
#ident	"$Header: $"

#include <util/types.h>
#include <util/ksynch.h>
#include <proc/uidquota.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <acc/priv/privilege.h>
#include <util/var.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/inline.h>
#include <mem/kmem.h>
#include <proc/proc_hier.h>
#ifdef CC_PARTIAL
#include <acc/mac/covert.h>
#include <acc/mac/cc_count.h>
#endif

STATIC	uidhash_t	*uidquotahash;

/*
 *+ ui_mutex is a spin lock governing a particular hash chain as well as the 
 *+ contents of the uidquota objects in the hash chain.
 */

STATIC  LKINFO_DECL(uidlkinfo, "PP::ui_mutex", 0);
STATIC  u_long		uid_hashsiz = 32;
#define UIDHASH(uid)    (&uidquotahash[((uid)&(uid_hashsiz-1))])
#define SCALE_FACTOR 10


/*
 * void uidquota_init(void)
 *	Initializes the uidquota management system.
 *
 * Calling/Exit State:
 *	Upon completion the uidquota system is initialized. 
 *
 * Remarks:
 *	This function must be called prior to invoking any of the other
 *	functions exported by this module.
 *
 */
void
uidquota_init(void)
{
	int i;

	/*
	 * In the present design we have chosen to go with a single lock
	 * (ui_mutex) to protect both the hash chain as well as the objects on the 
	 * hash chain. To minimize contention on this lock we must choose a hash
	 * size that is sufficiently large. We have chosen to make the hash size
	 * a function of nprocs which provides an upper bound on the number of 
	 * users a system can concurrently support. This solution is expected
	 * to scale well. 
	 */

	while (uid_hashsiz < v.v_proc/SCALE_FACTOR) { 
		uid_hashsiz *= 2;
	}


	/* Allocate the hash table */

	uidquotahash = (uidhash_t *)
		kmem_alloc(sizeof(uidhash_t)*uid_hashsiz, KM_NOSLEEP);
	if (uidquotahash == NULL) {
		/*
		 *+ Insufficient memory: could not allocate uid tables. 
		 *+ Try configuring a smaller kernel! 
		 */
		cmn_err(CE_PANIC, "No memory to  allocate uid tables\n");
	}

	/* Initialize the array */

	for (i = 0; i < uid_hashsiz; i++) {
		LOCK_INIT(&uidquotahash[i].ui_mutex,
			  UID_HIER, UID_MINIPL, &uidlkinfo, KM_NOSLEEP);
		uidquotahash[i].ui_link = NULL;
	}
}

/*
 * uidquo_t *uidquota_get(uid_t uid) 
 *	The uidquota_get() function returns a pointer to a held uidquotas
 *	structure that corresponds to the specified user-ID.
 *
 * Calling/Exit State:
 *	No locks held on entry and no locks held on return. This function can 
 *	sleep.
 */

uidquo_t *
uidquota_get(uid_t uid)
{
	uidhash_t *uidhashp;
	uidquo_t *uidquotap;
	uidquo_t *tmp;
	pl_t pl;

	/* Get the corresponding hash anchor */
	uidhashp = (uidhash_t *)UIDHASH(uid);
	pl = LOCK(&uidhashp->ui_mutex, PL_UID);
	uidquotap = uidhashp->ui_link;
	while (uidquotap != NULL) {
		if (uidquotap->uq_uid == uid) {		/* found it! */
			uidquotap->uq_ref++;
			UNLOCK(&uidhashp->ui_mutex, pl);
			return uidquotap;
		}
		uidquotap = uidquotap->uq_link;
	}

	/*
	 * The requested uid is not currently active; allocate one;
	 * drop the hash bucket lock as we may sleep.
	 */
	UNLOCK(&uidhashp->ui_mutex, pl);

	tmp = kmem_alloc(sizeof(uidquo_t), KM_SLEEP);

	/*
	 * Since we dropped the hash bucket lock; another context
	 * may have beat us to the punch!  Need to search the list 
	 * again before we can link up the newly allocated uidquotas
	 * structure.
	 */
	pl = LOCK(&uidhashp->ui_mutex, PL_UID);
	uidquotap = uidhashp->ui_link;
	while (uidquotap != NULL) {
	       if (uidquotap->uq_uid == uid) {	/* lost the race */
			uidquotap->uq_ref++;
			UNLOCK(&uidhashp->ui_mutex, pl);

			/* get rid of the one we allocated */
			kmem_free(tmp, sizeof(*tmp));
			return uidquotap;
		}
		uidquotap = uidquotap->uq_link;
	}

	/* Link up the one we created */

	tmp->uq_link = uidhashp->ui_link;
	uidhashp->ui_link = tmp;
	tmp->uq_uid = uid;
	tmp->uq_ref = 1;	/* hold a reference */
	tmp->uq_lwpcnt = 0;
	UNLOCK(&uidhashp->ui_mutex, pl);
	return tmp;
}

/*
 * boolean_t uidquota_incr(uidquo_t *uidp, uidres_t *uidresp , boolean_t flag) 
 *	The uidquota_incr() function updates the state of the given
 *	uidquotas structure, and depending on an input parameter will enforce
 *	per-uid quotas. If flag is B_TRUE, updates are made without checking 
 *	the quotas and if flag is B_FALSE, updates are made only if all 
 *	quota checks are satisfied. This option is provided to handle the 
 *	setuid case.
 *
 * Calling/Exit State: 
 *	uidresp must point to an initialized uidres_t object.
 *	uidp must point to the process's uidquo_t object or NULL;
 *	NULL indicates a process not subject to quotas.
 *
 *	The function returns B_TRUE if the specified uidquota object was
 *	updated successfully with the values in the uidres_t object. The
 *	function returns B_FALSE if any of the requested updates would have
 *	violated the quotas. In this case the state of the uidquota object is
 *	not modified. Note that the caller should ensure that the specified
 *	uidquota object is not deallocated when this function is called. 
 *
 * Remarks: 
 *	It is expected that in the future there will be more resources that
 *	may be monitored on a per-uid basis. Hence, we pass a pointer
 *	to a structure of resource counts that the caller wants. This structure 
 *	can be grown to accommodate new resources of interest. Presently, 
 *	we will track number of processes and number of LWPs on a per-uid
 *	basis. 
 *
 *	The clients of this and the uidquota_decr() interfaces should
 * 	allocate a uidres_t object (this would be on the stack), initialize
 * 	all the relevant fields and pass a pointer to this object down the 
 *	interface. By zeroing out the structure prior to initialization, the 
 *	calling code need not change when new resources are added to the 
 *	uidres_t object. 
 */

boolean_t
uidquota_incr(uidquo_t *uidp, uidres_t *uidresp, boolean_t flag)
{
	uidhash_t *uidhashp;
	pl_t pl;

	if (uidp == NULL) {
		/* The process does not affect or check UID quotas. */
		return B_TRUE;
	}

	uidhashp = (uidhash_t *)UIDHASH(uidp->uq_uid);
	pl = LOCK(&uidhashp->ui_mutex, PL_UID);

	/* handle the simple setuid case first */
	if (flag) {
		uidp->uq_ref += uidresp->ur_prcnt;
		uidp->uq_lwpcnt += uidresp->ur_lwpcnt;
		UNLOCK(&uidhashp->ui_mutex, pl);
		return B_TRUE;
	}

	uidp->uq_lwpcnt += uidresp->ur_lwpcnt;
	uidp->uq_ref += uidresp->ur_prcnt;

	/*
	 * If the per-user process limit has been exceeded, and the
	 * process is non-privileged, return failure.
	 */
	if (((uidp->uq_lwpcnt - uidp->uq_ref) > v.v_maxulwp ||
	     uidp->uq_ref > v.v_maxup) &&
	    pm_denied(CRED(), P_SYSOPS)) {

		/* 
		 * some limit was violated. Reset state. 
		 */ 
		uidp->uq_ref -= uidresp->ur_prcnt;
		uidp->uq_lwpcnt -= uidresp->ur_lwpcnt;
		UNLOCK(&uidhashp->ui_mutex, pl);

#ifdef CC_PARTIAL
		/*
		 * call the covert channel generic limiter.
		 */ 
		CC_COUNT(CC_RE_PROC, CCBITS_RE_PROC);
#endif

		return B_FALSE;
	}

	UNLOCK(&uidhashp->ui_mutex, pl);
	return B_TRUE;
}

/*
 * void uidquota_decr(uidquo_t *uidp, uidres_t uidresp)
 *	The uidquota_decr() function modifies the state of the
 *	designated uidquotas structure as directed, and releases the
 *	structure if no references remain.
 *
 * Calling/Exit State:
 *	uidresp must point to an initialized uidres_t object.
 *	uidp must point to the process's uidquo_t object or NULL;
 *	NULL indicates a process not subject to quotas.
 *
 *	The function does not sleep.
 *
 * Remarks: 
 *	Refer to the remarks under uidquota_incr() interface.
 *
 */

void
uidquota_decr(uidquo_t *uidp, uidres_t *uidresp)
{
	uidhash_t *uidhashp;
	uidquo_t *walkp;
	pl_t pl;

	if (uidp == NULL) {
		/* The process does not affect or check UID quotas. */
		return;
	}

	uidhashp = (uidhash_t *)UIDHASH(uidp->uq_uid);
	pl = LOCK(&uidhashp->ui_mutex, PL_UID);
	uidp->uq_lwpcnt -= uidresp->ur_lwpcnt;
	uidp->uq_ref -= uidresp->ur_prcnt;
	if (uidp->uq_ref > 0) {
		UNLOCK(&uidhashp->ui_mutex, pl);
	} else {
		/* Free up the quota object */
                walkp = uidhashp->ui_link;
                if (walkp == uidp) {            /* simple case */
                	uidhashp->ui_link = uidp->uq_link;
                } else {                        /* need to walk list */
                        while (walkp->uq_link != uidp) {
                        	walkp = walkp->uq_link;
                        }
                        walkp->uq_link = uidp->uq_link;
                }
                UNLOCK(&uidhashp->ui_mutex, pl);
                kmem_free(uidp, sizeof(*uidp));
        }
}
