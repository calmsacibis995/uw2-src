/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.ftpd/ftpd.c	1.12.14.3"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *	Copyrighted as an unpublished work.
 *      (c) Copyright 1990 INTERACTIVE Systems Corporation
 *      All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */
/*
 * Copyright (c) 1985, 1988, 1990 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1985, 1988, 1990 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)ftpd.c	5.37 (Berkeley) 6/27/90";
#endif /* not lint */

/*
 * FTP server.
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <shadow.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/wait.h>
#include <sys/dir.h>
#include <dirent.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#define	FTP_NAMES
#include <arpa/ftp.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>

#include <ctype.h>
#include <stdio.h>
#include <signal.h>
#include <setjmp.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <syslog.h>
#include <varargs.h>
#include <utmp.h>
#include <unistd.h>
#include <ia.h>
#include <lastlog.h>
#include "pathnames.h"
#include "../security.h"

#include <locale.h>
#include <pfmt.h>
extern char *gettxt();

#if !defined(BUFFER_SIZE)
#define BUFFER_SIZE 8192
#endif

#define ROOTUID 0

#define index(s, c)	strchr(s, c)
#define rindex(s, c)	strrchr(s, c)
#define bzero(b, n)	memset(b, '\0', n)

/*
 * This file has its own "smarter" version of popen()/pclose(). To avoid
 * possible conflict with std popen()/pclose(), they are named "ftpd_popen()"
 * and ftpd_close()" in ***THIS*** file.
 */
extern	int errno;
extern	char *sys_errlist[];
extern  int sys_nerr;
extern	char *crypt();
extern	char version[];
extern	char *home;		/* pointer to home directory for glob */
extern	FILE *ftpd_popen(), *fopen(), *freopen();
extern	int  ftpd_pclose(), fclose();
extern	char *getline();
extern	char cbuf[];
extern	off_t restart_point;

extern	DIR *opendir();
extern	struct dirent *readdir();

struct	sockaddr_in ctrl_addr;
struct	sockaddr_in data_source;
struct	sockaddr_in data_dest;
struct	sockaddr_in his_addr;
struct	sockaddr_in pasv_addr;

int	data;
jmp_buf	errcatch, urgcatch;
int	logged_in;
uinfo_t	uinfo;
int	debug;
int	timeout = 900;    /* timeout after 15 minutes of inactivity */
int	maxtimeout = 7200;/* don't allow idle time to be set beyond 2 hours */
int	logging;
int	guest;
int	type;
int	form;
int	stru;			/* avoid C keyword */
int	mode;
int	usedefault = 1;		/* for data transfers */
int	pdata = -1;		/* for passive mode */
int	transflag;
off_t	file_size;
off_t	byte_count;
#if !defined(CMASK) || CMASK == 0
#undef CMASK
#define CMASK 027
#endif
int	defumask = CMASK;		/* default umask value */
char	tmpline[7];
char	hostname[MAXHOSTNAMELEN];
char	remotehost[MAXHOSTNAMELEN];

/*
 * Timeout intervals for retrying connections
 * to hosts that don't accept PORT cmds.  This
 * is a kludge, but given the problems with TCP...
 */
#define	SWAITMAX	90	/* wait at most 90 seconds */
#define	SWAITINT	5	/* interval between retries */

int	swaitmax = SWAITMAX;
int	swaitint = SWAITINT;

int	lostconn();
int	myoob();
FILE	*getdatasock(), *dataconn();

#ifdef SETPROCTITLE
char	**Argv = NULL;		/* pointer to argument vector */
char	*LastArgv = NULL;	/* end of argv */
char	proctitle[BUFSIZ];	/* initial part of title */
#endif /* SETPROCTITLE */

/*
 * lastlog is read to check for logins that have been inactive too long
 */
#define LASTLOG		"/var/adm/lastlog"

#define DAY_SECS	(24L * 60 * 60)

main(argc, argv, envp)
	int argc;
	char *argv[];
	char **envp;
{
	int addrlen, on = 1, tos;
	char *cp;
	extern int optind;
	extern char *optarg;
	int c;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxftp");
	(void)setlabel("UX:in.ftpd");

	openlog("ftpd", LOG_PID, LOG_DAEMON);

	addrlen = sizeof (his_addr);
	if (getpeername(0, (struct sockaddr *)&his_addr, &addrlen) < 0) {
		syslog(LOG_ERR, "getpeername (%s): %m",argv[0]);
		exit(1);
	}
	addrlen = sizeof (ctrl_addr);
	if (getsockname(0, (struct sockaddr *)&ctrl_addr, &addrlen) < 0) {
		syslog(LOG_ERR, "getsockname (%s): %m",argv[0]);
		exit(1);
	}
#ifdef IP_TOS
	tos = IPTOS_LOWDELAY;
	if (setsockopt(0, IPPROTO_IP, IP_TOS, (char *)&tos, sizeof(int)) < 0)
		syslog(LOG_WARNING, "setsockopt (IP_TOS): %m");
#endif
	data_source.sin_port = ntohs(ctrl_addr.sin_port);
	data_source.sin_port = htons(data_source.sin_port - 1);
	debug = 0;
#ifdef SETPROCTITLE
	/*
	 *  Save start and extent of argv for setproctitle.
	 */
	Argv = argv;
	while (*envp)
		envp++;
	LastArgv = envp[-1] + strlen(envp[-1]);
#endif /* SETPROCTITLE */

	while ((c = getopt(argc, argv, "vdlt:T:u:")) != -1) {
		switch (c) {

		case 'v':
			debug = 1;
			break;

		case 'd':
			debug = 1;
			break;

		case 'l':
			logging = 1;
			break;

		case 't':
			timeout = atoi(optarg);
			if (maxtimeout < timeout)
				maxtimeout = timeout;
			break;

		case 'T':
			maxtimeout = atoi(optarg);
			if (timeout > maxtimeout)
				timeout = maxtimeout;
			break;

		case 'u':
		    {
			int val = 0;

			cp = optarg;
			while (*cp >= '0' && *cp <= '9') {
				val = val*8 + *cp - '0';
				cp++;
			}
			if (*cp || cp == optarg)
				pfmt(stderr, MM_WARNING, ":318:Bad value for -u\n");
			else
				defumask = val;
			break;
		    }

		default:
			pfmt(stderr, MM_WARNING, ":319:Unknown flag ignored.\n");
			break;
		}
	}
	(void) freopen(_PATH_DEVNULL, "w", stderr);
	(void) signal(SIGPIPE, (void (*)()) lostconn);
	(void) signal(SIGCLD, SIG_IGN);
	if ((int) signal(SIGURG, (void (*)()) myoob) == (int) SIG_ERR)
		syslog(LOG_ERR, "signal: %m");

	/* Try to handle urgent data inline */
#ifdef SO_OOBINLINE
	if (setsockopt(0, SOL_SOCKET, SO_OOBINLINE, (char *)&on, sizeof(on)) < 0)
		syslog(LOG_ERR, "setsockopt: %m");
#endif

#ifdef	F_SETOWN
	if (fcntl(fileno(stdin), F_SETOWN, getpid()) == -1)
		syslog(LOG_ERR, "fcntl F_SETOWN: %m");
#endif
	dolog(&his_addr);
	/*
	 * Set up default state
	 */
	data = -1;
	type = TYPE_A;
	form = FORM_N;
	stru = STRU_F;
	mode = MODE_S;
	tmpline[0] = '\0';
	(void) gethostname(hostname, sizeof (hostname));
	reply(220, gettxt(":320", "%s FTP server (%s) ready."), hostname, version);
	(void) setjmp(errcatch);
	for (;;)
		(void) yyparse();
	/* NOTREACHED */
}

