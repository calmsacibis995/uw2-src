/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)kern:svc/clock.c	1.79"
#ident	"$Header: $"

#include <util/types.h>
#include <util/ksynch.h>
#include <mem/anon.h>
#include <mem/as.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/tuneable.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <svc/clock.h>
#include <svc/time.h>
#include <util/plocal.h>
#include <util/engine.h>
#include <util/var.h>
#include <util/cmn_err.h>
#include <util/ipl.h>
#include <util/debug.h>
#include <svc/systm.h>
#include <proc/resource.h>
#include <proc/disp.h>
#include <proc/class.h>
#include <util/ghier.h>
#include <util/inline.h>
#include <util/metrics.h>
#include <util/sysmacros.h>
#include <util/param.h>

#ifndef NO_RDMA
#include <mem/rdma.h>
#endif

/*
 * Externs needed for once_sec().
 */
extern event_t poolrefresh_event;
extern clock_t poolrefresh_lasttime;
extern boolean_t poolrefresh_pending;
extern event_t fsflush_event;
#ifdef _PAGEIO_HIST
extern event_t bufsubr_event;
#endif
extern void vmmeter(void);
	
/*
 * Mutexes all forms of callout and time increment.
 */
fspin_t time_mutex;

extern int	tickdelta_usec;
extern long	tickdelta_nsec;
extern long	timedelta;
extern void	vmmeter();

/*
 * Statistics for fixtimed.
 */
long fixtime_net;	/* net total adjustments, in seconds */
long fixtime_abs;	/* total magnitude of adjustments, in seconds */
uint_t fixtime_cnt;	/* number of times adjusted */

time_t	time;		/* time in seconds since 1970 */
			/* is here only for compatibility, new usages */
			/* should use hrestime */
extern timestruc_t hrestime;	/* time in seconds and nsec since since 1970 */
uint_t timer_resolution = TICK;/* hardware clock ticks/sec */

volatile clock_t lbolt;		/* time in HZ since last boot */
sv_t	lbolt_sv;
lock_t	lbolt_lock;	/* necessary to use a SV for the lbolt */
LKINFO_DECL(lbolt_lkinfo, "lbolt_lock", 0);
STATIC int fsflushcnt;	/* counter for t_fsflushr	*/
#ifdef _PAGEIO_HIST
STATIC int bufsubr_count;
#endif

/*
 * Forward references and prototypes.
 */
struct callout;
STATIC struct callout *global_todo; /* global callout list */

STATIC void itimeout_init(void);
STATIC int callout_tick(struct callout **);
STATIC void dispcallout(struct callout **);
STATIC void once_sec(void);
STATIC void fixtimed(void);

boolean_t itimerdecr(struct itimerval *, int);

/*
 * void todclock(TODCLOCK_ARGS)
 *
 *	Process the time-of-day clock interrupt.
 *
 * Calling/Exit State:
 *
 *	Called from the time-of-day clock interrupt vector.
 *
 * Description:
 *
 *	This is the time of day clock interrupt handler.
 *	It is responsible for providing periodic "system wide" timing
 *	services, most notable of which are time-of-day increment and
 *	global callout handling.
 */
/* ARGSUSED */
void
todclock(TODCLOCK_ARGS)
{
	int one_sec;
	long delta;
#ifdef DEBUG
	boolean_t _tickd_nonzero = B_TRUE, _no_underflow = B_TRUE;
#endif

	TIME_LOCK();

	if (callout_tick(&global_todo))
		sendsoft(l.eng, GLOBALSOFTINT);

	/*
	 * Increment the time-of-day, drifting the clock if necessary.
	 * Clock drift is usually requested from one of two sources:
	 * the adjtime system call and platform-specific clock handling
	 * code which syncs time to a local hardware real-time clock
	 * or a network time source.
	 *
	 * TICK is the standard number of nanoseconds in a clock tick
	 * for this platform.
	 */
	delta = TICK;
	if (timedelta != 0) {
		/*
		 * Speed up or slow down the hrestime clock as necessary
		 * to achieve the requested drift.
		 *
		 * timedelta is the remaining total drift, in microseconds.
		 *
		 * tickdelta_usec is the extra time to add (may be negative)
		 * per tick, in microseconds.
		 *
		 * tickdelta_nsec is the extra time to add (may be negative)
		 * per tick, in nanoseconds.  This is pre-calculated from
		 * tickdelta_usec so hrestime can be updated without the
		 * overhead of converting microseconds to nanoseconds each
		 * time this handler runs.
		 */
		delta = TICK + tickdelta_nsec;
		timedelta -= tickdelta_usec;
#ifdef DEBUG
		/* Can't ASSERT while holding FSPIN */
		_tickd_nonzero = (tickdelta_usec != 0);
		_no_underflow = (tickdelta_usec > 0 ? (timedelta >= 0)
						    : (timedelta <= 0));
#endif
	}
	BUMPTIME(&hrestime, delta, one_sec);

	/*
	 * Increment the lbolt and "time" (if necessary).
	 */
	++lbolt;
	if (one_sec)
		++time;

	TIME_UNLOCK();

	ASSERT(_tickd_nonzero);
	ASSERT(_no_underflow);

	/*
	 * Signal any lbolt waiters.
	 */
	LBOLT_BROADCAST();

	return;
}

/*
 * void clockinit(void)
 *
 *	Do initialization processing for clock and callout handling.
 *
 * Calling/Exit State:
 *
 *	Must be called from sysinit from the boot processor.
 *
 * Description:
 *
 *	Clockinit allocates the private reserve of callout structures
 *	and establishes the "once-per-second" callout.
 */
void
clock_init(void)
{
	/*
	 * Initialize the local and global callout data structures.
	 */
	itimeout_init();
	SV_INIT(&lbolt_sv);
	LOCK_INIT(&lbolt_lock, LBOLT_HIER, PLHI, &lbolt_lkinfo, KM_NOSLEEP);

	/*
	 * Initiate the "once-per-second" callout.
	 * Also, initialize events it uses, in case the callout fires
	 * before the target processes initialize themselves.
	 */
	EVENT_INIT(&poolrefresh_event);
	EVENT_INIT(&fsflush_event);
#ifdef _PAGEIO_HIST
	EVENT_INIT(&bufsubr_event);
#endif
	if (itimeout(once_sec, (void *)0, HZ | TO_PERIODIC, PLHI) == 0) {
		/*
		 *+ The kernel was unable to allocate necessary space to
		 *+ enqueue a callout which executes a number of
		 *+ "once-per-second" services.  This allocation is being
		 *+ attempted at boot time when there is plenty of memory
		 *+ available.  Failure at this point indicates a essential
		 *+ problem with the kernel.
		 */
		cmn_err(CE_PANIC, "Cannot set up once-per-second callout.");
	}

	if (itimeout(fixtimed, (void *)0, (HZ*60) | TO_PERIODIC, PLHI) == 0) {
		cmn_err(CE_WARN, "clock_init: can't start fixtimed");
	}
}

/*
 * void globalsoftint(void)
 *
 *	Do deferred processing of the tod clock interrupt.
 *
 * Calling/Exit State:
 *
 *	None.
 *
 * Description:
 *
 *	Performs global callout dispatch.
 */
void
globalsoftint(void)
{
	dispcallout(&global_todo);
}

/*
 * void localsoftint(void)
 *
 *	Do deferred processing of the local clock interrupt handler.
 *
 * Calling/Exit State:
 *
 *	None.
 *
 * Description:
 *
 *	Performs local callout dispatch.
 */
void
localsoftint(void)
{
	dispcallout(&l.eng->e_local_todo);
}

/*
 * void once_sec(void)
 *
 *	Routine which performs a number of "once per second" system-wide
 *	services.
 *
 * Calling/Exit State:
 *
 *	None.
 *
 * Remarks:
 *
 *	These used to be resident directly in the time of day
 *	clock handler; however, it seems to be much more reasonable to
 *	make them callouts.
 */
STATIC void
once_sec(void)
{
	/* Wakeup the pool-refresh daemon */
	if (!poolrefresh_pending &&
	    (lbolt - poolrefresh_lasttime) >= ((HZ * 3) / 4)) {
		poolrefresh_pending = B_TRUE;
		EVENT_SIGNAL(&poolrefresh_event, 0);
	}

	vmmeter();

	/* 
	 * Update global runque and swap "queue" metrics
	 */
	MET_GLOB_RUNQUE();
	MET_GLOB_SWAPQUE();


	MET_FREEMEM(freemem);	/* Update freemem statistics */

	/*
	 * Update the freeswap cumulative count
	 */
	MET_FREESWAP(anoninfo.ani_kma_max - anoninfo.ani_resv);

	/* Wakeup the fsflush daemon */
	if (--fsflushcnt <= 0) {
		fsflushcnt = tune.t_fsflushr;
		EVENT_SIGNAL(&fsflush_event, 0);
	}

#ifdef _PAGEIO_HIST
	if (--bufsubr_count <= 0) {
		bufsubr_count = 60;
		EVENT_SIGNAL(&bufsubr_event, 0);
	}
#endif

}

/*
 * The following two macros are useful in computation of new agequantum 
 * values, in the per-processor clock handler.
 */
#define	USHORTMAX(a, b)	(u_short)MAX((u_short)(a), (u_short)(b))
#define USHORTMIN(a, b) (u_short)MIN((u_short)(a), (u_short)(b))

/*
 * void lclclock(LCLCLOCK_ARGS)
 *
 *	Per-processor clock handler.
 *
 * Calling/Exit State:
 *
 *	Must be called from the local, per-processor clock interrupt vector.
 *
 * Description:
 *
 *	This is the local clock handler.
 *	It performs per-process, per-lwp and per-processor timing services.
 */
