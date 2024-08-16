/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:psm/netframe/clockintr.c	1.1"
#ident	"$Header: $"

/*
 * Machine-dependent clock interrupt handler.
 */

#include <sys/privilege.h>
#include <sys/bootinfo.h>
#include <sys/clock.h>
#include <sys/nf_pit.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/debug.h>
#include <sys/inline.h>
#include <sys/plocal.h>
#include <sys/types.h>


/*
 * void clock(uint vec, uint oldpl, uint edx, uint ecx,
 *		uint eax, uint es, uint ds, uint eip, uint cs)
 *
 *	Call the global and local clock routines
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
void
clock(uint vec, uint oldpl, uint edx, uint ecx,
		uint eax, uint es, uint ds, uint eip, uint cs)
{
	if (myengnum == 0)
		todclock(&eax);
	lclclock(&eax);
}
