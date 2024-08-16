/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:common/lib/libthread/thread/timer.c	1.11"

#include <libthread.h>
#include <unistd.h>
#include <sys/time.h>

#pragma weak alarm = _alarm
#pragma weak getitimer = _getitimer
#pragma weak setitimer = _setitimer


STATIC int _thr_addcallout(callo_t *);
STATIC void * _thr_servecallout(void *);
STATIC void _thr_tvfix(struct timeval *);
STATIC void _thr_tsfix(timestruc_t *);
STATIC int _thr_itimer_check(struct timeval *);
STATIC int _thr_delcallout(callo_t *cop);
void _thr_timer_threxit(thread_desc_t *);
void _thr_dump_callo(callo_t *);

/*
 * _thr_calloutserver is a pointer to the bound thread used to service
 * the callout queue _thr_callout_queue.
 */

thread_desc_t	*_thr_calloutserver;

/*
 * _thr_callout_mutex is an LWP mutex lock protecting the global
 * callout queue _thr_callout_queue.
 * Threads must acquire this mutex before accessing the callout queue.
 * The callout server will also acquire this mutex before waiting on
 * the condition variable _thr_callout_cond for a timer expiration.
 */

lwp_mutex_t	_thr_callout_mutex;

/*
 * _thr_callout_cond is an LWP condition variable on which the callout
 * server waits for a timer expiration or a new callout request.
 */

lwp_cond_t	_thr_callout_cond;

/*
 * _thr_co_stat is a global flag to indicate if the callout 
 * server is running. _thr_co_stat is protected by _thr_callout_mutex.
 */

boolean_t	_thr_co_stat = B_FALSE;

/*
 * _thr_callout_queue is the head of a circular, doubly-linked list
 * of all callout requests. The callout queue is maintained in
 * ascending order of the absolute expiration time stored in a callout
 * request.
 */

callo_t	_thr_callout_queue;
STATIC callo_t *_thr_callout_qp = &_thr_callout_queue;

/*
 * int
 * _thr_setcallout(callo_t *cop, const timestruc_t *abstime,
 *      const timestruc_t *interval, void (*func)(void *arg), void *arg)
 *
 * The function initializes a callout request, places it on the
 * callout queue and sets its state to CO_TIMER_ON.
 *
 * Parameter/Calling State:
 *	On entry, signal handlers are disabled.
 *	During processing, signal handlers are disabled, 
 *	and _thr_callout_mutex is acquired. 
 *	Parameter cop is a pointer to the callout structure which is to be set.
 *	Parameter abstime is a pointer to timestruc_t structure containing 
 *	the absolute time value for the callout cop.
 *	Parameter interval is a pointer to timestruc_t structure containing 
 *	interval time value for the callout cop.
 *	Parameter func is a pointer to the function to be called by the callout
 *	server upon callout expiration.
 *	Parameter arg is the argument to be passed to the function func.
 *
 * Return Values/Exit State:
 *	On exit, no locks are held. The callout request state is set
 *	to CO_TIMER_ON.
 *	On success the function returns 0; otherwise it returns error code. 
 *
 */

int
_thr_setcallout(callo_t *cop, const timestruc_t *abstime,
                const timestruc_t *interval, void (*func)(void *arg), void *arg)
{
	int rval;

	ASSERT(THR_ISSIGOFF(curthread));
	cop->co_abstime = *abstime;
	if (interval == NULL) {
		cop->co_interval.tv_sec = 0;
		cop->co_interval.tv_nsec = 0;
	} else {
		cop->co_interval = *interval;
	}
	cop->co_func = func;
	cop->co_arg = arg;
	LOCK_CALLOUTQ;
	rval = _thr_addcallout(cop);
	UNLOCK_CALLOUTQ;
	return(rval);
}

