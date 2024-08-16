/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:svc/mmu.c	1.49"
#ident	"$Header: $"

/*
 * Build page tables when booting the system.
 *
 * These routines are executed when physical, not virtual, memory
 * addressing is in effect.
 */

#include <io/kd/kd.h>
#include <mem/immu.h>
#include <mem/tuneable.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <svc/bootinfo.h>
#include <svc/creg.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>

extern void psm_phys_puts(const char *);
extern boolean_t pse_supported(void);
extern boolean_t phystokvmem;
extern uint_t pse_physmem;
extern char stext[];	/* stext = beginning of kernel text */
extern char sbss[];	/* sbss = start of kernel bss */
extern char _etext[];	/* _etext = last byte of kernel text plus 1 */
extern char _edata[];	/* _edata = last byte of kernel data plus 1 */
extern char _end[];	/* _end = last byte of kernel BSS plus 1 */

#ifndef NO_RDMA

/*
 * AT systems with restricted DMA preferentially allocate non-DMAable
 * memory to kernel text, data, page tables, calloc()ed memory, etc.
 * If necessary, data loaded by the bootstrap is copied in order to
 * strongly implement this preference.
 */

#define P_DMA_PFN(pfn)	(p_dmalimit == 0 || (pfn) < p_dmalimit)
#define P_DMA_BYTE(b)	P_DMA_PFN(btop(b))
#define P_DMALIMIT	ptob(p_dmalimit)

STATIC paddr_t		p_dmalimit = 0;	/* copy of tune.t_dmalimit */
STATIC boolean_t	p_phystokvmem = 0;  /* copy of phystokvmem */
STATIC boolean_t 	p_rdma_enabled = B_FALSE;
STATIC uint_t		p_pse_physmem = 0;	/* copy of pse_physmem */

STATIC void add_unused_udf(paddr_t, ulong_t, ushort_t);

typedef struct {
	char	p_contents[MMU_PAGESIZE];
} page_contents_t;

#define phys_ppcopy(src, dest)	\
	(*((page_contents_t *)(dest)) = *((page_contents_t *)(src)))

#else /* NO_RDMA */

#define add_unused_fragment	add_unused_udf

#endif /* NO_RDMA */

/*
 * Physical address of the level 1 page table of the boot engine.
 * Used by kernel crashdump analyzers which assume that the physical
 * address of this variable is equal to the symbol table value.
 *
 * Note: This variable must be declared in a file that will be loaded V=P.
 */

paddr_t crash_kl1pt = (paddr_t)0;
/*
 * for platform other than AT-MP, it is the responsibility of psm_pstart
 * to initialize bootinfo_loc.
 * it is set to 0x600 so that the code will work for AT-MP.
 */
paddr_t bootinfo_loc = (paddr_t)BOOTINFO_LOC;

/*
 * The following values are set here and exported (via _start())
 * to sysinit():
 *
 * memNOTused	- Chunks of physical memory that are NOT used for 
 *		  kernel text and data. 
 *
 * memNOTusedNDMA
 *		- Chunks of physical memory that are NOT used for
 *		  kernel text and data and are not DMAable.
 *
 * vmemptr	- virtual address of next unused virtual memory.
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

struct	unusedmem  *memNOTused = (struct unusedmem *)0;
#ifndef NO_RDMA
struct  unusedmem  *memNOTusedNDMA = (struct unusedmem *)0;
#endif /* NO_RDMA */

/*
 * flags for phys_palloc2
 */
#define P_PREFER_DMA	0
#define P_PREFER_NDMA	1

struct  unusedmem   unused_array[3 * B_MAXMEMA + 1] = { 0 };
STATIC int p_unused = 0;
STATIC int p_prefer_l2pte = P_PREFER_NDMA;

paddr_t physkv_extent_p = 0;
paddr_t physkv_start_p = 0;

vaddr_t vmemptr = 0;			/* next unused virtual memory */
struct ppriv_pages *myengppriv = (struct ppriv_pages *)0;
					/* virt addr of my ppriv_pages */

paddr_t	phys_palloc_dma(size_t);
#ifndef NO_RDMA
paddr_t	phys_palloc_nodma(size_t);
#endif /* NO_RDMA */

STATIC	pte_t *pkl1pt = (pte_t *)0;	/* kernel level 1 page table */
STATIC	pte_t *pkl2ptes = (pte_t *)0;	/* kernel level 2 page table to map
					 * all kernel level 2 page tables */

#define pbootinfo	(*((struct bootinfo *)bootinfo_loc))

STATIC	void	build_table(void);
STATIC	void	pcreate_map(vaddr_t, uint_t, paddr_t, int);

paddr_t	phys_palloc2(uint_t, uint_t);
#ifndef NO_RDMA
STATIC	paddr_t	find_phys_addr(vaddr_t);
#endif
STATIC	void	find_unused_mem(void);
STATIC	void	add_unused_fragment(paddr_t, ulong_t, ushort_t);
STATIC	void	pse_palloc_init(void);
STATIC	paddr_t	pse_alloc_kernel(uint_t, long);

#define phys_palloc(n)		phys_palloc2(n, P_PREFER_NDMA);

/*
 * Return value from phys_palloc_dma() and phys_palloc_nodma()
 */
#define PALLOC_FAIL	((paddr_t)0xFFFFFFFF)

/*
 * Physical mode debugging tools
 */

#if	defined(PHYS_DEBUG) || defined(lint)

#define str(s)  #s
#define xstr(s) str(s)

/*
 * Physical mode assert
 */
