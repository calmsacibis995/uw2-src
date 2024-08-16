/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/procfs/prwrite.c	1.25"
#ident	"$Header: $"

#include <util/types.h>
#include <acc/audit/audit.h>
#include <acc/priv/privilege.h>
#include <proc/cred.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <util/ksynch.h>
#include <mem/kmem.h>
#include <mem/ublock.h>
#include <proc/proc.h>
#include <proc/pid.h>
#include <proc/regset.h>
#include <proc/signal.h>
#include <proc/siginfo.h>
#include <io/uio.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/procfs/procfs.h>
#include <fs/procfs/prdata.h>
#include <fs/procfs/prsystm.h>
#include <util/cmn_err.h>

STATIC int prwriteas(vnode_t *, uio_t *, int, cred_t *);
STATIC int prwritectl(vnode_t *, uio_t *, int, cred_t *);
STATIC int prparsectl(ulong_t, void *, size_t *, uio_t *);


/*
 * int prwrite(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
 *
 * Calling/Exit State:
 *	VOP_RWWRLOCK must have been applied by the caller.
 */
/* ARGSUSED */
int
prwrite(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
{
	struct prnode *pnp = VTOP(vp);
	int error;

	switch (pnp->pr_type) {
	case PR_PROCDIR:
	case PR_PIDDIR:
	case PR_OBJECTDIR:
	case PR_LWPDIR:
	case PR_LWPIDDIR:
		error = EISDIR;
		break;
	case PR_CTL:
	case PR_LWPCTL:
		error = EINVAL;		/* in case of negative uio_resid */
		while (uiop->uio_resid > 0 &&
		       (error = prwritectl(vp, uiop, ioflag, cr)) == 0)
			;
		break;
	case PR_AS:
		error = prwriteas(vp, uiop, ioflag, cr);
		break;
	case PR_STATUS:
	case PR_PSINFO:
	case PR_MAP:
	case PR_CRED:
	case PR_SIGACT:
	case PR_LWPSTATUS:
	case PR_LWPSINFO:
		error = ENOSYS;
		break;
	default:
		error = EINVAL;		/* "Can't happen" */
		break;
	}

	return error;
}


/*
 * int prwriteas(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
 * 	Write the address-space of the target process.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC int
prwriteas(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
{
	prnode_t *pnp = VTOP(vp);
	prcommon_t *prcp = pnp->pr_common;
	int error;

	if (pr_p_rdwr(prcp, B_TRUE) == 0)
		/*
		 * Address space is going, or gone.	
		 */
		return ENOENT;
	error = prusrio(prcp->prc_proc, UIO_WRITE, uiop);
	pr_v_rdwr(prcp);
	return error;
}


/*
 * int prwritectl(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
 * 	Write a control message to (i.e. perform a control operation
 *	upon) the target process or LWP.
 *
 * Calling/Exit State:
 *	Returns 0 for success, or an errno on failure.  No locks
 *	held on entry or exit.
 */