lostconn()
{

	if (debug)
		syslog(LOG_DEBUG, gettxt(":321", "lost connection"));
	dologout(-1);
}

static char ttyline[20];

int login_attempts;		/* number of failed login attempts */
int askpasswd;			/* had user command, ask for passwd */

/*
 * USER command.
 * Sets global passwd pointer pw if named account exists and is acceptable;
 * sets askpasswd if a PASS command is expected.  If logged in previously,
 * need to reset state.  If name is "ftp" or "anonymous", the name is not in
 * _PATH_FTPUSERS, and ftp account exists, set guest and pw, then just return.
 * If account doesn't exist, ask for passwd anyway.  Otherwise, check user
 * requesting login privileges.  Disallow anyone who does not have a standard
 * shell as returned by getusershell().  Disallow anyone mentioned in the file
 * _PATH_FTPUSERS to allow people such as root and uucp to be avoided.
 * Sets global passwd pointer pw if named account exists
 * and is acceptable; sets askpasswd if a PASS command is
 * expected. If logged in previously, need to reset state.
 * If name is "ftp" or "anonymous" and ftp account exists,
 * set guest and pw, then just return.
 * If account doesn't exist, ask for passwd anyway.
 * Otherwise, check user requesting login privileges.
 * Disallow anyone who does not have a standard
 * shell as returned by getusershell().
 * Disallow anyone mentioned in the file _PATH_FTPUSERS
 * to allow people such as root and uucp to be avoided.
 */
user(name)
	char *name;
{
	register char *cp;
	char *shell;
	char *getusershell();

	if (logged_in) {
		if (guest) {
			reply(530, gettxt(":322", "Can't change user from guest login."));
			return;
		}
		end_login();
	}

	guest = 0;
	if (strcmp(name, "ftp") == 0 || strcmp(name, "anonymous") == 0) {
		if (checkuser("ftp") || checkuser("anonymous"))
			reply(530, gettxt(":323", "User %s access denied."), name);
		else if (ia_openinfo("ftp", &uinfo) == 0) {
			guest = 1;
			askpasswd = 1;
			reply(331, gettxt(":324", "Guest login ok, send ident as password."));
		} else {
			reply(530, gettxt(":325", "User %s unknown."), name);
			uinfo = (uinfo_t) 0;
		}
		return;
	}


	if (ia_openinfo(name, &uinfo) == 0) {
		ia_get_sh(uinfo, &shell);
		if (shell == NULL || *shell == 0)
			shell = _PATH_BSHELL;
		while ((cp = getusershell()) != NULL)
			if (strcmp(cp, shell) == 0)
				break;
		endusershell();
		if (cp == NULL || checkuser(name)) {
			reply(530, gettxt(":323", "User %s access denied."), name);
			if (logging)
				syslog(LOG_NOTICE,
				    gettxt(":326", "FTP LOGIN REFUSED FROM %s, %s"),
				    remotehost, name);
			ia_closeinfo(uinfo);
			uinfo = (uinfo_t) 0;
			return;
		}
	}
	else {
		uinfo = (uinfo_t) 0;
	}

	reply(331, gettxt(":327", "Password required for %s."), name);
	askpasswd = 1;
	/*
	 * Delay before reading passwd after first failed
	 * attempt to slow down passwd-guessing programs.
	 */
	if (login_attempts)
		sleep((unsigned) login_attempts);
}

/*
 * Check if a user is in the file _PATH_FTPUSERS
 */
checkuser(name)
	char *name;
{
	register FILE *fd;
	register char *p;
	char line[BUFSIZ];

	if ((fd = fopen(_PATH_FTPUSERS, "r")) != NULL) {
		while (fgets(line, sizeof(line), fd) != NULL)
			if ((p = index(line, '\n')) != NULL) {
				*p = '\0';
				if (line[0] == '#')
					continue;
				if (strcmp(line, name) == 0)
					return (1);
			}
		(void) fclose(fd);
	}
	return (0);
}

/*
 * Terminate login as previous user, if any, resetting state;
 * used when USER command is given or login fails.
 */
end_login()
{

	(void) CLR_WORKPRIVS_NON_ADMIN(seteuid((uid_t)0));
	if (logged_in)
		logwtmp(ttyline, "", "", "ftp", getpid());
	ia_closeinfo(uinfo);
	uinfo = (uinfo_t) 0;
	logged_in = 0;
	guest = 0;
}

