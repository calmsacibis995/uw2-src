/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/compaq/compaq.c	1.17"
#ident	"$Header: $"

/*
 * AT PSM/mHAL hardware-specific routines and data.
 *
 * This file contains a set of subroutines and a set of data, all of which
 * are used elsewhere in the MP kernel but which vary for MP hardware from
 * different vendors. In general, all the routines with an "psm_" prefix
 * should be defined for whatever hardware you use, even if the routine does
 * nothing. The same applies for data (unfortunately the data names may not
 * have a distinctive prefix), even if it might not be used. Some routines
 * and data defined in this file are for specific platforms only. Vendors
 * do not need to support these items for their own hardware.
 *
 * The requirements for each item in this file are clearly stated in the
 * PSM/mHAL interface specification document.
 *
 * This MP PSM/mHAL file supports both the Systempro, Systempro/XL
 * dual processor and Proliant system.
 */

#include <io/cram/cram.h>
#include <io/prf/prf.h>
#include <mem/immu.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <proc/regset.h>
#include <proc/seg.h>
#include <proc/tss.h>
#include <svc/bootinfo.h>
#include <svc/creg.h>
#include <svc/eisa.h>
#include <svc/intr.h>
#include <svc/pic.h>
#include <svc/pit.h>
#include <svc/psm.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/processor.h>
#include <util/types.h>

#include <io/ddi.h>		/* Must come last after other kernel headers */
#include <io/ddi_i386at.h>

#include <syspro.h>
#include <xl.h>


#define	IO_ROM_INIT	0xfff0
#define	IO_ROM_SEG	0xf000
#define	RESET_FLAG	0x1234
#define	SHUT_DOWN	0x8f
#define	SHUT5		5
#define	STAT_8042	0x64
#define I8042_RESETCPU	0xfe
#define FPU_BUSY_LATCH	0xf0

#define	ISASYMINTR	(sp_iomode == ASYMINTR)
#define	ISSYMINTR	(sp_iomode == SYMINTR)
#define	ISCONFIGURED	(ISASYMINTR || ISSYMINTR)

/*
 * Interrupt entry points.
 */

#define SOFTINT		63	/* software interrupt */

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

extern int xclock_pending;
extern int prf_pending;
extern uchar_t kvpage0[];
extern boolean_t fpu_external;

STATIC void	psm_clr_fpbusy(void);
STATIC void	cpq_set_resetaddr(paddr_t);
STATIC void	cpq_halt(void);
void		cpq_noop(void);
void		softreset(void);

/*
 * Configurable objects -- defined in psm.cf/Space.c
 */
extern int sp_maxnumcpu;			/* max. cpus in SP or SP/XL */
extern int sp_systypes;				/* number of SP or SP/XL systems
						 * with different EISA ids.
						 */
extern int spxl_iomodemask;			/* mask the I/O mode */

extern void sp_intr_init(void);
extern struct idt_init *sp_idt(int);

struct cpqpsmops cpqupfuncs = {
	(void (*)())cpq_noop,			/* cpq_reboot */
	sp_idt,					/* cpq_idt */
	sp_intr_init,				/* cpq_intr_init */
	(void (*)())cpq_noop,			/* cpq_intr_start */
	(int (*)())cpq_noop,			/* cpq_numeng */
	(void (*)())cpq_noop,			/* cpq_configure */
	(void (*)())cpq_noop,			/* cpq_online_engine */
	(void (*)())cpq_noop,			/* cpq_selfinit */
	(void (*)())cpq_noop,			/* cpq_misc_init */
	(void (*)())cpq_noop,			/* cpq_offline_self */
	(void (*)())cpq_noop,			/* cpq_clear_xintr */
	(void (*)())cpq_noop,			/* cpq_send_xintr */
	(void (*)())cpq_noop,			/* cpq_timer_init */
	(ulong_t (*)())cpq_noop,		/* cpq_usec_time */
	(boolean_t (*)())cpq_noop,		/* cpq_isxcall */
	(boolean_t (*)())cpq_noop,		/* cpq_ecc_intr */
	(boolean_t (*)())cpq_noop,		/* cpq_dma_intr */
	(int (*)())cpq_noop,			/* cpq_assignvec */
	(int (*)())cpq_noop 			/* cpq_unassignvec */
};

