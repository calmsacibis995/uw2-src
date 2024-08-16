/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:common/cmd/ttymon/ttymon.notp/tmchild.c	1.7"

#include	<stdlib.h>
#include	<stdio.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<sys/types.h>
#include	<termio.h>
#include	<string.h>
#include	<signal.h>
#include	<poll.h>
#include	<unistd.h>
#include	<pfmt.h>
#include 	"sys/stropts.h"
#include	<sys/resource.h>
#include	"sac.h"
#include	"ttymon.h"
#include	"tmstruct.h"
#include	"tmextern.h"
#include	<sys/utsname.h>

static void openline();
static void sys_name();
static void invoke_service();
static char	*do_autobaud();
static	struct	Gdef	*next_speed();
static int check_hup();

extern	struct	Gdef	*get_speed();
extern	pid_t	tcgetsid(), getsid();

/*
 * tmchild	- process that handles peeking data, determine baud rate
 *		  and invoke service on each individual port.
 *
 */
void
tmchild(pmtab)
struct	pmtab	*pmtab;
{
	register struct Gdef *speedef;
	char	*auto_speed = NULL;
	int	first=FALSE;
	struct	sigaction sigact;

#ifdef	DEBUG
	debug("in tmchild");
#endif
	/*
	 * lock the line straight away to avoid timing holes. If
	 * we tmchild exits for any reason along the way it will serve
	 * as an implicit unlock.
	 */
	if (pmtab->p_ttyflags & B_FLAG) { /* port is bi-directional */
		/* set advisory lock on the line */
		if (tm_lock(pmtab->p_fd) != 0) { 
			/*
			 * device is locked 
			 * child exits and let the parent wait for
			 * the lock to go away
			 */
			exit(0);
		}
	}

	if (pmtab->p_status != GETTY) {
		child_sigcatch(); 
		(void)close(PCpipe[0]);	  /* close parent end of the pipe */
		if (ioctl(PCpipe[1], I_SETSIG, S_HANGUP) == -1) {
			(void)sprintf(Scratch,"I_SETSIG failed, errno = %d",
					errno);
			log(Scratch);
			exit(1);
		}
		/*
		 * the following check is to make sure no hangup
		 * happens before registering for SIGPOLL
		 */
		if (check_hup(PCpipe[1])) {
#ifdef	DEBUG
			debug("PCpipe hungup, tmchild exiting");
#endif
			exit(1);
		}

		/*
		 * become the session leader so that a controlling tty
		 * will be allocated.
		 */
		(void)setsid();
		first = TRUE;

	}

	speedef = get_speed(pmtab->p_ttylabel);

	/*
	 * we timeout around 'openline' (unless the C_FLAG is set) 
	 * because it is possible that we got spurious stuff on
	 * the line and carrier is not on so the open in openline
	 * would block forever. also if we don't get all out 'r' counts
	 * satisified we return.	
	 */
	if(pmtab->p_timeout && (! (pmtab->p_ttyflags & C_FLAG))) {
		sigact.sa_flags = SA_RESETHAND | SA_NODEFER;
		sigact.sa_handler = timedout;
		(void)sigemptyset(&sigact.sa_mask);
		(void)sigaction(SIGALRM, &sigact, NULL);
		(void)alarm((unsigned)pmtab->p_timeout);
	}

	openline(pmtab); 

	if(pmtab->p_timeout && (! (pmtab->p_ttyflags & C_FLAG))) {
		(void)alarm((unsigned)0);
		sigact.sa_flags = 0;
		sigact.sa_handler = SIG_DFL;
		(void)sigemptyset(&sigact.sa_mask);
		(void)sigaction(SIGALRM, &sigact, NULL);
	}

