/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ident	"@(#)kern:mem/vm_as.c	1.70"
#ident	"$Header: $"

/*
 * VM - address spaces.
 */

#include <io/async/aiosys.h>
#include <mem/as.h>
#include <mem/faultcode.h>
#include <mem/hat.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/page.h>
#include <mem/seg.h>
#include <mem/seg_kmem.h>
#include <mem/seg_vn.h>
#include <mem/vmmeter.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/mman.h>
#include <proc/proc.h>	
#include <proc/resource.h>
#include <proc/seize.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#ifndef NO_RDMA
#include <mem/rdma.h>
#endif

extern int valid_va_range(vaddr_t *, u_int *, u_int, int);

struct as	kas;

/*
 *+ a_rwslplck: per-as struct read/write sleeplock
 */
LKINFO_DECL(as_lckinfo, "MA::a_rwslplck", 0);

/*
 * struct seg *
 * as_segat(struct as *as, vaddr_t addr)
 * 	Find a segment containing addr.  as->a_seglast is used as a
 * 	cache to remember the last segment hit we had here.
 *
 * Calling/Exit State:
 *	AS is locked on entry and exit to function.
 *
 *	On success, a pointer to the the matching segment is returned; 
 *	on failure the pointer is set NULL.
 *
 * Remarks:
 *	This routine attempts to optimize on the number of segments
 *	searched and on the number of test required per segment searched.
 *
 *	One observation is that consecutive faults in an address
 *	space are likely to be in adjacent or closely placed segments. This
 *	is likely to be the case with shared libraries, or with data and
 *	bss segments in typical families. For this reason, the search
 *	always starts at a_seglast and proceeds to higher or lower
 *	addresses as appropriate.
 *
 *	Another advantage of this method is that should both a_seglast
 *	and the fault address be randomly distributed, only 1/3 of the
 *	segments need be searched on the average (for a hit). A circular
 *	search around the chain would require on the average 1/2 of the
 *	segments to be searched.
 *
 *	Finally, the search method uses only one test per segment in the
 *	search loops, plus 2 or 3 initial tests, and one final test.
 */
struct seg *
as_segat(struct as *as, vaddr_t addr)
{
	vaddr_t base;
	struct seg *seg, *hseg;

	seg = as->a_seglast;
	if (seg == NULL) {         /* address space has no segments */
		ASSERT(as->a_segs == NULL);
		return NULL;
	}

	base = seg->s_base;
	if (base <= addr) {
		/*
		 * First, test for a_seglast hit.
		 */
		if (addr - base < seg->s_size)
			return seg;

		/*
		 * Test for inclusion between a_seglast and the highest
		 * address segment.
		 */
		hseg = as->a_segs->s_prev;
		ASSERT(base <= hseg->s_base);
		ASSERT(hseg->s_size != 0);
		if (addr > hseg->s_base + hseg->s_size - 1)
			return NULL;

		/*
		 * Now, scan forwards from a_seglast till we reach
		 * the desired segment. The scan must terminate
		 * because of the inclusion property just verified.
		 */
		do {
			seg = seg->s_next;
			base = seg->s_base;
			ASSERT(seg->s_size != 0);
		} while (addr > base + seg->s_size - 1);
		if (base > addr)
			return NULL;
	} else {
		/*
		 * Test for inclusion between the lowest address segment
		 * and a_seglast.
		 */
		ASSERT(as->a_segs->s_base <= base);
		if (addr < as->a_segs->s_base)
			return NULL;

		/*
		 * Now, scan backwards from a_seglast till we reach the
		 * desired segment. The scan must terminate because of
		 * the inclusion property just verified.
		 */
		do {
			seg = seg->s_prev;
			base = seg->s_base;
		} while (base > addr);
		ASSERT(seg->s_size != 0);
		if (addr - base >= seg->s_size)
			return NULL;
	}
	as->a_seglast = seg;	/* update hint for next search */

	return seg;
}

/*
 * struct as *
 * as_alloc(void)
 * 	Allocate and initialize an address space data structure.
 * 	We call hat_alloc to allow any machine dependent
 * 	information in the hat structure to be initialized.
 *
 * Calling/Exit State:
 *	Called as part of process creation, the caller guarantees that
 *	this new AS is unknown to the system at large (e.g. can't be found 
 * 	by normal lookups) and hence the newly created AS is returned in an
 *	unlocked state.
 *
 *	This function cannot fail.
 */
struct as *
as_alloc(void)
{
	struct as *as;

	as = kmem_zalloc(sizeof(struct as), KM_SLEEP);

	RWSLEEP_INIT(&as->a_rwslplck, 0, &as_lckinfo, KM_SLEEP);
	(void) hat_alloc(as);

	as->a_whenaged = lbolt;

	/*
	 * PERF:
	 *	The short term memory needs of the new address space
	 * 	should be reflected by an increased deficit, just as those of
	 *	a swapped-in address space are. But is this the right place?
	 *	After all, we don't expect that a matching as_free() reduces
	 *	deficit! 
	 */
	return (as);
}

/*
 * void
 * as_free(struct as *as)
 * 	Free an address space data structure. Need to free the hat first and 
 *	then all the segments on this as and finally the space for the as 
 *	struct itself.
 *
 * Calling/Exit State:
 *	The caller guarantees that by the time they call this function, no
 *	LWPs within the AS are runnable and that the process is no longer
 *	available for access from outside the AS (e.g /proc interfaces).
 *	Thus no AS locking is required.
 *
 *	This function returns nothing useful.
 *
 * Remarks:
 *	After returning from as_free, the caller no longer has any AS  
 *	resources and cannot return to user mode without diasterous
 *	consequences.
 */
void
as_free(struct as *as)
{
	/*
	 * If Asynchronous I/O in use, wait for the IO 
	 * to finish, and unlock the locked down memory.
	 */
	if (u.u_procp->p_aioprocp)
		aio_as_free(as);

	while (as->a_segs != NULL) {
#ifdef DEBUG
		as->a_size -= as->a_segs->s_size;
#endif
		seg_unmap(as->a_segs);
	}
	ASSERT(as->a_size == 0);
	(void)hat_free(as);
	RWSLEEP_DEINIT(&as->a_rwslplck);
	kmem_free(as, sizeof(struct as));
}

/*
 * struct as *
 * as_dup(struct as *as)
 *	Duplicates all segments in an AS by calling appropriate segment
 *	driver dup functions.
 *
 * Calling/Exit State:
 *	The caller must hold a reader lock on its address space during the
 *	duplication process to prevent any other running LWPs in the AS
 *	from changing its constitution (adding more address space, deleting
 * 	or concatenating address space).
 *
 *	The newly created address space does not as yet contain any runnable
 *	LWPs nor is it findable via normal lookup mechanisms. This allows 
 *	us to manipulate the new AS without any special locking. The AS is
 *	returned unlocked.
 *
 *	This function executes with the child AS invisible to the swapper.
 *	As a consequence, translations are not loaded for the child.
 *	as_childload() will be called after the child becomes visible to
 *	the swapper, to load translations, and perhaps to complete other
 *	functions REQUIRED before the child's AS is ready to run.
 *
 *	On success, a pointer to the newly created and duplicated AS is
 *	returned. On failure, the pointer is set to NULL.
 *
 * Remarks:
 * 	as_dup has been restructured so that any pre-COW processing done
 *	on behalf of the newly created segment is entirely encapsulated 
 *	in the segment layers and below. However, it is now necessary
 *	to execute as_childload() later on in the fork.
 */
struct as *
as_dup(struct as *as)
{
	struct as *newas;
	struct seg *seg, *sseg, *newseg;

	newas = as_alloc();

	ASSERT(as);

	sseg = seg = as->a_segs;

	if (seg != NULL) {
		do {
			newseg = seg_alloc(newas, seg->s_base, seg->s_size);
			if (newseg == NULL) {
				as_free(newas);
				return (NULL);
			}
			if (SOP_DUP(seg, newseg) != 0) {
				/*
				 * We call seg_free() on the new seg
				 * because the segment is not set up
				 * completely; i.e. it has no ops.
				 */
				seg_free(newseg);
				as_free(newas);
				return (NULL);
			}
			newas->a_size += seg->s_size;	
			seg = seg->s_next;
		} while (seg != sseg);
	}

	newas->a_prevrss = newas->a_rss;
	newas->a_maxrss = as->a_maxrss;
	newas->a_et_age_interval = as->a_et_age_interval;
	newas->a_init_agequantum = as->a_init_agequantum;
	newas->a_min_agequantum = as->a_min_agequantum;
	newas->a_max_agequantum = as->a_max_agequantum;
	newas->a_agequantum = as->a_agequantum;
	newas->a_wss = (newas->a_size / 2);

	return (newas);
}

