/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/thread/sched.c	1.11"

/*
 * The interfaces contained here are for applications that need to
 * exercise control over scheduling at the thread library level.
 * A caching mechanism provided here allows to improve performance
 * by caching class id/class name mappings and thus eliminating 
 * unnecessary system calls. Upto MAX_CACHED_SCHEDS class id/class name 
 * mappings are allowed to be cached by a process.
 *
 * In general, to avoid race conditions, scheduler lock has to be 
 * held until requested scheduling policy and/or priority is set/retrieved.
 */

#include <sys/types.h> 
#include <sys/priocntl.h>
#include <sys/fppriocntl.h>
#include <sys/tspriocntl.h>
#include <values.h>
#include <string.h>
#include <trace.h>
#include <libthread.h>

#define LWP_MUTEX_ISLOCKED(/* lwp_mutex_t * */ mp) ((mp)->lock) /* XXX */

static int _thr_get_class_id(int *, char *);

/*
 * Cached class name/id information.
 * It allows to avoid extraneous system calls.
 */

#define MAX_CACHED_SCHEDS 16	/* Size of cache scheduling policies	*/

struct _thr_scheds {
	char	cl_name[PC_CLNMSZ];
};

static volatile struct _thr_scheds _thr_scheds[MAX_CACHED_SCHEDS]; /* Class id cache */

/*
 * Cached class name/id information is protected by _thr_schedlock
 */

lwp_mutex_t _thr_sched_lock;

#define LOCK_SCHED	_lwp_mutex_lock(&_thr_sched_lock);
#define UNLOCK_SCHED      _lwp_mutex_unlock(&_thr_sched_lock);

/*
 * Other definitions required by thread scheduling interfaces are in 
 * thread.h
 */

/*
 * int 
 * thr_setscheduler(thread_t tid, const sched_param_t *param)
 *	set the scheduling policy and policy-specific parameters of a 
 *	library thread within the current process.
 *
 * Description:
 *	tid is a library thread ID.
 *	param argument must be previously initialized with the
 *	desired scheduling policy identifier and parameters
 *	appropriate to the policy being set.
 *	Valid values for policy identifiers are SCHED_FIFO, SCHED_RRi,
 *	SCHED_TS, or SCHED_OTHER.
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During processing, signal handlers are disabled and sched lock is aquired.
 *	The target thread lock may need to be acquired.
 *
 * Return Values/Exit State:
 *	On exit, no locks are held and signal handlers are enabled.
 *	On success, thr_setscheduler() returns 0. 
 *	Upon failure, it returns appropriate errno value.
 *
 */

