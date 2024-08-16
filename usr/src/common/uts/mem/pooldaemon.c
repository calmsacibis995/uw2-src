/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/pooldaemon.c	1.27"
#ident	"$Header: $"

#include <fs/buf.h>
#include <mem/kma.h>
#include <mem/mem_hier.h>
#include <mem/seg_map.h>
#include <proc/exec.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>

event_t poolrefresh_event;
clock_t poolrefresh_lasttime;
boolean_t poolrefresh_pending;

/*
 * void poolrefresh(void *pool_argp)
 *	This daemon is woken up every second, to call refreshpool routines
 *	for users of private memory pools which may need to grow or shrink.
 *
 * Calling/Exit State:
 *	Never exits.
 *
 * Description:
 *	The operation of this daemon is required to allow LWPs which
 *	are blocked waiting for memory (or for swap or real memory
 *	reservations) to proceed.  Therefore, in order to prevnet
 *	deadlocks, nothing in this daemon may wait for memory, or for
 *	memory reservations.
 */
/* ARGSUSED */
void
poolrefresh(void *pool_argp)
{
	extern void mem_resv_service(void);
	extern void kma_refreshpools(void);
	extern void hat_refreshpools(void);
	extern void cleanup(void);
	extern void page_swapreclaim(boolean_t);
	extern void segkvn_age(void);
	extern void streams_check_bufcall(void);

	u.u_lwpp->l_name = "poolrefresh";

	/* EVENT_INIT(&poolrefresh_event) is done in clock_init(). */

	for (;;) {
		EVENT_WAIT(&poolrefresh_event, PRIMEM);

		if (bclnlist != NULL)
			cleanup();
		mem_resv_service();
		kma_refreshpools();
		hat_refreshpools();
		segkvn_age();
		page_swapreclaim(B_FALSE);
		streams_check_bufcall();
		segmap_age(segkmap);
		poolrefresh_lasttime = lbolt;
		poolrefresh_pending = B_FALSE;
	}
}

/*
 * void poolrefresh_nudge(void)
 *
 * Calling/Exit State:
 *	Called when resources (e.g. pages) have become available.
 *	Kicks off the pool-refresh daemon if anyone is waiting on the pools.
 */
void
poolrefresh_nudge(void)
{
	if (poolrefresh_pending)
		return;
	if (kma_waiters()) {
		poolrefresh_pending = B_TRUE;
		EVENT_SIGNAL(&poolrefresh_event, 0);
	}
}

/*
 * void poolrefresh_outofmem(void)
 *
 * Calling/Exit State:
 *	Called when resources (e.g. pages) have been exhausted.
 *	Kicks off the pool-refresh daemon in forced-shrink mode,
 *	so resources will be given back ASAP.
 */
void
poolrefresh_outofmem(void)
{
	if (kma_force_shrink()) {
		poolrefresh_pending = B_TRUE;
		EVENT_SIGNAL(&poolrefresh_event, 0);
	}
}
