/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/class/ts.c	1.52"
#ident	"$Header: $"

#include <acc/audit/audit.h>
#include <acc/priv/privilege.h>
#include <mem/kmem.h>
#include <proc/class/ts.h>
#include <proc/class/tspriocntl.h>
#include <proc/class.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/priocntl.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <proc/usync.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/ghier.h>
#include <util/ksynch.h>
#include <util/plocal.h>
#include <util/types.h>

/*
 * Class specific code for the time-sharing class
 */


/*
 * Extern declarations for variables defined in the ts master file
 */
extern tsdpent_t  ts_dptbl[];	/* time-sharing disp parameter table */
extern int	ts_kmdpris[];	/* array of global pris used by ts lwp's when */
				/*  sleeping or running in kernel after sleep */
extern short	ts_maxupri;	/* max time-sharing user priority */
extern short	ts_maxumdpri;	/* maximum user mode ts priority */
extern short	ts_maxkmdpri;	/* maximum kernel mode ts priority */
extern int	ts_sleepmax;	/* max out credited sleep time at this amount */
extern int	tssleepwait;	/* reward the LWP sleeping at least this amount */

extern	int	load_lwp();

#ifndef UNIPROC
extern clock_t	maxcachewarm;
extern int	maximbalance;
extern int	ts_affinity_on;
extern int	avgqlen;
extern int	nidle;
#endif

tshash_t *tshashp;  /* global pointer to tshash list */

#define TSHASH(pid, lwpid)	(((pid % TSHASHSZ) + (lwpid % TSHASHSZ)) % TSHASHSZ)

#define NZERO           20   /* for nice */


#define	tsumdpri	(tslwpp->ts_umdpri)
#define	tsmedumdpri	(ts_maxumdpri >> 1)

#define	TS_NEWUMDPRI(tslwpp)	\
{ \
if (((tslwpp)->ts_umdpri = (tslwpp)->ts_cpupri + (tslwpp)->ts_upri) > ts_maxumdpri) \
	(tslwpp)->ts_umdpri = ts_maxumdpri; \
else if ((tslwpp)->ts_umdpri < 0) \
	(tslwpp)->ts_umdpri = 0; \
}

/*
 *+ This is a per hash bucket spin lock. All TS lwp's are put on
 *+ on a hash list.
 */
STATIC LKINFO_DECL(tshash_mutex_buf, "PS:TS:ts hash lock", 0);

void		ts_init(id_t , int , classfuncs_t **, int *);
STATIC int	ts_admin(void * , id_t , cred_t *);
STATIC void	*ts_allocate(void *);
STATIC int	ts_cancelchange(lwp_t *, int, qpcparms_t *,
			pcparms_t *, void *, cred_t *, int *);
STATIC int	ts_changeparms(lwp_t *, int, id_t, pcparms_t *,
			void *, cred_t *, qpcparms_t **, int *);
STATIC int	ts_changeend(pcparms_t *, void *);
STATIC int	ts_changestart(pcparms_t *, void **);
STATIC void	ts_deallocate(void *);
STATIC void	ts_enterclass(long *, lwp_t *, lwpstat_t *,
			int *, uint *, cred_t **,void **, void *);
STATIC void	ts_exitclass(lwp_t *);
STATIC int	ts_fork(void *, lwp_t *, lwpstat_t *,
			int *, uint *, cred_t **, void **);
STATIC int	ts_getclinfo();
STATIC void	ts_insque(lwp_t *, list_t *, int);
STATIC int	ts_lwpcmp(void *, long *);
STATIC void	ts_parmsget(void *, long *);
STATIC int	ts_parmsin(long *);
STATIC int	ts_parmsout(long *);
STATIC void	ts_parmsset(long *, void *, void *);
STATIC void	ts_preempt(void *);
STATIC int	ts_rdblock(lwp_t *, list_t *);
STATIC void	ts_setrun(void *);
STATIC void	ts_tick(void *);
STATIC void	ts_wakeup(void *, int );
STATIC void	ts_audit(int, int, int, void *, void *);

STATIC int	ts_nosys();
STATIC void	ts_nullsys();

void		ts_donice();
STATIC void	ts_trapret(), ts_update();

id_t	ts_cid;		/* time-sharing class ID */
STATIC int	ts_maxglobpri;	/* maximum global priority used by ts class */
STATIC int ts_prepblock(lwp_t *, sq_t *, int);
STATIC int ts_urdblock(lwp_t *, sq_t *, int *);
STATIC int ts_yield(void *, int);
#define	ts_cancelblock	sq_cancelblock
#define ts_unblock sq_unblock
#define ts_block sq_block


STATIC struct classfuncs ts_classfuncs = {
	ts_admin,
	ts_enterclass,
	ts_exitclass,
	ts_fork,
	ts_nullsys,	/* forkret */
	ts_getclinfo,
	ts_nullsys,	/* getglobpri */
	ts_parmsget,
	ts_parmsin,
	ts_parmsout,
	ts_parmsset,
	ts_preempt,
	ts_lwpcmp,
	ts_setrun,
	ts_nullsys,	/* sleep */
	ts_nullsys,	/* stop */
	ts_nullsys,	/* swapin */
	ts_nullsys,	/* swapout */
	ts_tick,
	ts_trapret,
	ts_wakeup,
	ts_insque,
	ts_rdblock,
	ts_allocate,
	ts_deallocate,
	ts_changestart,
	ts_changeparms,
	ts_cancelchange,
	ts_changeend,
	ts_nullsys,
	ts_nosys,
	ts_prepblock,
	ts_block,
	ts_unblock,
	ts_cancelblock,
	ts_urdblock,
	ts_yield,
	ts_audit,
};


/*
 * void
 * ts_init(id_t cid, int clparmsz, classfuncs_t **clfuncspp, int *maxglobprip)
 *	TS class initialization routine.
 * 
 * Calling/Exit State:
 *
 *	Called at boot time. No locking requirements for caller.
 *
 * Description:
 *	
 *	Time sharing class initialization.  Called by dispinit()
 *	at boot time. Records the maximum global priority in TS
 *	class, class ID and allocates memory for the TS hash buckets.
 *	Sets a pointer to the class specific function vector
 *	and the maximum global priority within this class.
 *
 */
/*ARGSUSED*/
void
ts_init(id_t cid, int clparmsz, classfuncs_t **clfuncspp, int *maxglobprip)
{
	int	i;
	tshash_t	*tshp;


	ts_maxglobpri = max(ts_kmdpris[ts_maxkmdpri],
	    ts_dptbl[ts_maxumdpri].ts_globpri);

	ts_cid = cid;		/* Record our class ID */

	/*
	 * Allocate TS hash list.
	 */
	if ((tshashp = (tshash_t *)kmem_zalloc(TSHASHSZ * sizeof(struct tshash),
			KM_NOSLEEP)) == NULL) {
		/*
		 *+ No memory available for hash list. No recourse
		 *+ but to panic the system.
		 */
		cmn_err(CE_PANIC, "No memory for TS hash buckets");
		/* NOTREACHED */
	}

	/*
	 * Initialize hash buckets.
	 */
	for (i = 0; i < TSHASHSZ; i++) {
		tshp = &tshashp[i];
		INITQUE(&tshp->th_qlink);
		LOCK_INIT(&tshp->th_lock, TSHASH_MUT_HIER, PLHI,
			&tshash_mutex_buf, KM_SLEEP);
	}

	/*
	 * We're required to return a pointer to our classfuncs
	 * structure and the highest global priority value we use.
	 */
	*clfuncspp = &ts_classfuncs;
	*maxglobprip = ts_maxglobpri;

	if (itimeout(ts_update, NULL, HZ | TO_PERIODIC, PLTIMEOUT) == 0) {
		/*
		 *+ Insufficient resources to allocate ts_update timer.
		 */
		cmn_err(CE_PANIC, "itimeout for ts_update failed");
		/* NOTREACHED */
	}
}


/*
 * int
 * ts_admin(void * uaddr, id_t reqpcid, cred_t *reqcredp)
 *
 * 	Get or reset the ts_dptbl values per the user's request.
 *
 * Calling/Exit State:
 *
 *	Called from priocntl after acquiring the priocntl_update
 *	sleep lock. No other lock is required to be held. Function
 *	returns with priocntl_update lock still held.
 *
 * Description:
 *
 * 	ts_admin() is used to maintain and update the TS dispatcher
 *	table. The caller can either query the existing table values
 *	or overwrite the table with new values. In the latter case,
 *	sanity checking is done to ensure that the system does not
 *	panic. No other checking is done to ensure that the values
 *	supplied result in reasonable performance.
 *
 *	Locks are not held while getting table values. While setting values,
 *	by simply copying the new values into the table, inconsistencies
 *	(such as new global priority and old time quantum, for example)
 *	could result for very short durations, but this effect would
 *	cancel out within a few context switches.
 */
