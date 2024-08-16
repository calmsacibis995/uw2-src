/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:psm/netframe/pit.c	1.1"
#ident	"$Header: $"


/*
 *         INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *     This software is supplied under the terms of a license 
 *    agreement or nondisclosure agreement with Intel Corpo-
 *    ration and may not be copied or disclosed except in
 *    accordance with the terms of that agreement.
 */


#include <sys/bootinfo.h>
#include <sys/nf_pit.h>
#include <sys/nf_apic.h>
#include <sys/dl.h>
#include <sys/inline.h>
#include <sys/param.h>
#include <sys/types.h>

int pitctl_port  = PITCTL_PORT;		/* For 386/20 Board */
int pitctr0_port = PITCTR0_PORT;	/* For 386/20 Board */
int pitctr1_port = PITCTR1_PORT;	/* For 386/20 Board */
int pitctr2_port = PITCTR2_PORT;	/* For 386/20 Board */
/* We want PIT 0 in square wave mode */
int pit0_mode = PIT_C0|PIT_SQUAREMODE|PIT_READMODE;
#define COUNT	0x2000
#define PIT_COUNTDOWN	PIT_READMODE|PIT_NDIVMODE

unsigned int delaycount;		/* loop count in trying to delay for
					 * 1 millisecond
					 */
long microdata = 50;			/* loop count for 10 microsecond wait.
					 * MUST be initialized for those who
					 * insist on calling "_tenmicrosec"
					 * it before the clock has been
					 * initialized.
					 */
unsigned int clknumb = CLKNUM;		/* interrupt interval for timer 0 */

int pit_initialized = 0;

extern int is_gemstone;
extern volatile vaddr_t	local_apic_addr;	/* virtual address for mapping
						   the local APIC */

static void _spinwait(int);

/*
 * void
 * pit_setmode(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
pit_setmode(void)
{
	unsigned char	byte;


	/*
	 * Since we use only timer 0, we program that.
	 * 8254 Manual specifically says you do not need to program
	 * timers you do not use
	 */
	outb(pitctl_port, pit0_mode);
	byte = clknumb;
	outb(pitctr0_port, byte);
	byte = clknumb >> 8;
	outb(pitctr0_port, byte); 
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
	unsigned int	flags;
	unsigned char	byte;
	volatile long *p;
        long v;

	findspeed();
	microfind();

	if (is_gemstone) {
		p = (volatile long *)local_apic_addr;
		clknumb = AP_CLKNUM;
		p[AP_ICOUNT] = clknumb; /* initial count */
		p[AP_LVT_TIMER] = p[AP_LVT_TIMER] & 0xff | 0x20000;
	} else { 
		pit_setmode();
	}

	pit_initialized++;
}


/*
 * findspeed(void)
 *
 * Calling/Exit State:
 *	None.
 */
findspeed()
{
	unsigned int flags;
	unsigned char byte;
	unsigned int leftover;
	int i, j;
	int efl;
	volatile long *p;
        long v, ov;

	efl = intr_disable();                 /* disable interrupts */
	delaycount = COUNT;
	if (is_gemstone) {
		p = (volatile long *)local_apic_addr;
		v = ov = p[AP_LVT_TIMER];
		v &= 0x1ffff;
		v |= 0x60000;
		p[AP_LVT_TIMER] = v;
		p[AP_ICOUNT] = 0xffff;
		_spinwait(1);
		/* Read the value left in the counter */
		leftover = p[AP_CCOUNT];
		p[AP_LVT_TIMER] = ov;
		/* Formula for delaycount is :
		 *  (loopcount * timer clock speed)/ (counter ticks * 1000)
		 * 1000 is for figuring out milliseconds
		 */
		delaycount = (((COUNT * AP_CLKNUM)/1000)*HZ)/(0xffff-leftover);
	} else {
		outb(pitctl_port, PIT_COUNTDOWN);
		/* output a count of -1 to counter 0 */
		outb(pitctr0_port, 0xff);
		outb(pitctr0_port, 0xff);
		_spinwait(1);
		/* Read the value left in the counter */
		byte = inb(pitctr0_port);	/* least siginifcant */
		leftover = inb(pitctr0_port);	/* most significant */
		leftover = (leftover << 8) + byte ;
		/*
		 * Formula for delaycount is:
		 *	(loopcount * timer clock speed) / (counter ticks * 1000)
		 * 1000 is for figuring out milliseconds 
		 */
		delaycount = (((COUNT * CLKNUM)/1000)*HZ) / (0xffff-leftover);
	}
	intr_restore(efl);         /* restore interrupt state */
}


