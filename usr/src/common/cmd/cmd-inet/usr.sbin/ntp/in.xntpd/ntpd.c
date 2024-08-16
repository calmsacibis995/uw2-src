/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/in.xntpd/ntpd.c	1.2"
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
 * ntpd.c - main program for the fixed point NTP daemon
 */
#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <string.h>

#include "ntp_syslog.h"
#include "ntp_fp.h"
#include "ntp.h"

#if defined(ULT_2_0)
#ifndef sigmask
#define	sigmask(m)	(1<<(m))
#endif
#endif

#ifndef SIGIO
#define SIGIO SIGPOLL
#endif

/*
 * Mask for blocking SIGIO and SIGALRM
 */
#ifdef SYSV
#define SIGBLOCK(osig)		(sighold(SIGALRM),sighold(SIGIO))
#define SIGUNBLOCK(osig)	(sigrelse(SIGALRM),sigrelse(SIGIO))
#else
#define	BLOCKSIGMASK	(sigmask(SIGIO)|sigmask(SIGALRM))
#define SIGBLOCK(osig)		(osig = sigblock(BLOCKSIGMASK))
#define SIGUNBLOCK(osig)	((void) sigsetmask(osig))
#define
#endif

/*
 * Signals we catch for debugging.  If not debugging we ignore them.
 */
#define	MOREDEBUGSIG	SIGUSR1
#define	LESSDEBUGSIG	SIGUSR2

/*
 * Signals which terminate us gracefully.
 */
#define	SIGDIE1		SIGHUP
#define	SIGDIE2		SIGINT
#define	SIGDIE3		SIGQUIT
#define	SIGDIE4		SIGTERM

/*
 * Scheduling priority we run at
 */
#ifdef SYSV
#define	NTPD_PRIO	(0)
#else
#define	NTPD_PRIO	(-12)
#endif

/*
 * Debugging flag
 */
int debug;

/*
 * Initializing flag.  All async routines watch this and only do their
 * thing when it is clear.
 */
int initializing;

/*
 * Version declaration
 */
extern char *Version;

/*
 * Alarm flag.  Imported from timer module
 */
extern int alarm_flag;


/*
 * Main program.  Initialize us, disconnect us from the tty if necessary,
 * and loop waiting for I/O and/or timer expiries.
 */
