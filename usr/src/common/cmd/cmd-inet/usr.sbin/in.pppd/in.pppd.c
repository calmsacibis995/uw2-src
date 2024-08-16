/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static char TCPID[] = "@(#)in.pppd.c	1.2 STREAMWare for Unixware 2.0 source";
#endif /* lint */
/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright 1987-1994 Lachman Technology, Inc.
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */
/*      SCCS IDENTIFICATION        */
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.pppd/in.pppd.c	1.8"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */


#include <dial.h>
#undef DIAL
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <errno.h>
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#include <signal.h>
#include <sys/types.h>
#include <varargs.h>
/* STREAMS includes */
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <poll.h>
#include <string.h>
/* inet includes */
#include <netdb.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ppp.h>
#include <sys/time.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "./pppd.h"
#include "../../../bnu/uucp.h"

char s[256];

char *program_name;
struct ppp_ppc_inf_ctl_s ctlbuf;
struct ppp_ppc_inf_dt_s databuf;

struct strbuf ctlsb = {
	sizeof(ctlbuf),0,(char*) &ctlbuf,
};

struct strbuf datasb = {
	sizeof(databuf),0,(char*) &databuf,
};

#if defined(N_CONN)
#undef N_CONN
#endif
#define N_CONN 60

struct conn_made_s {
	int fd;
	int cfd;		/* the actual tty file descriptor */ 
	int muxid;
	short pgrp;
} conn_made[N_CONN];
extern int errno;

int Debug;

int ppppid;
int pppfd;
int ppcidfd;
int msg_sock;

#define USAGE() fprintf(stderr, "usage: %s [-d level] [-w max_wait]\n", program_name)

void myundial();

/* signal processor for parent when it is killed */
void
remove_child(sig)
int sig;
{
	kill(ppppid, SIGHUP);
	exit(0);
}

/* signal processor for child when it is killed */
void
sig_rm_conn(sig)
int sig;
{
	struct conn_made_s *cm;

	for (cm = conn_made; cm < &conn_made[N_CONN]; cm++) {
		if (cm->muxid) {
			ppp_rm_conn(ppcidfd,cm);
		}
	}
	exit(0);
}

