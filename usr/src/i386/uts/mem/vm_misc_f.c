/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

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

#ident	"@(#)kern-i386:mem/vm_misc_f.c	1.32"
#ident	"$Header: $"

/*
 * UNIX machine dependent virtual memory support.
 */

#include <mem/as.h>
#include <mem/hat.h>
#include <mem/immu.h>
#include <mem/lock.h>
#include <mem/mem_hier.h>
#include <mem/page.h>
#include <mem/seg.h>
#include <mem/vmparam.h>
#include <proc/disp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/cpu.h>
#include <svc/systm.h>
#include <svc/trap.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#define SEG_OVERLAP(seg, saddr, eaddr)	\
	((seg)->s_base < (eaddr) && ((seg)->s_base + (seg)->s_size > (saddr)))

/*
 * int
 * valid_va_range(vaddr_t *basep, u_int *lenp, u_int minlen, int dir)
 * 	Determine whether [base, base+len] contains a mapable range of
 * 	addresses at least minlen long. 
 *
 * Calling/Exit State:
 * 	No special requiments for MP.
 *
 *	If the specifed range is mapable, non-zero is returned; otherwise
 *	0 is returned.
 *
 * Remarks:
 *	On some architectures base and len may be adjusted if
 * 	required to provide a mapable range. That is why they are
 *	passed as pointers. The dir argument is used to specify the 
 *	relationship of base and len (i.e. whether len is added to or 
 *	subtracted from base).
 */
/* ARGSUSED */
int
valid_va_range(vaddr_t *basep, u_int *lenp, u_int minlen, int dir)
{
	vaddr_t hi, lo;

	lo = *basep;
	hi = lo + *lenp;
	if (hi < lo ) 		/* overflow */
		return(0);
	if (hi - lo < minlen)
		return (0);
	return (1);
}

/*
 * void
 * map_addr(vaddr_t, uint_t, off_t, int)
 * 	map_addr() is called when the system is to choose a
 * 	virtual address for the user.  We pick an address
 * 	range which is just below UVEND.
 *
 * Calling/Exit State:
 *	- the target address space should be writer locked by the caller.
 *	- this function returns with the lock state unchanged.
 *
 * Description:
 * 	addrp is a value/result parameter. On input it is a hint from the user
 * 	to be used in a completely machine dependent fashion. We decide to
 * 	completely ignore this hint.  On output it is NULL if no address can
 *	be found in the current processes address space or else an address
 *	that is currently not mapped for len bytes with a page of red zone on
 *	either side.
 * 	If align is true, then the selected address will obey the alignment
 *	constraints of a vac machine based on the given off value. On the
 *	machines without a virtual address cache, this arg is ignored.
 */
/*ARGSUSED*/
void
map_addr(vaddr_t *addrp, uint_t len, off_t off, int align)
{
	proc_t *p = u.u_procp;
	struct as *as = p->p_as;
	vaddr_t  base;
	uint_t   slen;

	len = (len + PAGEOFFSET) & PAGEMASK;

	/*
	 * Redzone for each side of the request. This is done to leave
	 * one page unmapped between segments. This is not required, but
	 * it's useful for the user because if their program strays across
	 * a segment boundary, it will catch a fault immediately making
	 * debugging a little easier.
	 */
	len += 2 * PAGESIZE;

	/*
	 * Look for a large enough hole in the address space to allocate
	 * dynamic memory.  First, look in the gap between stack and text,
	 * if any. If we find a hole there, use the lower portion. 
	 * If there is no gap, or there is not a large enough hole there,
	 * look above the break base.
	 */
	if (p->p_stkgapsize >= len) {
		base = p->p_stkbase;
		slen = p->p_stkgapsize;
		if (as_gap(as, len, &base, &slen, AH_LO, (vaddr_t)NULL) == 0) {
			*addrp = base + PAGESIZE;
			return;
		}
	}

	base = p->p_brkbase;
	slen = UVEND - base;

	if (as_gap(as, len, &base, &slen, AH_HI, (vaddr_t)NULL) == 0) {
		/*
		 * as_gap() returns the 'base' address of a hole of
		 * 'slen' bytes.  We want to use the top 'len' bytes
		 * of this hole, and set '*addrp' accordingly.
		 * The addition of PAGESIZE is to allow for the redzone
		 * page on the low end.
		 */
		ASSERT(slen >= len);
		*addrp = base + (slen - len) + PAGESIZE;
	} else
		*addrp = NULL;  /* no more virtual space available */
}

