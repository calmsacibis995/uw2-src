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

#ident	"@(#)ucb:common/ucbcmd/refer/deliv2.c	1.2"
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



#include <stdio.h>

hash (s)
char *s;
{
	int c, n;
	for(n=0; c= *s; s++)
		n += (c*n+ c << (n%4));
	return(n>0 ? n : -n);
}

err (s, a)
char *s;
{
	fprintf(stderr, "Error: ");
	fprintf(stderr, s, a);
	putc('\n', stderr);
	exit(1);
}

prefix(t, s)
char *t, *s;
{
	int c;

	while ((c= *t++) == *s++)
		if (c==0) return(1);
	return(c==0 ? 1: 0);
}

char *
mindex(s, c)
char *s;
{
	register char *p;
	for( p=s; *p; p++)
		if (*p ==c)
			return(p);
	return(0);
}

zalloc(m,n)
{
	char *calloc();
	int t;
# if D1
	fprintf(stderr, "calling calloc for %d*%d bytes\n",m,n);
# endif
	t = (int) calloc(m,n);
# if D1
	fprintf(stderr, "calloc returned %o\n", t);
# endif
	return(t);
}