	if ((pmtab->p_ttyflags & C_FLAG) &&
	    (State != PM_DISABLED) &&
	    (!(pmtab->p_flags & X_FLAG))) {
		flush_input(0);
		/*
		 * if "c" flag is set, and the port is not disabled
		 * invoke service immediately 
		 */
		if (set_termio(0,speedef->g_fflags,NULL,FALSE,CANON) == -1) {
			log("set final termio failed");
			exit(1);
		}
		invoke_service(pmtab);
		exit(1);	/*NOTREACHED*/
	}
	if (speedef->g_autobaud & A_FLAG) {
		auto_speed = do_autobaud(pmtab,speedef);
	}
	if ( (pmtab->p_ttyflags & (R_FLAG|A_FLAG)) ||
		(pmtab->p_status == GETTY) || (pmtab->p_timeout > 0) ) {
		write_prompt(1,pmtab,TRUE,TRUE);
		if(pmtab->p_timeout) {
			sigact.sa_flags = SA_RESETHAND | SA_NODEFER;
			sigact.sa_handler = timedout;
			(void)sigemptyset(&sigact.sa_mask);
			(void)sigaction(SIGALRM, &sigact, NULL);
			(void)alarm((unsigned)pmtab->p_timeout);
		}
	}

	/* Loop until user is successful in invoking service. */
	for(;;) {

		/* Peek the user's typed response and respond appropriately. */
		switch(poll_data(first)) {
		case GOODNAME:
#ifdef	DEBUG
			debug("got GOODNAME");
#endif	
			if (pmtab->p_timeout) {
				(void)alarm((unsigned)0);
				sigact.sa_flags = 0;
				sigact.sa_handler = SIG_DFL;
				(void)sigemptyset(&sigact.sa_mask);
				(void)sigaction(SIGALRM, &sigact, NULL);
			}
			if ((State == PM_DISABLED)||(pmtab->p_flags & X_FLAG)){
				write_prompt(1,pmtab,TRUE,FALSE);
				break;
			}
			if (set_termio(0,speedef->g_fflags,auto_speed,
				FALSE,CANON)==-1) {
				log("set final termio failed");
				exit(1);
			}
			invoke_service(pmtab);
			exit(1);	/*NOTREACHED*/

		case BADSPEED:
			/* wrong speed! try next speed in the list. */
			speedef = next_speed(speedef);
#ifdef	DEBUG
			debug("BADSPEED: setup next speed");
#endif
			if (speedef->g_autobaud & A_FLAG) {
				if (auto_termio(0, 0) == -1) {
					exit(1);
				}
				auto_speed = do_autobaud(pmtab,speedef);
			}
			else {
				auto_speed = NULL;
				/*
				 * this reset may fail if the speed is not 
				 * supported by the system
				 * we just cycle through it to the next one
				 */
				if (set_termio(0,speedef->g_iflags,NULL,
						FALSE,CANON) != 0) {
					(void)sprintf(Scratch, "Warning -- speed of <%s> may be not supported by the system", speedef->g_id);
					log(Scratch);
				}
			}
			write_prompt(1,pmtab,TRUE,TRUE);
			break;

		case NONAME:
#ifdef	DEBUG
			debug("got NONAME");
#endif	
			write_prompt(1,pmtab,FALSE,FALSE);
			break;

		}  /* end switch */

		if (first)
			first = FALSE;
		if(pmtab->p_timeout) {
			sigact.sa_flags = SA_RESETHAND | SA_NODEFER;
			sigact.sa_handler = timedout;
			(void)sigemptyset(&sigact.sa_mask);
			(void)sigaction(SIGALRM, &sigact, NULL);
			(void)alarm((unsigned)pmtab->p_timeout);
		}
	} /* end for loop */
}

static void
openline(pmtab)
struct	pmtab 	*pmtab;
{
	char	 buffer[5];
	int	 line_count;

#ifdef	DEBUG
	debug("in openline");
#endif

	if (pmtab->p_status != GETTY) {
		(void)close(0);
		/* open should return fd 0, if not, then close it */
		if (open(pmtab->p_device, O_RDWR) != 0) {
			(void)sprintf(Scratch,"open \"%s\" failed, errno = %d",
					pmtab->p_device,errno);
			log(Scratch);
			exit(1);
		}
	}
	(void)close(1);
	(void)close(2);
	(void)dup(0);
	(void)dup(0);

