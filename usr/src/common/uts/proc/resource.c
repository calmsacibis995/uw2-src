/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/resource.c	1.7"

#include <util/types.h>
#include <util/ksynch.h>
#include <util/debug.h>
#include <mem/kmem.h>
#include <proc/user.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/resource.h>
#include <acc/priv/privilege.h>
#include <svc/errno.h>

/*
 * The following global variable is used to reference the system resource
 * limits used when some process (e.g., a setuid process) must run without
 * being encumbered with the resource limits of the process that exec'ed it.
 */
struct rlimits *sys_rlimits;

/*
 *
 * rlimits_t *rlget()
 *	Allocates a new rlimits structure with a reference count of one.
 *
 * Calling/Exit State:
 *	No parameters are given to this function.  Upon completion,
 *	a pointer to the new rlimits structure is returned.
 *
 * Remarks:
 *	This function can block to allocate the new rlimits structure.
 *	Hence, the caller must not hold any spin locks when calling this
 *	function.
 *
 */
rlimits_t *
rlget()
{
	register rlimits_t *rlimitsp;

	rlimitsp = (rlimits_t *)kmem_zalloc(sizeof(rlimits_t), KM_SLEEP);
	FSPIN_INIT(&rlimitsp->rl_mutex);
	rlimitsp->rl_ref = 1;
	return rlimitsp;
}


/*
 *
 * void rlholdn(rlimits_t *rlimitsp, u_int holdcount)
 *	Increment the reference count on the indicated rlimits
 *	by the specified number.
 *
 * Calling/Exit State:
 *	The rlimitsp pointer and the unsigned holdcount parameters
 *	respectively identify the rlimits to be held and reference
 *	count increment.
 *
 */
void
rlholdn(register rlimits_t *rlimitsp, u_int holdcount)
{
	FSPIN_LOCK(&rlimitsp->rl_mutex);
	rlimitsp->rl_ref += holdcount;
	FSPIN_UNLOCK(&rlimitsp->rl_mutex);
}


/*
 *
 * void rlfreen(rlimits_t *rlimitsp, u_int derefcount)
 *	Release the specified number of references against the given
 *	rlimits structure.
 *
 * Calling/Exit State:
 *	The rlimitsp pointer and the unsigned holdcount parameters
 *	respectively identify the rlimits to be held and reference
 *	count decrement.
 *
 */
void
rlfreen(register rlimits_t *rlimitsp, u_int derefcount)
{
	FSPIN_LOCK(&rlimitsp->rl_mutex);
	if ((rlimitsp->rl_ref -= derefcount) > 0) {
		FSPIN_UNLOCK(&rlimitsp->rl_mutex);
	} else {
		FSPIN_UNLOCK(&rlimitsp->rl_mutex);
		kmem_free((void *)rlimitsp, sizeof(rlimits_t));
	}
}


/*
 *
 * void rlinstall(rlimits_t *newrlimitsp)
 *	Install the given resource limits object as the new resource
 *	limits for the calling LWP and process.
 *
 * Calling/Exit State:
 *	The caller must have already established the additional references
 *	needed against the given resource limits object.  Upon return, the
 *	given resource limits object is installed as the resource limits
 *	of both the calling LWP and process.  All other LWPs in the process
 *	upon completion, have the EVF_PL_RLIMIT trap event flag pending,
 *	alerting them of the need to update their own copy of the resource
 *	limits as soon as possible.
 *
 * Remarks:
 *	As part of the installation process, the references to the old
 *	credentials from both the calling LWP and process are discarded
 *	by this function.
 *
 */
void
rlinstall(rlimits_t *newrlimitsp)
{
	register proc_t *procp;			/* u.u_procp */
	rlimits_t *oldproc_rlimitsp;		/* old rlimits of the process */
	rlimits_t *oldlwp_rlimitsp;		/* old rlimits of the LWP */
	pl_t pl;

	oldlwp_rlimitsp = u.u_rlimits;
	u.u_rlimits = newrlimitsp;
	procp = u.u_procp;
	pl = LOCK(&procp->p_mutex, PLHI);
	oldproc_rlimitsp = procp->p_rlimits;
	procp->p_rlimits = newrlimitsp;
	if (!SINGLE_THREADED())
		trapevproc(procp, EVF_PL_RLIMIT, B_FALSE);
	UNLOCK(&procp->p_mutex, pl);
	if (oldlwp_rlimitsp == oldproc_rlimitsp) {
		rlfreen(oldproc_rlimitsp, 2);
	} else {
		rlfree(oldproc_rlimitsp);
		rlfree(oldlwp_rlimitsp);
	}
}

/*
 * int rlimit(int resource, rlim_t softlimit, rlim_t hardlimit)
 *	Support routine for ulimit(2)/setrlimit(2) system calls.
 *	Set the soft and hard limits of the specified resource
 *	for the calling process to the indicated values.
 *
 * Calling/Exit State:
 *	There are no special requirements of the caller; simply call
 *	this function to effect the change.  The caller must however
 *	be prepared to block as this function allocates a new resource
 *	limits object.
 *
 *	If successful, zero is returned and the new resource limits are
 *	installed for the calling LWP, and for the process.
 *	Otherwise, the resource limits are unchanged, and the value
 *	returned is the errno value identifying the cause of the failure.
 *
 * Description:
 *	This function operates by first verifying the new limits for
 *	the given resource.  If the new limits differ from the current
 *	limits, a new resource object is allocated that is a direct
 *	copy of the calling LWP's resource limits.  This object is then
 *	modified to reflect the new limits for the designated resource,
 *	and is installed as the resource limits of both the calling LWP
 *	and the process.  All other LWPs in the process have the 
 *	EVF_PL_RLIMIT trap event flag pending, alerting them of the
 *	need to update their own independent copy of the resource limits.
 *
 */
int
rlimit(int resource, rlim_t softlimit, rlim_t hardlimit)
{
	register rlimits_t *newrlimitsp;	/* new rlimits of the process */
	struct rlimit *rlimitp;			/* the selected rlimit */
	int err;

	ASSERT(KS_HOLD0LOCKS());

	if (softlimit > hardlimit)
		return EINVAL;

	rlimitp = &u.u_rlimits->rl_limits[resource];

	if (hardlimit > rlimitp->rlim_max &&
	    pm_denied(CRED(), P_SYSOPS))
		return EPERM;

	if (rlimitp->rlim_cur != softlimit || rlimitp->rlim_max != hardlimit) {
		if (resource == RLIMIT_STACK && 
				rlimitp->rlim_cur != softlimit) {
			err = rlimit_stack(rlimitp->rlim_cur, softlimit);
			if (err != 0)
				return err;
		}

		newrlimitsp = (rlimits_t *)
			       kmem_alloc(sizeof(rlimits_t), KM_SLEEP);
		*newrlimitsp = *u.u_rlimits;		/* struct assignment */
		FSPIN_INIT(&newrlimitsp->rl_mutex);
		/*
		 * Overwrite the reference count from the structure assignment.
		 * This count is set to 2 (for the process and LWP references).
		 */
		newrlimitsp->rl_ref = 2;
		newrlimitsp->rl_limits[resource].rlim_cur = softlimit;
		newrlimitsp->rl_limits[resource].rlim_max = hardlimit;
		rlinstall(newrlimitsp);		/* install new rlimits */
	}

	return 0;
}
