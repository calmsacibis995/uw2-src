/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:util/engine.c	1.32"
#ident	"$Header: $"

/*
 * Routines to deal with "engines" (another name for processors).
 */

#include <util/types.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/engine.h>
#include <io/clkarb.h>
#include <io/scan.h>
#include <io/SGSproc.h>
#include <io/slic.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <util/ipl.h>
#include <util/plocal.h>
#include <mem/kma.h>
#include <mem/kmem.h>
#include <mem/vmparam.h>
#include <proc/disp.h>
#include <util/processor.h>
#include <svc/systm.h>
#include <acc/priv/privilege.h>
#include <svc/keyctl.h>

extern void limit(int, int *);

/* private function declarations */
STATIC	void	halt_eng(engine_t *);

int	nonline	= 1;	/* first proc does not do online_engine() */

lock_t eng_tbl_mutex;
LKINFO_DECL(eng_tbl_lkinfo, "eng_tbl_mutex", 0);
sleep_t	onoff_mutex;
LKINFO_DECL(onoff_lkinfo, "onoff_mutex", 0);

/*
 * Used to mutex the "e_count" field.  This solves a fairly sticky
 * problem involving bind inheritance.  The lock heirachy between
 * is eng_tbl_mutex first and l_mutex after.  Unfortunately, when
 * creating a new lwp, we need to hold the l_mutex of both the parent
 * and child to prevent the bindings of either from being changed before
 * the inheritance can take place.  Fortunately, the only thing which
 * happens when a binding is inherited is 1) the l_rq of the parent is
 * given to the child and 2) the e_count of the engine is incremented.
 * (1) happens as a matter of course and (2) must happen without
 * acquiring eng_tbl_mutex (lock order violation).  Hence, inheritance
 * of a binding increments the e_count of an engine while holding
 * "eng_count_mutex".
 *
 * One more thing to note:  e_count can be incremented without holding
 * eng_tbl_mutex, but will be decremented only while holding eng_tbl_mutex
 * (as well as eng_count_mutex), thus, you can test e_count greater then
 * a certain value while holding eng_tbl_mutex and not eng_count_mutex.
 */
fspin_t eng_count_mutex;

event_t	eng_wait;

/*
 * online_kl1pt is used by online_engine() to communicate to reset_code()
 * the physical address of the newly onlined engine's level 1 page table.
 *
 * online_engno is used by online_engine() to communicate to reset_code()
 * the logical engine number (engine[] index) for the newly onlined engine.
 *
 * They are mutexed by onoff_mutex.
 *
 * They are statically initialized merely to force them into kernel static
 * data rather than BSS; this forces the bootstrap loader to load them into
 * physical memory such that reset_code() can calculate their physical
 * addresses by subtracting KVSBASE from their virtual addresses.
 */
paddr_t online_kl1pt = 0;	/* initialize to force into static data */
ulong_t online_engno = 0;	/* initialize to force into static data */

/*
 * void
 * engine_init(void)
 *	Initialize the locks used in this file.
 *
 * Calling/Exit State:
 *	Assume we're called from an lwp context.
 * 
 */
void
engine_init(void)
{
	SLEEP_INIT(&onoff_mutex, 0, &onoff_lkinfo, KM_NOSLEEP);
	EVENT_INIT(&eng_wait);
	FSPIN_INIT(&eng_count_mutex);
}

struct online_args {
	int	processor;
	int	flag;
};

/*
 * int
 * online(struct online_args *uap, rval_t *rvp)
 *
 *	System call for online and offline of a processor.
 *
 * Calling/Exit State:
 *
 *	This is a system call.
 *
 * Description:
 *
 *	System call to bring a processor online, offline, find the
 *	current status and the maximum number.
 */
