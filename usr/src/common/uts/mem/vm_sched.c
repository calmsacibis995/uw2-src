/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/vm_sched.c	1.78"
#ident	"$Header: $"

#include <svc/cpu.h>
#include <mem/as.h>
#include <mem/mem_hier.h>
#include <mem/page.h>
#include <mem/swap.h>
#include <mem/tuneable.h>
#include <mem/ublock.h>
#include <mem/vmmeter.h>
#include <mem/vmparam.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/proc_hier.h>
#include <proc/seize.h>
#include <proc/signal.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>
#include <util/inline.h>

#ifndef NO_RDMA
#include <mem/rdma.h>
#endif

STATIC	void 		build_swapout_shortlists(clock_t, int);
STATIC	void 		swapout(proc_t *, int);
STATIC 	void 		swap_unload_proc(proc_t *procp);
STATIC	void		swapout_deferred(proc_t *);
STATIC	void 		cancel_deferred_swapout(void);
STATIC	boolean_t	swapin(proc_t *);
STATIC 	proc_t 		*get_sleeper_mutexed(int);
STATIC	boolean_t 	isprocinactive(proc_t *p);
STATIC	boolean_t 	isprocactive(proc_t *p);

void 	vmmeter(void);
STATIC	void	loadav(long *, int);

extern clock_t et_age_interval_tune;

extern	int	scale_maxpgio;	/* 
				 * whether maxpgio should be increased as 
				 * swap devices are added.
				 */

extern	u_int	deficit_age;	/* the per second decay factor for deficit */
extern	int	maxslp;		/* 
				 * how long a process can avoid the clock hand-
				 * ler, without being "deadwood"; in seconds.
				 */

int	avefree; 	/* Average memory not in use over 5 seconds (pages) */
u_int	maxpgio;	/* 
			 * High water mark for paging operations (in pages).
			 * If paging rate exceeds maxpgio, the system is
			 * considered to be IO saturated.
			 */
int	verylowfree;	/* if mem available < verylowfree, force swapping */
int	mem_avail;	/* memory available: free + dirty lists */

int	mem_avail_state;	
int	rss_trim_factor = 1;
int	et_age_interval_shift;

/*
 * avenrun counters: these need to be maintained for BSD compatibility,
 * eventhough SVR4.2 kernel code may not read these counters.
 */
/* defines that Sun has in param.h but we don't right now */
#define	FSHIFT	8		/* bit to right of fixed binary point */
#define FSCALE	(1<<FSHIFT)	/* scaling factor for the load average */
long	avenrun[3];		/* FSCALED average runq lengths plus IOwaiters*/

/*
 * Constants for averages over 1, 5, and 15 minutes
 * when sampling at 5 second intervals.
 *
 * Although the constants are chosen for the indicated exponential
 * decay, they are very close approximations to the values
 * of another (obvious) running average approximation of size N,
 *   avg[new] ==  ((N-1)/N)*avg[old] + (1/N)*value,
 * since exp(-1/N) and ((N-1)/N) are very close to (within 1% of)
 * each other for N > 7.
 */

STATIC long cexp[3] = {
	0.9200444146293232 * FSCALE,	/* exp(-1/12) */
	0.9834714538216174 * FSCALE,	/* exp(-1/60) */
	0.9944598480048967 * FSCALE,	/* exp(-1/180) */
};

STATIC	int	nprocs_slpscan;	/* number of processes scanned for deadwood */
STATIC	int	nprocs_agescan;	/* number of processes scanned for ET aging */
STATIC	int	desperate;	
STATIC	int	deservin;
STATIC	int	should_swapin;
STATIC	int	sleeper_scan_interval;	/* 
					 * time interval (in seconds) between
					 * successive searches for deadwood.
					 */
STATIC	int	sleeper_timeout;
STATIC	int 	schedpoke = 0;	/*
				 * to wake up the swapin daemon every so
				 * often, even when memory conditions are dire.
				 */

STATIC	u_int	rate_pgpgin;	/* Rate of pages paged in per second */
STATIC	u_int	rate_pgpgout;	/* Rate of pages paged out per second */
STATIC	u_int	sched_aveload;	/* Av. #runnable LWPs (scaled by FSCALE) */
STATIC	u_int	ave_rtime = (60 * HZ);	/* Average incore residence time */	
STATIC	int	deficit;	/* memory deficit (in pages) */
STATIC	int	max_swapin_wait;/* 
				 * Max # of seconds for which the swapin
				 * daemon may not run due to memory overload.
				 */
STATIC	event_t	sched_unload_event;
STATIC	event_t	sched_swapin_event;
STATIC	event_t sched_et_age_event; 	/* Elapsed Time based aging event */
STATIC	event_t sched_sleeper_event;
STATIC	proc_t *swapped_procs = NULL;	/* List of swapped out processes */
STATIC	fspin_t	swapped_list_lock;	/* Stabilizes swapped out list */

STATIC	clock_t	et_age_interval;
STATIC	clock_t	et_age_interval_normal;
STATIC  int	swapin_mem_need;	/* Minimum #pages needed for swapin */

STATIC	int	seconds_count = 0;


/*
 * When the swapper is desperate, it still prefers blocked processes over
 * active processes for swapout. A blocked process has to be inactive for
 * minimum time before it is selected for swapout. This is so that the
 * chance of catching processes during IO waits is minimized.
 */
STATIC	int min_sleep_time_desperate;	/* in clock ticks */

#define ENGINE_VPGPGIN(i)	(ENGINE_PLOCALMET_PTR(i)->MET_PGPGIN_CNT)
#define ENGINE_VPGPGOUT(i) 	(ENGINE_PLOCALMET_PTR(i)->MET_PGPGOUT_CNT)
#define ENGINE_NOWAY(i)		(engine[i].e_flags & E_NOWAY)

#ifdef UNIPROC
#define ENGINE_BOUNDLWPS(i)     (global_rq->rq_srunlwps)
#else
#define ENGINE_BOUNDLWPS(i)     (engine[i].e_rqlist[0]->rq_srunlwps)
#endif

/* moving window mean */
#define ave(x,newval,n)	(x) = ((((n) - 1) * (x) + (newval)) / (n))

#define	IN_SCORE(incoretime)	MIN(1000, (lbolt - (incoretime)))
#define	SLP_SCORE(whenactive)	MIN(1000, (lbolt - (whenactive)))

/*
 * The search of the process directory  in order to discover deadwood
 * is divided into a number (SLEEPER_SCANS) of partial searches.
 */ 
#define SLEEPER_SCANS 5		

/*
 * Once a process remains swapped out for greater that SWOUT_THRESH clock
 * ticks, we will increase the likelihood of swapping it in, by
 * progressively discounting the effect of its size. This is motivated by
 * both performance and deadlock avoidance considerations. 
 */
#define	SWOUT_THRESH	(20 * HZ)

/*
 * After a process has been started or swapped in, we will not swap it out
 * for at least MIN_RESIDENCY_TIME (in clock ticks). This is necessary to
 * permit the system to make forward progress under heavy memory load.
 */
#define	MIN_RESIDENCY_TIME	HZ 

/*
 * After a process has been swapped out for MAXOUTAGE seconds, we would 
 * consider it worthy of special consideration for being swapped back in, by 
 * not applying the deficit consideration. 
 */ 
#define	MAXOUTAGE	60

/*
 * boolean_t
 * ISPROCAZOMBIE(proc_t *procp):
 *	TRUE if the process has become a zombie. A zombie process is one 
 * 	with p_nlwp == 0 and a non zero p_wcode. For swapper's purposes 
 *	(not interested in zombies), p_nlwp == 0 suffices.
 *
 * 	Eventually move the following #define to proc/proc.h. In addition,
 *	remove the p->p_as == NULL condition, since it is here temporarily
 *	to deal with such processes as nfs-daemons that do have a null AS,
 *	but do not otherwise indicate that they cannot be swapped out.
 */
#define ISPROCAZOMBIE(procp) ((procp)->p_nlwp == 0 || (procp)->p_as == NULL)

#define DOSWAP	0	/* Swap the process and recover its nonlocked pages */
#define DOSHRED	1	/* Recover nonlocked pages without swapping */


/*
 * Macros to determine whether a single LWP or all LWPs in a process are
 * inactive and if sleeping, are in interruptible sleeps.
 * PERF: If l_stat were a bit field, we could collapse three of the checks
 *	 that involve l_stat into a single check.
 */
#define ISLWPINACTIVE(lp)	((lp)->l_stat != SRUN && \
	(lp)->l_stat != SONPROC && (lp)->l_stat != SIDL && \
	((lp)->l_flag & L_NWAKE) == 0)
#define ISPROCINACTIVE(p) \
	((p)->p_nlwp > 1 ? isprocinactive((p)) : ISLWPINACTIVE((p)->p_lwpp))

/*
 * Macros to determine whether a single LWP or any LWPs in a process are
 * active.
 * PERF: If l_stat were a bit field, we could collapse three of the checks
 *	 that involve l_stat into a single check.
 */
#define ISLWPACTIVE(lp)	((lp)->l_stat == SRUN || \
	(lp)->l_stat == SONPROC || (lp)->l_stat == SIDL)
#define ISPROCACTIVE(p) \
	((p)->p_nlwp > 1 ? isprocactive((p)) : ISLWPACTIVE((p)->p_lwpp))

/*
 *			General Description
 *
 * 1.0 	The following terminology is used:
 * ---------------------------------------
 *
 * deadwood: 	a process that has not registered any activity
 * 	  	for maxslp seconds
 *
 * activity: 	a process registers activity whenever any of its LWPs
 * 	  	are dispatched or enter clock handler. The registration of
 * 		activity is by means of a timestamp in the process structure.
 * forced
 * swapout:	memory conditions call for desperate measures. this occurs
 * 		whenever instantaneous available memory is very low, or
 * 		short term average of available memory is low, or the
 * 		system has a high paging rate accompanied by a less than
 * 		desirable average memory availability. Or, a process is found
 *		that deserves to be brought in, but memory availability
 *		is so low that some other process _must_ be unloaded to
 *		make room.
 * big
 * processes:	processes that have a large number of non-locked
 * 		physical pages in their memory holding. the pages may
 * 		or may not be shared among multiple processes.
 *
 * system load:	average number of runnable LWPs over past 60 seconds
 *
 * runnable
 * process:	a process at least one of whose LWPs is runnable.
 *
 * process scheduling
 * priority: 	Maximum among the scheduling priorities of a process's LWPs.
 *
 * deservin:	a variable that records the fact that a swapped out process
 * 		deserves to come in.
 *
 * inactive
 * process:	one that has not registered any activity for a while, and
 * 		does have each of its LWPs stopped or in interruptible sleeps.
 *
 * shredding:	A term used to describe the unloading of a process's
 *		nonlocked page holding _without_ the actual unloading and
 *		deactivation of the process; the effect is identical to
 *		a swapout immediately followed by a swapin of the process.
 *		Shredding is employed in order to deal with situations in
 *		which it is inappropriate to swap a process out, but the
 *		process holds some nonlocked pages that need to be released
 *		to the system.
 *
 * 
 * 2.0	A sketch of the swapper code:
 * ---------------------------------
 *
 *	The swapping functionality is implemented in three functions each
 *	executed by a separate sched LWP. These functions are:
 *
 * 	1.	sched_force_unload: When activated, will identify a process
 *		to swap out.
 *	2.	sched_swapin: When activated, will identify the most suitable
 *		swapped out process to be swapped in,
 *	3.	sched_sleeper_search: Expected to be activated periodically.
 *		When activated, will search for processes that have been
 *		inactive for some time; these processes would then be aged
 *		or swapped out.
 *	
 *	The sched LWPs are activated by vmmeter. The policy is:
 *
 *	1.	Activate sched_force_unload when it is determined that
 *		immediate or longer term memory availability is poor,
 *		or the paging rate is too high;
 *
 *	2.	Activate sched_swapin, if swapped out processes exist, under
 *		three conditions:
 *		- Memory is not critically low,
 *		- Memory is critically low but sched_swapin has not been 
 *		  activated for some time,
 *		- Memory is not plentiful but the system has been idling (low
 *		  load average and paging rate). 
 *
 *	3.	Activate sched_sleeper_search periodically. Currently, we do
 *		not activate it if sched_force_unload is already activated,
 *		but that may change.
 *		
 * 2.1	Sched specific synchronization:
 *
 *	The sched_sleeper_search() and sched_force_unload() LWPs avoid
 *	invoking swapout() on the same process by each checking that the
 *	P_SEIZE flag is not set under p_mutex cover. The first one to
 *	get the p_mutex and call vm_seize will win this race; the other
 *	will simply drop the process from consideration for swapout().
 *	
 *	The testing of P_SEIZE flag prior to swapout also prevents races
 *	between swapin() and swapout(). The flag is cleared under p_mutex
 *	protection after all the critical swapin() actions are completed.
 *
 *	Note that requiring that P_SEIZE be cleared before a process is
 *	swapped out will make it nearly impossible for either of the 
 *	swapout() agents to queue for seizing the process while it is 
 *	being aged -- which is a plus, for, why bother aging and swapping
 *	a process in quick succession? Again, note that vm_seize does handle
 *	the race between swapping agents and process LWPs correctly, in any
 *	case.
 *
 * 2.2	Synchronization between process status and swap activity:
 *		
 *	This is provided by vm_seize. Note that we keep the process seized
 *	for the entire duration from swapout() to the end of swapin().
 *	This averts a race between swapin() and CL_WAKE/CL_SETRUN functions,
 *	both of which would attempt to enqueue the runnable LWPs on the
 *	dispatcher queues. The race is averted because the CL_WAKE and 
 *	CL_SETRUN functions don't setrun the LWPs of seized processes.
 *
 * 2.3	Protection of the list of swapped out processes:
 *
 *	The list is protected by a fast spin lock. In addition to the
 *	sched LWPs, one other agent contends for the lock -- this 
 *	is the get_swapque_cnt() function. It needs to traverse the list
 *	so that it can count the number of swapped out LWPs.
 *
 */

