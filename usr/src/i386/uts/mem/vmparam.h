/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_VMPARAM_H	/* wrapper symbol for kernel use */
#define _MEM_VMPARAM_H	/* subject to change without notice */

#ident	"@(#)kern-i386:mem/vmparam.h	1.60"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

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

#ifdef _KERNEL_HEADERS

#include <proc/user.h>		/* PORTABILITY */
#include <svc/cpu.h>		/* PORTABILITY */
#include <util/types.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/param.h>		/* PORTABILITY */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/user.h>		/* PORTABILITY */
#include <sys/types.h>		/* REQUIRED */
#include <sys/param.h>		/* PORTABILITY */

/*
 * cpu.h not installed in usr/include, but is only needed by VM itself,
 * which is always the _KERNEL_HEADERS case.
 */

#ifdef	_KERNEL

#include <sys/ksynch.h>		/* REQUIRED */

#endif

#endif /* _KERNEL_HEADERS */

/*
 * This file contains architecture family specific VM definitions.
 * Platform specific VM definitions are contained in <mem/vm_mdep.h>.
 */


/*
 * Division of virtual addresses between user and kernel.
 */

#define UVBASE	 ((vaddr_t)0x00000000L)	/* base user virtual address */
#define UVEND		KVBASE		/* end of user virtual address range */
#define KVBASE	 ((vaddr_t)0xC0000000L)	/* base of kernel virtual range */
#define KVEND	 ((vaddr_t)0x00000000L) /* end of kernel virtual range */

/*
 * Determine whether vaddr is a kernel virtual address.
 */

#define KADDR(vaddr)		((vaddr_t)(vaddr) >= KVBASE)


#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Functions which return a physical page ID return NOPAGE if
 * there is no valid physical page ID.
 * This is required for the DDI.
 */

#define NOPAGE		((ppid_t)-1)

/*
 * This is to maintain backward compatibility for DDI function hat_getppfnum.
 * PSPACE_MAINSTORE is the code for the physical address space which
 * includes, at least, "mainstore" system memory, which is the memory that
 * programs (and the kernel) execute out of.  See hat_getppfnum(D3K).
 */

#define PSPACE_MAINSTORE	0

/*
 * Paging thresholds (see vm_pageout.c).
 *      lotsfree is 256k bytes, but at most 1/8 of memory
 *      desfree is 100k bytes, but at most 1/16 of memory
 *      minfree is 32k bytes, but at most 1/2 of desfree
 */
#define LOTSFREE	(256 * 1024)
#define LOTSFREEFRACT	8
#define DESFREE		(100 * 1024)
#define DESFREEFRACT	16
#define MINFREE		(32 * 1024)
#define MINFREEFRACT	2

#define	SYSDAT_PAGES	1	/* for now */

/*
 * Fixed kernel virtual addresses.
 *
 * These addresses are all mapped by a level 2 page table which is
 * replicated for each engine to allow some of these mappings to
 * be different on a per-engine basis.  The rest of these mappings
 * are statically defined for the life of the system at boot.
 *
 * Each of these addresses must begin on a page boundary.
 *
 * WARNING: Since USER_DS must include UVUVWIN, any pages below this
 * address will be visible to the user if they have user page permissions.
 */

#define KVPER_ENG_END	((vaddr_t)0xFFFFF000)
	/* high end of per-engine range (inclusive) */

#define KVTMPPG2	KVPER_ENG_END
	/* per-engine temporary page slot for pagezero(), ppcopy() */

#define KVTMPPG1	(KVTMPPG2 - MMU_PAGESIZE)
	/* per-engine temporary page slot for pagezero(), ppcopy() */

#define KVPHYSMAP1	(KVTMPPG1 - MMU_PAGESIZE)
	/* per-engine temporary page slot for physmap1() */

#define KVMET		(KVPHYSMAP1 - (MET_PAGES * PAGESIZE))
	/* global kernel metrics */

#define KVPLOCALMET	(KVMET - (PLMET_PAGES * PAGESIZE))
	/* per-engine struct plocalmet; also mapped at e_local->pp_localmet  */

