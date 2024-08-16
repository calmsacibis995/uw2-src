/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oampkg:common/cmd/oampkg/pkgmk/main.c	1.2.16.25"
#ident  "$Header: $"

#include <sys/param.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <pkgstrct.h>
#include <pkgdev.h>
#include <pkginfo.h>
#include <pkglocs.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <sys/stat.h>
#include <unistd.h>

#include <locale.h>
#include <pfmt.h>

extern char	**environ, *optarg, 
		*pkgdir;
extern int	optind, errno,noprompt;

extern struct cfent 
		**procmap();
extern void	*calloc(), *realloc(),
		progerr(), logerr(),
		putparam(), quit(),
		free(), exit();
extern char	*qstrdup(), *srcpath(), 
		*fpkgparam(), *pkgparam(), 
		*devattr(), *fpkginst(), 
		*getenv(), *getinst();
extern long	atol(), time();
extern int	getopt(), access(),
		cftime(), mkdir(),
		isdir(), symlink(),
		splpkgmap(), mkpkgmap(),
		copyf(), rrmdir(),
		setlist(), ppkgmap(), 
		devtype(), pkgmount(),
		pkgumount(), cverify();

#define PBLK	512	/* 512 byte "physical" block */
#define MALSIZ	 16
#define MINSTSIZ  4	/* best guess at likely maximum value of MAXINST */ 
#define NROOT	  8
#define SPOOLDEV	"spool"

#define MSG_PROTOTYPE	":442:## Building pkgmap from package prototype file.\n"
#define MSG_PKGINFO	":443:## Processing pkginfo file.\n"
#define MSG_VOLUMIZE	":444:## Attempting to volumize %d entries in pkgmap.\n"
#define MSG_COMPRESS    ":445:## Compressing package files.\n"
#define MSG_MIRROR      ":446:## Making mirror image of package for compression.\n"
#define MSG_COMPROC     ":447:## Processing compressed version of package.\n"
#define	ERR_CPFILE	":448:could not copy <%s> to mirror image of package"
#define	ERR_COMPR	":449:unexpected error during compression of <%s>"
#define	ERR_READF	":450:could not read <%s>"
#define	ERR_REXEC	":451:error occurred in attempt to re-execute pkgmk."
#define ERR_MEMORY	":6:memory allocation failure, errno=%d"
#define ERR_NROOT	":452:too many paths listed with -r option, limit is %d"
#define ERR_PKGINST	":453:invalid package instance identifier <%s>"
#define ERR_PKGABRV	":454:invalid package abbreviation <%s>"
#define ERR_BADDEV	":455:unknown or invalid device specified <%s>"
#define ERR_TEMP	":456:unable to obtain temporary file resources, errno=%d"
#define ERR_DSTREAM	":457:invalid device specified (datastream) <%s>"
#define ERR_SPLIT	":458:unable to volumize package"
#define ERR_MKDIR	":227:unable to make directory <%s>"
#define ERR_SYMLINK	":459:unable to create symbolic link for <%s>"
#define ERR_OVERWRITE	":460:must use -o option to overwrite <%s>"
#define ERR_UMOUNT	":461:unable to unmount device <%s>"
#define ERR_NOPKGINFO	":462:unable to locate required pkginfo file"
#define ERR_RDPKGINFO	":463:unable to process pkginfo file <%s>"
#define ERR_PROTOTYPE	":464:unable to locate prototype file"
#define ERR_STATVFS	":465:unable to stat filesystem <%s>"
#define ERR_WHATVFS	\
	":466:unable to determine or access output file system for device <%s>"
#define ERR_DEVICE	":467:unable to find info for device <%s>"
#define ERR_BUILD	":468:unable to build pkgmap from prototype file"
#define ERR_ONEVOL	":469:other packages found - package must fit on a single volume"
#define ERR_COMPRESS    ":470:failure in compression routine" 
#define ERR_UNIQ    	":471:could not create compressed image directory list"
#define ERR_CFILE       ":472:unable to open compression list file <%s>"   
#define ERR_PWD         ":473:unable to get current working directory"
#define ERR_FREE	":474:package does not fit space currently available in <%s>"
#define ERR_NOPARAM	":475:parameter <%s> is not defined in <%s>"
#define ERR_PKGMTCH	":418:PKG parameter <%s> does not match instance <%s>"
#define WRN_COMPR	":476:WARNING:Not compressing <%s> - no savings"
#define	WRN_RELPATH	":477:WARNING:Not compressing <%s> - relative path above rootlist"
#define WRN_MISSINGDIR	":478:WARNING:missing directory entry for <%s>"
#define WRN_SETPARAM	":479:WARNING:parameter <%s> set to \"%s\""
#define WRN_CLASSES	\
	 ":480:WARNING:unreferenced class <%s> in prototype file"
#define ERR_SYSINFO	\
	":481:unable to process package information, errno=%d"

struct pkgdev pkgdev;	/* holds info about the installation device */
int	started;
char	pkgloc[PATH_MAX];
char	*pkgname, *pkgvers, *pkgarch, *pkgcat;
char	*pkginst;
char	*prog; 
char	*basedir; 
char	*root; 
char	*rootlist[NROOT];
char	*t_pkgmap; 
char	*t_pkginfo;
char	**class;
char	cmdpath[PATH_MAX];	/* absolute pathname to command */
int	nclass = (-1);	/* forces procmap() to use all classes */
int	compress = 0;	/* set if -c option used to specify compression */
static	int	*svcksum;
static	int	*svsize;
int	rflag = 0;
int	xflag = 0;	
int	cflag = 0;
int	sflag;
char	*tmpdir;		/* location to place temporary files/directory	*/
char 	cfile[PATH_MAX];	/* file contains prototype entries */
char 	cdir[PATH_MAX];		/* for compression, temporary directory for mirror image */
char 	cuniq[PATH_MAX];	/* file contains sorted directory list for mirror image  */
char	fname[PATH_MAX];	/* sfp file name */
struct	stat dirstat;		/* used to check if directory exists before creating */
FILE	*clist;
FILE	*sfp;	/* pointer to file of precompressed sizes and cksums */
pid_t	pid, xpid;
char	*actkey;		/* Package validation enabler value. */