main (argc,argv)
int argc;
char **argv;
{
	int i;
	struct strioctl iocb;
	struct ifreq	ifr;
	int confd;
	struct conn_made_s *cm;
	short w;
	extern char	*optarg;
	int	maxwait = 5;	/* default number of backlogged conn's	*/
	int	s;		/* socket for communicating with	*/
				/* ppp shell				*/
	int	n;		/* hold return value from select */
	fd_set	fds_set, fds_read, infds, outfds, exfds;  /* for select */
	int	max_fd;
	struct	sockaddr_in sin;
	int	sinlen, r;

	program_name = strrchr(argv[0],'/');
	program_name = program_name ? program_name + 1 : argv[0];

	Debug = 0;
	while ((i = getopt(argc, argv, "d:w:")) != EOF) {
		switch (i) {
		case 'd':
			Debug = atoi(optarg);
			break;
		case 'w':
			maxwait = atoi(optarg);
			break;
		default:
			USAGE();
			exit(1);
		}
	}

#ifdef LOG_DAEMON
	openlog(program_name, LOG_PID|LOG_NDELAY, LOG_DAEMON);
#else
	openlog(program_name, LOG_PID);
#endif
	/* fork a child so the parent can exit creating a daemon */
	ppppid = fork();
	if (ppppid < 0) {
		ppp_syslog(LOG_ERR,"fork failed: %m");
                exit(1);
        }
	if (ppppid) exit(0);

	close(0);
	confd = open("/dev/console",O_WRONLY);
	if (confd < 0) {
		ppp_syslog(LOG_ERR,"cannot open /dev/console: %m");
                exit(1);
        }
	close(1);
	close(2);
	dup(confd);
	dup(confd);

	setpgrp();
	sigset(SIGCLD, SIG_IGN);
	sigset(SIGHUP, remove_child);
	sigset(SIGINT, remove_child);
	sigset(SIGQUIT, remove_child);
	sigset(SIGTERM, remove_child);

	/* fork a child to handle the work. A child  must be forked so that the
	 * tty's which the daemon is handling do not get attached as controlling
	 * terminals
	 * DISCLAIMER: letting a parent process hang around with nothing to do
	 * seems ugly but can't figure out a better way to prevent the
	 * controlling terminal crap
	 */
	ppppid = fork();
	if (ppppid < 0 ) {
		ppp_syslog(LOG_ERR, "2nd fork failed: %m");
                exit(1);
        }
	if (ppppid) {
		/* parent daemon logs all debugging messages */
                /* open the ppp log driver to get logging messages */
                if ((pppfd = open(PPLOGDEV, O_RDWR|O_NDELAY)) < 0) {
                        ppp_syslog(LOG_ALERT, "open %s fail: %m", PPLOGDEV);
                        kill((pid_t)ppppid, SIGINT);
                        exit(-1);
                }
                FD_ZERO(&outfds);
                FD_ZERO(&infds);
                FD_ZERO(&exfds);
                while (1) {
                        FD_SET(pppfd, &infds);
                        FD_SET(pppfd, &exfds);

                        r = select(20, &infds, &outfds, &exfds, (struct timeval *)0);
                        if (r < 0) {
                                if (errno == EINTR) {
                                        continue;
                                }
                                ppp_syslog(LOG_ERR,"select: %m");
                                continue;
                        }
                        if (FD_ISSET(pppfd, &infds) || FD_ISSET(pppfd, &exfds))
                                 ppplog(pppfd);
                }
	}

	ppp_syslog(LOG_NOTICE, "restarted");
	sigset(SIGHUP,sig_rm_conn);
	sigset(SIGINT,sig_rm_conn);
	sigset(SIGQUIT,sig_rm_conn);
	sigset(SIGTERM,sig_rm_conn);
	sigset(SIGCLD,SIG_IGN);

	for (cm = conn_made; cm < &conn_made[N_CONN]; cm++) {
		cm->fd=0;
		cm->cfd=0;
	}

	ppcidfd = open(PPCIDDEV,O_RDWR|O_NDELAY);
	if (ppcidfd < 0) {
		ppp_syslog(LOG_ERR,"cannot open /dev/ppcid: %m");
                exit(1);
        }

	s = pppd_sockinit(maxwait);

	FD_ZERO(&fds_set);
	max_fd = -1;

	FD_SET(ppcidfd, &fds_set);
	if(ppcidfd > max_fd)
		max_fd = ppcidfd;
	
	FD_SET(s, &fds_set);
	if(s > max_fd)
		max_fd = s;

	while (1) {
		if (Debug)
			ppp_syslog(LOG_INFO, "select");
		fds_read = fds_set;
		if((n=select(max_fd+1, &fds_read, (fd_set *)0, (fd_set *)0,
				(struct timeval *)0)) <= 0){
			if(n==0 || errno == EINTR) {
			sleep(1);
			continue;
			}
			ppp_syslog(LOG_WARNING, "select: %m");
			exit(1);
		}

		
		if (Debug)
			ppp_syslog(LOG_INFO, "select returned");
		
		if(FD_ISSET(ppcidfd, &fds_read)) {
			if(Debug)
				ppp_syslog(LOG_INFO,"select ppcidfd: %d", ppcidfd);
			ppcid_msg(ppcidfd);
		}
		
		sinlen = sizeof(sin);
		if(FD_ISSET(s, &fds_read)) {
			if (Debug)
				ppp_syslog(LOG_INFO, "accept on socket %d",s);
			if ((msg_sock = accept(s, (caddr_t)&sin, &sinlen)) == -1) {
				if (errno == EINTR || errno == EWOULDBLOCK)
					continue;
				ppp_syslog(LOG_ERR,"accept: %m");
				exit(2);
			}

			sighold(SIGPOLL); 
			/*
		 	* validate caller
		 	*/
#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK 0x7f000001
#endif
			if (sin.sin_addr.s_addr != htonl(INADDR_LOOPBACK) ||
		    		ntohs(sin.sin_port) >= IPPORT_RESERVED) {
					ppp_syslog(LOG_WARNING,
					   "Connection refused from: %s:%d",
					   inet_ntoa(sin.sin_addr.s_addr), 
					   ntohs(sin.sin_port));
				close(msg_sock);
				sigrelse(SIGPOLL); 
				continue;
			}
		/* We close socket if the connection is from ppp shell
		 * if it is from pppstat, we will keep the socket
                 * until we write back the ppp status info.
		 */
			if(!pppd_sockread(msg_sock, ppcidfd)){
				close(msg_sock);
			}
			sigrelse(SIGPOLL); 
		}
	}
}


/*
 * process ppp debugging messages from /dev/ppp
 */