int
thr_setscheduler(thread_t tid, const sched_param_t *param)
{
	thread_desc_t	*tp;
	lwpid_t		lwpid;
	pcparms_t	pcparms;
	int		cid;
	int		prio;
	int		hashpri;
	thread_desc_t   *ctp = curthread;
	int		rval = 0;
	int		activate_lwp = INVALID_PRIO;
        int             index;

	PRINTF1("thr_setscheduler:(1) tid: %d\n", tid);
	if (param == NULL) {
		TRACE4(ctp, TR_CAT_THREAD, TR_EV_THR_SETSCHED, TR_CALL_ONLY, 
		   tid, param->policy, param->policy_params, EINVAL);
		return(EINVAL);
	}

	/* Validate thread's id */

	_thr_sigoff(ctp);
	tp = _thr_get_thread(tid);

	if (tp == NULL) {
		TRACE4(ctp, TR_CAT_THREAD, TR_EV_THR_SETSCHED, TR_CALL_ONLY, 
		   tid, param->policy, param->policy_params, ESRCH);
		_thr_sigon(ctp);
		return(ESRCH);
	}  else if (tp->t_state == TS_ZOMBIE) {
		UNLOCK_THREAD(tp);
		TRACE4(ctp, TR_CAT_THREAD, TR_EV_THR_SETSCHED, TR_CALL_ONLY, 
		   tid, param->policy, param->policy_params, ESRCH);
		_thr_sigon(ctp);
		return(ESRCH);
	}

	/* Get scheduling class ID */

	switch(param->policy) {
	case SCHED_FIFO:
	case SCHED_RR:
		PRINTF2("thr_setscheduler: (4) tid: %d param->policy: 0x%x\n", tid, param->policy);
		if (!ISBOUND(tp)) {
			/*
			 * multiplexed thread cannot be SCHED_FIFO
			 * or SCHED_RR.
			 */
			PRINTF2("thr_setscheduler: (5) tid: %d, tid policy = 0x%x\n", tid, param->policy);

			rval = ENOSYS;
			break;
		}
		/*
		 * Convert a bound thread to its LWP.
		 */

		lwpid = LWPID(tp);

		/*
		 * SCHED_FIFO and SCHED_RR thread library scheduling 
		 * policies map into kernel's FP (fixed priority).
		 */

		if ((rval = _thr_get_class_id(&cid, "FP")) != 0) {
			break;
		}
		PRINTF1("thr_setscheduler: (7) got cid %d\n", cid);

		/* 
		 * Set thread's scheduling policy and
		 * the policy-specific parameters as required.
		 */

		pcparms.pc_cid = cid;

		if (param->policy == SCHED_FIFO) {
			prio = ((struct fifo_param *)param->policy_params)->prio;
			((fpparms_t *)pcparms.pc_clparms)->fp_pri = prio;
			((fpparms_t *)pcparms.pc_clparms)->fp_tqnsecs = FP_TQINF;
		} else { /* SCHED_RR policy */

			prio = ((struct rr_param *)param->policy_params)->prio;
			((fpparms_t *)pcparms.pc_clparms)->fp_pri = prio;
			((fpparms_t *)pcparms.pc_clparms)->fp_tqsecs = RR_DFLTSECS;
			((fpparms_t *)pcparms.pc_clparms)->fp_tqnsecs = RR_DFLTNSECS;
		}
		if (priocntl(P_LWPID, lwpid, PC_SETPARMS, &pcparms) == -1) {
			rval = errno;
				PRINTF1("thr_setscheduler: (8) priocntl failed - errno = %d\n", rval);
			break;
		}
		tp->t_pri = prio;
		PRINTF1("thr_setscheduler: (9) tp->t_pri: %d\n", tp->t_pri);
		break;
	case SCHED_TS:	/* time sharing policy */
		prio = ((struct ts_param *)param->policy_params)->prio;

		if (!ISBOUND(tp)) {
			/* 
			 * For a multiplexed thread the only thing that
			 * we need to do is to set priority in the thread's
			 * descriptor.
			 */
			if (tp->t_pri == prio) { /* Check trivial case */
				PRINTF1("thr_setscheduler: (10) tp->t_pri == prio: %d\n", tp->t_pri);
				break;
			}

			/* 
			 * Check if all threads have same priority 
			 * until now; if so, initialize the
			 * user priority array.
			 */
			if (_thr_preempt_off ==  B_FALSE) {
				LOCK_COUNTER;
				if (_thr_userpricnt == 1) {
                                		int t_pri_index;
					HASH_PRIORITY(tp->t_pri, t_pri_index);
					HASH_PRIORITY(prio, index);
					_thr_prioq_init(t_pri_index, prio);
                                	} else { /* _thr_userpricnt > 1 */
                                                ASSERT(_thr_userpricnt > 1);
                                        	HASH_PRIORITY(prio, index);
                                        	ADD_TO_PRIO_ARRAY(index);
                                        	HASH_PRIORITY(tp->t_pri, index);
                                        	REM_FROM_PRIO_ARRAY(index);
				}
				UNLOCK_COUNTER;
			}

			switch(tp->t_state) {
			case TS_ONPROC:
				tp->t_pri = prio;
				PRINTF3("thr_setscheduler: (12a)  tid: %d, t_state: 0x%x, t_pri: %d\n",
				tp->t_tid, tp->t_state, tp->t_pri);
				if (_thr_preempt_off == B_FALSE) {
					HASH_PRIORITY(tp->t_pri,
					    hashpri);
					 if (hashpri < _thr_maxpriq) {
						/* preempt tp */
						if (ctp == tp) {
						     PREEMPT_SELF(tp);
						     _thr_sigon(ctp);
						     rval = 0;
						     return(rval);
						} else {
						      PREEMPT_THREAD(tp);
						}
					}
				}
				break;
			case TS_SUSPENDED:
				tp->t_pri = prio;
				PRINTF3("thr_setscheduler: (12b)  tid: %d, t_state: 0x%x, t_pri: %d\n",
				tp->t_tid, tp->t_state, tp->t_pri);
				break;
			case TS_SLEEP:
			case TS_RUNNABLE:
				activate_lwp = 
				   _thr_requeue(tp, prio);
				PRINTF3("thr_setscheduler: (12c)  tid: %d, t_state: 0x%x, t_pri: %d\n",
				  tp->t_tid, tp->t_state, 
				  tp->t_pri);
				break;
			default:
				_thr_panic("thr_setscheduler: illegal thread's state\n");
				rval = EINVAL;
				break;
			}
		} else {

			/*
			 * For a bound thread convert the thread 
			 * to its LWP.
			 */
			lwpid = LWPID(tp);

			/*
			 * SCHED_TS thread library scheduling 
			 * policy maps into kernel's TS (time scharing).
			 */

			if ((rval = _thr_get_class_id(&cid, "TS")) != 0) {
				break;
			}
			PRINTF1("thr_setscheduler: (14) cid: %d\n", cid);
			pcparms.pc_cid = cid;
			((tsparms_t *)pcparms.pc_clparms)->ts_uprilim = TS_NOCHANGE;
			((tsparms_t *)pcparms.pc_clparms)->ts_upri = prio;
			if (priocntl(P_LWPID, lwpid, PC_SETPARMS, &pcparms) == -1) {
				rval = errno;
				PRINTF1("thr_setscheduler: (15) priocntl failed - errno: %d\n", rval);
				break;
			} else {
				tp->t_pri = prio;
			}

	 	}
		break;
	default:
		/*
		 * An unknown scheduling policy.
		 */
		rval = EINVAL;
		PRINTF("thr_setscheduler: (16) unknown scheduling policy\n");
		break;
	}
	UNLOCK_THREAD(tp);
	if (activate_lwp != INVALID_PRIO) {
		/*
		 * if activate_lwp is set, it means that the thread was
		 * runnable and we increased its priority.  In this case, it
		 * is necessary to attempt preemption via _thr_activate_lwp().
		 */
		_thr_activate_lwp(activate_lwp);
	}
	TRACE4(ctp, TR_CAT_THREAD, TR_EV_THR_SETSCHED, TR_CALL_ONLY, 
	   tid, param->policy, param->policy_params, rval);
	_thr_sigon(ctp);
	PRINTF2("thr_setscheduler: (17) returns %d prio: %d\n", rval,tp->t_pri);
	return(rval);
}