/*
 * In order to select a process for swapout, sched_force_unload() builds
 * two lists of large processes : One list identifies a small number of 
 * active processes based on the length of time spent in memory, while
 * the  other list identifies a small number of (potentially) inactive
 * processes based on the length of time for which no activity has happened.
 *
 * Each list should be small, but not too small, so that we both
 * contain the overhead of search and ensure a high probability of success in
 * picking a good candidate. Two or more candidates should be picked, so
 * that the same process does not get swapped out all the time.
 *
 * The swapper will record the procdir slot numbers of processes it has
 * identified as candidates for swapout, so that it does not have to acquire
 * and hold locks on the processes themselves. It is possible, though very
 * unlikely, that a process identified by the swapper through the slot number
 * is replaced by a new process before the swapper locks the process (for
 * verifying eligibility) just prior to swapout. The race is benign.
 *
 * The swapper could also remember the process identifier for stronger
 * verification, but this will involve higher costs of serializing access
 * to the pid structure (between swapper and pid-freeing code) -- and so is
 * not done. The most harm this can cause, infrequently, is that a less
 * desirable candidate is picked.
 */

#define NBIG 4
#define NSLP 4

STATIC	int	nsleepers;
STATIC	int	nbigprocs;
STATIC	struct	unload_proc {
	int	up_slotid;	/* to record procdir slot id */
	int	up_swappri;	/* swapout priority */
	struct	unload_proc *up_next;
} bigp[NBIG], zzzp[NSLP];	/* bigp[] for big procs; zzzp[] for sleepers */

STATIC	struct	unload_proc bplist = {-1, {INT_MAX}, &bplist};
STATIC	struct	unload_proc zzlist = {-1, {INT_MAX}, &zzlist};
STATIC	pid_t	swap_defer_pid = NOPID;
STATIC	clock_t	swap_defer_timeout = 0;

/*
 * Miscellaneous debugging code.
 */
#ifdef DEBUG
STATIC	void	check_list_order(struct unload_proc *);
STATIC	void	check_list_count(struct unload_proc *, int);
STATIC	void	check_swapped_procs(void);
int	swapped_count = 0;
int	met_missed_swaplock = 0;	/* # times get_swapque_cnt() gave up */
#define CHECK_LIST_ORDER(listp)	check_list_order(listp)
#define	CHECK_LIST_COUNT(listp, nup)	check_list_count(listp, nup)
#define INCR_SWAPPEDCOUNT	++swapped_count
#define DECR_SWAPPEDCOUNT	--swapped_count
#define CHECK_SWAPPED_PROCS()	check_swapped_procs()
#else
#define CHECK_LIST_ORDER(x)
#define	CHECK_LIST_COUNT(listp, nup)	
#define INCR_SWAPPEDCOUNT
#define DECR_SWAPPEDCOUNT
#define CHECK_SWAPPED_PROCS()
#endif

/*
 * The following weights determine how swap-in selection is biased:
 *
 * io_weight:	
 *	Negatively weighs the cost function of the size of the process 
 *	when it was swapped out. The cost function itself is non-linear
 *	or piecewise linear, in order to reduce bias against very 
 *	large processes.
 *	
 * cpu_weight:	
 *	Positively weighs the highest among all the LWP scheduling priorities.
 *
 * swap_weight:	
 *	Positively weighs the length of time for which the process has been
 *	swapped out.
 *
 * sleep_weight: 	
 *	Positively weighs the length of time for which (all the LWPs of)
 *	the process remained blocked in interruptible sleeps while the
 *	process was swapped out. This serves two purposes:
 *	(1) favors a process that became runnable while swapped out, 
 *	(2) a process that remained inactive for a long time is likely
 *	to become inactive again, and is therefore a better swapin choice.
 */
extern	int	io_weight;
extern	int	cpu_weight;
extern	int	swap_weight;
extern	int	sleep_weight;

/*
 * STATIC boolean_t
 * isprocinactive(proc_t *)
 *	Returns B_TRUE if all LWPs in a process are blocked/stopped, and if
 * 	blocked, are in interruptible sleeps. Returns B_FALSE otherwise.
 *
 * Calling/Exit State:
 *	The process pointer and the proc structure need to be stable. Hence
 * 	the routine (and the macro, ISPROCINACTIVE()) is expected to be called
 *	with either the process mutex held or with the process vm_seized.
 *
 * Remarks:
 *	The reliability of the determination that a process has all its LWPs
 *	in inactive states is questionable unless the process is seized, since
 *	LWP state cannot be reliably inferred without holding the LWP mutex.
 */
STATIC	boolean_t
isprocinactive(proc_t *p)
{
	lwp_t	*lwpp = p->p_lwpp;

	do {
		if (ISLWPINACTIVE(lwpp))
			continue;
		return B_FALSE;
	} while ((lwpp = lwpp->l_next) != NULL);
	return B_TRUE;
}

/*
 * STATIC boolean_t
 * isprocactive(proc_t *)
 *	Returns B_TRUE if any LWPs in a process are active.
 * 	Returns B_FALSE otherwise.
 *
 * Calling/Exit State:
 *	The process pointer and the proc structure need to be stable. Hence
 * 	the routine (and the macro, ISPROCACTIVE()) is expected to be called
 *	with either the process mutex held or with the process vm_seized.
 *
 * Remarks:
 *	The reliability of the determination that a process has all its LWPs
 *	in inactive states is questionable unless the process is seized, since
 *	LWP state cannot be reliably inferred without holding the LWP mutex.
 */
STATIC	boolean_t
isprocactive(proc_t *p)
{
	lwp_t	*lwpp = p->p_lwpp;

	do {
		if (ISLWPACTIVE(lwpp))
			return B_TRUE;
	} while ((lwpp = lwpp->l_next) != NULL);
	return B_FALSE;
}



#ifdef DEBUG
int	_num_swapouts_;
int	_sum_freemem_;
int	_num_swapins_;
int	_num_deadwd_;
int	_num_deserv_;
int	_cons_trace_ = 0;
int	_cons_trace2_ = 0;
int	_stop_swapper_ = 0;
int	_num_seconds_ = 0;
int	_swproc_pid_;
#endif

/*
 * STATIC void
 * loadav(long *avg, int n)
 *
 * 	Compute a tenex style load average of a quantity on
 * 	1, 5 and 15 minute intervals.
 * 	NB: avg is kept as a scaled (by FSCALE) long as well as is cexp.
 *
 * Calling/Exit State:
 *	Called from vmmeter; cannot block. No locking needed within.
 */
STATIC void
loadav(long *avg, int n)
{
	int i;
	for (i = 0; i < 3; i++) {
		avg[i] = (cexp[i] * avg[i] + n * FSCALE * (FSCALE - cexp[i])) 
				>> FSHIFT;
	}
}

/*
 * void
 * vmmeter(void)
 *
 * 	Called once per second to :
 *
 *	[1]	Collect averages of various counters that gear 
 *		swapper activity. These are:
 *
 *	rate_pgpgin	- page-in rate, updated once per second,
 *	rate_pgpgout 	- page-out rate, updated once per second,
 *	sched_aveload	- the 5 second average of system load
 *	avefree		- 5 second free memory average,
 *
 * 	Since none of the above statistics need to be accurate, no 
 * 	locking is necessary for computing them.
 *
 *	[2]	Decay the memory deficit.
 *
 *	[3]	Signal the sched LWPs according to actions required.
 *
 * Calling/Exit State:
 *	Only the engine table mutex will be acquired, for computing the load.
 *	No other locks required, acquired or released. 
 */

