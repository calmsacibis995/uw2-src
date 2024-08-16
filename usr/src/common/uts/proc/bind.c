/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/bind.c	1.23"
#ident	"$Header: $"

#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <proc/bind.h>
#include <proc/class.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/procset.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/ghier.h>
#include <util/ksynch.h>
#include <util/plocal.h>
#include <util/types.h>
#include <util/var.h>

extern runque_t *global_rq;	/* test code */

extern void dispnonexclusive(engine_t *);
 
struct bind_args {
	idtype_t idtype;
	id_t	id;
	processorid_t processorid;
	processorid_t *obind;
};

int bind(lwp_t *, engine_t *, int);

/*
 * int
 * processor_bind(struct bind_args *uap, rval_t *rvp)
 *
 *	Bind the lwp or set of lwp's to the processor.
 *
 * Calling/Exit State:
 *
 *	The caller does not hold the eng_table_mutex or
 *	the lwp_mutex of the concerned lwp's.
 *	It returns 0 on successful completion.
 *	The error returns are : EPERM if the caller does not have
 *	permission for one or more of the lwp's, 
 *	EINVAL if an invalid idtype, id or processorid are specified,
 *	EIO if the processor specified is not operational,
 *	ESRCH if the lwp's specified by their id's do not exist,
 *	EFAULT if obind is not NULL and points to an invalid address.
 *
 * Description:
 *	This function performs the binding operation against all
 *	the lwp's specified by <idtype, id>.
 *	If processorid is PBIND_NONE, any user binding is removed.
 *	If processorid is PBIND_QUERY, the binding is unchanged
 *	-- this is used to determine the current binding of an lwp.
 *
 *	If processorid is a valid processorid, all lwp's possible
 *	will have their user binding changed so they run on the
 *	processor specified.
 *
 *	If processorid refers to a processor which is not configured,
 *	ESRCH is returned.
 *
 *	If processorid refers to a valid processor, but one which
 *	is not capable of being taken online, EIO is returned.
 *
 *	If processorid refers to a valid processor that is capable of
 *	being taken online, but is currently offlined, EINVAL is returned.
 *
 *	Idtype may be P_LWPID, meaning only the lwp specified
 *	by "id" is to be bound.
 *
 *	Idtype may be P_PID, meaning all lwp's contained in the
 *	process named by "id" are to be bound.
 *
 *	If idtype is neither P_LWPID or P_PID, EINVAL is returned.
 *
 *	If "id" does not refer to an existing  lwp or process,
 *	ESRCH is returned.
 *
 *	The "engine_update" lock (eng_tbl_mutex) is gained in order to
 *	keep the state of the engine stable while it is examined
 *	and lwp's are added to the processor's local run queue.
 *
 *	If idtype is P_LWPID, the lwp is looked up and locked (l_mutex).
 *
 *	If idtype is P_PID, the process is looked up and locked
 *	(p_mutex) via prfind() and all lwp's contained in process
 *	are locked (l_mutex).
 *
 *	bind() is called to do any actual binding or unbinding.
 *
 *	"obind" receives the previous binding of the first lwp
 *	examined. "obind" is mapped to PBIND_NONE if it is NULL
 *	and returned to the user. If idtype is P_PID and if there are
 *	multiple lwp's within that process, the binding of the
 *	first lwp is returned.
 */
