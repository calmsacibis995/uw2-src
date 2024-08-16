/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/class/fc.c	1.16"
#ident	"$Header: $"

#include <acc/audit/audit.h>
#include <acc/priv/privilege.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/class/fc.h>
#include <proc/class/fcpriocntl.h>
#include <proc/class.h>
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
 * Class specific code for the fixed-class scheduling class
 */


/*
 * Extern declarations for variables defined in the fc master file
 */
extern fcdpent_t  fc_dptbl[];	/* fixed-class disp parameter table */
extern int	fc_kmdpris[];	/* array of global pris used by fc lwp's when */
				/*  sleeping or running in kernel after sleep */
extern short	fc_maxupri;	/* max fixed-class user priority */
extern short	fc_maxumdpri;	/* maximum user mode ts priority */
extern short	fc_maxkmdpri;	/* maximum kernel mode ts priority */

extern  int	load_lwp();

#ifndef UNIPROC
extern clock_t	maxcachewarm;
extern int	maximbalance;
extern int	fc_affinity_on;
extern int	avgqlen;
extern int	nidle;
#endif


#define NZERO           20   /* for nice */
#define P_FCLASS	P_TSHAR


#define	fcumdpri	(fclwpp->fc_umdpri)
#define	fcmedumdpri	(fc_maxumdpri >> 1)

#define	FC_NEWUMDPRI(fclwpp)	\
{ \
if (((fclwpp)->fc_umdpri = (fclwpp)->fc_cpupri + (fclwpp)->fc_upri) > fc_maxumdpri) \
	(fclwpp)->fc_umdpri = fc_maxumdpri; \
else if ((fclwpp)->fc_umdpri < 0) \
	(fclwpp)->fc_umdpri = 0; \
}

void		fc_init(id_t , int , classfuncs_t **, int *);
STATIC int	fc_admin(void * , id_t , cred_t *);
STATIC void	*fc_allocate(void *);
STATIC int	fc_cancelchange(lwp_t *, int, qpcparms_t *,
			pcparms_t *, void *, cred_t *, int *);
STATIC int	fc_changeparms(lwp_t *, int, id_t, pcparms_t *,
			void *, cred_t *, qpcparms_t **, int *);
STATIC int	fc_changeend(pcparms_t *, void *);
STATIC int	fc_changestart(pcparms_t *, void **);
STATIC void	fc_deallocate(void *);
STATIC void	fc_enterclass(long *, lwp_t *, lwpstat_t *,
			int *, uint *, cred_t **,void **, void *);
STATIC void	fc_exitclass(lwp_t *);
STATIC int	fc_fork(void *, lwp_t *, lwpstat_t *,
			int *, uint *, cred_t **, void **);
STATIC int	fc_getclinfo();
STATIC void	fc_insque(lwp_t *, list_t *, int);
STATIC int	fc_lwpcmp(void *, long *);
STATIC void	fc_parmsget(void *, long *);
STATIC int	fc_parmsin(long *);
STATIC int	fc_parmsout(long *);
STATIC void	fc_parmsset(long *, void *, void *);
STATIC void	fc_preempt(void *);
STATIC int	fc_rdblock(lwp_t *, list_t *);
STATIC void	fc_setrun(void *);
STATIC void	fc_tick(void *);
STATIC void	fc_wakeup(void *, int );
STATIC void	fc_audit(int, int, int, void *, void *);

STATIC int	fc_nosys();
STATIC void	fc_nullsys();

void		fc_donice();
STATIC void	fc_trapret();

id_t	fc_cid;		/* fixed-class class ID */
STATIC int	fc_maxglobpri;	/* maximum global priority used by fc class */
STATIC int fc_prepblock(lwp_t *, sq_t *, int);
STATIC int fc_urdblock(lwp_t *, sq_t *, int *);
STATIC int fc_yield(void *, int);
#define	fc_cancelblock	sq_cancelblock
#define fc_unblock sq_unblock
#define fc_block sq_block


STATIC struct classfuncs fc_classfuncs = {
	fc_admin,
	fc_enterclass,
	fc_exitclass,
	fc_fork,
	fc_nullsys,	/* forkret */
	fc_getclinfo,
	fc_nullsys,	/* getglobpri */
	fc_parmsget,
	fc_parmsin,
	fc_parmsout,
	fc_parmsset,
	fc_preempt,
	fc_lwpcmp,
	fc_setrun,
	fc_nullsys,	/* sleep */
	fc_nullsys,	/* stop */
	fc_nullsys,	/* swapin */
	fc_nullsys,	/* swapout */
	fc_tick,
	fc_trapret,
	fc_wakeup,
	fc_insque,
	fc_rdblock,
	fc_allocate,
	fc_deallocate,
	fc_changestart,
	fc_changeparms,
	fc_cancelchange,
	fc_changeend,
	fc_nullsys,
	fc_nosys,
	fc_prepblock,
	fc_block,
	fc_unblock,
	fc_cancelblock,
	fc_urdblock,
	fc_yield,
	fc_audit,
};


/*
 * void
 * fc_init(id_t cid, int clparmsz, classfuncs_t **clfuncspp, int *maxglobprip)
 *	FC class initialization routine.
 * 
 * Calling/Exit State:
 *
 *	Called at boot time. No locking requirements for caller.
 *
 * Description:
 *	
 *	Fixed class initialization.  Called by dispinit()
 *	at boot time. Records the maximum global priority in FC
 *	class, and class ID. Sets a pointer to the class specific
 *	function vector and the maximum global priority
 *	within this class.
 *
 */
