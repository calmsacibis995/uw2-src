/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:svc/pit.c	1.17"
#ident	"$Header: $"


/*
 *         INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *     This software is supplied under the terms of a license 
 *    agreement or nondisclosure agreement with Intel Corpo-
 *    ration and may not be copied or disclosed except in
 *    accordance with the terms of that agreement.
 */


#include <svc/bootinfo.h>
#include <svc/clock.h>
#include <svc/pit.h>
#include <svc/psm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/dl.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/types.h>


/*
 * TODO:
 *	1. Optimize pit_usec_time()
 *
 *	2. Add another generic interface that would return the timer 0
 *	   free-flowing counter value. The massaging of the counter value
 *	   in this case would be done by the caller and not by this function.
 *
 *	3. Remove clkstart and convert it into a proper driver, s.t the
 *	   PIT would be initialized along with other drivers.
 */

/* #define TIME_CHECK		/* Debugging */
/* #define TIME_CHECKD		/* detailed Debugging */

int pitctl_port  = PITCTL_PORT;		/* For 386/20 Board */
int pitctr0_port = PITCTR0_PORT;	/* For 386/20 Board */
int pitctr1_port = PITCTR1_PORT;	/* For 386/20 Board */
int pitctr2_port = PITCTR2_PORT;	/* For 386/20 Board */
/* We want PIT 0 in square wave mode */
int pit0_mode = PIT_C0|PIT_SQUAREMODE|PIT_READMODE;

extern long microdata;			/* loop count for _tenmicrosec() wait */
unsigned int clknumb = CLKNUM;		/* interrupt interval for timer 0 */

boolean_t pit_initialized;

STATIC void	pit_microfind(void);
STATIC void	pit_setmode(void);
STATIC void	pit_init(void);


/*
 * STATIC void
 * pit_setmode(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Since we use only timer 0, we program that.
 *	8254 Manual specifically says you do not need to program
 *	timers you do not use.
 */
STATIC void
pit_setmode(void)
{
	unsigned char	byte;

	outb(pitctl_port, pit0_mode);
	byte = clknumb;
	outb(pitctr0_port, byte);
	byte = clknumb >> 8;
	outb(pitctr0_port, byte); 
}

/*
 * STATIC void
 * pit_init(void)
 *
 * Calling/Exit State:
 *	Interrupts must be disabled on entry.
 */
STATIC void
pit_init(void)
{
	unsigned int	flags;
	unsigned char	byte;

	pit_microfind();

	/*
	 * Set the PIT to squarewave mode and initialize 
	 * the timer 0 counter.
	 */
	pit_setmode();

	pit_initialized = B_TRUE;
}

/*
 * void
 * clkstart(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
clkstart(void)
{
	pit_init();
}

#define MICROCOUNT	0x2000
#define PIT_COUNTDOWN	(PIT_READMODE|PIT_NDIVMODE)

/*
 * STATIC void
 * pit_microfind(void)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
pit_microfind(void)
{
	unsigned int flags;
	unsigned char byte;
	unsigned short leftover;
	int efl;
#ifdef TIME_CHECK
	register int	tval;
#endif	/* TIME_CHECK */
	extern void _tenmicrosec(void);

	efl = intr_disable();			/* disable interrupts */

#ifdef TIME_CHECKD
	for (tval = 0x80; tval <= 0x80000; tval = tval * 2) {
		outb(pitctl_port, PIT_COUNTDOWN);
		outb(pitctr0_port, 0xff);
		outb(pitctr0_port, 0xff);
		microdata = tval;
		_tenmicrosec();
		byte = inb(pitctr0_port);
		leftover = inb(pitctr0_port);
		leftover = (leftover << 8) + byte;
		microdata = (long)((unsigned)(tval * CLKNUM * HZ) /
				((unsigned)(0xffff - leftover) * 100000));
		microdata = (long)((unsigned)(tval * CLKNUM) /
				((unsigned)(0xffff - leftover) * (100000/HZ)));
		printf("pit_microfind: tval=%x, mdat=%x, left=%x\n",
				tval, microdata, leftover);
	}
#endif	/* TIME_CHECKD */

	/* Put counter in count down mode */
	outb(pitctl_port, PIT_COUNTDOWN);
	/* output a count of -1 to counter 0 */
	outb(pitctr0_port, 0xff);
	outb(pitctr0_port, 0xff);
	microdata = MICROCOUNT;
	_tenmicrosec();
	/* Read the value left in the counter */
	byte = inb(pitctr0_port);	/* least siginifcant */
	leftover = inb(pitctr0_port);	/* most significant */
	leftover = (leftover << 8) + byte;
	/*
	 * Formula for microdata is:
	 *	(loopcount * timer clock speed) / (counter ticks * 1000)
	 *  Note also that 1000 is for figuring out milliseconds
	 */
	microdata = (long)((unsigned)(MICROCOUNT * CLKNUM) /
			((unsigned)(0xffff - leftover) * (100000 / HZ)));
	if (microdata == 0)
		microdata++;

