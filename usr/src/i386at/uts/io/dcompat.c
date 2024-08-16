/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/dcompat.c	1.21"
#ident	"$Header: $"

/*
 * Non-DDI driver compatibility.
 *
 * These routines provide support for binary drivers from previous releases
 * which were almost, but not quite, DDI/DKI conforming.  Such drivers were
 * marked as DDI conforming (or, rather, as not D_OLD, which isn't exactly
 * the same thing), but used some interfaces which were not actually part
 * of the DDI/DKI spec.
 *
 * We do not attempt to provide support for arbitrary non-conforming drivers,
 * as it would be impossible to determine which interfaces were necessary.
 * In particular, we do not provide support for access to kernel global
 * variables, such as "u".
 *
 * To prevent these old interfaces from accidentally being used in new code,
 * we prefix their names with "_Compat_".  At kernel build time, the kernel
 * configuration tool will detect old drivers using these interfaces and map
 * the original names to the "_Compat_" names.  For example, references to
 * "sptalloc" will be changed to references to "_Compat_sptalloc".
 *
 * Old interfaces which are just aliases for current routines are directly
 * mapped to those routines, rather than creating "_Compat_" routines here.
 */

#include <fs/buf.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <mem/as.h>
#include <mem/hat.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/seg_kmem.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

#ifndef NO_RDMA
#include <mem/rdma.h>
#endif

/*
 * Support for non-DDI variable references:
 */
int _Compat_Hz = HZ;
daddr_t _Compat_nswap;
dev_t _Compat_swapdev = NODEV;
boolean_t _Compat_merge386enable = B_TRUE;

/*
 * Support for old drivers including obsolete header files:
 */
int _Compat_Header_OK;


/*
 * char *
 * _Compat_kseg(int npgs)
 *	Allocate kernel memory which can be freed without passing in the size.
 *	The memory is returned zeroed.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Description:
 *	This implementation is optimized for the case where this routine is
 *	not used, at the expense of some extra data space cost when it is
 *	used (which we expect to be infrequently or never).
 *
 *	Instead of keeping track of the allocations in a list, which would
 *	take more code space and require a list header word and a lock,
 *	we just allocate an extra page for each request and store the size
 *	in the first page, returning the address of the second page to the
 *	caller.
 */
char *
_Compat_kseg(int npgs)
{
	void *kbuf;
	char *cbuf;

	if (npgs <= 0)
		return NULL;
	kbuf = kpg_alloc((ulong_t)(npgs + 1), PROT_ALL & ~PROT_USER,
			 SLEEP | P_DMA);
	*(ulong_t *)kbuf = npgs + 1;
	cbuf = (char *)kbuf + PAGESIZE;
	bzero(cbuf, ptob(npgs));
	return cbuf;
}

/*
 * void
 * _Compat_unkseg(char *kp)
 *	Free memory allocated by _Compat_kseg().
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
void
_Compat_unkseg(char *kp)
{
	ASSERT(kp != NULL);
	ASSERT((kp - PAGESIZE) != NULL);

	kpg_free(kp - PAGESIZE, *(ulong_t *)(void *)(kp - PAGESIZE));
}


/*
 * caddr_t
 * _Compat_sptalloc(int npgs, int mode, paddr_t base, int flag)
 *	Allocate a virtual mapping for a physical address range.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Remarks:
 *	The original sptalloc performed memory allocation as well, if
 *	the base argument was 0; we don't support that here.
 */
/* ARGSUSED */
caddr_t
_Compat_sptalloc(int npgs, int mode, paddr_t base, int flag)
{
	if (base == 0)
		return NULL;
	return kpg_ppid_mapin(npgs, phystoppid(base), PROT_ALL & ~PROT_USER,
			      flag);
}

/*
 * void
 * _Compat_sptfree(caddr_t vaddr, int npgs, int flag)
 *	Free a mapping allocated by _Compat_sptalloc().
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Remarks:
 *	The original sptfree used the flag argument to control whether
 *	or not the pages should be unlocked (and thus potentially freed).
 *	This is no longer relevant; just ignore the flag.
 */
