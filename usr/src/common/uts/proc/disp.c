/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/disp.c	1.68.1.8"
#ident	"$Header: $"

#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <mem/vmparam.h>
#include <mem/vm_mdep.h>
#include <proc/class.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/priocntl.h>
#include <proc/proc.h>
#include <proc/seize.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/bitmasks.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>
/*
 * This is the run-queue mutex.  It is held whenever searching any
 * of the lists associated with a run-queue and while pulling an lwp
 * off the run-queue.
 */
fspin_t runque_mutex;

extern void priocntl_init(void);
extern timestruc_t hrestime;
STATIC void swtch_seized(lwp_t *);


STATIC int	nglobpris;	/* number of global sched prios configured */
runque_t *global_rq;		/* pointer to the global run-queue(s) */

STATIC runque_t *runque_alloc(void);
STATIC void runque_free(runque_t *);

#ifndef UNIPROC
int avgqlen;	/* average local runqueue length - used in affinity code */
extern int load_balance_freq;
#endif

int prmpt_enable = 1;
extern int check_preemption_f(void);

/*
 * dispinit(void)
 *
 * Scheduler initialization.  Initialize all dispatcher data structures.
 *
 * Calling/Exit State:
 *
 *	Must be called after the KMA is initialized and before any other
 *	engines are onlined or lwp's created.
 *
 * Description:
 *
 *	Initialize all scheduling classes, the run-queue mutex and the
 *	global run-queue.
 *
 */
void
dispinit(void)
{
	register id_t	cid;
	register int	maxglobpri;
	register runque_t *rq;
	int		cl_maxglobpri;
#ifndef UNIPROC
	void load_average(void);
#endif

	maxglobpri = -1;

	/*
	 * Call the class specific initialization functions. We pass the size
	 * of a class specific parameter buffer to each of the initialization
	 * functions to try to catch problems with backward compatibility of
	 * class modules.  For example a new class module running on an old
	 * system which didn't provide sufficiently large parameter buffers
	 * would be bad news.  Class initialization modules can check for
	 * this and take action if they detect a problem.
	 */
	for (cid = 0; cid < nclass; cid++) {
		(*class[cid].cl_init)(cid, PC_CLPARMSZ, &class[cid].cl_funcs,
		    &cl_maxglobpri);
		if (cl_maxglobpri > maxglobpri)
			maxglobpri = cl_maxglobpri;
	}

	nglobpris = maxglobpri + 1;

	/*
	 * Initialize the run-queue lock.
	 */
	RUNQUE_INIT();

	/*
	 * Allocate memory for the global run-queue and attach this
	 * run-queue to all engines configured.
	 */

	/*
	 * We create and initialize two global run-queues for testing
	 * purposes.  This allows us to gain coverage on those portions of
	 * swtch and qswtch which need to deal with multiple run-queues.
	 */
	rq = runque_alloc();
	if (rq == NULL)
		/*
		 *+ Test code panic.
		 */
		cmn_err(CE_PANIC, "Can't allocate memory for global run-queue.");

	global_rq = rq;

#ifdef	lint
	runque_free(rq);
#endif
	dispmdepinit(nglobpris);
	priocntl_init();

#ifndef UNIPROC
	(void)itimeout(load_average, 0, load_balance_freq, PLHI);
#endif
}

/*
 * void dispp0init(lwp_t *lwp)
 *
 *	Do scheduling class specific initialization of process 0 lwp-1.
 *
 * Calling/Exit State:
 *
 *	Called at boot time and is single threaded, thus no locking
 *	required.
 */
void
dispp0init(lwp_t *lwpp)
{
	lwpp->l_cllwpp = (caddr_t) lwpp;
	lwpp->l_clfuncs = class[0].cl_funcs;	/* system class */
	lwpp->l_rq = global_rq;
	lwpp->l_eng = l.eng;
}

/*
 * runque_alloc(void)
 *
 * Allocate and initialize a run-queue.
 *
 * Calling/Exit State:
 *
 *	None.
 *
 * Description:
 *
 *	Allocate all necessary space for a run-queue.  This includes
 *	the dispatcher queue proper and summary information.
 */
STATIC runque_t *
runque_alloc(void)
{
	register runque_t *rq;
	register list_t *dq, *enddq;

	if ((rq = (runque_t *)kmem_zalloc(sizeof(*rq), KM_NOSLEEP)) == NULL)
		return (NULL);

	dq = (list_t *)kmem_zalloc(nglobpris * sizeof(list_t), KM_NOSLEEP);
	if (dq == NULL) {
		kmem_free((void *)rq, sizeof(*rq));
		return (NULL);
	}

	rq->rq_dispq = dq;

	/*
	 * Initialize the dispatcher queue.
	 */
	enddq = dq + nglobpris;
	while (dq < enddq) {
		dq->flink = dq->rlink = dq;
		dq++;
	}

	rq->rq_dqactmap = (uint_t *)kmem_zalloc(BITMASK_NWORDS(nglobpris) *
					sizeof(uint_t), KM_NOSLEEP);
	if (rq->rq_dqactmap == NULL) {
		kmem_free((void *)rq->rq_dispq, nglobpris * sizeof(list_t));
		kmem_free((void *)rq, sizeof(*rq));
		return (NULL);
	}

	rq->rq_maxrunpri = -1;
#ifndef UNIPROC
	rq->rq_lastnudge = rq->rq_nudgelist;
#endif  /* UNIPROC */

	return (rq);
}

/*
 * runque_free(runque_t *rq)
 *
 * Deallocate the runqueue pointed to by "rq".
 *
 * Calling/Exit State:
 *
 *	Rq must no longer be in use.
 *
 * Description:
 *
 *	Deallocate the linked list and summary arrays, as well
 *	as the run-queue proper.
 */
STATIC void
runque_free(runque_t *rq)
{
#ifdef	DEBUG
	register list_t *dq, *end;

#ifndef UNIPROC
	register struct engine **nudgelist, **nudgelist_end;
#endif

	/*
	 * Make absolutely certain the run-queue is empty.
	 */
	dq = rq->rq_dispq;
	end = dq + nglobpris;
	while (dq < end) {
		if (dq->flink != dq || dq->rlink != dq)
			cmn_err(CE_PANIC, "runque_free:  rq_dispq not empty");
		dq++;
	}

	ASSERT(rq->rq_maxrunpri == -1);
	ASSERT(rq->rq_srunlwps == 0);

	/*
	 * Make sure rq_dqactmap is zero.
	 */
	ASSERT(!BITMASKN_TESTALL(rq->rq_dqactmap, BITMASK_NWORDS(nglobpris)));

	/*
	 * Make sure the nudge-list is clean.
	 */
#ifndef UNIPROC
	nudgelist = rq->rq_nudgelist;
	nudgelist_end = nudgelist + MAXNUMCPU;
	while (nudgelist < nudgelist_end) {
		ASSERT(*nudgelist == NULL);
		nudgelist++;
	}
#endif  /* UNIPROC */
#endif	/* DEBUG */

	kmem_free((void *)rq->rq_dqactmap,
		  BITMASK_NWORDS(nglobpris) * sizeof(uint_t));
	kmem_free((void *)rq->rq_dispq, nglobpris * sizeof(list_t));
	kmem_free((void *)rq, sizeof(*rq));
}

/*
 * Currently, disponline allocates a local runqueue to be used for
 * binding. Later, this may be changed to allocate run queues dynamically.
 *
 * In almost-symmetric processor configurations,
 * disponline/dispoffline need to diagnose the processor type (in some manner)
 * and attach the engine to the proper run-queue (i.e. in a FPA configuration,
 * lwp's using the FPA would be on a given global run-queue, engine's posessing
 * an FPA would all schedule from this queue).
 */

static void attach_rq(engine_t *, runque_t *);

/*
 * disponline(engine_t *eng)
 *
 * Prepare "eng" for dispatch and preemption.
 *
 * Calling/Exit State:
 *
 *	No locks need be held by caller.
 *
 * Description:
 *
 *	An engine has just come online and is declaring itself available for
 *	scheduling.  Initialize the per-engine scheduling data structures to
 *	point to the default run-queues.  Point the default run-queues at the
 *	engine.  This allows the engine to successfully schedule lwp's and
 *	lwp's to preempt the engine.
 */
void
disponline(engine_t *eng)
{
#ifndef UNIPROC
	register runque_t *rq;

	/*
	 * Allocate a local run queue for the processor.
	 */
	rq = runque_alloc();
	if (rq == NULL)
	/*
	 *+ Test code panic.
	 */
		cmn_err(CE_PANIC, "Can't allocate memory for local run-queue.");
#endif  /* UNIPROC */
	RUNQUE_LOCK();
#ifndef UNIPROC
	attach_rq(eng, rq);
#endif  /* UNIPROC */

	attach_rq(eng, global_rq);
	eng->e_pri = eng->e_npri = -1;
	eng->e_flags &= ~E_OFFLINE;
	eng->e_smodtime = hrestime;
	RUNQUE_UNLOCK();
}

static engine_t *rti_violation(runque_t *);
static void detach_rq(engine_t *, runque_t *);

/*
 * dispoffline(engine_t *eng)
 *
 * Remove "eng" from preemption and dispatch prior to offline.
 *
 * Calling/Exit State:
 *
 *	Assumes the caller does not hold the run-queue lock.  Disassociates the
 *	engine from consideration for dispatch (primarily preemption).
 *	After returning, the engine will not be able to dispatch and will
 *	no longer be preempted.
 *
 * Description:
 *
 *	An engine is about to go offline and is declaring itself unavailable
 *	for scheduling.
 *
 *	This must be done with great care, as we may have a nudge pending
 *	against us.  If this is so, we need to examine the run-queues we're
 *	scheduling from and determine if any of these run-queues and the
 *	engine's they schedule from are in violation of the run-time-invariant
 *	(RTI - condition which is true when the available engine's are running
 *	the highest priority lwp's on the system).
 */
void
dispoffline(engine_t *eng)
{
	register runque_t *rq;
	register runque_t **rpp;
	register engine_t *neng;

	RUNQUE_LOCK();
	if (eng->e_npri != -1) {
		/*
		 * We currently have a nudge pending, walk through
		 * our run-queues, determining if we should transfer
		 * the nudge to another engine.
		 *
		 * Since the nudge was sent to us to cause the RTI
		 * to be satisfied, we only need to find and nudge
		 * engines scheduling from run-queues which are
		 * currently violating the run-time-invariant.
		 */
		for (rpp = (runque_t **) eng->e_rqlist; 
					(rq = *rpp) != NULL; rpp++) {
			if (neng = rti_violation(rq))
				kpnudge(rq->rq_maxrunpri, neng);
		}
	}

	eng->e_npri = -1;
	eng->e_pri = -1;
	eng->e_flags |= E_OFFLINE;
	eng->e_smodtime = hrestime;

	/*
	 * Detach the global run-queue.
	 */
	rq = global_rq;
	detach_rq(eng, rq);

#ifndef UNIPROC
	/*
	 * Cannot remove the last engine from a run-queue
	 * via a offline.  This amounts to stranding the
	 * lwp's on the run-queue by offlining the engine.
	 * This can never be true, as an engine cannot be
	 * offlined when it has bound lwp's, when it is
	 * attached to a private lwp-set or when it is the
	 * last processor attached to the default lwp-set.
	 */
	ASSERT(rq->rq_nudgelist[0] != NULL);

	/*
	 * Now, detach and deallocate local run queue
	 */
	rq = eng->e_rqlist[0];
	detach_rq(eng, rq);

	/*
	 * We had better deleted all run-queues the engine was
	 * scheduling from.
	 */
	ASSERT(eng->e_rqlist[0] == NULL);
#endif  /* UNIPROC */

	RUNQUE_UNLOCK();

#ifndef UNIPROC
	runque_free(rq);
#endif  /* UNIPROC */

	return;
}

