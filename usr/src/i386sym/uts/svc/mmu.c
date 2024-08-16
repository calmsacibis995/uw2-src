/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:svc/mmu.c	1.17"
#ident	"$Header: $"

/*
 * Build page tables when booting the system.
 *
 * These routines are executed when physical, not virtual, memory
 * addressing is in effect.
 */

#include <io/SGSproc.h>
#include <io/cfg.h>
#include <io/slic.h>
#include <mem/immu.h>
#include <mem/vmparam.h>
#include <mem/vm_mdep.h>
#include <svc/creg.h>
#include <svc/systm.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>


extern char _etext;	/* &_etext = last byte of kernel text plus 1 */
extern char _edata;	/* &_edata = last byte of kernel data plus 1 */
extern char _end;	/* &_end = last byte of kernel BSS plus 1 */

/*
 * The following values are set here and exported (via _start())
 * to sysinit():
 *
 * pmemptr	- physical address of next unused physical memory.
 *
 *		  This is page-aligned when used by routines in this
 *		  file and when exported to sysinit().
 *
 * vmemptr	- virtual address of next unused virtual memory.
 *
 *		  This is page-aligned when exported to sysinit().
 *
 * myengppriv	- virtual address of this engine's struct ppriv_pages.
 *
 *		  The pp_kl1pt and pp_pmap pages are already in use as the
 *		  current engine's page tables when exported to sysinit().
 *
 * We initialize these values to force them into static data (rather than
 * BSS) so that they are located in low physical memory.
 */

paddr_t pmemptr = 0;			/* next unused physical memory */
vaddr_t vmemptr = 0;			/* next unused virtual memory */
struct ppriv_pages *myengppriv = (struct ppriv_pages *)0;
					/* virt addr of my ppriv_pages */

STATIC	pte_t *pkl1pt = (pte_t *)0;	/* kernel level 1 page table */
STATIC	pte_t *pkl2ptes = (pte_t *)0;	/* kernel level 2 page table to map
					 * all kernel level 2 page tables */

STATIC	ulong_t symtabsize = 0;


STATIC	void	build_table(void);
STATIC	void	pcreate_map(vaddr_t, uint_t, void *, int);
STATIC	paddr_t	phys_palloc(uint_t);
STATIC	int	phys_hwpage_exists(int);
STATIC	int	phys_assfail(const char *, const char *);
STATIC	void	phys_puts(const char *);

extern size_t mod_obj_size;


/*
 * PASSERT(EX, STR)
 *
 *	Physical mode ASSERT().
 */

#ifdef lint

#define PASSERT(EX, STR)	(phys_assfail(#EX, STR))	/* for lint */

#else /* lint */

#define PASSERT(EX, STR)	((void)((EX) || phys_assfail(#EX, STR)))

#endif /* lint */



/*
 * void
 * init_mmu(void)
 *
 *	Initialize page tables and turn on mapping.
 *
 * Calling/Exit State:
 *
 *	Called from start() with physical addressing mode active.
 *
 *	Returns with virtual addressing mode active and the kernel
 *	virtual address space setup.  BSS is zeroed.  The following
 *	global variables have been set (so they may be passed by 
 *	_start() as arguments to sysinit()):
 *
 *	pmemptr	   - page-aligned physical address of next unused
 *		     physical memory (to initialize calloc()).
 *
 *	vmemptr	   - page-aligned virtual address of next unused
 *		     virtual memory (to initialize calloc()).
 *
 *	myengppriv - virtual address of this engine's struct ppriv_pages.
 *		     The pp_kl1pt and pp_pmap pages are already in use as
 *		     the current engine's level 1 and level 2 page tables.
 */

void
init_mmu(void)
{
	ulong_t cr0;

	/*
	 * Retrieve the symbol table size.
	 * (Calculate the physical address of the value from
	 * the virtual address by subtracting KVSBASE.)
	 */
	symtabsize = *(int *)((paddr_t)&mod_obj_size - KVSBASE);

	/*
	 * Start allocating physical memory at the first page past
	 * the end of the kernel (including symbol table if present).
	 */

	pmemptr = roundup((paddr_t)&_end - KVSBASE, MMU_PAGESIZE);
	pmemptr = roundup(pmemptr + symtabsize, MMU_PAGESIZE);

	/*
	 * Build kernel page tables.
	 */

	build_table();

	/*
	 * Load CR3 with kernel level 1 page table root
	 * and enable paging.
	 */

	WRITE_PTROOT((ulong_t)pkl1pt);
	cr0 = READ_MSW() | CR0_PG;
	WRITE_MSW(cr0);

	/*
	 * Now virtual memory is active.  
	 *
	 * Zero BSS.
	 */

	bzero(&_edata, &_end - &_edata);
}

