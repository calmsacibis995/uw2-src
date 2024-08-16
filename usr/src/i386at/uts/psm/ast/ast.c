/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/ast/ast.c	1.12"
#ident	"$Header: $"

/*
 * ast.c -- routines for manipulating the AST Manhattan hardware.
 *
 * Copyright (c) 1993, 1994 Microport, Inc.
 * All Rights Reserved
 *
 * Modification history:
 *
 * M001: fix to prevent panic on soft reboot and hang afterwards.
 * M002: fix to print message and panic if wrong EBI version
 * M003: now shuts power off on any init 0 or shutdown command
 *	 Defining PWROFF_IOCTL restores old behavior.
 * M004: call to astm_update_graph in psm_online_engine wasn't turning on
 * 	 LED because engine[i].e_flags doesn't get updated until after call.
 *	 Replaced call to astm_update_graph with code to turn on LED.
 */

#include <util/types.h>

#include <io/prf/prf.h>
#include <mem/immu.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <mem/hatstatic.h>
#include <svc/eisa.h>
#include <svc/pit.h>
#include <svc/psm.h>
#include <svc/bootinfo.h>
#include <svc/uadmin.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/processor.h>

#include <util/kdb/xdebug.h>
#include <io/cram/cram.h>
#include <io/ddi.h>

#include <ebi.h>
#include <ast.h>

#define	IO_ROM_INIT	    0xfff0
#define	IO_ROM_SEG	    0xf000
#define	RESET_FLAG	    0x1234
#define RESET_VECT      0x467
#define	SHUT_DOWN	    0x8f
#define	SHUT5		    5
#define FPU_BUSY_LATCH  0xf0
#define STAT_8042       0x64

#ifdef _A_SP_LOCKED
#undef _A_SP_LOCKED
#endif

#ifdef _A_SP_UNLOCKED
#undef _A_SP_UNLOCKED
#endif

#define _A_SP_LOCKED    1
#define _A_SP_UNLOCKED  0

/*
 * MACROS
 */
#define MMIONEED(x)  ((x).length)
#define MMIOSIZE(x)  ((x).length)
#define MMIOADDR(x)  ((x).address.low)
#define MMIOALLOC(x) ((x).flags & IO_ALLOCATE)

#define CALLOC(a)        calloc((a))
#define PHYSMAP(x, s)    physmap((x), (s), KM_NOSLEEP)

/* this is no longer defined in immu.h */
#define	ftop(x)	((((x) & 0xffff0000) >> 12) + ((x) & 0xffff))
/*
 * EXTERNALS
 */

/*
 * Interrupt entry points.
 */
extern void softint(void);
extern void devint0(void);
extern void devint1(void);
extern void devint2(void);
extern void devint3(void);
extern void devint4(void);
extern void devint5(void);
extern void devint6(void);
extern void devint7(void);
extern void devint8(void);
extern void devint9(void);
extern void devint10(void);
extern void devint11(void);
extern void devint12(void);
extern void devint13(void);
extern void devint14(void);
extern void devint15(void);
extern void devint16(void);
extern void devint17(void);
extern void devint18(void);
extern void devint19(void);
extern void devint20(void);
extern void devint21(void);
extern void devint22(void);
extern void devint23(void);
extern void devint24(void);
extern void devint25(void);
extern void devint26(void);
extern void devint27(void);
extern void devint28(void);
extern void devint29(void);
extern void devint30(void);
extern void devint31(void);
extern void devint32(void);
extern void devint33(void);
extern void devint34(void);
extern void devint35(void);
extern void devint36(void);
extern void devint37(void);
extern void devint38(void);
extern void devint39(void);
extern void devint40(void);
extern void devint41(void);
extern void devint42(void);
extern void devint43(void);
extern void devint44(void);
extern void devint45(void);
extern void devint46(void);
extern void devint47(void);
extern void devint48(void);
extern void devint49(void);
extern void devint50(void);
extern void devint51(void);
extern void devint52(void);
extern void devint53(void);
extern void devint54(void);
extern void devint55(void);
extern void devint56(void);
extern void devint57(void);
extern void devint58(void);
extern void devint59(void);
extern void devint60(void);
extern void devint61(void);
extern void devint62(void);
extern void devint63(void);
extern void devint64(void);
extern void devint65(void);
extern void devint66(void);
extern void devint67(void);
extern void devint68(void);
extern void devint69(void);
extern void devint70(void);
extern void devint71(void);
extern void devxcall(void);

extern int sanity_clk;			/* read by psm_reboot */
extern int ast_cache_on;		/* read by psm_selfinit(),
						ast_init_ebi() */
extern int xclock_pending;		/* read and set by psm_intr() */
extern int prf_pending;			/* read and set by psm_intr() */
extern uchar_t kvpage0[];		/* read by softreset() */
extern boolean_t soft_sysdump;		/* set by psm_misc_init() */
extern int ast_nmi(void);		/* read by psm_misc_init() */
extern void clkstart();			/* read by psm_timer_intr() */
extern void astpicinit();		/* called by psm_intr_init() */
extern void astpicstart();		/* called by psmintrstart() */
extern unsigned prfstat;		/* used in psm_intr() */
extern void ndisableint(int, pl_t, int, int); /* called by psm_introff() */
extern void nenableint(int, pl_t, int, int);  /* called by psm_intron() */
extern int ast_numproc_override;	/* used by psm_numeng() */
extern int ast_default_panel_mode;	/* used by psm_numeng() */
extern void reboot_prompt(int);		/* called by psm_reboot() */
/*
 * Generated by idtools.
 */
extern uchar_t intpri[];
extern pl_t svcpri[];
extern int (*ivect[])();
extern int intcpu[];
extern int intmp[];
extern int npic;

/*
 * GLOBAL DATA
 */

/*
 * for debug purposes
 */
#ifdef AST_DEBUG
int ast_xintr_sent[EBI_MAX_CPUS];
int ast_xintr_recv[EBI_MAX_CPUS];
#endif

unsigned int ast_cpuid[EBI_MAX_CPUS];

/*
 * set by SPI/NMI code.  To be used to communicate power fail 
 * signal, shutdown switch press and attention switch press to 
 * user level code.
 */
int          ast_event_code;
event_t      ast_event_sv;

STATIC volatile uint_t ast_eventflags[EBI_MAX_CPUS];

/*
 * common IDT structure for all engines in AST Manhattan advanced 
 * interrupt mode.
 */