/*ARGSUSED*/
void
fc_init(id_t cid, int clparmsz, classfuncs_t **clfuncspp, int *maxglobprip)
{

	fc_maxglobpri = max(fc_kmdpris[fc_maxkmdpri],
	    fc_dptbl[fc_maxumdpri].fc_globpri);

	fc_cid = cid;		/* Record our class ID */


	/*
	 * We're required to return a pointer to our classfuncs
	 * structure and the highest global priority value we use.
	 */
	*clfuncspp = &fc_classfuncs;
	*maxglobprip = fc_maxglobpri;
}


/*
 * int
 * fc_admin(void * uaddr, id_t reqpcid, cred_t *reqcredp)
 *
 * 	Get or reset the fc_dptbl values per the user's request.
 *
 * Calling/Exit State:
 *
 *	Called from priocntl after acquiring the priocntl_update
 *	sleep lock. No other lock is required to be held. Function
 *	returns with priocntl_update lock still held.
 *
 * Description:
 *
 * 	fc_admin() is used to maintain and update the FC dispatcher
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
fc_admin(void * uaddr, id_t reqpcid, cred_t *reqcredp)
{
	fcadmin_t	fcadmin;
	fcdpent_t	*tmpdpp;
	int		userdpsz;
	int		i;
	int		fcdpsz;

	if (copyin(uaddr, (caddr_t)&fcadmin, sizeof(fcadmin_t)))
		return(EFAULT);

	fcdpsz = (fc_maxumdpri + 1) * sizeof(fcdpent_t);

	switch(fcadmin.fc_cmd) {

	case FC_GETDPSIZE:

		fcadmin.fc_ndpents = fc_maxumdpri + 1;
		if (copyout((caddr_t)&fcadmin, uaddr, sizeof(fcadmin_t)))
			return(EFAULT);
		break;

	case FC_GETDPTBL:

		userdpsz = MIN(fcadmin.fc_ndpents * sizeof(fcdpent_t), fcdpsz);
		if (copyout((caddr_t)fc_dptbl,
		    (caddr_t)fcadmin.fc_dpents, userdpsz))
			return(EFAULT);

		fcadmin.fc_ndpents = userdpsz / sizeof(fcdpent_t);
		if (copyout((caddr_t)&fcadmin, uaddr, sizeof(fcadmin_t)))
			return(EFAULT);

		break;

	case FC_SETDPTBL:

		/*
		 * We require that the requesting lwp have privilege.
		 * We also require that the table supplied by the user
		 * exactly match the current fc_dptbl in size.
		 */
		if (pm_denied(reqcredp, P_SYSOPS)
		    && pm_denied(reqcredp, P_FCLASS))
			return(EPERM);
		if (fcadmin.fc_ndpents * sizeof(fcdpent_t) != fcdpsz)
			return(EINVAL);

		/*
		 * We read the user supplied table into a temporary buffer
		 * where it is validated before being copied over the
		 * fc_dptbl.
		 */
		tmpdpp = (fcdpent_t *)kmem_alloc(fcdpsz, KM_SLEEP);
		ASSERT(tmpdpp != NULL);
		if (copyin((caddr_t)fcadmin.fc_dpents, (caddr_t)tmpdpp,
		    fcdpsz)) {
			kmem_free(tmpdpp, fcdpsz);
			return(EFAULT);
		}
		for (i = 0; i < fcadmin.fc_ndpents; i++) {

			/*
			 * Validate the user supplied values.  All we are doing
			 * here is verifying that the values are within their
			 * allowable ranges and will not panic the system.  We
			 * make no attempt to ensure that the resulting
			 * configuration makes sense or results in reasonable
			 * performance.
			 */
			if (tmpdpp[i].fc_quantum <= 0) {
				kmem_free(tmpdpp, fcdpsz);
				return(EINVAL);
			}
		}

		/*
		 * Copy the user supplied values over the current fc_dptbl
		 * values.  The fc_globpri member is read-only so we don't
		 * overwrite it.
		 */
		for (i = 0; i < fcadmin.fc_ndpents; i++)
			fc_dptbl[i].fc_quantum = tmpdpp[i].fc_quantum;

		kmem_free(tmpdpp, fcdpsz);
		break;

	default:
		return(EINVAL);
	}
	return(0);
}


/*
 * int
 * fc_enterclass(long *fcparmsp_arg, lwp_t *lwpp, lwpstat_t *lstatp,
 *		int *lprip, uint *lflagp, cred_t **lcredpp,
 *		void **fclwppp, void *classdata)
 *
 *	Initialize FC class-specific lwp structure.
 *
 * Calling/Exit State:
 *
 *	Caller must hold lwp_mutex of the target lwp.
 *
 * Description:
 *
 *	Allocation of fixed-class class specific lwp structure is
 *	presumed to have been completed via an earlier call to
 *	to fc_allocate. This function initializes it with the
 *	parameters supplied or with default parameters.
 *	Also, move lwp to specified fixed-class priority.
 */
