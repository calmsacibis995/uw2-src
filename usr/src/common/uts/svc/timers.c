/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:svc/timers.c	1.39"

/*
 * setitimer, getitimer, and alarm.  These were library routines on top
 * of the High Resolution Timer facility in V.4 ES, and making them system
 * calls here breaks binary compatibility for statically-linked applications.
 *
 * settimeofday, gettimeofday syscalls included.
 */

#include <util/types.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <svc/time.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/clock.h>
#include <proc/signal.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <mem/kmem.h>
#include <acc/priv/privilege.h>
#include <acc/audit/audit.h>


extern time_t time;
extern void settime(timestruc_t *);

#define ITIMER_INVAL(which)	(which < 0 || which > 2)

/*
 * xtimercmp -- compare a timeval to a timestruc_t.  Otherwise the same
 * as timercmp().  Op must not be >= or <=.
 */
#define xtimercmp(a, b, op)			\
	((a)->tv_sec op (b)->tv_sec ||          \
	(a)->tv_sec == (b)->tv_sec && (a)->tv_usec*(NANOSEC/MICROSEC)  \
	op (b)->tv_nsec)

static struct timeval tvzero;		/* constant 0 time */

static int itimerfix(struct timeval *);
static void realitexpire(lwp_t *);
static void alarmexpire(lwp_t *);
static void tvadd(struct timeval *, struct timeval *);
static void tvsub(struct timeval *, struct timeval *);
static void tvfix(struct timeval *);
static void hrt2tv(timestruc_t *, struct timeval *);


int
setitimer_k(int which, struct itimerval *itv, struct itimerval *otv)
{
	lwp_t *lwp = u.u_lwpp;
	struct itimerval *realtimerp;
	void *co;
	int error = 0;

	ASSERT(KS_HOLD0LOCKS());
	if (ITIMER_INVAL(which))
		return (EINVAL);
	
	if (itimerfix(&itv->it_value) || itimerfix(&itv->it_interval)) {
		/*
		 * The input timer is invalid.
		 */
		return (EINVAL);
	}

	if (which != ITIMER_REAL) {
		/*
		 * One of the two virtual timers.  Simply locking out the
		 * per-processor clock interrupt is sufficient to stop them.
		 */
		pl_t pl = splhi();

		/*
		 * Save the old value of the timer, if the user wants it.
		 */
		if (otv) {
			if (u.u_italarm[which - 1] != NULL)
				*otv = *u.u_italarm[which - 1];
			else
				bzero(otv, sizeof(*otv));
		}

		/*
		 * If no memory has been allocated for the timer, get some
		 * now.
		 */
		if (u.u_italarm[which - 1] == NULL) {
			u.u_italarm[which - 1] = kmem_alloc(sizeof(struct
					itimerval), KM_SLEEP);
		}
		*u.u_italarm[which - 1] = *itv;
		splx(pl);

		return error;
	}

	/*
	 *  Cancel the previous timer, if any.
	 */

	if (lwp->l_rtid != 0) {
		(void) untimeout_r(lwp->l_rtid);
		lwp->l_rtid = 0;
	}

	if (timerisset(&itv->it_value)) {
		/*
		 * We allocate room for the timeout and other data
		 * structures prior to acquiring any spin locks. 
		 */

		co = itimeout_allocate(KM_SLEEP);
		ASSERT(co != NULL);
		if (lwp->l_realtimer == NULL) {
			realtimerp  = kmem_alloc(sizeof(*lwp->l_realtimer),
								KM_SLEEP);
		}
	}

	/*
	 * Grab the time_mutex to prevent the time-of-day from advancing.
	 */
	TIME_LOCK();

	/*
	 * Save the old value of the timer, if the user wants it.
	 */
	if (otv) {
		/*
		 * Convert the absolute time stored in the timer into
		 * relative time.
		 */
		if (lwp->l_realtimer != NULL) {
			*otv  = *lwp->l_realtimer;

			if (timerisset(&otv->it_value)) {
				if (xtimercmp(&otv->it_value, &hrestime, <)) {
					timerclear(&otv->it_value);
				} else {
					struct timeval tv;
					hrt2tv(&hrestime, &tv);
					tvsub(&otv->it_value, &tv);
				}
			}
		} else {
			bzero(otv, sizeof(*otv));
		}
	}

	/*
	 * Convert the new timer and set the timeout routine.  itv expresses
	 * relative time, here.
	 */
	if (timerisset(&itv->it_value)) {
		toid_t id;
		long ticks;
		long period;
		struct timeval tv;

		ticks = itv->it_value.tv_sec * HZ +
			itv->it_value.tv_usec * HZ / MICROSEC;

		if (ticks <= 0)
			ticks = 1;		/* round up to one tick */

		if (timerisset(&itv->it_interval))
		{
			period = itv->it_interval.tv_sec * HZ +
				itv->it_interval.tv_usec * HZ / MICROSEC;

			if (period <= 0)
				period = 1;	/* round up to one tick */
		}
		else
			period = 0;

		id = itimeout_periodic_l_a(realitexpire, lwp,
					ticks, period, PLBASE, co);

		ASSERT(id != 0);
		lwp->l_rtid = id;

		hrt2tv(&hrestime, &tv);
		tvadd(&itv->it_value, &tv);
		if (lwp->l_realtimer == NULL) {
			lwp->l_realtimer = realtimerp; 
		}
		*lwp->l_realtimer = *itv;
	} else {
		/*
		 *  setitimer was called to cancel the real timer
 		 *  Store zeroed time entry in l_realtimer so subsequent
		 *  calls to getitimer will return the correct value
 		 */

		if (lwp->l_realtimer != NULL)
			*lwp->l_realtimer = *itv;
	}

	TIME_UNLOCK();

	return error;
}


