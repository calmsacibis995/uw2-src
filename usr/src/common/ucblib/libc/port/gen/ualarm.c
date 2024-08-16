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

#ident	"@(#)ucb:common/ucblib/libc/port/gen/ualarm.c	1.2"
#ident	"$Header: $"
/*
 * Copyright (c) 1985 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
/* 	Portions Copyright(c) 1988, Sun Microsystems Inc.	*/
/*	All Rights Reserved					*/


#include <sys/time.h>

#define	USPS	1000000		/* # of microseconds in a second */

/*
 * Generate a SIGALRM signal in ``usecs'' microseconds.
 * If ``reload'' is non-zero, keep generating SIGALRM
 * every ``reload'' microseconds after the first signal.
 */
unsigned
ualarm(usecs, reload)
	register unsigned usecs;
	register unsigned reload;
{
	struct itimerval new, old;

	new.it_interval.tv_usec = reload % USPS;
	new.it_interval.tv_sec = reload / USPS;
	
	new.it_value.tv_usec = usecs % USPS;
	new.it_value.tv_sec = usecs / USPS;

	if (setitimer(ITIMER_REAL, &new, &old) == 0)
		return (old.it_value.tv_sec * USPS + old.it_value.tv_usec);
	/* else */
		return (-1);
}
