/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/pcmp/clockintr.c	1.2"
#ident	"$Header: $"

/*
 * Machine-dependent clock interrupt handler.
 */

#include <acc/priv/privilege.h>
#include <svc/bootinfo.h>
#include <svc/clock.h>
#include <svc/pit.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/plocal.h>
#include <util/types.h>

extern unsigned	prfstat;

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
		uint eax, uint es, uint ds, uint eip, uint cs, uint efl)
{
	if (prfstat)
		prfintr(eip, USERMODE(cs, efl));

	if (myengnum == 0)
		todclock(&eax);
	lclclock(&eax);
}
