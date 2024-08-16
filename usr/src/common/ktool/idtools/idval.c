/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/idtools/idval.c	1.5"
#ident	"$Header:"

/* This program supports the idtune command. The idtune shell script
 * used to rely on 'expr' to compare the new tunable parameter value
 * against its lower and upper bound. Both decimal and hexadecimal
 * values can be specified but expr would give incorrect return value 
 * when comparing a decimal with a hex value. Therefore, this supporting
 * C program was written to do the appropriate conversions.
 */
#include <stdio.h>

#include <locale.h>
#include <pfmt.h>

#define	USAGE	":260:Usage: %s -l | -g value1 value2\n"

char gflag = 0;		/* -g flag specified, is first value greater than second? */
char lflag = 0;		/* -l flag specified, is first value less than second? */
char value1[128];	/* char string for the first value */
char value2[128];	/* char string for the second value */

int lessthan();		/* is value1 less than lower bound (value2)? */
int greaterthan();	/* is value1 greater than upper bound (value2)? */

/* exit codes as follows:
 *		1: TRUE		(value1 is greater (or less) than value2.
 *		0: FALSE	(value1 is within upper (or lower) bound.
 *		-1: failure
 */

main(argc, argv)
int argc;
char **argv;
{
	int m;
	int ret;	/* returned value from comparisons */
	long lvalue1,
	     lvalue2;	/* returned values from strtol */
	extern int optind;
	long strtol();  /* does string to long integer conversion */

	umask(022);

        (void) setlocale(LC_ALL, "");
        (void) setcat("uxidtools");
        (void) setlabel("UX:idval");

	while ((m = getopt(argc, argv, "?gl")) != EOF)
		switch(m) {

			case 'g':
				gflag++;
				break;

			case 'l':
				lflag++;
				break;

			case '?':
				pfmt(stderr, MM_ACTION, USAGE, argv[0]);
				exit(1);
		}

		if (lflag && gflag) {
			pfmt(stderr, MM_ERROR, ":81:must have one of -g or -l options\n");
			pfmt(stderr, MM_ACTION, USAGE, argv[0]);
			exit(1);
		}

		if (argc == optind) {
			pfmt(stderr, MM_ERROR, ":82:must specify two values to compare\n");
			pfmt(stderr, MM_ACTION, USAGE, argv[0]);
			exit(1);
		}

		sprintf(value1, "%s", argv[optind]);

		if (*value1 == '\0') {
			pfmt(stderr, MM_ERROR, ":83:must specify two values to compare\n");
			pfmt(stderr, MM_ACTION, USAGE, argv[0]);
			exit(1);
		}

		sprintf(value2, "%s", argv[++optind]);

		if (*value2 == '\0') {
			pfmt(stderr, MM_ERROR, ":83:must specify two values to compare\n");
			pfmt(stderr, MM_ACTION, USAGE, argv[0]);
			exit(1);
		}
		lvalue1 = strtol(value1, (char **)NULL, 0);
		lvalue2 = strtol(value2, (char **)NULL, 0);

		if (lflag)
			ret = lessthan(lvalue1, lvalue2);

		if (gflag)
			ret = greaterthan(lvalue1, lvalue2);
		exit(ret);
}


lessthan(val1, val2)
long val1, val2;
{
	if (val1 < val2)
		return(1);
	else
		return(0);
}


greaterthan(val1, val2)
long val1, val2;
{
	if (val1 > val2)
		return(1);
	else
		return(0);
}