#ifdef TIME_CHECK
	printf("pit_microfind: microdata for ten microsecond delay is %d\n", 
			microdata);
#endif	/* TIME_CHECK */

	intr_restore(efl);		/* restore interrupt state */
}

#ifdef PIT_DEBUG

uchar_t	pit_status[2];
uint_t	pit_rval[2];
int	pit_wrongmode;
int	pit_wrongcount;
int	pit_rightcount;
uint_t	pit_leftover1, pit_leftover2;
uint_t	pit_usecs;

#define PIT_DEBUG_0(leftover, status) { \
		pit_wrongcount++; \
		pit_rval[0] = (leftover); \
		pit_status[0] = (status); \
}

#define	PIT_DEBUG_1(leftover, status) { \
		pit_rightcount++; \
		pit_rval[1] = (leftover); \
		pit_status[1] = (status); \
}

#define	PIT_DEBUG_2() { \
		pit_wrongmode++; \
}

#define PIT_DEBUG_3(var, val) { \
		(var) = (val); \
}

#else

#define	PIT_DEBUG_0(leftover, status)
#define	PIT_DEBUG_1(leftover, status)
#define PIT_DEBUG_2()
#define PIT_DEBUG_3(var, val)

#endif /* PIT_DEBUG */

#define	PIT_LATCH	0xC2	/* latch counter and status */
#define	PIT_OUT_HIGH	0x80	/* OUT pin */


ulong_t	ulbolt = 0;		/* free-flowing lbolt counter */

#ifndef UNIPROC

/*
 * Hand-rolled lock to protect the state of the PIT timer chip.
 * This is necessary because pit_usec_time may be called from inside
 * the normal lock code.
 */
volatile uint_t _pit_lock;

/*
 * uint_t
 * pit_lock_xchg(void)
 *	Utility routine used by pit_lock() to locked exchange.
 */
uint_t
pit_lock_xchg(void)
{
	asm("	movl	$1, %eax");
	asm("	xchgl	%eax, _pit_lock");
}

/*
 * int
 * pit_lock(void)
 *	Lock the PIT lock.
 * Returns saved efl to pass to pit_unlock().
 */
int
pit_lock(void)
{
	int	efl;

	efl = intr_disable();
	while (pit_lock_xchg() != 0) {
		intr_restore(efl);
		while (_pit_lock != 0)
			;
		(void)intr_disable();
	}
	return efl;
}

/*
 * void
 * pit_unlock(int efl)
 *	Unlock the PIT lock.
 */
void
pit_unlock(int efl)
{
	_pit_lock = 0;
	intr_restore(efl);
}

#define PIT_LOCK(efl)	(efl) = pit_lock()
#define PIT_UNLOCK(efl)	pit_unlock(efl)

#else /* UNIPROC */

#define PIT_LOCK(efl)	/**/
#define PIT_UNLOCK(efl)	/**/

#endif /* UNIPROC */

/*
 * ulong_t
 * pit_usec_time(void)
 *	Read the counter 0 (microsec resolution) from the PIT and add it
 *	to the ulbolt.
 *
 * Calling/Exit State:
 *	Return the microsec clock value.
 *
 * Remarks:
 *	This is an extremely critical block of code that is executed
 *	every time a lock is acquired or released. However, the code is
 *	unoptimized and is extremely slow because all the unit coversion
 *	is done here. It is done here because all its caller expects 
 *	a microsec resolution clock value from this interface.
 *
 *	The advantage of doing unit conversion here is that it provides
 *	source commonality across platforms thereby simplifying source 
 *	maintainability. As a reference, an optimized (without unit
 *	conversion) version of the code is listed below.
 *
 *      Assembler macro for constructing the timestamp on i386.
 *      The equivalent C code is:
 *
 *	#define PIT_CLKNUM	11931
 *      #define pit_usec_time()\
 *              x = splhi();\
 *              outb(PITCTL_PORT, PIT_LATCH);\
 *              status = inb(PITCTR0_PORT);\
 *              timestamp = (lbolt << 16) | \
 *                              inb(PITCTR0_PORT) |\
 *                              (inb(PITCTR0_PORT << 8);\
 *              if (status & PIT_OUT_HIGH)\
 *                      timestamp += CLKNUM;\
 *              splx(x);
 *
 * asm ulong_t
 * pit_usec_time(void)
 * {
 * %	lab	noinc;
 *
 *	movw    $PIT_LATCH, %ax         / pit command reg == PIT_LATCH
 *	movw    $PITCTL_PORT, %dx       / set pit control port address
 *	pushl   %ebx                    / save current contents of %ebx
 *	pushf                           / save current state
 *	cli                             / disable interrupts
 *	outb    (%dx)                   / latch pit
 *	movw    $PITCTR0_PORT, %dx      / set pit counter 0 address
 *	subl    %eax, %eax              / zero the destination register
 * 	inb     (%dx)                   / read status byte
 *	movl    %eax, %ebx              / save status in register ebx
 *	inb     (%dx)                   / read lsb
 *	movl	lbolt, %ecx		/ copy lbolt
 *	shll    $0x10, %ecx		/ shift lbolt left 16
 *	orl	%eax, %ecx		/ or in the lsb
 *	inb	(%dx)			/ read msb
 *	popf				/ reenable the interrupts
 *	shll	$0x8, %eax		/ shift msb left 8
 *	orl	%ecx, %eax		/ or in the lsb and lbolt
 *	testl	$PIT_OUT_HIGH, %ebx	/ test if clock OUT is high
 *	je	noinc			/ if not, don't adjust
 *	addl	$PIT_CLKNUM, %eax	/ otherwise, do adjust
 * noinc: popl	%ebx			/ restore previous value of %ebx
 * }
 */