	if (pmtab->p_ttyflags & R_FLAG) { /* wait_read is needed */
		if (pmtab->p_count) { 
			/*
			 * -b with -r > 0 is now defined to count one
			 * char for the -b and then start counting 
			 * the 'r' so we need to suck up the char if
			 * -b is on.
			 */
			 if (pmtab->p_ttyflags & B_FLAG) {
				if( read(0, buffer, 1) < 0) {
					(void)close(0);
					exit(0);
				}
			}
			/* 
			 * - wait for "p_count" lines 
			 * - datakit switch does not
			 *   know you are a host or a terminal
			 * - so it send you several lines of msg 
			 * - we need to swallow that msg
			 * - we assume the baud rate is correct
			 * - if it is not, '\n' will not look like '\n'
 			 *   and we will wait forever here
			 */
			for (line_count=0;line_count < pmtab->p_count;) {
				if ( read(0, buffer, 1) < 0
				     || *buffer == '\0'
				     || *buffer == '\004') {
					(void)close(0);
					exit(0);
				}
				if (*buffer == '\n')
					line_count++;
			}
		}
		else { /* wait for 1 char */
			/*
			 * NOTE: Cu on a direct line when ~. is encountered will
			 * send EOTs to the other side.  EOT=\004
			 */
			if ( read(0, buffer, 1) < 0
			  || *buffer == '\004') {
				(void)close(0);
				exit(0);
			}
		}
		if (!(pmtab->p_ttyflags & A_FLAG)) { /* autobaud not enabled */
			if (turnon_canon(0) == -1) {
				log("openline: turnon_canon failed");
				exit(1);
			}
		}
	}
	if (pmtab->p_ttyflags & B_FLAG) { /* port is bi-directional */
		/* change ownership back to root */
		(void)fchown(0, ROOTUID, Tty_gid);
		(void)fchmod(0, 0620);
	}
	return;
}

/*
 *	write_prompt	- write the msg to fd
 *			- if flush is set, flush input queue
 *			- if clear is set, write a new line 
 */
void
write_prompt(fd,pmtab,flush,clear)
int	fd;
struct	pmtab	*pmtab;
int	flush, clear;
{

#ifdef DEBUG
	debug("in write_prompt");
#endif
	if (flush)
		flush_input(fd);
	if (clear) {
		(void)write(fd,"\r\n",2);
	}

	sys_name(fd);

	/* Print prompt/disable message. */
	if ((State == PM_DISABLED)||(pmtab->p_flags & X_FLAG))
		(void)write(fd, pmtab->p_dmsg, (unsigned)strlen(pmtab->p_dmsg));
	else
		(void)write(fd, pmtab->p_prompt,
			(unsigned)strlen(pmtab->p_prompt));
}

/*
 *	timedout	- input period timed out
 */
void
timedout()
{
	exit(1);
}

/*
 * void sys_name() - print the /etc/issue file (if it exists)
 *		     parameterized with the nodename, release, version, etc.
 *		     indicated by %n, %r, %v and so on.
 */

#define NSTRS	6	/* large enough to print every string at least once */