void
vmmeter(void)
{
	int i;
	int nboundlwps = 0;	/* number of runnable, bound-to-cpu lwps */
	pl_t ospl;
	size_t min_mem;
	int tmp;
	int runnables = 0;
	extern size_t maxrss_tune;

	u_int new_pgin_sum = 0;
	u_int new_pgout_sum = 0;
	static u_int old_pgin_sum = 0;
	static u_int old_pgout_sum = 0;

	u_int delta;
	int def;		/* local copy of deficit, to avoid races */

	/*
	 * XXX - need to reexamine this. need to lock here unless
	 * selfinit code changed to not mark engine online until
	 * runq allocated and pointer set (rqlist[0]) and offlining
	 * does NOT free local rq (making rqlist[0] pointer bogus).
	 */

	ospl = LOCK(&eng_tbl_mutex, PLHI);

	for (i = 0; i < Nengine; i++) {

		if (ENGINE_NOWAY(i))
			continue;
		
		new_pgin_sum += ENGINE_VPGPGIN(i);
		new_pgout_sum += ENGINE_VPGPGOUT(i);

		/*
		 * Gag! could catch a partially on-lined CPU
		 */
#ifndef UNIPROC
		if (engine[i].e_rqlist[0])
#endif
			nboundlwps += ENGINE_BOUNDLWPS(i);
	}
	UNLOCK(&eng_tbl_mutex, ospl);

	/*
	 * The integer differences below will still give the correct unsigned
	 * values after wrap-around, since all values involved are unsigned.
	 */
	delta = new_pgin_sum - old_pgin_sum;
	old_pgin_sum = new_pgin_sum;
	ave(rate_pgpgin, delta, 5);

	delta = new_pgout_sum - old_pgout_sum;
	old_pgout_sum = new_pgout_sum;
	ave(rate_pgpgout, delta, 5);

	/*
	 * Age the deficit. We used the page_in rate in decaying deficit,
	 * if the page_in rate exceeds the normal decay fraction.
	 * While it is certainly true that not all page_ins reduce the
	 * deficit (at cost of freemem), it is also the case that some
	 * page_ins may be for shared pages and these would reduce the
	 * deficit more sharply. All in all, it should be okay to decline
	 * the deficit value by the number of page_ins.
	 */
	def = deficit;
	def -= MAX((def / deficit_age), rate_pgpgin);
	if (def < 0)
		def = 0;
	deficit = def;
	mem_avail = min_mem_notinuse();
	ave(avefree, mem_avail, 5);

	/*
	 * Set mem_avail_state to indicate the level of memory
	 * contention on the system.
	 */ 
	tmp = min(mem_avail, avefree) - (deficit + swapin_mem_need);

	rss_trim_factor = 1;
	et_age_interval_shift = 0;

	if (tmp > (8 * lotsfree)) {
		mem_avail_state = MEM_AVAIL_EXTRA_PLENTY;
		if (tmp > maxrss_tune) {
			rss_trim_factor = 0;
		}
	} else if (tmp > (3 * lotsfree)) {
		mem_avail_state = MEM_AVAIL_PLENTY;
	} else if (tmp > lotsfree) {
		mem_avail_state = MEM_AVAIL_NORMAL;	
	} else if (tmp > desfree) {
		mem_avail_state = MEM_AVAIL_FAIR;
	} else if (tmp > minfree) {
		mem_avail_state = MEM_AVAIL_POOR;
	} else {
		mem_avail_state = MEM_AVAIL_DESPERATE;
	}

	EVENT_SIGNAL(&sched_et_age_event, 0);
	runnables = global_rq->rq_srunlwps + nboundlwps;
	sched_aveload = ave(sched_aveload, (runnables << FSHIFT), 5);	
	
	/*
	 * Compute the rss trim factor.
	 */
	if (mem_avail_state > MEM_AVAIL_PLENTY) {

		et_age_interval_shift = 1;

		/* We add in "1" in order to avoid division by 0, below */
		min_mem = 1 + MIN(mem_avail, avefree);

		rss_trim_factor = (1 + MIN(((2 * lotsfree) / min_mem), 3));

		if (maxrss_tune < (2 * rss_trim_factor * MAXTRIM)) {
			rss_trim_factor = (maxrss_tune / 2 * MAXTRIM);
		}
	}

	/*
	 * wakeup swapper if there is work to do, or if it hasn't run in
	 * a while (~5 seconds).
	 */

#ifdef DEBUG
	if (_stop_swapper_ != 0)
		return;
	if (++_num_seconds_ > 29) {
		if (_cons_trace_ != 0) {
		/*
		 *+ Report some swapping stats on console.
		 */
		cmn_err(CE_CONT,"##Af %d, SwO %d, SwI %d, DdW %d, Dsrv %d\n",
			(_sum_freemem_ / 30), _num_swapouts_, _num_swapins_,
			_num_deadwd_, _num_deserv_);
		}
		_sum_freemem_ = 0;
		_num_swapouts_ = 0;
		_num_swapins_ = 0;
		_num_deadwd_ = 0;
		_num_deserv_ = 0;
		_num_seconds_ = 0;
	} else {
		_sum_freemem_ += mem_avail;
	}
	FSPIN_LOCK(&swapped_list_lock);
	CHECK_SWAPPED_PROCS();
	FSPIN_UNLOCK(&swapped_list_lock);
#endif

	desperate = 0;
	deservin = 0;
	should_swapin = 0;

	if ((mem_avail_state >= MEM_AVAIL_POOR) || 
	        ((mem_avail_state >= MEM_AVAIL_FAIR) && 
		(rate_pgpgin + rate_pgpgout > (maxpgio / 2)) )) {
		desperate = 1;
		if (++schedpoke > max_swapin_wait) {
			should_swapin = 1;
			EVENT_SIGNAL(&sched_swapin_event, 0);
			schedpoke = 0;
		} else if (ave_rtime >= (4 * MIN_RESIDENCY_TIME)) {
			EVENT_SIGNAL(&sched_unload_event, 0);	
		} else {
			/*
			 * in the rare case where the system is stuck at
			 * an extremely low ave_rtime, we provide for a
			 * slow increment to ave_rtime. 
			 */
			ave_rtime += (1 + (MIN_RESIDENCY_TIME / 10));
		} 
		goto done;
	} else if ((--sleeper_timeout) == 0) {
		sleeper_timeout = sleeper_scan_interval;
		if (mem_avail_state > MEM_AVAIL_PLENTY) {
			EVENT_SIGNAL(&sched_sleeper_event, 0);
		} 
	} else if (mem_avail_state >= MEM_AVAIL_NORMAL) {
		EVENT_SIGNAL(&sched_unload_event, 0);
	} 
	if ((swapped_procs != NULL) || (swap_defer_pid != NOPID)) {
		/*
		 * Wake up the swap-in daemon if either there is too little
		 * work going on (desite a high memory load), or memory load has
		 * begun to ease, or if it has not been woken up for some time.
		 */ 
		if (((sched_aveload < (4 * FSCALE)) &&
		    	(rate_pgpgin + rate_pgpgout < (maxpgio/4))) ||
		    (mem_avail_state <= MEM_AVAIL_NORMAL) || 
		    ((++schedpoke) > max_swapin_wait)) {
			should_swapin = 1;
			EVENT_SIGNAL(&sched_swapin_event, 0);
			schedpoke = 0;
		} 
	}  
done:
	if (++seconds_count == 5) {
		seconds_count = 0;
		loadav(avenrun, (runnables + MET_IO_OUTSTANDING));
	}
	
	return;
}

/*
 * void
 * sched_init(void)
 *	One time initializations of various sched() parameters.
 *
 * Calling/Exit State:
 *	Called only once, from kvm_init(). Initializes various
 *	parameters of interest to the swapper. No locking required or
 *	provided.
 *
 * Remarks:
 *	We EXPECT that the tune structure is initialized by the time
 *	sched_init gets called (from vm_sysinit).
 */

void
sched_init(void)
{
	/*
	 * set up values for the various parameters used by the swapper,
	 * based on tunables and configuration information.
	 *
	 * 	compute maxpgio = (diskrpm*2/3)*(tunable( == 1 by default)).
	 * 	if (tunable is nonzero), then for each added swap device
	 * 	after the first one, add (tunable* diskrpm*2/3) to maxpgio.
	 *
	 * 	compute nprocs_slpscan on the basis of max number of procs and
	 *	the desired inter-scan period.
	 */

#define DISKRPM		60

	verylowfree = tune.t_gpgslo;

	/*
	 * setting maxpgio: depends on whether scale_maxpgio is nonzero.
	 * If it is, then maxpgio will be set to the correct value at the
	 * first swapadd, and increase proportionally with other swapadds.
	 * If however scale_maxpgio is 0, then we need to set maxpgio to
	 * the correct value here, in order that non-swap IO throughput
	 * is correctly accounted by it.
	 */
	maxpgio = 0;
	if (scale_maxpgio == 0)
		maxpgio = (DISKRPM * 2) / 3;
	/*
	 * XXX: Remove the next line when swapadd does the right thing
	 * (bumps maxpgio by scale_maxpgio*(DISKRPM*2)/3).
	 */
	maxpgio = (DISKRPM * 2) / 3;

	if (deficit_age < 1)
		deficit_age = 1;

	max_swapin_wait = 5;
	if ((sleeper_scan_interval = (maxslp / MAX(1, SLEEPER_SCANS))) == 0)
		sleeper_scan_interval = 1;
	sleeper_timeout = sleeper_scan_interval;
	nprocs_slpscan = (2 * v.v_proc) / MAX(1, SLEEPER_SCANS);
	nprocs_agescan = ((2 * v.v_proc * HZ) / ((et_age_interval_tune * 3) / 2));
	min_sleep_time_desperate = (2 * HZ); /* 2 seconds */

	swapin_mem_need = 0;

	EVENT_INIT(&sched_unload_event);
	EVENT_INIT(&sched_swapin_event);
	EVENT_INIT(&sched_et_age_event);
	EVENT_INIT(&sched_sleeper_event);
	FSPIN_INIT(&swapped_list_lock);
}


/*
 * void
 * sched_elapsed_time_age(void *sched_et_age_argp)
 *	Identify any process that has not been aged in the last et_age_interval
 *	ticks and either age it or post it an aging event so that it will age 
 *	itself on return to user mode.
 * Calling/Exit State:
 *	Since this is a kernel LWP that is signalled from vmmeter(), no locking
 *	state is associated with it. When searching for processes to age, the
 *	LWP will hold the proc_list mutex in reader mode. In addition, the
 *	process p_mutex is held across the duration of aging or the posting of
 *	aging events, for affected processes.
 */
/* ARGSUSED */
void
sched_elapsed_time_age(void *sched_et_age_argp)
{
	proc_t 	*procp;
	lwp_t	*lwpp;
	int	wheretobegin = 0;	/* the procdir slot at which to begin */
	int	nprocs_scanned = 0;
	int	i;
	struct as *proc_as;
	wkset_t *wksetp;	
	int	runnable;
	clock_t et_age_interval_fast;

	u.u_lwpp->l_name = "wallclock_ager";

	et_age_interval_fast = ((et_age_interval_tune * 2) / 3);

	for(;;) {
		ASSERT(KS_HOLD0LOCKS());

		EVENT_CLEAR(&sched_et_age_event);
		EVENT_WAIT(&sched_et_age_event, PRIMEM);

		ASSERT(getpl() == PLBASE);

		/*
		 * Don't do any aging at all if gobs of extra memory are
		 * available.
		 */
		if (mem_avail_state == MEM_AVAIL_EXTRA_PLENTY)
			continue;

		if (mem_avail_state == MEM_AVAIL_PLENTY) {
			et_age_interval = et_age_interval_tune;
		} else {
			et_age_interval = et_age_interval_fast;
		}

		(void)RW_RDLOCK(&proc_list_mutex, PL_PROCLIST);
		
		nprocs_scanned = 0;

		for (i = wheretobegin; ;i++) {

			if (i >= v.v_proc) {
				wheretobegin = 0;
				break;
			} 

			if (((procp = PSLOT2PROC(i)) == NULL) ||
			     NOT_LOADED(procp) ||
			     DONT_ET_AGE(procp)) {
				continue;
			} 

			if (++nprocs_scanned > nprocs_agescan) {
				wheretobegin = i;
				break;
			}

			LOCK(&procp->p_mutex, PLHI);
			if ((proc_as = procp->p_as) == NULL) {
				UNLOCK(&procp->p_mutex, PL_PROCLIST);
				continue;
			}

			/*
			 * Note that the parent of vfork effectively
			 * has no rss.
			 */
			procp->p_nonlockedrss = ((procp->p_flag &
						  P_AS_ONLOAN) ? 0 :
							proc_as->a_rss -
							proc_as->a_lockedrss);

			/*
			 * Nonlockedrss can never be truly less than 0.
			 * However, the difference between a_rss and
			 * a_lockedrss is being computed without the
			 * protection of hat locks, and so it could
			 * yield unreasonable values in some cases.
			 * The occurrence is benign however.
			 */
			if (procp->p_nonlockedrss == 0) {
				UNLOCK(&procp->p_mutex, PL_PROCLIST);
				continue;
			}
			wksetp = &(procp->p_as->a_wkset);
			if (wksetp->whenaged < (lbolt - 
					max(et_age_interval,
					    wksetp->et_age_interval >> et_age_interval_shift))) {

				if (DONT_ET_AGE(procp)) {
					UNLOCK(&procp->p_mutex, PL_PROCLIST);	
					continue;
				}

				/* Any LWPs SRUN or SONPROC? */

				lwpp = procp->p_lwpp;
				runnable = 0;
				do {
					if ((lwpp->l_stat == SRUN) ||
					    (lwpp->l_stat == SONPROC) ||
					    ((lwpp->l_stat == SSLEEP) &&
					     ((lwpp->l_flag & L_NWAKE) == 
					     L_NWAKE))) {
						++runnable;
						break;
					}

				} while ((lwpp = lwpp->l_next) != NULL);
				
				if (runnable) {
					trapevproc(procp,EVF_L_ASAGE, B_TRUE);
					UNLOCK(&procp->p_mutex, PL_PROCLIST);
				} else {
					RW_UNLOCK(&proc_list_mutex, PLHI);
					as_age_externally_l(procp);
					ASSERT(KS_HOLD0LOCKS());
					ASSERT(getpl() == PLBASE);
					(void)RW_RDLOCK(&proc_list_mutex, 
						PL_PROCLIST);
				}
			} else {
				UNLOCK(&procp->p_mutex, PL_PROCLIST);
			}
		}
		RW_UNLOCK(&proc_list_mutex, PLBASE);
	}
}