/* ARGSUSED */
int
processor_bind(struct bind_args *uap, rval_t *rvp)
{
	int error = 0;
	processorid_t prid = uap->processorid;
	proc_t *proc;
	lwp_t *lwp;
	engine_t *eng = NULL;
	engine_t *oeng;
	pl_t pl1, pl2;
	int i;
	k_lwpid_t lwpid;
	int bind();

	/*
	 * Validate idtype.
	 */
	switch (uap->idtype) {
	case P_LWPID:
	case P_PID:
		break;
	default:
		return(EINVAL);
	}

	pl1 = LOCK(&eng_tbl_mutex, ENG_MINIPL);

	if (prid != PBIND_NONE && prid != PBIND_QUERY) {
		/*
		 * Trying to bind to an engine (as opposed to unbind or
		 * find-out about a binding).
		 * Map the processorid to the engine pointer.
		 * Make sure the resulting engine is suitable for
		 * binding.
		 */
		eng = (engine_t *)PROCESSOR_MAP(uap->processorid);
		if (eng == NULL) {
			error = EINVAL;
			goto out;
		}
		if ((eng->e_flags & E_BAD) != 0) {
			error = EIO;
			goto out;
		}
		if ((eng->e_flags & (E_OFFLINE|E_SHUTDOWN)) != 0) {
			error = EINVAL;
			goto out;
		}
		if ((eng->e_flags & E_EXCLUSIVE) != 0) {
			error = EBUSY;
			goto out;
		}
	}

	switch (uap->idtype) {
	case P_LWPID:
		if (uap->id <= 0) {  /* or uap->id > k_lwpid_t_MAX ?? */
			error = EINVAL;
			break;
		}
		/*
		 * Perform the binding against a particular lwp.
		 */
		pl2 = LOCK(&u.u_procp->p_mutex, PLHI);
		lwpid = (k_lwpid_t)uap->id;
		if (u.u_procp->p_nlwp == 1) { /* only one lwp in process */
			if (lwpid != u.u_lwpp->l_lwpid) { /* nonexistent lwp */
				error = EINVAL;
				UNLOCK(&u.u_procp->p_mutex, pl2);
				goto out;
			}
			lwp = u.u_lwpp;
		} else {
			if(lwpid > (k_lwpid_t)u.u_procp->p_nlwpdir ||
			    (lwp = u.u_procp->p_lwpdir[lwpid - 1]) == NULL) {
				error = EINVAL;
				UNLOCK(&u.u_procp->p_mutex, pl2);
				goto out;
			}
		}
		LOCK(&lwp->l_mutex, PLHI);

		/*
		 * No need to check for permission since the lwp is
		 * within the same process.
		 */
		UNLOCK(&u.u_procp->p_mutex, PLHI);
		oeng = lwp->l_ubind;
		if (prid != PBIND_QUERY) {
			i = bind(lwp, eng, 0);
			if (i != 0)
				error = EIO;  /* ??? CORRECT ??? */
		}
		UNLOCK(&lwp->l_mutex, ENG_MINIPL);
		break;

	case P_PID:
		if (uap->id < 0) {
			error = EINVAL;
			break;
		}
		proc = prfind(uap->id);	/* returns with proc locked */
		if (proc == NULL) {
			error = ESRCH;
			goto out;
		}
		if (MAC_ACCESS(MACEQUAL, u.u_lwpp->l_cred->cr_lid,
			proc->p_cred->cr_lid)) {
			/*
			 * Check MAC access.
			 */
			UNLOCK(&proc->p_mutex, ENG_MINIPL);
			error = ESRCH;
			goto out;
		}
		if (!hasprocperm(proc->p_cred, u.u_lwpp->l_cred)) {
			/*
			 * Don't have permissions to perform bind
			 * lwp's from this process.
			 */
			UNLOCK(&proc->p_mutex, ENG_MINIPL);
			error = EPERM;
			goto out;
		}
		if (proc->p_nlwp == 0) {	/* got a zombie process */
			UNLOCK(&proc->p_mutex, ENG_MINIPL);
			error = ESRCH;
			goto out;
		}
		lwp = proc->p_lwpp;
		oeng = NULL;
		if (prid != PBIND_QUERY) {
			/*
			 * Actually want to bind the lwp's, as opposed
			 * to query the current binding.
			 */
			do {
				LOCK(&lwp->l_mutex, PLHI);
				if (!oeng)
					oeng = lwp->l_ubind;
				bind(lwp, eng, 0);
				UNLOCK(&lwp->l_mutex, PLHI);
				lwp = lwp->l_next;
			} while (lwp != NULL);
		} else
			/*
			 * Just want to query the binding of a random
			 * lwp within the process.
			 */
			oeng = lwp->l_ubind;

		UNLOCK(&proc->p_mutex, ENG_MINIPL);
		break;

	default:
		error = EINVAL;
		goto out;
	}

	if (uap->obind) {
		/*
		 * User wants the old binding back.  Need to map
		 * it to the appropriate return code and copy the
		 * old binding back.
		 */
		processorid_t val;

		if (oeng == NULL)
			val = PBIND_NONE;
		else
			val = PROCESSOR_UNMAP(oeng);

		UNLOCK(&eng_tbl_mutex, pl1);

		if (copyout((caddr_t)&val, (caddr_t)uap->obind,
			sizeof(processorid_t)))
			error = EFAULT;
		return(error);
	}
out:
	UNLOCK(&eng_tbl_mutex, pl1);

	return (error);
}