/* ARGSUSED */
void
lclclock(LCLCLOCK_ARGS)
{
	engine_t *eng;
	lwp_t *lwp;
	boolean_t needs_aging;	/* does process need aging? */
	int rssgrowth;
	proc_t *procp;
	struct as *as;
	pl_t oldipl;

	eng = l.eng;
	needs_aging = B_FALSE;

	if (eng->e_local_todo) {
		TIME_LOCK();

		if (eng->e_local_todo && callout_tick(&eng->e_local_todo))
			sendsoft(eng, LOCALSOFTINT);

		TIME_UNLOCK();
	}

	if (--l.one_sec <= 0) {	
		l.one_sec += HZ;

		/*
		 * Update metrics for processor private run queue
		 */
		MET_PP_RUNQUE();
	}

	if (LCL_WAS_USER_MODE) {
		struct itimerval *itvp;

		lwp = u.u_lwpp;
		procp = u.u_procp;

		/* process BSD style ITIMER_VIRTUAL */
		itvp = u.u_italarm[ITIMER_VIRTUAL - 1];
		if (itvp != NULL && timerisset(&itvp->it_value) &&
		    itimerdecr(itvp, MICROSEC/HZ)) {
			sigtolwp(u.u_lwpp, SIGVTALRM, NULL);
		}

		/*
		 * Update user mode metrics
		 */
		MET_USER_CPU_UPDATE();

		/*
		 * Update the p_utime and l_utime fields. Also, if 
		 * profiling is enabled, turn on L_CLOCK. We also
		 * increment the ageticks here, and update the activity time.
		 * Also update the p_nonlockedrss, so that swapper can
		 * examine this without acquiring the process mutex.
		 * Adjust aging quantum as needed, and determine if aging needs
		 * to occur.
		 */

		lwp->l_utime++;
		oldipl = LOCK(&procp->p_mutex, PLHI);
		procp->p_utime = ladd(procp->p_utime, dl_one);

		if ((as = procp->p_as) != NULL) {
			/*
			 * if aginq quantum has expired, check whether the
			 * most recent growth rate justifies expanding the
			 * aging interval, subject to MAX_QUANTUM limit.
			 * PERF-I
			 * if the process has expired its current aging quantum
			 * but freemem is plentiful, then we let the process go
			 * on for an extra tick without aging. we do this by
			 * decrementing its aging counter. on configurations 
			 * that are memory rich, this lets processes make 
			 * forward progress without aging overhead as long as
			 * there is little or no memory contention. however,
			 * the instantaneous available memory, which is used to
			 * guide this "skipping" decision may not be the best
			 * indicator of memory availability, since it could be
			 * high despite memory contention.
			 * PERF-II
			 * the tests performed below, for adjusting aging 
			 * quantum, and for deciding that aging needs to occur
			 * are structured so that in the most common case, two,
			 * and less frequently four tests are performed in each
			 * execution of the clock handler. it should be verified
			 * that this is indeed the case, as otherwise, the
			 * overhead can be injurious to performance.
			 */

			procp->p_mem += as->a_rss;

			if (++as->a_ageticks >= as->a_agequantum) {

				rssgrowth = (int)(as->a_rss) - 
					    (int)(as->a_prevrss);
				/*  
				 * adjust for how long it took for rssgrowth to
				 * occur, before deciding that the rate is high
				 */

				if ((rssgrowth * RSS_SAMPLE_TIME) > 
				    (int)(hi_grow_rate * 
					(as->a_ageticks % RSS_SAMPLE_TIME))) {

					as->a_agequantum = USHORTMIN(
					   (RSS_SAMPLE_TIME + as->a_agequantum),
					     as->a_max_agequantum);

				   	if (as->a_ageticks >=
					     as->a_agequantum) {
						/*
						 * PERF: If memory conditions
						 * do not warrant a giveback,
						 * let this process go on for
						 * a tick more?
						 */
						needs_aging = B_TRUE;
				   	}
				} else { 
					/* 
					 * the aging interval has expired, with
					 * no upward adjustment. 
					 *
					 * PERF: If memory conditions do not 
					 * warrant a giveback, let this process
					 * go on for a tick more?
					 */

					needs_aging = B_TRUE;
				}

				/* 
				 * don't adjust prevrss, since the 
				 * rate-of-growth computation in this, the
				 * boundary case, accounts explicitly for
				 * the number of ticks since the last
				 * RSS_SAMPLE_TIME point.
				 */
				procp->p_nonlockedrss = (as->a_rss -
					as->a_lockedrss);

			} else if ((as->a_ageticks % RSS_SAMPLE_TIME) == 0) {

				/*
				 * periodic adjustment to (a) the aging quantum,
				 * based on physical growth rate of the process,
				 * and (2) the nonlocked RSS field in the 
				 * process structure, maintained for swapper.
				 */

				rssgrowth = (int)(as->a_rss) - 
					    (int)(as->a_prevrss);

				if (rssgrowth < lo_grow_rate) {

					/* low growth rate. reduce quantum */
					as->a_agequantum = USHORTMAX(
						as->a_min_agequantum, 
						(as->a_agequantum - 
						 QUANTUM_DECREMENT));

				} else if (rssgrowth > hi_grow_rate) {

					/* raise aging quantum */
					as->a_agequantum = USHORTMIN(
					   as->a_max_agequantum,
					   USHORTMAX((as->a_agequantum + 
						       QUANTUM_INCREMENT),
					   (RSS_SAMPLE_TIME + as->a_ageticks)));
				}
				
				/*
				 * Set the previous RSS value to be that ave-
				 * raged with the current RSS, to preserve a
				 * little history and to let the process
				 * stabilize before reacting.
				 */
				as->a_prevrss = (as->a_rss+as->a_prevrss)/2;

				/*
				 * record nonlocked RSS in process structure.
				 */
				procp->p_nonlockedrss = (as->a_rss -
					as->a_lockedrss);
			}
			procp->p_active = lbolt;
		}

		if (lwp->l_trapevf & EVF_PL_PROF) {
			(void) LOCK(&lwp->l_mutex, PLHI);
			lwp->l_flag |= L_CLOCK;
			lwp->l_trapevf |= EVF_PL_PROF_EVT;
			UNLOCK(&lwp->l_mutex, PLHI);
		}
		UNLOCK(&procp->p_mutex, oldipl);

	} else {
		/* 
		 * Former mode is kernel.
		 */
		if ((lwp = CURRENT_LWP()) != NULL) {
		   	procp = lwp->l_procp;

		   	lwp->l_stime++;
		   	oldipl = LOCK(&procp->p_mutex, PLHI);
		   	procp->p_stime = ladd(procp->p_stime, dl_one);	

		   	if ((as = procp->p_as) != NULL) {

			   /*
			    * Indented by 3 spaces for better readability.
			    *
			    * Code for the kernel mode case, agequantum
			    * adjustment and nonlocked RSS update to process
			    * structure, is identical to that for the user
			    * mode case. The code duplication is in order to
			    * avoid acquiring the process mutex a second time
			    * for these activities.
			    */

			   procp->p_mem += as->a_rss;

			   if (++as->a_ageticks >= as->a_agequantum) {

				rssgrowth = (int)(as->a_rss) - 
					    (int)(as->a_prevrss);

				if ((rssgrowth * RSS_SAMPLE_TIME) > 
				    (int)(hi_grow_rate * 
					(as->a_ageticks % RSS_SAMPLE_TIME))) {

					as->a_agequantum = USHORTMIN(
					   (RSS_SAMPLE_TIME + as->a_agequantum),
					   as->a_max_agequantum);

				   	if (as->a_ageticks >=
					     as->a_agequantum) {
						/*
						 * PERF: If memory conditions
						 * do not warrant a giveback,
						 * let this process go on for
						 * a tick more?
						 */
						needs_aging = B_TRUE;
				   	}
				} else { 
					/*
					 * PERF: If memory conditions do not 
					 * warrant a giveback, let this process
					 * go on for a tick more?
					 */
					needs_aging = B_TRUE;
				}

				procp->p_nonlockedrss = (as->a_rss -
					as->a_lockedrss);

			   } else if ((as->a_ageticks % RSS_SAMPLE_TIME) == 0) {

				rssgrowth = (int)(as->a_rss) - 
					    (int)(as->a_prevrss);

				if (rssgrowth < lo_grow_rate) {

					as->a_agequantum = USHORTMAX(
						as->a_min_agequantum, 
						(as->a_agequantum - 
						 QUANTUM_DECREMENT));

				} else if (rssgrowth > hi_grow_rate) {

					as->a_agequantum = USHORTMIN(
					   as->a_max_agequantum,
					   USHORTMAX((as->a_agequantum + 
						      QUANTUM_INCREMENT),
					   (RSS_SAMPLE_TIME + as->a_ageticks)));
				}
				
				as->a_prevrss = (as->a_rss+as->a_prevrss)/2;

				procp->p_nonlockedrss = (as->a_rss -
					as->a_lockedrss);
			   }
			   procp->p_active = lbolt;
			} 

		   	if (lwp->l_trapevf & EVF_PL_PROF) {
				(void) LOCK(&lwp->l_mutex, PLHI);
				lwp->l_trapevf |= EVF_PL_PROF_EVT;
				UNLOCK(&lwp->l_mutex, PLHI);
		   	}
			UNLOCK(&procp->p_mutex, oldipl);
		}
		if (MET_LCL_IS_IDLE) {
			if (MET_IO_OUTSTANDING) {
				/*
				 * Update waiting for I/O cpu metrics
				 */
				MET_WAIT_CPU_UPDATE();
			} else {
				/*
				 * Update idle cpu metrics
				 */
				MET_IDLE_CPU_UPDATE();
			}
		} else {
			/*
			 * Update kernel mode metrics
			 */
			MET_SYS_CPU_UPDATE();
		}

	}

	PRFINTR(LCLCLOCK_PC, LCL_WAS_USER_MODE);

	if ((lwp = CURRENT_LWP()) != NULL) {
		/*
		 * Both kernel and user mode cases.
		 */
		struct itimerval *itvp;

		/* process BSD style ITIMER_PROF */
		itvp = u.u_italarm[ITIMER_PROF - 1];
		if (itvp != NULL && timerisset(&itvp->it_value) &&
		    itimerdecr(itvp, MICROSEC/HZ)) {
			sigtolwp(u.u_lwpp, SIGPROF, NULL);
		}


		ASSERT(lwp->l_cllwpp != NULL);
		CL_TICK(lwp, lwp->l_cllwpp);

		procp = lwp->l_procp;

		if (needs_aging) { 
			oldipl = LOCK(&lwp->l_mutex, PLHI);
			lwp->l_trapevf |= EVF_L_ASAGE;	
			UNLOCK(&lwp->l_mutex, oldipl);
		}

		/*
		 * Enforce CPU rlimit.
		 */
		if (!(procp->p_flag & P_DESTROY) &&
		    (u.u_rlimits->rl_limits[RLIMIT_CPU].rlim_cur !=
		     RLIM_INFINITY)) {
			dl_t d1, d2;
			d1 = ladd(procp->p_utime, procp->p_stime);
			d2.dl_hop = 0;
			d2.dl_lop = HZ;
			d1 = ldivide(d1, d2);

			if (d1.dl_hop ||
			    d1.dl_lop >
			    u.u_rlimits->rl_limits[RLIMIT_CPU].rlim_cur)
				sigtoproc(procp, SIGXCPU, (sigqueue_t *)0);
		}
	}

	return;
}