/*
 * void
 * as_childload(struct as *pas, struct as *cas)
 *	Loads up translations in the child address space following the
 *	successful execution of an as_dup, and plus complete any other
 *	initialization necessary to prepare the child AS to run.
 *
 * Calling/Exit State:
 *	The caller must hold a reader lock on its address space. That lock
 *	must be held without interruption from before as_dup() was called,
 *	until after this function returns.
 *
 *	The child's does not as yet contain any runnable LWPs. It is also
 *	invisible to /proc. However, it is visible to the swapper (i.e.
 *	its address space may be subjected to as_ageswap()).
 *
 * Remarks:
 *	The segment driver has the option of delaying the COWing of
 *	writable PRIVATE pages and/or changing their protections until the
 *	SOP_CHILDLOAD function. Therefore, this function is not just an
 *	optional performance optimization. It is mandatory to call this
 *	function before allowing the child to run.
 */
void
as_childload(struct as *pas, struct as *cas)
{
	struct seg *pseg, *sseg, *cseg;

	ASSERT(pas);
	ASSERT(cas);

	pseg = pas->a_segs;
	cseg = sseg = cas->a_segs;

	if (cseg != NULL) {
		for (;;) {
			/*
			 * Not all segments are necessarily dup'ed.
			 * Skip those which have not been duped.
			 */
			ASSERT(pseg->s_base <= cseg->s_base);
			if (pseg->s_base == cseg->s_base) {
				ASSERT(pseg->s_size == cseg->s_size);
				ASSERT(pseg->s_ops == cseg->s_ops);
				SOP_CHILDLOAD(pseg, cseg);
				cseg = cseg->s_next;
				if (cseg == sseg)
					break;
			}
			pseg = pseg->s_next;
			ASSERT(pseg != pas->a_segs);
		}
	}
}

/*
 * int
 * as_addseg(struct as *as, struct seg *newseg)
 * 	Add a new segment to the address space, sorting it into the proper 
 *	place in the linked list.
 *
 * Calling/Exit State: 
 *	No AS specific locks need to be held to call or are held on return.
 *
 *	On success, a 0 is returned; on failure, a -1.
 *
 * Remarks:
 *	This function is called from as_exec; The exec code obliterates all
 *	other LWPs from the address space as part of the exec process. 
 *	 
 */
int
as_addseg(struct as *as, struct seg *newseg)
{
	struct seg *seg;
	vaddr_t base;
	vaddr_t eaddr;

	seg = as->a_segs;
	if (seg == NULL) {
		DISABLE_PRMPT();
		newseg->s_next = newseg->s_prev = newseg;
		as->a_segs = as->a_seglast = newseg;
		ENABLE_PRMPT();
	} else {
		/*
		 * Figure out where to add the segment to keep list sorted
		 */
		base = newseg->s_base;
		eaddr = base + newseg->s_size;
		do {
			if (base < seg->s_base) {
				if (eaddr > seg->s_base)
					return (-1);
				break;
			}
			if (base < seg->s_base + seg->s_size)
				return (-1);
			seg = seg->s_next;
		} while (seg != as->a_segs);
		DISABLE_PRMPT();
		newseg->s_next = seg;
		newseg->s_prev = seg->s_prev;
		seg->s_prev = newseg;
		newseg->s_prev->s_next = newseg;
		as->a_seglast = newseg;

		if (base < as->a_segs->s_base)
			as->a_segs = newseg;		/* newseg is at front */
		ENABLE_PRMPT();
	}

	return (0);
}

/*
 * faultcode_t
 * as_fault(struct as *as, vaddr_t addr, u_int size, 
 *	    enum fault_type type, enum seg_rw rw);
 * 	Handle a ``fault'' at addr for size bytes.
 *
 * Calling/Exit State:
 *	The caller guarantees that the AS lock has been grabbed in either
 *	read or write mode (as appropriate) before calling as_fault. The 
 *	lock remains in its entry state on return.
 *
 *	On success, a faultcode of zero is returned; on failure, a non-
 *	zero faultcode is returned to indicate the failure mode.
 */
faultcode_t
as_fault(struct as *as, vaddr_t addr, u_int size, 
	 enum fault_type type, enum seg_rw rw)
{
	struct seg *seg;
	vaddr_t raddr;			/* rounded down addr */
	u_int rsize;			/* rounded up size */
	u_int ssize;
	faultcode_t res = 0;
	vaddr_t addrsav;
	struct seg *segsav;

	switch (type) {

	case F_SOFTLOCK:
		MET_SOFTLOCK(1);
		break;

	case F_PROT:
		MET_PFAULT(1);
		break;

	case F_INVAL:
		MET_VFAULT(1);
		break;
	}

	raddr = (vaddr_t)((u_int)addr & PAGEMASK);
	rsize = (((u_int)(addr + size) + PAGEOFFSET) & PAGEMASK) - (u_int)raddr;

	seg = as_segat(as, raddr);
	if (seg == NULL)
		return (FC_NOMAP);

	addrsav = raddr;
	segsav = seg;

	do {
		if (raddr >= seg->s_base + seg->s_size) {
			seg = seg->s_next;	/* goto next seg */
			if (raddr != seg->s_base) {
				res = FC_NOMAP;
				break;
			}
		}
		if (raddr + rsize > seg->s_base + seg->s_size)
			ssize = seg->s_base + seg->s_size - raddr;
		else
			ssize = rsize;

		res = SOP_FAULT(seg, raddr, ssize, type, rw);
		if (res != 0)
			break;

		raddr += ssize;
		rsize -= ssize;
	} while (rsize != 0);

	/*
	 * If were SOFTLOCKing and we encountered a failure,
	 * we must SOFTUNLOCK the range we already did.
	 */
	if (res != 0 && type == F_SOFTLOCK) {
		for (seg = segsav; addrsav < raddr; addrsav += ssize) {
			if (addrsav >= seg->s_base + seg->s_size)
				seg = seg->s_next;	/* goto next seg */
			/*
			 * Now call the fault routine again to perform the
			 * unlock using S_OTHER instead of the rw variable
			 * since we never got a chance to touch the pages.
			 */
			if (raddr > seg->s_base + seg->s_size)
				ssize = seg->s_base + seg->s_size - addrsav;
			else
				ssize = raddr - addrsav;
			(void) SOP_FAULT(seg, addrsav, ssize,
			    F_SOFTUNLOCK, S_OTHER);
		}
	}

	return (res);
}

/*
 * int
 * as_setprot(struct as *as, vaddr_t addr, u_int size, u_int prot);
 * 	Set the virtual mapping for the interval from [addr : addr + size)
 * 	in address space `as' to have the specified protection. 
 *
 * Calling/Exit State:
 *	The caller must already hold the AS write locked on entry to this
 *	function. The AS is returned write locked.
 *
 * 	On success, a 0 is returned to the caller; on failure the return
 *	value is non-zero, set to the appropriate errno. 
 *
 * Remarks: 
 *	It is ok for the range to cross over several segments,
 * 	as long as they are contiguous.
 *
 *	Holding the AS writelocked guarantees that the specied protection
 *	change is applied atomically to the specified address range. Once
 *	the AS write lock is dropped by the caller, however, another LWP
 *	is free to modify the permissions again.
 */
int
as_setprot(struct as *as, vaddr_t addr, u_int size, u_int prot)
{
	struct seg *seg;
	u_int ssize;
	vaddr_t raddr;			/* rounded down addr */
	u_int rsize;			/* rounded up size */
	int error = 0;

	raddr = (vaddr_t)((u_int)addr & PAGEMASK);
	rsize = (((u_int)(addr + size) + PAGEOFFSET) & PAGEMASK) - (u_int)raddr;

	seg = as_segat(as, raddr);
	if (seg == NULL)
		return (ENOMEM);

	do {
		if (raddr >= seg->s_base + seg->s_size) {
			seg = seg->s_next;	/* goto next seg */
			if (raddr != seg->s_base)
				return (ENOMEM);
		}
		if ((raddr + rsize) > (seg->s_base + seg->s_size))
			ssize = seg->s_base + seg->s_size - raddr;
		else
			ssize = rsize;

		/*
		 * If Asynchronous I/O in use, see if
		 * the ranges overlap.  If they do, 
		 * wait for the IO to finish, and 
		 * unlock the locked down memory.
		 */
		if (u.u_procp->p_aioprocp)
			aio_intersect(as, raddr, ssize);

		error =  SOP_SETPROT(seg, raddr, ssize, prot);
		if (error != 0)
			return (error);

		raddr += ssize;
		rsize -= ssize;
	} while (rsize != 0);

	return (0);
}