/*
 * void
 * sched_sleeper_search(void *argp)
 *	Periodically search for a long term sleeper among nprocs_slpscan procdir
 *	entries. If one is found, swap/age it as appropriate.
 *
 * Calling/Exit State:
 *	None. The proc_list_mutex will be acquired and held in read mode
 *	during each activation. 
 */
/* ARGSUSED */
void
sched_sleeper_search(void *argp)
{
	proc_t *swapproc;

	u.u_lwpp->l_name = "sleeper_search";

	for(;;) {

		ASSERT(KS_HOLD0LOCKS());

		EVENT_CLEAR(&sched_sleeper_event);
		EVENT_WAIT(&sched_sleeper_event, PRIMEM);

		ASSERT(getpl() == PLBASE);

		/*
		 * PERF:
		 * Should we skip this round of selection, if the
		 * proc_list_mutex is under high contention?
		 */
		(void)RW_RDLOCK(&proc_list_mutex, PL_PROCLIST);

		if ((swapproc = get_sleeper_mutexed(nprocs_slpscan)) == NULL) {
			RW_UNLOCK(&proc_list_mutex, PLBASE);
		} else {

			ASSERT(LOCK_OWNED(&(swapproc->p_mutex)));
			RW_UNLOCK(&proc_list_mutex, PLHI);

			swapproc->p_slptime = (lbolt - swapproc->p_active) / HZ;
#ifdef DEBUG
			++_num_deadwd_;
#endif
			swapout(swapproc, DOSWAP); 
			/* swapout released p_mutex */
		}
	}
}


/*
 * STATIC proc_t *
 * get_sleeper_mutexed(int ntoscan)
 *	Returns a process locked that hasn't entered the clock handler
 * 	over the last maxslp seconds (i.e.,  is probably an inactive process).
 * 	Starts the search from the position that was last examined, and searches
 * 	upto ntoscan entries.
 *
 * Calling/Exit State:
 *	Called with proc_list_mutex held in read mode, and returns with
 *	proc_list_mutex held in read mode. Does not drop the mutex.
 *	Returns a pointer to a process which has not registered any activity
 *	for maxslp seconds. The process p_mutex is returned held. If no such 
 * 	process can be found over a scan of ntoscan entries, then NULL is 
 *	returned.
 */
STATIC proc_t *
get_sleeper_mutexed(int ntoscan)
{
	clock_t	epoch = lbolt - maxslp*HZ;
	proc_t	*procp;
	static int	sleeper_search = 1; 	/* next place to start */
						/* searching from */
	pl_t	pl;
	int	i,j;

	ASSERT((sleeper_search >= 0) && (sleeper_search < v.v_proc));
	ASSERT(KS_HOLD1LOCK());	/* proc_list_mutex */

	if (sleeper_search + ntoscan > v.v_proc) {
		/*
		 * The search needs to wrap around the procdir table.
		 * First search upto the end of the table.
		 */
		i = (v.v_proc - sleeper_search);
		if ((procp = get_sleeper_mutexed(i)) == NULL) {
			ASSERT(KS_HOLD1LOCK());
			ASSERT(sleeper_search == 0);
			ASSERT(ntoscan > i);
			ntoscan -= i;
			ASSERT(ntoscan <= v.v_proc);
		} else {
			ASSERT(LOCK_OWNED(&(procp->p_mutex)));
			return(procp);
		}
	}

	for (i = 0; i < ntoscan; i++) {
		j = sleeper_search + i;
		ASSERT((j >= 0) && (j < v.v_proc));
		if (((procp = PSLOT2PROC(j)) == NULL) ||
		    (procp->p_active > epoch) ||
		    NOT_LOADED(procp) ||
		    DONTSWAP(procp) || 
		    ISPROCAZOMBIE(procp))
			continue;
		ASSERT(j == (int)(procp->p_slot));

		pl = LOCK(&(procp->p_mutex), PLHI);
		/*
		 * Verify under lock cover that the process is really
		 * inactive and that it is swappable. If verification
		 * succeeds, return the process locked.
		 */
		if (NOT_LOADED(procp) || DONTSWAP(procp) || 
		    	ISPROCAZOMBIE(procp) || !ISPROCINACTIVE(procp)) {
			UNLOCK(&(procp->p_mutex), pl);
			/* continue; */
		} else {
			sleeper_search = (j + 1) % v.v_proc;
			return(procp);
		}
	}
	ASSERT(j == (sleeper_search + ntoscan - 1));
	sleeper_search = (j + 1) % v.v_proc;
	ASSERT(KS_HOLD1LOCK());
	return(NULL);
}


/*
 * void
 * sched_force_unload(void)
 *	Find and unload some process (daemon LWP). 
 *
 * Calling/Exit State:
 *	No locks held on entry; never returns. The proc_list_mutex will be
 *	acquired and released by this routine as needed; also, process mutexes
 *	will be acquired and released as appropriate.
 *
 * Description:
 * 	The context that executes this function is a swapper LWP, and is
 *	activated whenever the system is found to be running low on free
 *	pages and the situation cannot be corrected by the normal aging
 *	or deadwood elimination code.
 *	The routine calls build_swapout_shortlists(), which identifies upto
 *	NSLP processes as potentially inactive processes or, upto NBIG
 *	processes that are among the largest in terms of nonlocked page
 *	holding. The candidate processes are identified by means of their
 *	indexes in the process directory.
 *	The routine will prefer swapping out inactive processes over active
 *	(however large) processes.
 *	If no candidates can be found in one pass, the routine will perform a
 *	second pass during which more liberal conditions are applied for
 *	selecting the swapout candidates, so that it is more likely to find
 *	one or more swapout candidates in the second pass if it found none in
 *	the first pass.
 */
