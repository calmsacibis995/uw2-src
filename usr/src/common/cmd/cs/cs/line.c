/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cs:cs/line.c	1.2.2.7"
#ident	"$Header: $"

/* This is a new line.c, which consists of line.c and culine.c
 * merged together.
 */

#include <unistd.h>
#include "uucp.h"
#include <signal.h>
#define SIGRTN  void


static struct sg_spds {
	int	sp_val,
		sp_name;
} spds[] = {
	{  50,   B50},
	{  75,   B75},
	{ 110,  B110},
	{ 134,  B134},
	{ 150,  B150},
	{ 200,  B200},
	{ 300,  B300},
	{ 600,  B600},
	{1200, B1200},
	{1800, B1800},
	{2400, B2400},
	{4800, B4800},
	{9600, B9600},
#ifdef EXTA
	{19200,	EXTA},
#endif
#ifdef B19200
	{19200,	B19200},
#endif
#ifdef B38400
	{38400,	B38400},
#endif
	{0,    0}
};

#define PACKSIZE	64
#define HEADERSIZE	6

int
     packsize = PACKSIZE,
    xpacksize = PACKSIZE;

#define SNDFILE	'S'
#define RCVFILE 'R'
#define RESET	'X'

static int Saved_line;		/* was savline() successful?	*/
int
	Oddflag = 0,	/* Default is no parity */
	Evenflag = 0,	/* Default is no parity */
	Duplex = 1,	/* Default is full duplex */
	Terminal = 0,	/* Default is no terminal */
	term_8bit = -1,	/* Default to terminal setting or 8 bit */
	line_8bit = -1;	/* Default is same as terminal */

static struct termio Savettyb;

/*
 *      ioctltr()  -  catch alarm routine for TCSETAW ioctls, since they can
 *                    wait forever behind data
 */
/*ARGSUSED*/
GLOBAL void
ioctltr(sig)
int sig;
{
        CDEBUG(6, gettxt(":340", "timed out\n%s"), "");
}


/*
 * set speed/echo/mode...
 *	tty 	-> terminal name
 *	spwant 	-> speed
 *	type	-> type
 *
 *	if spwant == 0, speed is untouched
 *	type is unused, but needed for compatibility
 *
 * return:  
 *	none
 */

