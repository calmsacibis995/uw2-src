/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)oampkg:common/cmd/oampkg/pkginfo/pkginfo.c	1.16.11.30"
#ident  "$Header: $"

#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <dirent.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <pkginfo.h>
#include <sys/types.h>
#include <pkgstrct.h>
#include <pkglocs.h>
#include <errno.h>
#include <pkgdev.h>
#include <unistd.h>

#include <locale.h>
#include <pfmt.h>

extern char	*optarg;
extern char	*pkgdir;
extern int	optind;
extern char	*errstr;

extern void	*calloc(), 
		exit(),
		logerr(),
		progerr();
extern int	getopt(),
		access(),
		devtype(),
		srchcfile(),
		pkghead(),
		gpkgmap();
extern char	*pkgparam();

#define nblock(size)	((size + (BLKSIZ - 1)) / BLKSIZ)
#define	BLKSIZ		512
#define MAXCATG	64

/*
 * The rules for displaying information on sets are:
 *
 *      If the category has been specified via "pkginfo -c set", information 
 *	on set installation packages installed on the system will be produced.
 *
 *      If a set name has been specified on the command line and if the set
 *	has been installed on the system, information on the set member packages
 *	that are also installed on the system is displayed.  
 *
 *      If set information on a set that is located on an installation media is 
 *	requested, information on all set member packages is displayed.  In the
 *	case of a tape, all options to pkginfo are allowed since information on
 *	the set member packages is readily available on the first archive on the
 *	tape (contains all set member packages' pkginfo and pkgmap files).  If
 *	the set is made up of multiple diskettes only short reports are possible.
 *	This is because pkginfo cannot access information files for the set member
 *	packages that are located on subsequent diskettes.  In this case, pkginfo
 *	can only produce a short listing (the default) by using the SIP's setinfo
 *	file.
 */

#define IFDISPLAY	ncatg || pkgcnt || ( ! info.catg || strcmp(info.catg, "set") != 0)
#define	RELOC_DUMP	(rflag && info.basedir && info.basedir[0])
#define	MSG_RELOC	gettxt(":809", "Installation base")

char	*prog;
char	*pkginst;

static char	*device = NULL;
static char	*paramlist[] = {
	"DESC", "PSTAMP", "INSTDATE", "VSTOCK", "SERIALNUM", "HOTLINE", 
	"EMAIL", NULL 
};

static char	contents[PATH_MAX];
static int	errflg = 0;
static int	qflag = 0;
static int	iflag = -1;
static int	pflag = -1;
static int	lflag = 0;
static int	Lflag = 0;
static int	Nflag = 0;
static int	rflag = 0;
static int	xflag = 0;
static struct 	cfent	entry;
static char	**pkg = NULL;
static int	pkgcnt = 0;
static char	*ckcatg[MAXCATG] = {NULL};
static int	ncatg = 0;
static char	*ckvers = NULL;
static char	*ckarch = NULL;
static char	setinfo[PATH_MAX];	/* setinfo full pathname	*/
static char	*setinst = NULL;	/* save set pkg instance name	*/
static FILE	*fp;			/* used to open setinfo file	*/
struct cfstat	*choice;
struct pkgdev	pkgdev;			/* used to determine type of device */
int	output;
int	cflag = 0;

static struct cfstat {
	char	pkginst[15];
	short	exec;
	short	dirs;
	short	link;
	short	symlink;
	short	partial;
	short	spooled;
	short	installed;
	short	info;
	short	shared;
	short	setuid;
	long	tblks;
	struct cfstat *next;
} *data;

/*
 * Refer to setinfo(4) manual page for the source of these defines.
 */
#define	SF_ABBR_LEN	9
#define	SF_FULLNAME_LEN	256

typedef struct sfent {
	char	si_abbr[SF_ABBR_LEN + 1];
	int	si_parts;
	char	si_default;
	char	*si_category;
	char	si_fullname[SF_FULLNAME_LEN + 1];
} sfent_t;

static struct pkginfo info;

static struct cfstat	*fpkg();
static int	iscatg(),
		my_select(); 