/*
 * itimerdecr(struct itimerval *, int usec)
 *	Called from a local clock interrupt to decrement the VIRTUAL
 *	and PROF interval timers.  Usec is the number of microseconds
 *	to decrement the timer by.
 *
 * Calling/Exit State:
 *	Given a pointer to the timer value to be decremented.  If the
 *	timer becomes zero after being decremented, the value is reloaded
 *	from the interval.
 *
 *	Returns:  B_TRUE if a signal should be sent to the lwp who set
 *		the timer, B_FALSE if not.
 */
boolean_t
itimerdecr(struct itimerval *itvp, int usec)
{
	if (itvp->it_value.tv_usec < usec) {
		if (itvp->it_value.tv_sec == 0) {
			/* expired and already in next interval */
			usec -= itvp->it_value.tv_usec;
			goto expire;
		}
		itvp->it_value.tv_usec += MICROSEC;
		itvp->it_value.tv_sec--;
	}
	itvp->it_value.tv_usec -= usec;
	usec = 0;
	if (timerisset(&itvp->it_value)) {
		return B_FALSE;
	}
	/*
	 * If it_value isn't set, the timer has expired exactly at the end
	 * of the interval.
	 */
expire:
	if (timerisset(&itvp->it_interval)) {
		itvp->it_value = itvp->it_interval;
		itvp->it_value.tv_usec -= usec;
		if (itvp->it_value.tv_usec < 0) {
			itvp->it_value.tv_usec += MICROSEC;
			itvp->it_value.tv_sec--;
		}
	} else {
		itvp->it_value.tv_usec = 0;	/* .tv_sec is already 0 */
	}
	return B_TRUE;
}

/*
 * Structure to track and dispatch a callout.
 *
 * When a callout is created, it is placed on two lists:  a id-hashed
 * lookup linked list and a relatively timed binary "todo" tree.
 *
 * A callout can be in one of three states:
 *
 *	WAITING		On the todo-list, waiting for its timer to expire.
 *	INPROGRESS	Currently executing.
 *	FIRED		Dispatched, but being processed by untimeout.
 *
 * Callout entries which are waiting are on both lists; callouts in the
 * process of firing are removed from the todo list.
 *
 * The todo-tree is organized as a binary tree for fast insertion of a
 * callout.  Each callout entry contains enough back pointers to allow
 * a callout to be untimeout'ed with only pointer manipulation (i.e.
 * no search).
 *
 * The tree starts at its root.  Callout sub-trees attached to the "c_lower"
 * entry of the parent will all fire in less time then the parent.  Sub-trees
 * rooted at the "c_higher" branch of the parent will fire farther in the
 * future then the parent.  If a bredth-first walk is done of the terminal
 * nodes, they will appear sorted, with lower elements on the left side of
 * list and higher elements on the right side of the list.
 *
 * The left-most entry of the tree will always be the one to be dispatched.
 * A left-most terminal node may simply be deleted from the tree once they
 * become due.  Left-most non-terminal nodes are know to have only a c_higher
 * sub-tree (otherwise they wouldn't be left-most).  Left-most non-terminal
 * nodes are deleted from the tree by promoting the c_higher subtree to its
 * current position and removing itself from the tree.  This promotion
 * creates a new left-most node and things proceed like this in an orderly
 * fashion.
 *
 * Thus, if untouched by untimeout, deletion from the todo tree is fairly
 * straight-forward.  Untimeout, however, causes problems as it may attempt
 * to delete a node from the center of the tree (i.e. one with both left
 * and right-hand branches).  Such a deletion is difficult, as it requires
 * us to re-order both sub-trees of the node being deleted.  Rather than
 * doing the re-ordering, we just declare the node as being dead.  Eventually,
 * the left-most sub-tree will all dispatch and the right-most sub-tree
 * may be promoted to the position of the dead node, allowing it to be
 * deleted.  Whenever we delete a node from the tree, we always visit the
 * parent of the node and harvest as many dead ancestors as possible.
 *
 * Dead nodes are removed from the id-hash list and no longer occupy any
 * position in the callout-id name space (i.e. we can re-use the id of a
 * dead node without trouble).  It is important to note the lifespan of
 * a dead node is limited by the minimum lifespan of either of its sub-trees,
 * thus, dead nodes can only hold down allocated space for a bounded amount
 * of time.
 *
 * A dead node cannot be a terminal, left or right-most node, as such nodes
 * are deleted immediately from the callout or from the subsequent ancestor
 * walk after a descendent is deleted.
 *
 * The time associated with each node is a relative time.  The time at the
 * root of the tree is the number of ticks until the root callout is to
 * fire.  The time of a descendent node is relative to the time of the
 * parent node.  A descendent to the left or right of a parent will fire in the
 * parent node's time plus the descendent's time.  A descendent on the left
 * side of a parent will have a negative time relative to the parent.
 * A descendent on the right side of a parent will have a zero or positive
 * time associated with it.
 *
 * This allows the time-base of the callouts to be continually rolling-over
 * as timeouts in the tree fire and does not suffer from the overflow
 * problems associated with absolute time-bases.  For example, if the time
 * was an absolute time based on the lbolt, we'd need to do modular arithmetic
 * whenever we needed to do any time comparisons.
 */
struct callout {
	list_t c_q;		/* forward/backward links */
#define	c_flink	c_q.flink
#define	c_rlink	c_q.rlink
	long	c_time;		/* incremental time */
	long	c_period;	/* time period if C_PERIODIC */
	toid_t	c_id;		/* timeout id */
	void	*c_arg;		/* argument to routine */
	void	(*c_fcn)();	/* routine */
	pl_t	c_pl;		/* priority-level to call at */
	ushort_t c_want;	/* count of untimeout's waiting for this id */
	ushort_t c_flags;	/* flags for this structure entry */
#define	C_STAT		0x0f	/* mask for the status word */
#define	C_WAITING	0x00	/* callout is waiting to dispatch (must be 0) */
#define	C_INPROGRESS	0x01	/* callout is in progress of dispatching */
#define	C_FIRED		0x02	/* callout has fired and is awaiting
				   disposal from untimeout */
#define	C_STATIC	0x10	/* callout structure is part of the static reserve */
#define	C_DEAD		0x20	/* callout structure is invalid */
#define C_PERIODIC	0x40	/* periodic repeating callout */
	struct callout **c_prevpp;/* pointer to pointer of which sub-tree
				     we are on our parent; i.e. if we're on
				     our parent's "c_lower" sub-tree, this
				     points to c_parentp->c_lower; this
				     allows us to splice nodes without
				     having to examine our parent */
	struct callout *c_parentp;/* pointer to parent */
	struct callout **c_hdr;	/* where this callout is queued on */
	struct callout *c_lower;/* sub-tree of callouts with a higher time */
	struct callout *c_higher;/* sub-tree of callouts with a lower time */
#define	c_next	c_lower		/* free-list next pointer */
};

/*
 * Macro which allows the caller to loop against c_flags&C_STAT without having
 * the compiler perform any unwanted optimizations.
 */
#define	c_volatile_stat(co) \
		((ushort_t)(*((volatile ushort_t *)&(co)->c_flags)) & C_STAT)

/*
 * The following hash list allows fast lookup of a callout for
 * purposes of untimeout.
 */
#define	CALLOUT_HASH(id)	((id) & (NCALLOUT_HASH - 1))
STATIC list_t coid_hash[NCALLOUT_HASH];

/*
 * Add/delete a callout-id to/from the hash-list.
 */
#define	add_idhash(co)	insque(&co->c_q, \
			        coid_hash[CALLOUT_HASH(co->c_id)].rlink)

#define	sub_idhash(co)	remque(&co->c_q)

STATIC struct callout *callout_free(struct callout *, struct callout *);
STATIC void addcallout(struct callout *, long);
STATIC struct callout *subcallout(struct callout *, struct callout *);
STATIC struct callout *find_idhash(toid_t);
STATIC struct callout *callout_private(void);
STATIC void free_callouts(struct callout *);

/*
 * Current time-out id for allocation.  Must not be zero, as zero
 * indicates a failed itimeout/dtimeout.
 */
STATIC toid_t timeid;
STATIC void toid_alloc(struct callout *);

/*
 * This is the global list of pending callouts.
 */
STATIC struct callout *global_todo;