pass(passwd)
	char *passwd;
{
	char *xpasswd, *salt;
	char *homedir;
	uid_t uid;
	gid_t gid, *grouplist;
	long ngroups, expire, inact, today;
	struct lastlog ll;
	int llfd;

	if (logged_in || askpasswd == 0) {
		reply(503, gettxt(":328", "Login with USER first."));
		return;
	}
	askpasswd = 0;

	if (uinfo == NULL) /* "ftp" is only account allowed no password */
		salt = "xx";
	else {
		ia_get_logpwd(uinfo, &salt);
		ia_get_uid(uinfo, &uid);
	}
	if (!guest) {	/* "ftp" is only account allowed no password */
		xpasswd = crypt(passwd, salt);
		/* The strcmp does not catch null passwords! */
                if (uinfo == NULL || *salt == '\0' || strcmp(xpasswd, salt)) {
			reply(530, gettxt(":329", "Login incorrect."));
			if (uinfo)
				ia_closeinfo(uinfo);
			uinfo = (uinfo_t) 0;
			if (login_attempts++ >= 5) {
				syslog(LOG_NOTICE,
				    gettxt(":330", "repeated login failures from %s"),
				    remotehost);
				exit(0);
			}
			return;
		}
		/*
		 * check for expired login or inactive login
		 */
		today = time((long *) 0) / DAY_SECS;
		ia_get_logexpire(uinfo, &expire);
		if (expire > 0 && expire < today) {
			reply(530, gettxt(":331", "Login has expired."));
		}
		else {
			ia_get_loginact(uinfo, &inact);
			if (inact <= 0)
				goto login_ok;
			if ((llfd = open(LASTLOG, O_RDONLY)) == -1) {
				syslog(LOG_WARNING,
				  gettxt(":332", "Unable to open %s: %m (skipping check)"),
				  LASTLOG);
				goto login_ok;
			}
			(void) lseek(llfd, uid*sizeof(struct lastlog),
			    SEEK_SET);
			if (read(llfd, &ll, sizeof(struct lastlog))
			  != sizeof(struct lastlog)) {
				syslog(LOG_WARNING, gettxt(":333", "Unable to read last login info: %m (skipping check)"));
				goto login_ok;
			}
			(void) close(llfd);
			if (ll.ll_time > 0
			  && (ll.ll_time / DAY_SECS + inact) < today) {
				reply(530, gettxt(":334", "Login inactivity limit exceeded."));
			}
			else {
				goto login_ok;
			}
		}
		/*
		 * login problem
		 */
		ia_closeinfo(uinfo);
		uinfo = (uinfo_t) 0;
		return;
	}

login_ok:
	login_attempts = 0;		/* this time successful */
	ia_get_gid(uinfo, &gid);
	ia_get_sgid(uinfo, &grouplist, &ngroups);
	(void) setegid(gid);
	(void) setgroups(ngroups, grouplist);

	/* open wtmp before chroot */
	(void)sprintf(ttyline, "ftp%d", getpid());
	logwtmp(ttyline, uinfo->ia_name, remotehost, "ftp", getpid());
	logged_in = 1;

	ia_get_dir(uinfo, &homedir);
	if (guest) {
		/*
		 * We MUST do a chdir() after the chroot. Otherwise
		 * the old current directory will be accessible as "."
		 * outside the new root!
		 */
		if (chroot(homedir) < 0 || chdir("/") < 0) {
			reply(550, gettxt(":335", "Can't set guest privileges."));
			goto bad;
		}
	} else if (chdir(homedir) < 0) {
		if (chdir("/") < 0) {
			reply(530, gettxt(":336", "User %s: can't change directory to %s."),
			    uinfo->ia_name, homedir);
			goto bad;
		} else
			lreply(230, gettxt(":337", "No directory! Logging in with home=/"));
	}
	if (CLR_WORKPRIVS_NON_ADMIN(seteuid(uid)) < 0) {
		reply(550, gettxt(":338", "Can't set uid."));
		goto bad;
	}
	if (guest) {
		reply(230, gettxt(":339", "Guest login ok, access restrictions apply."));
#ifdef SETPROCTITLE
		sprintf(proctitle, "%s: anonymous/%.*s", remotehost,
		    sizeof(proctitle) - sizeof(remotehost) -
		    sizeof(": anonymous/"), passwd);
		setproctitle(proctitle);
#endif /* SETPROCTITLE */
		if (logging)
			syslog(LOG_INFO, gettxt(":340", "ANONYMOUS FTP LOGIN FROM %s, %s"),
			    remotehost, passwd);
	} else {
		reply(230, gettxt(":341", "User %s logged in."), uinfo->ia_name);
#ifdef SETPROCTITLE
		sprintf(proctitle, "%s: %s", remotehost, uinfo->ia_name);
		setproctitle(proctitle);
#endif /* SETPROCTITLE */
		if (logging)
			syslog(LOG_INFO, gettxt(":342", "FTP LOGIN FROM %s, %s"),
			    remotehost, uinfo->ia_name);
	}
	home = homedir;		/* home dir for globbing */
	(void) umask(defumask);
	return;
bad:
	/* Forget all about it... */
	end_login();
}

retrieve(cmd, name)
	char *cmd, *name;
{
	char line[BUFSIZ];
	FILE *fin, *dout;
	struct stat st;
	int (*closefunc)();

	if (cmd == 0) {
		fin = fopen(name, "r"), closefunc = fclose;
		st.st_size = 0;
	} else {
		char line[BUFSIZ];

		(void) sprintf(line, cmd, name), name = line;
		fin = ftpd_popen(line, "r"), closefunc = ftpd_pclose;
		st.st_size = -1;
	}
	if (fin == NULL) {
		if (errno != 0)
			perror_reply(550, name);
		return;
	}
	st.st_size = 0;
	if (cmd == 0 &&
	    (fstat(fileno(fin), &st) < 0 || (st.st_mode&S_IFMT) != S_IFREG)) {
		reply(550, gettxt(":266", "%s: not a plain file."), name);
		goto done;
	}
	if (restart_point) {
		if (type == TYPE_A) {
			register int i, n, c;

			n = restart_point;
			i = 0;
			while (i++ < n) {
				if ((c=getc(fin)) == EOF) {
					perror_reply(550, name);
					goto done;
				}
				if (c == '\n')
					i++;
			}	
		} else if (lseek(fileno(fin), restart_point, SEEK_SET) < 0) {
			perror_reply(550, name);
			goto done;
		}
	}
	dout = dataconn(name, st.st_size, "w");
	if (dout == NULL)
		goto done;
	send_data(fin, dout, BUFFER_SIZE);
	(void) fclose(dout);
	data = -1;
	pdata = -1;
done:
	(*closefunc)(fin);
}