static void	usage(), 
		look_for_installed(), 
		report(), 
		rdcontents(),
		getinfo(),
		pkgusage(),
		dumpinfo(); 
static	void	quit(int);
sfent_t		*fgetsipent(FILE *, sfent_t *);

static	char	arguments[BUFSIZ];

/* 
 *New for diskette media and sets.
 */
int	cmdarg = 0;
char	*mem_pkginst, 
	*mem_name,
	*mem_catg;

main(argc,argv)
int	argc;
char	**argv;
{
	int	c;
	int	dflag = 0;
	int	args = 0;

	prog = strrchr(argv[0], '/');
	if(!prog++)
		prog = argv[0];

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxpkgtools");
	(void) setlabel("UX:pkginfo");

	while ((c = getopt(argc,argv,"LNxv:a:d:qpilc:r?")) != EOF) {
		args++;
		switch(c) {
		  case 'v':
			ckvers = optarg;
			cmdarg++;
			(void) sprintf(arguments, "%s -v \"%s\"", arguments, ckvers); 
			break;

		  case 'a':
			ckarch = optarg;
			cmdarg++;
			(void) sprintf(arguments, "%s -a \"%s\"", arguments, ckarch); 
			break;

		  case 'd':
			/* -d could specify stream or mountable device */
			dflag++;
			device = optarg;
			break;

		  case 'q':
			qflag++;
			(void) sprintf(arguments, "%s -q", arguments); 
			break;

		  case 'i':
			iflag = 1;
			if(pflag > 0)
				usage();
			pflag = 0;
			(void) sprintf(arguments, "%s -i", arguments); 
			break;

		  case 'p':
			pflag = 1;
			if(iflag > 0)
				usage();
			iflag = 0;
			(void) sprintf(arguments, "%s -p", arguments); 
			break;

		  case 'N':
			Nflag++;
			cmdarg++;
			(void) sprintf(arguments, "%s -N", arguments); 
			break;

		  case 'L':
			if(xflag || lflag) {
				progerr(":236:-L and -l/-x flags are incompatible");
				usage();
			}
			Lflag++;
			cmdarg++;
			(void) sprintf(arguments, "%s -L", arguments); 
			break;

		  case 'l':
			if(xflag) {
				progerr(":237:-l and -x flags are incompatible");
				usage();
			}
			cmdarg++;
			lflag++;
			(void) sprintf(arguments, "%s -l", arguments); 
			break;

		  case 'x':
			if(lflag) {
				progerr(":238:-l and -x flags are not compatible");
				usage();
			}
			xflag++;
			(void) sprintf(arguments, "%s -x", arguments); 
			break;

		  case 'c':
			cflag++;
			ckcatg[ncatg++] = strtok(optarg, " \t\n,");
			while(ckcatg[ncatg] = strtok(NULL, " \t\n,"))
				ncatg++;
			(void) sprintf(arguments, "%s -c %s", arguments, optarg); 
			break;

		  case 'r':
			rflag++;
			(void) sprintf(arguments, "%s -r", arguments); 
			break;

		  default:
			usage();
		}
	}
	
	if (dflag && rflag) {
		progerr(":810:-d and -r flags are incompatible");
		usage();
	}
	
	pkg = &argv[optind];
	pkgcnt = (argc - optind);

	if(pkg[0] && !strcmp(pkg[0], "all")) {
		pkgcnt = 0;
		pkg[0] = NULL;
	} 

	if(pkgdir == NULL)
		pkgdir = PKGLOC;

        memset(&info,NULL,sizeof(info));

	/* convert device appropriately */
	if (pkghead(device)) {
		progerr(":605:unable to complete package transfer");
		quit(1);
	}

	/*
	 * If device was specified, get information on it.
	 */

	if((device != NULL) && devtype(device, &pkgdev)) {
		progerr(":134:bad device <%s> specified", device);
		quit(1);
	}

	look_for_installed();

	if(lflag && !strcmp(pkgdir, PKGLOC)) {
		/* look at contents file */
		(void) sprintf(contents, "%s/contents", PKGADM);
		rdcontents();
	}

	report();

	/*
	 * :XENIX Support:
	 * Only display these if no options or if -l or -i options 
	 * were specified. In this case, also show packages installed 
	 * via the "custom" utility.
	 */
	/*
	if(ckvers || ckarch || dflag || qflag || pflag || xflag || cflag )
		;
	*/
	if(!args && pkgcnt == 0) {
		if(access("/usr/bin/displaypkg", X_OK||R_OK) == 0) {
			(void) fflush(stdout);
			system ("/usr/bin/displaypkg XENIX");
		}
	}

	quit(errflg ? 1 : 0);
}