/*
 * void attach_rq(engine_t *eng, runque_t *rq)
 *
 *	Attach a run-queue to an engine.
 *
 * Calling/Exit State:
 *
 *	Must be called holding the runque-lock.  We attach the run-queue
 *	pointed to by "rq" to the engine "eng".
 */
void
attach_rq(engine_t *eng, runque_t *rq)
{
	register engine_t **epp;
	register runque_t **rpp;

	ASSERT(RUNQUE_OWNED());

	/*
	 * Attach the engine to the local run-queue.
	 */
#ifdef UNIPROC
	rq->rq_nudgelist = eng;
#else
	epp = rq->rq_nudgelist;
	while (*epp != NULL)
		epp++;

	ASSERT(epp < &rq->rq_nudgelist[MAXNUMCPU]);
	*epp = eng;
	*++epp = NULL;
#endif

	/*
	 * Attach the run-queue to the engine.
	 */
#ifdef UNIPROC
	eng->e_rqlist = rq;
#else
	rpp = eng->e_rqlist;
	while (*rpp != NULL)
		rpp++;
	ASSERT(rpp < &eng->e_rqlist[MAXRQS]);
	*rpp = rq;
	*++rpp = NULL;
	eng->e_lastpick = eng->e_rqlist;
#endif  /* UNIPROC */
	return;
}

/*
 * void
 * detach_rq(engine_t *eng, runque_t *rq)
 *
 *	Disassociate runque "rq" from engine "eng".
 *
 * Calling/Exit State:
 *
 *	Must be called holding the run_queue mutex.
 */
STATIC void
detach_rq(engine_t *eng, runque_t *rq)
{
	register engine_t **epp, **oepp;
	register runque_t **rpp, **orpp;

	ASSERT(RUNQUE_OWNED());

	/*
	 * Remove the engine from the array of engine considered
	 * for preemption when an lwp is set runnable on this run-queue.
	 */
#ifdef UNIPROC
	rq->rq_nudgelist = NULL;
#else
	epp = rq->rq_nudgelist;
	while (*epp != NULL && *epp != eng)
		epp++;

	ASSERT(*epp != NULL);

	/*
	 * Shift the array down.
	 */
	oepp = epp++;
	while (*oepp++ = *epp++)
		continue;

	epp = rq->rq_lastnudge;
	if (*epp == NULL)
		rq->rq_lastnudge = rq->rq_nudgelist;
#endif  /* UNIPROC */
	

	/*
	 * Remove the run-queue from the array of run-queues considered
	 * when this engine schedules an lwp.
	 * Find the run-queue on the run-queues scheduled from.
	 */
#ifdef UNIPROC
	eng->e_rqlist = NULL;
#else
	rpp = eng->e_rqlist;
	while (*rpp != NULL && *rpp != rq)
		rpp++;

	ASSERT(*rpp != NULL);

	/*
	 * Shift the array of run-queue pointers down on top of the
	 * current entry, thereby deleting it.
	 */
	orpp = rpp++;
	while (*orpp++ = *rpp++)
		continue;

	if (*eng->e_lastpick == NULL)
		eng->e_lastpick = eng->e_rqlist;
#endif  /* UNIPROC */

	return;
}

/*
 * lwpstat_t dispnewpri(lwp_t *lwp, int pri)
 *
 *	Change the priority of "lwp" to pri.
 *
 * Calling/Exit State:
 *
 *	Must be called with and returns with the lwp->l_mutex held.
 *	Returns an indication of whether the lwp was found runnable.
 *
 * Description:
 *
 *	We change the priority of lwp to "pri".  Depending on the
 *	current running status of lwp, this implies various additional
 *	processing.  If the lwp is runnable, we must remove the lwp from
 *	its current run-queue and re-insert it on its new run-queue.  If the
 *	lwp is running, we must update the e_pri of the engine its running
 *	on and determine if the lwp is OK to be running on the engine in
 *	question.  If the lwp is currently sleeping, only the priority
 *	need be updated, however, we return an indication of this, just
 *	in case the caller wants to do any queue reordering as a result of
 *	the priority change.
 */
lwpstat_t
dispnewpri(lwp_t *lwp, int pri)
{
	int oldpri;
	int wasonq;

	ASSERT(LOCK_OWNED(&lwp->l_mutex));

	/*
	 * Change the priority of the lwp.  This
	 * must be done carefully when we're dealing with
	 * a runnable or running lwp.
	 */
	switch (lwp->l_stat) {
	case SRUN:
		RUNQUE_LOCK();
		if (lwp->l_stat == SONPROC)
			goto onproc;

		/*
		 * Runnable lwp.  Need to remove it from
		 * its current run-queue and place it on
		 * its new run-queue.
		 */
		if (!LWP_SEIZED(lwp) && LWP_LOADED(lwp)) {
			wasonq = dispdeq(lwp);
			ASSERT(wasonq != 0);
			lwp->l_pri = pri;
			setbackrq(lwp);
		} else
			lwp->l_pri = pri;
		RUNQUE_UNLOCK();
		return(SRUN);
	case SONPROC:
		/*
		 * Running lwp.  Need to update the priority
		 * of the lwp and update the engine's
		 * priority.
		 */
		RUNQUE_LOCK();
	onproc:
		oldpri = lwp->l_pri;
		lwp->l_pri = pri;
		lwp->l_eng->e_pri = lwp->l_pri;

		if (oldpri > pri)
			/*
			 * Determine if the guy should be running, as we
			 * just lowered his priority and there may another
			 * higher-priority lwp waiting.
			 */
			ipreemption(lwp, nudge);
		RUNQUE_UNLOCK();
		return(SONPROC);
	default:
		/*
		 * Just change the priority, nothing extra needed.
		 */
		lwp->l_pri = pri;
		break;
	}
	return(lwp->l_stat);
}

/*
 * void dispnolwp(void)
 *
 *	Indicate the current engine is not running an lwp.
 *
 * Calling/Exit State:
 *
 *	Must not be called with the run-queue locked.
 *
 * Description:
 *
 *	dispnolwp is used by exit and lwp_exit when the current lwp is
 *	no longer capable of supporting per-lwp activities in the clock
 *	handler.  We mark the engine such that it is effectively
 *	considered idle.
 */
void
dispnolwp(void)
{
	RUNQUE_LOCK();
	l.eng->e_pri = -1;
	RUNQUE_UNLOCK();
}

static int nexclusive;

/*
 * int dispexbindok(engine *eng)
 *
 *	Determine if it's OK to exclusively bind "eng".
 *
 * Calling/Exit State:
 *
 *	Must be called holding eng_tbl_mutex.  Returns true if it's OK
 *	to take the engine exclusive.
 *
 * Description:
 *
 *	It's OK to exclusively bind to an engine if the configured number
 *	of engines are left non-exclusively bound and the engine doesn't
 *	already have bindings.
 */
/* ARGSUSED */
int
dispexbindok(engine_t *eng)
{
	u_int min = 1;

	ASSERT(LOCK_OWNED(&eng_tbl_mutex));

	if (min < v.v_nonexclusive)
		min = v.v_nonexclusive;

	if (nexclusive >= nonline - min)
		return(0);

	if (eng->e_count > 0)
		return(0);

	return(1);
}

/*
 * void dispexclusive(engine_t *eng)
 *
 *	Make the engine schedule exclusively from its local queue.
 *
 * Calling/Exit State:
 *
 *	Must be called holding eng_tbl_mutex.
 */
void
dispexclusive(engine_t *eng)
{
	engine_t *neng;

	ASSERT(LOCK_OWNED(&eng_tbl_mutex));
	RUNQUE_LOCK();
	detach_rq(eng, global_rq);
#ifndef UNIPROC
	ASSERT(eng->e_rqlist[1] == NULL && eng->e_rqlist[0] != NULL);
#endif
	eng->e_flags |= E_EXCLUSIVE;

	if (eng->e_npri >= 0) {
		/*
		 * We may have obligations to the runque we're
		 * abandoning.  Check for and pass along any nudge we
		 * may have gotten.
		 */
		neng = rti_violation(global_rq);
		if (neng)
			kpnudge(global_rq->rq_maxrunpri, neng);
		eng->e_npri = -1;
	}

	if (eng->e_npri < 0)
		/*
		 * Force the engine to reschedule.
		 */
		kpnudge(eng->e_pri, eng);

	nexclusive++;

	RUNQUE_UNLOCK();
}

/*
 * void dispnonexclusive(engine_t *eng)
 *
 *	Make the engine available for global scheduling.
 *
 * Calling/Exit State:
 *
 *	Must be called holding eng_tbl_mutex.  Reattach the engine
 *	to the new run-queue.
 */
void
dispnonexclusive(engine_t *eng)
{
	ASSERT(LOCK_OWNED(&eng_tbl_mutex));
	ASSERT(eng->e_count == 0);

	RUNQUE_LOCK();
	attach_rq(eng, global_rq);
	eng->e_flags &= ~E_EXCLUSIVE;

	if (eng->e_npri < 0)
		/*
		 * Force the engine to reschedule.
		 */
		kpnudge(eng->e_pri, eng);

	nexclusive--;

	RUNQUE_UNLOCK();
}

/*
 * int dispofflineok(engine *eng)
 *
 *	Determine if it's OK to take "eng" offline.
 *
 * Calling/Exit State:
 *
 *	Must be called holding eng_tbl_mutex.  Returns true if it's OK
 *	to take the engine offline.
 *
 * Description:
 *
 *	Make sure engine's which have bindings aren't taken offline.  Make
 *	sure we don't take something offline, leaving less than v_nonexclusive
 *	engines available to the non-exclusive workload.
 */
int
dispofflineok(engine_t *eng)
{
	int min = 1;

	ASSERT(LOCK_OWNED(&eng_tbl_mutex));

	if (min < v.v_nonexclusive)
		min = v.v_nonexclusive;

	if (nexclusive > 0 && nonline - nexclusive <= min)
		/*
		 * We're on the edge of the minimum number of
		 * engines guaranteed to the minimum workload.
		 * Note, if there aren't any engine's exclusively
		 * bound to, we don't apply the limitation.
		 * We're simply interested in preventing all the
		 * non-exclusively bound engine from being taken
		 * offline, leaving only exclusively bound engines.
		 */
		return(0);

	if (eng->e_count > 0)
		return(0);

	return(1);
}

/*
 * preemption(lwp_t *lwp, void (*nudge(), int prmpt, engine_t *engp)
 *
 * See if "lwp" should preempt anyone.
 *
 * Calling/Exit State:
 *
 *	Must be called with the run-queue and per-lwp mutex locked.
 *	Returns with the run-queue and per-lwp mutex locked.
 *
 * Description:
 *
 *	Associated with each run-queue is a list of processors which are
 *	scheduling from this run-queue.  We simply traverse the list of
 *	processors, looking for the processor running the lowest priority
 *	lwp which is less than the priority of the lwp we just made
 *	runnable.
 *
 *	We do the search in a circular fashion, starting after the last place
 *	we left off.  Starting like this tends to spread the preemptions out
 *	amonst all processors scheduling on this run-queue, as opposed to
 *	unfairly singling out a given processor.  If one or more engine's are
 *	running an lwp which has a priority lower then the global priority
 *	"lwp", we find the lowest priority of these and send it a reschedule
 *	by calling "nudge".  If an idle engine is found along the way, the
 *	search is terminated immediately.
 *
 *	If the engp argument is set (during affinity or binding), there
 *	is only one processor that we need to check, which is the bound or
 *	affinitized processor. If that processor is running a lower
 *	priority LWP or if it has a pending nudge of a lower priority,
 *	the processor is nudged.
 */
