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

#ident	"@(#)ucb:common/ucbcmd/refer/lookbib.c	1.2"
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
#include <ctype.h>

main(argc, argv)	/* look in biblio for record matching keywords */
int argc;
char **argv;
{
	FILE *hfp, *fopen(), *popen();
	char s[BUFSIZ], hunt[64];

	if (argc == 1 || argc > 2) {
		fputs("Usage:  lookbib database\n",
			stderr);
		fputs("\tfinds citations specified on standard input\n",
			stderr);
		exit(1);
	}
	sprintf(s, "%s.ia", argv[1]);
	if (access(s, 0) == -1) {
		sprintf (s, "%s", argv[1]);
		if (access(s, 0) == -1) {
			perror(s);
			fprintf(stderr, "\tNeither index file %s.ia ", s);
			fprintf(stderr, "nor reference file %s found\n", s);
			exit(1);
		}
	}
	sprintf(hunt, "/usr/ucblib/reftools/hunt %s", argv[1]);
	if (isatty(fileno(stdin))) {
		fprintf(stderr, "Instructions? ");
		fgets(s, BUFSIZ, stdin);
		if (*s == 'y')
			instruct();
	}
   again:
	fprintf(stderr, "> ");
	if (fgets(s, BUFSIZ, stdin)) {
		if (*s == '\n')
			goto again;
		if (strlen(s) <= 3)
			goto again;
		if ((hfp = popen(hunt, "w")) == NULL) {
			perror("lookbib: /usr/ucblib/reftools/hunt");
			exit(1);
		}
		map_lower(s);
		fputs(s, hfp);
		pclose(hfp);
		goto again;
	}
	fprintf(stderr, "EOT\n");
	exit(0);
	/* NOTREACHED */
}

map_lower(s)		/* map string s to lower case */
char *s;
{
	for ( ; *s; ++s)
		if (isupper(*s))
			*s = tolower(*s);
}

instruct()
{
	fputs("\nType keywords (such as author and date) after the > prompt.\n",
		stderr);
	fputs("References with those keywords are printed if they exist;\n",
		stderr);
	fputs("\tif nothing matches you are given another prompt.\n",
		stderr);
	fputs("To quit lookbib, press CTRL-d after the > prompt.\n\n",
		stderr);
}