/*ARGSUSED*/
void
fixline(tty, spwant, type)
int	tty, spwant, type;
{
	register struct sg_spds	*ps;
	struct termio		ttbuf;
	int			speed = -1;
	int 	ret;
	SIGRTN	(*SigWas)();    /* Caller's alarm handler */


	DEBUG(6, "fixline(%d, ", tty);
	DEBUG(6, "%d)\n", spwant);
	if ((*Ioctl)(tty, TCGETA, &ttbuf) != 0)
		return;
	if (spwant > 0) {
		for (ps = spds; ps->sp_val; ps++)
			if (ps->sp_val == spwant) {
				speed = ps->sp_name;
				break;
			}
		if ( speed < 0 )
		    DEBUG(5, gettxt(":96", "speed (%d) not supported\n"), spwant);
		ASSERT(speed >= 0, "BAD SPEED", "", spwant);
		ttbuf.c_cflag = (unsigned short) speed;
	} else { /* determine the current speed setting */
		ttbuf.c_cflag &= CBAUD;
		for (ps = spds; ps->sp_val; ps++)
			if (ps->sp_name == ttbuf.c_cflag) {
				spwant = ps->sp_val;
				break;
			}

	}
	ttbuf.c_iflag = ttbuf.c_oflag = ttbuf.c_lflag = (ushort)0;
	ttbuf.c_cflag &= ~CLOCAL;

	/* During the chat scripts setup the line as for "cu" */
	/* After the chat scrupts are done, and the if the	*/
	/* service is uucico, setup the line as for "uucico"	*/

	if ( type != D_TRANSFER) {

		/* set attributes associated with -h, -t, -e, and -o options */

		ttbuf.c_iflag = (IGNPAR | IGNBRK | IXON | IXOFF);
		if ( line_8bit ) {
		    ttbuf.c_cflag |= CS8;
		    ttbuf.c_iflag &= ~ISTRIP;
		} else {
		    ttbuf.c_cflag |= CS7;
		    ttbuf.c_iflag |= ISTRIP;
		}

		ttbuf.c_cc[VEOF] = '\1';
		ttbuf.c_cflag |= ( CREAD | (speed ? HUPCL : 0));
	
		if (Evenflag) {				/*even parity -e */
		    if(ttbuf.c_cflag & PARENB) {
			VERBOSE(gettxt(":97", "Parity option error\n"), 0);
			exit(1);
		    } else 
			ttbuf.c_cflag |= PARENB;
		} else if(Oddflag) {			/*odd parity -o */
		    if(ttbuf.c_cflag & PARENB) {
			VERBOSE(gettxt(":97", "Parity option error\n"), 0);
			exit(1);
		    } else {
			ttbuf.c_cflag |= PARODD;
			ttbuf.c_cflag |= PARENB;
		    }
		}

		if(!Duplex)				/*half duplex -h */
		    ttbuf.c_iflag &= ~(IXON | IXOFF);
		if(Terminal)				/* -t */
		    ttbuf.c_oflag |= (OPOST | ONLCR);

	} else { 
		ttbuf.c_cflag |= (CS8 | CREAD | (speed ? HUPCL : 0));
		ttbuf.c_cc[VMIN] = HEADERSIZE;
		ttbuf.c_cc[VTIME] = 1;
	}

DEBUG(5, "i_flag=%o ", ttbuf.c_iflag);
DEBUG(5, "o_flag=%o ", ttbuf.c_oflag);
DEBUG(5, "c_flag=%o ", ttbuf.c_cflag);
DEBUG(5, "l_flag=%o\n ", ttbuf.c_lflag);
	
	SigWas = signal(SIGALRM, ioctltr);
	alarm(10);
	(*Ioctl)(tty, TCSETAW, &ttbuf);
	alarm(0);
	signal(SIGALRM, SigWas);
	return;
}

void
sethup(dcf)
int	dcf;
{
	struct termio ttbuf;
	SIGRTN	(*SigWas)();    /* Caller's alarm handler */

	if ((*Ioctl)(dcf, TCGETA, &ttbuf) != 0)
		return;
	if (!(ttbuf.c_cflag & HUPCL)) {
		ttbuf.c_cflag |= HUPCL;
		SigWas = signal(SIGALRM, ioctltr);
		alarm(10);
		(void) (*Ioctl)(dcf, TCSETAW, &ttbuf);
		alarm(0);
		signal(SIGALRM, SigWas);
	}
	return;
}

void
ttygenbrk(fn)
register int	fn;
{
	if (isatty(fn)) 
		(void) (*Ioctl)(fn, TCSBRK, 0);
	return;
}


/*
 * optimize line setting for sending or receiving files
 * return:
 *	none
 */
