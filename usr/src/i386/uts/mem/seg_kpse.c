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

#ident	"@(#)kern-i386:mem/seg_kpse.c	1.2"
#ident	"$Header: $"

#include <fs/vnode.h>
#include <io/conf.h>
#include <mem/as.h>
#include <mem/faultcode.h>
#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/pse.h>
#include <mem/pse_hat.h>
#include <mem/seg.h>
#include <mem/seg_pse.h>
#include <mem/vmparam.h>
#include <proc/mman.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/map.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>

/*
 * Kernel segment for PSE mappings.  Allows large physmap requests to
 * be mapped via PSE mappings.
 #
 * segkpse is a kernel segment driver for creating PSE mappings.
 * It is derived, in part, from seg_kmem.
 *
 * Allocation of virtual space in the segment is controlled by
 * the resource map kpse_rmap.  Each unit in the resource map
 * represents PSE_PAGESIZE bytes.  kpse_rbase and kpse_rsize
 * are the base and size of the resource map.  Thus:
 *
 *	base virtual address of segment = kpse_rbase * PSE_PAGESIZE
 *	size segment in bytes = kpse_rsize * PSE_PAGESIZE
 *
 * kpse_cookie is a dynamically allocated area of TLBScookies
 * used to do shootdowns when an address is recycled.
 */
STATIC struct map *kpse_rmap = NULL;
STATIC uint_t kpse_rbase = 0;
STATIC uint_t kpse_rsize = 0;
STATIC TLBScookie_t *kpse_cookie = NULL;

extern int pse_vmemsize;	/* number of virtual pages to reserve */ 

/*
 * void
 * kpse_create(int npse)
 *	Create the kernel PSE mapping segment.
 *
 * Calling/Exit State:
 *	Called at startup, before VM is initialized.
 *
 * Description:
 *	Allocates virtual space from the kernel (via calloc_virtual),
 *	sets the size and base of the segment.
 */
void
kpse_create(void)
{
	void *va;
	size_t size;

	if ((pse_vmemsize > 0) && PSE_SUPPORTED()) {
		callocrnd(PSE_PAGESIZE);
		size = psetob(btopse(pse_vmemsize));
		va = calloc_virtual(size);
		ASSERT((va != NULL) && (((vaddr_t)va & PSE_PAGEOFFSET) == 0));
		kpse_rsize = size;
		kpse_rbase = btopse((vaddr_t)va);
	}
}

/*
 * void
 * kpse_init(void)
 *	Initialize the kernel pse segment
 *
 * Calling/Exit State:
 *	None
 *
 * Description:
 *	Initializes the kernel PSE mapping segment, by allocating the
 *	resource map and TLBScookie array.
 */
void
kpse_init(void)
{

	ASSERT(kpse_rmap == NULL);
	ASSERT(kpse_cookie == NULL);
	ASSERT(PSE_SUPPORTED());

	if (kpse_rsize == 0)
		return;

	kpse_rmap = rmallocmap(kpse_rsize);
	if (kpse_rmap == NULL) {
		kpse_rsize = 0;
		return;
	}

	kpse_cookie = kmem_zalloc(kpse_rsize * sizeof(TLBScookie_t),
		KM_NOSLEEP);
	if (kpse_cookie == NULL) {
		rmfreemap(kpse_rmap);
		kpse_rsize = 0;
		return;
	}
	rmfree(kpse_rmap, kpse_rsize, kpse_rbase);
}

/*
 * STATIC vaddr_t
 * kpse_vm_alloc(uint_t npse)
 *	Allocate virtual space from the kernel PSE mapping segment.
 *	The size is requested in PSE_PAGESIZE units, and the returned
 *	address is PSE_PAGESIZE-aligned.
 *
 * Calling/Exit State:
 *	None
 */
STATIC vaddr_t
kpse_vm_alloc(uint_t npse)
{
	ulong_t base;
	uint_t i;

	if (kpse_rsize == 0)
		return NULL;

	ASSERT(kpse_rmap != NULL);
	ASSERT(PSE_SUPPORTED());;

	base = rmalloc(kpse_rmap, npse);
	if (base == 0)
		return NULL;
	for (i = 0 ; i < npse ; ++i)
		hat_shootdown(kpse_cookie[(base - kpse_rbase) + i],
			HAT_HASCOOKIE);
	return psetob(base);
}

