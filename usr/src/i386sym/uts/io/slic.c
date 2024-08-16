/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/slic.c	1.12"
#ident	"$Header: $"

/*
 * Routines to access the SLIC.
 */

#include <util/types.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/plocal.h>
#include <mem/vmparam.h>
#include <mem/vm_mdep.h>
#include <mem/kmem.h>
#include <io/slic.h>
#include <svc/clock.h>
#include <util/ksynch.h>
#include <svc/systm.h>

/* private functions */
static int  slic_rdData(int);
static void slic_wrData(int, int);

STATIC fspin_t slave_lock;		/* serialize slave read/write */
LKINFO_DECL(slave_lkinfo, "SLIC slave lock", 0);
STATIC fspin_t mIntr_lock;		/* serialize in mIntr */
STATIC int enable_local_clock = 1;	/* for SLIC local clock control */

int	slic_prishift;			/* used by the priority mapper */

/*
 * void
 * slic_init(void)
 *	Initialize locks used to serialize SLIC access.
 *
 * Calling/Exit State:
 *	Returns: none.
 *
 * Remarks:
 *	Called from sysinit() before first lock usage.
 */
void
slic_init(void)
{
	FSPIN_INIT(&slave_lock);
	FSPIN_INIT(&mIntr_lock);
}

/*
 * void slic_priinit(int nglobpris)
 *
 *	Initialize the slic's priority mapper.
 *
 * Calling/Exit State:
 *
 *	Called from system initialization.
 *
 * Description:
 *
 *	The mapping from a global priority is designed to be simple
 *	arithmetically and to increase the granularity at the lower
 *	end of the priority range.  I.e. we know there are more global
 *	priorities then slic priorities (the SLIC has 32 priorities) and
 *	the SLIC sense of priority is reversed from the global priority
 *	(i.e. zero is a better priority then 31).
 *
 *	One attribute of our mapping function will be the distribution
 *	of the global priorities onto the SLIC priorities.
 *	If we used a divide, we could make this mapping uniform (i.e.
 *	the same number of global priority bands would map into each
 *	SLIC priority band).  If we use just a shift and subtract, we
 *	can have the same number of global priority bands in the first
 *	31 SLIC priority bands, with the rest piled into the last priority
 *	band.
 *
 *	The current mapping gives lower resolution at the lower global
 *	priorities (TS) and less at the higher priorities (RT), as
 *	an interrupt goes to the lowest priority engine.  High priority
 *	engine's usually aren't considered, thus we usually need to
 *	choose between low priority engine's.  Having better resolution
 *	at this end of the range facilitates this.
 */
void
slic_priinit(int nglobpris)
{
	register int i;

	for (i=0; 1 << i+1 <= nglobpris; i++)
		continue;

	slic_prishift = i;
}

/*
 * int
 * slic_rdslave(int, int)
 *
 *	Read a SLIC slave port.
 *
 * Calling/Exit State:
 *
 *	Assumes the slave_lock is not already held (no recursive calls
 *	are allowed).  Aquires the slave_lock, interacts with the SLIC
 *	to read a slave register, unlocks the lock, and returns the value.
 */

int
slic_rdslave(int dest, int reg)
{
	int	val;

	FSPIN_LOCK(&slave_lock);

	slic_wrAddr(dest, reg);			/* write address */
	val = slic_rdData(dest);		/* read data */

	FSPIN_UNLOCK(&slave_lock);
	return (val);
}

/*
 * int
 * slic_lrdslave(int, int)
 *
 *	Read a SLIC slave port (without aquiring the lock).
 *
 * Calling/Exit State:
 *
 *	Interacts with the SLIC to read a slave register,
 *	and returns the value.
 */

int
slic_lrdslave(int dest, int reg)
{
	slic_wrAddr(dest, reg);
	return slic_rdData(dest);
}

/*
 * void
 * slic_wrslave(int, int, int)
 *
 *	Write data to a slave port.
 *
 * Calling/Exit State:
 *
 *	Assumes the slave_lock is not already held (no recursive calls
 *	are allowed).  Aquires the slave_lock, interacts with the SLIC
 *	to write a slave register, unlocks the lock, and returns.
 */

void
slic_wrslave(int dest, int reg, int data)
{
	FSPIN_LOCK(&slave_lock);

	slic_wrAddr(dest, reg);			/* write address */
	slic_wrData(dest, data);		/* write data */

	FSPIN_UNLOCK(&slave_lock);
}

/*
 * slic_lwrslave()
 *	Write to a slave port. Don't acquire slave_lock.
 *
 * On panics, engines pausing themselves call slic_lwrslave to hold
 * themselves without acquiring slave_lock. Otherwise the lock would
 * never get released (slic_wrslave doesn't return) and all other engines
 * would block behind the lock.
 */
/*
 * void
 * slic_lwrslave(int, int, int)
 *
 *	Write a SLIC slave port (without aquiring the lock).
 *
 * Calling/Exit State:
 *
 *	On panics, engines pausing themselves call slic_lwrslave to hold
 *	themselves without acquiring slave_lock. Otherwise the lock would
 *	never get released (slic_wrslave doesn't return) and all other
 *	engines would block behind the lock.
 */