/*
 * This is a private reserve of callout structures.  Usually, callouts
 * are allocated via "kmem_alloc(..., KM_NOSLEEP)".  If this should fail,
 * "cofree" is a "private reserve" of callouts which should allow normal
 * interrupt-level functioning.  The size of this private reserve is
 * determined at system generation time.
 *
 * Each callout buffer in the reserve has the potential to tie down an
 * entire page from the memory pool unless the fragmentation problems are
 * considered.  To avoid fragmentation, the reserve consists of a static
 * set of buffers and a dynamic set of buffers.  Static buffers are
 * contiguously allocated at itimeout_init time as one big chunk of memory
 * and are never returned to the KMA pool.  Dynamic buffers are allocated
 * via kmem_alloc and are returned via kmem_free.
 * When the reserve falls below its configured level, freed dynamic buffers
 * are temporarily added to the reserve in an attempt to keep the reserve
 * at its configured level.  As static buffers are returned to the pool, these
 * dynamic buffers are returned to the KMA pool via kmem_free.  In a low
 * memory situation, the reserve will be composed of both static and dynamic
 * buffers.  When the low-memory situation passes, the dynamic buffers will
 * be purged from the reserve and only the static buffers will remain.
 */
STATIC struct callout *static_cofree;
STATIC struct callout *dynamic_cofree;
STATIC uint_t ncofree;		/* total number of free in private reserve */
STATIC uint_t static_ncofree;	/* static number of buffers in reserve */
STATIC uint_t dynamic_ncofree;	/* dynamic number of buffers in reserve */
STATIC uint_t co_nactive;	/* for debug: number of callouts active */
STATIC uint_t co_ndead;		/* number of dead items on the tree */

/*
 * svckmadv()
 *	Call kmem_advise for callout struct.
 *
 * Calling/Exit State:
 *	Called at sysinit time while still single threaded.
 */
void
svckmadv()
{
	kmem_advise(sizeof(struct callout));
}


/*
 * Timeout/Untimeout Interfaces:
 *
 * There are two basic types of time-out:  global -- fielded by any processor
 * on the system and local -- fielded by a named processor on the system.
 *
 * Global timeout's are the usual case.  These are maintained from the time
 * of day interrupt handler.  The TOD interrupt can be fielded by any processor
 * on the system.  The processor fielding the TOD interrupt will maintain and
 * dispatch global timeouts, subject to the usual rules.
 *
 * Local timeout's are used to provide support for uniprocessor and asymmetric
 * I/O code.  A local timeout is directed to a named processor.  The processor
 * maintains and dispatches the local callout from its per-processor clock
 * interrupt routine.
 *
 * The following are the interfaces supported:
 *
 * toid_t itimeout(void (*fcn)(), void *arg, long ticks, pl_t pl)
 *
 * Itimeout is the usual case and queues "<fcn, arg>" to be dispatched in
 * approximately "ticks" clock ticks.  "fcn" will be called at interrupt
 * level "pl".
 *
 * Itimeout allocates the space it needs to track the callout using
 * "kmem_alloc(..., KM_NOSLEEP)".  Since itimeout may be called while holding
 * spin-locks or from interrupt handlers, it cannot sleep waiting for
 * space.  If itimeout is unable to get space from kmem_alloc, it will attempt
 * to allocate from a private reserve allocated at boot time.  This private
 * reserve is intended to be sufficient to support processing until the
 * VM sub-system can provide additional space.  If no space can be gotten
 * from either kmem_alloc or the private reserve, itimeout returns zero and
 * expects the caller to take appropriate action.
 *
 * Itimeout associates and returns an id with the callout.  This callout may
 * be canceled via "untimeout" specifying the returned id.  Interrupt
 * handlers calling "untimeout" must be careful the call to "itimeout"
 * which establishes the callout specified an IPL high enough to mask the
 * interrupt handler.  Otherwise deadlock could occur.
 *
 * The deadlock of concern here is the result of a self-processor deadlock
 * involving a dispatched callout being interrupted by a handler which
 * attempts to cancel the callout being dispatched.
 *
 * When untimeout returns, there must be no possibility of the callout
 * happening.  Thus, untimeout must either remove it from the callout-queue
 * if the callout has yet to be dispatched, must detect the fact that the
 * callout has already been dispatched, or must detect the fact the callout
 * is currently being dispatched and wait for it to complete.
 *
 * If a processor is dispatching a callout, and the callout is interrupted
 * by a handler which attempts to untimeout the same callout, untimeout must
 * wait for the callout to complete, as the callout is currently being
 * dispatched.  Since the untimeout has interrupted the dispatching callout,
 * the callout will never complete and we're in the deadlock state.
 *
 * toid_t itimeout_a(void (*fcn)(), void *arg, long ticks, pl_t pl,
 *		     void *co)
 *
 * toid_t itimeout_l_a(void (*fcn)(), void *arg, long ticks, pl_t pl,
 *		       void *co)
 *
 * Itimeout_a is exactly like itimeout, except the caller has preallocated
 * the callout structure using "itimeout_allocate".  "co" is the space
 * allocated by "itimeout_allocate" and is known to be the space needed to
 * track the callout entry.  Itimeout_l_a is like itimeout_a, except the
 * TIME_LOCK is held across the call.
 *
 * Note that timeout_allocate/itimeout_a/itimeout_l_a deal with the space as a
 * (void *) in order to avoid having to export the callout structure definition
 * outside of this file.
 *
 * void *itimeout_allocate(int flags)
 *
 * Itimeout_allocate allocates space necessary to queue a callout entry.
 * It allocates this space using "kmem_alloc(..., KM_SLEEP)", which is known
 * to succeed.  It is used by process-level code which can tolerate sleeping
 * in order to allocate the structures to queue the callout.
 *
 * itimeout_allocate/itimeout_l_a is used by setitimer and alarm when setting
 * up a callout to trigger the alarm, since setitimer and alarm can sleep
 * before gaining the time-mutex.
 *
 * int timeout(void (*fcn)(), caddr_t arg, long ticks)
 *
 * Timeout is a compatibility interface for usage by uniprocessor code
 * (e.g. device drivers).  It duplicates the semantics of the SVR4 timeout
 * interface.  It provides much the same semantics as "itimeout", except the
 * "<fcn, arg>" is dispatched at PLHI.  SVR4's timeout allows the dispatched
 * function to lower the interrupt level below PLHI.  This is an unfortunate
 * mis-usage of interrupt priority allowed by SVR4 which must be supported
 * here.  Consistency checking during interrupt-level manipulation would
 * normally fail such a request, however, such checking will be disabled
 * for the compatibility routine.
 *
 * toid_t dtimeout(void (*fcn)(), void *arg, long ticks, pl_t pl,
 * 		   processorid_t processor)
 *
 * Dtimeout works the same as itimeout, except the callout will always be
 * dispatched on "processor".
 *
 * untimeout(toid_t id)
 *
 * Untimeout is used to cancel any callout scheduled from the any of the
 * timeout interfaces above.  When untimeout returns, the callout associated
 * with "id" will have been canceled, or will have already occured.  As
 * already mentioned, if the callout is in the process of dispatch, untimeout
 * will busy-wait for it to complete.
 *
 * Untimeout is subject to all the interrupt-handler restrictions mentioned
 * above.  In addition, callers of untimeout must not be holding spin-locks
 * contested by the callout routine being called.  Doing so could result
 * in a deadlock, where one processor holds a lock, calls untimeout against
 * a callout which contests the lock.  If the callout is dispatched on
 * another processor, the untimeout will spin waiting for the callout to
 * complete, while the callout will spin, waiting for the lock to be released.
 * This is technically a lock-ordering problem.
 */
/*
 * void itimeout_init(void)
 *
 *	Allocate callout private reserve.
 *
 * Calling/Exit State:
 *
 *	Allocate a pool of callout structures to be used in low-memory
 *	circumstances where kmem_alloc(..., KM_SLEEP) is unable to
 *	allocate memory.
 */
STATIC void
itimeout_init(void)
{
	struct callout *co;
	struct callout *end;
	int i;
	list_t *lp;

	co = kmem_zalloc(sizeof(*co)*v.v_call, KM_NOSLEEP);
	if (co == NULL) {
		/*
		 *+ The kernel was unable to allocate necessary
		 *+ space for the private-reserve of callout entries.
		 *+ This allocation is being attempted at boot time
		 *+ when there should be plenty of memory available.
		 *+ Corrective action:  it may be an excessive amount
		 *+ of configurable data structures have been allocated
		 *+ for the current memory configuration.  The
		 *+ current configuration should be examined and
		 *+ scaled back if it is found excessive for the
		 *+ current machine.
		 */
		cmn_err(CE_PANIC, "Cannot allocate private-reserve of "
				  "callouts");
	}

	ncofree = v.v_call;
	static_ncofree = v.v_call;

	/*
	 * Initialize the static reserve.
	 */
	for (end = co + v.v_call; co < end; co++) {
		co->c_next = static_cofree;
		static_cofree = co;
		co->c_flags |= C_STATIC;
	}

	/*
	 * Initialize the hash list.
	 */
	for (i=0, lp = &coid_hash[0]; i < NCALLOUT_HASH; i++, lp++) {
		INITQUE(lp);
	}
}

/*
 * toid_t itimeout(void (*fcn)(), void *arg, long ticks, pl_t pl)
 *
 *	Arrange for a function to be called in the future.
 *
 * Calling/Exit State:
 *
 *	Gains and releases time_mutex, thus caller cannot hold time_mutex.
 *	Returns an id for usage in subsequent call to untimeout or zero on
 *	failure.
 *
 * Description:
 *
 *	Arrange for "fcn" to be called with argument "arg" in "ticks" ticks
 *	from now at priority-level "pl".  See block comment above for more
 *	detail.
 */