/* ARGSUSED */
STATIC int
ts_admin(void * uaddr, id_t reqpcid, cred_t *reqcredp)
{
	tsadmin_t	tsadmin;
	tsdpent_t	*tmpdpp;
	int		userdpsz;
	int		i;
	int		tsdpsz;

	if (copyin(uaddr, (caddr_t)&tsadmin, sizeof(tsadmin_t)))
		return(EFAULT);

	tsdpsz = (ts_maxumdpri + 1) * sizeof(tsdpent_t);

	switch(tsadmin.ts_cmd) {

	case TS_GETDPSIZE:

		tsadmin.ts_ndpents = ts_maxumdpri + 1;
		if (copyout((caddr_t)&tsadmin, uaddr, sizeof(tsadmin_t)))
			return(EFAULT);
		break;

	case TS_GETDPTBL:

		userdpsz = MIN(tsadmin.ts_ndpents * sizeof(tsdpent_t), tsdpsz);
		if (copyout((caddr_t)ts_dptbl,
		    (caddr_t)tsadmin.ts_dpents, userdpsz))
			return(EFAULT);

		tsadmin.ts_ndpents = userdpsz / sizeof(tsdpent_t);
		if (copyout((caddr_t)&tsadmin, uaddr, sizeof(tsadmin_t)))
			return(EFAULT);

		break;

	case TS_SETDPTBL:

		/*
		 * We require that the requesting lwp have privilege.
		 * We also require that the table supplied by the user
		 * exactly match the current ts_dptbl in size.
		 */
		if (pm_denied(reqcredp, P_SYSOPS)
		    && pm_denied(reqcredp, P_TSHAR))
			return(EPERM);
		if (tsadmin.ts_ndpents * sizeof(tsdpent_t) != tsdpsz)
			return(EINVAL);

		/*
		 * We read the user supplied table into a temporary buffer
		 * where it is validated before being copied over the
		 * ts_dptbl.
		 */
		tmpdpp = (tsdpent_t *)kmem_alloc(tsdpsz, KM_SLEEP);
		ASSERT(tmpdpp != NULL);
		if (copyin((caddr_t)tsadmin.ts_dpents, (caddr_t)tmpdpp,
		    tsdpsz)) {
			kmem_free(tmpdpp, tsdpsz);
			return(EFAULT);
		}
		for (i = 0; i < tsadmin.ts_ndpents; i++) {

			/*
			 * Validate the user supplied values.  All we are doing
			 * here is verifying that the values are within their
			 * allowable ranges and will not panic the system.  We
			 * make no attempt to ensure that the resulting
			 * configuration makes sense or results in reasonable
			 * performance.
			 */
			if (tmpdpp[i].ts_quantum <= 0) {
				kmem_free(tmpdpp, tsdpsz);
				return(EINVAL);
			}
			if (tmpdpp[i].ts_tqexp > ts_maxumdpri ||
			    tmpdpp[i].ts_tqexp < 0) {
				kmem_free(tmpdpp, tsdpsz);
				return(EINVAL);
			}
			if (tmpdpp[i].ts_slpret > ts_maxumdpri ||
			    tmpdpp[i].ts_slpret < 0) {
				kmem_free(tmpdpp, tsdpsz);
				return(EINVAL);
			}
			if (tmpdpp[i].ts_maxwait < 0) {
				kmem_free(tmpdpp, tsdpsz);
				return(EINVAL);
			}
			if (tmpdpp[i].ts_lwait > ts_maxumdpri ||
			    tmpdpp[i].ts_lwait < 0) {
				kmem_free(tmpdpp, tsdpsz);
				return(EINVAL);
			}
		}

		/*
		 * Copy the user supplied values over the current ts_dptbl
		 * values.  The ts_globpri member is read-only so we don't
		 * overwrite it.
		 */
		for (i = 0; i < tsadmin.ts_ndpents; i++) {
			ts_dptbl[i].ts_quantum = tmpdpp[i].ts_quantum;
			ts_dptbl[i].ts_tqexp = tmpdpp[i].ts_tqexp;
			ts_dptbl[i].ts_slpret = tmpdpp[i].ts_slpret;
			ts_dptbl[i].ts_maxwait = tmpdpp[i].ts_maxwait;
			ts_dptbl[i].ts_lwait = tmpdpp[i].ts_lwait;
		}

		kmem_free(tmpdpp, tsdpsz);
		break;

	default:
		return(EINVAL);
	}
	return(0);
}


/*
 * int
 * ts_enterclass(long *tsparmsp_arg, lwp_t *lwpp, lwpstat_t *lstatp,
 *		int *lprip, uint *lflagp, cred_t **lcredpp,
 *		void **tslwppp, void *classdata)
 *
 *	Allocate/initialize TS class-specific lwp structure.
 *
 * Calling/Exit State:
 *
 *	Caller must hold lwp_mutex of the target lwp.
 *
 * Description:
 *
 *	Allocate a time-sharing class specific lwp structure and
 *	initialize it with the parameters supplied or with default
 *	parameters. Also, move lwp to specified time-sharing priority.
 */
/* ARGSUSED */
STATIC void
ts_enterclass(long *tsparmsp_arg, lwp_t *lwpp, lwpstat_t *lstatp,
		int *lprip, uint *lflagp, cred_t **lcredpp,
		void **tslwppp, void *classdata)
{
	tsparms_t *tsparmsp = (tsparms_t *)tsparmsp_arg;
	tslwp_t	*tslwpp;
	short		reqtsuprilim;
	short		reqtsupri;
	struct tshash		*tshp;
	pl_t			pl;
	int			oldpri;

	tslwpp = (tslwp_t *)*tslwppp;

	/*
	 * Initialize the tslwp structure.
	 */
	if (tsparmsp == NULL) {
		/*
		 * Use default values.
		 */
		tslwpp->ts_uprilim = tslwpp->ts_upri = 0;
		tslwpp->ts_nice = 20;
		tslwpp->ts_umdpri = tslwpp->ts_cpupri = tsmedumdpri;
	} else {
		/*
		 * Use supplied values.
		 */
		if (tsparmsp->ts_uprilim == TS_NOCHANGE)
			reqtsuprilim = 0;
		else
			reqtsuprilim = tsparmsp->ts_uprilim;

		if (tsparmsp->ts_upri == TS_NOCHANGE) {
			reqtsupri = reqtsuprilim;
		} else {
			/*
			 * Set the user priority to the requested value
			 * or the upri limit, whichever is lower.
			 */
			reqtsupri = tsparmsp->ts_upri;
			if (reqtsupri > reqtsuprilim)
				reqtsupri = reqtsuprilim;
		}

		tslwpp->ts_uprilim = reqtsuprilim;
		tslwpp->ts_upri = reqtsupri;
		tslwpp->ts_nice = 20 - (20 * reqtsupri) / ts_maxupri;
		tslwpp->ts_cpupri = tsmedumdpri;
		TS_NEWUMDPRI(tslwpp);
	}

	tslwpp->ts_sleepwait = 0;
	tslwpp->ts_dispwait = 0;
	tslwpp->ts_flags = 0;
	tslwpp->ts_lwpp = lwpp;
	tslwpp->ts_lstatp = lstatp;
	tslwpp->ts_lprip = lprip;
	tslwpp->ts_lflagp = lflagp;

	/*
	 * Link new structure into TS hash list.
	 */
	tshp = &tshashp[TSHASH((int)lwpp->l_procp->p_pidp->pid_id, (int)lwpp->l_lwpid)];
	pl = LOCK(&tshp->th_lock, PLHI);
	insque(tslwpp, tshp->th_last);
	UNLOCK(&tshp->th_lock, pl);

	/*
	 * Reset priority. Lwp goes to a "user mode" priority
	 * here regardless of whether or not it has slept since
	 * entering the kernel.
	 */

	tslwpp->ts_timeleft = ts_dptbl[tsumdpri].ts_quantum;
	oldpri = tslwpp->ts_lwpp->l_pri;
	tslwpp->ts_lwpp->l_pri = ts_dptbl[tsumdpri].ts_globpri;

	/*
	 * In order to update e_pri field, eng_tbl_mutex needs to
	 * be held. But this is not possible as lwp_mutex is already
	 * held and holding eng_tbl_mutex would violate the locking
	 * order. It is true that all routines that examine e_pri
	 * hold runque-lock. Thus it is sufficient to hold runque
	 * while updating e_pri.
	 */
	RUNQUE_LOCK();
	l.eng->e_pri = tslwpp->ts_lwpp->l_pri;
	RUNQUE_UNLOCK();
	if (tslwpp->ts_lwpp->l_pri < oldpri) {
		RUNQUE_LOCK();
		ipreemption(tslwpp->ts_lwpp, nudge);
		RUNQUE_UNLOCK();
		tslwpp->ts_flags |= TSBACKQ;
	}

	return;
}


/*
 * void
 * ts_exitclass(lwp_t *cllwpp)
 *
 * 	Free tslwp structure of lwp.
 *
 * Calling/Exit State:
 *
 *	Called with the lwp_mutex held. Returns with the lock still held.
 *
 * Description:
 *
 *	ts_exitclass() dequeues the tslwp structure from the
 *	hash queue.  It leaves the per-scheduling class data associated
 *	with the lwp un-freed, as it is the responsibility of the caller
 *	to free this.
 *	While manipulating the hash list, it acquires the hash
 *	bucket lock.
 */
