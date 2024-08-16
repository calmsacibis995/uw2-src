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

#ident	"@(#)curses:common/lib/xlibcurses/screen/getopt.c	1.2.2.3"
#ident  "$Header: getopt.c 1.2 91/06/26 $"
#include	<stdio.h>
#define	ERR(s, c)	if(opterr){\
	fputs(argv[0], stderr);\
	fputs(s, stderr);\
	fputc(c, stderr);\
	fputc('\n', stderr);}

int	opterr = 1;
int	optind = 1;
char	*optarg;
char	*index();

int
getopt (argc, argv, opts)
	char **argv, *opts;
{
	static int sp = 1;
	char c;
	char *cp;

	if (sp == 1)
		if (optind >= argc ||
		   argv[optind][0] != '-' || argv[optind][1] == '\0')
			return EOF;
		else if (strcmp(argv[optind], "--") == NULL) {
			optind++;
			return EOF;
		}
		else if (strcmp(argv[optind], "-?") == NULL) {
			optind++;
			return '?';
		}
	c = argv[optind][sp];
	if (c == ':' || (cp=index(opts, c)) == NULL) {
		ERR (": illegal option -- ", c);
		if (argv[optind][++sp] == '\0') {
			optind++;
			sp = 1;
		}
		return '?';
	}
	if (*++cp == ':') {
		if (argv[optind][2] != '\0')
			optarg = &argv[optind++][sp+1];
		else if (++optind >= argc) {
			ERR (": option requires an argument -- ", c);
			sp = 1;
			return '?';
		} else
			optarg = argv[optind++];
		sp = 1;
	}
	else if (argv[optind][++sp] == '\0') {
		sp = 1;
		optind++;
	}
	return c;
}