#define	PHYS_ASSERT(EX)	((void)((EX) || phys_assfail(#EX, __FILE__, \
				xstr(__LINE__))))

/*
 * Physical mode print
 */
#define	PHYS_PRINT	phys_printf

/*
 * Delay following each print
 */
#define	PHYS_IDLE	0x04000000

/*
 * MACRO char
 * digtohex(int dig)
 *	Return hex character code representing integer value from 0 to 15
 */
#define	digtohex(dig)	(((0 <= (dig)) && ((dig) <= 9)) ? \
				'0' + (dig) : 'A' + ((dig) - 10))

/*
 * Maximum number of hex digits needed to represent an unsigned long integer
 */
#define	NHEXDIG		8

/*
 * STATIC void
 * phys_printhex(uint_t)
 *	Print an unsigned integer in hex during physical mode startup
 *
 * Calling/Exit State:
 *	Must be in physical mode.
 */
STATIC void
phys_printhex(uint_t x)
{
	char buf[NHEXDIG + 1];
	int i;
	uint_t val;

	val = x;
	for (i = 0 ; i < NHEXDIG ; ++i) {
		buf[(NHEXDIG - 1) - i] = digtohex(val & 0x0F);
		val >>= 4;
	}
	buf[NHEXDIG] = 0;
	psm_phys_puts(buf);
}

/*
 * STATIC void
 * phys_printf(char *fmt, ...)
 *	Print formatted I/O during physical mode startup
 *
 *
 * Description:
 *	Provides simple printf-like formatted I/O.  Formats
 *	understood are:
 *		%x:	print 8 digit hex number
 *
 * Calling/Exit State:
 *	Must be in physical mode.
 */
void
phys_printf(char *fmt, ...)
{
	int i;
	char c, *pc, *pb;
	VA_LIST ap;

	pc = pb = fmt;
	VA_START(ap, fmt);
	while (*pc != '\0') {
		while ((*pc != '%') && (*pc != '\0') && (*pc != '\n'))
			++pc;
		if (pc > pb) {
			c = *pc;
			*pc = '\0';
			psm_phys_puts(pb);
			*pc = c;
		}
		if (*pc == '%') {
			if ((*++pc == 'x') || (*pc == 'X')) {
				phys_printhex(VA_ARG(ap, int));
				++pc;
			}
		} else if (*pc == '\n') {
			psm_phys_puts("\n\r");
			++pc;
		}
		pb = pc;
	}
	for (i = 0 ; i < PHYS_IDLE ; ++i)
		;
}

/*
 * STATIC void
 * phys_assfail(const char *assert_expr, const char *file, const char *line)
 *	Print a failure message if an assert fails in physical mode.
 *
 * Calling/Exit State:
 *	assert_expr is the failed assertion.
 *	file and line are the source file and line number where the
 *		assertion is located.
 *
 *	No return from this procedure, it goes into a spin loop
 *	after calling psm_phys_puts to print its argument on the console.
 */
int
phys_assfail(const char *assert_expr, const char *file, const char *line)
{
	psm_phys_puts("\r\nPANIC: assertion failed: ");
	psm_phys_puts(assert_expr);
	psm_phys_puts(", file: ");
	psm_phys_puts(file);
	psm_phys_puts(", line: ");
	psm_phys_puts(line);
	psm_phys_puts("\r\nLooping forever.\r\n");
	/* infinite spin loop */
	for (;;)
		continue;
	/*NOTREACHED*/
}

#else	/* PHYS_DEBUG || lint */

/*
 * The following definitions have the effect of turning PHYS_ASSERTs
 * and PHYS_PRINTs into no-ops if PHYS_DEBUG is undefined.  PHYS_PRINT
 * in particular has a couple of tricks in it.  It consists of 
 * a conditional expression, which always evaluates to (void)0.
 * The third operand of the conditional expression will be the
 * argument list to PHYS_PRINT cast to a void type.  For instance,
 * consider what happens with the following when PHYS_DEBUG is
 * undefined and PHYS_PRINT is defined as below:
 *
 *	PHYS_PRINT("x = 0x%x\n", x);
 *
 * becomes
 *
 *	1 ? (void)0 : (void) ("x = 0x%x\n", x);
 *
 * which ends up evaluating to just
 *
 *		(void)0
 *
 * which is a no-op.
 */
#define	PHYS_ASSERT(EX) 	((void)0)
#define	PHYS_PRINT		1 ? (void)0 : (void)

#endif	/* PHYS_DEBUG || lint */

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
 *	memNOTused - linked list of all the physical memory segments that
 *		       are not used by kernel text and data.
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
	/*
	 * Fill out the the memNOTused list with all the physical
	 * memory segment that are not used by kernel text and data.
	 */
	find_unused_mem();

	/*
	 * Build kernel page tables.
	 * Start allocating physical memory at the first page past
	 * the end of the kernel (including symbol table if present).
	 */

	build_table();
}