/*
 * Initialize the default cpqpsmops to a UP.
 */
struct cpqpsmops *cpqmpfuncs = &cpqupfuncs;

/*
 * Array of eventflags, one per engine, used in implementation of sendsoft
 */
STATIC volatile uint_t psm_eventflags[MAXNUMCPU];

/*
 * P0 IDT (interrupt distribution table) initialization.
 */
struct idt_init cpq_idt0_init[] = {
	{	SOFTINT,	GATE_386INT,	softint,	GATE_KACC },
	{	DEVINTS,	GATE_386INT,	devint0,	GATE_KACC },
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
	{	DEVINTS + 27,	GATE_386INT,	devint27,	GATE_KACC },
	{	DEVINTS + 28,	GATE_386INT,	devint28,	GATE_KACC },
	{	DEVINTS + 29,	GATE_386INT,	devint29,	GATE_KACC },
	{	DEVINTS + 30,	GATE_386INT,	devint30,	GATE_KACC },
	{	DEVINTS + 31,	GATE_386INT,	devint31,	GATE_KACC },
	{	DEVINTS + 32,	GATE_386INT,	devint32,	GATE_KACC },
	{	DEVINTS + 33,	GATE_386INT,	devint33,	GATE_KACC },
	{	DEVINTS + 34,	GATE_386INT,	devint34,	GATE_KACC },
	{	DEVINTS + 35,	GATE_386INT,	devint35,	GATE_KACC },
	{	DEVINTS + 36,	GATE_386INT,	devint36,	GATE_KACC },
	{	DEVINTS + 37,	GATE_386INT,	devint37,	GATE_KACC },
	{	DEVINTS + 38,	GATE_386INT,	devint38,	GATE_KACC },
	{	DEVINTS + 39,	GATE_386INT,	devint39,	GATE_KACC },
	{	DEVINTS + 40,	GATE_386INT,	devint40,	GATE_KACC },
	{	DEVINTS + 41,	GATE_386INT,	devint41,	GATE_KACC },
	{	DEVINTS + 42,	GATE_386INT,	devint42,	GATE_KACC },
	{	DEVINTS + 43,	GATE_386INT,	devint43,	GATE_KACC },
	{	DEVINTS + 44,	GATE_386INT,	devint44,	GATE_KACC },
	{	DEVINTS + 45,	GATE_386INT,	devint45,	GATE_KACC },
	{	DEVINTS + 46,	GATE_386INT,	devint46,	GATE_KACC },
	{	DEVINTS + 47,	GATE_386INT,	devint47,	GATE_KACC },
	{	DEVINTS + 48,	GATE_386INT,	devint48,	GATE_KACC },
	{	DEVINTS + 49,	GATE_386INT,	devint49,	GATE_KACC },
	{	DEVINTS + 50,	GATE_386INT,	devint50,	GATE_KACC },
	{	DEVINTS + 51,	GATE_386INT,	devint51,	GATE_KACC },
	{	DEVINTS + 52,	GATE_386INT,	devint52,	GATE_KACC },
	{	DEVINTS + 53,	GATE_386INT,	devint53,	GATE_KACC },
	{	DEVINTS + 54,	GATE_386INT,	devint54,	GATE_KACC },
	{	DEVINTS + 55,	GATE_386INT,	devint55,	GATE_KACC },
	{	DEVINTS + 56,	GATE_386INT,	devint56,	GATE_KACC },
	{	DEVINTS + 57,	GATE_386INT,	devint57,	GATE_KACC },
	{	DEVINTS + 58,	GATE_386INT,	devint58,	GATE_KACC },
	{	DEVINTS + 59,	GATE_386INT,	devint59,	GATE_KACC },
	{	DEVINTS + 60,	GATE_386INT,	devint60,	GATE_KACC },
	{	DEVINTS + 61,	GATE_386INT,	devint61,	GATE_KACC },
	{	DEVINTS + 62,	GATE_386INT,	devint62,	GATE_KACC },
	{	DEVINTS + 63,	GATE_386INT,	devint63,	GATE_KACC },
	{	DEVINTS + 64,	GATE_386INT,	devint64,	GATE_KACC },
	{	DEVINTS + 65,	GATE_386INT,	devint65,	GATE_KACC },
	{	DEVINTS + 66,	GATE_386INT,	devint66,	GATE_KACC },
	{	DEVINTS + 67,	GATE_386INT,	devint67,	GATE_KACC },
	{	DEVINTS + 68,	GATE_386INT,	devint68,	GATE_KACC },
	{	DEVINTS + 69,	GATE_386INT,	devint69,	GATE_KACC },
	{	DEVINTS + 70,	GATE_386INT,	devint70,	GATE_KACC },
	{	DEVINTS + 71,	GATE_386INT,	devint71,	GATE_KACC },
	{ 0 },
};

