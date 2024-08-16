/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/acer/acer.c	1.16"
#ident	"$Header: $"

/*
 * Multiprocessor PSM/mHAL hardware-specific routines and data.
 *
 * This file contains a set of subroutines and a set of data, all of which
 * are used elsewhere in the MP kernel but which vary for MP hardware from
 * different vendors. In general, all the routines with an "psm_" prefix
 * should be defined for whatever hardware you use, even if the routine does
 * nothing. The same applies for data (unfortunately the data names may not
 * have a distinctive prefix), even if it might not be used. Some routines
 * and data defined in this file are for specific platforms only. For
 * instance, the external cache allocation routines are specific to Acer
 * implementation. Vendors do not need to support these items for their
 * own hardware.
 *
 * The requirements for each item in this file are clearly stated in the 
 * PSM/mHAL interface specification document.
 *
 * This MP PSM/mHAL file supports the ACER Interrupt Distribution Board (IDB).
 *
 * ACER HARDWARE INFO:
 *	- The system cannot have vacant processor slots between two
 *	  processor boards. All processor boards must be plugged in
 *	  sequentially.
 *
 *	- The slot 0 and 1 on the F or S bus are assigned as procesor slots
 *	  whereas slots 2-6 are multiplexed between processor and memory.
 *
 *	- Clock interrupt is distributed only to processor 0.
 *
 *	- The system supports three interrupt distribution modes:
 *		o Systempro Compatible mode
 *		o Asymmetric mode
 *		o Symmetric mode
 *
 *	- Local interrupts (0 and 13), interrupts for single-threaded
 *	  drivers and interrupts shared by different types of controllers
 *	  cannot be moved between CPUs.
 */

#include <io/cram/cram.h>
#include <io/conf.h>
#include <io/prf/prf.h>
#include <mem/immu.h>
#include <mem/vm_mdep.h>
#include <mem/vmparam.h>
#include <proc/seg.h>
#include <proc/tss.h>
#include <svc/bootinfo.h>
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

#include <io/ddi.h>	/* Must come after other kernel headers */
#include <io/ddi_i386at.h>

#include <acer.h>

/*
 * TODO:
 *	- Support Asymmetric mode.
 */

#define	IO_ROM_INIT	0xfff0
#define	IO_ROM_SEG	0xf000
#define	RESET_FLAG	0x1234
#define	SHUT_DOWN	0x8f
#define	SHUT5		5
#define	STAT_8042	0x64
#define I8042_RESETCPU	0xfe
#define FPU_BUSY_LATCH	0xf0

#define RAM_RELOC_REG	0x80C00000	/* RAM Relocation Register */
#define ENG1_CACHE_REG  0x80C00002	/* P1 Cache Control Register */

#define EXTERNAL_CACHEON	0x40	/* external cache enable bit */

#define RESET_VECT	0x467		/* reset vector location */

#define SOFTINT		63              /* software interrupt */

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


void		softreset(void);
void		acer_noop(void);

STATIC void	psm_clr_fpbusy(void);

STATIC int	acer_ckidstr(char *, char *, int);
STATIC void	acer_extcache_alloc(void);
STATIC void	acer_extcache_dealloc(void);
STATIC void	acer_i486cache0on(void);
STATIC void	acer_set_resetaddr(paddr_t);
STATIC void	acer_halt(void);
STATIC int	acer_isxcall(void);
STATIC boolean_t acer_findeng(int);

extern struct idt_init *asp_idt(int);
extern void asp_intr_init(void);

extern struct acerpsmops aidbmpfuncs;
extern struct acerpsmops aspmpfuncs;
extern boolean_t fpu_external;
extern ushort_t engine_ctl_port[];
extern ushort_t engine_ivec_port[];
extern int acer_eng_slot[];
extern int acer_maxnumcpu;