toid_t
itimeout(void (*fcn)(), void *arg, long ticks, pl_t pl)
{
	struct callout *co;
	toid_t id;

	co = (struct callout *)kmem_zalloc(sizeof(*co), KM_NOSLEEP);
	TIME_LOCK();
	if (co == NULL) {
		/*
		 * Attempt to dig ourselves out of a hole by grabbing
		 * a callout buffer from the reserved pool.
		 */
		co = callout_private();
		if (co == NULL) {
			TIME_UNLOCK();
			return 0;
		}
	}

	co->c_fcn = fcn;
	co->c_arg = arg;
	if (ticks & TO_PERIODIC) {
		ticks &= ~TO_PERIODIC;
		co->c_flags |= C_PERIODIC;
		co->c_want = 1;
		co->c_period = ticks;
	}
	if (ticks <= 0)
		co->c_period = ticks = 1;

	if (pl == PLBASE)
		pl = PLTIMEOUT;
	co->c_pl = pl;

	/* Allocate an ID for this callout. */
	toid_alloc(co);
	id = co->c_id;

	co->c_hdr = &global_todo;
	addcallout(co, ticks);
	co_nactive++;

	TIME_UNLOCK();

	return id;
}

/*
 * toid_t itimeout_u(void (*fcn)(), void *arg, long ticks, pl_t pl)
 *
 *	Arrange for a function to be called in the future.
 *	Version for single-threaded drivers.
 *
 * Calling/Exit State:
 *
 *	Gains and releases time_mutex, thus caller cannot hold time_mutex.
 *	Returns an id for usage in subsequent call to untimeout or zero on
 *	failure.
 *
 * Description:
 *
 *	Arrange for "fcn" to be called with argument "arg" in "ticks" ticks
 *	from now at priority-level "pl".  See block comment above for more
 *	detail.  Does the same thing as itimeout, except bound to current
 *	engine.
 */
toid_t
itimeout_u(void (*fcn)(), void *arg, long ticks, pl_t pl)
{
	return dtimeout(fcn, arg, ticks, pl, myengnum);
}

/*
 * toid_t dtimeout(void (*fcn)(), void *arg, long ticks, pl_t pl,
 *		   processorid_t proc);
 *
 *	Arrange for a function to be called in the future by a given processor.
 *
 * Calling/Exit State:
 *
 *	Gains and releases time_mutex, thus caller cannot hold time_mutex.
 *	Returns an id for usage in subsequent call to untimeout and zero
 *	if the callout was not able to be queued.
 *
 * Description:
 *
 *	Arrange for "fcn" to be called with argument "arg" in "ticks" ticks
 *	from now at priority-level "pl" on processor "proc".  See block
 *	comment above for more detail.
 *
 *	Note that arrangements have been made to keep the engine named by
 *	proc from being offlined while there are still active clients of
 *	dtimeout.  If these clients are ever deactivated (e.g. unload a
 *	loadable device driver), the shutdown code of the client must be
 *	responsible for canceling any pending dtimeouts it has queued
 *	against the processor in question.
 */
toid_t
dtimeout(void (*fcn)(), void *arg, long ticks, pl_t pl, processorid_t proc)
{
	struct engine *eng;
	struct callout *co;
	toid_t id;

	/*
	 * Put a cap on the maximium amount of time we can wait.
	 * This is about 248 days on a 100 hz, 32-bit system.
	 */
	ASSERT((ulong_t)(ticks & ~TO_PERIODIC) <= CALLOUT_MAXVAL);

	eng = PROCESSOR_MAP(proc);
	if (eng == NULL) {
		/*
		 *+ An asymmetric device driver passed an invalid processor
		 *+ id to the dtimeout.
		 */
		cmn_err(CE_WARN, "Bad processorid from dtimeout");
		return 0;
	}

	co = (struct callout *)kmem_zalloc(sizeof(*co), KM_NOSLEEP);
	TIME_LOCK();
	if (co == NULL) {
		co = callout_private();
		if (co == NULL) {
			TIME_UNLOCK();
			return 0;
		}
	}

	co->c_fcn = fcn;
	co->c_arg = arg;
	if (ticks & TO_PERIODIC) {
		ticks &= ~TO_PERIODIC;
		co->c_flags |= C_PERIODIC;
		co->c_want = 1;
		co->c_period = ticks;
	}
	if (ticks <= 0)
		co->c_period = ticks = 1;

	if (pl == PLBASE)
		pl = PLTIMEOUT;
	co->c_pl = pl;

	/* Allocate an ID for this callout. */
	toid_alloc(co);
	id = co->c_id;

	co->c_hdr = &eng->e_local_todo;
	addcallout(co, ticks);
	co_nactive++;

	TIME_UNLOCK();

	return id;
}

/*
 * toid_t itimeout_l_a(void (*fcn)(), void *arg, long ticks, pl_t pl,
 *		       void *space)
 *
 *	Arrange for a function to be called in the future for a caller who
 *	has pre-allocated space and is holding the time_mutex.
 *
 * Calling/Exit State:
 *
 *	Caller must be holding time_mutex.
 *
 * Description:
 *
 *	Same as itimeout, except space is passed in, and
 *	the caller holds time_mutex.
 */
toid_t
itimeout_l_a(void (*fcn)(), void *arg, long ticks, pl_t pl, void *space)
{
	struct callout *co = (struct callout *)space;

	ASSERT(TIME_OWNED());
	ASSERT(co != NULL);

	/*
	 * Put a cap on the maximium amount of time we can wait.
	 * This is about 248 days on a 100 hz, 32-bit system.
	 */
	ASSERT((ulong_t)(ticks & ~TO_PERIODIC) <= CALLOUT_MAXVAL);

	co->c_fcn = fcn;
	co->c_arg = arg;
	if (ticks & TO_PERIODIC) {
		ticks &= ~TO_PERIODIC;
		co->c_flags |= C_PERIODIC;
		co->c_want = 1;
		co->c_period = ticks;
	}
	if (ticks <= 0)
		co->c_period = ticks = 1;

	if (pl == PLBASE)
		pl = PLTIMEOUT;

	co->c_pl = pl;

	/* Allocate an ID for this callout. */
	toid_alloc(co);

	co->c_hdr = &global_todo;
	addcallout(co, ticks);
	co_nactive++;

	return co->c_id;
}

/*
 * toid_t itimeout_a(void (*fcn)(), void *arg, long ticks, pl_t pl,
 *		     void *space)
 *
 *	Arrange for a function to be called in the future for a caller who
 *	has pre-allocated space.
 *
 * Calling/Exit State:
 *
 *	This routine is guaranteed not to block.
 *
 * Description:
 *
 *	Same as itimeout, except space is passed in.
 */
toid_t
itimeout_a(void (*fcn)(), void *arg, long ticks, pl_t pl, void *space)
{
	toid_t	toid;

	TIME_LOCK();
	toid = itimeout_l_a(fcn, arg, ticks, pl, space);
	TIME_UNLOCK();
	return toid;
}

toid_t
itimeout_periodic_l_a(void (*fcn)(), void *arg, long ticks, long period,
				 pl_t pl, void *space)
{
	struct callout *co = (struct callout *)space;

	ASSERT(TIME_OWNED());
	ASSERT(co != NULL);

	/*
	 * Put a cap on the maximium amount of time we can wait.
	 * This is about 248 days on a 100 hz, 32-bit system.
	 */
	ASSERT((ulong_t) ticks <= CALLOUT_MAXVAL);
	ASSERT((ulong_t) period <= CALLOUT_MAXVAL);

	co->c_fcn = fcn;
	co->c_arg = arg;

	if (ticks <= 0)
		ticks = 1;

	if (period > 0)
	{
		co->c_flags |= C_PERIODIC;
		co->c_want = 1;
		co->c_period = period;
	}

	if (pl == PLBASE)
		pl = PLTIMEOUT;

	co->c_pl = pl;

	/* Allocate an ID for this callout. */
	toid_alloc(co);

	co->c_hdr = &global_todo;
	addcallout(co, ticks);
	co_nactive++;

	return co->c_id;
}

/*
 * void *itimeout_allocate(int flags)
 *
 *	Allocate callout space for a caller who can afford to wait.
 *
 * Calling/Exit State:
 *
 *	Returns the address of space to be passed to a subsequent
 *	itimeout_l_a.
 *
 * Description:
 *
 *	Itimeout_allocate allocates space for a caller who can afford
 *	to sleep.  The caller then passes this space to a subsequent
 *	itimeout_l_a.
 */
void *
itimeout_allocate(int flags)
{
	void *co;

	co = kmem_zalloc(sizeof(struct callout), flags);

	if (co == NULL) {
		ASSERT(flags & KM_NOSLEEP);
		TIME_LOCK();
		co = callout_private();
		TIME_UNLOCK();
	}

	return co;
}

/*
 * void itimeout_free(void *)
 *
 *	Return space previously allocated via itimeout_allocate, but never
 *	used.
 *
 * Calling/Exit State:
 *
 *	Frees space previously allocated by itimeout_allocate.
 */
void
itimeout_free(void *co)
{
	kmem_free(co, sizeof(struct callout));
}

/*
 * toid_t timeout(void (*fcn)(), void *arg, long ticks)
 *
 *	Arrange for a function to be called in the future (old-style).
 *
 * Calling/Exit State:
 *
 *	Should not be called holding time_mutex.  Panics if a callout
 *	structure cannot be allocated.
 *
 * Description:
 *
 *	Compatibility interface.  Does the same thing as itimeout,
 *	except, dispatched priority level is always PLHI and we panic
 *	if the callout structure cannot be allocated.
 */
