/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rpcsvc:spray_subr.c	1.1.5.4"
#ident  "$Header: spray_subr.c 1.2 91/06/27 $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 
/*
 * "spray_subr.c 1.1 89/03/22 Copyr 1985 Sun Micro";
 */

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <rpc/rpc.h>
#include <rpcsvc/spray.h>

static spraycumul cumul;
static spraytimeval start_time;

void *
sprayproc_spray_1(argp, clnt)
	sprayarr *argp;
	CLIENT *clnt;
{
	cumul.counter++;
	return ((void *)0);
}

spraycumul *
sprayproc_get_1(argp, clnt)
	void *argp;
	CLIENT *clnt;
{
	gettimeofday(&cumul.clock, 0);
	if (cumul.clock.usec < start_time.usec) {
		cumul.clock.usec += 1000000;
		cumul.clock.sec -= 1;
	}
	cumul.clock.sec -= start_time.sec;
	cumul.clock.usec -= start_time.usec;
	return (&cumul);
}

void *
sprayproc_clear_1(argp, clnt)
	void *argp;
	CLIENT *clnt;
{
	static char res;

	cumul.counter = 0;
	gettimeofday(&start_time, 0);
	(void) memset((char *)&res, 0, sizeof(res));
	return ((void *)&res);
}
