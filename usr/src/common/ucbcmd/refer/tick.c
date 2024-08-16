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

#ident	"@(#)ucb:common/ucbcmd/refer/tick.c	1.2"
#ident	"$Header: $"
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved. The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
 * All Rights Reserved.
 */


/* time programs */
# include "stdio.h"
# include "sys/types.h"

struct tbuffer {
	long	proc_user_time;
	long	proc_system_time;
	long	child_user_time;
	long	child_system_time;
};
static long start, user, systm;
tick()
{
	struct tbuffer tx;
	time_t tp;
	times (&tx);
	time (&tp);
	user =  tx.proc_user_time;
	systm= tx.proc_system_time;
	start = tp;
}
tock()
{
	struct tbuffer tx;
	time_t tp;
	float lap, use, sys;
	if (start==0) return;
	times (&tx);
	time (&tp);
	lap = (tp - start)/60.;
	use = (tx.proc_user_time - user)/60.;
	sys = (tx.proc_system_time - systm)/60.;
	printf("Elapsed %.2f CPU %.2f (user %.2f, sys %.2f)\n",
		lap, use+sys, use, sys);
}
