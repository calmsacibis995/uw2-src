/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkginfo/pkgparam.c	1.7.8.5"
#ident  "$Header: $"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/types.h>
#include <pkgstrct.h>
#include <pkginfo.h>

#include <locale.h>
#include <pfmt.h>

extern char	*optarg, *pkgfile;
extern int	optind, errno;

extern char	*pkgparam();
extern void	exit(),
		progerr();
extern int	getopt(),
		pkgnmchk(),
		pkghead();

#define ERRMESG		":273:unable to locate parameter information for \"%s\""
#define ERRFLT		":274:parsing error in parameter file"
#define ERRINVAL	":275:<%s> is not a valid parameter for this package"

char	*prog;
char	*pkginst;

static char	*device = NULL;
static int	errflg = 0;
static int	vflag = 0;

static void
usage()
{
	(void) pfmt(stderr, MM_ACTION, ":26:usage:\n");
	(void) pfmt(stderr, MM_NOSTD,
		":276:\t%s [-v] [-d device] pkginst [param [param ...]]\n", prog);
	(void) pfmt(stderr, MM_NOSTD,
		":277:\t%s [-v] -f file [param [param ...]]\n", prog);
	exit(1);
}

main(argc, argv)
int argc;
char *argv[];
{
	char *value, *pkginst;
	char *param, parambuf[128];
	int c, paramflg = 0;

	prog = strrchr(argv[0], '/');
	if(!prog++)
		prog = argv[0];

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxpkgtools");
	(void) setlabel("UX:pkgparam");

	while ((c = getopt(argc,argv,"vd:f:?")) != EOF) {
		switch(c) {
		  case 'v':
			vflag++;
			break;

		  case 'f':
			/* -d could specify stream or mountable device */
			pkgfile = optarg;
			break;

		  case 'd':
			/* -d could specify stream or mountable device */
			device = optarg;
			break;

		  default:
		  case '?':
			usage();
		}
	}

	if(pkgfile) {
		if(device)
			usage();
		pkginst = pkgfile;
	} else {
		if((optind+1) > argc)
			usage();

		if(pkghead(device))
			return(1); /* couldn't obtain info about device */
		pkginst = argv[optind++];
		if(pkgnmchk(pkginst, "all", 0)) {
			progerr(ERRMESG, pkginst);
			exit(1);
		}
	}

	do {
		param = argv[optind];
		if(!param) {
			param = parambuf;
			*param = '\0';
		} else
			paramflg++;
		value = pkgparam(pkginst, param);
		if(value == NULL) {
			if(errno == EFAULT) {
				progerr(ERRFLT);
				errflg++;
				break;
			} else if(errno != EINVAL) {
				/* some other error besides no value for this
				 * particular parameter
				 */
				progerr(ERRMESG, pkginst);
				errflg++;
				break;
			} else if((errno == EINVAL) && paramflg) {
				progerr(ERRINVAL, param);
				errflg++;
			}
			if(!argv[optind])
				break;
			continue;
		}
		if(vflag) {
			(void) printf("%s='", param);
			while(*value) {
				if(*value == '\'') {
					(void) printf("'\"'\"'");
					value++;
				} else
					(void) putchar(*value++);
			}
			(void) printf("'\n");
		} else 
			(void) printf("%s\n", value);
		
	} while(!argv[optind] || (++optind < argc));
	(void) pkgparam(NULL, NULL); /* close open FDs so umount won't fail */

	(void) pkghead(NULL);
	return(errflg ? 1 : 0);
}