store(name, mode, unique)
	char *name, *mode;
	int unique;
{
	FILE *fout, *din;
	struct stat st;
	int (*closefunc)();
	char *gunique();

	if (unique && stat(name, &st) == 0 &&
	    (name = gunique(name)) == NULL)
		return;

	if (restart_point)
		mode = "r+w";
	fout = fopen(name, mode);
	closefunc = fclose;
	if (fout == NULL) {
		perror_reply(553, name);
		return;
	}
	if (restart_point) {
		if (type == TYPE_A) {
			register int i, n, c;

			n = restart_point;
			i = 0;
			while (i++ < n) {
				if ((c=getc(fout)) == EOF) {
					perror_reply(550, name);
					goto done;
				}
				if (c == '\n')
					i++;
			}	
			/*
			 * We must do this seek to "current" position
			 * because we are changing from reading to
			 * writing.
			 */
			if (fseek(fout, 0L, SEEK_CUR) < 0) {
				perror_reply(550, name);
				goto done;
			}
		} else if (lseek(fileno(fout), restart_point, SEEK_SET) < 0) {
			perror_reply(550, name);
			goto done;
		}
	}
	din = dataconn(name, (off_t)-1, "r");
	if (din == NULL)
		goto done;
	if (receive_data(din, fout) == 0) {
		if (unique)
			reply(226, gettxt(":343", "Transfer complete (unique file name:%s)."),
			    name);
		else
			reply(226, gettxt(":344", "Transfer complete."));
	}
	(void) fclose(din);
	data = -1;
	pdata = -1;
done:
	(*closefunc)(fout);
}

FILE *
getdatasock(mode)
	char *mode;
{
	int s, on = 1, tries;
	uid_t uid;

	ia_get_uid(uinfo, &uid);

	if (data >= 0)
		return (fdopen(data, mode));
	(void) CLR_WORKPRIVS_NON_ADMIN(seteuid((uid_t)0));
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0) {
		CLR_WORKPRIVS_NON_ADMIN(seteuid(uid));
		return (NULL);
	}
	if (is_enhancedsecurity()) {
		fd_set_to_my_level(s);
	}
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
	    (char *) &on, sizeof (on)) < 0)
		goto bad;
	/* anchor socket to avoid multi-homing problems */
	data_source.sin_family = AF_INET;
	data_source.sin_addr = ctrl_addr.sin_addr;
	/*
	** Since there's a window between the bind and the connect done
	** in dataconn, we can get an EADDRINUSE on this bind to the
	** ftp-data port.  So, we'll wait & try again if that happens. 
	*/
	for (tries = 1; ; tries++) {
		if (bind(s, (struct sockaddr *)&data_source,
		    sizeof (data_source)) >= 0)
			break;
		if (errno != EADDRINUSE || tries > 10)
			goto bad;
		sleep(tries);
	}
	(void) CLR_WORKPRIVS_NON_ADMIN(seteuid(uid));
#ifdef IP_TOS
	on = IPTOS_THROUGHPUT;
	if (setsockopt(s, IPPROTO_IP, IP_TOS, (char *)&on, sizeof(int)) < 0)
		syslog(LOG_WARNING, "setsockopt (IP_TOS): %m");
#endif
	return (fdopen(s, mode));
bad:
	(void) CLR_WORKPRIVS_NON_ADMIN(seteuid(uid));
	(void) close(s);
	return (NULL);
}

FILE *
dataconn(name, size, mode)
	char *name;
	off_t size;
	char *mode;
{
	char sizebuf[32];
	FILE *file;
	int retry = 0, tos;

	file_size = size;
	byte_count = 0;
	if (size != (off_t) -1)
		(void) sprintf (sizebuf, gettxt(":345", " (%ld bytes)"), size);
	else
		(void) strcpy(sizebuf, "");
	if (pdata >= 0) {
		struct sockaddr_in from;
		int s, fromlen = sizeof(from);

		s = accept(pdata, (struct sockaddr *)&from, &fromlen);
		if (s < 0) {
			reply(425, gettxt(":346", "Can't open data connection."));
			(void) close(pdata);
			pdata = -1;
			return(NULL);
		}
		(void) close(pdata);
		pdata = s;
#ifdef IP_TOS
		tos = IPTOS_LOWDELAY;
		(void) setsockopt(s, IPPROTO_IP, IP_TOS, (char *)&tos,
		    sizeof(int));
#endif
		reply(150, gettxt(":347", "Opening %s mode data connection for %s%s."),
		     type == TYPE_A ? "ASCII" : "BINARY", name, sizebuf);
		return(fdopen(pdata, mode));
	}
	if (data >= 0) {
		reply(125, gettxt(":348", "Using existing data connection for %s%s."),
		    name, sizebuf);
		usedefault = 1;
		return (fdopen(data, mode));
	}
	if (usedefault)
		data_dest = his_addr;
	usedefault = 1;
	file = getdatasock(mode);
	if (file == NULL) {
		reply(425, gettxt(":349", "Can't create data socket (%s,%d): %s."),
		    inet_ntoa(data_source.sin_addr),
		    ntohs(data_source.sin_port),
		    errno < sys_nerr ? sys_errlist[errno] : "unknown error");
		return (NULL);
	}
	data = fileno(file);
	while (connect(data, (struct sockaddr *)&data_dest,
	    sizeof (data_dest)) < 0) {
		if (errno == EADDRINUSE && retry < swaitmax) {
			sleep((unsigned) swaitint);
			retry += swaitint;
			continue;
		}
		perror_reply(425, gettxt(":350", "Can't build data connection"));
		(void) fclose(file);
		data = -1;
		return (NULL);
	}
	reply(150, gettxt(":347", "Opening %s mode data connection for %s%s."),
	     type == TYPE_A ? "ASCII" : "BINARY", name, sizebuf);
	return (file);
}

