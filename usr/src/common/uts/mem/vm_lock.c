/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/vm_lock.c	1.13"
#ident	"$Header: $"

#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <proc/lwp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/proc_hier.h>
#include <proc/user.h>
#include <mem/as.h>
#include <mem/seg.h>
#include <mem/kmem.h>
#include <mem/lock.h>
#include <mem/memresv.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/tuneable.h>
#include <mem/ublock.h>
#include <mem/vmparam.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>


/* 
 * int
 * textlock(void)
 *	Lock just the ``text'' segment of the current process.
 *
 * Calling/Exit State:
 *	Called with the target AS write locked and returns the same way.
 *
 *	On success, zero is returned and all segments which meet the 
 *	attribute specification (arg #4 to as_ctl) will be locked into 
 *	core.
 *
 *	On failure, a non-zero errno is returned.
 *
 * Remarks:
 *	SVR4 no longer has a notion of a specialized text segment. The
 *	attributes passed to as_ctl will cause the locking of all segments
 *	which are MAP_PRIVATE with RO permissions.
 *
 *	On failure, tunlock() is called which unlocks the address space. This
 *	means any lock established by previous calls to memctl (e.g.) are
 *	undone. This is consistent with SVR4 (and probably SVR3) behavour.
 */
int
textlock(void)
{
	proc_t *p = u.u_procp;
	int error;

	error = as_ctl(p->p_as, (vaddr_t)0, (u_int)0, 
		       MC_LOCKAS, PROC_TEXT|PRIVATE|PROT_USER, 
		       (void *)MCL_CURRENT);
	if (error) {
		tunlock();
		/* for compatibility we have to return EAGAIN */
		error = EAGAIN;
	} else
		u.u_procp->p_plock |= TXTLOCK;

	return error;
}
		

/*
 * void
 * tunlock(void)
 *      Unlock just the ``text'' segment of the current process.
 *
 * Calling/Exit State:
 *      Called with the target AS write locked and returns the same way.
 *
 * Remarks:
 *      SVR4 no longer has a notion of a specialized text segment. The
 *      attributes passed to as_ctl will cause the unlocking of all segments
 *      which are MAP_PRIVATE with RO permissions.
 */
void
tunlock(void)
{
	proc_t *p = u.u_procp;

	(void) as_ctl(p->p_as, (vaddr_t)0, (u_int)0, 
	              MC_UNLOCKAS, PROC_TEXT|PRIVATE|PROT_USER, (void *)0);
	u.u_procp->p_plock &= ~TXTLOCK;
}

/*
 * int
 * datalock(void)
 *      Lock just the ``data'' segment of the current process.
 *
 * Calling/Exit State:
 *      Called with the target AS write locked and returns the same way.
 *
 *      On success, zero is returned and all segments which meet the
 *      attribute specification (arg #4 to as_ctl) will be locked into
 *      core.
 *
 *      On failure, a non-zero errno is returned.
 *
 * Remarks:
 *      SVR4 no longer has a notion of a specialized ``data'' segment. The
 *      attributes passed to as_ctl will cause the locking of all segments
 *      which are MAP_PRIVATE with RW permissions.
 *
 *      On failure, dunlock() is called which unlocks the address space. This
 *      means any lock established by previous calls to memctl (e.g.) are
 *      undone. This is consistent with SVR4 (and probably SVR3) behavour.
 */
int
datalock(void)
{
	proc_t *p = u.u_procp;
	int error;

	error = as_ctl(p->p_as, (vaddr_t)0, (u_int)0, 
		       MC_LOCKAS, PROC_DATA|PRIVATE|PROT_USER, 
		       (void *)MCL_CURRENT);
	if (error) {
		dunlock();
		/* for compatibility we have to return EAGAIN */
		error = EAGAIN;
	} else
		u.u_procp->p_plock |= DATLOCK;
	return error;
}

