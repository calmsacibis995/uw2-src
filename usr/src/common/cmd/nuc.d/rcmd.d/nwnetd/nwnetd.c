/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rcmd.cmd:nwnetd/nwnetd.c	1.6"
#ident  "$Header: $"

/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>
#include <signal.h>
#include <netdb.h>
#include <syslog.h>
#include <pwd.h>
#include <stdio.h>
#include <string.h>

/*
 *	Originally TOMANY = 40 and CNT_INTVL = 60  Under stress testing
 *	CNT_INTVL proved to be to large.  
 */
#define	TOOMANY		60		/* don't start more than TOOMANY */
#define	CNT_INTVL	10		/* servers in CNT_INTVL sec. */
/*
 *	Originally RETRYTIME = (60 *10) 10 minutes seems way to large.
 *	3 minutes seems more reasonable
 */
#define	RETRYTIME	(60 * 3)	/* retry after bind or server fail */

#define	SIGBLOCK	(sigmask(SIGCHLD)|sigmask(SIGHUP)|sigmask(SIGALRM))

extern	int errno;


#include <sys/utsname.h>
#include <util.h>
extern int MoreConnections;


#include <unistd.h>			
#include <string.h>			
#include <locale.h>			
extern char *gettxt();

static int	debug = 0;
static int	nsock, maxsock;
static fd_set	allsock;
static int	options;
static int	timingout;
static struct	servent *sp;

struct	servtab {
	char	*se_service;	/* name of service */
	int		se_socktype;	/* type of socket to use */
	char	*se_proto;		/* protocol used */
	struct	netconfig *se_netconfigp;		/* transport configuration  */
	short	se_wait;		/* single threaded server */
	short	se_checked;		/* looked at during merge */
	char	*se_user;		/* user name to run as */
	struct	biltin *se_bi;	/* if built-in, description */
	char	*se_server;		/* server program */
#define	MAXARGV 20
/*--------------------TLI specific elements ----------------------------*/
#define	NETBUF_CNT 3		/* number of netbuf elements */
	char	se_argc;		/* index to last normal argument */
	struct t_call  *se_callptr;	/* accept info here */
	char 	*se_device;		/* transport device name */
/*--------------------end TLI specific elements --------------------------*/
	char	*se_argv[MAXARGV+1+NETBUF_CNT];	/* program arguments */
	int		se_fd;			/* open descriptor */
	struct	sockaddr_in se_ctrladdr;/* bound address */
	int		se_count;		/* number started since se_time */
	struct	timeval se_time;	/* start of se_count */
	struct	servtab *se_next;
}*servtab;

struct biltin {
	char	*bi_service;	/* internally provided service name */
	int		bi_socktype;	/* type of socket supported */
	short	bi_fork;		/* 1 if should fork before call */
	short	bi_wait;		/* 1 if should wait for child */
	int		(*bi_fn)();		/* function which performs it */
};

static struct biltin  biltins[] = {
	0
};
/*-----------------Function Prototypes ----------------------*/
static	void reapchild();
char	*index();
static 	void setup( struct servtab *sep);
static	void config();
static	setconfig();
static	void freeconfig(struct servtab *);
static	void endconfig();
static	void print_service( char *action, struct servtab *sep);
static	struct servtab * enter(struct servtab *cp);
static	char * newstr(char *cp);
static	struct servtab * getconfigent(void);
static	void retry(void);

#define NUMINT	(sizeof(intab) / sizeof(struct inent))
#define	_PATH_INETDCONF "/etc/nwnetd.conf"
static char	*CONFIG = _PATH_INETDCONF;

