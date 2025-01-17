/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cmp:cmp.c	1.4.3.1"

/***************************************************************************
 * Command: cmp
 * Inheritable Privileges: P_DACREAD,P_MACREAD
 *       Fixed Privileges: None
 * Notes: compares the difference of two files
 *
 ***************************************************************************/

/*
 *	compare two files
*/

#include	<stdio.h>
#include	<ctype.h>
#include	<locale.h>
#include	<pfmt.h>
#include	<errno.h>
#include	<string.h>
#include	<stdlib.h>

FILE	*file1,*file2;

int	eflg;
int	lflg = 1;

long	line = 1;
long	chr = 0;
long	skip1 = 0;
long	skip2 = 0;

/*
 * Procedure:     main
 *
 * Restrictions:
                 setlocale: 	none
                 fopen: 	none
		 pfmt:		none
 */
main(argc, argv)
char **argv;
{
	register c1, c2;
	extern int optind;
	int opt;


	(void)setlocale(LC_ALL, "");
	(void)setcat("uxcore.abi");
	(void)setlabel("UX:cmp");

	while ((opt = getopt(argc, argv, "ls")) != EOF){
		switch(opt){
		case 's':
			if (lflg != 1)
				usage(1);
			lflg--;
			continue;
		case 'l':
			if (lflg != 1)
				usage(1);
			lflg++;
			continue;
		default:
			usage(0);
		}
	}
	argc -= optind;
	if(argc < 2 || argc > 4)
		usage(1);
	argv += optind;


	if(strcmp(argv[0], "-") == 0)
		file1 = stdin;
	else if((file1 = fopen(argv[0], "r")) == NULL) {
		barg(argv[0]);
	}

	if(strcmp(argv[1], "-") == 0)
		file2 = stdin;
	else if((file2 = fopen(argv[1], "r")) == NULL) {
		barg(argv[1]);
	}

	if (argc>2)
		skip1 = otoi(argv[2]);
	if (argc>3)
		skip2 = otoi(argv[3]);
	while (skip1) {
		if ((c1 = getc(file1)) == EOF) {
			earg(argv[0]);
		}
		skip1--;
	}
	while (skip2) {
		if ((c2 = getc(file2)) == EOF) {
			earg(argv[1]);
		}
		skip2--;
	}

	while(1) {
		chr++;
		c1 = getc(file1);
		c2 = getc(file2);
		if(c1 == c2) {
			if (c1 == '\n')
				line++;
			if(c1 == EOF) {
				if(eflg)
					exit(1);
				exit(0);
			}
			continue;
		}
		if(lflg == 0)
			exit(1);
		if(c1 == EOF)
			earg(argv[0]);
		if(c2 == EOF)
			earg(argv[1]);
		if(lflg == 1) {
			pfmt(stdout, MM_NOSTD,
				":28:%s %s differ: char %ld, line %ld\n",
				argv[0], argv[1], chr, line);
			exit(1);
		}
		eflg = 1;
		printf("%6ld %3o %3o\n", chr, c1, c2);
	}
}

/*
 * Procedure:     otoi
 *
 * Restrictions:
                 isdigit: 	none
*/
otoi(s)
char *s;
{
	long v;
	char *termp;

	if (*s == '0')
		v = strtol(s, &termp, 8);
	else
		v = strtol(s, &termp, 10);

	if ((*termp != '\0') || (v < 0L)) {
		usage(1);
	}
	return(v);
}

/*
 * Procedure:     usage
 *
 * Restrictions:
                 pfmt: 	none
*/
usage(complain)
int complain;
{
	if (complain)
		pfmt(stderr, MM_ERROR, ":8:Incorrect usage\n");
	pfmt(stderr, MM_ACTION, ":1117:Usage: cmp [-l] [-s] filename1 filename2 [skip1 [skip2] ]\n");
	exit(2);
}

/*
 * Procedure:     barg
 *
 * Restrictions:
                 pfmt:		none
                 strerror: 	none
*/
barg(name)
char *name;
{
	if (lflg)
		pfmt(stderr, MM_ERROR, ":4:Cannot open %s: %s\n", name,
			strerror(errno));
	exit(2);
}

/*
 * Procedure:     earg
 *
 * Restrictions:
                 pfmt: 	none
*/
earg(name)
char *name;
{
	if (lflg)
		pfmt(stderr, MM_INFO, ":30:EOF on %s\n", name);
	exit(1);
}