/*
 * Tranfer the contents of "instr" to
 * "outstr" peer using the appropriate
 * encapsulation of the data subject
 * to Mode, Structure, and Type.
 *
 * NB: Form isn't handled.
 */
send_data(instr, outstr, blksize)
	FILE *instr, *outstr;
	off_t blksize;
{
	register int c, cnt;
	register char *buf;
	int netfd, filefd;
	extern void *malloc();

	transflag++;
	if (setjmp(urgcatch)) {
		transflag = 0;
		return;
	}
	switch (type) {

	case TYPE_A:
		while ((c = getc(instr)) != EOF) {
			byte_count++;
			if (c == '\n') {
				if (ferror(outstr))
					goto data_err;
				(void) putc('\r', outstr);
			}
			(void) putc(c, outstr);
		}
		fflush(outstr);
		transflag = 0;
		if (ferror(instr))
			goto file_err;
		if (ferror(outstr))
			goto data_err;
		reply(226, gettxt(":344", "Transfer complete."));
		return;

	case TYPE_I:
	case TYPE_L:
		if ((buf = (char *) malloc((u_int)blksize)) == NULL) {
			transflag = 0;
			perror_reply(451, gettxt(":351", "Local resource failure: malloc"));
			return;
		}
		netfd = fileno(outstr);
		filefd = fileno(instr);
		while ((cnt = read(filefd, buf, (u_int)blksize)) > 0 &&
		    write(netfd, buf, cnt) == cnt)
			byte_count += cnt;
		transflag = 0;
		(void)free(buf);
		if (cnt != 0) {
			if (cnt < 0)
				goto file_err;
			goto data_err;
		}
		reply(226, gettxt(":344", "Transfer complete."));
		return;
	default:
		transflag = 0;
		reply(550, gettxt(":352", "Unimplemented TYPE %d in send_data"), type);
		return;
	}

data_err:
	transflag = 0;
	perror_reply(426, gettxt(":353", "Data connection"));
	return;

file_err:
	transflag = 0;
	perror_reply(551, gettxt(":354", "Error on input file"));
}

/*
 * Transfer data from peer to
 * "outstr" using the appropriate
 * encapulation of the data subject
 * to Mode, Structure, and Type.
 *
 * N.B.: Form isn't handled.
 */
receive_data(instr, outstr)
	FILE *instr, *outstr;
{
	register int c;
	int cnt, bare_lfs = 0;
	char buf[BUFFER_SIZE];

	transflag++;
	if (setjmp(urgcatch)) {
		transflag = 0;
		return (-1);
	}
	switch (type) {

	case TYPE_I:
	case TYPE_L:
		while ((cnt = read(fileno(instr), buf, sizeof buf)) > 0) {
			if (write(fileno(outstr), buf, cnt) != cnt)
				goto file_err;
			byte_count += cnt;
		}
		if (cnt < 0)
			goto data_err;
		transflag = 0;
		return (0);

	case TYPE_E:
		reply(553, gettxt(":355", "TYPE E not implemented."));
		transflag = 0;
		return (-1);

	case TYPE_A:
		while ((c = getc(instr)) != EOF) {
			byte_count++;
			if (c == '\n')
				bare_lfs++;
			while (c == '\r') {
				if (ferror(outstr))
					goto data_err;
				if ((c = getc(instr)) != '\n') {
					(void) putc ('\r', outstr);
					if (c == '\0' || c == EOF)
						goto contin2;
				}
			}
			(void) putc(c, outstr);
	contin2:	;
		}
		fflush(outstr);
		if (ferror(instr))
			goto data_err;
		if (ferror(outstr))
			goto file_err;
		transflag = 0;
		if (bare_lfs) {
			lreply(230, gettxt(":356", "WARNING! %d bare linefeeds received in ASCII mode"), bare_lfs);
			pfmt(stdout, MM_NOSTD, ":357:   File may not have transferred correctly.\r\n");
		}
		return (0);
	default:
		reply(550, gettxt(":358", "Unimplemented TYPE %d in receive_data"), type);
		transflag = 0;
		return (-1);
	}

data_err:
	transflag = 0;
	perror_reply(426, gettxt(":359", "Data Connection"));
	return (-1);

file_err:
	transflag = 0;
	perror_reply(452, gettxt(":360", "Error writing file"));

	/*
	 * if we close data connection before all data's sent, then
	 * client will write to closed socket and fail with EPIPE, without
	 * ever reading our reply to see what the real problem was.
	 * reading and discarding the remaining data will let the correct
	 * error get reported on client side.
	 */
	while (read(fileno(instr), buf, sizeof buf) > 0)
		continue;

	return (-1);
}

statfilecmd(filename)
	char *filename;
{
	char line[BUFSIZ];
	FILE *fin;
	int c;

	(void) sprintf(line, "/bin/ls -l %s", filename);
	fin = ftpd_popen(line, "r");
	lreply(211, gettxt(":361", "status of %s:"), filename);
	while ((c = getc(fin)) != EOF) {
		if (c == '\n') {
			if (ferror(stdout)){
				perror_reply(421, gettxt(":362", "control connection"));
				(void) ftpd_pclose(fin);
				dologout(1);
				/* NOTREACHED */
			}
			if (ferror(fin)) {
				perror_reply(551, filename);
				(void) ftpd_pclose(fin);
				return;
			}
			(void) putc('\r', stdout);
		}
		(void) putc(c, stdout);
	}
	(void) ftpd_pclose(fin);
	reply(211, gettxt(":363", "End of Status"));
}