/* ARGSUSED */
void
_Compat_sptfree(caddr_t vaddr, int npgs, int flag)
{
	kpg_mapout(vaddr, npgs);
}


/*
 * caddr_t
 * _Compat_getcpages(int npgs, boolean_t nosleep)
 *	Allocate contiguous physical memory.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Remarks:
 *	Unlike the original getcpages, this implementation doesn't print
 *	any messages.
 *
 *	Also, the original had a bizarre behavior in that it could sleep
 *	even if nosleep was set, if npgs was exactly 1.  This behavior is not
 *	reproduced here; if nosleep is set, this routine never sleeps.
 *
 *	Even though the original did not zero the memory (except in DEBUG
 *	SVR4 kernels), some drivers have been found to depend on zeroed
 *	memory being returned.  How did they work?  One can only guess.
 *	So, this version zeroes the memory returned, just in case.
 *
 * Description:
 *	Since freepage, the free side of this interface, takes a physical
 *	page frame number instead of a virtual address, we need to cache
 *	the virtual address for each page in its page-private data, so
 *	freepage can find it.
 */
caddr_t
_Compat_getcpages(int npgs, boolean_t nosleep)
{
	caddr_t	kbuf;
	vaddr_t memp;
	page_t *pp;
	mresvtyp_t mtype;
	extern mresvtyp_t kpg_mtypes[];

	if (npgs <= 0)
		return NULL;
	mtype = kpg_mtypes[P_DMA];

	if (!mem_resv(npgs, mtype)) {
		if (nosleep) 
			return NULL;
		mem_resv_wait(npgs, mtype, B_FALSE);
	}

	memp = kpg_vm_alloc(npgs, nosleep);
	if (memp == NULL) {
		mem_unresv(npgs, mtype);
		return NULL;
	}

	pp = page_get_aligned(ptob(npgs), PAGESIZE, 0, 0, P_DMA | nosleep);
	if (pp == NULL) {
		ASSERT(nosleep);
		mem_unresv(npgs, mtype);
		kpg_vm_free(memp, npgs);
		return NULL;
	}

	segkmem_pl_mapin(kpgseg, memp, npgs, pp, PROT_READ | PROT_WRITE);

	kbuf = (caddr_t)memp;
	bzero(kbuf, ptob(npgs));

	while (npgs-- != 0) {
		pp = kvtopp(kbuf);
		pp->p_pgprv_data[0] = (ulong_t)kbuf;
		kbuf += PAGESIZE;
	}

	return (caddr_t)memp;
}

/*
 * void
 * _Compat_freepage(uint_t pfn)
 *	Free a page from space allocated by _Compat_getcpages().
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
void
_Compat_freepage(uint_t pfn)
{
	page_t *pp;

	pp = page_numtopp(pfn);
	ASSERT(pp >= pages && pp < epages);

	/* The virtual address was cached in the page-private data. */
	ASSERT(pp->p_pgprv_data[0] != 0);
	kpg_free((void *)pp->p_pgprv_data[0], 1);
}


/*
 * vaddr_t
 * _Compat_phystokv(paddr_t paddr)
 *	Return a virtual for a given physical without blocking.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Remarks:
 *	The original phystokv worked for the 0-1M range plus any other
 *	mainstore memory.  Since we only expect drivers to use this to
 *	access device memory, not mainstore memory, we just allow it for
 *	the 0-1M range.
 */
vaddr_t
_Compat_phystokv(paddr_t paddr)
{
	extern vaddr_t physkv_extent;

	if (paddr >= physkv_extent) {
		/*
		 *+ A non DDI-conforming driver, using the phystokv
		 *+ compatibility interface, attempted to access memory
		 *+ above 1MB.  This is no longer supported.  The driver
		 *+ must be reconfigured or removed.
		 */
		cmn_err(CE_PANIC, "phystokv out of range");
		/* NOTREACHED */
	}

	return (KVPHYSTOKV + paddr);
}