/*
 * setitimer() system call
 */

struct setitimerarg {
	int	which;
	struct	itimerval *itv, *oitv;
};

/*
 * 
 * int  setitimer(struct setitimerarg *uap, rval_t *rvp)
 *	Set an interval timer (system call).
 *
 * Calling/Exit State:
 * 	args are the user's args, return is a place to put the return value.
 *
 * Description:
 *	The ITIMER_VIRTUAL and ITIMER_PROF are stored in relative time
 *	in the user struct, and decremented automatically.  SIGVTALRM
 *	is sent when the relative time reaches 0.
 *
 *	ITIMER_REAL is stored in the lwp in absolute time, and we use a
 *	timeout to let us know when it's time to send the signal.  SIGALRM
 *	is sent when the timeout fires.
 *
 * Remarks:
 *	The goal is, that errors in when the timeout is called do not
 *	accumulate.  That is, if one interval is somewhat too long, the
 *	next should be shortened to compensate.
 */
/* ARGSUSED */
int
setitimer(struct setitimerarg *uap, rval_t *rvp)
{
	struct itimerval aitv;
	struct itimerval aotv;
	int error = 0;

	if (copyin(uap->itv, &aitv, sizeof(aitv)))
		return (EFAULT);

	if (error = setitimer_k(uap->which, &aitv, uap->oitv ? &aotv : NULL))
		return error;

	if (uap->oitv != NULL)
		error = copyout(&aotv, uap->oitv, sizeof(struct itimerval));

	return error;
}


int
getitimer_k(int which, struct itimerval *itv)
{
	lwp_t *lwp = u.u_lwpp;

	ASSERT(KS_HOLD0LOCKS());
	if (ITIMER_INVAL(which))
		return (EINVAL);
	
	if (which == ITIMER_REAL) {
		/*
		 * Convert the absolute time stored in the timer into
		 * relative time.
		 */
		TIME_LOCK();

		/*
		 * We've stopped the advance of the time-of-day, either
		 * the timer has fired (in which case we've been signaled and
		 * the timer has been zeroed) or it's pending and may occur
		 * after we return the timer.
		 */

		if (lwp->l_realtimer != NULL) {
			*itv = *u.u_lwpp->l_realtimer;

			if (timerisset(&itv->it_value)) {
				if (xtimercmp(&itv->it_value, &hrestime, <)) {
					timerclear(&itv->it_value);
				} else {
					struct timeval tv;
					hrt2tv(&hrestime, &tv);
					tvsub(&itv->it_value, &tv);
				}
			}
		} else {
			bzero(itv, sizeof(*itv));
		}

		TIME_UNLOCK();
	} else {
		/*
		 * Asking for a virtual timer.  Only need to lock out the
		 * per-processor clock interrupt to stop this one.
		 */
		pl_t pl = splhi();

		if (u.u_italarm[which - 1] != NULL)
			*itv = *u.u_italarm[which - 1];
		else
			bzero(itv, sizeof(*itv));
		splx(pl);
	}

	return 0;
}


/*
 * getitimer() system call.
 */