struct exbind_args {
	idtype_t idtype;
	id_t	*idlist;
	int	list_size;
	processorid_t processorid;
	processorid_t *oprocessoridp;
};

/*
 * int
 * processor_exbind(struct exbind_args *uap, rval_t *rvp)
 *
 *	Exclusively bind the list of lwp specified by "uap->idtype" and
 *	"uap->idlist".
 *
 * Calling/Exit State:
 *
 *	Must be called holding no locks, as this is a system call.
 *
 * Description:
 *
 *	This function exclusively binds a set of processes or lwp's to
 *	an engine.  Afterwards, the engine will exclusively run these
 *	lwp's and these lwp's will run exclusively on the processor
 *	in question.  There are two exceptions:  the processor will
 *	continue to run lwp's executing in bound drivers and lwp's executing
 *	in bound drivers will run on the processor assigned to the driver.
 *
 *	If the processor has previous bindings (exclusive or non-exclusive),
 *	or the caller lacks sufficient privilege to perform the
 *	operation against the engine or against the LWPs in question,
 *	the operation will fail.
 *
 *	Also note, there is a pre-configured limit on the number of
 *	processors which may be exclusively bound (minimum of one), as there
 *	must be some guarantee the general, non-privileged, time-sharing
 *	workload has at least one processor to work with.
 *
 *	To prevent memory deadlocks, the size of "uap->idlist" is limited
 *	to the maximum number of processes or lwp's currently configured
 *	on the system.  To do otherwise would allow the caller to specify
 *	an arbitrary large number of lwp's.
 */