/*
 * void
 * _Compat_pio_breakup(void (*strat)(buf_t *), buf_t *bp, int max_xfer)
 *	Call strategy routine with virtually-mapped buffer data and with
 *	pieces no larger than max_xfer (in bytes).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
/* ARGSUSED */
void
_Compat_pio_breakup(void (*strat)(buf_t *), buf_t *bp, int max_xfer)
{
	static physreq_t pio_preq = {
		/* phys_align */	1,
		/* phys_boundary */	0,
		/* phys_dmasize */	0,
		/* phys_max_scgth */	0
	};
	static bcb_t pio_bcb = {
		/* bcb_addrtypes */	BA_KVIRT,
		/* bcb_flags */		0,
		/* bcb_max_xfer */	0,
		/* bcb_granularity */	NBPSCTR,
		/* bcb_physreqp */	&pio_preq
	};
	pio_bcb.bcb_max_xfer = (uint_t)max_xfer;
	(void)physreq_prep(&pio_preq, KM_SLEEP);
	buf_breakup(strat, bp, &pio_bcb);
}


/*
 * void
 * _Compat_tenmicrosec(void)
 *	Busy-wait for ten microseconds.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
void
_Compat_tenmicrosec(void)
{
	drv_usecwait(10);
}

/*
 * void
 * _Compat_spinwait(int millisec)
 *	Busy-wait for a number of milliseconds.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
void
_Compat_spinwait(int millisec)
{
	drv_usecwait((ulong_t)millisec * 1000);
}


/*
 * int
 * _Compat_spl0s(void)
 *	Set interrupt priority level to PL0 (soft).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
_Compat_spl0s(void)
{

	return ((int)spl0());
}

/*
 * int
 * _Compat_spl1s(void)
 *	Set interrupt priority level to PL1 (soft).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
_Compat_spl1s(void)
{

	return ((int)spl1());
}

/*
 * int
 * _Compat_spl2(void)
 *	Set interrupt priority level to PL2 (hard).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
_Compat_spl2(void)
{
	pl_t pl = getpl();
	splx_h(PL2);
	return pl;
}

/*
 * int
 * _Compat_spl2s(void)
 *	Set interrupt priority level to PL2 (soft).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
_Compat_spl2s(void)
{
	pl_t pl = getpl();
	splx(PL2);
	return pl;
}

/*
 * int
 * _Compat_spl3(void)
 *	Set interrupt priority level to PL3 (hard).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
_Compat_spl3(void)
{
	pl_t pl = getpl();
	splx_h(PL3);
	return pl;
}

/*
 * int
 * _Compat_spl3s(void)
 *	Set interrupt priority level to PL3 (soft).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
_Compat_spl3s(void)
{
	pl_t pl = getpl();
	splx(PL2);
	return pl;
}

/*
 * int
 * _Compat_spl4s(void)
 *	Set interrupt priority level to PL4 (soft).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
_Compat_spl4s(void)
{
	return ((int)spl4());
}

/*
 * int
 * _Compat_spl5s(void)
 *	Set interrupt priority level to PL5 (soft).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
_Compat_spl5s(void)
{
	return ((int)spl5());
}


/*
 * int
 * _Compat_spl6s(void)
 *	Set interrupt priority level to PL6 (soft).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
_Compat_spl6s(void)
{
	return ((int)spl6());
}


/*
 * int
 * _Compat_spl7s(void)
 *	Set interrupt priority level to PL7 (soft).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
_Compat_spl7s(void)
{
	return ((int)spl7());
}

/*
 * int
 * _Compat_splhis(void)
 *	Set interrupt priority level to PLHI (soft).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
_Compat_splhis(void)
{
	return ((int)splhi());
}

/*
 * int
 * _Compat_splx(int newpl)
 *	Set interrupt priority level to specified newpl (hard).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
_Compat_splx(int newpl)
{
	pl_t pl = getpl();
	splx_h(newpl);
	return pl;
}

/*
 * int
 * _Compat_splxs(int newpl)
 *	Set interrupt priority level to specified newpl (soft).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
int
_Compat_splxs(int newpl)
{
	pl_t pl = getpl();
	splx(newpl);
	return pl;
}

/*
 * paddr_t
 * _Compat_kvtophys(vaddr_t vaddr)
 *
 * Calling/Exit State:
 *	Return the physical address equivalent of the virtual address
 *	argument.
 *
 * Remarks:
 *	Argument was of type caddr_t, but is now vaddr_t for consistency
 *	with the macro form used by the kernel; it doesn't matter for drivers
 *	since only old binaries should be using this function.
 */