struct getitimerarg {
	int	which;
	struct	itimerval *itv;
};

/*
 * int getitimer(struct getitimerarg *uap, rval_t *rvp)
 *	The getitimer() system call.
 *
 * Calling/Exit State:
 *	Returns the values of the /which/ argument timer.
 */
/* ARGSUSED */
int
getitimer(struct getitimerarg *uap, rval_t *rvp)
{
	struct itimerval aitv;
	int error;

	if (error = getitimer_k(uap->which, &aitv))
		return error;

	return copyout(&aitv, uap->itv, sizeof(struct itimerval));
}


/*
 * alarm() system call.
 */

struct alarmarg {
	uint_t	deltat;
};

/*
 * int alarm(struct alarmarg *uap, rval_t *rvp)
 *	the alarm system call.
 *
 * Calling/Exit State:
 *	Returns the seconds until the alarm would have gone off, and
 *	sets it to go off as specified by the argument.  If the argument
 *	is zero, the alarm is cancelled.
 */
int
alarm(struct alarmarg *uap, rval_t *rvp)
{
	lwp_t *lwp = u.u_lwpp;
	int diff;
	int error = 0;
	uint_t delta;
	toid_t id;

	ASSERT(KS_HOLD0LOCKS());
	if (lwp->l_artid != 0 && untimeout_r(lwp->l_artid) != 0) {
		diff = lwp->l_clktim - hrestime.tv_sec;
/*
 * From NIST PCTS 151-2 Beta test for alarm() assertion 3.4.1.3-09(A):
 *
 *	When there is a previous alarm() request with less than
 *	one second remaining, then the return from a call to alarm()
 *	returns the value 1.
 */
                if (diff <= 0)
                        diff = 1;
	} else {
		diff = 0;
	}
	
	/*
	 * Convert the alarm interval to an absolute seconds since the
	 * epoch.
	 */
	delta = uap->deltat;

	if (delta > LONG_MAX / HZ)
		delta = LONG_MAX / HZ;

	if (delta != 0) {
		void *co = itimeout_allocate(KM_SLEEP);

		TIME_LOCK();
		lwp->l_clktim = hrestime.tv_sec + delta;
		id = itimeout_l_a(alarmexpire, lwp, (long)HZ*delta,
			PLBASE, co);
		lwp->l_artid = id;
		TIME_UNLOCK();

		if (id == 0)
			error = EAGAIN;
	} else {
		lwp->l_artid = 0;
	}
	rvp->r_val1 = diff;
	return (error);
}

/*
 * int itimerfix(tv)
 *
 * Calling/Exit State:
 *	Check the plausibility of the given struct timeval, and make
 *	sure it represents at least as much time as the granularity of
 *	the system clock.  EINVAL is returned if the timer is not plausible.
 */
static int
itimerfix(struct timeval *tvp)
{
	if (tvp->tv_sec < 0 || tvp->tv_sec > 100000000 ||
	    tvp->tv_usec < 0 || tvp->tv_usec >= MICROSEC) {
		return (EINVAL);
	}

	/*
	 * Make sure that the time is at least one tick.
	 */
	if (tvp->tv_sec == 0 && tvp->tv_usec != 0 &&
					tvp->tv_usec < TICK / 1000)
	{
		tvp->tv_usec = TICK / 1000;
	}
	return (0);
}

/*
 * void timer_cancel()
 *	Called to clean up timer-related state of exiting lwp.
 *
 * Calling/Exit State:
 *	Frees memory, cancels timers, and so forth.  Returns: none.
 *
 * Locking:
 *	Must acquire the time_mutex to prevent timers from firing as
 *	they're deallocated.
 */
void
timer_cancel(void)
{
	lwp_t *lwp = u.u_lwpp;
	pl_t pl;
	int i;

	/*
	 *  Free the real timer, if any.
	 */

	if (lwp->l_rtid != 0) {
		(void) untimeout_r(lwp->l_rtid);
		lwp->l_rtid = 0;
	}

	if (lwp->l_realtimer != NULL) { 
		kmem_free(lwp->l_realtimer, sizeof(*lwp->l_realtimer));
	}

	/*
	 * Free the alarm timer, if any.
	 */
	if (lwp->l_artid != 0) {
		untimeout(lwp->l_artid);
		/*
		 * l_clktim is the absolute time (in sec since the Epoch)
		 * that the alarm clock should expire.  Clear it to be neat.
		 */
		lwp->l_clktim = 0;
	}

	/*
	 * Stop the virtual timers from going off, and free the memory
	 * used to hold them.
	 */
	pl = splhi();
	for (i = 0; i < 2; ++i) {
		if (u.u_italarm[i] != NULL) {
			kmem_free(u.u_italarm[i] , sizeof(struct itimerval));
			u.u_italarm[i] = NULL;
		}
	}
	splx(pl);
}

