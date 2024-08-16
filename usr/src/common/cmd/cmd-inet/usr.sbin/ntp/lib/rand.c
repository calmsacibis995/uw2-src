/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/rand.c	1.2"
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

#include <sys/time.h>

/*
 * See
 * Knuth Vol. 2 Section 3.2.2 Equation (7)
 */
static unsigned long y[55] = {
7293406, 4008611, 8829252, 6572805, 3830042,
4232384, 6406875, 9837390, 4897105,  904918,
5994311, 1762649,  294472, 2091020, 5766273,
8018117, 3857980, 2458074, 9052997, 5230361,
5043632, 2940191, 2711764, 6139962, 5096742,
4139939, 8163627,    1313, 1563471, 7971723,
9469651, 5924013, 2554304, 9399578, 1867851,
9001244, 6364576, 8570174, 8526981, 6226371,
6833137,   38950, 7257144, 1521009, 6739220,
8942125,  962375, 8072548, 6262011, 8416287,
8736814, 2956092,   51923, 8454597,  800412
};

unsigned long
mm_rand(m)
	unsigned long m;
{
	unsigned long retval;
	static int j = 23, k = 54;

	retval = y[k] += y[j];
	if (!(k--))
		k = 54;
	if (!(j--))
		j = 54;
	if (m)
		return retval % m;
	else
		return retval;
}

void
mm_rand_init(f)
	unsigned long (*f)();
{
	int i;
	unsigned long mult;
	unsigned long cpu_time();

	if (!f) {
		f = cpu_time;
	}
	for (i = 0; i < 55; i++) {
		mult = (*f)();
		y[i] *= mult ? mult : 1;	/* don't allow a zero mult */
	}

	/* make sure that there is at least one odd number in the batch */
	for (i = 0; i < 55; i++)
		if (y[i] & 0x01)
			return;

	/* No odd numbers?  Let's make some. */
	y[1]++;
	y[13]++;
	y[27]++;
	y[39]++;
	y[45]++;
	y[51]++;

	return;
}

init_random()
{
	mm_rand_init((unsigned long (*)())0);
	return;
}

unsigned long
ranp2(e)
	int e;
{
	unsigned long mm_rand();
	unsigned long retval;

	retval = mm_rand(0);
	return retval & ((1 << e)-1);
}

static
unsigned long
cpu_time()
{
	struct timeval tv;

	gettimeofday(&tv, (struct timezone *)0);
	return tv.tv_sec + tv.tv_usec;
}