static void
sys_name(fd)
int	fd;
{
	FILE	*fpi, *fpo;
	char	*ptr, *msg_id, *msg, buffer[BUFSIZ];
	struct	utsname utsname;
	char	*strs[NSTRS];				/* fprintf args */
	int	s;					/* index into strs[] */
	int	s0;					/* initializer for s */
	char	c;

	write(fd, "\r\n", 2);

	if ((fpi = fopen(ISSUEFILE, "r")) == NULL)
		return;

	if ((fd = dup(fd)) < 0 || (fpo = fdopen(fd, "w")) == NULL) {
		(void) fclose(fpi);
		(void) close(fd);	/* dup or -1, no matter */
		return;
	}

	s0 = (uname(&utsname) == FAILURE)? NSTRS : 0;	/* fail => no strs */

	while (fgets(buffer, sizeof buffer, fpi) != NULL) {
		if (buffer[0] == '#')
			continue;			/* comment */

		if ((msg_id = strtok(buffer, "\001\n")) == NULL)
			buffer[0] = '\0';		/* empty line */
		else if ((msg = strtok(NULL, "\001\n")) != NULL)
			strcpy(buffer, gettxt(msg_id, msg));

		for (s = s0, ptr = buffer; (c = *ptr++) != '\0'; ) {
			if (c != '%')
				continue;

			switch (*ptr) {
			case 'm':
				msg = utsname.machine;	/* %m */
				break;
			case 'n':
				msg = utsname.nodename;	/* %n */
				break;
			case 'r':
				msg = utsname.release;	/* %r */
				break;
			case 's':
				msg = utsname.sysname;	/* %s */
				break;
			case 'v':
				msg = utsname.version;	/* %v */
				break;
			case '\0':
				ptr[-1] = ' ';		/* %<NUL> -> blank */
				continue;		/* "for" terminates */
			default:
				*ptr++ = '%';		/* %? -> %% */
				continue;
			}

			if (s >= NSTRS)			/* strs exhausted */
				*ptr++ = '%';		/* format becomes %% */
			else {
				*ptr++ = 's';		/* format becomes %s */
				strs[s++] = msg;	/* arg for this %s */
			}
		}

		fprintf(fpo, buffer,
			/* note NSTRS dependency in following line */
			strs[0], strs[1], strs[2], strs[3], strs[4], strs[5]);
		fputs("\r\n", fpo);
	}
	(void) fclose(fpi);
	(void) fclose(fpo);
}

#undef NSTRS

/*
 *	do_autobaud	- do autobaud
 *			- if it succeed, set the new speed and return
 *			- if it failed, it will get the nextlabel
 *			- if next entry is also autobaud,
 *			  it will loop back to do autobaud again
 *			- otherwise, it will set new termio and return
 */
static	char	*
do_autobaud(pmtab,speedef)
struct	pmtab	*pmtab;
struct	Gdef	*speedef;
{
	int	done = FALSE;
	char	*auto_speed;
#ifdef	DEBUG
	debug("in do_autobaud");
#endif
	while (!done) {
		if ((auto_speed = autobaud(0,pmtab->p_timeout, 0)) == NULL) {
			speedef = next_speed(speedef);
			if (speedef->g_autobaud & A_FLAG) {
				continue;
			}
			else {
				if (set_termio(0,speedef->g_iflags,NULL,
						TRUE,CANON) != 0) {
					exit(1);
				}
				done = TRUE;
			}
		}
		else {
			if (set_termio(0,speedef->g_iflags,auto_speed,
					TRUE,CANON) != 0) {
				exit(1);
			}
			done = TRUE;
		}
	}
#ifdef	DEBUG
	debug("autobaud done");
#endif
	return(auto_speed);
}

/*
 * 	next_speed(speedef) 
 *	- find the next entry according to nextlabel. If "nextlabel"
 *	  is not valid, go back to the old ttylabel.
 */

static	struct	Gdef *
next_speed(speedef)
struct	Gdef *speedef;
{
	struct	Gdef *sp;

	if (strcmp(speedef->g_nextid,speedef->g_id) == 0) 
		return(speedef);
	if ((sp = find_def(speedef->g_nextid)) == NULL) {
		(void)sprintf(Scratch,"%s's next speed-label (%s) is bad.",
			speedef->g_id, speedef->g_nextid);
		log(Scratch);

		/* go back to the original entry. */
		if((sp = find_def(speedef->g_id)) == NULL) {
			 /* if failed, complain and quit. */
			(void)sprintf(Scratch,"unable to find (%s) again",
				speedef->g_id);
			log(Scratch);
			exit(1);
		}
	}
	return(sp);
}

