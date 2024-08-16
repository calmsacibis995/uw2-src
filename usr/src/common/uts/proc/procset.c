/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/procset.c	1.18"

#include <util/types.h>
#include <util/param.h>
#include <proc/lwp.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <proc/cred.h>
#include <proc/procset.h>
#include <proc/session.h>
#include <proc/proc_hier.h>
#include <fs/vnode.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>


/*
 * Set procset_t object (tps) to the reverse arguments of the
 * procset_t object (psp).
 */
#define	SWITCH_ARGS(psp, tps) \
{ \
	(tps).p_lid = (psp)->p_rid; \
	(tps).p_rid = (psp)->p_lid; \
	(tps).p_ridtype = (psp)->p_lidtype; \
	(tps).p_lidtype = (psp)->p_ridtype; \
	(tps).p_op = (psp)->p_op; \
}


/*
 *
 * STATIC boolean_t opinset(idtype_t idtype, id_t idval, proc_t *pp)
 *	Return B_TRUE if the ID-type attribute (idtype) of the process (pp)
 *	matches the given ID value (idval).
 *
 * Calling/Exit State:
 *    Locking rules:
 *	The p_mutex lock of the target process must be held by the caller
 *	--unless the attribute being checked is one of P_PID, P_PGID, P_SID,
 *	or P_ALL, in which case the caller can instead elect to hold the
 *	appropriate session locked to guarantee stability of the attribute
 *	and/or process existence.  In any case however, all locks held upon
 *	entry remain held upon return.
 *
 *    Return value:
 * 	B_TRUE if the ID-type attribute (idtype) of the process (pp) matches
 *	  the given ID value (idval).
 *	B_FALSE Otherwise.
 *
 * Remarks:
 * 	This function expects to be called with a valid ID-type and ID
 *	value.  The original procset_t from which these parameters are
 *	derived should be checked before calling this function.  In
 *	addition, it is assumed that the target process is not a zombie
 *	process (holding p_mutex and/or s_mutex prevents the target process
 *	from becoming one).
 *
 */
STATIC boolean_t
opinset(idtype_t idtype, id_t idval, proc_t *pp)
{
	register lwp_t *lwpp;
	
	ASSERT(pp->p_nlwp > 0);		/* must not be a zombie */

	switch (idtype) {
	case P_PID:
		return pp->p_pidp->pid_id == idval;

	case P_PPID:
		ASSERT(LOCK_OWNED(&pp->p_mutex));
		return pp->p_ppid == idval;

	case P_PGID:
		return pp->p_pgid == idval;

	case P_SID:
		return pp->p_sid == idval;

	case P_UID:
		ASSERT(LOCK_OWNED(&pp->p_mutex));
		return pp->p_cred->cr_uid == idval;

	case P_GID:
		ASSERT(LOCK_OWNED(&pp->p_mutex));
		return pp->p_cred->cr_gid == idval;

	case P_CID:
		ASSERT(LOCK_OWNED(&pp->p_mutex));
		lwpp = pp->p_lwpp;
		do {
			/*
			 * NOTE: l_cid is only protected by l_mutex, whilst
			 * we hold only p_mutex here.  Thus, it is possible
			 * for races to occur here.  However, holding p_mutex
			 * will prevent the given LWP from exiting.  Hence,
			 * the race has been deemed harmless and therefore
			 * acceptable....
			 */
			if (lwpp->l_cid == idval) 
				return (B_TRUE);
		} while ((lwpp = lwpp->l_next) != NULL);
		break;
	case P_ALL:
		return B_TRUE;
	default:
		/*
		 *+ A bad ridtype argument was passed to the opinset()
		 *+ function.
		 */
		cmn_err(CE_WARN, "opinset() called with bad ridtype");
	}

	return B_FALSE;
}


/* 
 *
 * STATIC boolean_t procinset(procset_t *psp, proc_t *pp)
 * 	Verify that the non-zombie, non-system process (pp), is in the
 *	process set specified by (psp).
 *
 * Calling/Exit State:
 * 	Locking rules:
 * 	   The p_mutex lock of the target process must be held by the caller.
 *	   This lock remains held upon return.
 * 	Return value:
 * 	   B_TRUE if the process is in specified set.
 *	   B_FALSE Otherwise.
 *
 * Remarks:
 * 	This function expects to be called with a valid procset_t.
 * 	The set should be checked before calling this function.
 *	The process is assumed to be a non-zombie, non-system process.
 *
 */  
STATIC boolean_t
procinset(register procset_t *psp, register proc_t *pp)
{
	ASSERT(LOCK_OWNED(&pp->p_mutex));

	switch (psp->p_op) {
	case POP_DIFF:
		return (opinset(psp->p_lidtype, psp->p_lid, pp) &&
		        !opinset(psp->p_ridtype, psp->p_rid, pp));

	case POP_AND:
		return (opinset(psp->p_lidtype, psp->p_lid, pp) &&
			opinset(psp->p_ridtype, psp->p_rid, pp));

	case POP_OR:
		return (opinset(psp->p_lidtype, psp->p_lid, pp) ||
			opinset(psp->p_ridtype, psp->p_rid, pp));

	case POP_XOR:
		return (opinset(psp->p_lidtype, psp->p_lid, pp) !=
			opinset(psp->p_ridtype, psp->p_rid, pp));

	default:
		/*
		 *+ A bad optype argument was passed to the procinset()
		 *+ function.
		 */
		cmn_err(CE_WARN, "procinset() called with bad set op");
		break;
	}

	return B_FALSE;
}


/* 
 *
 * STATIC id_t getmyid(idtype_t idtype)
 *	Returns the ID of the calling LWP based on the idtype.
 *
 * Calling/Exit State:
 * 	This routine returns the id value based on the parameter: idtype.
 *	It assumes that the idtype parameter is a valid type.
 *
 * Remarks:
 *	This routine relies upon long integer-sized memory read operations
 *	being "atomic."  If this assumption is not true for a particular
 *	architecture, then the p_mutex lock of the process should be
 *	acquired and released by this function to read the appropriate ID
 *	value.
 *
 *	Also, although the value of the ID sampled here remains constant
 *	during the execution of the system call, the original "source" ID
 *	could still change during the system call.  For example, the caller
 *	could specify P_SID in which case the session-ID would be sampled
 *	here, but the session-ID of the caller could be subsequently changed
 *	during the execution of the system call by another LWP in the
 *	calling process racing with the caller.  Attempts to prevent such
 *	situations incur overhead, and can (in some cases), lead to lock
 *	hierarchy troubles.
 *
 */
STATIC id_t
getmyid(idtype_t idtype)
{
	register proc_t	*pp;

	pp = u.u_procp;

	switch (idtype) {
	case P_PID:
		return pp->p_pidp->pid_id;
	
	case P_PPID:
		return pp->p_ppid;

	case P_PGID:
		return pp->p_pgid;

	case P_SID:
		return pp->p_sid;

	case P_CID:
		return u.u_lwpp->l_cid;

	case P_UID:
		return u.u_lwpp->l_cred->cr_uid;

	case P_GID:
		return u.u_lwpp->l_cred->cr_gid;

	case P_ALL:
		/*
		 * We shouldn't get called for the P_ALL case, but if we do,
		 * set the ID value to 0 so that checkprocset() detects
		 * trivial rejection cases correctly.
		 */
		return 0;

	case P_LWPID:
		return u.u_lwpp->l_lwpid;

	default:
		/*
		 *+ A bad idtype argument was passed to the getmyid()
		 *+ function.
		 */
		cmn_err(CE_WARN, "bad idtype passed to getmyid()\n");
		break;
	}

	return 0;
}