main(argc, argv, envp)
	int argc;
	char *argv[], *envp[];
{
	extern int optind;
	register struct servtab *sep;
	register struct passwd *pwd;
	register int tmpint;
	int flags;
#ifdef UCB
	struct sigvec sv;
#else
	struct sigaction sv;
#endif
	int ch, pid, dofork;
	char buf[50];
	struct netbuf	netbuf;
	int i;

	setlocale(LC_ALL, "");

	if (envp == 0 || *envp == 0)
		envp = argv;
	while (*envp)
		envp++;

	while ((ch = getopt(argc, argv, "d")) != EOF)
		switch(ch) {
		case 'd':
			debug = 1;
			options |= SO_DEBUG;
			break;
		case '?':
		default:
			fprintf(stderr,gettxt("nwnetdmsgs:1","nwnetd: Usage: nwnetd [-d]\n"));
			exit(1);
		}
	argc -= optind;
	argv += optind;

	if (argc > 0)
		CONFIG = argv[0];
	if (debug == 0)
		daemon(0, 0);
	openlog("nwnetd", LOG_PID | LOG_NOWAIT, LOG_DAEMON);


	bzero((char *)&sv, sizeof(sv));
#ifdef UCB
	sv.sv_mask = SIGBLOCK;
	sv.sv_handler = retry;
	sigset(SIGALRM,retry);        
#else
	sigaddset(&sv.sa_mask,SIGBLOCK);
	sv.sa_handler = retry;
#endif
	sigvec(SIGALRM, &sv, (struct sigvec *)0);

	
	config();
	if ( debug )
		syslog(LOG_DEBUG, "nsock %d",nsock);
#ifdef UCB
	sv.sv_handler = config;
#else
	sv.sa_handler = config;
#endif
	sigvec(SIGHUP, &sv, (struct sigvec *)0);
#ifdef UCB
	sv.sv_handler = reapchild;
#else
	sv.sa_handler = reapchild;
#endif
	sigvec(SIGCHLD, &sv, (struct sigvec *)0);

	{
		/* space for daemons to overwrite environment for ps */
#define	DUMMYSIZE	100
		char dummy[DUMMYSIZE];

		(void)memset(dummy, 'x', sizeof(DUMMYSIZE) - 1);
		dummy[DUMMYSIZE - 1] = '\0';
#if 0
		(void)setenv("nwnetd_dummy", dummy, 1);
#endif
	}
	MoreConnections = 0;
	timingout = 0;
	for (;;) 
	{
	    int n, ctrl;
	    fd_set readable;

#if 0
	    if (nsock == 0) 
		{
			if ( debug )
				syslog(LOG_DEBUG, "nsock = 0");
			(void) sigblock(SIGBLOCK);
			while (nsock == 0)
		    	sigpause(0L);
			(void) sigsetmask(0L);
	    }
#else
			(void) sigsetmask(0L);
			while (nsock == 0)
		    		(void)pause();
#endif
		if (!MoreConnections) {
			readable = allsock;
			if ((n = select(maxsock + 1, &readable, (fd_set *)0,
				(fd_set *)0, (struct timeval *)0)) <= 0) {
				if (n < 0 && errno != EINTR)
					syslog(LOG_WARNING, "select: %m");
				sleep(1);
				continue;
			}
		}
		else
		{
			n = 1;
		}
	    for (sep = servtab; n && sep; sep = sep->se_next)
		{
	        if (sep->se_fd != -1 && FD_ISSET(sep->se_fd, &readable)) 
			{
		    	n--;
		    	if (debug)
			    	syslog(LOG_DEBUG, "service_name %s", sep->se_service);
		    	if (sep->se_socktype == SOCK_STREAM) 
				{
					/*
	 				*-- Need to accept the connection.
					*/
					sigblock(SIGBLOCK);
					ctrl = BindAndAccept(sep->se_fd ,&(sep->se_callptr));
					sigsetmask(0);
			    	if (debug)
				    	syslog(LOG_DEBUG, "BindAndAccept: ctrl %d", ctrl);
			    	if (ctrl <= 0) 
					{
				    	if (errno == EINTR)
				    		continue;
					if ( debug )
				    	syslog(LOG_WARNING, gettxt("nwnetdmsgs:2", "t_accept for (%s) failed: %m"), sep->se_service);
				    	continue;
			    	}
		    	}
				else
			    	ctrl = sep->se_fd;
		    	(void) sigblock(SIGBLOCK);
		    	pid = 0;
		    	dofork = (sep->se_bi == 0 || sep->se_bi->bi_fork);
		    	if (dofork) 
				{
			    	if (sep->se_count++ == 0)
						(void)gettimeofday(&sep->se_time, NULL);
					else if (sep->se_count >= TOOMANY) 
					{
						struct timeval now;

						(void)gettimeofday(&now, NULL);
						if (now.tv_sec - sep->se_time.tv_sec > CNT_INTVL) 
						{
							sep->se_time = now;
							sep->se_count = 1;
						}
						else 
						{
							sep->se_time = now;
							sep->se_count = 1;
							syslog(LOG_ERR,gettxt("nwnetdmsgs:3", "%s/%s server failing (looping), service terminated"), sep->se_service, sep->se_proto);
							FD_CLR(sep->se_fd, &allsock);
					
							ReleaseTliCallBuf(sep->se_fd);
							t_free((char *)sep->se_callptr,T_CALL);
							(void) close(sep->se_fd);
							sep->se_fd = -1;
							sep->se_count = 0;
							nsock--;
							if ( debug )
								syslog(LOG_DEBUG,"now.tv_sec(%d) - sep->se_time.tv_sec(%d) > CNT_INTVL(%d)", now.tv_sec,sep->se_time.tv_sec,CNT_INTVL);
sigsetmask(0L);
							if (!timingout) 
							{
								timingout = 1;
								alarm(RETRYTIME);
							}
							continue;
						}
					}
					pid = fork();
				}
				if (pid < 0) 
				{
					syslog(LOG_ERR, gettxt("nwnetdmsgs:4", "fork failed: %m"));
					if (sep->se_socktype == SOCK_STREAM)
						t_close(ctrl);
					sigsetmask(0L);
					sleep(1);
					continue;
				}
				if (pid && sep->se_wait) 
				{
					sep->se_wait = pid;
					if (sep->se_fd >= 0) 
					{
						FD_CLR(sep->se_fd, &allsock);
						nsock--;
					}
				}
				sigsetmask(0L);
				if (pid == 0) 
				{
					if (debug && dofork)
					setsid();
					if (dofork)
					for (tmpint = maxsock; --tmpint > 2; )
					{
						if (tmpint != ctrl)
						{
							t_close(tmpint);
						}
					}
					if (sep->se_bi)
						(*sep->se_bi->bi_fn)(ctrl, sep);
					else 
					{
						if (debug)
							syslog(LOG_DEBUG, "%d execl %s\n", getpid(), sep->se_server);
						netbuf.buf = sep->se_callptr->addr.buf;
						netbuf.len = sep->se_callptr->addr.len;
						netbuf.maxlen = sep->se_callptr->addr.maxlen;
						sep->se_argv[sep->se_argc++] = sep->se_proto;
						if ( create_netbuf_arg(sep->se_argv,sep->se_argc,
											&netbuf,sep->se_netconfigp) != 0 )
						{
							t_close(ctrl);
							_exit(1);
						}
						dup2(ctrl, 0);

						t_close(ctrl);
						dup2(0, 1);
						dup2(0, 2);
						if ((pwd = getpwnam(sep->se_user)) == NULL) 
						{
							syslog(LOG_ERR, gettxt("nwnetdmsgs:5", "getpwnam: %s: No such user"), sep->se_user);
							if (sep->se_socktype != SOCK_STREAM)
								recv(0, buf, sizeof (buf), 0);
							_exit(1);
						}
						if (pwd->pw_uid) 
						{
							(void) setgid((gid_t)pwd->pw_gid);
							initgroups(pwd->pw_name, pwd->pw_gid);
							(void) setuid((uid_t)pwd->pw_uid);
						}
						execv(sep->se_server, sep->se_argv);
						if (sep->se_socktype != SOCK_STREAM)
							recv(0, buf, sizeof (buf), 0);
						syslog(LOG_ERR, gettxt("nwnetdmsgs:6", "execv of %s failed: %m"), sep->se_server);
						_exit(1);
					}
		    	}
		    	if (sep->se_socktype == SOCK_STREAM)
            	{
			    	t_close(ctrl);
				}
			}
		}
	}
}