ppplog(pppfd)
        int pppfd;
{
        struct ppp_log_ctl_s ctlbuf;
        struct ppp_log_dt_s databuf;
        struct strbuf ctlsb, datasb;
        char    fmt[LOGSIZE + 20];
        int     flgs = 0;

do_over:
        ctlsb.maxlen = sizeof(ctlbuf);
        ctlsb.len = 0;
        ctlsb.buf = (char *) &ctlbuf;
        datasb.maxlen = sizeof(databuf);
        datasb.len = 0;
        datasb.buf = (char *) &databuf;

        if (getmsg(pppfd, &ctlsb, &datasb, &flgs) < 0) {
                if (errno = EAGAIN)
                        return;
                ppp_syslog(LOG_WARNING, "ppplog() getmsg failed: %m");
                return;
        }

	if (ctlsb.len < sizeof(int)) {
                goto do_over;
        }

        switch (ctlbuf.function) {

        case PPP_LOG:
                if (datasb.len < sizeof(databuf)) {
                        goto do_over;
                }

                sprintf(fmt, "Link id(%d):", ctlbuf.l_index);

                strcat(fmt, databuf.fmt);

                ppp_syslog(LOG_WARNING, fmt, databuf.arg1, databuf.arg2, databuf.arg3, databuf.arg4);
                break;
        default:
                ppp_syslog(LOG_WARNING, "Get a unknow log message");
                break;
        }
}


struct conn_made_s *cm;
struct conn_made_s *
ppp_add_conn(ppcidfd,fd)
int ppcidfd,fd;
{
	struct termio trm;
	struct strioctl iocb;
	int	unit, i;
	int	speed;
	int	baudrate;
	char    mod_name[80];

	/* ditch any modules that were auto pushed onto the tty */
        while (ioctl(fd, I_LOOK, mod_name) >= 0) {
                ppp_syslog(LOG_INFO,"Popping Module %s\n", mod_name);
                if (ioctl(fd, I_POP, mod_name) < 0) {
                        perror("ioctl I_POP failed");
                        return(NULL);
                }
        }

	i = ioctl(fd,TCGETA,&trm);
	if (i < 0) {
	        ppp_syslog(LOG_ERR,"TCGETA failed: %m");
                return(NULL);
        }

	trm.c_iflag=IGNBRK;
	trm.c_cflag = (baudrate = trm.c_cflag & CBAUD)|CS8|HUPCL|CREAD;

	trm.c_oflag=trm.c_lflag=0;

	trm.c_cc[VINTR]=trm.c_cc[VQUIT]=trm.c_cc[VERASE]=trm.c_cc[VKILL]=trm.c_cc[VEOF]=trm.c_cc[VEOL]=trm.c_cc[VEOL2]=trm.c_cc[VSWTCH]=trm.c_cc[VSTART]=trm.c_cc[VSTOP]=trm.c_cc[VSUSP]=trm.c_cc[VDSUSP]=trm.c_cc[VREPRINT]=trm.c_cc[VDISCARD]=trm.c_cc[VWERASE]=trm.c_cc[VLNEXT]=_POSIX_VDISABLE;
	trm.c_cc[VTIME]=0;
        trm.c_cc[VMIN]=1;

	i=ioctl(fd,TCSETA,&trm);
	if (i < 0) {
                ppp_syslog(LOG_ERR,"TCGETA failed: %m");
                return(NULL);
        }

	speed = getbaudrate(baudrate);
	iocb.ic_cmd = P_SETSPEED;
	iocb.ic_timout = 15;
	iocb.ic_len = sizeof(speed);
	iocb.ic_dp = (caddr_t)&speed;
	i=ioctl(ppcidfd,I_STR,&iocb);
	if (i < 0) {
                ppp_syslog(LOG_ERR,"P_SETSPEED failed: %m");
                return(NULL);
        }

	ppp_syslog(LOG_INFO,"Connecting '%s' under PPP", ttyname(fd));
	i=ioctl(fd, I_PUSH, "asyh");
	if (i < 0) {
                ppp_syslog(LOG_ERR,"I_PUSH failed: %m");
                return(NULL);
        }
 
	i=ioctl(ppcidfd,I_LINK,fd);
	if (i < 0) {
                ppp_syslog(LOG_ERR,"I_LINK failed: %m");
                return(NULL);
        }

	for(cm = conn_made; cm < &conn_made[N_CONN]; cm++) {
		if (!cm->fd){
			cm->fd=fd;
			cm->muxid=i;
			cm->pgrp=0;
			return(cm);
		}
	}
	return(NULL);
}

/*
 * unlink the tty from under the ppp stack.
 */
ppp_rm_conn(ppcidfd,cm)
int ppcidfd;
struct conn_made_s *cm;
{
	int i;
	char *calltype = "Outgoing";

	if (Debug)
		ppp_syslog(LOG_INFO, "ppp_rm_conn: I_UNLINK muxid=%d", cm->muxid);
	i=ioctl(ppcidfd,I_UNLINK,cm->muxid);
	if (i < 0) {
                ppp_syslog(LOG_ERR, "tty unlink failed: %m");
        }
 
