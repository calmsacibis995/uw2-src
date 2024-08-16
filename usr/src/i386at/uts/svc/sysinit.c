/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:svc/sysinit.c	1.74.1.74"
#ident	"$Header: $"

/*
 * Machine dependent system initialization.
 */

#include <io/autoconf/confmgr/confmgr.h>
#include <util/inline.h>
#include <util/types.h>
#include <mem/kma.h>
#include <mem/immu.h>
#include <mem/vmparam.h>
#include <mem/vm_mdep.h>
#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/seg_kmem.h>
#include <util/sysmacros.h>
#include <util/param.h>
#include <util/cmn_err.h>
#include <util/plocal.h>
#include <util/ksynch.h>
#include <util/engine.h>
#include <proc/signal.h>
#include <proc/lwp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/disp.h>
#include <proc/class.h>
#include <io/conf.h>
#include <io/conssw.h>
#include <util/var.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <util/ipl.h>
#include <proc/user.h>
#include <svc/cpu.h>
#include <svc/creg.h>
#include <svc/fp.h>
#include <svc/intr.h>
#include <svc/psm.h>
#include <svc/systm.h>
#include <svc/trap.h>
#include <svc/utsname.h>
#ifdef WEITEK
#include <svc/weitek.h>
#endif
#include <mem/kmem.h>
#include <mem/hatstatic.h>
#include <util/kdb/xdebug.h>
#include <util/ghier.h>
#include <svc/bootinfo.h>
#include <svc/copyright.h>
#include <proc/usync.h>
#include <proc/tss.h>

extern void double_panic(kcontext_t *);
extern void xcall_init(void);
extern void detect_cpu(void);
extern int detect_fpu(void);
extern void dma_init(void);
 
#ifdef BUG386B1
extern int detect_387cr3(void);
#endif
#ifdef WEITEK
extern int detect_weitek(void);
#endif

STATIC int chip_detect(void);
STATIC void unmap_page0(engine_t *);
STATIC void init_desc_tables(void);
STATIC void tmp_init_desc_tables(void);
STATIC void setup_priv_maps(int, int);

/*
 * Title and copyright messages.  May be overridden by bootarg_parse().
 */
char *title[MAXTITLE] = {
	SYS_TITLE
};
uint_t ntitle = 1;
boolean_t title_changed = B_FALSE;
char *copyright[MAXCOPYRIGHT] = {
	COPYRIGHT_COMMON
	COPYRIGHT_FAMILY
	COPYRIGHT_PLATFORM
	"All Rights Reserved"
	"\nU.S. Pat. No. 5,349,642"
};
uint_t ncopyright = 1;
boolean_t copyright_changed = B_FALSE;

/*
 * Table for constructing IDT that is common across all platforms.
 */
struct idt_init idt_init[] = {
	{	DIVERR,		GATE_386TRP,	t_diverr,	GATE_KACC },
	{	SGLSTP,		GATE_386TRP,	t_dbg,		GATE_KACC },
	{	NMIFLT,		GATE_386INT,	t_nmi,		GATE_KACC },
	{	BPTFLT,		GATE_386TRP,	t_int3,		GATE_UACC },
	{	INTOFLT,	GATE_386TRP,	t_into,		GATE_UACC },
	{	BOUNDFLT,	GATE_386TRP,	t_check,	GATE_UACC },
	{	INVOPFLT,	GATE_386TRP,	t_und,		GATE_KACC },
	{	NOEXTFLT,	GATE_386TRP,	t_dna,		GATE_KACC },
	{	EXTOVRFLT,	GATE_386INT,	t_extovr,	GATE_KACC },
	{	INVTSSFLT,	GATE_386INT,	t_badtss,	GATE_KACC },
	{	SEGNPFLT,	GATE_386TRP,	t_notpres,	GATE_KACC },
	{	STKFLT,		GATE_386TRP,	t_stkflt,	GATE_KACC },
	{	GPFLT,		GATE_386TRP,	t_gpflt,	GATE_KACC },
	{	PGFLT,		GATE_386INT,	t_pgflt,	GATE_KACC },
	{	EXTERRFLT,	GATE_386TRP,	t_coperr,	GATE_UACC },
	{	ALIGNFLT,	GATE_386TRP,	t_alignflt,	GATE_KACC },
	{	MCEFLT,		GATE_386TRP,	t_mceflt,	GATE_KACC },
	{ 0 },
};

struct gate_desc def_intf0;
struct gate_desc fpuon_noextflt, fpuoff_noextflt;

struct desctab_info global_dt_info[NDESCTAB];
struct segment_desc *global_ldt;
struct desctab std_idt_desc;

extern void init_console(void);
extern void inituname(void);
extern void printuname(const char *);
extern void kdb_init(void);
extern void clock_init(void);
extern void kvm_init(void);
extern void strinit(void);
extern void ddi_init(void);
extern void mod_obj_kern_init(void);
extern void physkv_init();
extern void nmi_init(void);

extern void disponline(engine_t *);
extern void dispoffline(engine_t *);
#ifndef UNIPROC
extern void empty_local_runqueue(void);
#endif

/*
 * Initialize global defines of PL's for usage by DDI/DKI drivers.
 */
pl_t pl0 = PL0;
pl_t pl1 = PL1;
pl_t pl2 = PL2;
pl_t pl3 = PL3;
pl_t pl4 = PL4;
pl_t pl5 = PL5;
pl_t pl6 = PL6;
pl_t pl7 = PL7;

pl_t plbase = PLBASE;
pl_t pltimeout = PLTIMEOUT;
pl_t pldisk = PLDISK;
pl_t plstr = PLSTR;
pl_t plhi = PLHI;
pl_t invpl = INVPL;

/*
 * Initialize global priority values for use by DDI/DKI drivers.
 */
int pridisk = PRINOD;
int prinet = 27;
int pritty = 25;
int pritape = 25;
int prihi = PRIMEM;
int primed = 24;
int prilo = 5;

/*
 * set to 1 by psm_configure() by Corollary PSM to support early
 * startup of non-boot engines.  for other platforms, simply leave
 * this variable untouched.
 */

int     corollary = 0;

/*
 * Default CPU binding.  Some platforms have a binding requirement
 * on their main I/O bus.
 */
int default_bindcpu = -1;

extern void (*io_init[])(void);
extern void (*io_start[])(void);
extern void (*io_halt[])(void);
extern int bindcpu_init[];
extern int bindcpu_start[];
extern int bindcpu_halt[];
extern int intr_bindcpu[];
extern int intcpu[];
extern void (*ivect[])();
extern int int_itype[];
extern int nintr;

