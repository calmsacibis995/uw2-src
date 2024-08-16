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

#ident	"@(#)ucb:common/ucbcmd/users/users.c	1.3"
#ident	"$Header: $"
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Copyright (c) 1983, 1984, 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
 * All rights reserved.
 */


/*
 * users
 */
char	*malloc();

#include <stdio.h>
#include <sys/types.h>
#include <utmp.h>


#define NMAX sizeof(utmp.ut_name)

struct utmp utmp;

main(argc, argv)
char **argv;
{
	register char *tp, *s;
	register FILE *fi;
	int numusers = 0;

	s = UTMP_FILE;
	if(argc == 2)
		s = argv[1];
	if ((fi = fopen(s, "r")) == NULL) {
		perror(s);
		exit(1);
	}
	while (fread((char *)&utmp, sizeof(utmp), 1, fi) == 1) {
		if(utmp.ut_name[0] == '\0')
			continue;
		if (utmp.ut_type != USER_PROCESS)
			continue;
		if (numusers++ < 200)
			putline();
		else {
			fprintf(stderr,"\n**** The number of users exceeds 200.  Only 200 are listed. ****\n\n");
			break;
		}
	}
	summary();
	exit(0);
}

char	*names[200];
char	**namp = names;
putline()
{
	char temp[NMAX+1];
	strncpy(temp, utmp.ut_name, NMAX);
	temp[NMAX] = 0;
	*namp = malloc(strlen(temp) + 1);
	strcpy(*namp++, temp);
}

scmp(p, q)
char **p, **q;
{
	return(strcmp(*p, *q));
}
summary()
{
	register char **p;

	qsort(names, namp - names, sizeof names[0], scmp);
	for (p=names; p < namp; p++) {
		if (p != names)
			putchar(' ');
		fputs(*p, stdout);
	}
	if (namp != names)		/* at least one user */
		putchar('\n');
}
