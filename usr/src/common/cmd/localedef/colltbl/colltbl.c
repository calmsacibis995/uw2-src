/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)localedef:common/cmd/localedef/colltbl/colltbl.c	1.1.5.2"
#include <stdio.h>
#include "colltbl.h"
#define MAXPATH		32

/* Global Variables */
int	Status = 0, Lineno = 1;
int	regexp_flag;
char	*Cmd, *Infile;
char	codeset[50];

main(argc, argv)
int	argc;
char	**argv;
{
	extern int	optind;
	int		c;

	/*  Get name of command  */
	Cmd = argv[0];

	/*  Get command line options  */
	while ((c = getopt(argc, argv, "r")) != EOF) {
		switch (c) {
		case 'r':
#ifdef REGEXP
			regexp_flag++;
			break;
#endif
		case '?':
			usage();
			break;
		}
	}

	/*  Get input file argument  */
	switch (argc - optind) {
	case 0:
		Infile = "stdin";
		break;
	case 1:
		if (strcmp(argv[optind], "-") == 0)
			Infile = "stdin";
		else if (freopen((Infile = argv[optind]), "r", stdin) == NULL) {
			error(BAD_OPEN, Infile);
			exit(-1);
		}
		break;
	default:
		usage();
	}

	/*  Run parser  */
	yyparse();

	/* Produce database */
	setdb(codeset);

	exit(Status);
}
