/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/smtpd.c	1.10.5.3"
#ident "@(#)smtpd.c	1.27 'attmail mail(1) command'"

/*
 * smtpd - SMTP listener: receives SMTP mail & invokes rmail.
 */

#ifdef INETD
/* in.smtpd only runs in a sockets world */
#undef TLI
#endif

#include "libmail.h"
#include <sgtty.h>
#ifdef SVR4_1
# include <sys/ksym.h>
#else
# include <nlist.h>
#endif
#ifdef TLI
# include <netconfig.h>
# include <netdir.h>
# include <poll.h>
# include <tiuser.h>
# include <stropts.h>
#endif
#if defined(SOCKET) || defined(BIND)
# ifndef SVR3
#  include <sys/uio.h>
# endif
# include <netdb.h>
# include <sys/socket.h>
# ifdef SYS_INET_H
#  include <sys/in.h>	/* WIN/3B */
#  include <sys/inet.h>	/* WIN/3B */
# endif
# ifdef NETINET_IN_H
#  include <netinet/in.h>
# endif
# ifdef ARPA_INET_H
#  include <arpa/inet.h>
# endif
#endif
#ifdef SVR4_1
#include <mac.h>
#endif
#ifdef PRIV
#include <priv.h>
#endif

#include "smtp.h"
#include "xmail.h"
#include "smtpd_decl.h"
#ifdef TLI
#include "t_decl.h"
#endif

/* forward declarations */
SIGRETURN	reapchild proto((int));
SIGRETURN	decrlimit proto((int));
SIGRETURN	incrlimit proto((int));
SIGRETURN	idlesmtp proto((int));
double		loadav proto((void));
void		initla proto((void));
long		starttime;
extern char *full_domain_name proto((char *domain));

extern	char **environ;
extern	int errno, sys_nerr;
extern	char *sys_errlist[];
extern	char *sysname_read proto((void));

#ifdef SOCKET
struct sockaddr_in sin = { AF_INET };
struct sockaddr_in from;
struct servent *getservbyname proto((char *, char *));
#endif

#ifdef SOCKET
#ifndef SERVNAME
#ifndef	DEBUG
#define	SERVNAME "smtp"
#else				/*DEBUG*/
#define SERVNAME "smtpdebug"
#endif				/*DEBUG*/
#endif				/* SERVNAME */
#endif				/* SOCKET */

int	debug;
#ifdef INETD
const char	*progname = "in.smtpd";	/* Needed for logging */
#else
const char	*progname = "smtpd";	/* Needed for logging */
#endif
char	*helohost = NULL;
char	*thishost = NULL;
int	accepted;
int	norun;

/* Logging stuff for smtpd */

/*
 * This is a macro because it is used frequently in signal routines, and
 * in 4.3 BSD procedure calls seem to be unreliable from the signal
 * processor, even though the mail routine is almost certainly idle
 * waiting for a socket connection.
 */

#ifdef SVR4_1
#ifndef vax
#define AVENRUN "avenrun"
#else
#define AVENRUN "_avenrun"
#endif
#else

/*
 * Define NLIST_BUG if you have the nlist() bug
 * which causes large process growth
 */
#ifdef SVR4
#  define NLIST_BUG 1
#  define SLASH_UNIX "/stand/unix"
#else
#  define SLASH_UNIX "/unix"
#endif

struct	nlist nl[] = {
#ifndef	vax
	{ "avenrun" },
#else
	{ "_avenrun" },
#endif
#define	X_AVENRUN	0
	{ "" },
};
#endif

int	kmem;
char	*inet_ntoa proto((struct in_addr));

int	r_opt = 1;		/* 1 -> rewrite from/to/cc/bcc headers */
int	idled = 0;		/* idling down */
double	load;			/* current system load */
double	loadlim = 0.0;		/* maximum system load before we reject calls */
int	running = 0;		/* number of smtpd-s running at present */
int	maxrunning = 0;		/* max number of simultaneous smtpd-s
				-1 = infinite */