/*
 * void
 * _spinwait(int)
 *
 * Calling/Exit State:
 *	millis is number of milliseconds to delay.
 */
static void
_spinwait(int millis)
{
	int i, j;

	for (i = 0; i < millis; i++)
		for (j = 0; j < delaycount; j++)
			;
}

#define MICROCOUNT	0x2000

/*
 * microfind(void)
 *
 * Calling/Exit State:
 *	None.
 */
microfind()
{
	unsigned int flags;
	unsigned char byte;
	unsigned short leftover;
	int efl;
	volatile long *p;
        long v, ov;

	efl = intr_disable();			/* disable interrupts */
	microdata = MICROCOUNT;
	if (is_gemstone) {
		p = (volatile long *)local_apic_addr;
		v = ov = p[AP_LVT_TIMER];
		v &= 0x1ffff;
		v |= 0x60000;
		p[AP_LVT_TIMER] = v;
		p[AP_ICOUNT] = 0xffff;
		_tenmicrosec();
		/* Read the value left in the counter */
		leftover = p[AP_CCOUNT];
		/* Formula for delaycount is :
		 *  (loopcount * timer clock speed)/ (counter ticks * 1000)
		 *  Note also that 1000 is for figuring out milliseconds
		 */
		microdata = (unsigned)(MICROCOUNT * AP_CLKNUM) /
                        ((unsigned)(0xffff-leftover)*(100000/HZ));
	} else {
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
		 * Formula for delaycount is:
		 *	(loopcount * timer clock speed) / (counter ticks * 1000)
		 *  Note also that 1000 is for figuring out milliseconds
		 */
		microdata = (long)((unsigned)(MICROCOUNT * CLKNUM) /
			((unsigned)(0xffff - leftover) * (100000 / HZ)));
	}
	if (!microdata)
		microdata++;
	intr_restore(efl);		/* restore interrupt state */
}

#define	PIT_LATCH	0xC2	/* latch counter and status */
#define	PIT_OUT_HIGH	0x80	/* OUT pin */

ulong_t	ulbolt = 0;		/* free-flowing lbolt counter */

/*
 * ulong_t
 * pit_usec_time(void)
 *	Read the counter 0 (microsec resolution) from the pit and add it
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
 */
ulong_t
pit_usec_time(void)
{
	uint_t		usecs = 0;	/* microsecs */
	uchar_t		status;		/* status byte of counter 0 */
	uchar_t		byte;
	ulong_t		leftover;
	ulong_t		latched_ulbolt;	/* latched current ulbolt */
	volatile 	long *p;


	if (pit_initialized) {
		if (is_gemstone) {
			p = (volatile long *)APICADR;
			leftover = p[AP_CCOUNT];
			latched_ulbolt = ulbolt;
		} else {
			/* read the status of counter 0 */
			outb(pitctl_port, PIT_LATCH);
			status = inb(pitctr0_port);

			/* read the value left in the counter */
			byte = inb(pitctr0_port);	/* least siginifcant */
			leftover = inb(pitctr0_port);	/* most significant */
			leftover = (leftover << 8) + byte;

			/*
			 * Latch the current ulbolt value to reduce the window
			 * of a clock interrupt between the time at which the 
			 * pit counter value is read and the time at which the 
			 * usec return value is calculated which is done at thef
			 * end of this function. This is necessary to provide
			 * accurate usec time otherwise the result could be 
			 * skewed by 10 millisecs if the pit counter is close 
			 * to zero in the second half of the countdown.
			 */
			latched_ulbolt = ulbolt;

			if (status & PIT_SQUAREMODE) {
				if (status & PIT_OUT_HIGH) {
					if (leftover > clknumb) {
						leftover = clknumb - 1;
						pit_setmode(); 
					} else {
						leftover=((clknumb-leftover)/2);
					}
				} else {
					if (leftover > clknumb) {
						leftover = clknumb - 1;
						pit_setmode();
					} else {
						leftover = clknumb-(leftover/2);
					}
				}
			} else {
				pit_setmode(); 
				leftover = clknumb - 1;
			}

		}
		usecs = latched_ulbolt + ((leftover * 10000) / clknumb);
	}

	return (usecs);
}
