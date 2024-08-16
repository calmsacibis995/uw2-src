/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/qlib.c	1.7.5.2"
#ident "@(#)qlib.c	1.14 'attmail mail(1) command'"
#include "libmail.h"
#include "smtp.h"
#include "xmail.h"
#include "s5sysexits.h"
#ifdef SVR4_1
#include <pfmt.h>
#endif

char spoolsubdir[1026];

extern char *UPASROOT;
extern const char *progname;

/*
 *  A lock is considered stale if it contains the id of a
 *  nonexistent process or if it was created more than `lockwait'
 *  seconds ago.  lockwait should be larger than the maximum expected
 *  time skew among systems that might share a single spool directory
 */
static int lockwait = 3600;

/*
 *  copy the directory component of `file' into lf.  return
 *  a pointer to the base name of `file'
 */
static char *
copydir(lf, file)
	string *lf;
	char *file;
{
	char *base;

	base = strrchr(file, '/');
	if (base){
		*base = 0;
		s_append(lf, file);
		s_append(lf, "/");
		*base++ = '/';
	} else
		base = file;
	return base;
}

/*
 *  convert the control file name into a file name of the
 *  type specified.
 */
char *
fileoftype(type, ctl)
	char type;
	char *ctl;
{
	static string *x;
	char *cp;

	x = s_reset(x);
	s_append(x, ctl);
	cp = strrchr(s_to_c(x), '/');
	if(cp)
		cp++;
	else
		cp = s_to_c(x);
	*cp = type;
	return s_to_c(x);
}

/*
 *  creates a file with a unique name
 *  based on `template'.  Any trailing string of x's in the
 *  template are converted to pppppsssssvv, where ppppp is the
 *  process id, sssss is the last 16 bits of the time, vv is enough
 *  to make the name unique.  If there aren't enough x's to fit all
 *  of pppppsssssvv, only as much as will fit (starting right to left)
 *  will be substituted for the x's.
 *
 *  returns an open fd or -1 if the file couldn't be created.
 *  the new name is put into template.
 */
mkdatafile(template)
	char *template;
{
	struct stat s;
	int i, len, pid, fd;
	long seed;
	char hash[14];
	char *xp;

	/*
	 *  find the number of x's
	 */
	if((len = strlen(template))==0)
		return -1;
	for(xp = template+len-1; xp>=template; xp--)
		if(*xp!='x')
			break;
	xp++;
	len = len-(xp-template);

	/*
	 *  make sure it's <= 12
	 */
	if(len>12)
		len = 12;
	pid = getpid();
	seed = (time((long *)0)%100000)*100;

	/*
	 *  try 100 different file names
	 */
	for(i=0; i<100; i++){
		sprintf(hash, "%05d%07ld", pid, seed+i);
		strncpy(xp, hash+(12-len), len);
		if(stat(template, &s)>=0)
			continue;
		if((fd = creat(template, 0660))<0)
			return -1;
		return fd;
	}
	return -1;
}

/*
 *  creates a control file to go with the data file.
 *  the control file name is the same as the data file
 *  with the first character replaced by 'C'.
 */
int
mkctlfile(letter, dataname, contents)
	char letter;
	char *dataname;
	char *contents;
{
	int fd;
	static string *cf;
	static string *tf;

	/*
 	 *  make the file names
	 */
	cf = s_reset(cf);
	s_append(cf, fileoftype(letter, dataname));
	tf = s_reset(tf);
	s_append(tf, fileoftype('T', dataname));

	/*
	 *  create the control file with a temporary (non-control) name
	 */
	fd = creat(s_to_c(tf), 0660);
	if (fd<0)
		return -1;
	if(write(fd, contents, strlen(contents))!=strlen(contents)){
		close(fd);
		unlink(s_to_c(tf));
		return -1;
	}
	if(close(fd)<0){
		unlink(s_to_c(tf));
		return -1;
	}

	/*
	 *  change it's name so that it looks like a control file
	 */
	if(rename(s_to_c(tf), s_to_c(cf))<0)
		return -1;
	unlink(s_to_c(tf));
	return 0;
}

/*
 *  Fill name with the lock name for file.  The lockname is
 *  dir/L.xxx where dir is the directory containing the file
 *  being locked and xxx is the first 12 characters of that file's
 *  name.
 */
static
setlname(lf, file)
	string *lf;
	char *file;
{
	char *base;

	/*
	 *  copy over directory portion
	 */
	base = copydir(lf, file);

	/*
	 *  copy in the rest
	 */
	s_append(lf, "L.");
	s_append(lf, base);

	/*
	 *  make sure we didn't get too long
	 */
	base = strrchr(s_to_c(lf), '/');
	if(base)
		base++;
	else
		base = s_to_c(lf);
	if((int)strlen(base)>14)
		base[14] = 0;
	return 0;
}

/*
 *  Return true if file has been locked by us or another program using the same
 *  lock name scheme.
 *
 *  Remove the lock file if the locking process has gone away.
 */
int
islocked(file)
	char *file;
{
	struct stat stbuf;
	static string *ln;
	int pid;
	FILE *fp;

	ln = s_reset(ln);
	if(setlname(ln, file)<0)
		return 1;
	if(stat(s_to_c(ln), &stbuf)==0) {
		fp = fopen(s_to_c(ln), "r");
		if (fp == 0 || fscanf(fp, "%d", &pid)!=1) {
			/*
			 *  if the file is less than `lockwait' old,
			 *  assume it's still current even though
			 *  there's no pid there
			time_t now = time((time_t*) 0);
			if (now < stbuf.st_mtime + lockwait) {
				if (fp)
					fclose(fp);
				return 1;
			}

			/*
			 *  either we made the lock wrong
			 *  or it just went away (race)
			 */
			pfmt(stderr, MM_INFO, ":8:can't read pid; breaking lock %s\n",
				s_to_c(ln));
			if(fp)
				fclose(fp);
			unlink(s_to_c(ln));
			return 0;
		}
		if(fp)
			fclose(fp);
		if (kill(pid, 0) == 0)
			return 1;

		/*
		 *  locker has gone away.  We are tired of this message.
		 */
		pfmt(stderr, MM_INFO, ":9:breaking stale lock %s\n", s_to_c(ln));
		unlink(s_to_c(ln));
	}
	return 0;
}