char	*caller = NULL;		/* who is calling us */

#ifdef SYSLOG
#define	logit(sev, fmt, str) { \
	(void) sprintf(logm, "%s: %s", progname, fmt); \
	Syslog(sev, logm, (str == "" && errno <= sys_nerr)? \
		sys_errlist[errno]: str); \
}
#else

logit(sev, fmt, str)
char *fmt, *str;
{
	char msg[BUFSIZ];

	if (*str == '\0' && errno <= sys_nerr)
		(void) sprintf(msg, fmt, sys_errlist[errno]);
	else
		(void) sprintf(msg, fmt, str);
	smtplog(msg);
}

#endif


void
showstatus(status)
char	*status;
{
	char msg[BUFSIZ];

	if (caller == NULL)
		sprintf(msg, "%-9s count=%d/%d, load=%.2f/%.2f%s",
			status, running, maxrunning, load, loadlim,
			idled? " Idled": "");
	else
		sprintf(msg, "%s: %-9s count=%d/%d, load=%.2f/%.2f%s",
			caller, status, running, maxrunning, load, loadlim,
			idled? " Idled": "");
	smtplog(msg);
}

/*
 * Report mail transfer statistics from conversed.  The V9 version
 * of this routine is empty.
 */
void
xferstatus(msg, nbytes)
char *msg;
long nbytes;
{
	int flag;
	long finishtime;
	char msg2[BUFSIZ];

	(void) time(&finishtime);
	flag = (nbytes < 0L);
	sprintf(msg2, "%s: %s connected %ld seconds, %s%ld%s",
		caller, msg, (long)(finishtime-starttime),
		flag ? "err=" : "",
		nbytes,
		flag ? "" : " bytes");
	smtplog(msg2);
}

/*
 * Log an attempted DEBUG command, in response to the sendmail virus.
 */

/* VARARGS1 PRINTFLIKE1 */
void
#ifdef __STDC__
logdanger(char *format, ...)
#else
# ifdef lint
logdanger(Xformat, va_alist)
char *Xformat;
va_dcl
# else
logdanger(va_alist)
va_dcl
# endif
#endif
{
#ifndef __STDC__
	char *format;
#endif
	char msg[512], buf[512];
	va_list args;

#ifndef __STDC__
# ifdef lint
	format = Xformat;
# endif
#endif

#ifdef __STDC__
	va_start(args, format);
#else
	va_start(args);
	format = va_arg(args, char *);
#endif

	vsprintf(buf, format, args);
	sprintf(msg, "%s: %s", caller, buf);
	smtplog(msg);
}

/*
 * main - parse arguments and handle options
 */