/*
 * vaddr_t
 * execstk_addr(size_t size, uint_t *hatflagp)	
 *	Find a hole size bytes in length in the current address space,
 *	map it in, and return its address to the caller. Attempt to
 *	meet a number of machine-dependent alignment criteria to allow
 *	hat_exec to optimize the final stack relocation. Indicate the
 * 	success of this attempt by setting the outarg hatflagp.
 *
 * Calling/Exit State:
 *	Called as part of the exec sequence after all but one LWP has been
 *	destroyed making this a single-threaded process. The caller passes
 *	in the size (in bytes) needed to contain the new stack and a pointer 
 *	to the hatflagp outarg.
 *
 *	On success, the location of the newly mapped portion of the address 
 *	space is returned. The outarg hatflagp is used to indicate to the 
 *	caller that various machine-specific aligment criteria were met when 
 *	placing the stack build area in the old address space such that the
 *	stack may be relocated in the new address space in the most efficient
 *	manner possible. Generally this involves placement on page-table
 *	boundaries which avoids the necessity of copying individual PTEs.
 *	See hat_exec for more details.
 *
 *	On failure, a NULL pointer is returned. This will only happen if the
 *	current address space so completely mapped that a sufficiently 
 *	large hole cannot be mapped.
 *
 * Remarks:
 *	The current error return semantics disallow the possibility of
 *	mapping the stack build area starting at virtual zero. This is
 *	only a problem on machines with stacks that grow from low to high  
 *	addresses.
 *
 *	The hole must be large enough to accomodate the request size plus
 *	two additional pages worth of "guard-band" which remain unmapped.
 *
 */	
vaddr_t
execstk_addr(size_t size, uint_t *hatflagp)	
{
	struct seg *sseg, *seg;
	vaddr_t	redzone_addr, redzone_endaddr, addr, base;
	vaddr_t pt_round_down, pt_round_up;
	int err;
	boolean_t no_overlap = B_TRUE;
 	u_int len;

	/*
	 * We are looking for a hole of 'size' bytes with a free page
	 * both before and aft (so segment concatenation does not occur).
	 * We first look for a slot with its end aligned properly for the
	 * user stack and falling inside of page table(s) which aren't used
	 * for anything else, so that later, we can use the page table
	 * directly at the new location, instead of copying ptes.
	 * If we can't find one, then we drop the alignment and empty
	 * page table constraints. 
	 *
	 * Since our stack grows downwards, we must be careful not to use
	 * slots where spilling of the aft end (into the free page) doesn't
	 * look like stack growth.  This allows the copyarglist code to
	 * skip string length checks.
	 */
	ASSERT(UVBASE == 0);

	*hatflagp = 1;

	addr = (u.u_stkbase - size) & (VPTSIZE - 1);

	if ((sseg = seg = u.u_procp->p_as->a_segs) == NULL)
		return (vaddr_t)addr;

	if (addr == 0)
		addr = VPTSIZE;

	/* start addr offest by 1 guard page */
	redzone_addr = addr - PAGESIZE;
	redzone_endaddr = addr + size + PAGESIZE;

	while (redzone_endaddr < UVEND) {
		pt_round_down = btoptbl(redzone_addr) * VPTSIZE;
		pt_round_up = btoptblr(redzone_endaddr) * VPTSIZE;
		do {
			/* 
			 * Check if a segment intersects with this
			 * page table.
			 */
			if (SEG_OVERLAP(seg, pt_round_down, pt_round_up)) {
				no_overlap = B_FALSE;
				break;
			}
		} while (seg = seg->s_next, seg != sseg);

		if (no_overlap)
			return (vaddr_t)addr;

		/* this page table is not empty */
		addr += VPTSIZE;
		redzone_addr += VPTSIZE;
		redzone_endaddr += VPTSIZE;
		no_overlap = B_TRUE;
		seg = sseg;
	}	/* while loop */
	/*
	 * At this point we have come to a stage, where we could not
	 * find a free page table in the address space.
	 */
	*hatflagp = 0;

	base = PAGESIZE;
	len = UVEND - base;

	err = as_gap(u.u_procp->p_as, size + 2 * PAGESIZE, &base, &len, AH_HI,
		(vaddr_t)NULL);
	if (!err)
		return (vaddr_t)base + PAGESIZE;

	return (vaddr_t)0;
}


/*
 * # in-progress user-writes which are *not* using {begin,end}_user_write.
 * This is used to hold off any i386 CPUs from coming online.
 */
atomic_int_t n_user_write;
volatile int n_i386_online;	/* # of online CPUs which are CPU_386 */