/* ARGSUSED */
int
processor_exbind(struct exbind_args *uap, rval_t *rvp)
{
	int error = 0;
	processorid_t prid = uap->processorid;
	proc_t *proc;
	lwp_t *lwp;
	engine_t *eng = NULL;
	engine_t *oeng;
	pid_t pid;
	pl_t pl;
	id_t *idlist;
	id_t *ip, *endp;
	int list_size;
	int nlwps = 0;
	k_lwpid_t lwpid;
	int bind();

	/*
	 * Validate idtype.
	 */
	switch (uap->idtype) {
	case P_LWPID:
	case P_PID:
		break;
	default:
		return(EINVAL);
	}

	/*
	 * Validate list_size.
	 */
	if (uap->list_size <= 0)
		return(EINVAL);
	if (uap->list_size >= v.v_max_proc_exbind)
		return(E2BIG);

	list_size = uap->list_size;

	/*
	 * Allocate space for and copy-in the array of id's.
	 * We could do this a chunk at a time with a lock-roundtrip
	 * for each chunk, however, for simplicity, we allocate and
	 * copy the whole works in.
	 */
	idlist = (id_t *)kmem_alloc(list_size * sizeof(id_t), KM_SLEEP);

	if (copyin(uap->idlist, idlist, list_size * sizeof(id_t))) {
		kmem_free(idlist, list_size * sizeof(id_t));
		return(EFAULT);
	}

	pl = LOCK(&eng_tbl_mutex, ENG_MINIPL);

	if (prid != PEXBIND_NONE && prid != PEXBIND_QUERY) {
		/*
		 * Trying to bind to an engine (as opposed to unbind or
		 * find-out about a binding).
		 * Make sure the user has the permissions to perform the
		 * operation.
		 * Map the processorid to the engine pointer.
		 * Make sure the resulting engine is suitable for
		 * binding.
		 */
		if (pm_denied(u.u_lwpp->l_cred, P_BIND)) {
			error = EPERM;
			goto out;
		}

		eng = (engine_t *)PROCESSOR_MAP(prid);
		if (eng == NULL) {
			error = EINVAL;
			goto out;
		}
		if ((eng->e_flags & E_BAD) != 0) {
			error = EIO;
			goto out;
		}
		if ((eng->e_flags & (E_OFFLINE|E_SHUTDOWN)) != 0) {
			error = EINVAL;
			goto out;
		}
		if (eng->e_count != 0) {
			/*
			 * The processor in question already has a exclusive
			 * or non-exclusive binding and cannot be bound to
			 * again.
			 */
			error = EBUSY;
			goto out;
		}

		/*
		 * See if it's OK to exclusively bind to this engine.
		 */
		if (!dispexbindok(eng)) {
			/*
			 * Not allowed.
			 */
			error = EBUSY;
		out:
			UNLOCK(&eng_tbl_mutex, pl);
			kmem_free(idlist, list_size * sizeof(id_t));
			return(error);
		}
	}

	switch (uap->idtype) {
	case P_LWPID:
		/*
		 * Perform the binding against a set of LWPs within the
		 * current process.
		 */
		(void) LOCK(&u.u_procp->p_mutex, PLHI);

		for (ip = idlist, endp = ip + list_size; ip < endp; ip++) {
			lwpid = (k_lwpid_t)*ip;
			if (lwpid <= (k_lwpid_t)0 ||
			    lwpid >= (k_lwpid_t)u.u_procp->p_nlwpdir ||
			    (lwp = u.u_procp->p_lwpdir[lwpid - 1]) == NULL) {
				error = EINVAL;
				continue;
			}

			(void) LOCK(&lwp->l_mutex, PLHI);

			nlwps++;

			if (nlwps == 1) {
				/*
				 * Capture the previous binding of
				 * a lwp involved in the operation.
				 */
				oeng = lwp->l_xbind;

				if (prid == PEXBIND_QUERY) {
					/*
					 * No need to perform any
					 * operation, as we've gotten
					 * what we're after.
					 */
					UNLOCK(&lwp->l_mutex, PLHI);
					break;
				}
			}

			if (lwp->l_ubind != NULL) {
				/*
				 * You cannot exclusively bind an lwp
				 * which has an existing non-exclusive
				 * binding.
				 */
				error = EBUSY;
				UNLOCK(&lwp->l_mutex, PLHI);
				continue;
			}

			(void) bind(lwp, eng, 1);

			UNLOCK(&lwp->l_mutex, PLHI);
		}

		UNLOCK(&u.u_procp->p_mutex, ENG_MINIPL);
		break;

	case P_PID:
		for (ip = idlist, endp = ip + list_size; ip < endp; ip++) {
			pid = (pid_t)*ip;
			if (pid < 0) {
				error = EINVAL;
				continue;
			}
			proc = prfind(pid); /* returns with proc->p_mutex held */
			if (proc == NULL) {
				error = EINVAL;
				continue;
			}

			if (MAC_ACCESS(MACEQUAL, u.u_lwpp->l_cred->cr_lid,
				proc->p_cred->cr_lid)) {
				/*
				 * Check MAC access.
				 */
				UNLOCK(&proc->p_mutex, ENG_MINIPL);
				error = ESRCH;
				goto out;
			}
			if (!hasprocperm(proc->p_cred, u.u_lwpp->l_cred)) {
				/*
				 * Don't have permissions to perform bind
				 * lwp's from this process.
				 */
				UNLOCK(&proc->p_mutex, ENG_MINIPL);
				error = EPERM;
				continue;
			}

			if (proc->p_nlwp == 0) {
				/*
				 * This is a zombie process.
				 */
				UNLOCK(&proc->p_mutex, ENG_MINIPL);
				error = EINVAL;
				continue;
			}

			lwp = proc->p_lwpp;

			if (nlwps == 0) {
				/*
				 * This is the first lwp we've encountered,
				 * record its previous binding.
				 */
				oeng = lwp->l_xbind;

				if (prid == PEXBIND_QUERY) {
					/*
					 * No need to perform any
					 * operation, as we've gotten
					 * what we're after.
					 */
					nlwps++;
					UNLOCK(&proc->p_mutex, ENG_MINIPL);
					break;
				}
			}

			/*
			 * Actually want to bind the lwp's, as opposed
			 * to query the current binding.
			 */
			do {
				nlwps++;
				(void) LOCK(&lwp->l_mutex, PLHI);

				if (lwp->l_ubind == NULL) {
					(void) bind(lwp, eng, 1);
				} else {
					/*
					 * You cannot exclusively bind an lwp
					 * which has an existing non-exclusive
					 * binding.
					 */
					error = EBUSY;
				}

				UNLOCK(&lwp->l_mutex, PLHI);
				lwp = lwp->l_next;
			} while (lwp != NULL);

			UNLOCK(&proc->p_mutex, ENG_MINIPL);
		}
		break;

	default:
		UNLOCK(&eng_tbl_mutex, pl);
		return(EINVAL);
	}

	UNLOCK(&eng_tbl_mutex, pl);

	if (nlwps == 0) {
		/*
		 * Couldn't find any lwp's to perform the operation
		 * against.
		 */
		ASSERT(error != 0);
		if (error == EINVAL)
			error = ESRCH;
		kmem_free(idlist, list_size * sizeof(id_t));
		return(error);
	}

	if (uap->oprocessoridp != NULL) {
		/*
		 * User wants the old binding back.  Need to map
		 * it to the appropriate return code and copy the
		 * old binding back.
		 */
		processorid_t val;

		if (oeng == NULL)
			val = PEXBIND_NONE;
		else
			val = PROCESSOR_UNMAP(oeng);

		if (copyout((void *)&val, uap->oprocessoridp, sizeof(val)))
			error = EFAULT;
	}

	kmem_free(idlist, list_size * sizeof(id_t));

	return(error);
}