STATIC void
ts_exitclass(lwp_t *cllwpp)
{
	tslwp_t *tslwpp = (tslwp_t *)cllwpp;
	struct tshash	*tshp;
	pl_t		pl;

	tshp = &tshashp[TSHASH((int)tslwpp->ts_lwpp->l_procp->p_pidp->pid_id,
				(int)tslwpp->ts_lwpp->l_lwpid)];

	pl = LOCK(&tshp->th_lock, PLHI);

	remque(tslwpp);
		
	UNLOCK(&tshp->th_lock, pl);
}


/*
 * void *
 * ts_allocate(void * clwpp)
 *
 *	Allocate memory for new lwp.
 *
 * Calling/Exit State:
 *
 *	No locking requirements on caller. No locks held by the function.
 *
 * Description:
 *
 *	This class operation is called before creating a new
 *	lwp via fork or lwp_create.  It allocates memory needed by
 *	the TS scheduling class. Memory allocation is done with
 *	KM_SLEEP.
 */
/* ARGSUSED */
STATIC void *
ts_allocate(void * clwpp)
{
	struct tslwp *tslwpp;

	tslwpp = (tslwp_t *)kmem_zalloc(sizeof(tslwp_t), KM_SLEEP);

	return (tslwpp);
}


/*
 * void
 * ts_deallocate(void * clwpp_arg)
 *
 *	Deallocate memory for class-specific lwp structure.
 *
 * Calling/Exit State:
 *
 *	No locking requirements on caller. No locks held by the function.
 *
 * Description:
 *
 *	This class operation is called to back out of an
 *	earlier call to ts_allocate(). It simply deallocates the
 *	tslwp structure associated with the target lwp.
 */
STATIC void
ts_deallocate(void * clwpp_arg)
{
	tslwp_t *clwpp = (tslwp_t *)clwpp_arg;

	kmem_free(clwpp, sizeof(tslwp_t));
}


/*
 * int
 * ts_fork(void *ptslwpp_arg, lwp_t *clwpp, lwpstat_t *clstatp,
 *		int *clprip, uint *clflagp, cred_t **clcredpp,
 *		void **cllwpp)
 *
 *	Initialize TS class specific lwp structure for child.
 *
 * Calling/Exit State:
 *
 *	Called with the lwp_mutex of child and parent lwp held.
 *	Also, p_mutex of the child process is held.
 *	Returns with all mutexes held.
 *
 * Description:
 *
 *	ts_fork() is called to add the lwp pointed to by clwpp to
 *	the scheduling class of the parent (TS).
 *	The child tslwp structure is initialized and linked into
 *	the hashed list. The child is then placed on the run queue
 *	and the parent just continues.
 */
/* ARGSUSED */
STATIC int
ts_fork(void *ptslwpp_arg, lwp_t *clwpp, lwpstat_t *clstatp,
	int *clprip, uint *clflagp, cred_t **clcredpp,
	void **cllwpp)
{
	tslwp_t *ptslwpp = (tslwp_t *)ptslwpp_arg;
	struct tshash	*tshp;
	struct tslwp	*tslp = (tslwp_t *)*cllwpp;
	pl_t		pl;

	/*
	 * Initialize child's tslwp structure.
	 */
	tslp->ts_timeleft = ts_dptbl[ptslwpp->ts_umdpri].ts_quantum;
	tslp->ts_umdpri = ptslwpp->ts_umdpri;
	tslp->ts_cpupri = ptslwpp->ts_cpupri;
	tslp->ts_uprilim = ptslwpp->ts_uprilim;
	tslp->ts_upri = ptslwpp->ts_upri;
	tslp->ts_nice = ptslwpp->ts_nice;
	tslp->ts_sleepwait = 0;
	tslp->ts_dispwait = 0;
	tslp->ts_flags = ptslwpp->ts_flags & ~TSBACKQ;
	tslp->ts_lwpp = clwpp;
	tslp->ts_lstatp = clstatp;
	tslp->ts_lprip = clprip;
	tslp->ts_lflagp = clflagp;

	/*
	 * Link structure into TS hash list.
	 */
	tshp = &tshashp[TSHASH((int)clwpp->l_procp->p_pidp->pid_id,
				(int)clwpp->l_lwpid)];
	pl = LOCK(&tshp->th_lock, PLHI);
	insque(tslp, tshp->th_last);
	UNLOCK(&tshp->th_lock, pl);
	ptslwpp->ts_flags |= (TSBACKQ|TSFORK);

	ASSERT(LOCK_OWNED(&clwpp->l_mutex));
	clwpp->l_stat = SRUN;
	if (LWP_SEIZED(clwpp)) {
		/* 
		 * Since we will not be placing the LWP on the 
		 * run queue, null out the linkage pointers.
		 */
		clwpp->l_flink = clwpp->l_rlink = NULL;
		return (0);
	}	
	if (!(LWP_LOADED(clwpp))) {
		/*
		 * Child lwp is not loaded. Do not put on the
		 * the run queue, simply return.
		 */
		return (0);
	}
	RUNQUE_LOCK();
	setbackrq(clwpp);
	preemption(clwpp, nudge, 0, NULL);
	RUNQUE_UNLOCK();

	return (0);
}


/*
 * int
 * ts_getclinfo(tsinfo_t *tsinfop, id_t reqpcid, cred_t *reqcredp)
 *
 *	Get TS class info into a buffer.
 *
 * Calling/Exit State:
 *
 *	Called without any special locks held. Returns without
 *	any locks held.
 *
 * Description:
 *
 * 	Returns information about the TS class into the buffer
 *	pointed to by tsinfop. The maximum configured user
 *	priority is the only information we supply.  We ignore
 *	the class and credential arguments because anyone can have
 *	this information.
 */
/* ARGSUSED */
STATIC int
ts_getclinfo(tsinfo_t *tsinfop, id_t reqpcid, cred_t *reqcredp)
{
	tsinfop->ts_maxupri = ts_maxupri;
	return(0);
}


/*
 * int
 * ts_nosys()
 *
 *	Undefined function for TS scheduling class
 *
 * Calling/Exit State:
 *
 *	There are no locking requirements.
 *
 * Description:
 *
 *	This function is used for those generic class switch functions
 *	for which there is no support in the TS class. It returns
 *	the error code ENOSYS.
 */
STATIC int
ts_nosys()
{
	return(ENOSYS);
}


/*
 * void
 * ts_nullsys()
 *
 *	A null (no op) function for TS scheduling class.
 *
 * Calling/Exit State:
 *
 *	There are no locking requirements.
 *
 * Description:
 *
 *	This function is used for those generic class switch functions
 *	for which there is no special TS class specific action. However,
 *	it not an error to call this function. It simply returns success.
 */
STATIC void
ts_nullsys()
{
}


/*
 * void
 * ts_parmsget(void *tslwpp_arg, long *tsparmsp_arg)
 *
 *	Get the time-sharing parameters of a lwp.
 *
 * Calling/Exit State:
 *
 *	Caller must hold the lwp-mutex of the lwp.
 *	Function returns with the lock still held.
 *
 * Description:
 *
 *	This returns certain class specific attributes
 *	of the lwp pointed to by tslwpp into the buffer
 *	pointed to by tsparmsp.
 */
STATIC void
ts_parmsget(void *tslwpp_arg, long *tsparmsp_arg)
{
	tslwp_t *tslwpp = (tslwp_t *)tslwpp_arg;
	tsparms_t *tsparmsp = (tsparms_t *)tsparmsp_arg;

	tsparmsp->ts_uprilim = tslwpp->ts_uprilim;
	tsparmsp->ts_upri = tslwpp->ts_upri;
}


/*
 * int
 * ts_parmsin(long *tsparmsp_arg)
 *
 *	Check validity of time-sharing parameters in the buffer.
 *
 * Calling/Exit State:
 *
 *	Caller need not hold any locks to call this function.
 *
 * Description:
 *
 *	Check the validity of the time-sharing parameters in the
 *	buffer pointed to by tsparmsp. If our caller passes us a
 *	non-NULL reqcredp pointer we also verify that the requesting
 *	lwp (whose credentials are pointed to by reqcredp) has the
 *	necessary permissions to set these parameters for the
 *	target lwp.
 */
/* ARGSUSED */
STATIC int
ts_parmsin(long *tsparmsp_arg)
{
	tsparms_t *tsparmsp = (tsparms_t *)tsparmsp_arg;

	/*
	 * Check validity of parameters.
	 */
	if ((tsparmsp->ts_uprilim > ts_maxupri ||
	    tsparmsp->ts_uprilim < -ts_maxupri) &&
	    tsparmsp->ts_uprilim != TS_NOCHANGE)
		return(EINVAL);

	if ((tsparmsp->ts_upri > ts_maxupri || tsparmsp->ts_upri < -ts_maxupri) &&
	    tsparmsp->ts_upri != TS_NOCHANGE)
		return(EINVAL);

	return(0);
}


/*
 * int
 * ts_parmsout(long *tsparmsp_arg)
 *
 *	This function is generally intended to convert scheduling
 *	parameters from kernel format to user format before copying
 *	out to user address space.
 *
 * Calling/Exit State:
 *
 *	No locking requirements.
 *
 * Description:
 *
 *	For TS class, there is no conversion necessary, hence  this
 *	is a no op.
 */
/* ARGSUSED */
STATIC int
ts_parmsout(long *tsparmsp_arg)
{
	return(0);
}


