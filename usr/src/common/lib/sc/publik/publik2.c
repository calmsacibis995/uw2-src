/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:publik/publik2.c	3.4" */
/******************************************************************************
*
* C++ Standard Components, Release 3.0.
*
* Copyright (c) 1991, 1992 AT&T and Unix System Laboratories, Inc.
* Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.
*
* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System
* Laboratories, Inc.  The copyright notice above does not evidence
* any actual or intended publication of such source code.
*
******************************************************************************/

#include "publik.h"

#ifdef hpux
extern "C" int getopt(int, char * const [], const char*);
#endif

CXXLexer *lexer;

bool verbose = 0;
bool showModifiers = 0;
bool showMemberDefs = 0;
bool showComments = 1;
bool showWhere = 0;

static void getopts(int argc, char *argv[])
{
	extern char *optarg;
	extern int opterr;
	bool errflg = no;
	opterr = 0;
	int c;
	while ((c=getopt(argc, argv, "clmpv")) != EOF)
	{
		switch (c)
		{
			case 'c':
				showComments = 0;
				break;
			case 'l':
				showWhere = 1;
				break;
			case 'p':
				showModifiers = 1;
				break;
			case 'm':
				showMemberDefs = 1;
				break;
			case 'v':
				verbose = 1;
				break;
			case '?':
				errflg++;
				break;
		}
	}
	if (errflg)
	{
		cerr << "usage: publik [-clmp] file ...\n";
		exit(2);
	}
}



int const bufsize=2048;
char buf[bufsize];

main(int argc, char *argv[])
{
	extern int optind;
	extern char *optarg;

	getopts(argc, argv);

	lexer = new CXXLexer;
	lexer->verbose(verbose);
	if (showComments)
		lexer->commentIsToken();

	int nErrors = 0;
	if (optind == argc)
	{
		lexer->attach(&cin, "stdin");
		nErrors += parse();
	}
	else 
	{
		for (; optind < argc; optind++) 
		{
			ifstream in(argv[optind], ios::nocreate);
			if (in)
			{
				in.setbuf(buf, bufsize);
				lexer->attach(&in, argv[optind]);
				nErrors += parse();
			}
			else 
			{
				cerr << "publik: can't open " << argv[optind] << endl;
			}
		}			
	}
	return(nErrors);
}