statcmd()
{
	struct sockaddr_in *sin;
	u_char *a, *p;

	lreply(211, gettxt(":364", "%s FTP server status:"), hostname, version);
	printf("     %s\r\n", version);
	pfmt(stdout, MM_NOSTD, ":365     Connected to %s", remotehost);
	if (!isdigit(remotehost[0]))
		printf(" (%s)", inet_ntoa(his_addr.sin_addr));
	printf("\r\n");
	if (logged_in) {
		if (guest)
			pfmt(stdout, MM_NOSTD, ":366     Logged in anonymously\r\n");
		else
			pfmt(stdout, MM_NOSTD, ":367     Logged in as %s\r\n", uinfo->ia_name);
	} else if (askpasswd)
		pfmt(stdout, MM_NOSTD, ":368     Waiting for password\r\n");
	else
		pfmt(stdout, MM_NOSTD, ":369     Waiting for user name\r\n");
	printf("     TYPE: %s", typenames[type]);
	if (type == TYPE_A || type == TYPE_E)
		printf(", FORM: %s", formnames[form]);
	if (type == TYPE_L)
#if NBBY == 8
		printf(" %d", NBBY);
#else
		printf(" %d", bytesize);	/* need definition! */
#endif
	pfmt(stdout, MM_NOSTD, ":370; STRUcture: %s; transfer MODE: %s\r\n",
	    strunames[stru], modenames[mode]);
	if (data != -1)
		pfmt(stdout, MM_NOSTD, ":371     Data connection open\r\n");
	else if (pdata != -1) {
		pfmt(stdout, MM_NOSTD, ":372     in Passive mode");
		sin = &pasv_addr;
		goto printaddr;
	} else if (usedefault == 0) {
		pfmt(stdout, MM_NOSTD, ":373     PORT");
		sin = &data_dest;
printaddr:
		a = (u_char *) &sin->sin_addr;
		p = (u_char *) &sin->sin_port;
#define UC(b) (((int) b) & 0xff)
		printf(" (%d,%d,%d,%d,%d,%d)\r\n", UC(a[0]),
			UC(a[1]), UC(a[2]), UC(a[3]), UC(p[0]), UC(p[1]));
#undef UC
	} else
		pfmt(stdout, MM_NOSTD, ":374     No data connection\r\n");
	reply(211, gettxt(":375", "End of status"));
}

fatal(s)
	char *s;
{
	reply(451, gettxt(":376", "Error in server: %s\n"), s);
	reply(221, gettxt(":377", "Closing connection due to server error."));
	dologout(0);
	/* NOTREACHED */
}

/* VARARGS2 */
reply(va_alist)
va_dcl
{
	int n;
	char *fmt;
{
	va_list args;

	va_start(args);
	n = va_arg(args, int);
	fmt = va_arg(args, char *);
	printf("%d ", n);
	vprintf(fmt, args);
	va_end(args);
	printf("\r\n");
	(void)fflush(stdout);
	if (debug) {
		char obuf[1024];
		va_start(args);
		n = va_arg(args, int);
		fmt = va_arg(args, char *);
		syslog(LOG_DEBUG, "<--- %d ", n);
		vsprintf(obuf, fmt, args);
		syslog(LOG_DEBUG, obuf);
		va_end(args);
	}
}
}

/*VARARGS2*/
lreply(va_alist)
va_dcl
{
	int n;
	char *s;
	va_list args;

	va_start(args);
	n = va_arg(args, int);
	s = va_arg(args, char *);

	printf("%d-", n);
	vprintf(s, args);
	va_end(args);
	printf("\r\n");
	(void)fflush(stdout);
	if (debug) {
		char obuf[1024];
		va_start(args);
		n = va_arg(args, int);
		s = va_arg(args, char *);
		syslog(LOG_DEBUG, "<--- %d- ", n);
		vsprintf(obuf, s, args);
		syslog(LOG_DEBUG, obuf);
		va_end(args);
	}
}

ack(s)
	char *s;
{
	reply(250, gettxt(":378", "%s command successful."), s);
}

nack(s)
	char *s;
{
	reply(502, gettxt(":379", "%s command not implemented."), s);
}

/* ARGSUSED */
yyerror(s)
	char *s;
{
	char *cp;

	if (cp = index(cbuf,'\n'))
		*cp = '\0';
	reply(500, gettxt(":380", "'%s': command not understood."), cbuf);
}

delete(name)
	char *name;
{
	struct stat st;

	if (stat(name, &st) < 0) {
		perror_reply(550, name);
		return;
	}
	if ((st.st_mode&S_IFMT) == S_IFDIR) {
		if (rmdir(name) < 0) {
			perror_reply(550, name);
			return;
		}
		goto done;
	}
	if (unlink(name) < 0) {
		perror_reply(550, name);
		return;
	}
done:
	ack("DELE");
}

cwd(path)
	char *path;
{
	if (chdir(path) < 0)
		perror_reply(550, path);
	else
		ack("CWD");
}

makedir(name)
	char *name;
{
	if (mkdir(name, 0777) < 0)
		perror_reply(550, name);
	else
		reply(257, gettxt(":381", "MKD command successful."));
}

removedir(name)
	char *name;
{
#if defined(M_XENIX)
	struct stat sb;
	
	if (stat(name, &sb) < 0) {
		perror_reply(550, name);
		return;
	}
	if ((sb.st_mode&S_IFMT) == S_IFDIR) {
#endif
	if (rmdir(name) < 0)
		perror_reply(550, name);
	else
		ack("RMD");
#if defined(M_XENIX)
	} else {
		errno = ENOTDIR;
		perror_reply(550, name);
	}
#endif 
}

pwd()
{
	extern char *getcwd();
	char   path[MAXPATHLEN + 1];

	if (getcwd(path, sizeof(path)) == (char *)NULL) {
		if (errno >= sys_nerr || errno < 0)
			errno = 0;
		sprintf(path, gettxt(":382", "Can't get current directory: %s."),
			sys_errlist[errno]);
		reply(550, "%s.", path);
	} else 
		reply(257, gettxt(":383", "\"%s\" is current directory."), path);
}

char *
renamefrom(name)
	char *name;
{
	struct stat st;

	if (stat(name, &st) < 0) {
		perror_reply(550, name);
		return ((char *)0);
	}
	reply(350, gettxt(":384", "File exists, ready for destination name"));
	return (name);
}