/*
 * int
 * thr_getscheduler(thread_t tid, sched_param_t *param)
 *	returns scheduling policy and policy-specific parameters of a
 *	thread within the calling process.
 *
 * Description:
 *	tid is a library thread ID.
 *	param argument points to sched_param structure in
 *	which any policy-specific parameters are returned.
 *	
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During processing, signal handlers are disabled and sched lock 
 *	may be aquired.
 *
 * Return Values/Exit State:
 *	On exit, no locks are held and signal handlers are enabled.
 *	On success, thr_getscheduler() returns 0. 
 *	Upon failure, it returns appropriate errno value.
 *
 */
int 
thr_getscheduler(thread_t tid, sched_param_t *param)
{
	thread_desc_t   *tp;
	lwpid_t		lwpid;
	pcinfo_t        pcinfo;
	pcparms_t       pcparms;
	volatile char	*cl_name;
	thread_desc_t   *ctp = curthread;
	
	if (param == NULL) {
		TRACE4(ctp, TR_CAT_THREAD, TR_EV_THR_GETSCHED, TR_CALL_ONLY, 
		   tid, 0, 0, EINVAL);
		return(EINVAL);
	}

	/* Validate thread's id */

	_thr_sigoff(ctp);
	tp = _thr_get_thread(tid);

	if (tp == NULL) {
		TRACE4(ctp, TR_CAT_THREAD, TR_EV_THR_GETSCHED, TR_CALL_ONLY, 
		   tid, 0, 0, ESRCH);
		_thr_sigon(ctp);
		return(ESRCH);
	} else if (tp->t_state == TS_ZOMBIE) {
		UNLOCK_THREAD(tp);
		TRACE4(ctp, TR_CAT_THREAD, TR_EV_THR_GETSCHED, TR_CALL_ONLY, 
		   tid, 0, 0, ESRCH);
		_thr_sigon(ctp);
		return(ESRCH);
	}
		
	/* In case of an unbound thread, get priority 
	 * from the thread descriptor.
	 */
	if (!ISBOUND(tp)) {
		param->policy = SCHED_TS;
		((struct ts_param *)param->policy_params)->prio = tp->t_pri;
		UNLOCK_THREAD(tp);
		TRACE4(ctp, TR_CAT_THREAD, TR_EV_THR_GETSCHED, TR_CALL_ONLY, 
		   tid, param->policy, 
		    param->policy_params, 0);
		_thr_sigon(ctp);
		PRINTF2("thr_getscheduler: (3) tid: %d is multiplexed tp->t_pri = %d\n", tp->t_tid, tp->t_pri);
		return(0);
	}

	/* 
	 * In case of a bound thread, convert thread to its LWP.
	 */

	lwpid = LWPID(tp);
	pcparms.pc_cid = PC_CLNULL;

	if (priocntl(P_LWPID, lwpid, PC_GETPARMS, &pcparms) == -1) {
		UNLOCK_THREAD(tp);
		TRACE4(ctp, TR_CAT_THREAD, TR_EV_THR_GETSCHED, TR_CALL_ONLY, 
		   tid, 0, 0, errno);
		_thr_sigon(ctp);
PRINTF2("thr_getscheduler: (4) tid: %d priocntl returned: %d\n", tp->t_tid, errno);
		return(errno);
	}
	/* 
	 * Obtain the name that corresponds to this scheduling class id.
	 * Only call priocntl if either class is >= MAX_CACHED_SCHEDS
	 * or the mapping for this class is not cached.
	 */
	LOCK_SCHED;
	if (pcparms.pc_cid >= MAX_CACHED_SCHEDS || 		
		_thr_scheds[pcparms.pc_cid].cl_name[0] == '\0') {
		pcinfo.pc_cid = pcparms.pc_cid;
		(void) priocntl(0, 0, PC_GETCLINFO, &pcinfo);
		if (pcparms.pc_cid < MAX_CACHED_SCHEDS) {
			(void)strcpy(
				(char *)_thr_scheds[pcparms.pc_cid].cl_name,
					pcinfo.pc_clname);
			cl_name = _thr_scheds[pcparms.pc_cid].cl_name;
		} else {
			/* class id is >= MAX_CACHED_SCHEDS 
			 * we don't cache mappings for these ids.
			 */
			cl_name = pcinfo.pc_clname;
		}
	} else {
		/* class id is < MAX_CACHED_SCHEDS and mapping is cached. */
		cl_name = _thr_scheds[pcparms.pc_cid].cl_name;
	}

	/*
	 * Now it is OK to release scheduler lock.
	 */
	UNLOCK_SCHED;

	UNLOCK_THREAD(tp);
	_thr_sigon(ctp);

	if (strcmp((char *)cl_name, "TS") == 0) {
		param->policy = SCHED_TS;
		((struct ts_param *)param->policy_params)->prio = 
		((tsparms_t *)pcparms.pc_clparms)->ts_upri;		
		TRACE4(ctp, TR_CAT_THREAD, TR_EV_THR_GETSCHED, TR_CALL_ONLY, 
		   tid, param->policy, param->policy_params, 0);
		return(0);
	}
	if (strcmp((char *)cl_name, "FP") == 0) {
		if (((fpparms_t *)pcparms.pc_clparms)->fp_tqnsecs == FP_TQINF) {
			param->policy = SCHED_FIFO;
			((struct fifo_param *)param->policy_params)->prio =
			((fpparms_t *)pcparms.pc_clparms)->fp_pri;
		} else if ((((fpparms_t *)pcparms.pc_clparms)->fp_tqsecs
		    == RR_DFLTSECS) && 
		    (((fpparms_t *)pcparms.pc_clparms)->fp_tqnsecs
		    == RR_DFLTNSECS)) {

			param->policy = SCHED_RR;
			((struct rr_param *)param->policy_params)->prio =
			((fpparms_t *)pcparms.pc_clparms)->fp_pri;
		}
		TRACE4(ctp, TR_CAT_THREAD, TR_EV_THR_GETSCHED, TR_CALL_ONLY, 
		   tid, param->policy, param->policy_params, 0);
		return(0);
	}
	param->policy = SCHED_UNKNOWN;
	(void) memcpy(param->policy_params, pcparms.pc_clparms, POLICY_PARAM_SZ);
	TRACE4(ctp, TR_CAT_THREAD, TR_EV_THR_GETSCHED, TR_CALL_ONLY, 
	   tid, param->policy, param->policy_params, 0);
	return(0);
}

