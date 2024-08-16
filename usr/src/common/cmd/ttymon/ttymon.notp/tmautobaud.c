/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:common/cmd/ttymon/ttymon.notp/tmautobaud.c	1.2"

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <termio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stropts.h>

#define	NTRY	5

/*
 * At this time, we only recognize certain speeds.
 * This table can be expanded if new patterns are found
 */
static struct	autobaud {
	char	*a_speed;
	char	*a_pattern;	/* first byte is length */
} autob9600[] = {
	"1200",		"\1\0",	/* also 2400 */
	"1200",		"\2\200\200",
	"2400",		"\2\0\374",
	"4800",		"\2\346\200",
	"9600",		"\1\15",
	"19200",	"\1\362",
	"19200",	"\1\371",
	"19200",	"\1\376",
	"38400",	"\1\377",
	0,		0
};

static struct autobaud autob2400[] = {
	"1200",		"\2\346\200",
	"2400",		"\1\15",
	0,		0
};

extern	char 	Scratch[];
extern	void	log();
extern	void	exit();
extern	unsigned alarm();
extern	int	read();
extern	int	ioctl();

/*
 *	auto_termio - set termio to allow autobaud
 *		    - the line is set to raw mode, with VMIN = 5, VTIME = 1
 *		    - baud rate is set to 9600 if flag is 0, else 2400
 */
auto_termio(fd, flag)
int	fd;
int	flag;
{
	struct termio termio;

	if (ioctl(fd, TCGETA, &termio) == -1) {
		(void)sprintf(Scratch,
			"auto_termio: ioctl TCGETA failed, fd = %d, errno = %d",
			fd, errno);
		log(Scratch);
		return(-1);
	}
	termio.c_iflag = 0;
	termio.c_cflag &= ~(CBAUD|CSIZE|PARENB); 
	if (flag)
		termio.c_cflag |= CREAD|HUPCL|(CS8&CSIZE)|(B2400&CBAUD);
	else
		termio.c_cflag |= CREAD|HUPCL|(CS8&CSIZE)|(B9600&CBAUD);
	termio.c_lflag &= ~(ISIG|ICANON|ECHO|ECHOE|ECHOK);
	termio.c_oflag = 0;
	
	termio.c_cc[VMIN] = 5;
	termio.c_cc[VTIME] = 1;

	if (ioctl(fd, TCSETAF, &termio) == -1) {
		(void)sprintf(Scratch,
			"auto_termio: ioctl TCSETAF failed, fd = %d, errno = %d",
			fd, errno);
		log(Scratch);
		return(-1);
	}
	return(0);
}

/*
 *	autobaud - determine the baudrate by reading data at 9600 baud rate
 *		 - the program is anticipating <CR> 
 *		 - the bit pattern is matched again an autobaud table
 *		 - if a match is found, the matched speed is returned
 *		 - otherwise, NULL is returned 
 */

char *
autobaud(fd,timeout, flag)
int	fd;
int	timeout;
int	flag;
{
	int i, k, count;
	static char	buf[5];
	register char *cp = buf;
	char *retp;
	struct	autobaud *tp;
	struct	sigaction sigact;
	extern	void	timedout();
	extern	void	flush_input();

#ifdef	DEBUG
	debug("in autobaud");
#endif
	sigact.sa_flags = 0;
	sigact.sa_handler = SIG_IGN;
	(void)sigemptyset(&sigact.sa_mask);
	(void)sigaction(SIGINT, &sigact, NULL);
	count = NTRY;
	while (count--) {
		if (timeout) {
			sigact.sa_flags = SA_RESETHAND | SA_NODEFER;
			sigact.sa_handler = timedout;
			(void)sigemptyset(&sigact.sa_mask);
			(void)sigaction(SIGALRM, &sigact, NULL);
			(void)alarm((unsigned)timeout);
		}
		cp = &buf[1];
		if ((k=read(fd, cp, 5)) < 0) {
			(void)sprintf(Scratch, "autobaud: read failed, errno = %d",
				errno);
			log(Scratch);
			exit(1);
		}
		if (timeout)
			(void)alarm((unsigned)0);
		buf[0] = (char)k;
		if (flag)
			tp = autob2400;
		else
			tp = autob9600;
		for (; tp->a_speed; tp++) {
			for (i = 0;; i++) {
				if (buf[i] != tp->a_pattern[i])
					break;
				if (i == buf[0]) {
					/* special case of ambiguity */
					if (tp == autob9600) {
						if (auto_termio(fd, 1) == -1) {
							flush_input(fd);
							return(NULL);
						}
						retp = autobaud(fd, timeout, 1);
						if (retp == NULL) {
							flush_input(fd);
							return(NULL);
						} else
							return(retp);
					}
					return(tp->a_speed);
				}
			}
		}
		flush_input(fd);
	} /* end while */
	return(NULL);		/* autobaud failed */
}