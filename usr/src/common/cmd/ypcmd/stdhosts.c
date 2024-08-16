/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ypcmd:stdhosts.c	1.1"
#ident  "$Header: $"

#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <sys/types.h>
#include <netinet/in.h>

/*
 * Filter to convert addresses in /etc/hosts file to standard form
 */

main(argc, argv)
	char **argv;
{
	char line[256];
	char adr[256];
	char *any(), *trailer;
	extern char *inet_ntoa();
	extern u_long inet_addr();
	FILE *fp;

        (void)setlocale(LC_ALL,"");
        (void)setcat("uxstdhosts");
        (void)setlabel("UX:stdhosts");
 
	if (argc > 1) {
		fp = fopen(argv[1], "r");
		if (fp == NULL) {
			pfmt(stderr, MM_STD, ":1:can't open %s\n", argv[1]);
			exit(1);
		}
	} else
		fp = stdin;
	while ( fgets(line, sizeof(line), fp)) {
		struct in_addr in;

		if (line[0] == '#')
			continue;
		if ((trailer = any(line, " \t")) == NULL)
			continue;
		sscanf(line, "%s", adr);
		in.s_addr = inet_addr(adr);
		if (-1 == (int)in.s_addr) {
			pfmt(stderr, MM_STD | MM_WARNING,
				":2:malformed line ignored:\n%s",
					line);
		} else {
			fputs(inet_ntoa(in), stdout);
			fputs(trailer, stdout);
		}
	}
	exit(0);
	/* NOTREACHED */
}

/*
 * scans cp, looking for a match with any character
 * in match.  Returns pointer to place in cp that matched
 * (or NULL if no match)
 */
static char *
any(cp, match)
	register char *cp;
	char *match;
{
	register char *mp, c;

	while (c = *cp) {
		for (mp = match; *mp; mp++)
			if (*mp == c)
				return (cp);
		cp++;
	}
	return (NULL);
}