static void
reapchild()
{
	int status;
	int pid;
	register struct servtab *sep;
	siginfo_t info;

	for (;;) 
	{
#ifdef UCB
		pid = wait3(&status, WNOHANG, (struct rusage *)0);
#else
		pid = waitid(P_ALL,0,&info, WNOHANG|WEXITED);
		if ((pid == -1 ) || (info.si_pid == 0 ))
			break;
		pid = info.si_pid;
		status = info.si_status;
#endif
		if (debug)
			syslog(LOG_DEBUG, "reapchild() pid = %d:",pid);
		if (pid <= 0)
			break;
		for (sep = servtab; sep; sep = sep->se_next)
		{
			if (sep->se_wait == pid) {
				if (debug)
			    	syslog(LOG_DEBUG, "sep->se_wait == pid:service_name %s\
									  fd %d\n", sep->se_service, sep->se_fd);
				if (status)
					syslog(LOG_WARNING, gettxt("nwnetdmsgs:7", "%s: exit status 0x%x"), sep->se_server, status);
				FD_SET(sep->se_fd, &allsock);
				if (debug)
					syslog(LOG_DEBUG, "nsock++ :");
				nsock++;
				sep->se_wait = 1;
			}
		}
	}
}

void
config()
{
	register struct servtab *sep, *cp, **sepp;
	long omask;

	if ( debug )
		syslog(LOG_DEBUG, "config()");
	if (!setconfig()) {
		syslog(LOG_ERR, "%s: %m", CONFIG);
		exit(1);
	}
	for (sep = servtab; sep; sep = sep->se_next)
		sep->se_checked = 0;
	while (cp = getconfigent()) {
		for (sep = servtab; sep; sep = sep->se_next)
		{
			if (strcmp(sep->se_service, cp->se_service) == 0 &&
			    strcmp(sep->se_proto, cp->se_proto) == 0)
				break;
		}
		if (sep != 0) {
			int i;

			omask = sigblock(SIGBLOCK);
			/*
			 * sep->se_wait may be holding the pid of a daemon
			 * that we're waiting for.  If so, don't overwrite
			 * it unless the config file explicitly says don't 
			 * wait.
			 */
			if (cp->se_bi == 0 && 
			    (sep->se_wait == 1 || cp->se_wait == 0))
				sep->se_wait = cp->se_wait;
#define SWAP(a, b) { char *c = a; a = b; b = c; }
			if (cp->se_user)
				SWAP(sep->se_user, cp->se_user);
			if (cp->se_server)
				SWAP(sep->se_server, cp->se_server);
			for (i = 0; i < MAXARGV; i++)
				SWAP(sep->se_argv[i], cp->se_argv[i]);
			sep->se_argc = cp->se_argc;
			sigsetmask(omask);
			freeconfig(cp);
			if (debug)
				print_service(gettxt("nwnetdmsgs:8", "REDO"), sep);
		} else {
			sep = enter(cp);
			if (debug)
				print_service(gettxt("nwnetdmsgs:9", "ADD "), sep);
		}
		sep->se_checked = 1;
		sp = getservbyname(sep->se_service, sep->se_proto);
		if (sp == 0) {
			syslog(LOG_ERR, gettxt("nwnetdmsgs:10", "%s/%s: unknown service"),
			    sep->se_service, sep->se_proto);
			if (sep->se_fd != -1)
			{
				ReleaseTliCallBuf(sep->se_fd);
				t_free((char *)sep->se_callptr,T_CALL);
				(void) close(sep->se_fd);
			}
			sep->se_fd = -1;
			continue;
		}
		if (sp->s_port != sep->se_ctrladdr.sin_port) {
			sep->se_ctrladdr.sin_port = sp->s_port;
			if (sep->se_fd != -1)
			{
				ReleaseTliCallBuf(sep->se_fd);
				t_free((char *)sep->se_callptr,T_CALL);
				(void) close(sep->se_fd);
			}
			sep->se_fd = -1;
		}
		if (sep->se_fd == -1)
			setup(sep);
	}
	endconfig();
	/*
	 * Purge anything not looked at above.
	 */
	omask = sigblock(SIGBLOCK);
	sepp = &servtab;
	while (sep = *sepp) {
		if (sep->se_checked) {
			sepp = &sep->se_next;
			continue;
		}
		*sepp = sep->se_next;
		if (sep->se_fd != -1) {
			FD_CLR(sep->se_fd, &allsock);
			nsock--;
			ReleaseTliCallBuf(sep->se_fd);
			t_free((char *)sep->se_callptr,T_CALL);
			(void) close(sep->se_fd);
		}
		if (debug)
			print_service(gettxt("nwnetdmsgs:11", "FREE"), sep);
		freeconfig(sep);
		free((char *)sep);
	}
	(void) sigsetmask(omask);
	return;
}

