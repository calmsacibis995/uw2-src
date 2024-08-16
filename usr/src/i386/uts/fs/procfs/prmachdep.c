/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:fs/procfs/prmachdep.c	1.14"
#ident	"$Header: $"

#include <fs/procfs/procfs.h>
#include <fs/procfs/prdata.h>
#include <mem/vmparam.h>
#include <proc/proc.h>
#include <proc/regset.h>
#include <proc/user.h>
#include <svc/debugreg.h>
#include <svc/errno.h>
#include <svc/fp.h>
#ifdef WEITEK
#include <svc/weitek.h>
#endif
#include <util/debug.h>
#include <util/inline.h>
#include <util/kcontext.h>
#include <util/kdb/xdebug.h>
#include <util/ksynch.h>
#include <util/plocal.h>
#include <util/types.h>


/*
 * greg_t prgetpc(gregset_t rp)
 *	Return the value of the PC from the supplied register set.
 *
 * Calling/Exit State:
 *	None.
 */
greg_t
prgetpc(gregset_t rp)
{
	return rp[R_EIP];
}


/*
 * int prgetscall(gregset_t rp)
 *	Return the current system call number from the supplied
 *	register set.  This assumes that the target is stopped
 *	on a system call.
 *
 * Calling/Exit State:
 *	None.
 */
int
prgetscall(gregset_t rp)
{
	return rp[R_EAX] & 0xFF;
}


/*
 * int prhasfp(void)
 *	Is there floating-point support?
 *
 * Calling/Exit State:
 *	None.
 */
int
prhasfp(void)
{
#ifdef WEITEK
	return (fp_kind != FP_NO) || (weitek_kind != WEITEK_NO);
#else
	return (fp_kind != FP_NO);
#endif
}


/*
 * int prsetdbregs(user_t *up, dbregset_t *db)
 *	Set debug registers.
 *	Returns 0 for success or
 *	EINVAL if *db contains invalid register values.
 *
 * Calling/Exit State:
 *	The u-block is mapped in by the caller.
 *
 * Remarks:
 *	The order of operations in this function is very important,
 *	due to interactions with preemption, trap(), and KDB.  Be very
 *	careful about any changes.
 */
int
prsetdbregs(user_t *up, dbregset_t *db)
{
	char debugon, was_on;

	db->debugreg[DR_CONTROL] &= ~(DR_GLOBAL_SLOWDOWN |
				      DR_CONTROL_RESERVED |
				      DR_GLOBAL_ENABLE_MASK);

	/* Don't allow the user to set breakpoints on kernel addresses */
	if ((db->debugreg[DR_CONTROL] & DR_ENABLE0) && KADDR(db->debugreg[0]))
		db->debugreg[DR_CONTROL] &= ~DR_ENABLE0;
	if ((db->debugreg[DR_CONTROL] & DR_ENABLE1) && KADDR(db->debugreg[1]))
		db->debugreg[DR_CONTROL] &= ~DR_ENABLE1;
	if ((db->debugreg[DR_CONTROL] & DR_ENABLE2) && KADDR(db->debugreg[2]))
		db->debugreg[DR_CONTROL] &= ~DR_ENABLE2;
	if ((db->debugreg[DR_CONTROL] & DR_ENABLE3) && KADDR(db->debugreg[3]))
		db->debugreg[DR_CONTROL] &= ~DR_ENABLE3;

	debugon = !!(db->debugreg[DR_CONTROL] & (DR_LOCAL_SLOWDOWN |
						 DR_LOCAL_ENABLE_MASK));
	was_on = up->u_kcontext.kctx_debugon;

	up->u_kcontext.kctx_debugon = 0;
	up->u_kcontext.kctx_dbregs = *db;

	if (debugon != was_on) {
		if (debugon) {
#ifndef NODEBUGGER
			boolean_t user_now_owns_dbregs;

			FSPIN_LOCK(&debug_count_mutex);
			user_now_owns_dbregs = (user_debug_count++ == 0);
			FSPIN_UNLOCK(&debug_count_mutex);

			if (user_now_owns_dbregs) {
				/* kernel debugger must give up use of dbregs */
				(*cdebugger) (DR_RELEASE_DBREGS, NO_FRAME);
			}
#endif /* NODEBUGGER */

			atomic_or((void *)&up->u_lwpp->l_special,
				  SPECF_DEBUGON);
		} else {
			atomic_and((void *)&up->u_lwpp->l_special,
				   ~SPECF_DEBUGON);
		}
	} else if (!debugon)
		return 0;

	if (up == upointer) {
		_wdr7(0);

		l.special_lwp = up->u_lwpp->l_special;
	}

	if (debugon) {
		up->u_kcontext.kctx_debugon = 1;
		if (up == upointer) {
			_wdr0(db->debugreg[0]);
			_wdr1(db->debugreg[1]);
			_wdr2(db->debugreg[2]);
			_wdr3(db->debugreg[3]);
			_wdr6(0);
			_wdr7(db->debugreg[7]);
		}
#ifndef NODEBUGGER
	} else if (was_on) {
		FSPIN_LOCK(&debug_count_mutex);
		ASSERT(user_debug_count != 0);
		user_debug_count--;
		FSPIN_UNLOCK(&debug_count_mutex);
#endif /* NODEBUGGER */
	}

	return 0;
}