/*
 * void
 * enable_paging(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
enable_paging(void)
{
	ulong_t cr0;

	/*
	 * Load CR3 with kernel level 1 page table root
	 * and enable paging.
	 */

	WRITE_PTROOT((ulong_t)pkl1pt);
	if (pse_supported())
		_wcr4(_cr4() | CR4_PSE);
	cr0 = READ_MSW();
	cr0 |= CR0_PG;
	WRITE_MSW(cr0);
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
 *	memNOTused - linked list of all the physical memory segments that
 *		       are not used by kernel text and data.
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
	vaddr_t vaddr, va;
	paddr_t paddr, src, kpaddr;
	uint_t msize;
	uint_t size;
	pte_t *ptr;
	uint_t pprivpages;
	struct ppriv_pages *pmyengppriv;    /* phys addr of my ppriv_pages */
	struct mets *pmet;			/* phys addr of metrics */
	int i, j, prot, npages;
	paddr_t mapextent;
	long *end_addr;
	int data_flags = P_PREFER_NDMA;
	boolean_t pse_map;
	typedef struct {
		vaddr_t pmt_vaddr;
		uint_t	pmt_size;
		paddr_t	pmt_paddr;
		int	pmt_prot;
	} pcreate_map_table_t;
	pcreate_map_table_t *tp;
	static pcreate_map_table_t pcreate_map_table[] = {
		/*
		 * Allocate and map the per-engine pages for the current
		 * engine (plocal, u-area, kernel stack extension, ...)
		 *
		 * Note that these mappings must agree with those in
		 * setup_priv_maps().
		 *
		 * KVUENG note:
		 *
		 * The 80386 B1-step Errata #13 workaround requires the
		 * portion of the kernel stack used to return to user mode to
		 * have its page table entry set for user read access, but it
		 * does not actually require the user to be able to read the
		 * stack.
		 *
		 * Further, since the 80x86 does not allow separate page
		 * protections to be specified for user and supervisor mode,
		 * and since the kernel needs write permission to this area,
		 * we grant the user both read and write permission to the
		 * entire KVUENG.
		 *
		 * We rely on 80x86 segmentation limits to restrict the
		 * user from both reading and writing this area.
		 *
		 * KVPLOCAL note:
		 *
		 * User read and write permissions are also needed for
		 * floating- point emulator access to the floating-point
		 * state in l.fpe_kstate. This access will be via a separate
		 * USER_FP segment, which will not allow access to the rest
		 * of KVPLOCAL.
		 */

		/* 0 */ {KVPLOCAL, (PL_PAGES * MMU_PAGESIZE),
					0, PG_US | PG_RW | PG_V},
		/* 1 */ {KVPLOCALMET, (PLMET_PAGES * PAGESIZE), 0,PG_RW | PG_V},
		/* 2 */ {KVENG_L2PT, MMU_PAGESIZE, 0,PG_RW | PG_V},
		/* 3 */ {KVENG_L1PT, MMU_PAGESIZE, 0,PG_RW | PG_V},
		/* 4 */ {KVUVWIN, MMU_PAGESIZE, 0,PG_RW | PG_V},
		/* 5 */ {UVUVWIN, MMU_PAGESIZE, 0,PG_US | PG_V},
		/* 6 */ {KVUENG, (USIZE * PAGESIZE), 0, PG_US | PG_RW | PG_V},
		/* 7 */ {KVUENG_EXT, (KSE_PAGES * MMU_PAGESIZE),
					0, PG_RW | PG_V},
		/*
		 * KVUENG_REDZONE: per-engine kernel stack redzone; not mapped
		 */

		/*
		 * Map various hardware addresses.
		 */

		/* 8 */ {KVDISP_MONO, NP_DISP_MONO * MMU_PAGESIZE,
					MONO_BASE, PG_RW | PG_V},
		/* 9 */ {KVDISP_COLOR, NP_DISP_COLOR * MMU_PAGESIZE,
					COLOR_BASE, PG_RW | PG_V},

		/*
		 * Map the first physical page physical == virtual and
		 * writable. This page contains the current stack. This
		 * physical == virtual mapping will be unmapped by
		 * selfinit().
		 */
		/* 10 */ {0, MMU_PAGESIZE, 0, PG_RW | PG_V},

		/*
		 * Set up a permanent mapping for the bootinfo structure.
		 */
		/* 11 */ {KVBOOTINFO, MMU_PAGESIZE, 0, PG_RW | PG_V},

		/*
		 * Set up a mapping for page 0, for communicating with
		 * the BIOS ROM.
		 */
		/* 12 */ {KVPAGE0, MMU_PAGESIZE, 0, PG_RW | PG_V},

		{0, 0, 0, 0}
	};

	pse_map = B_FALSE;

	if (pse_supported() && !p_phystokvmem) {
		pse_palloc_init();
		npages = 0;
		for (i = 2; i < pbootinfo.memusedcnt; i++)
			npages += mmu_btopr(pbootinfo.memused[i].extent);
		npages += mmu_btopr((vaddr_t)_end - (vaddr_t)sbss);

		kpaddr = pse_alloc_kernel((vaddr_t)stext & PSE_PAGEOFFSET,
			mmu_ptob(npages));

		if (kpaddr != PALLOC_FAIL)
			pse_map = B_TRUE;

	}


	/*
	 * The per-engine pages must fit in a single level 2 page table.
	 */
	PHYS_ASSERT(KVLAST_PLAT >= KVPER_ENG);

	PHYS_ASSERT(stext >= KL2PTES + KL2PTES_SIZE);

	/*
	 * Allocate the set of per-engine pages for the current engine.
	 * The allocation for the rest of the engines happens later
	 * in sysinit().
	 */

	pprivpages = mmu_btopr(sizeof(struct ppriv_pages));

	pmyengppriv = (struct ppriv_pages *) phys_palloc(pprivpages);

	/*
	 * Set up temporary global pointers to the level 1 & 2 page tables
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
	PHYS_ASSERT(pgndx(KL2PTES) == 0);
	/* Assert KL2PTES and KL2PTESPTES in same page table */
	PHYS_ASSERT(ptnum(KL2PTESPTES) == ptnum(KL2PTES));

	pkl2ptes = (pte_t *)phys_palloc(1);
	pkl1pt[ptnum(KL2PTES)].pg_pte =
		pkl2ptes[pgndx(KL2PTESPTES)].pg_pte =
				mkpte(PG_RW | PG_V, pfnum(pkl2ptes));

	/*
	 * fill in pcreate_map_table entries
	 */

	pcreate_map_table[0].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_local[0][0];
	pcreate_map_table[1].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_localmet[0][0];
	pcreate_map_table[2].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_pmap[0][0];
	pcreate_map_table[3].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_kl1pt[0][0];
	pcreate_map_table[4].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_uvwin[0][0];
	pcreate_map_table[5].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_uvwin[0][0];
	pcreate_map_table[6].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_ublock[0][0];
	pcreate_map_table[7].pmt_paddr =
				(paddr_t)&pmyengppriv->pp_uengkse[0][0];
	pcreate_map_table[10].pmt_vaddr = (vaddr_t)pbootinfo.memused[0].base;
	pcreate_map_table[10].pmt_paddr = pbootinfo.memused[0].base;
	pcreate_map_table[10].pmt_size = ((bootinfo_loc >> MMU_PAGESHIFT)+1) * MMU_PAGESIZE;
	pcreate_map_table[11].pmt_paddr = (bootinfo_loc & MMU_PAGEMASK);

	/*
	 * do pcreate_map()s, driven by the table
	 */
	for (tp = pcreate_map_table; tp->pmt_size != 0; ++tp)
		pcreate_map(tp->pmt_vaddr, tp->pmt_size,
			    tp->pmt_paddr, tp->pmt_prot);

	/*
	 * Find mapping range for phystokv compatibility 
	 * (see _Compat_phystokv).
	 */
	physkv_start_p = pbootinfo.memavail[0].base;
	physkv_extent_p = 0;

	for (i = 0; i < pbootinfo.memavailcnt; i++) {
		mapextent = pbootinfo.memavail[i].base + 
				pbootinfo.memavail[i].extent;
		if (physkv_extent_p < mapextent)
			physkv_extent_p = mapextent;
	}

	/*
	 * We need to map the system startup code
	 * physical == virtual because we're using it right now!
	 * This physical == virtual mapping will be unmapped by selfinit().
	 * Some of this is text, some is data; just make the whole thing
	 * writeable.
	 */
	pcreate_map(pbootinfo.memused[1].base, pbootinfo.memused[1].extent,
		    pbootinfo.memused[1].base, PG_RW | PG_V);

	/*
	 * Map kernel text, data, bss and symbol table (if present) sections
	 * according to the memused array in booinfo passed from boot program.
	 * Text segment is mapped readonly, data segments (including 
	 * symbol table if present) are mapped read/write.
	 * (readonly is only effective on the 80486).
	 */