void
sched_force_unload(void)
{
	clock_t	epoch;
	proc_t *swapproc;	
	int i, tries;
	int swproc_slotid;
	struct unload_proc *prev_bp, *current_bp;

	u.u_lwpp->l_name = "swapout";
	
swaploop:

	ASSERT(KS_HOLD0LOCKS());
	EVENT_CLEAR(&sched_unload_event);
	EVENT_WAIT(&sched_unload_event, PRIMEM);

	epoch = lbolt - min_sleep_time_desperate;
	tries = 0;

	if (swap_defer_pid != NOPID) {
		pid_t procpid;
		ASSERT(getpl() == PLBASE);
		FSPIN_LOCK(&swapped_list_lock);
		procpid = swap_defer_pid;
		swap_defer_pid = NOPID; 
		FSPIN_UNLOCK(&swapped_list_lock);
		if (procpid == NOPID) 
			goto try_again;
		swapproc = prfind(procpid);
		if (swapproc == NULL)
			goto try_again;
		if (ISPROCAZOMBIE(swapproc)) {
			UNLOCK(&swapproc->p_mutex, PLBASE);
			goto try_again;
		}
		ASSERT(!NOT_LOADED(swapproc));	
		if (!CANTSHRED(swapproc)) {
			swapout_deferred(swapproc);
			ASSERT(getpl() == PLBASE);
			/*
			 * And, eventhough we just completed a deferred
			 * swapout, we continue on to try and find some
			 * other process to swapout. This is because the
			 * deferred swapout, in most cases, will only 
			 * yield a few u-block pages and will not relieve
			 * memory stress by much.
			 */
		} else {
			trapevunproc(swapproc, EVF_PL_SWAPWAIT, B_TRUE);
			if (SV_BLKD(&(swapproc->p_swdefer_sv))) {
				SV_BROADCAST(&swapproc->p_swdefer_sv, 0);
			} 
			UNLOCK(&swapproc->p_mutex, PLBASE);
		}
	}

try_again:
	ASSERT(KS_HOLD0LOCKS());

	/*
	 * We build two lists: one list of upto NSLP longest term
	 * sleepers, another list of upto NBIG largest, non-blocked
	 * processes. As an optimization, we don't continue to
	 * build the "big processes" list if at least one sleeper
	 * is found, with the implication that *as long as there
	 * is space to be freed up from swapping inactive processes*,
	 * other processes should not be swapped.
	 * Note that the non-locked RSS of a process does not include its
	 * U pages.
	 */

	/*
	 * Initialize the list of "sleepers". It's a circular list,
	 * beginning and ending at zzlist. We keep this list in ascending
	 * order of length of time that processes have been inactive; i.e.,
	 * in descending order of the time when processes became inactive.
	 */
	for (i=0; i < NSLP; i++) {
		zzzp[i].up_swappri = INT_MIN;
		zzzp[i].up_slotid = -1;
		zzzp[i].up_next = (&(zzzp[i]) +  1);
	}
	zzzp[NSLP - 1].up_next = &zzlist;
	zzlist.up_next = &zzzp[0];

	/*
	 * Initialize the list of "big" processes.
	 * We keep this list in ascending order of size.
	 */
	for (i=0; i < NBIG; i++) {
		bigp[i].up_swappri = INT_MIN;
		bigp[i].up_slotid = -1;
		bigp[i].up_next = (&(bigp[i]) + 1);
	}
	bigp[NBIG - 1].up_next = &bplist;
	bplist.up_next = &bigp[0];

#ifdef DEBUG
	if (_cons_trace_) {
		/*
		 *+ debugging information to console
		 */
		cmn_err(CE_CONT, "sched_force_unload: starting search");
		if (tries > 0) {
			cmn_err(CE_CONT, "\tsecond try\n");
		} else {
			cmn_err(CE_CONT, "\tfirst  try\n");
		}
	}
#endif

	build_swapout_shortlists(epoch, tries);

	CHECK_LIST_COUNT(&zzlist, nsleepers);
	CHECK_LIST_COUNT(&bplist, nbigprocs);

	/*
	 * At this point, we have completed a scan of the procdir array and
	 * built one or both of the big process list and the sleeping process
 	 * list. Next we pick a swapout candidate from these lists if either
	 * is nonempty. Note that if at least one sleeper was detected, then
	 * we may not have built the big process list, so we should only 
	 * search the big process list if no sleeper was found.
	 */

	swapproc = NULL;
	ASSERT(KS_HOLD0LOCKS());

	(void)RW_RDLOCK(&proc_list_mutex, PL_PROCLIST);

	while (nsleepers > 0) {
		/*
		 * Select the process with the highest swapout priority,
		 * check whether it can be swapped out and swap it out
		 * if possible. Since we created the list in the ascending
		 * order of swapout priority, just find the last element
		 * on the list.
		 */
		prev_bp = &zzlist;
		current_bp = prev_bp->up_next;
		ASSERT(current_bp != &zzlist);
		while (current_bp->up_next != &zzlist) {
			prev_bp = current_bp;
			current_bp = current_bp->up_next;
			ASSERT(prev_bp->up_swappri <= current_bp->up_swappri);
			ASSERT(current_bp->up_swappri < INT_MAX);
		}
		/*
		 * current_bp == the last element before the zzlist marker.
		 *
		 * drop the process that we are about to examine from the 
		 * unload_procs list. 
		 */
		--nsleepers;
		prev_bp->up_next = &zzlist;

		ASSERT(current_bp->up_swappri > INT_MIN);
		swproc_slotid = current_bp->up_slotid;
		ASSERT(swproc_slotid >= 0);
		ASSERT(swproc_slotid < v.v_proc);

		swapproc = PSLOT2PROC(swproc_slotid);

		if (swapproc != NULL &&
		    (swapproc->p_incoretime + MIN_RESIDENCY_TIME) < lbolt) {
			ASSERT(swapproc->p_slot == swproc_slotid);
			/*
			 * repeat swap eligibility check under p_mutex cover
			 * and swap the process out if check succeeds.
			 */
			(void)LOCK(&(swapproc->p_mutex), PLHI);
			if (NOT_LOADED(swapproc) || DONTSWAP(swapproc) ||
			    		ISPROCAZOMBIE(swapproc)) {
				/*
				 * Cannot/Do not swap this process out.
				 * We're still holding the proc_list_mutex,
				 * so can not drop to PLBASE.
				 */
				UNLOCK(&(swapproc->p_mutex), PLHI);
			} else {
				RW_UNLOCK(&proc_list_mutex, PLHI);
#ifdef DEBUG
				if (_cons_trace_) {
					/*
					 *+ debugging information to console
			 		 */
					cmn_err(CE_CONT, 
			  		"sched: picked sleeper procp %x to unload\n", swapproc);
				}
#endif
				/* 
				 * since we have gone through some effort
				 * to find this process, we will swap it
				 * out without checking whether its LWPs are
				 * really blocked interruptably. it is very
				 * unlikely to be otherwise, anyway.
				 */
				swapproc->p_slptime = ((lbolt - 
					swapproc->p_active) / HZ);
#ifdef DEBUG
					++_num_deadwd_;
#endif
				swapout(swapproc, DOSWAP); /* drops p_mutex */
				goto swaploop;
			} /* else: if (NOT_LOADED(swapproc) || ...etc */
		} /* if (swaproc != NULL && ... etc) */
		/*
		 * we are here because the selected process failed to be a 
		 * desirable swapout candidate.
		 * so we must move on to the next shortlisted one.
		 *
		 * we're still holding the proc_list_mutex.
		 */
		ASSERT(RW_OWNED(&proc_list_mutex));
		ASSERT(KS_HOLD1LOCK());

	} /* while there are sleeper candidates */

	ASSERT(RW_OWNED(&proc_list_mutex));
	ASSERT(KS_HOLD1LOCK());

	/* 
	 * we failed to find any processes on the list of inactive ones,
	 * that we could swapout. so we turn to the other list. the proc_
	 * list_mutex is still with us.
	 */
	while (nbigprocs > 0) {
		/*
		 * Select the process with the highest swapout priority,
		 * check whether it can be swapped out and swap it out
		 * if possible. Since we created the list in the ascending
		 * order of swapout priority, just find the last element
		 * on the list.
		 */
		prev_bp = &bplist;
		current_bp = prev_bp->up_next;
		ASSERT(current_bp != &bplist);
		while (current_bp->up_next != &bplist) {
			prev_bp = current_bp;
			current_bp = current_bp->up_next;
			ASSERT(prev_bp->up_swappri <= current_bp->up_swappri);
			ASSERT(current_bp->up_swappri < INT_MAX);
		}
		/*
		 * current_bp == the last element before the bplist marker.
		 * we take the selected process off the list.
		 */
		--nbigprocs;
		prev_bp->up_next = &bplist;
		
		ASSERT(current_bp->up_swappri > INT_MIN);
		swproc_slotid = current_bp->up_slotid;
		ASSERT(swproc_slotid >= 0);
		ASSERT(swproc_slotid < v.v_proc);

		swapproc = PSLOT2PROC(swproc_slotid);

		if (swapproc != NULL &&
		    (swapproc->p_incoretime + MIN_RESIDENCY_TIME) < lbolt) {
			ASSERT(swapproc->p_slot == swproc_slotid);
			/*
			 * repeat swap eligibility check under p_mutex cover
			 * and swap the process out if check succeeds.
			 */
			(void)LOCK(&(swapproc->p_mutex), PLHI);
			if (NOT_LOADED(swapproc) || CANTSHRED(swapproc) || 
			    ISPROCAZOMBIE(swapproc)) {
				UNLOCK(&(swapproc->p_mutex), PLHI);	
			} else {
				if (DONTSWAP(swapproc)) {
					RW_UNLOCK(&proc_list_mutex, PLHI);
#ifdef DEBUG
					if (_cons_trace_) {
						/*
						 *+ debugging console print
						 */
						cmn_err(CE_CONT, "sched: picked"
						 "big procp %x to shred\n",
			  				swapproc);
					}
#endif
					swapout(swapproc, DOSHRED);
					goto swaploop;	
				} else if (swapproc->p_nonlockedrss > 0 || 
				           tries > 0) {
					RW_UNLOCK(&proc_list_mutex, PLHI);
#ifdef DEBUG
					if (_cons_trace_) {
						/*
						 *+ debugging console print
						 */
						cmn_err(CE_CONT, "sched: picked"
						 "big procp %x to unload\n",
			  				swapproc);
					}
#endif
					swapproc->p_slptime = 0;
					swapout(swapproc, DOSWAP);
					goto swaploop;	
				} else {
					UNLOCK(&(swapproc->p_mutex), PLHI);
					/* proc_list_mutex still held */
				}
			}
			/* 
			 * the selected process was not swapped out. proceed
			 * to examine the next prospect. proc_list_mutex is
			 * still held.
			 */
		}
		ASSERT(RW_OWNED(&proc_list_mutex));
		ASSERT(KS_HOLD1LOCK());

	} /* while nbigprocs > 0 */

	RW_UNLOCK(&proc_list_mutex, PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	/*
	 * No process was swapped out or shredded. Go for a second pass,
	 * if conditions warant.
	 *
	 * In our second pass, we will also be willing to swap
	 * out a process even if it is under seize when we find it. We
	 * wait one clock tick for things to settle, before doing more work.
	 * We set epoch to 0 so that this time around, we just catch any
	 * large process that exists, instead of focussing on inactive
	 * processes (there should be none of them anyway, or we would
	 * have got one in oure first pass).
	 *
	 * The second try should be (made) extremely improbable.
	 *
	 * PERF:
	 * It may be desirable to replace the deservin and should_swapin
	 * considerations below with a swapin_prio threshold that
	 * varies with system load. 
	 */

	if ((++tries < 2) && (desperate || deservin || should_swapin)) {
		LBOLT_WAIT(PRIMEM);
		epoch = 0;
		goto try_again;
	}

	goto swaploop;

	/* NOTREACHED */
}


/*
 * STATIC void
 * build_swapout_shortlists(clock_t epoch, int hardup)
 *
 *	Build two lists of swapout candidates. The first is the list
 *	of potentially inactive (deadwood) processes, which have registered
 *	no activity since "epoch". The second is a list of processes
 *	that are among the largest in terms of their nonlocked page holding.
 *	hardup signifies how desperate the swapper is; if it is nonzero,
 *	then it will pick processes that can be shredded even if they cannot 
 *	be swapped out.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit. The function itself will acquire
 *	the proc_list mutex in read mode while scanning the list of processes.
 */
STATIC	void
build_swapout_shortlists(clock_t epoch, int hardup)
{
	int i;
	int swappri;
	proc_t *p;
	struct unload_proc *nbp, *bp;
	pl_t opl;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	(void)RW_RDLOCK(&proc_list_mutex, PL_PROCLIST);

	nsleepers = 0;
	nbigprocs = 0;

	/*
	 * Search the procdir for processes that are either inactive since
	 * epoch, or for the largest processes (in terms of non-locked RSS).
	 */

	for (i = 0; i < v.v_proc; i++) {

		if (((p = PSLOT2PROC(i)) == NULL) || NOT_LOADED(p) ||
			    ISPROCAZOMBIE(p))
			continue;

		ASSERT(p->p_slot == i);
		ASSERT(p->p_active > 0);

		if (p->p_active < epoch) { 

			/*
			 * found a process that was _probably_ inactive. 
			 * insert it on the list of sleepers if  it has been
			 * inactive longer than the first process on the list.
			 * we already checked that it is not a zombie process.
			 * ideally, all LWPs in the process ought to be blocked
			 * signallably; however, because of the force_unload, 
			 * we  are under some pressure to discover a swapout 
			 * candidate, and so will not limit the search to 
			 * signallably blocked candidates. 
			 */

			swappri = p->p_nonlockedrss + ublock_npages(p);
			swappri = ((swappri << 7) + SLP_SCORE(p->p_active));
	
			if (swappri > zzlist.up_next->up_swappri) {

				if (DONTSWAP(p))
					continue;
	
				ASSERT(swappri > INT_MIN);
				if (++nsleepers > NSLP) {
					nsleepers = NSLP;
				};

				/*
				 * unlink the first entry and use it for the new
				 * sleeper just found.
				 */
				nbp = zzlist.up_next;
				zzlist.up_next = nbp->up_next;
				nbp->up_slotid = i;
				nbp->up_swappri = swappri;
				/*
				 * search for a place in the list to enter 
				 * the nbp. guaranteed to find one, since 
				 * the "last" entry on the list is the one 
				 * with the maximum up_swappri.
				 */
				for (bp = &zzlist; bp->up_next != &zzlist;
					bp = bp->up_next) {
					if (bp->up_next->up_swappri > 
						nbp->up_swappri)
						break;
				}
				nbp->up_next = bp->up_next;
				bp->up_next = nbp;
				CHECK_LIST_ORDER(&zzlist);
				CHECK_LIST_COUNT(&zzlist, nsleepers);

			} 
			continue;	
		} 
		
		/*
		 * no processes, including the current one have been
		 * identified as swapout candidates based on the criterion
		 * of having been inactive for a long time. see whether the
		 * current process is large enough and has been resident
		 * long enough to be added to the list of "big" processes.
		 */
		swappri = INT_MIN;

		if (!DONTSWAP(p)) {

			swappri = (IN_SCORE(p->p_incoretime) + 
				   ((p->p_nonlockedrss + 
				     ublock_npages(p)) << 7));

		} else if ((hardup > 0) && !CANTSHRED(p)) {

			/*
			 * since hardup > 0, we are desperate and
			 * need to be able to count on shredding 
			 * whatever we can, from this process! 
			 * Since p_nonlockedrss can be stale, we'll 
			 * need to update it from the AS structure. 
			 * Since this update can still race with process
			 * execution, we can be looking at an AS when 
			 * a_rss is smaller than a_lockedrss; but this 
			 * should be very infrequent and in any case, 
			 * benign.
			 */
			opl = LOCK(&p->p_mutex, PLHI);
			p->p_nonlockedrss = (((p->p_flag & P_AS_ONLOAN) ||
					      p->p_as == NULL) ? 0 :
						p->p_as->a_rss -
						p->p_as->a_lockedrss);
			UNLOCK(&p->p_mutex, opl);
			swappri = IN_SCORE(p->p_incoretime);
			if (p->p_nonlockedrss == 0) {
				continue; 
			} else {
				/* ignore the upages contribution */
				swappri += (p->p_nonlockedrss << 7);
			}
		} else {
			/* 
			 * skip this process, since it is not a good candidate
			 * and we are not yet hardup. 
			 */
			continue;
		}

		if (swappri > bplist.up_next->up_swappri) {
			ASSERT(swappri > INT_MIN);
			if (++nbigprocs > NBIG) {
				nbigprocs = NBIG;
			};
			/*
			 * remove the first process on the list,
			 * and insert the current process in the
			 * sorted order.
			 */
			nbp = bplist.up_next;
			bplist.up_next = nbp->up_next;
			nbp->up_slotid = i;
			nbp->up_swappri = swappri;
			/*
			 * Look for a place to insert nbp.
			 * We're guaranteed to find one, since the
			 * "last" entry on the list (i.e., the bplist
			 * structure) has the maximum possible size.
			 *
			 * In the loop below, bp points to the
			 * element next to which the nbp should be
			 * inserted.
			 */
			for (bp = &bplist; bp->up_next != &bplist;
				bp = bp->up_next) {
				if (bp->up_next->up_swappri >
					nbp->up_swappri)
					break;
			}
			nbp->up_next = bp->up_next;
			bp->up_next = nbp;
			CHECK_LIST_ORDER(&bplist);
			CHECK_LIST_COUNT(&bplist, nbigprocs);

		}
		/*
	         * PERF: We're finished with looking at a procdir entry. Before
		 * continuing, consider releasing the procdir mutex if there
		 * is a blocked writer.
		 */

	} /* for loop scanning the procdir entries */

	RW_UNLOCK(&proc_list_mutex, PLBASE);

#ifdef DEBUG
	if (_cons_trace2_) {
		/*
		 *+ debugging information to console
		 */
		cmn_err(CE_CONT,"sched: built swapout candidate lists\n");
		call_demon();
	}
#endif

}

/*
 * STATIC void
 * swapout(proc_t *procp, int swaporshred)
 *	Strip away a process's unlocked memory holding. If swaporshred
 * 	is DOSWAP, then also swap the process out by swapping out the LWPs.
 * 	Else (i.e., if swaporshred is DOSHRED) then keep the process 
 *	LWPs loaded, and pretend that an aging step just occurred on 
 *	the process.
 *
 * Calling/Exit State:
 *	Only the process p_mutex is held on entry. It is dropped before
 * 	returning.
 */
STATIC void
swapout(proc_t *procp, int swaporshred)
{
	struct as *proc_as;
	struct lwp *lwpp;
	clock_t tmp;

	ASSERT(procp != NULL);
	ASSERT(PSLOT2PROC(procp->p_slot) == procp);
	ASSERT(!ISPROCAZOMBIE(procp));
	ASSERT(!NOT_LOADED(procp));
	ASSERT(KS_HOLD1LOCK());
	ASSERT((procp->p_flag & P_SEIZE) == 0);

	if (procp->p_pidp->pid_id == swap_defer_pid) {
		UNLOCK(&procp->p_mutex, PLBASE);
		return;
	}

#ifdef DEBUG
	_swproc_pid_ = procp->p_pidp->pid_id;
	if (_cons_trace_ != 0) {
		/*
		 *+ console trace.
		 */
		if (swaporshred == DOSWAP)
		cmn_err(CE_CONT,">>>>>>>>>> \t\t\t OUT %d\n", _swproc_pid_);
		else
		cmn_err(CE_CONT,">>>>><<<<< \t\t\t SHRED %d\n", _swproc_pid_);
	}
	++_num_swapouts_;
#endif

	/*
	 * PERF:
	 *
	 * Let the process LWPs know that they can back off from local
	 * aging if they have reached it.
	 */
	trapevunproc(procp, EVF_L_ASAGE, B_TRUE);

	if (!vm_seize(procp)) {
		/*
		 *+ This is an unexpected occurrence, as the semantics of
		 *+ vm_seize() are that it will always return successfully
		 *+ if caller is not a part of the process being seized.
		 *+ Ain't no recovery action for that.
		 */
		cmn_err(CE_PANIC,
		   "swapout: illegal return from vm_seize: process ptr %x\n",
		   (u_int)procp);
	}

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Because we lost the cover of p_mutex during vm_seize, we may
	 * have had the address space switched from beneath us. Sooo...
	 * we obtain the as pointer after the process is seized.
	 */

	proc_as = procp->p_as;

	if (proc_as == NULL || ISPROCAZOMBIE(procp)) {
		vm_unseize(procp);
		return;
	}

	tmp = MIN((lbolt - (procp->p_incoretime)), (20 * MIN_RESIDENCY_TIME));
	ave(ave_rtime, tmp, 5);

	/*
	 * We cannot call as_ageswap() on the parent of a vfork(), since
	 * the child now owns the address space.
	 */
	if (procp->p_flag & P_AS_ONLOAN) {
		procp->p_swrss = 0;
	} else {
		procp->p_swrss = proc_as->a_rss - proc_as->a_lockedrss;
		as_ageswap(proc_as, AS_SWAP);
	}

	if (swaporshred != DOSWAP)
		goto shredded;

	(void) LOCK(&procp->p_mutex, PLHI);
	ASSERT(!(procp->p_flag & P_SYS));
	if (procp->p_flag & P_NOSWAP) {
		UNLOCK(&procp->p_mutex, PLBASE);
		goto shredded;
	}
	/*
	 * Check whether any LWP in the process has a nonzero keepcnt, or
	 * is possibly in kernel mode and in an uninterruptable sleep.
	 */
	for (lwpp = procp->p_lwpp; lwpp != NULL; lwpp = lwpp->l_next) {
		/*
		 * (lwpp->l_start != 0) => HIGH likelihood that the lwp
		 *	is in a trap/syscall.
		 * 	
		 * The following unprotected peeks at LWP state are
		 * benign; at worst, we may get the false indication
		 * about the lwp being SRUN. In that case, whereas
		 * we would normally have deferred the swapout, we
		 * would carry it out. This should be extremely rare
		 * anyway, and unlikely to degrade performance 
		 * for that reason.
		 */
		if (lwpp->l_keepcnt == 0) {
			if (lwpp->l_start == 0) {
				continue;
			} else if (lwpp->l_stat != SRUN) {
				if ((lwpp->l_flag & L_NWAKE) == 0)
					continue;
			} /* else : defer this process swapout */
		}  /* else: defer this process swapout */

		if (swap_defer_pid == NOPID) {
			trapevproc(procp, EVF_PL_SWAPWAIT, B_TRUE);
			FSPIN_LOCK(&swapped_list_lock);
			if (swap_defer_pid == NOPID) {
				swap_defer_pid = procp->p_pidp->pid_id;
				swap_defer_timeout = (lbolt + (20 * HZ));
				FSPIN_UNLOCK(&swapped_list_lock);
			} else {
				FSPIN_UNLOCK(&swapped_list_lock);
				trapevunproc(procp, EVF_PL_SWAPWAIT, B_TRUE);
				if (SV_BLKD(&(procp->p_swdefer_sv))) {
					SV_BROADCAST(&procp->p_swdefer_sv,0);
				} 
			}
		} 
		UNLOCK(&procp->p_mutex, PLBASE);
		goto shredded;
	} /* for */

	/*
	 * we reach here if the process can be unloaded without deferment.
	 */

	ASSERT(procp->p_flag & P_LOAD);
	procp->p_flag &= ~P_LOAD;
	UNLOCK(&procp->p_mutex, PLBASE);
	swap_unload_proc(procp);
	return;

shredded:
	/*
	 * We just "shredded" the process by removing anything that
	 * was not locked down. We update the aging information to
	 * reflect this. We don't need p_mutex since process is seized.
	 */
	procp->p_nonlockedrss = ((procp->p_flag & P_AS_ONLOAN) ? 0 :
					proc_as->a_rss - proc_as->a_lockedrss);
	procp->p_incoretime = lbolt;
	RESET_AGING_INFO(proc_as, 
			((ushort_t)((proc_as->a_min_agequantum + 
					proc_as->a_max_agequantum) / 2)));
	vm_unseize(procp);
	ASSERT(KS_HOLD0LOCKS());
	return;
}

/*
 * STATIC void
 * cancel_deferred_swapout(void)
 *	Cancel the deferred swapout of a process, if there is such a process.
 *
 * Calling/Exit State:
 *	Must be called at PLBASE, and is expected to return at PLBASE.
 *	The function does not do anything to cause it to block.
 */
STATIC	void
cancel_deferred_swapout(void)
{
	proc_t *procp; 
	pid_t procpid;

	FSPIN_LOCK(&swapped_list_lock);
	procpid = swap_defer_pid;
	swap_defer_pid = NOPID;
	swap_defer_timeout = 0;
	FSPIN_UNLOCK(&swapped_list_lock);
	if (procpid == NOPID) {
		return;
	} 
	procp = prfind(procpid);
	if (procp == NULL) { 
		return;
	}
	if (ISPROCAZOMBIE(procp)) {
		UNLOCK(&(procp->p_mutex), PLBASE);
		return;
	}
	ASSERT(!NOT_LOADED(procp));	
	trapevunproc(procp, EVF_PL_SWAPWAIT, B_TRUE);
	if (SV_BLKD(&(procp->p_swdefer_sv))) {
		SV_BROADCAST(&procp->p_swdefer_sv, 0);
	} 
	UNLOCK(&(procp->p_mutex), PLBASE);
}

/*
 * void
 * lwp_swapout_wait(void)
 *	Check self into deferred swapout rendezvous. Executed by an
 *	LWP if an EVF_PL_SWAPWAIT trap event is found posted when returning
 *	to user mode.
 *
 * Calling/Exit State:
 */
void
lwp_swapout_wait(void)
{
	lwp_t *lwpp = u.u_lwpp;
	proc_t *procp = u.u_procp;

	ASSERT(getpl() == PLBASE);
	LOCK(&procp->p_mutex, PLHI);

	if (lwpp->l_trapevf & EVF_PL_SWAPWAIT) {

		SV_WAIT(&(procp->p_swdefer_sv), PRIMEM, &(procp->p_mutex));

	} else {
		UNLOCK(&procp->p_mutex, PLBASE);
	}
}

/*
 * STATIC void
 * swap_unload_proc(proc_t *procp)
 *	Unload the specified process.
 *
 * Calling/Exit State:
 *	The specified process is held in a vm_seized state by caller.
 */
STATIC void
swap_unload_proc(proc_t *procp)
{
	lwp_t *lwpp;

	ublock_unlock(procp, UB_SWAPPABLE);
	procp->p_swouttime = lbolt; /* record time when swapped out */
	procp->p_notblocked = B_FALSE;

	/*
	 * A process will remain swapped out while p_notblocked
	 * is clear.  Since we have just cleared P_LOAD and
	 * p_notblocked,  any future LWP wakeups will now 
	 * call load_lwp and set p_notblocked and poke sched.
	 * 
	 * There are two problems with clearing p_notblocked here.
	 * The first is that this process may have been runnable,
	 * but we are so desparate that we picked it anyway.
	 * The second problem is that an LWP wakeup could
	 * have occurred sometime before P_LOAD was cleared, 
	 * so again the LWP is now marked runnable. We 
	 * catch this by using ISPROCACTIVE to correctly
	 * set p_notblocked if any LWPs are now active.
	 *
	 * This segment of code is synchonizing with the various
	 * setrun/wakeup function without using locks. Note that:
	 *
	 *	=> The various setrun/wakeup routines do not acquire
	 *	   the p_mutex in order to inspect the P_LOAD bit.
	 *
	 *	=> No lock at all gaurds the p_notblocked field.
	 *
	 *	=> The ISPROCACTIVE() macro inspects l_stat without
	 *	   acquiring l_mutex.
	 *
	 * The WRITE_SYNC() operation is needed to guarantee that:
	 *
	 *	=> a racing setrun/wakeup function will observe
	 *	   the P_LOAD bit to be clear if this function
	 *	   observes l_stat to be INACTIVE (below).
	 *
	 *	=> the clearing of p_notblocked (above) will not be
	 *	   reordered so as to wipe out any future attempt
	 *	   to set it in racing setrun/wakeup function.
	 *
	 * The READ_SYNC() is needed so that:
	 *
	 *	=> the ISPROCACTIVE() macro (below) will observe
	 *	   l_stat to be ACTIVE if a racing setrun/wakeup
	 *	   observes P_LOAD to be set.
	 */
	WRITE_SYNC();
	READ_SYNC();
	if (ISPROCACTIVE(procp))
		procp->p_notblocked = B_TRUE;

	lwpp = procp->p_lwpp;
	procp->p_maxlwppri = (u_short)(lwpp->l_pri);
	while ((lwpp = lwpp->l_next) != NULL) {
		if (procp->p_maxlwppri < (u_short)(lwpp->l_pri))
			procp->p_maxlwppri = (u_short)(lwpp->l_pri);
	}

	if (SV_BLKD(&procp->p_swdefer_sv))
		SV_BROADCAST(&procp->p_swdefer_sv, 0);
	/*
	 * Insert the process on the list of those swapped out
	 */
	FSPIN_LOCK(&swapped_list_lock);
	procp->p_swnext = swapped_procs;
	swapped_procs = procp;
	INCR_SWAPPEDCOUNT;
	CHECK_SWAPPED_PROCS();
	FSPIN_UNLOCK(&swapped_list_lock);
	MET_SWPOUT(1);
	ASSERT(KS_HOLD0LOCKS());
	return;
}

/*
 * STATIC void
 * swapout_deferred(proc_t *procp)
 *	Perform the swapout of a process, which was previously submitted
 *	for swapout but was deferred at that time. This function repeats
 *	much of the swapout processing (seizing the process, and shredding
 *	its address space) before unloading the process. 
 *
 * Calling/Exit State:
 *	Process p_mutex must be held. The function will drop the p_mutex
 *	before returning. It can block. The return from this function will
 *	be at PLBASE.
 */

STATIC void
swapout_deferred(proc_t *procp)
{
	struct as *proc_as;
	struct lwp *lwpp;

	ASSERT(procp != NULL);
	ASSERT(PSLOT2PROC(procp->p_slot) == procp);
	ASSERT(!ISPROCAZOMBIE(procp));
	ASSERT(!NOT_LOADED(procp));
	ASSERT(KS_HOLD1LOCK());
	ASSERT((procp->p_flag & P_SEIZE) == 0);


#ifdef DEBUG
	_swproc_pid_ = procp->p_pidp->pid_id;
	if (_cons_trace_ != 0) {
		/*
		 *+ console trace.
		 */
		cmn_err(CE_CONT,">>>>>>>>>> \t\t\t OUT %d\n", _swproc_pid_);
	}
	++_num_swapouts_;
#endif

	/*
	 * PERF:
	 *
	 * Let the process LWPs know that they can back off from local
	 * aging if they have reached it.
	 */
	trapevunproc(procp, (EVF_L_ASAGE | EVF_PL_SWAPWAIT), B_TRUE);

	if (!vm_seize(procp)) {
		/*
		 *+ This is an unexpected occurrence, as the semantics of
		 *+ vm_seize() are that it will always return successfully
		 *+ if caller is not a part of the process being seized.
		 *+ Ain't no recovery action for that.
		 */
		cmn_err(CE_PANIC,
		   "swapout: illegal return from vm_seize: process ptr %x\n",
		   (u_int)procp);
	}

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Because we lost the cover of p_mutex during vm_seize, we may
	 * have had the address space switched from beneath us. Sooo...
	 * we obtain the as pointer after the process is seized.
	 */

	proc_as = procp->p_as;
	if (proc_as == NULL || ISPROCAZOMBIE(procp)) {
		/*
		 * XXX
		 *	If there are lwps, and they are blocked in
		 *	lwp_swapout_wait, release them from there?
		 */
		vm_unseize(procp);
		return;
	}

	/*
	 * We cannot call as_ageswap() on the parent of a vfork(), since
	 * the child now owns the address space.
	 */
	if (procp->p_flag & P_AS_ONLOAN) {
		procp->p_swrss = 0;
	} else {
		procp->p_swrss = proc_as->a_rss - proc_as->a_lockedrss;
		as_ageswap(proc_as, AS_SWAP);
	}
	ASSERT(procp->p_flag & P_LOAD);
	(void) LOCK(&procp->p_mutex, PLHI);
	ASSERT(!(procp->p_flag & P_SYS));
	if (procp->p_flag & P_NOSWAP) {
		goto shredded;
	}
	if (!(desperate)) {
		for (lwpp = procp->p_lwpp; lwpp != NULL; lwpp = lwpp->l_next) {
			/*
			 * Check whether any LWP in the process has a 
			 * nonzero keepcnt.
			 */
			if (lwpp->l_keepcnt == 0) {
				continue;
			}
			/* don't swap this process out */
			goto shredded;
		} /* for */
	}
	procp->p_flag &= ~P_LOAD;
	UNLOCK(&procp->p_mutex, PLBASE);
	swap_unload_proc(procp);
	return;

shredded:
	UNLOCK(&procp->p_mutex, PLBASE);
	if (SV_BLKD(&(procp->p_swdefer_sv))) {
		SV_BROADCAST(&procp->p_swdefer_sv, 0);
	} 

	/*
	 * We just "shredded" the process by removing anything that
	 * was not locked down. We update the aging information to
	 * reflect this. We don't need p_mutex since process is seized.
	 */
	procp->p_nonlockedrss = ((procp->p_flag & P_AS_ONLOAN) ? 0 :
					proc_as->a_rss - proc_as->a_lockedrss);
	procp->p_incoretime = lbolt;
	RESET_AGING_INFO(proc_as, 
			((ushort_t)((proc_as->a_min_agequantum +
					proc_as->a_max_agequantum) / 2)));
	vm_unseize(procp);
	ASSERT(KS_HOLD0LOCKS());
	return;
}

/*
 * STATIC boolean_t
 * swapin(proc_t *procp)
 *	Swap the process in. Insert all its runnable LWPs on the run queue.
 * 	Remove the process from the list of swapped out processes.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit. 
 */
STATIC boolean_t
swapin(proc_t *procp)
{
	proc_t *tmp_procp;
	struct as *proc_as;
	int err;

	ASSERT(procp != NULL);
	ASSERT(PSLOT2PROC(procp->p_slot) == procp);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT((procp->p_flag & P_SEIZE) != 0);

#ifdef DEBUG
	_swproc_pid_ = procp->p_pidp->pid_id;
	if (_cons_trace_ != 0) {
		/*
		 *+ console trace.
		 */
		cmn_err(CE_CONT,"<<<<<<<<<< \t\t\t IN %d\n", _swproc_pid_);
	}
	++_num_swapins_;
#endif

	proc_as = procp->p_as;
	ASSERT(procp->p_lwpp != NULL);
	ASSERT(proc_as != NULL);
	ASSERT(NOT_LOADED(procp));

	if ((err = ublock_lock(procp, UB_SWAPPABLE)) != 0) {
		if (err == ENOMEM)
			return B_FALSE;
		/*
		 *+ Swap I/O error while trying to swap in a process
		 *+ ublock.
		 */
		cmn_err(CE_PANIC, "swapin: swap I/O error");
	}
	(void) LOCK(&procp->p_mutex, PLHI);
	procp->p_flag |= P_LOAD;
	UNLOCK(&procp->p_mutex, PLBASE);

	/*
	 * Remove process from the swapped_procs list.
	 */
	FSPIN_LOCK(&swapped_list_lock);
	ASSERT(swapped_procs != NULL);
	if ((tmp_procp = swapped_procs) == procp) {
		/*
		 * It's the first process on the list.
		 */
		swapped_procs = procp->p_swnext;
	} else {
		ASSERT((tmp_procp != NULL) && (tmp_procp->p_swnext != NULL));
		/* Search for the process that precedes this one */
		while (tmp_procp->p_swnext != procp) {
			tmp_procp = tmp_procp->p_swnext;
			ASSERT(tmp_procp != NULL);
		}
		/*
		 * And link it to the one after, bypassing procp.
		 */
		tmp_procp->p_swnext = procp->p_swnext;
	}
	procp->p_swnext = NULL;	/* for sanity */
	DECR_SWAPPEDCOUNT;
	CHECK_SWAPPED_PROCS();
	FSPIN_UNLOCK(&swapped_list_lock);
	procp->p_incoretime = procp->p_active = lbolt;
	RESET_AGING_INFO(proc_as, ((ushort_t)((proc_as->a_min_agequantum + 
			proc_as->a_max_agequantum) / 2)));

	/*
	 * Update the count of pages swapped in.
	 */
	MET_PSWPIN(ublock_npages(procp));
	procp->p_nonlockedrss = ((procp->p_flag & P_AS_ONLOAN) ? 0 :
					proc_as->a_rss - proc_as->a_lockedrss);
	ASSERT(procp->p_nonlockedrss >= 0);
	vm_unseize(procp);
	ASSERT(KS_HOLD0LOCKS());
	MET_SWPIN(1);
	return B_TRUE;
}

/*
 * int
 * load_lwp(struct lwp *lwpp)
 *	An LWP of a swapped out process has become runnable. Nudge the
 * 	swapper if the process has been out for a while. The criteria for
 * 	nudging may be revised to include the maximum CPU priority of the
 * 	process's LWPs. Record the fact that the process has at least 
 *	one runnable LWP.
 *
 * Calling/Exit State:
 *	Caller holds lwp state lock held. Function returns with lock
 *	held, never dropped.
 */
int
load_lwp(struct lwp *lwpp)
{
	proc_t *procp;

	/* PERF:
	 * For the present, we assume that an incore LWP will remain
	 * unrunnable until the entire process is swapped in. In the
	 * future, we may decide to make incore LWPs runnable even if
	 * the process to which they belong has not been fully swapped
	 * in. For that, we will need a way to detect whether an LWP U area
	 * is in core or not.
	 */

	ASSERT(lwpp != NULL);
	procp = lwpp->l_procp;
	ASSERT(procp != NULL);

	/*
	 * This segment of code is synchonizing with the swapout()
	 * function without using locks. Note that:
	 *
	 *	=> The various setrun/wakeup routines do not acquire
	 *	   the p_mutex in order to inspect the P_LOAD bit.
	 *
	 *	=> No lock at all gaurds the p_notblocked field.
	 *
	 *	=> The ISPROCACTIVE() macro inspects l_stat without
	 *	   acquiring l_mutex.
	 *
	 * The WRITE_SYNC() operation is needed to guarantee that:
	 * 
	 *	=> a racing swapout() function will observe an active LWP
	 *	   status in l_stat (set by our caller) if this function
	 *	   observes p_notblocked to be clear [in the NOTRUNNABLE()
	 *	   macro below].
	 *
	 * The READ_SYNC() operation is needed so that:
	 *
	 *	=> The NOTRUNNABLE() macro below will observe p_notblocked
	 *	   to be clear if a racing swapout() does not observe an
	 *	   active status in l_stat.
	 *
	 */
	WRITE_SYNC();
	READ_SYNC();
	if (NOTRUNNABLE(procp)) {
		/* The first transition to process being runnable */
		procp->p_slptime += (lbolt - procp->p_swouttime) / HZ;
		procp->p_notblocked = B_TRUE;
	}

	/*
	 * If the process has been out for a while, signal
	 * the swapper at the next run of vmmeter(). Else
	 * just bump schedpoke.
	 */
	if (lbolt - procp->p_swouttime > (maxslp * HZ)) {
		/*
		 * We cannot event signal sched_swapin(), since the calling 
		 * context holds an lwp mutex and event signal will run 
		 * into * a lock hierarchy violation if it attempts to get
		 * a lock on swapper's lwp mutex. We could event signal the 
		 * swapper through a timeout, but for the moment we just 
		 * rely on vmmeter to do it for us. 
		 */
		schedpoke += max_swapin_wait;
	} else
		++schedpoke;
	return 0;
}

/*
 * void
 * sched_swapin(void *sched_argp)
 *	Swapper LWP that is responsible for swapping in processes.
 *
 * Calling/Exit State:
 *	None. The LWP will be set running if there is something that should
 *	be considered for swapping in. During the selection and subsequent
 *	swapin of a candidate process, the swapper list lock will be held.
 */
/* ARGSUSED */
void
sched_swapin(void *sched_argp)
{
	extern void poolrefresh_outofmem();

	/*
	 * Computation of swap-in priority: We compute the swap-in priority by
	 * biasing the io_cost, cpu priority, out-age, and sleep time prior to
	 * swap-out by the above weights.
	 *
	 * Computation of IO cost: Of these, io_cost, which is computed as
	 * a function of process size, needs some thought. In order not to bias
	 * extremely against large processes, the io_cost is a non-linear fun-
	 * ction of size. More accurately, it is piecewise linear, as follows:
	 *
	 *	size (pages)				negative bias
	 *	------------				-------------
	 *	upto 30					1*size
	 *	between 30 and 120			linear bet. 30 to 60
	 *	between 120 and 480			linear bet. 60 to 120
	 *	above 480				120
	 *
	 * The idea is to represent the decay of size consideration with respect
	 * to length of time spent out, without actually automatically decaying
	 * the relative impact of size differences between processes that are
	 * swapped out for long intervals due to excessive load. For example,
	 * if we just decayed the io-cost component every 10 seconds by 50%,
	 * then the cost difference between a 1M process and a 128K process
	 * will vanish in 60 seconds, if both processes stay out that long.
	 * Instead by the above policy of computing io_cost, we would prefer
	 * the smaller process as long as the larger process has not been out
	 * a minute longer.
	 */

	/*
	 * The following definitions are for the computation of io-cost, and
	 * the swap-in priority.
	 */
#define c1	30		/* cost threshold 1 */
#define c2	2*c1		/* cost threshold 2 */
#define c3	2*c2		/* cost threshold 3 */
#define t1	c1		/* size threshold 1 */
#define t2	(2*2*t1)	/* size threshold 2 */
#define t3	(2*2*t2)	/* size threshold 3 */
#define s1	(t1/c1)
#define s2	((t2-t1)/(c2-c1))
#define s3	((t3-t2)/(c3-c2))
#define cost(x, low, thresh, slope)	((low)+(((x)-(thresh))/(slope)))
#define io_cost(n) (((n) < t1) ? ((n)/s1) : \
             		  (((n) < t2) ? (cost((n),c1,t1,s2)) : \
				(((n) < t3) ? cost((n),c2,t2,s3) : c3)))

	/*
	 * XXX: Can we replace the above cost computation by something 
	 *      a little simpler? 
	 */

	int	swappri, procsize;
	proc_t *swapproc;
	int	swapin_prio;
	proc_t *p;
	int	needs;
	int	tmp;
	extern event_t pageout_event;

	u.u_lwpp->l_name = "swapin";

	for(;;) {

		ASSERT(KS_HOLD0LOCKS());

		EVENT_CLEAR(&sched_swapin_event);
		EVENT_WAIT(&sched_swapin_event, PRIMEM);

		ASSERT(getpl() == PLBASE);

		if (swap_defer_pid != NOPID) {
			if ((lbolt > swap_defer_timeout) || should_swapin) {
				cancel_deferred_swapout();
			}
		}

#ifdef DEBUG
		if (_cons_trace_) {
			/*
			 *+ debugging information to console
			 */
			cmn_err(CE_CONT, "sched: begin swap in selection\n");
		}
#endif


		/*
		 * Search the list of swapped out processes for a runnable
		 * process that we might bring in.
		 */

		FSPIN_LOCK(&swapped_list_lock);
		swapproc = NULL;
		swapin_prio = INT_MIN;
		for (p = swapped_procs; p != NULL; p = p->p_swnext) {
			ASSERT(NOT_LOADED(p));
			ASSERT((PSLOT2PROC((p->p_slot))) == p);
			if (NOTRUNNABLE(p)) /* Can check without p_mutex */
				continue;

			/*
			 * Compute the process size including U pages, for
			 * estimating I/O cost. 
			 */
			procsize = ublock_npages(p) + p->p_swrss;

			/* 
			 * PERF:
			 * Of the variables used to compute swap-in priority,
			 * p_swouttime and p_maxlwppri do not change while a 
			 * process is swapped out. (LWP scheduling priorities 
			 * do not change while the process as a whole is 
			 * swapped out). p_slptime will change.
			 * Therefore, we may compute the swap-in priority
			 * partially, and store it in proc structure, instead 
			 * of the p_swouttime. A problem to be concerned about 
			 * is the possibility of an overflow due to the product:
			 * 		swap_weight*(p_swouttime),
			 * if swap_weight is much greater than 1.
			 * Also, if (when) that is done, swap_weight*lbolt 
			 * must be added into swappri computation so that the 
			 * deservin check performed below is correct.
			 */
			swappri = swap_weight * (lbolt - p->p_swouttime) +
				  cpu_weight * (p->p_maxlwppri) +
				  (sleep_weight * HZ/2 * (p->p_slptime)) -
				  (io_weight * io_cost(procsize));

			if (swappri > swapin_prio) {
				swapin_prio = swappri;
				swapproc = p;
			}
		}
		FSPIN_UNLOCK(&swapped_list_lock);

#ifdef DEBUG
		if (_cons_trace_) {
			/*
			 *+ debugging information to console
			 */
			cmn_err(CE_CONT, 
				"sched: swap-in best procp is %x\n", swapproc);
		}
#endif

		if (swapproc == NULL) {
			continue;
		}

		/*
		 * Calculate how deserving the process is, and estimate
		 * the memory needed for bringing it in. 
		 *
		 * PERF:
		 *	The threshold needs to be increased dynamically
		 *	under heavy memory usage conditions so that
		 *	processes must meet harder criteria for coming in.
		 */
		if (swapin_prio > (HZ * MAXOUTAGE)) {
			deservin = 1;
		}

		/*
		 * If the process has a pending defaulted signal then
		 * don't count any of its swapped rss when estimating
		 * memory needs, since the process will most probably exit
		 * after getting in. The problem: how to check this without
		 * a lot of overhead, since we can't distinguish easily
		 * between defaulted signals and caught signals.
		 *
		 * It can be easily checked whether a process has *any*
		 * pending process-instance signals, which include
		 * defaulted, caught, non-ignored, and non LWP-specific
		 * signals. We can use the fact that such signals are pending
		 * against a process to let "needs" reflect just the minimum
		 * number of pages that need to be available for swapping the
		 * process in.
		 */

		swapin_mem_need = needs = ublock_npages(swapproc);
		swapin_mem_need += verylowfree;

		if (sigisempty(&swapproc->p_sigs)) {
			/*
			 * The computation of needs below, needs to be done
			 * with some indication of how long the process has
			 * been out, so that a very large process does not
			 * wedge here for long periods. 
			 *
			 * For this reason, for every 512 clock ticks above 
			 * the threshold value of SWOUT_THRESH, we will 
			 * discount the p_swrss field by 50%. 
			 *
			 * PERF: This should probably be based on some other
			 * starting point, such as the first time we failed
			 * to have enough memory to bring this process in.
			 */
			if (deservin && ((tmp = ((lbolt - swapproc->p_swouttime)
					 - SWOUT_THRESH)) > 0)) {
				tmp >>= 9 ;
				needs += ((swapproc->p_swrss >> tmp) +
						verylowfree);
			} else {
				tmp = 0;
				needs += (swapproc->p_swrss + desfree);
			}
		}

		/*
		 * see if the process can be loaded, and adjust
		 * the deficit accordingly.
		 */

		/* 
		 * PERF:
		 * Should we use freemem or mem_avail, in the decisions below?
		 * Since swapin() is a customer of clean, free memory, it
		 * seems reasonable to base swapin decisions on freemem.
		 * That could cause a large process to stay swapped out for
		 * longer durations; expect we will take care of that by
		 * recomputing its "needs" based on how long it has been out.
		 */ 
		if ((freemem >= needs &&
		     (mem_avail > needs + deficit || deservin)) ||
		    (should_swapin && freemem >= swapin_mem_need)) {

			if (swapin(swapproc)) {
				deficit += swapproc->p_swrss;
				deficit = MIN(deficit, max_deficit);
				swapin_mem_need = verylowfree;
			}

		} else if (deservin || should_swapin) {
			if (mem_avail < needs)
				EVENT_SIGNAL(&sched_unload_event, 0);

			/*
			 * The nudge to pageout and poolrefresh daemons 
			 * here is in order to ensure good responsiveness 
			 * under memory stress conditions.
			 */
			if (swapin_mem_need >= lotsfree) {
				EVENT_SIGNAL(&pageout_event, 0);
				poolrefresh_outofmem();
			}
		}
	} /* for (;;) */
}