toid_t
timeout(void (*fcn)(), caddr_t arg, long ticks)
{
	toid_t id;

	/*
	 * Put a cap on the maximium amount of time we can wait.
	 * This is about 248 days on a 100 hz, 32-bit system.
	 */
	ASSERT((ulong_t)ticks <= CALLOUT_MAXVAL);

	id = itimeout(fcn, arg, ticks, PLHI);
	if (id == 0) {
		/*
		 *+ The kernel was unable to allocate space for
		 *+ a timeout request.  Since timeout is a comp-
		 *+ tability interface, the kernel cannot return
		 *+ an error and allow processing to be handler
		 *+ by the caller.  Corrective action:  investigate
		 *+ compatibility device drivers for excessive usage
		 *+ of timeout or increase the v_call parameter of
		 *+ the system configuration.
		 */
		cmn_err(CE_PANIC, "Cannot allocate callout for timeout.");
		/* NOTREACHED */
	}

	return id;
}

/*
 * toid_t timeout_u(void (*fcn)(), void *arg, long ticks)
 *
 *	Arrange for a function to be called in the future (old-style).
 *	Version for single-threaded drivers.
 *
 * Calling/Exit State:
 *
 *	Gains and releases time_mutex, thus caller cannot hold time_mutex.
 *	Returns an id for usage in subsequent call to untimeout.
 *
 * Description:
 *
 *	Does the same thing as timeout, except bound to current engine.
 */
toid_t
timeout_u(void (*fcn)(), void *arg, long ticks)
{
	toid_t id;

	/*
	 * Put a cap on the maximium amount of time we can wait.
	 * This is about 248 days on a 100 hz, 32-bit system.
	 */
	ASSERT((ulong_t)ticks <= CALLOUT_MAXVAL);

	id = dtimeout(fcn, arg, ticks, PLHI, myengnum);
	if (id == 0) {
		/*
		 *+ The kernel was unable to allocate space for
		 *+ a timeout request.  Since timeout is a comp-
		 *+ tability interface, the kernel cannot return
		 *+ an error and allow processing to be handler
		 *+ by the caller.  Corrective action:  investigate
		 *+ compatibility device drivers for excessive usage
		 *+ of timeout or increase the v_call parameter of
		 *+ the system configuration.
		 */
		cmn_err(CE_PANIC, "Cannot allocate callout for timeout.");
		/* NOTREACHED */
	}

	return id;
}

/*
 * int untimeout_r(toid_t id)
 *
 *	Delete a previously scheduled timeout.
 *
 * Calling/Exit State:
 *
 *	Same as untimeout.
 *
 * Description:
 *
 *	Untimeout_r does the same thing as untimeout, except it returns
 *	non-zero if the timeout was canceled before firing.
 */
int
untimeout_r(toid_t id)
{
	struct callout *co;

	TIME_LOCK();

	co = find_idhash(id);
	if (co == NULL) {
		/*
		 * Not found on the system.
		 * Must have already fired.
		 */
		TIME_UNLOCK();
		return 0;
	}

	/*
	 * If this was a periodic repeating timer, it is no longer.
	 */
	if (co->c_flags & C_PERIODIC) {
		co->c_flags &= ~C_PERIODIC;
		ASSERT(co->c_want == 1);
		co->c_want = 0;
	}

	/*
	 * We're holding time_mutex.  Nobody can dispatch this callout
	 * while we're holding time_mutex.  If "co" isn't currently being
	 * dispatched, we can kill it off now and return.
	 */
	if ((co->c_flags & C_STAT) == C_WAITING) {
		/*
		 * Not dispatching, or noone else waiting,
		 * kill it off and return.
		 */
		co = subcallout(co, NULL);
		TIME_UNLOCK();
		free_callouts(co);
		return 1;
	} else if ((co->c_flags & C_STAT) == C_FIRED) {
		/*
		 * Callout has already fired and there are other
		 * untimeout_r's waiting for it.  We happened to
		 * jump in and gain the time_mutex before one of
		 * of the waiters got a hold of it.  We really don't
		 * need to do anything further, as one of the waiters
		 * will take care of disposing of the data structures.
		 */
		TIME_UNLOCK();
		return 0;
	}

	/*
	 * Callout is in progress, must drop locks and wait for it
	 * to be released.
	 */
	co->c_want++;
	TIME_UNLOCK();

	/*
	 * Wait on the callout-entry.  The dispatcher will set the
	 * c_flags&C_STAT of the callout to C_FIRED after the callout completes.
	 */
	while (c_volatile_stat(co) != C_FIRED)
		continue;

	/*
	 * At this point, we know the callout has dispatched.
	 * We've only got to be careful of others waiting for the same
	 * dispatch.
	 */
	ASSERT(c_volatile_stat(co) == C_FIRED);

	TIME_LOCK();

	if (--co->c_want == 0) {
		/*
		 * Need to release the callout structure.
		 */
		co = subcallout(co, NULL);
		TIME_UNLOCK();
		free_callouts(co);
		return 1;
	} /* else */
		/*
		 * Leave the deallocation to one of the other waiters.  They'll
		 * contend for it and the last will dealloc.
		 */

	TIME_UNLOCK();

	return 1;
}

/*
 * void untimeout(toid_t id)
 *
 *	Delete a previously scheduled timeout.
 *
 * Calling/Exit State:
 *
 *	Caller should not be holding locks which the callout identified
 *	by id may contend.  Otherwise, deadlock will occur.  Caller should
 *	not be holding the time_mutex.
 *
 * Description:
 *
 *	Untimeout deletes the callout identified by "id".  If the callout
 *	has not been dispatched, it is deleted immediately, otherwise, it
 *	waits for the dispatch of the callout to complete.  Upon return,
 *	it is known the callout has either fired or been canceled.
 */
void
untimeout(toid_t id)
{
	(void)untimeout_r(id);
}

/*
 * struct callout *callout_free(struct callout *co, struct callout *free)
 *
 *	Free the callout structure associated with "co".
 *
 * Calling/Exit State:
 *
 *	Must be called with the time_mutex held.  Returns the accumulated
 *	list of unused callout buffers.
 *
 * Description:
 *
 *	Callout_free adds "co" to the private reserve if it has been
 *	depleted or links the unused callout to the chain pointed to by
 *	"free".  We accumulate a list of unused callouts, as we cannot
 *	(and should not even if it was possible) call kmem_free while
 *	holding the time_mutex.
 */
struct callout *
callout_free(struct callout *co, struct callout *free)
{
	ASSERT(TIME_OWNED());

	if (co->c_flags & C_STATIC) {
		/*
		 * Statically allocated buffer.  We must always
		 * accept this back into the pool.
		 */
		ASSERT(static_ncofree < v.v_call);

		co->c_next = static_cofree;
		static_cofree = co;
		static_ncofree++;
		ncofree++;

		if (ncofree >= v.v_call && dynamic_ncofree != 0) {
			/*
			 * Reserve is back up to size, release
			 * a dynamic buffer back to the KMA pool.
			 */
			ASSERT(dynamic_cofree != NULL);
			co = dynamic_cofree;
			dynamic_cofree = co->c_next;
			ASSERT(!(co->c_flags & C_STATIC));
			ncofree--;
			dynamic_ncofree--;
			co->c_next = free;
			return co;
		}

		return free;
	}

	/*
	 * Dynamically allocated buffer.  See if we should temporarily
	 * accept this into the reserve.
	 */
	if (ncofree < v.v_call) {
		/*
		 * We attempt to keep the reserve greater than v_call.
		 * Until all the static buffers are freed, the reserve will
		 * consist of both static and dynamic buffers.
		 */
		ASSERT(dynamic_ncofree <= ncofree);
		co->c_next = dynamic_cofree;
		dynamic_cofree = co;
		ncofree++;
		dynamic_ncofree++;
		return free;
	}

	/*
	 * This is a dynamic buffer and the reserve is at its desired level.
	 */
	co->c_next = free;
	return co;
}

/*
 * void free_callouts(struct callout *co)
 *
 *	Free the list of unused callout structures pointed to by "co".
 *
 * Calling/Exit State:
 *
 *	Must not be holding the time_mutex.  Frees the chain of unused
 *	callout structures pointed to by "co".
 */
void
free_callouts(struct callout *co)
{
	struct callout *next;

	while (co != NULL) {
		next = co->c_next;
		kmem_free(co, sizeof(*co));
		co = next;
	}

	return;
}

/*
 * struct callout *callout_private(void)
 *
 *	Grab a callout structure from the private reserve.
 *
 * Calling/Exit State:
 *
 *	Must be called with the time_mutex held.
 *
 * Description:
 *
 *	Callout allocates a callout structure from the private reserve
 *	of callout structures.  If the reserve is exhausted, NULL is
 *	returned.
 */
STATIC struct callout *
callout_private(void)
{
	struct callout *co;

	/*
	 * Take buffers from the dynamic pool first, as they're less work
	 * to free up.
	 */
	co = dynamic_cofree;
	if (co) {
		ASSERT(!(co->c_flags & C_STATIC));
		dynamic_cofree = co->c_next;
		dynamic_ncofree--;
		ncofree--;
		bzero(co, sizeof(*co));
		return co;
	}

	/*
	 * Nothing in the dynamic, try the static portion of the reserve.
	 */
	co = static_cofree;
	if (co) {
		ASSERT((co->c_flags & C_STATIC) != 0);
		static_cofree = co->c_next;
		static_ncofree--;
		ncofree--;
		bzero(co, sizeof(*co));
		co->c_flags |= C_STATIC;
		return co;
	}

	/*
	 * Nothing there, just return NULL.
	 */
	return NULL;
}

/*
 * int callout_tick(struct callout **todo)
 *
 *	Do "once per tick" processing against a callout list.
 *
 * Calling/Exit State:
 *
 *	Must be called holding the time_mutex.
 *	Returns non-zero if callouts are due, zero otherwise.
 *
 * Description:
 *
 *	Advance the "todo" list pointed by "todo" by one tick and return
 *	non-zero if any callouts have become due.  We decrement the
 *	relative time associated with the root node and descend the
 *	tree looking for a callout which has fired.
 */