/*
 *
 * STATIC int checklwpsetid(idtype_t idtype, id_t *idp)
 *	Validate the procset_t ID-type and value as respectively given
 *	in idtype, and *idp, when the procset_t argument is used to
 *	select LWPs.
 *
 * Calling/Exit State:
 *    Parameters:
 *	The *idp value will be modified if it was originally equal to P_MYID,
 *	and the ID-type in idtype is valid.  In addition, *idp is set to zero
 *	(0) when the ID-type is P_ALL.
 *    Returns:
 *	0:	if the ID-type and value are valid.
 *	EINVAL:	if the ID-type is invalid.
 *
 */
STATIC int
checklwpsetid(register idtype_t idtype, register id_t *idp)
{
	switch (idtype) {
	case P_PID:
	case P_PPID:
	case P_PGID:
	case P_SID:
	case P_CID:
	case P_UID:
	case P_GID:
	case P_LWPID:
		if (*idp == P_MYID) {
			*idp = getmyid(idtype);
		} else if (*idp < 0) {
			/*
			 * for compatibility, set ESRCH rather than EINVAL
			 */
			return ESRCH;
		}
		break;
	case P_ALL:
		/*
		 * For P_ALL, set the ID value to 0 so that the code in
		 * checkprocset() will correctly detect trivial rejection
		 * cases with P_ALL.
		 */
		*idp = 0;
		break;
	default:
		return EINVAL;
	}

	return 0;
}


/*
 *
 * STATIC int checkprocsetid(idtype_t idtype, id_t *idp)
 *	Validate the procset_t ID-type and value as respectively given
 *	in idtype, and *idp, when the procset_t argument is used to
 *	select processes.
 *
 * Calling/Exit State:
 *    Parameters:
 *	The *idp value will be modified if it was originally equal to P_MYID,
 *	and the ID-type in idtype is valid.  In addition, *idp is set to zero
 *	(0) when the ID-type is P_ALL.
 *    Returns:
 *	0:	if the ID-type and value are valid.
 *	EINVAL:	if the ID-type is invalid.
 *
 */
STATIC int
checkprocsetid(register idtype_t idtype, register id_t *idp)
{
	switch (idtype) {
	case P_PID:
	case P_PPID:
	case P_PGID:
	case P_SID:
	case P_CID:
	case P_UID:
	case P_GID:
		if (*idp == P_MYID) {
			*idp = getmyid(idtype);
		} else if (*idp < 0) {
			return ESRCH;
		}
		break;
	case P_ALL:
		/*
		 * For P_ALL, set the ID value to 0 so that the code in
		 * checkprocset() will correctly detect trivial rejection
		 * cases with P_ALL.
		 */
		*idp = 0;
		break;
	case P_LWPID:	/* P_LWPID is not valid when selecting processes */
		return ESRCH;
	default:
		return EINVAL;
	}

	return 0;
}


/*
 *
 * STATIC int checkprocset(procset_t *psp, int (*funcp)(idtype_t, id_t *))
 *	Validate the procset_t parameter: psp, using the given function to
 *	validate the ID-types and values given by the procset_t parameter.
 *
 * Calling/Exit State:
 *    Parameters:
 *	The procset_t structure referenced by the 'psp' parameter may be
 *	modified by this function to a more efficient form, when a "trivial"
 *	selection set is designated (e.g., POP_AND or POP_OR with equal
 *	operand types and ID values).  An operand ID value of P_MYID also
 *	causes the procset_t structure referenced by 'psp' to be modified.
 *    Returns:
 *	0:	if procset_t is valid and specifies a potentially non-empty
 *		selection set of processes.
 *	ESRCH:	if the procset_t set is valid, but specifies an empty
 *		selection set of processes.
 *	EINVAL:	if the procset_t parameter is invalid (bad op or ID-types).
 *
 */
STATIC int
checkprocset(register procset_t *psp, register int (*funcp)(idtype_t, id_t *))
{
	register int error;

	if ((error = (*funcp)(psp->p_lidtype, &psp->p_lid)) != 0)
		return error;

	if ((error = (*funcp)(psp->p_ridtype, &psp->p_rid)) != 0)
		return error;

	/*
	 * Verify "op" validity, and catch ridiculous op and ID type
	 * combinations that lead to empty selection sets, avoiding the
	 * duplication of this logic throughout the dotoprocs() logic.
	 */
	switch (psp->p_op) {
	case POP_DIFF:
		if (psp->p_ridtype == P_ALL) {
			error = ESRCH;
			break;
		}
		/* FALLTHROUGH */
	case POP_XOR:
		if (psp->p_lidtype == psp->p_ridtype) {
			if (psp->p_lid == psp->p_rid) {
				error = ESRCH;
			}
		}
		break;
	case POP_AND:
		if (psp->p_lidtype == psp->p_ridtype) {
			if (psp->p_lid != psp->p_rid) {
				error = ESRCH;
			} else {
				psp->p_ridtype = P_ALL;	/* simplify selection */
			}
		}
		break;
	case POP_OR:
		if (psp->p_ridtype == P_ALL) {
			psp->p_lidtype = P_ALL;		/* simplify selection */
		} else if (psp->p_lidtype == psp->p_ridtype &&
			   psp->p_lid == psp->p_rid) {
			psp->p_op = POP_AND;		/* simplify selection */
			psp->p_ridtype = P_ALL;
		}
		break;
	default:
		error = EINVAL;
	}

	return error;
}


/* 
 *
 * STATIC int
 * linear_search(procset_t *psp, boolean_t init_is_special,
 *		 int (*funcp)(proc_t *, void *), void *arg)
 *	Locate the target processes specified by the procset_t (psp) parameter
 *	via linear search, when the target process set is not a proper subset
 *	of the caller's session.  For each target, invoke the given function
 *	with its associated argument.
 *
 * Calling/Exit State:
 *    Locking:
 *	No spin locks held upon entry or exit.
 *    Parameters:
 *	The process selection set criteria referenced from (psp) is
 *	presumed to be valid.  The processes selected by this function are
 *	operated upon using the function identified by the (funcp) and (arg)
 *	parameters.  If "init_is_special" is B_TRUE, then init(1) is to be
 *	selected only if it is the only process that satisfies the selection
 *	criteria.  Otherwise, init(1) is to be treated as an ordinary process,
 *	selected by the selection criteria, even if other processes are also
 *	selected by the selection criteria.
 *    Return value:
 *	Zero if successful.  Otherwise, the non-zero errno code identifying
 *	the failure is returned.
 *
 */
STATIC int
linear_search(procset_t *psp, boolean_t init_is_special,
	      int (*funcp)(proc_t *, void *), void *arg)
{
	register proc_t *pp;
	boolean_t init_selected = B_FALSE;	/* init(1) selected */
	boolean_t non_init_selected = B_FALSE;	/* non-init(1) proc selected */
 	int error;
#ifdef CC_PARTIAL
	int nsearched = 0;
	int psearch;
#endif /* CC_PARTIAL */

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Linearly walk through the process list selecting
	 * candidates as we go.
	 */
	(void)RW_RDLOCK(&proc_list_mutex, PL_PROCLIST);
	for (pp = practive; pp != NULL; pp = pp->p_next) {
#ifdef CC_PARTIAL
		nsearched++;
#endif /* CC_PARTIAL */
		(void)LOCK(&pp->p_mutex, PLHI); 
		if (pp->p_nlwp > 0 &&
		    (pp->p_flag & P_SYS) == 0 && procinset(psp, pp)) {
			/*
			 * This process is selected.
			 */
			if (pp != proc_init || !init_is_special) {
				if ((error = (*funcp)(pp, arg)) == 0) {
					non_init_selected = B_TRUE;
				} else if (error != ESRCH) {
					UNLOCK(&pp->p_mutex, PL_PROCLIST);
					RW_UNLOCK(&proc_list_mutex, PLBASE);
					return error;
				}
			} else
				init_selected = B_TRUE;
		}
		UNLOCK(&pp->p_mutex, PL_PROCLIST); 
	}

#ifdef CC_PARTIAL
	/*
	 * Search a minimum number of processes to treat
	 * a covert channel resulting from the ability
	 * to estimate the number of procs in the system by
	 * timing a kill().
	 */
	pp = practive;
	for (psearch = cc_getinfo(CC_PSEARCHMIN);
	     nsearched < psearch; nsearched++) {
		(void)LOCK(&pp->p_mutex, PLHI);
		if (pp->p_nlwp > 0 && (pp->p_flag & P_SYS) == 0)
			(void)procinset(psp, pp);
		UNLOCK(&pp->p_mutex, PL_PROCLIST); 


		if ((pp = pp->p_next) == NULL)
			pp = practive;
	}
#endif /* CC_PARTIAL */

	RW_UNLOCK(&proc_list_mutex, PLBASE);

	if (!non_init_selected) {		/* no non-init procs selected */
		error = ESRCH;			/* def. error for no targets */
		if (init_selected) {
			/*
			 * If init(1) tries to exit, the system panics.
			 */
			ASSERT(init_is_special);
			(void)LOCK(&proc_init->p_mutex, PLHI);
			error = (*funcp)(proc_init, arg);
			UNLOCK(&proc_init->p_mutex, PLBASE);
		}
		return error;
	}

	return 0;		/* we selected at least one process */
}