/*
 * P1 IDT (interrupt distribution table) initialization.
 */
struct idt_init cpq_idt1_init[] = {
	{	SOFTINT,	GATE_386INT,	softint,	GATE_KACC },
	{	DEVINTS + 13,	GATE_386INT,	devxcall,	GATE_KACC },
	{ 0 },
};

uchar_t sp_iomode;		/* I/O mode -- Systempro Compatible or
				 * Systempro/XL Symmetric mode
				 */

uchar_t	cpq_mptype;		/* type of Compaq multiprocessor platforms */
void (*cpq_ecc_intr_hdlr)();	/* Correctable memory error interrupt hdlr. */
void (*cpq_dma_intr_hdlr)();	/* DMA chaining interrupt hdlr. */

#define str(s)  #s
#define xstr(s) str(s)


/*
 * void
 * cpq_noop(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
cpq_noop(void)
{
}


/*
 * void
 * psm_do_softint(void)
 *
 * Calling/Exit State:
 *	Called if user preemption flag is set.
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
 * int
 * psm_doarg(char *s)
 *	psm specific boot argument parsing
 *
 * Calling/Exit State:
 *	This routine is called after first checking an argument
 *	against the standard set of supported arguments.  Unknown
 *	(non-standard) arguments will be passed to this routine.
 *
 *	Return Values:	 0 if argument handled
 *			-1 if argument unknown
 */ 
/* ARGSUSED */
int
psm_doarg(char *s)
{
	return(-1);		/* unknown argument */
}


/*
 * void
 * reset(void *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
void
reset(void *arg)
{
	extern int i8042_write(uchar_t, uchar_t);

	i8042_write(STAT_8042, I8042_RESETCPU);	/* trigger reboot */

}


/*
 * void
 * psm_reboot(int)
 *
 * Calling/Exit State:
 *	Intended use for flag values:
 *		flag != 1	halt the system and wait for user interaction
 *		flag == 1	automatic reboot, no user interaction required
 */
void
psm_reboot(int flag)
{
        uint_t  bustype;

	splhi();

	/*
	 * Halt the non-boot engine excluding self.
	 */
	cpq_halt();

        /*
         * If sanity timer in use, turn off to allow clean soft reboot.
         */
        drv_gethardware(IOBUS_TYPE, &bustype);
        if (bustype & BUS_EISA && myengnum == BOOTENG)
                eisa_sanity_halt();

	reboot_prompt(flag);

	softreset();

	if (myengnum == BOOTENG) {
		reset(NULL);
	} else {
		emask_t targets;

		EMASK_INIT(&targets, BOOTENG);
		xcall(&targets, NULL, reset, NULL);
	}

        /*
         * Prohibit processor from gaining access to M bus and
         * assert the reset line for non-boot self engine.
         */
	if (myengnum != BOOTENG)
		(*cpqmpfuncs->cpq_reboot)(myengnum);

	for (;;) {
		asm("   cli     ");
		asm("   hlt     ");
	}
}