struct acerpsmops acerupfuncs = {
	(void (*)())acer_noop,			/* acer_reboot */
	asp_idt,				/* acer_idt */
	asp_intr_init,				/* acer_intr_init */
	(void (*)())acer_noop,			/* acer_intr_start */
	(int (*)())acer_noop,			/* acer_numeng */
	(void (*)())acer_noop,			/* acer_configure */
	(void (*)())acer_noop,			/* acer_online_engine */
	(void (*)())acer_noop,			/* acer_selfinit */
	(void (*)())acer_noop,			/* acer_misc_init */
	(void (*)())acer_noop,			/* acer_offline_self */
	(void (*)())acer_noop,			/* acer_clear_xintr */
	(void (*)())acer_noop,			/* acer_send_xintr */
	(void (*)())acer_noop,			/* acer_timer_init */
	(ulong_t (*)())acer_noop,		/* acer_usec_time */
	(boolean_t (*)())acer_noop,		/* acer_isxcall */
	(int (*)())acer_noop,			/* acer_assignvec */
	(int (*)())acer_noop			/* acer_unassignvec */
};

/*
 * Initialize the default acerpsmops to a UP.
 */
struct acerpsmops *acermpfuncs = &acerupfuncs;

/*
 * Common IDT structure for all engines in Acer Symmetric mode.
 */
struct idt_init acer_idt0_init[] = {
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
 * IDT table required for the system to run in Systempro mode and
 * Asymmetric mode. It is used to initialize engine 1-3 in these modes
 */
struct idt_init acer_idt1_init[] = {
	{	SOFTINT,	GATE_386INT,	softint,	GATE_KACC },
	{	DEVINTS + 13,	GATE_386INT,	devxcall,	GATE_KACC },
	{ 0 },
};

uchar_t acer_iomode;		/* i/o mode type */

#define str(s)  #s
#define xstr(s) str(s)


/*
 * void
 * acer_noop(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
acer_noop(void)
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
 *	platform-specific boot argument parsing
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
	return -1;		/* unknown argument */
}


/*
 * void
 * acer_reset(void *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
void
acer_reset(void *arg)
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
 * 		flag != 1	halt the system and wait for user interaction
 * 		flag == 1	automatic reboot, no user interaction required
 */