void
preemption(lwp_t *lwp, void (*nudge)(), int prmpt, engine_t *engp)
{
	register engine_t **ep, **start, *e;
	runque_t *rq = lwp->l_rq;
	engine_t *bestpick;
	int lowest, pri;
	short flagtime = 0;

	ASSERT(rq != NULL);
	ASSERT(RUNQUE_OWNED());

	bestpick = NULL;
	lowest = lwp->l_pri;

#define COND_TIME(cond) { \
	if ((cond) == kpnudge) { \
		flagtime = !flagtime; \
		GET_TIME(&lwp->l_dispt); \
	} \
       }

#ifdef UNIPROC
	pri = MAX(engine->e_npri, engine->e_pri);
	if ((prmpt & KS_NOPRMPT) && (pri != -1))
		/*
		 * Don't preempt if KS_NOPRMPT is set and engine
		 * is not idle.
		 */
		return;
	if (lowest > pri)
		(*nudge)(lwp->l_pri, engine);
#else
	if (engp != NULL) {
		/*
		 * This LWP has been put on the local run queue of
		 * of the processor. Therefore, it is
		 * sufficient to check if the processor is
		 * running an LWP that is lower in priority than
		 * the LWP under consideration (if so, nudge the
		 * processor). There is no need to look at what
		 * the other processors are running.
		 */
		pri = MAX(engp->e_npri, engp->e_pri);
	        if ((prmpt & KS_NOPRMPT) && (pri != -1))
		/*
		 * Don't nudge an engine unless it's currently
		 * idle.
		 */
		      return;

	       if (engp->e_pri < lwp->l_pri)
		      (*nudge)(lwp->l_pri, engp);
	       return;
	}
	/*
	 * Search for an engine running an lwp with a lower-priority
	 * than we have.  The search begins after the last engine we found.
	 */
	ep = start = rq->rq_lastnudge;
	if (!*ep)
		/*
		 * Run queue is not currently bound to any processors.
		 * This occurs when setting an lwp runnable which is in
		 * an lwp-set which is unattached, or attached to an
		 * empty processor-set.
		 * This works out, as the lwp will remain on the run-queue
		 * until a processor is assigned to its containing run-queue
		 * and begins to schedule from its associated run-queue.
		 * Until then, the lwp will remain suspended in the SRUN
		 * state.
		 */
		return;

	/*
	 * Search over the list of processors scheduling from this run-queue
	 * and find the one who's running the lowest lwp, whose priority is
	 * less then the lwp we just added to the run-queue.
	 */
	do {
		if (!(e = *++ep)) {
			/*
			 * Hit the end of the list, start again
			 * at the beginning.
			 */
			ep = rq->rq_nudgelist;
			e = *ep;
		}
		pri = MAX(e->e_npri, e->e_pri);
		if (pri == -1) {
			/*
			 * Found an idle processor, short-circuit the
			 * search, as we know this processor has nothing
			 * better to do.
			 */
			bestpick = e;
			break;
		}
		if (lowest > pri) {
			lowest = pri;
			bestpick = e;
		}
	} while (ep != start);

	if (bestpick) {
		if ((prmpt & KS_NOPRMPT) && pri != -1)
			/*
			 * Don't preempt an engine unless it's currently
			 * idle.
			 */
			return;
		/*
		 * There's an engine running a lower priority lwp than
		 * the one we just added.  Send the engine an interrupt,
		 * causing it to reschedule.
		 */
#ifdef _MPSTATS
		/*
		 * If a kernel preemption is requested then start the 
		 * clock for measuring dispatch latency. The run-time 
		 * invariant has been violated. Note the start time for 
		 * this violation.
		 */

		COND_TIME(nudge);

#endif /* _MPSTATS */
		(*nudge)(lwp->l_pri, bestpick);
		/*
		 * We always start our search after the last engine
		 * we nudged so we don't favor a particular engine.
		 */
		rq->rq_lastnudge = ep;
	}
#endif /* UNIPROC */
}

/*
 * engine_t *
 * rti_violation(runque_t *rq)
 *
 * Examine the engine's scheduling from "rq" and determine if any of these
 * should reschedule.
 *
 * Calling/Exit State:
 *
 *	Must be called with the run-queue locked.
 *	Returns the lowest-priority engine if there is a violation.
 *
 * Description:
 *
 *	We search the list of engine's scheduling from "rq" and determine
 *	if there is a violation of the run-time-invariant (RTI).  I.e. if
 *	there is an lwp waiting which has a higher priority then a running
 *	lwp.  If any is found, the lowest priority engine in violation is
 *	returned.  The search terminated early when an idle engine is found.
 */
engine_t *
rti_violation(runque_t *rq)
{
	register engine_t **ep;
	register engine_t **start;
	register engine_t *e;
	register engine_t *bestpick;
	int lowest;
	int pri;

#ifdef UNIPROC
	e = rq->rq_nudgelist;
	pri = MAX(e->e_npri, e->e_pri);
	if (rq->rq_maxrunpri > pri)
		bestpick = e;
	else
		bestpick = NULL;
	return (bestpick);
#else
	lowest = rq->rq_maxrunpri;
	if (lowest == -1)
		/*
		 * Nothing to run, thus, no violation.
		 */
		return(NULL);

	ep = start = rq->rq_lastnudge;
	ASSERT(*ep != NULL);

	bestpick = NULL;

	/*
	 * Search over the list of processors scheduling from this run-queue
	 * looking for any which are running lwp's
	 * less then the lwp we just added to the run-queue.
	 */
	do {
		if (!(e = *++ep)) {
			ep = rq->rq_nudgelist;
			e = *ep;
		}
		if ((e->e_flags & E_SHUTDOWN) == 0)
			/*
			 * We skip engine's which are currently being
			 * shut-down.
			 */
			continue;
		pri = MAX(e->e_npri, e->e_pri);
		if (pri == -1) {
			bestpick = e;
			break;
		}
		if (lowest > pri) {
			lowest = pri;
			bestpick = e;
		}
	} while (ep != start);

	if (bestpick)
		rq->rq_lastnudge = ep;

	return(bestpick);
#endif  /* UNIPROC */
}

/*
 * ipreemption(lwp_t *lwp, void (*nudge())
 *
 * See if anyone should preempt lwp (inverse preemption).
 *
 * Calling/Exit State:
 *
 *	Must be called with the run-queue and per-lwp mutex locked.
 *	Returns with the run-queue and per-lwp mutex locked.
 *
 * Description:
 *
 *	When lowering the global priority of an lwp, in order to maintain
 *	the run time invariant, we must determine if there's an lwp capable
 *	of running on that engine which has a higher priority then the
 *	current lwp the engine is running.
 *
 *	We determine this by examining all run-queue's associated with the
 *	engine the lwp is currently running on (lwp->l_eng) and seeing if one
 *	of these has an lwp with a higher priority then us.
 *
 *	If such an lwp exists, we arrange for a reschedule at the appropriate
 *	time by calling "nudge" against the engine.
 */
void
ipreemption(lwp_t *lwp, void (*nudge)())
{
	engine_t *eng = lwp->l_eng;
	register runque_t **rqpp, *rq;
	register int pri = lwp->l_pri;

	ASSERT(LOCK_OWNED(&lwp->l_mutex));
	ASSERT(lwp->l_stat == SONPROC);
	ASSERT(eng != NULL);
	ASSERT(RUNQUE_OWNED());

	/*
	 * If there's a higher priority, we need to find the highest
	 * of these priorities (i.e. we can short-circuit the search),
	 * otherwise, we could end up with a preemption coming in, find
	 * that engine where it should have nudged another engine.
	 */
#ifdef UNIPROC
	pri = global_rq->rq_maxrunpri;
#else
	for (rqpp = eng->e_rqlist; (rq = *rqpp) != NULL; rqpp++)
		if (rq->rq_maxrunpri > pri)
			pri = rq->rq_maxrunpri;
#endif  /* UNIPROC */

	if (lwp->l_pri < pri)
		/*
		 * There is an lwp with a higher priority then ours.
		 * Pri now contains the priority of the highest priority
		 * lwp waiting for this engine.  Cause the engine to
		 * reschedule and make sure its e_npri is set properly.
		 */
		(*nudge)(pri, eng);

	return;
}

/*
 * swtch(lwp_t *lwp)
 *
 * Give up the cpu and pick another lwp to run.
 *
 * Calling/Exit State:
 *
 *	If the engine is currently running an lwp, it must be passed as "lwp"
 *	and must have its l_mutex locked.  If lwp is NULL, it indicates the
 *	engine does not have an lwp and is running on its private context.
 *
 * Description:
 *
 *	Called to choose a new lwp; this is the preferred interface for
 *	lwp's which are blocking.
 *	If lwp is NULL, it means the current processor is running on its
 *	per-processor data area and not on the context of a lwp.
 *	Otherwise, the current lwp is holding its own state-lock, has
 *	placed itself on the proper queue (callers of swtch usually have placed
 *	themselves on a sleep-queue of some sort).
 *	In either case, this loop causes the current processor to select another
 *	lwp and switch to that lwp (if present).
 *	If no lwp's are available to run, the processor switches onto its
 *	private data area and idles.
 *
 *	Processors switching off an lwp must hold the lwp-state lock,
 *	processors switching onto an lwp must hold the same lock.  This
 *	prevents a processor from running an lwp before the lwp's state has
 *	been fully saved.
 *
 *	This causes some small difficulty when we switch from one lwp to another
 *	(the far most likely case), as we hold the old lwp's lock while trying
 *	to gain the new lwp's lock.  This would normally introduce the
 *	potential for lock-ordering deadlocks, however, the existence of pswtch
 *	prevents these kind of deadlocks from developing (see comments below).
 *
 *	Always returns not holding the current lwp's state lock at PLBASE.
 */