static void
retry()
{
	register struct servtab *sep;

	timingout = 0;
	if ( debug )
		syslog(LOG_DEBUG, "retry()");
	for (sep = servtab; sep; sep = sep->se_next)
		if (sep->se_fd == -1)
			setup(sep);
}

static void 
setup(sep)
	register struct servtab *sep;
{

	int		fd;
	struct	nd_hostserv nd_hostserv;     
	struct	netconfig *netconfigp = NULL;
	struct	nd_addrlist *nd_addrlistp = NULL;
	struct	netbuf  *netbufp = NULL;


	/*
	 *	Use proto string as the network identifier
	 */
	nd_hostserv.h_host = HOST_SELF;			/* my machines name */
	nd_hostserv.h_serv = sep->se_service;		/* name of service */
	
	if ((netconfigp = getnetconfigent(sep->se_proto)) != NULL )
	{
		if (netdir_getbyname(netconfigp, &nd_hostserv, 
				&nd_addrlistp) == 0) {
			netbufp = nd_addrlistp->n_addrs;
			if ((fd = BindAndListenOnWellKnown(netconfigp, netbufp,
					&(sep->se_callptr))) < 0) {
				if ( debug )
					syslog(LOG_DEBUG, 
					"BindAndListenOnWellKnown  %s/%s: ",
					sep->se_service,sep->se_device);
				/*
				 * Someone else has the port or it is in a bad 
				 * state. Will retry the service again later. 
				 * Hopefully it will be released or timeout 
				 * by the retry time.
				 */
				if (!timingout) 
				{
					timingout = 1;
					alarm(RETRYTIME);
				}
				(void) netdir_free((char *)nd_addrlistp, ND_ADDRLIST);
				return;
			}
			sep->se_fd = fd;
			sep->se_netconfigp = netconfigp;
			/*
			 *	Save device name so that we know who to open
			 *	when it comes time to accept on this transport 
			 *	endpoint.
			 */
			sep->se_device = (char *)newstr(netconfigp->nc_device);
			(void) netdir_free((char *)nd_addrlistp, ND_ADDRLIST);
			SetupTliCallBuf(sep->se_fd,sep->se_device);
		}
	}
	if ( sep->se_fd > 0 )
	{
		FD_SET(sep->se_fd, &allsock);
		nsock++;
		if (sep->se_fd > maxsock)
			maxsock = sep->se_fd;
	}
}

