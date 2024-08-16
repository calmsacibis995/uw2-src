/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoauthent:newvt.c	1.3"
/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>
#include <cs.h>
#include "sys/types.h"
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <ctype.h>
#include "sys/at_ansi.h"
#include "sys/kd.h"
#include "sys/vt.h"
#include <sys/termio.h>
#include <sys/termios.h>
#include <sys/stermio.h>
#include <sys/termiox.h>
#include <sys/ioctl.h>
#include "errno.h"

extern int errno;
int get_ttymode( struct termio *termio, 
                 struct termios *termios,
                 struct stio *stermio);
void set_ttymode();
int do_setttymode( struct termio *termio,
               struct termios *termios,
               struct stio *stermio);

void newvt(char *cmdString);

void
newvt(char *cmdString)
{

	int status,newpid;

	int	fd, fdvt, wantvt = -1;
	long	vtno;
	char	*comm, *bname, prompt[11];
	char	name[VTNAMESZ], vtpref[VTNAMESZ], vtname[VTNAMESZ];
	ushort	ttype;
	struct vt_stat	vtinfo;

	comm = cmdString;
	errno = 0;
	if ((fd = open("/dev/vt00", O_RDWR)) == -1) {
		exit(1);
	}
	fdvt = -1;
	while ((ttype = ioctl(fd, KIOCINFO, 0)) == (ushort)-1) {
		if (ioctl(fd, TIOCVTNAME, name) == -1)
			break;
		sprintf(vtname, "/dev/%s", name);
		if ((fdvt = open(vtname, O_RDONLY)) == -1)
			break;
	}
	if (ttype == (ushort)-1) {
		exit(1);
	}
	if (ttype == 0x6b64) /* "kd" */
		strcpy(vtpref, "/dev/");
	else if ((((int)ttype & 0xff00) >> 8) == 0x73) /* SunRiver */
		sprintf(vtpref, "/dev/s%d", (ttype & 0xff));
	if (wantvt < 0) {
		ioctl(fd, VT_OPENQRY, &vtno);
		if (vtno < 0) {
			exit(1);
		}
	} else {
		vtno = wantvt;
		ioctl(fd, VT_GETSTATE, &vtinfo);
		if (vtinfo.v_state & (1 << vtno)) {
			exit(1);
		}
	}
	sprintf(vtname, "%svt%02d", vtpref, vtno);
	close(fd);
	close(2);
	close(1);
	close(0);
	newpid = fork  ();
   	if (newpid == 0) 
	{
		/* child does the work */
	putenv("TERM=AT386");
	setpgrp();   
	if (open(vtname, O_RDWR) == -1)
		exit(1);
	dup(0);
	dup(0);
	set_ttymode();
	sprintf(prompt,"PS1=VT %d> ", vtno);
	putenv(prompt);
	fputs("\033c", stdout);
	signal(SIGINT, SIG_DFL);
	signal(SIGQUIT, SIG_DFL);
	if (ioctl(0, VT_ACTIVATE, vtno) == -1)
        {
     		fprintf(stderr, "!VT_ACTIVATE");
        }
        else
        {
     		if (ioctl(0, VT_WAITACTIVE, 0) == -1)
            		fprintf(stderr, "!VT_WAITACTIVE");
        }

	if (comm != (char *)NULL) {
		system(comm);
	} else if ((comm = (char *)getenv("SHELL")) != (char *)NULL) {
		if ((bname = strrchr(comm, '/')) == (char *) NULL)
			bname = comm;
		else	/* skip past '/' */
			bname++;
		if (execl(comm, bname, 0) == -1)
			fprintf(stderr, "!exec: %s \n", comm);
	} else if (execl("/bin/sh", "sh", 0) == -1)
		fprintf(stderr, "!exec: /bin/sh \n");
	sleep(5);
	}
	else 
	{
		/* parent waits */
		if (newpid == -1) 
		{
           	exit (0);
        } 
		else 
		{
           	if (wait (&status) == -1) 
			{
               	exit (0);
           	}
		}
	}
	exit(1);
}

int term = 0;
#define	ASYNC	1
#define	TERMIOS	2

void
set_ttymode()
{
	struct termio termio;
	struct termios termios;
	struct stio stermio;

	get_ttymode(&termio, &termios, &stermio);
	do_setttymode(&termio, &termios, &stermio);
}

int
get_ttymode( struct termio *termio, struct termios *termios,
             struct stio *stermio)
{
	int i, fd = 0;

	if(ioctl(fd, STGET, stermio) == -1) {
		term |= ASYNC;
		if(ioctl(fd, TCGETS, termios) == -1) {
			if(ioctl(fd, TCGETA, termio) == -1) {
				return -1;
			}
			termios->c_lflag = termio->c_lflag;
			termios->c_oflag = termio->c_oflag;
			termios->c_iflag = termio->c_iflag;
			termios->c_cflag = termio->c_cflag;
			for(i = 0; i < NCC; i++)
				termios->c_cc[i] = termio->c_cc[i];
		} else
			term |= TERMIOS;
	}
	else {
		termios->c_cc[7] = (unsigned)stermio->tab;
		termios->c_lflag = stermio->lmode;
		termios->c_oflag = stermio->omode;
		termios->c_iflag = stermio->imode;
	}

	termios->c_cc[VERASE] = '\b';
	termios->c_cc[VINTR] = CINTR;
	termios->c_cc[VMIN] = 1;
	termios->c_cc[VTIME] = 1;
	termios->c_cc[VEOF] = CEOF;
	termios->c_cc[VEOL] = CNUL;
	termios->c_cc[VKILL] = CKILL;
	termios->c_cc[VQUIT] = CQUIT;
	
	termios->c_cflag &= ~(CSIZE|PARODD|CLOCAL);
	termios->c_cflag |= (CS7|PARENB|CREAD);
	
	termios->c_iflag &= ~(IGNBRK|PARMRK|INPCK|INLCR|IGNCR|IUCLC|IXOFF);
	termios->c_iflag |= (BRKINT|IGNPAR|ISTRIP|ICRNL|IXON|IXANY);

	termios->c_lflag &= ~(XCASE|ECHONL|NOFLSH|STFLUSH|STWRAP|STAPPL);
	termios->c_lflag |= (ISIG|ICANON|ECHO|ECHOE|ECHOK);
	
	termios->c_oflag &= ~(OLCUC|OCRNL|ONOCR|ONLRET|OFILL|OFDEL|
				NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY);
	termios->c_oflag |= (TAB3|OPOST|ONLCR);
	return term;
}

int
do_setttymode( struct termio *termio,
               struct termios *termios,
               struct stio *stermio)
{
	int i, fd = 0;

	if (term & ASYNC) {
		if(term & TERMIOS) {
			if(ioctl(fd, TCSETSW, termios) == -1) {
				return -1;
			}
		} else {
			termio->c_lflag = termios->c_lflag;
			termio->c_oflag = termios->c_oflag;
			termio->c_iflag = termios->c_iflag;
			termio->c_cflag = termios->c_cflag;
			for(i = 0; i < NCC; i++)
				termio->c_cc[i] = termios->c_cc[i];
			if(ioctl(fd, TCSETAW, termio) == -1) {
				return -1;
			}
		}
			
	} else {
		stermio->imode = termios->c_iflag;
		stermio->omode = termios->c_oflag;
		stermio->lmode = termios->c_lflag;
		stermio->tab = termios->c_cc[7];
		if (ioctl(fd, STSET, stermio) == -1) {
			return -1;
		}
	}
	return 0;
}

