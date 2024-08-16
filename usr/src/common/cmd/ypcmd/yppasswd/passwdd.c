/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ypcmd:yppasswd/passwdd.c	1.2"
#ident  "$Header: $"

#ifndef lint
static  char sccsid[] = "@(#)passwdd.c 1.2 93/12/28 ";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <errno.h>
#include <signal.h>
#include <rpc/rpc.h>
#include <netconfig.h>
#include <netdir.h>
#include <pwd.h>
#include "yppasswd.h"
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <ctype.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <crypt.h>
#include <termios.h>
#ifdef USE_SHADOW
#include <shadow.h>
#endif

#define	pwmatch(name, line, cnt) \
	(line[cnt] == ':' && strncmp(name, line, cnt) == 0)

char *file;			/* file in which passwd's are found */
char	temp[96];		/* lockfile for modifying 'file' */
#ifdef USE_SHADOW
char *shadow;
char	stemp[96];
#endif

extern char *crypt();
extern char *gettxt();
extern int errno;
static void boilerplate();
static struct passwd *getpnam();
#ifdef USE_SHADOW
static struct spwd *getsnam();
#endif

int Argc;
char **Argv;
int logging = 0;
char *logfile ="/var/yp/yppasswdd.log";
#define rmtaddr(b) (((struct sockaddr_in *)b->buf)->sin_addr)

/* must match sizes in passwd */
#define	STRSIZE 100
#define	PWBUFSIZE 10

#define	PWSIZE (PWBUFSIZE - 1)
#define	CRYPTPWSIZE 13
#define	FINGERSIZE (4 * STRSIZE - 4)
#define	SHELLSIZE (STRSIZE - 2)

#ifdef USE_SHADOW	/* arg count to need */
#define ARG_CNT 3
#else
#define ARG_CNT 2
#endif

int Mstart;
int single = 0;
int nogecos = 0;
int mflag = 0;			/* do a make */
int noshell = 0;
int nopw = 0;