/*
 * static runque_t *
 * newrq(lwp)
 *
 *	Determine what run-queue the lwp should be on and if we need
 *	to change the run-queue.
 *
 * Calling/Exit State:
 *
 *	Must be called with the engine table mutex (eng_tbl_mutex)
 *	and lwp->l_mutex held.
 *
 * Description:
 *
 *	We walk down the following precedence list for run-queue
 *	assignment:
 *
 *	 o  If the lwp has a kernel binding, take that.
 *	 o  else if the lwp has a exclusive binding, take that
 *	 o  else if the lwp has a non-exclusive binding that
 *	 o  otherwise take the global run-queue.
 *
 *	We return the run-queue the lwp should be on.
 */
/* ARGSUSED */
static runque_t *
newrq(lwp_t *lwp)
{
	runque_t *rq;

	/*
	 * Dedending on the configuration (uniprocessor/multiprocessor)
	 * set the variable 'rq' to the appropriate run queue pointer.
	 */
#ifndef UNIPROC
	if (lwp->l_kbind)
		rq = lwp->l_kbind->e_rqlist[0];
	else if (lwp->l_xbind)
		rq = lwp->l_xbind->e_rqlist[0];
	else if (lwp->l_ubind)
		rq = lwp->l_ubind->e_rqlist[0];
	else
#endif
		rq = global_rq;

	ASSERT(rq != NULL);

	return(rq);
}

/*
 * int bind(lwp_t *lwp, engine_t *eng, int exclusive)
 *
 *	Bind the lwp to the engine.
 *
 * Calling/Exit State:
 *
 *	Must be called with the engine table mutex (eng_tbl_mutex)
 *	and the lwp_mutex held.
 *
 * Description:
 *
 *	This function does the actual binding of the lwp to the engine.
 *	This binding may be either non-exclusive (i.e. the lwp will share
 *	the engine with the general workload) or exclusive (i.e. the lwp
 *	will share the engine only with those lwp's bound in the same
 *	processor_exbind system call).
 *
 *	"eng" has have been previously validated and eng_tbl_mutex
 *	held to prevent its state from changing. "Eng" == NULL implies
 *	the current binding of the lwp should be undone.
 *
 *	We modify the lwp's l_rq to point to the local run-queue of the
 *	engine being bound to or the global run-queue if the binding is
 *	being undone.  We update the counts of bound lwp's in the new
 *	and old engine.  If an exclusive binding is being affected for
 *	the first time, the dispatcher is called to cause the engine to
 *	schedule only from its local run-queue.  If the last exclusive
 *	binding is being undone, the dispatcher is called to cause
 *	the engine to schedule from the global run-queue.
 *
 *	Anyone wanting to place the lwp on a different run-queue,
 *	or wanting to change its l_rq pointer must first hold the lwp's
 *	l_mutex lock. Thus, l_mutex prevents others from putting an lwp
 *	on the run-queue, however it does not prevent an engine from
 *	pulling the lwp off a run-queue.  Thus, we need to be careful
 *	when dealing with SONPROC lwp's.
 */
