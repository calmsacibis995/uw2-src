/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bnu:uuname.c	2.9.4.6"
#ident "$Header: uuname.c 1.1 91/02/28 $"

#include "uucp.h"
#include <ctype.h>
#include <pfmt.h>
#include <locale.h>
 
/*
 * returns a list of remote systems accessible to uucico
 * option:
 *	-l	-> returns only the local system name.
 *	-c	-> print remote systems accessible to cu
 */
main(argc,argv)
int argc;
char **argv;
{
	int c, lflg = 0, cflg = 0;
	char s[BUFSIZ], prev[BUFSIZ], name[BUFSIZ];
	extern void setservice();
	extern int sysaccess(), getsysline();

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxbnu");
	(void)setlabel("UX:uuname");

	while ( (c = getopt(argc, argv, "lc")) != EOF )
		switch(c) {
		case 'c':
			cflg++;
			break;
		case 'l':
			lflg++;
			break;
		default:
			(void) pfmt(stderr, MM_ACTION, ":8:Usage: uuname [-l] [-c]\n");
			exit(34);
		}
 
	if (lflg) {
		if ( cflg )
			pfmt(stderr, MM_WARNING,
			":9: -l overrides -c ... -c option ignored\n");
		uucpname(name);

		puts(name);
		exit(0);
	}
 
	if ( cflg )
		setservice("cu");
	else
		setservice("uucico");


	if ( sysaccess(EACCESS_SYSTEMS) != 0 ) {
		pfmt(stderr, MM_ERROR,
			":10:cannot access Systems file\n");
		exit(35);
	}

	while ( getsysline(s, sizeof(s)) ) {
		if((s[0] == '#') || (s[0] == ' ') || (s[0] == '\t') || 
		    (s[0] == '\n'))
			continue;
		(void) sscanf(s, "%s", name);
		if (EQUALS(name, prev))
		    continue;
		puts(name);
		(void) strcpy(prev, name);
	}
	exit(0);
}

/* small, private copies of assert(), logent(), */
/* cleanup() so we can use routines in sysfiles.c */

/*ARGSUSED*/
void
assert(s1, s2, i1, file, line)
char *s1, *s2, *file; int i1, line;
{	
	(void)fprintf(stderr, "uuname: %s %s %d\n", s2, s1, i1);
	return;
}

/*ARGSUSED*/
void logent(s1, s2)
char *s1, *s2;
{}

void cleanup(code) 	{ exit(code);	}