/*
 * void
 * ts_parmsset(long *cllwpp, void *clparmsp, void *argp)
 *
 *	Set scheduling parameters of a lwp.
 *
 * Calling/Exit State:
 *
 *	Caller must hold lwp-mutex of the target lwp.
 *	Function returns with the lock still held.
 *
 * Description:
 *
 *	Set the scheduling parameters of the lwp pointed to by tslwpp
 *	to those specified in the buffer pointed to by tsparmsp.
 */
/* ARGSUSED */
STATIC void
ts_parmsset(long *cllwpp, void *clparmsp, void *argp)
{
	/*
	 *+ The time sharing scheduling detected incorrect processing
	 *+ of a scheduling parameter change.  This is a basic kernel
	 *+ problem and cannot be corrected by an administrator.
	 */
	cmn_err(CE_PANIC, "TS scheduling class deferred a parameter change");
	return;
}


/*
 * int
 * ts_rdblock(lwp_t *lwpp, list_t *lp)
 *
 *	Determine if the caller should be queued or not.
 *
 * Calling/Exit State:
 *
 *	Caller must hold both the list-lock and the lwp_mutex.
 *
 * Description:
 *
 *	This class operation is used by the reader/write sync primitive.
 *	The calling lwp is attempting to acquire a reader/writer lock
 *	in read mode, currently held by one or more readers when there
 *	are known to be writers waiting for the lock. This function
 *	determines if the caller should be granted the lock or it should
 *	wait behind one or more of the writers.
 *	The TS class enquing algorithm is FIFO, and not based on process
 *	scheduling priority. Hence it suffices if new readers are blocked
 *	some percentage of time so long as a writer is blocked. Therefore,
 *	we will return TRUE (randomly) 25% of the time that the queue of
 *	waiters is non-empty.
 */
/* ARGSUSED */
int
ts_rdblock(lwp_t *lwpp, list_t *lp)
{
	if (!EMPTYQUE(lp)) {
		if (((int)lwpp + (int)lbolt + (int)(lp)) % 17 > 12)
			return(1);
	}
	return(0);
}


/*
 * void
 * ts_preempt(void *tslwpp_arg)
 *
 *	Place the lwp on appropriate run queue.
 *
 * Calling/Exit State:
 *
 *	Caller does not hold lwp_mutex or run queue lock.
 *	This function acquires the lwp_mutex and the run queue lock.
 *	Both are released in qswtch() before this function returns.
 *
 * Description:
 *
 *	Arrange for the lwp to be placed at the appropriate
 *	location (front or back) of the appropriate dispatcher
 *	queue. It calls qswtch() with the run queue lock
 *	and lwp-mutex held. qswtch() returns with both locks
 *	released.
 *
 *	If ts_preempt is called in response to a user binding of an
 *	SONPROC lwp, preemption() must be called to notify the
 *	processor to which binding is taking place. The flag is
 *	also reset.
 */
STATIC void
ts_preempt(void *tslwpp_arg)
{
	tslwp_t *tslwpp = (tslwp_t *)tslwpp_arg;
	lwp_t *lwpp = tslwpp->ts_lwpp;

	(void)LOCK(&lwpp->l_mutex, PLHI);
	lwpp->l_stat = SRUN;

	if (LWP_SEIZED(tslwpp->ts_lwpp)) {
		/*
		 * If we're seized, we simply call
		 * swtch.  Swtch knows how to handle things.
		 */
		swtch(lwpp);
		return;
	}

#ifndef UNIPROC
	if (ts_affinity_on) {
		/*
		 * Cannot affinitize if hard bound or if l_eng is
		 * not set as yet.
		 */
		if ((lwpp->l_kbind == NULL) && (lwpp->l_xbind == NULL) &&
			(lwpp->l_ubind == NULL) &&
			(lwpp->l_eng != NULL)) {
			engine_t *lasteng = lwpp->l_eng;
			int runqlwps = lasteng->e_rqlist[0]->rq_srunlwps;

			RUNQUE_LOCK();
			if (((runqlwps - avgqlen) < maximbalance) &&
				(!((runqlwps > 0) && (nidle > 0))))
				/*
				 * The LWP is put on the global run queue
				 * if there is runqueue imbalance, or if
				 * if there are idle processors and the
				 * engine where it last ran has other
				 * LWP(s) on its local runqueue (and 
				 * therefore, will not run this LWP
				 * right away, so it is better off
				 * going on the global run queue.
				 */
				lwpp->l_rq = lwpp->l_eng->e_rqlist[0];
			else
				lwpp->l_rq = global_rq;
			RUNQUE_UNLOCK();
		}
	}
#endif


	switch (tslwpp->ts_flags & (TSBACKQ|TSKPRI|TSFORK)) {
		case TSBACKQ:
			tslwpp->ts_timeleft = ts_dptbl[tsumdpri].ts_quantum;
			tslwpp->ts_dispwait = 0;
			tslwpp->ts_flags &= ~TSBACKQ;
			RUNQUE_LOCK();
			setbackrq(lwpp);
			break;
		case (TSBACKQ|TSKPRI):
			tslwpp->ts_flags &= ~TSBACKQ;
			RUNQUE_LOCK();
			setbackrq(lwpp);
			break;
		case (TSBACKQ|TSFORK):
			tslwpp->ts_dispwait = 0;
			tslwpp->ts_flags &= ~(TSBACKQ|TSFORK);
			RUNQUE_LOCK();
			setbackrq(lwpp);
			break;
		default:
			RUNQUE_LOCK();
			setfrontrq(lwpp);
			break;
	}

	/*
	 * A check is now made to see if the "user-bind-pending"
	 * flag is set. This flag is set by the "bind()" code
	 * in addition to setting the l.runrun flag. Subsequently,
	 * CL_PREEMPT() gets called and the bound lwp gets queued
	 * on the local run queue of the processor to which it is
	 * bound. It is now necessary to call preemption() to
	 * nudge the processor to which it is bound so that it can
	 * go schedule this lwp, if it was idle, or it can recompute
	 * to see if the scheduler invariant is still maintained.
	 */

	if (lwpp->l_trapevf & EVF_L_UBIND) {
		engine_t *engp = NULL;

		if (lwpp->l_xbind !=NULL)
			engp = lwpp->l_xbind;
		else
			engp = lwpp->l_ubind;
		lwpp->l_trapevf &= ~EVF_L_UBIND; /* clear flag */
		preemption(lwpp, kpnudge, 0, engp);
	}
	qswtch(lwpp);		/*returns with run-queue unlocked */

}


/*
 * int
 * ts_lwpcmp(void *tslwp1p_arg, long *tsparmsp_arg)
 *
 * 	Compare scheduling priorities of two lwp's
 *
 * Calling/Exit State:
 *
 * 	Called from priocntl with the per-process and per-lwp mutex of
 *	tslwp2p_arg held.
 *
 * Description
 *
 *	ts_lwpcmp() is part of the implementation of the PC_GETPARMS
 *	command of the priocntl system call. When the user specifies
 *	multiple lwp's to priocntl PC_GETPARMS the criteria
 *	for selecting a lwp from the set is class specific. The
 *	criteria used by the time-sharing class is the upri value
 *	of the lwp. ts_lwpcmp() simply compares the upri values of current
 *	lwp with the parameters associated with a previously chosen lwp.
 *	All the ugly work of looping through the lwp's in the set is done
 *	by higher level (class independent) functions.
 */
STATIC int
ts_lwpcmp(void *tslwp1p_arg, long *tsparmsp_arg)
{
	tslwp_t *tslwp1p = (tslwp_t *)tslwp1p_arg;
	tsparms_t *tslwp2parmsp = (tsparms_t *)tsparmsp_arg;

	return(tslwp1p->ts_upri - tslwp2parmsp->ts_upri);
}


/*
 * void
 * ts_setrun(void *tslwpp_arg)
 *
 *	Place an lwp on appropriate run queue.
 *
 * Calling/Exit State:
 *
 *	Caller must hold lwp_state lock.
 *	Function returns with lwp_state lock still held.
 *
 * Description:
 *
 *	The lwp pointed to by tslwpp is placed on the
 *	appropriate run queue, after resetting its time
 *	quantum, if appropriate. Time quantum is not reset
 *	if the lwp is running in kernel mode.
 *
 *	The run_queue lock is acquired while calling setbackrq().
 *	After placing the lwp on the run queue, a check is made
 *	(via preemption) to see if any running lwp needs to be
 *	preempted.
 */