/*
 * inform_parent()	- inform ttymon that tmchild is going to exec service
 */
static	void
inform_parent(fd)
int	fd;
{
	pid_t	pid;

	pid = getpid();
	(void)write(fd, &pid, sizeof(pid));
}

static	char	 pbuf[BUFSIZ];	/* static buf for TTYPROMPT 	*/
static	char	 hbuf[BUFSIZ];	/* static buf for HOME 		*/

/*
 * void invoke_service	- invoke the service
 */

static	void
invoke_service(pmtab)
struct	pmtab	*pmtab;
{
	char	 *argvp[MAXARGS];		/* service cmd args */
	int	 cnt = 0;			/* arg counter */
	int	 i, fd;
	struct	 sigaction	sigact;
	extern	 int	doconfig();
	extern	 struct rlimit	Rlimit;
	static   char   buf[BUFSIZ];


#ifdef 	DEBUG
	debug("in invoke_service");
#endif

	if (tcgetsid(0) != getsid(getpid())) {
	    if ((fd = open(CONSOLE, O_WRONLY|O_NOCTTY)) != -1) {
		(void)sprintf(Scratch,
	       "Warning -- ttymon cannot allocate controlling tty on \"%s\",\n",
					pmtab->p_device);
		(void) write(fd, Scratch, strlen(Scratch));
		(void)sprintf(Scratch,
	      "           there may be another session active on this port.\n");
		(void) write(fd, Scratch, strlen(Scratch));
		(void) close(fd);
	    }
	    if (strcmp("/dev/console", pmtab->p_device) != 0) {
		/* if not on console, write to stderr to warn the user also */
		(void)fprintf(stderr,
	       "Warning -- ttymon cannot allocate controlling tty on \"%s\",\n",
					pmtab->p_device);
		(void)fprintf(stderr,
	      "           there may be another session active on this port.\n");
	    }
	}

	if (pmtab->p_status != GETTY) {
		inform_parent(PCpipe[1]);
		sigact.sa_flags = 0;
		sigact.sa_handler = SIG_DFL;
		(void)sigemptyset(&sigact.sa_mask);
		(void)sigaction(SIGPOLL, &sigact, NULL);
	}

	if (pmtab->p_flags & U_FLAG) {
		if (account(pmtab->p_device) != 0) {
			log("invoke_service: account failed");
			exit(1);
		}
	}
	/* put pmtab->p_server into buf, because strtok will break up buffer */
	(void) strcpy(buf,pmtab->p_server);

	/* parse command line */
	mkargv(buf,&argvp[0],&cnt,MAXARGS-1);

	if (!(pmtab->p_ttyflags & C_FLAG)) {
		(void) sprintf(pbuf, "TTYPROMPT=%s", pmtab->p_prompt);
		if (putenv(pbuf)) {
			(void) sprintf(Scratch, 
			"can't expand service <%s> environment",
			argvp[0]);
			log(Scratch);
			exit(1);
		}
	}
	if (pmtab->p_status != GETTY) {
		(void) sprintf(hbuf, "HOME=%s", pmtab->p_dir);
		if (putenv(hbuf)) {
			(void)sprintf(Scratch, 
			"can't expand service <%s> environment",argvp[0]);
			log(Scratch);
			exit(1);
		}
#ifdef	DEBUG
		debug("about to run config script");
#endif
		if ((i = doconfig(0, pmtab->p_tag, 0)) != 0) {
			if (i < 0) {
				log("doconfig failed, system error");
			}
			else {
				(void)sprintf(Scratch,
				"doconfig failed on line %d of script %s",
				i,pmtab->p_tag);
				log(Scratch);
			}
			exit(1);
		}
	}

#ifdef  DEBUG
        sprintf(Scratch, "before iascheme, VAL='%s'", pmtab->p_iascheme == NULL? "NULL" : pmtab->p_iascheme);
        debug(Scratch);
#endif
        if ((pmtab->p_iascheme != NULL) && (*(pmtab->p_iascheme) != '\0')) {
#ifdef  DEBUG
                debug("about to run iascheme");
#endif
                /* invoke identification & authentication scheme */
                if ((i = invoke(0, pmtab->p_iascheme)) != 0) {
                        /*
                         * Assume invoke will return -1 if it cannot parse or
                         * invoke the scheme, and return something else
                         * if the scheme fails after being exec'ed.
                         * The failed scheme should output the error message.
                         */
                        if (i == -1) {
                                sprintf(Scratch,":Cannot invoke iascheme <%s>: %s",                                        pmtab->p_iascheme, strerror(errno));
				log(Scratch);
                        }
                        exit(1);
                }
        }