/*
 * void prgetpfamily(const user_t *up, pfamily_t *fp)
 *	Get family-specific information.
 *	For i386 this is just the debug registers.
 *
 * Calling/Exit State:
 *	The u-block must be mapped in by the caller.
 *	The referenced process's p_mutex is held on entry and remains held.
 */
void
prgetpfamily(const user_t *up, pfamily_t *fp)
{
	fp->pf_flags = 0;
	fp->pf_dbreg = up->u_kcontext.kctx_dbregs;
}

/*
 * int prparsectl_family(ulong_t *cmdp, void *ap, size_t *sizep, uio_t *uiop)
 *	Parse and validate a family-specific control message.
 *	Copy the message from user space and remember its size,
 *	in case we have to "put it back" (for EINTR return).
 *	Command is already in *cmdp.
 *	Arguments are placed in *ap, size in *sizep.
 *
 * Calling/Exit State:
 *	Called from prwritectl_family().
 *	No locks are held on entry or exit.
 */
STATIC int
prparsectl_family(ulong_t cmd, void *ap, size_t *sizep, uio_t *uiop)
{
	switch (cmd) {
	case PCSDBREG:			/* set debug registers */
		ASSERT(sizeof (dbregset_t) <= sizeof (fpregset_t));
		*sizep = sizeof (dbregset_t);
		break;
	default:
		return EINVAL;
	}

	if (uiop->uio_resid < *sizep)
		return EINVAL;
	return uiomove(ap, *sizep, UIO_WRITE, uiop);
}

/* ARGSUSED */
/*
 * int prwritectl_family(ulong_t cmd,
 *			 vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
 *	Write a family-specific control message to a process or LWP.
 *
 * Calling/Exit State:
 *	Called from prwritectl().  Returns 0 for success, or an errno on
 *	failure.  No locks are held on entry or exit.
 */
int
prwritectl_family(ulong_t cmd, vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
{
	prnode_t *pnp = VTOP(vp);
	prcommon_t *prcp = pnp->pr_common;
	proc_t *p;
	lwp_t *lwp;
	union {
		dbregset_t a_dbregset;
	} arg;
	size_t argsize;
	int error;

	if (error = prparsectl_family(cmd, &arg, &argsize, uiop))
		return error;
	ASSERT(argsize <= sizeof arg);

	/*
	 * Lock appropriately: acquire p_mutex or the process
	 * reader-writer lock, depending on the operation.
	 */
	if (cmd == PCSDBREG) {
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
	case PCSDBREG:			/* set debug registers */
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
		if (lwp == NULL || !ISTOP(lwp))
			error = EBUSY;
		if (error = ublock_lock(p, UB_NOSWAP))
			break;
		error = prsetdbregs(lwp->l_up, &arg.a_dbregset);
		ublock_unlock(p, UB_NOSWAP);
		break;
	default:
		error = EINVAL;
	}

	if (cmd == PCSDBREG)
		pr_v_rdwr(prcp);
	else
		UNLOCK(&p->p_mutex, PLBASE);

	return error;
}