STATIC void
ts_setrun(void *tslwpp_arg)
{
	tslwp_t *tslwpp = (tslwp_t *)tslwpp_arg;
	lwp_t *lwpp;
	engine_t *engp = NULL;

	lwpp = tslwpp->ts_lwpp;
	ASSERT(LOCK_OWNED(&lwpp->l_mutex));
	lwpp->l_stat = SRUN;

	if (LWP_SEIZED(lwpp)) {
		/*
		 * Since we are not moving the lwp to the run queue,
		 * null out the linkage pointers.
		 */
		lwpp->l_flink = lwpp->l_rlink = NULL;
		if (!(LWP_LOADED(lwpp))) {
			load_lwp(lwpp);
		}
		return;
	}

	if (!(LWP_LOADED(lwpp))) {
		load_lwp(lwpp);
		return; 
	}

#ifndef UNIPROC
	/*
	 * Cache affinity: For LWPs that are not hard bound, first
	 * a check is made to see if the processor last ran is
	 * idle (and not offline) and if so, the LWP is put on its
	 * local run queue regardless of whether affinity is turned
	 * on or not. Otherwise, a check is made to see if affinity
	 * is turned on. If turned on, the LWP is affinitized only
	 * if cache is still warm and no load balancing needed.
	 */
	if ((lwpp->l_kbind == NULL) && (lwpp->l_xbind == NULL) &&
		(lwpp->l_ubind == NULL) &&
		(lwpp->l_eng != NULL)) { /* cannot affinitize if hard-bound */
		engine_t *lasteng = lwpp->l_eng;

		RUNQUE_LOCK();
		if ((lasteng->e_pri == -1) &&
			(!(lasteng->e_flags &  E_NOWAY))) {
			lwpp->l_rq = lwpp->l_eng->e_rqlist[0];
			engp = lasteng;
		} else {
			if ((ts_affinity_on == 1) &&
				((lbolt - lwpp->l_lastran) < maxcachewarm) &&
				(!(lasteng->e_flags &  E_NOWAY)) &&
				(((lasteng->e_rqlist[0]->rq_srunlwps) - avgqlen) < maximbalance) &&
				(nidle == 0)) {
				/*
				 * cache is still warm, and processor
				 * not offline or going offline or bad,
				 * and no runqueue imbalance and no
				 * other idle processors.
				 */
				lwpp->l_rq = lwpp->l_eng->e_rqlist[0];
				engp = lasteng;
			} else
				lwpp->l_rq = global_rq;
		}
		RUNQUE_UNLOCK();
	} else {
		if (lwpp->l_kbind !=NULL)
			engp = lwpp->l_kbind;
		else if (lwpp->l_xbind != NULL)
			engp = lwpp->l_xbind;
		else if (lwpp->l_ubind != NULL)
			engp = lwpp->l_ubind;
	}
#endif

	if ((tslwpp->ts_flags & TSKPRI) == 0) {
		tslwpp->ts_timeleft = ts_dptbl[tsumdpri].ts_quantum;
		if ((lwpp->l_flag & L_AGEDISP) == 0)
			tslwpp->ts_dispwait = 0;
	}
	tslwpp->ts_flags &= ~(TSBACKQ|TSFORK);

	RUNQUE_LOCK();
	setbackrq(lwpp);
	preemption(lwpp, nudge, 0, engp);
	RUNQUE_UNLOCK();
}


/*
 * void
 * ts_insque(lwp_t *lwpp, list_t *lp, int priority)
 *
 *	Enqueue the lwp on the named blocking queue.
 *
 * Calling/Exit State:
 *
 *	Caller must hold the lwp_mutex of the named lwp and
 *	also the list lock.
 *	Function returns with these locks still held.
 *
 * Description:
 *
 *	ts_insque() replaces the old ts_sleep() routine.
 *	It places the lwp on the blocking queue pointed to by lp.
 *	For TS class, the queueing order is FIFO.
 *	The priority of the lwp is reset so that it will run
 *	at the requested priority (as specified by the third argument)
 *	when it wakes up. The sense of this disp argument is reversed
 *	compared to SVR4. i.e., higher disp value means higher kernel
 *	mode priority when the lwp wakes up.
 */
/* ARGSUSED */
STATIC void
ts_insque(lwp_t *lwpp, list_t *lp, int disp)
{
	struct tslwp *p;

	p = (tslwp_t *)(lwpp->l_cllwpp);

	p->ts_flags |= TSKPRI;
	ASSERT(disp >= 0 && disp <= ts_maxkmdpri);
	lwpp->l_pri = ts_kmdpris[disp];

	/* Now, enqueue it in fifo order */
	insque(lwpp, lp->rlink);
}


/*
 * void
 * ts_tick(void *tslwpp_arg)
 *
 *	Processor-local clock interrupt handling - lwp specific part.
 *
 * Calling/Exit State:
 *
 *	Called from processor-local clock interrupt handler.
 *
 *	This function does not acquire the lwp_mutex to reduce
 *	unnecessary lock round trip times.
 *
 * Description:
 *
 *	This function decrements the remainder of time quantum left.
 *	If the time left in quantum is zero, it computes the new
 * 	quantum and priority for this lwp and puts it on appropriate
 *	dispatcher queue.
 */

STATIC void
ts_tick(void *tslwpp_arg)
{
	tslwp_t *tslwpp = (tslwp_t *)tslwpp_arg;

	if ((tslwpp->ts_flags & TSKPRI) != 0) {
		/*
		 * No time slicing of lwps at kernel mode priorities.
		 */
		return;
	}

	if (--tslwpp->ts_timeleft <= 0) {	
		tslwpp->ts_cpupri = ts_dptbl[tslwpp->ts_cpupri].ts_tqexp;
		tslwpp->ts_sleepwait = 0;
		TS_NEWUMDPRI(tslwpp);
		*tslwpp->ts_lprip = ts_dptbl[tsumdpri].ts_globpri;
		tslwpp->ts_dispwait = 0;

		tslwpp->ts_flags |= TSBACKQ;

		/* cause lwp to give up processor */
		RUNQUE_LOCK();
		nudge(tslwpp->ts_lwpp->l_pri, l.eng);
		RUNQUE_UNLOCK();
	}
}


/*
 * void
 * ts_trapret(tslwp_t *tslwpp)
 *
 *	Adjust global priority of lwp before returning to user mode.
 *
 * Calling/Exit State:
 *
 *	Caller does not hold lwp_mutex of the named lwp.
 *	This function acquires lwp_mutex before calling ipreemption()
 *	and releases it before returning.
 *
 * Description:
 *
 *	If the lwp is currently at a kernel mode priority (has slept)
 *	we assign it the appropriate user mode priority and
 *	time quantum here. Following this, ipreemption() is called
 *	to maintain the run time invariant. Note that no check is
 *	made to see if this is a non-preemptive wakeup case.
 *	This is because preempt-flag was already checked in cl_wakeup()
 *	and acted upon.
 */
STATIC void
ts_trapret(tslwp_t *tslwpp)
{
	pl_t pl;
	int newpri;

	if ((tslwpp->ts_flags & TSKPRI) == 0) {
		return;
	}

	if (tslwpp->ts_sleepwait > tssleepwait) {
		tslwpp->ts_cpupri = ts_dptbl[tslwpp->ts_cpupri].ts_slpret;
		tslwpp->ts_timeleft = ts_dptbl[tsumdpri].ts_quantum;

		/* Whenever we bump the quantuum, decay the sleepwait */

		if (tslwpp->ts_sleepwait > ts_sleepmax)
			tslwpp->ts_sleepwait = ts_sleepmax / 2;
		else
			tslwpp->ts_sleepwait = tslwpp->ts_sleepwait / 2;
	}
	TS_NEWUMDPRI(tslwpp);
	pl = LOCK(&tslwpp->ts_lwpp->l_mutex, PLHI);
	tslwpp->ts_flags &= ~TSKPRI;
	tslwpp->ts_flags |= TSBACKQ;
	newpri = ts_dptbl[tsumdpri].ts_globpri;
	*tslwpp->ts_lprip = newpri;
	tslwpp->ts_dispwait = 0;

	RUNQUE_LOCK();
	ipreemption(tslwpp->ts_lwpp, nudge);
	RUNQUE_UNLOCK();
	UNLOCK(&tslwpp->ts_lwpp->l_mutex, pl);

	dispmdepnewpri(newpri);
}


/*
 * void
 * ts_update(void)
 *
 *	Look for and bump up priorities of languishing TS lwp's.
 *
 * Calling/Exit State:
 *
 *	Called via timeout mechanism.
 *
 * Description:
 *
 *	This routine examines, for each TS class lwp, if it has
 *	been on the run queue for more than a specified amount
 *	of time (this is obtained from the table). If an lwp is
 *	found languishing on the run queue, its priority is boosted.
 *
 *	The hash queue organization results in minimization of
 *	lock latency. This function iterates over all the hash buckets,
 *	for each bucket, it locks it, traverses down the doubly linked
 *	list of lwp's and performs its check on each lwp. The lwp state
 *	will be locked while examining and resetting its priority, etc.
 *	Any lwp whose ts_dispwait exceeds ts_maxwait is a candidate
 *	for priority boost.
 *
 *	The function reschedules itself via timeout mechanism.
 */
