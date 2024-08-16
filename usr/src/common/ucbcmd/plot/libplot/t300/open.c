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

#ident	"@(#)ucb:common/ucbcmd/plot/libplot/t300/open.c	1.2"
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


#
#include <sgtty.h>
/* gsi plotting output routines */
# define DOWN 012
# define UP 013
# define LEFT 010
# define RIGHT 040
# define BEL 007
# define ACK 006
# define CR 015
# define FF 014
# define VERTRESP 48
# define HORZRESP 60.
# define HORZRES 6.
# define VERTRES 8.
/* down is line feed, up is reverse oyne feed,
   left is bwckspace, right is space.  48 points per inch
   vertically, 60 horizontally */

int xnow, ynow;
int OUTF;
struct sgttyb ITTY, PTTY;
float HEIGHT = 6.0, WIDTH = 6.0, OFFSET = 0.0;
int xscale, xoffset, yscale;
float botx = 0., boty = 0., obotx = 0., oboty = 0.;
float scalex = 1., scaley = 1.;

openpl ()
{
	int reset();
		xnow = ynow = 0;
		OUTF = 1;
		printf("\r");
		gtty(OUTF, &ITTY);
		signal (2, reset);
		PTTY = ITTY;
		PTTY.sg_flags &= ~CRMOD;	/* don't map lf */
		/* initialize constants */
		xscale  = 4096./(HORZRESP * WIDTH);
		yscale = 4096 /(VERTRESP * HEIGHT);
		xoffset = OFFSET * HORZRESP;
		return;
}

openvt(){
openpl();
}