/*
 * void
 * softreset(void)
 *	Indicate that the subsequent reboot is a "soft" reboot.
 *
 * Calling/Exit State:
 *	None.
 */
void
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
 * struct idt_init *
 * psm_idt(int engnum)
 *
 * Calling/Exit State:
 *      Returns a pointer to an idt_init table for all the
 *      sub-platform-specific IDT entries for the given engine.
 *
 * Remarks:
 *	This routine must be called before init_desc_tables() is called
 *	from selfinit().
 *
 * Note:
 *	It is also called very early in the system startup while setting
 *	up the tmp_init_desc_tables().
 */
struct idt_init *
psm_idt(int engnum)
{
	return ((*cpqmpfuncs->cpq_idt)(engnum));
}


/*
 * void
 * psm_intr_init(void)
 *	Initialize programmable interrupt controller.
 *
 * Calling/Exit State:
 *	Called when the system is being initialized.
 */
void
psm_intr_init(void)
{
	(*cpqmpfuncs->cpq_intr_init)();
}


/*
 * void
 * psm_intr_start(void)
 *	Enable interrupts.
 *
 * Calling/Exit State:
 *	Called from selfinit() when the engine is being onlined. 
 *
 * TODO:
 *	Must be changed to take logical engnum as an argument.
 */
void
psm_intr_start(void)
{
	(*cpqmpfuncs->cpq_intr_start)();
}


/*
 * int
 * psm_numeng(void)
 *
 * Calling/Exit State:
 *	Returns number of engines available in the system.
 *	It is used by conf_proc() to calloc space for engine
 *	structures.
 */
int
psm_numeng(void)
{
	return((*cpqmpfuncs->cpq_numeng)());
}


/*
 * void
 * psm_configure(void)
 *	Gather miscellaneous hardware configuration information.
 *
 * Calling/Exit State:
 *	None.
 *
 * Note:
 *	Routine to do initial setup;  This is done very early.
 *
 *	Read EISA info to determine what kind of CPU we have as base.
 *	readeisa(where slot=sp_xl_slot[0], and TYPE field)
 *		check for 80486 else Pentium.
 *
 *
 *	Read EISA info to determine what kind of CPU we have as secondary;
 *	readeisa(where slot=sp_xl_slot[1], and TYPE field)
 *		check for 80486 else Pentium.
 *
 *	This is not really necessary to check type since XL does not 
 *	allow mixing of 486's and Pentiums but it is necessary to see
 *	if second CPU is installed.
 */
void
psm_configure(void)
{
	extern int spxl_presense(void);
	extern struct cpqpsmops spmpfuncs;
	extern struct cpqpsmops xlmpfuncs;


	/*
	 * Find out if we have are running on a Systempro, Systempro/XL,
	 * Proliant or Powerpro?
	 */

	if (ISSYSTEMPRO) {
		if (ISSYSTEMPROXL) {
			cpq_mptype = CPQ_SYSTEMPROXL;
			/*
			 * Now check if it a proliant or an XL? The
			 * function will overwrite the cpq_mptype.
			 */
			spxl_presense();
		} else {
			cpq_mptype = CPQ_SYSTEMPRO;
			cmn_err(CE_CONT, "!Found Systempro...\n");
		}
	} else if (ISPOWERPRO) {
		cpq_mptype = CPQ_SYSTEMPRO_COMPATIBLE;
		cmn_err(CE_CONT, "!Found Powerpro...\n");
	} else {
		/*
		 *+ Check if your system is Systempro Compatible or if it is
		 *+ jumpered to Systempro compatible mode.
		 */
		cmn_err(CE_WARN,
			"!psm_configure: Check if your system "
			"is Systempro Compatible");
		cmn_err(CE_WARN,
			"!psm_configure: Defaulting to Systempro "
			"Compatible mode.");
		cpq_mptype = CPQ_SYSTEMPRO_COMPATIBLE;
	}
	
	/*
	 * Do not program an XL to symmetric mode if the mode 
	 * is masked.
	 */
	if ((cpq_mptype & (CPQ_SYSTEMPROXL | CPQ_PROLIANT)) && 
	    !(spxl_iomodemask & SYMINTR)) {
		sp_iomode = SYMINTR;
		cpqmpfuncs = &xlmpfuncs;
	} else {
		sp_iomode = ASYMINTR;
		cpqmpfuncs = &spmpfuncs;
	}

	(*cpqmpfuncs->cpq_configure)();

	psm_clr_fpbusy();	/* TEMP; until sysinit reorg */
}


