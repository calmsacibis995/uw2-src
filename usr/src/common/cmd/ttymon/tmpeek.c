/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:common/cmd/ttymon/tmpeek.c	1.8.12.2"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stropts.h>
#include <sys/termio.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>
#include <pfmt.h>
#include <priv.h>
#include "ttymon.h"
#include <sys/termios.h>
#include <sys/stream.h>
#include <sys/tp.h>
#include "tmstruct.h"
#include "tmextern.h"

#define BRK	1
#define DEL	2
#define CONT	3

static	struct	strbuf *do_peek();
static	int	process();
static	void	sigint();

static	int	interrupt;
static	char	rbuf[BUFSIZ];	/* buffer for holding raw data	*/
static	char	*rp = rbuf;	/* pointer to rbuf		*/

extern const char badioctl[];

/*
 * Procedure:	  poll_data
 *
 * Restrictions:
                 write(2): none
								 ioctl(2): none
*/
/*
 *	poll_data	- it polls the device, waiting for data
 *			- return BADSPEED it <brk> is received
 *			- return the result of process if data is received
 *			- write a newline if <del> is received
 *			- exit if hangup is received
 */
poll_data(pfirst)
int	pfirst;	/* if TRUE, will do peek before poll to improve performance */
{
	int	 j, ret;
	register struct	strbuf *ptr;
	struct 	 pollfd fds[1];
	struct	 sigaction sigact;
	struct	 termio termio;
	int	 mode = CANON;

#ifdef	DEBUG
	debug("in poll_data");
#endif
	if (ioctl(0, TCGETA, &termio) == 0) {
		if ((termio.c_lflag & ICANON) == 0) {
			mode = RAW;
		}
	}
	if (pfirst) {
		ptr = do_peek(0, 255);
		if (ptr != NULL) {
			if ((ret = process(0,ptr,mode)) != CONT)
				return(ret);
		}
	}
	fds[0].fd = 0;
	fds[0].events = POLLIN;
	sigact.sa_flags = 0;
	sigact.sa_handler = sigint;
	(void)sigemptyset(&sigact.sa_mask);
	(void)sigaddset(&sigact.sa_mask, SIGINT);
	(void)sigaction(SIGINT, &sigact, NULL);
	for (;;) {
		interrupt = 0;
		if ((j = poll(fds,1,-1)) == -1) {
			if (interrupt == BRK) {
				rp = rbuf;
				return(BADSPEED);
			}
			if (interrupt == DEL) {
				rp = rbuf;
				(void)write(1,"\r\n",2);
			}
		}
		else if (j > 0) {
			if (fds[0].revents & POLLHUP) {
				log(MM_HALT, ":640:POLLHUP received, about to exit");
				exit(1);
			}
			if (fds[0].revents & POLLIN) {
				ptr = do_peek(0, 255);
				if (ptr != NULL) {
					if ((ret = process(0,ptr,mode)) != CONT)
						return(ret);
				}
			}
		}
	}
}

/*
 * Procedure:	  process
 *
 * Restrictions:
		 read(2): none
		 ioctl(2): none
*/

/*
 *	process	- process the data 
 *		- return GOODNAME if it is a non-empty line 
 *		- return NONAME if a <CR> is received
 *		- return BADSPEED if byte of zero is detected
 *		- return CONT if raw mode processing is on
 *		- except the case of GOODNAME, data will be pulled out
 *		  of the stream 
 */
