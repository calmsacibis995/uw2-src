/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:proc/dispmdep.c	1.3"
#include <util/engine.h>
#include <util/ipl.h>
#include <io/clkarb.h>
#include <io/slic.h>
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
 *	The only thing we need to do is to setup the routines which map
 *	the current global priority to the SLIC global priority.
 */
void
dispmdepinit(int nglobpris)
{
	/*
	 * Initialize the slic's priority mapper.
	 */
	slic_priinit(nglobpris);
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
 */
void
dispmdepnewpri(int pri)
{
	SLICPRI(pri);
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
 *	If the processor light show is configured, we flash our light
 *	off to indicate we're idling and wait until we're nudged or
 *	we receive a shutdown request.  After this occurs, we turn
 *	the light back on to indicate we're off scheduling some lwp.
 */
void
idle(void)
{
	register engine_t *eng = l.eng;

	if (light_show) {
		/*
		 * Turn the light off to indicate we're idling.
		 */
		if (fp_lights)
			FP_LIGHTOFF(l.eng_num);
		*(int *)KVLED = 0;
	}

	/*
	 * Knock our priority down so we're likely to get interrupts.
	 */
	SLICPRI(0);

#ifndef UNIPROC
	RUNQUE_LOCK();
	nidle++;
	RUNQUE_UNLOCK();
#endif

	/*
	 * Wait for a nudge.  If the nudger see we're idle, it will
	 * simply change our e_npri and not bother us with a per-processor
	 * interrupt.  This is good, as the alternative would divert us
	 * to process the interrupt thereby increasing the latency encountered
	 * before the reschedule.
	 */
	(void)spl0();
	while (eng->e_npri == -1 && (eng->e_flags & E_SHUTDOWN) == 0)
		continue;
	(void)splhi();

#ifndef UNIPROC
	RUNQUE_LOCK();
	nidle--;
	RUNQUE_UNLOCK();
#endif

	if (light_show) {
		/*
		 * We're going off to seek work.  Set the light back on.
		 */
		if (fp_lights)
			FP_LIGHTON(l.eng_num);
		*(int *)KVLED = 1;
	}
}