/*
 * STATIC void
 * cpq_set_resetaddr(void (*)())
 *	Set the starting address of the target engine
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
cpq_set_resetaddr(paddr_t startaddr)
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
}


/*
 * void
 * psm_online_engine(int, paddr_t, int)
 *
 * Calling/Exit State:
 *	onoff_mutex lock is held on entry/exit.
 *
 * Remarks:
 *	Set the reset address and enable the engine to
 *	access the memory bus.
 */
/* ARGSUSED */
void
psm_online_engine(int engnum, paddr_t startaddr, int flag)
{
	if (flag == WARM_ONLINE)
		return;

	ASSERT(flag == COLD_ONLINE);

	/* set up the starting address of the target cpu */
	(void) cpq_set_resetaddr(startaddr);

	(*cpqmpfuncs->cpq_online_engine)(engnum);
}


/*
 * void
 * psm_offline_self(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
psm_offline_self(void)
{
	(*cpqmpfuncs->cpq_offline_self)();
}


/*
 * STATIC void
 * cpq_halt(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	The platform specific graceful shutdown sequence:
 *		- Retrieve the interrupts, and set PIC masks to all ones.
 *		- Turn off cache.
 *		- Restore the bios information (?).
 *		- "freeze" the other CPUs.
 *
 * Note:
 *	Can run on either P1 or P2.
 */
STATIC void
cpq_halt(void)
{
	int	engnum;


	for (engnum = BOOTENG; engnum < Nengine; engnum++) {

		if (engnum == BOOTENG || engnum == myengnum)
			continue;

		/*
		 * Prohibit processor from gaining access to M bus and
		 * assert the reset line for non-boot engine.
		 */
		(*cpqmpfuncs->cpq_reboot)(engnum);
        }
}


/*
 * void
 * psm_clear_xintr(int)
 *	Clear the cross-processor interrupt bit in the processor
 *	control port.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Should only be called by an engine to clear its own cross-processor
 *	interrupt.
 */
void
psm_clear_xintr(int engnum)
{
	ASSERT(engnum == myengnum);

	(*cpqmpfuncs->cpq_clear_xintr)(engnum);
}


/*
 * void
 * psm_send_xintr(int)
 *	Assert the cross-processor interrupt bit in the processor
 *	control port.
 *
 * Calling/Exit State:
 *	None.
 */
void
psm_send_xintr(int engnum)
{
	(*cpqmpfuncs->cpq_send_xintr)(engnum);
}