extern void intnull(void);

/*
 * crash_kl1pt must be declared in in a file that is loaded V=P
 * so that it's offset will equal it's symbol table address 
 * for the use of crash(1).
 *
 * This, and the other V=P variables, vmemptr and myengppriv, can
 * only be accessed before unmap_page0() has been called.
 *
 *	vmemptr - page-aligned virtual address of next unused
 *		       virtual memory (to initialize calloc()).
 *
 *	myengppriv  - virtual address of this engine's struct ppriv_pages.
 *		       The pp_kl1pt and pp_pmap pages are already in use as
 *		       the current engine's level 1 and level 2 page tables.
 */ 
extern paddr_t crash_kl1pt;
extern vaddr_t vmemptr;
extern struct ppriv_pages *myengppriv;

vaddr_t physkv_extent;

/*
 * void
 * sysinit(void)
 *
 *	Perform machine specific initialization.
 *
 * Calling/Exit State:
 *
 *	Called with virtual addressing mode active and the kernel
 *	virtual address space setup.  BSS has been zeroed.
 *
 *	Returns in a state ready to call main().
 */
void
sysinit(void)
{
	struct ppriv_pages *ppriv;
	struct segment_desc *sd;
	struct gate_desc *gd;
	int procid;
	ulong_t cr0;
	int i, j;
	extern paddr_t physkv_extent_p;
	extern boolean_t phystokvmem;
	extern char *bustype;
	ulong_t len;
	timestruc_t tv;
#ifdef DEBUG
	boolean_t intcpu_default = B_FALSE;
	boolean_t plbase_after_init;
#endif

	crash_kl1pt = kvtophys((addr_t)&myengppriv->pp_kl1pt[0][0]);

	/*
	 * Initialize enough stuff so we can get faults.
	 */
	tmp_init_desc_tables();

	/*
	 * Initialize l.userp to the per-engine u area address.
	 */
	l.userp = &ueng;

	/*
	 * this get put in to support corollary architecture.
         */
        l.plsti = PLMAX + 1;
	l.shadow_if = 1;

	/*
	 * no kernel preemptions pending
	 */
	l.prmpt_state.s_prmpt_state.s_noprmpt = 1;

	/*
	 * needed for kdb initialization.  will be adjusted by PSM.
	 */
	l.microdata = 50;


	/*
	 * Initialize the lock statistics data structures.
	 */
	lkstat_init();

	/* to init calloc() */
	hat_static_init(vmemptr);

	/*
	 * Initialize intr_bindcpu and int_itype for any
	 * statically-initialized vectors.
	 */
	for (i = 0; i < nintr; i++) {
#ifdef DEBUG /* Can't ASSERT yet. */
		if (intcpu[i] == -2)
			intcpu_default = B_TRUE;
#endif
		if (ivect[i] != intnull) {
			intr_bindcpu[i] = intcpu[i];
			int_itype[i] = 3;
		}
	}

	/*
	 * Initialize interrupt controller.  Disables all interrupts,
	 * but sets apparent level to PLBASE.
	 */
	psm_intr_init();

#ifdef DEBUG /* Can't ASSERT yet. */
	plbase_after_init = (getpl() == PLBASE);
#endif

	/*
	 * Retrieve any information passed from the bootstrap.
	 * calloc is available at this time, but cmn_err is not.
	 *
	 * This must come before console I/O so that boot parameters
	 * can be used to change the console device.
	 */
	bootarg_parse();

	/*
	 * Initialize console I/O.
	 */
	cmn_err_init();
	init_console();

	/*
	 * Initialize the bustype string (defaulting to the value
	 * assigned in name.cf/Space.c) for use by scoinfo.
	 */
	if (bootinfo.machflags & MC_BUS)
		bustype = "MCA";
	else if (bootinfo.machflags & EISA_IO_BUS)
		bustype = "EISA";
	else if (bootinfo.machflags & AT_BUS)
		bustype = "ISA";

	/*
	 * Initialize the O/S name/version, so it can be used in the
	 * message below.
	 */
	inituname();

	/*
	 * Print system title and copyright messages.
	 */
	cmn_err(CE_CONT, "^\n");	/* In case not at beginning of line */
	for (i = 0; i < ntitle; i++)
		printuname(title[i]);	/* parameterize and print line */
	cmn_err(CE_CONT, "\n");
	for (i = 0; i < ncopyright; i++)
		cmn_err(CE_CONT, "%s\n", copyright[i]);
	cmn_err(CE_CONT, "\n");

	/*
	 * Handle deferred ASSERTs.
	 */
	ASSERT(!intcpu_default);
	ASSERT(plbase_after_init);
	ASSERT(getpl() == PLBASE);

	/*
	 * Configure the HW and initialize the interrupt table.
	 * Configure is called with the temporary page tables in use.
	 *
	 * Configure fills out:
	 *	engine[] array		; one per processor
	 *	Nengine			; # of processors
	 *	topmem			; top of physical memory
	 */

	configure();

	/*
	 * Allocate and initialize global LDT.
	 */

	callocrnd(8);	/* LDT must be 8-byte aligned */
	global_ldt = calloc(LDTSZ * 8);

	gd = (struct gate_desc *)&global_ldt[seltoi(USER_SCALL)];
	BUILD_GATE_DESC(gd, KCSSEL, sys_call, GATE_386CALL, GATE_UACC, 1);
	gd = (struct gate_desc *)&global_ldt[seltoi(USER_SIGCALL)];
	BUILD_GATE_DESC(gd, KCSSEL, sig_clean, GATE_386CALL, GATE_UACC, 1);
	sd = &global_ldt[seltoi(USER_CS)];
	BUILD_MEM_DESC(sd, 0, mmu_btop(UVEND), UTEXT_ACC1, TEXT_ACC2);
	sd = &global_ldt[seltoi(USER_DS)];
	BUILD_MEM_DESC(sd, 0, mmu_btop(UVUVWIN)+1, UDATA_ACC1, DATA_ACC2);
	sd = &global_ldt[seltoi(UVWINSEL)];
	BUILD_MEM_DESC(sd, UVUVWIN, 1, UDATA_ACC1, DATA_ACC2);

	global_dt_info[DT_LDT].di_table = global_ldt;
	global_dt_info[DT_LDT].di_size = LDTSZ * 8;

	global_dt_info[DT_GDT].di_table = l.global_gdt;
	global_dt_info[DT_GDT].di_size = GDTSZ * 8;

	/*
	 * Allocate per-processor local data.
	 * (The one for the current processor was already allocated
	 * and passed to us.)
	 */

	callocrnd(MMU_PAGESIZE);			/* for L1PT's */
	ppriv = calloc((Nengine - 1) * SZPPRIV);

	/*
	 * Fill out remaining engine table fields not set by configure().
	 */

	procid = Nengine;
	for (i = 0, j = 0; i < Nengine; i++) {
		engine[i].e_flags |= E_OFFLINE;
		if (i == BOOTENG) {           /* boot engine ? */
			procid = i;
			engine[i].e_local = myengppriv;
		} else {
			engine[i].e_local = &ppriv[j++];
		}
	}
	ASSERT(j == (Nengine - 1));	/* used up exactly all of ppriv[] */

	if (procid >= Nengine) {
		/*
		 *+ During initialization of the system, the system 
		 *+ configuration data structures implied that the 
		 *+ booting processor was not a member of this system.
		 */
		cmn_err(CE_PANIC, "sysinit: boot engine not in engine table");
	}

	/*
	 * Although the code immediately above doesn't do this,
	 * there is code at the start of this function which assumes
	 * that the boot engine is engine[0].  This is currently
	 * always true, although we prefer not to assume it.
	 */

	ASSERT(procid == 0);

	/*
	 * Perform basic sanity of the kernel stack.  Since the
	 * kernel stack size is a function of sizeof(struct user),
	 * there's always the possibility the user structure won't
	 * leave enough space for the kernel stack.
	 * 3000 bytes is an estimate.
	 */
	if (UAREA_OFFSET - KSTACK_RESERVE < 3000)
		/*
		 *+ The current layout of the u-block leaves insufficient
		 *+ room for the kernel stack.  This is an internal
		 *+ inconsistency which can only be fixed by a kernel
		 *+ developer.
		 */
		cmn_err(CE_PANIC, "Not enough space for the kernel stack.");

	/*
	 * Initialize the struct modctl and struct module for the
	 * static kernel.
	 */
	mod_obj_kern_init();

#ifndef NODEBUGGER
	/*
	 * Initialize the kernel debugger(s).
	 * At this point:
	 *	BSS must be mapped and zeroed.
	 *	It must be possible to call the putchar/getchar routines
	 *		for the console device.
	 *	It must be possible to field debugger traps.
	 *		(Note: k_trap() references u.u_fault_catch)
	 *
	 * Command strings from "unixsyms -i" will be executed here
	 * by the first statically-configured kernel debugger, if any.
	 */
	kdb_init();
#endif

	/*
	 * Determine type of CPU/FPU/FPA.
	 * We do this here instead of in selfinit(), so we can enable
	 * the cache on i486 chips much sooner.
	 */

	(void) chip_detect();

	if (l.cpu_id != CPU_386) {
		/*
		 * Running on an i486 or greater; enable the onchip
		 * cache.  While we're at it, set other control bits.
		 * Note that we're explicitly clearing the "Alignment
		 * Mask" bit to prevent alignment checking in user
		 * programs.  Also, enable kernel-mode write-protect
		 * checking.
		 */
		INVAL_ONCHIP_CACHE();
		cr0 = READ_MSW();
		WRITE_MSW((cr0 & ~(CR0_CD|CR0_NW|CR0_AM)) | CR0_WP);
	}

	/*
	 * We initialize eng_tbl_mutex early, as it will be needed before
	 * engine_init is called.
	 */

	LOCK_INIT(&eng_tbl_mutex, ENG_MUT_HIER, ENG_MINIPL,
		  &eng_tbl_lkinfo, KM_SLEEP);

	/*
	 * Initialize physkv_extent 
	 */
	physkv_extent = physkv_extent_p;

	if (!phystokvmem)
		physkv_extent = PHYSTOKV_PARTIAL_COMPAT_RANGE;
	else {
		if (physkv_extent > PHYSTOKV_FULL_COMPAT_RANGE)
			physkv_extent = PHYSTOKV_FULL_COMPAT_RANGE;
	}

	hat_statpt_alloc(KVPHYSTOKV, physkv_extent);

	/*
	 * Initialize kernel virtual memory.
	 * Disable calloc(); enable kmem_alloc().
	 */

	kvm_init();	/* initialize kernel virtual memory and kmem_alloc() */

	ATOMIC_INT_INIT(&n_user_write, 0);

	/*
	 * phystokv support
	 */

	len = btop(physkv_extent);
	segkmem_ppid_mapin(kpgseg, KVPHYSTOKV, len, phystoppid(0),
			 PROT_READ|PROT_WRITE);

	/*
	 * Setup page tables for each engine
	 * based on our engine's page tables.
	 */
	for (i = 0; i < Nengine; i++)
		setup_priv_maps(i, procid);

	/*
	 * Override default CPU bindings for static H/W modules.
	 */
	for (i = 0; io_init[i] != NULL; i++) {
		if (bindcpu_init[i] == -2)
			bindcpu_init[i] = default_bindcpu;
	}
	for (i = 0; io_start[i] != NULL; i++) {
		if (bindcpu_start[i] == -2)
			bindcpu_start[i] = default_bindcpu;
	}
	for (i = 0; io_halt[i] != NULL; i++) {
		if (bindcpu_halt[i] == -2)
			bindcpu_halt[i] = default_bindcpu;
	}
	for (i = 0; i < bdevcnt; i++) {
		if (bdevsw[i].d_cpu == -2)
			bdevsw[i].d_cpu = default_bindcpu;
	}
	for (i = 0; i < cdevcnt; i++) {
		if (cdevsw[i].d_cpu == -2)
			cdevsw[i].d_cpu = default_bindcpu;
	}

	/*
	 * Initialize cross-processor interrupt
	 */
	xcall_init();

	/*
	 * Initialize STREAMS
	 */
	strinit();

	/*
	 * Initialize NMI handling + register any system NMI handlers.
	 */
	nmi_init();

	/*
	 * Start clock services.  Must do the clock handling before
	 * dispatcher, as the dispatcher calls into the scheduling
	 * classes which are known to set timeouts.  ddi_init() also
	 * needs timeouts.
	 */
	clock_init();

	/*
	 * Initialize DDI/DKI routines.
	 */
	ddi_init();

	/*
	 * Read the time-of-day clock to be used to initialize unix clock
	 * (hrestime and time).
	 */
	if (xtodc(&tv) == 0) {
		settime(&tv);
	}

	/*
	 * Initialize the dispatcher.
	 */
	dispinit();

	/*
	 * System is mapped, do self-init.
	 */
	selfinit(procid);
}