int
online(struct online_args *uap, rval_t *rvp)
{
	engine_t *eng;
	int error;

	if (uap->processor == PROC_BAD) {
		rvp->r_val1 = Nengine;
		return(0);
	}

	switch (uap->flag) {
	case P_ONLINE:
       		if (pm_denied(u.u_lwpp->l_cred, P_SYSOPS))
                	return EPERM;
 	        if (!(error = online_engine(uap->processor))) {
 		     eng = PROCESSOR_MAP(uap->processor);
 		     if (eng->e_flags & (E_OFFLINE|E_SHUTDOWN))
 		          rvp->r_val1 = P_OFFLINE;
 		     else
 		          rvp->r_val1 = P_ONLINE;
 		}
 	        return error;

	case P_OFFLINE:
       		if (pm_denied(u.u_lwpp->l_cred, P_SYSOPS))
                	return EPERM;
 		if (!(error = offline_engine(uap->processor))) {
 		     eng = PROCESSOR_MAP(uap->processor);
 		     if (eng->e_flags & (E_OFFLINE|E_SHUTDOWN))
 		          rvp->r_val1 = P_OFFLINE;
 		     else
 		          rvp->r_val1 = P_ONLINE;
 	        }
 	        return error;
 
	case P_QUERY:
		eng = PROCESSOR_MAP(uap->processor);
		if (eng == NULL)
			return(EINVAL);

		if (eng->e_flags & E_BAD)
			rvp->r_val1 = P_BAD;
		else if (eng->e_flags & (E_OFFLINE|E_SHUTDOWN))
			rvp->r_val1 = P_OFFLINE;
		else
			rvp->r_val1 = P_ONLINE;
		return(0);
	}

	return(EINVAL);
}

/*
 * int
 * online_engine(int)
 *
 *	Online a engine and do error checking.
 *
 * Calling/Exit State:
 *
 *	Coordinates concurrent online/offline by acquiring
 *	onoff_mutex.
 *
 *	Returns the errno on error, zero on sucess.
 */
int
online_engine(int engno)
{
	engine_t *eng;
	int error;
	pl_t s;
	int  maxneng;

	/*
	 * Validate engine number.
	 */

	if ((unsigned int)engno >= Nengine)
		return EINVAL;

	eng = &engine[engno];

	/*
	 * Coordinate with concurrent online or offline.
	 */
	SLEEP_LOCK(&onoff_mutex, PRIZERO);

	limit(K_GETPROCLIMIT, &maxneng);
	if (nonline >= maxneng && maxneng < MAXSKEYS) {
		SLEEP_UNLOCK(&onoff_mutex);
		return EINVAL;
	}

	/*
	 * Check if engine is already offline, return zero.
	 */

	if ((eng->e_flags & E_OFFLINE) == 0) {
		SLEEP_UNLOCK(&onoff_mutex);
		return 0;
	}

	/*
	 * Check if engine is bad (failed diagnostics).
	 */
	if (eng->e_flags & E_BAD) {
		SLEEP_UNLOCK(&onoff_mutex);
		return ENODEV;
	}

	/*
	 * online_kl1pt is used to communicate to reset_code() the physical
	 * address of the newly onlined engine's level 1 page table.
	 *
	 * online_engno is used by online_engine() to communicate to
	 * reset_code() the logical engine number (engine[] index) for
	 * the newly onlined engine.
	 */
	online_kl1pt = kvtophys((vaddr_t)&eng->e_local->pp_kl1pt[0][0]);
	online_engno = engno;

	EVENT_CLEAR(&eng_wait);

	/*
	 * Un-hold the processor, and *don't* reset.
	 * Also enable NMI's: it's ok for 1st online (don't expect any NMI
	 * sources) and subsequent online's need NMI's enabled here since they
	 * don't execute localinit() to enable NMI's.  This gives small risk
	 * of strange crash if NMI is asserted on 1st online (since processor
	 * is an 8086 at this time); if a problem, need to keep state in
	 * e_flags whether the processor has ever been online'd before, and
	 * initialize PROC_CTL differently here 1st time vs subsequent times.
	 */
	if (eng->e_flags&E_SGS2)
		unhold_proc(eng->e_slicaddr);
	else
		slic_wrslave(eng->e_slicaddr, PROC_CTL,
			(PROC_CTL_NO_SSTEP|PROC_CTL_NO_HOLD|PROC_CTL_NO_RESET));

	/* Wait for target engine to signal online completion. */
	EVENT_WAIT(&eng_wait, PRIZERO);
	if (eng->e_flags & E_BAD) {
		SLEEP_UNLOCK(&onoff_mutex);
		return ENODEV;
	}

	if (light_show && fp_lights) {
		s = splhi();
		FP_LIGHTON(engno);
		splx(s);
	}

	if ((error = kma_online(eng)) == 0)
		++nonline;

	SLEEP_UNLOCK(&onoff_mutex);

	return error;
}

/*
 * int
 * offline_engine(int)
 *	Offline a engine and do error checking.
 *
 * Calling/Exit State:
 *	Coordinates concurrent online/offline by acquiring
 *	onoff_mutex.
 *
 *	Returns the errno on error, zero on sucess.
 */