struct idt_init ast_idt0_init[] = {
	{	SOFTINT,	    GATE_386INT,	softint,	GATE_KACC },
	{	DEVINTS,	    GATE_386INT,	devint0,	GATE_KACC },
	{	DEVINTS + 1,	GATE_386INT,	devint1,	GATE_KACC },
	{	DEVINTS + 2,	GATE_386INT,	devint2,	GATE_KACC },
	{	DEVINTS + 3,	GATE_386INT,	devint3,	GATE_KACC },
	{	DEVINTS + 4,	GATE_386INT,	devint4,	GATE_KACC },
	{	DEVINTS + 5,	GATE_386INT,	devint5,	GATE_KACC },
	{	DEVINTS + 6,	GATE_386INT,	devint6,	GATE_KACC },
	{	DEVINTS + 7,	GATE_386INT,	devint7,	GATE_KACC },
	{	DEVINTS + 8,	GATE_386INT,	devint8,	GATE_KACC },
	{	DEVINTS + 9,	GATE_386INT,	devint9,	GATE_KACC },
	{	DEVINTS + 10,	GATE_386INT,	devint10,	GATE_KACC },
	{	DEVINTS + 11,	GATE_386INT,	devint11,	GATE_KACC },
	{	DEVINTS + 12,	GATE_386INT,	devint12,	GATE_KACC },
	{	DEVINTS + 13,	GATE_386INT,	devint13,	GATE_KACC },
	{	DEVINTS + 14,	GATE_386INT,	devint14,	GATE_KACC },
	{	DEVINTS + 15,	GATE_386INT,	devint15,	GATE_KACC },
	{	DEVINTS + 16,	GATE_386INT,	devint16,	GATE_KACC },
	{	DEVINTS + 17,	GATE_386INT,	devint17,	GATE_KACC },
	{	DEVINTS + 18,	GATE_386INT,	devint18,	GATE_KACC },
	{	DEVINTS + 19,	GATE_386INT,	devint19,	GATE_KACC },
	{	DEVINTS + 20,	GATE_386INT,	devint20,	GATE_KACC },
	{	DEVINTS + 21,	GATE_386INT,	devint21,	GATE_KACC },
	{	DEVINTS + 22,	GATE_386INT,	devint22,	GATE_KACC },
	{	DEVINTS + 23,	GATE_386INT,	devint23,	GATE_KACC },
	{	DEVINTS + 24,	GATE_386INT,	devint24,	GATE_KACC },
	{	DEVINTS + 25,	GATE_386INT,	devint25,	GATE_KACC },
	{	DEVINTS + 26,	GATE_386INT,	devint26,	GATE_KACC },
	{ 0 },
};

/*
 * XXX: ast_calltab is referenced in ast_setpicmasks() defined in spl.s
 * any changes to the definition of this structure in ebi.h need to 
 * resolved against that file.  AST currently sez that no changes in
 * current structure size or offsets are planned, so this should not
 * be a problem.
 */
EBI_II ast_calltab;
void   **MMIOTable;

#ifdef PWROFF_IOCTL				/* M003 */
int	ast_shutdown_pwr;
#endif

struct	dispinfo ast_display;

STATIC int          ast_bios_jmp_table = 0;
STATIC unsigned int ast_bios_vbase = 0;

/* 
 * FUNCTION PROTOTYPES
 */
	/*
	 * PSM entry points: prototyped in psm.h
	 *
	 * void psm_configure(void);
	 * void psm_intr_init(void);
	 * void psm_intr_start(void);
	 * void psm_timer_init(void);
	 * void psm_online_engine(int , paddr_t , int );
	 * void psm_offline_self(void);
	 * void psm_send_xintr(int );
	 * void psm_ledctl(led_request_t , uint_t );
	 * int psm_numeng(void);
	 * void psm_intron(int , pl_t , int , int , int );
	 * void psm_introff(int , pl_t , int , int );
	 * int psm_doarg(char *);
	 * void psm_reboot(int);
	 * void psm_sendsoft(int, uint_t);
	 * void psm_selfinit(void);
	 * void psm_misc_init(void);
	 * void psm_time_get(psmtime_t *);
	 * void psm_time_sub(psmtime_t *, psmtime_t *);
	 * void psm_time_add(psmtime_t *, psmtime_t *);
	 * void psm_time_cvt(dl_t *, psmtime_t *);
	 * struct idt_init *psm_idt(int); 
	 */

	/* other public functions */
	void spi_intr(unsigned);
	void lsi_intr(unsigned);
	void psm_intr(uint, uint, uint, uint, uint, uint, uint,
		 uint, uint, uint);
	int ast_chk_spurious(int);
	void psm_do_softint(void);

	/* this is called by the astm driver. */
	void astm_update_graph(void);
#ifdef NOT_DEF	
	asm void __ast_fspin_lock(unsigned char *);
#endif

	/* private functions */
STATIC	void ast_configure(void);
STATIC 	void build_call_table(unsigned int *, unsigned int *);
STATIC 	void ast_init_ebi(void);
STATIC	void reset(void);		/* M001 */
STATIC	void halt_all(void);		/* M001 */
STATIC	void softreset(void);
STATIC	void ast_set_resetaddr(paddr_t );

/*
 * FUNCTION DEFINITIONS
 */

#ifdef NOT_DEF
/*
 * asm void
 * __ast_fspin_lock(unsigned char *lockp)
 * 	Like __fspin_lock except that it assumes that interrupts are blocked 
 *	at the cpu already, ie. we are at PLHI.
 *
 * Calling/Exit States:
 *	Called by: 	selfinit()
 *	Parameters:	none
 *	Locking: 	none
 *	Globals read:	ast_cache_on, ast_calltab, MMIOTable	
 *	Globs. written: none
 *	Returns:	none
 *
 * Remarks:
 *  These routines attempt to get a lock using the following assumptions:
 *
 *       (1) We are at PLHI (or interrupts have been disabled via a "cli"
 *           instruction) and therefore cannot have interrupted another 
 *	         instance of psm_send_xintr *on the current engine*.
 *       (2) psm_send_xintr is never called at anything other than PLHI.
 *           This is currently true, the xcall mechanism raises the level to
 *           to PLHI prior to calling psm_send_xintr.
 *  
 *  These routines are meant only for use in psm_send_xintr.  Use for any other
 *  purpose is done at your own risk.
 */
asm void
__ast_fspin_lock(unsigned char *lockp)
{
%ureg lockp; lab loop, spin, done;

	movb	$_A_SP_LOCKED,%al
loop: 
	xchgb	%al,(lockp)
	cmpb  $_A_SP_UNLOCKED,%al
	je    done
spin:
	cmpb  $_A_SP_UNLOCKED,(lockp)
	je    loop
	jmp   spin
done:

%mem lockp; lab loop, spin, done;
	movl	lockp,%edx		/ move &lock to known register
	movb	$_A_SP_LOCKED,%al
loop: 
	xchgb	%al,(%edx)
	cmpb	$_A_SP_UNLOCKED,%al
	je	done
spin: 
	cmpb	$_A_SP_UNLOCKED,(%edx)
	je	loop
	jmp	spin
done:
}
#pragma asm partial_optimization __ast_fspin_lock

asm void
__ast_fspin_unlock(unsigned char *lockp)
{
%ureg lockp

	/ Release the lock
	movb	$_A_SP_UNLOCKED,%al
	xchgb	%al,(lockp)

%mem lockp
	movl	lockp,%edx		/ move &lock to known location

	/ Release the lock
	movb	$_A_SP_UNLOCKED,%al
	xchgb	%al,(%edx)
}
#pragma asm partial_optimization __ast_fspin_unlock

#endif	/* ifdef NOT_DEF */

asm int
_efl(void)
{
	pushfl
	popl %eax
}
#pragma asm partial_optimization _efl

/*
 * void
 * psm_selfinit(void)
 *	Performs any necessay per-engine initialization.
 *	In the case of the AST,  enables RAM cache for this CPU.
 *
 *
 * Calling/Exit States:
 *	Called by: 	selfinit()
 *	Parameters:	none
 *	Locking: 	none
 *	Globals read:	ast_cache_on, ast_calltab, MMIOTable	
 *	Globs. written: none
 *	Returns:	none
 *
 * Description:
 *	Called from selfinit() when the engine is brought online
 *	for the very first time.
 *
 * Remarks:
 *      The internal on-chip cache is already enabled for the boot engine.
 */
void
psm_selfinit(void)
{
	outb(FPU_BUSY_LATCH, 0); /* Clear FPU BUSY latch */
	if (ast_cache_on) {
		(ast_calltab.EnableRAMCache)(MMIOTable);
	}
}