main(argc, argv)
int argc;
char *argv[];
{
	int c;
	int errflg = 0;
	extern int optind;
	extern char *optarg;

#ifdef SVR4_1
#ifdef PRIV
	procprivl(CLRPRV, ALLPRIVS_W, (priv_t)0);
#endif
	(void) setcat("uxsmtp");
#ifdef INETD
	(void) setlabel("UX:in.smtpd");
#else
	(void) setlabel("UX:smtpd");
#endif
	(void) setlocale(LC_ALL, "");
	(void) mldmode(MLD_VIRT);
#endif
	signal(SIGCLD, SIG_DFL); /* Allow us to wait on children */
	while ((c = getopt(argc, argv, "rnH:h:dL:l:F")) != EOF)
		switch (c) {
		case 'r':
			r_opt = 0;
			break;
		case 'n':
			norun = 1;
			break;
		case 'F':
			helohost = full_domain_name((char*)0);
			break;
		case 'H':
			helohost = optarg;
			break;
		case 'h':
			thishost = optarg;
			break;
		case 'd':
			++debug;
			break;
		case 'L':
			loadlim = atoi(optarg);
			break;
		case 'l':
			maxrunning = atoi(optarg);
			break;
		case '?':
		default:
			errflg++;
			break;
		}
	if (errflg) {
		(void) Syslog(LOG_CRIT, "Usage: %s [-F] [-L sysloadlimit] [-l smtpdlim] [-n] [-h thishost] [-H helohost] [-r]\n",
						progname);
		(void) pfmt(stderr, MM_ACTION, ":85:usage: %s [-F] [-L sysloadlimit] [-l smtpdlim] [-n] [-h thishost] [-H helohost] [-r]\n",
			progname);
		exit(2);
	}
	if (helohost==NULL)
		helohost=sysname_read();

	if (optind >= argc)
		process();
#ifdef INETD
	else if (optind == argc - 1) {		/* one argument */
#ifdef SOCKET
		if (sscanf(argv[optind], "%lx.%hd", &from.sin_addr.s_addr,
		    &from.sin_port) != 2) {
			(void) Syslog(LOG_CRIT,
				"in.smtpd: bad arg from inetd: %s\n",
				argv[optind]);
			exit(2);
		}
		from.sin_family = AF_INET;
		from.sin_addr.s_addr = htonl(from.sin_addr.s_addr);
		from.sin_port = htons(from.sin_port);
		process();
#endif				/* SOCKET */
	}
#endif				/* INETD */
	else {
		(void) Syslog(LOG_CRIT, "%s: too many args\n", progname);
		(void) pfmt(stderr, MM_ACTION, ":76:%s: too many args\n", progname);
		exit(2);
	}

	exit(0);
}

/*
 *  process() can be built in one of three ways:
 *	TLI	=> will use SVR4 TLI interface (no sockets stuff)
 *	INETD	=> uses sockets (as if forked of by /usr/sbin/inetd)
 *	SOCKET	=> creates and uses sockets.
 */

#ifdef TLI

extern void t_log proto((char*));

#define	NFD	20			/* Max # of transports */
#define	MAXCONN	10			/* Max # of connections per transport */

struct pollfd	fds[NFD];		/* Transport file descriptors */
struct netconfig ncf[NFD];		/* We need this later */
struct t_call *	calls[NFD][MAXCONN];	/* Call indications */
unsigned long	nfd;			/* How many transports are open */

char *strsave(str)
char *str;
{
	register char *p;

	if ((p = malloc(strlen(str) + 1)) == NULL) {
		smtplog("out of core!");
		exit(1);	/* nuts */
	}
	(void) strcpy(p, str);
	return p;
}

char **strsave2(strs, n)
char **strs;
unsigned long n;
{
	register char **p;
	register int i;

	if ((p = (char **)malloc(n * sizeof(char *))) == NULL) {
		smtplog("out of core!");
		exit(1);	/* nuts */
	}
	for (i = 0; i < n; i++)
		p[i] = strsave(strs[i]);
	return p;
}

int my_netdir_getbyname(ncp, ndh, nap)
struct netconfig *ncp;
struct nd_hostserv *ndh;
struct nd_addrlist **nap;
{
#ifdef BIND
	/* Special case tcp (so that we can open all interfaces (su89-28602)) */
	if (strcmp(ncp->nc_proto, "tcp") == 0) {
		register struct sockaddr_in *xp;
		register struct netbuf *nbuf;
		register struct servent *sp;
		struct sockaddr_in sin;

		if ((sp = getservbyname("smtp", "tcp")) == NULL)
			return 1;
		*nap = (struct nd_addrlist *) malloc(sizeof(struct nd_addrlist));
		if (*nap == (struct nd_addrlist *) 0)
			return 1;
		(*nap)->n_cnt = 0;
		memset((char *)&sin, '\0', sizeof(sin));
		sin.sin_port = sp->s_port;
		sin.sin_family = AF_INET;

		(*nap)->n_cnt = 1;
		(*nap)->n_addrs = (struct netbuf *) malloc(sizeof(struct netbuf));
		xp = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in));
		if ((xp == (struct sockaddr_in *) 0) || ((*nap)->n_addrs == (struct netbuf *) 0))
			return 1;
	
		*xp = sin;
		nbuf = &(*nap)->n_addrs[0];
	
		nbuf->maxlen = sizeof(struct sockaddr_in);
		nbuf->len    = sizeof(struct sockaddr_in);
		nbuf->buf    = (char *) xp;
		if (debug)
			fprintf(stderr, "add %s\n", inet_ntoa(sin.sin_addr));

		return 0;
	}