#define KVSYSDAT	(KVPLOCALMET - (SYSDAT_PAGES * PAGESIZE))
	/* system read-only data: eg; hrt timer */

#define KVPLOCAL	(KVSYSDAT - (PL_PAGES * MMU_PAGESIZE))
	/* per-engine struct plocal; also mapped at e_local->pp_local;
	 * must be beyond USER_DS */

#define KVENG_L2PT	(KVPLOCAL - MMU_PAGESIZE)
	/* per-engine level 2 page table; also mapped at e_local->pp_pmap */

#define KVENG_L1PT	(KVENG_L2PT - MMU_PAGESIZE)
	/* per-engine level 1 page table; also mapped at e_local->pp_kl1pt */

#define FPEMUL_PAGES	16	/* max # pages for FPU emulator */

#define KVFPEMUL	(KVENG_L1PT - (FPEMUL_PAGES * MMU_PAGESIZE))
	/* FPU emulator code/data; must be beyond USER_DS */

#define KVUENG		(KVFPEMUL - (USIZE * PAGESIZE))
	/* per-engine idle ublock; also mapped at e_local->pp_ublock;
	 * must be beyond USER_DS */

#define KVUENG_EXT	(KVUENG - (KSE_PAGES * MMU_PAGESIZE))
	/* stack extension page for per-engine ublock */

#define KVUENG_REDZONE	(KVUENG_EXT - MMU_PAGESIZE)
	/* red zone (unmapped) to catch per-engine kernel stack overflow */

#define KVUVWIN		(KVUENG_REDZONE - MMU_PAGESIZE)
	/* kernel-writeable mapping to UVUVWIN page
	 * also mapped at e_local->pp_uvwin */

#define UVUVWIN		(KVUVWIN - MMU_PAGESIZE)
	/* per-engine page to contain user-visible read-only data;
	 * included in USER_DS so user progs can access directly */

#define KVLAST_ARCH	UVUVWIN
	/* KVLAST_ARCH is the last fixed kernel virtual address (working 
	 * down from high memory) allocated by architecture-specific but
	 * platform-independent code.  This symbol can be used by
	 * platform-specific code to begin allocating additional
	 * fixed kernel virtual addresses.
	 */

	/* start of 4 Meg per-engine area */
#define KVPER_ENG	((vaddr_t)0xFFC00000)

/*
 * If Weitek is present, it gets mapped at the following virtual address.
 * This address is fixed by the binary API.
 */

#define KVWEITEK        ((vaddr_t)0xFFC00000)  /* Weitek reserved space */
#define WEITEK_SIZE     (64*1024)               /*      ... for 64k */


extern	char	kvavbase[];

/*
 * The following defines indicate the limits of allocatable kernel virtual.
 * That is, the range [KVAVBASE,KVAVEND) is available for general use.
 */

#define KVAVBASE	((vaddr_t)kvavbase)
#define KVAVEND		KVPER_ENG


/*
 * Other misc virtual addresses.
 */

#define UVSTACK	 ((vaddr_t)0x80000000L)	/* default user stack location */
#define UVMAX_STKSIZE	0x1000000

/*
 * Determine whether kernel address vaddr is in the per-engine range.
 */

#define KADDR_PER_ENG(vaddr)	((vaddr_t)(vaddr) >= KVPER_ENG)


/*
 * Determine whether [addr, addr+len) are valid user address.
 */

#define VALID_USR_RANGE(addr, len) \
	((vaddr_t)(addr) + (len) > (vaddr_t)(addr) && \
	 (vaddr_t)(addr) >= UVBASE && (vaddr_t)(addr) + (len) <= UVEND)

/*
 * Given an address, addr, which is in or just past a valid user range,
 * return the (first invalid) address just past that user range.
 */
#define VALID_USR_END(addr)	UVEND

#ifdef _KERNEL

