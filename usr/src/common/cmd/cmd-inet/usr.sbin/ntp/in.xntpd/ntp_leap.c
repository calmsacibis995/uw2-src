/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/in.xntpd/ntp_leap.c	1.2"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */

/*
 * ntp_leap - maintain leap bits and take action when a leap occurs
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "ntp_syslog.h"
#include "ntp_fp.h"
#include "ntp.h"

/*
 * This module is devoted to maintaining the leap bits and taking
 * action when a leap second occurs.  It probably has bugs since
 * a leap second has never occurred to excercise the code.
 *
 * The code does two things when a leap second occurs.  It first
 * steps the clock one second in the appropriate direction.  It
 * then informs the reference clock code, if compiled in, that the
 * leap second has occured so that any clocks which need to disable
 * themselves can do so.  This is done within the first few seconds
 * after midnight, UTC.
 *
 * The code maintains two variables which may be written externally,
 * leap_warning and leap_indicator.  Leap_warning can be written
 * any time in the month preceeding a leap second.  24 hours before
 * the leap is to occur, leap_warning's contents are copied to
 * leap_indicator.  The latter is used by reference clocks to set
 * their leap bits.
 *
 * The module normally maintains a timer which is arranged to expire
 * just after 0000Z one day before the leap.  On the day a leap might
 * occur the interrupt is aimed at 2200Z and every 5 minutes thereafter
 * until 1200Z to see if the leap bits appear.
 */

/*
 * The leap indicator and leap warning flags.  Set by control messages
 */
u_char leap_indicator;
u_char leap_warning;

/*
 * Timer.  The timer code imports this so it can call us prior to
 * calling out any pending transmits.
 */
u_long leap_timer;

/*
 * Internal leap bits.  If we see leap bits set during the last
 * hour we set these.
 */
u_char leapbits;
u_char leapseenstratum1;

/*
 * Constants.
 */
#define	OKAYTOSETWARNING	(31*24*60*60)
#define	DAYBEFORE		(24*60*60)
#define	TWOHOURSBEFORE		(2*60*60)
#define	FIVEMINUTES		(5*60)
#define	ONEMINUTE		(60)

/*
 * Imported from the timer module.
 */
u_long current_time;


/*
 * Some statistics counters
 */
u_long leap_processcalls;	/* calls to leap_process */
u_long leap_notclose;		/* leap found to be a long time from now */
u_long leap_monthofleap;	/* in the month of a leap */
u_long leap_dayofleap;		/* This is the day of the leap */
u_long leap_hoursfromleap;	/* only 2 hours from leap */
u_long leap_happened;		/* leap process saw the leap */

/*
 * Imported from the main module
 */
extern int debug;

/*
 * init_leap - initialize the leap module's data.
 */
void
init_leap()
{
	/*
	 * Zero the indicators.  Schedule an event for just after
	 * initialization so we can sort things out.
	 */
	leap_indicator = leap_warning = 0;
	leap_timer = 1<<EVENT_TIMEOUT;
	leapbits = 0;
	leapseenstratum1 = 0;

	leap_processcalls = leap_notclose = 0;
	leap_monthofleap = leap_dayofleap = 0;
	leap_hoursfromleap = leap_happened = 0;
}


/*
 * leap_process - process a leap event expiry and/or a system time step
 */
