/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/class/sysclass.c	1.18"
#ident	"$Header: $"

#include <proc/class.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>
#include <util/var.h>

/*
 * Class specific code for the sys class. There are no
 * class specific data structures associated with
 * the sys class and the scheduling policy is trivially
 * simple. There is no time slicing and priority is
 * only changed when the process requests this through the
 * disp argument to sleep().
 */

void		sys_init();
STATIC int	sys_fork(), sys_nosys(), sys_yield();
STATIC void	sys_nullsys(), sys_preempt(), sys_setrun();
STATIC void	sys_wakeup();
STATIC void	sys_insque();
STATIC int	sys_rdblock();
STATIC void	*sys_allocate();

STATIC struct classfuncs sys_classfuncs = {
	sys_nosys,
	sys_nullsys,
	sys_nullsys,
	sys_fork,
	sys_nullsys,	/* forkret */
	sys_nosys,
	sys_nullsys,
	sys_nullsys,
	sys_nosys,
	sys_nosys,
	sys_nullsys,
	sys_preempt,
	sys_nosys,
	sys_setrun,
	sys_nullsys,
	sys_nullsys,
	sys_nullsys,
	sys_nullsys,
	sys_nullsys,
	sys_nullsys,
	sys_wakeup,
	sys_insque,
	sys_rdblock,
	sys_allocate,
	sys_nullsys,
	sys_nosys,
	sys_nosys,
	sys_nosys,
	sys_nosys,
	sys_nullsys,
	sys_nosys,
	sys_nosys,
	sys_nosys,
	sys_nosys,
	sys_nullsys,
	sys_nosys,
	sys_yield,
	sys_nullsys
};


/*
 * void sys_init(id_t cid, int clparmsz, classfuncs_t **clfuncssp,
 *		 int *maxglobprip)
 *
 * Initialize the system scheduling class, passing basis parameters back
 * via clfuncssp and maxglobprip.
 *
 * Calling/Exit State:
 *
 *	Called from dispinit, single-threaded.  No locking necessary.
 *
 * Description:
 *
 *	Sys_init returns a pointer to the class operations structure for
 *	the system scheduling class.  It also informs the general dispatcher
 *	of the maximium number of priority bands allocated for this
 */
/* ARGSUSED */
void
sys_init(id_t cid, int clparmsz, classfuncs_t **clfuncspp, int *maxglobprip)
{
	*clfuncspp = &sys_classfuncs;

	if (v.v_maxsyspri < PRINPRIS)
		/*
		 *+ The kernel was configured with less than 40 priority
		 *+ bands.  Corrective action:  reconfigure the kernel with
		 *+ 40 priority bands for v_maxsyspri.  It is not recommended
		 *+ this parameter be changed by the user.
		 */
		cmn_err(CE_PANIC, "configured MAXCLSYSPRI out of range");

	*maxglobprip = v.v_maxsyspri;
}


/*
 * int sys_fork(lwp_t *plwp, lwp_t *clwp, char *cpstatp,
 *		short *cpprip, uint *cpflagp, struct cred **cpcredpp,
 *		void **lwpp)
 *
 * Create a new systems lwp.
 *
 * Calling/Exit State:
 *
 *	lwp_mutex is held by caller.
 *
 * Description:
 *
 *	This simply places the child on the run queue and retuns.
 */
/* ARGSUSED */
STATIC int
sys_fork(lwp_t *plwp, lwp_t *clwp, char *cpstatp,
	 short *cpprip, uint *cpflagp, struct cred **cpcredpp,
	 void **lwpp)
{
	clwp->l_stat = SRUN;
	RUNQUE_LOCK();
	setbackrq(clwp);
	preemption(clwp, sys_nudge, 0, NULL);
	RUNQUE_UNLOCK();

	*lwpp = clwp;

	return(0);
}

/*
 * void sys_preempt(lwp_t *lwp)
 *
 * Involunatry preemption of the current lwp.
 *
 * Calling/Exit State:
 *
 *	No locks can be held on entry.
 *
 * Description:
 *
 *	Gain "lwp"'s l_mutex, do some basic verification,
 *	place lwp on the run-queue and give up the engine.
 */
STATIC void
sys_preempt(lwp_t *lwp)
{
	(void)LOCK(&lwp->l_mutex, PLHI);
	lwp->l_stat = SRUN;

	if (LWP_SEIZED(lwp)) {
		/*
		 *+ Address space aging was applied to a system lwp.
		 *+ System lwp's do not have user-mode address spaces to
		 *+ age.
		 */
		cmn_err(CE_PANIC, "sys_preempt: system lwp seized");
	}
	RUNQUE_LOCK();
	setfrontrq(lwp);
	qswtch(lwp);	/* returns with lwp and run-queue unlocked at pl0 */
}


/*
 * void sys_setrun(lwp_t *lwp)
 *
 * Abnormal activation of an lwp.
 *
 * Calling/Exit State:
 *
 *	Lwp's l_mutex must be held.
 *
 * Description:
 *
 *	Called to activate "lwp" as the result of a swap-in, interrupt
 *	or release from seizure.  Must be called with the lwp's l_mutex
 *	held.  Verifies the lwp hasn't been unloaded or seized.
 *	grabs the run-queue, puts the lwp on the run-queue, does the
 *	preemption computation and releases the run-queue.
 */