/*
 * int
 * as_checkprot(struct as *as, vaddr_t addr, u_int size, u_int prot)
 * 	Check to make sure that the interval from [addr : addr + size)
 * 	in address space `as' has at least the specified protection.
 *
 * Calling/Exit State:
 *	The caller must already hold the AS locked on entry to this
 *	function. The AS is returned locked.
 *
 *	On success, 0 is returned to the caller; on failure, the return
 *	value is non-zero and is set to the appropriate errno.
 *
 * Remarks:
 * 	It is ok for the range to cross over several segments, as long
 * 	as they are contiguous.
 */
int
as_checkprot(struct as *as, vaddr_t addr, u_int size, u_int prot)
{
	struct seg *seg;
	u_int ssize;
	vaddr_t raddr;		/* rounded down addr */
	vaddr_t laddr;
	u_int rsize;		/* rounded up size */
	int error;

	ASSERT(as != &kas);

	raddr = (vaddr_t)((u_int)addr & PAGEMASK);
	rsize = (((u_int)(addr + size) + PAGEOFFSET) & PAGEMASK) - (u_int)raddr;

	seg = as_segat(as, raddr);
	if (seg == NULL)
		return (ENOMEM);

	do {
		if (raddr >= seg->s_base + seg->s_size) {
			seg = seg->s_next;	/* goto next seg */
			if (raddr != seg->s_base)
				return (ENOMEM);
		}
		if ((raddr + rsize) > (seg->s_base + seg->s_size))
			ssize = seg->s_base + seg->s_size - raddr;
		else
			ssize = rsize;

		laddr = raddr;
		while (laddr < raddr + ssize) {
			error = SOP_CHECKPROT(seg, laddr, prot);
			if (error != 0)
				return (error);
			laddr += PAGESIZE;
		}

		rsize -= ssize;
		raddr += ssize;
	} while (rsize != 0);

	return (0);
}

/*
 * int
 * as_unmap(struct as *as, vaddr_t addr, u_int size);
 *	Unmap the specified range fromt the address space.
 *
 * Calling/Exit State:
 *	The caller must already hold the AS write locked on entry to this
 *	function. The AS is returned write locked.
 *
 *	On success, a 0 is returned; on faulure, -1.
 *
 * Remarks:
 * 	The address range indicated for unmapping need not be contained
 *	wholy within a single segment nor must the range begin on a segment
 *	boundary. Segments do not need to be of the same type.
 */
int
as_unmap(struct as *as, vaddr_t addr, u_int size)
{
	struct seg *seg, *seg_next;
	vaddr_t raddr, eaddr;
	u_int ssize;
	vaddr_t obase;

	/*
	 * If Asynchronous I/O in use, see if
	 * the ranges overlap.  If they do, 
	 * wait for the IO to finish, and 
	 * unlock the locked down memory.
	 */
	if (u.u_procp->p_aioprocp)
		aio_intersect(as, addr, size);

	raddr = (vaddr_t)((u_int)addr & PAGEMASK);
	eaddr = (vaddr_t)(((u_int)(addr + size) + PAGEOFFSET) & PAGEMASK);

	seg_next = as->a_segs;
	if (seg_next != NULL) {
		do {
			/*
			 * Save next segment pointer since seg can be
			 * destroyed during the segment unmap operation.
			 * We also have to save the old base.
			 */
			seg = seg_next;
			seg_next = seg->s_next;
			obase = seg->s_base;

			if (raddr >= seg->s_base + seg->s_size)
				continue;		/* not there yet */

			if (eaddr <= seg->s_base)
				break;			/* all done */

			if (raddr < seg->s_base)
				raddr = seg->s_base;	/* skip to seg start */

			if (eaddr > (seg->s_base + seg->s_size))
				ssize = seg->s_base + seg->s_size - raddr;
			else
				ssize = eaddr - raddr;

			if (SOP_UNMAP(seg, raddr, ssize) != 0)
				return (-1);

			as->a_size -= ssize;
			raddr += ssize;

			/*
			 * Check to see if we have looked at all the segs.
			 *
			 * We check a_segs because the unmaps above could
			 * have unmapped the last segment.
			 */
		} while (as->a_segs != NULL && obase < seg_next->s_base);
	}

	return (0);
}

/*
 * int
 * as_map(struct as *as, vaddr_t addr, u_int size, int (*crfp)(), void *argsp)
 *	Map in the specified address range to the address space specified.
 *
 * Calling/Exit State:
 *	The caller must already hold the AS write locked on entry to this
 *	function. The AS is returned write locked.
 *
 * 	On success, 0 is returned; on failure, the return code is non-zero
 *	and indicates the appropriate errno.
 *
 * Remarks: 
 *	as_unmap or as_gap have already been called to insure that we can map 
 *	in the new range. Requiring that the caller hold the AS write locked
 *	across both the unmap/gap and the map gurantees that no other LWP can 
 *	map into the reserved hole.
 *
 *	The a_seglast hint is now set by routine as_addseg. Should the the
 *	create routine merge the new segment into an existing segment,
 *	seg_detach will adjust the a_seglast hint.
 */
int
as_map(struct as *as, vaddr_t addr, u_int size, int (*crfp)(), void *argsp)
{
	struct seg *seg;
	vaddr_t raddr;			/* rounded down addr */
	u_int rsize;			/* rounded up size */
	int error;
	size_t limit = u.u_rlimits->rl_limits[RLIMIT_VMEM].rlim_cur;

	raddr = (vaddr_t)((u_int)addr & PAGEMASK);
	rsize = (((u_int)(addr + size) + PAGEOFFSET) & PAGEMASK) - (u_int)raddr;

	if (limit == RLIM_INFINITY || limit > UVEND - UVBASE)
		limit = UVEND - UVBASE;

	if (as->a_size + rsize > limit)
		return (ENOMEM);

	seg = seg_alloc(as, addr, size);
	if (seg == NULL)
		return (ENOMEM);

	error = (*crfp)(seg, argsp);
	if (error != 0) {
		seg_free(seg);
	} else {
		/*
		 * add size now so as_unmap will work if as_ctl fails
		 */
		as->a_size += rsize;

		if (as->a_paglck) {
			error = as_ctl(as, addr, size, MC_LOCK, 0,(void *)NULL);
			if (error != 0)
				(void) as_unmap(as, addr, size);
		}
	}

	return (error);
}

/*
 * int
 * as_gap(struct as *as, u_int minlen, vaddr_t *basep, u_int *lenp, 
 *        int flags, vaddr_t addr)
 * 	Find a hole of at least size minlen within [base, base+len).
 * 
 * Calling/Exit State:
 *	The caller must already hold the AS write locked on entry to this
 *	function. The AS is returned write locked.
 *
 *	On success, 0 is returned; on failure a -1. The meaning of the
 *	failure value is affected by which optional flags are passed in.
 *	See the description, below.
 *
 * Description:
 * 	If flags specifies AH_HI, the hole will have the highest possible 
 *	address in the range. Otherwise, it will have the lowest possible 
 * 	address. If flags specifies AH_CONTAIN, the hole will contain the 
 *	address addr. If an adequate hole is found, base and len are set to 
 *	reflect the part of the hole that is within range, and 0 is returned. 
 *	Otherwise, -1 is returned.
 *
 * Remarks:
 * 	XXX This routine is not correct when base+len overflows vaddr_t.
 *
 *	PERF: this function could be revisted and made more efficient.
 */
/* VARARGS5 */
int
as_gap(struct as *as, u_int minlen, vaddr_t *basep, u_int *lenp, 
       int flags, vaddr_t addr)
{
	vaddr_t lobound, hibound;
	vaddr_t lo, hi;
	struct seg *seg, *sseg;

	lobound = *basep;
	hibound = lobound + *lenp;
	if (lobound > hibound)		/* overflow */ 
		return (-1);

	sseg = seg = as->a_segs;
	if (seg == NULL) {
		if (valid_va_range(basep, lenp, minlen, flags & AH_DIR))
			return (0);
		else
			return (-1);
	}

	if ((flags & AH_DIR) == AH_LO) {	/* search from lo to hi */
		lo = lobound;
		do {
			hi = seg->s_base;
			if (hi > lobound && hi > lo) {
				*basep = MAX(lo, lobound);
				*lenp = MIN(hi, hibound) - *basep;
				if (valid_va_range(basep,lenp,minlen,AH_LO) &&
				    ((flags & AH_CONTAIN) == 0 ||
				    (*basep <= addr && *basep + *lenp > addr)))
					return (0);
			}
			lo = seg->s_base + seg->s_size;
		} while (lo < hibound && (seg = seg->s_next) != sseg);

		if (hi < lo) 
			hi = hibound;

		/* check against upper bound */
		if (lo < hibound) {
			*basep = MAX(lo, lobound);
			*lenp = MIN(hi, hibound) - *basep;
			if (valid_va_range(basep, lenp, minlen, AH_LO) &&
			    ((flags & AH_CONTAIN) == 0 ||
			    (*basep <= addr && *basep + *lenp > addr)))
				return (0);
		}
	} else {				/* search from hi to lo */
		seg = seg->s_prev;
		hi = hibound;
		do {
			lo = seg->s_base + seg->s_size;
			if (lo < hibound && hi > lo) {
				*basep = MAX(lo, lobound);
				*lenp = MIN(hi, hibound) - *basep;
				if (valid_va_range(basep,lenp,minlen,AH_HI) &&
				    ((flags & AH_CONTAIN) == 0 ||
				    (*basep <= addr && *basep + *lenp > addr)))
					return (0);
			}
			hi = seg->s_base;
		} while (hi > lobound && (seg = seg->s_prev) != sseg);

		if (lo > hi)
			lo = lobound;

		/* check against lower bound */
		if (hi > lobound) {
			*basep = MAX(lo, lobound);
			*lenp = MIN(hi, hibound) - *basep;
			if (valid_va_range(basep, lenp, minlen, AH_HI) &&
			    ((flags & AH_CONTAIN) == 0 ||
			    (*basep <= addr && *basep + *lenp > addr)))
				return (0);
		}
	}
	return (-1);
}