void
setline(type)
register char	type;
{
	static struct termio tbuf;
	int vtime;
	SIGRTN	(*SigWas)();    /* Caller's alarm handler */
	
	DEBUG(2, "setline(%c)\n", type);
	if ((*Ioctl)(Ifn, TCGETA, &tbuf) != 0)
		return;
	switch (type) {
	case RCVFILE:
		switch (tbuf.c_cflag & CBAUD) {
		case B9600:
			vtime = 1;
			break;
		case B4800:
			vtime = 4;
			break;
		default:
			vtime = 8;
			break;
		}
		if (tbuf.c_cc[VMIN] != packsize || tbuf.c_cc[VTIME] != vtime) {
		    tbuf.c_cc[VMIN] = packsize;
		    tbuf.c_cc[VTIME] = vtime;
		    SigWas = signal(SIGALRM, ioctltr);
		    alarm(10);
		    if ( (*Ioctl)(Ifn, TCSETAW, &tbuf) != 0 )
			DEBUG(4, gettxt(":323", "setline: Ioctl failed, errno=%d\n"), errno);
		    alarm(0);
		    signal(SIGALRM, SigWas);
		}
		break;

	case SNDFILE:
	case RESET:
		if (tbuf.c_cc[VMIN] != HEADERSIZE) {
		    tbuf.c_cc[VMIN] = HEADERSIZE;
		    SigWas = signal(SIGALRM, ioctltr);
		    alarm(10);
		    if ( (*Ioctl)(Ifn, TCSETAW, &tbuf) != 0 )
			DEBUG(4, gettxt(":323", "setline: Ioctl failed, errno=%d\n"), errno);
		    alarm(0);
		    signal(SIGALRM, SigWas);
		}
		break;
	}
	return;
}

int
savline()
{
	if ( (*Ioctl)(0, TCGETA, &Savettyb) != 0 )
		Saved_line = FALSE;
	else {
		Saved_line = TRUE;
		Savettyb.c_cflag = (Savettyb.c_cflag & ~CS8) | CS7;
		Savettyb.c_oflag |= OPOST;
		Savettyb.c_lflag |= (ISIG|ICANON|ECHO);
	}
	return(0);
}

#ifdef SYTEK

/*
 *	sytfixline(tty, spwant)	set speed/echo/mode...
 *	int tty, spwant;
 *
 *	return codes:  none
 */

void
sytfixline(tty, spwant)
int tty, spwant;
{
	struct termio ttbuf;
	struct sg_spds *ps;
	int speed = -1;
	SIGRTN	(*SigWas)();    /* Caller's alarm handler */

	if ( (*Ioctl)(tty, TCGETA, &ttbuf) != 0 )
		return;
	for (ps = spds; ps->sp_val >= 0; ps++)
		if (ps->sp_val == spwant)
			speed = ps->sp_name;
	DEBUG(4, gettxt(":324", "sytfixline: baud speed = %d\n"), speed);
	ASSERT(speed >= 0, "BAD SPEED", "", spwant);
	ttbuf.c_iflag = (ushort)0;
	ttbuf.c_oflag = (ushort)0;
	ttbuf.c_lflag = (ushort)0;
	ttbuf.c_cflag = speed;
	ttbuf.c_cflag |= (CS8|CLOCAL);
	ttbuf.c_cc[VMIN] = 6;
	ttbuf.c_cc[VTIME] = 1;
	SigWas = signal(SIGALRM, ioctltr);
	alarm(10);
	(*Ioctl)(tty, TCSETAW, &ttbuf);
	alarm(0);
	signal(SIGALRM, SigWas);
	return;
}

void
sytfix2line(tty)
int tty;
{
	struct termio ttbuf;
	SIGRTN	(*SigWas)();    /* Caller's alarm handler */

	if ( (*Ioctl)(tty, TCGETA, &ttbuf) != 0 )
		return;
	ttbuf.c_cflag &= ~CLOCAL;
	ttbuf.c_cflag |= CREAD|HUPCL;
	SigWas = signal(SIGALRM, ioctltr);
	alarm(10);
	(*Ioctl)(tty, TCSETAW, &ttbuf);
	alarm(0);
	signal(SIGALRM, SigWas);
	return;
}

#endif /* SYTEK */

int
restline()
{
	int ret;
	SIGRTN	(*SigWas)();    /* Caller's alarm handler */
	if ( Saved_line == TRUE ) {
		SigWas = signal(SIGALRM, ioctltr);
		alarm(10);
		ret = (*Ioctl)(0, TCSETAW, &Savettyb);
		alarm(0);
		signal(SIGALRM, SigWas);
		return(ret);
	}
	return(0);
}