int
callout_tick(struct callout **todo)
{
	struct callout *cp;
	long t;

	ASSERT(TIME_OWNED());

	cp = *todo;
	if (cp == NULL)
		return 0;

	/*
	 * Decrement the root callout structure entry.  This has the
	 * effect of decrementing all nodes in the tree.
	 */
	if (--cp->c_time <= 0)
		/*
		 * Know there is something to fire, even if the root
		 * is dead, as a dead node has two live sub-trees, one
		 * of which is lower.
		 */
		return 1;

	/*
	 * Walk down the tree to the lowest valued terminal node.  The
	 * leftmost node is the earliest to fire, therefore, we only need
	 * move left.
	 */
	t = cp->c_time;
	while (cp != NULL) {
		cp = cp->c_lower;
		if (cp)
			t += cp->c_time;
		if (t <= 0)
			return 1;
	}

	return 0;
}

/*
 * struct callout *callout_get(struct callout *co, long *timep)
 *
 *	Get a callout which is ready to fire.
 *
 * Calling/Exit State:
 *
 *	Called and returns with time_mutex held.
 *
 * Description:
 *
 *	Walks down the todo tree starting at "co", looking for the lowest
 *	valued callout on the tree.  This is known to be the node on the
 *	farthest left-hand side of the tree.  We find this node and if
 *	it is ready to fire, we return it, NULL otherwise.
 *
 *	On a non-NULL return, (*timep) is set to the relative time at which
 *	"co" was scheduled to have fired.
 */
STATIC struct callout *
callout_get(struct callout *co, long *timep)
{
	struct callout *cp;
	long t;

	if (co == NULL)
		return NULL;

	/*
	 * Walk down the tree, looking for the lowest valued callout.  This
	 * is known to be the node on the farthest left of the tree.
	 * We don't bother with c_higher nodes, as if the left-most node
	 * has a higher sub-tree, it will be promoted by callout_subtodo
	 * and it or one of its lower descendents will be found the next
	 * pass through.
	 */
	cp = co;
	t = cp->c_time;
	while (cp->c_lower != NULL) {
		cp = cp->c_lower;
		t += cp->c_time;
	}

	ASSERT(!(cp->c_flags & C_DEAD) &&
	       (co->c_flags & C_STAT) == C_WAITING);

	if (t <= 0) {
		*timep = t;
		return cp;
	}

	return NULL;
}

STATIC struct callout *delete_up(struct callout *, struct callout *);

/*
 * int callout_subtodo(struct callout *co, struct callout **freepp)
 *
 *	Remove a callout from its todo tree.
 *
 * Calling/Exit State:
 *
 *	Must be called and returns with the time-mutex held.
 *
 * Description:
 *
 *	Callout_subtodo removes "co" from its current callout tree.  If
 *	we are the parent of two sub-trees, we simply mark ourselves as
 *	C_DEAD and leave things are they are.  As time progresses, the
 *	callouts in our lower sub-tree will fire and eventually we'll
 *	be harvested from delete_up.
 *	If we have only a left or right-sub-tree, we promote the sub-tree
 *	to our current position and free the callout entry.  If we're
 *	a terminal node (i.e. no sub-trees), we delete ourselves from the
 *	tree and call "delete_up" to walk upwards, and harvest any dead
 *	parents.  Harvest is only necessary on terminal nodes, as non-terminal
 *	nodes don't really change the status of the parent.
 */
STATIC int
callout_subtodo(struct callout *co, struct callout **freepp)
{
	struct callout *cn;

	/*
	 * Delete the callout from its "todo" tree.
	 */
	ASSERT(co->c_hdr != NULL);
	ASSERT((co->c_flags & C_STAT) == C_WAITING);

	if (co->c_lower == NULL) {
		cn = co->c_higher;
		if (cn == NULL) {
			/*
			 * We're a terminal node, just delete.
			 */
			*co->c_prevpp = NULL;
			*freepp = delete_up(co->c_parentp, *freepp);
		} else {
			/*
			 * This is easy, we can just promote the
			 * higher node to our current position.
			 */
			ASSERT(cn->c_parentp == co);
			*co->c_prevpp = cn;
			cn->c_parentp = co->c_parentp;
			cn->c_prevpp = co->c_prevpp;
			cn->c_time += co->c_time;
		}
		return 1;
	} else if (co->c_higher == NULL) {
		cn = co->c_lower;
		/*
		 * We can just promote the lower node to our
		 * current position.
		 */
		ASSERT(cn->c_parentp == co);
		*co->c_prevpp = cn;
		cn->c_parentp = co->c_parentp;
		cn->c_prevpp = co->c_prevpp;
		cn->c_time += co->c_time;
		return 1;
	}

	/*
	 * We've got sub-trees on either side, just
	 * delare this node dead.
	 */
	co->c_flags |= C_DEAD;
	co_ndead++;

	return 0;
}

/*
 * void delete_up(struct callout *co)
 *
 *	Remove dead nodes moving up the tree.
 *
 * Calling/Exit State:
 *
 *	Must be called with the time_mutex held.
 *
 * Description:
 *
 *	Called when terminal node has been deleted.  This routine
 *	walks back up the tree, deleting any dead, non-terminal nodes
 *	which may have just been freed.  We can free a parent node if
 *	it's dead and both sub-nodes are free (i.e. our parent had only
 *	one sub-tree, which was just deleted).  If the other sub-node
 *	is busy, we can promote it and delete the parent entry.  We
 *	continue the freeing until we reach a non-dead node or a dead
 *	node with two live sub-trees.  We know our parent cannot be a
 *	newly-created terminal node, or we would have been promoted
 *	when the other sub-tree was deleted.
 *
 */
STATIC struct callout *
delete_up(struct callout *co, struct callout *free)
{
	struct callout *cn;

	/*
	 * Free or promote until we hit the root or a live
	 * node.
	 */
	while (co != NULL && co->c_flags & C_DEAD) {
		if (co->c_lower == NULL) {
			/*
			 * Can promote the higher node to
			 * the current node and free the current
			 * node.
			 */
			cn = co->c_higher;
			co_ndead--;
			ASSERT(cn != NULL);
			ASSERT(cn->c_parentp == co);
			*co->c_prevpp = cn;
			cn->c_parentp = co->c_parentp;
			cn->c_prevpp = co->c_prevpp;
			cn->c_time += co->c_time;
			cn = co->c_parentp;
			free = callout_free(co, free);
		} else if (co->c_higher == NULL) {
			/*
			 * Can promote the lower node to
			 * the current node and free the current
			 * node.
			 */
			co_ndead--;
			cn = co->c_lower;
			ASSERT(cn->c_parentp == co);
			*co->c_prevpp = cn;
			cn->c_parentp = co->c_parentp;
			cn->c_prevpp = co->c_prevpp;
			cn->c_time += co->c_time;
			cn = co->c_parentp;
			free = callout_free(co, free);
		} else {
			/*
			 * Struck a dead node with two live sub-trees.
			 * This can happen if we promote a live sub-tree
			 * to a dead parent.  Can't do anything further
			 * to this or any of our ancestors.
			 */
			break;
		}
		co = cn;
	}

	return free;
}

/*
 * void dispcallout(struct callout **todo)
 *
 *	Dispatch callouts.
 *
 * Calling/Exit State:
 *
 *	Should be called with the time_mutex released.
 *
 * Description:
 *
 *	Locking needs to be careful while dispatching, as a dispatched routine
 *	will typically set up the next callout to itself, thus, we can't keep
 *	the free and callout lists locked while dispatching, yet we can't
 *	allow untimeout's to complete while the corresponding function is in
 *	the process of dispatching.  For these reasons, we mark the callout
 *	structure as being dispatched.  If someone is waiting for us, we mark
 *	the dispatched callout as fired and allow them to deallocate the
 *	callout once they're done with it.
 *	We must run at PLHI and lower the priority to the callout level.
 *	If we try to run at PLTIMEOUT and raise the priority to the
 *	callout level after setting C_INPROGRESS and releasing the
 *	TIME_LOCK below, we would open a window where an interrupt
 * 	could come in and try to do an untimeout of the callout that
 * 	we are trying to dispatch, and untimeout would loop forever
 *	waiting for it to fire.
 */
STATIC void
dispcallout(struct callout **todo)
{
	struct callout *co;
	struct callout *free;
	long fire_time;
	pl_t pl;

	free = NULL;

	pl = splhi();
	TIME_LOCK();

	for (;;) {
		if (psm_intrpend(pl)) {
			TIME_UNLOCK();
			splx(pl);
			(void) splhi();
			TIME_LOCK();
		}
		co = callout_get(*todo, &fire_time);
		if (co == NULL) {
			/*
			 * Nothing to dispatch, or the timer hasn't
			 * expired.
			 */
			TIME_UNLOCK();
			free_callouts(free);
			splx(pl);
			return;
		}

		/*
		 * Compute time at which callout was scheduled to have fired.
		 * This may be less than the current time, if it didn't fire
		 * on time.  Before adding lbolt, fire_time (as returned by
		 * callout_get) has the relative time at which the callout
		 * was scheduled to fire; i.e. it will be zero or negative.
		 */
		fire_time += lbolt;

		(void) callout_subtodo(co, &free);
		ASSERT(!(co->c_flags & C_DEAD));
		ASSERT(co->c_want == ((co->c_flags & C_PERIODIC) ? 1 : 0));

		co->c_flags |= C_INPROGRESS;

		TIME_UNLOCK();

		splx(co->c_pl);	/* set up proper interrupt priority */

		(*co->c_fcn)(co->c_arg);

		(void) splhi();
		TIME_LOCK();

		if (co->c_want == 0) {
			/*
			 * Nobody is waiting, deallocate this entry.
			 */
			free = subcallout(co, free);
		} else if (co->c_flags & C_PERIODIC) {
			/*
			 * Periodic timer; re-queue it at the next interval.
			 * The next fire time is computed by adding the period
			 * to the last fire time (fire_time + co->c_period).
			 * By subtracting the current time (lbolt) from this,
			 * we get the interval to the next firing.
			 */
			ASSERT(co->c_want == 1);
			co->c_flags &= ~C_INPROGRESS;
			co->c_lower = co->c_higher = NULL;
			addcallout(co, (fire_time + co->c_period) - lbolt);
		} else {
			/*
			 * Someone's waiting for us;
			 * signal them by setting the state of
			 * the callout C_FIRED and pass the
			 * responsibility of freeing the
			 * callout entry to them.
			 */
			co->c_flags = (co->c_flags & ~C_STAT) | C_FIRED;
		}
	}
}