	i=ioctl(cm->fd,I_POP,0);
	if (i < 0) {
                ppp_syslog(LOG_ERR, "I_POP failed: %m");
        }

	if (cm->pgrp) {
		ppp_syslog(LOG_INFO, "ppp_rm_conn sending pid %d SIGHUP", cm->pgrp);
		kill(cm->pgrp, SIGHUP);
		if (cm->cfd)
			calltype = "Ougoing";
		else
			calltype = "Incoming";
			
	}
	ppp_syslog(LOG_INFO,"%s call on '%s' disconnected",
		calltype, ttyname(cm->fd));

	myundial(cm->fd);
	cm->cfd = 0;
	cm->fd = cm->muxid = cm->pgrp = 0;
}

/*
 * process messages from the ppp login shell or pppstat.
 * return 0 for ppp, return 1 for pppstat
 */
int
pppd_sockread(s, ppcidfd)
	int	s;
	int	ppcidfd;
{
	struct	conn_made_s	*cm;
	msg_t	msg;
	int	fd;
	int	totread;
	int	rval;
	char	*p;
	struct  ppphostent *hp;
	int	r, flgs=0;

	if (Debug)
		ppp_syslog(LOG_INFO, "pppd_sockread s=%d", s);
	totread = 0;
	p = (char *)&msg;
	memset(p, 0, sizeof(msg));
	do {
		if ((rval = read(s, p, sizeof(msg) - totread)) < 0) {
			ppp_syslog(LOG_ERR,"read: %m");
			return 0;
		}
		p += rval;
		totread += rval;
	} while (totread < sizeof(msg));

	if (Debug)
		ppp_syslog(LOG_INFO, "read m_type=%d", msg.m_type);

	switch (msg.m_type) {
	case MTTY:
		ppp_syslog(LOG_INFO,"Incoming call on '%s', pid=%d, name=%s",
					msg.m_tty, msg.m_pid, msg.m_name);
		if ((fd = open(msg.m_tty, O_RDWR)) < 0) {
			ppp_syslog(LOG_INFO, "can't open '%s'", msg.m_tty);
			kill(msg.m_pid, SIGHUP);
			return 0;
		}

		hp = getppphostbyname(msg.m_name);
		if (!hp) {
			ppp_syslog(LOG_INFO, "no parameter for '%s'", msg.m_name);
			kill(msg.m_pid, SIGHUP);
			return 0;
		}

		if (cm = ppp_add_conn(ppcidfd, fd)) {
			cm->pgrp = msg.m_pid;
                        ppp_syslog(LOG_DEBUG,"Assigned link id for incoming link(login:%s pid:%d) is %d", msg.m_name, msg.m_pid, cm->muxid);

			memcpy(&(((struct sockaddr_in *)
				&(databuf.di_ia.ia_dstaddr))->sin_addr),
				hp->ppp_h_ent->h_addr, sizeof(struct in_addr));

			memcpy(&databuf.u_di.s_di1.di1_cnf,
				&hp->ppp_cnf,sizeof(struct ppp_configure));
	
			ctlbuf.function = PPCID_CNF;
			ctlbuf.l_index = cm->muxid;
			ctlsb.len = sizeof(struct ppp_ppc_inf_ctl_s);
			datasb.len = sizeof(struct ppp_ppc_inf_dt_s);
			if (Debug) {
                                ppp_syslog(LOG_DEBUG,"sent PPCID_CNF");
                        }
			r = putmsg(ppcidfd,&ctlsb,&datasb,RS_HIPRI);
			if (r < 0) {
				 ppp_syslog(LOG_ERR, "config putmsg failed: %m");
                                return 0;
                        }
		} else {
			close(fd);
			kill(msg.m_pid, SIGHUP);
		}
		break;
	case MPID:
		if (Debug)
			ppp_syslog(LOG_INFO, "MPID pid=%d", msg.m_pid);
		for (cm = conn_made; cm < &conn_made[N_CONN]; cm++) {
			if (cm->pgrp == msg.m_pid) {
				ppp_rm_conn(ppcidfd, cm);
				return 0;
			}
		}
		return 0;
	case MSTAT:
		if (Debug)
			ppp_syslog(LOG_INFO, "MSTAT pid=%d", msg.m_pid);
		ctlbuf.function = PPCID_STAT;
		ctlsb.len = sizeof(struct ppp_ppc_inf_ctl_s);
		datasb.len = sizeof(struct ppp_ppc_inf_dt_s);
		if (Debug) {
                        ppp_syslog(LOG_DEBUG,"sent PPCID_STAT");
                }
		r = putmsg(ppcidfd,&ctlsb,&datasb,RS_HIPRI);
		if (r < 0) {
			 ppp_syslog(LOG_ERR, "config putmsg failed: %m");
                         return 0;
                }

		return 1;

        case MDIAL:
                ppp_syslog(LOG_INFO, "Connected to remote system: tty=%s, pid=%d,addr=%s",
                msg.m_tty, msg.m_pid, inet_ntoa(msg.m_remote.sin_addr));

                ctlbuf.function = PPCID_RSP;
                if ((fd = open(msg.m_tty, O_RDWR)) < 0) {
                        ppp_syslog(LOG_WARNING, "can't open %s: %m", msg.m_tty);
                        kill(msg.m_pid, SIGHUP);
                        ctlbuf.l_index = -1;
                        goto ppcid_rsp;
                }
		hp = getppphostbyaddr((char *)&(msg.m_remote.sin_addr),
                        sizeof(struct in_addr), msg.m_remote.sin_family);

                if (!hp) {
                        ppp_syslog(LOG_WARNING, "no parameter for '%s'",
                                inet_ntoa(msg.m_remote.sin_addr));
                        kill(msg.m_pid, SIGHUP);
                        ctlbuf.l_index = -1;
                        goto ppcid_rsp;
                }

                if (cm = ppp_add_conn(ppcidfd, fd)){
                        ppp_syslog(LOG_DEBUG,"Assigned link id for outgoing link(remote IP: %s) is %d", inet_ntoa(msg.m_remote.sin_addr), cm->muxid);
                        ctlbuf.l_index = cm->muxid;
                        cm->pgrp = msg.m_pid;
                        cm->cfd = fd;   /* hack for call type in rm_conn */
                } else {
                        kill(msg.m_pid, SIGHUP);
                        ctlbuf.l_index = -1;
                        goto ppcid_rsp;
		}
		/* Pass configurable parameters to kernel (Active open side) */
                memcpy(&databuf.u_di.s_di1.di1_cnf, &hp->ppp_cnf, 
			sizeof(struct ppp_configure));
ppcid_rsp:
                memcpy(&databuf.di_ia.ia_dstaddr, &msg.m_remote, 
				sizeof(struct sockaddr_in));
                ctlsb.len = sizeof(struct ppp_ppc_inf_ctl_s);
                datasb.len = sizeof(struct ppp_ppc_inf_dt_s);
                if (Debug) {
                        ppp_syslog(LOG_DEBUG,"sent PPCID_RSP");
                }
                if (putmsg(ppcidfd, &ctlsb, &datasb, flgs) < 0 ) {
                        if(cm)
				 ppp_rm_conn(cm);
                        ppp_syslog(LOG_ERR, "ppcid put msg fail: %m");
                }
                return 0;

	default:
		ppp_syslog(LOG_INFO, "pppd_sockread: bad m_type 0x%x.",
							msg.m_type);
		return 0;
	}
	return 0;
}