/*
 * boolean_t
 * begin_user_write(vaddr_t addr, size_t len)
 *	Set up appropriate locking for a write to user address space.
 *
 * Calling/Exit State:
 *	The AS is unlocked on entry, and remains unlocked on return.
 *	Returns B_TRUE if successful.
 *
 * Description:
 *	The Intel386(tm) does not check for page write permissions when writing
 *	from the kernel. However, the correct execution of some operations 
 *	depends upon the invocation of fault handling when a write access
 *	occurs to a write-protected address; for example, the copy-on-write
 *	semantics of memory sharing between address spaces, and backing store
 *	allocation (or reservation) at the time of the first modifying access 
 *	to an address or range of addresses relies upon the generation of
 *	a protection violation fault.
 *
 *	To get this right, we will ensure that:
 *
 *	(1) all valid pages over the range to which the write
 *	    is going to be performed are writeable, and 
 *	(2) the address space will not load any new read-only mappings 
 *	    EXCEPT for those needed by the current LWP, which may need the
 *	    pages on the source side of the write to be brought into memory,
 *	    and may have no choice about what their protections are.
 */

boolean_t
begin_user_write(vaddr_t addr, size_t len)
{
	pte_t *ptep;
	vaddr_t vaddr;
	vaddr_t	eaddr;
	struct as *as;
	hat_t *hatp;
	hatpt_t *dummy_ptap;
	extern pte_t *hat_vtopte_l();
	extern boolean_t upageflt_cmn();

	ASSERT(l.cpu_id == CPU_386);

	as = u.u_procp->p_as;
	hatp = &as->a_hat;
	ASSERT(CURRENT_LWP() != NULL);
	ASSERT(KS_HOLD0LOCKS());
	if (!SINGLE_THREADED()) {
		as_rdlock(as);
		u.u_aslock_stat = AS_READ_LOCKED;
	} else
		u.u_aslock_stat = NOT_AS_LOCKED;
	eaddr = addr + len;
tryagain:
	vaddr = addr & PAGEMASK;
	HATRL_LOCK_SVPL(hatp);
	HAT_RDONLY_LOCK(hatp);	
	/*
	 * Check whether pages are present (mapped), valid, and writeable.
	 * For each page that is valid but which is mapped in unwriteable,
	 * fault it in with as_fault. Pages not valid should be brought in
	 * via the upageflt_cmn code, so that auto-growth of stacks can be
	 * supported. 
	 */

	for(;;) {
		ASSERT(eaddr >= vaddr);
		/*
		 * PERF: Possible to avoid the overhead of calling
		 * 	 hat_vtopte_l for each address? Can we get at
		 * 	 successive pte's in a simple manner?
		 */
		ptep = hat_vtopte_l(hatp, vaddr, &dummy_ptap);
		if (ptep != NULL) {
			if (PG_ISVALID_WRITEABLE(ptep)) {
				vaddr += PAGESIZE;
				if (eaddr > vaddr)
					continue;
				HATRL_UNLOCK_SVDPL(hatp);
#ifdef BUG386B1
				/*
				 * Part of workaround for
				 * Intel386(tm) B1 stepping errata #9.
				 */
				u.u_386userwrite = B_TRUE;
#endif
				return B_TRUE;
			} 
			if (PG_ISVALID(ptep)) {
				HATRL_UNLOCK_SVDPL(hatp);
				if (as_fault(as, vaddr, PAGESIZE, F_PROT,
					     S_WRITE) != 0) {
					/* 
					 * Release the pseudo lock that 
					 * protected against concurrent 
					 * loading of READONLY translations.
					 */
					HAT_RDONLY_UNLOCK(hatp);
					if (u.u_aslock_stat != NOT_AS_LOCKED)
						as_unlock(as);
					u.u_aslock_stat = NOT_AS_LOCKED;
					ASSERT(KS_HOLD0LOCKS());
					return B_FALSE;
				} 
				/*
				 * PERF:
				 * We could potentially hold up other LWPs
				 * from this address space for considerable
				 * time. To not serialize LWPs that all need
				 * to acquire the "HAT_RDONLY" pseudo 
				 * lock, we could use a regular reader-writer
				 * sleep lock, which can be acquired in the
				 * reader mode by any LWP that needs to block
				 * the creation of read-only translations, and
				 * in the writer mode by LWPs that need to
				 * load read-only translations.
				 */
				vaddr += PAGESIZE;
				if (eaddr > vaddr) {
					HATRL_LOCK_SVPL(hatp);
					continue;
				}
				ASSERT(KS_HOLD0LOCKS());
#ifdef BUG386B1
				/*
				 * Part of workaround for
				 * Intel386(tm) B1 stepping errata #9.
				 */
				u.u_386userwrite = B_TRUE;
#endif
				return B_TRUE;
			}
		}
		/*
		 * Page not present or range not mapped in. Call upageflt_cmn
		 * to bring the page in. upageflt_cmn() will neither try to
		 * acquire the address space lock nor drop it, since it is held
		 * already by this context and indicated by u.u_aslock_stat.
		 */
		HATRL_UNLOCK_SVDPL(hatp);
		if (upageflt_cmn(vaddr, PF_ERR_PAGE|PF_ERR_WRITE, 
				NULL) == B_FALSE){
			/* 
			 * Release the pseudo lock that 
			 * protected against concurrent 
			 * loading of READONLY translations.
			 */
			HAT_RDONLY_UNLOCK(hatp);
			ASSERT(KS_HOLD0LOCKS());
			if (u.u_aslock_stat == AS_READ_LOCKED) {
				as_unlock(as);
				u.u_aslock_stat = NOT_AS_LOCKED;
			}
			if (u.u_aslock_stat == NOT_AS_LOCKED) {
				as_wrlock(as);
				u.u_aslock_stat = AS_WRITE_LOCKED;
				goto tryagain;
			}
			ASSERT(u.u_aslock_stat == AS_WRITE_LOCKED);
			as_unlock(as);
			u.u_aslock_stat = NOT_AS_LOCKED;
			return B_FALSE;
		} 
		/*
		 * PERF:
		 * We could potentially hold up other LWPs
		 * from this address space for considerable
		 * time. To not serialize LWPs that all need
		 * to acquire the "HAT_RDONLY" pseudo 
		 * lock, we could use a regular reader-writer
		 * sleep lock, which can be acquired in the
		 * reader mode by any LWP that needs to block
		 * the creation of read-only translations, and
		 * in the writer mode by LWPs that need to
		 * load read-only translations. 
		 */
		vaddr += PAGESIZE;
		if (eaddr > vaddr) {
			HATRL_LOCK_SVPL(hatp);
			continue;
		}
		/*
		 * PERF: 
		 * We could have downgraded the AS lock, if it were 
		 * held in write mode, to read mode. However it should
		 * be too infrequent a case, so we don't bother.
		 */
		ASSERT(KS_HOLD0LOCKS());
#ifdef BUG386B1
		/*
		 * Part of workaround for
		 * Intel386(tm) B1 stepping errata #9.
		 */
		u.u_386userwrite = B_TRUE;
#endif
		return B_TRUE;
	}
}