void
psm_reboot(int flag)
{
	uint_t	bustype;


	splhi();

	/* Halt the non-boot engine excluding self. */
	acer_halt();

        /*
         * If sanity timer in use, turn off to allow clean soft reboot.
         */
        drv_gethardware(IOBUS_TYPE, &bustype);
        if (bustype & BUS_EISA)
                eisa_sanity_halt();

	reboot_prompt(flag);

	softreset();

	if (myengnum == BOOTENG) {
		acer_reset(NULL);
	} else {
		emask_t	targets;

		EMASK_INIT(&targets, BOOTENG);
		xcall(&targets, NULL, acer_reset, NULL);
	}

	/*
	 * Prohibit processor from gaining access to M bus and
	 * assert the reset line for non-boot self engine.
	 */
	if (myengnum != BOOTENG)
		outb(engine_ctl_port[myengnum], RESET | PHOLD);

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
 * psm_idt(uint_t engnum)
 *
 * Calling/Exit State:
 *	Returns a pointer to an idt_init table for all the 
 *	sub-platform-specific IDT entries for the given engine.
 *
 * Remarks:
 *	This routine must be called before loading the IDTR from selfinit().
 */
struct idt_init *
psm_idt(int engnum)
{
	return ((*acermpfuncs->acer_idt)(engnum));
}


/*
 * void
 * psm_intr_init(void)
 *
 * Calling/Exit State:
 *	Called when the system is being initialized.
 *
 * Remarks:
 *	Do not initialize PIC if the system is jumpered to
 *	asymmetric interrupt distribution mode and is a 
 *	non-boot engine.
 */
void
psm_intr_init(void)
{
	(*acermpfuncs->acer_intr_init)();
}


/*
 * void
 * psm_intr_start(void)
 *	Enable interrupts.
 *
 * Calling/Exit State:
 *	Called from selfinit() when the engine is being onlined.
 */
void
psm_intr_start(void)
{
	(*acermpfuncs->acer_intr_start)();
}


/*
 * STATIC boolean_t
 * acer_findeng(int slot)
 *	Check for a CPU board in the EISA <slot>.
 *
 * Calling/Exit State:
 *	- <slot> is the eisa socket no. of the cpu board id.
 *
 *	- Return B_TRUE if an engine is found in the <slot>, otherwise
 *	  return B_FALSE.
 */
STATIC boolean_t
acer_findeng(int slot)
{
	char	eid[4];		/* EISA id */
	char	*cbuf;		/* character buffer */


	eisa_boardid(slot, eid);
	cbuf = eisa_uncompress(eid);

	/*
	 * All ACER Frame MP machines have AC[RS]32xx as CPU board eisa ID,
	 * But Acer Disk Array has ACR3251 or ACR3259 as board ID.
	 */
	if (strncmp(cbuf, "ACR32", 5) == 0) {
		if (strncmp(cbuf, "ACR3251" , 7) == 0)
			return B_FALSE;
		if (strncmp(cbuf, "ACR3259" , 7) == 0)
			return B_FALSE;
		return B_TRUE;
	}

	if (strncmp(cbuf, "ACS32", 5) == 0)
		return B_TRUE;

	return B_FALSE;
}

#ifdef DEBUG
uchar_t acer_bios_port_val;
#endif /* DEBUG */

/*
 * STATIC void
 * acer_extcache_alloc(void)
 *	Turn of write allocate on a 50mhz write-back cache.
 *
 * Calling/Exit State:
 *	Calling when an engine is onlined.
 *
 * Remarks:
 *	"Turn on write allocate" is a feature that allows the 
 *	allocation of a cache entry on a write/miss. The entire
 *	cache operates in write-back mode. If write allocation is 
 *	not enabled then no cache entry will be allocated on write 
 *	misses. Not allocating cache entries on write misses has  
 *	detrimental effects on performance in particular for stacks.
 *
 *	Apparently, there were some initial compatability problems with 
 *	one or more operating systems that caused the designer to make 
 *	this a program controlled feature.
 */
STATIC void
acer_extcache_alloc(void)
{
	uchar_t  shadow_ram_setup, high_ram_setup;


	/*
	 * Turn on write allocate on 50 mhz write-back cache for CPU 0
	 * also for CPU's 1, 2, and 3, but first need to know
	 * BIOS setup values for Shadow ram, and use of 15MB-16MB ram
	 */

	/*
	 *  retrieve BIOS setup shadow ram status
	 */
	outb(BIOS_SETUP_PORT, 0x39);

	/*
	 * read in byte, mask off bit 0 - 1-RAM BIOS 0-ROM BIOS
	 */
	/*
	 * shadow ram setup:
	 * bit 0 - 1-BIOS Shadow 0=No Shadowing
	 */
	shadow_ram_setup = (inb(BIOS_BINFO_PORT) & RAM_ROM_MASK);

	/*
	 *  retrieve BIOS setup 15MB-16MB ram status
	 */
	outb(BIOS_SETUP_PORT, 0x35);

	/*
	 * read in byte, mask off bit 1 -
	 * 	1-(15MB-16MB) DRAM   0-(15MB-16MB) EISA
	 */
	/*
	 * 15MB-16MB memory setup:
	 *	bit 5 1=Not Local Memory 16MB 0=Local Memory 16MB
	 */
	high_ram_setup = (inb(BIOS_BINFO_PORT) & DRAM_EISA_MASK);
	high_ram_setup ^= DRAM_EISA_MASK;
	high_ram_setup <<= 4;

	/*
	 * Turn on write allocate on 50 mhz write-back cache for CPU's
	 *	 bit 2	1=Write Allocation 
	 *		0=No Allocation
	 */
#ifdef DEBUG
	acer_bios_port_val = (WRITE_BALLOC_ON | shadow_ram_setup | high_ram_setup);
#endif /* DEBUG */

	/*
	 * 2 cpu board, cpus 0,1
	 */
	outb(BIOS_PORT_CPU01,
		(WRITE_BALLOC_ON | shadow_ram_setup | high_ram_setup));

	/*
	 * always write out to cpu 2,3, even if not there
	 */
	outb(BIOS_PORT_CPU23,
		(WRITE_BALLOC_ON | shadow_ram_setup | high_ram_setup));
}


/*
 * STATIC void
 * acer_extcache_dealloc(void)
 *	Turn off write allocate on 50 mhz write-back cache 
 *
 * Calling/Exit State:
 *	None
 *
 * Remarks:
 *	See comments above in acer_extcache_alloc() Remarks section.
 */
STATIC void
acer_extcache_dealloc(void)
{
	uchar_t  shadow_ram_setup, high_ram_setup;

	/*
	 * Turn off write allocate on 50 mhz write-back cache for CPU 0
	 * also for CPU's 1, 2, and 3 if present, but first need to know
	 * BIOS setup values for Shadow ram, and use of 15MB-16MB ram
	 */

	/*
	 *  retrieve BIOS setup shadow ram status
	 */
	outb(BIOS_SETUP_PORT, 0x39);

	/*
	 * read in byte, mask off bit 0 - 1-RAM BIOS 0-ROM BIOS
	 */
	/*
	 * shadow ram setup:
	 *	bit 0 - 1-BIOS Shadow 0=No Shadowing
	 */
	shadow_ram_setup = (inb(BIOS_BINFO_PORT) & RAM_ROM_MASK);

	/*
	 *  retrieve BIOS setup 15MB-16MB ram status
	 */
	outb(BIOS_SETUP_PORT, 0x35);

	/*
	 * read in byte, mask off bit 1 -
	 *	1-(15MB-16MB) DRAM   0-(15MB-16MB) EISA
	 */
	/*
	 * 15MB-16MB memory setup:
	 *	bit 5 1=Not Local Memory 16MB 0=Local Memory 16MB
	 */
	high_ram_setup = (inb(BIOS_BINFO_PORT) & DRAM_EISA_MASK);
	high_ram_setup ^= DRAM_EISA_MASK;
	high_ram_setup <<= 4;


	/*
	 * Turn off write allocate on 50 mhz write-back cache for CPU's
	 * i.e. bit 2 = 0 - No write allocation
	 */
	/*
	 * 2 cpu board, cpus 0,1
	 */
	outb(BIOS_PORT_CPU01, (shadow_ram_setup | high_ram_setup));

	/*
	 * always write out to cpu 2,3, even if not there
	 */
	outb(BIOS_PORT_CPU23, (shadow_ram_setup | high_ram_setup));
}


/*
 * STATIC void
 * acer_i486cacheon(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Enable internal on chip caches with write-thru cache bit off.
 */
STATIC void
acer_i486cacheon(void)
{
	asm(".set    CR0_CE, 0xbfffffff");
	asm(".set    CR0_WT, 0xdfffffff");
	asm("movl    %cr0, %eax");

	/* flush internal 486 cache */
	asm(".byte      0x0f");
	asm(".byte      0x09");

	asm("andl       $CR0_CE, %eax");
	asm("andl       $CR0_WT, %eax");

	/* flush queues */
	asm("jmp        i486flush1");
	asm("i486flush1:");
	asm("movl    %eax, %cr0");
}


/*
 * STATIC void
 * acer_i486cacheoff(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Calling/Exit State:
 *	Disable internal on-chip caches.
 */
STATIC void
acer_i486cacheoff(void)
{
	asm(".set    CR0_CD, 0x20000000");
	asm(".set    CR0_NW, 0x40000000");
	asm("movl    %cr0, %eax");

	/* flush internal 486 cache */
	asm(".byte      0x0f");
	asm(".byte      0x09");

	asm("orl        $CR0_CD, %eax");
	asm("orl        $CR0_NW, %eax");

	/* flush queues */
	asm("jmp        i486flush2");
	asm("i486flush2:");
	asm("movl    %eax, %cr0");
}


/*
 * STATIC void
 * acer_i486cache0on(void)
 *	Enable external cache.
 *
 * Calling/Exit State:
 *	None
 *
 * Note:
 *	Cannot be called from psm_configure() since vm subsystem
 *	is not initialized.
 */
STATIC void
acer_i486cache0on(void)
{
	caddr_t	addr;


	addr = physmap(RAM_RELOC_REG, 4, KM_NOSLEEP);
	/*
	 * Turn on the external cache enable bit in memory mapped i/o
	 * engine1 cache register (ENG1_CACHE_REG)
	 */
	addr[2] |= EXTERNAL_CACHEON;
	physmap_free(addr, 4, 0);
/*
	acer_i486cacheon();
*/
}


/*
 * int
 * psm_numeng(void)
 *	Detect no. of CPUs in the system.
 *
 * Calling/Exit State:
 *	Returns number of engines available in the system.
 *	It is called by conf_proc() to calloc space for engine
 *	structures.
 *
 * Note:
 *	Cannot have missing processor boards in the slots. IOW,
 *	a processor slot should not be empty between any two
 *	configured processors boards.
 */
int
psm_numeng(void)
{
	int	engnum;

	for (engnum = 1; engnum < acer_maxnumcpu; engnum++) {
		if (!acer_findeng(acer_eng_slot[engnum]))
			break;
	}

	if (engnum > acer_maxnumcpu) {
		/*
		 *+ Wrong slot number to check for a processor board in
		 *+ the system.
		 */
		cmn_err(CE_PANIC, "CPU board in an illegal slot.");
		/* NOTREACHED */
	} 

	return engnum;
}


/*
 * STATIC int
 * acer_ckidstr(char *, char *, int)
 *
 * Calling/Exit State:
 *	Return 1, if the string stored at addrp is equal
 *	to the string passed in the argument, otherwise
 *	return 0.
 */
STATIC int
acer_ckidstr(char *addrp, char *strp, int cnt)
{
	int	tcnt;


	for (tcnt = 0; tcnt < cnt; tcnt++) {
		if (*addrp != strp[tcnt]) {
			addrp++;
			continue;
		}

		if (strncmp(addrp, strp, strlen(strp)) == 0) {
			return (1);
		}
		addrp++;
	}

	return (0);
}


/*
 * void
 * psm_configure(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Gather miscellaneous hardware configuration information.
 */
void
psm_configure(void)
{
#define	ACER_IOMODE()		(inb(IOMODE_PORT))

	caddr_t addr;
	boolean_t found = B_FALSE;

	/*
	 * Determine if its an ACER machine (search
	 * 0xfe000-0xfe100 or 0xf4000-0x103fff for
	 * an ACER string)
	 */

	addr = physmap((paddr_t)0xFE000, 0x100, KM_NOSLEEP);
	if (acer_ckidstr((char *)addr, "ACER", 4))
		found = B_TRUE;

	addr = physmap((paddr_t)0xF4000, 0xFFFF, KM_NOSLEEP);
	if (acer_ckidstr((char *)addr, "ACER", 4))
		found = B_TRUE;

	if (!found) {
		/*
		 *+ Could not verify the identity of this machine.
		 *+ Check if you have correctly configured PSM/HAL
		 *+ module for the Acer Machine.
		 */
		cmn_err(CE_WARN,
			"!Incorrect PSM for the machine."
			" Verify if the PSM is for the Acer machine.");
	} else {
		cmn_err(CE_CONT,
			"!Found an Acer machine ...\n");
	}

	/* i/o mode type -- symmetric, asymmetric, systempro */	
	acer_iomode = ACER_IOMODE();

	if (acer_iomode & SYMINTR)
		acermpfuncs = &aidbmpfuncs;
	else 
		acermpfuncs = &aspmpfuncs;

	psm_clr_fpbusy();	/* TEMP; until sysinit reorg */
}


/*
 * STATIC void
 * acer_set_resetaddr(paddr_t)
 *	Set the starting address of the target engine
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
acer_set_resetaddr(paddr_t startaddr)
{
	struct resetaddr {
		ushort_t  offset;
		ushort_t  segment;
	} start;
	paddr_t addr;
	char    *vector, *source;
	int      i;


	addr = startaddr;

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
void
psm_online_engine(int engnum, paddr_t startaddr, int flag)
{
	uchar_t		data;


	if (flag == WARM_ONLINE)
		return;

	ASSERT(flag == COLD_ONLINE);

	/* set up the starting address of the target cpu */
	(void) acer_set_resetaddr(startaddr);

	/* disable engines interrupt */
	data = inb(engine_ctl_port[engnum]);
	data |= INTDIS;
	outb(engine_ctl_port[engnum], data);

	/* clear  engines Mbus access bit to let it run */
	data = inb(engine_ctl_port[engnum]);
	data &= ~PHOLD;
	data |= CACHEON;
	outb(engine_ctl_port[engnum], data);

	/* enable engines interrupt */
	data = inb(engine_ctl_port[engnum]);
	data &= ~INTDIS;
	outb(engine_ctl_port[engnum], data);
}