/*
 * int
 * _thr_rmcallout(callo_t *cop)
 *
 * The function removes a callout request from the callout queue and
 * sets the state of the callout request to CO_TIMER_OFF.
 *
 * Parameter/Calling State:
 *	On entry, signal handlers are disabled and no locks are held.
 *	During processing, signal handlers are disabled, 
 *	and _thr_callout_mutex is acquired. 
 *	Parameter cop is a pointer to the callout to be removed.
 *
 * Return Values/Exit State:
 *	On exit, no locks are held.
 *	Function returns old status of callout cop.
 */

int
_thr_rmcallout(callo_t *cop)
{
	int oldstat;

	ASSERT(THR_ISSIGOFF(curthread));

	LOCK_CALLOUTQ;
	oldstat = _thr_delcallout(cop);
	UNLOCK_CALLOUTQ;
	return (oldstat);
}

/*
 * int
 * _thr_addcallout(callo_t *cop)
 *
 * The function places a callout request on the callout queue.
 *
 * Parameter/Calling State:
 *	On entry, signal handlers are disabled and the callout queue
 *	lock is held.
 *	During processing, no locks are acquired.
 *	Parameter cop is a pointer to the callout to be added to th ecallout queue.
 *
 * Return Values/Exit State:
 *	On exit, signal handlers remain disabled and the callout
 *	queue lock is held.
 *	On success, function returns 0; otherwise, it returns error code.
 */

STATIC int
_thr_addcallout(callo_t *cop)
{
	callo_t *ccop;
	thread_desc_t *t;
	int rval;

	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(IS_CALLOUTQ_LOCKED);
	ASSERT(cop->co_stat != CO_TIMER_ON);
	PRINTF1("_thr_addcallout: tid %d\n", cop->co_arg);
	
	if (_thr_co_stat == B_FALSE) {
		if ((rval = _thr_alloc_tls(NULL, 0, &t)) == 0) {
			PRINTF("_thr_addcallout: creating timer server (_thr_alloc_tls) failed\n");
			return(rval);
		}
		_thr_calloutserver = t;
		t->t_tid = TIMERID;
		t->t_nosig = 1;
		sigfillset(&t->t_hold);
		_thr_sigdiffset(&t->t_hold, &_thr_sig_programerror);

		t->t_usropts = THR_BOUND;
		if ((rval = _thr_new_lwp(t, _thr_servecallout, (void *)0)) != 0) {
			/* free library allocated stack */
			if ((t->t_flags & T_ALLOCSTK)) {
				_thr_free_stack(t->t_stk, (int)t->t_stksize);
			}
			PRINTF("_thr_addcallout: creating timer server (_thr_new_lwp) failed\n");
			return(rval);
		}
		t->t_state = TS_ONPROC;
		_lwp_continue(LWPID(t));
		THRQ_INIT(_thr_callout_qp);	
		_thr_co_stat = B_TRUE;
		PRINTF("_thr_addcallout: created timer thread\n");
	}
	cop->co_stat = CO_TIMER_ON;

	/* place the callout request on the callout queue in ascending order of
	 * the timer value (co_abstime);
	 */

	if (THRQ_ISEMPTY(_thr_callout_qp)) {
		PRINTF1("_thr_addcallout: added callout tid %d\n", cop->co_arg);
		_thr_callout_add_end(_thr_callout_qp, cop);
	} else {
		ccop =  (callo_t *) _thr_callout_qp->co_link.thrq_next;
		PRINTF1("_thr_addcallout: first thread on callout queue %d\n",ccop->co_arg);
		while (ccop !=  _thr_callout_qp) {
			if (cop->co_abstime.tv_sec < ccop->co_abstime.tv_sec ||
			(cop->co_abstime.tv_sec == ccop->co_abstime.tv_sec &&
			cop->co_abstime.tv_nsec < ccop->co_abstime.tv_nsec))
				break;
			ccop = (callo_t *) ccop->co_link.thrq_next;
		}
		if (ccop  == _thr_callout_qp)
			_thr_callout_add_end(ccop, cop);
		else
			_thr_callout_ins_after((callo_t *)ccop->co_link.thrq_prev, cop);
	}

#ifdef THR_DEBUG
	{ 
		callo_t * cp;
		PRINTF("callout queue\n");
		cp  =  (callo_t *) _thr_callout_qp->co_link.thrq_next;
		 while (cp !=  _thr_callout_qp) {
			PRINTF5("tid %d\t time sec %ld\t nsec %ld\t inter sec %ld\t nsec %ld\n",
cp->co_arg, cp->co_abstime.tv_sec,cp->co_abstime.tv_nsec,cp->co_interval.tv_sec,cp->co_interval.tv_nsec);
			cp  = (callo_t *) cp->co_link.thrq_next;
		}
	} /* END DEBUG ONLY */
#endif

	if ((callo_t *) _thr_callout_qp->co_link.thrq_next == cop) {

		/*
	 	 * if the callout request is placed at the front of the queue
	 	 * signal the callout server to reset callout server's timer.
	 	 */

		 PRINTF("_thr_addcallout: signalling timer server\n");
		_lwp_cond_signal(&_thr_callout_cond);
	}
	return(0);
}