static struct servtab *
enter(cp)
	struct servtab *cp;
{
	register struct servtab *sep;
	long omask;

	sep = (struct servtab *)malloc(sizeof (*sep));
	if (sep == (struct servtab *)0) {
		syslog(LOG_ERR, gettxt("nwnetdmsgs:12", "Out of memory."));
		exit(-1);
	}
	*sep = *cp;
	sep->se_fd = -1;
	omask = sigblock(SIGBLOCK);
	sep->se_next = servtab;
	servtab = sep;
	sigsetmask(omask);
	return (sep);
}

static FILE	*fconfig = NULL;
static struct	servtab serv;
static char	line[256];
static char	*skip(), *nextline();

static
setconfig()
{

	if (fconfig != NULL) {
		fseek(fconfig, 0L, L_SET);
		return (1);
	}
	fconfig = fopen(CONFIG, "r");
	return (fconfig != NULL);
}

static void
endconfig()
{
	if (fconfig) {
		(void) fclose(fconfig);
		fconfig = NULL;
	}
}

static struct servtab *
getconfigent()
{
	register struct servtab *sep = &serv;
	int argc;
	char *cp, *arg;

more:
	while ((cp = nextline(fconfig)) && *cp == '#')
		;
	if (cp == NULL)
		return ((struct servtab *)0);
	sep->se_service = newstr(skip(&cp));
	arg = skip(&cp);
	if (strcmp(arg, "stream") == 0)
		sep->se_socktype = SOCK_STREAM;
	else if (strcmp(arg, "dgram") == 0)
		sep->se_socktype = SOCK_DGRAM;
	else if (strcmp(arg, "rdm") == 0)
		sep->se_socktype = SOCK_RDM;
	else if (strcmp(arg, "seqpacket") == 0)
		sep->se_socktype = SOCK_SEQPACKET;
	else if (strcmp(arg, "raw") == 0)
		sep->se_socktype = SOCK_RAW;
	else
		sep->se_socktype = -1;
	sep->se_proto = newstr(skip(&cp));
	arg = skip(&cp);
	sep->se_wait = strcmp(arg, "wait") == 0;
	sep->se_user = newstr(skip(&cp));
	sep->se_server = newstr(skip(&cp));
	if (strcmp(sep->se_server, "internal") == 0) {
		register struct biltin *bi;

		for (bi = biltins; bi->bi_service; bi++)
			if (bi->bi_socktype == sep->se_socktype &&
			    strcmp(bi->bi_service, sep->se_service) == 0)
				break;
		if (bi->bi_service == 0) {
			syslog(LOG_ERR, gettxt("nwnetdmsgs:13", "internal service %s unknown"), sep->se_service);
			goto more;
		}
		sep->se_bi = bi;
		sep->se_wait = bi->bi_wait;
	} else
		sep->se_bi = NULL;
	argc = 0;
	for (arg = skip(&cp); cp; arg = skip(&cp))
		if (argc < MAXARGV)
			sep->se_argv[argc++] = newstr(arg);
	sep->se_argc = argc;
	while (argc <= MAXARGV)
		sep->se_argv[argc++] = NULL;
	return (sep);
}