STATIC void
ts_update(void)
{
	struct tshash	*tshp;
	tslwp_t	*tslwpp1, *tslwpp2;
	int		i;
	pl_t		plhash, pllwp;
#ifndef UNIPROC
	lwp_t	*lwpp;
	boolean_t	wasonq;
#endif

	for (i = 0; i < TSHASHSZ; i++) {	/* for each hash bucket */
		tshp = &tshashp[i];
		plhash = LOCK(&tshp->th_lock, PLHI);
		tslwpp1 = (tslwp_t *)tshp->th_first;
		while (tslwpp1 != (tslwp_t *)tshp) {
			pllwp = TRYLOCK(&tslwpp1->ts_lwpp->l_mutex, PLHI);
			if (pllwp == (pl_t)INVPL) { /* must be a running lwp */
				tslwpp2 = tslwpp1;
				tslwpp1 = (tslwp_t *)tslwpp1->ts_flink;
				continue;
			}
#ifndef UNIPROC
			if (ts_affinity_on == 1) {

				/*
				 * Cache affinity: if the LWP is not in SRUN
				 * state, we do not concern ourselves with
				 * that LWP. However, if it is in state SRUN and
				 * has no user or kernel binding in effect, and
				 * is already affinitized, and if it has
				 * been on the run queue long enough to render
				 * its cache "cold", we deaffinitize it
				 * (put it on global run queue) here.
				 * This is done here to perform load balancing
				 * between run queues.
				 */
				lwpp = tslwpp1->ts_lwpp;
				if ((lwpp->l_stat == SRUN) && (lwpp->l_kbind == NULL) &&
					(lwpp->l_xbind == NULL) &&
					(lwpp->l_ubind == NULL) &&
					(lwpp->l_rq != global_rq)) {
					if (lbolt - (lwpp->l_lastran) > maxcachewarm) {
						RUNQUE_LOCK();
						wasonq = dispdeq(lwpp);
						lwpp->l_rq = global_rq;
						if (wasonq == B_TRUE) {
							setbackrq(lwpp);
						}
						RUNQUE_UNLOCK();
					}
				}
			}
#endif /* UNIPROC */
			if ((tslwpp1->ts_flags & TSKPRI) != 0) {
				/* If LWP is sleeping, bump its sleepwait */
				if (tslwpp1->ts_lwpp->l_stat == SSLEEP)
					tslwpp1->ts_sleepwait += 8;
				tslwpp2 = tslwpp1;
				tslwpp1 = (tslwp_t *)tslwpp1->ts_flink;
				UNLOCK(&tslwpp2->ts_lwpp->l_mutex, pllwp);
				continue;
			}
			if (!LWP_LOADED(tslwpp1->ts_lwpp) ||
				LWP_SEIZED(tslwpp1->ts_lwpp)) {
				tslwpp2 = tslwpp1;
				tslwpp1 = (tslwp_t *)tslwpp1->ts_flink;
				UNLOCK(&tslwpp2->ts_lwpp->l_mutex, pllwp);
				continue;
			}

			if (tslwpp1->ts_lwpp->l_stat != SRUN &&
				tslwpp1->ts_lwpp->l_stat != SONPROC) {
				tslwpp2 = tslwpp1;
				tslwpp1 = (tslwp_t *)tslwpp1->ts_flink;
				UNLOCK(&tslwpp2->ts_lwpp->l_mutex, pllwp);
				continue;
			}

			tslwpp1->ts_dispwait++;
			if (tslwpp1->ts_dispwait <=
				ts_dptbl[tslwpp1->ts_umdpri].ts_maxwait) {
				tslwpp2 = tslwpp1;
				tslwpp1 = (tslwp_t *)tslwpp1->ts_flink;
				UNLOCK(&tslwpp2->ts_lwpp->l_mutex, pllwp);
				continue;
			}
			tslwpp1->ts_cpupri = ts_dptbl[tslwpp1->ts_cpupri].ts_lwait;
			TS_NEWUMDPRI(tslwpp1);
			tslwpp1->ts_dispwait = 0;
			if (dispnewpri(tslwpp1->ts_lwpp,
				ts_dptbl[tslwpp1->ts_umdpri].ts_globpri) ==
				SONPROC) {
				tslwpp1->ts_flags |= TSBACKQ;
			} else {
				tslwpp1->ts_timeleft = ts_dptbl[tslwpp1->ts_umdpri].ts_quantum;
			}
			tslwpp2 = tslwpp1;
			tslwpp1 = (tslwp_t *)tslwpp1->ts_flink;
			UNLOCK(&tslwpp2->ts_lwpp->l_mutex, pllwp);
		} /* end while */
		UNLOCK(&tshp->th_lock, plhash);
	}
}


/*
 * void
 * ts_wakeup(void *tslwpp_arg, int preemptflg)
 *
 *	Put the lwp on appropriate run queue.
 *
 * Calling/Exit State:
 *
 *	Caller must hold lwp-mutex of the named lwp.
 *	Function returns with the mutex still held.
 *
 * Description:
 *
 *	This function places the lwp on the appropriate
 *	run queue (at the back). After placing the lwp
 *	on the run queue, preemption computation is done
 *	to check if any processor needs to be nudged, if
 *	it is running an lwp of lower priority than the
 *	one just woken up. Non-preemptive wakeup is supported
 *	by simply passing on the preempt flag to the
 *	preemption() function.
 */
/* ARGSUSED */
STATIC void
ts_wakeup(void *tslwpp_arg, int preemptflg)
{
	tslwp_t *tslwpp = (tslwp_t *)tslwpp_arg;
	lwp_t *lwpp;
	engine_t *engp = NULL;

	lwpp = tslwpp->ts_lwpp;
	ASSERT(LOCK_OWNED(&lwpp->l_mutex));
	lwpp->l_stat = SRUN;

	if (LWP_SEIZED(lwpp)) {
		/*
		 * Since we are not placing the LWP on the run queue
		 * null out the linkage.
		 */
		lwpp->l_flink = lwpp->l_rlink = NULL;
		if (!(LWP_LOADED(lwpp))) {
			load_lwp(lwpp);
		}
		return;
	}

	if (!(LWP_LOADED(lwpp))) {
		load_lwp(lwpp);
		return;
	}

#ifndef UNIPROC
	/*
	 * Cache affinity: For LWPs that are not hard bound, first
	 * a check is made to see if the processor last ran is
	 * idle (and not offline) and if so, the LWP is put on its
	 * local run queue regardless of whether affinity is turned
	 * on or not. Otherwise, a check is made to see if affinity
	 * is turned on. If turned on, the LWP is affinitized only
	 * if cache is still warm and no load balancing needed.
	 */
	if ((lwpp->l_kbind == NULL) && (lwpp->l_xbind == NULL) &&
		(lwpp->l_ubind == NULL) &&
		(lwpp->l_eng != NULL)) { /* cannot affinitize if hard-bound */
		engine_t *lasteng = lwpp->l_eng;

		RUNQUE_LOCK();
		if ((lasteng->e_pri == -1) &&
			(!(lasteng->e_flags &  E_NOWAY))) {
			lwpp->l_rq = lwpp->l_eng->e_rqlist[0];
			engp = lasteng;
		} else {
			if ((ts_affinity_on == 1) &&
				((lbolt - lwpp->l_lastran) < maxcachewarm) &&
				(!(lasteng->e_flags &  E_NOWAY)) &&
				(((lasteng->e_rqlist[0]->rq_srunlwps) - avgqlen) < maximbalance) &&
				(nidle == 0)) {
				/*
				 * cache is still warm, and processor
				 * not offline or going offline or bad,
				 * and no runqueue imbalance and no
				 * other idle processors.
				 */
				lwpp->l_rq = lwpp->l_eng->e_rqlist[0];
				engp = lasteng;
			} else
				lwpp->l_rq = global_rq;
		}
		RUNQUE_UNLOCK();
	} else {
		if (lwpp->l_kbind !=NULL)
			engp = lwpp->l_kbind;
		else if (lwpp->l_xbind != NULL)
			engp = lwpp->l_xbind;
		else if (lwpp->l_ubind != NULL)
			engp = lwpp->l_ubind;
	}
#endif

	tslwpp->ts_flags &= ~(TSBACKQ|TSFORK);
	RUNQUE_LOCK();
	setbackrq(lwpp);
	preemption(lwpp, nudge, preemptflg, engp);
	RUNQUE_UNLOCK();
}


/*
 * void
 * ts_donice(void *tslwpp_arg, int incr, int *retvalp)
 *
 *	Increment the nice value of the specified lwp.
 *
 * Calling/Exit State:
 *
 *	Caller must hold lwp_mutex.
 *
 * Description:
 *
 *	ts_donice() is called from priocntl to modify the
 *	nice value of a lwp. The new nice value is returned
 *	in *retvalp.
 */