void
swtch(register lwp_t *lwp)
{
	engine_t *eng;
	lwp_t *new;
	register runque_t **rqpp, *rq, **start;
	runque_t **bestpick;
	int fairness, maxpri;
	ulong_t disp_latency = 0L;

	ASSERT(lwp == NULL || LOCK_OWNED(&lwp->l_mutex));
	ASSERT(lwp == NULL && KS_HOLD0LOCKS() ||
	       lwp != NULL && KS_HOLD1LOCK());


#ifndef UNIPROC
	if (lwp != NULL) {
		/*
		 * Before switching out, save affinity information
		 */
		lwp->l_lastran = lbolt;
	}
#endif

	if (lwp != NULL) {
		/*
		 * If a context that was marked non-reentrant is switching out,
		 * re-enable preemption until it switches back in.
		 */
		prmpt_state -= lwp->l_notrt;
	}
	/*
	 * If we are on a per-context stack, we must be holding l_mutex
	 * and if we are executing on the idle stack we should have marked
	 * ourselves non-preemptable.
	 */
	ASSERT(prmpt_count == 1);

	if (lwp != NULL && LWP_SEIZED(lwp)) {
		/*
		 * This lwp is switching out during a seize operation.
		 * We need to report in to the seizure and if the last
		 * lwp to be seized, awaken the lwp performing the seize.
		 */
		if (become_seized(lwp)) {
			/*
			 * This is the last lwp to report into the
			 * seize operation.  We need to awaken the
			 * lwp performing the seize.
			 * Since we hold the l_mutex of "lwp", we cannot
			 * perform any "wakeup" operation against a sleeping
			 * synchronization, as doing so requires us to gain
			 * the l_mutex of the lwp performing the seize
			 * operation.  This has lock ordering problems.
			 * To avoid this, we switch to the per-processor
			 * data and perform the operation from there.
			 */
			if (save(lwp))
				return;
			use_private(lwp, swtch_seized, lwp);
			/* NOT REACHED */
		}
	}

	eng = l.eng;

	for (;;) {
#ifndef UNIPROC
		if (eng->e_flags & E_SHUTDOWN) {
			if (lwp) {
				if (save(lwp) != 0)
					return;
				use_private(lwp, swtch, NULL); 
				/* NOT REACHED */
			}
			/*
			 * Online clears the shutdown flag.
			 */
			offline_self();
		}
	
		bestpick = NULL;
		maxpri = -1;	
#endif  /* UNIPROC */


		RUNQUE_LOCK();
#ifdef UNIPROC
		if (global_rq->rq_srunlwps > 0) {  /* something to run */
			bestpick = &global_rq;
			maxpri = global_rq->rq_maxrunpri;
		} else {
			bestpick = NULL;
			maxpri = -1;
		}
#else

		rqpp = start = eng->e_lastpick;	/* always non-empty (i.e. local) */
		rq = *rqpp;

		/*
		 * Search through all run-queues we schedule from and find
		 * the run-queue containing the highest-priority lwp.
		 *
		 * If we have a set of run-queues with different priorities, we
		 * pick the run-queue with the highest priority.  When there's
		 * a tie, we need to take steps to prevent one run-queue from
		 * dominating dispatch.
		 */
		do {
			if (rq->rq_maxrunpri >= maxpri) {
				if (rq->rq_maxrunpri == maxpri) {
					/*
					 * Tie, need to invoke fairness.
					 */
					if (maxpri != -1)
						fairness = 1;
				} else {
					fairness = 0;
					bestpick = rqpp;
					maxpri = rq->rq_maxrunpri;
				}
			}
			if ((rq = *++rqpp) == NULL) {
				rqpp = eng->e_rqlist;
				rq = *rqpp;
			}
		} while (rqpp != start);
#endif  /* UNIPROC */

		if (bestpick) {
			register list_t *dq;

			/*
			 * We've found something to run, remove the appropriate
			 * lwp from its corresponding run-queue and update
			 * the run-queue's summary information.
			 */
			rq = *bestpick;
			dq = &rq->rq_dispq[maxpri];
			new = (lwp_t *)dq->flink;
			rq->rq_srunlwps--;
			ASSERT(rq->rq_srunlwps >= 0);

			remque_null(new);
			if (EMPTYQUE(dq)) {
				/*
				 * Last lwp on this queue has been removed,
				 * turn off the summary bit corresponding to
				 * this queue.
				 */
				BITMASKN_CLR1(rq->rq_dqactmap, maxpri);
				if (rq->rq_srunlwps == 0) {
					rq->rq_maxrunpri = -1;
					rq->rq_dispcnt = 0;
				} else {
					/*
					 * Figure out what the next highest
					 * priority is.
					 */
					rq->rq_maxrunpri =
						BITMASKN_FLS(rq->rq_dqactmap,
							BITMASK_NWORDS(maxpri));
					ASSERT(rq->rq_maxrunpri != maxpri);
				}
			}
#ifndef UNIPROC

			if (fairness) {
				/*
				 * Rq->rq_dispcnt is an upper bound on the
				 * maximum number of contiguous reschedules
				 * from a run-queue (which happens to be the
				 * number of lwp on the queue the last time we
				 * exhausted the count).  When rq->rq_dispcnt
				 * becomes zero, we reset it and start our next
				 * dispatch on the first run-queue after the
				 * current run-queue.
				 */
				if (--rq->rq_dispcnt <= 0) {
					/*
					 * Exhausted the max count of
					 * dispatches, bump the pointer along.
					 */
					bestpick++;
					if (!*bestpick)
						bestpick = eng->e_rqlist;
					eng->e_lastpick = bestpick;
					rq->rq_dispcnt = rq->rq_srunlwps;
				} else
					eng->e_lastpick = bestpick;
			}
#endif  /* UNIPROC */

			/*
			 * Indicate that we've picked up a new lwp and responded to
			 * any nudge.
			 */
			eng->e_pri = maxpri;

			/*
			 * Important to set the status to SONPROC, otherwise,
			 * someone holding the lwp-mutex wouldn't be able to
			 * tell whether the lwp was on the run-queue or not
			 * should it need to move the lwp off of the run-queue
			 * (e.g. change binding, swapout, etc).
			 * Someone attempting to take an lwp off the run-queue
			 * must be holding the lwp's mutex, grab the run-queue
			 * and recheck the lwp's l_stat.  If it is racing with
			 * an engine wanting to schedule the lwp, it will
			 * see the l_stat of the lwp change from SRUN to
			 * SONPROC.  This indicates the scheduling engine has
			 * won the race and it must take appropriate action.
			 */
			new->l_stat = SONPROC;
			new->l_eng = l.eng;
			/*
			 * If the new context we picked up is marked as 
			 * being non-reentrant, mark the engine 
			 * appropriately.
			 */
			prmpt_state += new->l_notrt;
#ifdef _MPSTATS
			/*
			 * If the LWP we picked up was not the one that
			 * got us into this scheduling loop, clear the
			 * l_dispt field. We are really interested in the
			 * the latency for the highest priority context.
			 */
			if (l.eng->e_npri != new->l_pri)
				new->l_dispt = 0;
#endif /* _MPSTATS */

			dispclrpreempt();

			RUNQUE_UNLOCK();
			break;
		} else {
			eng->e_pri = -1;
			eng->e_npri = -1;

			RUNQUE_UNLOCK();

			/*
			 * Didn't find an lwp to run -- idle.
			 */
			if (lwp) {
				/*
				 * Get off the context of the current
				 * lwp.
				 */
				if (save(lwp) != 0)
					return;
				use_private(lwp, swtch, NULL); 
				/* NOT REACHED */
			}
			idle();
		}
	}


	if (new != lwp) {
		/*
		 * Resume releases both the run-queue and lwp->l_mutex if
		 * lwp is non-NULL.
		 */
		if (lwp) {
			/*
			 * Gaining a peer lock with lwp->l_mutex.  This
			 * forces us to consider lock ordering deadlock.
			 * If two processors both place an lwp on the
			 * run-queue, release the run-queue and call
			 * swtch and each attempt to switch to the other's
			 * lwp, this would result in a ordering deadlock,
			 * as each would be holding an lwp and attempting
			 * to gain the other lwp.
			 *
			 * An lwp will switch-out for two reasons:  preemption
			 * and blocking.  If the lwp is blocking, it will not
			 * place itself on the run-queue; also, since it will
			 * hold its l_mutex until it switches out, no-one
			 * else can place it back on the run-queue as this
			 * requires the l_mutex to be held.
			 *
			 * An lwp which is preempting places itself on the
			 * run-queue and calls qswtch without releasing the
			 * run-queue.  The run-queue lock will prevent other
			 * preempting lwp's from being entered on the run-queue,
			 * thus, it cannot choose a preempting lwp.
			 *
			 * Thus, with the above assumptions, lwp's which are
			 * switching-out serialize through the run-queue,
			 * meaning we won't find another lwp in the process
			 * of switching out on the run-queue and there's no
			 * chance for deadlock.  We still require a little
			 * slight of hand to prevent the debug code from
			 * detecting this attempt.
			 */
			LOCK_SH(&new->l_mutex, PLHI);
		} else
			/*
			 * No peer lock, can just lock directly.
			 */
			(void)LOCK(&new->l_mutex, PLHI);
		MET_PSWITCH();
#ifdef _MPSTATS
		/*
		 * Update preemption metrics.
		 */
		if (new->l_dispt != 0) {
			++l.prmpt_cs;
			GET_DELTA(&disp_latency, new->l_dispt);
			new->l_dispt = 0;
			l.prmpt_total += disp_latency;
			if (disp_latency > l.prmpt_max)
				l.prmpt_max = disp_latency;
		}
#endif /* _MPSTATS */
		if (lwp == NULL)
			/*
			 * Switching from the idle stack.
			 */
			ENABLE_PRMPT();
		resume(new, lwp);	/* resume releases all locks at spl0 */
	} else
		/*
		 * Found ourselves, no need to do a context switch.
		 * To provide consistent return semantics, we must
		 * return with the same conditions as the non-self case.
		 */
		UNLOCK(&lwp->l_mutex, PLBASE);
}

/*
 * qswtch(lwp_t *lwp)
 *
 * Give up the processor as in swtch.  Preferred interface for preemption.
 *
 * Calling/Exit State:
 *
 *	Must be called with lwp's mutex and the run-queue locked.
 *
 * Description:
 *
 *	Called from CL_preempt with "lwp"'s state and the run-queue locked.
 *	It returns at PLBASE with the both the current lwp's and the run-queue
 *	lock released.
 *
 *	This routine does the same thing as swtch, however, it is optimized
 *	for the involuntary preemption case.  It avoids an extra lock round-
 *	trip on the run-queue, avoids a number of checks against "lwp" being
 *	NULL (there's a current lwp or we wouldn't be switching) and doesn't
 *	concern itself (much) about idling and shutdown, as we'll always have
 *	the current lwp to run (except in a few, rare instances).
 *
 *	CL_preempt was required to hold the run-queue lock while adding the
 *	current lwp back onto the appropriate run-queue.  In order for
 *	CL_preempt to call swtch, it would need to release the run-queue lock;
 *	swtch would immediately reacquire the run-queue lock.  Also, swtch must
 *	deal with the case where there's no current lwp running on the
 *	processor, qswtch knows it has a current lwp and can avoid any such
 *	checks.
 *
 *	Most of the code is identical to swtch, see swtch for detailed
 *	commentary for each corresponding section of code.
 */