void
slic_lwrslave(int dest, int reg, int data)
{
	slic_wrAddr(dest, reg);		/* write address */
	slic_wrData(dest, data);	/* write data */
}

/*
 * void
 * slic_wrAddr(int, int)
 *
 *	Write address to a SLIC slave.
 *
 * Calling/Exit State:
 *
 *	This function assumes caller has provided sufficient locking
 *	on interacting with the SLIC.
 */

void
slic_wrAddr(int dest, int addr)
{
	volatile struct cpuslic *sl = (volatile struct cpuslic *)KVSLIC;

	sl->sl_dest = (unchar)dest;
	sl->sl_smessage = (unchar)addr;
	sl->sl_cmd_stat = (unchar)SL_WRADDR;

	do { /* nothing */ }
	while (sl->sl_cmd_stat & SL_BUSY);
}

/*
 * void
 * slic_wrSubslave(int, int, int, int)
 *
 *	Write a register that responds to SLIC slave sub-register addressing.
 *
 * Calling/Exit State:
 *
 *	For compatibility with diagnostic usage, if slave==0
 *	don't do the slic_wrAddr().
 */

void
slic_wrSubslave(int slic, int slave, int subreg, int val)
{
	FSPIN_LOCK(&slave_lock);

	if (slave != 0)
		slic_wrAddr(slic, slave);
	slic_wrData(slic, subreg);
	slic_wrData(slic, val);

	FSPIN_UNLOCK(&slave_lock);
}

/*
 * int
 * slic_rdSubslave(int, int, int)
 *
 *	Read a register that responds to SLIC slave sub-register addressing.
 *
 * Calling/Exit State:
 *
 *	For compatibility with diagnostic usage, if slave==0
 *	don't do the slic_wrAddr().
 */

int
slic_rdSubslave(int slic, int slave, int subreg)
{
	int	val;

	FSPIN_LOCK(&slave_lock);

	if (slave != 0)
		slic_wrAddr(slic, slave);
	slic_wrData(slic, subreg);
	val = slic_rdData(slic);

	FSPIN_UNLOCK(&slave_lock);
	return (val);
}

/*
 * static int
 * slic_rdData(int)
 *
 *	Read data from previously addressed slave register.
 *
 * Calling/Exit State:
 *
 *	Must be called at PLHI or greater.  If we get interrupted
 *	while setting up the sl_dest register and before writing the
 *	sl_cmd_stat register, the interrupter could do another slic
 *	related operation.  Upon return, we would initiate our
 *	command to the destination of the interrupter and not necessarily
 *	"dest".
 */

static int
slic_rdData(int dest)
{
	volatile struct cpuslic *sl = (volatile struct cpuslic *)KVSLIC;

	sl->sl_dest = (unchar)dest;
	sl->sl_cmd_stat = (unchar)SL_RDDATA;

	do { /* nothing */ }
	while (sl->sl_cmd_stat & SL_BUSY);

	return ((int)sl->sl_sdr);
}

/*
 * static int
 * slic_wdData(int, int)
 *
 *	Write data to previously addressed slave register.
 *
 * Calling/Exit State:
 *
 *	Must be called at PLHI or greater for reasons discussed in
 *	slic_rdData commentary.
 */

static void
slic_wrData(int dest, int data)
{
	volatile struct cpuslic *sl = (volatile struct cpuslic *)KVSLIC;

	sl->sl_dest = (unchar)dest;
	sl->sl_smessage = (unchar)data;
	sl->sl_cmd_stat = (unchar)SL_WRDATA;

	do { /* nothing */ }
	while (sl->sl_cmd_stat & SL_BUSY);
}

/*
 * void
 * slic_sendsoft(int, int)
 *
 *	Post SW interrupt to (typically) another processor.
 *
 * Calling/Exit State:
 *
 *	Must be called at PLHI or greater for reasons discussed in
 *	slic_rdData commentary.  There is no chance for deadlock here,
 *	as the destination slic will always accept software interrupts,
 *	no matter what the masking.
 */

void
slic_sendsoft(int dest, int bitmask)
{
	volatile struct cpuslic *sl = (volatile struct cpuslic *)KVSLIC;

	sl->sl_dest = (unchar)dest;
	sl->sl_smessage = (unchar)bitmask;
	sl->sl_cmd_stat = (unchar)SL_MINTR;

	do { /* nothing */ }
	while (sl->sl_cmd_stat & SL_BUSY);
}

/*
 * void
 * slic_nmIntr(int, int)
 *
 *	Post NMI interrupt to somebody.  Used to post send NMI
 *	to a processor to have it shut down.
 *
 * Calling/Exit State:
 *
 *	Must be called at PLHI or greater for reasons discussed in
 *	slic_rdData commentary.  There is no potential for processor-to-
 *	processor deadlocks here, as the destination SLIC always accepts
 *	NMI's.
 */