/*
 * int
 * _thr_delcallout(callo_t *cop)
 *
 * The function removes a callout request from the callout queue and
 * sets the state of the callout request to CO_TIMER_OFF.
 *
 * Parameter/Calling State:
 *	On entry, signal handlers are disabled and the callout queue
 *	lock is held.
 *	During processing, no locks are acquired.
 *	Parameter cop is a pointer to callout to be removed from the callout queue.
 *
 * Return Values/Exit State:
 *	On exit, callout queue lock is held, the callout request is 
 *	removed from the callout queue, and its state is set to 
 *	CO_TIMER_OFF.
 *	Function returns old status of the removed callout.
 */

STATIC int
_thr_delcallout(callo_t *cop)
{
	int oldstat;

	ASSERT(THR_ISSIGOFF(curthread));
	ASSERT(IS_CALLOUTQ_LOCKED);
	
	oldstat = cop->co_stat;
	
	/*
	 * If the callout request is active
	 * remove the callout request from the callout queue;
	 */
	
	if (cop->co_stat == CO_TIMER_ON) {
		_thr_callout_rem_from_q(cop);				
	}
	/*
	 * change the state of the callout request to inactive
	 */
	cop->co_stat = CO_TIMER_OFF;
	return (oldstat);
}

/*
 * void *
 * _thr_servecallout(void *)
 *
 * This is the function executed by the callout server daemon.
 *
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During processing, signal handlers are disabled, 
 *	_thr_callout_mutex is acquired, and the function waits on
 *	callout condition variable.
 *
 * Return Values/Exit State:
 *	The function executes in an infinite loop and never exits.
 *
 */

