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

#ident	"@(#)ucb:common/ucbcmd/plot/libplot/gigi/open.c	1.2"
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


/*
 * Displays plot files on a gigi "graphics" terminal.
 */

#include <signal.h>
#include "gigi.h"

int currentx = 0;
int currenty = 0;
double lowx = 0.0;
double lowy = 0.0;
double scalex = 1.0;
double scaley = 1.0;

openpl()
{
	void closepl();

	/* catch interupts */
	signal(SIGINT, closepl);
	currentx = 0;
	currenty = 0;
	/* enter grapics mode */
	putchar(ESC); putchar('P'); putchar('p');

	/* set some parameters */
	printf("S(I0 T0 [0,0])");

	space(0, 0, XMAX, YMAX);
}