static void
freeconfig(cp)
	register struct servtab *cp;
{
	int i;

	if (cp->se_service)
		free(cp->se_service);
	if (cp->se_proto)
		free(cp->se_proto);
	if (cp->se_netconfigp)
		freenetconfigent(cp->se_netconfigp);
	if (cp->se_user)
		free(cp->se_user);
	if (cp->se_server)
		free(cp->se_server);
	if (cp->se_device)
		free(cp->se_device);
	if (cp->se_callptr && (cp->se_fd > 0))
		t_free((char *)cp->se_callptr, T_CALL);
	for (i = 0; i < MAXARGV; i++)
		if (cp->se_argv[i])
			free(cp->se_argv[i]);
}

char *
skip(cpp)
	char **cpp;
{
	register char *cp = *cpp;
	char *start;

again:
	while (*cp == ' ' || *cp == '\t')
		cp++;
	if (*cp == '\0') {
		int c;

		c = getc(fconfig);
		(void) ungetc(c, fconfig);
		if (c == ' ' || c == '\t')
			if (cp = nextline(fconfig))
				goto again;
		*cpp = (char *)0;
		return ((char *)0);
	}
	start = cp;
	while (*cp && *cp != ' ' && *cp != '\t')
		cp++;
	if (*cp != '\0')
		*cp++ = '\0';
	*cpp = cp;
	return (start);
}

char *
nextline(fd)
	FILE *fd;
{
	char *cp;

	if (fgets(line, sizeof (line), fd) == NULL)
		return ((char *)0);
	cp = index(line, '\n');
	if (cp)
		*cp = '\0';
	return (line);
}

static char *
newstr(cp)
	char *cp;
{
	if (cp = strdup(cp ? cp : ""))
		return(cp);
	syslog(LOG_ERR, "strdup: %m");
	exit(-1);
}


/*
 * print_service:
 *	Dump relevant information to stderr
 */
static void
print_service(action, sep)
	char *action;
	struct servtab *sep;
{
	syslog(LOG_DEBUG,"%s: %s proto=%s, wait=%d, user=%s builtin=%x server=%s\n",
	    action, sep->se_service, sep->se_proto,
	    sep->se_wait, sep->se_user, (int)sep->se_bi, sep->se_server);
}