/*
 * WRITEABLE_USR_RANGE() checks that an address range is within the
 * valid user address range, and that it is user-writeable.
 * On machines where read/write page permissions are enforced on kernel
 * accesses as well as user accesses, this can be simply defined as
 * VALID_USR_RANGE(addr, len), since the appropriate checks will be done
 * at the time of the actual writes.  Otherwise, this must also call a
 * routine to simulate a user write fault on the address range.
 */

#define WRITEABLE_USR_RANGE(addr, len) \
		(VALID_USR_RANGE(addr, len) && \
		 (n_i386_online != 0 ? \
			begin_user_write((vaddr_t)(addr), len) \
		  : \
			(ATOMIC_INT_INCR(&n_user_write), \
			 (u.u_user_write = B_TRUE))))

/*
 * END_USER_WRITE() is called after writing to user address space
 * to perform any necessary clean-up from the previous WRITEABLE_USR_RANGE().
 */

#define END_USER_WRITE(addr, len) \
		if (u.u_user_write) { \
			u.u_user_write = B_FALSE; \
			ATOMIC_INT_DECR(&n_user_write); \
		} else \
			end_user_write((vaddr_t)(addr), len);

extern boolean_t begin_user_write(vaddr_t, size_t);
extern void end_user_write(vaddr_t, size_t);

extern atomic_int_t n_user_write;

#endif /* _KERNEL */

/*
 * KL2PTES is an array of all the global kernel level 2 ptes (the PTEs
 * for the [KVAVBASE,KVAVEND) range of kernel virtual addresses).  It
 * provides kernel virtual addresses for these PTEs so that they can be
 * read/written easily by the kernel when it is retrieving/changing page
 * mappings.
 * 
 * KL2PTES is implemented by having a level 2 page table which contains
 * ptes that point to all the kernel level 2 page tables (including this
 * one).  Thus all kernel level 2 page tables are referenced by both this
 * level 2 page table and by the level 1 page table.  An exception to this
 * is the single level 2 page table, KVENG_L2PT, which is kept on a
 * per-engine basis.
 * 
 * KL2PTESPTES is an array of the kernel level 2 ptes that map KL2PTES.
 * It provides kernel virtual adddresses for these ptes so that they can
 * be read/written by the kernel when new level 2 page tables are added to
 * the kernel virtual address space (i.e., to keep KL2PTES up to date).
 * 
 * KL2PTESPTES is implemented by just overlaying it with the range of
 * KL2PTES which maps the level 2 page table used to map KL2PTES,
 * saving one physical page since we don't need two page tables.
 * This means that KL2PTESPTES must be on a page boundary somewhere
 * within [KL2PTES,KL2PTES+KL2PTES_SIZE) (and that range must be covered
 * by a single page table).
 *
 * We constrain KL2PTES to begin on a page table boundary in order to
 * simplify some of the calculations below.
 *
 * For simplicity, we just use the first available virtual address for
 * both KL2PTES and KL2PTESPTES.
 */

#define KL2PTES		KVAVBASE	/* array of kernel level 2 PTEs */
#define KL2PTES_SIZE	(mmu_btop(KVAVEND - KVAVBASE) * \
			  sizeof(pte_t))  /* num. (virtual) bytes in KL2PTES */
#define KL2PTESPTES	KL2PTES		/* array of ptes that map KL2PTES */


/* Index into KL2PTESPTES[] for a kernel virtual address */

#define kl2ptesndx(va)	ptnum((vaddr_t)(va) - KVAVBASE)


/*
 * pte_t *
 * kvtol1ptep(vaddr_t addr)
 *	Return pointer to level 1 pte for the given kernel virtual address.
 */

#define kvtol1ptep(addr) \
		(ASSERT(KADDR(addr)), \
		&((pte_t *)KVENG_L1PT)[ptnum(addr)])

/*
 * pte_t *
 * kvtol2ptep(vaddr_t addr)
 *	Return pointer to level 2 pte for the given kernel virtual address.
 *
 * Description:
 *	If addr is in the range mapped by the per-engine level 2 page table,
 *	KVENG_L2PT, return a pointer into it.  Otherwise, return a pointer
 *	into the global set of level 2 pages.
 */