/* 
 *
 * STATIC int
 * oneproc(procset_t *psp, int (*funcp)(proc_t *, void *), void *arg)
 *	Locate the single target process specified by the procset_t (psp)
 *	parameter where the selected process is identified by the "left-hand"
 *	selection set id-type of P_PID.  If the given process is not a zombie
 *	or a system process, and satisfies the "right-hand" search criteria,
 *	then invoke the given function with its associated argument.
 *
 * Calling/Exit State:
 *	No spin locks held upon entry or exit.
 *
 *	The process selected by this function is operated upon using
 *	the function identified by the (funcp) and (arg) parameters.
 *
 * 	Return Value:
 *	  Zero if successful.  Otherwise, the non-zero errno code identifying
 *	  the failure is returned.
 *
 * Remarks:
 *	The selection set criteria identified by (psp) must have a p_op
 *	value of either POP_DIFF or POP_AND.
 *
 */
STATIC int
oneproc(procset_t *psp, int (*funcp)(proc_t *, void *), void *arg)
{
	register proc_t *pp;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(psp->p_lidtype == P_PID);
	ASSERT(psp->p_op == POP_DIFF || psp->p_op == POP_AND);

	error = ESRCH;
	if ((pp = prfind((pid_t)psp->p_lid)) != NULL) {	/* p_mutex held */
		/*
		 * Before considering the process further, make sure
		 * that it is not a zombie or a system process.
		 */
		if (pp->p_nlwp != 0 && (pp->p_flag & P_SYS) == 0) {
			/* check the right-hand search criteria */
			if (  (psp->p_op == POP_AND)
			    == opinset(psp->p_ridtype, psp->p_rid, pp)) {
				error = (*funcp)(pp, arg);
			}
		}
		UNLOCK(&pp->p_mutex, PLBASE);
	}
	return error;
}


/*
 *
 * STATIC int
 * pgrp_in_sess(proc_t *pp, boolean_t init_is_special, procset_t *psp,
 *		int (*funcp)(proc_t *, void *), void *arg,
 *		boolean_t *init_selp, boolean_t *non_init_selp)
 *	Operate on the proper subset of processes in the process group whose
 *	first process is (pp), as selected by the procset_t (psp) parameter
 *	where the selected processes are known to be limited to a proper
 *	subset of the "left-hand" selection set id-type of P_PGID.  For each
 *	such target, invoke the given function with its associated argument.
 *
 * Calling/Exit State:
 *    Locking:
 *	The s_mutex lock of the session containing the target process group
 *	must be held by the caller upon entry.  This lock remains held upon
 *	return.
 *    Parameters:
 *	The processes selected by this function are operated upon using
 *	the function identified by the (funcp) and (arg) parameters.
 *	If (*init_selp) is B_TRUE, then the system init(1) process has been
 *	selected by the search criteria.
 *	If (*non_init_selp) is B_TRUE, then at least one process besides
 *	the system init(1) process has already been selected by the search
 *	criteria.
 *	If (init_is_special) is B_TRUE, then init(1) is to be selected only
 *	if it is the only process that satisfies the selection criteria.
 *	Otherwise, init(1) is to be treated as an ordinary process, selected
 *	by the selection criteria, even if other processes are also selected
 *	by the selection criteria.
 *    Return Value:
 *	Zero if successful.  Otherwise, the non-zero errno code identifying
 *	the failure is returned.
 *
 * Remarks:
 *	The selection set criteria identified by (psp) must have a p_op
 *	value of either POP_DIFF or POP_AND.
 *
 */ 
STATIC int
pgrp_in_sess(register proc_t *pp, boolean_t init_is_special, procset_t *psp,
	     int (*funcp)(proc_t *, void *), void *arg,
	     boolean_t *init_selp, boolean_t *non_init_selp)
{
 	int retval;			/* return value from (*funcp)(pp,arg) */

	ASSERT(pp != NULL);
	ASSERT(LOCK_OWNED(&pp->p_sessp->s_mutex));
	ASSERT(getpl() == PL_SESS);
	ASSERT(psp->p_lidtype == P_PGID || psp->p_lidtype == P_SID);
	ASSERT(psp->p_op == POP_DIFF || psp->p_op == POP_AND);

	/*
	 * While continuing to hold the session locked,
	 * for each non-zombie process in the group....
	 */
	switch (psp->p_ridtype) {
	case P_PID:
		/*
		 * The process will continue to exist as a non-zombie process
		 * since we hold s_mutex, and p_mutex is not needed to check
		 * the process-ID.  Furthermore, the only possible way to get
		 * here is for POP_DIFF.  All other optype combinations with
		 * P_PID are to have been caught by the caller using process
		 * lookup, or linear search.
		 */
		ASSERT(psp->p_op == POP_DIFF);
		do {
			if (psp->p_rid != pp->p_pidp->pid_id) {
				if (pp != proc_init || !init_is_special) {
					(void)LOCK(&pp->p_mutex, PLHI);
					if ((pp->p_flag & P_SYS) == 0) {
						retval = (*funcp)(pp, arg);
						UNLOCK(&pp->p_mutex, PL_SESS);
						if (retval == 0) {
							*non_init_selp = B_TRUE;
						} else if (retval != ESRCH) {
							return retval;
						}
					} else
						UNLOCK(&pp->p_mutex, PL_SESS);
				} else
					*init_selp = B_TRUE;
			}
		} while ((pp = pp->p_pglinkf) != NULL);
		break;
	case P_PGID:
	case P_SID:
	case P_ALL:
		/*
		 * Right-hand operand constant for all processes in group by
		 * holding s_mutex.  Check only once.
		 */
		if (  (psp->p_op == POP_AND)
		    == opinset(psp->p_ridtype, psp->p_rid, pp)) {
			do {
				if (pp != proc_init || !init_is_special) {
					(void)LOCK(&pp->p_mutex, PLHI);
					if ((pp->p_flag & P_SYS) == 0) {
						retval = (*funcp)(pp, arg);
						UNLOCK(&pp->p_mutex, PL_SESS);
						if (retval == 0) {
							*non_init_selp = B_TRUE;
						} else if (retval != ESRCH) {
							return retval;
						}
					} else
						UNLOCK(&pp->p_mutex, PL_SESS);
				} else
					*init_selp = B_TRUE;
			} while ((pp = pp->p_pglinkf) != NULL);
		}
		break;
	case P_PPID:
	case P_CID:
	case P_UID:
	case P_GID:
		/*
		 * For P_PPID, P_CID, P_UID, P_GID, we must check each process
		 * in the session individually, and we must hold p_mutex to
		 * perform the check.
		 */
		do {
			(void)LOCK(&pp->p_mutex, PLHI);
			if (  (psp->p_op == POP_AND)
			    == opinset(psp->p_ridtype, psp->p_rid, pp) &&
			    (pp->p_flag & P_SYS) == 0) {
				if (pp != proc_init || !init_is_special) {
					retval = (*funcp)(pp, arg);
					UNLOCK(&pp->p_mutex, PL_SESS);
					if (retval == 0) {
						*non_init_selp = B_TRUE;
					} else if (retval != ESRCH)
						return retval;
				} else {
					UNLOCK(&pp->p_mutex, PL_SESS);
					*init_selp = B_TRUE;
				}
			} else
				UNLOCK(&pp->p_mutex, PL_SESS);
		} while ((pp = pp->p_pglinkf) != NULL);
		break;
	default:
		/*
		 *+ A bad ridtype argument was passed to the
		 *+ pgrp_in_sess() function.
		 */
		cmn_err(CE_WARN, "pgrp_in_sess() called with bad ridtype");
		return EINVAL;
	}

	return 0;	/* no errors, but maybe no processes selected either */
}