/*
 * process messages from the ppp protocol stack (/dev/ppcid)
 */
ppcid_msg(ppcidfd)
int ppcidfd;
{
	int	i,flgs=0;
	int	fd;
	int	retval;
	struct ppphostent *hp;
	struct conn_made_s *cm;
	struct ifreq ifr;
	char pid[PID_SIZE],pwd[PWD_SIZE];
	struct termio trm;
	int	r;
	int	dpid;

do_over:
	if ((retval = getmsg(ppcidfd, &ctlsb, &datasb, &flgs)) < 0) {
		int oerrno = errno;
		if (errno == EAGAIN) {
			return;
		}
		ppp_syslog(LOG_DEBUG, "ppcid_msg() getmsg FAILED");
		ppp_syslog(LOG_DEBUG, "retval=%d: %m", retval);
		exit(oerrno);
	}

	if (ctlsb.len < sizeof(int)) {
		goto do_over;
	}
	
	switch (ctlbuf.function) {

	case PPCID_UP:
		if (Debug)
			ppp_syslog(LOG_INFO, "PPCID_UP");
		if (datasb.len < sizeof(struct inf_dt2)) {
			goto do_over;
		}
		fd = open(IPDEV, O_RDONLY);
		if (fd < 0) {
			ppp_syslog(LOG_ERR,"open of %s failed: %m",IPDEV);
                        return;
                }

		(void)sprintf(ifr.ifr_name, "%s%x", databuf.di_ifname,
						    databuf.di_ifunit);
		r = sioctl(fd, SIOCGIFFLAGS, (caddr_t)&ifr,
				      sizeof(struct ifreq));
		if (r < 0) {
			ppp_syslog(LOG_ERR,"get if flags on %s failed: %m ",
                                        ifr.ifr_name);
                        close(fd);
                        return;
                }
 
		if (!(ifr.ifr_flags & IFF_UP)) {
			ifr.ifr_flags |= IFF_UP;
			r = sioctl(fd, SIOCSIFFLAGS, (caddr_t)&ifr,
				      sizeof(struct ifreq)); 
			if (r < 0) {
                                ppp_syslog(LOG_ERR,
                                        "set if flags on %s failed: %m",
                                        ifr.ifr_name);
                                close(fd);
                                return;
                        }
			if (Debug)
				ppp_syslog(LOG_INFO, "PPP interface (%s) marked UP",
					ifr.ifr_name);
		}
		if (Debug)
			ppp_syslog(LOG_INFO, "closing fd to /dev/ip");
		(void)close(fd);
		break;
	case PPCID_PAP:
		if (Debug)
			ppp_syslog(LOG_INFO, "PPCID_PAP");
		if (datasb.len < sizeof(struct inf_dt3)) {
			goto do_over;
		}
		(void)sprintf(pid,"%s", databuf.di_pid);
		papgetpwd(pid,pwd);
		if(pwd[0]!='\0' && strcmp(pwd,databuf.di_pwd)==0){
			databuf.di_pid[0] = '\0';
			ppp_syslog(LOG_INFO, "PPCID_PAP auth success");
		} else {
			strcpy(databuf.di_pid,"PWD not match");
			ppp_syslog(LOG_INFO, "PPCID_PAP auth fail");
		};	
		if (Debug) {
                        ppp_syslog(LOG_DEBUG,"sent PPCID_PAP");
                }
		r = putmsg(ppcidfd,&ctlsb,&datasb,RS_HIPRI);
		if (r < 0) {
                        ppp_syslog(LOG_ERR, "PAP putmsg failed: %m");
                }
		break;
	case PPCID_STAT:
		if (Debug)
			ppp_syslog(LOG_INFO, "PPCID_STAT");
		if (datasb.len < sizeof(struct inf_dt4)) {
			ppp_syslog(LOG_INFO, "PPCID_STAT do_over");
			goto do_over;
		}
		if(write(msg_sock,(char *)&databuf.di_stat, 
			sizeof(struct ppp_stat))<0)
			ppp_syslog(LOG_INFO, "socket write fail");
		close(msg_sock); 
		break;
	case PPCID_REQ:
		if (Debug)
			ppp_syslog(LOG_INFO, "PPCID_REQ");
		if (datasb.len<sizeof(struct in_ifaddr)) {
			goto do_over;
		}

		/* fork a child process to do the dialing */
                if ((dpid = fork()) < 0) {
                        ppp_syslog(LOG_ERR, "fork() fail: %m");
                        ctlbuf.function = PPCID_RSP;
                        ctlbuf.l_index = -1;
                        if (putmsg(ppcidfd, &ctlsb, &datasb, flgs) < 0)
                                ppp_syslog(LOG_ERR, "ppcid putmsg fail: %m");
                        break;
                }

		if (dpid == 0) {
                        pppdial(databuf.di_ia.ia_dstaddr);
                }
                break;

	case PPCID_CLOSE:
		if (Debug)
			ppp_syslog(LOG_INFO, "PPCID_CLOSE: muxid=%d",ctlbuf.l_index);
		for (cm = conn_made; cm < &conn_made[N_CONN]; cm++) {
			if (cm->muxid == ctlbuf.l_index) {
				ppp_rm_conn(ppcidfd, cm);
			}
		}
		break;
	}
	goto do_over;
}

