/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/olivetti/apit.c	1.2"
/*	Copyright (c) 1993 UNIX System Laboratories, Inc. 	*/
/*	  All Rights Reserved                             	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.   	            	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*								*/
/*	Copyright Ing. C. Olivetti & C. S.p.A.			*/

/*
 *         INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *     This software is supplied under the terms of a license 
 *    agreement or nondisclosure agreement with Intel Corpo-
 *    ration and may not be copied or disclosed except in
 *    accordance with the terms of that agreement.
 */

#ifdef _KERNEL_HEADERS
#include <svc/bootinfo.h>
#include <psm/olivetti/oliapic.h>
#include <svc/pit.h>
#include <util/dl.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/types.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

/* This is used when using PSK environment */

#include <sys/bootinfo.h>
#include "oliapic.h"
#include <sys/pit.h>
#include <sys/dl.h>
#include <sys/inline.h>
#include <sys/param.h>
#include <sys/types.h>
#endif /* _KERNEL_HEADERS */

extern int clock_intr_vector;

unsigned int delaycount;		/*
					 * loop count in trying to delay for
					 * 1 millisecond
					 */
extern	unsigned int clknumb;
extern	long microdata;

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
        long v;

	findspeed();
	microfind();

	/*
	 * set the pit to squarewave mode and initialize 
	 * the timer 0 counter.
	 */
       	clknumb = AP_CLKNUM;
       	AP_ICOUNT = clknumb; /* initial count */
       	v = clock_intr_vector;
       	v &= 0xffff;    /* vector from picinit */
       	v |= 0x60000;   /* Periodic, TMBASE, not masked */
       	AP_LVT_TIMER = v;

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

#define COUNT	0x2000

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
#ifdef TIME_CHECK
	register int	tval;
#endif	/* TIME_CHECK */
        long v, ov;

	efl = intr_disable();                 /* disable interrupts */

	/*
	 * Put counter in count down mode 
	 */

#define PIT_COUNTDOWN	PIT_READMODE|PIT_NDIVMODE

        v = ov = AP_LVT_TIMER;
        v &= 0x1ffff;
        v |= 0x60000;
        AP_LVT_TIMER = v;
        AP_ICOUNT = 0xffff;
        delaycount = COUNT;
        spinwait(1);
        /* Read the value left in the counter */
        leftover = AP_CCOUNT;
        /* Formula for delaycount is :
         *  (loopcount * timer clock speed)/ (counter ticks * 1000)
         * 1000 is for figuring out milliseconds
         */
        delaycount = (((COUNT * AP_CLKNUM)/1000) * HZ) / (0xffff-leftover);
        AP_LVT_TIMER = ov;

#ifdef TIME_CHECK
	cmn_err(CE_CONT, "findspeed: delaycount for one millisecond delay is %d\n", 
			delaycount);
#endif	/* TIME_CHECK */

	intr_restore(efl);         /* restore interrupt state */
}


/*
 * void
 * spinwait(int)
 *
 * Calling/Exit State:
 *	millis is number of milliseconds to delay.
 */
spinwait(int millis)
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
#ifdef TIME_CHECK
	register int	tval;
#endif	/* TIME_CHECK */
        long v, ov;

	efl = intr_disable();                 /* disable interrupts */

        v = ov = AP_LVT_TIMER;
        v &= 0x1ffff;
        v |= 0x60000;
        AP_LVT_TIMER = v;
        AP_ICOUNT = 0xffff;
        microdata=MICROCOUNT;
        _tenmicrosec();
        /* Read the value left in the counter */
        leftover = AP_CCOUNT;
        /* Formula for delaycount is :
         *  (loopcount * timer clock speed)/ (counter ticks * 1000)
         *  Note also that 1000 is for figuring out milliseconds
         */
        microdata = (unsigned)(MICROCOUNT * AP_CLKNUM) /
                        ((unsigned)(0xffff-leftover)*(100000/HZ));
        if (!microdata)
                microdata++;
        AP_LVT_TIMER = ov;

#ifdef TIME_CHECK
	cmn_err(CE_CONT, "microfind: delaycount for ten microsecond delay is %d\n", 
			microdata);
#endif	/* TIME_CHECK */

	intr_restore(efl);         /* restore interrupt state */
}
