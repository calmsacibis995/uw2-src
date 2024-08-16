/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:fs/fsippsrc/fsipp.c	3.6"	*/
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

#include "fsipp.h"
#if 0	/* don't know how to locate sgs.h in the makefile for SC;
	   if could, this should be an #ifdef for UnixWare */
#include "sgs.h"
#else
#define CPLUS_PKG "C++ Compilation System"
#define CPLUS_REL " "	/* normally, release, date, and load */
#endif

#ifdef hpux
extern "C" int getopt(int, char * const [], const char*);
#endif

CXXLexer *lexer;
bool alternateLineDirectiveFormat = 0;

static bool verbose = 0;
static bool Qflag = 0;
static ofstream ofile;

/* TBD: internationalisation */

static void getopts(int argc, char *argv[])
{
	bool errflg = no;
	opterr = 0;
	int c;
	while ((c=getopt(argc, argv, "Lo:Q:Vv")) != EOF)
	{
		switch (c)
		{
			case 'L':
				alternateLineDirectiveFormat = 1;
				break;
			case 'o':
				// write output to file instead of stdout
				ofile.open(optarg, ios::out);
				if (ofile)
					cout = ofile;
				else {
					cerr << "fsipp: cannot open " << optarg << endl;
					errflg++;
				}
				break;
			case 'Q':
				// add tool identifying information to output
			        if (strcmp(optarg, "y") == 0) 
					Qflag = 1;
				else if (strcmp(optarg, "n") == 0)
					;	// default, do nothing
				else
					cerr << "fsipp: invalid -Q option " << optarg << endl;
				break;
			case 'V':
				// print version information
				cerr << "fsipp: " << CPLUS_PKG  << " " << CPLUS_REL << endl;
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
		cerr << "usage: fsipp [-Lo:Q:Vv] file ...\n";
		exit(2);
	}
}



int const bufsize=2048;
char buf[bufsize];

main(int argc, char *argv[])
{
	getopts(argc, argv);

	if (Qflag)
		cout << "#ident \"fsipp: " << CPLUS_REL << "\"" << endl;

	lexer = new CXXLexer;
	lexer->verbose(verbose);

	int nErrors = 0;
	if (optind == argc)
	{
		lexer->attach(cin);
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
				lexer->attach(in, argv[optind]);
				nErrors += parse();
			}
			else 
			{
				cerr << "fsipp: can't open " << argv[optind] << endl;
			}
		}			
	}

	if (ofile)
		ofile.close();

	return(nErrors);
}

