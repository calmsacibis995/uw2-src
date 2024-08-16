/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)uniq:uniq.c	1.4.3.1"
/*
 * Deal with duplicated lines in a file
 */

#include <stdio.h>
#include <ctype.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>
#include <errno.h>
#include <sys/euc.h>
#include <getwidth.h>
#include <stdlib.h>
#include <limits.h>

#ifndef	LINE_MAX	/* in case of LINE_MAX not defined */
#define	LINE_MAX	2048
#endif

#ifndef isblank
#define	isblank(c)	((c) == ' ' || (c) == '\t')
#endif

int	fields = 0;
int	letters = 0;
int	linec;
char	mode;
int	uniq;
char	*skip();

eucwidth_t	wp;

#define	NEW	1		/* New synopsis */
#define	OLD	-1		/* Obsolescent version */

static char oldinv[] =
	":126:Invalid option for obsolescent synopsis\n";
static char usage[] =
	":127:Usage:\n"
	"\tuniq [-c|-d|-u] [-f fields] [-s chars] [input [output]]\n"
	"\tuniq [-c|-d|-u] [+n] [-n] [input [output]]\n";
static char linetoolong[] =
	":128:Line too long\n";
static char badusg[] =
	":93:Invalid argument to option -%c\n";

void newsynopsis();
int  gline();
void pline();
int  equal();
void printe();


main(argc, argv)
int argc;
char *argv[];
{
	static char b1[LINE_MAX], b2[LINE_MAX];
	FILE *temp;
	int version = 0;
	int sargc = argc;
	char **sargv = argv;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxdfm");
	(void)setlabel("UX:uniq");

	getwidth(&wp);
	wp._eucw2++;
	wp._eucw3++;

	while(argc > 1) {
		if(*argv[1] == '+') {
			version = OLD;
			letters = atoi(&argv[1][1]);
			argc--;
			argv++;
			continue;
		}
		if(*argv[1] == '-') {
			switch (argv[1][1]) {
			case '\0':	/* stdin */
				break;

			case '-':	/* end of options */
				argc--; argv++;
				break;

			case 'c':
			case 'd':
			case 'u':
				mode = argv[1][1];
				argc--; argv++;
				continue;

			default:
				if (isdigit(argv[1][1])) {
					fields = atoi(&argv[1][1]);
					version = OLD;
					argc--; argv++;
					continue;
				} else {
					if (version == OLD) {
						pfmt(stderr, MM_ERROR,
							oldinv);
						exit(1);
					} else
						version = NEW;
				}
			}
			break;
		}
		break;
	}
	if (version == NEW)
		newsynopsis(sargc, sargv);
	else {
		if (argc > 1)
			if (strcmp(argv[1], "-") != 0) {
				if ( (temp = fopen(argv[1], "r")) == NULL)
					printe(":3:Cannot open %s: %s\n", argv[1],
						strerror(errno));
				else {  (void) fclose(temp);
					(void) freopen(argv[1], "r", stdin);
				     }
			}
		if(argc > 2 && freopen(argv[2], "w", stdout) == NULL)
			printe(":12:Cannot create %s: %s\n", argv[2],
				strerror(errno));
	}

	if(gline(b1))
		exit(0);
	for(;;) {
		linec++;
		if(gline(b2)) {
			pline(b1);
			exit(0);
		}
		if(!equal(b1, b2)) {
			pline(b1);
			linec = 0;
			do {
				linec++;
				if(gline(b1)) {
					pline(b2);
					exit(0);
				}
			} while(equal(b1, b2));
			pline(b2);
			linec = 0;
		}
	}
}

int
gline(buf)
register char buf[];
{
	register c;
	register char *bp = buf;

	while((c = getchar()) != '\n') {
		if(c == EOF)
			return(1);
		if (bp >= buf + (LINE_MAX - 1)) {
			pfmt(stderr, MM_ERROR, linetoolong);
			exit(1);
		}
		*bp++ = c;
	}
	*bp = 0;
	return(0);
}

void
pline(buf)
register char buf[];
{

	switch(mode) {

	case 'u':
		if(uniq) {
			uniq = 0;
			return;
		}
		break;

	case 'd':
		if(uniq) break;
		return;

	case 'c':
		(void) printf("%d ", linec);
	}
	uniq = 0;
	(void) fputs(buf, stdout);
	(void) putchar('\n');
}

int
equal(b1, b2)
register char b1[], b2[];
{
	register char c;

	b1 = skip(b1);
	b2 = skip(b2);
	while((c = *b1++) != 0)
		if(c != *b2++) return(0);
	if(*b2 != 0)
		return(0);
	uniq++;
	return(1);
}

char *
skip(s)
register char *s;
{
	register nf, nl;
	register int i;
	register int c;

	nf = nl = 0;
	while(nf++ < fields) {
		while( isblank(*s) )
			s++;
		while( !isblank(*s) && *s != 0 )
			s++;
	}
	while(nl++ < letters && (c = *s&0xff) != 0)
	{
		if (!wp._multibyte || ISASCII(c))
			s++;
		else {
			if (ISSET2(c)) {
				i = wp._eucw2;
			} else if (ISSET3(c)) {
				i = wp._eucw3;
			} else if (c < 0240) {
				i = 1;
			} else {
				i = wp._eucw1;
			}
				s += i;
		}
	}
	return(s);
}

void
printe(p,s, s2)
char *p,*s, *s2;
{
	pfmt(stderr, MM_ERROR, p, s, s2);
	exit(1);
}

void
newsynopsis(argc, argv)
char *argv[];
{
	extern char *optarg;
	extern int optind;
	int c;
	char *cp;
	FILE *temp;

	while ((c = getopt(argc, argv, "cduf:s:")) != EOF)
		switch (c) {
		case 'c':
		case 'd':
		case 'u':
			mode = c;
			break;

		case 'f':
			fields = (int) strtol(optarg, &cp, 10);
			if (fields < 0 || *cp != 0) {
				pfmt(stderr, MM_ERROR, badusg, c);
				goto pusage;
			}
			break;

		case 's':
			letters = (int) strtol(optarg, &cp, 10);
			if (letters < 0 || *cp != 0) {
				pfmt(stderr, MM_ERROR, badusg, c);
				goto pusage;
			}
			break;

		case '?':
		pusage:
			pfmt(stderr, MM_ACTION, usage);
			exit(1);
		}

	if (argc - optind >= 1) {
		if (strcmp(argv[optind], "-") != 0) {
			if ( (temp = fopen(argv[optind], "r")) == NULL)
				printe(":3:Cannot open %s: %s\n",
					argv[optind], strerror(errno));
			else {  (void) fclose(temp);
				(void) freopen(argv[optind], "r", stdin);
			     }
		}
	}

	if(argc - optind > 1
		&& freopen(argv[optind+1], "w", stdout) == NULL)
		printe(":12:Cannot create %s: %s\n", argv[optind+1],
			strerror(errno));
}