/*
 * void
 *	get_rr_interval(timestruc_t *rr_time)
 *	returns time quantum for SCHED_RR policy.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	The function get_rr_interval returns
 *	default values for RR time quantum:
 *	RR_DFLTSECS  - Default secs 
 *	RR_DFLTNSECS - Default nanoseconds
 *	These values are set in the header file thread.h
 *
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During processing, no locks are acquired. 
 *
 * Return Values/Exit State:
 *	On exit, no locks are held. 
 */

void
thr_get_rr_interval(timestruc_t *rr_time)
{
	rr_time->tv_sec = RR_DFLTSECS;
	rr_time->tv_nsec = RR_DFLTNSECS;
	TRACE0(0, TR_CAT_THREAD, TR_EV_THR_GET_RR, TR_CALL_ONLY);
}

/*
 * int
 * thr_setprio(thread_t tid, int prio)
 *	sets the priority of a specified library thread within a
 *	calling process.
 *
 * Description:
 *	This operation serves as a shorthand to manipulate thread's 
 *	priorities within the current scheduling class.
 *	
 * Parameter/Calling State:
 *      On entry, no locks are held.
 *      During processing, signal handlers are disabled and sched lock
 *      may be aquired.
 *
 * Return Values/Exit State:
 *      On exit, no locks are held and signal handlers are enabled.
 *      On success, thr_setprio() returns 0.
 *      Upon failure, it returns appropriate errno value.
 */