/*
 * u_int
 * get_swapque_cnt(void)
 *	Called once per second and returns the number of runnable but
 *	swapped out lwps.  This count is used to update the swapque
 *	metrics.
 *
 * Calling/Exit State:
 *	The routine will traverse the swapped processes list under the
 *	protection of swapped_list_lock. If the lock cannot be obtained
 *	(because either swapout or swapin hold it), then the function
 *	will return 0 rather than spin for the lock.
 */
u_int
get_swapque_cnt()
{
	proc_t *pp;
	struct lwp *lwpp;
	u_int count = 0;

	if (!FSPIN_TRYLOCK(&swapped_list_lock)) {
#ifdef DEBUG
		met_missed_swaplock++;
#endif
		return(0);
	}

	for (pp = swapped_procs; pp != NULL; pp = pp->p_swnext)
		for (lwpp = pp->p_lwpp; lwpp != NULL; lwpp = lwpp->l_next)
			if (lwpp->l_stat == SRUN)
				count++;
	FSPIN_UNLOCK(&swapped_list_lock);	
	return(count);
}


#ifdef DEBUG

/*
 * Debugging code for swapper
 */
STATIC	int maxlistsz = MAX(NBIG, NSLP);
STATIC	void
sched_dbprint_list(struct unload_proc *listp)
{
	struct unload_proc *bp = listp->up_next;
	int i = 0;

	while (bp->up_next != listp) {
		cmn_err(CE_CONT, "%d\t%d\t%d\n", i++, bp->up_slotid,
			bp->up_swappri);
		bp = bp->up_next;
	}
}

