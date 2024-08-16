/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Header: $"
#ident	"@(#)kern:mem/seg_kmem.c	1.58"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * VM - kernel segment routines
 */

#include <mem/as.h>
#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/memresv.h>
#include <mem/page.h>
#include <mem/seg.h>
#include <mem/seg_kmem.h>
#include <mem/tuneable.h>
#include <mem/vmparam.h>
#include <mem/zbm.h>
#include <proc/mman.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/bitmasks.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>
#include <util/var.h>

#ifndef NO_RDMA
#include <mem/rdma.h>
#endif

extern void poolrefresh_outofmem(void);

/*
 * STATIC kpg data
 */

STATIC zbm_t kpg_zbm;			/* virtual space allocation info */

/*
 * Declarations: private and SOP_* routines
 */

STATIC faultcode_t segkmem_fault(struct seg *, vaddr_t, u_int,
				 enum fault_type, enum seg_rw);
STATIC void segkmem_badop(void);

STATIC struct seg_ops segkmem_ops = {
	(int(*)())segkmem_badop,		/* unmap */
	(void(*)())segkmem_badop,		/* free */
	segkmem_fault,
	(int(*)())segkmem_badop,		/* setprot */
	(int(*)())segkmem_badop,		/* checkprot */
	(int(*)())segkmem_badop,		/* kluster */
	(int(*)())segkmem_badop,		/* sync */
	(int(*)())segkmem_badop,		/* incore */
	(int(*)())segkmem_badop,		/* lockop */
	(int(*)())segkmem_badop,		/* dup */
	(void(*)())segkmem_badop,		/* childload */
	(int(*)())segkmem_badop,		/* getprot */
	(off_t(*)())segkmem_badop,		/* getoffset */
	(int(*)())segkmem_badop,		/* gettype */
	(int(*)())segkmem_badop,		/* getvp */
	(void(*)())segkmem_badop,		/* age */
	(boolean_t(*)())segkmem_badop,		/* sop_shootdown */
	(int(*)())segkmem_badop			/* sop_memory */	
};

/*
 * Globals:
 */

struct seg *kpgseg;	/* segkmem segment */
int kpg_cellsize;	/* number of pages per cell */

#ifndef NO_RDMA
/*
 * The following table is used to translate the P_DMA flag to the appropriate
 * mresvtyp_t for kpg_alloc. This table is correct for rdma_mode(s) RDMA_SMALL
 * and RDMA_LAREGE. It will be adjusted should the system transition into
 * some other mode.
 */
mresvtyp_t kpg_mtypes[] = {
	M_KERNEL_ALLOC,		/* !P_DMA */
	0,
	M_DMA       		/* P_DMA */
				/* should be M_KERNEL_ALLOC for */
				/* RDMA_DISABLED and RDMA_MICRO */
};
#endif /* NO_RDMA */

/*
 * seg_kmem SOP routines
 */

/*
 * STATIC int
 * segkmem_fault(struct seg *seg, vaddr_t addr, uint_t len,
 *		 fault_type type, enum seg_rw rw)
 *	This routine is a no-op since in-use translations are always
 *	loaded, and not paged.  (The physical pages are locked, as well.)
 *
 * Calling/Exit State:
 *	None.
 */

/* ARGSUSED */
STATIC faultcode_t
segkmem_fault(struct seg *seg, vaddr_t addr, uint_t len,
		enum fault_type type, enum seg_rw rw)
{
	if (type == F_SOFTLOCK || type == F_SOFTUNLOCK) {
		return (0);
	} else {
		return (FC_NOMAP);
	}
}

/*
 * int
 * segkmem_badop(void)
 *	This is a catch-all routine for all illegal operations.
 * Calling/Exit State:
 *	None.
 */

STATIC void
segkmem_badop(void)
{

	/*
	 *+ to shut up Klint.
	 */
	cmn_err(CE_PANIC, "segkmem_badop");
	return;
}

/*
 * void
 * kpg_calloc(void)
 *	calloc the kpg_cell(s) and the bitmap
 *
 * Calling/Exit State:
 *	- called only once while calloc is still alive during
 *	  system initialization
 *	- kpgseg->s_size will have been set to an upper bound on the eventual
 *	  segment size
 */

void
kpg_calloc(void)
{
	zbm_calloc(&kpg_zbm, kpgseg->s_size, kpg_cellsize);
}

