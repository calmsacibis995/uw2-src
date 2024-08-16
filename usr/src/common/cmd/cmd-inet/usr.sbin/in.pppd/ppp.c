/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static char TCPID[] = "@(#)ppp.c	1.2 STREAMWare for Unixware 2.0 source";
#endif /* lint */
/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1994 Lachman Technology, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */
/*      SCCS IDENTIFICATION        */
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.pppd/ppp.c	1.3"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */


#include <stdio.h>
#include <pwd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stropts.h>
#include <sys/signal.h>
#include <sys/syslog.h>
#include <netinet/in.h>

#include "./pppd.h"

char *program_name;

/* SIGNAL Processing
 * SIGHUP -- sent by in.pppd when it gets a close from kernel or a second
 *           SEND_FD from this process
 * SIGINT -- ignored
 * SIGQUIT -- ignored
 * SIGTERM -- notify daemon that its being killed so that daemon
 *            can close the connection
 */

int	pid;

/*
 * notify_daemon -
 *  SIGTERM signal handler
 */
notify_daemon(sig)
	int	sig;
{
	msg_t	msg;
	int	s;
	int	rval;

	s = ppp_sockinit();

	memset((char *)&msg, 0, sizeof(msg));
	msg.m_type = MPID;
	msg.m_pid = pid;
	rval = write(s, (char *)&msg, sizeof(msg));
	if (rval < 0) {
		syslog(LOG_INFO, "write to socket failed: %m");
		exit(2);
	}
	close(s);
	syslog(LOG_INFO, "notify_daemon sig=%d pid=%d", sig, pid);
	exit(0);
}

/*
 * sig_hup - SIGHUP signal handler
 *		pppd sends a SIGHUP when it receives a PPCID_CLOSE.
 */
sig_hup(sig)
	int	sig;
{
	syslog(LOG_INFO, "sig_hup sig=%d", sig);
	exit(0);
}

main (argc,argv)
	int	argc;
	char	*argv[];
{
	char *p, *ttyname();
	extern char *getlogin();
	msg_t msg;
	int	s, c;
	int	rval;
	int	userid;
	struct 	passwd *pw;
	struct 	ppphostent *hp;
        char *login_name;

	program_name = strrchr(argv[0],'/');
	program_name = program_name ? program_name + 1 : argv[0];

	sigignore(SIGQUIT);
	sigignore(SIGINT);
	sigset(SIGTERM, notify_daemon);
	sigset(SIGHUP, sig_hup);
	pid = getpid();


#if defined(LOG_DAEMON)
	openlog(program_name, LOG_PID|LOG_CONS|LOG_NDELAY, LOG_DAEMON);
#else
	openlog(program_name, LOG_PID);
#endif

	if (!(p = ttyname(0))) {
		syslog(LOG_INFO, "not a tty!?");
		exit(1);
	}

	syslog(LOG_INFO, "login on '%s'", p);

	/* get the login name, which will be used to get
	 * configurable parameters */
	 if ((login_name = getlogin()) == NULL){
		 syslog(LOG_INFO, "invalid user");
		 exit(1);
	}

	memset((char *)&msg, 0, sizeof(msg));
	msg.m_type = MTTY;
	msg.m_pid = pid;
	strncpy(msg.m_tty, p, TTY_SIZE);
	strncpy(msg.m_name, login_name, NAME_SIZE);

	s = ppp_sockinit();
	rval = write(s, (char *)&msg, sizeof(msg));
	if (rval < 0) {
		syslog(LOG_INFO, "write to socket failed: %m");
		exit(1);
	}
	close(s);
	syslog(LOG_INFO, "sent pppd m_type=%d, m_pid=%d, m_tty='%s'",
			msg.m_type, msg.m_pid, msg.m_tty);

	/*
	 * Wait for daemon to notify (signal)
	 * us that we can go away.
	 */
	while(pause() == -1);

	exit(0);
}