/* ARGSUSED */
STATIC void
fc_enterclass(long *fcparmsp_arg, lwp_t *lwpp, lwpstat_t *lstatp,
		int *lprip, uint *lflagp, cred_t **lcredpp,
		void **fclwppp, void *classdata)
{
	fcparms_t *fcparmsp = (fcparms_t *)fcparmsp_arg;
	fclwp_t	*fclwpp;
	short		reqtsuprilim;
	short		reqtsupri;
	int			oldpri;

	fclwpp = (fclwp_t *)*fclwppp;

	/*
	 * Initialize the fclwp structure.
	 */
	if (fcparmsp == NULL) {
		/*
		 * Use default values.
		 */
		fclwpp->fc_uprilim = fclwpp->fc_upri = 0;
		fclwpp->fc_nice = 20;
		fclwpp->fc_umdpri = fclwpp->fc_cpupri = fcmedumdpri;
	} else {
		/*
		 * Use supplied values.
		 */
		if (fcparmsp->fc_uprilim == FC_NOCHANGE)
			reqtsuprilim = 0;
		else
			reqtsuprilim = fcparmsp->fc_uprilim;

		if (fcparmsp->fc_upri == FC_NOCHANGE) {
			reqtsupri = reqtsuprilim;
		} else {
			/*
			 * Set the user priority to the requested value
			 * or the upri limit, whichever is lower.
			 */
			reqtsupri = fcparmsp->fc_upri;
			if (reqtsupri > reqtsuprilim)
				reqtsupri = reqtsuprilim;
		}

		fclwpp->fc_uprilim = reqtsuprilim;
		fclwpp->fc_upri = reqtsupri;
		fclwpp->fc_nice = 20 - (20 * reqtsupri) / fc_maxupri;
		fclwpp->fc_cpupri = fcmedumdpri;
		FC_NEWUMDPRI(fclwpp);
	}

	fclwpp->fc_flags = 0;
	fclwpp->fc_lwpp = lwpp;
	fclwpp->fc_lstatp = lstatp;
	fclwpp->fc_lprip = lprip;
	fclwpp->fc_lflagp = lflagp;

	/*
	 * Reset priority. Lwp goes to a "user mode" priority
	 * here regardless of whether or not it has slept since
	 * entering the kernel.
	 */

	fclwpp->fc_timeleft = fc_dptbl[fcumdpri].fc_quantum;
	oldpri = fclwpp->fc_lwpp->l_pri;
	fclwpp->fc_lwpp->l_pri = fc_dptbl[fcumdpri].fc_globpri;

	/*
	 * In order to update e_pri field, eng_tbl_mutex needs to
	 * be held. But this is not possible as lwp_mutex is already
	 * held and holding eng_tbl_mutex would violate the locking
	 * order. It is true that all routines that examine e_pri
	 * hold runque-lock. Thus it is sufficient to hold runque
	 * while updating e_pri.
	 */
	RUNQUE_LOCK();
	l.eng->e_pri = fclwpp->fc_lwpp->l_pri;
	RUNQUE_UNLOCK();
	if (fclwpp->fc_lwpp->l_pri < oldpri) {
		RUNQUE_LOCK();
		ipreemption(fclwpp->fc_lwpp, nudge);
		RUNQUE_UNLOCK();
		fclwpp->fc_flags |= FCBACKQ;
	}

	return;
}


/*
 * void
 * fc_exitclass(lwp_t *cllwpp)
 *
 * 	Do final processing for an exiting FC lwp. There is nothing
 *	to be done.
 *
 * Calling/Exit State:
 *
 *	Called with the lwp_mutex held. Returns with the lock still held.
 *
 * Description:
 *
 *	fc_exitclass() does nothing.
 */
/* ARGSUSED */
STATIC void
fc_exitclass(lwp_t *cllwpp)
{
	return;
}


/*
 * void *
 * fc_allocate(void * clwpp)
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
 *	the FC scheduling class. Memory allocation is done with
 *	KM_SLEEP.
 */
/* ARGSUSED */
STATIC void *
fc_allocate(void * clwpp)
{
	struct fclwp *fclwpp;

	fclwpp = (fclwp_t *)kmem_zalloc(sizeof(fclwp_t), KM_SLEEP);

	return (fclwpp);
}


/*
 * void
 * fc_deallocate(void * clwpp_arg)
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
 *	earlier call to fc_allocate(). It simply deallocates the
 *	fclwp structure associated with the target lwp.
 */
STATIC void
fc_deallocate(void * clwpp_arg)
{
	fclwp_t *clwpp = (fclwp_t *)clwpp_arg;

	kmem_free(clwpp, sizeof(fclwp_t));
}


/*
 * int
 * fc_fork(void *pfclwpp_arg, lwp_t *clwpp, lwpstat_t *clstatp,
 *		int *clprip, uint *clflagp, cred_t **clcredpp,
 *		void **cllwpp)
 *
 *	Initialize FC class specific lwp structure for child.
 *
 * Calling/Exit State:
 *
 *	Called with the lwp_mutex of child and parent lwp held.
 *	Also, p_mutex of the child process is held.
 *	Returns with all mutexes held.
 *
 * Description:
 *
 *	fc_fork() is called to add the lwp pointed to by clwpp to
 *	the scheduling class of the parent (FC).
 *	The child fclwp structure is initialized.
 *	The child is then placed on the run queue
 *	and the parent just continues.
 */