paddr_t
_Compat_kvtophys(vaddr_t vaddr)
{
	return _KVTOPHYS(vaddr);
}

/*
 * uint_t
 * _Compat_hat_getkpfnum(caddr_t vaddr)
 *
 * Calling/Exit State:
 *	Return the physical page ID equivalent to the kernel virtual
 *	address argument.
 */
uint_t
_Compat_hat_getkpfnum(caddr_t vaddr)
{
	return (uint_t)kvtoppid(vaddr);
}

/*
 * uint_t
 * _Compat_hat_getppfnum(caddr_t paddr, u_int pspace)
 *
 * Calling/Exit State:
 *	Return the physical page ID equivalent to the physical address
 *	argument.
 */
uint_t
_Compat_hat_getppfnum(caddr_t paddr, u_int pspace)
{
	if (pspace != PSPACE_MAINSTORE)
		return NOPAGE;

	return (uint_t)phystoppid((paddr_t)paddr);
}

/*
 * queue_t *
 * _Compat_backq(q)
 *	Return the queue upstream from this one.
 *
 * Calling/Exit State:
 *	sd_mutex assumed unlocked.
 */
queue_t *
_Compat_backq(queue_t *q)
{
	pl_t pl;
	queue_t *retq;

	pl = LOCK(q->q_str->sd_mutex, PLSTR);
	retq = backq_l(q);
	UNLOCK(q->q_str->sd_mutex, pl);
	return retq;
}

/*
 * int
 * _Compat_useracc(caddr_t addr, uint_t count, int access)
 *	Determine if a user address range is accessible
 *
 * Calling/Exit State:
 *	Return 1 if the specified range is accessible.
 *	Otherwise return 0.
 *
 * Remarks:
 *	Note that in a multi-LWP process, the answer we give is
 *	potentially stale.
 */
int
_Compat_useracc(caddr_t addr, uint_t count, int access)
{
	uint_t prot;
	struct as *as = u.u_procp->p_as;
	int err;

	ASSERT(as != NULL);

	prot = PROT_USER | ((access == B_READ) ? PROT_READ : PROT_WRITE);
	as_rdlock(as);
	err = as_checkprot(as, (vaddr_t)addr, count, prot);
	as_unlock(as);
	return (err == 0);
}

/*
 * void
 * _Compat_dma_pageio(void (*strat)(), buf_t *bp)
 *	Break up a B_PHYS request at page boundaries (for DMA).
 *
 * Calling/Exit State:
 *	See buf_breakup().
 */
void
_Compat_dma_pageio(void (*strat)(), buf_t *bp)
{
	static physreq_t dma_preq = {
		/* phys_align */	1,
		/* phys_boundary */	PAGESIZE,
		/* phys_dmasize */	0,
		/* phys_max_scgth */	0
	};
	static const bcb_t dma_bcb = {
		/* bcb_addrtypes */	BA_KVIRT|BA_UVIRT|BA_PAGELIST,
		/* bcb_flags */		BCB_PHYSCONTIG,
		/* bcb_max_xfer */	PAGESIZE,
		/* bcb_granularity */	NBPSCTR,
		/* bcb_physreqp */	&dma_preq
	};
	(void)physreq_prep(&dma_preq, KM_SLEEP);
	buf_breakup(strat, bp, &dma_bcb);
}