static struct cfent *svept;
static struct cfent *seept = NULL;	/* For setinfo file */
static char	**allclass,
		*protofile,
		*device; 
static long	limit = 0, llimit = 0;
static int	overwrite,
		ilimit = 0,
		nflag;
static void
	trap(), 
	addclass(), 
	outvol(),
	ckmissing(),
	cleanup(),
	usage();
static int
	comprpkg(),
	slinkf();
struct cfent	**eptlist;

main(argc,argv)
int	argc;
char	*argv[];
{
	struct utsname utsbuf;
	struct statvfs statvfsbuf;
	struct pkginfo 	*prvinfo;
	FILE	*fp;
	char	listfile[PATH_MAX];	/* used to pass sfp file to 2nd invocation */
	char	*cmd;			/* save value returned from pathfind	*/
	char	cmdline[BUFSIZ];
	char	*pwd;			/* present working directory		*/
	char	rootl[BUFSIZ];		/* for placing rootlist in environment	*/ 
	char	savopt[BUFSIZ];		/* save options for reinvocation	*/
	char	tdirenv[PATH_MAX];	/* pass tmpdir to child pkgmk		*/

	int	i, j, c, n, eptnum, found,
		part, nparts, npkgs, objects;
	char	buf[64], temp[64], param[64], 
		*pt, *value,
		**envparam, **order;
	void	(*func)();
	long	clock;
	ulong	bsize;
	ulong	frsize = 0;

        limit = 0L;
	ilimit = 0;
	bsize = (ulong) 0;

	/*
	 * Get command name and create command path for 
	 * reinvocation if the -c option was specified.
	 */
	prog = strrchr(argv[0], '/');
	if(!prog++) {
		/* argv[0] contained no '/' - uses $PATH */
		(void) sprintf(cmdpath, "%s", argv[0]);
		prog = argv[0];
	} else {
		/* argv[0] contained a '/' */
		if (argv[0][0] == '/')
			/* argv[0] is an absolute pathname */
			(void) sprintf(cmdpath, "%s", argv[0]); 
		else {
			/* argv[0] is a relative pathname */
			pwd = getcwd(NULL, PATH_MAX);
			(void) sprintf(cmdpath, "%s/%s", pwd, argv[0]);
		}
	}

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxpkgtools");
	(void) setlabel("UX:pkgmk");

	tmpdir = getenv("TMPDIR");
	/*
	 * Find out if ACTKEY variable is set and save it to be restored
	 * later.
	 */
	actkey = getenv("ACTKEY");
	if (strcmp(actkey, "YES") != 0) {
		actkey = NULL;
	}
	if(tmpdir == NULL || stat(tmpdir, &dirstat) < 0)
		tmpdir = P_tmpdir;
		

	func = sigset(SIGINT, trap);
	if(func != SIG_DFL)
		func = sigset(SIGINT, func);
	func = sigset(SIGHUP, trap);
	if(func != SIG_DFL)
		func = sigset(SIGHUP, func);

	environ = NULL;
	pid = getpid();
	while((c = getopt(argc, argv, "ckosnB:p:l:r:b:d:f:a:v:x:?")) != EOF) {
		switch(c) {
		  case 'c':
			compress++;
			cflag++;
			(void) sprintf(cdir, "%s/cdir%d", tmpdir, pid);
			if (mkdir(cdir, S_IRWXU|S_IRGRP|S_IXGRP|S_IROTH|S_IXOTH) < 0) {
				progerr(ERR_MKDIR, cdir);
				quit(1);
			}
			break;

		  case 'n':
			nflag++;
			break;

		  case 's':
			sflag++;
			break;

		  case 'o':
			overwrite++;
			strcat(savopt, " -o ");
			break;

		  case 'B':
			strcat(savopt, " -B ");
			strcat(savopt, optarg);
			bsize = atol(optarg);
			frsize = atol(optarg);
			break;

		  case 'p':
			putparam("PSTAMP", optarg);
			strcat(savopt, " -p\"");
			strcat(savopt, optarg);
			strcat(savopt, "\"");
			break;

		  case 'k':
			actkey = qstrdup("YES");
			strcat(savopt, " -k ");
			break;

		  case 'l':
			strcat(savopt, " -l ");
			strcat(savopt, optarg);
			llimit = atol(optarg);
			break;

		  case 'r':
			rootlist[0] = optarg;
			rflag = strlen(rootlist[0]);
			/* 
			 * If -x was specified, rootlist[1]
			 * gets parent's rootlist[0].
			 */
			if (!xflag)
				rootlist[1] = NULL;
			break;

		  case 'b':
			strcat(savopt, " -b \"");
			strcat(savopt, optarg);
			strcat(savopt, "\"");
			basedir = optarg;
			break;

		  case 'f':
			strcat(savopt, " -f ");
			strcat(savopt, optarg);
			protofile = optarg;
			break;

		  case 'd':
			strcat(savopt, " -d ");
			strcat(savopt, optarg);
			device = optarg;
			break;

		  case 'a':
			strcat(savopt, " -a\"");
			strcat(savopt, optarg);
			strcat(savopt, "\"");
			putparam("ARCH", optarg);
			break;

		  case 'v':
			strcat(savopt, " -v\"");
			strcat(savopt, optarg);
			strcat(savopt, "\"");
			putparam("VERSION", optarg);
			break;

		  /*
		   * The -x option is used only for compression.  When compression
		   * is requested (-c), pkgmk will reinvoke itself after it has made
		   * a mirror image of the package, compressed the appropriate files
		   * and saved original check sum and size values for the non-compressed
		   * versions of the files that are compressed.  Additionally, this
		   * option is used to supply (to the child pkgmk) the original rootlist
		   * used by the parent pkgmk.  Also, the original checksum and size
		   * values for the uncompressed version of files in this package are
		   * accessed from the file <tmpdir>/svlist<parent_pid>.  We use the
		   * values in this file when creating the final pkgmap file for this
		   * package.  In this way, the pkgmap file will contain the size and
		   * checksum of the files before compression was performed.
		   */
		  case 'x':
			xflag++;
			noprompt++;

			/*
			 * This is the reinvocation of pkgmk for the compressed mirror
			 * image.  Get parent's rootlist[0] in case we can't access the
			 * file in compressed mirror image for case of entry in prototype
			 * that refers to a relative pathname above the rootlist, such as:
			 *
			 * 	f none /usr/bin/foo=../src/foo 755 root sys
			 *
			 * Such entries will not be mapped correctly into a the mirror
			 * image created for compression, so we'll use the original 
			 * uncompressed versions of these files from their original
			 * location.
			 */

			rootlist[1] = optarg;

			/*
			 * Get pid of invoking pkgmk and build path to file
			 * containing precompressed sizes and sums of package
			 * files.
			 */

			xpid = getppid();
			(void) sprintf(listfile ,"%s/svlist%d", tmpdir, xpid); 
			break;

		  default:
			usage();
		}
	}

	if (actkey) {
		putparam("ACTKEY", "YES");
	}

	/* 
	 * add command line variable assignments to environment
	 */
	envparam = &argv[optind];
	while(argv[optind] && strchr(argv[optind], '='))
		optind++;
	if(pkginst = argv[optind]) {
		argv[optind++] = NULL;
	}
	if(optind != argc)
		usage();

	if(device == NULL) {
		device = devattr(SPOOLDEV, "pathname");
		if(device == NULL) {
			progerr(ERR_DEVICE, SPOOLDEV);
			exit(99);
		}
	}

	if(protofile == NULL) {
		if(access("prototype", 0) == 0)
			protofile = "prototype";
		else if(access("Prototype", 0) == 0)
			protofile = "Prototype";
		else {
			progerr(ERR_PROTOTYPE);
			quit(1);
		}
	}

	if(devtype(device, &pkgdev)) {
		progerr(ERR_BADDEV, device);
		quit(1);
	}
	if(pkgdev.norewind) {
		/* initialize datastream */
		progerr(ERR_DSTREAM, device);
		quit(1);
	}
	if(pkgdev.mount) {
		if(pkgmount(&pkgdev, NULL, 0, 0, 1))
			quit(n);
	}

	/*
	 * convert prototype file to a pkgmap, while locating
	 * package objects in the current environment
	 */
	t_pkgmap = tempnam(tmpdir, "tmpmap");
	if(t_pkgmap == NULL) {
		progerr(ERR_TEMP, errno);
		exit(99);
	}

	if (!cflag)
		(void) pfmt(stdout, MM_NOSTD, MSG_PROTOTYPE);
	if(n = mkpkgmap(t_pkgmap, protofile, envparam)) {
		progerr(ERR_BUILD);
		quit(1);
	}
	if((fp = fopen(t_pkgmap, "r")) == NULL) {
		progerr(ERR_TEMP, errno);
		quit(99);
	}
	eptlist = procmap(fp, 0);
	if(eptlist == NULL)
		quit(1);
	(void) fclose(fp);

	/* unlink t_pkgmap file now and set its pointer to NULL
	   so quit() doesn't try to remvoe it again */

	(void) unlink(t_pkgmap);
	t_pkgmap=NULL;

	if (!xflag)
		(void) pfmt(stdout, MM_NOSTD, MSG_PKGINFO); 
	pt = NULL;
	for(i=0; eptlist[i]; i++) {
		ckmissing(eptlist[i]->path, eptlist[i]->ftype);
		if(eptlist[i]->ftype != 'i')
			continue;
		if(!strcmp(eptlist[i]->path, "pkginfo"))
			svept = eptlist[i];
		if(!strcmp(eptlist[i]->path, "setinfo"))
			seept = eptlist[i];
	}
	if(svept == NULL) {
		progerr(ERR_NOPKGINFO);
		quit(99);
	}
	eptnum = i;

	/*
	 * process all parameters from the pkginfo file
	 * and place them in the execution environment
	 */
	if((fp = fopen(svept->ainfo.local, "r")) == NULL) {
		progerr(ERR_RDPKGINFO, svept->ainfo.local);
		quit(99);
	}

	param[0] = '\0';
	while(value = fpkgparam(fp, param)) {
		if(getenv(param) == NULL) {
			/* 
			 * If the package instance was not specified on 
			 * command line, use the one in pkginfo file.
			 * Use pkginst as the value for PREDEPEND no matter
			 * what value is therein.  This is so that all
			 * packages will place a <pkginst>.name file in
			 * /usr/options for use by the displaypkg command.
			 */
			if(!pkginst) { 
				if(!strcmp(param, "PKG"))
					pkginst = strdup(value); 
			}
			if(!strcmp(param, "PREDEPEND")) {
				free((void *)value);
				param[0] = '\0';
				continue;
			}
			putparam(param, value);

		}
		free((void *)value);
		param[0] = '\0';
	}
	(void) fclose(fp);

	putparam("PREDEPEND", pkginst);

	/* make sure parameters are valid */
	(void) time(&clock);
	if(pt = getenv("PKG")) {
		if(pkgnmchk(pt, NULL, 0) || strchr(pt, '.')) {
			progerr(ERR_PKGABRV, pt);
			quit(1);
		}
		if(pkginst == NULL)
			pkginst = pt;
	} else {
		progerr(ERR_NOPARAM, "PKG", svept->path);
		quit(1);
	}
	/*
	 * verify consistency between PKG parameter and pkginst 
	 */
	(void) sprintf(param, "%s.*", pt);
	if(pkgnmchk(pkginst, param, 0)) {
		progerr(ERR_PKGMTCH, pt, pkginst);
		quit(1);
	}
	if((pkgname = getenv("NAME")) == NULL) {
		progerr(ERR_NOPARAM, "NAME", svept->path);
		quit(1);
	}
	if(ckparam("NAME", pkgname)) 
		quit(1);
	if((pkgvers = getenv("VERSION")) == NULL) {
		progerr(ERR_NOPARAM, "VERSION", svept->path);
		quit(1);
	}
	if(ckparam("VERSION", pkgvers)) 
		quit(1);
	if((pkgarch = getenv("ARCH")) == NULL) {
		progerr(ERR_NOPARAM, "ARCH", svept->path);
		quit(1);
	}
	if(ckparam("ARCH", pkgarch)) 
		quit(1);
	if(getenv("PSTAMP") == NULL) {
		/* use octal value of '%' to fight sccs expansion */
		(void) cftime(buf, gettxt(":482", "%y%m%d\045H\045M\045S"), &clock);
		(void) uname(&utsbuf);
		(void) sprintf(temp, "%s%s", utsbuf.nodename, buf);
		putparam("PSTAMP", temp);
		logerr(WRN_SETPARAM, "PSTAMP", temp);
	}
	if((pkgcat = getenv("CATEGORY")) == NULL) {
		progerr(ERR_NOPARAM, "CATEGORY", svept->path);
		quit(1);
	}
	if(ckparam("CATEGORY", pkgcat)) 
		quit(1);

	/* 
	 * inspect the destination directory to determine if any 
	 * instances of the package being created already exist 
	 */
	pkgdir = pkgdev.dirname;
	if((overwrite == 0) && (strchr(pkginst, '.') == NULL)) {
		npkgs = 0;
		prvinfo = (struct pkginfo *) calloc(MINSTSIZ, sizeof(struct pkginfo));
		if(prvinfo == NULL) {
			progerr(ERR_MEMORY, errno);
			quit(99);
		}
		for(;;) {
			if(pkginfo(&prvinfo[npkgs], param, NULL, NULL)) {
				if((errno == ESRCH) || (errno == ENOENT)) 
					break;
				progerr(ERR_SYSINFO, errno);
				quit(99);
			}
			if((++npkgs % MINSTSIZ) == 0) {
				prvinfo = (struct pkginfo *) realloc(prvinfo, 
					(npkgs+MINSTSIZ) * sizeof(struct pkginfo));
				if(prvinfo == NULL) {
					progerr(ERR_MEMORY, errno);
					quit(99);
				}
			}
		}
	
		if(npkgs > 0) {
			/* 
		 	* since an instance of this package already exists on
		 	* the system, we will create a new instance
		 	*/
			pkginst = getinst(pkginst, prvinfo, npkgs); 
		}
	}

	/* 
	 * If compression is requested, make mirror image of package in temporary
	 * area and use that area (directory) as the argument to -r when we re-exec
	 * pkgmk.  This simple approach takes advantage of this feature of pkgmk
	 * to now use the compressed mirror image as the root for the package.
	 * Also, we want to keep the original, non-compressed values for the check
	 * sum and size for each file that was compressed.  To do so, let's save 
	 * these in a temporary file and read them back when creating the final 
	 * pkgmap file.  This is done so that verification of the files, as they
	 * are being installed on the system when pkgadd is used, is based on the
	 * file as it will look when it is placed on the system (uncompressed).
	 */
	if (cflag) { 
		if(n = comprpkg(eptlist)) { 
			(void) progerr(ERR_COMPRESS); 
			(void) quit(1); 
		}
		cflag--;
		(void) sprintf(fname, "%s/svlist%d", tmpdir, pid);
		if ((sfp = fopen(fname, "w")) == NULL) {
			progerr(":483:could not save compressed list\n"); 
			quit(1); 
		}
		for(i=0; i < eptnum; i++) {  
			(void) fprintf(sfp, "%d %d\n",  
				eptlist[i]->cinfo.cksum, eptlist[i]->cinfo.size); 
		}
		(void) fclose(sfp);
		(void) pfmt(stdout, MM_NOSTD, MSG_COMPROC);
		if (rflag)
			(void) sprintf(cmdline, "%s -x %s -r %s %s", cmdpath, rootlist[0], cdir, savopt);
		else
			(void) sprintf(cmdline, "%s -x / -r %s %s", cmdpath, cdir, savopt);
		/*
		 * Append command line variable=value .. arguments
		 * to the about to be reexec'ed pkgmk command line.
		 */
		for(i=0; envparam[i]; i++) {
			(void) strcat(cmdline, " ");
			(void) strcat(cmdline, envparam[i]);
		}
		(void) strcat(cmdline, " ");
		(void) strcat(cmdline, pkginst);

		/*
		 * Make tmpdir known to child pkgmk.
		 */
		(void) sprintf(tdirenv, "TMPDIR=%s", tmpdir);
		(void) putenv(tdirenv);

		if(pkgdev.mount && pkgdev.mntflg && pkgumount(&pkgdev)) {
			progerr(ERR_UMOUNT, pkgdev.mount);
			quit(99);
		}
		(void) execl("/bin/sh", "/bin/sh", "-c", cmdline, NULL);
		progerr(":484:exec of <%s> failed, errno=%d", cmdline, errno);
	}

	/*
	 * If this is the reinvocation of pkgmk for the compressed image of 
	 * the package, then lets build correct path names to files and
	 * directories created by parent using its process id.
	 */
	if (xflag) {
		(void) sprintf(fname, "%s/svlist%d", tmpdir, xpid);
		(void) sprintf(cuniq, "%s/clist%d.uniq", tmpdir, xpid);
		(void) sprintf(cdir, "%s/cdir%d", tmpdir, xpid);
		(void) sprintf(cfile, "%s/clist%d", tmpdir, xpid);
	}

	/*
	 * warn user of classes listed in package which do
	 * not appear in CLASSES variable in pkginfo file
	 */
	objects = 0;
	allclass = (char **) calloc(MALSIZ, sizeof(char *));
	for(i=0; eptlist[i]; i++) {
		if(eptlist[i]->ftype != 'i') {
			objects++;
			addclass(eptlist[i]->class);
		}
	}

	pt = getenv("CLASSES");
	if(pt == NULL) {
		class = allclass;
		if(class[0]) {
			j = 1; /* room for ending null */
			for(i=0; class[i]; i++)
				j += strlen(class[i]) + 1;
			pt = (char *) calloc((unsigned)j, sizeof(char));
			(void) strcpy(pt, class[0]);
			for(i=1; class[i]; i++) {
				(void) strcat(pt, " ");
				(void) strcat(pt, class[i]);
			}
			logerr(WRN_SETPARAM, "CLASSES", pt);
			putparam("CLASSES", pt);
			free((void *)pt);
		}
	} else {
		(void) setlist(&class, qstrdup(pt));
		for(i=0; allclass[i]; i++) {
			found = 0;
			for(j=0; class[j]; j++) {
				if(!strcmp(class[j], allclass[i])) {
					found++;
					break;
				}
			}
			if(!found)
				logerr(WRN_CLASSES, allclass[i]);
		}
	}

	(void) pfmt(stdout, MM_NOSTD, MSG_VOLUMIZE, objects);
	order = (char **)0;
	if(pt = getenv("ORDER")) {
		pt = qstrdup(pt);
		(void) setlist(&order, pt);
	}

	/* stat the intended output filesystem to get blocking information */
	if(pkgdev.dirname == (char *)NULL) {
		progerr(ERR_WHATVFS, device);
		quit(99);
	}
	if(statvfs(pkgdev.dirname, &statvfsbuf)) {
		progerr(ERR_STATVFS, pkgdev.dirname);
		quit(99);
	}

	if(bsize == 0)
		bsize = statvfsbuf.f_bsize;
	if(frsize == 0)
		frsize = statvfsbuf.f_frsize;
	if(limit == 0)
		/* bavail is interms of fragment size blocks - change
		 * to 512 byte blocks
		 */
		limit = statvfsbuf.f_bavail * (((frsize - 1) / PBLK) + 1);
	if(ilimit == 0)
		ilimit = statvfsbuf.f_favail;

	nparts = splpkgmap(eptlist, eptnum, order, frsize, &limit, &ilimit, &llimit);
	if(nparts <= 0) {
		progerr(ERR_SPLIT);
		quit(1);
	}

	if(nflag) {
		for(i=0; eptlist[i]; i++)
			(void) ppkgmap(eptlist[i], stdout);
		exit(0);
		/*NOTREACHED*/
	} 

	(void) sprintf(pkgloc, "%s/%s", pkgdev.dirname, pkginst);
	if(!isdir(pkgloc) && !overwrite) {
		progerr(ERR_OVERWRITE, pkgloc);
		quit(1);
	}

	/* output all environment parameters */
	t_pkginfo = tempnam(tmpdir, "pkginfo");
	if((fp = fopen(t_pkginfo, "w")) == NULL) {
		progerr(ERR_TEMP, errno);
		exit(99);
	}
	

	for(i=0; environ[i]; i++) {
		(void) fputs(environ[i], fp);
		(void) fputc('\n', fp);
	}

	/*
	 * If the xflag is set, then we know we've compressed this package.
	 * Place the parameter in the package pkginfo file that will inform
	 * pkgadd to uncompress the package during installation.
	 */
	if (xflag) {
		(void) fputs("COMPRESSED=true", fp);
		(void) fputc('\n', fp);
	}

	(void) fclose(fp);

	started++;
	(void) rrmdir(pkgloc);
	if(mkdir(pkgloc, 0755)) {
		progerr(ERR_MKDIR, pkgloc);
		quit(1);
	}

	/* determine how many packages already reside on the medium */
	npkgs = 0;
	while(pt = fpkginst("all", NULL, NULL))
		npkgs++;
	(void) fpkginst(NULL); /* free resource usage */

	if(nparts > 1) {
		/*if(!limit && !pkgdev.mount) {
			* if no limit was specified and 
			 * the output device is a directory,
			 * we exceed the free space available
			 *
			progerr(ERR_FREE, pkgloc);
			quit(1);
		}*/
		if(pkgdev.mount && npkgs) {
			progerr(ERR_ONEVOL);
			quit(1);
		}
	}

	/*
	 *  update pkgmap entry for pkginfo file, since it may
	 *  have changed due to command line or failure to
	 *  specify all neccessary parameters
	 */
	for(i=0; eptlist[i]; i++) {
		if(eptlist[i]->ftype != 'i')
			continue;
		if(!strcmp(eptlist[i]->path, "pkginfo")) {
			svept = eptlist[i];
			svept->ftype = '?';
			svept->ainfo.local = t_pkginfo;
			(void) cverify(0, &svept->ftype, t_pkginfo, 
				&svept->cinfo);
			svept->ftype = 'i';
			break;
		}
	}
	/*
	 * If this is the reinvoked pkgmk process for the compressed image
	 * of the package, read in the original check sum and size values
	 * saved by the pkgmk that invoked this one.
	 */
	if (xflag)
		/* 
		 * Open file containing original check sum and size values.
		 */
		if ((sfp = fopen(listfile, "r")) == NULL) {
			progerr(":485:could not open compressed list file <%s>", listfile);	
			quit(1);
		}

	for(part=1; part <= nparts; part++) {
		if((part > 1) && pkgdev.mount) {
			if(pkgumount(&pkgdev)) {
				progerr(ERR_UMOUNT, pkgdev.mount);
				quit(99);
			}
			if(n = pkgmount(&pkgdev, NULL, part, nparts, 1))
				quit(n);
			(void)rrmdir(pkgloc);
			if(mkdir(pkgloc, 0555)) {
				progerr(ERR_MKDIR, pkgloc);
				quit(99);
			}
		}
		outvol(eptlist, eptnum, part, nparts);
	}
	if (xflag)
		(void) fclose(sfp);

	(void) quit(0);
	/*NOTREACHED*/
}