/*
 * void
 * selfinit(int procid)
 *
 * Calling/Exit State:
 *
 *	Perform per-processor initialization.
 */
void
selfinit(int procid)
{
	engine_t *eng;
	uint_t cr0, cr4;
	boolean_t eng_lock = B_FALSE;
	void (**funcp)(void);
	struct intr_list *ilistp;
	int i;
	extern boolean_t user_rdtsc;

	eng = &engine[procid];

	/*
	 * Fill out fields in l.
	 */

	myengnum = procid;
	l.eng = eng;
	EMASK_S_INIT(&l.eng_mask, myengnum);

	/*
	 * Initialize per-processor GDT, IDT, TSS, and segment registers.
	 *
	 * MUST be done after myengnum is initialized. This is necessary
	 * to build systempro kernel.
	 */
	init_desc_tables();

	/*
	 * Initialize the engine's view of the u area.
	 */
	l.userp = &ueng;

	/*
	 * Init miscellaneous struct plocal members.
	 */
	l.one_sec = 1;

	/*
	 * Needed for driver initialization.  Will be adjusted by PSM.
	 */
	l.microdata = 50;

	/*
	 * Determine type of CPU/FPU/FPA.
	 * (chip_detect for boot processor done in sysinit().)
	 */
	if (upyet) {
        	l.plsti = PLMAX + 1;
		l.shadow_if = 1;

		/*
		 * no kernel preemptions pending
		 */
		l.prmpt_state.s_prmpt_state.s_noprmpt = 1;

                /*
                 * Initialize interrupt controller.  Disables all interrupts,
                 * but sets apparent level to PLBASE.
                 */
                psm_intr_init();

		if (chip_detect() == -1) {
			/* Can't online this engine. */
			l.eng->e_flags |= E_BAD;
			EVENT_SIGNAL(&eng_wait, 0);
			for (;;)
				asm("hlt");
			/* NOTREACHED */
		}

		if (l.cpu_id != CPU_386) {
			/*
			 * Running on an i486 or greater; enable the onchip
			 * cache.  While we're at it, set other control bits.
			 * Note that we're explicitly clearing the "Alignment
			 * Mask" bit to prevent alignment checking in user
			 * programs.  Also, enable kernel-mode write-protect
			 * checking.
			 */
			INVAL_ONCHIP_CACHE();
			cr0 = READ_MSW();
			WRITE_MSW((cr0 & ~(CR0_CD|CR0_NW|CR0_AM)) | CR0_WP);
		}
	}

	/*
	 * Set up floating point management.
	 *
	 * There may or may not really be hardware, regardless of what
	 * fp_kind indicates.  Init the FPU if it's there.
	 * Note that init_fpu() disables the FPU when it's done.
	 */
	l.fpuon = (READ_MSW() & ~(CR0_MP|CR0_EM|CR0_TS|CR0_NE));
	l.fpuoff = (l.fpuon | CR0_EM);
	l.fpuon |= CR0_MP;
	if (l.cpu_id != CPU_386)
		l.fpuon |= CR0_NE;	/* 486 numeric error mode */
	init_fpu();
	if (!(fp_kind & FP_HW))
		l.fpuon = l.fpuoff;

	/*
	 * The CR0_NE bit is only set on a i486 or above systems and when
	 * set enables the standard mechanism for reporting floating-point
	 * numeric errors.
	 *
	 * When the CR0_NE bit is clear a numeric error causes the processor
	 * to stop and wait for an interrupt.
	 *
	 * On a i486SX with an i387 coprocessor should have the CR0_NE bit
	 * clear and report numeric error thru interrupt 13.
	 */
	l.fpu_external = (fp_kind & FP_HW) && !(l.fpuon & CR0_NE);

	/*
	 * If necessary, set up floating-point emulator vectors.
	 */
	if (fp_kind == FP_SW)
		fpesetvec();

	/*
	 * Enable/disable special CPU features.
	 */
	if (l.cpu_features[0] & CPUFEAT_MCE) {
		/* enable machine-check exceptions */
		cr4 = _cr4();
		_wcr4(cr4 | CR4_MCE);
	}
	if (l.cpu_features[0] & CPUFEAT_TSC) {
		/* enable/disable user use of rdtsc */
		cr4 = _cr4();
		if (user_rdtsc)
			cr4 &= ~CR4_TSD;
		else
			cr4 |= CR4_TSD;
		_wcr4(cr4);
	}

	/*
	 * Compute l.cpu_speed for spin delay loops.  The value
	 * corresponds to the mips rating of the processor at it
	 * clock speed.
	 */
	if (l.cpu_id == CPU_486)
		l.cpu_speed = (i486_lcpuspeed * l.eng->e_cpu_speed) / 100;
	else
		l.cpu_speed = (lcpuspeed * l.eng->e_cpu_speed) / 100;

	/*
	 * If we're a 386 and any user-writes are in progress which
	 * aren't using the 386 workaround, we can't come online
	 * until they're finished.
	 */
	if (l.cpu_id == CPU_386) {
		while (ATOMIC_INT_READ(&n_user_write) != 0)
			;
	}

	/*
	 * Pre-compute PTE value for this engine's stack extension page.
	 */
	l.kse_pte.pg_pte = mkpte(PG_RW|PG_V,
				 pfnum(kvtophys(l.eng->e_local->pp_kse)));

	/*
	 * Make FPU/FPA type visible to user process.
	 */
	uvwin.uv_fp_hw = fp_kind;
#ifdef WEITEK
	uvwin.uv_fp_hw |= (weitek_kind << 8);
#endif

	/*
	 * Initialize the DMA controller.
	 * (Must be called before driver init routines).
	 */
	if (myengnum == BOOTENG)
		dma_init();


	/*
	 * Announce our availability to the dispatching sub-system.
	 */
        if (corollary) {
                if (myengnum == BOOTENG)
                        disponline(eng);
        } else {
                disponline(eng);
	}

	/*
	 * Perform misc. per-processor platform-specific initialization
	 * before enabling interrupts.
	 */
	psm_selfinit();

	/*
	 * Initialize the programmable interrupt timer chip (i8254).
	 */
	psm_timer_init();

	/*
	 * Call driver init(D2D) routines.
	 */
	for (funcp = io_init, i = 0; *funcp != NULL; i++, funcp++) {
		if (bindcpu_init[i] == myengnum ||
		    (!upyet && bindcpu_init[i] == -1))
			(**funcp)();
	}

	/*
	 * Now that we're off page 0, blow away the mapping of page zero.
	 * This assumes there's nothing else mapped in the lower 4MB.
	 */
	unmap_page0(eng);

	/*
	 * enable interrupts
	 */
	psm_intr_start();

	/*
	 * Attach interrupts for those who don't do it themselves.
	 */
	if (!upyet) {
		for (ilistp = static_intr_list; ilistp->il_name != NULL;
								ilistp++) {
			cm_intr_attach_all(ilistp->il_name, ilistp->il_handler,
					   ilistp->il_devflagp, NULL);
		}
	}

	/*
	 * Invoke module "start" functions [start(D2DK)].
	 */
	for (funcp = io_start, i = 0; *funcp != NULL; i++, funcp++) {
		if (bindcpu_start[i] == myengnum ||
		    (!upyet && bindcpu_start[i] == -1))
			(**funcp)();
	}

	/*
	 * Check both bdevsw and cdevsw to see if there are
	 * any drivers bound to this engine. If so, lock
	 * the engine online because engines that have
	 * bound drivers cannot be offline'd.
	 */
	for (i = 0; i < bdevcnt; i++) {
		if ((bdevsw[i].d_cpu == myengnum) ||
		    (bdevsw[i].d_cpu == -1 && myengnum == 0 &&
		    !(*bdevsw[i].d_flag & D_MP))) {
			eng_lock = B_TRUE;
			break;
		}
	}
	if (!eng_lock) {
		for (i = 0; i < cdevcnt; i++) {
			if ((cdevsw[i].d_cpu == myengnum) ||
			    (cdevsw[i].d_cpu == -1 && myengnum == 0 &&
			    !(*cdevsw[i].d_flag & D_MP))) {
				eng_lock = B_TRUE;
				break;
			}
		}
	}
	if (eng_lock && !engine_disable_offline(myengnum)) {
		/*
		 *+ When bringing a processor online, the system
		 *+ fails to lock the engine online when there
		 *+ are drivers bound to the engine.
		 */
		cmn_err(CE_PANIC, "selfinit: failed to lock engine\n");
	}
	/* Turn on hat accounting */
	if (corollary) {
		if (myengnum == BOOTENG)
			hat_online();
	} else {
		hat_online();
	}

	/*
	 * Signal that online is complete.
	 * MUST NOT come before disponline().
	 */
	EVENT_SIGNAL(&eng_wait, 0);

#ifndef NODEBUGGER
	/*
	 * Let kernel debugger know we're here.
	 */
	(*cdebugger) (DR_ONLINE, NO_FRAME);
#endif

	/*
	 * Perform misc. initialization including interrupt
	 * distribution/assignments to non-boot engine.
	 */
	psm_misc_init();

	/*
	 * Switch on the leds to indicate that the processor is online.
	 */
	psm_ledctl(LED_ON, ONLINE_LED);
}