/*
 * wrapper for doing STREAMS ioctls
 */
ifioctl(s, cmd, arg)
char           *arg;
{
	struct strioctl ioc;

	ioc.ic_cmd = cmd;
	ioc.ic_timout = 0;
	ioc.ic_len = sizeof(struct ifreq);
	ioc.ic_dp = arg;
	return (ioctl(s, I_STR, (char *) &ioc));
}

/*
 * convert baud rate from termio.c_flag format to and integer.
 */
getbaudrate(b)
	int b;
{
	switch (b) {
#if defined(B50)
	case B50:
		return 50;
#endif
#if defined(B75)
	case B75:
		return 75;
#endif
#if defined(B110)
	case B110:
		return 110;
#endif
#if defined(B134)
	case B134:
		return 134;
#endif
#if defined(B150)
	case B150:
		return B150;
#endif
#if defined(B200)
	case B200:
		return 200;
#endif
#if defined(B300)
	case B300:
		return 300;
#endif
#if defined(B600)
	case B600:
		return 600;
#endif
#if defined(B1200)
	case B1200:
		return 1200;
#endif
#if defined(B2400)
	case B2400:
		return 2400;
#endif
#if defined(B4800)
	case B4800:
		return 4800;
#endif
#if defined(B9600)
	case B9600:
		return 9600;
#endif
#if defined(EXTA)
	case EXTA:
		return 19200;
#else
#if defined(B19200)
	case B19200:
		return 19200;
#endif
#endif
#if defined(EXTB)
	case EXTB:
		return 38400;
#else
#if defined(B38400)
	case B38400:
		return 38400;
#endif
#endif
	case B0:
	default:
		return 0xffff;
	}
}