int
thr_setprio(thread_t tid, int prio)
{
	thread_desc_t	*tp;
	lwpid_t         lwpid;
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;
	volatile char	*cl_name;
	thread_desc_t	*ctp = curthread;
	int		rval = 0;
	int		activate_lwp = INVALID_PRIO;
	int		hashpri;
        int             index;

	    /* Validate thread's id */

	_thr_sigoff(ctp);
	tp = _thr_get_thread(tid);

	if (tp == NULL) {
		TRACE3(ctp, TR_CAT_THREAD, TR_EV_THR_SETPRIO, TR_CALL_ONLY, 
		   tid, prio, ESRCH);
		_thr_sigon(ctp);
		return(ESRCH);
	}  else if (tp->t_state == TS_ZOMBIE) {
		UNLOCK_THREAD(tp);
		TRACE3(ctp, TR_CAT_THREAD, TR_EV_THR_SETPRIO, TR_CALL_ONLY, 
		   tid, prio, ESRCH);
		_thr_sigon(ctp);
		return(ESRCH);
	}

	if (!ISBOUND(tp)) {
		/*
		 * The only scheduling policy allowed for unbound
		 * threads is SCHED_TS.
		 * Only need to set priority in the thread descriptor.
		 */

		/* first check if prio is valid */
		if (prio < 0 || prio == MAXINT) {
			UNLOCK_THREAD(tp);
                        TRACE3(ctp, TR_CAT_THREAD, TR_EV_THR_SETPRIO,
                           TR_CALL_ONLY, tid, prio, EINVAL);
                        _thr_sigon(ctp);
                        return(EINVAL);
		}
	
		/* Check trivial case */
		if (tp->t_pri == prio) { 
			UNLOCK_THREAD(tp);
			TRACE3(ctp, TR_CAT_THREAD, TR_EV_THR_SETPRIO,
			   TR_CALL_ONLY, tid, prio, 0);
			_thr_sigon(ctp);
			return(0);
		}

		/* 
		 * Check if all threads have same priority 
		 * until now; if so, initialize the
		 * user priority array.
		 */
		if (_thr_preempt_off == B_FALSE) {
			LOCK_COUNTER;
			if (_thr_userpricnt == 1) {
				int t_pri_index;
				HASH_PRIORITY(tp->t_pri, t_pri_index);
				HASH_PRIORITY(prio, index);
				_thr_prioq_init(t_pri_index, prio);
			} else { /* _thr_userpricnt > 1 */
				ASSERT(_thr_userpricnt > 1);
                        	HASH_PRIORITY(prio, index);
                        	ADD_TO_PRIO_ARRAY(index);
                        	HASH_PRIORITY(tp->t_pri, index);
                        	REM_FROM_PRIO_ARRAY(index);
			}
			UNLOCK_COUNTER;
		}

		switch(tp->t_state) {
		case TS_ONPROC:
			tp->t_pri = prio;
			PRINTF3("thr_setprio: (4a)  tid: %d, t_state: 0x%x, t_pri: %d\n",
			tp->t_tid, tp->t_state, tp->t_pri);
			if (_thr_preempt_off == B_FALSE) {
				HASH_PRIORITY(tp->t_pri, hashpri);
				if (hashpri < _thr_maxpriq) {
					/* preempt tp */
					if (ctp == tp) {
						PREEMPT_SELF(tp);
						TRACE3(ctp, TR_CAT_THREAD, 
						   TR_EV_THR_SETPRIO,
						   TR_CALL_ONLY, 
						   tid, prio, rval);
						_thr_sigon(ctp);
						return(0);
					} else {
						PREEMPT_THREAD(tp);
					}
				}
			}
			break;
		case TS_SUSPENDED:
			tp->t_pri = prio;
			PRINTF3("thr_setprio: (4b)  tid: %d, t_state: 0x%x, t_pri: %d\n",
			tp->t_tid, tp->t_state, tp->t_pri);
			break;
		case TS_SLEEP:
		case TS_RUNNABLE:
			activate_lwp = _thr_requeue(tp, prio);
                        PRINTF3("thr_setprio: (4c) tid: %d, t_state: 0x%x, t_pri: %d\n",
                	  tp->t_tid, tp->t_state, tp->t_pri);
			break;
		default:
			_thr_panic("thr_setprio: illegal thread's state\n");
			rval = EINVAL;
			break;
		}
		UNLOCK_THREAD(tp);

		/*
		 * if activate_lwp is set, it means that the thread was
		 * runnable and we increased its priority.  In this case, it
		 * is necessary to attempt preemption via _thr_activate_lwp().
		 */
		if (activate_lwp != INVALID_PRIO) {
			_thr_activate_lwp(activate_lwp);
		}
		TRACE3(ctp, TR_CAT_THREAD, TR_EV_THR_SETPRIO, TR_CALL_ONLY, 
		   tid, prio, rval);
		_thr_sigon(ctp);
                 PRINTF3("thr_setprio: (5) tid: %d, t_state: 0x%x, t_pri: %d\n",
                tp->t_tid, tp->t_state, tp->t_pri);
		return(rval);
	}

	/* 
	 * For bound threads convert thread to its LWP.
	 */

	lwpid = LWPID(tp);

	pcparms.pc_cid = PC_CLNULL;

	if (priocntl(P_LWPID, lwpid, PC_GETPARMS, &pcparms) == -1) {
		UNLOCK_THREAD(tp);
		TRACE3(ctp, TR_CAT_THREAD, TR_EV_THR_SETPRIO, TR_CALL_ONLY, 
		   tid, prio, errno);
		_thr_sigon(ctp);
		PRINTF1("thr_setprio: (6) priocntl errno: %d\n", errno);
		return(errno);
	}

	PRINTF2("thr_setprio: (7) returns %d prio: %d\n", rval, tp->t_pri);
	/* 
	 * Obtain the name that corresponds to this scheduling class id.
	 * We are calling priocntl only if either class is >= MAX_CACHED_SCHEDS
	 * or the mapping for this class is not cached.
	 */

	LOCK_SCHED;
	PRINTF1("thr_setprio: (8) pcparms.pc_cid: %d\n", pcparms.pc_cid);
	if (pcparms.pc_cid >= MAX_CACHED_SCHEDS || 		
	    _thr_scheds[pcparms.pc_cid].cl_name[0] == '\0') {
		pcinfo.pc_cid = pcparms.pc_cid;
		(void) priocntl(0, 0, PC_GETCLINFO, &pcinfo);
		PRINTF1("thr_setprio: (9) priocntl errno: %d\n", errno);
			if (pcparms.pc_cid < MAX_CACHED_SCHEDS) {
				(void)strcpy(
				   (char *)_thr_scheds[pcparms.pc_cid].cl_name,
					     pcinfo.pc_clname);
				cl_name = _thr_scheds[pcparms.pc_cid].cl_name;
			} else {
				/* class id is >= MAX_CACHED_SCHEDS 
				 * we don't cache mappings for these ids.
				 */
				cl_name = pcinfo.pc_clname;
			}
	} else {
		/* class id is < MAX_CACHED_SCHEDS and mapping is cached. */
		cl_name = _thr_scheds[pcparms.pc_cid].cl_name;
	}
	UNLOCK_SCHED;
	/* Set thread's priority */
	if (strcmp((char *)cl_name, "TS") == 0) {
		((tsparms_t *) pcparms.pc_clparms)->ts_uprilim = TS_NOCHANGE;

		((tsparms_t *)pcparms.pc_clparms)->ts_upri = prio;
	} else if (strcmp((char *)cl_name, "FP") == 0) {
		((fpparms_t *)pcparms.pc_clparms)->fp_pri = prio;
		if (((fpparms_t *)pcparms.pc_clparms)->fp_tqnsecs != FP_TQINF) {
			/* if time quantum != FP_TQINF it means
			 * sched. policy is RR (not FIFO).
			 * Since, for sched RR time quantum is
			 * changing with priority we must reset the
			 * time quantum to what thread library
			 * defines it to be.
			 */
			
			((fpparms_t *)pcparms.pc_clparms)->fp_tqsecs = RR_DFLTSECS;
			((fpparms_t *)pcparms.pc_clparms)->fp_tqnsecs = RR_DFLTNSECS;
		} else {
			/* 
			 * it is sched FIFO
			 */
			((fpparms_t *)pcparms.pc_clparms)->fp_tqnsecs = FP_NOCHANGE;
		}
	} else {
		/*
		 * An unknown scheduling policy.
		 * We don't deal with those at this time.
		 */
		rval = EINVAL;
		goto prio_out;
	}
	if (priocntl(P_LWPID, lwpid, PC_SETPARMS, &pcparms) == -1) {
		rval = errno;
		PRINTF1("thr_setprio: (10) priocntl errno: %d\n", errno);
	} 
	if (rval == 0) {
		tp->t_pri = prio;
	}

prio_out:
	UNLOCK_THREAD(tp);
	TRACE3(ctp, TR_CAT_THREAD, TR_EV_THR_SETPRIO, TR_CALL_ONLY,
	   tid, prio, rval);
	_thr_sigon(ctp);
	return(rval);
}