void
ts_donice(void *tslwpp_arg, int incr, int *retvalp)
{
	tslwp_t *tslwpp = (tslwp_t *)tslwpp_arg;
	int		newnice;
	int		oldpri;

	/*
	 * Specifying a nice increment greater than the upper limit of
	 * 2 * NZERO - 1 will result in the lwp's nice value being
	 * set to the upper limit.  We check for this before computing
	 * the new value because otherwise we could get overflow 
	 * if a privileged lwp specified some ridiculous increment.
	 */
	if (incr >= 2 * NZERO)
		incr = 2 * NZERO - 1;

	newnice = tslwpp->ts_nice + incr;
	if (newnice >= 2 * NZERO)
		newnice = 2 * NZERO - 1;
	else if (newnice < 0)
		newnice = 0;

	/*
	 * Reset the uprilim and upri values of the lwp.
	 */

	tslwpp->ts_uprilim = tslwpp->ts_upri = 
	    -((newnice - NZERO) * ts_maxupri) / NZERO;

	tslwpp->ts_nice = (char)newnice;

	TS_NEWUMDPRI(tslwpp);

	tslwpp->ts_timeleft = ts_dptbl[tsumdpri].ts_quantum;
	tslwpp->ts_dispwait=0;
	oldpri = tslwpp->ts_lwpp->l_pri;
	tslwpp->ts_lwpp->l_pri = ts_dptbl[tsumdpri].ts_globpri;

	/*
	 * In order to update e_pri field, eng_tbl_mutex needs to
	 * be held. But this is not possible as lwp_mutex is already
	 * held and holding eng_tbl_mutex would violate the locking
	 * order. It is true that all routines that examine e_pri
	 * hold runque-lock. Thus it is sufficient to hold runque
	 * while updating e_pri.
	 */
	RUNQUE_LOCK();
	l.eng->e_pri = tslwpp->ts_lwpp->l_pri;
	RUNQUE_UNLOCK();
	if (tslwpp->ts_lwpp->l_pri < oldpri) {
		RUNQUE_LOCK();
		ipreemption(tslwpp->ts_lwpp, nudge);
		RUNQUE_UNLOCK();
		tslwpp->ts_flags |= TSBACKQ;
	}


	if (retvalp)
		*retvalp = newnice - NZERO;
}


/*
 * int
 * ts_changeparms(lwp_t *lwpp, int cmd, id_t cid, pcparms_t *pcparmsp,
 *		  void * classdata, cred_t *credp, qpcparms_t **qpcparmspp,
 *		  int *donep)
 *
 *	Inform scheduling class of pending parameter change.
 *
 * Calling/Exit State:
 *
 *	Must be called with the lwp_mutex held.
 *
 * Description:
 *
 *	This function informs the scheduling class of a scheduling
 *	class parameter change pending against the lwp. It will
 *	analyze the pending change to see if it can be performed
 *	right away or if it must be queued for later processing.
 *	For TS class, any request for change in 'upri' or 'uprilim'
 *	can be accommodated right away. If it is request for
 *	scheduling class change, that must be deferred and this
 *	pending change is indicated by setting a flag in the
 *	lwp structure.
 */
/* ARGSUSED */
int
ts_changeparms(lwp_t *lwpp, int cmd, id_t cid,  pcparms_t *pcparmsp,
	       void *classdata, cred_t *reqcredp, qpcparms_t **qpcparmspp,
	       int *donep)
{
	struct tsparms *tsparmsp;
	short	reqtsuprilim;
	short	reqtsupri;
	char nice;
	tslwp_t *tslwpp = lwpp->l_cllwpp;

	if (cmd == CHANGEPARMS_FORK) {
		/*
		 * Changeparms from parmsprop.  The caller doesn't
		 * really expect us to do anything, just return the
		 * buffer.
		 */
		*qpcparmspp = &tslwpp->ts_qpcparms;
		return(0);
	}

	/*
	 * Check for permission.
	 */
	if (reqcredp && !hasprocperm(lwpp->l_cred, reqcredp)) {
		return EPERM;
	}

	if ((cmd == CHANGEPARMS_CHECKONLY) && (lwpp->l_qparmsp != NULL)) {
		/*
		 * Parmsset is dealing with a queued change and is first
		 * determining if the requester can apply the change to
		 * the target.
		 */
		*donep = 0;
		return(0);
	}

	/*
	 * Parmsset is giving us a fair and square chance to apply
	 * the parameter change directly.
	 */
	if (pcparmsp->pc_cid == cid) {	/* no class change */
		/*
		 * For TS class, the only other changes that can be
		 * requested are upri and/or uprilim.
		 * These can be applied immediately.
		 */
		tsparmsp = (tsparms_t *)pcparmsp->pc_clparms;

		if (tsparmsp->ts_uprilim == TS_NOCHANGE)
			reqtsuprilim = tslwpp->ts_uprilim;
		else
			reqtsuprilim = tsparmsp->ts_uprilim;
	
		if (tsparmsp->ts_upri == TS_NOCHANGE)
			reqtsupri = tslwpp->ts_upri;
		else
			reqtsupri = tsparmsp->ts_upri;
	
		/*
		 * Make sure the user priority doesn't exceed the upri limit.
		 */
		if (reqtsupri > reqtsuprilim)
			reqtsupri = reqtsuprilim;

		if (reqtsuprilim > tslwpp->ts_uprilim &&
		    pm_denied(reqcredp, P_TSHAR))
			/*
			 * You can only raise your limit if you're the super
			 * user.
			 */
			return(EPERM);

		tslwpp->ts_uprilim = reqtsuprilim;
		tslwpp->ts_upri = reqtsupri;
		TS_NEWUMDPRI(tslwpp);
	
		/*
		 * Set ts_nice to the nice value corresponding to the user
		 * priority we are setting.
		 */
		nice = 20 - (tsparmsp->ts_upri * 20) / ts_maxupri;
		if (nice == 40)
			nice = 39;
		tslwpp->ts_nice = nice;

		tslwpp->ts_dispwait = 0;

		/*
		 * Determine if the lwp is at a kernel-mode priority.
		 * We don't fuss with the priority of an lwp at a kernel-mode
		 * priority.
		 */
		if ((tslwpp->ts_flags & TSKPRI) == 0) {
			/*
			 * The lwp is not at a kernel-mode priority and
			 * we can directly change the priority.
			 */
			tslwpp->ts_timeleft = ts_dptbl[tsumdpri].ts_quantum;
			if (dispnewpri(lwpp, ts_dptbl[tsumdpri].ts_globpri) ==
				SONPROC) {
				/*
				 * Make sure the lwp gets put on the
				 * end of the queue.
				 */
				tslwpp->ts_flags |= TSBACKQ;
			}
		}
		*donep = 1;
	} else {
		/*
		 * Scheduling class change.
		 */
		if (cmd == CHANGEPARMS_CHECKONLY) {
			tsparmsp = (tsparms_t *)pcparmsp->pc_clparms;
	
			/*
			 * Only the privileged user can set upri limit
			 * above zero.
			 */
			if (tsparmsp->ts_uprilim > 0 && pm_denied(reqcredp, P_TSHAR))
				return(EPERM);
		}

		if (qpcparmspp != NULL)
			*qpcparmspp = &tslwpp->ts_qpcparms;
		*donep = 0;
	}
	return (0);
}

/*
 * int ts_cancelchange(lwp_t *lwpp, int cmd, qpcparms_t *qpcparmsp,
 *		       pcparms_t *pcparmsp, void *classdata, cred_t *reqcredp,
 *		       int *combinedp)
 *
 *	Deal with a TS queued change relative to a new change.
 *
 * Calling/Exit State:
 *
 *	Must be holding lwpp->l_mutex when called.
 *
 * Description:
 *
 *	The parms* layer wants us to do something to a queued change.
 *	We must either combine a new request with it, or unconditionally
 *	cancel it.
 */
/* ARGSUSED */
int
ts_cancelchange(lwp_t *lwpp, int cmd, qpcparms_t *qpcparmsp,
		pcparms_t *pcparmsp, void *classdata, cred_t *reqcredp,
		int *combinedp)
{
	tsparms_t *oldtsparmsp, *newtsparmsp;
	short		reqtsuprilim;
	short		reqtsupri;

	*combinedp = 0;

	/*
	 * Check for permission.
	 */
	if (!hasprocperm(lwpp->l_cred, reqcredp)) {
		return EPERM;
	}

	ASSERT(qpcparmsp->qpc_pcparms.pc_cid == ts_cid);

	switch (cmd) {
	case CANCELCHANGE_TRYCOMBINE:
		ASSERT(pcparmsp->pc_cid == qpcparmsp->qpc_pcparms.pc_cid);

		/*
		 * Only priorities need to readjusted. This is OK.
		 * The new upri and uprilim values have already been
		 * checked for sanity by priocntl and will be check
		 * here for permissions only.
		 */
		oldtsparmsp = (tsparms_t *)qpcparmsp->qpc_pcparms.pc_clparms;
		newtsparmsp = (tsparms_t *)pcparmsp->pc_clparms;

		if (newtsparmsp->ts_uprilim == TS_NOCHANGE)
			reqtsuprilim = oldtsparmsp->ts_uprilim;
		else
			reqtsuprilim = newtsparmsp->ts_uprilim;
	
		if (newtsparmsp->ts_upri == TS_NOCHANGE)
			reqtsupri = oldtsparmsp->ts_upri;
		else
			reqtsupri = newtsparmsp->ts_upri;
	
		/*
		 * Make sure the user priority doesn't exceed the upri limit.
		 */
		if (reqtsupri > reqtsuprilim)
			reqtsupri = reqtsuprilim;

		/*
		 * If the lwp is "currently" in the TS scheduling
		 * class, only the privileged user can raise his
		 * upri limit.
		 */
		if (reqtsuprilim > oldtsparmsp->ts_uprilim &&
		    pm_denied(reqcredp, P_TSHAR))
			/*
			 * You can only raise your limit if you're the super
			 * user.
			 */
			return(EPERM);

		oldtsparmsp->ts_uprilim = reqtsuprilim;
		oldtsparmsp->ts_upri = reqtsupri;

		*combinedp = 1;
		break;

	case CANCELCHANGE_MUSTCANCEL:
		ASSERT(pcparmsp->pc_cid != qpcparmsp->qpc_pcparms.pc_cid);

		break;
	case CANCELCHANGE_EXIT:
		break;
	}
	return (0);
}


