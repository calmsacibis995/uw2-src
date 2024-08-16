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

#ident	"@(#)ucb:common/ucbcmd/plot/libplot/t300s/line.c	1.2"
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


#include "con.h"
line(x0,y0,x1,y1){
	iline(xconv(xsc(x0)),yconv(ysc(y0)),xconv(xsc(x1)),yconv(ysc(y1)));
	return;
}
cont(x0,y0){
	iline(xnow,ynow,xconv(xsc(x0)),yconv(ysc(y0)));
	return;
}
iline(cx0,cy0,cx1,cy1){
	int maxp,tt,j,np;
	char chx,chy,command;
	    float xd,yd;
	float dist2(),sqrt();
	movep(cx0,cy0);
	maxp = sqrt(dist2(cx0,cy0,cx1,cy1))/2.;
	xd = cx1-cx0;
	yd = cy1-cy0;
	command = COM|((xd<0)<<1)|(yd<0);
	if(maxp == 0){
		xd=0;
		yd=0;
	}
	else {
		xd /= maxp;
		yd /= maxp;
	}
	inplot();
	spew(command);
	for (tt=0; tt<=maxp; tt++){
		chx= cx0+xd*tt-xnow;
		xnow += chx;
		chx = abval(chx);
		chy = cy0+yd*tt-ynow;
		ynow += chy;
		chy = abval(chy);
		spew(ADDR|chx<<3|chy);
	}
	outplot();
	return;
}