/* ARGSUSED */
STATIC void *
_thr_servecallout(void *arg)
{
	timestruc_t	clock;
	timestruc_t	timerval;
	callo_t		*cop;
	int		rval;
	thread_desc_t	*ctp = curthread;

	_thr_sigoff(ctp);

	PRINTF("_thr_servecallout: timer server starts\n");

 	/* 
	 * obtain the system clock in timestruc_t format and 
	 * initialize clock with the system clock value.
	 * This is an optimization. We need to call hrestime() only once
	 * when the callout server starts up. Once the callout clock is
	 * initialized, it is reset at each callout expiration.
	 */
	
	hrestime(&clock);

	/*
	 * This is where the real work starts.
	 */

	LOCK_CALLOUTQ;
	for (;;) {
		if (THRQ_ISEMPTY(_thr_callout_qp)) { 
			PRINTF("_thr_servecallout: _thr_callout_qp empty\n");
			_lwp_cond_wait(&_thr_callout_cond, &_thr_callout_mutex);
			PRINTF("_thr_servecallout: is up\n");
			continue;
		}
		cop = (callo_t *) _thr_callout_qp->co_link.thrq_next;
		if (cop->co_abstime.tv_sec > clock.tv_sec || 
			(cop->co_abstime.tv_sec == clock.tv_sec &&
			cop->co_abstime.tv_nsec > clock.tv_nsec)) {

			/*
			 * If the timer value of the first callout request
			 * is greater than clock time, wait for the expiration 
			 */
			timerval.tv_sec = cop->co_abstime.tv_sec;
			timerval.tv_nsec = cop->co_abstime.tv_nsec;
			
			rval = _lwp_cond_timedwait(&_thr_callout_cond,
			&_thr_callout_mutex, &timerval);
			
			if (rval == EINVAL) {
				_thr_dump_callo(cop);
				_thr_panic("_thr_servecallout: _lwp_cond_timedwait returned EINVAL");
			}
			if (rval == ETIME) {
				clock = timerval;
			}
			continue;
		}
		cop = _thr_callout_rem_first(_thr_callout_qp);
		cop->co_stat = CO_TIMEDOUT;
		(*cop->co_func)(cop->co_arg);
		if (cop->co_interval.tv_sec != 0 ||
			cop->co_interval.tv_nsec != 0) {
			cop->co_abstime.tv_sec += cop->co_interval.tv_sec;
			cop->co_abstime.tv_nsec += cop->co_interval.tv_nsec;
			_thr_tsfix(&cop->co_interval);
			(void) _thr_addcallout(cop);
		}
	}
}

/*
 * void
 * _thr_tsfix(timestruc_t *tsp)
 *
 * adjust timestruc_t appropriately.
 *
 * Parameter/Calling State:
 *	On entry, signal handlers are disabled and queue lock is held.
 *	During processing, signal handlers are disabled, 
 *	no new locks are acquired.
 * Return Values/Exit State:
 *	On exit, signal handlers are disabled and queue lock is held.
 *	Function doesn't return any value.
 *
 */

STATIC void 
_thr_tsfix(timestruc_t *tsp)
{
	if (tsp->tv_nsec >= NANOSEC) {
		++tsp->tv_sec;
		tsp->tv_nsec -= NANOSEC;
	}
	if (tsp->tv_nsec < 0) {
		--tsp->tv_sec;
		tsp->tv_nsec += NANOSEC;
	}
}

/*
 * void
 * _thr_timeup(thread_t tid)
 *
 * The function executed by the callout service when a timer expires.
 *
 * Parameter/Calling State:
 *	On entry, signal handlers are disabled, and the callout queue lock
 *	is held.
 *	During processing, signal handlers are disabled, 
 *	_thr_tidvec lock and the target thread's descriptor lock 
 *	is acquired.
 *	Argument tid is the thread id to which the function sends SIGALRM.
 *
 * Return Values/Exit State:
 *	On exit, no locks are held.
 *	Function doesn't return any value.
 *
 */

STATIC void
_thr_timeup(thread_t tid)
{
	thread_desc_t *tp;
	int rval = INVALID_PRIO;

	PRINTF1("_thr_timeup: entered for tid %d\n", tid);
	ASSERT(THR_ISSIGOFF(curthread));
	if((tp = _thr_get_thread(tid)) != NULL) {
		ASSERT(tp->t_state != TS_ZOMBIE);

		rval = _thr_sigsend(tp, SIGALRM);
		UNLOCK_THREAD(tp);
		if (rval != INVALID_PRIO) {
			_thr_activate_lwp(rval);
		}
	}
}

