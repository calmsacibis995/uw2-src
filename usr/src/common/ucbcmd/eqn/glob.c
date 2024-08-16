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

#ident	"@(#)ucb:common/ucbcmd/eqn/glob.c	1.2"
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
  

#include "e.h"

int	dbg;	/* debugging print if non-zero */
int	lp[80];	/* stack for things like piles and matrices */
int	ct;	/* pointer to lp */
int	used[100];	/* available registers */
int	ps;	/* default init point size */
int	deltaps	= 3;	/* default change in ps */
int	gsize	= 10;	/* default initial point size */
int	gfont	= ITAL;	/* italic */
int	ft;	/* default font */
FILE	*curfile;	/* current input file */
int	ifile;
int	linect;	/* line number in file */
int	eqline;	/* line where eqn started */
int	svargc;
char	**svargv;
int	eht[100];
int	ebase[100];
int	lfont[100];
int	rfont[100];
int	eqnreg;	/* register where final string appears */
int	eqnht;	/* inal height of equation */
int	lefteq	= '\0';	/* left in-line delimiter */
int	righteq	= '\0';	/* right in-line delimiter */
int	lastchar;	/* last character read by lex */
int	markline	= 0;	/* 1 if this EQ/EN contains mark or lineup */
