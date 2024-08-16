/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/subr.c	1.22"
#ident	"$Header: $"

/*
 * Miscellaneous kernel subroutines.
 */

#include <mem/hat.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/ksynch.h>
#include <util/types.h>


#ifdef DEBUG

/*
 * void
 * check_basepl(void)
 *	Check that processor interrupt priority level is PLBASE.
 *
 * Calling/Exit State:
 *	Processor must be at base interrupt priority, else an assertion
 *	failure is triggered.
 */
void
check_basepl(void)
{
	ASSERT(getpl() == PLBASE);
}

#endif

/*
 * int
 * nodev(void)
 *
 *	Routine which sets a user error; placed in
 *	illegal entries in the bdevsw and cdevsw tables.
 *
 * Calling/Exit State:
 *
 *	Returns ENODEV error.
 */
int
nodev(void)
{
	return ENODEV;
}

/*
 * int
 * nxio(void)
 *
 *	Routine which sets a user error; placed in
 *	illegal entries in the block and character device
 *	switch tables.
 *
 * Calling/Exit State:
 *
 *	Returns ENXIO error.
 */
int
nxio(void)
{
	return ENXIO;
}

/*
 * int
 * nulldev(void)
 *
 *	Null routine; placed in insignificant entries
 *	in the bdevsw and cdevsw tables.
 *
 * Calling/Exit State:
 *
 *	Returns 0.
 */
int
nulldev(void)
{
	return 0;
}

/*
 * void
 * nullsys(void)
 *
 *	Null void routine.
 *
 * Calling/Exit State:
 *
 *	None.
 */

void
nullsys(void)
{
}

/*
 * int
 * nosys(char *uap, rval_t *rvp)
 *
 *	Nonexistent system call-- signal bad system call.
 *
 * Calling/Exit State:
 *
 *	Placed in the sysent table for non-existent or bad system calls.
 *	All bad or non-existent system calls get directed here.
 */
/* ARGSUSED */
int
nosys(char *uap, rval_t *rvp)
{
	(void)sigtolwp(u.u_lwpp, SIGSYS, (sigqueue_t *)0);
	return EINVAL;
}


/*
 * int
 * invsys(char *uap, rval_t *rvp)
 *	Stubs support.
 *
 * Calling/Exit State:
 *	return EINVAL.
 *
 */
/* ARGSUSED */
int
invsys(char *uap, rval_t *rvp)
{
	return EINVAL;
}


/*
 * int
 * nopkg(char *uap, rval_t *rvp)
 *	Stubs support.
 *
 * Calling/Exit State:
 *	return ENOPKG.
 *
 */
/* ARGSUSED */
int
nopkg(char *uap, rval_t *rvp)
{
	return ENOPKG;
}


/*
 * void
 * noreach(void)
 *	Stubs support.
 *
 * Calling/Exit State:
 * 	internal function call for uninstalled package -- panic system.  If 
 *	the system ever gets here, it means that an internal routine was 
 *	called for an optional package, and the OS is in 
 *	trouble.  (STUBS support)
 */

void
noreach(void)
{
	/*
	 *+ Internal function was called for uninstalled package.
	 *+ User can not take any correctibe action.
	 */
	cmn_err(CE_PANIC, "Call to internal routine of uninstalled package");
}

/*
 * void *
 * modbit_track(vaddr_t start_addr, ulong_t nbytes)
 * 	Begins tracking mod bits in at least the specified range
 * 	for the current user address space.
 *
 * Calling/Exit State:
 * 	Returns NULL on failure, or an opaque token.
 *
 * Remarks:
 *	Multiple modbit_track() ranges may be pending on the same
 *      address space, as long as they do not overlap.  Mod bit state
 *      for each range is independent (i.e. when an address is
 *      modified, all ranges currently tracking that address are
 *      marked modified, but these mod bits are cleared independently).
 *      The initial mod bit state is undefined.
 *
 *	Currently, return token is unused.
 */
void *
modbit_track(vaddr_t start_addr, ulong_t nbytes)
{
	hat_start_stats(u.u_procp->p_as, start_addr & PAGEMASK,
			ptob(btopr(start_addr + nbytes) - btop(start_addr)));
	return (void *)1;
}

/*
 * void
 * modbit_cancel(void *token, vaddr_t start_addr, ulong_t nbytes)
 *	Mod bit tracking no longer required for the specified range
 *	of the current user address space.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	start_addr and nbytes must exactly match a previous successful
 *      modbit_track() call, and token is the return value from that
 *      call.
 */
/* ARGSUSED */
void
modbit_cancel(void *token, vaddr_t start_addr, ulong_t nbytes)
{
	ASSERT(token == (void *)1);
	hat_stop_stats(u.u_procp->p_as, start_addr & PAGEMASK,
			ptob(btopr(start_addr + nbytes) - btop(start_addr)));
}

/*
 * void
 * modbit_check(void *token, vaddr_t start_addr, ulong_t nbytes,
 *		uint_t *bitp, boolean_t clear)
 *	Check current mod bits for the specified range in the current
 *	user address space.
 *
 * Calling/Exit State:
 *	start_addr and nbytes must be a subset of a range given to
 *	a previous successful modbit_track() call, and token is the
 *	return value from that call.
 *
 *	bitp points to a bit array (possibly more than one word) with
 *	each bit being the mod bit for a page in the range.
 *
 *	The clear flag is B_TRUE if the mod bits should be cleared
 *	after reading.  Otherwise, they will persist.
 */
/* ARGSUSED */
void
modbit_check(void *token, vaddr_t start_addr, ulong_t nbytes,
		uint_t *bitp, boolean_t clear)
{
	ASSERT(token == (void *)1);
	hat_check_stats(u.u_procp->p_as, start_addr & PAGEMASK,
			ptob(btopr(start_addr + nbytes) - btop(start_addr)),
			bitp, clear);
}
