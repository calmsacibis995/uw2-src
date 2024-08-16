/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)wc:wc.c	1.5.4.4"
/*
**	wc -- word and line count
*/

#include	<stdio.h>
#include	<ctype.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<errno.h>
#include	<string.h>
#include	<stdlib.h>

unsigned char	b[BUFSIZ];

FILE *fptr = stdin;
long	wordct;
long	twordct;
long	linect;
long	tlinect;
long	charct;
long	tcharct;

static int lflg;
static int wflg;
static int cflg;
#ifdef __STDC__
static void wcp(long, long, long);
#else
static void wcp();
#endif

main(argc,argv)
char **argv;
{
	register unsigned char *p1, *p2;
	register unsigned int c;
	register int optc;
	int	i, token;
	int	status = 0;
	int	errflg = 0;

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore");
	(void)setlabel("UX:wc");

	while ((optc=getopt(argc,argv,"lwc")) != EOF ) {
		switch (optc) {
		case 'l':
			lflg++;
			break;
		case 'w':
			wflg++;
			break;
		case 'c':
			cflg++;
			break;
		case '?':
		default:
			errflg++;
			break;
		}
	}

	if(errflg) {
		pfmt(stderr, MM_ACTION,
				":552:Usage: wc [-clw] [name ...]\n");
		exit(2);
	}
	if((lflg | wflg | cflg) == 0)
		lflg = wflg = cflg = 1;

	argc -= optind;
	argv = &argv[optind];

	i = 0;
	do {
		if(argc >= 1 && (fptr=fopen(argv[i], "r")) == NULL) {
			pfmt(stderr, MM_ERROR, ":92:Cannot open %s: %s\n",
				argv[i], strerror(errno));
			status = 2;
			continue;
		}
		p1 = p2 = b;
		linect = 0;
		wordct = 0;
		charct = 0;
		token = 0;
		for(;;) {
			if(p1 >= p2) {
				p1 = b;

				if (feof(fptr) || ferror(fptr) ||
					(c = fread(p1, 1, BUFSIZ, fptr)) <= 0)
					break;

				charct += c;
				p2 = p1+c;
			}
			c = *p1++;
			if(isgraph(c)) {
				if(!token) {
					wordct++;
					token++;
				}
				continue;
			}
			if(c=='\n')
				linect++;
			if(!isspace(c))
				continue;
			token = 0;
		}

		/* print lines, words, chars */
		wcp(charct, wordct, linect);
		if(argc >= 1) {
			printf(" %s\n", argv[i]);
		}
		else
			printf("\n");
#if 0
		if (ferror(fptr))
			status = 2;
#endif 
		fclose(fptr);
		tlinect += linect;
		twordct += wordct;
		tcharct += charct;
	} while(++i<argc);
	if(argc >= 2) {
		wcp(tcharct, twordct, tlinect);
		pfmt(stdout, MM_NOSTD, ":551: total\n");

	}
	exit(status);
	/*NOTREACHED*/
}

static void
#ifdef __STDC__
wcp(long charct, long wordct, long linect)
#else
wcp(charct, wordct, linect)
long charct; long wordct; long linect;
#endif 
{
#ifdef lint
	char *format ="%dl";
#else
	char *format ="%d";
#endif

	if(lflg)

		(void) printf(format, linect);

	if(wflg)
	{
		if(lflg)
			(void) putchar(' ');
		(void) printf(format , wordct);
	}
	if(cflg)
	{
		if(lflg || wflg)
			(void) putchar(' ');
		(void) printf(format , charct);
	}
}