/*
 * unsigned
 * _alarm(unsigned sec)
 *
 *	This function is a wrapper of the alarm(2) system call to
 *	set the alarm clock in seconds.
 *	If the calling thread is a bound thread, the function invokes
 *	the alarm(2) system call.
 *	If the calling thread is a multiplexed thread, the function
 *	disables signal handlers and locks the callout queue; if the callout
 *	of the calling thread is active, the function removes it from the
 *	callout queue and saves the current alarm clock value; it
 *	initializes the alarm callout request; places the initialized
 *	request on the callout queue; and before returning, the function
 *	enables signal handlers and unlocks the callout queue.
 *
 * Parameter/Calling State:
 *	On entry, no locks are held.
 *	During processing, signal handlers are disabled and 
 *	the callout queue lock is acquired. 
 *
 * Return Values/Exit State:
 *	On exit, no locks are held.
 */

unsigned
_alarm(unsigned sec)
{
	thread_desc_t   *ctp = curthread;
	int		rval = 0;
	timestruc_t	clocktime;
	callo_t		*cop;

	if (ISBOUND(ctp)) {
		return((*_sys_alarm)(sec));
	}
	  
	/* the calling thread is a multiplexed thread */
	hrestime(&clocktime);
	_thr_sigoff(ctp);
	LOCK_CALLOUTQ;
	cop = &ctp->t_callo_alarm;
	if (_thr_delcallout(cop) == CO_TIMER_ON) {
		rval = cop->co_abstime.tv_sec - clocktime.tv_sec;
		if (rval < 0) {
			rval = 0;
		}
	}
	if (sec) {
		/*
		 * We don't check an overflow on the calculation 
		 * (system clock + sec) since the kernel does not check either.
		 * In addition, alarm does not have an error.
		 */
		cop->co_abstime.tv_sec = clocktime.tv_sec + sec;
		cop->co_abstime.tv_nsec = clocktime.tv_nsec;
		cop->co_interval.tv_sec = 0;
		cop->co_interval.tv_nsec = 0;
		cop->co_func = _thr_timeup;
		cop->co_arg = (void *) ctp->t_tid;
		if(_thr_addcallout(cop)) {
			UNLOCK_CALLOUTQ;
			_thr_sigon(ctp);
			_thr_panic("creating timer server failed\n");
			return(0);
		}
	}
	UNLOCK_CALLOUTQ;
	_thr_sigon(ctp);
	return (rval);
}

/*
 * int _setitimer(int which, struct itimerval *nitp, struct itimerval *oitp)
 * 
 * This function is a wrapper of the libc setitimer(2) system call to set 
 * the interval timer.
 *
 * Parameter/Calling State:
 *      On entry, no locks are held.
 *      During processing, signal handlers are disabled,
 *      and _thr_callout_mutex is acquired.
 *
 * Return Values/Exit State:
 *      On exit, no locks are held. 
 */

