/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:common/cmd/ttymon/ttymon.notp/tmterm.c	1.9"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <termio.h>
#include <sys/stermio.h>
#include <sys/termiox.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include "sys/stropts.h"
#include "sys/signal.h"
#include "ttymon.h" 
#include "tmstruct.h" 

extern	char	Scratch[];
extern	void	log();
extern	void	mkargv();
extern  pid_t 	fn_getpid();
extern  void	fn_rmlock();

#define DIAL_RETRY 3

/*
 *	set_termio	- set termio on device 
 *		fd	- fd for the device
 *		options - stty termio options 
 *		aspeed  - autobaud speed 
 *		clear	- if TRUE, current flags will be set to some defaults
 *			  before applying the options 
 *		    	- if FALSE, current flags will not be cleared
 *		mode	- terminal mode, CANON, RAW
 */
int
set_termio(fd,options,aspeed,clear,mode)
int	fd;
char	*options;
char	*aspeed;
int	clear;
long	mode;
{
	struct 	 termio termio;
	struct 	 termios termios;
	struct 	 stio stermio;
	struct 	 termiox termiox;
	struct 	 winsize winsize;
	struct 	 winsize owinsize;
	int	 term;
	int	 cnt = 1;
	char	 *uarg;	
	char	 *argvp[MAXARGS];	/* stty args */
	static   char	 *binstty = "/usr/bin/stty";
	static	 char	buf[BUFSIZ];
	extern 	 int get_ttymode(), set_ttymode();
	extern	 char	*sttyparse();

#ifdef	DEBUG
	debug("in set_termio");
#endif

	if ((term = get_ttymode(fd, &termio, &termios, &stermio, 
				&termiox, &winsize)) < 0) {
		(void)sprintf(Scratch,
			"set_termio: get_ttymode failed, errno = %d", errno);
		log(Scratch);
		return(-1);
	}
	owinsize = winsize;
	if (clear) {
		termios.c_iflag = 0;
		termios.c_cflag = 0;
		termios.c_lflag = 0;
		termios.c_oflag = 0;
		termios.c_iflag |= (IGNPAR|ISTRIP|ICRNL|IXON); 
		termios.c_cflag |= CS7|CREAD|PARENB|(B9600&CBAUD);
		if (mode & CANON) {
			termios.c_lflag |= (ISIG|ICANON|ECHO|ECHOE|ECHOK); 
			termios.c_cc[VEOF] = CEOF;
			termios.c_cc[VEOL] = CNUL;
		}
		termios.c_oflag |= OPOST|ONLCR;
	}



	if (options != NULL && *options != '\0') {
		/* just a place holder to make it look like invoking stty */
		argvp[0] = binstty;
		(void)strcpy(buf,options);
		mkargv(buf,&argvp[1],&cnt,MAXARGS-1);
		if ((aspeed != NULL) && (*aspeed != '\0')) {
			argvp[cnt++] = aspeed;
		}
		argvp[cnt] = (char *)0;
		if ((uarg = sttyparse(cnt, argvp, term, &termio, &termios, 
				&termiox, &winsize)) != NULL) {
			(void)sprintf(Scratch, "sttyparse unknown mode: %s",
				uarg);
			log(Scratch);
			return(-1);
		}
	}
	if (clear && (! (mode & CANON))) {
		termios.c_lflag &= ~ECHO;
		termios.c_cc[VMIN] = 1;
		termios.c_cc[VTIME] = 0;
	}

	if (set_ttymode(fd, term, &termio, &termios, &stermio, 
			&termiox, &winsize, &owinsize) != 0) {
		(void)sprintf(Scratch,
			"set_termio: set_ttymode failed, errno = %d", errno);
		log(Scratch);
		return(-1);
	}
	return(0);
}

/*
 *	turnon_canon	- turn on canonical processing
 *			- return 0 if succeeds, -1 if fails
 */