/*
 * STATIC void
 * psm_clr_fpbusy(void)
 *	Clear FPU BUSY latch.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
psm_clr_fpbusy(void)
{
	/*
	 * The coprocessor's busy line is only held active while actually
	 * executing, but hardware that sits between the main processor
	 * and the coprocessor latches the coprocessor error output and
	 * uses this to feed the coprocessor busy input to the CPU.
	 * (This is an 8086/8087 PC compatibility configuration.)
	 *
	 * Since this busy line prevents normal coprocessor communication,
	 * we must clear the latch before attempting to examine the FPU
	 * status word.  Otherwise, the CPU would hang on any FPU access.
	 */
	outb(FPU_BUSY_LATCH, 0);	/* Clear FPU BUSY latch */
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
 * psm_intr(uint vec, uint oldpl, uint edx, uint ecx,
 *               uint eax, uint es, uint ds, uint eip, uint cs)
 *	Interrupt handler for cross-processor interrupts and
 *	floating-point coprocessor interrupts. It distinguishes
 *	between the two by checking the PINT bit in processor
 *	control port and then calls the appropriate handler.
 *
 * Calling/Exit State:
 *	On the second engine (engine 1), this routine is called at PLHI with
 *	interrupts disabled.  Interrupts must remain disabled until the
 *	cross-processor interrupt is cleared.
 *
 *	On return, the engine is at PLHI with interrupts enabled.
 *
 * Description:
 *	The cross-processor interrupt is used to signal soft interrupts
 *	as well as xcall interrupts.  Thus, once psm_intr determines that
 *	the interrupt was actually a cross-processor interrupt (rather
 *	than a fpu interrupt), it does several steps:
 *		(1) Clear the interrupt bit in the control register
 *		(2) Enable interrupts at the CPU
 *		(3) Copy new soft interrupt flags (if any) to l.eventflags.
 *		(4) Call xcall_intr to handle a pending xcall if present.
 *		(5) Handle profiling if that was signalled.
 *		(6) Handle local clock processing if that was signalled.
 *
 * Remarks:
 *	On engine 0, the routine is called at PLHI with interrupts enabled;
 *	otherwise it is called with interrupts DISABLEd.
 *
 *	Note that the same interrupt is used for both soft interrupts and
 *	xcall interrupts.  There are a number of races possible, but these
 *	are all handled correctly because both the soft interrupt handler
 *	and the xcall handler are implemented such that they effectively
 *	do nothing if there is nothing to do.  See xcall_intr in xcall.c
 *	for more details.
 *
 *	atomic_fnc is used to read and clear the psm_eventflags entry for
 *	this engine atomically using a bus lock.  A bus lock is used rather
 *	than a software lock for reasons given under Remarks for psm_sendsoft.
 *
 *	The flags in l.eventflags will be processed on the next return from
 *	trap or interrupt which goes to PLBASE.
 */
void
psm_intr(uint vec, uint oldpl, uint edx, uint ecx, uint eax, uint es, uint ds,
		uint eip, uint cs, uint efl)
{
	extern void xcall_intr(void);


	if ((*cpqmpfuncs->cpq_isxcall)()) {

		psm_clear_xintr(myengnum);

		if (psm_eventflags[myengnum] != 0)
			engine_evtflags |=
				atomic_fnc(&psm_eventflags[myengnum]);
		sti();

#ifdef NOTREQD
		/*
		 * See comments around int_handle in intr_p.s
		 */
		if (myengnum == BOOTENG) {
			outb(engine_ctl_port[myengnum], 
				inb(engine_ctl_port[myengnum]) & ~INTDIS);
		}
#endif /* NOTREQD */

		xcall_intr();

		if (sp_iomode & ASYMINTR) {
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
		}
	} else if ((*cpqmpfuncs->cpq_ecc_intr)()) {
		/*
		 * Handle correctable memory error interrupt.
		 */
		if (cpq_ecc_intr_hdlr == NULL)
			cpq_ecc_intr_hdlr = cpq_noop;
		(*cpq_ecc_intr_hdlr)();
	} else if ((*cpqmpfuncs->cpq_dma_intr)()) {
		/*
		 * Handle DMA chaining interrupt.
		 */
		if (cpq_dma_intr_hdlr == NULL)
			cpq_dma_intr_hdlr = cpq_noop;
		(*cpq_dma_intr_hdlr)();
	} else {
		/*
		 * Handle an FPU error interrupt:
		 *
		 * First, clear any exception. Note that we do not have
		 * to check if an external FPU is present to clear the
		 * exception because the interrupt-on-error functionality
		 * is compatible with the AT architecture (irq 13). See
		 * the following note from Compaq spec.
		 *
		 * Compaq Hardware Note:
		 * The numeric coprocessor logic controls the interface
		 * between the i486/Pentium and its internal numeric
		 * coprocessor. The interrrupt-on-error functionality
		 * is compatible with the AT architecture (irq 13) and
		 * prevents the system from executing additional coproc-
		 * essor instructions after an error occurs. The irq13
		 * service routine checks the numeric coprocessor error
		 * bit in the CPU control/status port (0x0c6a or 0xfc6a)
		 * or the extended irq13 control/status porc (0x0cc9) to
		 * determine if an error exists. If there is an error
		 * condition, the ISR writes to the NCP interrupt clear
		 * port (0x00f0) to clear the interrupt. After doing so,
		 * the numeric coprocessor may continue execution. 
		 */

		psm_clr_fpbusy();

		fpu_error();
	}
}