/*
 *
 * STATIC int
 * procs_in_pgrp(procset_t *psp, boolean_t init_is_special,
 *		 int (*funcp)(proc_t *, void *), void *arg)
 *	Locate the target processes specified by the procset_t (psp) parameter
 *	where the selected processes are known to be limited to a proper
 *	subset of the "left-hand" selection set id-type of P_PGID.  For each
 *	target, invoke the given function with its associated argument.
 *
 * Calling/Exit State:
 *    Locking:
 *	No spin locks held upon entry or exit.
 *    Parameters:
 *	The processes selected by this function are operated upon using
 *	the function identified by the (funcp) and (arg) parameters.
 *	If (init_is_special) is B_TRUE, then init(1) is to be selected only
 *	if it is the only process that satisfies the selection criteria.
 *	Otherwise, init(1) is to be treated as an ordinary process, selected
 *	by the selection criteria, even if other processes are also selected
 *	by the selection criteria.
 *    Return Value:
 *	Zero if successful.  Otherwise, the non-zero errno code identifying
 *	the failure is returned.
 *
 * Remarks:
 *	For locking reasons, this function checks to see if the process
 *	group-ID specified as the left-operand of the procset_t parameter
 *	(psp), identifies a process group in the session of the calling
 *	process (a likely case for shell job control).  This is accomplished
 *	rapidly using the process group linkages within the session.  If
 *	however the group is not within the session, then the search
 *	degenerates to a simple (but slow) linear search.
 *
 *	The selection set criteria identified by (psp) must have a p_op
 *	value of either POP_DIFF or POP_AND.
 *
 */ 
STATIC int
procs_in_pgrp(procset_t *psp, boolean_t init_is_special,
	      int (*funcp)(proc_t *, void *), void *arg)
{
	register proc_t *pp;
	register struct pid *pgidp;
	sess_t *sessp;
	boolean_t init_selected = B_FALSE;	/* init(1) selected */
	boolean_t non_init_selected = B_FALSE;	/* non-init(1) proc selected */
 	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(psp->p_lidtype == P_PGID);
	ASSERT(psp->p_op == POP_DIFF || psp->p_op == POP_AND);

	pp = u.u_procp;
	(void)LOCK(&pp->p_sess_mutex, PL_SESS);
	sessp = pp->p_sessp;
	(void)LOCK(&sessp->s_mutex, PL_SESS);
	UNLOCK(&pp->p_sess_mutex, PL_SESS);

	/* 
	 * Check if the target process group exists
	 * within the caller's session.
	 */
	if ((pgidp = pgfind_sess((pid_t)psp->p_lid)) == NULL) {
		/* Not in this session.  Revert to linear search. */
		UNLOCK(&sessp->s_mutex, PLBASE);
		error = linear_search(psp, init_is_special, funcp, arg);
		/*
		 * If no processes were found, but the pgrp still exists,
		 * return success.
		 */
		if (error == ESRCH && pgfind((pid_t)psp->p_lid))
			error = 0;
		return error;
	}

	/*
	 * The process group exists within the caller's session.
	 * However, it is possible that the found process group contains
	 * only zombie processes....
	 */
	pp = pgidp->pid_pgprocs;
	if (pp == NULL) {			/* only zombies */
		UNLOCK(&sessp->s_mutex, PLBASE);
		return 0;
	}

	/*
	 * While continuing to hold the session locked,
	 * for each non-zombie process in the group....
	 */
	error = pgrp_in_sess(pp, init_is_special, psp, funcp, arg,
			     &init_selected, &non_init_selected);

	UNLOCK(&sessp->s_mutex, PLBASE);

	if (error != 0 && error != ESRCH)
		return error;

	if (non_init_selected)
		return 0;		/* we selected at least one process */

	if (!init_selected)
		return 0;		/* no error for empty pgrp */

	/*
	 * If init(1) tries to exit, the system panics.
	 */
	ASSERT(init_is_special);
	(void)LOCK(&proc_init->p_mutex, PLHI);
	error = (*funcp)(proc_init, arg);
	UNLOCK(&proc_init->p_mutex, PLBASE);
	return error;
}


/* 
 *
 * STATIC int
 * procs_in_sess(procset_t *psp, boolean_t init_is_special,
 *		 int (*funcp)(proc_t *, void *), void *arg)
 *	Locate the target processes specified by the procset_t (psp) parameter
 *	where the selected processes are known to be limited to a proper
 *	subset of the "left-hand" selection set id-type of P_SID.  For each
 *	target, invoke the given function with its associated argument.
 *
 * Calling/Exit State:
 *    Locking:
 *	No spin locks held upon entry or exit.
 *    Parameters:
 *	The processes selected by this function are operated upon using
 *	the function identified by the (funcp) and (arg) parameters.
 *	If (init_is_special) is B_TRUE, then init(1) is to be selected only
 *	if it is the only process that satisfies the selection criteria.
 *	Otherwise, init(1) is to be treated as an ordinary process, selected
 *	by the selection criteria, even if other processes are also selected
 *	by the selection criteria.
 *    Return Value:
 *	Zero if successful.  Otherwise, the non-zero errno code identifying
 *	the failure is returned.
 *
 * Remarks:
 *	For locking reasons, this function checks to see if the session-ID
 *	specified as the left-operand of the procset_t parameter (psp) is
 *	the session-ID of the calling process.  If so, the search operation
 *	is performed using the various session and process group linkages
 *	of the caller's session.  Otherwise, the search degenerates to a
 *	simple (but slow) linear search.
 *
 *	The selection set criteria identified by (psp) must have a p_op
 *	value of either POP_DIFF or POP_AND.
 *
 */ 