/* ARGSUSED */
STATIC int
fc_fork(void *pfclwpp_arg, lwp_t *clwpp, lwpstat_t *clstatp,
	int *clprip, uint *clflagp, cred_t **clcredpp,
	void **cllwpp)
{
	fclwp_t *pfclwpp = (fclwp_t *)pfclwpp_arg;
	struct fclwp	*fclp = (fclwp_t *)*cllwpp;

	/*
	 * Initialize child's fclwp structure.
	 */
	fclp->fc_timeleft = fc_dptbl[pfclwpp->fc_umdpri].fc_quantum;
	fclp->fc_umdpri = pfclwpp->fc_umdpri;
	fclp->fc_cpupri = pfclwpp->fc_cpupri;
	fclp->fc_uprilim = pfclwpp->fc_uprilim;
	fclp->fc_upri = pfclwpp->fc_upri;
	fclp->fc_nice = pfclwpp->fc_nice;
	fclp->fc_flags = pfclwpp->fc_flags & ~FCBACKQ;
	fclp->fc_lwpp = clwpp;
	fclp->fc_lstatp = clstatp;
	fclp->fc_lprip = clprip;
	fclp->fc_lflagp = clflagp;

	pfclwpp->fc_flags |= (FCBACKQ|FCFORK);

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
 * fc_getclinfo(fcinfo_t *fcinfop, id_t reqpcid, cred_t *reqcredp)
 *
 *	Get FC class info into a buffer.
 *
 * Calling/Exit State:
 *
 *	Called without any special locks held. Returns without
 *	any locks held.
 *
 * Description:
 *
 * 	Returns information about the fc class into the buffer
 *	pointed to by fcinfop. The maximum configured user
 *	priority is the only information we supply.  We ignore
 *	the class and credential arguments because anyone can have
 *	this information.
 */
/* ARGSUSED */
STATIC int
fc_getclinfo(fcinfo_t *fcinfop, id_t reqpcid, cred_t *reqcredp)
{
	fcinfop->fc_maxupri = fc_maxupri;
	return(0);
}


/*
 * int
 * fc_nosys()
 *
 *	Undefined function for FC scheduling class
 *
 * Calling/Exit State:
 *
 *	There are no locking requirements.
 *
 * Description:
 *
 *	This function is used for those generic class switch functions
 *	for which there is no support in the FC class. It returns
 *	the error code ENOSYS.
 */
STATIC int
fc_nosys()
{
	return(ENOSYS);
}


/*
 * void
 * fc_nullsys()
 *
 *	A null (no op) function for FC scheduling class.
 *
 * Calling/Exit State:
 *
 *	There are no locking requirements.
 *
 * Description:
 *
 *	This function is used for those generic class switch functions
 *	for which there is no special FC class specific action. However,
 *	it not an error to call this function. It simply returns success.
 */
STATIC void
fc_nullsys()
{
}


/*
 * void
 * fc_parmsget(void *fclwpp_arg, long *fcparmsp_arg)
 *
 *	Get the fixed-class parameters of a lwp.
 *
 * Calling/Exit State:
 *
 *	Caller must hold the lwp-mutex of the lwp.
 *	Function returns with the lock still held.
 *
 * Description:
 *
 *	This returns certain class specific attributes
 *	of the lwp pointed to by fclwpp into the buffer
 *	pointed to by fcparmsp_arg.
 */
STATIC void
fc_parmsget(void *fclwpp_arg, long *fcparmsp_arg)
{
	fclwp_t *fclwpp = (fclwp_t *)fclwpp_arg;
	fcparms_t *fcparmsp = (fcparms_t *)fcparmsp_arg;

	fcparmsp->fc_uprilim = fclwpp->fc_uprilim;
	fcparmsp->fc_upri = fclwpp->fc_upri;
}


/*
 * int
 * fc_parmsin(long *fcparmsp_arg)
 *
 *	Check validity of fixed-class parameters in the buffer.
 *
 * Calling/Exit State:
 *
 *	Caller need not hold any locks to call this function.
 *
 * Description:
 *
 *	Check the validity of the fixed-class parameters in the
 *	buffer pointed to by fcparmsp.
 */
/* ARGSUSED */
STATIC int
fc_parmsin(long *fcparmsp_arg)
{
	fcparms_t *fcparmsp = (fcparms_t *)fcparmsp_arg;

	/*
	 * Check validity of parameters.
	 */
	if ((fcparmsp->fc_uprilim > fc_maxupri ||
	    fcparmsp->fc_uprilim < -fc_maxupri) &&
	    fcparmsp->fc_uprilim != FC_NOCHANGE)
		return(EINVAL);

	if ((fcparmsp->fc_upri > fc_maxupri || fcparmsp->fc_upri < -fc_maxupri) &&
	    fcparmsp->fc_upri != FC_NOCHANGE)
		return(EINVAL);

	return(0);
}


/*
 * int
 * fc_parmsout(long *fcparmsp_arg)
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
 *	For FC class, there is no conversion necessary, hence  this
 *	is a no op.
 */
/* ARGSUSED */
STATIC int
fc_parmsout(long *fcparmsp_arg)
{
	return(0);
}


/*
 * void
 * fc_parmsset(long *cllwpp, void *clparmsp, void *argp)
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
 *	Set the scheduling parameters of the lwp pointed to by fclwpp
 *	to those specified in the buffer pointed to by fcparmsp.
 *	Presently, this function is not called by the class
 *	independent code (all parameter changes are performed in
 *	fc_changeparms itself and nothing is deferred, except the
 *	scheduling class change).
 */
/* ARGSUSED */
STATIC void
fc_parmsset(long *cllwpp, void *clparmsp, void *argp)
{
	/*
	 *+ The fixed class scheduling detected incorrect processing
	 *+ of a scheduling parameter change.  This is a basic kernel
	 *+ problem and cannot be corrected by an administrator.
	 */
	cmn_err(CE_PANIC, "FC scheduling class deferred a parameter change");
	return;
}


/*
 * int
 * fc_rdblock(lwp_t *lwpp, list_t *lp)
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
 *
 *	Since FC class does FIFO queueing, if the first lwp on the
 *	queue is a writer, then the lwp must block. Otherwise, it
 *	can be granted the lock.
 */
/* ARGSUSED */
int
fc_rdblock(lwp_t *lwpp, list_t *lp)
{
	/*
	 * If anyone is queued, this lwp must be queued.
	 */
	return !EMPTYQUE(lp);
}


/*
 * void
 * fc_preempt(void *fclwpp_arg)
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
 *	If fc_preempt is called in response to a user binding of an
 *	SONPROC lwp, preemption() must be called to notify the
 *	processor to which binding is taking place. The flag is
 *	also reset.
 */
STATIC void
fc_preempt(void *fclwpp_arg)
{
	fclwp_t *fclwpp = (fclwp_t *)fclwpp_arg;
	lwp_t *lwpp = fclwpp->fc_lwpp;

	(void)LOCK(&lwpp->l_mutex, PLHI);
	lwpp->l_stat = SRUN;

	if (LWP_SEIZED(fclwpp->fc_lwpp)) {
		/*
		 * If we're seized, we simply call
		 * swtch.  Swtch knows how to handle things.
		 */
		swtch(lwpp);
		return;
	}


#ifndef UNIPROC
	if (fc_affinity_on) {
		if ((lwpp->l_kbind == NULL) && (lwpp->l_xbind == NULL) &&
			(lwpp->l_ubind == NULL) &&
			(lwpp->l_eng != NULL)) { /* cannot affinitize if hard-bound */
			engine_t *lasteng = lwpp->l_eng;
			int runqlwps = lasteng->e_rqlist[0]->rq_srunlwps;

			RUNQUE_LOCK();
			if (((runqlwps - avgqlen) < maximbalance) &&
				(!((runqlwps > 0) && (nidle > 0))))
				/*
				 * No imbalance.
				 */
				lwpp->l_rq = lwpp->l_eng->e_rqlist[0];
			else
				lwpp->l_rq = global_rq;
			RUNQUE_UNLOCK();
		}
	}
#endif


	switch (fclwpp->fc_flags & (FCBACKQ|FCKPRI|FCFORK)) {
		case FCBACKQ:
			fclwpp->fc_timeleft = fc_dptbl[fcumdpri].fc_quantum;
			fclwpp->fc_flags &= ~FCBACKQ;
			RUNQUE_LOCK();
			setbackrq(lwpp);
			break;
		case (FCBACKQ|FCKPRI):
			fclwpp->fc_flags &= ~FCBACKQ;
			RUNQUE_LOCK();
			setbackrq(lwpp);
			break;
		case (FCBACKQ|FCFORK):
			fclwpp->fc_flags &= ~(FCBACKQ|FCFORK);
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
	 * to see if the run time invariant is still maintained.
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
 * fc_lwpcmp(void *fclwp1p_arg, long *fcparmsp_arg)
 *
 * 	Compare scheduling priorities of two lwp's
 *
 * Calling/Exit State:
 *
 * 	Called from priocntl with the per-process and per-lwp mutex of
 *	fclwp2p_arg held.
 *
 * Description
 *
 *	fc_lwpcmp() is part of the implementation of the PC_GETPARMS
 *	command of the priocntl system call. When the user specifies
 *	multiple lwp's to priocntl PC_GETPARMS the criteria
 *	for selecting a lwp from the set is class specific. The
 *	criteria used by the fixed-class is the upri value
 *	of the lwp. fc_lwpcmp() simply compares the upri values of current
 *	lwp with the parameters associated with a previously chosen lwp.
 *	All the ugly work of looping through the lwp's in the set is done
 *	by higher level (class independent) functions.
 */
STATIC int
fc_lwpcmp(void *fclwp1p_arg, long *fcparmsp_arg)
{
	fclwp_t *fclwp1p = (fclwp_t *)fclwp1p_arg;
	fcparms_t *fclwp2parmsp = (fcparms_t *)fcparmsp_arg;

	return(fclwp1p->fc_upri - fclwp2parmsp->fc_upri);
}


/*
 * void
 * fc_setrun(void *fclwpp_arg)
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
 *	The lwp pointed to by fclwpp is placed on the
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
fc_setrun(void *fclwpp_arg)
{
	fclwp_t *fclwpp = (fclwp_t *)fclwpp_arg;
	lwp_t *lwpp;
	engine_t *engp = NULL;

	lwpp = fclwpp->fc_lwpp;
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
			if ((fc_affinity_on == 1) &&
				((lbolt - lwpp->l_lastran) < maxcachewarm) &&
				(!(lasteng->e_flags &  E_NOWAY)) &&
				(((lasteng->e_rqlist[0]->rq_srunlwps) - avgqlen) < maximbalance) &&
				(nidle == 0)) {
				/*
				 * cache is still warm, and processor
				 * not offline or going offline or bad,
				 * and no runqueue imbalance.
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


	if ((fclwpp->fc_flags & FCKPRI) == 0) {
		fclwpp->fc_timeleft = fc_dptbl[fcumdpri].fc_quantum;
	}
	fclwpp->fc_flags &= ~(FCBACKQ|FCFORK);

	RUNQUE_LOCK();
	setbackrq(lwpp);
	preemption(lwpp, nudge, 0, engp);
	RUNQUE_UNLOCK();
}


/*
 * void
 * fc_insque(lwp_t *lwpp, list_t *lp, int priority)
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
 *	fc_insque places the lwp on the blocking queue pointed to by lp.
 *	For FC class, the queueing order is FIFO.
 *	The priority of the lwp is reset so that it will run
 *	at the requested priority (as specified by the third argument)
 *	when it wakes up. The sense of this disp argument is reversed
 *	compared to SVR4. i.e., higher disp value means higher kernel
 *	mode priority when the lwp wakes up.
 */
/* ARGSUSED */
STATIC void
fc_insque(lwp_t *lwpp, list_t *lp, int disp)
{
	struct fclwp *p;

	p = (fclwp_t *)(lwpp->l_cllwpp);

	p->fc_flags |= FCKPRI;
	ASSERT(disp >= 0 && disp <= fc_maxkmdpri);
	lwpp->l_pri = fc_kmdpris[disp];

	/* Now, enqueue it in fifo order */
	insque(lwpp, lp->rlink);
}


/*
 * void
 * fc_tick(void *fclwpp_arg)
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
 *	If the time left in quantum is zero, it simply puts the lwp
 *	on the appropriate dispatcher queue without recomputing the
 *	new priority (since priorities are fixed).
 */

STATIC void
fc_tick(void *fclwpp_arg)
{
	fclwp_t *fclwpp = (fclwp_t *)fclwpp_arg;

	if ((fclwpp->fc_flags & FCKPRI) != 0) {
		/*
		 * No time slicing of lwps at kernel mode priorities.
		 */
		return;
	}

	if (--fclwpp->fc_timeleft == 0) {	

		fclwpp->fc_flags |= FCBACKQ;

		/* cause lwp to give up processor */
		RUNQUE_LOCK();
		nudge(fclwpp->fc_lwpp->l_pri, l.eng);
		RUNQUE_UNLOCK();
	}
}


/*
 * void
 * fc_trapret(fclwp_t *fclwpp)
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
fc_trapret(fclwp_t *fclwpp)
{
	pl_t pl;
	int newpri;

	if ((fclwpp->fc_flags & FCKPRI) == 0) {
		return;
	}

	pl = LOCK(&fclwpp->fc_lwpp->l_mutex, PLHI);
	fclwpp->fc_flags &= ~FCKPRI;
	fclwpp->fc_flags |= FCBACKQ;
	newpri = fc_dptbl[fcumdpri].fc_globpri;
	*fclwpp->fc_lprip = newpri;
	fclwpp->fc_timeleft = fc_dptbl[fcumdpri].fc_quantum;

	RUNQUE_LOCK();
	ipreemption(fclwpp->fc_lwpp, nudge);
	RUNQUE_UNLOCK();
	UNLOCK(&fclwpp->fc_lwpp->l_mutex, pl);

	dispmdepnewpri(newpri);
}


/*
 * void
 * fc_wakeup(void *fclwpp_arg, int preemptflg)
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
fc_wakeup(void *fclwpp_arg, int preemptflg)
{
	fclwp_t *fclwpp = (fclwp_t *)fclwpp_arg;
	lwp_t *lwpp;
	engine_t *engp = NULL;

	lwpp = fclwpp->fc_lwpp;
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
			if ((fc_affinity_on == 1) &&
				((lbolt - lwpp->l_lastran) < maxcachewarm) &&
				(!(lasteng->e_flags &  E_NOWAY)) &&
				(((lasteng->e_rqlist[0]->rq_srunlwps) - avgqlen) < maximbalance) &&
				(nidle == 0)) {
				/*
				 * cache is still warm, and processor
				 * not offline or going offline or bad,
				 * and no runqueue imbalance.
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


	fclwpp->fc_flags &= ~(FCBACKQ|FCFORK);
	RUNQUE_LOCK();
	setbackrq(lwpp);
	preemption(lwpp, nudge, preemptflg, engp);
	RUNQUE_UNLOCK();
}


/*
 * int
 * fc_changeparms(lwp_t *lwpp, int cmd, id_t cid, pcparms_t *pcparmsp,
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
 *	For FC class, any request for change in 'upri' or 'uprilim'
 *	can be accommodated right away. If it is request for
 *	scheduling class change, that must be deferred and this
 *	pending change is indicated by setting a flag in the
 *	lwp structure.
 */
/* ARGSUSED */
int
fc_changeparms(lwp_t *lwpp, int cmd, id_t cid,  pcparms_t *pcparmsp,
	       void *classdata, cred_t *reqcredp, qpcparms_t **qpcparmspp,
	       int *donep)
{
	struct fcparms *fcparmsp;
	short	reqtsuprilim;
	short	reqtsupri;
	char nice;
	fclwp_t *fclwpp = lwpp->l_cllwpp;

	if (cmd == CHANGEPARMS_FORK) {
		/*
		 * Changeparms from parmsprop.  The caller doesn't
		 * really expect us to do anything, just return the
		 * buffer.
		 */
		*qpcparmspp = &fclwpp->fc_qpcparms;
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
		 * For FC class, the only other changes that can be
		 * requested are upri and/or uprilim.
		 * These can be applied immediately.
		 */
		fcparmsp = (fcparms_t *)pcparmsp->pc_clparms;

		if (fcparmsp->fc_uprilim == FC_NOCHANGE)
			reqtsuprilim = fclwpp->fc_uprilim;
		else
			reqtsuprilim = fcparmsp->fc_uprilim;
	
		if (fcparmsp->fc_upri == FC_NOCHANGE)
			reqtsupri = fclwpp->fc_upri;
		else
			reqtsupri = fcparmsp->fc_upri;
	
		/*
		 * Make sure the user priority doesn't exceed the upri limit.
		 */
		if (reqtsupri > reqtsuprilim)
			reqtsupri = reqtsuprilim;

		if (reqtsuprilim > fclwpp->fc_uprilim &&
		    pm_denied(reqcredp, P_FCLASS))
			/*
			 * You can only raise your limit if you're the super
			 * user.
			 */
			return(EPERM);

		fclwpp->fc_uprilim = reqtsuprilim;
		fclwpp->fc_upri = reqtsupri;
		FC_NEWUMDPRI(fclwpp);
	
		/*
		 * Set fc_nice to the nice value corresponding to the user
		 * priority we are setting.
		 */
		nice = 20 - (fcparmsp->fc_upri * 20) / fc_maxupri;
		if (nice == 40)
			nice = 39;
		fclwpp->fc_nice = nice;


		/*
		 * Determine if the lwp is at a kernel-mode priority.
		 * We don't fuss with the priority of an lwp at a kernel-mode
		 * priority.
		 */
		if ((fclwpp->fc_flags & FCKPRI) == 0) {
			/*
			 * The lwp is not at a kernel-mode priority and
			 * we can directly change the priority.
			 */
			fclwpp->fc_timeleft = fc_dptbl[fcumdpri].fc_quantum;
			if (dispnewpri(lwpp, fc_dptbl[fcumdpri].fc_globpri) ==
				SONPROC) {
				/*
				 * Make sure the lwp gets put on the
				 * end of the queue.
				 */
				fclwpp->fc_flags |= FCBACKQ;
			}
		}
		*donep = 1;
	} else {
		/*
		 * Scheduling class change.
		 */
		if (cmd == CHANGEPARMS_CHECKONLY) {
			fcparmsp = (fcparms_t *)pcparmsp->pc_clparms;
	
			/*
			 * Only the privileged user can set upri limit
			 * above zero.
			 */
			if (fcparmsp->fc_uprilim > 0 && pm_denied(reqcredp, P_FCLASS))
				return(EPERM);
		}

		if (qpcparmspp != NULL)
			*qpcparmspp = &fclwpp->fc_qpcparms;
		*donep = 0;
	}
	return (0);
}

/*
 * int fc_cancelchange(lwp_t *lwpp, int cmd, qpcparms_t *qpcparmsp,
 *		       pcparms_t *pcparmsp, void *classdata, cred_t *reqcredp,
 *		       int *combinedp)
 *
 *	Deal with a FC queued change relative to a new change.
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
fc_cancelchange(lwp_t *lwpp, int cmd, qpcparms_t *qpcparmsp,
		pcparms_t *pcparmsp, void *classdata, cred_t *reqcredp,
		int *combinedp)
{
	fcparms_t *oldfcparmsp, *newfcparmsp;
	short		reqfcuprilim;
	short		reqfcupri;

	*combinedp = 0;

	/*
	 * Check for permission.
	 */
	if (!hasprocperm(lwpp->l_cred, reqcredp)) {
		return EPERM;
	}

	ASSERT(qpcparmsp->qpc_pcparms.pc_cid == fc_cid);

	switch (cmd) {
	case CANCELCHANGE_TRYCOMBINE:
		ASSERT(pcparmsp->pc_cid == qpcparmsp->qpc_pcparms.pc_cid);

		/*
		 * Only priorities need to readjusted. This is OK.
		 * The new upri and uprilim values have already been
		 * checked for sanity by priocntl and will be check
		 * here for permissions only.
		 */
		oldfcparmsp = (fcparms_t *)qpcparmsp->qpc_pcparms.pc_clparms;
		newfcparmsp = (fcparms_t *)pcparmsp->pc_clparms;

		if (newfcparmsp->fc_uprilim == FC_NOCHANGE)
			reqfcuprilim = oldfcparmsp->fc_uprilim;
		else
			reqfcuprilim = newfcparmsp->fc_uprilim;
	
		if (newfcparmsp->fc_upri == FC_NOCHANGE)
			reqfcupri = oldfcparmsp->fc_upri;
		else
			reqfcupri = newfcparmsp->fc_upri;
	
		/*
		 * Make sure the user priority doesn't exceed the upri limit.
		 */
		if (reqfcupri > reqfcuprilim)
			reqfcupri = reqfcuprilim;

		/*
		 * If the lwp is "currently" in the FC scheduling
		 * class, only the privileged user can raise its
		 * upri limit.
		 */
		if (reqfcuprilim > oldfcparmsp->fc_uprilim &&
		    pm_denied(reqcredp, P_FCLASS))
			/*
			 * You can only raise your limit if you're the super
			 * user.
			 */
			return(EPERM);

		oldfcparmsp->fc_uprilim = reqfcuprilim;
		oldfcparmsp->fc_upri = reqfcupri;

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
 * fc_changestart(pcparms_t *pcparmsp, void **argpp)
 *
 *	Function that generally deals with some priocntl related
 *	initializations for scheduling class changes (not for FC).
 *
 * Calling/Exit State:
 *
 *	As this function is a no-op for FC class, there are no
 *	locking requirements.
 *
 * Description:
 *
 *	This function is intended to deal with some initializations
 *	for scheduling classes that deal with aggregations of lwp's.
 *	Since FC class does not deal with such aggregations, this function
 *	is a no op.
 */
/* ARGSUSED */
int
fc_changestart(pcparms_t *pcparmsp, void **argpp)
{
	return(0);
}


/*
 * int
 * fc_changeend(pcparms_t *pcparmsp, void *argp)
 *
 *	Function that generally deals with some priocntl related
 *	end-of-scheduling class change operations (not for FC).
 *
 * Calling/Exit State:
 *
 *	As this function is a no-op for FC class, there are no
 *	locking requirements.
 *
 * Description:
 *
 *	This function is intended to deal with some final processing
 *	for scheduling classes that deal with aggregations of lwp's.
 *	Since FC class does not deal with such aggregations, this function
 *	is a no op.
 */
/* ARGSUSED */
int
fc_changeend(pcparms_t *pcparmsp, void *argp)
{
	return(0);
}

/*
 * int fc_prepblock(lwp_t *lwp, sq_t *sq, int rdwr)
 *
 *	Place the lwp on the end of the queue.
 *
 * Calling/Exit State:
 *
 *	Called with lwp->l_mutex and sq->sq_lock locked.
 */
STATIC int
fc_prepblock(lwp_t *lwp, sq_t *sq, int rdwr)
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
 * int fc_urdblock(lwp_t *lwp, sq_t *sq, int *ret)
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
 *	writer, rather than take the reader/writer semaphore.  Since
 *	FC does all its queuing FIFO, and the caller wouldn't be
 *	calling unless a reader held the lock, we can conclude the
 *	reader would block behind a writer if there is anyone else
 *	on the queue.
 */
/* ARGSUSED */
STATIC int
fc_urdblock(lwp_t *lwp, sq_t *sq, int *ret)
{
	ASSERT(LOCK_OWNED(&lwp->l_mutex));

	*ret = 0;
	if (sq->sq_head)
		/*
		 * Somebody's on the list.  If somebody's on the list,
		 * then we can conclude this is a writer (as it is known
		 * that a reader already holds the sync-queue).  Since
		 * queuing in the FC class if FIFO, we'll be queued after
		 * this lwp, hence, we must block.
		 */
		*ret = 1;
	return(0);
}

/*
 * void fc_yield(flag)
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
fc_yield(void *vfclwpp, int flag)
{
	fclwp_t *fclwpp = vfclwpp;
	lwp_t *lwpp = fclwpp->fc_lwpp;

	(void)LOCK(&lwpp->l_mutex, PLHI);
	lwpp->l_stat = SRUN;

	if (flag) {
		/*
		 * Perform the "end of quantum" action.
		 */
		fclwpp->fc_timeleft = fc_dptbl[fcumdpri].fc_quantum;
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
			if ((fc_affinity_on == 1) &&
				((lbolt - lwpp->l_lastran) < maxcachewarm) &&
				(!(lasteng->e_flags &  E_NOWAY)) &&
				(((lasteng->e_rqlist[0]->rq_srunlwps) - avgqlen) < maximbalance) &&
				(nidle == 0)) {
				/*
				 * cache is still warm, and processor
				 * not offline or going offline or bad,
				 * and no runqueue imbalance.
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
 * STATIC void fc_audit(int flags, int cmd, int error, void *args,
 *			void *classdata)
 *	When the flag is ADTTST, this function checks if ADT_SCHED_FC event 
 *	is audited for the calling LWP. Otherwise, it dumps the audit record.
 *
 * Calling/Exit State:
 *	No lock must be held on entry and none held at exit.
 *
 */
/* ARGSUSED */
STATIC void
fc_audit(int flags, int cmd, int error, void *args, void *classdata)
{
	alwp_t *alwp = u.u_lwpp->l_auditp;
	pcparms_t *pcparmsp;
	struct fcparms *fcparmsp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(flags == ADTTST || flags == ADTDUMP);

	/* check if the event is being audited */
	if (flags == ADTTST) {
		ADT_SCHED_EVENTCK(ADT_SCHED_FC, alwp);
		return;
	}

	ASSERT(cmd == PC_SETPARMS || cmd == PC_ADMIN);

	/* dump the sched event record */
	switch (cmd) {
	case PC_SETPARMS:
		pcparmsp = args;
		fcparmsp = (fcparms_t *) pcparmsp->pc_clparms;

		adt_parmsset(ADT_SCHED_FC, error, fcparmsp->fc_upri, 
			     fcparmsp->fc_uprilim);
		break;
	case PC_ADMIN:
	{
		fcdpent_t *tmpdpp;
		fcadmin_t fcadmin;
		int 	  fcdpsz;

		if (alwp && EVENTCHK(ADT_SCHED_FC, alwp->al_emask->ad_emask)) {
			SET_AUDITME(alwp);

			if (copyin(args, &fcadmin, sizeof(fcadmin_t))) {
				adt_admin(ADT_SCHED_FC, EFAULT, 0, NULL);
				return;
			}
			if (fcadmin.fc_cmd != FC_SETDPTBL) 
				return;


			fcdpsz = (fc_maxumdpri + 1) * sizeof(fcdpent_t);
			if ((error == EFAULT) || (error == EPERM) || 
			    (fcadmin.fc_ndpents*sizeof(fcdpent_t) != fcdpsz)) {
				/* Dump the record with table size eq 0 */
				adt_admin(ADT_SCHED_FC,error,0, NULL);
				return;
			}

			tmpdpp = (fcdpent_t *)kmem_alloc(fcdpsz, KM_SLEEP);

			ASSERT(tmpdpp != NULL);

			copyin(fcadmin.fc_dpents, tmpdpp, fcdpsz);

			adt_admin(ADT_SCHED_FC, error, fcadmin.fc_ndpents, 
				  tmpdpp);
			kmem_free(tmpdpp, fcdpsz);
		}
	}
	}
}