int
bind(lwp_t *lwp, engine_t *eng, int exclusive)
{
	engine_t *oldeng;
	runque_t *rq;

	if (exclusive) {
		if ((oldeng = lwp->l_xbind) == eng) {
			/*
			 * No-op.  Either binding to the same engine again, or
			 * unbinding an unbound lwp.
			 */
			return (0);
		}
	} else {
		if ((oldeng = lwp->l_ubind) == eng) {
			return (0);
		}
	}

	if (exclusive) {
		lwp->l_xbind = eng;
	} else {
		lwp->l_ubind = eng;
	}

	rq = newrq(lwp);

	if (rq != lwp->l_rq) {
		/*
		 * Need to change the run-queue the lwp is assigned to.
		 */
		switch (lwp->l_stat) {
		case SRUN:
			RUNQUE_LOCK();
	
			/*
			 * We may be racing with an engine scheduling the
			 * lwp in question.  We need to re-check the state of
			 * the lwp, as an engine may have beat us to the
			 * lwp (i.e. we hold l_mutex, they hold run_queue,
			 * they pull lwp off run-queue, release run-queue,
			 * bang on l_mutex.  They update l_stat to indicate
			 * this.  So, even though we hold l_mutex, we still
			 * must be careful when dealing with SRUN lwp's.
			 */
			if (lwp->l_stat == SONPROC)
				goto onproc;
			if (LWP_LOADED(lwp) && !LWP_SEIZED(lwp)) {
				dispdeq(lwp);
				/*
				 * l_rq field already set. Simply add to new 
				 * run queue.
				 */
				lwp->l_rq = rq;
				setfrontrq(lwp);
				preemption(lwp, kpnudge, 0, eng);
			} else
				lwp->l_rq = rq;
			RUNQUE_UNLOCK();
			break;
		case SONPROC:
			RUNQUE_LOCK();
		onproc:
			lwp->l_trapevf |= EVF_L_UBIND; /* set bind-pending flag */
			kpnudge(lwp->l_pri, lwp->l_eng);
			RUNQUE_UNLOCK();

			lwp->l_rq = rq;
			break;
		default:
			lwp->l_rq = rq;
			break;
		}
	}

	if (oldeng) {
		FSPIN_LOCK(&eng_count_mutex);
		oldeng->e_count--;
		FSPIN_UNLOCK(&eng_count_mutex);
		ASSERT(oldeng->e_count >= 0);

		if (exclusive && oldeng->e_count == 0) {
			/*
			 * Indicate to the engine that certain lwp's have been
			 * removed from exclusive binding to the engine.
			 * dispnonenxclusive will check the current number and
			 * re-enable general scheduling if there aren't any.
			 */
			dispnonexclusive(oldeng);
		}
	}

	if (eng) {
		int count;

		FSPIN_LOCK(&eng_count_mutex);
		count = ++eng->e_count;
		FSPIN_UNLOCK(&eng_count_mutex);
		ASSERT(eng->e_count >= 0);

		if (exclusive && count == 1) {
			/*
			 * We've exclusively bound at least one lwp in the
			 * set, thus, we need to inform the dispatcher
			 * this engine must be devoted entirely to its local
			 * run-queue.
			 */
			dispexclusive(eng);
		}
	}

	return (0);
}


/*
 * void bind_create(lwp_t *creator, lwp_t *lwp)
 *
 *	Cause "lwp" to inherit bindings of the "creator".
 *
 * Calling/Exit State:
 *
 *	Called while holding the l_mutex of both the creator
 *	and created lwp.
 *
 * Description:
 *
 *	Called by the creator of a new lwp.  bind_create causes the
 *	lwp to inherit the bindings of the creator, causing the
 *	necessary per-engine side-effects.
 *
 *	There is the potential for lock ordering problems here, as the
 *	proper order is "eng_tbl_mutex", then "l_mutex".  Fortunately,
 *	the effect of binding inheritance is mild, as any special actions
 *	which had to be applied to the engine were done so when the
 *	creator was bound.  Also, the creator does the proper inheritence
 *	of the l_rq.  Thus, the only concern left is to properly increment
 *	the binding count associated with the engine.  This can be done with
 *	a low-level fspin.
 */
void
bind_create(lwp_t *creator, lwp_t *lwp)
{
	engine_t *eng;

	ASSERT(LOCK_OWNED(&lwp->l_mutex));
	ASSERT(LOCK_OWNED(&creator->l_mutex));
	ASSERT(lwp->l_kbind == NULL);

	if (creator->l_xbind == NULL && creator->l_ubind == NULL)
		return;

	if ((eng = creator->l_xbind) != NULL) {
		ASSERT(creator->l_ubind == NULL);
		ASSERT(eng->e_count > 0 && eng->e_flags & E_EXCLUSIVE);
		FSPIN_LOCK(&eng_count_mutex);
		eng->e_count++;
		FSPIN_UNLOCK(&eng_count_mutex);
		lwp->l_xbind = eng;
		return;
	}

	if ((eng = creator->l_ubind) != NULL) {
		ASSERT(creator->l_xbind == NULL);
		ASSERT(eng->e_count > 0);
		FSPIN_LOCK(&eng_count_mutex);
		eng->e_count++;
		FSPIN_UNLOCK(&eng_count_mutex);
		lwp->l_ubind = eng;
		return;
	}

	return;
}