/*
 * STATIC void
 * build_call_table(unsigned int *offtab, unsigned int *calltab)
 *	Copy addresses of EBI functions from offset table to call table.
 *
 * Calling/Exit States:
 *	Called by: 	ast_init_ebi()
 *	Parameters:
 *		offtab		int pointer to beginning of offset table
 *		calltab 	int pointer to beginning of call table
 *	Locking: 	none
 *	Globals read:	ast_bios_vbase
 *	Globs. written: none
 *	Returns:	none
 *
 * Description:
 *	Adds virtual base address of EBI to each offset to calculate 
 *	virtual address of each EBI entry point, store in call table.
 */

STATIC void
build_call_table(unsigned int *offtab, unsigned int *calltab)
{
	int i;
	
	for (i = 0; i < sizeof(offsetTable) / sizeof(unsigned int); i++)
	{
		calltab[i] = offtab[i] + ast_bios_vbase;
	}
}

/*
 * STATIC void
 * ast_init_ebi(void)
 *	Set up call table (calltab), IO info table (IOinfotab), and
 *	IO table (MMIOTable) needed to make EBI access possible.
 *
 * Calling/Exit States:
 *	Called by: 	ast_configure()
 *	Parameters:	none
 *	Locking: 	none
 *	Globals read:	ast_calltab, ast_cache_on, ebi_initted, ast_bios_vbase,
 *				ast_bios_jmp_tbl, ast_cache_on
 *	Globs. written: ebi_initted, calltable
 *	Returns:	none
 *
 * Description:
 *
 * Remarks:
 *	Does nothing if called more than once.
 */
STATIC void
ast_init_ebi(void)
{
	static int ebi_initted = 0;
	unsigned int nslots;
	int i;
	unsigned int calltable;
	unsigned int *funcaddr = (unsigned int *)&ast_calltab;
	IOInfoTable *itab;
	revisionCode revcode;
	
	if (ebi_initted) 
		return;

	ebi_initted++;

	/*
	 * calculate the offset of the jump table contained within EBI 
	 * segment.
	 */
	calltable = ast_bios_vbase + (ast_bios_jmp_table - ftop(EBI_BIOS_SEG));
	
	/*
	 * Build the callable addresses for ast_calltab.
	 */
	build_call_table((unsigned int *)calltable, funcaddr);
	
	
	/*
	 * find out how many device slots we have.
	 */
	if (((ast_calltab.GetNumSlots)(&nslots)) != OK) 
	{
		cmn_err(CE_PANIC, "ast_init_ebi: Unable to get number of bus slots");
	}

	/*
	 * allocate a little space for the io info table.  
	 */
	itab = (IOInfoTable *)CALLOC(sizeof(IOInfoTable) * nslots);
	if (itab == NULL) 
	{
		cmn_err(CE_PANIC, "ast_init_ebi: Unable to allocate IOInfoTable");
	}

	/*
	 * Although the spec says that we only need to allocate
	 * enough space for "nslots", the example code supplied by
	 * AST shows an allocation for 32 slots worth.  We will
	 * do the same thing here just in case this is a bug in
	 * the EBI.
	 */
	MMIOTable = (void *)CALLOC(sizeof(void *) * 32);
	if (MMIOTable == NULL) 
	{
		cmn_err(CE_PANIC, "ast_init_ebi: Unable to allocate MMIOTable");
	}

	/*
	 * Get the information necessary to build the MMIOTable
	 * which we will use from here on out.
	 */
	if ((ast_calltab.GetMMIOTable(itab)) != OK) 
	{
		cmn_err(CE_PANIC, "ast_init_ebi: Unable to get MMIOTable");
	}

	for (i = 0; i < nslots; i++) 
	{
		if (MMIONEED(itab[i])) 
		{
			if (MMIOALLOC(itab[i]))
			{
				MMIOTable[i] = (void *)CALLOC(MMIOSIZE(itab[i]));
			} else {
				MMIOTable[i] = (void *)PHYSMAP(MMIOADDR(itab[i]), 
												MMIOSIZE(itab[i]));
			}
			
			if (MMIOTable[i] == NULL)
			{
				cmn_err(CE_PANIC, "ast_init_ebi: no memory for MMIOTable");
			}
		} 
	}
	
	if (((ast_calltab.InitEBI)(MMIOTable)) != OK)
	{
		cmn_err(CE_PANIC, "ast_init_ebi: Cannot initialize EBI II");
	}

	if (((ast_calltab.GetRevision)(MMIOTable, &revcode)) != OK)
	{
		cmn_err(CE_PANIC, "ast_init_ebi: Cannot get revision level");
	}

	if (revcode.major <= MIN_MAJOR && revcode.minor < MIN_MINOR) 
	{
		cmn_err(CE_PANIC, "ast_init_ebi: EBI rev %d.%d not supported", 
				revcode.major, revcode.minor);
	}

	/*
	 * If cache is on, disable it.
	 */
	if (!ast_cache_on) {
		(ast_calltab.DisableRAMCache)(MMIOTable);
	}	
}

/*
 * void
 * spi_intr(void intno)
 *	SPI interrupt service routine.  Sets event flags for
 *	front panel events: button presses, thermal condition, etc.
 *
 * Calling/Exit States:
 *	Called by: 	base kernel (interrupt context)
 *	Parameters:
 *		intno	interrupt number
 *	Locking: 	none
 *	Globals read:	ast_calltab, MMIOTable
 *	Globs. written: ast_event_code
 *	Returns:	none
 *
 * Description:
 *
 * Remarks:
 *
 * XXX: I don't understand the difference between the SPI, which is maskable
 * and the NMI which is non-maskable.  It seems to me as though they both 
 * inform the system of the same events.  How does one cause these events
 * to generate an SPI instead of an NMI?  Is there any difference in the
 * urgency of the problem indicated??
 *
 * Yea!! on looking through it turns out that everything except the really
 * bad things (BUS TIMEOUT, BUS PARITY, CPU ERROR, UNCORRECTABLE ECC ERROR)
 * will cause an SPI instead of an NMI.  Everything generates an NMI when
 * running in ISA or EISA interrupt mode.  We'll leave these possible sources
 * here in the SPI handler also since the EBI II spec is not clear on 
 * whether or not these sources can also generate an SPI.  The EBI II spec
 * seems to indicate that only the software generated NMI cannot be an SPI
 * source also.
 */
void
spi_intr(unsigned intno)
{
	memoryErrorInfo mei;
	unsigned int source;
	char *s;
	
	if ((ast_calltab.GetSPISource)(MMIOTable, &source) == ERR_UNKNOWN_INT) 
	{
		(ast_calltab.MaskableIntEOI)(MMIOTable, intno);
		return;
	}

	(ast_calltab.MaskableIntEOI)(MMIOTable, intno);

	switch (source) 
	{
		case INT_IO_ERROR:
			s = "system I/O error";
			break;
		case INT_MEMORY_ERROR:
			s = "uncorrectable ECC error";
			if ((ast_calltab.GetMemErrorInfo)(MMIOTable, &mei) == 
				MEMORY_ERROR_FOUND)
			{
				cmn_err(CE_CONT, "       AST SPI: memory error at 0x%x\n", 
						mei.location.low);
				cmn_err(CE_CONT, "                slot number     0x%x\n", 
						mei.slotNumber);
				cmn_err(CE_CONT, "                module number   0x%x\n", 
						mei.moduleNumber);
			}
			cmn_err(CE_PANIC, "AST SPI: uncorrectable ECC error");
			break;
		case INT_CPU_ERROR:
			s = "CPU error";
			break;
		case INT_POWER_FAIL:
			ast_event_code = EBI_EVENT_PWR_FAIL;
			EVENT_BROADCAST(&ast_event_sv, 0);
			cmn_err(CE_WARN, "AST SPI: !!!Power Failure Detected!!!");
			return;
			/*NOTREACHED*/
			break;
		case INT_BUS_ERR:
			s = "system bus address/parity error";
			break;
		case INT_BUS_TIMEOUT:
			s = "system bus timeout";
			break;
		case INT_SHUTDOWN:
			ast_event_code = EBI_EVENT_SHUTDOWN;
			EVENT_BROADCAST(&ast_event_sv, 0);
			return;
			/*NOTREACHED*/
			break;
		case INT_ATTENTION:
#ifdef AST_DEBUG
			calldebug();
#endif
			ast_event_code = EBI_EVENT_ATTENTION;
			EVENT_BROADCAST(&ast_event_sv, 0);
			return;
			/*NOTREACHED*/
			break;
		default:
			s = "unknown SPI source";
			break;
	}
	cmn_err(CE_PANIC, "AST SPI: source is %d: %s.", source, s);
}