static void
trap(n)
int n;
{
	(void) signal(SIGINT, SIG_IGN);
	(void) signal(SIGHUP, SIG_IGN);

	if(n == SIGINT)
		quit(3);
	else {
		(void) pfmt(stderr, MM_NOSTD, ":486:%s terminated (signal %d).\n", prog, n);
		quit(99);
	}
}

static void
outvol(eptlist, eptnum, part, nparts)
struct cfent	**eptlist;
int	eptnum;
int	part, nparts;
{
	FILE	*fp;
	char	*svpt, *path, temp[PATH_MAX];
	int	i;
	int	sum, size;

	if(part == 1) {
		/* re-write pkgmap, but exclude local pathnames */
		(void) sprintf(temp, "%s/pkgmap", pkgloc);
		if((fp = fopen(temp, "w")) == NULL) {
			progerr(ERR_TEMP, errno);
			quit(99);
		}

		(void) fprintf(fp, ": %d %ld\n", nparts, limit);
		for(i=0; eptlist[i]; i++) {
			svpt = eptlist[i]->ainfo.local;
			if(!strchr("sl", eptlist[i]->ftype))
				eptlist[i]->ainfo.local = NULL;

			/*
			 * If we compressed files then place saved
			 * original cksum and size values into pkgmap.
			 */
			if (xflag) {
				(void) fscanf(sfp, "%d %dn", &sum, &size);
				switch(eptlist[i]->ftype) {
					case 'f':
					case 'e':
					case 'v':
						eptlist[i]->cinfo.cksum = sum;
						eptlist[i]->cinfo.size = size;
						break;
					default:
						break;
				}
			}
			if(ppkgmap(eptlist[i], fp)) {
				progerr(ERR_TEMP, errno);
				quit(99);
			}
			eptlist[i]->ainfo.local = svpt;
		}
		(void) fclose(fp);
		(void) fprintf(stdout, "%s\n", temp);
	}

	(void) sprintf(temp, "%s/pkginfo", pkgloc);
	if(copyf(svept->ainfo.local, temp, svept->cinfo.modtime))
		quit(1);
	(void) fprintf(stdout, "%s\n", temp);

	if (seept) {
		(void) sprintf(temp, "%s/setinfo", pkgloc);
		if(copyf(seept->ainfo.local, temp, seept->cinfo.modtime))
			quit(1);
		(void) fprintf(stdout, "%s\n", temp);
	}

	for(i=0; i < eptnum; i++) {
		if(eptlist[i]->volno != part)
			continue;
		if(strchr("dxslcbp", eptlist[i]->ftype))
			continue;
		if(eptlist[i]->ftype == 'i') {
			if(eptlist[i] == svept)
				continue; /* don't copy pkginfo file */
			if(eptlist[i] == seept)
				continue; /* don't copy setinfo file */
			(void) sprintf(temp, "%s/install/%s", pkgloc, 
				eptlist[i]->path);
			path = temp;
		} else
			path = srcpath(pkgloc, eptlist[i]->path, part, nparts);
		if(sflag) {
			if(slinkf(eptlist[i]->ainfo.local, path))
				quit(1);
		} else if(copyf(eptlist[i]->ainfo.local, path, eptlist[i]->cinfo.modtime))
				quit(1);
		(void) fprintf(stdout, "%s\n", path);
	}
}