void
leap_process()
{
	u_long leapnext;
	u_long leaplast;
	l_fp ts;
	u_char bits;
	extern struct peer *sys_peer;
	extern u_char sys_leap;
	static void setnexttimeout();
	extern void get_systime();
	extern void calleapwhen();
	extern void step_systime();
#ifdef REFCLOCK
	extern void refclock_leap();
#endif

	leap_processcalls++;
	get_systime(&ts);
	calleapwhen(ts.l_ui, &leaplast, &leapnext);

	/*
	 * Figure out what to do based on how long to the next leap.
	 */
	if (leapnext > OKAYTOSETWARNING) {
		if (leaplast < ONEMINUTE) {
			/*
			 * The golden moment!  See if there's anything
			 * to do.
			 */
			leap_happened++;
			bits = 0;
			if (leap_indicator != 0)
				bits = leap_indicator;
			else if (leapbits != 0)
				bits = leapbits;
			
			if (bits != 0) {
				l_fp tmp;

				/*
				 * Step the clock 1 second in the proper
				 * direction.
				 */
				if (bits == LEAP_DELSECOND)
					tmp.l_i = 1;
				else
					tmp.l_i = -1;
				tmp.l_uf = 0;

				step_systime(&tmp);
#ifdef REFCLOCK
				/*
				 * Tell the clocks about it
				 */
				refclock_leap();
#endif
				syslog(LOG_INFO,
			"leap second occured, stepped time %s 1 second",
				    tmp.l_i > 0 ? "forward" : "back");
			}
		} else {
			leap_notclose++;
		}
		leap_warning = 0;
	} else {
		if (leapnext > DAYBEFORE)
			leap_monthofleap++;
		else if (leapnext > TWOHOURSBEFORE)
			leap_dayofleap++;
		else
			leap_hoursfromleap++;
	}

	if (leapnext > DAYBEFORE) {
		leap_indicator = 0;
		leapbits = 0;
		leapseenstratum1 = 0;
		/*
		 * Berkeley's setitimer call does result in alarm
		 * signal drift despite rumours to the contrary.
		 * Schedule an event no more than 24 hours into
		 * the future to allow the event time to be
		 * recomputed.
		 */
		if ((leapnext - DAYBEFORE) >= DAYBEFORE)
			setnexttimeout(DAYBEFORE);
		else
			setnexttimeout(leapnext - DAYBEFORE);
		return;
	}

	/*
	 * Here we're in the day of the leap.  Set the leap indicator
	 * bits from the warning, if necessary.
	 */
	if (leap_indicator == 0 && leap_warning != 0)
		leap_indicator = leap_warning;
	
	if (leapnext > TWOHOURSBEFORE) {
		leapbits = 0;
		leapseenstratum1 = 0;
		setnexttimeout(leapnext - TWOHOURSBEFORE);
		return;
	}

	/*
	 * Here we're in the final 2 hours.  If sys_leap is set, set
	 * leapbits to it.
	 */
	if (sys_peer != 0 && sys_peer->stratum == 1) {
		/*
		 * If we're stratum 1, we leap on the basis of
		 * leap_indicator only.
		 */
		leapseenstratum1 = 1;
		leapbits = 0;
	}
	if (!leapseenstratum1
	    && (sys_leap == LEAP_ADDSECOND || sys_leap == LEAP_DELSECOND))
		leapbits = sys_leap;
	setnexttimeout((leapnext > FIVEMINUTES) ? FIVEMINUTES : leapnext);
}


/*
 * setnexttimeout - set the next leap alarm
 */
static void
setnexttimeout(secs)
	u_long secs;
{
	/*
	 * We try to aim the time out at between 1 and 1+(1<<EVENT_TIMEOUT)
	 * seconds after the desired time.
	 */
	leap_timer = (secs + 1 + (1<<EVENT_TIMEOUT) + current_time)
	    & ~((1<<EVENT_TIMEOUT)-1);
}


/*
 * leap_setleap - set leap_indicator and/or leap_warning.  Return failure
 *		  if we don't want to do it.
 */
int
leap_setleap(indicator, warning)
	int indicator;
	int warning;
{
	u_long leapnext;
	u_long leaplast;
	l_fp ts;
	int i;
	extern void get_systime();
	extern void calleapwhen();

	get_systime(&ts);
	calleapwhen(ts.l_ui, &leaplast, &leapnext);

	warning &= LEAP_NOTINSYNC;
	indicator &= LEAP_NOTINSYNC;

	i = 0;
	if (warning != LEAP_NOTINSYNC)
		if (leapnext > OKAYTOSETWARNING)
			i = 1;

	if (indicator != LEAP_NOTINSYNC)
		if (leapnext > DAYBEFORE)
			i = 1;
	
	if (i) {
		syslog(LOG_ERR,
		    "attempt to set leap bits at unlikely time of month");
		return 0;
	}

	if (warning != LEAP_NOTINSYNC)
		leap_warning = warning;

	if (indicator != LEAP_NOTINSYNC) {
		if (indicator == LEAP_NOWARNING) {
			leap_warning = LEAP_NOWARNING;
		}
		leap_indicator = indicator;
	}
	return 1;
}