/*
 * void
 * lsi_intr(unsigned int)
 *
 * Calling/Exit States:
 *	Called by:	base kernel (interrupt context)
 *	Parameters:
 *		intno	interrupt number
 *	Locking: 	none
 *	Globals read:	none
 *	Globs. written: none
 *	Returns:	none
 *
 * Description:
 * 	Since we won't be using the LSI in this implementation, we'll just ACK
 *	it and print a message indicating that we received an unexpected LSI.
 *
 * Remarks:
 */
void
lsi_intr(unsigned int intno)
{
	(ast_calltab.MaskableIntEOI)(MMIOTable, intno);
	cmn_err(CE_WARN, "AST LSI: unexepected LSI");
}


/*
 * int
 * lsi_intr(unsigned int)
 * 	Check for a spurious interrupt and ack it.
 *
 * Calling/Exit States:
 *	Called by:	spl.s
 *	Parameters:
 *		intno	interrupt number
 *	Locking: 	none
 *	Globals read:	ast_calltab, MMIOTable, ast_cpuid[]
 *	Globs. written: none
 *	Returns:
 *		0	if not spurious or able to EOI
 *		-1	if EBI call to EOI spurious interrupt fails
 *
 * Description:
 *
 * Remarks:
 * 	Because of the overhead associated with the spurious interrupt
 * 	check, it is not recommended that 8250 or other types of devices
 * 	with small receive buffers be configured on this interrupt.
 *
 */
int
ast_chk_spurious(int intno)
{
	unsigned int requested;
	unsigned int inservice;
	
	/*
	 * we know what the ADI subsystem looks like (two 8259 pics)
	 * so we check for the irq's to which the spurious interrupts will
	 * be reflected.  If intno is not one of these then we bail because
	 * the interrupt cannot be spurious.
	 */
	if (intno != 7 && intno != 15)
	{
		return(0);
	}

	/*
	 * call EBI to get IRQ status bit map
	 */
	if ((ast_calltab.GetLocalIRQStatus)(MMIOTable, ast_cpuid[myengnum], 
										&inservice, &requested) != OK)
	{
		cmn_err(CE_PANIC, "ast_adi_pic_spurious: cannot get pic status");
	}
	
	/*
	 * check to see if the bit is set.  if it is then it is a real
	 * interrupt, otherwise it is spurious.
	 */
	if (inservice & IRQBIT(intno))
	{
		return(0);
	}

	if ((ast_calltab.SpuriousIntEOI)(MMIOTable) != OK)
	{
		cmn_err(CE_PANIC, "ast_chk_spurious: cannot EOI spurious int");
	}
	return(-1);
}

#define str(s)  #s
#define xstr(s) str(s)

/*
 * void
 * psm_do_softint(void)
 *	Send a low_priority inter-processor interrupt.
 *
 * Calling/Exit State:
 *	Called by:	base kernel (PSM entry point)
 *	Parameters:	none
 *	Locking: 	none
 *	Globals read:	none
 *	Globs. written: none
 *	Returns:	none
 *
 * Remarks:
 *	User preemption flag is set if either of the following events
 *	are pending:
 *		- streams service procedures
 *		- local callouts
 *		- global callouts
 *		- runrun flag is set
 */
void
psm_do_softint(void)
{
        asm("int  $" xstr(SOFTINT));
}

/*
 * void
 * psm_send_xint(int engnum)
 * 	Send a cross-cpu interrupt to "engnum"
 *
 * Calling/Exit State:
 *	Called by:	psm_send_soft()
 *	Parameters:
 *		engnum		processor number to send xint
 *	Locking:
 * 		This code is currently called only at PLHI or with interrupts
 * 		disabled via "cli".
 *	Globals read:	none
 *	Globs. written: none
 *	Returns:	none
 *
 * Remarks:
 *	Currently, __ast_fspin_lock() is not needed in this routine
 *	because it is alway called at plhi or with CLI.
 * 	See the comments for psm_sendsoft and __ast_fspin_lock for the
 * 	caveats about using __ast_fspin_lock().
 */
/*ARGSUSED*/
void
psm_send_xintr(int engnum)
{
#ifdef NOT_DEF
	static unsigned char ast_IPI_lock = _A_SP_UNLOCKED;
#endif
	
	/*
	 * this routine is always called w/interrupts disabled at the chip
	 * or at PLHI.
	 */
	ASSERT(((_efl() & 0x200) == 0) || (getpl() == plhi));
	
#ifdef NOT_DEF
	__ast_fspin_lock(&ast_IPI_lock);
#endif

#ifdef AST_DEBUG
	ast_xintr_sent[engnum]++;
#endif

	if ((ast_calltab.GenIPI)(MMIOTable, IPI_ID(engnum)) != OK)
	{
#ifdef NOT_DEF
		__ast_fspin_unlock(&ast_IPI_lock);
#endif
		cmn_err(CE_PANIC, "ast: unable to send xintr to %d", engnum);
	}
#ifdef NOT_DEF
	__ast_fspin_unlock(&ast_IPI_lock);
#endif
}


/*
 * void
 * psm_doarg(char *s)
 *	Process an argument to the PSM.  Not currently supported.
 *
 * Calling/Exit State:
 *	Called by:	base kernel (PSM entry point)
 *	Parameters:
 *		s	pointer to argument string
 *	Locking:	none
 *	Globals read:	none
 *	Globs. written: none
 *	Returns:	always returns -1
 *
 * Remarks:
 */
/*ARGSUSED*/
int
psm_doarg(char *s)
{
	return(-1);
}

/*
 * psm support routines.
 */

/*
 * void
 * ast_configure(void)
 *	Map in the AST Extended Bios Interface (EBI).
 *
 * Calling/Exit State:
 *	Called by:	psm_intr_init()
 *	Parameters:	none
 *	Locking:	none
 *	Globals read:	ast_bios_vbase
 *	Globs. written: ast_bios_jmp_table
 *	Returns:	none
 *
 * Description:
 *	1. Map in the EBI segment. 
 *	2. Verify version signature.  Panic if not EBI II.
 * 	3. Clear FPU busy latch.
 *	4. Call ast_init_ebi() to initialize the EBI.
 */