static void
addclass(cl)
char	*cl;
{
	int	i;

	for(i=0; allclass[i]; i++)
		if(!strcmp(cl, allclass[i]))
			return;

	allclass[i] = qstrdup(cl);
	if((++i % MALSIZ) == 0) {
		allclass = (char **) realloc((void *)allclass, 
			(i+MALSIZ) * sizeof(char *));
		if(allclass == NULL) {
			progerr(ERR_MEMORY, errno);
			quit(99);
		}
	}
	allclass[i] = (char *)NULL;
}

static void
ckmissing(path, type)
char *path, type;
{
	static char	**dir;
	static int	ndir;
	char	*pt;
	int	i, found;

	if(dir == NULL) {
		dir = (char **) calloc(MALSIZ, sizeof(char *));
		if(dir == NULL) {
			progerr(ERR_MEMORY, errno);
			quit(99);
		}
	}

	if(strchr("dx", type)) {
		dir[ndir] = path;
		if((++ndir % MALSIZ) == 0) {
			dir = (char **) realloc((void *)dir, 
				(ndir+MALSIZ)*sizeof(char *));
			if(dir == NULL) {
				progerr(ERR_MEMORY, errno);
				quit(99);
			}
		}
		dir[ndir] = (char *)NULL;
	}

	pt = path;
	if(*pt == '/')
		pt++;
	while(pt = strchr(pt, '/')) {
		*pt = '\0';
		found = 0;
		for(i=0; i < ndir; i++) {
			if(!strcmp(path, dir[i])) {
				found++;
				break;
			}
		}
		if(!found) {
			logerr(WRN_MISSINGDIR, path);
			ckmissing(qstrdup(path), 'd');
		}
		*pt++ = '/';
	}
}

