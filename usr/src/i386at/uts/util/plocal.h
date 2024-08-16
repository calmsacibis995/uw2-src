/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_UTIL_PLOCAL_H	/* wrapper symbol for kernel use */
#define	_UTIL_PLOCAL_H	/* subject to change without notice */

#ident	"@(#)kern-i386at:util/plocal.h	1.69"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * Definitions of per-processor data structures.
 */

#ifdef _KERNEL_HEADERS

#include <io/strsubr.h>		/* REQUIRED */
#include <mem/immu.h>		/* REQUIRED */
#include <mem/kma.h>		/* REQUIRED */
#include <mem/kmem.h>		/* REQUIRED */
#include <mem/vmmeter.h>	/* REQUIRED */
#include <proc/seg.h>		/* REQUIRED */
#include <proc/tss.h>		/* REQUIRED */
#include <svc/fp.h>		/* REQUIRED */
#include <util/emask.h>		/* REQUIRED */
#include <util/ipl.h> 		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/locktest.h>	/* REQUIRED */
#include <util/metrics.h>	/* REQUIRED */
#include <util/param.h>		/* REQUIRED */
#include <util/sysmacros.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/strsubr.h>	/* REQUIRED */
#include <sys/immu.h>		/* REQUIRED */
#include <vm/kma.h>		/* REQUIRED */
#include <sys/kmem.h>		/* REQUIRED */
#include <sys/vmmeter.h>	/* REQUIRED */
#include <sys/seg.h>		/* REQUIRED */
#include <sys/tss.h>		/* REQUIRED */
#include <sys/fp.h>		/* REQUIRED */
#include <sys/emask.h>		/* REQUIRED */
#include <sys/ipl.h> 		/* REQUIRED */
#include <sys/ksynch.h>		/* REQUIRED */
#include <sys/locktest.h>	/* REQUIRED */
#include <sys/metrics.h>	/* REQUIRED */
#include <sys/param.h>		/* REQUIRED */
#include <sys/sysmacros.h>	/* REQUIRED */
#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * The per-processor private data is allocated at boot time.
 * It consists of:
 *
 *		Data-Structure		Size (HW Pages)
 *		==============		===============
 *		Engine Private U-block	USIZE * (PAGESIZE / MMU_PAGESIZE)
 *		Stack Extension		KSE_PAGES
 *		LWP Private Data Page	1
 *		Local L1PT		KL1PT_PAGES
 *		Private Mapping Page	1
 *		Processor Local Data	whatever, rounded to page size
 *		Processor Local Metrics	whatever, rounded to page size
 *
 * A virtual address pointer to this is saved in the engine structure of
 * the processor.
 */

#if defined _KERNEL || defined _KMEMUSER

#ifndef NCPUFEATWORD
#define NCPUFEATWORD	27
#endif

/*
 * Processor Local Data.
 * Fields are addressed by "l." -- eg, l.eng_num.  See selfinit() for how this
 * is set up.
 */