/*
 * void
 * _Compat_rdma_filter(void (*strat)(), buf_t *bp)
 *	Make sure a buffer is DMAable.
 *
 * Calling/Exit State:
 *	See buf_breakup().
 */
void
_Compat_rdma_filter(void (*strat)(), buf_t *bp)
{
#ifndef NO_RDMA
	if (rdma_mode != RDMA_DISABLED)
		buf_breakup(strat, bp, &rdma_dflt_bcb);
	else
#endif
		(*strat)(bp);
}

/*
 * int
 * _Compat_v86setint(void *v86p, unsigned int intr_bits)
 *	Set virtual interrupt bits for v86 task -- stubbed.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Remarks:
 *	In the absence of VPIX support we do not expect this
 *	routine to be called; it exists primarily to satisfy the
 *	external reference in old drivers with VPIX support.
 */
int
_Compat_v86setint(void *v86p, unsigned int intr_bits)
{
	return 0;
}

/*
 * int
 * _Compat_validproc(proc_t *procp, pid_t pid)
 *	Return true iff the specified process is alive and well.
 *
 * Calling/Exit State:
 *	This function acquires and releases p_mutex.
 *	No locks held on entry or exit.
 *
 * Remarks:
 *	Both the pid and procp must match, and zombie processes don't count.
 *	However, as soon as we drop the process lock the result is stale.
 */
int
_Compat_validproc(proc_t *procp, pid_t pid)
{
	pl_t	pl;
	proc_t	*p;
	int	rv;

	if (procp == NULL)			/* common case */
		return 0;

	pl = getpl();

	if ((p = prfind(pid)) == NULL)		/* leaves p->p_mutex locked */
		return 0;

	rv = (p == procp && p->p_nlwp > 0);	/* same procp and not zombie */
	UNLOCK(&p->p_mutex, pl);
	return rv;
}

#if defined (_KMEM_STATS) || defined(_KMEM_HIST)
/*
 * Kernels built with _KMEM_STATS and/or _KMEM_HIST turn
 * kmem_alloc, kmem_zalloc, kmem_alloc_physcontig, and
 * kmem_alloc_physcreq into macros.  Such macros are unwelcome here.
 */
#undef kmem_alloc
#undef kmem_zalloc
#undef kmem_alloc_physcontig
#undef kmem_alloc_physreq
extern void *kmem_alloc(size_t, int);
extern void *kmem_zalloc(size_t, int);
extern void *kmem_alloc_physcontig(size_t, const physreq_t *, int);
extern void *kmem_alloc_physreq(size_t, const physreq_t *, int);

/*
 * void *
 * kmem_i_alloc_physcont(size_t size, physreq_t *physreq, int flags,
 *			 int line, char *file)
 *	Allocate physically-contiguous memory.
 *
 * Calling/Exit State:
 *	Instrumented older version of kmem_alloc_physreq() which always
 *	returns physically contiguous memory. This is now a ``level 2''
 *	interface. New code should be using kmem_alloc_physreq() in
 *	preference to this interface.
 *
 *	The only valid flags are KM_SLEEP and KM_NOSLEEP.
 */
void *
kmem_i_alloc_physcont(size_t size, const physreq_t *physreq, int flags,
		      int line, char *file)
{
	ASSERT(!(flags & ~KM_NOSLEEP));

	return kmem_i_alloc_physreq(size, physreq, flags | KM_PHYSCONTIG,
				    line, file);
}

/*
 * void *
 * _Compat_kmem_instr_alloc(size_t size, int flags, int line, char *file)
 *	Allocate virtual and physical memory of arbitrary size, plus
 *	gather statistics and/or history.
 *
 *	The memory returned is DMAable for compatibility with the
 *	default behavior provided by previous implementations of the DDI.
 *
 * Calling/Exit State:
 *	None.
 */