static int
slinkf(from, to)
char	*from, *to;
{
	char	*pt;

	pt = to;
	while(pt = strchr(pt+1, '/')) {
		*pt = '\0';
		if(isdir(to) && mkdir(to, 0755)) {
			progerr(ERR_MKDIR, to);
			*pt = '/';
			return(-1);
		}
		*pt = '/';
	}
	if(symlink(from, to)) {
		progerr(ERR_SYMLINK, to);
		return(-1);
	}
	return(0);
}

static void
usage()
{
	/*
	 * Don't advertise the -x option.
	 */
	(void) pfmt(stderr, MM_ACTION,
		":487:usage: %s [options] [VAR=value [VAR=value]] [pkginst]\n",
		prog);
	(void) pfmt(stderr, MM_NOSTD, ":488:   where options may include:\n");
	(void) pfmt(stderr, MM_NOSTD, ":489:\t-o\n");
	(void) pfmt(stderr, MM_NOSTD, ":490:\t-a arch\n");
	(void) pfmt(stderr, MM_NOSTD, ":491:\t-c\n");
	(void) pfmt(stderr, MM_NOSTD, ":775:\t-B blksize\n");
	(void) pfmt(stderr, MM_NOSTD, ":492:\t-v version\n");
	(void) pfmt(stderr, MM_NOSTD, ":493:\t-p pstamp\n");
	(void) pfmt(stderr, MM_NOSTD, ":494:\t-l limit\n");
	(void) pfmt(stderr, MM_NOSTD, ":495:\t-r rootpath\n");
	(void) pfmt(stderr, MM_NOSTD, ":496:\t-b basedir\n");
	(void) pfmt(stderr, MM_NOSTD, ":497:\t-d device\n");
	(void) pfmt(stderr, MM_NOSTD, ":498:\t-f protofile\n");
	(void) pfmt(stderr, MM_NOSTD, ":805:\t-k\n");
	quit(1);
	/*NOTREACHED*/
}