/*
 * void
 * segkmem_create(struct seg *seg)
 *	This routine sets up a segkmem segment.
 *
 * Calling/Exit State:
 *	- called only once after calloc is disabled and palloc still
 *	  enabled during initialization
 *	- called without any locks held.
 */

void
segkmem_create(struct seg *seg)
{
	ASSERT((seg->s_base & POFFMASK) == 0);
	ASSERT((seg->s_size & POFFMASK) == 0);

	/* Allocate static page tables if necessary. */
	hat_statpt_alloc(seg->s_base, seg->s_size);

	seg->s_ops = &segkmem_ops;
}

/*
 * void
 * kpg_init(void)
 *	Final initialization for kpgseg.
 *	Initializes locks and kpg structures.
 *
 * Calling/Exit State:
 *	- must be called before kmem_alloc is enabled.
 *	- called without any locks held.
 */

void
kpg_init(void)
{
	zbm_init(&kpg_zbm, kpgseg->s_base, kpgseg->s_size,
		 poolrefresh_outofmem);
}

/*
 * void
 * kpg_vaddr_limits(vaddr_t *basep, ulong_t *sizep)
 *	This routine returns current values of s_base and s_size.
 *	It is currently called by KMA only.
 *
 * Calling/Exit State:
 *	None.
 */

void
kpg_vaddr_limits(vaddr_t *basep, ulong_t *sizep)
{
	*basep = kpgseg->s_base;
	*sizep = (ulong_t)kpgseg->s_size;	/* in bytes */
	return;
}

/*
 * void *
 * kpg_alloc(ulong_t npages, uint_t prot, uint_t flag)
 *	This routine allocates "npages" contiguous pages of kernel heap,
 *	backed by physical memory, and sets up HAT translations.
 *
 * Calling/Exit State:
 *	- called with kpg_lock unlocked
 *	- returns the starting virtual address for the contiguous
 *	  kernel adress range, if succeed.  Otherwise, NULL.
 *	- caller prepares to block if !NOSLEEP.
 */

void *
kpg_alloc(ulong_t npages, uint_t prot, uint_t flag)
{
	vaddr_t addr;
	page_t *pp;
#ifdef NO_RDMA
	const mresvtyp_t mtype = M_KERNEL_ALLOC;
#else /* !NO_RDMA */
	mresvtyp_t mtype = kpg_mtypes[(flag & P_DMA)];
#endif /* NO_RDMA */

	if (flag & NOSLEEP) {
		if (!mem_resv(npages, mtype)) 
			return((void *)NULL);
	} else {
		mem_resv_wait(npages, mtype, B_FALSE);
	}

	/*
	 * Now actually allocate the physical pages we need
	 * using page_get.
	 */

        pp = page_get(ptob(npages), flag);

	if (pp == NULL) {
		ASSERT(flag & NOSLEEP);
		mem_unresv(npages, mtype);
		return((void *)NULL);
	}

	/*
	 * allocate kernel virtual heap space
	 */

	addr = _KPG_VM_ALLOC(npages, flag);

	ASSERT((addr & POFFMASK) == 0);

	if (addr == (vaddr_t)NULL) {
		ASSERT(flag & NOSLEEP);
		mem_unresv(npages, mtype);
		page_list_unlock(pp);
		return((void *)addr);
	}

	ASSERT(pp != NULL);

	hat_statpt_memload(addr, npages, pp, prot);

	return((void *)addr);
}

/*
 * void
 * kpg_free(void *vaddr, ulong_t npages)
 *	This routine frees up all physical and virtual resources
 *	for kernel address range [vaddr, vaddr + ptob(npages)).
 *
 * Calling/Exit State:
 *	- called with kpg_lock unlocked
 *	- called with vaddr page-aligned
 *	- exits with kpg_lock unlocked
 *
 */

void
kpg_free(void *vaddr, ulong_t npages)
{
	ulong_t dummy = npages;
	vaddr_t addr;
	page_t *pp;
#ifdef NO_RDMA
	const mresvtyp_t mtype = M_KERNEL_ALLOC;
#else
	static mresvtyp_t mtype_page[] = {
			M_KERNEL_ALLOC,		/* STD_PAGE */
			M_DMA,			/* DMA_PAGE */
			M_KERNEL_ALLOC		/* PAD_PAGE */
	};
	mresvtyp_t mtype;
#endif

	ASSERT(((vaddr_t)vaddr & POFFMASK) == 0);

	/*
	 * reserve swap and physical pages
	 */
	for (addr = (vaddr_t)vaddr; dummy-- != 0; addr += PAGESIZE) {
		pp = kvtopp(addr);
#ifndef NO_RDMA
		mtype = mtype_page[pp->p_type];
#endif /* NO_RDMA */

		ASSERT(pp != (page_t *)NULL);
		page_unlock(pp);
	}

	_KPG_VM_FREE((vaddr_t)vaddr, npages);

	mem_unresv(npages, mtype);
}

