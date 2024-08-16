/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/class/fp.c	1.44"
#ident	"$Header: $"

#include <acc/audit/audit.h>
#include <acc/priv/privilege.h>
#include <mem/kmem.h>
#include <proc/disp.h>
#include <proc/class/fppriocntl.h>
#include <proc/class/fpri.h>
#include <proc/class.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/proc_hier.h>
#include <proc/user.h>
#include <proc/usync.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/plocal.h>
#include <util/types.h>

/*
 * Extern declarations for variables declared in the Space.c file in fp.cf.
 */
extern fpdpent_t	fp_dptbl[];
extern short		fp_maxpri;

extern	void	preemption();
extern	void	ipreemption();
extern	int	copyin();
extern	int	copyout();
extern	void	nudge();
extern	int	pm_denied();
extern  int	max();
extern  int	load_lwp();
extern  void	kpnudge();

#ifndef UNIPROC
extern clock_t	maxcachewarm;
extern int	maximbalance;
extern int	fp_affinity_on;
extern int	avgqlen;
extern int	nidle;
#endif

#define SEC_TO_NSEC	1000000000	/* number of nanosecs in a sec */

/*
 * Class specific code for the fixed priority (FP) scheduling class
 */

void		fp_init(id_t , int , classfuncs_t **, int *);
STATIC int	fp_admin(void * , id_t , cred_t *);
STATIC void	*fp_allocate(void *);
STATIC int	fp_cancelchange(lwp_t *, int, qpcparms_t *,
			pcparms_t *, void *, cred_t *, int *);
STATIC int	fp_changeparms(lwp_t *, int, id_t, pcparms_t *,
			void *, cred_t *, qpcparms_t **, int *);
STATIC int	fp_changeend(pcparms_t *, void *);
STATIC int	fp_changestart(pcparms_t *, void **);
STATIC void	fp_deallocate(void *);
STATIC void	fp_enterclass(long *, lwp_t *, lwpstat_t *,
			int *, uint *, cred_t **,void **, void *);
STATIC void	fp_exitclass(lwp_t *);
STATIC int	fp_fork(void *, lwp_t *, lwpstat_t *,
			int *, uint *, cred_t **, void **);
STATIC int	fp_getclinfo();
STATIC void	fp_insque(lwp_t *, list_t *, int);
STATIC int	fp_lwpcmp(void *, long *);
STATIC void	fp_parmsget(void *, long *);
STATIC int	fp_parmsin(long *);
STATIC int	fp_parmsout(long *);
STATIC void	fp_parmsset(long *, void *, void *);
STATIC void	fp_preempt(void *);
STATIC int	fp_rdblock(lwp_t *, list_t *);
STATIC void	fp_setrun(void *);
STATIC void	fp_tick(void *);
STATIC void	fp_wakeup(void *, int );
STATIC void	fp_audit(int, int, int, void *, void *);

STATIC int	fp_nosys();
STATIC void	fp_nullsys();

id_t	fp_cid;		/* fixed priority class ID */

STATIC int fp_prepblock(lwp_t *, sq_t *, int);
STATIC int fp_urdblock(lwp_t *, sq_t *, int *);
STATIC int fp_yield(void *, int);
#define	fp_cancelblock	sq_cancelblock
#define fp_unblock sq_unblock
#define fp_block sq_block

STATIC struct classfuncs fp_classfuncs = {
	fp_admin,
	fp_enterclass,
	fp_exitclass,
	fp_fork,
	fp_nullsys,	/* forkret */
	fp_getclinfo,
	fp_nullsys,	/* getglobpri */
	fp_parmsget,
	fp_parmsin,
	fp_parmsout,
	fp_parmsset,
	fp_preempt,
	fp_lwpcmp,
	fp_setrun,
	fp_nullsys,	/* sleep */
	fp_nullsys,	/* stop */
	fp_nullsys,	/* swapin */
	fp_nullsys,	/* swapout */
	fp_tick,
	fp_nullsys,	/* trapret */
	fp_wakeup,
	fp_insque,
	fp_rdblock,
	fp_allocate,
	fp_deallocate,
	fp_changestart,	/* changestart */
	fp_changeparms,
	fp_cancelchange,
	fp_changeend,	/* changeend */
	fp_nullsys,	/* newpri */
	fp_nosys,	/* movetoque */
	fp_prepblock,
	fp_block,
	fp_unblock,
	fp_cancelblock,
	fp_urdblock,
	fp_yield,
	fp_audit,
};


/*
 * void
 * fp_init(id_t cid, int clparmsz, classfuncs_t **clfuncspp, int *maxglobprip)
 *	FP class initialization routine.
 * 
 * Calling/Exit State:
 *
 *	Called at boot time. No locking requirements for caller.
 *
 * Description
 *	
 *	Fixed priority class initialization.  Called by dispinit()
 *	at boot time. Records the maximum global priority in FP
 *	class and the class ID. Sets a pointer to the class specific
 *	function vector and the maximum global priority within this
 *	class.
 *
 */
/*ARGSUSED*/
void
fp_init(id_t cid, int clparmsz, classfuncs_t **clfuncspp, int *maxglobprip)
{

	fp_cid = cid;		/* Record our class ID */

	/*
	 * We're required to return a pointer to our classfuncs
	 * structure and the highest global priority value we use.
	 */
	*clfuncspp = &fp_classfuncs;
	*maxglobprip = fp_dptbl[fp_maxpri].fp_globpri;

}


/*
 * int
 * fp_admin(void * uaddr, id_t reqpcid, cred_t *reqpcredp)
 *
 * 	Get or reset the fp_dptbl values per the user's request.
 *
 * Calling/Exit State:
 *
 *	Called from priocntl after acquiring the priocntl_update
 *	sleep lock. No other lock is required to be held. Function
 *	returns with priocntl_update lock still held.
 *
 * Description:
 *
 * 	fp_admin() is used to maintain and update the FP dispatcher
 *	table. The caller can either query the existing table values
 *	or overwrite the table with new values. In the latter case,
 *	sanity checking is done to ensure that the system does not
 *	panic. No other checking is done to ensure that the values
 *	supplied result in reasonable performance.
 *
 *	No extra locking is required here since priocntl_update
 *	sleep lock is held in priocntl prior to a call to fp_admin().
 */