/*
 * void realitexpire(lwp)
 *	Called when a real interval timer expires.
 *
 * Calling/Exit State:
 *	lwp is the lwp to send the signal to.  Returns: none.
 */
static void
realitexpire(lwp_t *lwp)
{

	(void)sigtolwp(lwp, SIGALRM, (sigqueue_t *)NULL);

	if (!timerisset(&lwp->l_realtimer->it_interval)) {
		/*
		 * Not a repeating timer.
		 */
		timerclear(&lwp->l_realtimer->it_value);
		lwp->l_rtid = 0;
		return;
	}

	/*
	 *  Reload it_value with the time when the next repeating
	 *  timer should fire.
	 */

	TIME_LOCK();
	hrt2tv(&hrestime, &lwp->l_realtimer->it_value);
	tvadd(&lwp->l_realtimer->it_value, &lwp->l_realtimer->it_interval);
	TIME_UNLOCK();
}

/*
 * void alarmexpire(lwp)
 *	Send a SIGALRM to the given lwp; it's alarm clock has gone off.
 *
 * Calling/Exit State:
 *	lwp is the lwp to send the signal to.  Returns:  none.
 */
static void
alarmexpire(lwp_t *lwp)
{
	(void)sigtolwp(lwp, SIGALRM, (sigqueue_t *)NULL);
	lwp->l_artid = (toid_t)0;
	lwp->l_clktim = (time_t)0;
}

/*
 * gettimeofday() system call
 */

struct gettimeofdayarg {
	struct timeval	*tp;
	struct timezone *tzp;
};
/*
 * int gettimeofday(struct gettimeofdayarg *uap, rval_t *rvp)
 *	Get time of day.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
gettimeofday(struct gettimeofdayarg *uap, rval_t *rvp)
{
	struct timeval tv;
	timestruc_t atv;
	int error;

	ASSERT((KS_HOLD0LOCKS()));
	GET_HRESTIME(&atv)

	if (uap->tp == NULL)
		return (0);

	tv.tv_sec = atv.tv_sec;
	tv.tv_usec = atv.tv_nsec / (NANOSEC/MICROSEC);

	error = copyout((caddr_t)&tv, (caddr_t)uap->tp, sizeof(tv));
	if (error)
		return (error);

	/* No support for timezone information */
	return (0);
}

/*
 * settimeofday() system call.
 */
struct settimeofdayarg {
	struct timeval *tp;
	struct timezone *tzp;
};
/*
 * int settimeofday(struct settimeofdayarg *uap, rval_t *rvp)
 *	Set the time of day.
 *
 * Calling/Exit State:
 *	uap->tp is a pointer to the struct timeval to set the time
 *	to.  uap->tzp is the timezone information, which is currently
 *	unused.
 */
/* ARGSUSED */
int
settimeofday(struct settimeofdayarg *uap, rval_t *rvp)
{
	struct timeval tv;
	timestruc_t atv;
	int error;

	ASSERT((KS_HOLD0LOCKS()));

	if (uap->tp == NULL)
		return (0);

	if (error = pm_denied(u.u_lwpp->l_cred, P_SYSOPS)) {
		adt_stime(error, &atv);
		return (error);
	}

	if ((copyin((caddr_t)uap->tp, (caddr_t)&tv, sizeof(tv))) != 0) {
		error = EFAULT;
	} else {

		atv.tv_sec = tv.tv_sec;
		atv.tv_nsec = tv.tv_usec * (NANOSEC/MICROSEC);
	
		if (tv.tv_sec < 0 || tv.tv_usec < 0 || tv.tv_usec >= MICROSEC) {
			/*
			 * The time value is invalid.
			 */
			error = EINVAL;
		} else {
			settime(&atv);
			wtodc(&atv);
		}
	}
	adt_stime(error, &atv);
	return(error);
}



/*
 * int gtime(char *uap, rval_t *rvp)
 *	The gtime() syscall.
 *
 * Calling/Exit State:
 *	Uses no args, returns the number of seconds since the epoch
 *	in rvp->r_time.
 */