static int
process(fd,ptr,mode)
int	fd;			/* fd to read data from if necessary 	*/
register struct strbuf *ptr;	/* ptr that holds data in ptr->buf 	*/
int	mode;			/* RAW or CANON				*/
{
	unsigned i, j;
	char	junk[BUFSIZ];
	struct	termio	termio;
	int	ret;
	char	*cp;
	int	lflag;

	for (i = 0; i < ptr->len; i++) {
		if (ptr->buf[i] == '\0') {
			(void)read(fd, junk, i+1);
			rp = rbuf;
			return(BADSPEED);
		}
		else if ((ptr->buf[i] == '\n') || (ptr->buf[i] == '\r')) {
			if (i == 0 && rp == rbuf) {
				(void)read(fd, junk, ptr->len);
				return(NONAME);
			}
			else {
				/*
				 * If tty is in RAW mode and
				 * some data has been read into rbuf,
				 * we need to put the data back on the stream. 
				 * ICANON is turn on before putting the data
				 * back because ldterm will flush data when
				 * changing from raw mode to canonical mode.
				 * For this to work, the driver has to support
				 * the TIOCSTI ioctl.
				 */
				if (rp != rbuf) {
					(void)read(fd, junk, ptr->len);
					if (ioctl(0, TCGETA, &termio) == 0) {
						termio.c_lflag |= ICANON;
						lflag = termio.c_lflag;
						termio.c_lflag &= ~ECHO;
						(void)ioctl(0, TCSETA, &termio);
					}
					cp = rbuf;
					for (cp = rbuf; cp < rp; ) {
					   (void)ioctl(fd, TIOCSTI, cp++);
					}
					cp = ptr->buf;
					for (j = 0; j < ptr->len; j++) {
						(void)ioctl(fd,TIOCSTI,cp++);
					}
					termio.c_lflag = lflag;
					(void)ioctl(0, TCSETA, &termio);
					rp = rbuf;
				}
				return(GOODNAME);
			}
		}
	}	/* end for loop */


        /*
         * we may arive here in RAW mode, mostly from no-flags. In this
         * case we will echo whatever chars are down ourselves (send each
         * down with TIOCSTI) after switching to CANON. Note: this code is
         * very time sensitive since we get a FLUSH on RAW->CANON transition.
         * Time tricks used here include, reading as many chars as possible
         * (not ptr->len chars) since more chars may have arrived in the
         * interum. also the GETA occurs before the read as does just
         * in case settings of params. We also do the ioctl to set CANON
         * ourselves instead of calling turnon_canon since turnon_canon
         * would do another GETA
         */

        if (ptr->len > 0 && mode == RAW) {
                (void)ioctl(0, TCGETA, &termio);
                lflag = termio.c_lflag;
                termio.c_lflag |= (ISIG|ICANON|ECHO|ECHOE|ECHOK);
                termio.c_cc[VEOF] = CEOF;
                termio.c_cc[VEOL] = CNUL;
                if ((ret = read(fd, junk, BUFSIZ)) > 0) {
                        if (!(lflag & ECHO)) {
                                (void)ioctl(fd, TCSETA, &termio);
                                for (i = 0; i < ret; i++) {
                                        (void)ioctl(fd,TIOCSTI,&junk[i]);
                                }
                        }
                        else {
                                for (i = 0; i < ret; i++) {
                                        *rp++ = junk[i];
                                }
                        }
                }
                return(CONT);
        }

	/* end of input is encountered */
#ifdef	DEBUG
	debug("in process: EOF encountered");
#endif
	exit(1);
	/*NOTREACHED*/
}

/*
 * Procedure:	  do_peek
 *
 * Restrictions:
                 ioctl(2): none
                 strerror: none
*/

/*
 *	do_peek	- peek at the stream to get the data
 *		- this only called when POLLIN is detected,
 *		- so there should always be something there 
 *		- return a ptr to the buf that contains the data
 *		- return NULL if nothing to peek at
 */
static	struct	strbuf	*
do_peek(fd,n)
int	fd;	/* fd to do the ioctl on */
int	n;	/* maxlen of data to peek at */
{
	int	 ret, tmperr;
	static 	 struct strpeek peek;
	register struct strpeek *peekp;
	static	 char	buf[BUFSIZ];

#ifdef	DEBUG
	debug("in do_peek");
#endif

	peekp = &peek;
	peekp->flags = 0;
	/* need to ask for ctl info to avoid bug in I_PEEK code */
	peekp->ctlbuf.maxlen = 1;
	peekp->ctlbuf.buf = buf;
	peekp->databuf.maxlen = n;
	peekp->databuf.buf = buf;
	ret = ioctl(fd, I_PEEK, &peek); tmperr=errno;
	if (ret == -1) {
		log(MM_HALT, badioctl, "do_peek", "I_PEEK", strerror(tmperr));
		exit(1);
	}
	if (ret == 0) {
		return( (struct strbuf *)NULL );
	}
	return(&(peekp->databuf));
}

/*
 * Procedure:	  sigint
 *
 * Restrictions:
		 ioctl(2): none
		 read(2): none
*/

/*
 *	sigint	- this is called when SIGINT is caught
 */
static	void
sigint()
{
	struct strbuf *ptr;
	char   junk[2];
	struct	termio	termio;

#ifdef	DEBUG
	debug("in sigint");
#endif
	if (ioctl(0, TCGETA, &termio) == 0) {
		/*
		 * If BRKINT is set, we cannot distinguish between
		 * Break and Del, so treat it as Break.
		 */
		if (termio.c_iflag & BRKINT) {
			interrupt = BRK;
			return;
		}
	}
	ptr = do_peek(0, 1);
	if (ptr == NULL) {	/* somebody type <del> */
		interrupt = DEL;
	}
	else {
		if (ptr->buf[0] == '\0') {
			/* somebody type <brk> or frame error */
			(void)read(0,junk,1);
			interrupt = BRK;
		}
	}
}