/*
 * Do a STREAMS ioctl call
 */
int
sioctl(fd, cmd, dp, len)
	int	fd, cmd, len;
	caddr_t	dp;
{
	struct strioctl iocb;

	iocb.ic_cmd = cmd;
	iocb.ic_timout = 15;
	iocb.ic_dp = dp;
	iocb.ic_len = len;
	return ioctl(fd, I_STR, &iocb);
}

ppp_syslog(pri, fmt, va_alist)
	int pri;
	char *fmt;
	va_dcl
{
	va_list args;

	va_start(args);
#if 1
	vsyslog(pri, fmt, args);
#else
	fprintf(stderr, "in.pppd: ");
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	fflush(stderr);
#endif
}


/* Global variables shared between dial process and its signal handler */
int     dialfd = 0, rsp = 0;
struct sockaddr_in      rifaddr;

/* signal processor for dial process when gets SIGHUP from the main daemon
 */
void
sig_undial(sig)
int sig;
{
	ppp_syslog(LOG_INFO, "sig_undial: kill dial process");
        if (dialfd) {
                myundial(dialfd);
                dialfd = 0;
       	}
	exit(0);
}

/* signal processor for dial process : SIGHUP, SIGINT, SIGQUIT and SIGTERM */
void
sig_ntfy(sig)
int sig;
{
        int     flgs=0;
        msg_t   msg;
        int     s, rval;

        ppp_syslog(LOG_INFO, "sig_ntfy: kill dial process");
	if (dialfd) {
		myundial(dialfd);
                dialfd = 0;
        }
        if (!rsp) {
                /* get a sig before finish dialing
                 * inform the kernel
                 */
                rsp = 1;
                ctlbuf.function = PPCID_RSP;
                ctlbuf.l_index = -1;
                memcpy(&databuf.di_ia.ia_dstaddr, &rifaddr, 
			sizeof(struct sockaddr_in));
                ctlsb.len = sizeof(struct ppp_ppc_inf_ctl_s);
                datasb.len = sizeof(struct ppp_ppc_inf_dt_s);
                if (Debug) {
                        ppp_syslog(LOG_DEBUG,"sending PPCID_RSP from sig_ntfy");
                }
                if (putmsg(ppcidfd, &ctlsb, &datasb, flgs) < 0)
                        ppp_syslog(LOG_ERR, "ppcid put msg fail: %m");
	} else {
                /* get a sig after finish dialing
                 * inform the daemon
                 */
                s = ppp_sockinit();
                memset((char *)&msg, 0, sizeof(msg));
                msg.m_type = MPID;
                msg.m_pid = getpid();
                rval = write(s, (char *)&msg, sizeof(msg));
                if (rval < 0) {
                        ppp_syslog(LOG_INFO, "write to socket failed: %m");
                        exit(0);
                }
                close(s);
                ppp_syslog(LOG_INFO, "notify_daemon sig=%d pid=%d", sig, msg.m_pid);
        }
        exit(0);
}
	
pppdial(ifaddr)
        struct  sockaddr_in ifaddr;
{
        int i, retry = 2, s, rval, flgs=0;
        struct ppphostent *hp;
        char *p;
        msg_t msg;
	CALL	call;
	CALL_EXT	call_ext;
	char	service[] = "uucico";

        memcpy(&rifaddr, &ifaddr, sizeof(struct sockaddr_in));
        sigset(SIGCLD, SIG_DFL);
        sigset(SIGHUP, sig_ntfy);
        sigset(SIGINT, sig_ntfy);
        sigset(SIGQUIT, sig_ntfy);
        sigset(SIGTERM, sig_ntfy);
        sigset(SIGUSR1, sig_undial);

        ctlbuf.function = PPCID_RSP;

