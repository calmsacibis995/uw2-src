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

#ident	"@(#)ucb:common/ucbcmd/refer/hunt3.c	1.2"
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


#include "refer..c"
#define BSIZ 250

getq(v)
char *v[];
{
	static char buff[BSIZ];
	static int eof = 0;
	extern char *sinput;
	char *p;
	int c, n = 0, las = 0;
	if (eof) return(-1);
	p = buff;
	while ( (c = (sinput ? *sinput++ : getchar()) ) > 0)
	{
		if (c== '\n')
			break;
		if (isalpha(c) || isdigit(c))
		{
			if (las==0)
			{
				v[n++] = p;
				las=1;
			}
			if (las++ <= 6)
				*p++ = c;
		}
		else
		{
			if (las>0)
				*p++ = 0;
			las=0;
		}
	}
	*p=0;
	if (p > buff + BSIZ)
		fprintf(stderr, "query long than %d characters\n", BSIZ);
	assert(p < buff + BSIZ);
	if (sinput==0 && c<= 0) eof=1;
# if D1
	fprintf(stderr, "no. keys %d\n",n);
	for(c=0; c<n; c++)
		fprintf(stderr, "keys X%sX\n", v[c]);
# endif
	return(n);
}