/*
 * STATIC void
 * build_table(void)
 *
 *	Allocate various statically defined chunks of virtual space
 *	and build page tables.
 *
 * Calling/Exit State:
 *
 *	Caller initializes pmemptr so that phys_palloc() works.
 *
 *	Returns with the kernel virtual address space setup.
 *	The following global variables have been set:
 *
 *	pmemptr	   - page-aligned physical address of next unused
 *		     physical memory.
 *
 *	vmemptr	   - page-aligned virtual address of next unused
 *		     virtual memory.
 *
 *	myengppriv - virtual address of this engine's struct ppriv_pages.
 *		     The pp_kl1pt and pp_pmap pages are already in use as
 *		     the current engine's level 1 and level 2 page tables.
 */

STATIC void
build_table(void)
{
	vaddr_t vaddr;
	paddr_t paddr;
	uint_t msize;
	pte_t *ptr;
	uint_t pprivpages;
	struct ppriv_pages *pmyengppriv;       /* phys addr of my ppriv_pages */
	struct mets *pmet;			/* phys addr of metrics */

	/*
	 * The per-engine pages must fit in a single level 2 page table.
	 */
	PASSERT(KVLAST_PLAT >= KVPER_ENG, "build_table");

	/*
	 * The hardwired kernel virtual load address (KVSBASE)
	 * must allow enough room for KL2PTES.
	 */
	PASSERT(KVSBASE >= KL2PTES + KL2PTES_SIZE, "build_table");

	/*
	 * Allocate the set of per-engine pages for the current engine.
	 * The allocation for the rest of the engines happens later
	 * in sysinit().
	 */

	pprivpages = mmu_btopr(sizeof(struct ppriv_pages));
	pmyengppriv = (struct ppriv_pages *) phys_palloc(pprivpages);

	/*
	 * Setup temporary global pointers to the level 1 & 2 page tables
	 * allocated for this engine in struct ppriv_pages.
	 * (Used only by routines in this file.)
	 *
	 * Then fill in the level 1 pointers to the level 2 page tables for:
	 *	- the per-engine pages (4 Meg)
	 *	- the KL2PTES array of global kernel level 2 ptes
	 *
	 * This gets us to the point where we can call pcreate_map().
	 */

	pkl1pt = &pmyengppriv->pp_kl1pt[0][0];

	ptr = &pmyengppriv->pp_pmap[0][0];
	pkl1pt[ptnum(KVPER_ENG)].pg_pte =
				mkpte(PG_US | PG_RW | PG_V, pfnum(ptr));

	/* Assert KL2PTES on page table boundary */
	PASSERT(pgndx(KL2PTES) == 0, "build_table");
	/* Assert KL2PTES and KL2PTESPTES in same page table */
	PASSERT(ptnum(KL2PTESPTES) == ptnum(KL2PTES), "build_table");

	pkl2ptes = (pte_t *)phys_palloc(1);
	pkl1pt[ptnum(KL2PTES)].pg_pte =
		pkl2ptes[pgndx(KL2PTESPTES)].pg_pte =
				mkpte(PG_RW | PG_V, pfnum(pkl2ptes));

	/*
	 * Allocate and map the per-engine pages for the current engine
	 * (plocal, u-area, kernel stack extension, ...)
	 *
	 * Note that these mappings must agree with those in setup_priv_maps().
	 *
	 * KVUENG note:
	 *
	 * The 80386 B1-step Errata #13 workaround requires the portion of
	 * the kernel stack used to return to user mode to have its page
	 * table entry set for user read access, but it does not actually
	 * require the user to be able to read the stack.
	 *
	 * Further, since the 80x86 does not allow separate page
	 * protections to be specified for user and supervisor mode, and
	 * since the kernel needs write permission to this area, we grant
	 * the user both read and write permission to the entire KVUENG.
	 *
	 * We rely on 80x86 segmentation limits to restrict the user from
	 * both reading and writing this area.
	 *
	 * KVPLOCAL note:
	 *
	 * User read and write permissions are also needed for floating-
	 * point emulator access to the floating-point state in l.fpe_kstate.
	 * This access will be via a separate USER_FP segment, which will not
	 * allow access to the rest of KVPLOCAL.
	 */

	pcreate_map(KVPLOCAL, (PL_PAGES * MMU_PAGESIZE),
		    &pmyengppriv->pp_local[0][0], PG_US | PG_RW | PG_V);
	pcreate_map(KVPLOCALMET, (PLMET_PAGES * PAGESIZE),
		    &pmyengppriv->pp_localmet[0][0],      PG_RW | PG_V);
	pcreate_map(KVENG_L2PT, MMU_PAGESIZE,
		    &pmyengppriv->pp_pmap[0][0],          PG_RW | PG_V);
	pcreate_map(KVENG_L1PT, MMU_PAGESIZE,
		    &pmyengppriv->pp_kl1pt[0][0],         PG_RW | PG_V);
	pcreate_map(KVUVWIN, MMU_PAGESIZE,
		    &pmyengppriv->pp_uvwin[0][0],         PG_RW | PG_V);
	pcreate_map(UVUVWIN, MMU_PAGESIZE,
		    &pmyengppriv->pp_uvwin[0][0],         PG_US | PG_V);
	pcreate_map(KVUENG, (USIZE * PAGESIZE),
		    &pmyengppriv->pp_ublock[0][0], PG_US | PG_RW | PG_V);
	/* KVUENG_REDZONE: per-engine kernel stack redzone; not mapped */


	/*
	 * Map various hardware addresses.
	 */

	pcreate_map(KVSYNC_POINT, MMU_PAGESIZE, (void *)PHYS_SYNC_POINT,
								PG_RW | PG_V);
	pcreate_map(KVLED, MMU_PAGESIZE, (void *)PHYS_LED, PG_RW | PG_V);
	pcreate_map(KVETC, MMU_PAGESIZE, (void *)PHYS_ETC, PG_RW | PG_V);
	pcreate_map(KVSLIC, MMU_PAGESIZE, (void *)PHYS_SLIC, PG_RW | PG_V);

	/*
	 * Map kernel text and CFG structure readonly
	 * (readonly is only effective on the 80486).
	 *
	 * We know that kernel text immediately follows the
	 * CFG structure, both in physical and in virtual space.
	 * Note this assumes no holes in physical memory in this range.
	 */

	vaddr = mmu_ptob(mmu_btop(KVCD_LOC));
	paddr = (paddr_t) mmu_ptob(mmu_btop(CD_LOC));
	msize = roundup((vaddr_t)&_etext - vaddr, MMU_PAGESIZE);
	pcreate_map(vaddr, msize, (void *)paddr, PG_V);

	/*
	 * Map kernel data and bss sections read/write.
	 *
	 * We know that kernel data and bss immediately follow the
	 * kernel text, both in physical and in virtual space.
	 * Note this assumes no holes in physical memory in this range.
	 */

	vaddr += msize;
	paddr += msize;
	msize = roundup((vaddr_t)&_end - vaddr, MMU_PAGESIZE);
	pcreate_map(vaddr, msize, (void *)paddr, PG_RW | PG_V);

	/*
	 * Map kernel symbol table (if present) readonly
	 * (readonly is only effective on the 80486).
	 *
	 * We know that the kernel symbol table immediately follows
	 * kernel data and bss, both in physical and in virtual space.
	 * Note this assumes no holes in physical memory in this range.
	 */

	if (symtabsize > 0) {
		vaddr += msize;
		paddr += msize;
		msize = roundup(symtabsize, MMU_PAGESIZE);
		pcreate_map(vaddr, msize, (void *)paddr, PG_V);
	}

	/*
	 * Map the set of per-engine pages for the current engine
	 * in a global kernel virtual address (so that it can
	 * be referenced by engine[].e_local).
	 *
	 * Remember the corresponding virtual address to later
	 * pass to sysinit().
	 */

	vaddr += msize;
	msize = mmu_ptob(pprivpages);
	pcreate_map(vaddr, msize, pmyengppriv, PG_RW | PG_V);

	myengppriv = (struct ppriv_pages *) vaddr; /* vaddr of my ppriv_pages */

	/*
	 * Start allocating virtual memory at the
	 * next available (page aligned) virtual address.
	 */

	vmemptr = roundup(vaddr + msize, MMU_PAGESIZE);

	/*
	 * Map system startup code and data for use by subsequent engine online.
	 *
	 * This must be mappped virtual == physical so that the first on-line
	 * for each processor can execute reset_code().
	 *
	 * We also need to map the system startup code and stack
	 * physical == virtual because we're using it right now!
	 *
	 * This physical == virtual mapping will be unmapped by selfinit().
	 *
	 * Note that, after every engine has been onlined once (and hence
	 * has executed reset_code()) we could, in theory, reclaim the
	 * physical memory for the startup code and data as well as the
	 * level 2 page table which pcreate_map() allocates here to map them.
	 * (This reclaim would be done in unmap_page0().)  However, we don't
	 * bother doing this.  So we lose 3 pages.
	 */
	/*
	 * Map CD_LOC physical == virtual and writable until cfg_relocate()
	 * has been called (by sysinit()) to rewrite the pointers in the
	 * CFG table from physical to virtual pointers.
	 *
	 * This physical == virtual mapping will be unmapped by selfinit().
	 */

	pcreate_map((vaddr_t)0, (paddr_t)CD_LOC + CD_SIZE - (paddr_t)0,
						(void *)0, PG_RW | PG_V);

	pmet = (struct mets *) phys_palloc(MET_PAGES * (PAGESIZE/MMU_PAGESIZE));
	pcreate_map(KVMET, (MET_PAGES * PAGESIZE), 
						(void *)pmet,  PG_RW | PG_V);

	paddr = phys_palloc(SYSDAT_PAGES * (PAGESIZE/MMU_PAGESIZE));
	pcreate_map(KVSYSDAT, (SYSDAT_PAGES * PAGESIZE), paddr, PG_RW | PG_V);
}