/*
 * int
 *	thr_getprio(thread_t tid, int *prio)
 *	returns the priority of a specified library thread within a
 *	calling process.
 *
 * Description:
 *	This operation returns the priority of a specified library
 *	thread within a calling process.
 *	
 * Parameter/Calling State:
 *      On entry, no locks are held.
 *      During processing, signal handlers are disabled and sched lock
 *      may be aquired.
 *
 * Return Values/Exit State:
 *      On exit, no locks are held and signal handlers are enabled.
 *      On success, thr_getprio() returns 0.
 *      Upon failure, it returns appropriate errno value.
 */

int
thr_getprio(thread_t tid, int *prio)
{
	thread_desc_t	*tp;
	lwpid_t         lwpid;
	pcinfo_t	pcinfo;
	pcparms_t	pcparms;
	volatile char	*cl_name;
	thread_desc_t	*ctp  = curthread;

	    /* Validate thread's id */

	_thr_sigoff(ctp);
	tp = _thr_get_thread(tid);

	if (tp == NULL) {
		TRACE3(ctp, TR_CAT_THREAD, TR_EV_THR_GETPRIO, TR_CALL_ONLY,
		   tid, 0, ESRCH);
		_thr_sigon(ctp);
		return(ESRCH);
	}  else if (tp->t_state == TS_ZOMBIE) {
		UNLOCK_THREAD(tp);
		TRACE3(ctp, TR_CAT_THREAD, TR_EV_THR_GETPRIO, TR_CALL_ONLY,
		   tid, 0, ESRCH);
		_thr_sigon(ctp);
		return(ESRCH);
	}

	if (!ISBOUND(tp)) {

		/* In case of a multiplexed thread get 
		 * thread's priority from the thread descriptor.
		 */
		*prio = tp->t_pri;
		UNLOCK_THREAD(tp);
		TRACE3(ctp, TR_CAT_THREAD, TR_EV_THR_GETPRIO, TR_CALL_ONLY,
		   tid, *prio, 0);
		_thr_sigon(ctp);
		PRINTF2("thr_getprio: (1) multiplexed tid: %d prio: %d\n", tid, tp->t_pri);
		return(0);
	}

	/* For bound threads convert thread to its LWP and
	 * release sched lock.
	 */

	lwpid = LWPID(tp);
	
	pcparms.pc_cid = PC_CLNULL;
	
	if (priocntl(P_LWPID, lwpid, PC_GETPARMS, &pcparms) == -1) {
		UNLOCK_THREAD(tp);
		TRACE3(ctp, TR_CAT_THREAD, TR_EV_THR_GETPRIO, TR_CALL_ONLY,
		   tid, 0, errno);
		_thr_sigon(ctp);
		PRINTF1("thr_getprio: (2)  priocntl errno: %d\n", errno);
		return(errno);
	}
	/* 
	 * Obtain the name that corresponds to this scheduling class id.
	 * Only call priocntl if either class is >= MAX_CACHED_SCHEDS
	 * or the mapping for this class is not cached.
	 */
	LOCK_SCHED;
	if (pcparms.pc_cid >= MAX_CACHED_SCHEDS || 		
	    _thr_scheds[pcparms.pc_cid].cl_name[0] == '\0') {
		pcinfo.pc_cid = pcparms.pc_cid;
		(void) priocntl(0, 0, PC_GETCLINFO, &pcinfo);
		if (pcparms.pc_cid < MAX_CACHED_SCHEDS) {
			(void)strcpy(
				(char *)_thr_scheds[pcparms.pc_cid].cl_name,
				     pcinfo.pc_clname);
			cl_name = _thr_scheds[pcparms.pc_cid].cl_name;
		} else {
			/* class id is >= MAX_CACHED_SCHEDS 
			 * we don't cache mappings for these ids.
			 */
			cl_name = pcinfo.pc_clname;
		}
	} else {
		/* class id is < MAX_CACHED_SCHEDS and mapping is cached. */
		cl_name = _thr_scheds[pcparms.pc_cid].cl_name;
	}
	/*
	 * Now it is OK to release the scheduler lock.
	 */

	UNLOCK_SCHED;

	if (strcmp((char *)cl_name, "TS") == 0) {
		*prio = ((tsparms_t *)pcparms.pc_clparms)->ts_upri;

	} else if (strcmp((char *)cl_name, "FP") == 0) {
		*prio = ((fpparms_t *)pcparms.pc_clparms)->fp_pri;
	} else {
		UNLOCK_THREAD(tp);
		TRACE3(ctp, TR_CAT_THREAD, TR_EV_THR_GETPRIO, TR_CALL_ONLY,
		   tid, 0, EINVAL);
		_thr_sigon(ctp);
		return(EINVAL);
	}
	UNLOCK_THREAD(tp);
	TRACE3(ctp, TR_CAT_THREAD, TR_EV_THR_GETPRIO, TR_CALL_ONLY,
	   tid, *prio, 0);
	_thr_sigon(ctp);
	return(0);
}