/*
 * void
 * psm_offline_self(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Do any necessary work to redistribute interrupts.
 */
void
psm_offline_self(void)
{
	(*acermpfuncs->acer_offline_self)();
}


/*
 * STATIC void
 * acer_halt(void)
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
 * Remarks:
 *	Do not need to reassign interrupts that were distributed to the
 *	non-boot engines back to the boot engine since all the drivers 
 *	are halted before rebooting the system and no interrupts would 
 *	be received on any engine.
 *
 *	The RESET, PHOLD and CACHEON bits for the CPU0 engine 
 *	control port (0x0c6a) are not used.
 */
STATIC void
acer_halt(void)
{
	int	engnum = BOOTENG; 

	/*
	 * Switch-off external cache write allocation.
	 */
	acer_extcache_dealloc();

	while (engnum < Nengine) {
		if (engnum == BOOTENG || engnum == myengnum) {
			engnum++;
			continue;
		}

		/*
		 * Prohibit processor from gaining access to M bus and
		 * assert the reset line for non-boot engine.
		 */
		outb(engine_ctl_port[engnum], RESET | PHOLD);

		engnum++;
	}
}


/*
 * STATIC int
 * acer_isxcall(void)
 *
 * Calling/Exit State:
 *	Return 1 if the cross processor interrupt bit is asserted
 *	in the current engines processor control port, otherwise
 *	return 0.
 */