/* ARGSUSED */
STATIC int
fp_admin(void *  uaddr, id_t reqpcid, cred_t *reqpcredp)
{
	fpadmin_t	fpadmin;
	fpdpent_t	*tmpdpp;
	int		userdpsz;
	int		i;
	int		fpdpsz;

	if (copyin(uaddr, (caddr_t)&fpadmin, sizeof(fpadmin_t)))
		return(EFAULT);

	fpdpsz = (fp_maxpri + 1) * sizeof(fpdpent_t);

	switch(fpadmin.fp_cmd) {

	case FP_GETDPSIZE:

		fpadmin.fp_ndpents = fp_maxpri + 1;
		if (copyout((caddr_t)&fpadmin, uaddr, sizeof(fpadmin_t)))
			return(EFAULT);
		break;

	case FP_GETDPTBL:

		userdpsz = MIN(fpadmin.fp_ndpents * sizeof(fpdpent_t), fpdpsz);
		if (copyout((caddr_t)fp_dptbl,
		    (caddr_t)fpadmin.fp_dpents, userdpsz))
			return(EFAULT);

		fpadmin.fp_ndpents = userdpsz / sizeof(fpdpent_t);
		if (copyout((caddr_t)&fpadmin, uaddr, sizeof(fpadmin_t)))
			return(EFAULT);

		break;

	case FP_SETDPTBL:

		/*
		 * We require that the requesting lwp have privilege.
		 * We also require that the table supplied by the user
		 * exactly match the current fp_dptbl in size.
		 */
		if (pm_denied(reqpcredp, P_SYSOPS)
		    && pm_denied(reqpcredp, P_FPRI))
			return(EPERM);
		if (fpadmin.fp_ndpents * sizeof(fpdpent_t) != fpdpsz)
			return(EINVAL);

		/*
		 * We read the user supplied table into a temporary buffer
		 * where it is validated before being copied over the
		 * fp_dptbl.
		 */
		tmpdpp = (fpdpent_t *)kmem_alloc(fpdpsz, KM_SLEEP);
		if (copyin((caddr_t)fpadmin.fp_dpents, (caddr_t)tmpdpp,
		    fpdpsz)) {
			kmem_free(tmpdpp, fpdpsz);
			return(EFAULT);
		}
		for (i = 0; i < fpadmin.fp_ndpents; i++) {

			/*
			 * Validate the user supplied time quantum
			 * values.
                         */
                        if (tmpdpp[i].fp_quantum <= 0 &&
 				tmpdpp[i].fp_quantum != FP_TQINF) {
				kmem_free(tmpdpp, fpdpsz);
				return(EINVAL);
                        }

		}

		/*
		 * Copy the user supplied values over the current fp_dptbl
		 * values.  The fp_globpri member is read-only so we don't
		 * overwrite it.
		 * It must be noted that the priocntl_update sleep lock
		 * only serializes updates/lookup of the table from
		 * priocntl. There is the possibility of someone using
		 * the table while it is being updated. This may result
		 * in some inconsistent values for such a user of the
		 * table for a very short period of time. This is
		 * considered benign and the inconsistencies will
		 * be lost in a very short period of time.
		 */
		for (i = 0; i < fpadmin.fp_ndpents; i++)
			fp_dptbl[i].fp_quantum = tmpdpp[i].fp_quantum;

		kmem_free(tmpdpp, fpdpsz);
		break;

	default:
		return(EINVAL);
	}
	return(0);
}


/*
 * void
 * fp_enterclass(long *fpkparmsp_arg, lwp_t *lwpp, lwpstat_t *lstatp_arg,
 *		int *lprip, uint *lflagp, cred_t **lcredpp,
 *		void **fplwppp, void *classdata)
 *
 *	Initialize FP class-specific lwp structure.
 *
 * Calling/Exit State:
 *
 *	Caller must hold lwp_mutex of the target lwp.
 *	The function returns with the lock still held.
 *	This function always applies to the calling lwp itself
 *	and not to another lwp.
 *
 * Description:
 *
 *	Initialize the fixed-priority class specific lwp structure
 *	with the parameters supplied or with default parameters.
 *	Also, move lwp to specified fixed-priority class priority.
 *
 *	Several arguments are passed to this function and a brief
 *	explanation of the various arguments is useful.
 *	fpkparmsp_arg : pointer to buffer containing parameters (fp_pri
 *	and fp_quantum) to be applied to the lwp,
 *	lwpp : pointer to the lwp entering the new class,
 *	lstatp_arg, lprip, lflagp : pointers to the l_stat, l_pri and
 *	l_flag fields of the lwp structure,
 *	lcredpp : pointer to pointer to the lwp's cred structure,
 *	fplwpp : pointer to pointer to the class specific lwp structure
 *	returned to the caller from a previous call to fp_allocate(),
 *	classdata : this is a pointer to a structure that will be used
 *	by some scheduling classes (such as the gang scheduling class
 *	where the enterclass function needs to know the address of
 *	the "gang" structure) but ignored by the FP class.
 */