/*
 * int
 * as_memory(struct as *as, vaddr_t *basep, u_int *lenp)
 * 	Return the next range within [base, base+len) that is backed
 * 	with "real memory".
 *
 * Calling/Exit State:
 *	No explicit lock on the AS is required to be held by
 *	the caller, but they are expected to secure the sanity of the 
 *	address space by other means.	
 *
 * 	On success, 0 is returned; on failure a non-zero value is 
 *	returned and indicates the appropriate errno.	
 *
 * Remarks:
 * 	We're lazy and only return one segment at a time.
 */
int
as_memory(struct as *as, vaddr_t *basep, u_int *lenp)
{
	struct seg *seg, *sseg;
	vaddr_t addr, eaddr;
	struct seg *cseg = NULL;
	int err;

	addr = *basep;
	eaddr = addr + *lenp;

	sseg = seg = as->a_seglast;
	if (seg == NULL)
		return(EINVAL);

	do {
		if (seg->s_base <= addr &&
		    addr < (seg->s_base + seg->s_size)) {
			/* found a containing segment */
			as->a_seglast = seg;
			if (addr < seg->s_base)
				*basep = seg->s_base;
			if (eaddr > seg->s_base + seg->s_size)
				eaddr = seg->s_base + seg->s_size;
			*lenp = eaddr - addr;

			err = SOP_MEMORY(seg, basep, lenp);
			if (!err)
				return (0);
		} else if (seg->s_base > addr) {
			if (cseg == NULL || cseg->s_base > seg->s_base) {
				/*
				 * Save closest seg above the range.
				 * We have to keep scanning because
				 * we started with the hint instead
				 * of the beginning of the list.
				 */
				cseg = seg;
			}
		}
	} while ((seg = seg->s_next) != sseg);

	if (cseg == NULL)		/* no valid segs within range */
		return (EINVAL);

	/*
	 * Only found a close segment, see if there's
	 * a valid range we can return.
	 */
	if (cseg->s_base >= eaddr)	/* closest segment is out of range */
		return (ENOMEM);

	if (cseg->s_base > addr)
		*basep = cseg->s_base;
	if (eaddr > cseg->s_base + cseg->s_size)
		eaddr = cseg->s_base + cseg->s_size;

	*lenp = eaddr - addr;

	as->a_seglast = cseg;		/* reset hint */
	return(SOP_MEMORY(cseg, basep, lenp));
}

/*
 * int
 * as_incore(struct as *as, vaddr_t addr, u_int size, char *vec, u_int *sizep)
 * 	Determine whether data from the mappings in interval [addr : addr + 
 *	size) are in the primary memory (core) cache.
 *
 * Calling/Exit State:
 *	The caller must already hold the AS locked on entry to this
 *	function. The AS is returned locked.
 *
 *	On success (all pages in the specificed range are incore) a 0 
 *	is returned. On failure, a -1 is returned.
 *
 * Remarks:
 *	Given the transient nature of all pages not memory locked, the
 *	information returned by this call is more or less accurate to the
 *	extent that the caller guarantees the stability of the address
 *	space constitution and/or prevents swapping and page faults.
 */
int
as_incore(struct as *as, vaddr_t addr, u_int size, char *vec, u_int *sizep)
{
	struct seg *seg;
#ifdef lint 
	u_int ssize = 0;
#else
	u_int ssize;
#endif /* lint */
	vaddr_t raddr;		/* rounded down addr */
	u_int rsize;		/* rounded up size */
	u_int isize;		/* iteration size */

	*sizep = 0;
	raddr = (vaddr_t)((u_int)addr & PAGEMASK);
	rsize = ((((u_int)addr + size) + PAGEOFFSET) & PAGEMASK) - (u_int)raddr;

	seg = as_segat(as, raddr);
	if (seg == NULL)
		return (-1);
	for (; rsize != 0; rsize -= ssize, raddr += ssize) {
		if (raddr >= seg->s_base + seg->s_size) {
			seg = seg->s_next;
			if (raddr != seg->s_base)
				return (-1);
		}
		if ((raddr + rsize) > (seg->s_base + seg->s_size))
			ssize = seg->s_base + seg->s_size - raddr;
		else
			ssize = rsize;
		*sizep += isize =
		    SOP_INCORE(seg, raddr, ssize, vec);
		if (isize != ssize)
			return (-1);
		vec += btopr(ssize);
	}
	return (0);
}

/*
 * int
 * as_ctl(struct as *as, vaddr_t addr, u_int size, int func, int attr,
 *        void *arg)
 * 	Cache control operations over the interval [addr : addr + size) in 
 * 	address space "as".
 *
 * Calling/Exit State:
 *	The caller must already hold the AS locked on entry to this
 *	function. The AS is returned locked. The AS may be read or 
 *	write locked depending on the function specfied.
 *
 *	On success a zero is returned and the specfied function has been
 *	performed on the range [addr : addr + size).
 *
 *      On failure, a -1 is returned.
 */
int
as_ctl(struct as *as, vaddr_t addr, u_int size, int func, int attr,
       void *arg)
{
	struct seg *seg;	/* working segment */
	struct seg *sseg;	/* first segment of address space */
	vaddr_t raddr;		/* rounded down addr */
	u_int rsize;		/* rounded up size */
	u_int ssize;		/* size of seg */
	int error;		/* result */

	/*
	 * If these are address space lock/unlock operations, loop over
	 * all segments in the address space, as appropriate.
	 */
	if (func == MC_LOCKAS) {

		if ((int)arg & MCL_FUTURE)
			as->a_paglck = 1;
		if (((int)arg & MCL_CURRENT) == 0)
			return (0);

		sseg = seg = as->a_segs;
		if (seg == NULL)
			return(0);
		do {
			error = SOP_LOCKOP(seg, seg->s_base,
						seg->s_size, attr, func);
			if (error != 0)
				return (error);
		} while((seg = seg->s_next) != sseg);

		return (0);
	} else if (func == MC_UNLOCKAS) {
		as->a_paglck = 0;

		sseg = seg = as->a_segs;
		if (seg == NULL)
			return(0);
		do {
			/*
			 * If Asynchronous I/O in use, see if
			 * the ranges overlap.  If they do, 
			 * wait for the IO to finish, and 
			 * unlock the locked down memory.
			 */
			if (u.u_procp->p_aioprocp)
				aio_intersect(as, seg->s_base, seg->s_size);

			error = SOP_LOCKOP(seg, seg->s_base, seg->s_size,
						attr, func);
			if (error != 0)
				return (error);
		} while((seg = seg->s_next) != sseg);

		return (0);
	}

	/*
	 * Normalize addresses and sizes.
	 */
	raddr = (vaddr_t)((u_int)addr & PAGEMASK);
	rsize = (((u_int)(addr + size) + PAGEOFFSET) & PAGEMASK) - (u_int)raddr;

	/*
	 * Get initial segment.
	 */
	if ((seg = as_segat(as, raddr)) == NULL)
		return (ENOMEM);

	/*
	 * Loop over all segments.  If a hole in the address range is
	 * discovered, then fail.  For each segment, perform the appropriate
	 * control operation.
	 */

	while (rsize != 0) {

		/*
		 * Make sure there's no hole, calculate the portion
		 * of the next segment to be operated over.
		 */
		if (raddr >= seg->s_base + seg->s_size) {
			seg = seg->s_next;
			if (raddr != seg->s_base) 
				return (ENOMEM);
		}
		if ((raddr + rsize) > (seg->s_base + seg->s_size))
			ssize = seg->s_base + seg->s_size - raddr;
		else
			ssize = rsize;

		/*
		 * Dispatch on specific function.
		 */
		switch (func) {

		/*
		 * Synchronize cached data from mappings with backing
		 * objects.
		 */
		case MC_SYNC:
			if (error = SOP_SYNC(seg, raddr, ssize, 
					     attr, (u_int)arg))
				return (error);
			break;

		/*
		 * Lock pages in memory.
		 */
		case MC_LOCK:
			if (error = SOP_LOCKOP(seg, raddr, ssize, attr, func))
				return (error);
			break;

		/*
		 * Unlock mapped pages.
		 */
		case MC_UNLOCK:
			/*
			 * If Asynchronous I/O in use, see if
			 * the ranges overlap.  If they do, 
			 * wait for the IO to finish, and 
			 * unlock the locked down memory.
			 */
			if (u.u_procp->p_aioprocp)
				aio_intersect(as, raddr, ssize);

			(void) SOP_LOCKOP(seg, raddr, ssize, attr, func);
			break;

		/*
		 * Can't happen.
		 */
		default:
			/*
			 *+ as_ctl was called to perform an unrecognized
			 *+ request. This can technically ``never happen''
			 *+ and may be the symptom of problems with other
			 *+ part of the kernel or the hardware.
			 */
			cmn_err(CE_PANIC, "as_ctl: bad operation %d", func);
			/* NOTREACHED */
		}

		rsize -= ssize;
		raddr += ssize;
	}
	return (0);
}