STATIC int
procs_in_sess(procset_t *psp, boolean_t init_is_special,
	      int (*funcp)(proc_t *, void *), void *arg)
{
	register proc_t *pp;
	register struct pid *pgidp;
	sess_t *sessp;
	boolean_t init_selected = B_FALSE;	/* init(1) selected */
	boolean_t non_init_selected = B_FALSE;	/* non-init(1) proc selected */
 	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(psp->p_lidtype == P_SID);
	ASSERT(psp->p_op == POP_DIFF || psp->p_op == POP_AND);

	pp = u.u_procp;
	if (pp->p_sid != psp->p_lid) {
		return linear_search(psp, init_is_special, funcp, arg);
	}

	(void)LOCK(&pp->p_sess_mutex, PL_SESS);
	if (pp->p_sid != psp->p_lid) {		/* catch any LWP setsid race */
		UNLOCK(&pp->p_sess_mutex, PLBASE);
		return linear_search(psp, init_is_special, funcp, arg);
	}

	sessp = pp->p_sessp;
	(void)LOCK(&sessp->s_mutex, PL_SESS);
	UNLOCK(&pp->p_sess_mutex, PL_SESS);

	pgidp = sessp->s_pgrps;
	while (pgidp != NULL) {
		/*
		 * Process groups linked-into the s_pgrps list contain at
		 * least one non-zombie process each.
		 */
		pp = pgidp->pid_pgprocs;
		ASSERT(pp != NULL);

		error = pgrp_in_sess(pp, init_is_special, psp, funcp, arg,
				     &init_selected, &non_init_selected);

		if (error != 0 && error != ESRCH) {
			UNLOCK(&sessp->s_mutex, PLBASE);
			return error;
		}

		pgidp = pgidp->pid_pgrpf;
	}
	UNLOCK(&sessp->s_mutex, PLBASE);

	if (!non_init_selected) {	/* no non-init procs selected */
		error = ESRCH;		/* def. error for no targets */
		if (init_selected) {
			/*
			 * If init(1) tries to exit, the system panics.
			 */
			ASSERT(init_is_special);
			(void)LOCK(&proc_init->p_mutex, PLHI);
			error = (*funcp)(proc_init, arg);
			UNLOCK(&proc_init->p_mutex, PLBASE);
		}
		return error;
	}

	return 0;		/* we selected at least one process */
}


/*
 *
 * int dotoprocs(procset_t *psp, boolean_t init_is_special,
 *		 int (*funcp)(proc_t *, void *), void *arg)
 *	Locate the target process(es) specified by the procset_t structure
 *	(psp), and invoke the function (funcp) with the given argument for
 *	each such process.
 *
 * Calling/Exit State:
 *    Locking:
 *	No spin locks held upon entry or exit.
 *    Parameters:
 *	If "init_is_special" is B_TRUE, then init(1) is to be selected only
 *	if it is the only process that satisfies the selection criteria.
 *	Otherwise, init(1) is to be treated as an ordinary process, selected
 *	by the selection criteria, even if other processes are also selected
 *	by the selection criteria.
 *    Return Value:
 *	Zero if successful.  Otherwise, the non-zero errno code identifying
 *	the failure is returned (e.g., ESRCH or EPERM).
 *
 */
int
dotoprocs(procset_t *psp, boolean_t init_is_special,
	  int (*funcp)(proc_t *, void *), void *arg)
{
	procset_t tps;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Check that the procset_t is valid.
	 */
	if ((error = checkprocset(psp, checkprocsetid)) != 0)
		return error;

	switch (psp->p_lidtype) {
	case P_PID:
		switch (psp->p_op) {
		case POP_AND:
		case POP_DIFF:
			error = oneproc(psp, funcp, arg);
			break;
		default:		/* POP_OR and POP_XOR */
			/*
			 * NOTE: We don't attempt optimizations for POP_OR and
			 * POP_XOR when p_ridtype is also P_PID, to avoid
			 * duplicating the logic dealing with the selection
			 * of the init(1) process, when it is the only
			 * process selected by the search criteria.
			 */
			error = linear_search(psp, init_is_special, funcp, arg);
			break;
		}
		break;
	case P_PGID:
		switch (psp->p_op) {
		case POP_AND:
			switch (psp->p_ridtype) {
			case P_PID:		/* reverse the arguments */
				SWITCH_ARGS(psp, tps);
				error = oneproc(&tps, funcp, arg);
				break;
			case P_PPID:
			case P_UID:
			case P_GID:
			case P_CID:
			case P_ALL:
				break;
			default:
				return EINVAL;
			}
			/* FALLTHROUGH */
		case POP_DIFF:
			error = procs_in_pgrp(psp, init_is_special, funcp, arg);
			break;
		default:		/* POP_OR and POP_XOR */
			error = linear_search(psp, init_is_special, funcp, arg);
			break;
		}
		break;
	case P_SID:
		switch (psp->p_op) {
		case POP_AND:
			switch (psp->p_ridtype) {
			case P_PID:		/* reverse the arguments */
				SWITCH_ARGS(psp, tps);
				error = oneproc(&tps, funcp, arg);
				break;
			case P_PGID:		/* reverse the arguments */
				SWITCH_ARGS(psp, tps);
				error = procs_in_pgrp(&tps, init_is_special,
						      funcp, arg);
				break;
			case P_PPID:
			case P_UID:
			case P_GID:
			case P_CID:
			case P_ALL:
				error = procs_in_sess(psp, init_is_special,
						      funcp, arg);
				break;
			default:
				return EINVAL;
			}
			break;
		case POP_DIFF:
			error = procs_in_sess(psp, init_is_special, funcp, arg);
			break;
		default:		/* POP_OR and POP_XOR */
			error = linear_search(psp, init_is_special, funcp, arg);
			break;
		}
		break;
	case P_PPID:
	case P_UID:
	case P_GID:
	case P_CID:
	case P_ALL:
		if (psp->p_op == POP_AND) {
			/*
			 * Based on the "right-hand" selection attributes,
			 * select processes.
			 */
			switch (psp->p_ridtype) {
			case P_PID:
				SWITCH_ARGS(psp, tps);	/* reverse the args */
				error = oneproc(&tps, funcp, arg);
				break;
			case P_PGID:
				SWITCH_ARGS(psp, tps);	/* reverse the args */
				error = procs_in_pgrp(&tps, init_is_special,
						      funcp, arg);
				break;
			case P_SID:
				SWITCH_ARGS(psp, tps);	/* reverse the args */
				error = procs_in_sess(&tps, init_is_special,
						      funcp, arg);
				break;
			default:	/* P_{PPID, UID, GID, CID, ALL} */
				error = linear_search(psp, init_is_special,
						      funcp, arg);
				break;
			}
		} else			/* POP_DIFF, POP_OR, POP_XOR */
			error = linear_search(psp, init_is_special, funcp, arg);
		break;
	default:
		return EINVAL;
	}

	return error;
}


/*
 *
 * STATIC int 
 * dotoall_lwps(lwp_t *lwpp, int (*funcp)(lwp_t *, void *, int *), void *arg)
 *	Beginning with the given LWP (which is assumed to be the first LWP
 *	in its process), invoke the given function (funcp) with its associated
 *	argument (arg) for each LWP in the process.
 *
 * Calling/Exit State:
 *    Locking:
 *	The p_mutex lock of the process containing the given LWP must be
 *	held locked by the caller.  This lock remains held upon return.
 *    Return Value:
 *	Zero if successful.  Otherwise, the non-zero errno code identifying
 *	the failure is returned.
 *
 */
STATIC int 
dotoall_lwps(lwp_t *lwpp, int (*funcp)(lwp_t *, void *, int *), void *arg)
{
	int error = 0;
	int gotpid = 0;

	ASSERT(LOCK_OWNED(&lwpp->l_procp->p_mutex));
	ASSERT(getpl() == PLHI);
	ASSERT(lwpp->l_procp->p_nlwp != 0 &&
	       (lwpp->l_procp->p_flag & P_SYS) == 0);

	do
		if (lwpp->l_stat != SIDL) {
			(void)LOCK(&lwpp->l_mutex, PLHI);
			error = (*funcp)(lwpp, arg, &gotpid);
			UNLOCK(&lwpp->l_mutex, PLHI);
			if (error)
				break;
		}
	while ((lwpp = lwpp->l_next) != NULL);
	return error;
}


/*
 *
 * STATIC int
 * lwp_leftop_cid(procset_t *psp, proc_t *pp,
 *		  int (*funcp)(lwp_t *, void *, int *), void *arg)
 *      Locate the target LWPs specified by the procset_t (psp) parameter
 *	within the given process (pp) (which must not be a zombie or system
 *	process), where the "left-hand" selection set criteria is P_CID.
 *	For each such target, invoke the given function with its associated
 *	argument.
 *
 * Calling/Exit State:
 *    Locking:
 *	The p_mutex lock of the target process must be held locked by the
 *	caller.  This lock remains held upon return.
 *    Parameters:
 *	The LWP selection set criteria referenced from (psp) is presumed
 *	to be valid.  The LWPs selected by this function are operated
 *	upon using the function identified by the (funcp) and (arg)
 *	parameters.
 *    Return Value:
 *	Zero if successful.  Otherwise, the non-zero errno code identifying
 *	the failure is returned.
 *
 */
