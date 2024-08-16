/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ttymon:common/cmd/ttymon/ttymon.notp/tmsig.c	1.1"

#include	<stdio.h>
#include	<signal.h>
#include	"tmextern.h"

extern	char	Scratch[];
extern	void	log();

/*
 * catch_signals:
 *	ttymon catch some signals and ignore the rest.
 *
 *	SIGTERM	- killed by somebody
 *	SIGPOLL - got message on pmpipe, probably from sac
 *			   or on PCpipe
 *	SIGCLD	- tmchild died
 */
void
catch_signals()
{
	sigset_t cset;
	struct sigaction sigact;
	extern void sigterm();
	extern void sigchild();
	extern void sigpoll_catch();
#ifdef	DEBUG
	extern void dump_pmtab();
	extern void dump_ttydefs();
	extern void debug();

	debug("in catch_signals");
#endif

	cset = Origmask;
	(void)sigdelset(&cset, SIGTERM);
	(void)sigdelset(&cset, SIGCLD);
	(void)sigdelset(&cset, SIGPOLL);
#ifdef	DEBUG
	(void)sigdelset(&cset, SIGUSR1);
	(void)sigdelset(&cset, SIGUSR2);
#endif
	(void)sigprocmask(SIG_SETMASK, &cset, NULL);
	sigact.sa_flags = 0;
	sigact.sa_handler = sigterm;
	(void)sigemptyset(&sigact.sa_mask);
	(void)sigaddset(&sigact.sa_mask, SIGTERM);
	(void)sigaction(SIGTERM, &sigact, NULL);
	sigact.sa_flags = 0;
	sigact.sa_handler = sigchild;
	(void)sigemptyset(&sigact.sa_mask);
	(void)sigaction(SIGCLD, &sigact, NULL);
	sigact.sa_flags = 0;
	sigact.sa_handler = sigpoll_catch;
	(void)sigemptyset(&sigact.sa_mask);
	(void)sigaddset(&sigact.sa_mask, SIGPOLL);
	(void)sigaction(SIGPOLL, &sigact, NULL);
#ifdef	DEBUG
	sigact.sa_flags = 0;
	sigact.sa_handler = dump_pmtab;
	(void)sigemptyset(&sigact.sa_mask);
	(void)sigaddset(&sigact.sa_mask, SIGUSR1);
	(void)sigaction(SIGUSR1, &sigact, NULL);
	sigact.sa_flags = 0;
	sigact.sa_handler = dump_ttydefs;
	(void)sigemptyset(&sigact.sa_mask);
	(void)sigaddset(&sigact.sa_mask, SIGUSR2);
	(void)sigaction(SIGUSR2, &sigact, NULL);
#endif
}

/*
 * child_sigcatch() - tmchild inherits some signal_catch from parent
 *		      and need to reset them
 */
void
child_sigcatch()
{
	struct	sigaction	sigact;
	sigset_t cset;
	extern	void	sigpoll();

	cset = Origmask;
	(void)sigdelset(&cset, SIGINT);
	(void)sigdelset(&cset, SIGPOLL);
	(void)sigprocmask(SIG_SETMASK, &cset, NULL);
	sigact.sa_flags = 0;
	sigact.sa_handler = sigpoll;
	(void)sigemptyset(&sigact.sa_mask);
	(void)sigaddset(&sigact.sa_mask, SIGPOLL);
	(void)sigaction(SIGPOLL, &sigact, NULL);
}