renamecmd(from, to)
	char *from, *to;
{
	if (rename(from, to) < 0)
		perror_reply(550, gettxt(":385", "rename"));
	else
		ack("RNTO");
}

dolog(sin)
	struct sockaddr_in *sin;
{
	struct hostent *hp = gethostbyaddr((char *)&sin->sin_addr,
		sizeof (struct in_addr), AF_INET);
	time_t t, time();
	extern char *ctime();

	if (hp)
		(void) strncpy(remotehost, hp->h_name, sizeof (remotehost));
	else
		(void) strncpy(remotehost, inet_ntoa(sin->sin_addr),
		    sizeof (remotehost));
#ifdef SETPROCTITLE
	sprintf(proctitle, gettxt(":386", "%s: connected"), remotehost);
	setproctitle(proctitle);
#endif /* SETPROCTITLE */

	if (logging) {
		t = time((time_t *) 0);
		syslog(LOG_INFO, gettxt(":387", "connection from %s at %s"),
		    remotehost, ctime(&t));
	}
}

/*
 * Record logout in wtmp file
 * and exit with supplied status.
 */
dologout(status)
	int status;
{
	if (logged_in) {
		(void) CLR_WORKPRIVS_NON_ADMIN(seteuid((uid_t)0));
		logwtmp(ttyline, "", "", "ftp", getpid());
	}
	/* beware of flushing buffers after a SIGPIPE */
	_exit(status);
}

myoob()
{
	char *cp;

	/* only process if transfer occurring */
	if (!transflag)
		return;
	cp = tmpline;
	if (getline(cp, 7, stdin) == NULL) {
		reply(221, gettxt(":308", "You could at least say goodbye."));
		dologout(0);
	}
	upper(cp);
	if (strcmp(cp, "ABOR\r\n") == 0) {
		tmpline[0] = '\0';
		reply(426, gettxt(":388", "Transfer aborted. Data connection closed."));
		reply(226, gettxt(":389", "Abort successful"));
		longjmp(urgcatch, 1);
	}
	if (strcmp(cp, "STAT\r\n") == 0) {
		if (file_size != (off_t) -1)
			reply(213, gettxt(":390", "Status: %lu of %lu bytes transferred"),
			    byte_count, file_size);
		else
			reply(213, gettxt(":391", "Status: %lu bytes transferred"), byte_count);
	}
}

/*
 * Note: a response of 425 is not mentioned as a possible response to
 * 	the PASV command in RFC959. However, it has been blessed as
 * 	a legitimate response by Jon Postel in a telephone conversation
 *	with Rick Adams on 25 Jan 89.
 */
passive()
{
	int len;
	register char *p, *a;
	uid_t uid;

	ia_get_uid(uinfo, &uid);

	pdata = socket(AF_INET, SOCK_STREAM, 0);
	if (pdata < 0) {
		perror_reply(425, gettxt(":392", "Can't open passive connection"));
		return;
	}
	pasv_addr = ctrl_addr;
	pasv_addr.sin_port = 0;
	(void) CLR_WORKPRIVS_NON_ADMIN(seteuid((uid_t)0));
	if (bind(pdata, (struct sockaddr *)&pasv_addr, sizeof(pasv_addr)) < 0) {
		(void) CLR_WORKPRIVS_NON_ADMIN(seteuid(uid));
		goto pasv_error;
	}
#ifdef	SYSV
	if (is_enhancedsecurity()) {
		fd_set_to_my_level(pdata);
	}
#endif	SYSV
	(void) CLR_WORKPRIVS_NON_ADMIN(seteuid(uid));
	len = sizeof(pasv_addr);
	if (getsockname(pdata, (struct sockaddr *) &pasv_addr, &len) < 0)
		goto pasv_error;
	if (listen(pdata, 1) < 0)
		goto pasv_error;
	a = (char *) &pasv_addr.sin_addr;
	p = (char *) &pasv_addr.sin_port;

#define UC(b) (((int) b) & 0xff)

	reply(227, gettxt(":393", "Entering Passive Mode (%d,%d,%d,%d,%d,%d)"), UC(a[0]),
		UC(a[1]), UC(a[2]), UC(a[3]), UC(p[0]), UC(p[1]));
	return;

pasv_error:
	(void) close(pdata);
	pdata = -1;
	perror_reply(425, gettxt(":392", "Can't open passive connection"));
	return;
}

/*
 * Generate unique name for file with basename "local".
 * The file named "local" is already known to exist.
 * Generates failure reply on error.
 */
char *
gunique(local)
	char *local;
{
	static char new[MAXPATHLEN];
	struct stat st;
	char *cp = rindex(local, '/');
	int count = 0;

	if (cp)
		*cp = '\0';
	if (stat(cp ? local : ".", &st) < 0) {
		perror_reply(553, cp ? local : ".");
		return((char *) 0);
	}
	if (cp)
		*cp = '/';
	(void) strcpy(new, local);
	cp = new + strlen(new);
	*cp++ = '.';
	for (count = 1; count < 100; count++) {
		(void) sprintf(cp, "%d", count);
		if (stat(new, &st) < 0)
			return(new);
	}
	reply(452, gettxt(":394", "Unique file name cannot be created."));
	return((char *) 0);
}

/*
 * Format and send reply containing system error number.
 */
perror_reply(code, string)
	int code;
	char *string;
{
        reply(code, "%s: %s.", string, strerror(errno));
}

static char *onefile[] = {
	"",
	0
};