STATIC void
sys_setrun(lwp_t *lwp)
{
	if (!LWP_LOADED(lwp) || LWP_SEIZED(lwp))
		/*
		 *+ Address space aging was applied to a system lwp or
		 *+ the user-mode address space of a system lwp was swapped
		 *+ out.  System lwp's do not have user-mode address spaces to
		 *+ age.
		 */
		cmn_err(CE_PANIC, "sys_setrun: system lwp swapped or seized");

	RUNQUE_LOCK();
	setbackrq(lwp);
	preemption(lwp, sys_nudge, 0, NULL);
	RUNQUE_UNLOCK();
}


/*
 * void sys_wakeup(lwp_t *lwp, int preemptflg)
 *
 * Normal activation of an lwp.
 *
 * Calling/Exit State:
 *
 *	Lwp's l_mutex must be held.
 *
 * Description:
 *
 *	Called to activate "lwp" due to the award of a resource of the
 *	occurance of an event.  Places lwp back on the run-queue and
 *	does any necessary preemption.
 */
STATIC void
sys_wakeup(lwp_t *lwp, int preemptflg)
{
	if (!LWP_LOADED(lwp) || LWP_SEIZED(lwp))
		/*
		 *+ Address space aging was applied to a system lwp or
		 *+ the user-mode address space of a system lwp was swapped
		 *+ out.  System lwp's do not have user-mode address spaces to
		 *+ age.
		 */
		cmn_err(CE_PANIC, "sys_setrun: system lwp swapped or seized");

	RUNQUE_LOCK();
	setbackrq(lwp);
	preemption(lwp, sys_nudge, preemptflg, NULL);
	RUNQUE_UNLOCK();
}

/*
 * void sys_insque(lwp_t *lwp, list_t *q, int priority)
 *
 * Place lwp on a kernel sync object.
 *
 * Calling/Exit State:
 *
 *	Lwp's l_mutex must be held.
 *
 * Description:
 *
 *	Place "lwp" on the list of blocked lwp's pointed to by
 *	"q".
 */
/* ARGSUSED */
STATIC void
sys_insque(lwp_t *lwp, list_t *q, int priority)
{
	ASSERT(PRINPRIS <= v.v_maxsyspri);

	lwp->l_pri = v.v_maxsyspri - PRINPRIS + priority;

	/*
	 * Simple FIFO queuing.
	 */
	insque(lwp, q->rlink);
}

/*
 * void sys_rdblock(lwp_t *lwp, list_t *q)
 *
 * Determine if reader "lwp" should block against a lock.
 *
 * Calling/Exit State:
 *
 *	Lwp's l_mutex must be held.
 *
 * Description:
 *
 *	Lwp is attempting to gain a contended lock as a reader upon which
 *	there are lwp's waiting, at least one of which is a writer.
 *	Since we know queuing is FIFO and the queue can only be non-empty
 *	if the first queued lwp is a writer, simply check for an empty
 *	queue.  If empty, allow the reader in, otherwise cause the reader
 *	to block.
 */
/* ARGSUSED */
STATIC int
sys_rdblock(lwp_t *lwp, list_t *q)
{
	/*
	 * If anyone is queued, force the current lwp to queue.
	 */
	return !EMPTYQUE(q);
}

/*
 * int sys_yield(lwp_t *lwp, int flag)
 *
 *	Yield the processor to an lwp in the same priority band.
 *
 * Calling/Exit State:
 *
 *	Called without locks held.  Contends lwp->l_mutex and
 *	the RUNQUE_LOCK.  Returns at PLBASE with the locks released.
 */
/* ARGSUSED */
STATIC int
sys_yield(lwp_t *lwp, int flag)
{
	(void)LOCK(&lwp->l_mutex, PLHI);

	lwp->l_stat = SRUN;

	RUNQUE_LOCK();
	setbackrq(lwp);
	qswtch(lwp);

	return(0);
}

/*
 * int sys_nosys(void)
 *
 * System scheduling no-op returning an integer.
 *
 * Calling/Exit State:
 *
 *	None.
 *
 * Description:
 *
 *	Used for unneeded class operations which returned an integer.
 */
STATIC int
sys_nosys(void)
{
	return(ENOSYS);
}


/*
 * void sys_nullsys(void)
 *
 * System scheduling no-op not returning anything.
 *
 * Calling/Exit State:
 *
 *	None.
 *
 * Description:
 *
 *	Used for unneeded class operations which return void.
 */
STATIC void
sys_nullsys(void)
{
}

/*
 * void *sys_allocate(void)
 *
 * Allocation function.
 *
 * Calling/Exit State:
 *
 *	None.
 *
 * Description:
 *
 *	Since the system class doesn't have any per-lwp private data,
 *	we simply need to return a value which indicates to the caller
 *	that the lwp is being accepted into the scheduling class.
 */
/* ARGSUSED */
STATIC void *
sys_allocate(lwp_t *clwp)
{
	return((void *)clwp);
}