char *fmt = "%10s:  %s\n";

static void
report()
{
	struct cfstat *dp;
	int	nsetpkgs;
	int	set = 0;
	int	i, n;
	char	member[BUFSIZ];
	char	cmd[BUFSIZ];
	int	installed_dir;
	int	gotit = 0;
	int	set_installed = 0;
	char	*msg_locale;
	sfent_t	sf;

	output = 0;
	for (;;) {
		choice = (struct cfstat *)0;
		installed_dir = 0;
		for (dp = data; dp; dp = dp->next) {
			/*
			 * Get information about this package.
			 */
			if (dp->installed < 0) {
				/*
				 * Already used.
				 */
				continue;
			}
			if (Lflag && pkgcnt) {
				choice = dp;
				break;
			} else if (choice == NULL ||
				   (strcmp(choice->pkginst, dp->pkginst) > 0)) {
				choice = dp;
			}
		}
		if (choice == NULL) {
			/*
			 * No more packages.
			 */
			break;
		}
		if (pkginfo(&info, choice->pkginst, ckarch, ckvers)) {
			choice->installed = (-1);
			continue;
		}

		/*
		 * Is it in an appropriate category?
		 */
		if (iscatg(info.catg)) {
			choice->installed = (-1);
			continue;
		}

		/*
		 * If the category for the current package is "set", no -c
		 * option was specified and one or more package instances
		 * were specified on the command line, report information
		 * on the set's member packages rather than on the SIP(s)
		 * themselves.  If the set is installed, report only on
		 * those set member packages that are also installed.  If
		 * the set is not installed and the packages are spooled or
		 * on tape media, report on all that set's member packages.
		 * If the packages are not located in the installed directory
		 * (PKGLOC), we'll only be able to do a short report by
		 * using the setinfo file to simulate what we would get
		 * from the set member package's pkginfo file.
		 */ 
		if (info.catg && !strcmp(info.catg, "set")) {
			/*
			 * Save SIP name for later.
			 */
			setinst = strdup(info.pkginst);
		}
		if (setinst && !cflag && pkgcnt > 0) {
			if (info.status == PI_INSTALLED) {
				set_installed++;
			}
			set++;
			/*
			 * Get member package names from setinfo file.
			 * Skip reporting on the set installation 
			 * package itself.
			 */

			msg_locale = setlocale(LC_MESSAGES, NULL);
			if (strcmp(msg_locale, "C") != 0) {
				(void) sprintf(setinfo,
					"%s/%s/install/inst/locale/%s/setinfo",
						pkgdir, choice->pkginst,
						msg_locale);
				if (access(setinfo, R_OK) == -1) {
					(void)sprintf(setinfo, "%s/%s/setinfo",
							pkgdir,
							choice->pkginst);
				}
			} else {
				(void) sprintf(setinfo, "%s/%s/setinfo",
						pkgdir, choice->pkginst);
			}

			if ((fp = fopen(setinfo, "r")) == NULL) {
				progerr(":164:Could not open setinfo file for <%s>",
						choice->pkginst);
				quit(1);
			}

			/* 
			 * Get names of set member packages from setinfo
			 * file.  Check if the package is in a directory
			 * other than the system installed directory.
			 * If so, we'll only be able to do a short listing
			 * by using the contents of the setinfo file.
			 * For this case, if -a, -l, -L, -n or -v options
			 * were specified exit with an error.  If installed
			 * or on tape media, just build a pkginfo command
			 * line with all set member packages on it to be
			 * exec'ed. NOTE: This may require additional smarts
			 * when CD-ROM comes into the picture.
			 */
			if (strcmp(pkgdir, PKGLOC) == 0) {
				installed_dir++;
			}
			(void)memset(member, '\0', BUFSIZ);
			sf.si_abbr[0] = '\0';
			sf.si_fullname[0] = '\0';
			sf.si_category = NULL;
			for (nsetpkgs = 0;
			     fgetsipent(fp, &sf) != NULL; nsetpkgs++) {
				if (installed_dir) {
					if (set_installed) {
						(void)pkginfo(&info, NULL);
						(void)pkginfo(&info, sf.si_abbr,
								NULL, NULL);
						if (info.status != PI_INSTALLED)
							continue;
						else
							gotit++;
					}	
					(void)strcat(member, sf.si_abbr);
					(void)strcat(member, " ");
				} else {
					/*
					 * For sets that are not installed
					 * use the SIP's setinfo file to
					 * produce a list of set member
					 * packages.  No information beyond
					 * category, pkginst and package
					 * name is available.
					 */
					if (cmdarg && !cflag) {
						progerr(":239:detailed set member package information is not available");
						quit(1);
					}
					(void)fprintf(stdout,
						"%-11.11s %-14.14s %s\n",
						sf.si_category,
						sf.si_abbr,
						sf.si_fullname);
				}
				sf.si_abbr[0] = '\0';
				sf.si_fullname[0] = '\0';
				if (sf.si_category != NULL) {
					(void)free(sf.si_category);
					sf.si_category = NULL;
				}
			}
			(void) fclose(fp);	/* Close setinfo file */

			/*
			 * For installed SIP or for one that is located on
			 * tape, build pkginfo command line that will be
			 * exec'ed for set member package instances.  For
			 * installed sets, only report on those member
			 * packages that are also installed.
			 */
			if (installed_dir && gotit) {
				(void)sprintf(cmd, "pkginfo -d %s %s %s",
						pkgdir, arguments, member);
				if (n = esystem(cmd, -1, -1)) {
					rpterr();
					progerr(":243:unexpected error re-executing pkginfo command");
					quit(1);
				}
				/*
				 * Reset set members string to null in case
				 * we're asking for information on more than
				 * one set. Otherwise, we'll get information
				 * on this set's packages displayed along
				 * with the next set's members.
				 */
				 (void)strcpy(member, NULL);
			}

			/* 
			 * Set set installation package instance in pkg[] to
			 * null so that we don't get an error message stating
			 * that information for it was not found.
			 */
			for (i = 0; i < pkgcnt; i++) {
				if (!strcmp(pkg[i], choice->pkginst)) {
					pkg[i] = NULL;
					break;
				}
			}
			/*
			 * Force the "for(dp=data; ... )" loop to go on to
			 * the next package instance specified on the command
			 * line, if any.
			 */
			choice->installed = -1;
		}

		/*
		 * If this was a set installation package, then the report
		 * was done from the exec'ed pkginstall -- skip it here.
		 */
		if (!set) {
			if (doreport())
				return;
		} else {
			set--;
		}
	}

	/*
	 * Verify that each package listed on command line was output.
	 */
	for (i = 0; i < pkgcnt; ++i) {
		if (pkg[i]) {
			if (!qflag) {
				logerr(":244:ERROR:information for \"%s\" was not found", pkg[i]);
			}
			errflg++;
		}
	}
	/*
	 * Free up all memory and close opened fds.
	 */
	(void)pkginfo(&info, NULL);
}