        hp=getppphostbyaddr(
                &(((struct sockaddr_in *)&(databuf.di_ia.ia_dstaddr))->sin_addr),sizeof(struct in_addr),databuf.di_ia.ia_dstaddr.sa_family);

	if(!hp) {
		goto fail;
	}
		
tryagain:
	ppp_syslog(LOG_INFO,"Dialing host '%s'(%s) - uucp system '%s;",
		hp->ppp_h_ent->h_name ? hp->ppp_h_ent->h_name : "-",
                inet_ntoa(*(ulong *)hp->ppp_h_ent->h_addr),
                hp->uucp_system);

	call.attr = NULL;               /* termio attributes */
	call.baud = 0;                  /* unused */
	call.speed = -1;                /* any speed */
	/*
	 * If "-" is specified for the tty line in /etc/inet/ppphosts,
	 * pass the call.line field as NULL to the connection server.
	 * Passing "-" to the connection server used to work because
	 * the connection server ignored the line field of the call
	 * structure, but now it honors it, so it would try to find
	 * the device named "-".
	 */
	if (strcmp(hp->tty_line, "-") == 0)
		call.line = NULL;
	else
		call.line = hp->tty_line;
	call.telno = hp->uucp_system;   /* give name, not number */
	call.modem = 0;                 /* modem control not required */
	call.device = (char *)&call_ext;
	call.dev_len = 0;               /* unused */

	call_ext.service = service;
	call_ext.class = NULL;
	call_ext.protocol = NULL;

	if ((dialfd = dial(call)) < 0) {
		ppp_syslog(LOG_INFO, "Dial failure host '%s'(%s) using uucp system '%s' = %d",
			hp->ppp_h_ent->h_name ? hp->ppp_h_ent->h_name : "-",
			inet_ntoa(*(ulong *)hp->ppp_h_ent->h_addr),
			hp->uucp_system, dialfd);

		if (--retry)
			 goto tryagain;
		goto fail;
	}

	if (!(p = ttyname(dialfd))){
                ppp_syslog(LOG_WARNING, "dialfd returned from conn() not tty");
                goto fail;
        }

	 /* If dial succeeds, send a MDIAL message to the main daemon */
        memset((char *)&msg, 0, sizeof(msg));
        msg.m_type = MDIAL;
        msg.m_pid = getpid();
        strncpy(msg.m_tty, p, TTY_SIZE);
        memcpy(&msg.m_remote, &ifaddr, sizeof(struct sockaddr_in));

        s = ppp_sockinit();
        rval = write(s, (char *)&msg, sizeof(msg));
        if (rval < 0) {
                ppp_syslog(LOG_WARNING, "dial process: write to socket failed: %m");
                goto fail;
        }
        rsp = 1;
        close(s);
        ppp_syslog(LOG_INFO, "dial process sent pppd m_type=%d, m_pid=%d, m_tty='%s'",
                        msg.m_type, msg.m_pid, msg.m_tty);

	/*
         * Wait for daemon to notify (signal)
         * us that we can go away.
         */
        while (pause() == -1);

        exit(0);

        /* If dial fails, send a message to the kernel directly.
         * Note it is possible the link between dial process
         * and main daemon could be broken
         */    
fail:
        rsp = 1;
        ctlbuf.function = PPCID_RSP;
        ctlbuf.l_index = -1;

	memcpy(&databuf.di_ia.ia_dstaddr, &ifaddr, sizeof(struct sockaddr_in));

	ctlsb.len = sizeof(struct ppp_ppc_inf_ctl_s);
	datasb.len = sizeof(struct ppp_ppc_inf_dt_s);
	if (Debug) {
	        ppp_syslog(LOG_DEBUG,"send PPCID_RSP after pppdial failed");
        }

	if (putmsg(ppcidfd,&ctlsb,&datasb,flgs) < 0) {
                ppp_syslog(LOG_DEBUG, "PPCID_REQ putmsg failed: %m");
        }
        exit(0);
}

void
myundial(fd)
int	fd;
{
	struct	stat	_st_buf;
       	char    lockname[BUFSIZ];
	
	if (fd) {
		/* NOTE: undial() doesn't remove the lock file.
	  	 * we have to remove the lock file
		 */
		if (fstat(fd, &_st_buf) == 0 ) {
        	(void) sprintf(lockname, "%s.%3.3lu.%3.3lu.%3.3lu", L_LOCK,
            		(unsigned long) major(_st_buf.st_dev),
           		(unsigned long) major(_st_buf.st_rdev),
            		(unsigned long) minor(_st_buf.st_rdev));
    		}
		(void) unlink(lockname);
                
		undial(fd);
	}
}