void
slic_nmIntr(int dest, int message)
{
	volatile struct cpuslic *sl = (volatile struct cpuslic *)KVSLIC;
	int stat;

	sl->sl_dest = (unchar)dest;
	sl->sl_smessage = (unchar)message;

	do {
		sl->sl_cmd_stat = (unchar)SL_NMINTR;
		do { /* nothing */ }
		while ((stat = sl->sl_cmd_stat) & SL_BUSY);
	} while ((stat & SL_OK) == 0);
}

/*
 * void
 * slic_mIntr(int, int, int)
 *
 *	Post HW interrupt to somebody.  Used to send commands to
 *	various peripheral controllers.  slic_mIntr's to processors
 *	has potential for deadlock as mentioned below.
 *
 * Calling/Exit State:
 *
 *	Implementation acquires a spin lock before sending message to
 *	avoid SLIC bus saturation.  There is potential for deadlock here,
 *	if two processors are attempting to exchange interrupts while
 *	masking interrupts, neither slic will accept the interrupt and
 *	both processors will wait indefinitely for the other slic to
 *	accept the interrupt.  mIntr_lock helps a bit, as it prevents
 *	two processors from attempting this, however, if both processors
 *	are masking the interrupt being sent prior to calling slic_mIntr,
 *	a similar deadlock will result:  one processor will gain mIntr_lock,
 *	and spin waiting for the interrupt to be accepted, while the other
 *	processor spins against the lock.
 */

void
slic_mIntr(int dest, int bin, int data)
{
	volatile struct cpuslic *sl = (volatile struct cpuslic *)KVSLIC;
	int	stat;

	FSPIN_LOCK(&mIntr_lock);

	sl->sl_dest = (unchar)dest;
	sl->sl_smessage = (unchar)data;
	do {
		sl->sl_cmd_stat = (unchar)(SL_MINTR | bin);
		do { /* nothing */ }
		while ((stat = sl->sl_cmd_stat) & SL_BUSY);
	} while ((stat & SL_OK) == 0);

	FSPIN_UNLOCK(&mIntr_lock);
}

/*
 * void
 * slic_setgm(int, int)
 *
 *	Set group mask in destination SLIC.
 *
 * Calling/Exit State:
 *
 *	Must be called at PLHI or greater for reasons discussed in
 *	slic_rdData commentary.
 */

void
slic_setgm(int dest, int mask)
{
	volatile struct cpuslic *sl = (volatile struct cpuslic *)KVSLIC;

	sl->sl_dest = (unchar)dest;
	sl->sl_smessage = (unchar)mask;
	sl->sl_cmd_stat = (unchar)SL_SETGM;
	do { /* nothing */ }
	while (sl->sl_cmd_stat & SL_BUSY);
}

/*
 * void
 * startrtclock(void)
 *
 *	Start the real-time clock.
 *
 * Calling/Exit State:
 *
 *	Startrtclock restarts the real-time clock, which provides
 *	hardclock interrupts to kern_clock.c.  On Sequent HW, this
 *	is one-time only per processor (eg, no restart, clock reprimes
 *	itself).
 *
 *	Called by localinit() during selfinit().
 *	This turns on the processor-local SLIC timer.
 *
 * Remarks:
 *
 *	For testing/performance measurement convenience, enable_local_clock
 *	allows the per-processor clock to be left OFF.  Need to patch the
 *	kernel binary or system memory to effect this.
 */

void
startrtclock(void)
{
	volatile struct cpuslic *sl = (volatile struct cpuslic *)KVSLIC;

	if (enable_local_clock == 0)
		return;

	sl->sl_trv = ((sys_clock_rate * 1000000) / (SL_TIMERDIV * HZ)) - 1;
	/* clear prescaler, load reload value */
	sl->sl_tcont = 0;
	sl->sl_tctl = (SL_TIMERINT | LCLKBIN);	/* timer on in given bin */
}

/*
 * void slic_flush_intr(void)
 *
 *	Flush pending interrupts.
 *
 * Calling/Exit State:
 *
 * 	Called from offline_self in order to flush any pending interrupts
 *	out of the slic.
 *
 * Description:
 *
 *	Used when shutting down processor to insure pending interrupts
 *	are cleared (and handled).
 */
void
slic_flush_intr(void)
{
	volatile struct cpuslic *sl = (volatile struct cpuslic *)KVSLIC;
	int counter;

	/*
	 * While there is a pending (HW or SW) interrupt, open
	 * a window to let it in.
	 *
	 * Before doing this, we set the slic interrupt arbitration
	 * priority to its highest value in order to discourage
	 * awards to this processor.
	 */
	sl->sl_ipl = 0;

	splblockall();
	for (;;) {
		if ((sl->sl_ictl & (SL_HARDINT|SL_SOFTINT)) == 0)
			/*
			 * There are no interrupts pending.
			 */
			break;
		(void) spl0();
		for (counter = 0; counter < 10; counter++)
			continue;		/* window to take int */
		splblockall();
	}
}