#ifndef NO_RDMA
	/*
	 * In the p_phystokvmem case, place data and bss segments in
	 * DMAable memory for the benefit of 4.2 binary HBA drivers.
	 */
	if (p_phystokvmem)
		data_flags = P_PREFER_DMA;
#endif /* NO_RDMA */

	vaddr = (vaddr_t)stext;
	paddr = kpaddr;
	for (i = 2, j=0; i <= pbootinfo.memusedcnt; i++) {
		if (vaddr == (vaddr_t)sbss)  {
			msize = roundup((vaddr_t)_end - (vaddr_t)sbss,
				MMU_PAGESIZE);
			if (pse_map) {
				end_addr = (long *)(paddr + msize);
				do {
					*--end_addr = 0;
				} while (end_addr != (long *)paddr);
				pcreate_map((vaddr_t)sbss, msize, paddr,
					PG_RW | PG_V);
				vaddr += msize;
				paddr += msize;
			} else {
				/*
				 * Allocate physical pages for bss. These
				 * pages don't have to be contiguous, so
				 * they are allocated one page at a time
				 * to avoid wasting memory.
				 */
				size = 0;
				while (size < msize) {
					paddr = phys_palloc2(1, data_flags);
					pcreate_map(vaddr, MMU_PAGESIZE, paddr,
						PG_RW | PG_V);
					vaddr += MMU_PAGESIZE;
					size += MMU_PAGESIZE;
				}
			}
		}
		if (i >= pbootinfo.memusedcnt)
			break;
		src = pbootinfo.memused[i].base;
		msize = pbootinfo.memused[i].extent;
		prot = (pbootinfo.memused[i].flags & B_MEM_KTEXT) ?
				PG_V : (PG_RW | PG_V);
		if (( pbootinfo.memused[i].flags & B_MEM_KRDATA ) &&
			( pbootinfo.kd[j].paddr == src ))
				pbootinfo.kd[j++].vaddr = vaddr;
		if (pse_map) {
			/*
			 * setup the mapping for what we're about to copy
			 */
			pcreate_map(vaddr, roundup(msize, MMU_PAGESIZE), paddr,
				prot);
	
			/*
			 * Copy the memory a page at a time
			 */
			for (size = 0 ; size < msize ; size += MMU_PAGESIZE) {
				phys_ppcopy(src, paddr);
				src += MMU_PAGESIZE;
				vaddr += MMU_PAGESIZE;
				paddr += MMU_PAGESIZE;
			}
			/*
			 * Now, return the source of the copy to the
			 * free list.
			 */
			add_unused_fragment(pbootinfo.memused[i].base, msize,
				pbootinfo.memused[i].flags);
		/*
		 * Note: Once B_MEM_MEMFSROOT support is added, make
		 * sure pse_map case handles it properly.
		 */
#ifndef NO_RDMA
		} else if (p_rdma_enabled &&
					!((prot & PG_RW) && p_phystokvmem) &&
					!(pbootinfo.memused[i].flags &
						B_MEM_MEMFSROOT)) {
			/*
			 * If copying data, then prefer DMAable level 2
			 * page tables. This is done to preserve the
			 * physical contiguity of the symbol table, and
			 * thus to avoid additional page pool chunks.
			 */
			p_prefer_l2pte = (prot & PG_RW) ?
					P_PREFER_DMA : P_PREFER_NDMA;

			/*
			 * Copy the memory a page at a time, hopefully into
			 * non-DMAable pages.
			 */
			for (size = 0; size < msize; size += MMU_PAGESIZE) {
				paddr = phys_palloc(1);
				phys_ppcopy(src, paddr);
				pcreate_map(vaddr, MMU_PAGESIZE, paddr,
					    prot);
				src += MMU_PAGESIZE;
				vaddr += MMU_PAGESIZE;
			}

			/*
			 * Now, return the source of the copy to the
			 * free list.
			 */
			add_unused_fragment(pbootinfo.memused[i].base,
					    msize, pbootinfo.memused[i].flags);
#endif /* NO_RDMA */
		} else {
			pcreate_map(vaddr, msize, src, prot);
			vaddr += msize;
		}
	}

	if (pse_map) {
		/*
		 * convert standard page mappings to PSE page mappings
		 */
		paddr = kpaddr & PSE_PAGEMASK;
		va = (vaddr_t)stext & PSE_PAGEMASK;
		while (paddr < (kpaddr + mmu_ptob(npages))) {
			pkl1pt[ptnum(va)].pg_pte = pse_mkpte(PG_RW | PG_V,
				pfnum(paddr));
			va += PSE_PAGESIZE;
			paddr += PSE_PAGESIZE;
		}

		/*
		 * roundup virtual address to the next PSE_PAGESIZE boundary
		 */
		vaddr = roundup(vaddr, PSE_PAGESIZE);
	}

	/*
	 * Map the set of per-engine pages for the current engine
	 * in a global kernel virtual address (so that it can
	 * be referenced by engine[].e_local).
	 *
	 * Remember the corresponding virtual address to later
	 * pass to sysinit().
	 */

	msize = mmu_ptob(pprivpages);
	pcreate_map(vaddr, msize, (paddr_t)pmyengppriv, PG_RW | PG_V);

	myengppriv = (struct ppriv_pages *)vaddr; /* vaddr of my ppriv_pages */
	vaddr += msize;

	/*
	 * Start allocating virtual memory at the
	 * next available (page aligned) virtual address.
	 */

	vmemptr = roundup(vaddr, MMU_PAGESIZE);
	
	pmet = (struct mets *) phys_palloc(MET_PAGES * (PAGESIZE/MMU_PAGESIZE));
	pcreate_map(KVMET, (MET_PAGES * PAGESIZE), (paddr_t)pmet,
		    PG_RW | PG_V);
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
pcreate_map(vaddr_t vaddr, uint_t size, paddr_t paddr, int prot)
{
	pte_t	*ptr;
	vaddr_t	va;
	vaddr_t	ea;

	/*
	 * Assert addresses and size are multiples of pagesize.
	 */

	PHYS_ASSERT(PAGOFF(vaddr) == 0);
	PHYS_ASSERT(PAGOFF(size)  == 0);
	PHYS_ASSERT(PAGOFF(paddr) == 0);

	va = vaddr;
	ea = vaddr + size;

	while (va != ea) {
		/*
		 * Allocate memory for level 2 page table (if necessary).
		 *
		 * We prefer DMA pages for this purpose so that when the
		 * symbols table is copied to non-DMAable memory, it is
		 * not fragmented into two page pool chunks by page
		 * table allocation.
		 */

		ptr = pkl1pt + ptnum(va);
		if (ptr->pg_pte == 0) {
			paddr_t tmppa = phys_palloc2(1, p_prefer_l2pte);
			ptr->pg_pte = mkpte(PG_RW | PG_V, pfnum(tmppa));

			if (KADDR(va)) {
				/*
				 * Level 2 page tables for kernel addrs
				 * (top 1 G) are themselves virtually
				 * mapped in KL2PTES[].
				 */
				PHYS_ASSERT(pkl2ptes[kl2ptesndx(va)].pg_pte ==
					0);

				pkl2ptes[kl2ptesndx(va)].pg_pte =
					mkpte(PG_RW | PG_V, pfnum(tmppa));
			}
		}

		/*
		 * Fill out a level 2 page table entry.
		 */

		ptr = (pte_t *)(ptr->pg_pte & MMU_PAGEMASK);
		ptr += pgndx((uint_t)va);
		ptr->pg_pte = mkpte(prot, pfnum(paddr));

		va += MMU_PAGESIZE; 
		paddr += MMU_PAGESIZE;
	}
}