/*
 * void *
 * kpg_pl_mapin(ulong_t npages, page_t *pp, uint_t prot, uint_t sleep_flag)
 *	maps in a list of page(s) and returns base virtual address.
 *
 * Calling/Exit State:
 *	- called with kpg_lock unlocked
 *	- returns the starting virtual address for the contiguous
 *	  kernel adress range, if succeed.  Otherwise, NULL.
 *
 * Description:
 *	This routine allocates "npages" contiguous pages of kernel heap
 *	and set up their hat translations for given pp I/O list.
 */

void *
kpg_pl_mapin(ulong_t npages, page_t *pp, uint_t prot, uint_t sleep_flag)
{
	vaddr_t addr;

	ASSERT(pp != (page_t *)NULL);

	/*
	 * allocate kernel virtual heap space
	 */
	if (((addr = _KPG_VM_ALLOC(npages, sleep_flag)) == (vaddr_t)NULL)
	     && (sleep_flag & KM_NOSLEEP))
		return((void *)NULL);

	ASSERT(addr != (vaddr_t)NULL);
	ASSERT((addr & POFFMASK) == 0);

	hat_statpt_memload(addr, npages, pp, prot);
	return((void *)addr);
}

/*
 * void *
 * kpg_ppid_mapin(ulong_t npages, ppid_t ppid, uint_t prot, uint_t sleep_flag)
 *	This routine allocates "npages" contiguous pages of kernel heap
 *	and set up their hat translations for physical pages list.
 *
 * Calling/Exit State:
 *	- called with kpg_lock unlocked
 *	- returns the starting virtual address for the contiguous
 *	  kernel adress range, if succeed.  Otherwise, NULL.
 */

void *
kpg_ppid_mapin(ulong_t npages, ppid_t ppid, uint_t prot, uint_t sleep_flag)
{
	vaddr_t addr;

	/*
	 * allocate kernel virtual space
	 */

	if (((addr = _KPG_VM_ALLOC(npages, sleep_flag)) == (vaddr_t)NULL) &&
	    (sleep_flag & KM_NOSLEEP))
		return((void *)NULL);

	ASSERT(addr != (vaddr_t)NULL);
	ASSERT((addr & POFFMASK) == 0);

	hat_statpt_devload(addr, npages, ppid, prot);
	return((void *)addr);
}

/*
 * void
 * kpg_mapout(void *vaddr, ulong_t npages)
 *	This routine frees up virtual resources and unmap
 *	for kernel address range [vaddr, vaddr + ptob(npages)).
 *
 * Calling/Exit State:
 *	- called with kpg_lock unlocked
 *	- called with vaddr page-aligned
 *	- exits with kpg_lock unlocked
 *
 */

void
kpg_mapout(void *vaddr, ulong_t npages)
{
	ASSERT(((vaddr_t)vaddr & POFFMASK) == 0);

	_KPG_VM_FREE((vaddr_t)vaddr, npages);
}

/*
 * void
 * segkmem_ppid_mapin(struct seg *seg, vaddr_t vaddr, ulong_t npages,
 *		      ppid_t ppid, uint_t prot)
 *	This routine sets up hat translations for physical pages,
 *	starting at kernel virtual address "vaddr".  This routine does
 *	not allocate kernel virtual addresses and assumes that its caller
 *	has gotten a valid vaddr somehow already (e.g. with kpg_vm_alloc()).
 *
 * Calling/Exit State:
 *	Caller guarantees that the virtual address range is "clean";
 *	i.e. it has not been mapped in since it was allocated, or
 *	segkmem_mapout() was called since the last use.
 */

/* ARGSUSED */
void
segkmem_ppid_mapin(struct seg *seg, vaddr_t vaddr, ulong_t npages, ppid_t ppid,
		   uint_t prot)
{
	ASSERT(vaddr != (vaddr_t)NULL);
	ASSERT((vaddr & POFFMASK) == 0);

	hat_statpt_devload(vaddr, npages, ppid, prot);
	return;
}