int
_setitimer(int which, struct itimerval *nitp, struct itimerval *oitp)
{
	thread_desc_t   *ctp = curthread;
	struct timeval  clocktime;
	callo_t		*cop;
	int		rval;
	
	if (ISBOUND(ctp)) {
		return((*_sys_setitimer)(which, nitp, oitp));
	}
	  
	/*
	 * the calling thread is a multiplexed thread.
	 */

	if (which != ITIMER_REAL) {
		errno = EINVAL;
		return (-1);
	}

	/*
	 * check the validity of *nitp and fix them if necessary.
	 */

	if (_thr_itimer_check(&nitp->it_value) || _thr_itimer_check(&nitp->it_interval)) {
		errno = EINVAL;
		return (-1);
	}

	/*
	 * clock starts ticking here.
	 */

	(void) gettimeofday(&clocktime, (struct timezone *) NULL);

	_thr_sigoff(ctp);
	LOCK_CALLOUTQ;
	cop = &ctp->t_callo_realit;
	rval = _thr_delcallout(cop);
		
	if (oitp != NULL) {
		if (rval == CO_TIMER_ON) {
			oitp->it_value.tv_sec = cop->co_abstime.tv_sec  - clocktime.tv_sec;
			oitp->it_value.tv_usec = (cop->co_abstime.tv_nsec * 
				MICROSEC/NANOSEC) - clocktime.tv_usec;
			_thr_tvfix(&oitp->it_value);	
			oitp->it_interval.tv_sec = cop->co_interval.tv_sec;
			oitp->it_interval.tv_usec = cop->co_interval.tv_nsec * 
				MICROSEC/NANOSEC;
			_thr_tvfix(&oitp->it_interval);
		} else {
			oitp->it_value.tv_sec = 0;
			oitp->it_value.tv_usec = 0;
			oitp->it_interval.tv_sec = 0;
			oitp->it_interval.tv_usec = 0;
		}
	}
	if (nitp->it_value.tv_sec == 0 && nitp->it_value.tv_usec == 0) {
			cop->co_abstime.tv_sec = 0;
			cop->co_abstime.tv_nsec = 0;
			cop->co_interval.tv_sec = 0;
			cop->co_interval.tv_nsec = 0;
	} else {
		cop->co_abstime.tv_sec = clocktime.tv_sec + nitp->it_value.tv_sec;
		cop->co_abstime.tv_nsec = clocktime.tv_usec * (NANOSEC/MICROSEC);
		cop->co_interval.tv_sec = nitp->it_interval.tv_sec;
		cop->co_interval.tv_nsec = nitp->it_interval.tv_usec * (NANOSEC/MICROSEC);
		cop->co_func = _thr_timeup;
		cop->co_arg = (void *) ctp->t_tid;
		if(_thr_addcallout(cop) != 0) {
			/* 
			 * _thr_addcallout can fail because e.g. we cannot
			 * create lwp callout server. 
			 */ 
			UNLOCK_CALLOUTQ;
			 _thr_sigon(ctp);
			errno = EAGAIN;
			return(-1);
		}
	}
	UNLOCK_CALLOUTQ;
	_thr_sigon(ctp);
	return (0);
}

/*
 * int _getitimer(int which, struct itimerval *oitp)
 * 
 * This function is a wrapper of the libc getitimer(2) system call to get 
 * the interval timer.
 *
 * Parameter/Calling State:
 *      On entry, no locks are held.
 *      During processing, signal handlers are disabled,
 *      and _thr_callout_mutex is acquired.
 *
 * Return Values/Exit State:
 *      On exit, no locks are held. 
 */

int
_getitimer(int which, struct itimerval *oitp)
{
	thread_desc_t   *ctp = curthread;
	struct timeval	clocktime;
	callo_t         *cop;

	if (ISBOUND(ctp)) {
		return((*_sys_getitimer)(which, oitp));
	}

	/*
	 * the calling thread is a multiplexed 
	 */
	if (which != ITIMER_REAL) {
		errno = EINVAL;
		return (-1);
	}
	 _thr_sigoff(ctp);
	LOCK_CALLOUTQ;
	 cop = &ctp->t_callo_realit;
	if (cop->co_stat == CO_TIMER_ON) {
		gettimeofday(&clocktime, (struct timezone *) NULL);
		oitp->it_value.tv_sec =
			cop->co_abstime.tv_sec - clocktime.tv_sec;
		oitp->it_value.tv_usec =
			(cop->co_abstime.tv_nsec * MICROSEC/NANOSEC)
			- clocktime.tv_usec;
		_thr_tvfix(&oitp->it_value);
		oitp->it_interval.tv_sec = cop->co_interval.tv_sec;
		oitp->it_interval.tv_usec = cop->co_interval.tv_nsec *
			MICROSEC/NANOSEC;
		_thr_tvfix(&oitp->it_interval);
	} else {
		oitp->it_value.tv_sec = 0;
		oitp->it_value.tv_usec = 0;
		oitp->it_interval.tv_sec = 0;
		oitp->it_interval.tv_usec = 0;
	}
	UNLOCK_CALLOUTQ;
	_thr_sigon(ctp);
	return (0);
}