static int
doreport()
{
	int i;

	if(!pflag &&  
		/* don't include partially installed packages */
		(choice->partial || (info.status == PI_PARTIAL) || 
			(info.status == PI_UNKNOWN))) {
		/*
		 * If we're reporting only fully installed
		 * package don't incrament error flag.
		 */
		if(!iflag)
			errflg++;
		choice->installed = (-1);
		return(0);
	}

	if(Nflag && (info.status == PI_PRESVR4)) {
		/* don't include preSVR4 packages */
		choice->installed = (-1);
		return(0);
	}

	if(!iflag && ((info.status == PI_INSTALLED) ||
			(info.status == PI_PRESVR4))) {
		/* don't include completely installed packages */
		choice->installed = (-1);
		return(0);
	}

	output++;
	dumpinfo(choice);
	choice->installed = (-1);
	if(pkgcnt && !qflag) {
		i = my_select(choice->pkginst);
		if(i >= 0)
			pkg[i] = NULL;
	}

	if(!output)
		errflg++;

	if(qflag)
		return(1);

	return(0);
}

static void
dumpinfo(dp)
struct cfstat *dp;
{
	register int i;
	char *pt, category[128];

	if(qflag)
		return; /* print nothing */

	if(Lflag) {
		if(IFDISPLAY) {
			if (RELOC_DUMP) {
				fprintf(stdout, "%-14.14s  %s: %s\n",
					info.pkginst, MSG_RELOC, info.basedir);
			} else {
				(void) puts(info.pkginst);
			}
		}
		return;
	} else if(xflag) {
		if(IFDISPLAY) {
			(void) printf("%-14.14s  %s\n", info.pkginst, info.name);
			if(info.arch || info.version) 
				(void) printf("%14.14s  ", "");
			if(info.arch)
				(void) printf("(%s) ", info.arch);
			if(info.version)
				(void) printf("%s", info.version);
			if(info.arch || info.version)
				(void) printf("\n");
			if (RELOC_DUMP) {
				fprintf(stdout, "%16.16s%s: %s\n",
					"", MSG_RELOC, info.basedir);
			}
		}
		return;
	} else if(!lflag) {
		category[0] = '\0';
		if(info.catg)
			(void) sscanf(info.catg, "%[^, \t\n]", category);
		else if(info.status == PI_PRESVR4)
			(void) strcpy(category, "preSVR4");
		else
			(void) strcpy(category, gettxt(":113", "(unknown)"));
		if(IFDISPLAY) {
			(void) fprintf(stdout, "%-11.11s %-14.14s %s\n", category,
					info.pkginst, info.name);
			if (RELOC_DUMP) {
				fprintf(stdout, "%27.27s%s: %s\n",
					"", MSG_RELOC, info.basedir);
			}
		}
		return;
	}
	if(IFDISPLAY) {
		if(info.pkginst)
			(void) printf(fmt, "PKGINST", info.pkginst);
		if(info.name)
			(void) printf(fmt, "NAME", info.name);
		if(lflag && info.catg)
			(void) printf(fmt, "CATEGORY", info.catg);
		if(lflag && info.arch)
			(void) printf(fmt, "ARCH", info.arch);
		if(info.version)
			(void) printf(fmt, "VERSION", info.version);
		if(info.basedir)
			(void) printf(fmt, "BASEDIR", info.basedir);
		if(info.vendor)
			(void) printf(fmt, "VENDOR", info.vendor);
	
		if(info.status == PI_PRESVR4)
			(void) printf(fmt, "STATUS", "preSVR4");
		else {
			if( ! cflag && ( strcmp(info.catg,"set")) && (pt = pkgparam(info.pkginst, "SETINST")) && *pt)
				(void) printf(fmt, "SETINST", pt);
			if( ! cflag && ( strcmp(info.catg,"set")) && (pt = pkgparam(info.pkginst, "SETNAME")) && *pt)
				(void) printf(fmt, "SETNAME", pt);
			for(i=0; paramlist[i]; ++i) {
				if((pt = pkgparam(info.pkginst, paramlist[i])) && *pt)
					(void) printf(fmt, paramlist[i], pt);
			}
			if(info.status == PI_SPOOLED)
				(void) printf(fmt, "STATUS", gettxt(":245", "spooled"));
			else if(info.status == PI_PARTIAL)
				(void) printf(fmt, "STATUS", gettxt(":246", "partially installed"));
			else if(info.status == PI_INSTALLED)
				(void) printf(fmt, "STATUS", gettxt(":247", "completely installed"));
			else
				(void) printf(fmt, "STATUS", gettxt(":113", "(unknown)"));
		}
		if (RELOC_DUMP) {
			fprintf(stdout, "  %s: %s\n", MSG_RELOC, info.basedir);
		}
	}
	(void) pkgparam(NULL, NULL);

