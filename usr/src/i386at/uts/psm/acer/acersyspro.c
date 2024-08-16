/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/acer/acersyspro.c	1.2"
#ident	"$Header: $"

/*
 * ACER COMPAQ SYSTEMPRO COMPATIBLE INFO:
 *      - All interrupts are distributed only to processor 0.
 *	- Symmetric I/O access.
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

#include <io/ddi.h>		/* Must come last */

#include <acer.h>

/*
 * Configurable objects -- defined in psm.cf/Space.c
 */
extern ushort_t engine_ctl_port[];
extern ushort_t engine_ivec_port[];
extern int acer_maxnumcpu;
extern struct idt_init acer_idt0_init[], acer_idt1_init[];

extern void	acer_noop(void);

struct idt_init *asp_idt(int);
void		asp_intr_init(void);
void		asp_intr_start(void);
int		asp_numeng(void);
void		asp_bootcpu(int);
void		asp_cpuclrintr(int);
void		asp_cpuintr(int);
void		asp_timer_init(void);
ulong_t		asp_usec_time(void);
boolean_t	asp_isxcall(void);
int		asp_assignvec(int, int);
int		asp_unassignvec(int);


struct acerpsmops aspmpfuncs = {
	(void (*)())acer_noop,			/* acer_reboot */
	asp_idt,				/* acer_idt */
	asp_intr_init,				/* acer_intr_init */
	asp_intr_start,				/* acer_intr_start */
	(int (*)())acer_noop,			/* acer_numeng */
	(void (*)())acer_noop,			/* acer_configure */
	(void (*)())acer_noop,			/* acer_online_engine */
	(void (*)())acer_noop,			/* acer_selfinit */
	(void (*)())acer_noop,			/* acer_misc_init */
	(void (*)())acer_noop,			/* acer_offline_self */
	(void (*)())acer_noop,			/* acer_clear_xintr */
	(void (*)())acer_noop,			/* acer_send_xintr */
	asp_timer_init,				/* acer_timer_init */
	(ulong_t (*)())acer_noop,		/* acer_usec_time */
	(boolean_t (*)())acer_noop,		/* acer_isxcall */
	asp_assignvec,				/* acer_assignvec */
	asp_unassignvec				/* acer_unassignvec */
};


/*
 * struct idt_init *
 * asp_idt(int engnum)
 *
 * Calling/Exit State:
 *      Returns a pointer to an idt_init table for all the
 *      sub-platform-specific IDT entries for the given engine.
 */
struct idt_init *
asp_idt(int engnum)
{
	if (engnum == BOOTENG)
		return (acer_idt0_init);
	else
		return (acer_idt1_init);
}


/*
 * void
 * asp_intr_init(void)
 *	Initialize programmable interrupt controller.
 *
 * Calling/Exit State:
 *	Called when the system is being initialized.
 */
void
asp_intr_init(void)
{
	/*
	 * Initialize the i8259 based interrupt controller
	 */
	if (myengnum == BOOTENG) {
		picinit();
	}

	/*
	 * Initialize processor interrupt vector port.
	 *
	 * Boot engine receives cross-processor interrupt thru pics.
	 */

	if (myengnum != BOOTENG)
		/* Initialize P2's interrupt number to 13. */
		outb(engine_ivec_port[myengnum], PIC_VECTBASE + 13);

	/* Clear all interrupts */
	outb(engine_ctl_port[myengnum],
		inb(engine_ctl_port[myengnum]) & ~(PINT | INTDIS));
}


/*
 * void
 * asp_intr_start(void)
 *	Enable interrupts.
 *
 * Calling/Exit State:
 *	Called from selfinit() when the engine is being onlined. 
 */
void
asp_intr_start(void)
{
	if (upyet) {
		spl0();
		return;
	}

	picstart();
}


/*
 * int
 * asp_numeng(void)
 *
 * Calling/Exit State:
 *	Returns number of engines available in the system.
 *	It is used by conf_proc() to calloc space for engine
 *	structures.
 */
int
asp_numeng(void)
{
	if (engine_ctl_port[1] != 0xFF)
		/* number of engines equal 2 */
		return(2);
	else
		/* number of engines equal 1 */
		return(1);
}


/*
 * STATIC void
 * asp_bootcpu(char cpu)
 *	Boot additional CPUs.
 *
 * Calling/Exit State:
 *	None.
 */
void
asp_bootcpu(engnum)
{
	uchar_t		data;


	ASSERT(engnum < acer_maxnumcpu);
	ASSERT(myengnum == 0);

	/* disable engine's interrupt */
	data = inb(engine_ctl_port[engnum]);
	data |= INTDIS;
	outb(engine_ctl_port[engnum], data);

	/* clear engine's Mbus access bit to let it run */
	data = inb(engine_ctl_port[engnum]);
	data |= CACHEON;
	data &= ~PHOLD;
	outb(engine_ctl_port[engnum], data);

	/* enable engine's interrupt */
	data = inb(engine_ctl_port[engnum]);
	data &= ~INTDIS;
	outb(engine_ctl_port[engnum], data);
}


/*
 * boolean_t 
 * asp_isxcall(void)
 *
 * Calling/Exit State:
 *	Return 1 if the cross processor interrupt bit is asserted
 *	in the current engines processor control port, otherwise
 *	return 0.
 */
boolean_t
asp_isxcall(void)
{
	if (inb(engine_ctl_port[myengnum]) & PINT)
		return (B_TRUE);
	else
		return (B_FALSE);
}


/*
 * boolean_t
 * asp_isbooteng(int engnum)
 *
 * Calling/Exit State:
 *	Return 1 if executing on the boot engine (i.e processor 0 in
 *	Systempro mode), otherwise return 0.
 */
boolean_t
asp_isbooteng(int engnum)
{
	uchar_t whoami;


	/*
	 * In Syspro mode the whoami port contains
	 * - 0x00 if executing on processor 1 and
	 * - 0xF0 if executing on processor 2.
	 */
	if ((whoami = inb(WHOAMI_PORT)) == engnum)
		return (B_TRUE);
	else
		return (B_FALSE);
}


/*
 * void
 * asp_cpuclrintr(int engnum)
 *	To clear a CPU interrupt
 *
 * Calling/Exit State:
 *	None.
 */
void
asp_cpuclrintr(int engnum)
{
	outb(engine_ctl_port[engnum], inb(engine_ctl_port[engnum]) & ~PINT);
}


/*
 * void
 * asp_cpuintr(int engnum)
 *	Interrupt a CPU.
 *
 * Calling/Exit State:
 *	None.
 */
void
asp_cpuintr(int engnum)
{
	outb(engine_ctl_port[engnum], inb(engine_ctl_port[engnum]) | PINT);
}


/*
 * int 
 * asp_assignvec(int vec, int itype)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
asp_assignvec(int vec, int itype)
{
	return BOOTENG;
}


/*
 * int 
 * asp_unassignvec(int vec)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
asp_unassignvec(int vec)
{
	return BOOTENG; 
}


/*
 * void
 * asp_timer_init(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
asp_timer_init(void)
{
	/*
	 * Initialize the i8254 programmable interrupt timer.
	 */
	if (myengnum == BOOTENG) {
		clkstart();
	}
}


/*
 * ulong_t
 * asp_usec_time(void)
 *
 * Calling/Exit State:
 *	- Return the current time (free-running counter value)
 *	  in microseconds.
 */
ulong_t
asp_usec_time(void)
{
	extern ulong_t pit_usec_time(void);

	return (pit_usec_time());
}
