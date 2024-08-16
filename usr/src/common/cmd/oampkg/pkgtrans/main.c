/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkgtrans/main.c	1.4.11.5"
#ident  "$Header: $"

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <pkgtrans.h>

#include <locale.h>
#include <pfmt.h>

extern char	*optarg;
extern int	optind;
extern void	exit(), progerr();
extern int	getopt(), pkgtrans();

char	*prog;
char	*pkginst;

static int	options;
static void	usage(), quit(), trap();

main(argc, argv)
char	*argv[];
int	argc;
{
	int	c;
	void	(*func)();

	prog = strrchr(argv[0], '/');
	if(!prog++)
		prog = argv[0];

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxpkgtools");
	(void) setlabel("UX:pkgtrans");

	while ((c = getopt(argc,argv,"snio?")) != EOF) {
		switch(c) {
		  case 'n':
			options |= PT_RENAME;
			break;

		  case 'i':
			options |= PT_INFO_ONLY;
			break;

		  case 'o':
			options |= PT_OVERWRITE;
			break;

		  case 's':
			options |= PT_ODTSTREAM;
			break;

		  default:
			usage();
		}
	}
	func = signal(SIGINT, trap);
	if(func != SIG_DFL)
		(void) signal(SIGINT, func);
	(void) signal(SIGHUP, trap);
	(void) signal(SIGQUIT, trap);
	(void) signal(SIGTERM, trap);
	(void) signal(SIGPIPE, trap);
	(void) signal(SIGPWR, trap);

	if((argc-optind) < 2)
		usage();

	quit(pkgtrans(argv[optind], argv[optind+1], &argv[optind+2], options));
	/*NOTREACHED*/
}

static void
quit(status) 
int	status;
{
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGHUP, SIG_IGN);
	(void)ds_close(1);
	exit(status);
}

static void
trap(signo)
int signo;
{
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGHUP, SIG_IGN);

	if(signo == SIGINT) {
		progerr(":592:\n%s aborted at user request.\n", prog);
		quit(3);
	}
	progerr(":593:\n%s aborted by signal %d\n", prog, signo);
	quit(1);
}

static void
usage()
{
	(void) pfmt(stderr, MM_ACTION,
		":594:usage: %s [-inos] srcdev dstdev [pkg [pkg...]]\n", prog);
	exit(1);
}