int
offline_engine(int engno)
{
	engine_t *eng;
	pl_t pl;

	/*
	 * Validate engine number.
	 */

	if ((unsigned int)engno >= Nengine)
		return EINVAL;
	eng = &engine[engno];

	/*
	 * Coordinate with concurrent online or offline.
	 */
	SLEEP_LOCK(&onoff_mutex, PRIZERO);

	/*
	 * If engine is already offline return zero.  If engine
	 * is bad, return error.
	 */

	if (eng->e_flags & E_OFFLINE) {
		SLEEP_UNLOCK(&onoff_mutex);
		return 0;
	}
	if (eng->e_flags & E_BAD) {
		SLEEP_UNLOCK(&onoff_mutex);
		return ENODEV;
	}

	/*
	 * If this is the only processor online, then error.
	 */

	if (nonline == 1) {
		SLEEP_UNLOCK(&onoff_mutex);
		return EBUSY;
	}

	kma_offline(eng);

	pl = LOCK(&eng_tbl_mutex, PLHI);

	/*
	 * Call into the dispatcher and see if this engine
	 * cannot be taken offline due to bindings (exclusive or otherwise).
	 *
	 * If the engine's currently got bound drivers, refuse to offline
	 * things.
	 */
	if (!dispofflineok(eng) || eng->e_flags & E_DRIVERBOUND) {
		UNLOCK(&eng_tbl_mutex, pl);
		(void) kma_online(eng);
		SLEEP_UNLOCK(&onoff_mutex);
		return EBUSY;
	}

	/*
	 * Shutdown ok, set shutdown request bit while holding
	 * engine table mutex.  Setting of the shutdown request
	 * bit and nudge must be atomic to avoid races.
	 */

	EVENT_CLEAR(&eng_wait);
	eng->e_flags |= E_SHUTDOWN;
	UNLOCK(&eng_tbl_mutex, pl);

	/*
	 * Nudge engine and wait for engine to
	 * see shutdown request.
	 */
	RUNQUE_LOCK();
	kpnudge(PRINPRIS - 1, eng);
	RUNQUE_UNLOCK();
	EVENT_WAIT(&eng_wait, PRIZERO);

	/*
	 * Now halt the engine.
	 */

	halt_eng(eng);
	eng->e_flags &= ~E_SHUTDOWN;
	nonline--;

	SLEEP_UNLOCK(&onoff_mutex);

	return 0;
}

/*
 * STATIC void
 * halt_eng(engine_t *)
 *	Halt a processor.
 *
 * Calling/Exit State:
 *	Assumes caller has arranged to serialize calls to prevent
 *	deadlocks.
 */
STATIC void
halt_eng(engine_t *eng)
{
	int	slicid;
	int	s;

	slicid = eng->e_slicaddr;

	/*
	 * HOLD the processor, but don't reset it (also turn OFF led).
	 * Wait for processor to be HELD.
	 */

	if (eng->e_flags & E_SGS2) {
		/*
		 * No way for a remote processor to turn off the LED.
		 */
		hold_proc(slicid);
	} else {
		slic_wrslave(slicid, PROC_CTL,
				PROC_CTL_LED_OFF | PROC_CTL_NO_NMI |
				PROC_CTL_NO_SSTEP | PROC_CTL_NO_RESET);
		do { /* nothing */ }
		while (slic_rdslave(slicid, PROC_STAT) & PROC_STAT_NO_HOLDA);
	}

	if (light_show && fp_lights) {
		s = splhi();
		FP_LIGHTOFF(eng - engine);
		splx(s);
	}
}

/*
 * boolean_t
 * engine_disable_offline(int engno)
 *      Test and lock the cpu to turn off offline capability.
 *
 * Calling/Exit State:
 *	The global onoff_mutex sleep lock may or may not be held
 *	on entry.
 *
 * Description:
 *	If the cpu is offline, this function returns B_FALSE.
 *	Otherwise, this function sets the E_DRIVERBOUND flag and
 *	turn off the offline capability of the engine.
 */
boolean_t
engine_disable_offline(int engno)
{
	engine_t *eng;
	pl_t pl;

	/*
	 * Validate engine number.
	 */
	if ((unsigned int)engno >= Nengine)
		return B_FALSE;

	eng = &engine[engno];
	pl = LOCK(&eng_tbl_mutex, PLHI);

	if (eng->e_flags & E_NOWAY) {
		UNLOCK(&eng_tbl_mutex, pl);
		return B_FALSE;
	}

	eng->e_flags |= E_DRIVERBOUND;
	UNLOCK(&eng_tbl_mutex, pl);

	return B_TRUE;
}