/*
 * paddr_t
 * phys_palloc2(uint_t npages, uint_t flags)
 *
 *	Return the physical address of ``npages'' of contiguous
 *	zeroed memory.
 *
 * Calling/Exit State:
 *
 *	Returns the starting (page-aligned) physical address of
 *	"npages" of physically contiguous, zeroed pages of memory.
 *	Pages are MMU_PAGESIZE bytes long.
 *
 * Description:
 *
 *	The more typical caller will first try for non-DMAable memory,
 *	then DMAable memory. However, if P_PREFER_DMA is specified by
 *	the flags, then the search order is reversed.
 */

paddr_t
phys_palloc2(uint_t npages, uint_t flags)
{
	paddr_t ret_paddr = PALLOC_FAIL;
	long *end_addr;
	size_t size = npages * MMU_PAGESIZE;

	PHYS_ASSERT(npages != 0);

#ifndef NO_RDMA

	if (flags == P_PREFER_DMA)
		ret_paddr = phys_palloc_dma(size);

	if (ret_paddr == PALLOC_FAIL)
		ret_paddr = phys_palloc_nodma(size);

#endif /* NO_RDMA */

	if (ret_paddr == PALLOC_FAIL)
		ret_paddr = phys_palloc_dma(size);

	PHYS_ASSERT(ret_paddr != PALLOC_FAIL);

	/*
	 * Zero the allocated pages.
	 * (Can't call bzero() since it isn't virtually mapped yet.)
	 */
	end_addr = (long *)(ret_paddr + size);
	do {
		*(--end_addr) = 0;
	} while (end_addr != (long *)ret_paddr);

	return ret_paddr;
}

