/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1993 UNIX System Laboratories, Inc.	*/
/*	(a wholly-owned subsidiary of Novell, Inc.).     	*/
/*	All Rights Reserved.                             	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF UNIX SYSTEM     	*/
/*	LABORATORIES, INC. (A WHOLLY-OWNED SUBSIDIARY OF NOVELL, INC.).	*/
/*	The copyright notice above does not evidence any actual or     	*/
/*	intended publication of such source code.                      	*/

#ident	"@(#)kern-i386:mem/pse_hat.c	1.3"
#ident	"$Header: $"

#include <acc/mac/mac.h>
#include <util/types.h>
#include <util/param.h>
#include <util/emask.h>
#include <mem/immu.h>
#include <fs/vnode.h>
#include <proc/mman.h>
#include <mem/tuneable.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/sysmacros.h>
#include <util/inline.h>
#include <svc/errno.h>
#include <proc/proc.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <svc/systm.h>
#include <mem/kmem.h>
#include <mem/pse.h>
#include <mem/vmparam.h>
#include <mem/vm_hat.h>
#include <mem/hat.h>
#include <mem/seg.h>
#include <mem/as.h>
#include <mem/page.h>
#include <mem/anon.h>
#include <mem/hatstatic.h>
#include <mem/seg_kmem.h>
#include <mem/mem_hier.h>
#include <proc/cred.h>
#include <svc/creg.h>
#include <svc/cpu.h>
#include <util/plocal.h>
#include <svc/clock.h>
#include <util/engine.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <proc/disp.h>
#ifdef CC_PARTIAL
#include <acc/mac/covert.h>
#include <acc/mac/cc_count.h>
#endif

/*
 * Hat layer support for PSE.  Supports the following entries:
 *	pse_hat_chgprot
 *	pse_hat_devload
 *	pse_hat_unload
 *
 * The PSE hat layer support is used by the seg_pse and seg_kpse
 * drivers.  It's written with the following assumptions:
 *	- PSE pages are locked in memory.
 *	- PSE pages have no backing store; their contents aren't
 *		saved anywhere.  Therefore, the reference and
 *		modify bits of PSE page directory entries
 *		are not tracked.
 *	- User-level PSE translations are created when a seg_pse
 *		segment is created, and unloaded only when the
 *		segment is unmapped.
 */
void unlink_ptap(struct as *, hatpt_t *);
void link_ptap(struct as *, hatpt_t *, hatpt_t *);
hatpt_t *hat_findpt(hat_t *, pte_t *);
void hat_reload_l1ptes(hat_t *, pte_t *, uint_t);
void hat_zerol1ptes(hat_t *, pte_t *, uint_t);
int hat_uas_shootdown_l(hat_t *);

extern hat_t *kas_hatp;

/*
 * STATIC hatpt_t *
 * pse_hat_ptalloc(void)
 *	Allocate a hatpt_t for a PSE mapping
 *
 * Calling/Exit State:
 *	returns a pointer to a hatpt_t, or NULL.
 *
 * Remarks:
 *	Just kmem_zalloc a hatpt_t; no mapping chunks or page table
 *	needed.
 */
STATIC hatpt_t *
pse_hat_ptalloc(void)
{

	return kmem_zalloc(sizeof(hatpt_t), KM_SLEEP);
}

/*
 * STATIC void
 * pse_hat_ptfree(hatpt_t *ptap)
 *	Frees up a hatpt_t previously used for a PSE mapping.
 *
 * Calling/Exit State:
 *	ptap is for a PSE mapping.
 *	returns a pointer to a hatpt_t, or NULL.
 *
 * Remarks:
 *	Just kmem_free a hatpt_t; doesn't need mapping chunks or a
 *	page table proper.
 */
STATIC void
pse_hat_ptfree(hatpt_t *ptap)
{

	ASSERT(ptap->hatpt_pde.pgm.pg_ps);
	kmem_free(ptap, sizeof(hatpt_t));
}

/*
 * boolean
 * pse_hat_chgprot(struct seg *seg, vaddr_t addr, ulong_t len, uint_t prot,
 *		bolean_t doshoot)
 *	Change the protections for the virtual address range
 *	[addr,addr+len] to the protection prot.
 *
 * Calling/Exit State:
 *	The return value is useful only when argument doshoot is B_FALSE.
 *	It return B_TRUE, if the caller needs to perform a shootdown.
 *	It returns B_FALSE, if no shootdown is neccessary. 
 *	No hat or page locks are held by the caller.
 *	No hat or page locks are held on return.
 *	This is never used on the kas.
 *	This changes only active PTEs, no mappings are added,
 *	and none are removed. So only the hat resource lock is needed.
 *	TLB Shootdown is performed based on the doshoot flag. If the flag
 *	is B_FALSE, then the caller gurantees that there are no accesses
 *	to this range and the caller is reponsible to perfrom the shootdown.
 *	Since vtop operations must search the hatpt list anyway, the trick of 
 *	unloading level 1 entries is used to minimize the TLBS
 *	synchronization time (the time between TLBSsetup and TLBSteardown).
 *
 * Remarks:
 *	We don't sweat the loss of modify and reference information from
 *	the pte, since the contents of PSE pages aren't saved anywhere.
 */