void *
_Compat_kmem_instr_alloc(size_t size, int flags, int line, char *file)
{
	return kmem_instr_alloc(size, flags | KM_REQ_DMA, line, file);
}

/*
 * void *
 * _Compat_kmem_instr_zalloc(size_t size, int flags, int line, char *file)
 *	Allocate virtual and physical memory of arbitrary size, and
 *	return it zeroed-out. Also gather statistics and/or history.
 *
 *	The memory returned is DMAable for compatibility with the
 *	default behavior provided by previous implementations of the DDI.
 *
 * Calling/Exit State:
 *	None.
 */
void *
_Compat_kmem_instr_zalloc(size_t size, int flags, int line, char *file)
{
	return kmem_instr_zalloc(size, flags | KM_REQ_DMA, line, file);
}

/*
 * void *
 * _Compat_kmem_i_alloc_physcont(size_t size, physreq_t *physreq,
 *				       int flags, int line, char *file))
 *	Allocate physically-contiguous memory. Also gather statistics and/or
 *	history.
 *
 *	The memory returned is DMAable for compatibility with the
 *	default behavior provided by previous implementations of the DDI.
 *
 * Calling/Exit State:
 *	None.
 */
void *
_Compat_kmem_i_alloc_physcont(size_t size, physreq_t *physreq, int flags,
				    int line, char *file)
{
	return kmem_i_alloc_physreq(size, physreq,
				    flags | KM_REQ_DMA | KM_PHYSCONTIG,
				    line, file);
}
#endif /* defined (_KMEM_STATS) || defined(_KMEM_HIST) */

/*
 * void *
 * kmem_alloc_physcontig(size_t size, physreq_t *physreq, int flags)
 *	Allocate physically-contiguous memory.
 *
 * Calling/Exit State:
 *	Older version of kmem_alloc_physreq() which always returns
 *	physically contiguous memory. This is now a ``level 2''
 *	interface. New code should be using kmem_alloc_physreq()
 *	in preference to this interface.
 *
 *	The only valid flags are KM_SLEEP and KM_NOSLEEP.
 */
void *
kmem_alloc_physcontig(size_t size, const physreq_t *physreq, int flags)
{
	ASSERT(!(flags & ~KM_NOSLEEP));

	return kmem_alloc_physreq(size, physreq, flags | KM_PHYSCONTIG);
}

/*
 * void *
 * _Compat_kmem_alloc(size_t size, int flags)
 *	Allocate virtual and physical memory of arbitrary size.
 *
 *	The memory returned is DMAable and contiguous for compatibility
 *	with the default behavior provided by previous implementations of
 *	the DDI.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	We try for contiguous memory, but not so hard that we will
 *	wait forever for it. We should probably try harder than
 *	just KM_NOSLEEP, but this will do for now.
 */
void *
_Compat_kmem_alloc(size_t size, int flags)
{
	void * mem;

	mem = kmem_alloc(size, flags | KM_REQ_DMA | KM_PHYSCONTIG | KM_NOSLEEP);
	if (mem == NULL)
		mem = kmem_alloc(size, flags | KM_REQ_DMA);

	return mem;
}

/*
 * void *
 * _Compat_kmem_zalloc(size_t size, int flags)
 *	Allocate virtual and physical memory of arbitrary size, and
 *	return it zeroed-out.
 *
 *	The memory returned is DMAable and contiguous for compatibility
 *	with the default behavior provided by previous implementations of
 *	the DDI.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	We try for contiguous memory, but not so hard that we will
 *	wait forever for it. We should probably try harder than
 *	just KM_NOSLEEP, but this will do for now.
 */
void *
_Compat_kmem_zalloc(size_t size, int flags)
{
	void * mem;

	mem = kmem_zalloc(size,
			  flags | KM_REQ_DMA | KM_PHYSCONTIG | KM_NOSLEEP);
	if (mem == NULL)
		mem = kmem_zalloc(size, flags | KM_REQ_DMA);

	return mem;
}