/*
 * STATIC void
 * pcreate_map(vaddr_t vaddr, uint_t size, void *paddr, int prot)
 *
 *	Create a page table for a given virtual address range.
 *	Allocate memory to hold the page table entries if needed.
 *
 * Calling/Exit State:
 *
 *	This runs in physical rather than virtual mode.
 *
 *	vaddr is the starting (page-aligned) virtual address to map.
 *	size is the size in bytes of the virtual range to map;
 *		it must be an integral number of pages.
 *	paddr is the starting (page-aligned) physical address to map
 *		at vaddr.  "size" bytes of physically contiguous
 *		memory starting at "paddr" is mapped.
 *	prot are the i386 hardware page protections to use for the mapping.
 *
 *	Pages are MMU_PAGESIZE bytes long.
 */

STATIC void
pcreate_map(vaddr_t vaddr, uint_t size, void *paddr, int prot)
{
	pte_t	*ptr;
	vaddr_t	va;
	vaddr_t	ea;
	paddr_t	pa;

	/*
	 * Assert addresses and size are multiples of pagesize.
	 */

	PASSERT(PAGOFF(vaddr) == 0, "pcreate_map");
	PASSERT(PAGOFF(size)  == 0, "pcreate_map");
	PASSERT(PAGOFF(paddr) == 0, "pcreate_map");

	va = vaddr;
	pa = (paddr_t) paddr;
	ea = vaddr + size;

	while (va != ea) {

		/*
		 * Allocate memory for level 2 page table (if necessary).
		 */

		ptr = pkl1pt + ptnum(va);
		if (ptr->pg_pte == 0) {
			paddr_t tmppa = phys_palloc(1);
			ptr->pg_pte = mkpte(PG_RW | PG_V, pfnum(tmppa));

			if (KADDR(va)) {
				/*
				 * Level 2 page tables for kernel addrs
				 * (top 1 G) are themselves virtually
				 * mapped in KL2PTES[].
				 */
				PASSERT(pkl2ptes[kl2ptesndx(va)].pg_pte == 0,
								"pcreate_map");

				pkl2ptes[kl2ptesndx(va)].pg_pte =
					mkpte(PG_RW | PG_V, pfnum(tmppa));
			}
		}

		/*
		 * Fill out a level 2 page table entry.
		 */

		ptr = (pte_t *)(ptr->pg_pte & MMU_PAGEMASK);
		ptr += pgndx((uint_t)va);
		ptr->pg_pte = mkpte(prot, pfnum(pa));

		va += MMU_PAGESIZE; 
		pa += MMU_PAGESIZE;
	}
}