/*
 * u_int
 * as_getprot(struct as *as, vaddr_t addr, vaddr_t *naddr)
 *	Determine and return the protection used to map the page at addr
 *	and set the outarg naddr to indicate the first address past addr
 *	which uses *different* protection.
 *
 * Calling/Exit State:
 *      The caller must hold the AS locked on entry to this function or
 *	guarantee by other means that no other LWP in the AS is active
 *	at this time. In addition, the caller must insure that addr is 
 *	currently mapped into the indicated address space so that as_segat 
 *	does not fail.
 *
 *	This function cannot fail (i.e. it relies on its caller to insure
 *	the validity of the passed arguments). The return value is equal
 *	to the protection used to map the page at addr (the page may or 
 *	may not be currently valid). The outarg naddr is set to indicate
 *	either:
 *
 *		- The first address past addr within the segment which uses a
 *	 	  protection mode different from that of addr.
 *
 *	OR:
 *
 *		- The first address past the end of the segment.
 *
 *	This allows the caller to determine the logical protection sub-     
 *	segmentation of the address space. See prnsegs() and elfcore()
 *	for an example of how this is used.
 *
 * Remarks:
 *	If the AS lock is not held across multiple calls to as_getprot
 *	for the same address space then the validity of the protection 
 *	returned and the value of naddr may be suspect.
 */
u_int
as_getprot(struct as *as, vaddr_t addr, vaddr_t *naddr)
{
	struct seg *seg;
 	vaddr_t eaddr;
	u_int prot;
	u_int nprot;

	ASSERT(as != &kas);

	seg = as_segat(as, addr);

	if (seg == NULL) {
		/*
	 	 *+ A call to as_setprot was made for an address unmapped 
	 	 *+ in the indicated address space. This function assumes
	 	 *+ that the caller both stabilizes the passed address space
	 	 *+ (by getting an lock on the address space) and validates
	 	 *+ that the passed addr is currently mapped by calling 
 		 *+ as_segat(). This is an unrecoverable situation and probably
		 *+ indicates a more profound problem in the caller.
	 	 */
		cmn_err(CE_PANIC, "as_getprot: addr not mapped");
	}

	/*
	 * Loop over the range from addr to addr + seg->s_size calling
	 * SOP_GETPROT for each page. Quit and return when we either
	 * reach the end of the segment or find a page whose protection
	 * is different from the initial value of addr.
	 */

        eaddr = seg->s_base + seg->s_size;
	addr &= PAGEMASK;
	ASSERT(addr < eaddr);
	SOP_GETPROT(seg, addr, &prot);
	while ((addr += PAGESIZE) < eaddr) {
		SOP_GETPROT(seg, addr, &nprot);
		if (nprot != prot)
                        break;
	}

	/*
	 * Set the naddr outarg. This is equal to either the first page
	 * past the end of the segment (indicating all pages from addr to
	 * addr + seg->s_size have the same protection) or the first page
	 * within the segment with permissions different from those of
	 * addr.
	 */

	*naddr = addr;
	return prot;
}

/* 
 * int
 * as_exec(struct as *oas, vaddr_t ostka, u_int stksz, struct as *nas, 
 *         vaddr_t nstka, u_int hatflag)
 * 	Special code for exec to move the stack segment from its interim
 * 	place in the old address to the right place in the new address space.
 *
 * Calling/Exit State:
 *	No explicit lock on the AS is required to be held by
 *	the caller, but they are expected to secure the sanity of both the 
 *	new and old address spaces by other means.	
 *
 *	Opaquely passes the return value from hat_exec back to the caller.
 *
 * Remarks:
 *	as_addseg can fail, but we ignore this possibility.
 * 
 */
int
as_exec(struct as *oas, vaddr_t ostka, u_int stksz, struct as *nas, 
        vaddr_t nstka, u_int hatflag)
{
	struct seg *stkseg;

	stkseg = as_segat(oas, ostka);
	ASSERT(stkseg != NULL);
	ASSERT(stkseg->s_base == ostka && stkseg->s_size == stksz);
	seg_detach(stkseg);
	stkseg->s_as = nas;
	stkseg->s_base = nstka;
	nas->a_size += stkseg->s_size;
	oas->a_size -= stkseg->s_size;
	nas->a_wss = (nas->a_size / 2);
	(void) as_addseg(nas, stkseg);
	return(hat_exec(oas, ostka, stksz, nas, nstka, hatflag));
}

/*
 * faultcode_t
 * as_prmapin(struct as *as, vaddr_t uvaddr, enum seg_rw rw, vaddr_t *kvaddrp,
 *	      as_prmapcookie_t *cookiep)
 *	Provide a kernel virtual mapping to an arbitrary user virtual address.
 *
 * Calling/Exit State:
 *	Given a user virtual address, uvaddr, in an address space, as, which
 *	is not necessarily the current context's address space, which the
 *	caller wishes to read (rw == S_READ) or write (rw == S_WRITE),
 *	as_prmapin() returns a kernel virtual address, in (*kvaddrp), which
 *	has been mapped to the desired user page.  The cookie returned in
 *	(*cookiep) must be passed to the matching as_prmapout() call.
 *
 *	The uvaddr address may be anywhere within a page, and the returned
 *	(*kvaddrp) will be correspondingly aligned.  The caller may only
 *	access data within that page.  The page is guaranteed to be locked in
 *	memory until as_prmapout() is called.
 *
 *      The caller must already hold the AS locked on entry to this function.
 *	The AS is returned locked. It must remain locked until after
 *	as_prmapout() is called.
 *
 *	Since this function may SOFTLOCK the pages, and since the AS lock
 *	will still be held, the caller may not subsequently call as_fault(),
 *	directly or indirectly (e.g. it may not touch user addresses), until
 *	the mapping is released by calling as_prmapout().
 *
 *	On success, 0 is returned; otherwise the fault code is returned.
 *
 * Description:
 *	In the nominal case, as_prmapin() does an F_MAXPROT_SOFTLOCK fault
 *	on the requested address, finds the physical page ID for the page
 *	mapped at that address, and maps it into a kernel virtual with
 *	kpg_ppid_mapin().  However, for better performance, if the page is
 *	already present, there is a translation with sufficient permissions,
 *	and we can try-lock the page, we directly acquire a read-lock on the
 *	page and map it in with kpg_pl_mapin().
 */
faultcode_t
as_prmapin(struct as *as, vaddr_t uvaddr, enum seg_rw rw, vaddr_t *kvaddrp,
	   as_prmapcookie_t *cookiep)
{
	faultcode_t fc;
	uint_t pgoff = (uvaddr & PAGEOFFSET);
	page_t *pp;
	uint_t prot = (rw == S_WRITE ? PROT_WRITE : PROT_READ);

	ASSERT(as != (struct as *)NULL);
	ASSERT(!KADDR(uvaddr));

	if ((pp = hat_vtopp(as, uvaddr, rw)) != NULL) {
		/*
		 * Fast-path optimization.  The page is already present,
		 * with sufficient permissions, and is now read-locked.
		 * Just map it in and leave it locked.
		 */
		*kvaddrp = (vaddr_t)kpg_pl_mapin(1, pp, prot, SLEEP) + pgoff;
		*(page_t **)cookiep = pp;
		return (faultcode_t)0;
	}

	fc = as_fault(as, uvaddr & PAGEMASK, PAGESIZE, F_MAXPROT_SOFTLOCK, rw);
	if (fc)
		return fc;

	*kvaddrp = (vaddr_t)kpg_ppid_mapin(1, hat_vtoppid(as, uvaddr), prot,
					   SLEEP) + pgoff;

	*(page_t **)cookiep = (page_t *)NULL;
	return (faultcode_t)0;
}