#endif

	if (netdir_getbyname(ncp, ndh, nap) == 0)
		return 0;

	return 1;
}

open_transports()
{
	register int		i;
	struct nd_hostserv	ndh;
	struct netconfig *	ncp;
	struct nd_addrlist *	nap;
	struct netbuf *		nbp;
	struct utsname		u;
	struct t_bind *		bp;
	char			msg[BUFSIZ];
	void *			handle;

	(void) uname(&u);
	ndh.h_host = u.nodename;
	ndh.h_serv = "smtp";

	if ((handle = setnetpath()) == NULL) {
		smtplog("ERROR: can't find /etc/netconfig");
		return 1;
	}

	nfd = 0;
	while ((ncp = getnetpath(handle)) != NULL) {
		if (my_netdir_getbyname(ncp, &ndh, &nap))
			continue;

		/* Open Transport endpoint. */
		if ((fds[nfd].fd = t_open(ncp->nc_device, O_RDWR, NULL)) < 0) {
			t_log("could not open transport");
			continue;
		}

		/* Allocate address struct */
		if ((bp = (struct t_bind *) t_alloc(fds[nfd].fd, T_BIND, T_ALL)) == NULL) {
			t_log("could not allocate t_bind");
			continue;
		}

		/* Bind to an address */
		bp->qlen = MAXCONN;
		nbp = nap->n_addrs;
		for (i = 0; i < nap->n_cnt; i++, nbp++) {
			bp->addr = *nbp;
			if (t_bind(fds[nfd].fd, bp, bp) >= 0)
				break;
		}
		if (i >= nap->n_cnt) {
			t_log("could not bind address");
			continue;
		}

		/* Log that the transport is open */
		sprintf(msg, "transport <%s> opened on fd <%d> address <%s>",
			ncp->nc_netid, fds[nfd].fd, taddr2uaddr(ncp, nbp));
		smtplog(msg);

		/* Save the netconfig entry for later use */
		ncf[nfd] = *ncp;
		ncf[nfd].nc_netid     = strsave(ncp->nc_netid);
		ncf[nfd].nc_protofmly = strsave(ncp->nc_protofmly);
		ncf[nfd].nc_proto     = strsave(ncp->nc_proto);
		ncf[nfd].nc_device    = strsave(ncp->nc_device);
		ncf[nfd].nc_lookups   = strsave2(ncp->nc_lookups, ncp->nc_nlookups);

		fds[nfd].events = POLLIN;
		nfd++;
	}
	endnetpath(handle);

	if (nfd == 0) {
		smtplog("could not open any transports");
		return 1;
	}
	return 0;
}

void set_caller(ncp, addr)
struct netconfig *ncp;
struct netbuf *addr;
{
	static char buf[48];

	sprintf(buf, "<%s,%s>", ncp->nc_netid, taddr2uaddr(ncp, addr));
	caller = buf;
}