/*
 * The comprpkg() routine is used when the -c option is specified for
 * creating a package with compressed files.  The only files upon which
 * compression is attempted are those that are of type 'e', 'f' and 'v'
 * (see prototype(4)).  We don't want to compress files of type 'i' since
 * these are used during the installation process itself.  If no file
 * size reduction results from the compression, the original uncompressed
 * version of the file is copied over to the mirror image of the package
 * and a warning message produced.
 */

extern	char *dirname();

static int
comprpkg(eptlist)
struct cfent **eptlist;
{
	int i;
	char cmd[BUFSIZ];		/* cpio command	*/
	char comp[BUFSIZ];		/* compress command */
	char comprf[PATH_MAX];		/* name of file to be compressed for err msg */
	char dirnm[PATH_MAX];		/* name of directory to be created for mirror image */
	char *svdir;			/* if -r OPTION used, save current dir here */
	char directory[PATH_MAX];	/* name of directory to create */
	char *dirp;			/* pointer to directory name from dirname() */
	int n;				/* return value from esystem() */

	/*
	 * Create list of files to be copied to mirror image.  If the -r option was used
	 * to set a rootpath, use the relative pathname of the file from that directory on.
	 *
	 * 	 e.g.,	filename is: /home/foo/pkg/root/usr/bin/foo
	 * 		rootlist[0] is: /home/foo/pkg/root 
	 * 	  	pathname is: usr/bin/foo
	 */
	(void) sprintf(cfile, "%s/clist%d", tmpdir, pid);
	if((clist = fopen(cfile, "w")) == NULL) {
		progerr(ERR_CFILE, cfile);
		quit(1);
	}