        if ((pmtab->p_identity != NULL) && (*(pmtab->p_identity) != '\0')) {
                if (set_id(pmtab->p_identity) != 0) {
                        sprintf(Scratch, "set_id(%s) failed: %s",
				 pmtab->p_identity, strerror(errno));
			log(Scratch);
                        exit(1);
                }
        }
        else {
                if (set_id(NULL) != 0) {
                        sprintf(Scratch, "set_id(%s) failed: %s",
				 NULL, strerror(errno));
			log(Scratch);
                        exit(1);
                }
        }


	if (pmtab->p_status != GETTY) {
		sigact.sa_flags = 0;
		sigact.sa_handler = SIG_DFL;
		(void)sigemptyset(&sigact.sa_mask);
		(void)sigaction(SIGINT, &sigact, NULL);
		if (setrlimit(RLIMIT_NOFILE, &Rlimit) == -1) {
			(void)sprintf(Scratch, "setrlimit failed, errno = %d",errno);
			log(Scratch);
			exit(1);
		}
		/* invoke the service */
		(void)sprintf(Scratch,"Starting service (%s) on %s",argvp[0], 
			pmtab->p_device);
		log(Scratch);
	}

	/* restore signal handlers and mask */
	(void)sigaction(SIGINT, &Sigint, NULL);
	(void)sigaction(SIGALRM, &Sigalrm, NULL);
	(void)sigaction(SIGPOLL, &Sigpoll, NULL);
	(void)sigaction(SIGCLD, &Sigcld, NULL);
	(void)sigaction(SIGTERM, &Sigterm, NULL);
#ifdef	DEBUG
	(void)sigaction(SIGUSR1, &Sigusr1, NULL);
	(void)sigaction(SIGUSR2, &Sigusr2, NULL);
#endif
	(void)sigprocmask(SIG_SETMASK, &Origmask, NULL);
	(void) execve(argvp[0], argvp, environ);

	/* exec returns only on failure! */
	(void)sprintf(Scratch,"tmchild: exec service failed, errno = %d",
			errno);
	log(Scratch);
	exit(1);
}

/*
 *	check_hup(fd)	- do a poll on fd to check if it is in hangup state
 *			- return 1 if hangup, otherwise return 0
 */

static	int
check_hup(fd)
int	fd;
{
	int	ret;
	struct	pollfd	pfd[1];

	pfd[0].fd = fd;
	pfd[0].events = POLLHUP;
	for (;;) {
		ret = poll(pfd, 1, 0);
		if (ret < 0) {
			if (errno == EINTR) 
				continue;
			(void)sprintf(Scratch, 
			"check_hup: poll failed, errno = %d", errno);
			log(Scratch);
			exit(1);
		}
		else if (ret > 0) {
			if (pfd[0].revents & POLLHUP) {
				return(1);
			}
		}
		return(0);
	}
}

/*
 * sigpoll()	- SIGPOLL handle for tmchild
 *		- when SIGPOLL is received by tmchild,
 *		  the pipe between ttymon and tmchild is broken.
 *		  Something must happen to ttymon.
 */
void
sigpoll()
{
#ifdef	DEBUG
	debug("tmchild got SIGPOLL, exiting");
#endif
	exit(1);
}