/*
 * paddr_t
 * phys_palloc_dma(size_t size)
 *
 *	Return the physical address of size bytes of contiguous pages.
 *
 * Calling/Exit State:
 *
 *	Allocates from the memNOTused list of memory (DMAable).
 *
 *	Returns the starting (page-aligned) physical address of size
 *	bytes of physically contiguous pages of memory.
 *	Pages are MMU_PAGESIZE bytes long.
 *
 *	On failure, PALLOC_FAIL is returned.
 */

paddr_t
phys_palloc_dma(size_t size)
{
	paddr_t ret_paddr;
	struct unusedmem *mnup, **mnupp, **fmnupp;
	struct unusedmem *fmnup = NULL;

	/*
	 * Find the smallest available range of physically contiguous
	 * pages that is large enough to satisfy the requested size.
	 */
	mnupp = &memNOTused;
	while ((mnup = *mnupp) != NULL) {
		if (mnup->extent >= size &&
		    (fmnup == NULL || mnup->extent < fmnup->extent)) {
			fmnup = mnup;
			fmnupp = mnupp;
		}
		mnupp = &mnup->next;
	}

	if (fmnup == NULL)
		return PALLOC_FAIL;

	ret_paddr = fmnup->base;
	fmnup->base += size;
	if ((fmnup->extent -= size) == 0)
		*fmnupp = fmnup->next;

	return ret_paddr;
}

#ifndef NO_RDMA
/*
 * paddr_t
 * phys_palloc_nodma(size_t size)
 *
 *	Return the physical address of "npages" contiguous zeroed pages.
 *
 * Calling/Exit State:
 *
 *	Allocates from the memNOTusedNDMA list of memory (non-DMAable).
 *
 *	Returns the starting (page-aligned) physical address of
 *	size bytes of physically contiguous pages of memory.
 *	Pages are MMU_PAGESIZE bytes long.
 *
 *	On failure, PALLOC_FAIL is returned.
 */

paddr_t
phys_palloc_nodma(size_t size)
{
	struct unusedmem *mnup, **mnupp, **fmnupp;
	struct unusedmem *fmnup = NULL;

	/*
	 * Find the smallest available range of physically contiguous
	 * pages that is large enough to satisfy the requested size.
	 */
	mnupp = &memNOTusedNDMA;
	while ((mnup = *mnupp) != NULL) {
		if (mnup->extent >= size &&
		    (fmnup == NULL || mnup->extent <= fmnup->extent)) {
			fmnup = mnup;
			fmnupp = mnupp;
		}
		mnupp = &mnup->next;
	}

	if (fmnup == NULL)
		return PALLOC_FAIL;

	if ((fmnup->extent -= size) == 0)
		*fmnupp = fmnup->next;

	return fmnup->base + fmnup->extent;
}

/*
 * STATIC paddr_t
 * find_phys_addr(vaddr_t symbol)
 *
 *	Locate the physical address for a kernel symbol, given its
 *	virtual address.
 *
 * Calling/Exit State:
 *
 *	Called at sysinit time while still single-threaded, so no
 *	locks are needed. Called before any of the kernel is
 *	copied into non-DMAable memory.
 *
 *	Alignment constraints guarantee that the symbol does not
 *	cross a page boundry.
 */

STATIC paddr_t
find_phys_addr(vaddr_t symbol)
{
	int i;
	vaddr_t vaddr;
	ulong_t msize;

	/*
	 * Scan the bootinfo table, looking for the chunk which encloses
	 * the virtual for ``symbol''.
	 */
	vaddr = (vaddr_t)stext;
	for (i = 2; i < pbootinfo.memusedcnt; i++) {
		msize = pbootinfo.memused[i].extent;
		if (vaddr <= symbol && symbol < vaddr + msize)
		    return pbootinfo.memused[i].base + (symbol - vaddr);
		vaddr += msize;
	}

	/*NOTREACHED*/
	PHYS_ASSERT(0);
}
#endif /* NO_RDMA */

/*
 * STATIC void
 * find_unused_mem(void)
 *
 *	This routine determines which portions of physical memory (memavail)
 *	are not already used (memused).  The unused memory tuples 
 *	(paddr, extent, flags) are stored in the memNOTused list.
 *
 * Calling/Exit State:
 *
 *	Called at sysinit time while still single-threaded, so no
 *	locks are needed.
 */

