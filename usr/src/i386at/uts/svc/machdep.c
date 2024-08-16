/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:svc/machdep.c	1.10"
#ident	"$Header: $"

/*
 * Highly Machine dependent routines.
 */

#include <fs/buf.h>
#include <proc/disp.h>
#include <svc/bootinfo.h>
#include <util/cmn_err.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/plocal.h>
#include <util/types.h>

/*
 * int
 * calc_delay(int)
 *	Returns a number that can be used in delay loops to wait
 *	for a small amount of time, such as 0.5 second.
 *
 * Calling/Exit State:
 *	Takes a single integer argument.
 *
 *	Returns: an integer whose value depends on the cpu speed.
 *
 * Description:
 *	Return a number to use in spin loops that takes into account
 *	both the cpu rate and the mip rating.
 */

int
calc_delay(int x)
{
	return (x * l.cpu_speed);
}

/*
 * int buscheck(buf *bp)
 *      This platform does nothing with this. Other platforms
 *      may do something more useful.
 *
 * Calling/Exit State:
 *      Just return 0.
 */
/* ARGSUSED */
int
buscheck(struct buf *bp)
{
	return 0;
}

/*
 * void
 * softint_hdlr(void)
 *	Handle software interrupts (sent by sendsoft).
 *
 * Calling/Exit State:
 *	Called at PL1 as if from an interrupt.
 */
void
softint_hdlr(void)
{
	extern void runqueues(void);
	extern void globalsoftint(void);
	extern void localsoftint(void);

	do {
		if (l.eventflags & EVT_STRSCHED) {
			atomic_and(&l.eventflags, ~EVT_STRSCHED);
			runqueues();
		} else if (l.eventflags & EVT_GLOBCALLOUT) {
			atomic_and(&l.eventflags, ~EVT_GLOBCALLOUT);
			globalsoftint();
		} else if (l.eventflags & EVT_LCLCALLOUT) {
			atomic_and(&l.eventflags, ~EVT_LCLCALLOUT);
			localsoftint();
		}
	} while (l.eventflags & EVT_SOFTINTMASK);
}

/*
 * boolean_t
 * mainstore_memory(paddr_t phys)
 *	Determines if the physical address phys is part of mainstore memory
 *	or device memory?
 *
 * Calling/Exit State:
 *	Returns True if the address is in mainstore memory; False otherwise.
 */
boolean_t
mainstore_memory(paddr_t phys)
{
	int i;

	for (i = 0; i < bootinfo.memavailcnt; i++) {
		if (phys >= bootinfo.memavail[i].base &&
		    phys < bootinfo.memavail[i].base + 
			    bootinfo.memavail[i].extent)
			return B_TRUE;
	}
	return B_FALSE;
}