/*
 * void
 *	thr_yield(void)
 *	yield the processor.
 *
 * Description:
 *	The calling thread's execution is halted and the 
 *	thread placed in a runnable state.
 *
 * Parameter/Calling State:
 *      On entry, no locks are held.
 *      During processing, signal handlers are disabled and thread lock,
 *	and sched lock may be aquired.
 *
 * Return Values/Exit State:
 *      On exit, no locks are held and signal handlers are enabled.
 *      On success, thr_getprio() returns 0.
 *      Upon failure, it returns appropriate errno value.
 */

void 
thr_yield(void)
{
	thread_desc_t	*ctp = curthread;

	TRACE0(ctp, TR_CAT_THREAD, TR_EV_THR_YIELD, TR_CALL_ONLY);

	if(ISBOUND(ctp)) {

		/*
		 * For a bound thread call priocntl ...
		 */

		(void) priocntl(P_LWPID, P_MYID, PC_YIELD, 0);
	} else {

	/*
	 * ... and for an unbound thread do the library switch.
	 */
		_thr_sigoff(ctp);
		LOCK_THREAD(ctp);
		ctp->t_state = TS_RUNNABLE;
		_thr_swtch(1,ctp);
		_thr_sigon(ctp);
	}
}

/*
 * int
 * _thr_get_class_id(int *cid, char *cl_name)
 *
 *	converts a bound thread's scheduling class name into its corresponding
 *	scheduling class id.
 *
 * Description:
 *	First argument is a bound thread's scheduling class id returned by
 *	the function. Second argument is a pointer to scheduling class name 
 *	passed to the function.
 *
 * Parameter/Calling State:
 *      On entry, caller's signal handlers are disabled, no locks need to be
 *	held.
 *      During processing, signal handlers are disabled and sched lock 
 *	is aquired.
 *
 * Return Values/Exit State:
 *      On exit, no locks are held and signal handlers still disabled.
 *      On success, function returns 0 and cid is set to the scheduling class id.
 *      Upon failure, function returns appropriate errno value, and the
 *	scheduling class id is set to -1.
 */