void
ast_configure(void)
{
	ebi_iiSig *sigp;
	int bad_bios = 0;	/* M002 */
	
	/*
	 * map in the AST EBI BIOS segment.
	 */
	ast_bios_vbase = (unsigned int) PHYSMAP(ftop(EBI_BIOS_SEG), EBI_BIOS_SIZ + 1);
	if (ast_bios_vbase == (unsigned int) NULL)
	{
		cmn_err(CE_PANIC, "Cannot map EBI BIOS!!!!!!!");
	}

    	/*
	 * calculate the virtual address of the EBI signature.
    	 */
	    sigp = (ebi_iiSig *)(ast_bios_vbase + (ftop(EBI_SIG_LOC) - ftop(EBI_BIOS_SEG)));

	/*
	 * Let's see if it supports EBI2
	 */
	if (sigp->sig[0] != 'E') bad_bios++;
	if (sigp->sig[1] != 'B') bad_bios++;    
	if (sigp->sig[2] != 'I') bad_bios++;  
	if (sigp->sig[3] != '2') bad_bios++;    

   	if (bad_bios)					/* M002 */
		cmn_err(CE_PANIC, 
			"ast_configure: EBI2 BIOS signature not present");

	/*
	 * now get the physical address of the EBI II call table offsets.
	 */
	ast_bios_jmp_table = ftop((unsigned int)(((sigp->seg) << 16) | (sigp->off)));

	outb(FPU_BUSY_LATCH, 0); /* Clear FPU BUSY latch */

	/*
	 * now we configure the EBI...
	 */
	ast_init_ebi();

	return;
}

/*
 * void
 * psm_configure(void)
 *	Get front panel alpha-numeric display type and size.
 *
 * Calling/Exit State:
 *	Called by:	PSM
 *	Parameters:	none
 *	Locking:	none
 *	Globals read:   ast_calltab, MMIOTable, ast_display
 *	Globs. written:
 *	Returns:	none
 *
 * Description:
 *
 * Remarks:
 * 	The work that should have been done here is really done
 * 	in ast_configure which is called from psm_intr_init(). 
 *	this was necessary because the EBI must be initialized 
 *	before the interrupt subsystem can be initialized.
 */
void
psm_configure(void)
{

	(ast_calltab.GetPanelAlphaNumInfo)(MMIOTable,
		&ast_display.type, &ast_display.size);
	
	return;
}


/*
 * struct idt_init *
 * psm_idt(int engnum)
 * 	Returns pointer to idt_init table.
 *
 * Calling/Exit State:
 *	Called by:	base kernel (PSM entry point)
 *	Parameters:
 *		engnum		processor number
 *	Locking:	none
 *	Globals read:   ast_idt0_init
 *	Globs. written: none
 *	Returns:	none
 *
 * Description:
 *
 * Remarks:
 * 	We use the same idt for all processors since all of them can handle
 * 	interrupts in the normal way.
 */
/*ARGSUSED*/
struct idt_init *
psm_idt(int engnum)
{
	return(ast_idt0_init);
}

/*
 * int
 * psm_numeng(void)
 *	Returns that number of processors that are operational.
 *
 * Calling/Exit State:
 *	Called by:	base kernel (PSM entry point)
 *	Parameters:	none
 *	Locking:	none
 *	Globals read:   ast_calltab, MMIOTable, ast_numproc_override
 *	Globs. written: none
 *	Returns:	number of processors operational
 *
 * Description:
 *	1.  Ask EBI for number of installed CPUs.
 *	2.  Subtract any that failed to initialize.
 *	3.  Turn off all LEDs in graph display, and set
 *	    graph mode to default.
 *	4.  Log the calling processor as busy.
 *	5.  Display message announcing number of CPUs available.
 *
 * Remarks:
 *	If UNIPROC is defined,  this function always returns the value
 *	of ast_numproc_override (normally 1).
 */
int
psm_numeng(void)
{
	unsigned int numprocs = 0;
	int i, badcount = 0;
	procConfigData procinfo;

	if ((ast_calltab.GetNumProcs)(MMIOTable, &numprocs) != OK) 
	{
		cmn_err(CE_WARN, 
				"ast_findcpus: unable to get number of processors, assuming 1");
		numprocs = 1;
	}

	/*
	 * find out if any of the cpu's failed startup diagnostics.
	 */
	for (i = 0; i < numprocs; i++)
	{
		switch ((ast_calltab.GetProcConf)(MMIOTable, i, &procinfo)) 
		{
			case ERR_BAD_PROC_ID:
			case ERR_PROC_BAD:
				badcount++;
				break;
			default:
				if (procinfo.processorStatus == PS_FAULTY)
				{
					badcount++;
				}
				else if (procinfo.processorStatus == PS_ABSENT) 
				{
					badcount++;
				}
				break;
		}
	}

	/*
	 * adjust the processor count.
	 */
	numprocs -= badcount;

	/*
	 *  Clear the LED graph display and set the graph mode to default
	 */
	(ast_calltab.SetPanelProcGraphMode)(MMIOTable, PANEL_MODE_OVERRIDE);
	(ast_calltab.SetPanelProcGraphValue)(MMIOTable, 0);
	(ast_calltab.SetPanelProcGraphMode)(MMIOTable, ast_default_panel_mode);
	(ast_calltab.LogProcBusy)(MMIOTable);
	/*
	 * Let the EBI know that this the processor is busy
	 */

	cmn_err(CE_CONT, "AST Manhattan SMP: %d processors\n", numprocs);

#ifndef UNIPROC
	if (ast_numproc_override) 
	{
		return(ast_numproc_override);
	}
	return(numprocs);
#else
	return(1);
#endif
}

/*
 * void
 * psm_misc_init(void)
 * 	Perform miscellaneous initialization after engine has been 
 *	started and interrupt subsystem for that engine has been started.
 *
 * Calling/Exit State:
 *	Called by:	base kernel (PSM entry point)
 *	Parameters:	none
 *	Locking:	none
 *	Globals read:   ast_calltab, MMIOTable, ast_display, myengnum, 
 *				extern soft_sysdump
 *	Globs. written: ast_event_sv
 *	Returns:	none
 *
 * Description:
 *
 * Remarks:
 *
 * in the future we will distribute interrupts which should be bound
 * to a processor other than BOOTENG here.
 *
 * since secondary cpu's (cpu's other than the boot processor) start up
 * with their cache disabled, we enable the cache here.
 */
void
psm_misc_init(void)
{

    
	if (IS_BOOT_ENG(myengnum))
	{
		/*
		 * Set front panel mode -- we will support reboot via
		 * the off switch and will put the machine into single
		 * user mode the attention switch.
		 *
		 * set the UPS LED to green; we will change it to red 
		 * if we get a power fail interrupt.
		 */
		(ast_calltab.SetPanelUPS)(MMIOTable, UPS_LED_GREEN);
		(ast_calltab.SetPanelSwitchVisibility)(MMIOTable, PANEL_SWITCH_VISIBLE);
		(ast_calltab.SetPanelOffSwitchMode)(MMIOTable, OFF_SWITCH_SOFT);

		EVENT_INIT(&ast_event_sv);

		drv_callback(NMI_ATTACH, ast_nmi, NULL);
	}

	soft_sysdump = B_TRUE;
}

/*
 * void
 * psm_timer_init(void)
 *	Initialize the programmable interrupt timer.
 *
 * Calling/Exit State:
 *	Called by:	base kernel (PSM entry point)
 *	Parameters:	none
 *	Locking:	none
 *	Globals read:   none
 *	Globs. written: none
 *	Returns:	none
 *
 * Description:
 *
 * Remarks:
 * 	The system clock.  only the boot engine will receive clock 
 * 	interrupts.  these will be distributed via the xcall mechanism
 * 	to other cpu's.
 */
void
psm_timer_init(void)
{
	if (IS_BOOT_ENG(myengnum))
	{
		/* 
		 * initialize the i8254 PIT.
		 */
		clkstart();
	}
}