STATIC void
find_unused_mem(void)
{
    paddr_t  free_paddr;
    unsigned avail;
    unsigned used;
    ulong_t  free_size, size;
    ushort_t free_flags;
    int	     first_used;
#ifndef NO_RDMA
    int      dma_off;
#endif /* NO_RDMA */

#ifndef NO_RDMA
    dma_off = btop(pbootinfo.dma_offset * 1024 * 1024); /* megabytes => pages */
    *((int *)find_phys_addr((vaddr_t)&tune.t_dmabase)) += dma_off;
    p_dmalimit =
	(*((int *)find_phys_addr((vaddr_t)&tune.t_dmalimit)) += dma_off);
    p_phystokvmem = *((boolean_t *) find_phys_addr((vaddr_t)&phystokvmem));
#endif /* NO_RDMA */
    p_pse_physmem = *((uint_t *)find_phys_addr((vaddr_t)&pse_physmem));

    for (avail = 0; avail < pbootinfo.memavailcnt; avail++) {
	free_paddr = pbootinfo.memavail[avail].base;
	free_size = pbootinfo.memavail[avail].extent;
	free_flags = pbootinfo.memavail[avail].flags;

	for (;;) {
		first_used = -1;
		for (used = 0; used < pbootinfo.memusedcnt; used++) {
		    if ((pbootinfo.memused[used].base >= free_paddr) &&
		        (pbootinfo.memused[used].base < free_paddr + free_size)) {
			if (first_used == -1 || pbootinfo.memused[used].base <
						pbootinfo.memused[first_used].base)
				first_used = used;
		    }
		}
		if (first_used == -1)
			break;

		/*
		 * Split out the unused parts of the availmem[] entry
		 * from the used parts.
		 */
		if (free_paddr < pbootinfo.memused[first_used].base) {
			size = pbootinfo.memused[first_used].base - free_paddr;
			add_unused_fragment(free_paddr, size, free_flags);
			free_size -= size;
		}

		/*
		 * Skip over the used chunk of memory
		 * starting at free_paddr.
		 */
		free_paddr = pbootinfo.memused[first_used].base +
				pbootinfo.memused[first_used].extent;
		free_size -= pbootinfo.memused[first_used].extent;
	}

	if (free_size != 0) {
		/*
		 * There is a free chunk of unused available memory at
		 * the end of the availmem[] entry.  Add it to memNOTused.
		 */
		add_unused_fragment(free_paddr, free_size, free_flags);
	}
    }
}

#ifndef NO_RDMA
/*
 * STATIC void
 * add_unused_fragment(paddr_t base, ulong_t extent, ushort_t flags)
 *	Add an unused fragment to the memNOTused list.
 *
 * Calling/Exit State:
 *	Called at sysinit time while still single-threaded, so no
 *	locks are needed.
 *
 * Description:
 *	If the fragment contains both DMAable and non-DMAable pages,
 *	then segregate into two fragments at the p_dmalimit boundary.
 */
STATIC void
add_unused_fragment(paddr_t base, ulong_t extent, ushort_t flags)
{
	if (P_DMA_BYTE(base) && !P_DMA_BYTE(base + extent - MMU_PAGESIZE)) {
		p_rdma_enabled = B_TRUE;
		add_unused_udf(base, P_DMALIMIT - base, flags);
		add_unused_udf(P_DMALIMIT, extent - (P_DMALIMIT - base),
			       flags | B_MEM_NODMA);
	} else {
		if (!P_DMA_BYTE(base)) {
			p_rdma_enabled = B_TRUE;
			add_unused_udf(base, extent, flags | B_MEM_NODMA);
		} else {
			add_unused_udf(base, extent, flags);
		}
	}
}

#endif /* NO_RDMA */

/*
 * STATIC void
 * add_unused_udf(paddr_t base, ulong_t extent, ushort_t flags)
 *	Add an unused fragment to the memNOTused list. The frament
 * 	is either uniformly DMAable or non-DMAable.
 *
 * Calling/Exit State:
 *	Called at sysinit time while still single-threaded, so no
 *	locks are needed.
 */
STATIC void
add_unused_udf(paddr_t base, ulong_t extent, ushort_t flags)
{
	struct unusedmem *mnup, *next_mnup, **mnupp;

	/*
	 * There is a chunk of unused, available memory
	 * from free_paddr up to memused[first_used].base.
	 * Try to concatenate with an adjacent piece. Otherwise,
	 * add it to memNOTused or memNOTusedNDMA.
	 *
	 * memNOTused is sorted in order of base address. This allows for
	 * easy concatenation of adjacent pieces.
	 *
	 * For RDMA, we keep a separate list of non-DMAable pieces.
	 */
#ifndef NO_RDMA
	mnupp = (flags & B_MEM_NODMA) ? &memNOTusedNDMA : &memNOTused;
#else /* NO_RDMA */
	mnupp = &memNOTused;
#endif /* NO_RDMA */

	mnup = *mnupp;
	while (mnup != NULL) {
		next_mnup = mnup->next;

		if (base + extent == mnup->base) {
			mnup->base = base;
			mnup->extent += extent;
			return;
		}

		if (base < mnup->base)
			break;

		if (mnup->base + mnup->extent == base) {
			mnup->extent += extent;
			if (next_mnup != NULL &&
			    mnup->base + mnup->extent == next_mnup->base) {
				mnup->extent += next_mnup->extent;
				mnup->next = next_mnup->next; /* discard */
			}
			return;
		}

		mnupp = &mnup->next;
		mnup = next_mnup;
	}

	/*
	 * Allocate and merge in a new fragment.
	 */
	mnup = &unused_array[p_unused++];
	mnup->base = base;
	mnup->extent = extent;
	mnup->flags = flags;
	mnup->next = *mnupp;
	*mnupp = mnup;
}