/*
 * void addcallout(struct callout *co, long ticks);
 *
 *	Add a callout to its assigned "todo" list.
 *
 * Calling/Exit State:
 *
 *	Must be called with the time_mutex held.
 *
 * Description:
 *
 *	Service routine to add a timeout to the callout queue described by
 *	co->c_hdr.  The callout queue is a tree of relatively timed
 *	callouts.  The absolute amount of time until callout for a given
 *	node can be calculated by starting with the root time of the callout
 *	tree, and descending the tree util the node of interest is reached.
 *	Whenever a left movement is made, the time of the node is
 *	subtracted from the accumulated time.  If a right movement, the
 *	node's time is added to the accumulated time.  Continuing to proceed
 *	in this manner until the node in question is reached will result in
 *	the absolute amount of time until the node will fire.
 */
STATIC void
addcallout(struct callout *co, long ticks)
{
	long curtime;
	struct callout *cp;
	struct callout **cpp;
	struct callout *prevp;

	ASSERT(TIME_OWNED());
	ASSERT(co->c_hdr != NULL);
	ASSERT((co->c_flags & C_STAT) == C_WAITING);

	/*
	 * Just insert the callout into the tree, always going
	 * down to an unoccupied leaf node.  We make no attempt to
	 * keep the tree balanced.
	 */
	prevp = NULL;
	cpp = co->c_hdr;
	if ((cp = *cpp) != NULL) {
		/*
		 * Non-NULL tree.
		 */
		curtime = cp->c_time;
		for (;;) {
			prevp = cp;
			if (ticks < curtime) {
				/*
				 * The new timeout will expire before the
				 * timeout we're currently looking at.
				 */
				cpp = &cp->c_lower;
			} else {
				/*
				 * The new timeout will expire at the
				 * same time or after the timeout we're
				 * currently looking at.
				 */
				cpp = &cp->c_higher;
			}
			if ((cp = *cpp) == NULL)
				break;
			curtime += cp->c_time;
		}
	} else
		curtime = 0;

	co->c_time = ticks - curtime;

	/*
	 * We keep track of the address of the previous entry
	 * so untimeout can delete an entry without searching
	 * the tree.
	 */
	co->c_parentp = prevp;
	co->c_prevpp = cpp;

	*cpp = co;

	return;
}

/*
 * struct callout *subcallout(struct callout *co, struct callout *free)
 *
 *	Delete a callout from all appropriate lists.
 *
 * Calling/Exit State:
 *
 *	Caller must be holding time_mutex.  Returns pointer to accumulated
 *	chain of unused callout structures.
 *
 * Description:
 *
 *	Delete the callout pointed to by "co" from both its todo (if resident)
 *	and hash lists and deallocate the structure if it is not dead.
 *	We call "callout_free" to actually free the buffer after doing this
 *	and return the (potentially NULL) chain of buffers back to the caller
 *	for subsequent freeing after the time_mutex has been released.
 */
STATIC struct callout *
subcallout(struct callout *co, struct callout *free)
{
	ASSERT(TIME_OWNED());

	co_nactive--;

	sub_idhash(co);

	if ((co->c_flags & C_STAT) != C_WAITING) {
		/*
		 * Callout has already fired; no need to remove it
		 * from the "todo" tree.
		 */
		return callout_free(co, free);
	}

	/*
	 * Remove the entry from its todo tree if it is not
	 * pinned to the tree, and free up the structure.
	 */
	if (callout_subtodo(co, &free)) {
		/*
		 * Structure not pinned to its tree; free it up.
		 */
		free = callout_free(co, free);
	}

	return free;
}

/*
 * struct callout *find_idhash(toid_t id)
 *
 *	Find the callout associated with an id.
 *
 * Calling/Exit State:
 *
 *	Must be called holding the time_mutex.
 *
 * Description:
 *
 *	Lookup "id" and return associated callout structure.  Called from
 *	untimeout to find a particular id to untimeout.
 */
STATIC struct callout *
find_idhash(toid_t id)
{
	list_t *start;
	struct callout *cp;

	start = &coid_hash[CALLOUT_HASH(id)];
	cp = (struct callout *)start->flink;
	while (cp != (struct callout *)start) {
		if (cp->c_id == id)
			return cp;
		cp = (struct callout *)cp->c_flink;
	}

	return NULL;
}

#define	PDELAY	PRIMED

STATIC void delay_wakeup(void *);

/*
 * void delay(long ticks)
 *
 * 	Sleep for a certain number of ticks.
 *
 * Calling/Exit State:
 *
 *	Caller must not be holding locks, as we sleep.
 *
 * Description:
 *
 *	We allocate an event, set a callout against this event and
 *	sleep on the event.  The callout will fire and wake us up.
 *	Since we allocate space while sleeping, we snapshot the target
 *	wakeup-time before sleeping and set the callout for the appropriate
 *	number of ticks left after we've allocated all the necessary
 *	resources.
 */
void
delay(long ticks)
{
	event_t *eventp;
	clock_t sample;
	long diff;
	void *co;

	if (ticks <= 0)
		return;

	sample = lbolt;
	eventp = EVENT_ALLOC(KM_SLEEP);
	co = itimeout_allocate(KM_SLEEP);

	TIME_LOCK();

	/*
	 * Diff is the number of ticks which have passed while we waited
	 * for memory.
	 */
	diff = lbolt - sample;
	if (diff < 0) {
		/*
		 * Lbolt rolled over while we were allocating.
		 */
		diff = -diff;
	}

	if (ticks > diff) {
		/*
		 * Haven't delayed greater than ticks allocating the
		 * necessary memory.
		 */
		itimeout_l_a(delay_wakeup, eventp, ticks - diff,
			     PLBASE, co);
		TIME_UNLOCK();
		EVENT_WAIT(eventp, PDELAY);
	} else {
		/*
		 * Delayed greater then ticks just getting memory.  Don't
		 * have to worry about doing the timeout (i.e. we blew the
		 * time allocating memory).
		 */
		TIME_UNLOCK();
		itimeout_free(co);
	}

	EVENT_DEALLOC(eventp);
	return;
}

/*
 * void delay_wakeup(void *eventp)
 *
 * 	Activate someone doing a delay.
 *
 * Calling/Exit State:
 *
 *	Called from callout dispatch.  Simply signals the event the
 *	waiter is blocked on.
 */
STATIC void
delay_wakeup(void *eventp)
{
	EVENT_SIGNAL((event_t *)eventp, 0);
	return;
}

/*
 * void uniqtime(struct timeval *tv)
 *
 *	Generate monotonically increasing time values.
 *
 * Calling/Exit State:
 *
 *	Called to generate a unique time across all processors.  Uses
 *	the time-mutex for interlock.  If the last time passed back is
 *	still accurate (from a seconds standpoint), we return the last
 *	time with the usec portion incremented.  Otherwise, we re-initialize
 *	last and pass this value back.
 */
void
uniqtime(struct timeval *tv)
{
	static struct timeval last;

	TIME_LOCK();

	if (last.tv_sec != (long)hrestime.tv_sec) {
		last.tv_sec = (long)hrestime.tv_sec;
		last.tv_usec = (long)0;
	} else {
		last.tv_usec++;
	}

	*tv = last;

	TIME_UNLOCK();
}


/*
 * void fixtimed(void)
 *
 *	Periodically compare the hardware clock with the unix clock.
 *	Call clockadj to drift the unix clock back to the hardware
 *	clock if they get out of sync.
 *
 * Calling/Exit State:
 *
 *	None.
 */
STATIC void
fixtimed(void)
{
	timestruc_t rt;		/* hardware clock */
	timestruc_t ut;		/* unix clock */
	long fix;

	/*
	 *  If someone is already adjusting the clock,
	 *  do nothing.
	 */
	if (timedelta)
		return;

	if (xtodc(&rt)) {
		cmn_err(CE_WARN, "fixtimed: can't read hardware clock");
		return;		/* error reading hardware clock */
	}

	GET_HRESTIME(&ut);

	fix = rt.tv_sec - ut.tv_sec;

	if (fix < 2 && fix > -2)	/* threshold 2 seconds error */
		return;			/* no adjustment necessary */

	clockadj(fix * MICROSEC, B_FALSE);

	/*
	 * Adjust statistics.
	 */
	fixtime_net += fix;
	fixtime_abs += (fix > 0 ? fix : -fix);
	fixtime_cnt++;
}

STATIC void
toid_alloc(struct callout *co)
{
	list_t *start;
	struct callout *cp;
	toid_t id;

restart:
	id = toid_incr(timeid);
	start = &coid_hash[CALLOUT_HASH(id)];
	cp = (struct callout *)start->flink;
	while (cp != (struct callout *)start) {
		if (cp->c_id == id)
			goto restart;
		cp = (struct callout *)cp->c_flink;
	}
	co->c_flink = start;
	co->c_rlink = start->rlink;
	start->rlink = (list_t *)co;
	co->c_rlink->flink = (list_t *)co;
	co->c_id = id;
}