/*
 * void
 * psm_time_get(psmtime_t *)
 *	Save the current time stamp that is opaque to the base kernel
 *	  in psmtime_t.	  
 *
 * Calling/Exit State:
 *	Called by:	base kernel (PSM entry point)
 *	Parameters:	
 *		ptime -- pointer to time structure
 *	Locking:	none
 *	Globals read:   ast_calltab, MMIOTable
 *	Globs. written: none
 *	Returns:	none
 *
 * Calling/Exit State:
 */
void
psm_time_get(psmtime_t *ptime)
{
	unsigned int timerval;
	
	if ((ast_calltab.GetSysTimer)(MMIOTable, &timerval) != OK)
	{
		cmn_err(CE_WARN, "Could not get usec counter");
	}
        ptime->pt_lo = timerval;
        ptime->pt_hi = 0;
}


/*
 * void psm_time_sub(psmtime_t *dst, psmtime_t *src);
 * 	Subtracts the time stamp `src' from `dst', and 
 *	stores the result in `dst'.
 *
 * Calling/Exit State:
 *	Called by:	base kernel (PSM entry point)
 *	Parameters:	
 *		dst -- pointer to time structure to subtract from
 *			and to receive result
 *		src -- pointer to time structure to be subtracted
 *	Locking:	none
 *	Globals read:   none
 *	Globs. written: none
 *	Returns:	none
 */
void
psm_time_sub(psmtime_t *dst, psmtime_t *src)
{
        dst->pt_lo -= src->pt_lo;
}


/*
 * void psm_time_add(psmtime_t *dst, psmtime_t *src);
 * 	Add the time stamp `src' to `dst', and stores the result in `dst'.
 *
 * Calling/Exit State:
 *	Called by:	base kernel (PSM entry point)
 *	Parameters:	
 *		dst -- pointer to time structure to add and to receive result
 *		src -- pointer to time structure to add
 *	Locking:	none
 *	Globals read:   none
 *	Globs. written: none
 *	Returns:	none
 */
void
psm_time_add(psmtime_t *dst, psmtime_t *src)
{
        dst->pt_lo += src->pt_lo;
}

/*
 * void psm_time_cvt(dl_t *dst, psmtime_t *src);
 * 	Convert the opaque time stamp to micro second.
 *
 * Calling/Exit State:
 *	Called by:	base kernel (PSM entry point)
 *	Parameters:	
 *		dst -- pointer to time structure to receive result
 *		src -- pointer to time structure to be converted
 *	Locking:	none
 *	Globals read:   none
 *	Globs. written: none
 *	Returns:	none
 */
void
psm_time_cvt(dl_t *dst, psmtime_t *src)
{
        dst->dl_lop = src->pt_lo;
        dst->dl_hop = (long)src->pt_hi;
}


/*
 * void 
 * psm_intr_init(void)
 * 	Initialize the PICs for the current engine.
 *
 * Calling/Exit State:
 *	Called by:	base kernel (PSM entry point
 *	Parameters:	none
 *	Locking:	none
 *	Globals read:   upyet, astpicinit()
 *	Globs. written: none
 *	Returns:	none
 *
 * if we are being called from sysinit() (as opposed to selfinit()) we
 * must initialize the EBI prior to initializing the PIC hardware and
 * data structures.
 */
void
psm_intr_init(void)
{
	if (!upyet) 
	{
		ast_configure();
	}
	
	astpicinit(myengnum);
}


/*
 * void psm_intr_start(void)
 *	Enables interrupts.  
 *
 * Calling/Exit State:
 *	Called by:	base kernel (PSM entry point)
 *	Parameters:	none
 *	Locking:	none
 *	Globals read:   astpicstart()
 *	Globs. written: none
 *	Returns:	none
 *
 * Remarks:
 *	Called during latter part of system startup when base kernel 
 *	ready to allow interrupts.
 */
void
psm_intr_start(void)
{
	astpicstart();
}

/*
 * Special asm macro to enable interrupts before calling xcall_intr
 */
asm void sti(void)
{
	sti;
}

#pragma	asm partial_optimization sti

/*
 * void
 * psm_intr(uint vect, uint oldpl, uint edx, uint ecx, uint eax, 
 *		uint es, uint ds, uint eip, uint cs, uint efl)
 * 	Handle a cross processor interrupt.
 *
 * Calling/Exit State:
 *	Called by:	base kernel (PSM entry point)	
 *	Parameters:
 *		vect --  vector number
 *		oldpl -- interrupt priority level before interrupt
 *		edx, ecx, eax, es, ds, eip, cs, efl  -- register contents
 *	Locking:	none
 *	Globals read: prfstat, ast_event_flags, prf_pending
 *	Globs. written: engine_evtflags, prf_pending
 *	Returns:	none
 *
 * Description:
 * 	This routine is called when a cross processor interrupt (IPI in AST
 * 	parlance) is received.  The System Priority Interrupt (SPI) and Local
 * 	Software Interrupt (LSI) are also routed here even though they come
 * 	in on different vectors.  This was done in the interest of ease of 
 * 	configuration for the user and no other reason.
 *
 * Remarks:
 * 	This routine is called with interrupts disabled at the CPU.
 */
/*ARGSUSED*/
void
psm_intr(uint vect, uint oldpl, uint edx, uint ecx, uint eax, uint es, uint ds,
		uint eip, uint cs, uint efl)
{
	
	ASSERT(((vect >= SPI) && (vect <= IPI)));

	switch (vect)
	{
		case SPI:
			spi_intr(vect);
			return;
		case LSI:
			lsi_intr(vect);
			return;
		default:
#ifdef AST_DEBUG
			ast_xintr_recv[myengnum]++;
#endif
			break;
	}

#ifndef UNIPROC
	if (ast_eventflags[myengnum] != 0)
		engine_evtflags |= atomic_fnc(&ast_eventflags[myengnum]);

	sti();
	xcall_intr();

	if (prf_pending > 0) {
		--prf_pending;
		if (prfstat)
			prfintr(eip, USERMODE(cs, efl));
	}

	if ((xclock_pending >= 1) && (oldpl < plhi)) {
		xclock_pending = -1;
		lclclock(&eax);
		xclock_pending = 0;
	}
#endif
}

/*
 * STATIC void
 * reset(void)
 * 	Trigger reboot and immediately halt processor.	M001
 *
 * Calling/Exit State:
 *	Called by:	psm_reboot()
 *	Parameters:	none
 *	Locking:	none
 *	Globals read:   none
 *	Globs. written: none
 *	Returns:	none
 *
 * Description:
 *	Puts processor in endless loop (with interrupts masked)
 *	after triggering reboot.  
 *
 * Remarks:
 *	Called on the boot processor.
 */
STATIC void
reset(void)
{
	outb(STAT_8042, 0xfe);	/* trigger reboot */

	for (;;)		/* endless loop */
	{
		asm("cli ");
		asm("hlt ");
	}
}	

/*  
 * STATIC void
 * halt_all(void)
 *  	Gracefully shut down as many CPUs as possible,
 *  	excluding self and boot engine.		M001
 *
 * Calling/Exit State:
 *	Called by:	psm_reboot()
 *	Parameters:	none
 *	Locking:	none
 *	Globals read:   myengnum, ast_calltab, MMIOTable
 *	Globs. written: none
 *	Returns:	none
 */
STATIC void
halt_all(void)
{
	int eng;

	for (eng = 0; eng < Nengine; eng++)
	{
		if (eng != myengnum && ! IS_BOOT_ENG(eng))
			(ast_calltab.StopProc)(MMIOTable, eng);
	}

}

