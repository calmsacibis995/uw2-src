/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/installf/main.c	1.13.11.7"
#ident  "$Header: main.c 1.2 91/06/27 $"

#include <stdio.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/errno.h>
#include <pkginfo.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pkgstrct.h>
#include <pkglocs.h>
#include "install.h"

#include <locale.h>
#include <pfmt.h>

extern char	*optarg, *errstr;
extern char	dbst;
extern int	optind;
extern int	errno;

extern void	*calloc();
extern int	pkgnmchk(),
		pkgdbmerg(),
		ocfile(),
		swapcfile(),
		averify(),
		installf(),
		dofinal(),
		unlink(),
		getopt();
extern long	atol();
extern void	progerr(),
		logerr(),
		removef(),
		exit();
extern char	*mktemp(), 
		*getenv(), 
		*pkgparam(), 
		*fpkginst();
extern unsigned long
		umask();

#define BASEDIR	"/BASEDIR/"

#define INSTALF	(*prog == 'i')
#define REMOVEF	(*prog == 'r')

#define ERR_CLASSLONG	":21:classname argument too long"
#define ERR_CLASSCHAR	":22:bad character in classname"
#define ERR_INVAL	":23:package instance <%s> is invalid"
#define ERR_NOTINST	":24:package instance <%s> is not installed"
#define ERR_MERG	":25:unable to merge contents file"

char	*prog;
char	*classname = NULL;

struct cfent
	**eptlist;
char	*pkginst;
int	eptnum,
	warnflag,
	nointeract,
	nosetuid, 
	nocnflct;

void	quit(),
	usage();

main(argc,argv)
int	argc;
char	**argv;
{
	FILE	*mapfp, *tmpfp;
	struct mergstat *mstat;
	char	*pt;
	int	c, n, dbchg;
	int	fflag = 0;
	char	*label;

	(void) signal(SIGHUP, exit);
	(void) signal(SIGINT, exit);
	(void) signal(SIGQUIT, exit);

	(void) umask(0022);
	prog = strrchr(argv[0], '/');
	if(!prog++)
		prog = argv[0];

	label = (char *) malloc(strlen(prog)+4);
	(void) sprintf(label, "UX:%s", prog);
	(void) setlocale(LC_ALL, "");
	(void) setcat("uxpkgtools");
	(void) setlabel(label);

	while ((c = getopt(argc,argv,"c:f?")) != EOF) {
		switch(c) {
		  case 'f':
			fflag++;
			break;

		  case 'c':
			classname = optarg;
			/* validate that classname is acceptable */
			if(strlen(classname) > (size_t)CLSSIZ) {
				progerr(ERR_CLASSLONG);
				exit(1);
			}
			for(pt=classname; *pt; pt++) {
				if(!isalpha(*pt) && !isdigit(*pt)) {
					progerr(ERR_CLASSCHAR);
					exit(1);
				}
			}
			break;

		  default:
			usage();
		}
	}

	pkginst = argv[optind++];
	if(fflag) {
		/* installf and removef must only have pkginst */
		if(optind != argc)
			usage();
	} else {
		/* installf and removef must have at minimum
		 * pkginst & pathname specified on command line 
		 */
		if(optind >= argc)
			usage();
	}
	if(REMOVEF) {
		if(classname)
			usage();
	}
	if(pkgnmchk(pkginst, "all", 0)) {
		progerr(ERR_INVAL, pkginst);
		exit(1);
	}
	if(fpkginst(pkginst, NULL, NULL) == NULL) {
		progerr(ERR_NOTINST, pkginst);
		exit(1);
	}

	if(ocfile(&mapfp, &tmpfp))
		exit(1);

	if(fflag)
		dbchg = dofinal(mapfp, tmpfp, REMOVEF, classname);
	else {
		if(INSTALF) {
			dbst = '+';
			if(installf(argc-optind, &argv[optind]))
				quit(1);
		} else {
			dbst = '-';
			removef(argc-optind, &argv[optind]);
		}

		/*
		 * alloc an array to hold information about how each
		 * entry in memory matches with information already
		 * stored in the "contents" file
		 */
		mstat = (struct mergstat *) calloc((unsigned int)eptnum, 
			sizeof(struct mergstat));

		dbchg = pkgdbmerg(mapfp, tmpfp, eptlist, mstat, 0);
		if(dbchg < 0) {
			progerr(ERR_MERG);
			quit(99);
		}
	}
	(void) fclose(mapfp);
	if(dbchg) {
		if(swapcfile(tmpfp, pkginst))
			quit(99);
	}

	if(REMOVEF && !fflag) {
		for(n=0; eptlist[n]; n++) {
			if(!mstat[n].shared)
				(void) printf("%s\n", eptlist[n]->path);
		}
	} else if(INSTALF && !fflag) {
		for(n=0; eptlist[n]; n++) {
			if(strchr("dxcbp", eptlist[n]->ftype)) {
				(void) averify(1, &eptlist[n]->ftype, 
				   eptlist[n]->path, &eptlist[n]->ainfo, 
				   mstat[n].shared);
			}
		}
	}
	return(warnflag ? 1 : 0);
}

void
quit(n)
int n;
{
	exit(n);
}

void
usage()
{
	(void) pfmt(stderr, MM_ACTION, ":26:usage:\n");
	if(REMOVEF) {
		(void) pfmt(stderr, MM_NOSTD,
			":27:\t%s pkginst path [path ...]\n", prog);
		(void) pfmt(stderr, MM_NOSTD, ":28:\t%s -f pkginst\n", prog);
	} else {
		(void) pfmt(stderr,  MM_NOSTD,
			":29:\t%s [-c class] <pkginst> <path>\n", prog);
		(void) pfmt(stderr, MM_NOSTD,
			":30:\t%s [-c class] <pkginst> <path> <specs>\n", prog);
		(void) pfmt(stderr, MM_NOSTD,
			":31:\t   where <specs> may be defined as:\n");
		(void) pfmt(stderr, MM_NOSTD, ":32:\t\tf <mode> <owner> <group> [<mac> <fixed> <inherited>]\n"); 
		(void) pfmt(stderr, MM_NOSTD, ":33:\t\tv <mode> <owner> <group> [<mac> <fixed> <inherited>]\n"); 
		(void) pfmt(stderr, MM_NOSTD, ":34:\t\te <mode> <owner> <group> [<mac> <fixed> <inherited>]\n"); 
		(void) pfmt(stderr, MM_NOSTD, ":35:\t\td <mode> <owner> <group> [<mac> <fixed> <inherited>]\n"); 
		(void) pfmt(stderr, MM_NOSTD, ":36:\t\tx <mode> <owner> <group> [<mac> <fixed> <inherited>]\n"); 
		(void) pfmt(stderr, MM_NOSTD, ":37:\t\tp <mode> <owner> <group> [<mac> <fixed> <inherited>]\n"); 
		(void) pfmt(stderr, MM_NOSTD,
			":38:\t\tc <major> <minor> <mode> <owner> <group> [<mac> <fixed> <inherited>]\n");
		(void) pfmt(stderr, MM_NOSTD,
			":39:\t\tb <major> <minor> <mode> <owner> <group> [<mac> <fixed> <inherited>]\n");
		(void) pfmt(stderr, MM_NOSTD, ":771:\t%s [-c class] pkginst path1=path2 [l|s]\n", prog);
		(void) pfmt(stderr, MM_NOSTD, ":42:\t%s [-c class] -f pkginst\n", prog);
	}
	exit(1);
}
