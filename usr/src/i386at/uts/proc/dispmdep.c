/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:proc/dispmdep.c	1.6"
#ident	"$Header: $"

#include <svc/psm.h>
#include <util/engine.h>
#include <util/ipl.h>
#include <proc/disp.h>
#include <mem/vmparam.h>
#include <mem/vm_mdep.h>

#ifndef UNIPROC
int nidle;
#endif

/*
 * void dispmdepinit(int nglobpris)
 *
 *	Initialize any machine specific functions.
 *
 * Calling/Exit State:
 *
 *	Called from dispinit, early on in startup, hence the processor
 *	is single threaded at this point.
 *
 * Description:
 *
 *	This function initializes some SLIC priorities and therefore
 *	is a no op for the AT architecture.
 */

/* ARGSUSED */
void
dispmdepinit(int nglobpris)
{
	return;
}

/*
 * void dispmdepnewpri(int pri)
 *
 *	Make machine specific adjustments resulting from a new
 *	priority.
 *
 * Calling/Exit State:
 *
 *	Called when the currently running lwp changes its priority.
 *	Currently only called from ts_trapret.
 * Description:
 *
 *	For the AT architecture, this is a no op.
 */

/* ARGSUSED */
void
dispmdepnewpri(int pri)
{
	return;
}

/*
 * void idle(void)
 *
 *	Idle the engine until work arrives.
 *
 * Calling/Exit State:
 *
 *	Called at splhi, drops the priority to spl0, returns again
 *	at splhi.
 *
 * Description:
 *
 *	Set our IPL to zero and wait until we are nudged or we receive
 *	a shutdown request.
 */
void
idle(void)
{
	register engine_t *eng = l.eng;


	(void)spl0();
	/*
	 * Switch off the leds to indicate that we are idling.
	 */
	psm_ledctl(LED_OFF, ACTIVE_LED);

#ifndef UNIPROC
	RUNQUE_LOCK();
	nidle++;
	RUNQUE_UNLOCK();
#endif

	/*
	 * Wait for a nudge.
	 */
	while (eng->e_npri == -1 && (eng->e_flags & E_SHUTDOWN) == 0)
		continue;
	(void)splhi();

#ifndef UNIPROC
	RUNQUE_LOCK();
	nidle--;
	RUNQUE_UNLOCK();
#endif

	/*
	 * We are going off to seek work. Switch the leds back on.
	 */
	psm_ledctl(LED_ON, ACTIVE_LED);
}
