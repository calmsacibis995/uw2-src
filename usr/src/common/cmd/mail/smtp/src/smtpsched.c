/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/smtpsched.c	1.8.4.2"
#ident "@(#)smtpsched.c	1.22 'attmail mail(1) command'"
#include "libmail.h"
#include "smtp.h"
#include "xmail.h"
#include "s5sysexits.h"
#ifdef SVR4_1
#include <mac.h>
#endif
#include "sched_decl.h"

#if defined(SVR3)
#define	SYS5	1
static char RMAIL[] = "/bin/rmail";
static char smtpadmin[] = "root";	/* allowed to use -c, -r, and -w */
#endif

#if defined(SVR4) || defined(SUN41)
#define	SYS5	1
static char RMAIL[] = "/usr/bin/rmail";
static char smtpadmin[] = "smtp";	/* allowed to use -c, -r, and -w */
#endif

/*
 *  names of the limit files
 */
char CON[]	= ".smtpscheds";
char NCON[]	= ".nsmtpscheds";

#define	OLD	1*3600L		/* old C file -> try less often */
#define	VOLD	6*3600L		/* very old C file -> try much less often */
#define	OLDW	1*3600L		/* wait time between tries at old files */
#define	VOLDW	4*3600L		/* wait time between tries at very old files */

extern char *UPASROOT;
int warn = -1;
int removedays = -1;
int cleanup;
int verbose;
int testmode;
int Xonly = 0;
int Conly = 0;
int batchjobs = 0;
string *replyaddr;
string *dest;
int mypid;

extern char **getcmd proto((char*,char*));
extern int dodirdir proto((char *dname, int action, char *direction));

#define	MAXDEST 50		/* # destinations remembered */
#define	MAXTPERD 5*60		/* total time allowed before skipping dests */
struct {
	string	*dest;
	time_t	time;
} destlist[MAXDEST];		/* time consumed by unsuccessful tries at dest */
int	ndest = 0;

int	debug;


/*
 *  actions to take when locking
 */
#define BLOCK 0		/* wait for 5 minutes if the directory is locked */
#define SKIP 1		/* skip the directory if it is locked */
#define IGNORE 2	/* don't lock the directory or care if it is */

/*
 *  If called with arguments, those arguments are spool directories.  Descend
 *  each one processing the control files in it (C.* and X.*).
 *
 *  If called without arguments, descend all spool directories.
 *
 *  -B		batch multiple jobs to one host in one conversation.
 *  -s #scheds  specifies a maximum number of concurrent smtpscheds.
 *  -w #days	causes users to be warned if their mail files are older
 *		than #days days
 *  -r #days	causes mail older than #days days to be returned to sender
 *  -c		cleanup empty directories
 *  -D		debug
 *  -L level	logging level
 *
 *  Note: -c, -r, and -w are restricted to the "smtp admin" login (either "root"
 *  or "smtp".
 */

const char *progname = "smtpsched";

usage()
{
	pfmt(stderr, MM_ACTION, ":81:[-cvtBDL] [-w #days] [-r #days] [-s #scheds] [dir]\n");
	exit(1);
}

int smtpuser()
{
	struct passwd *pw = getpwuid(getuid());
	return !strcmp(pw->pw_name, smtpadmin);
}