/*
 * void
 * as_prmapout(struct as *as, vaddr_t uvaddr, vaddr_t kvaddr,
 *	       as_prmapcookie_t cookie)
 *	Release a mapping obtained from as_prmapin().
 *
 * Calling/Exit State:
 *	Called with the same as and uvaddr as the prior call to as_prmapin(),
 *	along with the kvaddr and cookie returned by as_prmapin().
 *
 *      The caller must already hold the AS locked on entry to this function.
 *	The AS is returned locked.
 */
void
as_prmapout(struct as *as, vaddr_t uvaddr, vaddr_t kvaddr,
	    as_prmapcookie_t cookie)
{
	page_t *pp;

	/* Unmap the kernel virtual */
	kpg_mapout((void *)(kvaddr & PAGEMASK), 1);

	if ((pp = (page_t *)cookie) != NULL) {
		/* Fast-path was used; just unlock the page */
		page_unlock(pp);
	} else {
		/* Need to do the full F_SOFTUNLOCK */
		(void) as_fault(as, uvaddr & PAGEMASK, PAGESIZE, F_SOFTUNLOCK,
				S_OTHER);
	}
}

/*
 * void
 * as_age(void)
 * 
 *	Age/Trim the address space to which the caller LWP belongs. 
 *	Unload unlocked mappings that were not referenced over
 *	the recent aging interval. Pages are freed when no mappings
 *	remain. 
 *
 * Calling/Exit State:
 *
 *	No spin locks are held by the caller, none are held upon return.
 *	The caller must be an LWP from a non-kernel address space. Process
 *	p_mutex will be acquired and released. The LWP l_mutexes for the
 *	LWPs in the process may be acquired and released.
 *	
 *	Upon return, the address space working set information is updated, 
 *	in order to restart the aging interval. 
 */
void
as_age(void)
{
	lwp_t	*lwpp;
	proc_t	*procp;		/* process pointer for calling LWP */
	struct as *asp;		/* process address space pointer */
	size_t nonlocked_rss;
	struct seg *lastsegp;
	vaddr_t lastaddr;
	int	newquantum;
	boolean_t was_multi_threaded = B_TRUE;
	
	lwpp = CURRENT_LWP();
	ASSERT(lwpp);
	procp = lwpp->l_procp;
	ASSERT(procp);
	asp = procp->p_as;
	ASSERT(asp);
	nonlocked_rss = asp->a_rss - asp->a_lockedrss;
	ASSERT(nonlocked_rss >= (size_t)0);
	ASSERT(KS_HOLD0LOCKS());	
	ASSERT(getpl() == PLBASE);

	/*
	 * First we check to see if there is anything to get back from
	 * aging, or if memory is so plentiful that there is no reason
	 * to age at this time. In either case, cancel the aging step.
	 */ 
	if (nonlocked_rss <= nonlocked_minpg ||
	    (mem_avail_state == MEM_AVAIL_EXTRA_PLENTY &&
	    !IS_TRIM_NEEDED(asp))) {
		/*
		 * The process does not have enough nonlocked memory for it
		 * to be a worthwhile aging candidate, at this point. Just
		 * restart the aging counters and unpost the aging notice
		 * to other LWPs before returning.
		 */
		if (!SINGLE_THREADED()) {
			(void)LOCK(&procp->p_mutex, PLHI);
			trapevunproc(procp, EVF_L_ASAGE, B_TRUE);
			UNLOCK(&procp->p_mutex, PLBASE);	
		} else {
			UNPOST_AGING_EVENT_SELF();
		}
		/*
		 * Unprotected update of non-critical data. Besides, a race
		 * for updating the aging counters should be very rare indeed.
		 */
		procp->p_nonlockedrss = nonlocked_rss;
		RESET_AGING_INFO(asp, asp->a_agequantum);
		return;
	}

	(void)LOCK(&procp->p_mutex, PLHI);
	if (CAN_SKIP_AGING(procp)) {
		UNLOCK(&procp->p_mutex, PLBASE);
		UNPOST_AGING_EVENT_SELF();
		return;
	}
	procp->p_flag |= P_LOCAL_AGE;

	if (SINGLE_THREADED()) {
		was_multi_threaded = B_FALSE;
		UNLOCK(&procp->p_mutex, PLBASE);
		/*
		 * In the single threaded case, even the AS read lock
		 * is not necessary. (Assuming /proc cannot affect the
		 * segment chain consistency, and that any autogrowth
		 * stack will be supported entirely within a segment.
		 */
	} else {
		trapevunproc(procp, EVF_L_ASAGE, B_FALSE);
		if (LOCAL_AGE_LOCK(procp) == B_FALSE) {
			/* p_mutex dropped by LOCAL_AGE_LOCK */	
			ASSERT(KS_HOLD0LOCKS());
			UNPOST_AGING_EVENT_SELF();
			return;
		}
	}

	UNPOST_AGING_EVENT_SELF();
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * We have the necessary serialization to proceed with aging.
	 *
	 * We decide whether trimming is to be performed by testing if
	 * the RSS is above the trim threshold. In some cases, this will
	 * cause a trim step to occur, eventhough the AS was ready for
	 * a full aging step. Since the aging counters are not restored
	 * when a trim operation is done, the process will get aged fully
	 * in the near future.
	 */

	if (IS_TRIM_NEEDED(asp)) {
		int num_vpages_trimmed = 0;
		size_t	rss_before;
		size_t	rss_after;
		struct seg *trimsegp = NULL;
		vaddr_t next_startaddr;
		vaddr_t trimseg_lastaddr;
		
		
		/* 
		 * Remark about how hat_agerange is called in this (AS_TRIM) 
		 * case: hat_agerange is expected to return either:
		 *
		 * (1) 	the next unloadable address within the specified 
		 *	range, if MAXTRIM pages were unloaded, or
		 *
		 * (2) 	otherwise, the last address in the specified range.
		 *
		 * In case (1), we just cache the returned address for use 
		 * in the next trimming event. Otherwise, in case (2), we must 
		 * set the cached address to NULL, or the first virtual
		 * address of the first segment -- it does not matter). In 
		 * some cases, this will mean that less than MAXTRIM pages 
		 * will be unloaded, but that is not a harmful occurrence.
		 */

		lastsegp = asp->a_segs->s_prev;
		lastaddr = lastsegp->s_base + lastsegp->s_size;
		do {

			/*
			 * The cached address, a_trimnext could be beyond 
			 * lastaddr, if segments got unmapped meanwhile. 
			 * Or, it could be inside a segment that got 
			 * unmapped, as well.
			 */
			if ((asp->a_trimnext >= lastaddr) ||
				((trimsegp = as_segat(asp, asp->a_trimnext)) 
					== NULL)) {
				asp->a_trimnext = asp->a_segs->s_base;
				trimsegp = asp->a_segs;
			}

			trimseg_lastaddr = (trimsegp->s_base + 
						trimsegp->s_size);

			/*
			 * If current segment is the last segment, then
			 * set next_startaddr to be just past the largest
			 * legal address in the AS; otherwise, set it to
			 * the first legal address in the next segment.
			 */
			next_startaddr = ((trimsegp == lastsegp) ?
				lastaddr : trimsegp->s_next->s_base);

			ASSERT (next_startaddr >= asp->a_trimnext);

			rss_before = asp->a_rss;

			if ((trimsegp->s_ops != NULL) &&
					(trimsegp->s_ops->sop_age != NULL)) {
				/*
				 * The segment where trimming is to begin 
				 * contains a self-aging function. Pass 
				 * control to it. It is possible that there 
				 * may not be enough in the segment to be 
				 * trimmed, so continue on to the next 
				 * segment if necessary.
				 */
				SOP_AGE(trimsegp, AS_TRIM);
				asp->a_trimnext = next_startaddr;
			} else {
				/*
				 * Below, we call hat_agerange over a range
				 * covering the remainder of the current
				 * segment and extending over any 
				 * inter-segment gap that may be present, upto
				 * the first address in the next segment, if
				 * the current segment is not the last segment
				 * in the address space. Otherwise, the outer
				 * end of the range is just the first address
				 * past the end of the segment.
				 *
				 * Then, if the address returned by 
				 * hat_agerange is not within the current
				 * segment, the next trim address will be
				 * at the start of the next segment. This
				 * assures that we will proceed in the
				 * direction of increasing addresses, and 
				 * so the loop will terminate.
				 */
			
				asp->a_trimnext = hat_agerange(asp, 
						asp->a_trimnext, next_startaddr, 						AS_TRIM);
				if (asp->a_trimnext >= trimseg_lastaddr) {
					asp->a_trimnext = next_startaddr;
				}
			}
			rss_after = asp->a_rss;
			num_vpages_trimmed += MAX(0, (rss_before - rss_after));
			asp->a_prevrss = asp->a_rss;
		} while ((num_vpages_trimmed < MAXTRIM) && 
					(trimsegp != lastsegp));
	} else {
		/*
		 * call as_ageswap to do the actual aging.
		 */
		as_ageswap(asp, AS_AGE);
		asp->a_wss = asp->a_rss;
		newquantum = (((int)asp->a_agequantum + 
				asp->a_init_agequantum) / 2);
		RESET_AGING_INFO(asp, newquantum);
	} 

	/*
	 * Perform the necessary unlocking/unseizing of the address space.
	 * And, clear the P_LOCAL_AGE flag.
	 */
	LOCK(&procp->p_mutex, PLHI);
	procp->p_flag &= ~P_LOCAL_AGE;
	UNLOCK(&procp->p_mutex, PLBASE);

	if (was_multi_threaded) 
		LOCAL_AGE_UNLOCK(procp);
	return;
}

