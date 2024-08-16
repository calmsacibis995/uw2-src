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

#ident	"@(#)ucb:common/ucbcmd/eqn/eqnbox.c	1.2"
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
  

# include "e.h"

eqnbox(p1, p2, lu) {
	int b, h;
	char *sh;

	yyval = p1;
	b = max(ebase[p1], ebase[p2]);
	eht[yyval] = h = b + max(eht[p1]-ebase[p1], 
		eht[p2]-ebase[p2]);
	ebase[yyval] = b;
	if(dbg)printf(".\te:eb: S%d <- S%d S%d; b=%d, h=%d\n", 
		yyval, p1, p2, b, h);
	if (rfont[p1] == ITAL && lfont[p2] == ROM)
		sh = "\\|";
	else
		sh = "";
	if (lu) {
		printf(".nr %d \\w'\\s%d\\*(%d%s'\n", p1, ps, p1, sh);
		printf(".ds %d \\h'|\\n(97u-\\n(%du'\\*(%d\n", p1, p1, p1);
	}
	printf(".as %d \"%s\\*(%d\n", yyval, sh, p2);
	rfont[p1] = rfont[p2];
	ofree(p2);
}