main(ac, av)
	int ac;
	char *av[];
{
	DIR *dirp;
	Direct *dp;
	int max=0;
	int c;
	extern int optind;
	extern char *optarg;

	umask(2);

	mypid = getpid();

	/*
	 *  avoid annoying distractions
	 */
	signal(SIGPIPE, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(SIGCLD, SIG_DFL); /* Allow us to wait on children */

#ifdef SVR4_1
	(void) setcat("uxsmtp");
	(void) setlabel("UX:smtpsched");
	(void) setlocale(LC_ALL, "");
	(void) mldmode(MLD_VIRT);
#endif
	Openlog("smtpsched", LOG_PID, LOG_SMTPSCHED);
	setlogmask(LOG_UPTO(LOG_INFO));

	while ((c = getopt(ac, av, "XBCDcvtr:w:s:dL:")) != EOF)
		switch (c) {
		case 'X':	Xonly = 1;		break;
		case 'C':	Conly = 1;		break;
		case 'B':	batchjobs = 1;		break;
		case 't':	testmode = 1;		break;
		case 'v':	verbose = 1;		break;
		case 'c':
			if (!smtpuser()) {
				pfmt(stderr, MM_ACTION, ":17:only '%s' may use the '-c' option\n", smtpadmin);
				exit(1);
			}
			cleanup = 1;
			break;
		case 'r':
			if (!smtpuser()) {
				pfmt(stderr, MM_ACTION, ":18:only '%s' may use the '-r' option\n", smtpadmin);
				exit(1);
			}
			removedays = atoi(optarg);
			break;
		case 's':	max = atoi(optarg);	break;
		case 'w':
			if (!smtpuser()) {
				pfmt(stderr, MM_ACTION, ":19:only '%s' may use the '-w' option\n", smtpadmin);
				exit(1);
			}
			warn = atoi(optarg);
			break;
		case 'L':	setloglevel(optarg);	break;
		case 'D':
		case 'd':	debug = 1;		break;
		default:	usage();
		}

	/*
	 *  go to top spool directory
	 */
	if (chdir(SMTPQROOT) < 0) {
		Syslog(LOG_ALERT, "can't chdir to %s\n", SMTPQROOT);
		exit(1);
	}

	/*
	 *  if there are too many running exit
	 */
	if (max && toomany(max) < 0)
		exit(0);

	/*
	 *  If specific directories, do just them.  Keep running the directory until there
	 *  is no change.
	 */
	if (optind != ac) {
		for (; optind < ac; optind++)
			while (dodir(av[optind], SKIP) && !warn && !removedays)
				;
		return 0;
	}

	/*
	 *  walk through all directories in top directory.  the lock is
	 *  non-blocking (if neither r nor w options specified) to let
	 *  different instances of smtpsched to skip over each other.
	 */
	dirp = opendir(".");
	if (dirp == NULL) {
		Syslog(LOG_ALERT, "couldn't read %s\n", SMTPQROOT);
		exit(1);
	}
	while (dp = readdir(dirp)) {
		if(strcmp(dp->d_name, ".")!=0 && strcmp(dp->d_name, "..")!=0)
			dodir(dp->d_name, (warn>=0 || removedays>=0) ? IGNORE : SKIP);
	}
	closedir(dirp);

	/* Rename Log file nightly */
	if (cleanup) {
		register struct tm *tp;
		char nm[256];
		time_t now;

		(void) time(&now);
		tp = localtime(&now);
		(void) sprintf(nm, "LOG.%d", tp->tm_wday);
		(void) unlink(nm);
		(void) rename("LOG", nm);
	}

	return 0;
}

/*
 *  do both directions in a directory
 */
dodir(dname, action)
	char *dname;
{
	int i, err;
	static string *ds;
	struct stat buf;

	if ((err=stat(dname, &buf)) < 0)
		return 0;

	if (!(buf.st_mode & S_IFDIR))
		return 0;

	ds = s_reset(ds);
	s_append(ds, dname);

	if (debug)
		fprintf(stderr, "Checking %s\n", dname);
	i = dodirdir(s_to_c(ds), action, "X.");
	i += dodirdir(s_to_c(ds), action, "C.");
	return i;
}

/*
 *  walk through all entries in this directory.  process any
 *  not starting with '.'.  lock the directory before proceeding.
 */
dodirdir(dname, action, direction)
	char *dname;
	char *direction;
{
	DIR *dirp;
	Direct *dp;
	int i;
	int changed=0;
	int ents=0, files=0;
	static string *ls;

	ls = s_reset(ls);
	s_append(ls, direction);
	s_append(ls, dname);

	/*
	 *  lock the directory.  the lock is in the parent directory.
	 */
	switch(action){
	case BLOCK:
		for(i=0; i<3; i++){
			if(lock(s_to_c(ls))==0)
				break;
			if (debug)
				Syslog(LOG_DEBUG, "pausing for lock");
			sleep(5);
		}
		if(i==3)
			return changed;
		break;
	case SKIP:
		if(lock(s_to_c(ls))<0){
			Syslog(LOG_DEBUG, "couldn't lock %s\n", dname);
			return changed;
		}
		break;
	case IGNORE:
		break;
	}

	/*
	 *  descend into the directory.  if it isn't a directory,
	 *  this will fail.
	 */
	if(chdir(dname)<0){
		if(action != IGNORE)
			unlock(s_to_c(ls));
		return changed;
	}

	/*
	 *  walk through the entries
	 */
	dirp = opendir(".");
	if (dirp == NULL) {
		Syslog(LOG_INFO, "couldn't read directory %s\n", dname);
		if(chdir(SMTPQROOT)<0){
			Syslog(LOG_ALERT, "can't chdir back to SMTPQROOT\n");
			exit(1);
		}
		if(action != IGNORE)
			unlock(s_to_c(ls));
		return changed;
	}
	while (dp = readdir(dirp)) {
		if (strcmp(dp->d_name, ".")==0 || strcmp(dp->d_name, "..")==0)
			continue;
		files++;
		if (cleanup)
			continue;
		if (dp->d_name[0] == *direction) {
			switch (dofile(dname, dp->d_name)) {
			case 0:
				/* file removed */
				changed = 1;
				break;
			case 1:
				/* file left alone */
				ents += 1;
				break;
			}
		}
	}
	closedir(dirp);

	/*
	 *  If we are batching jobs, we now have a list of C. files
	 *  which should be batched.
	 */
	if (batchjobs)
		dobatch();

	/*
	 *  go back up.  symbolic links could be painful!!!!
	 */
	if(chdir(SMTPQROOT)<0){
		Syslog(LOG_ALERT, "Can't chdir back to SMTPQROOT\n");
		exit(1);
	}

	/*
	 *  cleanup empty directories
	 */
	if(cleanup && files==0){
		Syslog(LOG_DEBUG, "%s empty\n", dname);
		if(rmdir(dname)<0)
			Syslog(LOG_ALERT, "can't unlink: %d\n", errno);
	}

	if(action != IGNORE)
		unlock(s_to_c(ls));

	return changed;
}

/*
 *  process a spool control file.  control file names start with C. or
 *  X.  all error goes into an error file.
 *
 *  return 0 if file removed, 1 otherwise.
 */
int dofile(dname, name)
	char *dname;
	char *name;
{
	int rv;
	int fd, ofd;
	char *ef;
	struct stat sb;
	time_t now, Edate, Cdate;

	rv = -1;

	/* Process files that are detected to be corrupt by smtp */
	if (name[0] == 'c' && name[1] == '.') {
		fclose(parseline1(name));
		returnmail(name, 2);
		Syslog(LOG_NOTICE, "%s/%s returned\n", dname, name);
		doremove(name);
		return 0;
	}

	/*
	 *  if the file is inconsistent, remove it
	 */
	if (cleanup && inconsistent(name)) {
		Syslog(LOG_NOTICE, "%s/%s inconsistent\n", dname, name);
		unlink(name);
		return 0;
	}

	/*
	 *  if this is not a control file, ignore it
	 */
	if (name[1] != '.' || (name[0] != 'C' && name[0] != 'X'))
		return 1;

	/*
	 *  if file is too old, warn user and remove it.  if checking age,
	 *  don't run the control file.
	 */
	if(warn>=0 || removedays>=0) {
		if(checkage(name)==0) {
			Syslog(LOG_NOTICE, "%s/%s too old\n", dname, name);
			doremove(name);
			return 0;
		}
		return 1;
	}

	/*
	 *  don't run control file when cleaning up
	 */
	if (cleanup)
		return 1;

	/*
	 * Backoff scheme: don't try old C files very often
	 */
	ef = fileoftype('E', name);
	if (name[0]=='C') {
		now = time((time_t *)0);
		Cdate = now;
		Edate = now-VOLDW-1;
		if (stat(name, &sb)==0)
			Cdate = sb.st_ctime;
		if (stat(ef, &sb)==0)
			Edate = sb.st_mtime;
		if (now-Cdate>VOLD && now-Edate<VOLDW
		 || now-Cdate>OLD  && now-Edate<OLDW) {
			if (verbose)
				Syslog(LOG_DEBUG, "ignore %s/%s: not time yet\n", dname, name);
			if (debug == 0) {
				/*
				 * If batching, we may send this anyway
				 * (if there are any "new" ctl files).
				 */
				if (batchjobs)
					addtobatch(name, 0);
				return 1;
			}
		}
	}

	/*
	 * in test mode, just return
	 */
	if (testmode) {
		Syslog(LOG_DEBUG, "would process %s/%s\n", dname, name);
		return 1;
	}

	/*
	 *  Save the names of C. files (if we are batching).
	 */
	if (batchjobs && (name[0] == 'C')) {
		addtobatch(name, 1);
		return 1;
	}

	/*
	 *  redirect output to an error file
	 */
	ofd = dup(2);
	close(2);
	fd = open(ef, 1);
	if(fd<0)
		fd = creat(ef, 0666);
	if(fd>=0){
		lseek(fd, 0l, 2);
	
		/*
		 *  process the file
		 */
		if(name[0]=='C') {
			rv = dosmtp(dname, name);
		} else if(name[0]=='X') {
			rv = dormail(dname, name);
		}

		/*
		 *  get old error file back
		 */
		close(2);
		(void) dup(ofd);
		close(ofd);
	}

	/*
	 *  if processing was successful, remove the spool files
	 */
	if (rv == 0) {
		doremove(name);
		return 0;
	}
	return 1;
}

/*
 *  remove the control file, data file, and error file
 */
doremove(ctl)
	char *ctl;
{
	fflush(stdout);
	unlink(fileoftype('E', ctl));
	unlink(ctl);
	unlink(fileoftype('D', ctl));
}

/*
 *  run rmail.  rmail takes care of its own errors, so if rmail fails,
 *  just don't remove the files.
 */
dormail(dname, ctl)
	char *dname;
	char *ctl;
{
	char **av;
	int rc;

	Syslog(LOG_NOTICE, "dormail %s/%s\n", dname, ctl);

	/*
	 *  fork off the command
	 */
	if ((av = getcmd(ctl, RMAIL)) == NULL) {
		Syslog(LOG_WARNING, "Could not get rmail params for %s", ctl);
		return -1;
	}

	if ((rc = docmd(ctl, av)) == 0){
		Syslog(LOG_DEBUG, "success");
		return 0;
	} else {
		Syslog(LOG_DEBUG, "failed, rc = %d", rc);
		return -1;
	}
}

/*
 *  run smtp.  if an error occurs, determine its importance and send
 *  a error mail message if it is fatal.
 */
dosmtp(dname, ctl)
	char *dname;
	char *ctl;
{
	static string *cmd;
	int status, i;
	char **av;
	time_t t0, t1;

	Syslog(LOG_INFO, "dosmtp %s/%s\n", dname, ctl);

	/*
	 *  fork off the command
	 */
	cmd = s_reset(cmd);
	s_append(cmd, UPASROOT);
	s_append(cmd, SMTP);
	av = getcmd(ctl, s_to_c(cmd));
	if (av==NULL) {
		Syslog(LOG_WARNING, "Could not get smtp params for %s", ctl);
		return -1;
	}
	/*
	 * Check whether unsuccessful attempts at this destination
	 * have taken too much time.  If so, pass over the file.
	 */
	for (i=0; i<ndest; i++) {
		if (strcmp(s_to_c(dest), s_to_c(destlist[i].dest))==0) {
			if (destlist[i].time > MAXTPERD) {
				Syslog(LOG_DEBUG, "passed %s (%d sec)\n", s_to_c(dest), destlist[i].time);
				pfmt(stderr, MM_INFO, ":83:Cannot contact destination\n");
				return -1;
			}
			break;
		}
	}
	if (i==ndest) {
		if (ndest<MAXDEST)
			ndest++;
		else
			i = 0;		/* loses storage */
/*
 * the following s_copy died on a malformed `C' file.  The
 * contents of these files should be checked more carefully. 
 */
		destlist[i].dest = s_copy(s_to_c(dest));
		destlist[i].time = 0;
	}
	if (debug)
		Syslog(LOG_INFO, "time %d for %s\n", destlist[i].time, s_to_c(dest));
	time(&t0);
	switch(status=docmd(ctl, av)){
	case 0:		/* it worked */
		Syslog(LOG_DEBUG, "success\n");
		destlist[i].time = 0;
		return 0;

	case EX_UNAVAILABLE:	/* service unavailable */
	case EX_NOPERM:		/* permission denied */
	case EX_NOUSER:		/* rejected by the other side */
	case EX_NOHOST:		/* host name unknown */
	case EX_DATAERR:	/* data format error */
	case EX_USAGE:		/* command line usage error */
		Syslog(LOG_INFO, "failed with status %d\n", status);
		returnmail(ctl, 1);		/*permanant failure*/
		destlist[i].time = 0;
		return 0;

	case EX_CANTCREAT:	/* can't create (user) output file */
	case EX_IOERR:		/* input/output error */
	case EX_OSERR:		/* system error (e.g., can't fork) */
	case EX_OSFILE:		/* critical OS file missing */
	case EX_SOFTWARE:	/* internal software error */
	case EX_NOINPUT:	/* cannot open input */
	case EX_PROTOCOL:	/* remote error in protocol */
		/* gauss is having flakey datakit errors that confuse the
		 * SMTP protocol.  EX_PROTOCOL is a temporary error for gauss-ches*/
	case EX_TEMPFAIL:	/* temp failure; user is invited to retry */
		Syslog(LOG_INFO, "temp fail with status %d\n", status);
		time(&t1);			/*temporary failure*/
		destlist[i].time += t1-t0;
		return -1;

	default:	/* possibly a temporary problem */
		Syslog(LOG_WARNING, "unknown fail with status %d\n", status);
		time(&t1);
		destlist[i].time += t1-t0;
		return -1;
	}
}

/*
 *  open a control file and parse the first line.  It contains
 *  the reply address and the destination (for returning the mail).
 *
 *  It leaves the control file open and returns the fp.
 */
FILE *
parseline1(ctl)
	char *ctl;
{
	FILE *fp;
	static string *line;

	fp = fopen(ctl, "r");
	if(fp==NULL)
		return NULL;

	/*
	 *  get reply address and destination
	 */
	line = s_reset(line);
	if(s_read_line(fp, line)==NULL){
		pfmt(stderr, MM_ERROR, ":5:error reading ctl file %s\n", ctl);
		fclose(fp);
		return NULL;
	}
	replyaddr = s_parse(s_restart(line), s_reset(replyaddr));
	if(replyaddr==NULL){
		pfmt(stderr, MM_ERROR, ":84:error reading ctl file replyaddr %s\n",
			ctl);
		fclose(fp);
		return NULL;
	}
	dest = s_parse(line, s_reset(dest));
	if(dest==NULL){
		pfmt(stderr, MM_ERROR, ":84:error reading ctl file dest %s\n",
			ctl);
		fclose(fp);
		return NULL;
	}
	return fp;
}

/*
 * Read control file to get arguments for command.  Leave dest and replyaddr
 * available.  The control file has two lines.  The first is reply address
 * and recipients.  the second is the arguments for the command.
 */
char **
getcmd(ctl, cmd)
	char *ctl;
	char *cmd;
{
	static string *args;
	FILE *fp;
	static char *av[1024];
	int ac=0;
	char *cp;

	fp = parseline1(ctl);
	if (fp==NULL)
		return NULL;

	/*
	 *  make command line
	 */
	av[ac++] = cmd;
	args = s_reset(args);
	if(s_read_line(fp, args)==NULL){
		pfmt(stderr, MM_ERROR, ":5:error reading ctl file %s\n", ctl);
		fclose(fp);
		return NULL;
	}
	fclose(fp);
	cp = s_to_c(args);
	cp[strlen(cp) - 1] = '\0';	/*zap the newline*/
	Syslog(LOG_INFO, "%s <%s", cmd, ctl);
	for (cp = s_to_c(args); *cp && ac<1023;){
		av[ac++] = cp++;
		while(*cp && !isspace(*cp))
			cp++;
		while(isspace(*cp))
			*cp++ = 0;
	}
	av[ac] = 0;
	return av;
}

/*
 *  execute a command, put standard error in the error file.
 */
docmd(ctl, av)
	char *ctl;
	char **av;
{
	int fd;
	int pid, status;
	int n;

	/*
	 *  fork off the command
	 */
	switch(pid = fork()){
	case -1:
		return -1;
	case 0:
		/*
		 *  make data file standard input
		 */
		close(0);
		fd = open(fileoftype('D', ctl), 0);
		if(fd<0){
			pfmt(stderr, MM_ERROR, ":45:smtpsched: error reading data file: %s\n", Strerror(errno));
			exit(1);
		}
	
		/*
		 *  make error file standard output
		 */
		close(1);
		fd = dup(2);
	
		/*
		 *  start the command
		 */
		execvp(av[0], av);
		exit(-2);
	default:
		/*
		 *  wait for the command to terminate
		 */
		while((n = wait(&status))>=0)
			if(n == pid)
				break;
		if(status&0xff)
			return -2;
		else
			return (status>>8)&0xff;
	}

}

/*
 *  see if the number of consumers has been exceeded.  if not, add this process
 *  to the list.
 *
 *  returns 0 if there were the number was not exceeded, -1 otherwise
 */
int toomany(max)
	int max;
{
	FILE *ifp=NULL;
	FILE *ofp=NULL;
	int cur=0;
	int pid;

	/*
	 *  lock consumers file
	 */
	if(lock(CON)<0)
		return -1;

	/*
	 *  open old and new consumer files
	 */
	ofp = fopen(NCON, "w");
	if(ofp==NULL){
		pfmt(stderr, MM_ERROR, ":21:can't open %s\n", NCON);
		goto error;
	}
	ifp = fopen(CON, "r");
	if(ifp!=NULL){
		/*
		 *  see how many consumers are still around
		 */
		while(fscanf(ifp, "%d", &pid)==1){
			if(kill(pid, 0) == 0){
				cur++;
				if(fprintf(ofp, "%d\n", pid)<0){
					pfmt(stderr, MM_ERROR, ":22:error writing %s\n", NCON);
					goto error;
				}
			}
		}
		if(cur >= max)
			goto error;
	}

	/*
	 *  add us to the group of consumers
	 */
	if(fprintf(ofp, "%d\n", getpid())<0){
		pfmt(stderr, MM_ERROR, ":22:error writing %s\n", NCON);
		goto error;
	}
	if(ifp!=NULL)
		fclose(ifp);
	if(fclose(ofp)==EOF)
		goto error;
	unlink(CON);
	if(rename(NCON, CON)<0)
		pfmt(stderr, MM_ERROR, ":23:can't rename %s to %s\n", NCON, CON);
	unlock(CON);
	return 0;
error:
	/*
	 *  too many consumers or we can't make a new consumer file
	 */
	if(ifp!=NULL)
		fclose(ifp);
	if(ofp!=NULL)
		fclose(ofp);
	unlink(NCON);
	unlock(CON);
	return -1;
}

/*
 *  return true if the file is inconsistent.  The following are inconsistent:
 *	- a control file without a datafile
 *	- an error file without a datafile
 *	- a day old data file without a control file
 *	- a limit file of any kind
 */
int inconsistent(file)
	char *file;
{
	struct stat s;
	int days;

	/*
	 *  switch on file type
	 */
	switch(file[0]){
	case 'C':
	case 'X':
		/*
		 *  if no data file, control file is inconsistent
		 */
		if(stat(fileoftype('D', file), &s)<0)
			return 1;
		break;
	case 'E':
		/*
		 *  if no control file, error file is inconsistent
		 */
		if(stat(fileoftype('X', file), &s)<0
		&& stat(fileoftype('C', file), &s)<0)
			return 1;

		/*
		 *  if no data file, error file is inconsistent
		 */
		if(stat(fileoftype('D', file), &s)<0)
			return 1;
		break;
	case 'D':
		/*
		 *  look for a control file
		 */
		if(stat(fileoftype('X', file), &s)==0
		|| stat(fileoftype('C', file), &s)==0)
			break;

		/*
		 *  no control file, data file inconsistent if >=1 day old
		 */
		if(stat(file, &s)<0)
			return 0;
		days = (time((long *)0) - s.st_ctime)/(24*60*60);
		if(days>0)
			return 1;
		break;
	default:
		break;
	}
	return 0;
}

/*
 *  check the age of a file.  if it is greater than warn or removedays, tell the
 *  sender.  return 0 if the file is to be removed, -1 otherwise.
 */
checkage(ctl)
	char *ctl;
{
	struct stat s;
	int days;
	char buf[256];
	FILE *fp;

	/*
	 *  get the file's age
	 */
	if(stat(ctl, &s)<0)
		return -1;
	days = (time((long *)0) - s.st_ctime)/(24*60*60);

	/*
	 *  check for removal
	 */
	if(removedays>=0 && days>=removedays){
		fp = parseline1(ctl);
		if(fp==NULL)
			return -1;
		fclose(fp);

		Syslog(LOG_INFO,"returning mail to %s orig to %s after %d days",
			s_to_c(replyaddr), s_to_c(dest), days);
		return returnmail(ctl, 1);
	}

	/*
	 *  check for warning
	 */
	if(warn>=0 && days>=warn){
		fp = parseline1(ctl);
		if(fp==NULL)
			return -1;
		fclose(fp);

		Syslog(LOG_INFO, "warning %s about %s after %d days",
			s_to_c(replyaddr), s_to_c(dest), days);
		returnmail(ctl, 0);
	}

	return -1;
}

#ifndef OLDSTYLE
static char *unixtime()
{
	static char tbuf[32];
	long now = time((long *)0);
	register char *p = ctime(&now);

	tzset();
	(void) strncpy(tbuf, p, 20);		/* "Sun Sep 16 01:03:52 " */
	tbuf[20] = '\0';
	(void) strncat(tbuf, tzname[daylight?0:1], 4); /* "AEST" */
	tbuf[24] = '\0';
	(void) strncat(tbuf, &p[19], 5);	/* " 1973" */
	tbuf[29] = '\0';
	return tbuf;
}
#endif

static char *next_recip proto((char*));

/*
 *  return a piece of mail with a reason for the return
 */
returnmail(ctl, warn)
	char *ctl;
{
	int pid, status;
	string *cmd;
	int pfd[2];
	char buf[132];
	int fd, n;
	int reads;
	FILE *fp;
	FILE *ifp;
	long now;
	char asct[27];
	char *recip;

	Syslog(LOG_INFO, "returnmail %s to %s about %s\n", ctl, s_to_c(replyaddr),
		s_to_c(dest));

	if(pipe(pfd)<0)
		return -1;

	switch(pid=fork()){
	case -1:
		close(pfd[0]);
		close(pfd[1]);
		return -1;
	case 0:
		/*
		 *  start up the mailer to take the refusal message
		 */
		close(0);
		dup(pfd[0]);
		close(pfd[1]);
		execl(RMAIL, RMAIL,  s_to_c(replyaddr), (char *)0);
		exit(1);
	default:
		/*
		 *  pipe the refusal message to the mailer
		 */
		close(pfd[0]);
		fp = fdopen(pfd[1], "w");
		if(fp==NULL) {
			close(pfd[1]);
			break;
		}

		/*
		 *  the From line
		 */
		now = time((long *)0);
		strcpy(asct, ctime(&now));
		asct[24] = 0;
		fprintf(fp, "From postmaster %s\n", asct);

#ifdef OLDSTYLE
		/*
		 *  the refusal message
		 */
		if(warn) {
			fprintf(fp, "Subject: smtp mail failed\n\n");
			fprintf(fp, "Your mail to %s is undeliverable.\n",
				s_to_c(dest));
		} else {
			fprintf(fp, "Subject: smtp mail warning\n\n");
			fprintf(fp, "Your mail to %s is not yet delivered.\n",
				s_to_c(dest));
			fprintf(fp, "Delivery attempts continue.\n");
		}

		/*
		 *  then diagnosis of error
		 */
		fprintf(fp, "---------- diagnosis ----------\n");
		ifp = fopen(fileoftype('E', ctl), "r");
		if(ifp!=NULL){
			for(reads=0; reads<20; reads++) {
				if(fgets(buf, sizeof(buf), ifp)==NULL)
					break;
				fputs(buf, fp);
			}
			fclose(ifp);
		}

		/*
		 *  finally the message itself
		 */
		fprintf(fp, "---------- unsent mail ----------\n");
		ifp = fopen(fileoftype('D', ctl), "r");
		if(ifp!=NULL){
			for(reads=0; reads<50; reads++) {
				if(fgets(buf, sizeof(buf), ifp)==NULL)
					break;
				fputs(buf, fp);
			}
			fclose(ifp);
		}
#else
		/*  First, the UNIX Header is created above. */
		/*  Second, the UMRH */
		fprintf(fp, "Report-Version: 2\n");
		fprintf(fp, ">To: %s\n", s_to_c(replyaddr));
		fprintf(fp, "Date: %s\n", unixtime());
		recip = next_recip(ctl);
		while (recip) {
			char *m;
			switch (warn) {
			case 0:
				m = "Message Transfer Agent Congestion\n\t(Cannot reach host. Delivery attempts will continue.)";
				break;
			case 1:
				m = "Expired Maximum Time";
				break;
			case 2:
				m = "Transfer Failure\n\t(Error in message or message format.)";
				break;
			}
			fprintf(fp, "Not-Delivered-To: %s due to %s\n", recip, m);
			recip = next_recip(NULL);
		}

		/*  Third, the SMTP diagnosis */
		ifp = fopen(fileoftype('E', ctl), "r");
		if (ifp != NULL) {
			char *front;
			front = "X-SMTP-Diagnosis: ";
			for (reads = 0; reads < 20; reads++) {
				register char *p;
				if (fgets(buf, sizeof(buf), ifp) == NULL)
					break;
				p = &buf[strspn(buf, " \t")];
				if (*p == '\0')
					continue;	/* skip blank lines */
				fputs(front, fp);
				fputs(buf, fp);
				front = "    ";
			}
			fclose(ifp);
		}
		fprintf(fp, "End-of-Header:\n");

		/*  Finally the message itself (wrapped by a UCDH) */
		ifp = fopen(fileoftype('D', ctl), "r");
		if (ifp != NULL) {
			struct stat sb;
			(void) fstat(fileno(ifp), &sb);
			fprintf(fp, "Content-Type: message\n");
			fprintf(fp, "Content-Length: %d bytes\n\n", sb.st_size);
			while (fgets(buf, sizeof(buf), ifp) != NULL)
				fputs(buf, fp);
			fclose(ifp);
		}
#endif

		fclose(fp);

		/*
		 *  wait for the warning to finish
		 */
		while((n = wait(&status))>=0)
			if(n == pid)
				break;
		return status ? -1 : 0;
	}
	close(pfd[1]);
	return -1;
}

#ifndef OLDSTYLE
static char *next_recip(file)
char *file;
{
	static char *arg, *dest;
	static char card[BUFSIZ];
	static char recip[330];
	static char ws[] = " \t";

	if (file != NULL) {
		/* Reinitialize card and arg for a new control file */
		register FILE *fp = fopen(file, "r");
		if (fp == NULL)
			return NULL;
		(void) fgets(card, BUFSIZ, fp);	/* skip 1st line */
		if (fgets(card, BUFSIZ, fp) == NULL) {
			fclose(fp);
			return NULL;
		}
		card[strlen(card)-1] = '\0';
		arg = strtok(card, ws);
		while (arg[0] == '-' && strlen(arg) == 2) {
			if (strchr("daH", arg[1]) != NULL)
				/* Skip next argument */
				arg = strtok(NULL, ws);
			arg = strtok(NULL, ws);
		}
		dest = strtok(NULL, ws);	/* Destination node */
		fclose(fp);
	}
	arg = strtok(NULL, ws);
	if (arg == NULL)
		return NULL;
	(void) sprintf(recip, "%.256s!%.64s", dest, arg);
	return recip;
}
#endif