send_file_list(whichfiles)
	char *whichfiles;
{
	struct stat st;
	DIR *dirp = NULL;
	struct dirent *dir;
	FILE *dout = NULL;
	register char **dirlist, *dirname;
	int simple = 0;
	char *strpbrk();

	if (strpbrk(whichfiles, "~{[*?") != NULL) {
		extern char **glob(), *globerr;

		globerr = NULL;
		dirlist = glob(whichfiles);
		if (globerr != NULL) {
			reply(550, globerr);
			return;
		} else if (dirlist == NULL) {
			errno = ENOENT;
			perror_reply(550, whichfiles);
			return;
		}
	} else {
		onefile[0] = whichfiles;
		dirlist = onefile;
		simple = 1;
	}

	if (setjmp(urgcatch)) {
		transflag = 0;
		return;
	}
	while (dirname = *dirlist++) {
		if (stat(dirname, &st) < 0) {
			/*
			 * If user typed "ls -l", etc, and the client
			 * used NLST, do what the user meant.
			 */
			if (dirname[0] == '-' && *dirlist == NULL &&
			    transflag == 0) {
				retrieve("/bin/ls %s", dirname);
				return;
			}
			perror_reply(550, whichfiles);
			if (dout != NULL) {
				(void) fclose(dout);
				transflag = 0;
				data = -1;
				pdata = -1;
			}
			return;
		}

		if ((st.st_mode&S_IFMT) == S_IFREG) {
			if (dout == NULL) {
				dout = dataconn(gettxt(":395", "file list"), (off_t)-1, "w");
				if (dout == NULL)
					return;
				transflag++;
			}
			fprintf(dout, "%s%s\n", dirname,
				type == TYPE_A ? "\r" : "");
			byte_count += strlen(dirname) + 1;
			continue;
		} else if ((st.st_mode&S_IFMT) != S_IFDIR)
			continue;

		if ((dirp = opendir(dirname)) == NULL)
			continue;

		while ((dir = readdir(dirp)) != NULL) {
			char nbuf[MAXPATHLEN];

			if (dir->d_name[0] == '.' && dir->d_reclen == 1)
				continue;
			if (dir->d_name[0] == '.' && dir->d_name[1] == '.' &&
			    dir->d_reclen == 2)
				continue;

			sprintf(nbuf, "%s/%s", dirname, dir->d_name);

			/*
			 * We have to do a stat to insure it's
			 * not a directory or special file.
			 */
			if (simple || (stat(nbuf, &st) == 0 &&
			    (st.st_mode&S_IFMT) == S_IFREG)) {
				if (dout == NULL) {
					dout = dataconn(gettxt(":395", "file list"), (off_t)-1,
						"w");
					if (dout == NULL)
						return;
					transflag++;
				}
				if (nbuf[0] == '.' && nbuf[1] == '/')
					fprintf(dout, "%s%s\n", &nbuf[2],
						type == TYPE_A ? "\r" : "");
				else
					fprintf(dout, "%s%s\n", nbuf,
						type == TYPE_A ? "\r" : "");
				byte_count += strlen(nbuf) + 1;
			}
		}
		(void) closedir(dirp);
	}

	if (dout == NULL)
		reply(550, gettxt(":396", "No files found."));
	else if (ferror(dout) != 0)
		perror_reply(550, gettxt(":353", "Data connection"));
	else
		reply(226, gettxt(":344", "Transfer complete."));

	transflag = 0;
	if (dout != NULL)
		(void) fclose(dout);
	data = -1;
	pdata = -1;
}

#ifdef SETPROCTITLE
/*
 * clobber argv so ps will show what we're doing.
 * (stolen from sendmail)
 * warning, since this is usually started from inetd.conf, it
 * often doesn't have much of an environment or arglist to overwrite.
 */

/*VARARGS1*/
setproctitle(fmt, a, b, c)
char *fmt;
{
	register char *p, *bp, ch;
	register int i;
	char buf[BUFSIZ];

	(void) sprintf(buf, fmt, a, b, c);
	/* make ps print our process name */
	p = Argv[0];
	*p++ = '-';

	i = strlen(buf);
	if (i > LastArgv - p - 2) {
		i = LastArgv - p - 2;
		buf[i] = '\0';
	}
	bp = buf;
	while (ch = *bp++)
		if (ch != '\n' && ch != '\r')
			*p++ = ch;
	while (p < LastArgv)
		*p++ = ' ';
}
#endif /* SETPROCTITLE */

/*
 * leadpath -- Return the leading component of a file name, if 
 * there is one, or the current directory if there isn't.
 */

char nbuf[MAXPATHLEN + 1];

char *
leadpath(name)
	char *name;
{
	char *s;
	char *t;
	extern char *getcwd();

	(void)bzero(nbuf, sizeof(nbuf));

	if ((s = strrchr(name, '/')) == (char *)0) {
		t = getcwd((char *)0, MAXPATHLEN + 1);
		if (t) {
			(void)strcpy(nbuf, t);
			(void)free(t);
		} else
			strcpy(nbuf, "./");
	} else {
		if (s == name)
			strcpy(nbuf, "/");
		else
			strncpy(nbuf, name, (int)(s - name));
	}
	return &nbuf[0];
}

int
frkrename(from, to, uid, gid)
	char	*from;
	char	*to;
	int	uid;
	int	gid;
{
	int wstat;
	int w;
	int pid;
	struct stat parent, target;

	if (stat(leadpath(from), &parent) < 0)
		return -1;
	if (stat(from, &target) < 0)
		return -1;

#if (INTEL > 31) || (ATT > 31)
	if (parent.st_mode&S_ISVTX && uid != 0 &&
		parent.st_uid != uid && target.st_uid != uid)
			return(EACCES);
#endif
	if ((target.st_mode&S_IFMT) == S_IFDIR) {
		int	status, pid, w;
		register void (*istat)(), (*qstat)();

		sigset(SIGCLD, SIG_DFL);
		if((pid = fork()) == 0) {
			setgid(gid);
			setuid(uid);
			execl("/usr/lib/mv_dir", "mv", from, to, (char *)0);
			_exit(127);
		}
		istat = sigset(SIGINT, SIG_IGN);
		qstat = sigset(SIGQUIT, SIG_IGN);
		while((w = wait(&status)) != pid && w != -1)
			;
		(void) sigset(SIGINT, istat);
		(void) sigset(SIGQUIT, qstat);
		return((w == -1)? w: status);
	} else {
		(void) unlink(to);
		if (link(from, to) < 0)
			return(errno);

		(void) unlink(from);
		return(0);
	}
}