	if(!lflag) {
		(void) putchar('\n');
		return;
	}

	/*
	 * If -l option was used and no pkginst specified, we must check that this
	 * is not a SIP instance so that we don't display FILES information on the
	 * set installation package itself.  In the case where category "set" was
	 * specified to -c option and this is a SIP, we want to display FILES
	 * information on the SIP itself.
	 */
	if (  setinst == NULL || cflag)  {
		if(info.status != PI_PRESVR4) {
			if(strcmp(pkgdir, PKGLOC))
				getinfo(dp);
	
			if(dp->spooled)
				(void) pfmt(stdout, MM_NOSTD, ":248:%10s:  %5d spooled pathnames\n", 
					"FILES", dp->spooled);
			if(dp->installed)
				(void) pfmt(stdout, MM_NOSTD, ":249:%10s:  %5d installed pathnames\n", 
					"FILES", dp->installed);
			if(dp->partial)
				(void) pfmt(stdout, MM_NOSTD, ":250:%18d partially installed pathnames\n", 
					dp->partial);
			if(dp->shared)
				(void) pfmt(stdout, MM_NOSTD, ":251:%18d shared pathnames\n", 
					dp->shared);
			if(dp->link)
				(void) pfmt(stdout, MM_NOSTD, ":252:%18d linked files\n", dp->link);
			if(dp->symlink)
				(void) pfmt(stdout, MM_NOSTD, ":790:%18d symbolic links\n", dp->symlink);
			if(dp->dirs)
				(void) pfmt(stdout, MM_NOSTD, ":253:%18d directories\n", dp->dirs);
			if(dp->exec)
				(void) pfmt(stdout, MM_NOSTD, ":254:%18d executables\n", dp->exec);
			if(dp->setuid)
				(void) pfmt(stdout, MM_NOSTD, ":255:%18d setuid/setgid executables\n", 
					dp->setuid);
			if(dp->info)
				(void) pfmt(stdout, MM_NOSTD, ":256:%18d package information files\n", 
					dp->info+1); /* pkgmap counts! */
			
			if(dp->tblks)
				(void) pfmt(stdout, MM_NOSTD, ":257:%18ld blocks used (approx)\n", 
					dp->tblks);
		}
		(void) putchar('\n');
	}
	/* reset */
	setinst = NULL;
}

