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

#ident	"@(#)ucb:common/ucbcmd/refer/mkey1.c	1.2"
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

extern char *comname;	/* "/usr/ucblib/reftools/eign" */
int wholefile = 0;
int keycount = 100;
int labels = 1;
int minlen = 3;
extern int comcount;
char *iglist = "XYZ#";

main (argc,argv)
char *argv[];
{
	/* this program expects as its arguments a list of
	 * files and generates a set of lines of the form
	 *	filename:byte-add,length (tab) key1 key2 key3
	 * where the byte addresses give the position within
	 * the file and the keys are the strings off the lines
	 * which are alphabetic, first six characters only.
	 */

	int i;
	char *name, qn[200];
	char *inlist = 0;

	FILE *f, *ff;

	while (argc>1 && argv[1][0] == '-')
	{
		switch(argv[1][1])
		{
		case 'c':
			comname = argv[2];
			argv++; 
			argc--;
			break;
		case 'w':
			wholefile = 1;  
			break;
		case 'f':
			inlist = argv[2];
			argv++; 
			argc--;
			break;
		case 'i':
			iglist = argv[2];
			argv++; 
			argc--;
			break;
		case 'l':
			minlen = atoi(argv[1]+2);
			if (minlen<=0) minlen=3;
			break;
		case 'n': /* number of common words to use */
			comcount = atoi(argv[1]+2);
			break;
		case 'k': /* number  of keys per file max */
			keycount = atoi(argv[1]+2);
			break;
		case 's': /* suppress labels, search only */
			labels = 0;
			break;
		}
		argc--; 
		argv++;
	}
	if (inlist)
	{
		ff = fopen(inlist, "r");
		while (fgets(qn, 200, ff))
		{
			trimnl(qn);
			f = fopen (qn, "r");
			if (f!=NULL)
				dofile(f, qn);
			else
				fprintf(stderr, "Can't read %s\n",qn);
		}
	}
	else
		if (argc<=1)
			dofile(stdin, "");
		else
			for(i=1; i<argc; i++)
			{
				f = fopen(name=argv[i], "r");
				if (f==NULL)
					err("No file %s",name);
				else
					dofile(f, name);
			}
	exit(0);
	/* NOTREACHED */
}
