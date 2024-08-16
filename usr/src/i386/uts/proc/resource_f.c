/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:proc/resource_f.c	1.5"
#ident	"$Header: $"

#include <mem/as.h>
#include <mem/seg_dz.h>
#include <mem/vmparam.h>
#include <proc/mman.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <proc/resource.h>
#include <util/debug.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/types.h>
#include <svc/errno.h>

/*
 * int rlimit_stack(rlim_t curlimit, rlim_t newlimit)
 *	Change the AS layout to accomodate the new stack limits.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *	At the exit of this function, the stack would be able to grow to
 *	to the new limit or would be shrunk by the new limit.
 *
 * Remarks:
 *	This function is processor dependent in choosing the range in
 *	which it grows (clips) the stack. In this architecture where the
 *	stack grows from high to low, the stack is grown (clipped)
 *	from the low end of the existing stack. On architectures where
 *	the stack grows low to high, the stack is grown (clipped) at the
 *	high end.
 */
int
rlimit_stack(rlim_t curlimit, rlim_t newlimit)
{
	proc_t *p = u.u_procp;
	struct as *as = p->p_as;
	struct segdz_crargs a;
	int err = 0;
	vaddr_t base;
	size_t size;
	vaddr_t lowend;

	ASSERT(getpl() == PLBASE);
	ASSERT(curlimit != newlimit);

	as_wrlock(as);

	if (newlimit > curlimit) {
		if (p->p_stkbase == UVSTACK) {
			ASSERT(p->p_stksize <= UVMAX_STKSIZE);
			if (p->p_stksize == min(UVMAX_STKSIZE, newlimit))
				goto bye;
			lowend = UVSTACK - min(UVMAX_STKSIZE, newlimit);
		} else {
			lowend = ptob(1);
#ifdef BUG386B1
		        /*
        		 * Workaround for Intel386(tm) B1 stepping errata #10.
			 * Errata #10 requires that virtual addresses between 
			 * 0x1000 and 0x10000 (pages 1 through 15) never have 
			 * dirty TLB entries if I/O ports in the same range 
			 * are used.
			 *
			 * We can't completely prevent the user from using 
			 * (and dirtying) these addresses, but we attempt to
			 * decrease the likelihood, by not allowing the stack
			 * to include them, and pre-mapping a special segment
			 * which always fails faults.
			 */
			if (do386b1) 
				lowend = ptob(16);
#endif
		}
		base = p->p_stkbase - p->p_stksize;
		if (base == lowend)
			goto bye;
		a.flags = SEGDZ_CONCAT;
		a.prot = PROT_ALL;
		size = min(newlimit - curlimit, base - lowend);
		base -= size;
		err =  as_map(as, base, size, segdz_create, &a);
		if (!err) {
			LOCK(&p->p_mutex, PLHI);
			p->p_stksize += size;
			UNLOCK(&p->p_mutex, PLBASE);
		}
	} else {
		if (p->p_stksize <= newlimit)
			goto bye;
		base = p->p_stkbase - p->p_stksize;
		size = (p->p_stkbase - newlimit) - base;
		err = as_unmap(as, base, size);
		if (!err) {
			LOCK(&p->p_mutex, PLHI);
			p->p_stksize -= size;
			UNLOCK(&p->p_mutex, PLBASE);
		}
	}
bye:
	as_unlock(as);
	return err;
}