static struct cfstat *
fpkg(pkginst)
char *pkginst;
{
	struct cfstat *dp, *last;

	dp = data;
	last = (struct cfstat *)0;
	while(dp) {
		if(!strcmp(dp->pkginst, pkginst))
			return(dp);
		last = dp;
		dp = dp->next;
	}
	dp = (struct cfstat *)calloc(1, sizeof(struct cfstat));
	if(!dp) {
		progerr(":258:no memory, malloc() failed");
		quit(1);
	}
	if(!last)
		data = dp;
	else
		last->next = dp; /* link list */
	(void) strncpy(dp->pkginst, pkginst, 14);
	return(dp);
}
	
#define SEPAR	','

static int
iscatg(list)
char *list;
{
	register int i;
	register char *pt;
	int	match;

	if(!ckcatg[0])
		return(0); /* no specification implies all packages */
	if(info.status == PI_PRESVR4) {
		for(i=0; ckcatg[i]; ) {
			if(!stricmp(ckcatg[i++], "preSVR4"))
				return(0);
		}
		return(1);
	}
	if(!list)
		return(1); /* no category specified in pkginfo is a bug */

	match = 0;
	do {
		if(pt = strchr(list, ','))
			*pt = '\0';

		for(i=0; ckcatg[i]; ) {
			if(!stricmp(list, ckcatg[i++])) {
				match++;	
				break;
			}
		}

		if(pt)
			*pt++ = ',';
		if(match)
			return(0);
		list = pt; /* points to next one */
	} while(pt);
	return(1);
}