/* ARGSUSED */
STATIC int
prwritectl(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
{
	prnode_t *pnp = VTOP(vp);
	prcommon_t *prcp = pnp->pr_common;
	proc_t *p;
	lwp_t *lwp;
	union {
		int a_int;
		long a_long;
		ulong_t a_ulong;
		sigset_t a_sigset;
		fltset_t a_fltset;
		sysset_t a_sysset;
		gregset_t a_gregset;
		fpregset_t a_fpregset;
		k_siginfo_t a_ksiginfo;
	} arg;
	size_t argsize;
	ulong_t cmd;
	int error;

	if (uiop->uio_resid < sizeof cmd)
		return EINVAL;
	if (error = uiomove(&cmd, sizeof cmd, UIO_WRITE, uiop))
		return error;

	/* Undocumented feature: ignore extra zero words. */
	if (cmd == 0)
		return 0;

	if (error = prparsectl(cmd, &arg, &argsize, uiop))
		if (error == EINVAL)
			return prwritectl_family(cmd, vp, uiop, ioflag, cr);
		else
			return error;
	ASSERT(argsize <= sizeof arg);

	/*
	 * Lock appropriately: acquire p_mutex or the process
	 * reader-writer lock, depending on the operation.
	 */
	if (cmd == PCSREG ||
	    cmd == PCSFPREG) {
		if (!pr_p_rdwr(prcp, B_FALSE))
			return ENOENT;
		if (pnp->pr_flags & PR_INVAL) {
			pr_v_rdwr(prcp);
			return EBADF;
		}
		p = prcp->prc_proc;
		ASSERT(p != NULL);
	} else {
		if (!pr_p_mutex(prcp))
			return ENOENT;
		p = prcp->prc_proc;
		ASSERT(p != NULL);
		if (p->p_nlwp == 0) {	/* zombie */
			UNLOCK(&p->p_mutex, PLBASE);
			return ENOENT;
		}
		if (pnp->pr_flags & PR_INVAL) {
			UNLOCK(&p->p_mutex, PLBASE);
			return EBADF;
		}
	}

	switch (cmd) {
	case PCSTOP:		/* Direct a stop, and wait for stop */
	case PCDSTOP:		/* Direct a stop */
		/*
		 * If applied to the process, all LWPs that are not
		 * already stopped on an event of interest must be
		 * marked for requested stop.  If applied to an LWP,
		 * the specific LWP must be so marked.
		 */
		if (prcp->prc_flags & PRC_LWP) {
			lwp = prcp->prc_lwp;
			if (!ISTOP(lwp)) {
				if (lwp->l_stat == SSTOP)
					dbg_restop(lwp);
				else if (!(lwp->l_trapevf & EVF_PL_PRSTOP)) {
					LOCK(&lwp->l_mutex, PLHI);
					lwp->l_trapevf |= EVF_PL_PRSTOP;
					trapevnudge(lwp, B_FALSE);
					UNLOCK(&lwp->l_mutex, PLHI);
				}
			}
		} else {
			for (lwp = p->p_lwpp; lwp != NULL; lwp = lwp->l_next) {
				if (!ISTOP(lwp)) {
					if (lwp->l_stat == SSTOP)
						dbg_restop(lwp);
					else if (!(lwp->l_trapevf &
						   EVF_PL_PRSTOP)) {
						LOCK(&lwp->l_mutex, PLHI);
						lwp->l_trapevf |= EVF_PL_PRSTOP;
						trapevnudge(lwp, B_FALSE);
						UNLOCK(&lwp->l_mutex, PLHI);
					}
				}
			}
			p->p_flag |= P_PRSTOP; /* try to stop whole process */
		}
		if (cmd == PCDSTOP)
			break;
		/* FALLTHROUGH */

	case PCWSTOP:		/* Wait for stop */
		/*
		 * Sleep on the process or LWP sync variable, as
		 * appropriate, after first making sure that the
		 * target is stopped.
		 */
		if (prcp->prc_flags & PRC_LWP) {
			while (!ISTOP(prcp->prc_lwp)) {
				if (!SV_WAIT_SIG(&prcp->prc_stopsv, PRIMED,
						 &p->p_mutex)) {
					uiop->uio_resid += argsize + sizeof cmd;
					return EINTR;
				}
				if (pnp->pr_flags & PR_INVAL)
					return EBADF;
				/*
				 * Reacquire p_mutex and re-check for the
				 * terminating condition.  If we can't get
				 * p_mutex, the LWP is gone.  Note that we
				 * don't take the normal exit because that
				 * code assumes that p_mutex is held.
				 */
				if (!pr_p_mutex(prcp))
					return ENOENT;
			}
		} else while (p->p_nprstopped < p->p_nlwp) {
			if (!SV_WAIT_SIG(&prcp->prc_stopsv, PRIMED,
					 &p->p_mutex)) {
				uiop->uio_resid += argsize + sizeof cmd;
				return EINTR;
			}
			if (pnp->pr_flags & PR_INVAL)
				return EBADF;
			/*
			 * Reacquire p_mutex and also make sure that the
			 * target process hasn't exited.
			 */
			if (!pr_p_mutex(prcp))
				return ENOENT;
			if (p->p_flag & P_DESTROY) {
				error = ENOENT;
				break;
			}
		}
		break;

	case PCRUN:		/* set process or LWP running after a stop */
	{
		u_long runflags, evflags = 0;

		if (prcp->prc_flags & PRC_LWP)
			lwp = prcp->prc_lwp;
		else
			lwp = prchoose(p);
		if (lwp == NULL || !ISTOP(lwp)) {
			error = EBUSY;
			break;
		}
		/*
		 * Apply run-time options.
		 */
		runflags = arg.a_ulong;
		if (runflags & PRSABORT) {
			/*
			 * Process must be stopped on syscall entry, or
			 * else blocked in a system call.
			 */
			if (lwp->l_whystop != PR_REQUESTED &&
			    (lwp->l_flag & L_SIGWOKE) == 0) {
				error = EBUSY;
				break;
			}
			evflags |= EVF_PL_SYSABORT;
		}
		if (runflags & PRSTOP)
			evflags |= EVF_PL_PRSTOP;
		if (runflags & PRCSIG)
			dbg_clearlwpsig(lwp);
		if (runflags & PRCFAULT)
			lwp->l_curflt = 0;
		if (runflags & PRSTEP)
			evflags |= EVF_PL_STEP;
		if (evflags) {
			(void) LOCK(&lwp->l_mutex, PLHI);
			lwp->l_trapevf |= evflags;
			trapevnudge(lwp, B_FALSE);
			UNLOCK(&lwp->l_mutex, PLHI);
		}

		p->p_flag &= ~P_PRSTOP;	/* no longer stopping whole process */

		if (prcp->prc_flags & PRC_LWP)
			dbg_setrun(lwp);
		else {
			/*
			 * If PRSTEP or PRSTOP were requested, set the
			 * target LWP running.  Otherwise, mark it
			 * PR_REQUESTED and set all LWPs running if
			 * as a consequence all LWPs are now in
			 * PR_REQUESTED stops.
			 */
			if (runflags & (PRSTOP|PRSTEP))
				dbg_setrun(lwp);
			else {
				if (lwp->l_whystop != PR_REQUESTED) {
					lwp->l_whystop = PR_REQUESTED;
					p->p_nreqstopped++;
				}
				if (p->p_nreqstopped == p->p_nlwp) {
					for (lwp = p->p_lwpp; lwp != NULL;
					  lwp = lwp->l_next)
						dbg_setrun(lwp);
				}
			}
		}
		break;	
	}

	case PCSTRACE:		/* set signal tracing mask */
	{
		k_sigset_t sigmask;

		sigutok(&arg.a_sigset, &sigmask);
		sigdelset(&sigmask, SIGKILL);
		if (!sigisempty(&sigmask))
			p->p_flag |= P_PROCTR;
		else if (prisempty(&p->p_fltmask)) {
			/*
			 * Pick any LWP to see whether system-call tracing
			 * is enabled.
			 */
			lwp = p->p_lwpp;
			if (!(lwp->l_trapevf &
			      (EVF_PL_SYSENTRY|EVF_PL_SYSEXIT)))
				p->p_flag &= ~P_PROCTR;
		}
		dbg_sigtrmask(p, sigmask);	/* releases p_mutex */
		return 0;
	}

	case PCSFAULT:	/* set mask of traced faults */
	{
		if (!prisempty(&arg.a_fltset))
			p->p_flag |= P_PROCTR;
		else if (sigisempty(&p->p_sigtrmask)) {
			lwp = p->p_lwpp;
			if (!(lwp->l_trapevf &
			      (EVF_PL_SYSENTRY|EVF_PL_SYSEXIT)))
				p->p_flag &= ~P_PROCTR;
		}
		prassignset(&p->p_fltmask, &arg.a_fltset);
		break;
	}

	case PCSENTRY:	/* set syscall entry bit mask */
	case PCSEXIT:	/* set syscall exit bit mask */
	{
		k_sysset_t *newmaskp, **maskpp, **othermaskpp;
		u_int evflag;

		if (cmd == PCSENTRY) {
			maskpp = &p->p_entrymask;
			othermaskpp = &p->p_exitmask;
			evflag = EVF_PL_SYSENTRY;
		} else {
			maskpp = &p->p_exitmask;
			othermaskpp = &p->p_entrymask;
			evflag = EVF_PL_SYSEXIT;
		}

		if (prisempty(&arg.a_sysset)) {
			/*
			 * Setting an empty syscall mask.  Discard the old
			 * one, if it exists.
			 */
			if (*maskpp != NULL) {
				kmem_free(*maskpp, sizeof (k_sysset_t));
				*maskpp = NULL;
			}
			trapevunproc(p, evflag, B_TRUE);
			if (sigisempty(&p->p_sigtrmask)
			  && prisempty(&p->p_fltmask)
			  && *othermaskpp == NULL) 
				p->p_flag &= ~P_PROCTR;
			break;
		}

		if (*maskpp == NULL) {
			/*
			 * Drop p_mutex, allocate storage for a syscall
			 * mask, reacquire p_mutex (allowing for the
			 * possibility that the process may disappear
			 * in the interim) and then check to make sure
			 * we haven't raced with someone else to create
			 * the mask.
			 */
			UNLOCK(&p->p_mutex, PLBASE);
			newmaskp = kmem_alloc(sizeof(k_sysset_t), KM_SLEEP);
			if (!pr_p_mutex(prcp)) {
				kmem_free(newmaskp, sizeof(k_sysset_t));
				return ENOENT;
			} else if (pnp->pr_flags & PR_INVAL) {
				kmem_free(newmaskp, sizeof(k_sysset_t));
				error = EBADF;
				break;
			} else if (p->p_nlwp == 0) {
				kmem_free(newmaskp, sizeof(k_sysset_t));
				error = ENOENT;
				break;
			}
			if (*maskpp == NULL)
				*maskpp = newmaskp;
			else
				kmem_free(newmaskp, sizeof(k_sysset_t));
		}
		/*
		 * At this point the storage for the mask exists, and
		 * p_mutex is held.  Update the mask (which we know to be
		 * nonempty) and mark the process and the LWPs.
		 */
		prassignset(*maskpp, &arg.a_sysset);
		trapevproc(p, evflag, B_TRUE);
		p->p_flag |= P_PROCTR;
		ASSERT(*maskpp == NULL || !prisempty(*maskpp));
		break;
	}

	case PCSET:	/* set mode flags */
	case PCRESET:	/* reset mode flags */
	{
		long uflags, prflags = 0;

		uflags = arg.a_long;

		if (uflags & ~(PR_FORK|PR_PTRACE|PR_RLC|PR_KLC|PR_ASYNC)) {
			error = EINVAL;
			break;
		}
		if (uflags & PR_FORK)
			prflags |= P_PRFORK;
		if (uflags & PR_RLC)
			prflags |= P_PRRLC;
		if (uflags & PR_PTRACE)
			prflags |= P_TRC;
		if (uflags & PR_KLC)
			prflags |= P_PRKLC;
		if (uflags & PR_ASYNC)
			prflags |= P_PRASYNC;

		if (cmd == PCSET)
			p->p_flag |= prflags;
		else
			p->p_flag &= ~prflags;
		break;
	}

	case PCSREG:	/* set general registers */
	{
		if (prcp->prc_flags & PRC_LWP)
			lwp = prcp->prc_lwp;
		else {
			(void)LOCK(&p->p_mutex, PLHI);
			lwp = prchoose(p);
			(void)UNLOCK(&p->p_mutex, PLBASE);
		}
		/*
		 * No LWP will exit as long as we hold the process
		 * reader-writer lock.
		 */
		if (lwp == NULL || !ISTOP(lwp)) {
			error = EBUSY;
			break;
		}
		if ((error = ublock_lock(lwp->l_procp, UB_NOSWAP)) != 0)
			break;
		error = prsetregs(lwp->l_up, arg.a_gregset);
		ublock_unlock(lwp->l_procp, UB_NOSWAP);
		break;
	}

	case PCSFPREG:	/* set floating-point registers */
	{
		if (prcp->prc_flags & PRC_LWP)
			lwp = prcp->prc_lwp;
		else {
			(void)LOCK(&p->p_mutex, PLHI);
			lwp = prchoose(p);
			(void)UNLOCK(&p->p_mutex, PLBASE);
		}
		/*
		 * No LWP will exit as long as we hold the process
		 * reader-writer lock.
		 */
		if (lwp == NULL || !ISTOP(lwp)) {
			error = EBUSY;
			break;
		}
		if ((error = ublock_lock(p, UB_NOSWAP)) != 0)
			break;
		error = prsetfpregs(lwp->l_up, &arg.a_fpregset);
		ublock_unlock(p, UB_NOSWAP);
		break;
	}

	case PCKILL:
	{
		sigsend_t s;
		s.ss_sig = arg.a_int;
		s.ss_sqp = (sigqueue_t *)0;
		s.ss_checkperm = B_TRUE;
		s.ss_pidlistp = 0;

		if (s.ss_sig < 0 || s.ss_sig > MAXSIG)
			error = EINVAL;
		else
			error = sigsendproc(p, &s);
		break;
	}

	case PCUNKILL:
	{
		int sig = arg.a_int;

		if (sig < 1 || sig > MAXSIG || sig == SIGKILL)
			return EINVAL;

		if (prcp->prc_flags & PRC_LWP)
			dbg_unkilllwp(prcp->prc_lwp, sig);
		else
			dbg_unkillproc(p, sig);
		break;
	}

	case PCSSIG:
	{
		if (prcp->prc_flags & PRC_LWP)
			lwp = prcp->prc_lwp;
		else
			lwp = prchoose(p);

		if (lwp == NULL || !ISTOP(lwp))
			error = EBUSY;
		else if (arg.a_ksiginfo.si_signo < 0 ||
			 arg.a_ksiginfo.si_signo > MAXSIG)
			error = EINVAL;
		else if (arg.a_ksiginfo.si_signo == 0)
			dbg_clearlwpsig(lwp);
		else {
			sigqueue_t *sqp = siginfo_get(KM_NOSLEEP, 0);
			if (sqp)
				sqp->sq_info = arg.a_ksiginfo;
			if (dbg_setlwpsig(lwp, arg.a_ksiginfo.si_signo, sqp) == 0 && sqp)
				siginfo_free(sqp);
		}

		break;
	}

	case PCCFAULT:
		if (prcp->prc_flags & PRC_LWP)
			lwp = prcp->prc_lwp;
		else
			lwp = prchoose(p);

		if (lwp == NULL || !ISTOP(lwp))
			error = EBUSY;
		else if (lwp->l_whystop == PR_FAULTED)
			lwp->l_curflt = 0;
		break;

	case PCSHOLD:
	{
		k_sigset_t sigmask;

		sigutok(&arg.a_sigset, &sigmask);
		sigdiffset(&sigmask, &sig_cantmask);

		if (prcp->prc_flags & PRC_LWP)
			lwp = prcp->prc_lwp;
		else
			lwp = prchoose(p);

		if (lwp == NULL || !ISTOP(lwp))
			error = EBUSY;
		else
			dbg_sigheld(lwp, sigmask);
		break;
	}

	default:
		error = EINVAL;
		break;
	}

	if (cmd == PCSREG ||
	    cmd == PCSFPREG)
		pr_v_rdwr(prcp);
	else
		UNLOCK(&p->p_mutex, PLBASE);

	if (cmd == PCKILL)
		ADT_PCKILL(p->p_pidp->pid_id, arg.a_int, error);
	return error;
}

/*
 * int prparsectl(ulong_t cmd, void *ap, size_t *sizep, uio_t *uiop)
 *	Parse and validate a control message.
 *	The message's "command part" has already been copied (cmd);
 *	copy the message's argument part from user space and remember
 *	its size, in case we have to "put it back" (for EINTR return).
 *	The arguments are placed in *ap, size in *sizep.
 *
 * Calling/Exit State:
 *	Called from prwritectl().
 *	No locks are held on entry or exit.
 */
STATIC int
prparsectl(ulong_t cmd, void *ap, size_t *sizep, uio_t *uiop)
{
	switch (cmd) {
	case PCSTOP:
	case PCDSTOP:
	case PCWSTOP:
	case PCCFAULT:
		*sizep = 0;
		break;
	case PCRUN:
		*sizep = sizeof (ulong_t);
		break;
	case PCSTRACE:
	case PCSHOLD:
		*sizep = sizeof (sigset_t);
		break;
	case PCSFAULT:
		*sizep = sizeof (fltset_t);
		break;
	case PCSENTRY:
	case PCSEXIT:
		*sizep = sizeof (sysset_t);
		break;
	case PCSET:
	case PCRESET:
		*sizep = sizeof (long);
		break;
	case PCSREG:
		*sizep = sizeof (gregset_t);
		break;
	case PCSFPREG:
		*sizep = sizeof (fpregset_t);
		break;
	case PCSSIG:
		*sizep = sizeof (siginfo_t);
		break;
	case PCKILL:
	case PCUNKILL:
		*sizep = sizeof (int);
		break;
	default:
		return EINVAL;
	}

	if (uiop->uio_resid < *sizep)
		return EINVAL;
	return uiomove(ap, *sizep, UIO_WRITE, uiop);
}
