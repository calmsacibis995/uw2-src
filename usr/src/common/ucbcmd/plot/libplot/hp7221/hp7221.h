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

#ident	"@(#)ucb:common/ucbcmd/plot/libplot/hp7221/hp7221.h	1.2"
#ident	"$Header: $"

/*
 * Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
 * All Rights Reserved.
 */

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Displays plot files on an HP7221 plotter.
 * Cloned from bgplot.c and gigiplot.c by Jim Kleckner
 * Thu Jun 30 13:35:04 PDT 1983
 *  Requires a handshaking program such as hp7221cat to get
 *  the plotter open and ready.
 */

#include <stdio.h>
#include <math.h>

#define ESC	033
#define PI	3.141592659
#define ENDOFSTRING 03

/* The graphics address range is 0..XMAX, 0..YMAX. */
#define XMAX	1800
#define YMAX	1800

#define scaleX(xi)	((int) ((xi - lowx)*scale +0.5))
#define scaleY(yi)	((int) ((yi - lowy)*scale +0.5))

extern int currentx;
extern int currenty;
extern double lowx;
extern double lowy;
extern double scale;
