/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)valtools:ckkeywd.c	1.2.6.2"
/* This file is not fully internationalized. */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include "usage.h"
#include <locale.h>							
#include <ctype.h>							
#include <pfmt.h>							

extern int	optind, ckquit, ckwidth;
extern char	*optarg;

extern long	atol();
extern void	exit();
extern int	getopt(), ckkeywd();
	
#define BADPID	(-2)

char	*prog,
	*deflt,
	*prompt,
	*error,
	*help;
int	kpid = BADPID;
int	signo; 

char	*keyword[128];
int	nkeyword = 0;


void
usage()
{
	(void) fprintf(stderr, "usage: %s [options] keyword [...]\n", prog);
	(void) pfmt(stderr, MM_NOSTD, OPTMESG);				
	(void) pfmt(stderr, MM_NOSTD, STDOPTS);				
	exit(1);
}

main(argc, argv)
int argc;
char *argv[];
{
	int	c, n;
	char	strval[256];

	(void)setlocale(LC_ALL,"");					
	(void)setcat("uxvaltools");					
	(void)setlabel("UX:ckkeywd");					

	prog = strrchr(argv[0], '/');
	if(!prog++)
		prog = argv[0];

	while((c=getopt(argc, argv, "d:p:e:h:k:s:QW:?")) != EOF) {
		switch(c) {
		  case 'Q':
			ckquit = 0;
			break;

		  case 'W':
			ckwidth = atol(optarg);
			if(ckwidth < 0) {
				progerr(":8:negative display width specified");
				exit(1);
			}
			break;

		  case 'd':
			deflt = optarg;
			break;

		  case 'p':
			prompt = optarg;
			break;

		  case 'e':
			error = optarg;
			break;

		  case 'h':
			help = optarg;
			break;

		  case 'k':
			kpid = atol(optarg);
			break;
			
		  case 's':
			signo = atol(optarg);
			break;

		  default:
			usage();
		}
	}

	if(signo) {
		if(kpid == BADPID)
			usage();
	} else
		signo = SIGTERM;

	if(optind >= argc)
		usage(); /* must be at least one keyword */

	while(optind < argc)
		keyword[nkeyword++] = argv[optind++];
	keyword[nkeyword] = NULL;

	n = ckkeywd(strval, keyword, deflt, error, help, prompt);
	if(n == 3) {
		if(kpid > -2)
			(void) kill(kpid, signo);
		(void) puts("q");
	} else if(n == 0)
		(void) fputs(strval,stdout);
	exit(n);
}