/*ARGSUSED*/
/*
 * void
 * psm_reboot(int flag)
 *	Do hard reboot or power down of the system.
 *
 * Calling/Exit State:
 *	Called by:	psm_reboot()
 *	Parameters:
 *		flag -- if AD_BOOT then reset, else shut off power
 *	Locking:	none
 *	Globals read:   ast_shutdown_pwr, ast_calltab, MMIOTable, myengnum
 *	Globs. written: none
 *	Returns:	none
 *
 * Description:
 *	If doing a reboot, then it will first shut down all processors except 
 *	the boot engine and the current engine.  Then it gives a cross-call to 
 *	boot engine to do reset(), then goes into endless loop with interrupts
 *	masked.
 *
 * Remarks:
 *	Called as final step in reboot or shutdown. Chops power because
 *	man page for "init 0" and "shutdown says this is the correct behavior
 *	for systems that have this capability.  Defining PWROFF_IOCTL will
 *	restore the old behavior of shutting off power on shutdown only if
 *	ast_shutdown_pwr has previously been set with an ioctl call.
 */
void
psm_reboot(int flag)
{
	splhi();	/*  No interruptions, please */

	/*
	 * If we've been told to shutdown the power supply, do so.
	 * otherwise, make the front panel shutdown switch visible
	 * so that it is possible to power down the machine.
	 */
#ifdef PWROFF_IOCTL			/* M003 */
	if (ast_shutdown_pwr) 
#else
	if (flag != AD_BOOT) 
#endif
	{
		cmn_err(CE_CONT, "Shutting off power\n");
		/*
		 *  Give user time to read messages
		 */
		drv_usecwait(500000);

		(ast_calltab.ShutdownPowerSupply)(MMIOTable);
		/*
		 * That's all, folks!
		 */
	} else {
		(ast_calltab.SetPanelOffSwitchMode)(MMIOTable, OFF_SWITCH_HARD);
	}

	/*
	 *  Gracefully shut down as many CPUs as possible,
	 *  excluding self and boot engine
	 */
	halt_all();			/* M001 */

	/* if sanity timer in use, turn off to allow clean soft reboot */ 
	if ((bootinfo.machflags & EISA_IO_BUS) && sanity_clk)
		outb(SANITY_CHECK, RESET_SANITY);

	reboot_prompt(flag);
	
	softreset();

	/* 
	 * M001: If running on boot engine, call reset()
	 * else use xcall to make boot engine call reset()
	 */
	if IS_BOOT_ENG(myengnum)
	{
		reset();
		/* never returns */
	}	
	else
	{
		emask_t targets;

		EMASK_INIT(&targets, BOOTENG);
		xcall(&targets, NULL, reset, NULL);	
	}

	for (;;)	/* Endless loop */
	{
		asm("cli ");
		asm("hlt ");
	}
}	

/*
 * void
 * psm_offline_self(void)
 *	Called when taking current processor off line--
 *	actually does nothing except update front panel LEDs
 *	if in special "on-line" mode.
 *
 * Calling/Exit State:
 *	Called by:	base kernel (PSM entry point)
 *	Parameters:	none
 *	Locking:	none
 *	Globals read:   ast_calltab, MMIOTable
 *	Globs. written: none
 *	Returns:	none
 *
 * Remarks:
 *	If in so-called "on-line mode", then call ast_update_graph() 
 *	to update LEDS for which CPUS are configured  (enabled with
 *	psradm -n or online commands).
 */
void
psm_offline_self(void)
{
	unsigned int panmode;

	(ast_calltab.GetPanelProcGraphMode)(MMIOTable, &panmode);
	if (panmode == PANEL_MODE_OVERRIDE)
		astm_update_graph();
	return;
}

/*
 * void
 * psm_sendsoft(int engnum, uint_t arg)
 *	Post SW interrupt to (typically) another processor.
 *
 * Calling/Exit State:
 *	Called by:	base kernel (PSM entry point)
 *	Parameters:
 *		engnum -- CPU to get the interrupt
 *		arg -- event number to send to other CPU
 *	Locking:	none
 *	Globals read:   myengnum
 *	Globs. written: engine_evtflags
 *	Returns:	none
 *
 * Description:
 *	Post a software interrupt to a processor with the specified flags.
 *	The implementation uses the static array ast_eventflags; each entry
 *	in ast_eventflags corresponds to an engine and acts as a mailbox for
 *	soft interrupts to that engine.  A soft interrupt is sent by:
 *		(1) Atomically or'ing the specified flags into the
 *			ast_eventflags entry for the engine (using
 *			a bus lock prefix with an OR instruction)
 *		(2) Sending a cross-processor interrupt (directly through
 *			psm_send_xintr, not via xcall)
 *
 * Remarks:
 *	Since sendsoft is sometimes called with an fspinlock held, we
 *	can't acquire locks while doing a sendsoft.  Thus, sendsoft does
 *	not use the xcall mechanism, and it uses bus locking rather than
 *	a software lock to update ast_eventflags atomically.
 *
 *  On the AST this has nasty implications since the EBI routine GenIPI
 *  is not multi-threaded.  This means that we have to have some sort of
 *  syncronization to ensure that only one instance of this routine is 
 *  executing at a time.
 *
 *  This is done via the __ast_fspin_lock/__ast_fspin_unlock routines.
 *  These routines attempt to get a lock using the following assumptions:
 *       (1) We are at PLHI (or interrupts have been disabled via a "cli"
 *           instruction) and therefore cannot have interrupted another instance 
 *           of psm_sendsoft *on the current engine*.
 *       (2) psm_send_xintr is never called at anything other than PLHI.
 *           This is currently true, the xcall mechanism raises the level to
 *           to PLHI prior to calling psm_send_xintr.
 */
void
psm_sendsoft(int engnum, uint_t arg)
{

	if (engnum == myengnum)
		engine_evtflags |= arg;
	else {
		atomic_or(&ast_eventflags[engnum], arg);
		psm_send_xintr(engnum);
	}
}

/*
 * void
 * psm_ledctl(led_request_t req, uint_t led_bits)
 *	Display current engine busy/idle status on front panel CPU LEDs
 *
 * Calling/Exit State:
 *	Called by:	base kernel (idle loop)
 *	Parameters:	none
 *		req	-- controls action: LED_ON or LED_OFF
 *		led_bit -- not used: present for compatibility only
 *	Locking:	none
 *	Globals read:   ast_calltab, MMIOTable
 *	Globs. written: none
 *	Returns:	none
 */
/*ARGSUSED*/
void
psm_ledctl(led_request_t req, uint_t led_bits)
{
	switch (req)
	{
		case LED_ON:
			(ast_calltab.LogProcBusy)(MMIOTable);
			return;
		case LED_OFF:
			(ast_calltab.LogProcIdle)(MMIOTable);
			return;
		default:
			return;
	}
	/*NOTREACHED*/
}


/*
 * void
 * psm_intron(int iv, pl_t level, int engnum, int intmp, int itype)
 *	Unmask the interrupt vector from the iplmask.
 *
 * Calling/Exit State:
 *	Called by:	base_kernel (PSM entry point)
 *	Parameters:	none
 *		iv 	-- interrupt request no. that needs to be enabled.
 *		engnum 	-- engine on which the interrupt must be unmasked.
 *		level 	-- interrupt priority level of the iv.
 *		intmp 	-- flag indicating if the driver is multithreaded.
 *		itype 	-- interrupt type.
 *	Locking:	mod_iv_lock spin lock is held on entry/exit.
 *	Globals read:   none
 *	Globs. written: none
 *	Returns:	none
 *
 *
 * Remarks:
 *	intcpu	intmp
 *	 -1       1	Bind it to cpu 0   (Handled here)
 *	 !-1	  1	Bind it to cpu !-1 (Ignore intmp)
 *	 !-1	  0	Bind it to cpu !-1 (Ignore intmp)
 *	 -1       0	Bind it to cpu 0   (DLM handles it by passing the
 *						correct engnum)
 *
 *	nenableint() is defined in astpic.c
 */