void
qswtch(register lwp_t *lwp)
{
	lwp_t *new;
	register runque_t **rqpp, *rq, **start;
	runque_t **bestpick;
	engine_t *eng;
	int fairness, maxpri;
	ulong_t	disp_latency = 0L;
	extern void qswtch_f(lwp_t *);

	ASSERT(LOCK_OWNED(&lwp->l_mutex));
	ASSERT(RUNQUE_OWNED());

#ifndef UNIPROC
	/*
	 * Before switching out, save affinity information
	 */
	lwp->l_lastran = lbolt;
#endif

	qswtch_f(lwp);

	eng = l.eng;

#ifndef UNIPROC
	if (eng->e_flags & E_SHUTDOWN) {
		/*
		 * Still hold onto the state lock of the lwp, preventing
		 * anyone else from picking up the lwp.  We need to
		 * get off the context of the lwp prior to calling swtch,
		 * otherwise an ordering deadlock could result.
		 */
		RUNQUE_UNLOCK();

		if (save(lwp) != 0)
			return;
		use_private(lwp, swtch, NULL);
		/* NO RETURN */
	}

	bestpick = NULL;
#endif  /* UNIPROC */
#ifdef UNIPROC
	if (global_rq->rq_srunlwps > 0) {  /* something to run */
		bestpick = &global_rq;
		maxpri = global_rq->rq_maxrunpri;
	} else {
		bestpick = NULL;
		maxpri = -1;
	}
#else
	maxpri = -1;
	rqpp = start = eng->e_lastpick;
	rq = *rqpp;

	do {
		if (rq->rq_maxrunpri >= maxpri) {
			if (rq->rq_maxrunpri == maxpri) {
				fairness = 1;
			} else {
				fairness = 0;
				bestpick = rqpp;
				maxpri = rq->rq_maxrunpri;
			}
		}
		if ((rq = *++rqpp) == NULL) {
			rqpp = eng->e_rqlist;
			rq = *rqpp;
		}
	} while (rqpp != start);
#endif  /* UNIPROC */

	if (bestpick) {
		list_t *dq;

		rq = *bestpick;
		dq = &rq->rq_dispq[maxpri];
		new = (lwp_t *)dq->flink;
		rq->rq_srunlwps--;
		ASSERT(rq->rq_srunlwps >= 0);

		remque_null(new);
		if (EMPTYQUE(dq)) {
			BITMASKN_CLR1(rq->rq_dqactmap, maxpri);
			if (rq->rq_srunlwps == 0) {
				rq->rq_maxrunpri = -1;
				rq->rq_dispcnt = 0;
			} else {
				rq->rq_maxrunpri =
					BITMASKN_FLS(rq->rq_dqactmap,
						     BITMASK_NWORDS(maxpri));
				ASSERT(rq->rq_maxrunpri != maxpri);
			}
		}

#ifndef UNIPROC
		if (fairness) {
			if (--rq->rq_dispcnt <= 0) {
				bestpick++;
				if (!*bestpick)
					bestpick = eng->e_rqlist;
				eng->e_lastpick = bestpick;
				rq->rq_dispcnt = rq->rq_srunlwps;
			} else
				eng->e_lastpick = bestpick;
		}
#endif  /* UNIPROC */

		eng->e_pri = maxpri;

		new->l_stat = SONPROC;
		new->l_eng = l.eng;
#ifdef _MPSTATS
		/*
		 * Check if we entered this scheduling loop
		 * because of the rti violation experienced by the
		 * new context we picked up.
		 */
		if (l.eng->e_npri != new->l_pri)
			new->l_dispt = 0;
#endif /* _MPSTATS */
		dispclrpreempt();
		RUNQUE_UNLOCK();
	} else {
		/*
		 * Nobody else around, release the run-queue and
		 * call switch to idle.
		 */
		RUNQUE_UNLOCK();

		/*
		 * If a context that was marked non-reentrant is switching out,
		 * re-enable preemption until it switches back in.
		 */
		prmpt_state -= lwp->l_notrt;

		if (save(lwp) != 0)
			return;
		use_private(lwp, swtch, NULL);
		/* NO RETURN */
	}

	if (new != lwp) {
		if (lwp) {
			LOCK_SH(&new->l_mutex, PLHI);
		} else
			(void)LOCK(&new->l_mutex, PLHI);
		MET_PSWITCH();
#ifdef _MPSTATS
		/*
		 * Update preemption metrics.
		 */
		if (new->l_dispt != 0) {
			++l.prmpt_cs;
			GET_DELTA(&disp_latency, new->l_dispt);
			new->l_dispt = 0;
			l.prmpt_total += disp_latency;
			if (disp_latency > l.prmpt_max)
				l.prmpt_max = disp_latency;
		}
#endif /* _MPSTATS */
		/*
		 * Adjust the preemption enable count, based on the
		 * difference between the non-reentrant counts on the new
		 * and old contexts.
		 */
		prmpt_state += new->l_notrt - lwp->l_notrt;

		resume(new, lwp);	/* resume releases all locks at spl0 */
	} else
		UNLOCK(&lwp->l_mutex, PLBASE);
}

/*
 * void swtch_seized(lwp_t *lwpp)
 *	This function is executed by the last LWP that checks into the 
 *	seize barrier.
 *
 * Calling/Exit State:
 *	No locks held on entry. The function does not return.
 */
STATIC
void swtch_seized(lwp_t *lwpp)
{
	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	PROC_SEIZED(lwpp);
	swtch(NULL);
	/* NOT REACHED */
}

/*
 * nudge(int pri, engine_t *eng);
 *
 * Tell "eng" to reschedule due to a preemption from an lwp at priority
 * "pri".
 *
 * Calling/Exit State:
 *
 *	Must be called with the run-queue locked.
 *
 * Description:
 *
 *	Sends an interrupt to engine "eng", telling it to reschedule.
 *	If the engine already has such a interrupt pending, just
 *	update the priority of the reschedule and return.  If we're
 *	nudging ourselves, just set the appropriate rescheduling variable.
 *	If an engine is idling, don't bother sending an interrupt, as
 *	it's polling memory in the idle loop and needs no interrupt.
 */
void
nudge(pri, eng)
	register char pri;		/* new nudged pri */
	register struct engine *eng;	/* processor to nudge */
{
	ASSERT(RUNQUE_OWNED());

	/*
	 * If the processor in question has been nudged,
	 * then all that's necessary is to update its npri.
	 */
	if (eng->e_npri != -1) {
		eng->e_npri = pri;
		return;
	}

	/*
	 * Record priority at which we nudged (npri == "nudge priority").
	 * If some future nudger notices that we've already been nudged
	 * at the same or better priority, we can save ourselves the
	 * trouble.
	 */
	eng->e_npri = pri;

	/*
	 * If the processor to be nudged is the current processor, then
	 * setting the l.runrun flag is sufficient. If the processor
	 * is idle, it will immediately find something on the run-queue,
	 * so the nudge is unnecessary.
	 */
	if (eng == l.eng)
		SETRUNRUN();
	else if (eng->e_pri != -1)
		sendsoft(eng, NUDGE);
}

/*
 * nudge_int(void)
 *
 * Called in response to a nudge interrupt and sets the reschedule flag.
 *
 * Calling/Exit State:
 *	none.
 */
void
nudge_int(void)
{
	SETRUNRUN();
}

/*
 * kpnudge(int pri, engine_t *eng);
 *
 * Same as nudge, except we set both the l.runrun and l.kprunrun reschedule
 * flags.
 *
 * Calling/Exit State:
 *
 *	Must be called with run-queue locked.
 *
 * Description:
 *
 *	Same tricks as nudge, except we set both runrun and kprunrun.
 */
void
kpnudge(pri, eng)
	register char pri;		/* new nudged pri */
	register struct engine *eng;	/* processor to nudge */
{
	ASSERT(RUNQUE_OWNED());

	eng->e_npri = pri;

	if (eng == l.eng) {
		SETKPRUNRUN();
	} else if (eng->e_pri != -1) {
		if (!(ENGINE_PLOCAL_PTR(eng - engine)->eventflags &
		       EVT_KPRUNRUN))
			sendsoft(eng, KPNUDGE);
	}
}

/*
 * kpnudge_int(void)
 *
 * Same as nudge_int, except sets both reschedule variables.
 *
 * Calling/Exit State:
 *
 *	none.
 */
void
kpnudge_int(void)
{
	SETKPRUNRUN();
}

/*
 * setbackrq(lwp_t *lwp)
 *
 * Put "lwp" on the end of its run-queue.
 *
 * Calling/Exit State:
 *
 *	Must be called with the lwp-state and run-queue locked and only
 *	if "lwp" is truely runnable (i.e. not unloaded or seized).
 *
 * Description:
 *
 *	Put the specified lwp on the back of the dispatcher-queue
 *	corresponding to its current run-queue and priority.Update
 *	activity timestamp for the encompassing process, but without
 *	additional locking, due to benign nature of the race.
 */
void
setbackrq(lwp_t *lwp)
{
	register runque_t *rq = lwp->l_rq;
#ifdef DISP_DEBUG
	register lwp_t		*rlwp;
#endif /* DISP_DEBUG */
	register int pri;
	register list_t *dispq, *dq;

	ASSERT(RUNQUE_OWNED());
	ASSERT(rq != NULL);
	ASSERT(lwp->l_pri < nglobpris);
	dispq = rq->rq_dispq;
#ifdef DISP_DEBUG
	for (dq = &dispq[0] ; dq < &dispq[nglobpris] ; dq++) {
		for (rlwp = (lwp_t *)dq->flink; rlwp != (lwp_t *)dq;
		     rlwp = (lwp_t *)rlwp->l_flink) {
			if (rlwp == lwp)
				/*
				 *+ The kernel attempted to place an lwp
				 *+ on a queue (the run-queue) when the
				 *+ lwp was already on that queue.
				 */
				cmn_err(CE_PANIC, "setbackrq - lwp on q.");
		}
	}
#endif /* DISP_DEBUG */
	/*
	 *+ The kernel attempted to make an lwp eligible for execution
	 *+ when the state of the lwp indicates it is not capable of
	 *+ being executed.
	 */
	ASSERT((lwp->l_stat == SRUN || lwp->l_stat == SONPROC) &&
	       LWP_LOADED(lwp) && !LWP_SEIZED(lwp));

	pri = lwp->l_pri;
	dq = &dispq[pri];

	insque(lwp, dq->rlink);

	rq->rq_srunlwps++;
	if (dq->flink == dq->rlink) {
		BITMASKN_SET1(rq->rq_dqactmap, pri);
		if (pri > rq->rq_maxrunpri)
			rq->rq_maxrunpri = pri;
	}

	/*
	 * Timestamp the process as having been active at this instant.
	 * No process locking necessary because the proc structure cannot 
	 * disappear and the race between different LWPs for setting the 
	 * timestamp is benign. Assumed atomicity for 32 bit load/stores.
	 */
	lwp->l_procp->p_active = lbolt;
}

/*
 * setfrontrq(lwp_t *lwp)
 *
 * Put "lwp" on the front of its run-queue.
 *
 * Calling/Exit State:
 *
 *	Must be called with the lwp-state and run-queue locked and only
 *	if "lwp" is truely runnable (i.e. not unloaded or seized).
 *
 * Description:
 *	Put the specified lwp on the front of the dispatcher-queue
 *	corresponding to its current run-queue and priority.Update
 *	activity timestamp for the encompassing process, but without
 *	additional locking, due to benign nature of the race.
 */
void
setfrontrq(lwp_t *lwp)
{
	register runque_t *rq = lwp->l_rq;
#ifdef DISP_DEBUG
	register lwp_t		*rlwp;
#endif /* DISP_DEBUG */
	register int pri;
	register list_t *dispq, *dq;

	ASSERT(RUNQUE_OWNED());
	ASSERT(rq != NULL);
	ASSERT(lwp->l_pri < nglobpris);
	dispq = rq->rq_dispq;
#ifdef DISP_DEBUG
	for (dq = &dispq[0] ; dq < &dispq[nglobpris] ; dq++) {
		for (rlwp = (lwp_t *)dq->flink; rlwp != (lwp_t *)dq;
		     rlwp = (lwp_t *)rlwp->l_flink) {
			if (rlwp == lwp)
				/*
				 *+ The kernel attempted to place an lwp
				 *+ on a queue (the run-queue) when the
				 *+ lwp was already on that queue.
				 */
				cmn_err(CE_PANIC, "setbackrq - lwp on q.");
		}
	}
#endif	/* DISP_DEBUG */

	/*
	 *+ The kernel attempted to make an lwp eligible for execution
	 *+ when the state of the lwp indicates it is not capable of
	 *+ being executed.
	 */
	ASSERT((lwp->l_stat == SRUN || lwp->l_stat == SONPROC) &&
	       LWP_LOADED(lwp) && !LWP_SEIZED(lwp));

	pri = lwp->l_pri;
	dq = &dispq[pri];

	insque(lwp, dq->flink);

	rq->rq_srunlwps++;
	if (dq->flink == dq->rlink) {
		BITMASKN_SET1(rq->rq_dqactmap, pri);
		if (pri > rq->rq_maxrunpri)
			rq->rq_maxrunpri = pri;
	}

	/*
	 * Timestamp the process as having been active at this instant.
	 * No process locking necessary because the proc structure cannot 
	 * disappear and the race between different LWPs for setting the 
	 * timestamp is benign. Assumed atomicity for 32 bit load/stores.
	 */
	lwp->l_procp->p_active = lbolt;
}

