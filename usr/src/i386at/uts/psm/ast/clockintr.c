/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:psm/ast/clockintr.c	1.4"
#ident	"$Header: $"

/*
 * Machine-dependent clock interrupt handler.
 */

#include <acc/priv/privilege.h>
#include <io/prf/prf.h>
#include <svc/bootinfo.h>
#include <svc/clock.h>
#include <svc/pit.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/debug.h>
#include <util/emask.h>
#include <util/engine.h>
#include <util/inline.h>
#include <util/plocal.h>
#include <util/types.h>

extern int xclock_pending;

/*
 * STATIC void set_lclclock(void *arg)
 *	Increment local clock count to indicate pending local
 *	clock processing when returning from cross-processor
 *	interrupt handler.
 *
 * Calling/Exit State:
 *	Called by:	from clock() via cross-call (xcall())
 *	Parameters:	none
 *	Locking:	none
 *	Globals read:   none
 *	Globs. written: xclock_pending
 *	Returns:	none
 */
/* ARGSUSED */
STATIC void
set_lclclock(void *arg)
{
	xclock_pending++;
}


/*
 * void clock(uint vec, uint oldpl, uint edx, uint ecx,
 *		uint eax, uint es, uint ds, uint eip, uint cs)
 *	Call the global and local clock routines.
 *
 * Calling/Exit State:
 *	Called by:	from clock() via cross-call (xcall())
 *	Parameters:
 *		vec	-- vector number
 *		oldpl	-- prior ipl level
 *		edx,ecx,eax,es,ds,eip,cs -- register values
 *	Locking:	none
 *	Globals read:   myengnum
 *	Globs. written: none
 *	Returns:	none
 *
 * Remarks:
 *	Only engine zero receives clock interrupts in Systempro
 *	compatible and Manhattan symmetric mode. 
 */
/* ARGSUSED */
void
clock(uint vec, uint oldpl, uint edx, uint ecx,
		uint eax, uint es, uint ds, uint eip, uint cs)
{
	emask_t clock_slaves;
	extern int nonline;


	ASSERT(myengnum == BOOTENG);

	todclock(&eax);
	lclclock(&eax);

        if (nonline > 1)
		xcall_all(&clock_slaves, B_TRUE, set_lclclock, NULL);
}