void
psm_intron(int iv, pl_t level, int engnum, int intmp, int itype)
{
	if (intmp == 1 && engnum == -1)
		engnum = 0;

	nenableint(iv, level, engnum, itype);
}


/*
 * void
 * psm_introff(int iv, pl_t level, int engnum, int itype)
 *	Mask the interrupt vector in the iplmask of the engine currently
 *	running on.
 *
 * Calling/Exit State:
 *	Called by:	base kernel (PSM entry point)
 *	Parameters:	none
 *		iv 	-- interrupt that needs to be disabled.
 *		engnum 	-- engine on which the interrupt must be masked.
 *		level 	-- interrupt priority level of the iv.
 *		itype 	-- interrupt type.
 *	Locking:	mod_iv_lock spin lock is held on entry/exit.
 *	Globals read:   ast_calltab, MMIOTable
 *	Globs. written: none
 *	Returns:	none
 *
 *  Remarks:
 *	ndisableint() is defined in astpic.c
 */
void
psm_introff(int iv, pl_t level, int engnum, int itype)
{
	if (engnum == -1)
		engnum = 0;
	ndisableint(iv, level, engnum, itype);
}

/*
 * STATIC void
 * softreset(void)
 *	Indicate that the subsequent reboot is a "soft" reboot.
 *
 * Calling/Exit State:
 *	Called by:	psm_reboot()
 *	Parameters:	none
 *		startaddr -- address to jump to after reset for this processor
 *	Locking:	none
 *	Globals read:   ast_calltab, MMIOTable
 *	Globs. written: none
 *	Returns:	none
 *
 *  Called by psm_reboot().
 */
STATIC void
softreset(void)
{
	/* do soft reboot; only do memory check after power-on */

	*((ulong_t *)&kvpage0[0x467]) = ((ulong_t)IO_ROM_SEG << 16) |
					 (ulong_t)IO_ROM_INIT;
	*((ushort_t *)&kvpage0[0x472]) = RESET_FLAG;

	/* set shutdown flag to reset using int 19 */
	outb(CMOS_ADDR, SHUT_DOWN);
	outb(CMOS_DATA, SHUT5);
}

/*
 * STATIC void
 * ast_set_resetaddr(paddr_t startaddr)
 *	Set the starting address of the target engine
 *
 * Calling/Exit State:
 *	Called by:	psm_online_engine()
 *	Parameters:	none
 *		startaddr -- address to jump to after reset for this processor
 *	Locking:	none
 *	Globals read:   ast_calltab, MMIOTable
 *	Globs. written: none
 *	Returns:	none
 */
STATIC void
ast_set_resetaddr(paddr_t startaddr)
{
	struct resetaddr {
		ushort_t  offset;
		ushort_t  segment;
	} start;
	paddr_t addr;
	char    *vector, *source;
	int      i;


	addr = (paddr_t)startaddr;

	/* get the real address seg:offset format */
	start.offset = addr & 0x0f;
	start.segment = (addr >> 4) & 0xFFFF;

	/* now put the address into warm reset vector (40:67) */
	vector = (char *)physmap(RESET_VECT, 
				sizeof(struct resetaddr), KM_SLEEP);

	/*
	 * copy byte by byte since the reset vector port is
	 * not word aligned
	 */
	source = (char *) &start;
	for (i = 0; i < sizeof(struct resetaddr); i++)
		*vector++ = *source++;

	physmap_free(vector, sizeof(struct resetaddr), 0);

	(ast_calltab.FlushRAMCache)(MMIOTable, CACHE_NO_INVALID);
}

/*
 * void
 * psm_online_engine(int engnum, paddr_t startaddr, int flags)
 *	Start a processor executing.
 *
 * Calling/Exit State:
 *	Called by:	base kernel (PSM entry point)
 *	Parameters:	none
 *		engnum -- processor number to start
 *		startaddr -- address to jump to on reset
 *		flags  -- indicates processor has already been initialized
 *	Locking:	none
 *	Globals read:   ast_calltab, MMIOTable
 *	Globs. written: none
 *	Returns:	none
 *
 * Remarks:
 * 	This function is called with onoff_mutex held, so no lock
 * 	is needed even though the EBI call is *not* multi-threaded.
 */
/*ARGSUSED*/
void
psm_online_engine(int engnum, paddr_t startaddr, int flags)
{

	char *reason;
	unsigned int panmode, leds;
	
	/* M004: 
	 * If in online mode, turn on LED corresponding to given CPU
	 */
	(ast_calltab.GetPanelProcGraphMode)(MMIOTable, &panmode);
	if (panmode == PANEL_MODE_OVERRIDE)
	{
		(ast_calltab.GetPanelProcGraphValue)
			(MMIOTable, &leds);
		leds |= (1 << engnum);
		(ast_calltab.SetPanelProcGraphValue)
			(MMIOTable, leds);
	}

	if (flags == WARM_ONLINE)
	{
		return;
	}
	
	ast_set_resetaddr(startaddr);

	switch ((ast_calltab.StartProc)(MMIOTable, engnum))
	{
		case PROC_RUNNING:
			reason = "processor already running";
			break;
		case ERR_BAD_PROC_ID:
			reason = "invalid processor id";
			break;
		case OK:
			return;
		default:
			reason = "unknown error";
			break;
	}
	cmn_err(CE_NOTE, "psm_online_engine: error starting processor %d: %s.", engnum, reason);
}

/*
 * void 
 * astm_update_graph(void)
 * 	Update the panel graph display to show only which processors
 *	are online (available).
 *
 * Calling/Exit State:
 *	Called by:	base kernel (PSM entry point)
 *	Parameters:	none
 *	Locking:	none
 *	Globals read:   engine
 *	Globs. written: none
 *	Returns:	none
 *
 * Remarks:
 *	Called in USG's "on-line mode" only.
 */
void 
astm_update_graph(void)
{
		int numprocs,i;
		int leds = 0;

		if ((ast_calltab.GetNumProcs)(MMIOTable, 
				(unsigned int *) &numprocs) != OK) 
		{
			cmn_err(CE_WARN, 
			"ast_update_graph: unable to get number of processors, assuming 1");
			numprocs = 1;
		}

		for (i=0; i<numprocs; i++)
		{
			if ( (engine[i].e_flags & E_NOWAY) == 0 ) 
				leds |= (1 << i);
		}	

		if ((ast_calltab.SetPanelProcGraphValue)(MMIOTable, leds))
			cmn_err(CE_WARN, "Unable to set panel graph value\n");
}

/*
 * boolean_t
 * psm_intrpend(pl_t pl)
 *	Check for pending interrupts above specified priority level.
 *
 * Calling/Exit State:
 *	None.
 * 
 * Description:
 *	Returns B_TRUE (B_FALSE) if there are (are not) any pending
 *	interrupts above the specified priority level.
 *
 * Remarks:
 *	picipl is generally set to the ipl of the highest priority
 *	pending interrupt on this processor; if picipl == PLBASE,
 *	then there are no pending interrupts on this processor.
 */
boolean_t
psm_intrpend(pl_t pl)
{
	extern pl_t picipl;

	return (picipl > pl);
}

/*
 * void
 * psm_panic()
 *      Dump platform specific information to memory upon system panicing.
 *
 * Calling/Exit State:
 *      None.
 */
void
psm_panic()
{
}
