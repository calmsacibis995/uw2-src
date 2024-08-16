/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/in.xntpd/ntp_timer.c	1.2"
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
 * ntp_event.c - event timer support routines
 */
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "ntp_syslog.h"
#include "ntp_fp.h"
#include "ntp.h"

/*
 * These routines provide support for the event timer.  The timer is
 * implemented by an interrupt routine which sets a flag once every
 * 2**EVENT_TIMEOUT seconds (currently 4), and a timer routine which
 * is called when the mainline code gets around to seeing the flag.
 * The timer routine dispatches the clock adjustment code if its time
 * has come, then searches the timer queue for expiries which are
 * dispatched to the transmit procedure.  Finally, we call the hourly
 * procedure to do cleanup and print a message.
 */

/*
 * Alarm flag.  The mainline code imports this.
 */
int alarm_flag;

/*
 * adjust and hourly counters
 */
u_long adjust_timer;
u_long hourly_timer;

/*
 * Imported from the leap module.  The leap timer.
 */
extern u_long leap_timer;

/*
 * Statistics counter for the interested.
 */
u_long alarm_overflow;

#define	HOUR	(60*60)

/*
 * Current_time holds the number of seconds since we started, in
 * increments of 2**EVENT_TIMEOUT seconds.  The timer queue is the
 * hash into which we sort timer entries.
 */
u_long current_time;
struct event timerqueue[TIMER_NSLOTS];

/*
 * Stats.  Number of overflows and number of calls to transmit().
 */
u_long timer_timereset;
u_long timer_overflows;
u_long timer_xmtcalls;


/*
 * init_timer - initialize the timer data structures
 */
void
init_timer()
{
	register int i;
	struct itimerval itimer;
	void alarming();

	/*
	 * Initialize...
	 */
	alarm_flag = 0;
	alarm_overflow = 0;
	adjust_timer = (1<<CLOCK_ADJ);
	hourly_timer = HOUR;
	current_time = 0;
	timer_overflows = 0;
	timer_xmtcalls = 0;
	timer_timereset = 0;

	for (i = 0; i < TIMER_NSLOTS; i++) {
		/*
		 * Queue pointers should point at themselves.  Event
		 * times must be set to 0 since this is used to
		 * detect the queue end.
		 */
		timerqueue[i].next = &timerqueue[i];
		timerqueue[i].prev = &timerqueue[i];
		timerqueue[i].event_time = 0;
	}

	/*
	 * Set up the alarm interrupt.  The first comes 2**EVENT_TIMEOUT
	 * seconds from now and they continue on every 2**EVENT_TIMEOUT
	 * seconds.
	 */
	(void) sigset(SIGALRM, alarming);
	itimer.it_interval.tv_sec = itimer.it_value.tv_sec = (1<<EVENT_TIMEOUT);
	itimer.it_interval.tv_usec = itimer.it_value.tv_usec = 0;
	setitimer(ITIMER_REAL, &itimer, (struct itimerval *)0);
}



/*
 * timer - dispatch anyone who needs to be
 */
void
timer()
{
	register struct event *ev;
	register struct event *tq;
	extern void hourly_stats();
	extern void leap_process();

	current_time += (1<<EVENT_TIMEOUT);

	/*
	 * Adjustment timeout first
	 */
	if (adjust_timer <= current_time) {
		adjust_timer += (1<<CLOCK_ADJ);
		adj_host_clock();
	}

	/*
	 * Leap timer next.
	 */
	if (leap_timer != 0 && leap_timer <= current_time)
		leap_process();

	/*
	 * Now dispatch any peers whose event timer has expired.
	 */
	tq = &timerqueue[TIMER_SLOT(current_time)];
	ev = tq->next;
	while (ev->event_time != 0
	    && ev->event_time < (current_time + (1<<EVENT_TIMEOUT))) {
		tq->next = ev->next;
		tq->next->prev = tq;
		ev->prev = ev->next = 0;
		timer_xmtcalls++;
		ev->event_handler(ev->peer);
		ev = tq->next;
	}

	/*
	 * Finally, call the hourly routine
	 */
	if (hourly_timer <= current_time) {
		hourly_timer += HOUR;
		hourly_stats();
	}
}


/*
 * alarming - tell the world we've been alarmed
 */
void
alarming()
{
	extern int initializing;	/* from main line code */

	if (initializing)
		return;
	if (alarm_flag)
		alarm_overflow++;
	else
		alarm_flag++;
}


/*
 * timer_clr_stats - clear timer module stat counters
 */
void
timer_clr_stats()
{
	timer_overflows = 0;
	timer_xmtcalls = 0;
	timer_timereset = current_time;
}