/*
 *int _thr_itimer_check(struct timeval *tvp) 
 * 
 * This function checks the validity of the given struct timeval, and make
 * sure it represents at least as much time as the granularity of the system
 * clock.
 *
 * Parameter/Calling State:
 *      On entry, no locks need to be held.
 *      During processing, no locks are acquired.
 *
 * Return Values/Exit State:
 *      On exit, no locks are held. 
 */

STATIC int
_thr_itimer_check(struct timeval *tvp)
{
	static long tick_in_usec = 0;
	long clk_tck;
	if (tick_in_usec == 0) {
		clk_tck = sysconf(_SC_CLK_TCK);
		tick_in_usec = 1000 * 1000 / clk_tck;
	}
	if (tvp->tv_sec < 0 || tvp->tv_sec > 100*1000*1000
	    || tvp->tv_usec < 0 || tvp->tv_usec >= 1000*1000) {
		return (EINVAL);
	}

	/*
	 * Make sure that the time is zero or at least one tick.
	 */
	if (tvp->tv_sec == 0 && tvp->tv_usec != 0 && tvp->tv_usec < tick_in_usec) {
		tvp->tv_usec = tick_in_usec;
	}
	return (0);
}

/*
 * void _thr_timer_threxit(thread_desc_t *tp)
 * 
 * The function removes thread callouts from the callout queue.
 *
 * Parameter/Calling State:
 *      On entry, signal handlers are disabled.
 *      During processing, the callout queue lock is acquired.
 *
 * Return Values/Exit State:
 *      On exit, no locks are held, thread's callouts are removed from 
 *	the callout queue, and its state is set to CO_TIMER_OFF. 
 */

void
_thr_timer_threxit(thread_desc_t *tp)
{
	ASSERT(THR_ISSIGOFF(curthread));

	/*
	 * This is an optimization to remove thread's callouts.
	 */

	LOCK_CALLOUTQ;
	_thr_delcallout(&tp->t_callo_alarm);
	_thr_delcallout(&tp->t_callo_realit);
	_thr_delcallout(&tp->t_callo_cv);
	UNLOCK_CALLOUTQ;
}

/*
 * void _thr_tvfix(tvp)
 *      Adjust a struct timeval to have a reasonable format (the
 *      number of microseconds should be positive but less than 1000000).
 *
 * Parameter/Calling State:
 *      tvp is a pointer to the timeval to fix.  Returns:  none.
 *	On entry, no locks need to be held.
 *	During processing, no locks are acquired.
 *
 * Return Values/Exit State:
 *	On exit, no locks are held.
 */

STATIC void
_thr_tvfix(struct timeval *tvp)
{
        if (tvp->tv_usec >= MICROSEC) {
                ++tvp->tv_sec;
                tvp->tv_usec -= MICROSEC;
        }
        if (tvp->tv_usec < 0) {
                --tvp->tv_sec;
                tvp->tv_usec += MICROSEC;
        }
}

/*
 * void
 * _thr_dump_callo(callo_t *cop)
 *      Dump callout structure. 
 *	This function is used for debug only.
 * Parameter/Calling State:
 *      cop is a pointer to callout structure.
 *	On entry, no locks need to be held.
 *	During processing, no locks are acquired.
 *
 * Return Values/Exit State:
 *	On exit, no locks are held.
 */

void
_thr_dump_callo(callo_t *cop)
{
	printf("thrq_elt_t->thrq_next = 0x%x\t",cop->co_link.thrq_next);
	printf("thrq_elt_t->thrq_prev = 0x%x\n",cop->co_link.thrq_prev);
	printf("co_stat = %d\n",cop->co_stat);
	printf("co_abstime.tv_sec = %ld\t",cop->co_abstime.tv_sec);
	printf("co_abstime.tv_nsec = %ld\n",cop->co_abstime.tv_nsec);
	printf("co_interval.tv_sec = %ld\t",cop->co_interval.tv_sec);
	printf("co_interval.tv_nsec = %ld\n",cop->co_interval.tv_nsec);
}