#define kvtol2ptep(addr) \
		(ASSERT(KADDR(addr)), \
		(KADDR_PER_ENG(addr)) ? \
			&((pte_t *)KVENG_L2PT)[pgndx(addr)] : \
			&((pte_t *)KL2PTES)[pfnum((vaddr_t)(addr) - KVAVBASE)])

/*
 * pte_t *
 * kvtol2pteptep(vaddr_t addr)
 *	Return pointer to level 2 pte for the level 2 page table which
 *	maps the given kernel virtual address.
 *
 * Description:
 *	Since this function is used only to add kernel page tables (during
 *	system initialization), and the per-engine page table(s) will already
 *	have been established by this point, it doesn't have to handle
 *	addresses in the per-engine range(s).  If it did, we'd need something
 *	like KL2PTESPTES for the per-engine page tables.
 */

#define kvtol2pteptep(addr) \
		(ASSERT(KADDR(addr)), \
		 ASSERT(!KADDR_PER_ENG(addr)), \
		&((pte_t *)KL2PTESPTES)[kl2ptesndx(addr)])

#define pteptokv(ptep) \
      (KVAVBASE + ((ulong_t)((ptep) - (pte_t *)KL2PTES) << MMU_PAGESHIFT))


/*
 * paddr_t
 * kvtophys(vaddr_t addr)
 *	Return the physical address equivalent of given kernel
 *	virtual address.
 *
 * Calling/Exit State:
 *	The caller must ensure that the given kernel virtual address
 *	is currently mapped.
 */

#define _KVTOPHYS(addr) \
		(ASSERT(PG_ISVALID(kvtol1ptep(addr))), \
		(kvtol1ptep(addr)->pgm.pg_ps ? \
		(paddr_t) ((kvtol1ptep(addr)->pg_pte & MMU_PAGEMASK) + \
			((vaddr_t)(addr) & PAGE4OFFSET)) : \
		 (ASSERT(PG_ISVALID(kvtol2ptep(addr))), \
		(paddr_t) ((kvtol2ptep(addr)->pg_pte & MMU_PAGEMASK) + \
			   ((vaddr_t)(addr) & MMU_PAGEOFFSET)))))
#define kvtophys(addr)	_KVTOPHYS(addr)

/*
 * struct page *
 * kvtopp(vaddr_t addr)
 *	Return the page struct for the physical page corresponding to
 *	a given kernel virtual address.
 *
 * Calling/Exit State:
 *	The caller must ensure that the given kernel virtual address
 *	is currently mapped to a page with a page struct.
 */

#define kvtopp(addr)	 (ASSERT(!PG_ISPSE(kvtol1ptep(addr))), \
				pteptopp(kvtol2ptep(addr)))


/*
 * vaddr_t
 * STK_LOWADDR(proc_t *p)
 *	Return the lowest address which is part of the autogrow stack.
 *
 * Calling/Exit State:
 *	Caller must insure that p is stabilized and cannot be deallocated
 *	out from underneath us. This is usually guaranteed by the fact that
 *	this p is that of the caller. Of the fields referenced, p_stkbase
 *	never changes and p_stksize is stabilized by the AS lock for p->p_as,
 *	which the caller must hold.
 */

#define STK_LOWADDR(p)	((p)->p_stkbase - (p)->p_stksize)

/*
 * vaddr_t
 * STK_HIGHADDR(proc_t *p)
 *	Return the highest address which is part of the autogrow stack.
 *
 * Calling/Exit State:
 *	Caller must insure that p is stabilized and cannot be deallocated
 *	out from underneath us. This is usually guaranteed by the fact that
 *	this p is that of the caller. Although this implemenation doesn't
 *	require it, the caller must hold the AS lock for p->p_as, since
 *	for some implementations this may be the grow end of the stack.
 */

#define STK_HIGHADDR(p)	((p)->p_stkbase)

#define HI_TO_LOW	0
#define LO_TO_HI	1

#define STK_GROWTH_DIR	HI_TO_LOW

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_VMPARAM_H */
