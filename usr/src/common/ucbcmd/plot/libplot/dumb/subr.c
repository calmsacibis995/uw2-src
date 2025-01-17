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

#ident	"@(#)ucb:common/ucbcmd/plot/libplot/dumb/subr.c	1.2"
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


#include "dumb.h"

/* Does not plot first point -- assumed that it is already plotted */
dda_line(ch, x0, y0, x1, y1)
	char ch;
	int x0, y0;	/* already transformed to screen coords */
	int x1, y1;	/* untransformed */
{
	int length, i;
	double deltaX, deltaY;
	double x, y;
	double floor();
	int abs();

	scale(x1, y1);

	length = abs(x1 - x0);
	if (abs(y1 -y0) > length)
		length = abs(y1 - y0);

	if (length == 0)
		return;

	deltaX = (double) (x1 - x0)/(double) length;
	deltaY = (double) (y1 - y0)/(double) length;

	x = (double) x0 + 0.5;
	y = (double) y0 + 0.5;

	for (i=0; i < length; ++i) {
		x += deltaX;
		y += deltaY;
		x0 = floor(x);
		y0 = floor(y);
		currentx = x0;
		currenty = y0;
		screenmat[currentx][currenty] = ch;
	}
}