static int
_thr_get_class_id(int *cid, char *cl_name)
{
	pcinfo_t	pcinfo;
	int		scid;
	int		rval = 0;

	LOCK_SCHED;

	for (scid = 0; scid < MAX_CACHED_SCHEDS; scid++) {
		if (strcmp(cl_name, (char *)_thr_scheds[scid].cl_name) == 0)
			break;
	}
	if (scid >= MAX_CACHED_SCHEDS) {
		/* class id is not cached.
		 * we have to retrieve it.
		 */
		(void) strcpy(pcinfo.pc_clname, cl_name);
		if (priocntl(0, 0, PC_GETCID, &pcinfo) == -1) {
			rval = errno;
			PRINTF1("_thr_get_class_id: (1) priocntl failed - errno:  %d\n", rval);
			*cid = -1;
			UNLOCK_SCHED;
			return(rval);
		}
		scid = pcinfo.pc_cid;
		/*
		 * Only class ids in the range 
		 * 0 <= scid < MAX_CACHED_SCHEDS
		 * are cached.
		 */
		if (scid < MAX_CACHED_SCHEDS) {
			(void) strcpy((char *)_thr_scheds[scid].cl_name,
				      cl_name);
		}
	}
	*cid = scid;
	UNLOCK_SCHED;
	return(rval);
}