turnon_canon(fd)
int	fd;
{
	struct termio termio;

#ifdef	DEBUG
	debug("in turnon_canon");
#endif
	if (ioctl(fd, TCGETA, &termio) != 0) {
		(void)sprintf(Scratch,
		"turnon_canon: TCGETA failed, fd = %d, errno = %d", fd, errno);
		log(Scratch);
		return(-1);
	}
	termio.c_lflag |= (ISIG|ICANON|ECHO|ECHOE|ECHOK); 
	termio.c_cc[VEOF] = CEOF;
	termio.c_cc[VEOL] = CNUL;
	if (ioctl(fd, TCSETA, &termio) != 0) {
		(void)sprintf(Scratch,
		"turnon_canon: TCSETA failed, fd = %d, errno = %d", fd, errno);
		log(Scratch);
		return(-1);
	}
	return(0);
}

/*
 *	flush_input	- flush the input queue
 */
void
flush_input(fd)
int	fd;
{
	if (ioctl(fd, I_FLUSH, FLUSHR) == -1) {
		(void)sprintf(Scratch,"flush_input failed, fd = %d, errno = %d",
			fd, errno);
		log(Scratch);
	}
	return;
}

/*
 * push_linedisc	- if modules is not NULL, pop everything
 *			- then push modules specified by "modules"
 */

push_linedisc(fd,modules,device)
int	fd;		/* fd to push modules on			 */
char	*modules;	/* ptr to a list of comma separated module names */
char	*device;	/* device name for printing msg			 */
{
	char	*p, *tp;
	char	buf[BUFSIZ];

#ifdef	DEBUG
	debug("in push_linedisc");
#endif
	/*
	 * copy modules into buf so we won't mess up the original buffer
	 * because strtok will chop the string
	 */
	p = strcpy(buf,modules);

	while(ioctl(fd, I_POP) >= 0)  /* pop everything */ 
		;
	for (p=(char *)strtok(p,","); p!=(char *)NULL; 
		p=(char *)strtok(NULL,",")) {
		for (tp = p + strlen(p) - 1; tp >= p && isspace(*tp); --tp)
			*tp = '\0';
		if (ioctl(fd, I_PUSH, p) == -1) {
			(void)sprintf(Scratch,
			"push (%s) on %s failed, errno = %d",
			p, device, errno);
			log(Scratch);
			return(-1);
		}  
	}
	return(0);
}

/*
 *	hang_up_line	- set speed to B0. This will drop DTR
 */
hang_up_line(fd)
int	fd;
{
	struct termio termio;

#ifdef	DEBUG
	debug("in hang_up_line");
#endif

	if (ioctl(fd,TCGETA,&termio) < 0) {
		(void)sprintf(Scratch,
			"hang_up_line: TCGETA failed, errno = %d", errno);
		log(Scratch);
		return(-1);
	}
	termio.c_cflag &= ~CBAUD;
	termio.c_cflag |= B0;

	if (ioctl(fd,TCSETA,&termio) < 0) {
		(void)sprintf(Scratch,
			"hang_up_line: TCSETA failed, errno = %d", errno);
		log(Scratch);
		return(-1);
	}
	return(0);
}

/*
 * initial_termio	- set initial termios
 *			- return 0 if successful, -1 if failed.
 */
int
initial_termio(fd,pmptr)
int	fd;
struct	pmtab	*pmptr;
{
	int	ret;
	struct	Gdef *speedef;
	struct	Gdef *get_speed();
	extern	int  auto_termio();

	speedef = get_speed(pmptr->p_ttylabel);
	if (speedef->g_autobaud & A_FLAG) {
		pmptr->p_ttyflags |= A_FLAG;
		if (auto_termio(fd, 0) == -1) {
			(void)close(fd);
			return(-1);
		}
	}
	else {
		ret = set_termio(fd,speedef->g_iflags,
			(char *)NULL, TRUE, (long)RAW);	
		if (ret == -1) {
			(void)sprintf(Scratch,"initial termio on (%s) failed",
				pmptr->p_device);
			log(Scratch);
			(void)close(fd);
			return(-1);
		}
	}
	return(0);
}

#include <dial.h>

void sigcatch(signo)
int signo;
{
	int notmuch;
}