/*
 * void
 * as_ageswap(struct as *asp, int type)
 *
 *	Either unload the address space to remove unreferenced, unlocked
 *	mappings for a process that is aging itself, or remove all 
 *	unlocked mappings for a process being swapped out. The "type" 
 * 	argument that is passed in, is passed on to the hat_agerange function. 
 *	The type can be one of {AS_AGE, AS_TRIM, AS_SWAP}; if AS_AGE is
 *	specified, then hat_agerange will not remove referenced translations.
 *	While both AS_TRIM and AS_SWAP specify that unlocked translations
 *	are to be removed (independent of reference information), AS_TRIM
 *	indicates that a limited number of such translations should be removed.
 *
 * Calling/Exit State:
 *
 *	In general, the process is seized, and no spin locks are held on
 *	entry or exit. However, there are two exceptions:
 *
 *	    1) Depending upon the platform, it is possible that in one
 *	       case, as_ageswap may be called _without_ the process being
 *	       seized; this is when the address space is aged by an LWP
 *	       within the address space. If this is the case on a given
 *	       platform, and the process has more than one LWP, then the AS
 *	       will be held read locked by the calling LWP during local
 *	       aging.
 *
 *	    2) The process is the parent of a vfork() and the child
 *	       has already unloaded the address space. If the child
 *	       detects that the parent has been seized, then it needs
 *	       to shred the address space. In this case, the
 *	       address space is stabilized because the parent is waiting
 *	       in vfwait() [or is executing in the kernel headed towards
 *	       vfwait().
 *
 *	This function does not block.
 */

void
as_ageswap(struct as *as, int type)
{
	struct seg *firstseg = as->a_segs;	
	struct seg *seg, *nextseg;	
	vaddr_t	start, end;
	struct seg_ops *ops;

	ASSERT(KS_HOLD0LOCKS());

	ASSERT(firstseg);
	seg = firstseg;

	/*
	 * For each segment,
	 * 	If a segment has an aging function defined for 
	 * it, call the function to perform the aging. 
	 * 	Otherwise age the address range for the segment by 
	 * calling hat_agerange. As a performance optimization, glue 
	 * two or more segment address ranges that are contiguous,
	 * eventhough the segments may not be concatenable for other
	 * reasons.
	 */


	do {

		ASSERT(seg->s_next->s_prev == seg);
		ASSERT(seg->s_prev->s_next == seg);

		/* 
		 * We're here exactly when beginning with a new
		 * (yet to be aged) address range. 
		 * seg points to to the first segment in the new range. 
		 * Also, all previous ranges have been aged at this point.
		 *
		 * It is possible that some LWP in the process may be holding 
		 * its AS write locked, and may in fact be creating a segment. 
		 * Thus, another serialization convention is required to 
		 * guarantee consistent access to the segment chain for aging. 
		 *
		 * The AS level routines and the segment creation routines 
		 * follow the convention that the segment chain is left intact 
		 * at each preemption point. The additional convention we 
		 * follow here is:
		 *
		 *	The segment is ignored if it does not advertise an
		 *	ops vector.
		 *
		 *	The segment's SOP_AGE function is called if the
		 *	segment presents one.
		 *
		 *	Otherwise, the aging is done here.
		 */
		if ((ops = seg->s_ops) == (struct seg_ops *)NULL) {
			continue;
		}
		if (ops->sop_age != NULL) {
			/* seg specific aging function exists */
			SOP_AGE(seg, type);
		} else {
			/* 
			 * Possibly the start of a new range over which
			 * hat_agerange will be called.
			 */
			start = seg->s_base;
			end = seg->s_base + seg->s_size;
			if (seg->s_size == 0)
				continue;
			/*
			 * Extend the range as far as possible. 
			 */
			for (;;) {
				nextseg = seg->s_next;
				ops = nextseg->s_ops;
				if (ops == (struct seg_ops *)NULL) {
					break;
				}
				if (ops->sop_age != NULL) {
					break;
				}
				if (nextseg->s_base != end) {
					break;
				}
				ASSERT(nextseg->s_prev == seg);
				ASSERT(nextseg->s_next->s_prev == nextseg);
				end += nextseg->s_size;
				seg = nextseg;
			}
			/*
			 * We break out of the while loop above, if:
			 * 1. seg->s_next->s_ops is NULL
			 * 2. seg->s_next has a special aging function, or
			 * 3. seg->s_next cannot be concatenated to the range.
			 *
			 * Age the current accumulated range.
			 */
			(void) hat_agerange(as, start, end, type);
		}

	} while ((seg = seg->s_next) != firstseg);

	return;
}

/*
 * boolean_t
 * as_getwchan(vaddr_t addr, uint_t *typep, void **key1, void **key2)
 *	Provide the basis for a user-mode wait channel hash based on
 *	the supplied user address.
 *
 * Calling/Exit State:
 *	The caller holds the AS locked and this function returns the
 *	same way.
 *
 *	On success, B_TRUE is returned and the outargs typep, key1, and
 *	key2 are set as follows:
 *
 *              If (segment type == MAP_PRIVATE) typep only is set to
 *              MAP_PRIVATE indicating it is safe to use addr as a process
 *              private wait channel. The other outargs are unset.
 *
 *              If (segment type == MAP_SHARED), typep is set to
 *              MAP_SHARED indicating that the underlying memory can
 *              potentially be accessed outside of this process (and be
 *              used as a means of inter-process signalling). In this case,
 *		key1 and key2 are set so they can be used as the basis for
 *		a global hash of wait channels (since the underlying memory
 *              may be mapped at different virtual addresses in different
 *              address spaces.) In implementation, key1 is a pointer to
 *		the underlying vnode and key2 is the offset of addr relative
 *		to the vnode. 
 *		
 *	On failure, B_FALSE is returned. This function can fail for any of
 *	the following reasons:
 *
 *		- The address falls outside of the valid user range
 *		- The address supplied is not currently mapped
 *		- The underlying segment is MAP_SHARED but has no `vnode.'
 *
 * Remarks:
 *	This interface is not as general as it might be. Ideally we would
 *	be able to `synthesize' a single unique value as the basis of a global
 *	wait channel hash and pass this back. It should also be possible to
 *	create this value independently of whether or not the underlying 
 *	segment manager is vnode-backed. Logic is only the beginning of wisdom.
 */
boolean_t
as_getwchan(vaddr_t addr, uint_t *typep, void **key1, void **key2)
{
	struct seg *seg;

	if ((seg = as_segat(u.u_procp->p_as, addr)) == NULL)
		return B_FALSE;

	if ((*typep = SOP_GETTYPE(seg, addr)) == MAP_PRIVATE)	
		return B_TRUE;

	if (SOP_GETVP(seg, addr, (vnode_t **)key1) == -1)
		return B_FALSE;

	/*
	 * SOP_GETOFFSET cannot fail if SOP_GETVP succeeds.
	 * Unfortunately, this cannot be ASSERTed since both
	 * 0 and -1 (0xffffffff) are possible (valid) returns.
	 */

	*key2 = (void *)SOP_GETOFFSET(seg, addr);

	return B_TRUE;
}