/*
 * void
 * psm_sendsoft(int engnum, uint_t arg)
 *	Post SW interrupt to (typically) another processor.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Post a software interrupt to a processor with the specified flags.
 *	The implementation uses the static array psm_eventflags; each entry
 *	in psm_eventflags corresponds to an engine and acts as a mailbox for
 *	soft interrupts to that engine.  A soft interrupt is sent by:
 *		(1) Atomically or'ing the specified flags into the
 *			psm_eventflags entry for the engine (using
 *			a bus lock prefix with an OR instruction)
 *		(2) Sending a cross-processor interrupt (directly through
 *			psm_send_xintr, not via xcall)
 *
 * Remarks:
 *	Since sendsoft is sometimes called with an fspinlock held, we
 *	can't acquire locks while doing a sendsoft.  Thus, sendsoft does
 *	not use the xcall mechanism, and it uses bus locking rather than
 *	a software lock to update psm_eventflags atomically.
 */
void
psm_sendsoft(int engnum, uint_t arg)
{

	if (engnum == myengnum)
		engine_evtflags |= arg;
	else {
		atomic_or(&psm_eventflags[engnum], arg);
		psm_send_xintr(engnum);
	}
}


/*
 * void
 * psm_timer_init(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
psm_timer_init(void)
{
	(*cpqmpfuncs->cpq_timer_init)();
}


/*
 * ulong_t
 * psm_time_get(psmtime_t *)
 *
 * Calling/Exit State:
 *	- save the current time stamp that is opaque to the base kernel
 *	  in psmtime_t.	  
 * Note:
 *	psm_configure() may not be called before psm_usec_time(). So
 *	an assert to check the system is in SYMINTR mode is incorrect.
 */

void
psm_time_get(psmtime_t *ptime)
{
        ptime->pt_lo = (*cpqmpfuncs->cpq_usec_time)();
        ptime->pt_hi = 0;
}


/*
 * subtracts the time stamp `src' from `dst', and stores the result in `dst'.
 */
void
psm_time_sub(psmtime_t *dst, psmtime_t *src)
{
        dst->pt_lo -= src->pt_lo;
}


/*
 * add the time stamp `src' to `dst', and stores the result in `dst'.
 */
void
psm_time_add(psmtime_t *dst, psmtime_t *src)
{
        dst->pt_lo += src->pt_lo;
}

/*
 * Convert the opaque time stamp to micro second.
 */
void
psm_time_cvt(dl_t *dst, psmtime_t *src)
{
        dst->dl_lop = src->pt_lo;
        dst->dl_hop = (long)src->pt_hi;
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)

uint_t  psm_usec_wait_count = 100000;
uint_t	psm_usec_rval[2];

/*
 * void
 * psm_usec_time_debug(void)
 *
 * Calling/Exit State:
 *      None.
 *
 * Description:
 *	This piece of code is to debug psm_usec_time(). Since we know 
 *      the time to execute the null for loop we can verify if the
 *	return value from psm_usec_time() is in fact what we expect.
 *
 *	Within kdb we can modify the psm_usec_wait_count value.
 */
void
psm_usec_time_debug(void)
{
        int     i, j;
	psmtime_t ptime;

	for (j = 0; j < 10; j++) {
		psm_time_get(&ptime);
		psm_usec_rval[0] = ptime.pt_lo;

		for (i = 0; i < psm_usec_wait_count; i++)
			;

		psm_time_get(&ptime);
		psm_usec_rval[1] = ptime.pt_lo;

		debug_printf("\tstart=0x%x, \tend=0x%x \tdiff=0x%x(%d)\n",
				psm_usec_rval[0], psm_usec_rval[1],
				(psm_usec_rval[1] - psm_usec_rval[0]),
				(psm_usec_rval[1] - psm_usec_rval[0]));
	}
}