/* ARGSUSED */
STATIC void
fp_enterclass(long *fpkparmsp_arg, lwp_t *lwpp, lwpstat_t *lstatp_arg,
		int *lprip, uint *lflagp, cred_t **lcredpp,
		void **fplwppp, void *classdata)
{
	fpkparms_t *fpkparmsp = (fpkparms_t *)fpkparmsp_arg;
	char * lstatp = (char *)lstatp_arg;
	fplwp_t		*fplwpp;
	int		oldpri;


	fplwpp = (fplwp_t *)*fplwppp;

	/*
	 * Initialize the fplwp structure.
	 */
	if (fpkparmsp == NULL) {
		/*
		 * Use default values.
		 */
		fplwpp->fp_pri = 0;
		fplwpp->fp_pquantum = fp_dptbl[0].fp_quantum;
	} else {
		/*
		 * Use supplied values.
		 */
		if (fpkparmsp->fp_pri == FP_NOCHANGE)
			fplwpp->fp_pri = 0;
		else
			fplwpp->fp_pri = fpkparmsp->fp_pri;

		if (fpkparmsp->fp_tqntm == FP_TQINF)
			fplwpp->fp_pquantum = FP_TQINF;
		else    if (fpkparmsp->fp_tqntm == FP_TQDEF ||
				fpkparmsp->fp_tqntm == FP_NOCHANGE)
				fplwpp->fp_pquantum = fp_dptbl[fplwpp->fp_pri].fp_quantum;
			else
				fplwpp->fp_pquantum = fpkparmsp->fp_tqntm;
	}

	fplwpp->fp_flags = 0;
	fplwpp->fp_lwpp = lwpp;
	fplwpp->fp_lstatp = lstatp;
	fplwpp->fp_lprip = lprip;
	fplwpp->fp_lflagp = lflagp;

	/*
	 * Reset priority.
	 */

	fplwpp->fp_timeleft = fplwpp->fp_pquantum;
	oldpri = fplwpp->fp_lwpp->l_pri;
	fplwpp->fp_lwpp->l_pri = fp_dptbl[fplwpp->fp_pri].fp_globpri;

	/*
	 * Before updating the e_pri field, eng_tbl_mutex is generally
	 * held to serialize accesses to the engine structure. However,
	 * that is not possible here since lwp_mutex is already
	 * held and holding eng_tbl_mutex would violate the locking
	 * order. It is true that all routines that examine e_pri
	 * hold runque-lock. Thus it is sufficient to hold runque
	 * lock while updating e_pri.
	 */
	RUNQUE_LOCK();
	l.eng->e_pri = fplwpp->fp_lwpp->l_pri;

	if (fplwpp->fp_lwpp->l_pri < oldpri) {
		ipreemption(fplwpp->fp_lwpp, kpnudge);
		fplwpp->fp_flags |= FPBACKQ;
	}
	RUNQUE_UNLOCK();


	return;
}


/*
 * void
 * fp_exitclass(lwp_t *fplwpp)
 *
 * 	Do final processing for an exiting FP lwp. There is nothing
 *	to be done.
 *
 * Calling/Exit State:
 *
 *	Called with the lwp_mutex held. Returns with the lock still held.
 *	When this lwp exits this class, it would have already entered
 *	the new scheduling class, if it is changing classes.
 *
 * Description:
 *
 *	fp_exitclass() does nothing.
 */
/* ARGSUSED */
STATIC void
fp_exitclass(lwp_t *fplwpp)
{
	return;
}


/*
 * void *
 * fp_allocate(void * clwpp)
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
 *	the FP scheduling class. Memory allocation is done with
 *	KM_SLEEP.
 */
/* ARGSUSED */
STATIC void *
fp_allocate(void * clwpp)
{
	register struct fplwp *fplwpp;

	fplwpp = (fplwp_t *)kmem_zalloc(sizeof(fplwp_t), KM_SLEEP);
	return (fplwpp);
}


/*
 * void
 * fp_deallocate(void * clwpp_arg)
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
 *	earlier call to fp_allocate(). It simply deallocates the
 *	fplwp structure associated with the target lwp.
 */
STATIC void
fp_deallocate(void * clwpp_arg)
{
	fplwp_t *clwpp = (fplwp_t *)clwpp_arg;

	kmem_free(clwpp, sizeof(fplwp_t));
}


/*
 * int
 * fp_fork(void *pfplwpp_arg, lwp_t *clwpp, lwpstat_t *clstatp_arg,
 *		int *clprip, uint *clflagp, cred_t **clcredpp,
 *		void **cllwpp)
 *
 *	Initialize FP class specific lwp structure for child.
 *
 * Calling/Exit State:
 *
 *	Called with the lwp_mutex of child and parent lwp held.
 *	The p_mutex of the child process is also held.
 *	Returns with all mutexes held.
 *
 * Description:
 *
 *	fp_fork() is called to add the lwp pointed to by clwpp to
 *	the scheduling class of the parent (FP).
 *	The child fplwp structure is initialized. The child is
 *	placed at the back of the run queue and the parent just
 *	continues.
 *
 *	As several arguments are passed in, it is useful to briefly
 *	state what each argument is;
 *	pfplwpp_arg : pointer to class specific lwp structure of the
 *	parent, clwpp : pointer to the lwp structure of the child,
 *	clstatp_arg, clprip, clflagp : pointers to l_stat, l_pri
 *	and l_flag members of the child lwp, clcredpp : pointer to
 *	pointer to the child lwp's cred structure, cllwpp : pointer
 *	to pointer to the fplwp structure to be associated with the
 *	child lwp (which was allocated via a previous call to
 *	fp_allocate() by the caller).
 */
/* ARGSUSED */
STATIC int
fp_fork(void *pfplwpp_arg, lwp_t *clwpp, lwpstat_t *clstatp_arg,
	int *clprip, uint *clflagp, cred_t **clcredpp,
	void **cllwpp)
{
	fplwp_t *pfplwpp = (fplwp_t *)pfplwpp_arg;
	char * clstatp = (char *)clstatp_arg;
	struct fplwp	*fplp = (fplwp_t *)*cllwpp;

	/*
	 * Initialize child's fplwp structure.
	 */
	fplp->fp_timeleft = pfplwpp->fp_pquantum;
	fplp->fp_pquantum = pfplwpp->fp_pquantum;
	fplp->fp_prmptoffset = pfplwpp->fp_prmptoffset;
	fplp->fp_pri = pfplwpp->fp_pri;
	fplp->fp_flags = pfplwpp->fp_flags & ~FPBACKQ;
	fplp->fp_lwpp = clwpp;
	fplp->fp_lstatp = clstatp;
	fplp->fp_lprip = clprip;
	fplp->fp_lflagp = clflagp;

	ASSERT(LOCK_OWNED(&clwpp->l_mutex));
	clwpp->l_stat = SRUN;
	if (LWP_SEIZED(clwpp)) {
		/*
		 * Since we are not placing the LWP on the run queue,
		 * null out the linkage.
		 */
		clwpp->l_rlink = clwpp->l_flink = NULL;
		return (0);
	}
	if (!(LWP_LOADED(clwpp))) {
		/*
		 * Child lwp is not loaded. Do not put it on
		 * the run queue. Simply return.
		 */
		return (0);
	}
	/*
	 * Place the child on the run queue.
	 */
	RUNQUE_LOCK();
	setbackrq(clwpp);
	preemption(clwpp, kpnudge, 0, NULL);
	RUNQUE_UNLOCK();

	return (0);
}