/*
 * STATIC paddr_t
 * phys_palloc(uint_t npages)
 *
 *	Return the physical address of "npages" contiguous zeroed pages.
 *
 * Calling/Exit State:
 *
 *	Returns the starting (page-aligned) physical address of
 *	"npages" of physically contiguous, zeroed pages of memory.
 *	Pages are MMU_PAGESIZE bytes long.
 *
 * Description:
 *
 *	We skip over holes in physical memory, however, the range of
 *	memory we allocate from during this part of system initialization
 *	never has any holes in it.
 */

STATIC paddr_t
phys_palloc(uint_t npages)
{
	uint_t i;
	paddr_t ret_pmemptr;

	/* Assert pmemptr is page aligned */
	PASSERT(PAGOFF(pmemptr) == 0, "phys_palloc");

	/*
	 * Find the next available range of physically contiguous
	 * pages that is large enough to satisfy the requested size.
	 * We skip over (and lose forever) any intervening fragments
	 * which are not big enough.
	 */

again:	ret_pmemptr = pmemptr;

	for (i = 1; i <= npages; i++) {

		/* Assert we haven't exhausted physical memory */
		PASSERT(pmemptr != (paddr_t) 0, "phys_palloc");

		if (!phys_hwpage_exists(pfnum(pmemptr))) {
			pmemptr += MMU_PAGESIZE;
			goto again;
		}

		pmemptr += MMU_PAGESIZE;
	}
		
	/*
	 * Zero the allocated pages.
	 *
	 * (Can't call bzero() since it isn't virtually mapped yet.)
	 */

	for (i = 0; i < (npages * MMU_PAGESIZE); i++)
		((char *)ret_pmemptr)[i] = '\0';

	return (ret_pmemptr);
}