/* Used to initialize the modem using the connection server and a new class
 * called Reset. Edit the Devices file and add eg. Reset tty00,M 9600 mine
 * where mine is a chat label in the Dialers file. Use the -o flag to ttyadm
 */

void
initial_chat(tty)
char *tty;
{
	CALL call;              /* call structure for dial() */
	CALL_EXT call_ext;      /* extension structure for dial() */
	struct termio _Lv;       /* attributes for the line to remote */
	struct  sigaction       sigact;
	int Cn;
	extern fd_rmcslock();
	sigset_t	cset;
	sigset_t	tset;
	pid_t		pid;
	int		count;

        /* set up some default settings for dial */
        _Lv.c_iflag = (IGNPAR | IGNBRK | IXON | IXOFF);
        _Lv.c_oflag = 0;
        _Lv.c_cflag = (CS8 | CREAD | HUPCL);
        _Lv.c_lflag = 0;
        _Lv.c_line = 0;
        _Lv.c_cc[VINTR]  = CINTR;
        _Lv.c_cc[VQUIT]  = CQUIT;
        _Lv.c_cc[VERASE] = CERASE;
        _Lv.c_cc[VKILL]  = CKILL;
        _Lv.c_cc[VMIN]   = 1;
        _Lv.c_cc[VEOL]   = CEOL;
        _Lv.c_cc[VSWTCH] = CNSWTCH;

        /* turn off input mappings */
        _Lv.c_iflag &= ~(INLCR | IGNCR | ICRNL | IUCLC);

        /* turn off output post-processing */
        _Lv.c_oflag &= ~OPOST;

        /* turn off cannonical processing */
        _Lv.c_lflag &= ~(ISIG | ICANON | ECHO);

        /* set VMIN */
        _Lv.c_cc[VMIN] = 1;

        call.attr = &_Lv;
        call.baud = -1;         /* unused */
        call.speed = -1;        /* -1 means Any speed. Use Devices file value */
        call.line = tty;       /* line name if direct */
        call.telno = NULL;      /* telno or system name */
        call.modem = -1;        /* unused */
        call.device = (char *) &call_ext;
        call.dev_len = -1;      /* unused */

        call_ext.service = NULL;
        call_ext.class = "Reset";
        call_ext.protocol = NULL;
        call_ext.reserved1 = NULL;

	sigact.sa_flags = SA_RESETHAND | SA_NODEFER;
	sigact.sa_handler = sigcatch;
	(void)sigemptyset(&sigact.sa_mask);
	(void)sigaction(SIGALRM, &sigact, NULL);
        alarm(60);

	for (count=0; count < DIAL_RETRY; count++) {

		/* protect the dial call from a sigpoll */
		(void)sigprocmask(SIG_SETMASK, NULL, &cset);
		tset = cset;
		(void)sigaddset(&tset, SIGPOLL);
		(void)sigprocmask(SIG_SETMASK, &tset, NULL);
	
		Cn = dial(call);
	
		(void)sigprocmask(SIG_SETMASK, &cset, NULL);
	
		alarm(0);
		if(Cn < 0) {
/****
			(void)sprintf(Scratch, "Reset dial failed = %d", Cn);
			log(Scratch);
****/
			/* 
		 	* On failure, cs often leaves a lock file around. if
		 	* it has it can be disatorous for us since this ttymon
		 	* process will be around for a while and attempt to 	
		 	* dial again will fail showing that the PID is still active			 * so, we get the PID in the lock file, if it is us, 
		 	* we remove it. ul94-00908
		 	*/
			if((pid = fn_getpid(tty)) != -1) {
/*****
				(void)sprintf(Scratch, "pid %d still has line locked", pid);
				log(Scratch);
****/
				if(pid == getpid())  {
					(void)sprintf(Scratch, "pid is our PID. removing the lock");
					log(Scratch);
					fn_rmlock(tty);
				}
			}
		} else {
			fd_rmcslock(Cn); /* nasty but harder to put in libnsl undial */
			undial(Cn);
			return;
		}
	}
}