/*
 * void
 * dunlock(void)
 *      Unlock just the ``data'' segment of the current process.
 *
 * Calling/Exit State:
 *      Called with the target AS write locked and returns the same way.
 *
 * Remarks:
 *      SVR4 no longer has a notion of a specialized data segment. The
 *      attributes passed to as_ctl will cause the unlocking of all segments
 *      which are MAP_PRIVATE with RW permissions.
 */		
void
dunlock(void)
{
	proc_t *p = u.u_procp;

	(void) as_ctl(p->p_as, (vaddr_t)0, (u_int)0, 
		      MC_UNLOCKAS, PROC_DATA|PRIVATE|PROT_USER, (void *)0);
	u.u_procp->p_plock &= ~DATLOCK;
}

/*
 * int
 * proclock(void)
 *	Lock all the PRIVATE regions of the current process and render it
 *	unswappable.
 *
 * Calling/Exit State:
 *      Called with the target AS write locked and returns the same way.
 *
 *	On success, zero is returned and all the PRIVATE regions of the
 *	process have been locked in core. All COW pages in the AS have 
 *	been privitized. The process cannot be swapped.
 *
 *	On failure, a non-zero errno is returned and the process remains
 *	unlocked and swappable.
 *
 * Remarks:
 *	The semantic is archaic and though its name indicates that the
 *	``process'' is locked, in reality MAP_SHARED segments can still be
 *	preyed upon via pageout.
 */
int
proclock(void)
{
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * p_plock is protected by the AS lock which we hold for 
	 * writing (exclusive access)
	 */

	ASSERT(!(u.u_procp->p_plock & (PROCLOCK|TXTLOCK|DATLOCK)));

	if ((error = textlock()) != 0)
		return error;

	if ((error = datalock()) != 0) {
		tunlock();
		return error;
	}

	if (!(u.u_procp->p_plock & MEMLOCK)) {
		if ((error = ublock_lock(u.u_procp, UB_NOSWAP_USER)) != 0) {
			dunlock();
			tunlock();
			return error;
		}
		(void) LOCK(&u.u_procp->p_mutex, PLHI);
		ASSERT(!(u.u_procp->p_flag & P_NOSWAP));
		u.u_procp->p_flag |= P_NOSWAP;
		UNLOCK(&u.u_procp->p_mutex, PLBASE);
	}

	u.u_procp->p_plock |= PROCLOCK;

	return 0;
}

/* 
 * void
 * punlock(void)
 *	Unlock a previously memory locked process. All or part of the
 *	process may have been locked.
 *
 * Calling/Exit State:
 *	Requires the address space to be write locked. Returns the same
 *	way. The AS lock protects p_pplock in the proc structure.
 *
 * 	On success (some parts or all of the AS were locked) a zero is
 *	returned and the address space is no longer memory locked. The
 *	process may also be eligible for swapping. 
 *
 *	On failure (no part of the address space was locked) EINVAL is 
 *	returned to the caller.
 *
 * Remarks:
 *	If the proccess used memcntl to lock a MAP_SHARED portion of its
 *	address space the locking will NOT be undone here. The segments
 *	be caught and cleaned up by the underlying segment manager when 
 *	the AS is unmapped. 
 */
void
punlock(void)
{
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (u.u_procp->p_plock & PROCLOCK) {
		u.u_procp->p_plock &= ~PROCLOCK;
		if (!(u.u_procp->p_plock & MEMLOCK)) {
			ublock_unlock(u.u_procp, UB_NOSWAP_USER);
			(void) LOCK(&u.u_procp->p_mutex, PLHI);
			u.u_procp->p_flag &= ~P_NOSWAP;
			UNLOCK(&u.u_procp->p_mutex, PLBASE);
		}
	}
	if (u.u_procp->p_plock & TXTLOCK)
		tunlock();
	if (u.u_procp->p_plock & DATLOCK)
		dunlock();
}
