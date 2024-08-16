/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)ucb:common/ucblib/libc/port/gen/usleep.c	1.2"
#ident	"$Header: $"
/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
/* 	Portions Copyright(c) 1988, Sun Microsystems Inc.	*/
/*	All Rights Reserved					*/


#include <sys/time.h>
#include <signal.h>

#define USPS	1000000		/* number of microseconds in a second */
#define TICK	10000		/* system clock resolution in microseconds */

#define	setvec(vec, a) \
	vec.sv_handler = a; vec.sv_mask = vec.sv_onstack = 0

static int ringring;

usleep(n)
	unsigned n;
{
	static void sleepx();
	int omask;
	struct itimerval itv, oitv;
	register struct itimerval *itp = &itv;
	struct sigvec vec, ovec;

	if (n == 0)
		return;
	timerclear(&itp->it_interval);
	timerclear(&itp->it_value);
	if (setitimer(ITIMER_REAL, itp, &oitv) < 0)
		return;
	itp->it_value.tv_sec = n / USPS;
	itp->it_value.tv_usec = n % USPS;
	if (timerisset(&oitv.it_value)) {
		if (timercmp(&oitv.it_value, &itp->it_value, >)) {
			oitv.it_value.tv_sec -= itp->it_value.tv_sec;
			oitv.it_value.tv_usec -= itp->it_value.tv_usec;
			if (oitv.it_value.tv_usec < 0) {
				oitv.it_value.tv_usec += USPS;
				oitv.it_value.tv_sec--;
			}
		} else {
			itp->it_value = oitv.it_value;
			oitv.it_value.tv_sec = 0;
			oitv.it_value.tv_usec = 2 * TICK;
		}
	}
	setvec(vec, sleepx);
	(void) sigvec(SIGALRM, &vec, &ovec);
	omask = sigblock(sigmask(SIGALRM));
	ringring = 0;
	(void) setitimer(ITIMER_REAL, itp, (struct itimerval *)0);
	while (!ringring)
		sigpause(omask &~ sigmask(SIGALRM));
	(void) sigvec(SIGALRM, &ovec, (struct sigvec *)0);
	(void) sigsetmask(omask);
	(void) setitimer(ITIMER_REAL, &oitv, (struct itimerval *)0);
}

static void
sleepx()
{

	ringring = 1;
}