/*
 * int
 * ts_changestart(pcparms_t *pcparmsp, void **argpp)
 *
 *	Function that generally deals with some priocntl related
 *	initializations for scheduling class changes (not for TS).
 *
 * Calling/Exit State:
 *
 *	As this function is a no-op for TS class, there are no
 *	locking requirements.
 *
 * Description:
 *
 *	This function is intended to deal with some initializations
 *	for scheduling classes that deal with aggregations of lwp's.
 *	Since TS class does not deal with such aggregations, this function
 *	is a no op.
 */
/* ARGSUSED */
int
ts_changestart(pcparms_t *pcparmsp, void **argpp)
{
	return(0);
}


/*
 * int
 * ts_changeend(pcparms_t *pcparmsp, void *argp)
 *
 *	Function that generally deals with some priocntl related
 *	end-of-scheduling class change operations (not for TS).
 *
 * Calling/Exit State:
 *
 *	As this function is a no-op for TS class, there are no
 *	locking requirements.
 *
 * Description:
 *
 *	This function is intended to deal with some final processing
 *	for scheduling classes that deal with aggregations of lwp's.
 *	Since TS class does not deal with such aggregations, this function
 *	is a no op.
 */
/* ARGSUSED */
int
ts_changeend(pcparms_t *pcparmsp, void *argp)
{
	return(0);
}

/*
 * int ts_prepblock(lwp_t *lwp, sq_t *sq, int rdwr)
 *
 *	Place the lwp on the end of the queue.
 *
 * Calling/Exit State:
 *
 *	Called with lwp->l_mutex and sq->sq_lock locked.
 */
STATIC int
ts_prepblock(lwp_t *lwp, sq_t *sq, int rdwr)
{
	lwp_t *lp;

	ASSERT(LOCK_OWNED(&lwp->l_mutex));

	lwp->l_ublocktype = (u_char)rdwr;

	if ((lp = sq->sq_tail) == NULL) {
		sq->sq_head = lwp;
		sq->sq_tail = lwp;
		lwp->l_sfwd = NULL;
		lwp->l_sbak = NULL;
	} else {
		lwp->l_sbak = lp;
		lp->l_sfwd = lwp;
		lwp->l_sfwd = NULL;
		sq->sq_tail = lwp;
	}

	return(0);
}



/*
 * int ts_urdblock(lwp_t *lwp, sq_t *sq, int *ret)
 *
 *	Determine if a reader should block or should be granted
 *	the lock.
 *
 * Calling/Exit State:
 *
 *	Must be called with both sq->sq_lock and lwp->l_mutex held.
 *
 * Description:
 *
 *	We determine if the caller (a reader) should block behind a
 *	writer, rather then take the reader/writer semaphore.  Since
 *	TS does all its queuing FIFO, and the caller wouldn't be
 *	calling unless a reader held the lock, we can conclude the
 *	reader would block behind a writer if there is anyone else
 *	on the queue.
 */
/* ARGSUSED */
STATIC int
ts_urdblock(lwp_t *lwp, sq_t *sq, int *ret)
{
	ASSERT(LOCK_OWNED(&lwp->l_mutex));

	*ret = 0;
	if (sq->sq_head)
		/*
		 * Somebody's on the list.  If somebody's on the list,
		 * then we can conclude this is a writer (as it is known
		 * that a reader already holds the sync-queue).  Since
		 * queuing in the TS class if FIFO, we'll be queued after
		 * this lwp, hence, we must block.
		 */
		*ret = 1;
	return(0);
}

/*
 * void ts_yield(flag)
 *
 *	Yield the processor, possibly ending our quantum.
 *
 * Calling/Exit State:
 *
 *	Called with no locks held.
 *
 * Description:
 *
 *	If "flag" is zero, we simply position ourselves at the end of
 *	our current run-queue, otherwise, we perform our end of quantum
 *	action and place ourselves at the end of that run-queue.
 */
STATIC int
ts_yield(void *vtslwpp, int flag)
{
	tslwp_t *tslwpp = vtslwpp;
	lwp_t *lwpp = tslwpp->ts_lwpp;

	(void)LOCK(&lwpp->l_mutex, PLHI);
	lwpp->l_stat = SRUN;

	if (flag) {
		/*
		 * Perform the "end of quantum" action.
		 */
		tslwpp->ts_cpupri = ts_dptbl[tslwpp->ts_cpupri].ts_tqexp;
		TS_NEWUMDPRI(tslwpp);
		*tslwpp->ts_lprip = ts_dptbl[tsumdpri].ts_globpri;
		tslwpp->ts_timeleft = ts_dptbl[tsumdpri].ts_quantum;
		tslwpp->ts_dispwait = 0;
	}

#ifndef UNIPROC
	/*
	 * Cache affinity: For LWPs that are not hard bound, first
	 * a check is made to see if the processor last ran is
	 * idle (and not offline) and if so, the LWP is put on its
	 * local run queue regardless of whether affinity is turned
	 * on or not. Otherwise, a check is made to see if affinity
	 * is turned on. If turned on, the LWP is affinitized only
	 * if cache is still warm and no load balancing needed.
	 */
	if ((lwpp->l_kbind == NULL) && (lwpp->l_xbind == NULL) &&
		(lwpp->l_ubind == NULL) &&
		(lwpp->l_eng != NULL)) { /* cannot affinitize if hard-bound */
		engine_t *lasteng = lwpp->l_eng;

		RUNQUE_LOCK();
		if ((lasteng->e_pri == -1) &&
			(!(lasteng->e_flags &  E_NOWAY))) {
			lwpp->l_rq = lwpp->l_eng->e_rqlist[0];
		} else {
			if ((ts_affinity_on == 1) &&
				((lbolt - lwpp->l_lastran) < maxcachewarm) &&
				(!(lasteng->e_flags &  E_NOWAY)) &&
				(((lasteng->e_rqlist[0]->rq_srunlwps) - avgqlen) < maximbalance) &&
				(nidle == 0)) {
				/*
				 * cache is still warm, and processor
				 * not offline or going offline or bad,
				 * and no runqueue imbalance and no
				 * other idle processors.
				 */
				lwpp->l_rq = lwpp->l_eng->e_rqlist[0];
			} else
				lwpp->l_rq = global_rq;
		}
		RUNQUE_UNLOCK();
	}
#endif

	/*
	 * Put the lwp on the end of the run-queue.
	 */
	RUNQUE_LOCK();
	setbackrq(lwpp);
	qswtch(lwpp);

	return(0);
}


/*
 * STATIC void ts_audit(int flags, int cmd, int error, void *args,
 *			void *classdata)
 *	When the flag is ADTTST, this function checks if ADT_SCHED_TS event 
 *	is audited for the calling LWP. Otherwise, it dumps the audit record.
 *
 * Calling/Exit State:
 *	No lock must be held on entry and none held at exit.
 *
 */
/* ARGSUSED */
STATIC void
ts_audit(int flags, int cmd, int error, void *argp, void *classdata)
{
	tsadmin_t tsadmin;
	tsdpent_t *tmpdpp;
	int 	  tsdpsz;
	pcparms_t *pcparmsp;
	struct tsparms *tsparmsp;
	alwp_t *alwp = u.u_lwpp->l_auditp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(flags == ADTTST || flags == ADTDUMP);

	/* check if the event is being audited */
	if (flags == ADTTST) {
		ADT_SCHED_EVENTCK(ADT_SCHED_TS, alwp);
		return;
	}

	ASSERT(cmd == PC_SETPARMS || cmd == PC_ADMIN);

	/* dump the sched event record */
	switch (cmd) {
	case PC_SETPARMS:
		pcparmsp = argp;
		tsparmsp = (tsparms_t *) pcparmsp->pc_clparms;

		adt_parmsset(ADT_SCHED_TS, error, tsparmsp->ts_upri, 
			     tsparmsp->ts_uprilim);
		break;

	case PC_ADMIN:
		if (alwp && (EVENTCHK(ADT_SCHED_TS, alwp->al_emask->ad_emask))){
			SET_AUDITME(alwp);
			if (copyin(argp, &tsadmin, sizeof(tsadmin_t))) {
				adt_admin(ADT_SCHED_TS, EFAULT, 0, NULL);
				return;
			}
			if (tsadmin.ts_cmd != TS_SETDPTBL) 
				return;

			tsdpsz = (ts_maxumdpri + 1) * sizeof(tsdpent_t);
			if ((error == EFAULT) || (error == EPERM) || 
			    (tsadmin.ts_ndpents*sizeof(tsdpent_t) != tsdpsz)) {
				/* Dump the record with table size eq 0 */
				adt_admin(ADT_SCHED_TS,error,0, NULL);
				return;
			}

			tmpdpp = (tsdpent_t *)kmem_alloc(tsdpsz, KM_SLEEP);
			ASSERT(tmpdpp != NULL);
			copyin(tsadmin.ts_dpents, tmpdpp, tsdpsz);

			adt_admin(ADT_SCHED_TS, error, tsadmin.ts_ndpents, 
				  tmpdpp);
			kmem_free(tmpdpp, tsdpsz);
		}
	}
}