/*
 * void
 * kpse_vm_free(vaddr_t vaddr, uint_t npse)
 *	Free kernel virtual space previously obtained from the
 *	kernel PSE mapping segment.   The virtual address passed
 *	in is a PSE_PAGESIZE-aligned byte address, and the size
 *	is specified as a number of PSE_PAGESIZE units.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
kpse_vm_free(vaddr_t va, uint_t npse)
{
	int base, i;
	TLBScookie_t cookie;

	base = btopse(va);

	ASSERT((kpse_rbase != 0) && (kpse_rsize != 0) && (kpse_rmap != NULL) &&
		(kpse_cookie != NULL));
	ASSERT((kpse_rbase <= base) &&
			((base + npse) <= (kpse_rbase + kpse_rsize)));

	cookie = hat_getshootcookie();

	pse_hat_statpt_unload(va, npse);
	rmfree(kpse_rmap, npse, base);

	for (i = 0 ; i < npse ; ++i)
		kpse_cookie[(base - kpse_rbase) + i] = cookie;
}

/*
 * void *
 * kpse_ppid_mapin(ulong_t npse, ppid_t ppid, uint_t prot, uint_t sleep_flag)
 *	Allocates virtual space in PSE mapping segment and maps
 *	the allocated space to specified physical pages.  Requested
 *	virtual size is in PSE_PAGESIZE units.
 *	
 * Calling/Exit State:
 *	If successful, returns the starting virtual address for the
 *	virtual space, otherwise, returns NULL.
 */
/*ARGSUSED3*/
void *
kpse_ppid_mapin(ulong_t npse, ppid_t ppid, uint_t prot, uint_t sleep_flag)
{
	vaddr_t addr;

	/*
	 * allocate kernel virtual space
	 */

	if ((addr = kpse_vm_alloc(npse)) == NULL)
		return ((void *)NULL);

	ASSERT(addr != (vaddr_t)NULL);
	ASSERT((addr & PSE_PAGEOFFSET) == 0);

	pse_hat_statpt_devload(addr, npse, ppid, prot);
	return((void *)addr);
}

/*
 * void
 * kpse_mapout(void *vaddr, ulong_t npse)
 *	Unmap and free specified virtual address range.  vaddr
 *	is a PSE_PAGESIZE-aligned address specifying the start
 *	of the range, and npse is the number of PSE_PAGESIZE units
 *	in the range.
 *
 * Calling/Exit State:
 *	On entry, the specified range must be mapped.
 *
 *	On exit, the range is unmapped.
 */
void
kpse_mapout(void *vaddr, ulong_t npse)
{
	ASSERT(((vaddr_t)vaddr & PSE_PAGEOFFSET) == 0);

	kpse_vm_free((vaddr_t)vaddr, npse);
}

/*
 * caddr_t
 * pse_physmap(paddr_t, ulong_t, uint_t)
 *	Allocate a virtual address mapping for a range
 *	of physical addresses using PSE mappings.
 *
 * Calling/Exit State:
 *	Returns virtual address allocated, or NULL.
 */
caddr_t
pse_physmap(paddr_t physaddr, ulong_t nbytes, uint_t flags)
{
	paddr_t base;
	ulong_t npse;
	caddr_t addr;

	ASSERT(nbytes != 0);

	if (!PSE_SUPPORTED() || (nbytes < KPSE_MIN))
		return NULL;

	base = physaddr & PSE_PAGEMASK;	/* round-down physical address */
	npse = btopser(physaddr - base + nbytes);	/* round-up pages */

	if ((psetob(npse) - nbytes) > KPSE_WASTE)
		return NULL;

	addr = kpse_ppid_mapin(npse, phystoppid(base), PROT_ALL & ~PROT_USER,
				flags);

	if (addr == (caddr_t)NULL)
		return (caddr_t)NULL;
	else
		return addr + PSE_PAGOFF(physaddr);
}

/*
 * void
 * pse_physmap_free(caddr_t, ulong_t, uint_t)
 *	Release a mapping allocated by pse_physmap(). 
 *
 * Calling/Exit State:
 *	The mapping requested may not have been allocated by
 *	pse_physmap.  The routine checks to see if it is
 *	mapped using PSE, and, if it is, then it unmaps it
 *	If it is not mapped via PSE, it does not unmap it.
 *	The routine returns B_TRUE if it unmapped the mapping,
 *	or B_FALSE if it left it alone.
 */
/*ARGSUSED*/
boolean_t
pse_physmap_free(caddr_t vaddr, ulong_t nbytes, uint_t flags)
{
	vaddr_t base;
	ulong_t npse;

	ASSERT((vaddr != NULL) && (nbytes != 0));

	if (!PG_ISPSE(kvtol1ptep((vaddr_t)vaddr)))
		return B_FALSE;

	ASSERT(PSE_SUPPORTED());

	base = (vaddr_t)vaddr & PSE_PAGEMASK; 	/* round-down address */
	npse = btopser((vaddr_t)vaddr + nbytes - base);  /* round-up pages */

	kpse_mapout((void *)base, npse);

	return B_TRUE;
}
