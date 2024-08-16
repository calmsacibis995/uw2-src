/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)renice:renice.c	1.1"
#ident	"@(#)ucb:common/ucbcmd/renice/renice.c	1.1"
#ident	"$Header: $"

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

#ifndef lint
static	char *sccsid = "@(#)renice.c 1.7 88/08/04 SMI"; /* from UCB 4.6 83/07/24 */
#endif

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#include <stdio.h>
#include <pwd.h>
#include <locale.h>
#include <pfmt.h>

void	usage();
int	nflag = 0;

#define PRIO_MAX	20
#define PRIO_MIN	-20

/*
 * Change the priority (nice) of processes
 * or groups of processes which are already
 * running.
 */
main(argc, argv)
	char **argv;
{
	int which = PRIO_PROCESS;
	int who = 0, prio, errs = 0;
	int increment = 0;
	struct passwd *pwd;
	char *c;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:renice");

	argc--, argv++;
	if (argc < 2) {
		usage();
		/* NOTREACHED */
	}
	if (strcmp(*argv, "-n") == 0) {
		if (argc < 3) {
			usage();
			/* NOTREACHED */
		}
		argc--, argv++;
		nflag = 1;
		increment = atoi(*argv);
		argc--, argv++;
	} else {
		prio = atoi(*argv);
		argc--, argv++;
		if (prio > PRIO_MAX)
			prio = PRIO_MAX;
		if (prio < PRIO_MIN)
			prio = PRIO_MIN;
	}
	for (; argc > 0; argc--, argv++) {
		if (strcmp(*argv, "-g") == 0) {
			which = PRIO_PGRP;
			continue;
		}
		if (strcmp(*argv, "-u") == 0) {
			which = PRIO_USER;
			continue;
		}
		if (strcmp(*argv, "-p") == 0) {
			which = PRIO_PROCESS;
			continue;
		}
		if (which == PRIO_USER) {
                      if ((who = getint(*argv)) < 0) {
				pwd = getpwnam(*argv);
				if (pwd == NULL) {
					pfmt(stderr, MM_ERROR,
						 ":962:%s: unknown user\n",
					*argv);
					continue;
				} else 
					who = pwd->pw_uid;
			}
		} else {
                      if ((who = getint(*argv)) < 0) {
				pfmt(stderr, MM_ERROR, ":963:%s: bad value\n",
					*argv);
				continue;
			}
		}
		errs += donice(which, who, prio, increment);
	}
	exit(errs != 0);
	/* NOTREACHED */
}

donice(which, who, prio, increment)
	int which, who, prio, increment;
{
	int oldprio;
	extern int errno;

	errno = 0, oldprio = getpriority(which, who);
	if (oldprio == -1 && errno) {
		pfmt(stderr, MM_ERROR, ":192:%d:", who);
		perror(gettxt(":963", " getpriority"));
		return (1);
	}
	if (nflag) {
		if (setpriority(which, who, oldprio + increment) < 0) {
			pfmt(stderr, MM_ERROR, ":192:%d:", who);
			perror(gettxt(":964", " setpriority"));
			return (1);
		}
		pfmt(stdout, MM_NOSTD,
			":966:%d: old priority %d, new priority %d\n",
			who, oldprio, oldprio + increment);
	} else {
		if (setpriority(which, who, prio) < 0) {
			pfmt(stderr, MM_ERROR, ":192:%d:", who);
			perror(gettxt(":964", " setpriority"));
			return (1);
		}
		pfmt(stdout, MM_NOSTD,
			":966:%d: old priority %d, new priority %d\n",
			who, oldprio, prio);
	}
	return (0);
}

void
usage()
{
	pfmt(stderr, MM_ACTION, 
	   ":959:Usage: renice [ -n increment ] [ -g | -p | -u ] ID\n");
	pfmt(stderr, MM_NOSTD, 
		":960:                          renice priority [ [ -p ] pids ]");
	pfmt(stderr, MM_NOSTD,
		":961: [ [ -g ] pgrps ] [ [ -u ] users ]\n");
	exit(1);
}

getint(arg)
char *arg;
{
	char *c;

        c = arg;
        while (*c != '\0') {
		if (!isdigit(*c)) 
        		break;
        	else c++;
        }
        if (*c == '\0')
        	return(atoi(arg));
	else 
		return(-1);
}
