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

#ident	"@(#)ucb:common/ucbcmd/tbl/ts.c	1.2"
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
  

 /* ts.c: minor string processing subroutines */
match (s1, s2)
	char *s1, *s2;
{
	while (*s1 == *s2)
		if (*s1++ == '\0')
			return(1);
		else
			s2++;
	return(0);
}
prefix(small, big)
	char *small, *big;
{
int c;
while ((c= *small++) == *big++)
	if (c==0) return(1);
return(c==0);
}
letter (ch)
	{
	if (ch >= 'a' && ch <= 'z')
		return(1);
	if (ch >= 'A' && ch <= 'Z')
		return(1);
	return(0);
	}
numb(str)
	char *str;
	{
	/* convert to integer */
	int k;
	for (k=0; *str >= '0' && *str <= '9'; str++)
		k = k*10 + *str - '0';
	return(k);
	}
digit(x)
	{
	return(x>= '0' && x<= '9');
	}
max(a,b)
{
return( a>b ? a : b);
}
tcopy (s,t)
	char *s, *t;
{
	while (*s++ = *t++);
}