/* ARGSUSED */
int
gtime(char *uap, rval_t *rvp)
{
	ASSERT((KS_HOLD0LOCKS()));
	rvp->r_time = hrestime.tv_sec;
	return (0);
}

/*
 * stime() system call.
 */

struct stimearg {
	time_t	time;
};

/*
 * int stime(struct stimearg *uap, rval_t *rvp)
 *
 * Calling/Exit State:
 *	time is the time to set the system clock to.  Returns:  the usual
 *	syscall stuff, -1 for error, 0 for success.
 */
/* ARGSUSED */
int
stime(struct stimearg *uap, rval_t *rvp)
{
	int error;
	timestruc_t hrt;
	ASSERT(KS_HOLD0LOCKS());

	if ((error = pm_denied(u.u_lwpp->l_cred, P_SYSOPS)) == 0) {
		hrt.tv_sec = uap->time;
		hrt.tv_nsec = 0;
		settime(&hrt);
		wtodc(&hrt);
	}
	adt_stime(error, &hrt);
	return (error);
}

/*
 * void tvadd(struct timeval t1, struct timeval t2)
 *	Add two struct timevals.
 *
 * Calling/Exit State:
 *	t1 = t1 + t2; the result of the addition is returned in t1.
 */
static void
tvadd(struct timeval *t1, struct timeval *t2)
{
	t1->tv_sec += t2->tv_sec;
	t1->tv_usec += t2->tv_usec;
	tvfix(t1);
}

/*
 * void tvsub(struct timeval t1, struct timeval t2)
 *	Substract two timevals.
 *
 * Calling/Exit State:
 *	t1 = t1 - t2; the result of the subtraction is left in t1.
 */
static void
tvsub(struct timeval *t1, struct timeval *t2)
{
	t1->tv_sec -= t2->tv_sec;
	t1->tv_usec -= t2->tv_usec;
	tvfix(t1);
}

/*
 * void tvfix(tvp)
 *	Adjust a struct timeval to have a reasonable format (the
 *	number of microseconds should be positive but less than 1000000).
 *
 * Calling/Exit State:
 *	tvp is a pointer to the timeval to fix.  Returns:  none.
 */
static void
tvfix(struct timeval *tvp)
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
 * void hrt2tv(timestruc_t *, struct timeval *)
 *
 * Calling/Exit State:
 *	The struct timeval is an outparameter which contains the
 *	timestruc_t, converted.
 */
static void
hrt2tv(timestruc_t *hrt, struct timeval *tv)
{
	tv->tv_sec = hrt->tv_sec;
	tv->tv_usec = hrt->tv_nsec * (MICROSEC/NANOSEC);
}

/*
 * adjtime() system call
 */
struct adjtimea {
	struct timeval *delta;
	struct timeval *olddelta;
};

/*
 * Parameters for adjtime skew rate calculation.
 *
 * Time differences up to SMALL_CUTOFF drift the clock at SMALL_DRIFT
 * microseconds per tick.  Time differences greater than or equal to
 * LARGE_CUTOFF drift the clock at LARGE_DRIFT microseconds per tick.
 */
#define SMALL_CUTOFF	(5 * MICROSEC)			/* 5 seconds */
#define SMALL_DRIFT	(((MICROSEC/HZ) * 8) / 1000)	/* 0.8% */
#define LARGE_CUTOFF	(5 * 60 * MICROSEC)		/* 5 minutes */
#define LARGE_DRIFT	((MICROSEC/HZ) / 10)		/* 10% */

/*
 * State of current adjtime adjustment.
 */
long timedelta;			/* unapplied time correction in microseconds */
int tickdelta_usec;		/* current clock skew, microseconds per tick */
long tickdelta_nsec;		/* current clock skew, nanoseconds per tick  */


/*
 * long clockadj(long ndelta, boolean_t set_todc)
 *	Adjust the system time.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Cause the system time to advance more or less quickly until the
 *	system time is changed by ndelta microseconds.
 *
 *	If set_todc is set, then the hardware time-of-day clock will
 *	also be set to the same time.
 *
 *	Returns any remaining adjustment in progress, in milliseconds.
 */