/*
 * dispdeq(lwp_t *lwp)
 *
 * Remove an lwp from its run-queue.
 *
 * Calling/Exit State:
 *
 *	Remove a lwp from its run-queue if it is on it.
 *	It is not an error if it is not found but we return whether
 *	or not it was found in case the caller wants to check.
 *	It must be called with the run-queue lock held.
 */
boolean_t
dispdeq(lwp_t *lwp)
{
	register runque_t *rq = lwp->l_rq;
	register list_t	*dq;
#ifdef DISP_DEBUG
	register lwp_t *rlwp;
	register list_t *dispq;
	int found = 0;
#endif /* DISP_DEBUG */
	register int		pri;

	/*
	 *+ The kernel encountered an lwp with basic scheduling
	 *+ variables corrupted.
	 */
	ASSERT(LOCK_OWNED(&lwp->l_mutex));
	ASSERT(RUNQUE_OWNED());
	ASSERT(rq != 0);

#ifdef DISP_DEBUG
	/*
	 * Independently determine if the lwp is on or off its
	 * associated run-queue.  We'll see later if this accurately
	 * reflects the l_flink/l_rlink pointers.
	 */
	dispq = rq->rq_dispq;
	for(dq = dispq ; dq < &dispq[nglobpris]; dq++) {
		for (rlwp = (lwp_t *)dq->flink; rlwp != (lwp_t *)dq;
		     rlwp = (lwp_t *)rlwp->l_flink) {
			if (lwp == rlwp)
				found = 1;
		}
	}
#endif /* DISP_DEBUG */

	if (lwp->l_flink == NULL) {
#ifdef DISP_DEBUG
		if (found)
			/*
			 *+ An lwp was found on run-queue whose state
			 *+ indicated it was not on the run-queue.
			 */
			cmn_err(CE_PANIC, "dispdeq - lwp %x on q", lwp);
#endif /* DISP_DEBUG */
		return (B_FALSE);
	}
#ifdef DISP_DEBUG
	if (!found)
		/*
		 *+ An lwp whose state indicated it was on the run-queue
		 *+ was not found on the run-queue.
		 */
		cmn_err(CE_PANIC, "dispdeq - lwp %x not on q", lwp);
#endif /* DISP_DEBUG */
	pri = lwp->l_pri;
	dq = &rq->rq_dispq[pri];

	/*
	 *+ The dispatcher queue associated with a particular lwp
	 *+ was found to be empty, when other state indicates at
	 *+ one lwp should have been resident.
	 */
	ASSERT(!EMPTYQUE(dq));		/* can't be empty */

	if (dq->flink == dq->rlink && dq->flink == (list_t *)lwp) {
		register uint_t *bm = rq->rq_dqactmap;

		/*
		 * This is the last lwp on the list.
		 */
		BITMASKN_CLR1(bm, pri);
		if (rq->rq_srunlwps == 0) {
			rq->rq_maxrunpri = -1;
		} else if (pri == rq->rq_maxrunpri) {
			rq->rq_maxrunpri =
				BITMASKN_FLS(bm, BITMASK_NWORDS(pri));
		}
	}

	rq->rq_srunlwps--;

	remque_null(lwp);

	return(B_TRUE);
}

/*
 * int getcid(char *clname, id_t *cidp)
 *
 * Get the ASCII name associated with the numeric scheduling class.
 *
 * Calling/Exit State:
 *
 *	none
 */
int
getcid(clname, cidp)
char	*clname;
id_t	*cidp;
{
	register class_t	*clp;
	char localclname[PC_CLNMSZ];

	if (strcmp(clname, "RT") == 0)
		strcpy(localclname, "FP");
	else
		strcpy(localclname, clname);

	for (clp = &class[0]; clp < &class[nclass]; clp++) {
		if (strcmp(clp->cl_name, localclname) == 0) {
			*cidp = clp - &class[0];
			return(0);
		}
	}
	return(EINVAL);
}

/*
 * void parmsget(lwp_t *lwpp, pcparms_t *parmsp)
 *
 *	Get the scheduling parameters of the lwp.
 *
 * Calling/Exit State:
 *
 *	Must be called with the per-lwp mutex held to keep the
 *	parameters from changing out under the operation.
 *
 * Description:
 *
 *	Get the scheduling parameters of the lwp pointed to by
 *	lwpp into the buffer pointed to by parmsp.
 */
void
parmsget(lwp_t *lwpp, pcparms_t *parmsp)
{
	parmsp->pc_cid = lwpp->l_cid;
	CL_PARMSGET(lwpp, lwpp->l_cllwpp, parmsp->pc_clparms);
}


/*
 * int parmsin(pcparms_t *parmsp)
 *
 *	Validate scheduling class id and call scheduling class to copy-in
 *	and further validate parameters for a PC_SETPARMS.
 *
 * Calling/Exit State:
 *
 *	No locking required, as this only validates data to be used by
 *	a subsequent parmsset.
 *
 * Description:
 *
 *	Check the validity of the scheduling parameters in the buffer
 *	pointed to by parmsp.  We make no determination if the caller is
 *	allowed to actually apply these permissions, as these checks
 *	get made on a case-by-case basis by parmsset.
 *
 *	Note that the format of the parameters may be changed by
 *	CL_PARMSIN.
 */
int
parmsin(register pcparms_t *parmsp)
{
	if (parmsp->pc_cid >= nclass || parmsp->pc_cid < 1)
		return(EINVAL);

	/*
	 * Call the class specific routine to validate class
	 * specific parameters.  Note that the data pointed to
	 * by targclpp is only meaningful to the class specific
	 * function if the target process belongs to the class of
	 * the function.
	 */
	return(CL_PARMSIN(&class[parmsp->pc_cid], parmsp->pc_clparms));
}

/*
 * int parmsout(pcparms_t *parmsp)
 *
 *	Validate/map parameters prior to copyout.
 *
 * Calling/Exit State:
 *
 * 	No locks required, as it simply validates/maps parameters sitting
 *	in an internal kernel buffer.
 *
 * Description:
 *
 *	Call the class specific code to do the required processing
 *	and permissions checks before the scheduling parameters
 *	are copied out to the user.
 *	Note that the format of the parameters may be changed by the
 *	class specific code.
 */
int
parmsout(register pcparms_t *parmsp)
{
	return(CL_PARMSOUT(&class[parmsp->pc_cid], parmsp->pc_clparms));
}

/*
 * int parmsset(void *classdata, pcparms_t *parmsp, lwp_t *reqlwpp, lwp_t *targlwpp)
 *
 *	Do processing to change the parameters of "targlwpp".
 *
 * Calling/Exit State:
 *
 *	Called with the per-lwp and per-process mutex of "targlwpp" held.
 *	This is necessary to prevent others from examining/changing the
 *	parameters, as well as keeping the "targlwpp"'s process credentials
 *	stable.
 *
 * Description:
 *
 *	We effect a scheduling class change against "targlwpp".  This change
 *	could be a simple parameter change which can be handled entirely
 *	within the scheduling class or could be a scheduling class change.
 *
 *	We'll use the following terminology when discussing a class change:
 *	The scheduling class and class specific parameters associate with
 *	the lwp now are the "current" scheduling class and parmeters.  If
 *	an lwp has an operation queued, the queued operation has a
 *	"queued" scheduling class and parameters.  The operation we're
 *	trying to apply at the moment has a "new" scheduling class and
 *	set of parameters associated with it.  In summary:  "current" is
 *	the set of things currently possessed by the lwp; the pending
 *	set of things is the "queued" set; the set we're trying to apply
 *	is the "new" set of things.
 *
 *	We call CL_CHANGEPARMS to inform the current scheduling class of
 *	the parameter change.  CL_CHANGEPARMS analyzes the changes and
 *	returns zero if the parameters can be applied immediately.
 *
 *	If "targlwpp" has existing queued parameters, we first call
 *	CL_CANCELCHANGE to cancel or combine the existing queued-parameter
 *	change.
 *
 *	When the scheduling class of "targlwpp" is being called, the
 *	CL_CHANGEPARMS for the current scheduling class must return non-zero.
 *	This will prompt us to
 *	check for the scheduling class change and call the CL_CHANGEPARMS
 *	routine of the new scheduling class.
 *
 *	Before performing any of these operations, we first determine
 *	that we have permissions to change the scheduling class parameters
 *	of "targlwpp".
 */