/*
 * void
 * end_user_write(vaddr_t addr, size_t len)
 *	Dismantle any locking set up for a write to user address space.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit. The function does not block.
 *	It acquires and releases the hat_resourcelock.
 */
/*ARGSUSED*/
void
end_user_write(vaddr_t addr, size_t len)
{
	struct as *as = u.u_procp->p_as;

	ASSERT(CURRENT_LWP() != NULL);	
	ASSERT(SINGLE_THREADED() || u.u_aslock_stat != NOT_AS_LOCKED);
	if (u.u_aslock_stat != NOT_AS_LOCKED) {
		as_unlock(as);
		u.u_aslock_stat = NOT_AS_LOCKED;
	}
	HAT_RDONLY_UNLOCK((&as->a_hat));
	ASSERT(KS_HOLD0LOCKS());

#ifdef BUG386B1
	/*
	 * Part of workaround for
	 * Intel386(tm) B1 stepping errata #9.
	 */
	u.u_386userwrite = B_FALSE;
#endif
}

/*
 * void
 * modify_code(vaddr_t oldfunc, vaddr_t newfunc)
 *	This function modifies kernel code on the fly. "oldfunc" will be
 * 	replaced by a jump to "newfunc".
 *
 * Calling/Exit State:
 *	At this point, we better make sure that "oldfunc" is not being
 *	executed!
 *
 * Description:
 * 	The code is modified by using the indirect jump instruction
 *	using the eax register to the new function. This jump instruction
 * 	is written over the existing text segment for "oldfunc".
 */
void
modify_code(vaddr_t oldfunc, vaddr_t newfunc)
{
	char instr[7];
	pte_t *ptep;
	int i, num_pages;
	int num_bytes_chg = 7; /* number of bytes overwriting oldfunc */

	/*
	 * opcode for "movl immediate value to eax"
	 */
	instr[0] = 0xb8;

	*(ulong_t *)&instr[1] = newfunc;
	/*
	 * opcode for indirect jmp via eax
	 */
	instr[5] = 0xff;
	instr[6] = 0xe0;

	num_pages = 1;
	ptep = kvtol2ptep(oldfunc);
	if (PAGNUM(oldfunc + num_bytes_chg) != PAGNUM(oldfunc))
		num_pages++;
	for (i = 0; i < num_pages; i++)
		PG_SETPROT(ptep + i, PG_RW);
	bcopy((void *)instr, (void *)oldfunc, num_bytes_chg);
	for (i = 0; i < num_pages; i++)
		PG_CLRPROT(ptep + i);
}