main(argc, argv)
	int argc;
	char *argv[];
{
	char *cp;
	int was_alarmed;
	struct recvbuf *rbuflist;
	struct recvbuf *rbuf;
	extern struct recvbuf *getrecvbufs();
	extern void receive();
	extern void getstartup();
#if 0
#ifdef DEBUG
	void moredebug(), lessdebug();
#endif
#endif
#ifdef SIGDIE1
	void finish();
#endif	/* SIGDIE1 */

	initializing = 1;	/* mark that we are initializing */
	debug = 0;		/* no debugging by default */

	getstartup(argc, argv);	/* startup configuration, may set debug */

#ifndef NODETACH
	/*
	 * Detach us from the terminal.  May need an #ifndef GIZMO.
	 */
#ifdef	DEBUG
	if (!debug) {
#endif
		if (fork())
			exit(0);

		{
			int s;
			for (s = getdtablesize(); s >= 0; s--)
				(void) close(s);
			(void) open("/", 0);
			(void) dup2(0, 1);
			(void) dup2(0, 2);
#ifdef SYSV
			(void) setpgrp();
#else
			(void) setpgrp(0, getpid());
			s = open("/dev/tty", 2);
			if (s >= 0) {
				(void) ioctl(s, (u_long) TIOCNOTTY, (char *) 0);
				(void) close(s);
			}
#endif
		}
#ifdef	DEBUG
	}
#endif
#endif /* NODETACH */

	/*
	 * Logging.  This may actually work on the gizmo board.  Find a name
	 * to log with by using the basename of argv[0]
	 */
	cp = rindex(argv[0], '/');
	if (cp == 0)
		cp = argv[0];
	else
		cp++;

#ifndef	LOG_DAEMON
	openlog(cp, LOG_PID);
#else

#ifndef	LOG_NTP
#define	LOG_NTP	LOG_DAEMON
#endif
	openlog(cp, LOG_PID | LOG_NDELAY, LOG_NTP);
#ifdef	DEBUG
	if (debug)
		setlogmask(LOG_UPTO(LOG_DEBUG));
	else
#endif	/* DEBUG */
		setlogmask(LOG_UPTO(LOG_INFO));
#endif	/* LOG_DAEMON */

	syslog(LOG_INFO, Version);


	/*
	 * Set the priority.
	 */
#ifdef SYSV
	(void) nice(NTPD_PRIO-20);
#else
#if defined(NTPD_PRIO) && NTPD_PRIO != 0
	(void) setpriority(PRIO_PROCESS, 0, NTPD_PRIO);
#endif	/* ... */
#endif

	/*
	 * Set up signals we pay attention to locally.
	 */
#ifdef SIGDIE1
	(void) sigset(SIGDIE1, finish);
#endif	/* SIGDIE1 */
#ifdef SIGDIE2
	(void) sigset(SIGDIE2, finish);
#endif	/* SIGDIE2 */
#ifdef SIGDIE3
	(void) sigset(SIGDIE3, finish);
#endif	/* SIGDIE3 */
#ifdef SIGDIE4
	(void) sigset(SIGDIE4, finish);
#endif	/* SIGDIE4 */

#if 0
#ifdef DEBUG
	(void) sigset(MOREDEBUGSIG, moredebug);
	(void) sigset(LESSDEBUGSIG, lessdebug);
#else
	(void) sigset(MOREDEBUGSIG, SIG_IGN);
	(void) sigset(LESSDEBUGSIG, SIG_IGN);
#endif 	/* DEBUG */
#endif

	/*
	 * Call the init_ routines to initialize the data structures.
	 * Note that init_systime() may run a protocol to get a crude
	 * estimate of the time as an NTP client when running on the
	 * gizmo board.  It is important that this be run before
	 * init_subs() since the latter uses the time of day to seed
	 * the random number generator.  That is not the only
	 * dependency between these, either, be real careful about
	 * reordering.
	 */
	init_auth();
	init_util();
	init_restrict();
	init_loopfilter();
	init_mon();
	init_systime();
	init_timer();
	init_lib();
	init_random();
	init_request();
	init_leap();
	init_peer();
#ifdef REFCLOCK
	init_refclock();
#endif
	init_proto();
	init_io();

	/*
	 * Get configuration.  This (including argument list parsing) is
	 * done in a separate module since this will definitely be different
	 * for the gizmo board.
	 */
	getconfig(argc, argv);
	initializing = 0;

	/*
	 * Report that we're up to any trappers
	 */
	report_event(EVNT_SYSRESTART, (struct peer *)0);

	/*
	 * Done all the preparation stuff, now the real thing.  We block
	 * SIGIO and SIGALRM and check to see if either has occured.
	 * If not, we pause until one or the other does.  We then call
	 * the timer processing routine and/or feed the incoming packets
	 * to the protocol module.  Then around again.
	 */
	was_alarmed = 0;
	rbuflist = (struct recvbuf *)0;
	for (;;) {
		int omask;

#if 0
		omask = sigblock(BLOCKSIGMASK);
#else
		SIGBLOCK(omask);
#endif
		if (alarm_flag) {		/* alarmed? */
			was_alarmed = 1;
			alarm_flag = 0;
		}
		rbuflist = getrecvbufs();	/* get received buffers */

		if (!was_alarmed && rbuflist == (struct recvbuf *)0) {
			/*
			 * Nothing to do.  Wait for something.
			 */
#ifdef SYSV
			SIGUNBLOCK(omask);
			pause();
			SIGBLOCK(omask);
#else
			sigpause(omask);
#endif
			if (alarm_flag) {		/* alarmed? */
				was_alarmed = 1;
				alarm_flag = 0;
			}
			rbuflist = getrecvbufs();  /* get received buffers */
		}
#if 0
		(void)sigsetmask(omask);
#else
		(void)SIGUNBLOCK(omask);
#endif

		/*
		 * Out here, signals are unblocked.  Call timer routine
		 * to process expiry.
		 */
		if (was_alarmed) {
			timer();
			was_alarmed = 0;
		}

		/*
		 * Call the data procedure to handle each received
		 * packet.
		 */
		while (rbuflist != (struct recvbuf *)0) {
			rbuf = rbuflist;
			rbuflist = rbuf->next;
			(rbuf->receiver)(rbuf);
			freerecvbuf(rbuf);
		}
		/*
		 * Go around again
		 */
	}
}


#ifdef SIGDIE1
/*
 * finish - exit gracefully
 */
void
finish()
{
	struct timeval tv;

	/*
	 * The only thing we really want to do here is make sure
	 * any pending time adjustment is terminated, as a bug
	 * preventative.  Also log any useful info before exiting.
	 */
	tv.tv_sec = tv.tv_usec = 0;
	(void) adjtime(&tv, (struct timeval *)0);

#ifdef notdef
	log_exit_stats();
#endif
	exit(0);
}
#endif	/* SIGDIE1 */


#ifdef DEBUG
/*
 * moredebug - increase debugging verbosity
 */
void
moredebug()
{
	if (debug < 255)
		debug++;
}


/*
 * lessdebug - decrease debugging verbosity
 */
void
lessdebug()
{
	if (debug > 0)
		debug--;
}
#endif	/* DEBUG */