STATIC void
check_list_count(struct unload_proc *listp, int num_procs)
{
	struct	unload_proc *bp;
	int	listsize = 0;

	ASSERT((listp == &zzlist) || (listp == &bplist));
	ASSERT(listp->up_slotid == -1);

	bp = listp->up_next;
	ASSERT(bp != NULL);
	while (bp != listp) {
		ASSERT(bp->up_swappri < INT_MAX);
		if (bp->up_swappri > INT_MIN) {
			ASSERT(bp->up_slotid != -1);
			++listsize;
		} else {
			ASSERT(listp->up_slotid == -1);
		}
		bp = bp->up_next;
	}
	ASSERT(listsize == num_procs);
}


STATIC	void
check_list_order(struct unload_proc *listp)
{
	struct	unload_proc *bp;
	int	listsize = 0;

	ASSERT((listp == &zzlist) || (listp == &bplist));
	ASSERT(listp->up_slotid == -1);

	bp = listp->up_next;
	ASSERT(bp != NULL);
	while (bp != listp) {
		if (++listsize > maxlistsz) {
			/*
			 *+ unexpectedly large number of elements on the
			 *+ list. Kernel software error; no recovery.
			 */
			cmn_err(CE_PANIC,
			 "check_list_order: list at 0x%x too long\n", listp);
		}
		if (bp->up_swappri > bp->up_next->up_swappri) {
			sched_dbprint_list(listp);
			if (listp == &zzlist) {
				/*
				 *+ Elements on the list not in the desired 
				 *+ order. Kernel software error; no recovery.
			 	 */
				cmn_err(CE_PANIC, "zzzp list out of order\n");
			} else {
				/*
				 *+ Elements on the list not in the desired 
				 *+ order. Kernel software error; no recovery.
			 	 */
				cmn_err(CE_PANIC, "bigp list out of order\n");
			}
		}
		bp = bp->up_next;
	}
}

STATIC void
check_swapped_procs(void)
{
	int i = 0;
	proc_t *p = swapped_procs;

	while (p != NULL) {
		if (++i > swapped_count) {
			/*
			 *+ debugging code. Klint shut up.
			 */
			cmn_err(CE_PANIC,"inconsistently high swapped count\n");
		}
		ASSERT(PSLOT2PROC(p->p_slot) == p);
		/* check that process state for p agrees with its being on */
		/* the swapped-out list */
		ASSERT((p->p_flag & P_SEIZE) != 0);
		p = p->p_swnext;
	}
	if (i != swapped_count) {
		/*
		 *+ debugging code. Klint shut up.
		 */
		cmn_err(CE_PANIC, "inconsistent swapped processes count\n");
	}
}

#endif /* DEBUG */