/*
 * int
 * fp_getclinfo(fpinfo_t *fpinfop, id_t reqpcid, cred_t *reqcredp)
 *
 *	Get FP class info into a buffer.
 *
 * Calling/Exit State:
 *
 *	Called without any special locks held. Returns without
 *	any locks held.
 *
 * Description:
 *
 * 	Returns information about the FP class into the buffer
 *	pointed to by fpinfop. The maximum configured FP
 *	priority is the only information we supply.  We ignore
 *	the class and credential arguments because anyone can have
 *	this information.
 */
/* ARGSUSED */
STATIC int
fp_getclinfo(fpinfo_t *fpinfop, id_t reqpcid, cred_t *reqcredp)
{
	fpinfop->fp_maxpri = fp_maxpri;
	return(0);
}


/*
 * int
 * fp_nosys()
 *
 *	Unsupported function.
 *
 * Calling/Exit State:
 *
 *	None.
 *
 * Description:
 *
 *	It simply returns ENOSYS.
 */
STATIC int
fp_nosys()
{
	return(ENOSYS);
}


/*
 * int
 * fp_nullsys()
 *
 *	No op function.
 *
 * Calling/Exit State:
 *
 *	None.
 *
 * Description:
 *
 *	It simply returns success.
 */
STATIC void
fp_nullsys()
{
}


/*
 * void
 * fp_parmsget(void * fplwpp_arg, long * fpkparmsp_arg)
 *
 *	Get the fixed-priority parameters of an lwp.
 *
 * Calling/Exit State:
 *
 *	Caller must hold the lwp-mutex of the lwp.
 *	Function returns with the lock still held.
 *
 * Description:
 *
 *	This returns certain class specific attributes
 *	of the lwp pointed to by fplwpp into the buffer
 *	pointed to by fpparmsp.
 */
STATIC void
fp_parmsget(void * fplwpp_arg, long * fpkparmsp_arg)
{
	fplwp_t *fplwpp = (fplwp_t *)fplwpp_arg;
	fpkparms_t *fpkparmsp = (fpkparms_t *)fpkparmsp_arg;

	fpkparmsp->fp_pri = fplwpp->fp_pri;
	fpkparmsp->fp_tqntm = fplwpp->fp_pquantum;
}


/*
 * int
 * fp_parmsin(long *fpparmsp_arg)
 *
 *	Check validity of fixed-priority parameters in the buffer.
 *
 * Calling/Exit State:
 *
 *	Caller need not hold any locks to call this function.
 *
 * Description:
 *
 *	Check the validity of the fixed-priority parameters in the
 *	buffer pointed to by fpparmsp. 
 *	The  memory pointed to by fpparms_t * and
 *	fpkparms_t * in  are used somewhat interchangeably.
 *	Thus, the offsets of elements in these structures are not
 *	arbitrary, but related.
 */
/* ARGSUSED */
STATIC int
fp_parmsin(long *fpparmsp_arg)
{
	fpparms_t *fpparmsp = (fpparms_t *)fpparmsp_arg;
	long temp1, temp2;

	/*
	 * First check the validity of parameters and convert
	 * the buffer to kernel format.
	 */
	if ((fpparmsp->fp_pri < 0 || fpparmsp->fp_pri > fp_maxpri) &&
	    fpparmsp->fp_pri != FP_NOCHANGE)
		return(EINVAL);

	if ((fpparmsp->fp_tqsecs == 0 && fpparmsp->fp_tqnsecs == 0) ||
	    fpparmsp->fp_tqnsecs >= SEC_TO_NSEC)
		return(EINVAL);
	
	if (fpparmsp->fp_tqnsecs >= 0) {
		temp1 = (fpparmsp->fp_tqsecs) * HZ;
		temp2 = (fpparmsp->fp_tqnsecs) / (1000000000 / HZ);
		((fpkparms_t *)fpparmsp)->fp_tqntm = temp1 + temp2;
	} else {
		if (fpparmsp->fp_tqnsecs != FP_NOCHANGE &&
		    fpparmsp->fp_tqnsecs != FP_TQINF &&
		    fpparmsp->fp_tqnsecs != FP_TQDEF)
			return(EINVAL);
		((fpkparms_t *)fpparmsp)->fp_tqntm = fpparmsp->fp_tqnsecs;
	}

	return(0);
}


/*
 * int
 * fp_parmsout(long * fpkparmsp_arg)
 *
 *	Convert output parameters from kernel form to user form.
 *
 * Calling/Exit State:
 *
 *	No locks are required to be held by the caller.
 *
 * Description:
 *
 *	This function does the required processing on the FP parameter
 *	buffer before it is copied out to the user. No permission
 *	checking is performed here. We simply convert the time quantum
 *	from ticks to seconds-nanoseconds. The  memory pointed to by
 *	fpparms_t * and fpkparms_t * in  are used somewhat interchangeably.
 *	Thus, the offsets of elements in these structures are not
 *	arbitrary, but related.
 */
/* ARGSUSED */
STATIC int
fp_parmsout(long * fpkparmsp_arg)
{
	fpkparms_t *fpkparmsp = (fpkparms_t *)fpkparmsp_arg;
	int		temp;

	if (fpkparmsp->fp_tqntm < 0) {
		/*
		 * Quantum field set to special value (e.g. FP_TQINF)
		 */
		((fpparms_t *)fpkparmsp)->fp_tqnsecs = fpkparmsp->fp_tqntm;
		((fpparms_t *)fpkparmsp)->fp_tqsecs = 0;
	} else {
		/* Convert quantum from ticks to seconds-nanoseconds */
		
		temp = fpkparmsp->fp_tqntm % HZ;
		((fpparms_t *)fpkparmsp)->fp_tqsecs = fpkparmsp->fp_tqntm / HZ;
		((fpparms_t *)fpkparmsp)->fp_tqnsecs = (SEC_TO_NSEC / HZ) * temp;
	}

	return(0);
}


/*
 * void
 * fp_parmsset(long *cllwpp, void *clparmsp, void *argp)
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
 *	to those specified in the buffer pointed to by fpkparmsp.
 *
 *	For FP class, there is no deferred parameter change permitted.
 *	Hence, it would be an error if this function got called.
 */