/*
 * int
 * phys_hwpage_exists(int pg)
 *
 *	Return non-zero iff MMU page "pg" exists in physical memory.
 *
 * Calling/Exit State:
 *
 *	pg refers to an MMU_PAGESIZE sized page.
 *
 *	Returns 0 if address space given by page number is not
 *	backed up by physical memory, otherwise returns 1.
 *
 *	Works in physical mode, before virtual mapping is enabled
 *	or while physical memory is virtually mapped:  virtual == physical.
 *	(The physical address it needs is CD_LOC.)
 */

STATIC int
phys_hwpage_exists(int pg)
{
	pg /= mmu_btop(MC_CLICK);
	return((pg >= CD_LOC->c_mmap_size) ? 0 : MC_MMAP(pg, CD_LOC));
}

/*
 * STATIC void
 * phys_assfail(const char *assert_expr, const char *s)
 *
 *	Print a failure message if an assert fails in physical mode.
 *
 * Calling/Exit State:
 *
 *	`assert_expr' is a string containing the assertion expression
 *		which failed.
 *	`s' is a string, typically containing the name of the function
 *		where the assertion failed.
 *
 *	No return from this procedure, it goes into a spin loop
 *	after calling phys_puts to print its argument on the console.
 */

STATIC int
phys_assfail(const char *assert_expr, const char *s)
{
	phys_puts("\nPANIC: ");
	phys_puts(s);
	phys_puts(": PASSERT(");
	phys_puts(assert_expr);
	phys_puts(") failed.\nLooping forever.\n");
	/* infinite spin loop */
	for (;;)
		continue;
	/*NOTREACHED*/
}

/*
 * STATIC void
 * phys_puts(const char *s)
 *
 *	Print a string argument on the console running in physical mode.
 *
 * Calling/Exit State:
 *
 *	No return value.
 */

STATIC void
phys_puts(const char *s)
{
	int console_id = CD_LOC->c_cons->cd_slic;
	volatile struct cpuslic *sl = (struct cpuslic *)PHYS_SLIC;
	int stat;

	while (*s != '\0') {
		if (*s == '\n')
			phys_puts("\r");
		sl->sl_dest = (unchar)console_id;
		sl->sl_smessage = (unchar)*s++;
		do {
			sl->sl_cmd_stat = (unchar)(SL_MINTR | PUTCHAR_BIN);
			do { /* nothing */ }
			while ((stat = sl->sl_cmd_stat) & SL_BUSY);
		} while ((stat & SL_OK) == 0);
	}
}