STATIC int
lwp_leftop_cid(procset_t *psp, proc_t *pp,
	       int (*funcp)(lwp_t *, void *, int *), void *arg)
{
	register lwp_t *lwpp;
	register idop_t idop;
	boolean_t roperand;
	id_t lid, rid;
	int gotpid = 0;
	int error;

	ASSERT(LOCK_OWNED(&pp->p_mutex));
	ASSERT(getpl() == PLHI);
	ASSERT(pp->p_nlwp != 0 && (pp->p_flag & P_SYS) == 0);
	ASSERT(psp->p_lidtype == P_CID);

	lid = psp->p_lid;
	rid = psp->p_rid;

	roperand = B_FALSE;
	switch (psp->p_ridtype) {
	case P_PID:
	case P_PGID:
	case P_SID:
	case P_PPID:
	case P_UID:
	case P_GID:
	case P_ALL:
		/*
		 * The "right-hand" selection criteria identifies
		 * process attributes that need only be tested once.
		 */
                roperand = opinset(psp->p_ridtype, rid, pp);
		break;
	case P_CID:
	case P_LWPID:
		break;
	default:
		/*
		 *+ A bad ridtype argument was passed to the
		 *+ lwp_leftop_cid() function.
		 */
		cmn_err(CE_WARN, "lwp_leftop_cid() called with bad ridtype");
		return EINVAL;
	}

	error = ESRCH;

	for (lwpp = pp->p_lwpp; lwpp != NULL; lwpp = lwpp->l_next)
		if (lwpp->l_stat != SIDL) {

			idop = psp->p_op; /* need private copy, see below */
			(void)LOCK(&lwpp->l_mutex, PLHI);
			if (lwpp->l_cid != lid) {
				/*
				 * "Left-hand" selection set criteria is not
				 * satisfied.  The only remaining hope is for
				 * POP_OR and POP_XOR.
				 */
				if (idop != POP_OR && idop != POP_XOR) {
					UNLOCK(&lwpp->l_mutex, PLHI);
					continue;
				}
				/*
				 * Flow into the code below with idop set to
				 * POP_AND, forcing the "right-hand" criteria
				 * to be satisfied for the LWP(s) to be
				 * selected, since the "left-hand" criteria
				 * is not satisfied.
				 */
				ASSERT(idop == POP_OR || idop == POP_XOR);
				idop = POP_AND;
			}
			if (idop != POP_OR) {
				/*
				 * Handle the POP_DIFF, POP_AND, and POP_XOR
				 * cases.  The "left-hand" selection set
				 * criteria is satisfied, or else idop was
				 * originally POP_OR or POP_XOR, and has been
				 * changed to POP_AND for failed "left-hand"
				 * criteria.
				 */
				switch (psp->p_ridtype) {
				case P_PID:
				case P_PGID:
				case P_SID:
				case P_PPID:
				case P_UID:
				case P_GID:
				case P_ALL:
					/*
					 * The "right-hand" selection criteria
					 * identifies process attributes that
					 * have already been tested.
					 */
					if ((idop == POP_AND) != (roperand)) {
						UNLOCK(&lwpp->l_mutex, PLHI);
						continue;
					}
					break;
				case P_CID:
					if ((idop == POP_AND) != (lwpp->l_cid == rid)) {
						UNLOCK(&lwpp->l_mutex, PLHI);
						continue;
					}
					break;
				case P_LWPID:
					if ((idop == POP_AND) !=
					    (pp == u.u_procp && lwpp->l_lwpid == rid)) {
						UNLOCK(&lwpp->l_mutex, PLHI);
						continue;
					}
					break;
				default:
					UNLOCK(&lwpp->l_mutex, PLHI);
					/*
					 *+ A bad ridtype argument was passed to the
					 *+ lwp_leftop_cid() function.
					 */
					cmn_err(CE_WARN,
						"lwp_leftop_cid() called with bad ridtype");
					return EINVAL;
				}
			}

			ASSERT(LOCK_OWNED(&lwpp->l_mutex));
			error = (*funcp)(lwpp, arg, &gotpid);
			UNLOCK(&lwpp->l_mutex, PLHI);
			if (error)
				break;
		}

	return error;
}


/*
 *
 * STATIC int
 * lwp_leftop_lwpid(procset_t *psp, proc_t *pp,
 *		    int (*funcp)(lwp_t *, void *, int *), void *arg)
 *      Locate the target LWPs specified by the procset_t (psp) parameter
 *	within the given process (pp) (which must not be a zombie or system
 *	process), where the "left-hand" selection set criteria is P_LWPID.
 *	For each such target, invoke the given function with its associated
 *	argument.
 *
 * Calling/Exit State:
 *    Locking:
 *	The p_mutex lock of the target process must be held locked by the
 *	caller.  This lock remains held upon return.
 *    Parameters:
 *	The LWP selection set criteria referenced from (psp) is presumed
 *	to be valid.  The LWPs selected by this function are operated
 *	upon using the function identified by the (funcp) and (arg)
 *	parameters.
 *    Return Value:
 *	Zero if successful.  Otherwise, the non-zero errno code identifying
 *	the failure is returned.
 *
 */
STATIC int
lwp_leftop_lwpid(procset_t *psp, proc_t *pp,
		 int (*funcp)(lwp_t *, void *, int *), void *arg)
{
	register lwp_t *lwpp;
	register idop_t idop;
	boolean_t roperand;
	id_t lid, rid;
	int error;
	int gotpid = 0;

	ASSERT(LOCK_OWNED(&pp->p_mutex));
	ASSERT(getpl() == PLHI);
	ASSERT(pp->p_nlwp != 0 && (pp->p_flag & P_SYS) == 0);
	ASSERT(psp->p_lidtype == P_LWPID);

	lid = psp->p_lid;
	rid = psp->p_rid;

	roperand = B_FALSE;
	switch (psp->p_ridtype) {
	case P_PID:
	case P_PGID:
	case P_SID:
	case P_PPID:
	case P_UID:
	case P_GID:
	case P_ALL:
		/*
		 * The "right-hand" selection criteria identifies
		 * process attributes that need only be tested once.
		 */
                roperand = opinset(psp->p_ridtype, rid, pp);
		break;
	case P_CID:
	case P_LWPID:
		break;
	default:
		/*
		 *+ A bad ridtype argument was passed to the
		 *+ lwp_leftop_lwpid() function.
		 */
		cmn_err(CE_WARN, "lwp_leftop_lwpid() called with bad ridtype");
		return EINVAL;
	}

	error = ESRCH;

	for (lwpp = pp->p_lwpp; lwpp != NULL; lwpp = lwpp->l_next)
		if (lwpp->l_stat != SIDL) {
			idop = psp->p_op; /* need private copy, see below */
			if (u.u_procp != pp || lwpp->l_lwpid != lid) {
				/*
				 * "Left-hand" selection set criteria is not
				 * satisfied.  The only remaining hope is for
				 * POP_OR and POP_XOR.
				 */
				if (idop != POP_OR && idop != POP_XOR) {
					continue;
				}
				/*
				 * Flow into the code below with idop set to
				 * POP_AND, forcing the "right-hand" criteria
				 * to be satisfied for the LWP(s) to be
				 * selected, since the "left-hand" criteria
				 * is not satisfied.
				 */
				ASSERT(idop == POP_OR || idop == POP_XOR);
				idop = POP_AND;
			}

			if (idop != POP_OR) {
				/*
				 * Handle the POP_DIFF, POP_AND, and POP_XOR
				 * cases.  The "left-hand" selection set
				 * criteria is satisfied, or else idop was
				 * originally POP_OR or POP_XOR, and has been
				 * changed to POP_AND for failed "left-hand"
				 * criteria.
				 */
				switch (psp->p_ridtype) {
				case P_PID:
				case P_PGID:
				case P_SID:
				case P_PPID:
				case P_UID:
				case P_GID:
				case P_ALL:
					/*
					 * The "right-hand" selection criteria
					 * identifies process attributes that
					 * have already been tested.
					 */
					if ((idop == POP_AND) != (roperand))
						continue;
					(void)LOCK(&lwpp->l_mutex, PLHI);
					break;
				case P_CID:
					(void)LOCK(&lwpp->l_mutex, PLHI);
					if ((idop == POP_AND) != (lwpp->l_cid == rid)) {
						UNLOCK(&lwpp->l_mutex, PLHI);
						continue;
					}
					break;
				case P_LWPID:
					if ((idop == POP_AND) !=
					    (pp == u.u_procp && lwpp->l_lwpid == rid))
						continue;
					(void)LOCK(&lwpp->l_mutex, PLHI);
					break;
				default:
					/*
					 *+ A bad ridtype argument was passed to the
					 *+ lwp_leftop_lwpid() function.
					 */
					cmn_err(CE_WARN,
						"lwp_leftop_lwpid() called with bad ridtype");
					return EINVAL;
				}
			} else
				(void)LOCK(&lwpp->l_mutex, PLHI);

			ASSERT(LOCK_OWNED(&lwpp->l_mutex));
			error = (*funcp)(lwpp, arg, &gotpid);
			UNLOCK(&lwpp->l_mutex, PLHI);
			if (error)
				break;
		}

	return error;
}