static void
look_for_installed()
{
	struct dirent *drp;
	struct stat	status;
	DIR	*dirfp;
	char	path[PATH_MAX];
	int	n;

	if(!strcmp(pkgdir, PKGLOC) && (dirfp = opendir(PKGOLD))) {
		while(drp = readdir(dirfp)) {
			if(drp->d_name[0] == '.')
				continue;
			n = strlen(drp->d_name);
			if((n > 5) && !strcmp(&drp->d_name[n-5], ".name")) {
				(void) sprintf(path, "%s/%s", PKGOLD, 
					drp->d_name);
				if(lstat(path, &status))
					continue;
				if((status.st_mode & S_IFMT) != S_IFREG)
					continue;
				drp->d_name[n-5] = '\0';
				if(!pkgcnt || (my_select(drp->d_name) >= 0))
					(void) fpkg(drp->d_name);
			}
		}
		(void) closedir(dirfp);
	}

	if((dirfp = opendir(pkgdir)) == NULL)
		return;

	while(drp = readdir(dirfp)) {
		if(drp->d_name[0] == '.')
			continue;

		if(pkgcnt && (my_select(drp->d_name) < 0))
			continue;

		(void) sprintf(path, "%s/%s/pkginfo", pkgdir, drp->d_name);
		if(access(path, 0))
			continue; /* doesn't appear to be a package */
		(void) fpkg(drp->d_name);
	}
	(void) closedir(dirfp);
}

static int
my_select(p)
char *p;
{
	register int i;

	for(i=0; i < pkgcnt; ++i) {
		if(pkg[i] && (pkgnmchk(p, pkg[i], 1) == 0))
			return(i);
	}
	return(-1);
}

static void
rdcontents()
{
	FILE *fp1;
	struct cfstat *dp;
	struct pinfo *pinfo;
	int n;

	if((fp1 = fopen(contents, "r")) == NULL) {
		progerr(":259:unable open \"%s\" for reading", contents);
		quit(1);
	}

	/* check the contents file to look for referenced packages */
	while((n = srchcfile(&entry, "*", fp1, NULL)) > 0) {
		for(pinfo=entry.pinfo; pinfo; pinfo=pinfo->next) {
			/* see if entry is used by indicated packaged */
			if(pkgcnt && (my_select(pinfo->pkg) < 0))
				continue;

			dp = fpkg(pinfo->pkg);
			pkgusage(dp, &entry);

			if(entry.npkgs > 1)
				dp->shared++;

			if(pinfo->status)
				dp->partial++;
			else
				dp->installed++;
		}
	}
	if(n < 0) {
		progerr(":2:bad entry read in contents file");
		logerr(":199:pathname: %s", entry.path);
		logerr(":200:problem: %s", errstr);
		quit(1);
	}

	(void) fclose(fp1);
}

static void
getinfo(dp)
struct cfstat	*dp;
{
	FILE *fp;
	int n;
	char pkgmap[256];

	(void) sprintf(pkgmap, "%s/%s/pkgmap", pkgdir, dp->pkginst);
	if((fp = fopen(pkgmap, "r")) == NULL) {
		progerr(":259:unable open \"%s\" for reading", pkgmap);
		quit(1);
	}
	dp->spooled = 1; /* pkgmap counts! */
	while((n = gpkgmap(&entry, fp)) > 0) {
		dp->spooled++;
		pkgusage(dp, &entry);
	}
	if(n < 0) {
		progerr(":260:bad entry read in pkgmap file");
		logerr(":199:pathname: %s", entry.path);
		logerr(":200:problem: %s", errstr);
		quit(1);
	}
	(void) fclose(fp);
}
	