/*
 *  lock a file being processed.  see setlname (above) for the name of the lock.
 *  the lock file is in the same directory.
 *
 *  returns 0 if the lock was granted, -1 otherwise.  this is a none
 *  blocking routine.
 */
lock(file)
	char *file;
{
	int fd;
	char *sp;
	char pidbuf[20];
	static string *tn;
	static string *ln;

	/*
	 *  create a temporary file (in same directory)
	 */
	tn = s_reset(tn);
	copydir(tn, file);
	s_append(tn, "T.xxxxxxxxxxxx");
	fd = mkdatafile(s_to_c(tn));
	if(fd<0)
		return -1;
	sprintf(pidbuf, "%d lock", getpid());
	write(fd, pidbuf, strlen(pidbuf));
	close(fd);

	/*
	 *  Make a link to it with the lock file name.  This will fail only
	 *  if it already exists.
	 */
	ln = s_reset(ln);
	setlname(ln, file);
	while(link(s_to_c(tn), s_to_c(ln)) < 0) {
		/*
		 *  might be a stale lock
		 */
		if(islocked(file)){
			unlink(s_to_c(tn));
			return -1;
		}
	}
	unlink(s_to_c(tn));
	return 0;
}

/*
 *  unlock a file
 */
void unlock(file)
	char *file;
{
	static string *ln;

	ln = s_reset(ln);
	setlname(ln, file);
	unlink(s_to_c(ln));
}

/*
 *  make a spool directory and cd into it.  the spool directory is in
 *  /usr/spool/smtpq and it's name is the 2 most significant elements of
 *  the domain name, `target'.
 */
#define WEIRD	"weird.domain"
gotodir(target)
	char *target;
{
	register char *bp, *lp, *last;
	char t[256];
	register struct passwd *pw = NULL;
	register struct group *gr = NULL;
#ifdef SVR3
	struct passwd *getpwnam();
	struct group *getgrnam();
#endif
	int elems;

	if (chdir(SMTPQROOT) < 0) {
		mkdir(SMTPQROOT, 0775);
		if (chdir(SMTPQROOT) < 0) {
			Syslog(LOG_WARNING, "Could not create %s", SMTPQROOT);
			return -1;
		}
#ifdef SVR4_1
		if ((pw = getpwnam("smtp")) == NULL)
#else
		if ((pw = getpwnam("uucp")) == NULL)
#endif
			return -1;
		if ((gr = getgrnam("mail")) == NULL)
			return -1;
		(void) chmod(".", 0775);
		if (chown(".", pw->pw_uid, gr->gr_gid) == -1)
			(void) posix_chown(".");
	}

	for (bp=target, lp=t; *bp; bp++, lp++) {
		if (*bp == '/')
			*lp = '.';
		else if (isupper(*bp))
			*lp = tolower(*bp);
		else
			*lp = *bp;
	}
	*lp = '\0';

	if (strncmp(t, "dk!", 3) == 0)
		bp = t + 3;
	else if (strncmp(t, "tcp!", 4) == 0)
		bp = t + 4;
	else
		bp = t;

	if ((lp=strchr(bp, '!')) != NULL) /*ignore trailing service*/
		*lp-- = '\0';
	else 
		lp = bp + strlen(bp);
	last = lp;

	for(elems=0; lp>bp && last-(--lp)<MAXPATHLEN; ){
		if(*lp=='.')
			if(++elems==6){
				lp++;
				break;
			}
	}
	bp = lp;
	while ((*bp != '\0') && (*bp == '.'))
		bp++;

	if (*bp == '\0')
		strcpy(spoolsubdir, WEIRD);
	else
		strcpy(spoolsubdir, bp);

	if (chdir(spoolsubdir) < 0) {
		mkdir(spoolsubdir, 0775);
		if (chdir(spoolsubdir) < 0) {
			Syslog(LOG_WARNING, "Could not chdir to %s", spoolsubdir);
			return -1;
		}
#ifdef SVR4_1
		if (!pw && ((pw = getpwnam("smtp")) == NULL))
#else
		if (!pw && ((pw = getpwnam("uucp")) == NULL))
#endif
			return -1;
		if (!gr && ((gr = getgrnam("mail")) == NULL))
			return -1;
		(void) chmod(".", 0775);
		if (chown(".", pw->pw_uid, gr->gr_gid) == -1)
			(void) posix_chown(".");
	}

	return 0;
}

/*
 *  start the scheduler
 */
void smtpsched(av0, target, batch)
	char *av0;
	char *target;
	int batch;
{
	int status;
	static string *cmd;

	switch(fork()){
	case -1:
		break;
	case 0:
		/*
		 *  exec the sched process
		 */
		cmd = s_reset(cmd);
		s_append(cmd, UPASROOT);
		s_append(cmd, "/smtpsched");
		if (batch)
			execl(s_to_c(cmd), av0, "-B", target, (char *)0);
		else
			execl(s_to_c(cmd), av0, target, (char *)0);
		exit(1);
	default:
#ifndef SVR3
#ifndef SVR4
		/*
		 *  wait for any sub processes to finish
		 */
		while(wait(&status)>=0)
			;
#endif
#endif
		break;
	}
}
