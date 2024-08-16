/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:util/kdb/kdb_util/kdb_p.c	1.11"
#ident	"$Header: $"

/*
 * Platform-specific routines for KDB.
 */

#include <io/conf.h>
#include <io/conssw.h>
#include <io/intctl.h>
#include <io/slic.h>
#include <io/ssm/ssm.h>
#include <io/ssm/ssm_misc.h>
#include <svc/clock_p.h>
#include <util/kdb/kdb/debugger.h>
#include <util/param.h>

/*
 * void kdb_pdep_enter(void)
 *	Hook called on interactive entry into the kernel debugger,
 *	which will hold all processors and interrupts disabled for an
 *	arbitarily long time.  This can be used to disable watchdog timers
 *	and the like.
 *
 * Calling/Exit State:
 *	We are single-threaded at this point.  The caller has already
 *	suspended the other processors.
 */
void
kdb_pdep_enter(void)
{
	if (SSM_desc != NULL) {
		/*
		 * Turn off TOD interrupts
		 */
		ssm_tod_freq(SM_NOLOCK, 0, SL_GROUP | TMPOS_GROUP,
			TODCLKVEC, SL_MINTR | TODCLKBIN);
		/*
		 * Suspend the system's watchdog timer.
		 */
		FW_WDTDISABLE();
	}
}

/*
 * void kdb_pdep_exit(void)
 *	Hook called on exit from the kernel debugger interactive mode.
 *
 * Calling/Exit State:
 *	We are still single-threaded at this point.
 */

void
kdb_pdep_exit(void)
{
	if (SSM_desc != NULL) {
		/*
		 * Turn on TOD interrupts
		 */
		ssm_tod_freq(SM_NOLOCK, TODFREQ, SL_GROUP | TMPOS_GROUP,
			TODCLKVEC, SL_MINTR | TODCLKBIN);
		/*
		 * Resume the system's watchdog timer.
		 */
		ssm_poke_wdt();
		FW_WDTREENABLE();
	}
}
