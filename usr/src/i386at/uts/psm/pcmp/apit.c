/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/pcmp/apit.c	1.2"
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
#include <svc/pit.h>
#include <util/dl.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/types.h>

#include <apic.h>

/*
 * TODO:
 *	1. Optimize pit_usec_time()
 *	2. Add another generic interface that would return the timer 0
 *	   free-flowing counter value. The massaging of the counter value
 *	   in this case would be done by the caller and not by this function.
 */

#ifdef NOTYET
/* for hrtimer in clkstart() */
extern uint	timer_resolution;
extern dl_t	tick;
extern uint	ticks_til_clock;
extern uint	unix_tick;
#endif /* NOTYET */

/* #define TIME_CHECK		/* Debugging */
/* #define TIME_CHECKD		/* detailed Debugging */

extern int clock_intr_vector;

extern long microdata;			/* loop count for _tenmicrosec() wait */
uint_t apic_clknumb = 0;		/* interrupt interval for timer 0 */

extern vaddr_t local_apic_addr;

STATIC void apic_microfind(void);
STATIC void findapicspeed(void);
STATIC uint_t subapicspeed(void);


/*
 * void
 * apic_clkstart(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
apic_clkstart(void)
{
	unsigned int	flags;
	unsigned char	byte;
	volatile long *p;
        long v;

	if (apic_clknumb == 0)
		findapicspeed();
	apic_microfind();

	/*
	 * set the pit to squarewave mode and initialize 
	 * the timer 0 counter.
	 */
        p = (volatile long *)local_apic_addr;
       	p[AP_ICOUNT] = apic_clknumb; /* initial count */
       	v = clock_intr_vector;
       	v &= 0xffff;    /* vector from picinit */
       	v |= 0x20000;   /* Periodic, CLKIN, not masked */
       	p[AP_LVT_TIMER] = v;

#ifdef NOTYET
	/* initialize hrtimer variables */

	ticks_til_clock = timer_resolution / HZ;
	unix_tick = ticks_til_clock;
 
 	/*
  	 * The variable "tick" is used in hrt_alarm for
  	 * HRT_RALARM case.
         */
	tick.dl_lop = SCALE / timer_resolution;
#endif /* NOTYET */
}

#define MICROCOUNT	0x2000

/*
 * STATIC void
 * apic_microfind(void)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
apic_microfind(void)
{
	unsigned int flags;
	unsigned char byte;
	unsigned short leftover;
	int efl;
#ifdef TIME_CHECK
	register int	tval;
#endif	/* TIME_CHECK */
	volatile long *p;
        long v, ov;

	efl = intr_disable();                 /* disable interrupts */

        p = (volatile long *)local_apic_addr;
        v = ov = p[AP_LVT_TIMER];
        v &= 0x1ffff;
        v |= 0x20000;
        p[AP_LVT_TIMER] = v;
        p[AP_ICOUNT] = 0xffff;
        microdata = MICROCOUNT;
        _tenmicrosec();
        /* Read the value left in the counter */
        leftover = p[AP_CCOUNT];
        /* Formula for microdata is :
         *  (loopcount * timer clock speed)/ (counter ticks * 1000)
         *  Note also that 1000 is for figuring out milliseconds
         */
        microdata = (unsigned)(MICROCOUNT * apic_clknumb) /
                        ((unsigned)(0xffff-leftover)*(100000/HZ));
        if (microdata == 0)
                microdata++;
        p[AP_LVT_TIMER] = ov;

#ifdef TIME_CHECK
	cmn_err(CE_CONT, "apic_microfind: microdata for ten microsecond delay is %d\n", 
			microdata);
#endif	/* TIME_CHECK */

	intr_restore(efl);         /* restore interrupt state */
}

#include <io/rtc/rtc.h>

STATIC void
findapicspeed(void)
{
	int i;

	outb(RTC_ADDR, RTC_D);
	if ((inb(RTC_DATA) & RTC_VRT) == 0)
		rtcinit();
	apic_clknumb = 0;
	for (i=0; i<3; i++)
		apic_clknumb += subapicspeed();
	apic_clknumb /= 300;
}

STATIC uint_t
subapicspeed(void)
{
	volatile long *p;
	long v, ov, lleftover;
	unsigned char byte, obyte;
	unsigned int leftover;

	p = (volatile long *)local_apic_addr;
	v = ov = p[AP_LVT_TIMER];
	v &= 0xffff;	/* vector from picinit */
#ifdef USE_TMBASE
	v |= 0x50000;	/* One Shot, TMBASE, masked */
#else
	v |= 0x10000;	/* One Shot, CLKIN, masked */
#endif

wait0:
	outb(RTC_ADDR, 0);
	obyte = inb(RTC_DATA);
	if (obyte == 0xff)
		goto wait0;
wait1:
	outb(RTC_ADDR,RTC_A);
	byte = inb(RTC_DATA);
	if (byte & RTC_UIP)
		goto wait1;
	outb(RTC_ADDR,0);
	byte = inb(RTC_DATA);
	if (byte == 0xff || byte == obyte)
		goto wait1;

	p[AP_LVT_TIMER] = v;
	p[AP_ICOUNT] = 0x10000000;	/* initial count */

	obyte = byte;
waiting:
	outb(RTC_ADDR,RTC_A);
	byte = inb(RTC_DATA);
	if (byte & RTC_UIP)
		goto waiting;
	outb(RTC_ADDR,0);
	byte = inb(RTC_DATA);
	if (byte == 0xff || byte == obyte)
		goto waiting;

	lleftover = p[AP_CCOUNT];

	p[AP_LVT_TIMER] = ov;
	return (0x10000000 - lleftover);
}