ulong_t
pit_usec_time(void)
{
	uint_t		usecs = 0;	/* microsecs */
	uchar_t		status;		/* status byte of counter 0 */
	uchar_t		byte;
	ushort_t	leftover;
	ulong_t		latched_ulbolt;	/* latched current ulbolt */
#ifndef UNIPROC
	int		efl;
#endif

	if (!pit_initialized)
		return ((uint_t) 0);

	/*
	 * latch/read status and count of counter 0
	 * (counter value read is in microsecs since
	 * the clock rate driving the pit is 1.2 MHz)
	 */

	PIT_LOCK(efl);

	/* read the status of counter 0 */
	outb(pitctl_port, PIT_LATCH);
	status = inb(pitctr0_port);

	/* read the value left in the counter */
	byte = inb(pitctr0_port);	/* least siginifcant */
	leftover = inb(pitctr0_port);	/* most significant */
	leftover = (leftover << 8) + byte;

	/*
	 * Latch the current ulbolt value to reduce the window
	 * of a clock interrupt between the time at which the pit
	 * counter value is read and the time at which the microsec
	 * return value is calculated which is done at the end of
	 * this function. This is necessary to provide accurate
	 * microsecond time otherwise the result could be skewed
	 * by 10 millisecs if the pit counter is close to zero
	 * in the second half of the countdown.
	 */
#ifdef _MPSTATS
	latched_ulbolt = ulbolt;
	PIT_UNLOCK(efl);
#else
	latched_ulbolt = lbolt;
	PIT_UNLOCK(efl);
	latched_ulbolt *= 10000;
#endif /* _MPSTATS */

	PIT_DEBUG_3(pit_leftover1, leftover);

	if (status & PIT_SQUAREMODE) {
		if (status & PIT_OUT_HIGH) {
			if (leftover > clknumb) {
				PIT_DEBUG_0(leftover, status);
				leftover = clknumb - 1;
				/* Error: reset the timer 0 counter value */
				PIT_LOCK(efl);
				pit_setmode();
				PIT_UNLOCK(efl);
			} else {
				leftover = ((clknumb - leftover) / 2);
				PIT_DEBUG_1(leftover, status);
			}
		} else {
			if (leftover > clknumb) {
				PIT_DEBUG_0(leftover, status);
				leftover = clknumb - 1;
				/* Error: reset the timer 0 counter value */
				PIT_LOCK(efl);
				pit_setmode();
				PIT_UNLOCK(efl);
			} else {
				/*
				 * An unreduced form of the leftover 
				 * calculation looks like the following:
				 *
				 * leftover = (clknumb / 2) + 
				 *		((clknumb - leftover) / 2); 
				 */
				leftover = clknumb - (leftover / 2);
				PIT_DEBUG_1(leftover, status);
			}
		}
	} else {
		PIT_DEBUG_2();
		/* Error: reset the pit to squarewave mode */
		PIT_LOCK(efl);
		pit_setmode();
		PIT_UNLOCK(efl);
		return ((uint_t) latched_ulbolt);
	}

	PIT_DEBUG_3(pit_leftover2, leftover);

	/*
	 * To return the pit value in millisecs, the formula is:
	 *	msecs = (ulbolt / 1000) + 
	 *		((CLKNUM - leftover) / (CLKNUM / 10));
	 *
	 * To return the pit value in microsecs, the formula is:
	 *	usecs = (ulbolt) +	
	 *		((CLKNUM - leftover) / (CLKNUM / 10000));
	 */
	usecs = latched_ulbolt + ((leftover * 10000) / clknumb);

	PIT_DEBUG_3(pit_usecs, usecs);

	return (usecs);
}

#ifdef PIT_DEBUG

uint_t	pit_count = 10000;

/*
 * void
 * pit_debug(void)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	This piece of code is to debug pit_usec_time(). Within
 *	kdb we can modify the pit_count value and since we know
 *	the time to execute the null for loop we can verify if
 *	the return value from pit_usec_time() is in fact what
 *	we expect.
 */
void
pit_debug(void)
{
	int	i;


	pit_rval[0] = pit_usec_time();


	for (i = 0; i < pit_count; i++)
		;

	pit_rval[1] = pit_usec_time();
}

#endif /* PIT_DEBUG */