int accept_call(i)
{
	extern int t_errno;
	int server_fd, fd;
	int j, pid;


	fd = fds[i].fd;

	for (j = 0; j < MAXCONN; j++) {
		if (calls[i][j] == NULL)
			continue;

		if ((server_fd = t_open(ncf[i].nc_device, O_RDWR, NULL)) < 0) {
			t_log("can't t_open transport");
			return -1;
		}

		if (t_bind(server_fd, NULL, NULL) < 0) {
			t_log("can't bind address for server");
			return -1;
		}

		if (t_accept(fd, server_fd, calls[i][j]) < 0) {
			if (t_errno == TLOOK) {
				t_close(server_fd);
				return 0;
			}
			t_log("t_accept failed");
			return -1;
		}

		set_caller(&ncf[i], &(calls[i][j]->addr));
		t_free((char *)calls[i][j], T_CALL);
		calls[i][j] = NULL;

		/* Set up file descriptor for read/write use */
		if (ioctl(server_fd, I_PUSH, "tirdwr") < 0) {
			t_log("push tirdwr failed");
			(void) t_close(server_fd);
			return -1;
		}

		/* Start server conversation */
		load = loadav();
		(void) time(&starttime);
		if (!idled && 
		   ((maxrunning == 0)   || (running < maxrunning)) &&
		   ((loadlim <= 0.001)  || (load <= loadlim))) {
			/* fork a child for this connection */
			running++;
			showstatus("accepted");
			if ((pid = fork()) < 0) {
				logit(LOG_CRIT, "can't fork!!", "");
				running--;
			} else if (pid == 0) {
				(void) signal(SIGCLD,  (void (*)())SIG_DFL);
				(void) signal(SIGUSR1, (void (*)())SIG_DFL);
				(void) signal(SIGUSR2, (void (*)())SIG_DFL);
				(void) signal(SIGHUP,  (void (*)())SIG_DFL);
				doit(server_fd, 1);  /* listen to SMTP dialogue */
				/* NOTREACHED */
				exit(0);
			}
		} else {
			logit(LOG_NOTICE, "Connection refused: ", "");
			showstatus("refused");
		}
		(void) t_close(server_fd);
	}
}

void handle_event(i)
register int i;
{
	register int j, fd, tlook;

	fd = fds[i].fd;
	tlook = t_look(fd);
	if (tlook == T_LISTEN) {
		for (j = 0; j < MAXCONN; j++)
			if (calls[i][j] == NULL)
				break;
		if (j >= MAXCONN) {
			smtplog("can't handle any more calls");
			return;
		}
		if ((calls[i][j] = (struct t_call *) t_alloc(fd, T_CALL, T_ALL)) == NULL) {
			t_log("can't alloc t_call");
			return;
		}
		if (t_listen(fd, calls[i][j]) < 0) {
			t_log("t_listen failed");
			return;
		}
	} else if (tlook == T_DISCONNECT) {
		register struct t_discon *dcp;

		dcp = (struct t_discon *) t_alloc(fd, T_DIS, T_ALL);
		if (t_rcvdis(fd, dcp) < 0) {
			t_log("t_rcvdis failed");
			return;
		}
		for (j = 0; j < nfd; j++) {
			if (dcp->sequence == calls[i][j]->sequence) {
				t_free((char *)calls[i][j], T_CALL);
				calls[i][j] = NULL;
			}
		}
		t_free((char *)dcp, T_DIS);
	} else {
		t_log("t_look failed");
	}
}

/*
 * process - handle incoming connections over all transports for
 *	which the "smtp" service is defined.
 */