/*
 * This structure is used when operating upon a collection of LWPs within
 * a specific process selected by the higher-level dotoprocs() logic.  The
 * contents of this structure describe the original selection criteria, as
 * well as the function to call with its associated argument when an LWP
 * within the process is selected by the search criteria.
 */
typedef struct dotolwps {
	procset_t *d_psp;	/* ptr to procset_t LWP selection criteria */
	int (*d_funcp)(lwp_t *, void*, int *);	/* fn to call when operating on LWP */
	void *d_arg;		/* arg to pass to fn operating upon LWP */
} do2lwpargs_t;


/*
 *
 * STATIC int lwps_inproc(proc_t *pp, void *do2lwpargsp)
 *	Operate on the LWPs within the process: pp (which must not be
 *	a zombie or system process), as selected by the procset_t
 *	selection criteria passed via the do2lwpargsp parameter.
 *	For each target LWP, invoke the function given in the structure
 *	referenced by do2lwpargsp, with its associated argument.
 *
 * Calling/Exit State:
 *    Locking:
 *	The p_mutex lock of the target process must be held locked by the
 *	caller.  This lock remains held upon return.
 *    Return Value:
 *	Zero if successful.  Otherwise, the non-zero errno code identifying
 *	the failure is returned.
 *
 */
STATIC int
lwps_inproc(proc_t *pp, void *do2lwpargsp)
{
	register lwp_t *lwpp;
	register idop_t idop;
	procset_t *psp;
	int (*funcp)(lwp_t *, void *, int *);
	void *arg;
	boolean_t inset;
	id_t id;
	int error;
	int gotpid = 0;

	ASSERT(LOCK_OWNED(&pp->p_mutex));
	ASSERT(getpl() == PLHI);
	ASSERT(pp->p_nlwp != 0 && (pp->p_flag & P_SYS) == 0);

	if (pp->p_nlwpdir == 0)		/* already exited */
		return 0;

	psp   = ((do2lwpargs_t *)do2lwpargsp)->d_psp;
	funcp = ((do2lwpargs_t *)do2lwpargsp)->d_funcp;
	arg   = ((do2lwpargs_t *)do2lwpargsp)->d_arg;

	id = psp->p_lid;
	inset = B_FALSE;
	switch (psp->p_lidtype) {
	case P_PID:
		if (pp->p_pidp->pid_id == id)
			inset = B_TRUE;
		break;
	case P_PPID:
		if (pp->p_ppid == id)
			inset = B_TRUE;
		break;
	case P_PGID:
		if (pp->p_pgid == id)
			inset = B_TRUE;
		break;
	case P_SID:
		if (pp->p_sid == id)
			inset = B_TRUE;
		break;
	case P_UID:
		if (pp->p_cred->cr_uid == id)
			inset = B_TRUE;
		break;
	case P_GID:
		if (pp->p_cred->cr_gid == id)
			inset = B_TRUE;
		break;
	case P_CID:
		return lwp_leftop_cid(psp, pp, funcp, arg);
	case P_ALL:
		inset = B_TRUE;
		break;
	case P_LWPID:
		return lwp_leftop_lwpid(psp, pp, funcp, arg);
	default:
		/*
		 *+ A bad lidtype argument was passed to the lwps_inproc()
		 *+ function.
		 */
		cmn_err(CE_WARN, "lwps_inproc() called with bad lidtype");
		return EINVAL;
	}

	/*
	 * The "left-hand" ID type describes a process-level attribute that
	 * remains constant as long as p_mutex is held.
	 */
	error = ESRCH;
	idop = psp->p_op;		/* need private copy, see below */
	lwpp = pp->p_lwpp;
	if (!inset) {
		/*
		 * "Left-hand" selection set criteria is not satisfied.
		 * The only remaining hope is for POP_OR and POP_XOR.
		 */
		if (idop != POP_OR && idop != POP_XOR)
			return ESRCH;
		/*
		 * Flow into the code below with idop set to POP_AND, forcing
		 * the "right-hand" criteria to be satisfied for the LWP(s) to
		 * be selected, since the "left-hand" criteria is not satisfied.
		 */
		ASSERT(idop == POP_OR || idop == POP_XOR);
		idop = POP_AND;
	}
	if (idop != POP_OR) {
		/*
		 * Handle the POP_DIFF, POP_AND, and POP_XOR cases.
		 * The "left-hand" selection set criteria is satisfied, or
		 * else idop was originally POP_OR or POP_XOR, and has been
		 * changed to POP_AND for failed "left-hand" criteria.
		 */
		id = psp->p_rid;
		switch (psp->p_ridtype) {
		case P_PID:
		case P_PGID:
		case P_SID:
		case P_PPID:
		case P_UID:
		case P_GID:
		case P_ALL:
			/*
			 * The "right-hand" selection criteria identifies
			 * process attributes that need only be tested once.
			 */
			if ((idop == POP_AND) ==
			    (opinset(psp->p_ridtype, id, pp))) {
				error = dotoall_lwps(lwpp, funcp, arg);
			}
			break;
		case P_CID:
			do
				if (lwpp->l_stat != SIDL) {
					(void)LOCK(&lwpp->l_mutex, PLHI);
					if ((idop == POP_AND) ==
					    (lwpp->l_cid == id)) {
						error = (*funcp)(lwpp, arg, &gotpid);
						UNLOCK(&lwpp->l_mutex, PLHI);
						if (error)
							break;
					} else
						UNLOCK(&lwpp->l_mutex, PLHI);
				}
			while ((lwpp = lwpp->l_next) != NULL);
			break;
		case P_LWPID:
			do
				if (lwpp->l_stat != SIDL &&
				    (idop == POP_AND) ==
				    (pp == u.u_procp && lwpp->l_lwpid == id)) {
					(void)LOCK(&lwpp->l_mutex,PLHI);
					error = (*funcp)(lwpp, arg, &gotpid);
					UNLOCK(&lwpp->l_mutex, PLHI);
					if (error)
						break;
				}
			while ((lwpp = lwpp->l_next) != NULL);
			break;
		default:
			/*
			 *+ A bad ridtype argument was passed to the
			 *+ lwps_inproc() function.
			 */
			cmn_err(CE_WARN,
				"lwps_inproc() called with bad ridtype");
			return EINVAL;
		}
	} else {
		/*
		 * "Left-hand" selection set criteria is satisfied.
		 * This is the POP_OR case (all LWPs selected).
		 */
		error = dotoall_lwps(lwpp, funcp, arg);
	}

	return error;
}