#endif /* DEBUG || DEBUG_TOOLS */

/*
 * void
 * psm_selfinit(void)
 *      Performs any necessay per-engine initialization.
 *
 * Calling/Exit State:
 *      Called from selfinit() when the engine is brought online
 *      for the very first time.
 */
void
psm_selfinit(void)
{
	psm_clr_fpbusy();

	(*cpqmpfuncs->cpq_selfinit)();
}


/*
 * void
 * psm_intron(int iv, pl_t level, int engnum, int mpflag, int itype)
 *	Unmask the interrupt vector <iv> from the iplmask of engine <engnum>.
 *
 * Calling/Exit State:
 *	<iv> is the interrupt request no. that needs to be enabled.
 *	<engnum> is the engine on which the interrupt must be unmasked.
 *	<level> is the interrupt priority level of the iv.
 *	<mpflag> is the flag that indicates if the driver is multithreaded.
 *	<itype> is the interrupt type.
 *
 *	mod_iv_lock is held on entry/exit.
 *
 * Remarks:
 *	engnum		mpflag
 *	 -1		  1		Bind it to cpu 0   (Handled here)
 *	 !-1		  1		Bind it to cpu !-1 (Ignore intmp)
 *	 !-1		  0		Bind it to cpu !-1 (Ignore intmp)
 *	 -1		  0		Bind it to cpu 0   (DLM handles it
 *							    by passing the
 *							    correct engnum)
 */
void
psm_intron(int iv, pl_t level, int engnum, int mpflag, int itype)
{
	int e = engnum;
	extern void nenableint(int, pl_t, int, int);

	ASSERT(mpflag == 1);

	/*
	 * Ignore enabling of the inter-processor interrupt, since
	 * it is always enabled on each processor. 
	 */
	if (iv == XINTR)
		return;

	if (e == -1)
		e = (*cpqmpfuncs->cpq_assignvec)(iv, itype);

	ASSERT(e >= BOOTENG && e < Nengine);
	nenableint(iv, level, e, itype);
}


/*
 * void
 * psm_introff(int iv, pl_t level, int engnum, int itype)
 *	Mask the interrupt vector in the iplmask of the engine currently
 *	running on.
 *
 * Calling/Exit State:
 *	<iv> is the interrupt that needs to be disabled.
 *	<engnum> is the engine on which the interrupt must be masked.
 *	<level> is the interrupt priority level of the iv.
 *	<itype> is the interrupt type.
 *
 *	mod_iv_lock is held on entry/exit.
 */
void
psm_introff(int iv, pl_t level, int engnum, int itype)
{
	int e = engnum;
	extern void ndisableint(int, pl_t, int, int);

	/*
	 * Ignore disabling of the inter-processor interrupt, since
	 * it must always be enabled on each processor. 
	 */
	if (iv == XINTR)
		return;

	if (e == -1)
		e = (*cpqmpfuncs->cpq_unassignvec)(iv);

	ASSERT(e >= BOOTENG && e < Nengine);
	ndisableint(iv, level, e, itype);
}


/*
 * void
 * psm_ledctl(led_request_t, uint_t)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Noop for Syspro and Syspro/XL.
 */
/* ARGSUSED */
void
psm_ledctl(led_request_t req, uint_t led_bits)
{
}


/*
 * void
 * psm_misc_init(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
psm_misc_init(void)
{
	extern boolean_t soft_sysdump;
        uint_t bustype;

	if (myengnum == BOOTENG) {
		/*
		 * Initialize fail-safe timer or sanity clock.
		 */
		drv_gethardware(IOBUS_TYPE, &bustype);
		if (bustype & BUS_EISA)
			eisa_sanity_halt();
	}

	(*cpqmpfuncs->cpq_misc_init)();

	psm_clr_fpbusy();

	soft_sysdump = B_TRUE;
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