void process()
{
	register int i;
	register struct passwd *pw = NULL;
	register struct group *gr = NULL;
#ifdef SVR3
	struct passwd *getpwnam();
	struct group *getgrnam();
#endif
	char msg[BUFSIZ/2];

	/* force creation of queuedir; make sure it has the right permissions */
	if (chdir(SMTPQROOT) < 0) {
		(void) mkdir(SMTPQROOT, 0775);
		(void) chdir(SMTPQROOT);
	}
#ifdef SVR4_1
	if (((pw = getpwnam("smtp")) != NULL) && ((gr = getgrnam("mail")) != NULL)) {
#else
	if (((pw = getpwnam("uucp")) != NULL) && ((gr = getgrnam("mail")) != NULL)) {
#endif
		(void) chmod(".", 0775);
		if (chown(".", pw->pw_uid, gr->gr_gid) == -1)
			(void) posix_chown(".");
	}

	if (fork())			/* run in the background */
		exit(0);
	closeallfiles(0);
	(void) setpgrp();		/* leave current process group */
	(void) open("/dev/null", 0);	/* reopen them on harmless streams */
	(void) dup2(0, 1);
	(void) dup2(0, 2);

	initla();
	load = loadav();
	showstatus("startup");

	if (open_transports())
		exit(0);

	(void) signal(SIGCLD,  (void (*)())reapchild);	/* gross hack! */
	(void) signal(SIGUSR1, (void (*)())decrlimit);
	(void) signal(SIGUSR2, (void (*)())incrlimit);
	(void) signal(SIGHUP,  (void (*)())idlesmtp);


	for (;;) {
		int rc = poll(fds, nfd, -1);

		if ((rc < 0) && (errno == EINTR))
			continue;	/* signals are to be expected */
		if (rc < 0) {
			sprintf(msg, "poll failed, errno = %d", errno);
			smtplog(msg);
			exit(1);
		}

		for (i = 0; i < nfd; i++) {
			switch (fds[i].revents) {
			case POLLIN:
				handle_event(i);
				accept_call(i);
				/* fall thru */
			case 0:
				break;
			default:
				sprintf(msg, "unexpected event %d on fd %d", fds[i].revents, fds[i].fd);
				smtplog(msg);
				exit(1);
			}
		}
	}
	/*NOTREACHED*/
}

#else

#ifdef INETD

/*
 * process - process input file
 */
void process()
{
	extern int getpeername();
	struct sockaddr_in remote;
	struct servent *sp;
	int remotesize = sizeof remote;
	char msg[128];

	sp = getservbyname(SERVNAME, "tcp");
	if (sp == 0) {
		logit(LOG_CRIT, "tcp/%s: unknown service\n", SERVNAME);
		exit(1);
	}
	sin.sin_port = sp->s_port;

	/* Set up start time and IP address of remote */
	(void) time(&starttime);
	if (getpeername(0, &remote, &remotesize) == 0)
		caller = inet_ntoa(remote.sin_addr);
	else
		caller = "<unknown>";
	sprintf(msg, "%s: accepted", caller);
	smtplog(msg);

	/* connection on fd 0 from inetd */
	doit(0, 1);
	/* NOTREACHED */
	exit(0);
}

#else

/*
 * process - process input file
 */
void process()
{
#ifdef SOCKET
	int s, pid;
	struct servent *sp;

	sp = getservbyname(SERVNAME, "tcp");
	if (sp == 0) {
		logit(LOG_CRIT, "tcp/%s: unknown service\n", SERVNAME);
		exit(1);
	}
	sin.sin_port = sp->s_port;
#endif		/* SOCKET */

#ifndef DEBUG
	if (fork())			/* run in the background */
		exit(0);
	for (s = 0; s < 10; s++)	/* close most file descriptors */
		(void) close(s);
	(void) setpgrp();		/* leave current process group */
	(void) open("/dev/null", 0);	/* reopen them on harmless streams */
	(void) dup2(0, 1);
	(void) dup2(0, 2);
#endif

	initla();
	load=loadav();
	showstatus("startup");

#ifdef SOCKET
	/* create internet socket s; retry 5 times at 5 s. intervals if no luck */
	while ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		static int nlog = 0;

		if (nlog++ <= 5)
			logit(LOG_CRIT, "socket", "");
		sleep(5);
	}
	/* set socket options, notably keepalive */
	if (debug) {
		int debugval = 1;

		if (setsockopt(s, SOL_SOCKET, SO_DEBUG,
		    (char *)&debugval, sizeof(int)) < 0)
			logit(LOG_CRIT, "setsockopt (SO_DEBUG)", "");
	}
	if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *)0, 0) < 0)
		logit(LOG_CRIT, "setsockopt (SO_KEEPALIVE)", "");
	/* bind socket to SERVNAME (SMTP) port; retry as above on failure */
	while (bind(s, (struct sockaddr *)&sin, sizeof sin) < 0) {
		static int nlog = 0;

		if (nlog++ <= 100)
			logit(LOG_CRIT, "bind", "");
		sleep(5);
	}
	(void) signal(SIGCLD,  (SIG_TYP)reapchild);	/* gross hack! */
	(void) signal(SIGUSR1, (SIG_TYP)decrlimit);
	(void) signal(SIGUSR2, (SIG_TYP)incrlimit);
	(void) signal(SIGHUP,  (SIG_TYP)idlesmtp);
	/* listen with 5 input buffers on socket (?) */
	if (listen(s, 5) == -1)
		logit(LOG_CRIT, "listen", "");
	for (;;) {
		int conn, fromlen = sizeof from;

		/* get a connection on fd conn; stores src host addr in from */
		conn = accept(s, (struct sockaddr_in *)&from, &fromlen);
		if (conn < 0) {
			static int nlog = 0;

			if (errno == EINTR)
				continue;
			if (++nlog <= 5)
				logit(LOG_CRIT, "accept", "");
			sleep(1);
			continue;
		}
		load = loadav();
		accepted=accept_call(caller = inet_ntoa(from.sin_addr));
		switch (accepted) {
			case 0:	showstatus("rejected");
				(void) close(conn);
				sleep(15);
				continue;
			case 1: showstatus("accepted");  break;
			case 2: showstatus("special");   break;
		}
		running++;
		(void) time(&starttime);
		/* fork a child for this connection */
		if ((pid = fork()) < 0) {
			logit(LOG_CRIT, "can't fork!!", "");
			running--;
		} else if (pid == 0) {
			(void) signal(SIGCHLD, (SIG_TYP)SIG_DFL);
			(void) signal(SIGUSR1, (SIG_TYP)SIG_DFL);
			(void) signal(SIGUSR2, (SIG_TYP)SIG_DFL);
			(void) signal(SIGHUP,  (SIG_TYP)SIG_DFL);
			doit(conn, accepted); /* listen to SMTP dialogue */
			/* NOTREACHED */
			exit(0);
		}
		(void) close(conn);
		sleep(12);
	}
	/*NOTREACHED*/