long
clockadj(long ndelta, boolean_t set_todc)
{
	int ntickdelta;
	long ntickdelta_nsec;
	long otimedelta;
	timestruc_t time0;

	/*
	 * Compute the rate at which to drift the clock.
	 *
	 * Negative differences and small positive differences
	 * use a small drift (less than one percent).  Larger
	 * positive differences use a linearly increasing drift
	 * rate, up to the maximum drift rate (not more than 10%).
	 */
	ntickdelta = SMALL_DRIFT;
	if (ndelta > SMALL_CUTOFF) {
		if (ndelta >= LARGE_CUTOFF)
			ntickdelta = LARGE_DRIFT;
		else {
			ntickdelta += ((((ndelta - SMALL_CUTOFF) / 1000) *
					(LARGE_DRIFT - SMALL_DRIFT)) /
					((LARGE_CUTOFF - SMALL_CUTOFF) / 1000));
		}
	} else if (ndelta < 0) {
		/*
		 * To slow down the clock, we need a negative adjustment.
		 */
		ntickdelta = -SMALL_DRIFT;
	}

	if (ndelta % ntickdelta) {
		/*
		 * Make the adjustment an even multiple of the per-tick
		 * adjustment.
		 */
		ndelta = (ndelta / ntickdelta) * ntickdelta;
	}
	ntickdelta_nsec = ntickdelta * (NANOSEC/MICROSEC);

	/*
	 * Install the new set of parameters.
	 */
	TIME_LOCK();

	time0 = hrestime;
	otimedelta = timedelta;

	timedelta = ndelta;
	tickdelta_usec = ntickdelta;
	tickdelta_nsec = ntickdelta_nsec;

	TIME_UNLOCK();

	if (set_todc) {
		/*
		 * Add ndelta (in microseconds) to the starting time,
		 * time0 (in seconds and nanoseconds).
		 */
		ndelta += time0.tv_nsec / (NANOSEC/MICROSEC);
		time0.tv_nsec %= (NANOSEC/MICROSEC);
		time0.tv_sec += ndelta / MICROSEC;
		ndelta %= MICROSEC;
		time0.tv_nsec += ndelta * (NANOSEC/MICROSEC);
		/*
		 * Set the h/w time-of-day clock to the adjusted value.
		 */
		wtodc(&time0);
	}

	return otimedelta;
}


/*
 * int adjtime(struct adjtimea *uap, rval_t *rvp)
 *	Adjust the system time.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Cause the system time to advance more or less quickly until the
 *	system time is changed by "uap->delta".  Races will be resolved
 *	by having the last LWP to acquire the TIME_LOCK and install its
 *	values over the previous values.
 */
/* ARGSUSED */
int
adjtime(struct adjtimea *uap, rval_t *rvp)
{
	long ndelta;
	long otimedelta;
	struct timeval atv, oatv;
	timestruc_t hrt;		/* argument for adt_stime() */
	int error;

	ASSERT(KS_HOLD0LOCKS());
	if (error = pm_denied(u.u_lwpp->l_cred, P_SYSOPS)) {
		adt_stime(error, &hrt);
		return(error);
	}

	if (uap->delta == NULL) {
		/*
		 * Only checking the previous value.
		 */
		otimedelta = timedelta;
		goto out;
	}

	if (copyin((caddr_t)uap->delta, (caddr_t)&atv, sizeof(struct timeval))){
		error = EFAULT;
		adt_stime(error, &hrt);
		return(error);
	}

	ndelta = atv.tv_sec * MICROSEC + atv.tv_usec;

	otimedelta = clockadj(ndelta, B_TRUE);

	hrt.tv_sec = atv.tv_sec;
	hrt.tv_nsec = atv.tv_usec * (NANOSEC/MICROSEC);
	adt_stime(error, &hrt);

out:
	if (uap->olddelta) {
		oatv.tv_sec = otimedelta / MICROSEC;
		oatv.tv_usec = otimedelta % MICROSEC;

		if (copyout((caddr_t)&oatv, (caddr_t)uap->olddelta,
		            sizeof(struct timeval))) {
			error = EFAULT;
		}
	}

	return(error);
}

/*
 * void settime(timestruc_t *tvp)
 *	Set hrestime and time with the value of the specified argument.
 *
 * Calling/Exit State:
 *
 *	Acquires and releases time_mutex, thus caller cannot hold time_mutex.
 */
void
settime(timestruc_t *tvp)
{
	TIME_LOCK();
	/*
	 * Set time and hrestime to the specified value.
	 */
	time = hrestime.tv_sec = tvp->tv_sec;
	hrestime.tv_nsec = tvp->tv_nsec;
	/*
	 * Cancel any pending adjustments.
	 */
	timedelta = 0;
	TIME_UNLOCK();
}