/*
 * void
 * bind_exit(lwp_t *lwp)
 *
 *	Unbind the exiting lwp.
 *
 * Calling/Exit State:
 *
 *	Called without holding eng_tbl_mutex or lwp_mutex.  The
 *	lwp can no longer be located as it has been removed from
 *	all lists.
 *
 * Description:
 *
 *	Called when an lwp is exiting. It unbinds the named
 *	lwp from the engine to which it is bound.  Since the lwp
 *	can no longer be found, we're assured the lwp cannot be
 *	re-bound after our return.
 */
void
bind_exit(lwp_t *lwp)
{
	pl_t pl;

	ASSERT(lwp->l_kbind == NULL);

	if (lwp->l_xbind == NULL && lwp->l_ubind == NULL)
		return;

	pl = LOCK(&eng_tbl_mutex, ENG_MINIPL);
	(void)LOCK(&lwp->l_mutex, PLHI);

	if (lwp->l_xbind)
		bind(lwp, NULL, 1);
	if (lwp->l_ubind)
		bind(lwp, NULL, 0);

	UNLOCK(&lwp->l_mutex, ENG_MINIPL);
	UNLOCK(&eng_tbl_mutex, pl);
}


/*
 * engine_t *
 * kbind(engine_t *eng)
 *
 *	Kernel-bind the caller to the engine.
 *
 * Calling/Exit State:
 *
 *	Caller must not be holding either its lwp_mutex 
 *	or the runque-lock.
 *
 * Description:
 *
 *	This function binds the caller to the engine that is
 *	being passed as the argument. It sets the l_kbind
 *	and the l_rq fields in the caller's lwp structure and
 *	then causes the lwp to be placed on the local run queue
 *	of the target engine. The lwp is placed at the front of
 *	the local run queue, the justification being that it was
 *	already running while calling this function. preemption()
 *	is called to maintain the scheduler invariant and then
 *	it relinquishes the present processor by calling qswtch.
 *	Device drivers will have to hold eng_tbl_mutex or some
 *	equivalent lock to prevent offlining of an engine while
 *	some lwp is kbound to it.
 */
engine_t *
kbind(engine_t *eng)
{
	lwp_t *lwp = u.u_lwpp;
	engine_t *oldkeng;
	pl_t pl;

#ifdef UNIPROC
	oldkeng = lwp->l_kbind;
	lwp->l_kbind = eng;
#else
	pl = LOCK(&lwp->l_mutex, PLHI);
	oldkeng = lwp->l_kbind;
	lwp->l_kbind = eng;
	lwp->l_rq = eng->e_rqlist[0];
	if (eng != l.eng) {
		/*
		 * If on a different processor, go to the
		 * bound processor.
		 */
		lwp->l_stat = SRUN;
		if (LWP_SEIZED(lwp)) {
			/*
			 * If we're seized, we simply call
			 * swtch.  Swtch knows how to handle things.
			 */
			swtch(lwp);
			return(oldkeng);
		}

		RUNQUE_LOCK();
		setfrontrq(lwp);
		preemption(lwp, kpnudge, 0, eng);
		qswtch(lwp); /* returns with l_mutex and RUNQUE unlocked */
	} else
		UNLOCK(&lwp->l_mutex, pl);
#endif  /* UNIPROC */
	return(oldkeng);
}


/*
 * engine_t *
 * kunbind(engine_t *oldkeng)
 *
 *	Kernel-unbind the caller from the engine.
 *
 * Calling/Exit State:
 *
 *	Caller must not be holding either its lwp_mutex 
 *	or the runque-lock.
 *
 * Description:
 *
 *	This function unbinds the caller from the engine that the
 *	caller is presently bound to. If the oldkeng argument is
 *	NULL, then a check is made to see if there is any user
 *	binding present. If so, the l_rq member is set to the user
 *	bound engine, else it is set to point to the global run queue.
 *	If the oldkeng argument is non-NULL, then the run-queue pointer
 *	is set to local run queue of the engine being passed in.
 *	It then puts the lwp on the appropriate run queue (at the front,
 *	since the lwp was already running), calls preemption to
 *	maintain scheduler invariant and then gives up the present
 *	processor via a call to qswtch.
 */