int
parmsset(void *classdata, pcparms_t *parmsp, lwp_t *reqlwpp, lwp_t *targlwpp)
{
	register int	error;
	int		changedone;
	int		combined;
	int		classchg;
	id_t		cid;
	cred_t		*reqcredp;
	cred_t		*targcredp;
	qpcparms_t	*qparmsp;

	if (reqlwpp != NULL) {
		reqcredp = reqlwpp->l_cred;
		targcredp = targlwpp->l_procp->p_cred;

		/*
		 * Check MAC access.  In order to set the parameters of
		 * a target process, the requesting process must be at
		 * the same level or have P_MACWRITE.
		 */
		if (MAC_ACCESS(MACEQUAL, reqcredp->cr_lid, 
		    targcredp->cr_lid) && pm_denied(reqcredp, P_MACWRITE)) {

			/*
			 * If the target process is at a higher level than the
			 * requestor, return ESRCH since the target should not
			 * be visible to the requestor; otherwise return EPERM.
			 */
			if (MAC_ACCESS(MACDOM, reqcredp->cr_lid,
			    targcredp->cr_lid))
				return(ESRCH);
			else
				return(EPERM);
		}

		/*
		 * See if we have permissions to apply the new
		 * change.
		 */
		if (!hasprocperm(targcredp, reqcredp))
			return(EPERM);
	} else {
		reqcredp = NULL;
	}

	classchg = B_FALSE;

	if ((qparmsp = targlwpp->l_qparmsp) != NULL) {
		/*
		 * There's a queued change.  If its has same
		 * scheduling class as the new change, investigate combining
		 * the new change with the queued change.
		 */
		cid = qparmsp->qpc_pcparms.pc_cid;
		ASSERT(cid != 0);

		if (cid == parmsp->pc_cid) {
			error = CL_CANCELCHANGE(&class[cid], targlwpp,
						CANCELCHANGE_TRYCOMBINE,
						qparmsp, parmsp, classdata,
						reqcredp, &combined);
			if (error) {
				/*
				 * The new change was refused.
				 */
				return(error);
			}

			if (combined) {
				/*
				 * The two changes were combined, thus
				 * we're done.
				 */
				return(0);
			}

			classchg = qparmsp->qpc_classchg;
		} else {
			/*
			 * The new and queued changes don't have the
			 * same scheduling class.
			 * Need to see if the new change is OK before
			 * cancelling the queued change.
			 */
			error = CL_CHANGEPARMS(&class[parmsp->pc_cid], targlwpp,
					       CHANGEPARMS_CHECKONLY,
					       cid, parmsp, classdata, reqcredp,
					       NULL, &changedone);

			if (error) {
				/*
				 * The new change has been refused.
				 */
				return(error);
			}

			ASSERT(changedone == 0);

			/*
			 * Cancel the queued change.  There's no chance
			 * of combination here.
			 */
			error = CL_CANCELCHANGE(&class[cid], targlwpp,
						CANCELCHANGE_MUSTCANCEL,
						qparmsp, parmsp, classdata,
						reqcredp, &combined);

			/*
			 * The old change was cancelled.  The new
			 * change's class may be the same as the existing
			 * class.  We still need to exit the current
			 * class and (re)enter the new change's class
			 * to preserve the semantics of the scheduling
			 * class change.  E.g. if we leave the TS class,
			 * enter RT and re-entry TS, we'll see default
			 * parameters on re-entry to TS.  Thus, if we have
			 * a new TS cancel a queued RT, we still want the
			 * new TS to re-enter the scheduling class, even
			 * though the current class is TS.
			 */
			classchg = qparmsp->qpc_classchg;

			ASSERT(error == 0 && combined == 0);
		}

		/*
		 * Enqueue the new change.
		 */
		qparmsp->qpc_pcparms = *parmsp;
		qparmsp->qpc_argp = classdata;
		qparmsp->qpc_classchg = classchg;
		ASSERT(parmsp->pc_cid != 0);
		return(0);
	}

	/*
	 * No queued change.
	 *
	 * Inform the old scheduling class of the parameter change,
	 * see if the old scheduling class can accommodate the change.
	 * The old scheduling class must not process a scheduling class
	 * change.  It must only return data to enqueue the change and
	 * return zero.  The data won't be touched until the new
	 * scheduling class approves the change.  In this manner,
	 * the scheduling class the lwp is in is responsible for allocating
	 * such data, but must not make assumptions about its usage.
	 */
	error = CL_CHANGEPARMS(&class[targlwpp->l_cid], targlwpp,
			       CHANGEPARMS_MAYAPPLY, targlwpp->l_cid,
			       parmsp, classdata, reqcredp, &qparmsp,
			       &changedone);
	if (error != 0) {
		/*
		 * The new change was not a class change and has been
		 * refused by the scheduling class.
		 */
		return(error);
	}

	if (changedone) {
		/*
		 * Scheduling class has effected the class change, no need
		 * for further processing.
		 */
		ASSERT(targlwpp->l_cid == parmsp->pc_cid);
		return(0);
	}
	ASSERT(qparmsp != NULL);

	/*
	 * The current scheduling class of the lwp cannot immediately
	 * process the scheduling class change.  It has provided space to
	 * enqueue the parameter change.
	 */
	cid = parmsp->pc_cid;
	if (cid != targlwpp->l_cid) {
		/*
		 * This is a scheduling class change, we must call the
		 * new scheduling class so it does any necessary processing
		 * in perparation for the new lwp and also enforces any
		 * per-scheduling class credential checks.
		 */
		error = CL_CHANGEPARMS(&class[cid], targlwpp,
				       CHANGEPARMS_CHECKONLY,
				       targlwpp->l_cid, parmsp, classdata,
				       reqcredp, NULL, &changedone);

		if (error) {
			/*
			 * The new class change has been refused.  Thus,
			 * there's no net effect on the lwp.
			 */
			return(error);
		}

		if (changedone) {
			/*
			 *+ The kernel detected a scheduling class which
			 *+ responded incorrectly to a scheduling class
			 *+ change.  Corrective action:  examine all configured
			 *+ scheduling classes for inappropriate processing
			 *+ of scheduling class changes.  If none are found
			 *+ then this is a base kernel problem and no further
			 *+ corrective action on part of an administrator is
			 *+ possible.
			 */
			cmn_err(CE_PANIC, "Changeparms did not defer "
					  "scheduling class change");
		}
		classchg = B_TRUE;
	}

	/*
	 * There is now no request pending against the lwp and this is
	 * either a scheduling class change or a parameter change which
	 * could not be processed synchronously by the scheduling class.
	 * We enqueue the new change.  The new lwp will effect the change
	 * sometime in the future.
	 */
	qparmsp->qpc_pcparms = *parmsp;
	qparmsp->qpc_argp = classdata;
	qparmsp->qpc_classchg = classchg;

	targlwpp->l_qparmsp = qparmsp;
	targlwpp->l_trapevf |= EVF_L_SCHEDPARM;

	ASSERT(parmsp->pc_cid != 0);

	return(0);
}

STATIC void classchg(lwp_t *, int, long *, void *);

/*
 * void parmsret(void)
 *
 *	Process a parameter change on our own behalf.
 *
 * Calling/Exit State:
 *
 *	Must be called by the lwp on its own behalf.  Per-lwp mutex
 *	must not be held.  Acquires and releases per-lwp mutex.
 *
 * Description:
 *
 *	We're called when our EVF_L_SCHEDPARM flag is set.  This means
 *	we have a scheduling parameter change (usually a scheduling class
 *	change) pending.  We capture and reset the queued parameter pointer
 *	and determine the nature of the change.  If this is a scheduling
 *	class change, we enter the new class and then leave the old class.
 *	Otherwise, we simply call CL_PARMSSET to effect the other type
 *	of change.
 */
void
parmsret(void)
{
	register lwp_t *lwp;
	pcparms_t *parmsp;
	qpcparms_t *qparmsp;
	void *perlwpdata;
	void *oldlwpdata;
	class_t *classp;
	int cid;
	int oldcid;
	pl_t pl;

	lwp = u.u_lwpp;

	ASSERT(KS_HOLD0LOCKS());
again:
	pl = LOCK(&lwp->l_mutex, PLHI);
	ASSERT((lwp->l_trapevf & EVF_L_SCHEDPARM) != 0);

	/*
	 * Get the parameter change.
	 */
	qparmsp = lwp->l_qparmsp;
	ASSERT(qparmsp != NULL);

	/*
	 * At this point, we've officially begun processing the
	 * parameter change.  If a new change comes along, it will
	 * be posted and processed after we've finished this one.
	 */
	parmsp = &qparmsp->qpc_pcparms;

	if (qparmsp->qpc_classchg) {
		cid = qparmsp->qpc_pcparms.pc_cid;
		classp = &class[cid];
		UNLOCK(&lwp->l_mutex, pl);

		perlwpdata = CL_ALLOCATE(classp, lwp);

		pl = LOCK(&lwp->l_mutex, PLHI);
		qparmsp = lwp->l_qparmsp;
		parmsp = &qparmsp->qpc_pcparms;

		if (!qparmsp->qpc_classchg ||
		    cid != qparmsp->qpc_pcparms.pc_cid) {
			/*
			 * While we were allocating, someone came
			 * along and modified the parameters to be
			 * changed.  Hence, the storage we allocated may
			 * be for the wrong scheduling class.  Release it
			 * and try again.
			 */
			UNLOCK(&lwp->l_mutex, pl);
			CL_DEALLOCATE(classp, perlwpdata);
			goto again;
		}

		oldlwpdata = lwp->l_cllwpp;
		oldcid = lwp->l_cid;

		/*
		 * We now have the storage in hand for the scheduling
		 * class change.  Leave our old class and enter our new
		 * scheduling class.
		 */
		CL_EXITCLASS(lwp, lwp->l_cllwpp);

		CL_ENTERCLASS(classp, parmsp->pc_clparms, lwp,
			      &lwp->l_stat, &lwp->l_pri, &lwp->l_flag,
			      &lwp->l_cred, &perlwpdata, qparmsp->qpc_argp);

		lwp->l_cid = cid;
		lwp->l_clfuncs = classp->cl_funcs;
		lwp->l_cllwpp = perlwpdata;

		lwp->l_trapevf &= ~EVF_L_SCHEDPARM;
		lwp->l_qparmsp = NULL;

		UNLOCK(&lwp->l_mutex, pl);

		/*
		 * Cannot deallocate the data while holding the
		 * l_mutex.
		 */
		CL_DEALLOCATE(&class[oldcid], oldlwpdata);
	} else {
		/*
		 * Not changing class, just change the parameters.
		 * Note this will always succeed, as CL_CHANGEPARMS
		 * has already approved the change.
		 */
		CL_PARMSSET(lwp, lwp->l_cllwpp, parmsp->pc_clparms,
			    qparmsp->qpc_argp);
		lwp->l_trapevf &= ~EVF_L_SCHEDPARM;
		lwp->l_qparmsp = NULL;

		UNLOCK(&lwp->l_mutex, pl);
	}

	return;
}

/*
 * void parmsp1init(void)
 *
 *	Called by the init process to move itself to its chosen
 *	scheduling class.
 *
 * Calling/Exit State:
 *
 *	Called by the init process on its own behalf.  Since multiple
 *	processes are on the system now, we cannot make the single-threaded
 *	assumptions we're used to making at init time.
 *
 * Description:
 *
 *	We're init.  We move ourselves to the appropriate scheduling
 *	class.
 */
void
parmsp1init(void)
{
	id_t cid;
	extern char initclass[];	/* tunable class name for proc_init */

	/*
	 * Map the scheduling identifier associated with init.
	 */
	if (getcid(initclass, &cid) || cid <= 0) {
		/*
		 *+ The scheduling class configured for init does not
		 *+ match any know scheduling class on the system.
		 *+ Corrective action:  examine and fix the configured
		 *+ scheduling class for init or be sure the scheduling class
		 *+ which init is to belong to has been configured.
		 */
		cmn_err(CE_PANIC, "Illegal or unconfigured class (%s) "
				  "specified for init process.\n"
				  "Change INITCLASS configuration parameter.",
				  initclass);
	}

	/*
	 * Switch to the new scheduling class.
	 */
	classchg(u.u_lwpp, cid, NULL, NULL);

	return;
}

/*
 * void classchg(lwp_t *lwp, int cid, long *clparmsp, void *argp)
 *
 *	Apply a parameter change on our own behalf.
 *
 * Calling/Exit State:
 *
 *	Must not be holding the per-lwp mutex when called.
 *
 * Description:
 *
 *	We remove ourselves from our current scheduling class and
 *	add ourselves to the scheduling class specified by cid.  It
 *	under certain circumstances, cid may be equal to our
 *	current scheduling class.
 */
STATIC void
classchg(lwp_t *lwp, int cid, long *clparmsp, void *argp)
{
	void *perlwpdata;
	void *oldlwpdata;
	class_t *classp;
	int oldcid;
	pl_t pl;

	ASSERT(cid < nclass && cid > 0);

	/*
	 * We're changing our scheduling class.  First allocate
	 * storage.  We do this while not holding locks, as we
	 * need to sleep.
	 */
	classp = &class[cid];
	perlwpdata = CL_ALLOCATE(classp, lwp);

	/*
	 * We now gain our per-lwp mutex.  This is necessary, as
	 * we're not going to be in a scheduling class for a short
	 * while and must prevent others or the clock interrupt
	 * handler from accessing our per-lwp scheduling class data
	 * while this is so.
	 */
	pl = LOCK(&lwp->l_mutex, PLHI);

	oldcid = lwp->l_cid;
	oldlwpdata = lwp->l_cllwpp;

	/*
	 * Remove ourselves from our old scheduling class.
	 */
	CL_EXITCLASS(lwp, lwp->l_cllwpp);

	/*
	 * Enter our new scheduling class.
	 */
	CL_ENTERCLASS(classp, clparmsp, lwp,
		      &lwp->l_stat, &lwp->l_pri, &lwp->l_flag,
		      &lwp->l_cred, &perlwpdata, argp);

	/*
	 * Update our per-lwp pointers to reflect the new
	 * scheduling class.
	 */
	lwp->l_cid = cid;
	lwp->l_clfuncs = classp->cl_funcs;
	lwp->l_cllwpp = perlwpdata;

	UNLOCK(&lwp->l_mutex, pl);

	/*
	 * Cannot deallocate while holding l_mutex.
	 */
	CL_DEALLOCATE(&class[oldcid], oldlwpdata);

	return;
}