STATIC int
acer_isxcall(void)
{
	return (inb(engine_ctl_port[myengnum]) & PINT);
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

	outb(engine_ctl_port[engnum], inb(engine_ctl_port[engnum]) & ~PINT);
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
	outb(engine_ctl_port[engnum], inb(engine_ctl_port[engnum]) | PINT);
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
 * Array of eventflags, one per engine, used in implementation of sendsoft
 */
STATIC volatile uint_t psm_eventflags[MAXNUMCPU];

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
 *	This routine is called at PLHI with interrupts enabled.
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
/* ARGSUSED */
void
psm_intr(uint vec, uint oldpl, uint edx, uint ecx, uint eax, uint es, uint ds,
		uint eip, uint cs, uint efl)
{
	extern void xcall_intr(void);

	if (acer_isxcall()) {
		psm_clear_xintr(myengnum);

		if (psm_eventflags[myengnum] != 0)
			engine_evtflags |=
				atomic_fnc(&psm_eventflags[myengnum]);

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
	} else {
		if (!(fpu_external))
			return;

		/*
		 * Handle an FPU error interrupt:
		 *
		 * First, clear any exception.
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
 * psm_selfinit(void)
 *	Performs any necessay per-engine initialization.	
 *
 * Calling/Exit State:
 *	Called from selfinit() when the engine is brought online
 *	for the very first time.
 *
 * Note:
 *	The internal on-chip cache is already enabled for the boot engine.
 */
void
psm_selfinit(void)
{
	psm_clr_fpbusy();

	if (myengnum == BOOTENG) {
		/*
		 * Enable the external and internal on-chip cache.
		 *
		 * The external caches must always be enabled, and this
		 * is done thru cmos. Anyway, enable it if any software
		 * contol mechanism is available to enable/disable caches. 
		 */
		acer_i486cache0on();

		/*
		 * Write-allocate external write-back cache on the 50MHz
		 * AcerFrame 3000MP. 
		 */
		acer_extcache_alloc();
		
		return;
	}
}


/*
 * void
 * psm_intron(int iv, pl_t level, int engnum, int intmp, int itype)
 *	Unmask the interrupt vector iv from the iplmask of engine engnum.
 *
 * Calling/Exit State:
 *	<iv> is the interrupt request no. that needs to be enabled.
 *	<engnum> is the engine on which the interrupt must be unmasked.
 *	<level> is the interrupt priority level of the iv.
 *	<intmp> is the flag that indicates if the driver is multithreaded.
 *	<itype> is the interrupt type.
 *
 *	mod_iv_lock spin lock is held on entry/exit.
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
		e = (*acermpfuncs->acer_assignvec)(iv, itype);

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
 *	mod_iv_lock spin lock is held on entry/exit.
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
		e = (*acermpfuncs->acer_unassignvec)(iv);

	ASSERT(e >= BOOTENG && e < Nengine);
	ndisableint(iv, level, e, itype);
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
	(*acermpfuncs->acer_timer_init)();
}


/*
 * ulong_t
 * psm_time_get(psmtime_t *)
 *
 * Calling/Exit State:
 *	- save the current time stamp that is opaque to the base kernel
 *	  in psmtime_t.	  
 */

void
psm_time_get(psmtime_t *ptime)
{
	extern ulong_t pit_usec_time(void);

        ptime->pt_lo = pit_usec_time();
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


/*
 * void 
 * psm_ledctl(led_request_t, uint_t)
 *
 * Calling/Exit State:
 *	Called when a processor is onlined and before/after the processor
 *	enters/exit from the idle state.
 *
 * Remarks:
 *	Noop for acer.
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
	extern void ecc_init(void);
	extern int ecc_merrhandler(int *);
	extern boolean_t soft_sysdump;
	uint_t bustype;

	if (myengnum == BOOTENG) {
		/*
		 * Initialize fail-safe timer or sanity clock.
		 */
		drv_gethardware(IOBUS_TYPE, &bustype);
		if (bustype & BUS_EISA)
			eisa_sanity_halt();

		/*
		 * Identify type of memory -- ECC or EDC 
		 */
		ecc_init();

		/*
		 * Start mem error handler, timeout(5 sec) in
		 * ecc_merrhandler(). It is a periodic check
		 * for single-bit and mulitple-bit errors.
		 */
		ecc_merrhandler(0);

		/*
		 * System dump needs to be done in software
		 */
		soft_sysdump = B_TRUE;

		return;
	}

	/*
	 * Do any further initialization required on a platform like
	 * interrupt distribution.
	 */
	(*acermpfuncs->acer_misc_init)();
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
 * 	Dump platform specific information to memory upon system panicing.
 *
 * Calling/Exit State:
 * 	None.
 */

void
psm_panic()
{
}