void
kunbind(engine_t *oldkeng)
{
 
	lwp_t *lwp = u.u_lwpp;
	pl_t pl;
#ifndef UNIPROC
	engine_t *engp = NULL;
#endif

#ifdef UNIPROC
	lwp->l_kbind = oldkeng;
	return;
#else
	pl = LOCK(&lwp->l_mutex, PLHI);
	lwp->l_kbind = oldkeng;

	if (oldkeng != NULL) {
		lwp->l_rq = oldkeng->e_rqlist[0];
		engp = oldkeng;
	} else if (lwp->l_xbind != NULL) {
		lwp->l_rq = lwp->l_xbind->e_rqlist[0];
		engp = lwp->l_xbind;
	} else if (lwp->l_ubind != NULL) {
		lwp->l_rq = lwp->l_ubind->e_rqlist[0];
		engp = lwp->l_ubind;
	} else {
		lwp->l_rq = global_rq;
		engp = NULL;
	}

	if (engp == NULL || l.eng == engp) {
		/* not bound or already on the correct engine */
		UNLOCK(&lwp->l_mutex, pl);
	} else {
		lwp->l_stat = SRUN;
		if (LWP_SEIZED(lwp)) {
			/*
			 * If we're seized, we simply call
			 * swtch.  Swtch knows how to handle things.
			 */
			swtch(lwp);
			return;
		}

		RUNQUE_LOCK();
		setfrontrq(lwp);
		preemption(lwp, kpnudge, 0, engp);
		qswtch(lwp); /* returns with lwp_mutex and RUNQUE unlocked */
	}
#endif  /* UNIPROC */
}

/*
 * int
 * bindproc(processorid_t processorid)
 *	User bind the process to the engine mapped to the processorid.
 *
 * Calling/Exit State:
 *	<processorid> is the engine to which the process is to be bound.
 */
int 
bindproc(processorid_t processorid)
{
	int error = 0;
	proc_t *proc = u.u_procp;
	lwp_t *lwp;
	engine_t *eng = NULL;
	pl_t opl1;

	opl1 = LOCK(&eng_tbl_mutex, ENG_MINIPL);

	eng = (engine_t *)PROCESSOR_MAP(processorid);

	ASSERT(eng < engine_Nengine);
	
	lwp = proc->p_lwpp;
	LOCK(&proc->p_mutex, PLHI);

	if (proc->p_bindnum) {
		if (proc->p_bind != eng) {
			UNLOCK(&proc->p_mutex, ENG_MINIPL);
			UNLOCK(&eng_tbl_mutex, opl1);
			return(EBUSY);
		}
	} else {
		do {
			LOCK(&lwp->l_mutex, PLHI);
			bind(lwp, eng, 0);
			UNLOCK(&lwp->l_mutex, PLHI);
			lwp = lwp->l_next;
		} while (lwp != NULL);

		proc->p_bind = eng;
	 }
	 proc->p_bindnum++;
         UNLOCK(&proc->p_mutex, ENG_MINIPL);
	 UNLOCK(&eng_tbl_mutex, opl1);
	 return(0);
}

/*
 * int
 * unbindproc(void)
 *	Unbind the engine to which the process was bound earlier.
 *
 * Calling/Exit State:
 *	None.
 */
void
unbindproc(void)
{
	int error = 0;
	proc_t *proc = u.u_procp;
	lwp_t *lwp;
	pl_t opl1;

	ASSERT(proc->p_bindnum != 0);

	opl1 = LOCK(&eng_tbl_mutex, ENG_MINIPL);

	LOCK(&proc->p_mutex, PLHI);
	lwp = proc->p_lwpp;

	proc->p_bindnum--;
	if (proc->p_bindnum == 0) {
		do {
			LOCK(&lwp->l_mutex, PLHI);
			bind(lwp, NULL, 0);
			UNLOCK(&lwp->l_mutex, PLHI);
			lwp = lwp->l_next;
		} while (lwp != NULL);
		proc->p_bind = NULL;
	}

	UNLOCK(&proc->p_mutex, ENG_MINIPL);
	UNLOCK(&eng_tbl_mutex, opl1);
}