/*
 * boolean_t
 * as_aio_able(struct as *asp, vaddr_t addr, u_int len)
 * 	Check that all pages in the named range are suitable for an async
 *	I/O buffer.  as->a_seglast is used as a cache to remember the
 * 	last segment hit we had here.  This is a special-purpose entry point
 *	for the aio driver. 
 *
 * Calling/Exit State:
 *	AS is locked on entry and exit to function.
 *	This function is a predicate that returns B_TRUE if all virtual pages
 *	are (1) anonymous or (2) mapped private, readable, and writeable,
 *	B_FALSE otherwise.
 */
boolean_t
as_aio_able(struct as *as, vaddr_t addr, u_int len)
{
	vaddr_t segbase, segend, rangebase, rangend;
	struct seg *seg, *hseg;
	vnode_t	*segvp;
	u_int	prot;
	int	segtype;
	boolean_t res = B_FALSE;

	/*
	 * NOTE:  SOP_GETVP is construed as if the address in the segment is
	 * significant, which is untrue for any known segment driver.
	 */
#	define SEG_ANON(seg, addr, vpp) \
	 (SOP_GETVP(seg, addr, (vpp)) == -1 || (*(vpp))->v_type == VUNNAMED ||\
		(*(vpp))->v_type == VCHR)
#	define SEG_RW(seg, addr, protp) \
		(!SOP_GETPROT(seg, addr, protp) && \
		  (*(protp) & (PROT_READ|PROT_WRITE)) == (PROT_READ|PROT_WRITE))

	seg = as_segat(as, addr);
	if (!seg)
		return B_FALSE;

	/*
	 * 'xxxend' is decremented by 1 to prevent wrapping around to 0.
	 */
	segbase = seg->s_base;
	segend = segbase + seg->s_size - 1;
	rangebase = addr;
	rangend = addr + len - 1;

	/*
	 * Check for wraparound.
	 */
	if (rangend <= rangebase)
		return B_FALSE;

	/*
	 * Search the segment list until we cover the range or fail the test.
	 */
	do {
		if (rangebase < segbase)
			/*
			 * A prefix of the remaining range is not mapped.
			 */
			goto ret;

		segtype = ~MAP_PRIVATE;
		if (rangebase <= segend) {
			/*
			 * The range and segment intersect.
			 */
			if (!SEG_ANON(seg, rangebase, &segvp) &&
			    (segtype = SOP_GETTYPE(seg, rangebase))
			     != MAP_PRIVATE)
				/*
				 * The segment is neither anonymous nor
				 * private.
				 */
				goto ret;

			/*
			 * The segment is anonymous or private.  If private,
			 * check all pages in the segment that are in the range
			 * for read and write permissions.
			 */
			if (segtype == MAP_PRIVATE)
				for ( ;
				     rangebase <= segend &&
				      rangebase <= rangend;
				     rangebase += PAGESIZE)
			     		if (!SEG_RW(seg, rangebase, &prot))
						goto ret;

			if (segend >= rangend) {
				/*
				 * The range is consumed by eligible pages.
				 */
				res = B_TRUE;
				goto ret;
			}

			/*
			 * Consume the part of the range covered by seg.
			 */
			rangebase = segend + 1;
		}

		seg = seg->s_next;
		segbase = seg->s_base;
		segend = segbase + seg->s_size - 1;

	} while (seg != as->a_segs);
ret:
	return res;
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_as(const struct as *as)
 *	Dump information about the address space indicated by the pointer as.
 *
 * Calling/Exit State:
 *	DEBUG-only function to be called from kernel debugger.
 *
 * Remarks:
 *	print_as trusts that as points to a bona-fide AS structure.
 */
void
print_as(const struct as *as)
{
	struct seg *s;

	debug_printf("Dump of AS (0x%lx) and related data structures\n\n",
		     (ulong_t)as); 

	if (as == NULL) {
		debug_printf("AS pointer is NULL?\n\n");
		return; 
	}

	debug_printf("\ta_paglck=%x a_size=%x a_rss=%x\n",
		     as->a_paglck, as->a_size, as->a_rss);
	debug_printf("\ta_lockedrss=%x &a_hat=%x &a_rwslplck=%x\n",
		     as->a_lockedrss, &as->a_hat, &as->a_rwslplck);
	debug_printf("\ta_trimnext=%x &a_wkset=%x\n",
		     as->a_trimnext, &as->a_wkset);
	debug_printf("\ta_isize=%x\n\n", as->a_isize);
	debug_printf("\tSegments attached to AS:\n\n");

	if (as->a_segs == NULL) {
		debug_printf("\t\tNONE\n\n");
		return;
	}

	s = as->a_segs;
	do {
		debug_printf("\t\ts_base=%x s_size=%x s_as=%x\n",
			     s->s_base, s->s_size, s->s_as); 
		debug_printf("\t\ts_next=%x s_prev=%x s_ops=%x s_data=%x\n\n",
			     s->s_next, s->s_prev, s->s_ops, s->s_data);
		if (debug_output_aborted())
			return;
	} while ((s = s->s_next) != as->a_segs);

	debug_printf("\n\n");
}

/*
 * void
 * print_proc_as(const proc_t *procp)
 *	Dump information about the process's address space.
 *
 * Calling/Exit State:
 *	DEBUG-only function to be called from kernel debugger.
 *
 * Remarks:
 *	print_proc_as trusts that procp points to a bona-fide proc structure.
 */
void
print_proc_as(const proc_t *procp)
{
	if (procp->p_as != NULL)
		print_as(procp->p_as);
}

#endif /* DEBUG || DEBUG_TOOLS */


/*
 * void
 * as_age_externally_l(struct proc *procp)
 *	Age the specified process. 
 *
 * Calling/Exit State:
 *	The process p_mutex is held on entry. It will be dropped before
 * 	return. During the aging step, the process will be vm_seize'd. 
 *	The caller has already ensured that the process is not being
 *	swapped or locally aged.
 */
void
as_age_externally_l(struct proc *procp)
{
	size_t nonlocked_rss;
	struct as *asp;
	int newquantum;
	
	asp = procp->p_as;
	ASSERT(asp != NULL);
	nonlocked_rss = asp->a_rss - asp->a_lockedrss;
	ASSERT(nonlocked_rss >= (size_t)0);


	if (nonlocked_rss <= nonlocked_minpg) {
		UNLOCK(&procp->p_mutex, PLBASE);
		return;
	}

	if (vm_seize(procp) != B_FALSE) {

		/*
		 * We could not be racing with a local aging step within
		 * the process (since vm_seize would set the P_SEIZE flag
		 * atomically and hold off local aging), although the
		 * race is harmless.
		 */
		ASSERT(!(procp->p_flag & P_LOCAL_AGE));
		/*
		 * After vm_seize(), we lost the p_mutex cover. If vm_seize()
		 * had to block, it is possible to have had the address space
		 * switched from beneath us (due to process exit or relvm). 
		 * Note that since vm_seize atomically sets the P_SEIZE 
	 	 * flag, we need not be concerned about races with the 
		 * swapper here.
		 */
		if (procp->p_as != asp) {
			vm_unseize(procp);
			return;
		}

		/*
		 * If the process has done a vfork(), then the child is
		 * controlling the address space. Normally, our caller
		 * attempts to screen out this case. However, it does
		 * not have the processes seized. Since we cannot call
		 * as_ageswap() when the child is active on the address
		 * space, we must test again after the process is seized.
		 */
		if (procp->p_flag & P_AS_ONLOAN) {
			vm_unseize(procp);
			return;
		}

		/*
		 * call as_ageswap to do the actual aging.
		 */

		as_ageswap(asp, AS_AGE);

		newquantum = (((int)asp->a_agequantum + 
					asp->a_init_agequantum) / 2);
		RESET_AGING_INFO(asp, newquantum);
		vm_unseize(procp);
		return;
	} 
	/*
	 *+ Unexpected vm_seize failure. This is a system software bug.
	 */
	cmn_err(CE_PANIC,"as_age_externally: failed to seize proc %x\n",procp);
}


/*
 * void
 * as_rdlock_func(struct as *as)
 *	Function form of as_rdlock() for binary compatibility.
 *
 * Calling/Exit State:
 *	AS lock will be read-locked on return.
 */
void
as_rdlock_func(struct as *as)
{
	as_rdlock(as);
}


/*
 * void
 * as_wrlock_func(struct as *as)
 *	Function form of as_wrlock() for binary compatibility.
 *
 * Calling/Exit State:
 *	AS lock will be write-locked on return.
 */
void
as_wrlock_func(struct as *as)
{
	as_wrlock(as);
}


/*
 * void
 * as_unlock_func(struct as *as)
 *	Function form of as_unlock() for binary compatibility.
 *
 * Calling/Exit State:
 *	AS lock will be unlocked on return.
 */
void
as_unlock_func(struct as *as)
{
	as_unlock(as);
}