	/*
	 * If length of rootlist is not greater than 1, then
	 * we know that it is "/".  For this case, don't incrament
	 * past this "/".
	 */
	if (rflag > 1)
		rflag++;	/* go beyond '/' */
	for(i=0; eptlist[i]; i++) {
		if (eptlist[i]->ainfo.local != NULL && eptlist[i]->ftype != 'i'
			&& eptlist[i]->ftype != 'l' && eptlist[i]->ftype != 's'
			&& eptlist[i]->ftype != 'd') {
			/*
			 * If the file being packaged is specified by a relative pathname
			 * that is above the rootlist specified on the command line, we
			 * cannot predict where the compressed mirror image's directory
			 * for this entry will wind up being created.   Given the following
			 * prototype entries,
			 *
			 *	f none /usr/bin/foo1=../src/foo1
			 *	f none /usr/bin/foo2=../../src/foo2
			 *	f none /usr/sbin/foo3=../../../src/foo3
			 *
			 * and the pkgmk command line,
			 *
			 *	pkgmk -r /home/me/root -c .. ... 
			 *
			 * If TMPDIR is set to /tmp, then cdir becomes '/tmp/cdir<pid>'.
			 * Trying to create directory '/tmp/cdir<pid>/../../src' and
			 * '/tmp/cdir<pid>/../../../src' wil result in the directory
			 * '/src' being created, while the the first entry's directory
			 * would wind up being '/tmp/src'.   Because of this, entries
			 * referencing relative paths above the roolist specification
			 * will not be placed into the mirror image and therefore not
			 * compressed.
			 *
			 * These entries's pathnames never begin the rootlist so we know
			 * we can pass them by.
			 */
			if( rootlist[0] == NULL || strncmp(eptlist[i]->ainfo.local, rootlist[0], strlen(rootlist[0])) != 0)
				continue;

			/*
			 * Create a package directory list to be used for creating the
			 * mirror image for compression.  Since dirname() may corrupt
			 * the original pathname, copy it over to a temporary place.
			 */
			if (rflag > 1) 
				(void) sprintf(directory, "%s", eptlist[i]->ainfo.local+rflag); 
			else 
				(void) sprintf(directory, "%s", eptlist[i]->ainfo.local);
			if (strcmp((dirp = dirname(directory)), ".") != 0)
				(void) fprintf(clist, "%s\n", dirp);
		}
	}
	(void) fclose(clist);