/*
 * void *
 * _Compat_kmem_alloc5(size_t size, int flags)
 *	Allocate virtual and physical memory of arbitrary size.
 *
 *	The memory returned is DMAable for compatibility with the
 *	default behavior provided by previous implementations of the DDI.
 *
 * Calling/Exit State:
 *	None.
 */
void *
_Compat_kmem_alloc5(size_t size, int flags)
{
	return kmem_alloc(size, flags | KM_REQ_DMA);
}

/*
 * void *
 * _Compat_kmem_zalloc5(size_t size, int flags)
 *	Allocate virtual and physical memory of arbitrary size, and
 *	return it zeroed-out.
 *
 *	The memory returned is DMAable for compatibility with the
 *	default behavior provided by previous implementations of the DDI.
 *
 * Calling/Exit State:
 *	None.
 */
void *
_Compat_kmem_zalloc5(size_t size, int flags)
{
	return kmem_zalloc(size, flags | KM_REQ_DMA);
}

/*
 * void *
 * _Compat_kmem_alloc_physcontig(size_t size, const physreq_t *physreq,
 *				 int flags)
 *	Allocate physically-contiguous memory.
 *
 *	The memory returned is DMAable for compatibility with the
 *	default behavior provided by previous implementations of the DDI.
 *
 * Calling/Exit State:
 *	None.
 */
void *
_Compat_kmem_alloc_physcontig(size_t size, const physreq_t *physreq, int flags)
{
	return kmem_alloc_physreq(size, physreq,
				  flags | KM_REQ_DMA | KM_PHYSCONTIG);
}

/*
 * mblk_t *
 * _Compat_allocb(int size, uint_t pri)
 *	Allocate a new message block.
 *
 *	The data block memory returned is physically contiguous and DMAable
 *	for compatibility with the default behavior provided by previous
 *	implementations of the DDI.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */
mblk_t *
_Compat_allocb(int size, uint_t pri)
{
	return allocb_physreq(size, pri, strphysreq);
}

/*
 * mblk_t *
 * _Compat_copyb(mblk_t *bp)
 *	Copy data from message block to newly allocated message block and
 *	data block.
 *
 *	The data block memory returned is physically contiguous and DMAable
 *	for compatibility with the default behavior provided by previous
 *	implementations of the DDI.
 *
 * Calling/Exit State:
 *	Returns new message block pointer, or NULL if error.  No locking
 *	assumptions.
 */
mblk_t *
_Compat_copyb(mblk_t *bp)
{
	return copyb_physreq(bp, strphysreq);
}

/*
 * mblk_t *
 * _Compat_copymsg(mblk_t *bp)
 *	Copy data from message to newly allocated message using new
 *	data blocks.
 *
 *	The data block memory returned is physically contiguous and DMAable
 *	for compatibility with the default behavior provided by previous
 *	implementations of the DDI.
 *
 * Calling/Exit State:
 *	Returns a pointer to the new message, or NULL if error.  No locking
 *	assumptions.
 */
mblk_t *
_Compat_copymsg(mblk_t *bp)
{
	return copymsg_physreq(bp, strphysreq);
}

/*
 * mblk_t *
 * _Compat_msgpullup(mblk_t *bp, int len)
 *      Concatenate and align first len bytes of common message type.
 *      len == -1, means concat everything.
 *
 *	The data block memory returned is physically contiguous and DMAable
 *	for compatibility with the default behavior provided by previous
 *	implementations of the DDI.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */
mblk_t *
_Compat_msgpullup(mblk_t *bp, int len)
{
	return msgpullup_physreq(bp, len, strphysreq);
}

/*
 * void
 * _Compat_seterror(int errno)
 *	Set the error number to be returned from the current syscall.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Remarks:
 *	Since seterror() can only be called in user context, we assume
 *	its use is limited to driver open and close routines, and, for
 *	non-STREAMS drivers, read/write/ioctl.
 */
void
_Compat_seterror(int errno)
{
	u.u_compat_errno = errno;
}