main(argc, argv)
	int argc;
	char **argv;
{
	SVCXPRT *transp;
	int s;
	char	*cp;	/* Temporary for building the temp file name */
	int i;

	struct netconfig *nconf;
	int fd;
 
        (void)setlocale(LC_ALL,"");
        (void)setcat("uxyppasswdd");
        (void)setlabel("UX:yppasswdd");
 
	(void) signal(SIGCHLD, SIG_IGN); /* replaces earlier wait3 */
	Argc = argc;
	Argv = argv;
	if (argc < ARG_CNT) {
		(void)pfmt(stderr, MM_STD,
#ifdef USE_SHADOW
			":1:usage: %s passwdfile shadowfile [-nosingle] [-nopw] [-nogecos] [-noshell] [-m arg1 arg2 ...]\n",
#else
			":2:usage: %s file [-nosingle] [-nopw] [-nogecos] [-noshell] [-m arg1 arg2 ...]\n",
#endif
			argv[0]);
		exit(1);
	}
	file = argv[1];
	if (access(file, W_OK) < 0) {
		(void)pfmt(stderr, MM_STD, ":3:can't write %s\n", file);
		exit(1);
	}
#ifdef USE_SHADOW
	shadow = argv[2];
	if (access(shadow, W_OK) < 0) {
		(void)pfmt(stderr, MM_STD, ":3:can't write %s\n", shadow);
		exit(1);
	}
#endif
	if (argc > ARG_CNT) {
		for (i = ARG_CNT; i < argc; i++) {
			if (strcmp(argv[i], "-m") == 0) {
				mflag = 1;
				Mstart = i + 1;
				break;
			} else if (strcmp(argv[i], "-single") == 0)
				single = 1;
			else if (strcmp(argv[i], "-nosingle") == 0)
				single = 0;
			else if (strcmp(argv[i], "-nogecos") == 0)
				nogecos = 1;
			else if (strcmp(argv[i], "-nopw") == 0)
				nopw = 1;
			else if (strcmp(argv[i], "-noshell") == 0)
				noshell = 1;
			else
				(void)pfmt(stderr, MM_STD,
				    ":4:unknown option %s ignored\n",
				argv[i]);
		}
	}

	if (chdir("/var/yp") < 0) {
		(void)pfmt(stderr, MM_STD, ":6:can't chdir to /var/yp\n");
		exit(1);
	}

	/* make a temp file in the same dir as the passwd file */
	(void)strcpy(temp, file);
#ifdef USE_SHADOW
	(void)strcpy(stemp, shadow);
#endif

	/* find end of the path ... */
	for (cp = &(temp[strlen(temp)]); (cp != temp) && (*cp != '/'); cp--)
		;
	if (*cp == '/')
		(void)strcat(cp+1, ".ptmp");
	else
		(void)strcat(cp, ".ptmp");
	/* temp now has either '.ptmp' or 'filepath/.ptmp' in it */

#ifdef USE_SHADOW
	/* find end of the path ... */
	for (cp = &(stemp[strlen(stemp)]); (cp != stemp) && (*cp != '/'); cp--)
		;
	if (*cp == '/')
		(void)strcat(cp+1, ".stmp");
	else
		(void)strcat(cp, ".stmp");
	/* temp now has either '.stmp' or 'filepath/.stmp' in it */
#endif

	if (fork())
		exit(0);
	if (access(logfile, 002) == -1) {
		(void) freopen("/dev/null", "w", stderr);
		(void) freopen("/dev/null", "w", stdout);
	} else {
		logging = 1;
		(void) freopen(logfile, "a", stderr);
		(void) freopen(logfile, "a", stdout);
	}
	(void) freopen("/dev/null", "r", stdin);
	{ int t;
		for (t=3; t < 20; t++)
			(void)close(t);
	}
	(void) setpgrp();

	openlog("yppasswdd", LOG_CONS| LOG_PID, LOG_AUTH);
	unlimit(RLIMIT_CPU);
	unlimit(RLIMIT_FSIZE);
	
	if ((nconf = getnetconfigent("udp")) == NULL) {
		plog(LOG_ERR, gettxt(":7", "transport not supported\n"));
		exit(1);
	}
	if ((fd = t_open(nconf->nc_device, O_RDWR, NULL)) == -1) {
		plog(LOG_ERR, gettxt(":8", "svcudp_create: could not open connection\n"));
		exit(1);
	}
	if (netdir_options(nconf, ND_SET_RESERVEDPORT, fd, NULL)) {	
		plog(LOG_ERR,
			gettxt(":9", "can't bind to a privileged port\n"));
		exit(1);
	}	
	transp = svc_tli_create(fd, nconf, (struct t_bind *)NULL, 8800,
		8800);
	if (transp == NULL) {
		plog(LOG_ERR, gettxt(":10", "couldn't create an RPC server\n"));
		exit(1);
	}
	(void) rpcb_unset((u_long)YPPASSWDPROG, (u_long)YPPASSWDVERS,
			nconf);
	if (svc_reg(transp, (u_long)YPPASSWDPROG, (u_long)YPPASSWDVERS,
		boilerplate, nconf) == FALSE) {
		plog(LOG_ERR, gettxt(":11", "couldn't register yppasswdd\n"));
		exit(1);
	}
	
	svc_run();
	plog(LOG_ERR, gettxt(":12", "svc_run shouldn't have returned\n"));
	exit(1);
	/* NOTREACHED */
}

static void
boilerplate(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
#ifdef DEBUG
	plog(LOG_ERR, gettxt(":13", "server received a request\n"));
#endif
	switch (rqstp->rq_proc) {
		case NULLPROC:
			if (!svc_sendreply(transp, xdr_void, (char *)0))
				plog(LOG_WARNING,
				    gettxt(":14", "couldn't reply to RPC call\n"));
			break;
		case YPPASSWDPROC_UPDATE:
			changepasswd(rqstp, transp);
			break;
	}
}