#endif			/* SOCKET */
}

accept_call(caller)
char *caller;
{
#define	NO	0
#define YES	1
	register int i;

	if (idled)
		return NO;
	if ((loadlim > 0.001) && (load > loadlim))
		return NO;
	if ((maxrunning > 0) && (running >= maxrunning))
		return NO;
	return YES;
}

#endif	/* INETD */
#endif	/* SVR4 */

#ifndef INETD
SIGRETURN
reapchild(s)
	int s;
{
	int status;
	/* gross hack! */
	(void) wait(&status);
	running--;
	if(idled && running <= 0) {
		showstatus("exiting");
		exit(0);
	}
	(void) signal(SIGCLD,  (void (*)())reapchild);	/* gross hack! */
}
#endif			/* INETD */

/*
 * handle some input.  never returns.
 */
int doit(f, accepted)
int f, accepted;
{
	FILE *fi, *fo;
	struct passwd *p;
#ifdef SVR3
	struct passwd *getpwnam();
#endif

	/*
	 *  become uucp (smtp in 4.1)
	 */
#ifdef SVR4_1
	p = getpwnam("smtp");
#else
	p = getpwnam("uucp");
#endif
	if (p) {
#ifdef PRIV
		procprivl(SETPRV, SETUID_W, (priv_t)0);
		setuid(p->pw_uid);
		procprivl(CLRPRV, SETUID_W, (priv_t)0);
#else
		setuid(p->pw_uid);
#endif
	}
	umask(002);

	if ((fi = fdopen(f, "r")) == NULL)
		logit(LOG_CRIT, "fdopen of socket for input", "");
	if ((fo = fdopen(f, "w")) == NULL)
		logit(LOG_CRIT, "fdopen of socket for output", "");

	converse(fi, fo, accepted);
	/* NOTREACHED */
	return 0;
}

/*
 * loadav - return the 1 minute load average.
 *	(Found by looking in kernel for avenrun).
 */