/*
 * STATIC int
 * chip_detect(void)
 *	Detect CPU/FPU/FPA types.
 *
 * Calling/Exit State:
 *	Returns -1 if the online cannot succeed due to unsupported
 *	configurations.
 *
 *	Called when booting or when an engine is coming online for
 *	the first time, and uses only processor-local variables or
 *	globals which are constant once the system is up, so no locking
 *	is needed.  The exceptions are n_i386_online and n_weitek_online,
 *	which are effectively mutexed by onoff_mutex.
 */
STATIC int
chip_detect(void)
{
	static int detect_flags;
#define DETECT_FPU	0x01
#ifdef BUG386B1
#define DETECT_386B1	0x02
#define DETECT_387CR3	0x04
#endif
	int my_fp_kind;

	if (!upyet) {
		if (fp_kind == 0) /* auto-detect */
			detect_flags |= DETECT_FPU;
#ifdef BUG386B1
		if (do386b1 == 2) /* auto-detect */
			detect_flags |= DETECT_386B1;
		if (do387cr3 == 2) /* auto-detect */
			detect_flags |= DETECT_387CR3;
#endif /* BUG386B1 */
	}

	detect_cpu();
#ifdef BUG386B1
	if (!(detect_flags & DETECT_386B1)) {
		if (l.cpu_id == CPU_386 || do386b1) {
			l.cpu_id = CPU_386;
			l.cpu_stepping = do386b1;
		}
	}
#else /* !BUG386B1 */
	if (l.cpu_id == CPU_386 && l.cpu_stepping == STEP_386B1) {
		/*
		 *+ B1-step i386 CPUs not supported by this system.
		 *+ Upgrade to a more recent i386 CPU chip.
		 */
		cmn_err(CE_PANIC, "B1-step i386 not supported");
	}
#endif /* BUG386B1 */
	if (detect_flags & DETECT_FPU)
		my_fp_kind = detect_fpu();
	else if (!((my_fp_kind = fp_kind) & FP_HW))
		my_fp_kind = FP_NO;
	if (my_fp_kind == FP_NO && fp_kind == FP_SW && upyet)
		my_fp_kind = FP_SW;
#ifdef WEITEK
	l.weitek_kind = detect_weitek();
#endif

#ifdef DEBUG
	{
		static char *cpu_name[] = {
			"???", "???", "???", "i386", "i486", "Pentium"
		};
		static char *fpu_name[] = {
			"no fpu", "no fpu", "80287", "80387"
		};
		uint_t id = l.cpu_id;

		if (id >= sizeof cpu_name / sizeof cpu_name[0])
			id = 0;
		cmn_err(CE_CONT, "# processor %d: %s ",
				 myengnum, cpu_name[id]);
		cmn_err(CE_CONT, "model %d step %d;",
				 l.cpu_model, l.cpu_stepping);
		cmn_err(CE_CONT, " %s", fpu_name[my_fp_kind]);
#ifdef WEITEK
		if (l.weitek_kind == WEITEK_HW)
			cmn_err(CE_CONT, " w/weitek");
#endif
		cmn_err(CE_CONT, "\n");
	}
#endif /* DEBUG */

	if (!upyet) {
		fp_kind = my_fp_kind;
#ifdef BUG386B1
		if (detect_flags & DETECT_386B1) {
			do386b1 = (l.cpu_id == CPU_386 &&
				   l.cpu_stepping == STEP_386B1);
		}
#endif /* BUG386B1 */
	} else {
		if (my_fp_kind != fp_kind) {
			if ((my_fp_kind & FP_HW) && !(fp_kind & FP_HW)) {
				/*
				 *+ All processors must have the same type
				 *+ of floating-point hardware (none, 80287
				 *+ or 80387).  To take advantage of math
				 *+ coprocessors, you must install them for
				 *+ all processors.
				 */
				cmn_err(CE_NOTE,
				  "Mixed FPU configurations not supported;\n"
				  "\tfloating-point coprocessor for processor "
				  "%d not used.", myengnum);
			} else {
				/*
				 *+ All processors must have the same type
				 *+ of floating-point hardware (none, 80287
				 *+ or 80387).  Since the processor being
				 *+ brought online does not have the same
				 *+ math coprocessor as the boot processor,
				 *+ it cannot be used.  To fix: install a
				 *+ math coprocessor on this processor.
				 */
				cmn_err(CE_NOTE,
				  "Mixed FPU configurations not supported;\n"
				  "\tprocessor %d cannot be brought online.",
				  myengnum);
				return -1;
			}
		}
#ifdef BUG386B1
		if (do386b1 ||
#else /* !BUG386B1 */
		if (
#endif /* BUG386B1 */
		    (l.cpu_id == CPU_386 && l.cpu_stepping == STEP_386B1)) {
			/*
			 *+ Multiple processors cannot be supported when
			 *+ one of them is an old 80386 B1 stepping chip.
			 *+ If the boot processor was an 80836 B1, no other
			 *+ processors can be used; otherwise, no 80386 B1
			 *+ processors can be used.  To fix: upgrade any
			 *+ 80386 B1 chips to more recent 80386 chips.
			 */
			cmn_err(CE_NOTE,
			  "Multi-processor configurations not supported "
			  "for 80386 B1 stepping;\n"
			  "\tprocessor %d cannot be brought online.",
			  myengnum);
			return -1;
		}
	}

#ifdef BUG386B1
	if (do386b1 && (fp_kind & FP_HW)) {
		do386b1_x87 = 1;
		if (detect_flags & DETECT_387CR3)
			do387cr3 = detect_387cr3();
		if (do387cr3) {
			/*
			 * Workaround for Intel386(tm) B1 stepping errata #21.
			 * Errata #21 requires the value in %cr3 (which is the
			 * physical address of the page directory) to be at
			 * least 0x80001000.
			 *
			 * This workaround can only be implemented on systems
			 * which have memory subsystems with "bit 31 aliasing",
			 * i.e. where physical address (x + 0x80000000) maps
			 * to the same memory as address x.  For these systems,
			 * we use the alias form of the page directory address,
			 * which will be above 0x80000000; address 0 is never
			 * used for the page directory.
			 */
			uint_t cr3 = _cr3();
			_wcr3(cr3 | 0x80000000);
#ifdef DEBUG
			cmn_err(CE_CONT, "# 387cr3 workaround enabled\n");
#endif
		}
	} else
		do387cr3 = 0;
#endif /* BUG386B1 */

	if (l.cpu_id == CPU_386)
		n_i386_online++;
#ifdef WEITEK
	if (l.weitek_kind == WEITEK_HW)
		n_weitek_online++;
#endif /* WEITEK */

	return 0;
}


/*
 * STATIC void
 * unmap_page0(engine_t *eng)
 *
 *	Unmap page-0 after boot.
 *
 * Calling/Exit State:
 *
 *	Called first thing after the engine comes online.
 *
 * Description:
 *
 *	Blow away the mapping of page 0 necessary to online subsequent
 *	processors.  This is helpful to catch dereferences of NULL pointers.
 *	We assume there's nothing else mapped in this page and we can just
 *	cut it loose.
 *
 *	We actually need to unmap the first *two* pages to totally
 *	unmap the leftover from build_table().  We do this by actually
 *	unmapping the entire bottom 4 Meg.
 *
 *	Note that, after every engine has been onlined once (and hence
 *	invoked this function) we could, in theory, reclaim the
 *	physical memory for the startup code and data as well as the
 *	level 2 page table which maps them.  However, we don't bother
 *	doing this.  So we lose 3 pages.
 */
STATIC void
unmap_page0(engine_t *eng)
{
	struct ppriv_pages *ppriv;

	ppriv = eng->e_local;
	ppriv->pp_kl1pt[0][0].pg_pte = 0;

	TLBSflushtlb();
}


/*
 * void
 * offline_self(void)
 *	Called for a processor to take itself offline.
 *
 * Calling/Exit State:
 *	None.
 */
void
offline_self(void)
{
	engine_t *eng = l.eng;

#ifndef UNIPROC
	/*
	 * There may be affinitized LWPs on the local run queue.
	 * Empty local run queue (tranfer them to global run queue)
	 * before proceeding further with offlining self.
	 */
	empty_local_runqueue();
#endif
        /*
         * For platform which can hold the processor, it can be held here.
         * for platform with static interrupt distribution capability,
         * interrupts should be re-distributed to other engines.
         */
        psm_offline_self();

	/*
	 * Let the HAT know we're no longer available.
	 */
	hat_offline();

	/*
	 * Disable all interrupts.
	 */
	splhi();

	/*
	 * Let the dispatcher know we're no longer available.
	 */
	dispoffline(eng);

	/*
	 * Flush out local KMA buffers.
	 */
	kma_offline_self();

	/*
	 * Update counts of CPU/FPA types of interest.
	 */
	if (l.cpu_id == CPU_386)
		n_i386_online--;
#ifdef WEITEK
	if (l.weitek_kind == WEITEK_HW)
		n_weitek_online--;
#endif /* WEITEK */

	EVENT_SIGNAL(&eng_wait, 0);

	/*
	 * Switch off the leds to indicate that the processor is offline.
	 */
	psm_ledctl(LED_OFF, ONLINE_LED);

	while (eng->e_flags & E_SHUTDOWN)
		continue;

	/*
	 * Update counts of CPU/FPA types of interest.
	 */
	if (l.cpu_id == CPU_386) {
		n_i386_online++;
		/*
		 * If we're a 386 and any user-writes are in progress which
		 * aren't using the 386 workaround, we can't come online
		 * until they're finished.
		 */
		while (ATOMIC_INT_READ(&n_user_write) != 0)
			;
	}
#ifdef WEITEK
	if (l.weitek_kind == WEITEK_HW)
		n_weitek_online++;
#endif /* WEITEK */

	disponline(eng);
	hat_online();

	/*
	 * Signal that online is complete.
	 * MUST NOT come before disponline().
	 */
	EVENT_SIGNAL(&eng_wait, 0);

#ifndef NODEBUGGER
	/*
	 * Let kernel debugger know we're here.
	 */
	(*cdebugger) (DR_ONLINE, NO_FRAME);
#endif

        /*
         * All necessary platform-specific setup. For example, interrupt
         * re-distribution to non-boot engine.
         */
        psm_misc_init();

	/*
	 * Switch on the leds to indicate that the processor is online.
	 */
	psm_ledctl(LED_ON, ONLINE_LED);
}


/*
 * STATIC void
 * mem_vmapin(pte_t *pt, uint_t npages, addr_t vaddr, int prot)
 *
 *	Duplicately map the specified virtual pages.
 *
 * Calling/Exit State:
 *
 *	"vaddr" is the virtual address of "npages" virtually contiguous
 *	pages which are to be duplicately mapped using the array of
 *	pte's, "pt".
 *
 *	Pages are MMU_PAGESIZE bytes long.
 */
STATIC void
mem_vmapin(pte_t *pt, uint_t npages, vaddr_t vaddr, int prot)
{
	int	i;

	for (i = 0; i < npages; i++) {
		pt++->pg_pte = mkpte(prot, pfnum(kvtophys(vaddr)));
		vaddr += MMU_PAGESIZE;
	}
}


/*
 * STATIC void
 * setup_priv_maps(int procid, int master_procid)
 *
 *	Setup page tables for the specified processor
 *	based on the specified master processor.
 *
 * Calling/Exit State:
 *
 *	procid is the processor ID to setup.
 *	master_procid is the processor ID to clone.
 *	Call only after disabling calloc().
 *
 * Description:
 *
 *	Shared page mappings are copied from the master.
 *	Per-engine mappings are setup appropriately.
 */
STATIC void
setup_priv_maps(int procid, int master_procid)
{
	struct	ppriv_pages *mppriv;
	struct	ppriv_pages *ppriv;
	pte_t	*kl1pt;
	pte_t	*pmap;

	if (procid == master_procid)
		return;		/* already setup */

	mppriv = engine[master_procid].e_local;	/* master ppriv */
	ppriv = engine[procid].e_local;		/* ppriv to setup */

	kl1pt = &ppriv->pp_kl1pt[0][0];
	pmap  = &ppriv->pp_pmap[0][0];

	/*
	 * Grab a copy of the master level 1 & 2 page tables
	 */

	bcopy((char *)&mppriv->pp_kl1pt[0][0], (char *)kl1pt, MMU_PAGESIZE);
	bcopy((char *)&mppriv->pp_pmap[0][0],  (char *)pmap,  MMU_PAGESIZE);

	/*
	 * Overwrite the per-engine level 1 & 2 entries.
	 *
	 * Note that these mappings must agree with those in build_table().
	 *
	 * We can't merely loop over the pages in struct ppriv_pages
	 * establishing consecutive mappings for each consecutive page
	 * since there exist gaps in kernel virtual space between some
	 * of the pages.
	 *
	 * Note:  KVUENG must be mapped user readable for the 80386
	 * B1-step Errata #13 workaround.  See the KVUENG comment near
	 * the similar mapping call in build_table().
	 *
	 * Note:  KVPLOCAL must be mapped user read-write for the
	 * floating-point emulator.  See the KVPLOCAL comment near
	 * the similar mapping call in build_table().
	 */

	kl1pt[ptnum(KVPER_ENG)].pg_pte = 
		 mkpte(PG_US | PG_RW | PG_V, pfnum(kvtophys((vaddr_t)pmap)));

	mem_vmapin(&pmap[pgndx(KVPLOCAL)], PL_PAGES,
			(vaddr_t) &ppriv->pp_local[0][0], PG_US | PG_RW | PG_V);
	mem_vmapin(&pmap[pgndx(KVPLOCALMET)],
			PLMET_PAGES * (PAGESIZE / MMU_PAGESIZE),
			(vaddr_t) &ppriv->pp_localmet[0][0], PG_RW | PG_V);
	mem_vmapin(&pmap[pgndx(KVENG_L2PT)], 1,
			(vaddr_t) &ppriv->pp_pmap[0][0], PG_RW | PG_V);
	mem_vmapin(&pmap[pgndx(KVENG_L1PT)], 1,
			(vaddr_t) &ppriv->pp_kl1pt[0][0], PG_RW | PG_V);
	mem_vmapin(&pmap[pgndx(KVUVWIN)], 1,
			(vaddr_t) &ppriv->pp_uvwin[0][0], PG_RW | PG_V);
	mem_vmapin(&pmap[pgndx(UVUVWIN)], 1,
			(vaddr_t) &ppriv->pp_uvwin[0][0], PG_US | PG_V);
	mem_vmapin(&pmap[pgndx(KVUENG)], USIZE * (PAGESIZE / MMU_PAGESIZE),
		   (vaddr_t)&ppriv->pp_ublock[0][0], PG_US | PG_RW | PG_V);
	mem_vmapin(&pmap[pgndx(KVUENG_EXT)], KSE_PAGES,
		   (vaddr_t)&ppriv->pp_uengkse[0][0], PG_RW | PG_V);
}

STATIC struct segment_desc tmp_gdt[GDTSZ];
STATIC struct gate_desc tmp_idt[IDTSZ];
STATIC struct tss386 tmp_tss;

/*
 * STATIC void
 * tmp_init_desc_tables(void)
 *
 *	Setup enough of a GDT to allow IDT to be established.
 *
 * Calling/Exit State:
 *
 *	Called immediately after enabling the MMU.  This routine allows
 *	the kernel to detect various faults/traps in a relatively graceful
 *	manner.
 */
STATIC void
tmp_init_desc_tables(void)
{
	struct idt_init *id;
	struct segment_desc *sd;
	struct gate_desc *gd;
	struct desctab desctab;

	/*
	 * Build GDT
	 */
	sd = &tmp_gdt[seltoi(KCSSEL)];
	BUILD_MEM_DESC(sd, 0, SD_MAX_SEG, KTEXT_ACC1, TEXT_ACC2);
	sd = &tmp_gdt[seltoi(KDSSEL)];
	BUILD_MEM_DESC(sd, 0, SD_MAX_SEG, KDATA_ACC1, DATA_ACC2);
	sd = &tmp_gdt[seltoi(KTSSSEL)];
	BUILD_SYS_DESC(sd, &tmp_tss, sizeof(struct tss386), TSS3_KACC1,
								TSS_ACC2);

	/*
	 * Build IDT
	 */
	for (gd = tmp_idt; gd < &tmp_idt[IDTSZ]; gd++)
		BUILD_GATE_DESC(gd, KCSSEL, t_res, GATE_386INT, GATE_KACC, 0);

	for (id = idt_init; id->idt_addr; id++) {
		gd = &tmp_idt[id->idt_desc];
		BUILD_GATE_DESC(gd, KCSSEL, id->idt_addr, id->idt_type,
							  id->idt_priv, 0);
	}

	id = psm_idt(myengnum);
	for (; id->idt_addr; id++) {
		gd = &tmp_idt[id->idt_desc];
		BUILD_GATE_DESC(gd, KCSSEL, id->idt_addr, id->idt_type,
							  id->idt_priv, 0);
	}

	/*
	 * Init GDTR and IDTR to start using new stuff.
	 */
	BUILD_TABLE_DESC(&desctab, tmp_idt, IDTSZ);
	loadidt(&desctab);

	BUILD_TABLE_DESC(&desctab, tmp_gdt, GDTSZ);
	loadgdt(&desctab);

	/*
	 * Initialize the segment registers.
	 */
	setup_seg_regs();
}

/*
 * STATIC void
 * init_desc_tables(void)
 *
 * Calling/Exit State:
 *
 *	Set up processor-local copy of GDT and IDT, and start using them.
 */
STATIC void
init_desc_tables(void)
{
	struct idt_init *id;
	struct segment_desc *sd;
	struct gate_desc *gd;

	/*
	 * Build GDT
	 */

	sd = &l.global_gdt[seltoi(KCSSEL)];
	BUILD_MEM_DESC(sd, 0, SD_MAX_SEG, KTEXT_ACC1, TEXT_ACC2);
	sd = &l.global_gdt[seltoi(KDSSEL)];
	BUILD_MEM_DESC(sd, 0, SD_MAX_SEG, KDATA_ACC1, DATA_ACC2);
	sd = &l.global_gdt[seltoi(KTSSSEL)];
	BUILD_SYS_DESC(sd, &l.tss, sizeof(l.tss), TSS3_KACC1, TSS_ACC2);
	sd = &l.global_gdt[seltoi(DFTSSSEL)];
	BUILD_SYS_DESC(sd, &l.dftss, sizeof(l.dftss), TSS3_KACC1, TSS_ACC2);
	sd = &l.global_gdt[seltoi(KLDTSEL)];
	BUILD_SYS_DESC(sd, global_ldt, LDTSZ * 8, LDT_KACC1, LDT_ACC2);

	/*
	 * Build IDT
	 */

	cur_idtp = l.std_idt;
	for (gd = cur_idtp; gd < &cur_idtp[IDTSZ]; gd++)
		BUILD_GATE_DESC(gd, KCSSEL, t_res, GATE_386INT, GATE_KACC, 0);

	/*
	 * Initialize IDT entries common to all platforms. In particular,
	 * traps and faults IDT entries are initialized.
	 */ 
	for (id = idt_init; id->idt_addr; id++) {
		gd = &cur_idtp[id->idt_desc];
		BUILD_GATE_DESC(gd, KCSSEL, id->idt_addr, id->idt_type,
							  id->idt_priv, 0);
	}

	/*
	 * Initialize IDT entries specific to a platform. In particular,
	 * device interrupts IDT entries are initialized.
	 */
	id = psm_idt(myengnum);
	for (; id->idt_addr; id++) {
		gd = &cur_idtp[id->idt_desc];
		BUILD_GATE_DESC(gd, KCSSEL, id->idt_addr, id->idt_type,
							  id->idt_priv, 0);
	}

	BUILD_GATE_DESC(&cur_idtp[DBLFLT], DFTSSSEL, 0, GATE_TSS, GATE_KACC, 0);

	def_intf0 = cur_idtp[0xF0];
	fpuon_noextflt = fpuoff_noextflt = cur_idtp[NOEXTFLT];

	/*
	 * Init GDTR and IDTR to start using new stuff.
	 */
	BUILD_TABLE_DESC(&std_idt_desc, cur_idtp, IDTSZ);
	loadidt(&std_idt_desc);

	BUILD_TABLE_DESC(&ueng.u_gdt_desc, l.global_gdt, GDTSZ);
	loadgdt(&ueng.u_gdt_desc);
	ueng.u_dt_infop[DT_GDT] = &global_dt_info[DT_GDT];

	/*
	 * Initialize the segment registers.
	 */
	setup_seg_regs();

	/*
	 * Fill out the TSS and load the TSS register.
	 * TSS was zeroed by selfinit() when the whole plocal structure
	 * got zapped.
	 */
	l.tss.t_ss0 = KDSSEL;
	l.tss.t_esp0 = (ulong_t)UBLOCK_TO_UAREA(KVUENG);
	l.tss.t_bitmapbase = TSS_NO_BITMAP;	/* no user I/O allowed */
	loadtr(KTSSSEL);

	/*
	 * Indicate that we're going to use the global LDT.
	 */
	ueng.u_dt_infop[DT_LDT] = &global_dt_info[DT_LDT];
	loadldt(KLDTSEL);

	/*
	 * Fill out the double-fault TSS.
	 * This TSS was zeroed by selfinit() when the whole plocal structure
	 * got zapped.  Don't set the stack pointer without checking if it
	 * has already been set; a kernel debugger may already have set it.
	 */
	if (l.dftss.t_esp == 0)
		l.dftss.t_esp = (ulong_t)UBLOCK_TO_UAREA(KVUENG);
	l.dftss.t_esp0 = l.dftss.t_esp;
	l.dftss.t_ss0 = l.dftss.t_es = l.dftss.t_ss = l.dftss.t_ds = KDSSEL;
	l.dftss.t_cs = KCSSEL;
	l.dftss.t_eip = (ulong_t)t_syserr;
	l.dftss.t_bitmapbase = TSS_NO_BITMAP;
	l.dftss.t_cr3 = _cr3();
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_kernel_addrs(void)
 *
 *	Print the value of misc. kernel virtual addresses for debugging.
 *
 * Calling/Exit State:
 *
 *	Intended to be called from a kernel debugger.
 */
void
print_kernel_addrs(void)
{
extern  char stext[];
#define X(v)	debug_printf("0x%x	"#v"\n", v)
	
	debug_printf("\n");

	X(KVTMPPG2);
	X(KVTMPPG1);
	X(KVMET);
	X(KVPLOCALMET);
	X(KVPLOCAL);
	X(KVENG_L2PT);
	X(KVENG_L1PT);
	X(KVUENG);
	X(KVUENG_REDZONE);
	X(KVUVWIN);
	X(UVUVWIN);
	X(KVFPEMUL);
	X(KVLAST_ARCH);
	
	debug_printf("\n");

	X(KVDISP_MONO);
	X(KVDISP_COLOR);
	X(KVWEITEK);
	X(KVPER_ENG);
	
	debug_printf("\n");

	X(KVBOOTINFO);
	X(stext);
	
	debug_printf("\n");

	X(KL2PTES);
	X(KL2PTES_SIZE);
	X(KL2PTESPTES);
	X(KVBASE);
	X(KVAVBASE);
	X(KVAVEND);
	
	debug_printf("\n");

}

#endif /* DEBUG || DEBUG_TOOLS */