/* ARGSUSED */
STATIC void
fp_parmsset(long *cllwpp, void *clparmsp, void *argp)
{
	/*
	 *+ The fixed priority scheduling detected incorrect processing
	 *+ of a scheduling parameter change.  This is a basic kernel
	 *+ problem and cannot be corrected by an administrator.
	 */
	cmn_err(CE_PANIC, "FP scheduling class deferred a parameter change");
	return;
				
}



/*
 * void
 * fp_preempt(void *fplwpp_arg)
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
 *	released. qswtch() releases lwp-mutex at PL0. This is OK
 *	since we are context switching out and there is no
 *	information about ipl stored in the context anyway.
 *
 *	If the lwp has seize pending, the lwp is not put on the
 *	run queue. Instead, the lwp is forced to switch out of the processor.
 *
 *	If fp_preempt is called in response to a user binding of an
 *	SONPROC lwp, preemption() must be called to notify the
 *	processor to which binding is taking place. The flag is
 *	also reset.
 */
STATIC void
fp_preempt(void *fplwpp_arg)
{
	fplwp_t *fplwpp = (fplwp_t *)fplwpp_arg;
	lwp_t	*lwpp = fplwpp->fp_lwpp;

	(void)LOCK(&fplwpp->fp_lwpp->l_mutex, PLHI);
	fplwpp->fp_lwpp->l_stat = SRUN;

	/*
	 * If the lwp has seize pending, do not put it on 
	 * run queue, instead give up the processor.
	 */
	if (LWP_SEIZED(fplwpp->fp_lwpp)) {
		swtch(fplwpp->fp_lwpp);
		return;
	}


#ifndef UNIPROC
	if (fp_affinity_on) {
		if ((lwpp->l_kbind == NULL) && (lwpp->l_xbind == NULL) &&
			(lwpp->l_ubind == NULL) &&
			(lwpp->l_eng != NULL)) { /* cannot affinitize if hard-bound */
			engine_t *lasteng = lwpp->l_eng;
			int runqlwps = lasteng->e_rqlist[0]->rq_srunlwps;

			RUNQUE_LOCK();
			if (((runqlwps - avgqlen) < maximbalance) &&
				(!((runqlwps > 0) && (nidle > 0))))
				/*
				 * No imbalance, no other idle processors.
				 */
				lwpp->l_rq = lwpp->l_eng->e_rqlist[0];
			else
				lwpp->l_rq = global_rq;
			RUNQUE_UNLOCK();
		}
	}
#endif


	if ((fplwpp->fp_flags & FPBACKQ) != 0) {
		fplwpp->fp_timeleft = fplwpp->fp_pquantum;
		fplwpp->fp_flags &= ~FPBACKQ;
		RUNQUE_LOCK();
		setbackrq(fplwpp->fp_lwpp);
	} else {
		RUNQUE_LOCK();
		setfrontrq(fplwpp->fp_lwpp);
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
	 * to see if the scheduler invariant (i.e., at any time,
	 * the system should be running the highest priority lwp's
	 * that are runnable on the system) is still maintained.
	 */

	if (fplwpp->fp_lwpp->l_trapevf & EVF_L_UBIND) {
		engine_t *engp = NULL;

		if (fplwpp->fp_lwpp->l_xbind !=NULL)
			engp = fplwpp->fp_lwpp->l_xbind;
		else
			engp = fplwpp->fp_lwpp->l_ubind;
		fplwpp->fp_lwpp->l_trapevf &= ~EVF_L_UBIND; /* clear flag */
		preemption(fplwpp->fp_lwpp, kpnudge, 0, engp);
	}
	qswtch(fplwpp->fp_lwpp);	/*returns with run-queue and l_mutex unlocked */

}


/*
 * int
 * fp_lwpcmp(void *fplwp1p_arg, long *fpparmsp_arg)
 *
 * 	Compare scheduling priorities of two lwp's
 *
 * Calling/Exit State:
 *
 * 	Called from priocntl with the priocntl_update sleep lock held.
 *
 * Description
 *
 *	fp_lwpcmp() is part of the implementation of the PC_GETPARMS
 *	command of the priocntl system call. When the user specifies
 *	multiple lwp's to priocntl PC_GETPARMS the criteria
 *	for selecting a lwp from the set is class specific. The
 *	criteria used by the fixed-priority class is the priority value
 *	of the lwp. fp_lwpcmp() simply compares the priority of the current
 *	lwp against the priority of the previously chosen lwp.
 *	All the ugly work of looping through the lwp's in the set is done
 *	by higher level (class independent) functions.
 */
STATIC int
fp_lwpcmp(void *fplwp1p_arg, long *fpparmsp_arg)
{
	fplwp_t *fplwp1p = (fplwp_t *)fplwp1p_arg;
	fpkparms_t *fplwp2kparmsp = (fpkparms_t *)fpparmsp_arg;

	return(fplwp1p->fp_pri - fplwp2kparmsp->fp_pri);
}


/*
 * int
 * fp_rdblock(lwp_t *lwpp, list_t *lp)
 *
 *	Determine if the caller should be queued or not.
 *
 * Calling/Exit State:
 *
 *	Caller must hold both the list-lock and the lwp_mutex.
 *
 * Description:
 *
 *	This class operation is used by the reader/writer sync primitive.
 *	The calling lwp is attempting to acquire a reader/writer lock
 *	in read mode, currently held by one or more readers when there
 *	are known to be writers waiting for the lock. This function
 *	determines if the caller should be granted the lock or it should
 *	wait behind one or more of the writers.
 *
 *	For the FP class, if the first lwp on the queue is a writer
 *	and has a higher priority than the calling lwp, it must block.
 *	Otherwise, it can be granted the lock.
 */
/* ARGSUSED */
int
fp_rdblock(lwp_t *lwpp, list_t *lp)
{
	if (EMPTYQUE(lp)) { /* if nobody queued, don't block */
		return(0);
	} else {  /* first waiter is a writer, compare with its pri */
		lwp_t *waitlwpp;

		waitlwpp = (lwp_t *)lp->flink;
		ASSERT(waitlwpp != NULL);
		if (waitlwpp->l_pri > lwpp->l_pri)
			return (1);  /* caller must block */
		else
			return (0);
	}
}


/*
 * void
 * fp_setrun(void *fplwpp_arg)
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
 *	The lwp pointed to by fplwpp is placed on the
 *	appropriate run queue, after resetting its time
 *	quantum.
 *
 *	The run_queue lock is acquired while calling setbackrq().
 *	After placing the lwp on the run queue, a check is made
 *	(via preemption) to see if any running lwp needs to be
 *	preempted.
 */
STATIC void
fp_setrun(void *fplwpp_arg)
{
	fplwp_t *fplwpp = (fplwp_t *)fplwpp_arg;
	lwp_t 	*lwpp = fplwpp->fp_lwpp;
	engine_t *engp = NULL;

	ASSERT(LOCK_OWNED(&fplwpp->fp_lwpp->l_mutex));
	if (LWP_SEIZED(fplwpp->fp_lwpp)) {
		/*
		 * Since we are not placing the LWP on the run queue,
		 * null out the linkage.
		 */
		fplwpp->fp_lwpp->l_rlink = fplwpp->fp_lwpp->l_flink = NULL;
		if (!(LWP_LOADED(fplwpp->fp_lwpp))) {
			load_lwp(fplwpp->fp_lwpp);
		}
		return;
	}

	if (!(LWP_LOADED(fplwpp->fp_lwpp))) {
		load_lwp(fplwpp->fp_lwpp);
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
			if ((fp_affinity_on == 1) &&
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


	fplwpp->fp_timeleft = fplwpp->fp_pquantum;
	fplwpp->fp_flags &= ~FPBACKQ;

	RUNQUE_LOCK();
	setbackrq(fplwpp->fp_lwpp);
	preemption(fplwpp->fp_lwpp, kpnudge, 0, engp);
	RUNQUE_UNLOCK();
}


/*
 * void
 * fp_insque(lwp_t *lwpp, list_t *lp, int priority)
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
 *	fp_insque() replaces the old fp_sleep() routine.
 *	It places the lwp on the blocking queue pointed to by lp.
 *	For the FP class, the queueing is in decreasing priority order.
 *	For same priority lwp's, the ordering is FIFO.
 */
/* ARGSUSED */
STATIC void
fp_insque(lwp_t *lwpp, list_t *lp, int disp)
{
	lwp_t *lwpp1;

	if (LWP_SEIZED(lwpp))
		return;

	/* Now, enqueue it in priority order */
	lwpp1 = (lwp_t *)lp->flink;
	while (lwpp1 != (lwp_t *)lp && lwpp1->l_pri >= lwpp->l_pri) {
		lwpp1 = (lwp_t *)(lwpp1->l_flink);
	}
	insque(lwpp, lwpp1->l_rlink);
}


/*
 * void
 * fp_tick(void *fplwpp_arg)
 *
 *	Processor-local clock interrupt handling.
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
 *	This function checks for time slice expiration (unless the
 *	lwp has infinite time quantum). If the time slice has
 *	expired, it calls kpnudge to arrange for the preemption
 *	of the lwp. The lwp is placed at the back of the dispatcher
 *	queue by setting the FPBACKQ flag.
 */

STATIC void
fp_tick(void *fplwpp_arg)
{
	fplwp_t *fplwpp = (fplwp_t *)fplwpp_arg;

	if (fplwpp->fp_pquantum != FP_TQINF &&
		--fplwpp->fp_timeleft == 0) {
		fplwpp->fp_flags |= FPBACKQ;

		/* cause lwp to give up processor */
		RUNQUE_LOCK();
		kpnudge(fplwpp->fp_lwpp->l_pri, l.eng);
		RUNQUE_UNLOCK();
	}

}


/*
 * void
 * fp_wakeup(void *fplwpp_arg, int preemptflg)
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
 *	to check if any processor needs to be kpnudged, if
 *	it is running an lwp of lower priority than the
 *	one just woken up.
 */
/* ARGSUSED */
STATIC void
fp_wakeup(void *fplwpp_arg, int preemptflg)
{
	fplwp_t *fplwpp = (fplwp_t *)fplwpp_arg;
	lwp_t	*lwpp = fplwpp->fp_lwpp;
	engine_t *engp = NULL;

	ASSERT(LOCK_OWNED(&fplwpp->fp_lwpp->l_mutex));
	if (LWP_SEIZED(fplwpp->fp_lwpp)) {
		/*
		 * Since we are not placing the LWP on the run queue,
		 * null out the linkage.
		 */
		fplwpp->fp_lwpp->l_flink = fplwpp->fp_lwpp->l_rlink = NULL;
		if (!(LWP_LOADED(fplwpp->fp_lwpp))) {
			load_lwp(fplwpp->fp_lwpp);
		}
		return;
	}

	if (!(LWP_LOADED(fplwpp->fp_lwpp))) {
		load_lwp(fplwpp->fp_lwpp);
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
			if ((fp_affinity_on == 1) &&
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


	fplwpp->fp_timeleft = fplwpp->fp_pquantum;
	fplwpp->fp_flags &= ~FPBACKQ;
	RUNQUE_LOCK();
	setbackrq(fplwpp->fp_lwpp);
	preemption(fplwpp->fp_lwpp, kpnudge, preemptflg, engp);
	RUNQUE_UNLOCK();
}


/*
 * int
 * fp_changeparms(lwp_t *lwpp, int cmd, id_t cid, pcparms_t *pcparmsp,
 *		  void * classdata, cred_t *credp, qpcparms_t **qpcparmspp,
 *		  int *donep)
 *
 *	Inform scheduling class of pending parameter change.
 *
 * Calling/Exit State:
 *
 *	Must be called with the lwp_mutex held.
 *	The function returns with the lock still held.
 *
 * Description:
 *
 *	This function informs the scheduling class of a scheduling
 *	class parameter change pending against the lwp. It will
 *	analyze the pending change to see if it can be performed
 *	right away or if it must be queued for later processing.
 *	For FP class, any request to change priority or quantum
 *	can be accommodated right away. If it is a request for
 *	scheduling class change, that must be deferred and this
 *	pending change is indicated by setting a flag in the
 *	lwp structure.
 */
/* ARGSUSED */
int
fp_changeparms(lwp_t *lwpp, int cmd, id_t cid,  pcparms_t *pcparmsp,
	       void *classdata, cred_t *reqcredp, qpcparms_t **qpcparmspp,
	       int *donep)
{
	struct fpkparms *fpkparmsp;
	fplwp_t *fplwpp = lwpp->l_cllwpp;

	if (cmd == CHANGEPARMS_FORK) {
		/*
		 * Changeparms from parmsprop.  The caller doesn't
		 * really expect us to do anything, just return the
		 * buffer.
		 */
		*qpcparmspp = &fplwpp->fp_qpcparms;
		return(0);
	}

	/*
	 * Check for permission.
	 */
	if (reqcredp && !hasprocperm(lwpp->l_cred, reqcredp)) {
		return EPERM;
	}

	if (cmd == CHANGEPARMS_CHECKONLY) {
		/*
		 * Parmsset is dealing with a queued change and is first
		 * determining if the requester can apply the change to
		 * the target.
		 */

		if (pm_denied(reqcredp, P_FPRI))
			return(EPERM);
		*donep = 0;
		return(0);
	}

	/*
	 * Parmsset is giving us a fair and square chance to apply
	 * the parameter change directly.
	 * Privilege check is required whether it is class change
	 * or parameter change within FP class.
	 */
	if (pm_denied(reqcredp, P_FPRI))
		return(EPERM);

	if (pcparmsp->pc_cid == cid) {	/* no class change */
		/*
		 * For FP class, the only other changes that can be
		 * requested are priority and time quantum.
		 * These can be applied immediately.
		 */

		fpkparmsp = (fpkparms_t *)pcparmsp->pc_clparms;

		if (fpkparmsp->fp_pri != FP_NOCHANGE)
			fplwpp->fp_pri = fpkparmsp->fp_pri;

		switch (fpkparmsp->fp_tqntm) {
		case FP_NOCHANGE :
			break;
		case FP_TQDEF :
			fplwpp->fp_pquantum = fp_dptbl[fpkparmsp->fp_pri].fp_quantum;
			break;
		case FP_TQINF :
			fplwpp->fp_pquantum = FP_TQINF;
			break;
		default :
			fplwpp->fp_pquantum = fpkparmsp->fp_tqntm;
			break;
		}
	
		fplwpp->fp_timeleft = fplwpp->fp_pquantum;
		switch (dispnewpri(lwpp, fp_dptbl[fplwpp->fp_pri].fp_globpri)) {
		case SONPROC:
			fplwpp->fp_flags |= FPBACKQ;
			break;
		case SSLEEP:
			break;
		default:
			break;
		}
		*donep = 1;
	} else {
		/*
		 * Scheduling class change.
		 */

		if (qpcparmspp != NULL)
			*qpcparmspp = &fplwpp->fp_qpcparms;
		*donep = 0;
	}
	return (0);
}


/*
 * int
 * fp_cancelchange(lwp_t *lwpp, int cmd, qpcparms_t *qpcparmsp,
 *		       pcparms_t *pcparmsp, void *classdata, cred_t *reqcredp,
 *		       int *combinedp)
 *
 *	Deal with an FP queued change relative to a new change.
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
fp_cancelchange(lwp_t *lwpp, int cmd, qpcparms_t *qpcparmsp,
		pcparms_t *pcparmsp, void *classdata, cred_t *reqcredp,
		int *combinedp)
{
	fpkparms_t *oldfpkparmsp, *newfpkparmsp;
	short		reqpri;
	long		reqqntm;
	int		temp;

	*combinedp = 0;

	/*
	 * Check for permission.
	 */
	if (!hasprocperm(lwpp->l_cred, reqcredp)) {
		return EPERM;
	}

	ASSERT(qpcparmsp->qpc_pcparms.pc_cid == fp_cid);

	switch (cmd) {
	case CANCELCHANGE_TRYCOMBINE:
		ASSERT(pcparmsp->pc_cid == qpcparmsp->qpc_pcparms.pc_cid);

		/*
		 * Only priority and quantum need to be readjusted.
		 * This is OK. The new values have already been
		 * checked for sanity by priocntl and will not be
		 * checked here again.
		 */
		oldfpkparmsp = (fpkparms_t *)qpcparmsp->qpc_pcparms.pc_clparms;
		newfpkparmsp = (fpkparms_t *)pcparmsp->pc_clparms;

		reqpri = newfpkparmsp->fp_pri;

		switch (newfpkparmsp->fp_tqntm) {
		case FP_NOCHANGE :
			reqqntm = oldfpkparmsp->fp_tqntm;
			break;
		case FP_TQDEF :
			temp = fp_dptbl[newfpkparmsp->fp_pri].fp_quantum;
			/* temp is now in ticks */
			reqqntm = temp;
			break;
		default :
			reqqntm = newfpkparmsp->fp_tqntm;
			break;
		};
	
		oldfpkparmsp->fp_pri = reqpri;
		oldfpkparmsp->fp_tqntm = reqqntm;

		*combinedp = 1;
		break;

	case CANCELCHANGE_MUSTCANCEL:
		ASSERT(pcparmsp->pc_cid != qpcparmsp->qpc_pcparms.pc_cid);

		break;
	case CANCELCHANGE_EXIT:
		break;
	};
	return (0);
}


/*
 * int
 * fp_changestart(pcparms_t *pcparmsp, void **argpp)
 *
 *	Deal with some initializations for scheduling classes that
 *	care to know if multiple lwp's that were supplied to
 *	priocntl form a special set that needs to be identified.
 *
 * Calling/Exit State:
 *
 *	None.
 *
 * Description:
 *
 *	For FP class, this is a no op.
 */
/* ARGSUSED */
int
fp_changestart(pcparms_t *pcparmsp, void **argpp)
{
	return(0);
}


/*
 * int
 * fp_changeend(pcparms_t *pcparmsp, void *argp)
 *
 *	Deal with end-of-priocntl operations related to sets of
 *	lwp's that were supplied to priocntl via a single syscall.
 *	For scheduling classes that deal with sets of lwp's and
 *	treat the sets in a special manner, this function is used
 *	to perform any final cleanup operation related to that
 *	aggregation of lwp's.
 *
 * Calling/Exit State:
 *
 *	None.
 *
 * Description:
 *
 *	For FP class, this is a no op.
 */
/* ARGSUSED */
int
fp_changeend(pcparms_t *pcparmsp, void *argp)
{
	return(0);
}

/*
 * int fp_prepblock(lwp_t *lwp, sq_t *sq, int rdwr)
 *
 *	Place the lwp on the sync-queue.
 *
 * Calling/Exit State:
 *
 *	Must be called with both lwp->l_mutex and sq->sq_lock held.
 *
 * Description:
 *
 *	Place the lwp in front of the first lwp with a lower
 *	priority.
 */
STATIC int
fp_prepblock(lwp_t *lwp, sq_t *sq, int rdwr)
{
	lwp_t *lp;
	int pri = lwp->l_pri;
	lwp_t *last;

	ASSERT(LOCK_OWNED(&lwp->l_mutex));

	lwp->l_ublocktype = (u_char)rdwr;


	lp = NULL;
	last = sq->sq_tail;
	while ((last != NULL) && (last->l_pri < pri)) {
		lp = last;
		last = last->l_sbak;
	}

	lwp->l_sfwd = lp;
	lwp->l_sbak = last;

	if (lp == NULL)
		sq->sq_tail = lwp;
	else
		lp->l_sbak = lwp;

	if (last == NULL)
		sq->sq_head = lwp;
	else {
		last->l_sfwd = lwp;
	}

	return(0);
}

/*
 * int fp_urdblock(lwp_t *lwp, sq_t *sq, int *ret)
 *
 *	Determine if the current lwp (a reader) should block against "sq".
 *
 * Calling/Exit State:
 *
 *	Must be called with both sq->sq_lock and lwp->l_mutex held.
 *
 * Description:
 *
 *	We're called by a reader when there are known to be writers
 *	waiting for a reader-writer lock.  If there are any writers with
 *	higher priority waiting for the synchronization, we cause the
 *	reader to block, otherwise, we allow the reader to take the lock
 *	associated with the sync-queue.
 */
STATIC int
fp_urdblock(lwp_t *lwp, sq_t *sq, int *ret)
{
	lwp_t *lp = sq->sq_head;

	ASSERT(LOCK_OWNED(&lwp->l_mutex));

	/*
	 * Find the first writer and check its priority.  This will
	 * always be the first lwp on the list.
	 */
	if (lp == NULL) {
		/*
		 * No waiters, let this lwp in.
		 */
		*ret = 0;
		return (0);
	}
	if (lp->l_pri < lwp->l_pri)
		*ret = 0;
	else
		*ret = 1;
	return(0);
}

/*
 * int fp_yield(void *fplwpp_arg, int flag)
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
fp_yield(void *fplwpp_arg, int flag)
{
	fplwp_t *fplwpp = (fplwp_t *)fplwpp_arg;
	lwp_t	*lwpp = fplwpp->fp_lwpp;

	(void)LOCK(&fplwpp->fp_lwpp->l_mutex, PLHI);
	fplwpp->fp_lwpp->l_stat = SRUN;

	/*
	 * If the lwp has seize pending, do not put it on 
	 * run queue, instead give up the processor.
	 */
	if (LWP_SEIZED(fplwpp->fp_lwpp)) {
		swtch(fplwpp->fp_lwpp);
		return(0);
	}

	/*
	 * Put ourselves at the end of the run-queue (ignoring the flag).
	 */
	fplwpp->fp_timeleft = fplwpp->fp_pquantum;
	fplwpp->fp_flags &= ~FPBACKQ;

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
			if ((fp_affinity_on == 1) &&
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


	RUNQUE_LOCK();
	setbackrq(fplwpp->fp_lwpp);
	qswtch(fplwpp->fp_lwpp);

	return(0);
}

/*
 * STATIC void fp_audit(int flags, int cmd, int error, void *args,
 *			void *classdata)
 *	When the flag is ADTTST, this function checks if ADT_SCHED_FP event 
 *	is audited for the calling LWP. Otherwise, it dumps the audit record.
 *
 * Calling/Exit State:
 *	No lock must be held on entry and none held at exit.
 *
 */
/* ARGSUSED */
STATIC void
fp_audit(int flags, int cmd, int error, void *args, void *classdata)
{
	alwp_t *alwp = u.u_lwpp->l_auditp;
	pcparms_t *pcparmsp;
	fpkparms_t *fpkparmsp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(flags == ADTTST || flags == ADTDUMP);

	/* check if the event is being audited */
	if (flags == ADTTST) {
		ADT_SCHED_EVENTCK(ADT_SCHED_FP, alwp);
		return;
	}

	ASSERT(cmd == PC_SETPARMS || cmd == PC_ADMIN);

	/* dump the sched event record */
	switch (cmd) {
	case PC_SETPARMS:
		pcparmsp = (pcparms_t *)args;
		fpkparmsp = (fpkparms_t *)pcparmsp->pc_clparms;
		adt_parmsset(ADT_SCHED_FP, error, fpkparmsp->fp_pri, 0);
                break;


	case PC_ADMIN:
	{
		fpdpent_t *tmpdpp;
		fpadmin_t fpadmin;
		int 	  fpdpsz;

		if (alwp && EVENTCHK(ADT_SCHED_FP, alwp->al_emask->ad_emask)) { 
			SET_AUDITME(alwp);
			if (copyin(args, &fpadmin, sizeof(fpadmin_t))) {
				adt_admin(ADT_SCHED_FP, EFAULT, 0, NULL);
				return;
			}
			if (fpadmin.fp_cmd != FP_SETDPTBL) 
				return;


			fpdpsz = (fp_maxpri + 1) * sizeof(fpdpent_t);
			if ((error == EFAULT) || (error == EPERM) || 
			    (fpadmin.fp_ndpents*sizeof(fpdpent_t) != fpdpsz)) {
				/* Dump the record with table size eq 0 */
				adt_admin(ADT_SCHED_FP, error, 0, NULL);
				return;
			}

			tmpdpp = (fpdpent_t *)kmem_alloc(fpdpsz, KM_SLEEP);
			ASSERT(tmpdpp != NULL);
			copyin(fpadmin.fp_dpents, tmpdpp, fpdpsz);

			adt_admin(ADT_SCHED_FP, error, fpadmin.fp_ndpents, 
				  tmpdpp);
			kmem_free(tmpdpp, fpdpsz);
		}
	}
	}
}