/*ARGSUSED4*/
boolean_t
pse_hat_chgprot(struct seg *seg, vaddr_t addr, ulong_t len, uint_t prot,
		boolean_t doshoot)
{
	struct hat *hatp = &seg->s_as->a_hat;
	pl_t opl;
	hatpt_t *ptap;
	pte_t *vpdte, *evpdte;
	uint_t pprot, pmask;
	boolean_t doflush, asloaded;

	ASSERT((addr & PSE_PAGEOFFSET) == 0);
	ASSERT((len & PSE_PAGEOFFSET) == 0);
	ASSERT(PSE_SUPPORTED());
	if (prot == (unsigned)~PROT_WRITE) {
		pmask = (unsigned)~PG_RW;
		pprot = 0;
	} else {
		pmask = (unsigned)~(PTE_PROTMASK|PG_V);
		pprot = hat_vtop_prot(prot);
	}
	vpdte = &kpd0[ptnum(addr)];
	evpdte = vpdte + btopse(len);
	opl = HATRL_LOCK(hatp);
	ptap = hat_findpt(hatp, vpdte);
	ASSERT((ptap != NULL) && (ptap->hatpt_pdtep == vpdte));
	asloaded = (seg->s_as == u.u_procp->p_as);
	do {
		ASSERT(ptap->hatpt_pde.pg_pte & PG_PS);
		ptap->hatpt_pde.pg_pte &= pmask;
		ptap->hatpt_pde.pg_pte |= pprot;
		if (asloaded)
			ptap->hatpt_pdtep->pg_pte = ptap->hatpt_pde.pg_pte;
		ptap = ptap->hatpt_forw;
	} while ((ptap != hatp->hat_pts) && (ptap->hatpt_pdtep < evpdte));
	hat_reload_l1ptes(hatp, vpdte, evpdte - vpdte);
	doflush = hat_uas_shootdown_l(hatp);
	HATRL_UNLOCK(hatp, opl);
	ASSERT(doflush == asloaded);
	if (doflush)
		TLBSflushtlb();
	return B_FALSE;
}

/*
 * void
 * pse_hat_devload(struct seg *seg, vaddr_t addr, ppid_t ppid, uint_t prot)
 *	
 *
 * Calling/Exit State:
 *	The containing as is write-locked on entry and remains
 *	write-locked on exit.
 * 
 * Remarks:
 *	Don't bother trimming after loading the pte, since we don't
 *	count this against a_rss.
 */
void
pse_hat_devload(struct seg *seg, vaddr_t addr, ppid_t ppid, uint_t prot)
{
	hat_t *hatp;
	hatpt_t *prev_ptap, *ptap;
	uint_t mode;

	ASSERT(PSE_SUPPORTED());
	ASSERT((addr & PSE_PAGEOFFSET) == 0);
	ASSERT((ppid & PSE_NPGOFFSET) == 0);
	ASSERT(seg->s_as != &kas);

	/*
	 * Disable caching for anything outside of the mainstore memory.
	 */
	mode = hat_vtop_prot(prot);
	if (!mainstore_memory(mmu_ptob(ppid)))
		mode |= PG_CD;

	hatp = &seg->s_as->a_hat;
	ptap = pse_hat_ptalloc();

	ptap->hatpt_pde.pg_pte = pse_mkpte(mode, ppid);
	ptap->hatpt_pdtep = &kpd0[ptnum(addr)];
	ptap->hatpt_as = seg->s_as;
	ptap->hatpt_aec = 1;
	ptap->hatpt_hec = 1;
	/*
	 * The structure was zeroed out on allocation, thus
	 * the remaining fields are:
	 *
	 * ptap->hatpt_ptva = NULL;
	 * ptap->hatpt_locks = 0;
	 * ptap->hatpt_mcp[all of them] = NULL;
	 */
	HATRL_LOCK_SVPL(hatp);
	prev_ptap = hat_findpt(hatp, ptap->hatpt_pdtep);
	ASSERT((prev_ptap == NULL) ||
			(prev_ptap->hatpt_pdtep != ptap->hatpt_pdtep));
	link_ptap(seg->s_as, prev_ptap, ptap);
	ptap->hatpt_pdtep->pg_pte = ptap->hatpt_pde.pg_pte;
	hat_reload_l1ptes(hatp, ptap->hatpt_pdtep, 1);
	HATRL_UNLOCK_SVDPL(hatp);
}