/*
 * void
 * segkmem_pl_mapin(struct seg *seg, vaddr_t vaddr, ulong_t npages, page_t *pp,
 *		    uint_t prot)
 *	Maps in a list of page(s) starting at the given kernel virtual address
 *	"vaddr".  This routine differs from kpg_pl_mapin() in not allocating
 *	kernel vaddr and just setting up the mapping.  Its callers should have
 *	gotten vaddr already (e.g. with kpg_vm_alloc()).
 *
 * Calling/Exit State:
 *	Caller guarantees that the virtual address range is "clean";
 *	i.e. it has not been mapped in since it was allocated, or
 *	segkmem_mapout() was called since the last use.
 */

/* ARGSUSED */
void
segkmem_pl_mapin(struct seg *seg, vaddr_t vaddr, ulong_t npages, page_t *pp,
		 uint_t prot)
{
	ASSERT(vaddr != (vaddr_t)NULL);
	ASSERT((vaddr & POFFMASK) == 0);

	hat_statpt_memload(vaddr, npages, pp, prot);
	return;
}

/*
 * void
 * segkmem_mapout(struct seg *seg, vaddr_t vaddr, ulong_t npages)
 *	Unmap a seg_kmem mapping without releasing the virtual.
 *
 * Calling/Exit State:
 *	The address range passed in should previously have been mapped by
 *	segkmem_*_mapin, kpg_*_mapin, or kpg_alloc.  On exit, the translations
 *	will have been unloaded.  This forces a TLB shootdown.
 */
/* ARGSUSED */
void
segkmem_mapout(struct seg *seg, vaddr_t vaddr, ulong_t npages)
{
	ASSERT(vaddr != (vaddr_t)NULL);
	ASSERT((vaddr & POFFMASK) == 0);

	hat_statpt_unload(vaddr, npages);
	hat_shootdown((TLBScookie_t)0, HAT_NOCOOKIE);
	return;
}

/*
 * vaddr_t
 * kpg_vm_alloc(ulong_t npages, uint_t flag)
 * 	This routine allocates a contiguous kernel address range of "size"
 *	bytes, but no physical allocation or mapping is setup yet.
 *	The address returned is a page-aligned kernel vaddr.
 *
 * Calling/Exit State:
 * 	- called with no SPIN locks held and returns that way
 *	- caller is prepared to block if !NOSLEEP
 */

vaddr_t
kpg_vm_alloc(ulong_t npages, uint_t flag)
{
#ifdef DEBUG
	if ((flag & NOSLEEP) && (u.u_debugflags & FAIL_KPG_VM_ALLOC))
		return((vaddr_t)NULL);
#endif

	return (_KPG_VM_ALLOC(npages, flag));
}

/*
 * void
 * kpg_vm_free(vaddr_t vaddr, ulong_t npages)
 * 	This function frees a range of kernel virtual heap space.
 *
 * Calling/Exit State:
 *	None.
 */

void
kpg_vm_free(vaddr_t vaddr, ulong_t npages)
{
	_KPG_VM_FREE(vaddr, npages);
}


#if defined(DEBUG)

/*
 * void
 * print_kpg_zbm(void)
 * 	This is routine prints out the internal ZBM data structures
 *	for kpg.
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger or in-kernel test.
 */

void
print_kpg_zbm(void)
{
	extern void print_zbm(const zbm_t *);

	print_zbm(&kpg_zbm);
}

/*
 * void
 * print_kpg_stats(void)
 *	Statistic audit routine to print out a summary of the ZBM statistics
 *	for the kpg segment.
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger or in-kernel test.
 */

void
print_kpg_stats(void)
{
	extern void print_zbm_stats(const zbm_t *);

	print_zbm_stats(&kpg_zbm);
}

#endif /* DEBUG */

#ifdef DEBUG

/*
 * void
 * kpg_audit(void)
 *	This heavy duty routine should be run in a debug mode in which
 *	performance is of no consequence.
 *
 * Calling/Exit State:
 *	- called with no SPIN locks held and returns that way
 */

void
kpg_audit(void)
{
	extern void zbm_audit(const zbm_t *);

	zbm_audit(&kpg_zbm);
}

#endif /* DEBUG */