STATIC paddr_t pse_palloc_base = 0;
STATIC uint_t pse_palloc_left = 0;

/*
 * STATIC paddr_t
 * pse_phys_alloc(uint_t npse)
 *	Allocate npse page-size extension pages (i.e., 4MB pages)
 *
 * Calling/Exit State:
 *	Returns physical address of allocated memory, or PALLOC_FAIL
 *	if none allocated.
 */
STATIC paddr_t
pse_phys_alloc(uint_t npse)
{
	uint_t i, size;
	paddr_t pa;
	ushort_t flags;
	struct unusedmem *mnup, **mnupp, **fmnupp;
	struct unusedmem *fmnup = NULL;

	/*
	 * Find the largest available range of physically contiguous
	 * non-DMA pages
	 */
	fmnup = NULL;
	mnupp = &memNOTusedNDMA;
	while ((mnup = *mnupp) != NULL) {
		if ((fmnup == NULL || mnup->extent > fmnup->extent)) {
			fmnup = mnup;
			fmnupp = mnupp;
		}
		mnupp = &mnup->next;
	}
	if (fmnup == NULL)
		return PALLOC_FAIL;

	pa = (fmnup->base + fmnup->extent) & PSE_PAGEMASK;
	size = (fmnup->base + fmnup->extent) & PSE_PAGEOFFSET;
	i = 0;
	while (((pa - PSE_PAGESIZE) >= fmnup->base) && (i < npse)) {
		pa -= PSE_PAGESIZE;
		size += PSE_PAGESIZE;
		++i;
	}
	if (i < npse)
		return PALLOC_FAIL;

	flags = fmnup->flags;
	if ((fmnup->extent -= size) == 0)
		*fmnupp = fmnup->next;
	if ((size & PSE_PAGEOFFSET) != 0)
		add_unused_fragment(pa + (size & PSE_PAGEMASK), 
			size & PSE_PAGEOFFSET, flags);
	return pa;
}
/*
 * STATIC
 * void pse_palloc_init(void)
 *	Initialize the pse page allocator by reserving pages from the top of
 *	memory.
 *
 * Calling/Exit State:
 *	Called from build_table.  On exit, physical memory has been
 *	reserved for PSE-mapped shared memory.
 */
STATIC void
pse_palloc_init(void)
{
	uint_t npse;
	paddr_t pa;

	/*
	 * Find the largest available range of physically contiguous
	 * non-DMA pages
	 */

	npse = btopser(p_pse_physmem);
	while ((npse != 0) && ((pa = pse_phys_alloc(npse)) == PALLOC_FAIL))
		--npse;

	if (npse != 0) {
		pse_palloc_base = pa;
		pse_palloc_left = npse;
	}
}

/*
 * paddr_t
 * pse_palloc(void)
 *	Return a physical PSE page from the PSE page pool.
 *
 * Calling/Exit State:
 *	Called during system initilization (after sysinit has started).
 *	PSE page pool has previously been allocated from memory.
 *
 *	Returns physical address of PSE page, or PALLOC_FAIL if no
 *	PSE pages are left, 
 */
paddr_t
pse_palloc(void)
{

	if ((pse_palloc_left == 0) || (pse_palloc_base == 0))
		return PALLOC_FAIL;
	--pse_palloc_left;
	return pse_palloc_base + psetob(pse_palloc_left);
}

/*
 * STATIC paddr_t
 * pse_alloc_kernel(uint_t offset, long request)
 *	Allocate PSE-mappable memory for kernel text and data.  offset
 *	is the offset of the beginning of the kernel within a PSE page
 *	and request is the size of kernel text and data.
 *
 * Calling/Exit State:
 *	Returns physical address of allocated memory, or PALLOC_FAIL
 *	if no memory could be allocated.
 */
STATIC paddr_t
pse_alloc_kernel(uint_t offset, long request)
{
	long osize;
	ushort_t flags;
	paddr_t pa;
	struct unusedmem *mnup, **mnupp, **fmnupp;
	struct unusedmem *fmnup = NULL;

	/*
	 * Find the largest available range of physically contiguous
	 * non-DMA pages
	 */
	fmnup = NULL;
	mnupp = &memNOTusedNDMA;
	while ((mnup = *mnupp) != NULL) {
		if ((fmnup == NULL || mnup->extent > fmnup->extent)) {
			fmnup = mnup;
			fmnupp = mnupp;
		}
		mnupp = &mnup->next;
	}
	if (fmnup == NULL)
		return PALLOC_FAIL;

	pa = ((fmnup->base + fmnup->extent) & PSE_PAGEMASK) + offset;
	while (pa > (fmnup->base + fmnup->extent)) {
		pa -= PSE_PAGESIZE;
		if (pa < fmnup->base)
			return PALLOC_FAIL;
	}
	while (((fmnup->base + fmnup->extent) - pa) < request) {
		pa -= PSE_PAGESIZE;
		if (pa < fmnup->base)
			return PALLOC_FAIL;
	}
	osize = fmnup->extent;
	flags = fmnup->flags;
	fmnup->extent = pa - fmnup->base;
	if (fmnup->extent == 0)
		*fmnupp = fmnup->next;
	if ((pa + request) < (fmnup->base + osize)) {
		add_unused_fragment(pa + request,
			(fmnup->base + osize) - (pa + request), flags);
	}
	return pa;
}
