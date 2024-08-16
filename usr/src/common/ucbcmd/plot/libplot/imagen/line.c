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

#ident	"@(#)ucb:common/ucbcmd/plot/libplot/imagen/line.c	1.2"
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


#include "imp.h"
#include "imPcodes.h"
float obotx = 0.;
float oboty = 0.;
float botx = 2.;
float boty = 2.;
float scalex = 1.;
float scaley = 1.;
line(x0,y0,x1,y1)
{
	putch(imP_CREATE_PATH);
	putwd(2);		/* two coordinates follow */
	putwd((int)((x0-obotx)*scalex+botx));	
	putwd((int)((y0-oboty)*scaley+boty));	
	putwd((int)((x1-obotx)*scalex+botx));	
	putwd((int)((y1-oboty)*scaley+boty));	
	putch(imP_DRAW_PATH);
	putch(15);		/* "black" lines */
	imPx = x1;
	imPy = y1;
}
putch(c)
{
	putc(c, stdout);
}
putwd(w)
{
	putch(w>>8);
	putch(w);
}