struct plocal {
	/*
	 * Per-processor Global Descriptor Table (GDT) and Interrupt
	 * Descriptor Table (IDT) to avoid LOCK# contention during
	 * table access (80386 asserts LOCK# when accessing GDT or IDT
	 * even if table isn't being changed by HW).
	 *
	 * Placed at front of structure to insure 8-byte alignment (the
	 * structure is allocated page aligned).
	 */
	struct segment_desc	global_gdt[GDTSZ];
	struct gate_desc	std_idt[IDTSZ];
	/*
	 * Per-processor Task-State Segment (TSS) supplies SS0 and ESP0
	 * when enter kernel from user mode.  Allocated per-processor
	 * to avoid problems of "busy" TSS when turning protection on.
	 * (A shared system-wide TSS is marked BUSY when loaded into
	 * a processor TSS register; the type would have to be cleared
	 * while other processors may be accessing it.  Although this
	 * might work, it is better to keep it separate.)  Separate per-
	 * processor TSSs are also necessary due to floating ublocks,
	 * since each LWP needs its own ESP0 value.
	 */
	struct tss386	tss;
	/*
	 * Double-fault TSS.  When the CPU has trouble delivering an
	 * exception, it delivers a double-fault exception.  To increase
	 * the likelihood of being able to process this exception, we use
	 * a task gate to switch to a reserved task which is in a known
	 * state.  In particular, this lets us reset the kernel stack
	 * pointer, so we can panic cleanly after a kernel stack overflow.
	 */
	struct tss386	dftss;
	/*
	 * The following fields will be used to maintain per-cpu spin lock
	 * metrics.
	 *
	 * These fields MUST NOT have different offsets under different
	 * compilation options (particularly DEBUG), since they are accessed
	 * by special files which may be compiled with different options
	 * than the rest of the kernel.
	 */
	ulong_t		lk_stime;	/* Start time */
	ulong_t		lk_mxtime;	/* Max hold time */
	ulong_t		lk_depth;	/* lock nesting depth */
	ulong_t		lk_mxdepth;	/* Max lock nesting depth */
	void		*lk_lkpf;	/* pointer to first lock */
	void		*lk_retpcf;	/* ret pc from the first lock() */
	void		*lk_lkpl;	/* last lk pointer */
	void		*lk_retpcl;	/* ret pc from last unlock() */
	void		*lk_mxlkpf;	/* first lkp for max hold time */
	void		*lk_mxretpcf;	/* first ret pc for max hold time */
	ulong_t		lk_spare[4];	/* for future growth */
	/*
	 * The following fields will be used to maintain the per-cpu
	 * interrupt statistics. Stats will be maintained on a per-bin
	 * basis and we allocate an array intr_stats_t objects. For
	 * details on the intr_stats_t object refer to ipl.h.
	 *
	 * These fields MUST NOT have different offsets under different
	 * compilation options (particularly DEBUG), since they are accessed
	 * by special files which may be compiled with different options
	 * than the rest of the kernel.
	 */
	intr_stats_t	intr_stat[MAX_INTR_LEVELS];
	intr_stack_t	intr_stack;	/* to keep track of ipl */
	/*
	 * The following fields are used by _LOCKTEST code.
	 *
	 * These fields MUST NOT have different offsets under different
	 * compilation options (particularly DEBUG), since they are accessed
	 * by special files which may be compiled with different options
	 * than the rest of the kernel.
	 */
	hier_stack_t	hier_stack;	/* for lock hierarchy checking */
	boolean_t	holdfastlock;	/* are interrupts off? */
	void		*fspin;		/* held fspin_t (really fspin_t *) */
	/*
	 * Per-CPU event flags (runrun, etc.); defined in disp.h.
	 * Never modified from another CPU, so no locks are needed;
	 * however, to prevent interrupts in the middle of read-modify-write
	 * sequence, all places which modify eventflags must either have
	 * *all* interrupts disabled (including xcall), or must use atomic
	 * (bus-locked) modification operations.  In many places this is
	 * accomplished implicitly by holding the RUNQUE_LOCK fspin lock.
	 */
	uint_t		eventflags;
	/*
	 * Miscellaneous per-processor fields.
	 */
	long		microdata;      /* loop count for 10 microseconds */
	int		eng_num;	/* Which logical processor am I? */
	struct engine	*eng;		/* my engine table pointer */
	emask_single_t	eng_mask;	/* my engine mask */
	int		fpuon;		/* cr0 value to turn on FPU */
	int		fpuoff;		/* value to turn off FPU */
	boolean_t	usingfpu;	/* B_TRUE if FPU in use */
	boolean_t	fpu_external;	/* B_TRUE if a separate FPU chip */
	int		cpu_speed;	/* cpu mips rate */
	int		one_sec;	/* lclclock() one_sec "timer" */
	kmlocal_t	kmlocal[(KMEM_POOL_TYPES * NFIXEDBUF) + NVARBUF];
					/* local KMA pools */
	pte_array_t	ptesbuf[MAXL2_PTES];  /* for tlb shootdown */
	struct bclist	bcall;		/* bufcall list */
	struct qsvc	qsvc;		/* enabled service procedures */
	unsigned char	qbf[NBAND];	/* streams flow control */
	int		trap_err_code;	/* trap entries put HW err code here */
	paddr_t		panic_pt;	/* page table address */
	int		panic_level;	/* 1 = single panic, 2 = dbl panic */
	int		noproc;		/* set if we are in the switcher */
	pl_t		puselockpl;	/* pl for currently held p_uselock */
	pl_t		vmpageidlockpl; /* pl for id lock */
	int		intr_depth;	/* depth of interrupt nesting */
	pl_t		picipl;		/* PIC mask value */
	pl_t		plsti;		/* should sti be called while spinning
					   for a lock - for corollary */
	int		shadow_if;	/* if 0 (1), then don't (do) sti during
					 * FSPIN_UNLOCK/ENABLE */
	int		xclock_pending;	/* cross-processor clock pendings */
	int		prf_pending;	/* cross-processor profiler pending */
	uchar_t		special_lwp;	/* most recently running LWP needs
					 * special context-switch handling
					 * (see l_special in lwp.h) */
	struct fpemul_kstate  fpe_kstate; /* FP emulator state */

	/*
	 * This is the scratch area to save arguments to the
	 * use_private function.
	 */
	uint_t		argsave[3];	/* arg save area */

