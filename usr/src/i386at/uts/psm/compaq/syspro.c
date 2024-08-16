/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/compaq/syspro.c	1.5"
#ident	"$Header: $"

/*
 * COMPAQ SYSTEMPRO HARDWARE INFO:
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

#include <syspro.h>
#include <xl.h>

/*
 * Configurable objects -- defined in psm.cf/Space.c
 */
extern int sp_maxnumcpu;			/* max. cpus in SP or SP/XL */
extern ushort_t engine_ctl_port[];
extern ushort_t engine_ivec_port[];

extern struct idt_init cpq_idt0_init[], cpq_idt1_init[];
extern uchar_t cpq_mptype;

extern void	cpq_noop(void);

void		sp_stopcpu(int);
struct idt_init *sp_idt(int);
void		sp_intr_init(void);
void		sp_intr_start(void);
int		sp_numeng(void);
void		sp_bootcpu(int);
void		sp_cpuclrintr(int);
void		sp_cpuintr(int);
void		sp_timer_init(void);
ulong_t		sp_usec_time(void);
boolean_t	sp_isxcall(void);
boolean_t	sp_ecc_intr(void);
boolean_t	sp_dma_intr(void);
int		sp_assignvec(int, int);
int		sp_unassignvec(int);


struct cpqpsmops spmpfuncs = {
	sp_stopcpu,				/* cpq_reboot */
	sp_idt,					/* cpq_idt */
	sp_intr_init,				/* cpq_intr_init */
	sp_intr_start,				/* cpq_intr_start */
	sp_numeng,				/* cpq_numeng */
	cpq_noop,				/* cpq_configure */
	sp_bootcpu,				/* cpq_online_engine */
	cpq_noop,				/* cpq_selfinit */
	cpq_noop,				/* cpq_misc_init */
	cpq_noop,				/* cpq_offline_self */
	sp_cpuclrintr,				/* cpq_clear_xintr */
	sp_cpuintr,				/* cpq_send_xintr */
	sp_timer_init,				/* cpq_timer_init */
	sp_usec_time,				/* cpq_usec_time */
	sp_isxcall,				/* cpq_isxcall */
	sp_ecc_intr,				/* cpq_ecc_intr */
	sp_dma_intr,				/* cpq_dma_intr */
	sp_assignvec,				/* cpq_assignvec */
	sp_unassignvec				/* cpq_unassignvec */
};


/*
 * void
 * sp_stopcpu(int engnum)
 *	Stop additional (P2) CPUs.
 *
 * Calling/Exit State:
 *	None.
 */
void
sp_stopcpu(int engnum)
{
	outb(engine_ctl_port[engnum], RESET | PHOLD);
}


/*
 * struct idt_init *
 * sp_idt(int engnum)
 *
 * Calling/Exit State:
 *      Returns a pointer to an idt_init table for all the
 *      sub-platform-specific IDT entries for the given engine.
 */
struct idt_init *
sp_idt(int engnum)
{
	if (engnum == BOOTENG)
		return (cpq_idt0_init);
	else
		return (cpq_idt1_init);
}


/*
 * void
 * sp_intr_init(void)
 *	Initialize programmable interrupt controller.
 *
 * Calling/Exit State:
 *	Called when the system is being initialized.
 */
void
sp_intr_init(void)
{
	if (myengnum == BOOTENG) {
		/*
		 * Initialize the i8259 based interrupt controller
		 */
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
 * psm_intr_start(void)
 *	Enable interrupts.
 *
 * Calling/Exit State:
 *	Called from selfinit() when the engine is being onlined. 
 */
void
sp_intr_start(void)
{
	if (upyet) {
		spl0();
		return;
	}

	picstart();
}


/*
 * int
 * sp_numeng(void)
 *
 * Calling/Exit State:
 *	Returns number of engines available in the system.
 *	It is used by conf_proc() to calloc space for engine
 *	structures.
 */
int
sp_numeng(void)
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
 * sp_bootcpu(char cpu)
 *	Boot additional CPUs.
 *
 * Calling/Exit State:
 *	None.
 */
void
sp_bootcpu(engnum)
{
	uchar_t		data;


	ASSERT(engnum < sp_maxnumcpu);
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
 * sp_isxcall(void)
 *
 * Calling/Exit State:
 *	Return 1 if the cross processor interrupt bit is asserted
 *	in the current engines processor control port, otherwise
 *	return 0.
 */
boolean_t
sp_isxcall(void)
{
	if (inb(engine_ctl_port[myengnum]) & PINT)
		return (B_TRUE);
	else
		return (B_FALSE);
}


/*
 * boolean_t
 * sp_isbooteng(int engnum)
 *
 * Calling/Exit State:
 *	Return 1 if executing on the boot engine (i.e processor 0 in
 *	Systempro mode), otherwise return 0.
 */
boolean_t
sp_isbooteng(int engnum)
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
 * sp_cpuclrintr(int engnum)
 *	To clear a CPU interrupt
 *
 * Calling/Exit State:
 *	None.
 */
void
sp_cpuclrintr(int engnum)
{
	outb(engine_ctl_port[engnum], inb(engine_ctl_port[engnum]) & ~PINT);
}


/*
 * void
 * sp_cpuintr(int engnum)
 *	Interrupt a CPU.
 *
 * Calling/Exit State:
 *	None.
 */
void
sp_cpuintr(int engnum)
{
	outb(engine_ctl_port[engnum], inb(engine_ctl_port[engnum]) | PINT);
}


/*
 * int 
 * sp_assignvec(int vec, int itype)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
sp_assignvec(int vec, int itype)
{
	return BOOTENG;
}


/*
 * int 
 * sp_unassignvec(int vec)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
sp_unassignvec(int vec)
{
	return BOOTENG; 
}


/*
 * void
 * sp_timer_init(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
sp_timer_init(void)
{
	if (myengnum == BOOTENG) {
		/*
		 * Initialize the i8254 programmable interrupt timer.
		 */
		clkstart();
	}
}


/*
 * ulong_t
 * sp_usec_time(void)
 *
 * Calling/Exit State:
 *	- Return the current time (free-running counter value)
 *	  in microseconds.
 */
ulong_t
sp_usec_time(void)
{
	extern ulong_t pit_usec_time(void);

	return (pit_usec_time());
}


/*
 * boolean_t
 * sp_ecc_intr(void)
 *
 * Calling/Exit State:
 *	Returns B_TRUE if it is an ECC interrupt, otherwise return B_FALSE.
 */
boolean_t
sp_ecc_intr(void)
{
	if ((cpq_mptype != CPQ_SYSTEMPRO_COMPATIBLE) &&
	    (inb(INT13_XSTATUS_PORT) & INT13_ECC_MEMERR_ACTIVE))
		return B_TRUE;
	else
		return B_FALSE;
}


/*
 * boolean_t
 * sp_dma_intr(void)
 *
 * Calling/Exit State:
 *	Returns B_TRUE if it is a DMA chaining interrupt, otherwise 
 *	return B_FALSE.
 */
boolean_t
sp_dma_intr(void)
{
	if ((cpq_mptype != CPQ_SYSTEMPRO_COMPATIBLE) && 
	    (inb(INT13_XSTATUS_PORT) & INT13_DMA_CHAIN_ACTIVE))
		return B_TRUE;
	else
		return B_FALSE;
}