/*
 *
 * STATIC int alllwps_inproc(proc_t *pp, void *do2lwpargsp)
 *	Operate on all LWPs within the process: pp (which must not be
 *	a zombie or system process).  For each LWP, invoke the function
 *	given in the structure referenced by do2lwpargsp, with its
 *	associated argument.
 *
 * Calling/Exit State:
 *    Locking:
 *	The p_mutex lock of the target process must be held locked by the
 *	caller.  This lock remains held upon return.
 *    Return Value:
 *	Zero if successful.  Otherwise, the non-zero errno code identifying
 *	the failure is returned.
 *
 */
STATIC int
alllwps_inproc(proc_t *pp, void *do2lwpargsp)
{
	ASSERT(LOCK_OWNED(&pp->p_mutex));
	ASSERT(getpl() == PLHI);
	ASSERT(pp->p_nlwp != 0 && (pp->p_flag & P_SYS) == 0);

	if (pp->p_nlwpdir == 0)		/* already exited */
		return 0;

	/*
	 * For DEBUG kernels, make sure the selection criteria really is
	 * process-level specific.
	 */
	ASSERT(((do2lwpargs_t *)do2lwpargsp)->d_psp->p_lidtype != P_LWPID);
	ASSERT(((do2lwpargs_t *)do2lwpargsp)->d_psp->p_lidtype != P_CID);
	ASSERT(((do2lwpargs_t *)do2lwpargsp)->d_psp->p_ridtype != P_LWPID);
	ASSERT(((do2lwpargs_t *)do2lwpargsp)->d_psp->p_ridtype != P_CID);

	return dotoall_lwps(pp->p_lwpp,
			     ((do2lwpargs_t *)do2lwpargsp)->d_funcp,
			     ((do2lwpargs_t *)do2lwpargsp)->d_arg);
}


/*
 *
 * int dotolwps(procset_t *psp, boolean_t init_is_special,
 *		int (*funcp)(lwp_t *, void *), void *arg)
 *	Locate the target LWP(s) specified by the procset_t structure
 *	(psp), and invoke the function (funcp) with the given argument for
 *	each such LWP.
 *
 * Calling/Exit State:
 *    Locking:
 *	No spin locks held upon entry or exit.
 *    Parameters:
 *	If "init_is_special" is B_TRUE, then LWPs in init(1) are to be
 *	selected only if init(1) is the only process containing LWPs that
 *	satisfy the selection criteria.  Otherwise, init(1) is to be treated
 *	as an ordinary process, with its LWPs selected by the selection
 *	criteria, even if LWPs in other processes are also selected by the
 *	selection criteria.
 *    Return Value:
 *	Zero if successful.  Otherwise, the non-zero errno code identifying
 *	the failure is returned (e.g., ESRCH or EPERM).
 *
 */
int
dotolwps(procset_t *psp, boolean_t init_is_special,
	 int (*funcp)(lwp_t *, void *, int *), void *arg)
{
	do2lwpargs_t do2lwpargs;
	procset_t dotoprocset;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Check that the procset_t is valid.
	 */
	if ((error = checkprocset(psp, checklwpsetid)) != 0)
		return error;

	do2lwpargs.d_psp = psp;
	do2lwpargs.d_funcp = funcp;
	do2lwpargs.d_arg = arg;

	switch (psp->p_lidtype) {
	case P_CID:
		if (psp->p_op == POP_AND) {
			/*
			 * For those "right-hand" attributes restricting us to
			 * a proper subset of a session, change the selection
			 * criteria to call lwps_inproc() for every process
			 * selected by the "right-hand" attribute, since the
			 * scheduling class of each LWP must be examined while
			 * holding l_mutex.
			 */
			switch (psp->p_ridtype) {
			case P_LWPID:
				setprocset(&dotoprocset, POP_AND,
					   P_PID, u.u_procp->p_pidp->pid_id,
					   P_ALL, P_MYID);
				break;
			case P_PID:
			case P_PGID:
			case P_SID:
				setprocset(&dotoprocset, POP_AND,
					   psp->p_ridtype, psp->p_rid,
					   P_ALL, P_MYID);
				break;
			default:	/* P_{PPID, UID, GID, CID, ALL} */
				/*
				 * Worst case: a linear examination of every
				 * process (going LWP by LWP within each
				 * process) is required.
				 */
				setprocset(&dotoprocset, POP_OR,
					   P_ALL, P_MYID, P_ALL, P_MYID);
				break;
			}
		} else {		/* POP_DIFF, POP_OR, POP_XOR */
			/*
			 * Worst case: a linear examination of every process
			 * (going LWP by LWP within each process) is required.
			 */
			setprocset(&dotoprocset, POP_OR,
				   P_ALL, P_MYID, P_ALL, P_MYID);
		}
		break;
	case P_LWPID:
		if (psp->p_op == POP_AND || psp->p_op == POP_DIFF ||
		    psp->p_ridtype == P_LWPID) {
			/*
			 * The target LWP(s) are restricted to the calling
			 * process.  Alter the selection criteria accordingly.
			 */
			setprocset(&dotoprocset, POP_AND,
				   P_PID, u.u_procp->p_pidp->pid_id,
				   P_ALL, P_MYID);
		} else {		/* POP_OR, POP_XOR */
			/*
			 * Worst case: a linear examination of every process
			 * (going LWP by LWP within each process) is required.
			 */
			setprocset(&dotoprocset, POP_OR,
				   P_ALL, P_MYID, P_ALL, P_MYID);
		}
		break;
	default:		/* P_{PID, PGID, SID, PPID, UID, GID, ALL} */
		/*
		 * The "left-hand" selection attribute is a process-level
		 * attribute.  Based on the "right-hand" selection attributes,
		 * select processes.
		 */
		switch (psp->p_ridtype) {
		case P_CID:
			switch (psp->p_op) {
			case POP_AND:
			case POP_DIFF:
				setprocset(&dotoprocset, POP_AND,
					   psp->p_lidtype, psp->p_lid,
					   P_ALL, P_MYID);
				break;
			default:	/* POP_OR, POP_XOR */
				/*
				 * Worst case: a linear examination of every
				 * process (going LWP by LWP within each
				 * process) is required.
				 */
				setprocset(&dotoprocset, POP_OR,
					   P_ALL, P_MYID, P_ALL, P_MYID);
				break;
			}
			break;
		case P_LWPID:
			switch (psp->p_op) {
			case POP_AND:
				setprocset(&dotoprocset, POP_AND,
					   P_PID, u.u_procp->p_pidp->pid_id,
					   P_ALL, P_MYID);
				break;
			case POP_DIFF:
				setprocset(&dotoprocset, POP_AND,
					   psp->p_lidtype, psp->p_lid,
					   P_ALL, P_MYID);
				break;
			default:	/* POP_OR, POP_XOR */
				/*
				 * Worst case: a linear examination of every
				 * process (going LWP by LWP within each
				 * process) is required.
				 */
				setprocset(&dotoprocset, POP_OR,
					   P_ALL, P_MYID, P_ALL, P_MYID);
				break;
			}
			break;
		default:	/* P_{PID, PGID, SID, PPID, UID, GID, ALL} */
			/*
			 * Both selection attributes are process-level
			 * attributes not requiring individual LWP
			 * inspection.
			 */
			return dotoprocs(psp, init_is_special,
					  alllwps_inproc, (void *)&do2lwpargs);
		}
	}

	return dotoprocs(&dotoprocset, init_is_special,
			  lwps_inproc, (void *)&do2lwpargs);
}