	/*
	 * Fields to support kernel preemption and gather preemption
	 * statistics.
	 *
	 * The following union combines three pieces of information
	 * in a single int for performance reasons.  It relies on
	 * the property that a struct consisting of an unsigned short
	 * and two characters will fit in a single int, and that the
	 * the fields are listed in order of significance, i.e.,
	 * least significant first.
 	 *
	 * In general, check_preemption should be called when the
	 * u_prmpt_state value is 0, which means that each of the
	 * fields of the s_prmpt_state structure are 0.  The fields
	 * of that structure are:
	 *
	 *	s_prmpt_count, which is incremented on entry to a code
	 *		segment for which preemption is disabled and
	 *		decremented on exit from that code segment.
	 *		Preemption is allowed only when s_prmpt_count
	 *		is 0.
	 *
	 *	s_noprmpt, which is 1 when there is no kernel preemption
	 *		pending, and 0 when there is a kernel preemption
	 *		pending.
	 *
	 *	s_ipl, which stores the system interrupt priority level.
	 *		Preemption is allowed only when s_ipl is PLBASE,
	 *		or 0.
	 */
	union {
		int u_prmpt_state;
		struct {
			ushort_t s_prmpt_count;	/* control the preemptability */
			uchar_t s_noprmpt;	/* 0 if kernel preempt set */
			k_pl_t s_ipl;		/* current pl */
		} s_prmpt_state;
	} prmpt_state;
	long s_pad;				/* padding to allow long
						 * accesses to ipl - temporary
						 */
#ifdef _MPSTATS
	ulong_t	prmpt_max;	/* max dispatch latency. */
	ulong_t	prmpt_total;	/* a running sum of dispatch latencies. */
	ulong_t	prmpt_kern;	/* # of times kernel got preempted. */
	ulong_t prmpt_user;	/* # of times user got preempted. */
	ulong_t prmpt_cs;	/* # of preemption based context switches. */
#endif /* _MPSTATS */

	void (*nmi_handler)();		/* Adapter recovery function; NULL if
					 * no handler currently in use.
					 */
	struct user *userp;		/* Pointer to the u area this engine
					 * is currently executing on.
					 */
	struct proc *procp;		/* Pointer to current proc structure
					 * associated with userp
					 */
	/*
	 * CPU and support chip identification.
	 */
	int cpu_id;			/* CPU type (CPU_386, etc.) */
	int cpu_model;			/* Model # w/in the CPU type */
	int cpu_stepping;		/* Chip stepping (STEP_386B1, etc.) */
	int cpu_features[NCPUFEATWORD];	/* CPU feature bits */
#ifdef WEITEK
	int weitek_kind;		/* WEITEK_NO or WEITEK_HW */
#endif

	pte_t	kse_pte;	/* Stack extension page PTE for this engine */

	struct gate_desc *idtp;		/* Current IDT */
#ifdef MERGE386
	void *vm86_idtp;	/* IDT pointer for MERGE386 */
#endif

	int disable_locktest;	/* Disable locktest checks if non-zero */
};

/*
 * There must be an integral # pages allocated to the plocal structure,
 * so it can be mapped separate from other data-structures.
 */

#define	PL_PAGES	mmu_btopr(sizeof(struct plocal))

/*
 * Processor private pages layout.
 *
 * This just provides a simple way to allocate and locate the
 * per-engine memory during system initialization (boot).
 *
 * Must be allocated on a page boundary since all fields are treated
 * as pages of memory.  However, there is no need that the structure
 * members occur in any particular order within the structure;
 * specifically, there is no need that they correspond to the order
 * that they are mapped into the kernel virtual address space.
 */

struct	ppriv_pages {
	char	pp_localmet[PLMET_PAGES][PAGESIZE]; /* misc vars */
	char	pp_ublock[USIZE][PAGESIZE];	    /* idle kstack & uarea */
	char	pp_uengkse[KSE_PAGES][MMU_PAGESIZE];/* ueng stack extension */
	char	pp_kse[KSE_PAGES][MMU_PAGESIZE];    /* stack extension */
	char	pp_uvwin[1][MMU_PAGESIZE];	    /* user-visible window */
	pte_t	pp_kl1pt[KL1PT_PAGES][NPGPT];	    /* idle/private L1PT */
	pte_t	pp_pmap[1][NPGPT];		    /* private L2 mapping */
	char	pp_local[PL_PAGES][MMU_PAGESIZE];   /* misc vars */
};

#define	SZPPRIV	sizeof(struct ppriv_pages)

#endif /* _KERNEL || _KMEMUSER */

#ifdef _KERNEL

extern struct plocal l;

/* Macro to access arbitrary engines' plocal structures */
#define ENGINE_PLOCAL_PTR(engnum) \
	((struct plocal *)(void *)&engine[engnum].e_local->pp_local[0][0])

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _UTIL_PLOCAL_H */