	/*
	 * Sort and retain unique directory names in cfile.
	 */
	(void) sprintf(cmd, "sort %s | uniq >%s.uniq", cfile, cfile);
	if(n = esystem(cmd, -1, -1)) {
		rpterr();
		progerr(ERR_UNIQ);
		quit(1);
	}

	/*
	 * If rootlist was defined, then change directory to
	 * that directory before creating directories in cdir.
	 */
	if (rflag) {
		if((svdir = getcwd(NULL, PATH_MAX)) == NULL) {
			progerr(ERR_PWD);
			quit(1);
		}
		chdir(rootlist[0]);
	}

	/*
	 * Create directories for mirror image of package.
	 */
	(void) sprintf(cuniq, "%s/clist%d.uniq", tmpdir, pid);
	if((clist = fopen(cuniq, "r")) == NULL) {
		progerr(ERR_CFILE, cfile);
		quit(1);
	}
	(void) pfmt(stdout, MM_NOSTD, MSG_MIRROR);
	/*
	 * Get sorted list of directory names to create.
	 */
	for(i=0; fgets(directory, PATH_MAX, clist) != NULL; i++) {
		*strrchr(directory,'\n')='\0';	/* replace newline with null */
		(void) sprintf(dirnm, "%s/%s", cdir, directory);
		/*
		 * Create directory in mirror image
		 * only if it doesn't exist already.
		 */
		if(stat(dirnm, &dirstat) < 0) {
			if(errno == ENOENT) {
			 	(void) sprintf(cmd, "mkdir -p %s", dirnm);
				if(n = esystem(cmd, -1, -1)) {
					rpterr();	/* report error from esystem() */ 
					progerr(ERR_MKDIR, dirnm);
					quit(1);
				}
			}
		}
	}
	(void) fclose(clist);
	(void) unlink(cuniq);

	/*
	 * After changing directory to cdir to make mirror image
	 * directories, return to directory where package is.
	 */
	if (rflag)
		chdir(svdir);
	/*
	 * Compress file types 'e', 'f' and 'v' only.
	 */
	(void) pfmt(stdout, MM_NOSTD, MSG_COMPRESS);
	for(i=0; eptlist[i]; i++) {
		switch(eptlist[i]->ftype) {
			case 'f':
			case 'v':
			case 'e':

				/*
				 * If this is an entry referencing a relative pathname above
				 * the rootlist, skip compression - it will be copied based
				 * the first invocation's prototype entry local path.
				 */
				if( rootlist[0] == NULL || strncmp(eptlist[i]->ainfo.local, rootlist[0], strlen(rootlist[0])) != 0) {
					logerr(WRN_RELPATH, eptlist[i]->ainfo.local);
					continue;
				}

				/*
				 * Make sure that we can read the file that is about to 
				 * be compressed.  If not produce error message and abort.
				 */
				if(access(eptlist[i]->ainfo.local, R_OK) < 0) {
					progerr(ERR_READF, eptlist[i]->ainfo.local);
					quit(1);
				}
				if (rflag > 1) {
					(void) sprintf(comprf, "%s/%s", cdir, eptlist[i]->ainfo.local+rflag);
					(void) sprintf(cmd, "/usr/bin/compress < \"%s\" > \"%s\"",
						eptlist[i]->ainfo.local, comprf);
				}
				else {
					(void) sprintf(comprf, "%s%s", cdir, eptlist[i]->ainfo.local);  
					(void) sprintf(cmd, "/usr/bin/compress < \"%s\" > \"%s\"",
						eptlist[i]->ainfo.local, comprf);
				}
				/*
				 * Attempt to compress the file.
				 */
				if(n = esystem(cmd, -1, -1)) {
					if(n == 2) {
						/*
						 * If no savings occurr from the compress, let's move
						 * the original uncompressed file to the mirror image.
						 */
						ecleanup();	/* remove errfile from esystem() */
						logerr(WRN_COMPR, eptlist[i]->ainfo.local);
						(void) sprintf(cmd, "/usr/bin/cp \"%s\" \"%s\"", 
							eptlist[i]->ainfo.local, comprf);
						if(n = esystem(cmd, -1, -1)) {
							rpterr(); /* report error from esystem() */
							progerr(ERR_CPFILE, eptlist[i]->ainfo.local);
							quit(1);
						}
					}
					else {
						/*
						 * Unexpected error during compress command.
						 */
						
						rpterr();
						progerr(ERR_COMPR, eptlist[i]->ainfo.local);
							quit(1);
					}
				}
		}
	}
	return(0);
}