/*
 * void
 * pse_hat_unload(struct seg *seg, vaddr_t addr, ulong_t len)
 *	Unload PSE mappings in the range [addr, addr + len).
 *
 * Calling/Exit State:
 *	HATRL_LOCK is available on entry and exit.
 *
 *	Takes care of TLB synchronization.
 */
/*ARGSUSED3*/
void
pse_hat_unload(struct seg *seg, vaddr_t addr, ulong_t len)
{
	struct hat *hatp;
	pl_t opl;
	hatpt_t *ptap, *nptap;
	pte_t *vpdte, *evpdte;
	boolean_t doflush, asloaded;
	struct as *as = seg->s_as;

	ASSERT((addr & PSE_PAGEOFFSET) == 0);
	ASSERT((len & PSE_PAGEOFFSET) == 0);
	ASSERT(PSE_SUPPORTED());
	vpdte = &kpd0[ptnum(addr)];
	evpdte = vpdte + btopse(len);
	hatp = &as->a_hat;
	opl = HATRL_LOCK(hatp);
	ptap = hat_findpt(hatp, vpdte);
	ASSERT((ptap != NULL) && (ptap->hatpt_pdtep == vpdte));
	asloaded = (u.u_procp->p_as == as);
	do {
		ASSERT(ptap->hatpt_pde.pg_pte & PG_PSE);
		ASSERT(ptap->hatpt_aec == 1);
		if (asloaded)
			ptap->hatpt_pdtep->pg_pte = 0;
		if ((nptap = ptap->hatpt_forw) == ptap)
			nptap = NULL;
		unlink_ptap(as, ptap);
		pse_hat_ptfree(ptap);
		ptap = nptap;
	} while ((ptap != hatp->hat_pts) && (ptap->hatpt_pdtep < evpdte));
	hat_zerol1ptes(hatp, vpdte, evpdte - vpdte);
	doflush = hat_uas_shootdown_l(hatp);
	HATRL_UNLOCK(hatp, opl);
	if (doflush)
		TLBSflushtlb();
}

/*
 * void
 * pse_hat_statpt_devload(vaddr_t addr, ulong_t npse, ppid_t phys,
 *		uint_t prot)
 *	Load static PSE page translations.
 *
 * Calling/Exit State:
 *	addr is the starting kernel virtual address.
 *	npse is the number of PSE_PAGESIZE pages to translate.
 *	phys is the starting physical page ID to map.
 *	prot is the protection to establish for the page translations
 *		specified in the same format as supplied to hat_vtop_prot().
 *
 *	Caller ensures the specified address range is not currently translated.
 *
 *	Both addr and phys are on a PSE_PAGESIZE-aligned boundary (virtual
 *	for addr, physical for phys).
 */
void
pse_hat_statpt_devload(vaddr_t addr, ulong_t npse, ppid_t phys, uint_t prot)
{
	pte_t *ptep, *baseptep;
	uint_t pprot, i;

	ASSERT(KADDR(addr));
	ASSERT(npse != 0);
	ASSERT((addr & PSE_PAGEOFFSET) == 0);
	ASSERT((phys & PSE_NPGOFFSET) == 0);

	baseptep = ptep = kvtol1ptep(addr);
	pprot = hat_vtop_prot(prot);
	for (i = 0 ; i < npse ; ++i) {
		ASSERT(ptep->pg_pte == 0);
		ptep->pg_pte = pse_mkpte(pprot, phys);
		ptep++;
		phys += PSE_NPAGES;
	}
	hat_reload_l1ptes(kas_hatp, baseptep, npse);
}

/*
 * void
 * pse_hat_statpt_unload(vaddr_t addr, ulong_t npse)
 *	Unload static PSE page translations.
 *
 * Calling/Exit State:
 *	addr is the starting kernel virtual address.
 *	npse is the number of 4MB pages to unload.
 *
 *	Caller is responsible for doing TLB flush/shootdown!
 */
void
pse_hat_statpt_unload(vaddr_t addr, ulong_t npse)
{
	pte_t *ptep, *baseptep;
	int i;

	ASSERT(KADDR(addr));
	ASSERT((addr & PSE_PAGEOFFSET) == 0);
	ASSERT(npse != 0);

	baseptep = ptep = kvtol1ptep(addr);
	for (i = 0 ; i < npse ; ++i) {
		ASSERT(ptep->pg_pte != 0);
		ptep->pg_pte = 0;
		ptep++;
	}
	hat_zerol1ptes(kas_hatp, baseptep, npse);
}