/*
 * void parmsprop(lwp_t *child_lwp)
 *
 *	Called to propagate any pending parameter changes from the
 *	parent (u.u_lwpp) to "child_lwp".
 *
 * Calling/Exit State:
 *
 *	Called by the parent in a lwp-create/fork when there are pending
 *	parameter changes against the parent.  Locks on both the parent and
 *	child lwp's are held.  This prevents anyone from changing the
 *	scheduling class of both the parent and the child.  Called after the
 *	parent has called CL_FORK on behalf of the child.
 *
 * Description:
 *
 *	We call CL_CHANGEPARMS to propagate the new parameter change to
 *	newly created lwp after doing the appropriate mutexing.  We expect
 *	the scheduling class to refuse to directly apply the change to the
 *	child and to allocate queue space.
 */
void
parmsprop(lwp_t *child_lwp)
{
	lwp_t *parent_lwp = u.u_lwpp;
	int error;
	qpcparms_t *qparmsp;
	int cid;

	ASSERT(LOCK_OWNED(&child_lwp->l_mutex));
	ASSERT(LOCK_OWNED(&parent_lwp->l_mutex));
	ASSERT(parent_lwp->l_trapevf & EVF_L_SCHEDPARM);
	ASSERT(parent_lwp->l_qparmsp != NULL);

	/*
	 * We allow the scheduling class to copy and return the parameters.
	 */
	qparmsp = parent_lwp->l_qparmsp;
	cid = child_lwp->l_cid;
	error = CL_CHANGEPARMS(&class[cid], child_lwp, CHANGEPARMS_FORK,
			       cid, NULL, NULL, NULL, &qparmsp, NULL);

	ASSERT(qparmsp != NULL);
	ASSERT(error == 0);

	child_lwp->l_qparmsp = qparmsp;
	child_lwp->l_trapevf |= EVF_L_SCHEDPARM;
}


/*
 * void parmsexit(void)
 *
 *	Called to cancel a pending parameter change as result of
 *	an lwp exit.
 *
 * Calling/Exit State:
 *
 *	Called after the after the lwp has become invisible on its
 *	own behalf.  This is necessary to prevent subsequent priocntl
 *	operations from enqueuing parameters against us after we've tried
 *	to shed them all.
 *
 * Description:
 *
 *	Calls CL_CANCELCHANGE to cancel the request.  The new parameter and
 *	class data are passed in as NULL.  This indicates to the scheduling
 *	class it cannot refuse the cancel, nor combine with another.
 */
void
parmsexit(void)
{
	lwp_t *lwp = u.u_lwpp;
	qpcparms_t *qparms;
	int error;

	ASSERT(LOCK_OWNED(&lwp->l_mutex));
	ASSERT(lwp->l_trapevf & EVF_L_SCHEDPARM);

	qparms = (qpcparms_t *)lwp->l_qparmsp;
	ASSERT(qparms != NULL);

	error = CL_CANCELCHANGE(&class[qparms->qpc_pcparms.pc_cid], lwp,
				CANCELCHANGE_EXIT, qparms, NULL, NULL, NULL,
				NULL);

	ASSERT(error == 0);
}

/*
 * int parmsstart(struct pcparms *parms, void **classdatap)
 *
 *	Notify scheduling class of start of aggragate operation.
 *
 * Calling/Exit State:
 *
 *	Called by priocntlsys immediately before performing an operation.
 *	Expects to be called with no locks held, as the scheduling class
 *	may sleep.
 *
 * Description:
 *
 *	Calling into the scheduling class which is the possible target
 *	of scheduling class movement.  Certain scheduling classes are
 *	sensitive to such movement and use it to aggragate the lwp's
 *	affected by priocntlsys.  "*classdatap" is set to scheduling class
 *	specific information/storage allowing each lwp in the operation
 *	to be identified with the aggragation.
 */
int
parmsstart(struct pcparms *parms, void **classdatap)
{
	ASSERT(KS_HOLD0LOCKS());
	return(CL_CHANGESTART(&class[parms->pc_cid], parms, classdatap));
}

/*
 * void parmsend(struct pcparms *parms, void *classdata)
 *
 *	Announce to the scheduling class the aggragate opertion is completed.
 *
 * Calling/Exit State:
 *
 *	Called while not holding locks, as the scheduling class may sleep.
 *
 * Description:
 *
 *	Calls into the scheduling class to inform it the aggragate operation
 *	associated with "classdata" is not complete.
 */
void
parmsend(struct pcparms *parms, void *classdata)
{
	ASSERT(KS_HOLD0LOCKS());

	CL_CHANGEEND(&class[parms->pc_cid], parms, classdata);
}

/*
 * void check_preemption(void)
 *	Check to see if a preemption is to be effected.
 *
 * Calling/Exit State:
 *	No spin locks can be held on entry and none will be held on return.
 */
void
check_preemption(void)
{
	lwp_t	*lwpp = u.u_lwpp;

	if (!prmpt_enable)
		return;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(prmpt_count == 0);
	ASSERT(getpl() == PLBASE);
	ASSERT(!servicing_interrupt());

	if (check_preemption_f())
		return;
	/*
	 * If the context is in the process of being preempted,
	 * return.
	 */
	if (lwpp->l_beingpt)
		return;
	lwpp->l_beingpt = B_TRUE;
#ifdef _MPSTATS
	++l.prmpt_kern;
#endif
	UPREEMPT();
	lwpp->l_beingpt = B_FALSE;
}

/*
 * void
 * block_preemption(void)
 *	Disable preemption for a code sequence which might block
 *
 * Calling/Exit State:
 *	No locks required or acquired.
 *	Must be called in process context.
 */
void
block_preemption(void)
{
	DISABLE_PRMPT();
	u.u_lwpp->l_notrt++;
}


/*
 * void
 * unblock_preemption(void)
 *	Re-enable preemption blocked by block_preemption()
 *
 * Calling/Exit State:
 *	No locks required or acquired.
 *	Must be called in process context.
 */
void
unblock_preemption(void)
{
	ASSERT(u.u_lwpp->l_notrt != 0);
	u.u_lwpp->l_notrt--;
	ENABLE_PRMPT();
}


#ifdef _MPSTATS

/*
 * void print_dispmetrics(int cpu)
 *	Prints all the context switch metrics. If a valid processor Id is 
 *	specified, the metrics for the specified processor are printed. If a 
 *	negative processor ID is specified, then the metrics for all the 
 *	processors are printed.
 *
 * Calling/Exit State:
 *	None.
 */

void
print_dispmetrics(int cpu)
{
	int	i;
	struct plocal	*plp;
	
	if (cpu < 0) {
		for (i = Nengine; i-- != 0;) {
			cmn_err(CE_CONT, "\n Engine #: %d\n", i);
			plp = ENGINE_PLOCAL_PTR(i);
			/*
			 * Print info on preemption stats.
			 */
			cmn_err(CE_CONT, "\n Max Latency: %d\n", 
				plp->prmpt_max);
			cmn_err(CE_CONT, "\n Total # of preemptive CS: %d\n",
				plp->prmpt_cs);
			cmn_err(CE_CONT, 
				"\n Total # of kernel preemptions: %d\n",
				plp->prmpt_kern);
			cmn_err(CE_CONT,
				"\n Total # of user preemptions: %d\n",
				plp->prmpt_user);
			if (plp->prmpt_cs != 0) {
				cmn_err(CE_CONT, "\n Average dispatch latency: %d\n",
				plp->prmpt_total/plp->prmpt_cs);
			}
			else 
				cmn_err(CE_CONT, "WARNING: 0 cs\n");
		}
		return;
	}
	if (cpu >= Nengine) {
		cmn_err(CE_CONT, "Invalid processor ID\n");
		return;
	}
	plp = ENGINE_PLOCAL_PTR(cpu);
	cmn_err(CE_CONT, "\n Max Latency: %d\n",
		plp->prmpt_max);
	cmn_err(CE_CONT, "\n Total # of preemptive CS: %d\n",
		plp->prmpt_cs);
	cmn_err(CE_CONT,
		"\n Total # of kernel preemptions: %d\n",
		plp->prmpt_kern);
	cmn_err(CE_CONT,
		"\n Total # of user preemptions: %d\n",
		plp->prmpt_user);
	if (plp->prmpt_cs != 0) {
		cmn_err(CE_CONT, "\n Average dispatch latency: %d\n",
			plp->prmpt_total/plp->prmpt_cs);
	} else
		cmn_err(CE_CONT, "WARNING: 0 cs\n");
}

#endif /* _MPSTATS */

#ifndef UNIPROC

/*
 * void
 * empty_local_runqueue(void)
 *
 *	Empty the local run queue before taking engine off-line.
 *
 * Calling/Exit State:
 *
 *	No locks held on entry or exit.
 *
 * Description:
 *
 *	This function empties the local run queue on to the global
 *	run queue. This is called when an engine is going off-line
 *	and it has soft-bound (affinitized) LWPs on the local
 *	run queue.
 */
void
empty_local_runqueue(void)
{
	list_t *dq;
	runque_t *rq = l.eng->e_rqlist[0];
	lwp_t *next;
	int maxpri;

	RUNQUE_LOCK();
	ASSERT(rq->rq_srunlwps >= 0);

	if (rq->rq_srunlwps == 0) {  /* empty run queue */
		RUNQUE_UNLOCK();
		return;
	}

	ASSERT(rq->rq_maxrunpri >= 0);
	while (rq->rq_srunlwps > 0) {

			maxpri = rq->rq_maxrunpri;
			dq = &rq->rq_dispq[maxpri];
			next= (lwp_t *)dq->flink;
			rq->rq_srunlwps--;
			ASSERT(rq->rq_srunlwps >= 0);

			remque(next);
			next->l_rq = global_rq;
			RUNQUE_UNLOCK();
			LOCK(&next->l_mutex, PLHI);
			RUNQUE_LOCK();
			setbackrq(next);
			preemption(next, nudge, 0, 0);
			UNLOCK(&next->l_mutex, PLBASE);
			if (dq->flink == dq) {
				/*
				 * Last lwp on this queue has been removed,
				 * turn off the summary bit corresponding to
				 * this queue.
				 */
				BITMASKN_CLR1(rq->rq_dqactmap, maxpri);
				if (rq->rq_srunlwps == 0) {
					rq->rq_maxrunpri = -1;
					rq->rq_dispcnt = 0;
				} else {
					/*
					 * Figure out what the next highest
					 * priority is.
					 */
					rq->rq_maxrunpri =
						BITMASKN_FLS(rq->rq_dqactmap,
							BITMASK_NWORDS(maxpri));
					ASSERT(rq->rq_maxrunpri != maxpri);
				}
			}
	}
	RUNQUE_UNLOCK();
}

/*
 * void
 * load_average(void)
 *
 *	Calculate average local run-queue length and store in a global.
 *
 * Calling/Exit State:
 *
 *	None.
 *
 * Description:
 *
 *	This function acquires the run queue lock. Then it traverses
 *	down the list of engines, skipping offlined engines. For those
 *	that are on-line, the local queue length is added to a running
 *	sum. After traversing the entire list, the average queue
 *	length is computed, stored in the global variable and the
 *	run queue lock is unlocked. Before returning, it reschedules
 *	itself, after a configurable number of ticks.
 */
void
load_average(void)
{
	engine_t *nexteng = engine;
	int average = 0;

	RUNQUE_LOCK();
	while (PROCESSOR_UNMAP(nexteng) < Nengine) {
		if (!(nexteng->e_flags & E_NOWAY)) {
			/*
			 * If the engine is not offline/bad.
			 */
			average += (nexteng->e_rqlist[0]->rq_srunlwps);
		}
		nexteng++;
	}
	avgqlen = average/nonline;  /* update average local q length */
	RUNQUE_UNLOCK();
	(void)itimeout(load_average, 0, load_balance_freq, PLHI);
}
#endif