static void
pkgusage(dp, pentry)
struct cfstat	*dp;
struct cfent	*pentry;
{
	if(pentry->ftype == 'i') {
		dp->info++;
		return;
	} else if(pentry->ftype == 'l') {
		dp->link++;
	} else if(pentry->ftype == 's') {
		dp->symlink++;
	} else {
		if((pentry->ftype == 'd') || (pentry->ftype == 'x'))
			dp->dirs++;
		if(pentry->ainfo.mode & 06000)
			dp->setuid++;
		if(!strchr("dxcbp", pentry->ftype) && 
		(pentry->ainfo.mode & 0111))
			dp->exec++;
	}

	if(strchr("ifve", pentry->ftype)) 
		dp->tblks += nblock(pentry->cinfo.size);
}

static void
usage()
{
	(void) pfmt(stderr, MM_ACTION, ":26:usage:\n");
	(void) pfmt(stderr, MM_NOSTD,
	":811:  %s [-q] [-p|-i] [-x|-l] [-r] [options] [pkg ...]\n", prog);
	(void) pfmt(stderr, MM_NOSTD,
	":262:  %s -d device [-q] [-x|-l] [options] [pkg ...]\n", prog);
	(void) pfmt(stderr, MM_NOSTD, ":263:where\n");
	(void) pfmt(stderr, MM_NOSTD, ":264:  -q #quiet mode\n");
	(void) pfmt(stderr, MM_NOSTD, ":265:  -p #select partially installed packages\n");
	(void) pfmt(stderr, MM_NOSTD, ":266:  -i #select completely installed packages\n");
	(void) pfmt(stderr, MM_NOSTD, ":267:  -x #extracted listing\n"); 
	(void) pfmt(stderr, MM_NOSTD, ":268:  -l #long listing\n"); 
	(void) pfmt(stderr, MM_NOSTD,
	":812:  -r #list installed base if package is relocatable\n");
	(void) pfmt(stderr, MM_NOSTD, ":269:and options may include:\n");
	(void) pfmt(stderr, MM_NOSTD, ":270:  -c category,[category...]\n");
	(void) pfmt(stderr, MM_NOSTD, ":271:  -a architecture\n");
	(void) pfmt(stderr, MM_NOSTD, ":272:  -v version\n");
	exit(1);
}

stricmp(s1, s2)
	register char *s1, *s2;
{
static char charmap[] = {
	'\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
	'\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
	'\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
	'\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
	'\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
	'\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
	'\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
	'\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
	'\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
	'\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
	'\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
	'\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
	'\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
	'\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
	'\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
	'\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
	'\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
	'\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
	'\300', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\333', '\334', '\335', '\336', '\337',
	'\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
};

	register char *cm = charmap;

	while (cm[*s1] == cm[*s2++])
		if (*s1++ == '\0')
			return(0);
	return(cm[*s1] - cm[*--s2]);
}

static void
quit(int retval)
{

	(void)pkghead(NULL);
	exit(retval);
}

sfent_t *
fgetsipent(FILE *sfp, sfent_t *sp)
{
	register char *pt;
	char buffer[BUFSIZ];

	while (fgets(buffer, BUFSIZ, sfp) != NULL) {
		if (buffer[0] == '#' || buffer[0] == ' ' || buffer[0] == '\n') {
			continue;
		}
		pt = strtok(buffer, "\t");
		if (pt == NULL || strlen(pt) > SF_ABBR_LEN) {
			progerr(":240:could not obtain set member package instance name");
			quit(1);
		}
		(void)strcpy(sp->si_abbr, pt);
		sp->si_parts = atoi(strtok(NULL, "\t"));
		pt = strtok(NULL, "\t");
		sp->si_default = pt ? *pt : '\0';
		if ((pt = strtok(NULL, "\t")) == NULL) {
			progerr(":241:could not obtain set member package category");
			quit(1);
		}
		sp->si_category = strdup(pt);
		pt = strtok(NULL, "\n");
		if (pt == NULL || strlen(pt) > SF_FULLNAME_LEN) {
			progerr(":242:could not obtain set member package name");
			quit(1);
		}
		(void)strcpy(sp->si_fullname, pt);
		return(sp);
	}
	return(NULL);
}