double loadav()
{
	long avenrun[3];
#ifdef SVR4_1
	struct mioc_rksym rks;
#endif

	if (kmem == -1)
		return (double) 0;
#ifdef SVR4_1
	rks.mirk_symname = AVENRUN;
	rks.mirk_buf = avenrun;
	rks.mirk_buflen = sizeof(avenrun);
	if(ioctl(kmem,MIOC_READKSYM,&rks) != 0)
		return(0);
#else
	lseek(kmem, (long)nl[X_AVENRUN].n_value, 0);
	read(kmem, avenrun, sizeof(avenrun));
#endif
#ifdef SVR3
	return ((double) avenrun[0]) / 1000.0;	/* one minute average */
#endif
#ifdef SVR4
	return ((double) avenrun[0]) / 256.0;	/* one minute average */
#endif
}


/* Initialize the load-average stuff */
void
initla()
{
#ifdef SVR4_1
#else
#ifdef NLIST_BUG
	/* Get around the nasty bug in nlist that */
	/* causes lots of memory to be swallowed up */
	int pipes[2];

	if (pipe(pipes) < 0) {
		kmem = -1;
		return;
	}
	switch (fork()) {
	case 0:
		/* Child does nlist and writes to pipe */
		nlist(SLASH_UNIX, nl);
		if (nl[0].n_value) {
			(void) write(pipes[1], nl, sizeof nl);
			close(pipes[1]);
		}
		exit(0);
	case -1:
		kmem = -1;
		return;
	default:
		/* Parent reads nlist values from child */
		(void) close(pipes[1]);
		if (read(pipes[0], nl, sizeof nl) != sizeof nl) {
			(void) close(pipes[0]);
			(void) wait((int *)0);
			kmem = -1;
			return;
		}
		(void) close(pipes[0]);
		(void) wait((int *)0);
	}
#else
	nlist(SLASH_UNIX, nl);
	if (nl[0].n_value == 0) {
		kmem = -1;
		return;
	}
#endif /* NLIST_BUG */
#endif /* SVR4_1 */

	if ((kmem = open("/dev/kmem", 0)) < 0) {
		kmem = -1;
		return;
	}
}

SIGRETURN
incrlimit(s)
	int s;
{
	maxrunning++;
	showstatus("incr");
}

SIGRETURN
decrlimit(s)
	int s;
{
	if(maxrunning > 0)
		maxrunning--;
	showstatus("decr");
}

SIGRETURN
idlesmtp(s)
	int s;
{
	idled = !idled;
	if (idled) {
		showstatus("idled");
	} else {
		showstatus("unidled");
	}
	if(idled && running <= 0) {
		showstatus("exiting");
		exit(0);
	}
}

#ifdef INETD
#ifdef SVR4
static char bin_shell[] = "/usr/bin/sh";
#else
static char bin_shell[] = "/bin/sh";
#endif
static char shell[] = "sh";
static char shflg[]= "-c";
static char devnull[] = "/dev/null";

/* Replacement for system() for use in in.smtpd (called from conversed.c) */
/* This is needed because the SYSV shell does not like stdin/out/err */
/* attached to a socket (as it will be in in.smtpd) */
int system(s)
const char *s;
{
        int     status, pid, w;
        void (*istat)(), (*qstat)();

        if((pid = fork()) == 0) {
		close(0);
		close(1);
		close(2);
		open(devnull, O_RDWR);
		open(devnull, O_RDWR);
		open(devnull, O_RDWR);
                (void) execl(bin_shell, shell, shflg, s, (char *)0);
                _exit(127);
        }
        istat = signal(SIGINT, SIG_IGN);
        qstat = signal(SIGQUIT, SIG_IGN);
        while((w = wait(&status)) != pid && w != -1)
                ;
        (void) signal(SIGINT, istat);
        (void) signal(SIGQUIT, qstat);
        return((w == -1)? w: status);
}
#endif