/*ARGSUSED*/
changepasswd(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	int tempfd, i, len, pid;
	static int ans;
	FILE *tempfp, *filefp;
	char buf[256], *p;
	char cmdbuf[BUFSIZ];
	void (*f1)(), (*f2)(), (*f3)();
	struct passwd *oldpw, *newpw;
#ifdef USE_SHADOW
	int stempfd;
	FILE *stempfp, *sfilefp;
	struct spwd *oldsp;
#endif
	struct yppasswd yppasswd;
	int *status;
	char *ptr;
	char str1[224];
	char str2[224];
	int changedpw = 0;
	int changedsh = 0;
	int changedgecos = 0;
	struct netbuf *caller;
	char *cp;

#ifdef EXTRA_PARANOID
	struct netconfig *nconf;

	if ((nconf = getnetconfigent("udp")) == NULL) {
		(void)plog(LOG_ERR, gettxt(":7", "transport not supported\n"));
		exit(1);
	}
	caller = svc_getrpccaller(transp);
	if (netdir_options(nconf, ND_CHECK_RESERVEDPORT, 0, caller) != 0) {
		(void)plog(LOG_ERR, gettxt(":15", "%s"), 
			gettxt(":16", "yppasswd client is not running on privileged port\n"));
		exit(1);
	}
#endif

	memset((char *)&yppasswd, 0, sizeof (yppasswd));
	if (!svc_getargs(transp, xdr_yppasswd, (char *)&yppasswd)) {
		svcerr_decode(transp);
		return;
	}

	if ((! validstr(yppasswd.oldpass, PWSIZE)) ||
	    (! validstr(yppasswd.newpw.pw_passwd, CRYPTPWSIZE)) ||
	    (! validstr(yppasswd.newpw.pw_gecos, FINGERSIZE)) ||
	    (! validstr(yppasswd.newpw.pw_shell, SHELLSIZE))) {
		svcerr_decode(transp);
		return;
	}

	/*
	 * Clean up from previous changepasswd() call
	 */

	newpw = &yppasswd.newpw;
	ans = 2;

#ifdef USE_SHADOW
	if ((oldpw = getpnam(newpw->pw_name)) == NULL ||
		(oldsp = getsnam(newpw->pw_name)) == NULL) {
#else
	if ((oldpw = getpnam(newpw->pw_name)) == NULL) {
#endif 
		plog(LOG_NOTICE, gettxt(":17", "no passwd for %s\n"),
		    newpw->pw_name);
		goto done;
	}
	/*
	 * the order in which changes in gecos, shell and passwd are 
	 * checked to enforce single is specific here to always ensure
	 * that executing yppasswd at a client would change "passwd"
	 * even if the client's + entry has different gecos and sh
	 * fields to override NIS entry.
	 */
	if ((!nogecos) && (strcmp(oldpw->pw_gecos, newpw->pw_gecos) != 0))
		changedgecos = 1;

	if ((!noshell) &&
		(strcmp(oldpw->pw_shell, newpw->pw_shell) != 0)){
		if (single) changedgecos = 0;
			changedsh = 1;
	}

	if ((!nopw) &&
#ifdef USE_SHADOW
		(strcmp(oldsp->sp_pwdp, newpw->pw_passwd) != 0)){
#else
		(strcmp(oldpw->pw_passwd, newpw->pw_passwd) != 0)){
#endif
		if (single){
			changedgecos = 0;
			changedsh = 0;
		}
		changedpw = 1;
	}

	if (!(changedpw + changedgecos + changedsh)) {
		plog(LOG_NOTICE, gettxt(":18", "no change for %s\n"),
		    newpw->pw_name);
		    ans = 3;
		    goto done;
	}

#ifdef USE_SHADOW
	if (/*changedpw && */oldsp->sp_pwdp && *oldsp->sp_pwdp &&
		strcmp(crypt(yppasswd.oldpass, oldsp->sp_pwdp),
		oldsp->sp_pwdp) != 0) {
#else
	if (/*changedpw && */oldpw->pw_passwd && *oldpw->pw_passwd &&
		strcmp(crypt(yppasswd.oldpass, oldpw->pw_passwd),
		oldpw->pw_passwd) != 0) {
#endif
			plog(LOG_NOTICE,
				gettxt(":19", "%s: password incorrect\n"), newpw->pw_name);
			ans = 7;
			goto done;
	}
#ifdef DEBUG
	(void)pfmt(stderr, MM_STD | MM_INFO, ":20:%d %d %d\n", changedgecos, changedsh, changedpw);
	(void)pfmt(stderr, MM_STD | MM_INFO, ":21:%s %s %s\n", yppasswd.newpw.pw_gecos,
			yppasswd.newpw.pw_shell, yppasswd.newpw.pw_passwd);
	(void)pfmt(stderr, MM_STD | MM_INFO, ":21:%s %s %s\n", oldpw->pw_gecos, oldpw->pw_shell,
#ifdef USE_SHADOW
			oldsp->sp_pwdp);
#else
			oldpw->pw_passwd);
#endif
#endif
	/*
	if (changedsh && !validloginshell(oldpw, yppasswd.newpw.pw_shell)) {
		goto done;
	}
	*/

	(void) umask(0);

	f1 = signal(SIGHUP, SIG_IGN);
	f2 = signal(SIGINT, SIG_IGN);
	f3 = signal(SIGQUIT, SIG_IGN);

changeothers:
	(void)strcpy(temp, file);
	/* find end of the path ... */
	for (cp = &(temp[strlen(temp)]); (cp != temp) && (*cp != '/'); cp--)
		;
	if (*cp == '/')
		(void)strcat(cp + 1, ".ptmp");
	else
		(void)strcat(cp, ".ptmp");
	/* temp now has either '.ptmp' or 'filepath/.ptmp' in it */
#ifdef USE_SHADOW
	(void)strcpy(stemp, file);
	/* find end of the path ... */
	for (cp = &(stemp[strlen(stemp)]); (cp != stemp) && (*cp != '/'); cp--)
		;
	if (*cp == '/')
		(void)strcat(cp + 1, ".stmp");
	else
		(void)strcat(cp, ".stmp");
	/* temp now has either '.stmp' or 'filepath/.stmp' in it */
#endif

	tempfd = open(temp, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (tempfd < 0) {
		if (errno == EEXIST){
			plog(LOG_WARNING,
				gettxt(":22", "password file busy - try again.\n"));
			ans = 8;
		} else {
			ans = 9;
			plog(LOG_ERR,
				gettxt(":23", "password temp file open bug %s errno = %d.\n"), 
					temp, errno);
		}
		goto cleanup;
	}
#ifdef USE_SHADOW
	stempfd = open(stemp, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (stempfd < 0) {
		if (errno == EEXIST){
			plog(LOG_WARNING,
				gettxt(":24", "shadow file busy - try again.\n"));
			ans = 8;
		} else {
			ans = 9;
			plog(LOG_ERR,
				gettxt(":25", "shadow temp file open bug %s errno=%d.\n"), 
					stemp, errno);
		}
		goto cleanup;
	}
#endif

	(void)signal(SIGTSTP, SIG_IGN);
	if ((tempfp = fdopen(tempfd, "w")) == NULL) {
		plog(LOG_ERR, gettxt(":26", "%s fdopen failed\n"), temp);
		ans = 10;
		goto cleanup;
	}
#ifdef USE_SHADOW
	if ((stempfp = fdopen(stempfd, "w")) == NULL) {
		plog(LOG_ERR, gettxt(":26", "%s fdopen failed\n"), stemp);
		ans = 10;
		goto cleanup;
	}
#endif
	/*
	 * Copy passwd to temp, replacing matching lines
	 * with new password.
	 */
	if ((filefp = fopen(file, "r")) == NULL) {
		plog(LOG_CRIT, gettxt(":27", "fopen of %s failed errno = %d \n"),
		    file, errno);
		ans = 12;
		goto cleanup;
	}
#ifdef USE_SHADOW
	if ((sfilefp = fopen(shadow, "r")) == NULL) {
		plog(LOG_CRIT, "fopen of %s failed errno = %d\n",
		    file, errno);
		ans = 12;
		goto cleanup;
	}
#endif
	len = strlen(newpw->pw_name);
	/*
	 * This fixes a really bogus security hole, basically anyone can
	 * call the rpc passwd daemon, give them their own passwd and a
	 * new one that consists of ':0:0:Im root now:/:/bin/csh^J' and
	 * give themselves root access. With this code it will simply make
	 * it impossible for them to login again, and as a bonus leave
	 * a cookie for the always vigilant system administrator to ferret
	 * them out.
	 */
	for (p = newpw->pw_name; (*p != '\0'); p++)
		if ((*p == ':') || !(isprint(*p)))
			*p = '$';	/* you lose buckwheat */
	for (p = newpw->pw_passwd; (*p != '\0'); p++)
		if ((*p == ':') || !(isprint(*p)))
			*p = '$';	/* you lose buckwheat */

	while (fgets(buf, sizeof (buf), filefp)) {
		p = strchr(buf, ':');
		if (p && p - buf == len &&
		    strncmp(newpw->pw_name, buf, p - buf) == 0) {
			(void)pfmt(tempfp, MM_NOSTD, ":28:%s:%s:%d:%d:%s:%s:%s\n",
			    oldpw->pw_name,
#ifdef USE_SHADOW
			    oldpw->pw_passwd,
#else
			    (changedpw ?
				newpw->pw_passwd: oldpw->pw_passwd),
#endif
			    oldpw->pw_uid,
			    oldpw->pw_gid,
			    (changedgecos ?
				newpw->pw_gecos : oldpw->pw_gecos),
			    oldpw->pw_dir,
			    (changedsh ?
				newpw->pw_shell  : oldpw->pw_shell));
		} else {
			if (fputs(buf, tempfp) == EOF) {
				plog(LOG_CRIT, gettxt(":29", "%s: write error\n"), "passwd");
				ans = 13;
				goto cleanup;
			}
		}
		if (ferror(tempfp)) {
			plog(LOG_CRIT, gettxt(":30", "%s: write ferror set.\n"), "passwd");
			ans = 14;
			goto cleanup;
		}
	}
	(void)fclose(filefp);
	(void)fflush(tempfp);
	if (ferror(tempfp)) {
		plog(LOG_CRIT, gettxt(":31", "%s: fflush ferror set.\n"), "passwd");
		ans = 15;
		goto cleanup;
	}
	(void)fclose(tempfp);
	tempfp = (FILE *) NULL;

#ifdef USE_SHADOW
	while (fgets(buf, sizeof (buf), sfilefp)) {
		p = strchr(buf, ':');
		if (p && p - buf == len &&
		    strncmp(newpw->pw_name, buf, p - buf) == 0) {
			(void)pfmt(stempfp, MM_NOSTD, ":32:%s:%s:",
				oldsp->sp_namp,
				(changedpw ?
					newpw->pw_passwd: oldsp->sp_pwdp));
				if(oldsp->sp_lstchg)
					(void)pfmt(stempfp, MM_NOSTD
					  | MM_NOGET, "%d",oldsp->sp_lstchg);
				(void)pfmt(stempfp, MM_NOSTD | MM_NOGET, ":");
				if(oldsp->sp_min) (void)pfmt(stempfp,
				   MM_NOSTD | MM_NOGET, "%d",oldsp->sp_min);
				(void)pfmt(stempfp, MM_NOSTD | MM_NOGET, ":");
				if(oldsp->sp_max) (void)pfmt(stempfp,
				   MM_NOSTD | MM_NOGET, "%d",oldsp->sp_max);
				(void)pfmt(stempfp, MM_NOSTD | MM_NOGET, ":");
				if(oldsp->sp_warn) (void)pfmt(stempfp,
				   MM_NOSTD | MM_NOGET,
					"%d",oldsp->sp_warn);
				(void)pfmt(stempfp, MM_NOSTD |
				   MM_NOGET, ":");
				if(oldsp->sp_inact) (void)pfmt(stempfp,
				  MM_NOSTD | MM_NOGET, "%d",oldsp->sp_inact);
				(void)pfmt(stempfp, MM_NOSTD |
					MM_NOGET, ":");
				if(oldsp->sp_expire) (void)pfmt(
				   stempfp, MM_NOSTD | MM_NOGET,
				     "%d",oldsp->sp_expire);
				(void)pfmt(stempfp, MM_NOSTD |
					MM_NOGET, ":");
				if((long)oldsp->sp_flag) (void)pfmt(
				  stempfp, MM_NOSTD | MM_NOGET, "%d",(long)oldsp->sp_flag);
				(void)pfmt(stempfp, MM_NOSTD |
				   MM_NOGET, "\n");
		} else {
			if (fputs(buf, stempfp) == EOF) {
				plog(LOG_CRIT, gettxt(":33", "%s: write error\n"), "shadow");
				ans = 13;
				goto cleanup;
			}
		}
		if (ferror(stempfp)) {
			plog(LOG_CRIT, gettxt(":34", "%s: write ferror set.\n"), "shadow");
			ans = 14;
			goto cleanup;
		}
	}
	(void)fclose(sfilefp);
	(void)fflush(stempfp);
	if (ferror(stempfp)) {
		plog(LOG_CRIT,
			gettxt(":31", "%s: fflush ferror set.\n"),
				"shadow");
		ans = 15;
		goto cleanup;
	}
	(void)fclose(stempfp);
	stempfp = (FILE *) NULL;
#endif

	if (rename(temp, file) < 0) {
		plog(LOG_CRIT, gettxt(":35", "unable to rename %s to %s: errno %d\n"),
		 temp, file, errno);
		(void)unlink(temp);
		ans = 17;
		goto cleanup;
	}
#ifdef USE_SHADOW
	if (rename(stemp, shadow) < 0) {
		plog(LOG_CRIT, "unable to rename %s to %s: errno %d\n",
			stemp, shadow, errno);
		(void)unlink(stemp);
		ans = 17;
		goto cleanup;
	}
#endif
	if (changedpw && (changedsh || changedgecos)) {
		(void)pfmt(stdout, MM_NOSTD, ":36:changed both pw and %s %s\n",
				changedsh?"shell":"", changedgecos?" gecos":"");
		changedpw = 0;	/* already changed it */
		goto changeothers;
	}
	caller = svc_getrpccaller(transp);
	plog(LOG_NOTICE,
		gettxt(":37", "%s password entry changed: remote host: %s\n"),
		    newpw->pw_name, inet_ntoa(rmtaddr(caller)));
	ans = 0;
	if (mflag && ((pid = fork()) == 0)) {
		int ret;
#ifdef UNIXWARE
#define YPBUILD "/var/yp/ypbuild MAKE=\"/var/yp/ypbuild\""
		(void)strcpy(cmdbuf, YPBUILD);
#else
		(void)strcpy(cmdbuf, "make");
#endif
		for (i = Mstart; i < Argc; i++) {
			(void)strcat(cmdbuf, " ");
			(void)strcat(cmdbuf, Argv[i]);
		}
		ret = system(cmdbuf);
		if (ret != 0)
			plog(LOG_ERR, gettxt(":38", "failed %s\n"), cmdbuf);
		else
			plog(LOG_ERR, gettxt(":39", "executed %s\n"), cmdbuf);
		exit(0);
	}
#ifdef DEBUG
	if (mflag)
		(void)pfmt(stderr, MM_STD | MM_INFO, ":40:forked a child %d for make\n", pid);
#endif
#ifdef UNIXWARE
	/*
	 * Only run creatiadb if we are dealing with the 
	 * real password and shadow passwd.
	 */
	if (!strcmp(file, "/etc/passwd") && !strcmp(shadow, "/etc/shadow"))
		creatiadb();
#endif
cleanup:
	if (tempfp)
		(void)fclose(tempfp);
#ifdef USE_SHADOW
	if (stempfp)
		(void)fclose(stempfp);
#endif
	(void)signal(SIGHUP, f1);
	(void)signal(SIGINT, f2);
	(void)signal(SIGQUIT, f3);
done:
	if (!svc_sendreply(transp, xdr_int, (char *)&ans))
		plog(LOG_WARNING, gettxt(":41", "couldnt reply to RPC call\n"));
}

static char *
pwskip(p)
	register char *p;
{
	while (*p && *p != ':' && *p != '\n')
		++p;
	if (*p) *p++ = 0;
	return (p);
}

static struct passwd *
getpnam(name)
	char *name;
{
	FILE *pwf;
	int cnt;
	char *p;
	static char line[BUFSIZ+1];
	static struct passwd passwd;

	pwf = fopen(file, "r");
	if (pwf == NULL)
		return (NULL);
	cnt = strlen(name);
	while ((p = fgets(line, BUFSIZ, pwf)) && !pwmatch(name, line, cnt))
		;
	if (p) {
		passwd.pw_name = p;
		p = pwskip(p);
		passwd.pw_passwd = p;
		p = pwskip(p);
		passwd.pw_uid = atoi(p);
		p = pwskip(p);
		passwd.pw_gid = atoi(p);
		p = pwskip(p);
		passwd.pw_gecos = p;
		p = pwskip(p);
		passwd.pw_dir = p;
		p = pwskip(p);
		passwd.pw_shell = p;
		while (*p && *p != '\n') p++;
		*p = '\0';
		(void)fclose(pwf);
		return (&passwd);
	} else {
		(void)fclose(pwf);
		return (NULL);
	}
}

#ifdef USE_SHADOW
static struct spwd *
getsnam(name)
char *name;
{
	FILE *spwf;
	int cnt;
	char *p;
	static char line[BUFSIZ+1];
	static struct spwd sp;

	spwf = fopen(shadow, "r");
	if (spwf == NULL)
		return (NULL);
	cnt = strlen(name);
	while ((p = fgets(line, BUFSIZ, spwf)) && !pwmatch(name, line, cnt))
		;
	if (p) {
		sp.sp_namp = p;
		p = pwskip(p);
		sp.sp_pwdp = p;
		p = pwskip(p);
		sp.sp_lstchg = atoi(p);
		p = pwskip(p);
		sp.sp_min = atoi(p);
		p = pwskip(p);
		sp.sp_max = atoi(p);
		p = pwskip(p);
		sp.sp_warn = atoi(p);
		p = pwskip(p);
		sp.sp_inact = atoi(p);
		p = pwskip(p);
		sp.sp_expire = atoi(p);
		p = pwskip(p);
		sp.sp_flag = atoi(p);
		while (*p && *p != '\n') p++;
		*p = '\0';
		(void)fclose(spwf);
		return (&sp);
	} else {
		(void)fclose(spwf);
		return (NULL);
	}
}
#endif

validstr(str, size)
	char *str;
	int size;
{
	char c;

	if (str == NULL || (int)strlen(str) > size || strchr(str, ':'))
		return (0);
	while (c = *str++) {
		if (iscntrl(c))
			return (0);
	}
	return (1);
}


#ifdef EXTRA_PARANOID
/*ARGSUSED*/
int
yp_valid_client(a)
	unsigned long a;
{
	return (TRUE);
}
#endif

unlimit(lim)
{
	struct rlimit rlim;
	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
	(void) setrlimit(lim, &rlim);
}
#ifdef UNIXWARE
creatiadb()
{
	int pid, status;
	char *cmd = "/sbin/creatiadb";
	char *errstr = gettxt(":5", "Unable to update IAF database");

	if ((pid = fork()) == 0) {
		(void) signal(SIGCHLD, SIG_DFL);
		switch (pid = fork()){
			case 0:
				execl(cmd, cmd, (char *)0);
				plog(LOG_ERR, gettxt(":42", "%s: execl of %s failed: errno = %d"), 
					errstr, cmd, errno);
				break;
			case -1:
				plog(LOG_ERR, gettxt(":43", "%s: fork failed: errno = %d"), 
					errstr, errno);
				break;
				
			default:
				if (waitpid(pid, &status, 0) < 0) {
					plog(LOG_ERR, gettxt(":44", "waitpid failed: errno = %d"), errno);
				} else {
					if (status)
						plog(LOG_ERR, 
					  gettxt(":45", "%s return with a status of %d\n"), 
					   cmd, status);
				}
		}
		exit(0);
	}
	if (pid < 0){
		plog(LOG_ERR, gettxt(":43",
			"%s: fork failed: errno = %d"), errstr, errno);
	}
}
#endif
plog(lvl, arg1, arg2, arg3, arg4, arg5)
int lvl;
char *arg1;
int arg2, arg3, arg4, arg5;
{

	long tm = time((long *)0);
	char *hdr="yppasswdd:";

	if (logging){
		pfmt(stderr, MM_STD | MM_NOGET, "%19.19s: ", ctime(&tm));
		if (strstr(arg1, hdr))
			arg1 = (arg1 + strlen(hdr));
		pfmt(stderr, MM_STD | MM_NOGET, arg1, arg2, arg3, arg4, arg5);
		fflush(stderr);
	} else {
		syslog(lvl, arg1, arg2, arg3, arg4, arg5);
	}
}
