/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.telnetd/sys_term.c	1.1.1.4"
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
 *      System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *      Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *      All Rights Reserved.
 *
 *      The copyright above and this notice must be preserved in all
 *      copies of this source code.  The copyright above does not
 *      evidence any actual or intended publication of this source
 *      code.
 *
 *      This is unpublished proprietary trade secret source code of
 *      Lachman Associates.  This source code may not be copied,
 *      disclosed, distributed, demonstrated or licensed except as
 *      expressly authorized by Lachman Associates.
 *
 *      System V STREAMS TCP was jointly developed by Lachman
 *      Associates and Convergent Technologies.
 */

/*      SCCS IDENTIFICATION        */

/*
 * Copyright (c) 1989 Regents of the University of California.
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
static char sccsid[] = "@(#)sys_term.c	5.15 (Berkeley) 3/5/91";
#endif /* not lint */

#include "telnetd.h"
#include "pathnames.h"
#include <sys/stream.h>
#include <sys/stropts.h>
#include <utmpx.h>

#include <locale.h>
#include <pfmt.h>
extern char *gettxt();

#if	defined(AUTHENTICATE)
#include <libtelnet/auth.h>
#endif

# define SC_WILDC       0xff    /* wild character for utmp ids */

#ifdef	NEWINIT
#include <initreq.h>
#else	/* NEWINIT*/
#include <utmp.h>
struct	utmp wtmp;

# ifndef CRAY
char	wtmpf[]	= "/etc/wtmp";
char	utmpf[] = "/etc/utmp";
# else	/* CRAY */
char	wtmpf[]	= "/etc/wtmp";
#include <tmpdir.h>
#include <sys/wait.h>
# endif	/* CRAY */
#endif	/* NEWINIT */

#define SCPYN(a, b)	(void) strncpy(a, b, sizeof(a))
#define SCMPN(a, b)	strncmp(a, b, sizeof(a))

#ifdef	STREAMS
#include <sys/stream.h>
#endif
#ifdef	t_erase
#undef	t_erase
#undef	t_kill
#undef	t_intrc
#undef	t_quitc
#undef	t_startc
#undef	t_stopc
#undef	t_eofc
#undef	t_brkc
#undef	t_suspc
#undef	t_dsuspc
#undef	t_rprntc
#undef	t_flushc
#undef	t_werasc
#undef	t_lnextc
#endif

#if defined(UNICOS5) && defined(CRAY2) && !defined(EXTPROC)
# define EXTPROC 0400
#endif

#ifndef	USE_TERMIO
struct termbuf {
	struct sgttyb sg;
	struct tchars tc;
	struct ltchars ltc;
	int state;
	int lflags;
} termbuf, termbuf2;
# define	cfsetospeed(tp, val)	(tp)->sg.sg_ospeed = (val)
# define	cfsetispeed(tp, val)	(tp)->sg.sg_ispeed = (val)
# define	cfgetospeed(tp)		(tp)->sg.sg_ospeed
# define	cfgetispeed(tp)		(tp)->sg.sg_ispeed
#else	/* USE_TERMIO */
# ifdef	SYSV_TERMIO
#	define termios termio
# endif
# ifndef	TCSANOW
#  ifdef TCSETS
#   define	TCSANOW		TCSETS
#   define	TCSADRAIN	TCSETSW
#   define	tcgetattr(f, t)	ioctl(f, TCGETS, (char *)t)
#  else
#   ifdef TCSETA
#    define	TCSANOW		TCSETA
#    define	TCSADRAIN	TCSETAW
#    define	tcgetattr(f, t)	ioctl(f, TCGETA, (char *)t)
#   else
#    define	TCSANOW		TIOCSETA
#    define	TCSADRAIN	TIOCSETAW
#    define	tcgetattr(f, t)	ioctl(f, TIOCGETA, (char *)t)
#   endif
#  endif
#  define	tcsetattr(f, a, t)	ioctl(f, a, t)
#  define	cfsetospeed(tp, val)	(tp)->c_cflag &= ~CBAUD; \
					(tp)->c_cflag |= (val)
#  define	cfgetospeed(tp)		((tp)->c_cflag & CBAUD)
#  ifdef CIBAUD
#   define	cfsetispeed(tp, val)	(tp)->c_cflag &= ~CIBAUD; \
					(tp)->c_cflag |= ((val)<<IBSHIFT)
#   define	cfgetispeed(tp)		(((tp)->c_cflag & CIBAUD)>>IBSHIFT)
#  else
#   define	cfsetispeed(tp, val)	(tp)->c_cflag &= ~CBAUD; \
					(tp)->c_cflag |= (val)
#   define	cfgetispeed(tp)		((tp)->c_cflag & CBAUD)
#  endif
# endif /* TCSANOW */
struct termios termbuf, termbuf2;	/* pty control structure */
#endif	/* USE_TERMIO */

#if defined(M_XENIX)
/* BEGIN SCO_BASE */
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/fcntl.h>
#include <sys/uadmin.h>
#include <sys/serial.h>
#include <sys/oemserial.h>
#endif
/* END SCO_BASE */

#define bcmp(a,b,n)	memcmp(a,b,n)
#define bzero(b,n)	memset(b, '\0', n)

int ttyp;

/*
 * init_termbuf()
 * copy_termbuf(cp)
 * set_termbuf()
 *
 * These three routines are used to get and set the "termbuf" structure
 * to and from the kernel.  init_termbuf() gets the current settings.
 * copy_termbuf() hands in a new "termbuf" to write to the kernel, and
 * set_termbuf() writes the structure into the kernel.
 */

	void
init_termbuf()
{
#ifndef	USE_TERMIO
	(void) ioctl(pty, TIOCGETP, (char *)&termbuf.sg);
	(void) ioctl(pty, TIOCGETC, (char *)&termbuf.tc);
	(void) ioctl(pty, TIOCGLTC, (char *)&termbuf.ltc);
# ifdef	TIOCGSTATE
	(void) ioctl(pty, TIOCGSTATE, (char *)&termbuf.state);
# endif
#else
	(void) tcgetattr(ttyp, &termbuf);
#endif
	termbuf2 = termbuf;
}

#if	defined(LINEMODE) && defined(TIOCPKT_IOCTL)
	void
copy_termbuf(cp, len)
	char *cp;
	int len;
{
	if (len > sizeof(termbuf))
		len = sizeof(termbuf);
	bcopy(cp, (char *)&termbuf, len);
	termbuf2 = termbuf;
}
#endif	/* defined(LINEMODE) && defined(TIOCPKT_IOCTL) */

	void
set_termbuf()
{
	/*
	 * Only make the necessary changes.
	 */
#ifndef	USE_TERMIO
	if (bcmp((char *)&termbuf.sg, (char *)&termbuf2.sg, sizeof(termbuf.sg)))
		(void) ioctl(pty, TIOCSETN, (char *)&termbuf.sg);
	if (bcmp((char *)&termbuf.tc, (char *)&termbuf2.tc, sizeof(termbuf.tc)))
		(void) ioctl(pty, TIOCSETC, (char *)&termbuf.tc);
	if (bcmp((char *)&termbuf.ltc, (char *)&termbuf2.ltc,
							sizeof(termbuf.ltc)))
		(void) ioctl(pty, TIOCSLTC, (char *)&termbuf.ltc);
	if (termbuf.lflags != termbuf2.lflags)
		(void) ioctl(pty, TIOCLSET, (char *)&termbuf.lflags);
#else	/* USE_TERMIO */
	if (bcmp((char *)&termbuf, (char *)&termbuf2, sizeof(termbuf))) {
		if (tcsetattr(ttyp, TCSANOW, &termbuf) < 0)
			syslog(LOG_INFO, gettxt(":495", "tcsetattr failed: %m"));
	}
# if	defined(CRAY2) && defined(UNICOS5)
	needtermstat = 1;
# endif
#endif	/* USE_TERMIO */
}


/*
 * spcset(func, valp, valpp)
 *
 * This function takes various special characters (func), and
 * sets *valp to the current value of that character, and
 * *valpp to point to where in the "termbuf" structure that
 * value is kept.
 *
 * It returns the SLC_ level of support for this function.
 */

#ifndef	USE_TERMIO
	int
spcset(func, valp, valpp)
	int func;
	cc_t *valp;
	cc_t **valpp;
{
	switch(func) {
	case SLC_EOF:
		*valp = termbuf.tc.t_eofc;
		*valpp = (cc_t *)&termbuf.tc.t_eofc;
		return(SLC_VARIABLE);
	case SLC_EC:
		*valp = termbuf.sg.sg_erase;
		*valpp = (cc_t *)&termbuf.sg.sg_erase;
		return(SLC_VARIABLE);
	case SLC_EL:
		*valp = termbuf.sg.sg_kill;
		*valpp = (cc_t *)&termbuf.sg.sg_kill;
		return(SLC_VARIABLE);
	case SLC_IP:
		*valp = termbuf.tc.t_intrc;
		*valpp = (cc_t *)&termbuf.tc.t_intrc;
		return(SLC_VARIABLE|SLC_FLUSHIN|SLC_FLUSHOUT);
	case SLC_ABORT:
		*valp = termbuf.tc.t_quitc;
		*valpp = (cc_t *)&termbuf.tc.t_quitc;
		return(SLC_VARIABLE|SLC_FLUSHIN|SLC_FLUSHOUT);
	case SLC_XON:
		*valp = termbuf.tc.t_startc;
		*valpp = (cc_t *)&termbuf.tc.t_startc;
		return(SLC_VARIABLE);
	case SLC_XOFF:
		*valp = termbuf.tc.t_stopc;
		*valpp = (cc_t *)&termbuf.tc.t_stopc;
		return(SLC_VARIABLE);
	case SLC_AO:
		*valp = termbuf.ltc.t_flushc;
		*valpp = (cc_t *)&termbuf.ltc.t_flushc;
		return(SLC_VARIABLE);
	case SLC_SUSP:
		*valp = termbuf.ltc.t_suspc;
		*valpp = (cc_t *)&termbuf.ltc.t_suspc;
		return(SLC_VARIABLE);
	case SLC_EW:
		*valp = termbuf.ltc.t_werasc;
		*valpp = (cc_t *)&termbuf.ltc.t_werasc;
		return(SLC_VARIABLE);
	case SLC_RP:
		*valp = termbuf.ltc.t_rprntc;
		*valpp = (cc_t *)&termbuf.ltc.t_rprntc;
		return(SLC_VARIABLE);
	case SLC_LNEXT:
		*valp = termbuf.ltc.t_lnextc;
		*valpp = (cc_t *)&termbuf.ltc.t_lnextc;
		return(SLC_VARIABLE);
	case SLC_FORW1:
		*valp = termbuf.tc.t_brkc;
		*valpp = (cc_t *)&termbuf.ltc.t_lnextc;
		return(SLC_VARIABLE);
	case SLC_BRK:
	case SLC_SYNCH:
	case SLC_AYT:
	case SLC_EOR:
		*valp = (cc_t)0;
		*valpp = (cc_t *)0;
		return(SLC_DEFAULT);
	default:
		*valp = (cc_t)0;
		*valpp = (cc_t *)0;
		return(SLC_NOSUPPORT);
	}
}

#else	/* USE_TERMIO */

	int
spcset(func, valp, valpp)
	int func;
	cc_t *valp;
	cc_t **valpp;
{

#define	setval(a, b)	*valp = termbuf.c_cc[a]; \
			*valpp = &termbuf.c_cc[a]; \
			return(b);
#define	defval(a) *valp = ((cc_t)a); *valpp = (cc_t *)0; return(SLC_DEFAULT);

	switch(func) {
	case SLC_EOF:
		setval(VEOF, SLC_VARIABLE);
	case SLC_EC:
		setval(VERASE, SLC_VARIABLE);
	case SLC_EL:
		setval(VKILL, SLC_VARIABLE);
	case SLC_IP:
		setval(VINTR, SLC_VARIABLE|SLC_FLUSHIN|SLC_FLUSHOUT);
	case SLC_ABORT:
		setval(VQUIT, SLC_VARIABLE|SLC_FLUSHIN|SLC_FLUSHOUT);
	case SLC_XON:
#ifdef	VSTART
		setval(VSTART, SLC_VARIABLE);
#else
		defval(0x13);
#endif
	case SLC_XOFF:
#ifdef	VSTOP
		setval(VSTOP, SLC_VARIABLE);
#else
		defval(0x11);
#endif
	case SLC_EW:
#ifdef	VWERASE
		setval(VWERASE, SLC_VARIABLE);
#else
		defval(0);
#endif
	case SLC_RP:
#ifdef	VREPRINT
		setval(VREPRINT, SLC_VARIABLE);
#else
		defval(0);
#endif
	case SLC_LNEXT:
#ifdef	VLNEXT
		setval(VLNEXT, SLC_VARIABLE);
#else
		defval(0);
#endif
	case SLC_AO:
#if	!defined(VDISCARD) && defined(VFLUSHO)
# define VDISCARD VFLUSHO
#endif
#ifdef	VDISCARD
		setval(VDISCARD, SLC_VARIABLE|SLC_FLUSHOUT);
#else
		defval(0);
#endif
	case SLC_SUSP:
#ifdef	VSUSP
		setval(VSUSP, SLC_VARIABLE|SLC_FLUSHIN);
#else
		defval(0);
#endif
#ifdef	VEOL
	case SLC_FORW1:
		setval(VEOL, SLC_VARIABLE);
#endif
#ifdef	VEOL2
	case SLC_FORW2:
		setval(VEOL2, SLC_VARIABLE);
#endif
	case SLC_AYT:
#ifdef	VSTATUS
		setval(VSTATUS, SLC_VARIABLE);
#else
		defval(0);
#endif

	case SLC_BRK:
	case SLC_SYNCH:
	case SLC_EOR:
		defval(0);

	default:
		*valp = 0;
		*valpp = 0;
		return(SLC_NOSUPPORT);
	}
}
#endif	/* USE_TERMIO */

#ifdef CRAY
/*
 * getnpty()
 *
 * Return the number of pty's configured into the system.
 */
	int
getnpty()
{
#ifdef _SC_CRAY_NPTY
	int numptys;

	if ((numptys = sysconf(_SC_CRAY_NPTY)) != -1)
		return numptys;
	else
#endif /* _SC_CRAY_NPTY */
		return 128;
}
#endif /* CRAY */

#ifndef	convex
/*
 * getpty()
 *
 * Allocate a pty.  As a side effect, the external character
 * array "line" contains the name of the slave side.
 *
 * Returns the file descriptor of the opened pty.
 */
#ifndef	__GNUC__
char *line = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
#else
static char Xline[] = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
char *line = Xline;
#endif
#ifdef	CRAY
char *myline = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
#endif	/* CRAY */

	int
getpty()
{
	register int p;
	extern char *ptsname();

	if ((p = open("/dev/ptmx", O_RDWR | O_NOCTTY)) == -1) {
		fatalperror (net,"open /dev/ptmx");
	}
	if (grantpt(p) == -1)
		fatal(net, gettxt(":496", "could not grant slave pty"));
	if (unlockpt(p) == -1)
		fatal(net, gettxt(":497", "could not unlock slave pty"));
	if ((line = ptsname(p)) == NULL)
		fatal (net, gettxt(":498", "could not enable slave pty"));

	return p;
}
#endif	/* convex */

#ifdef	LINEMODE
/*
 * tty_flowmode()	Find out if flow control is enabled or disabled.
 * tty_linemode()	Find out if linemode (external processing) is enabled.
 * tty_setlinemod(on)	Turn on/off linemode.
 * tty_isecho()		Find out if echoing is turned on.
 * tty_setecho(on)	Enable/disable character echoing.
 * tty_israw()		Find out if terminal is in RAW mode.
 * tty_binaryin(on)	Turn on/off BINARY on input.
 * tty_binaryout(on)	Turn on/off BINARY on output.
 * tty_isediting()	Find out if line editing is enabled.
 * tty_istrapsig()	Find out if signal trapping is enabled.
 * tty_setedit(on)	Turn on/off line editing.
 * tty_setsig(on)	Turn on/off signal trapping.
 * tty_issofttab()	Find out if tab expansion is enabled.
 * tty_setsofttab(on)	Turn on/off soft tab expansion.
 * tty_islitecho()	Find out if typed control chars are echoed literally
 * tty_setlitecho()	Turn on/off literal echo of control chars
 * tty_tspeed(val)	Set transmit speed to val.
 * tty_rspeed(val)	Set receive speed to val.
 */

	int
tty_flowmode()
{
#ifndef USE_TERMIO
	return(((termbuf.tc.t_startc) > 0 && (termbuf.tc.t_stopc) > 0) ? 1 : 0);
#else
	return(termbuf.c_iflag & IXON ? 1 : 0);
#endif
}

#ifdef convex
static int linestate;
#endif

	int
tty_linemode()
{
#ifndef convex
#ifndef	USE_TERMIO
	return(termbuf.state & TS_EXTPROC);
#else
	return(termbuf.c_lflag & EXTPROC);
#endif
#else
	return(linestate);
#endif
}

	void
tty_setlinemode(on)
	int on;
{
#ifdef	TIOCEXT
# ifndef convex
	set_termbuf();
# else
	linestate = on;
# endif
	(void) ioctl(pty, TIOCEXT, (char *)&on);
# ifndef convex
	init_termbuf();
# endif
#else	/* !TIOCEXT */
# ifdef	EXTPROC
	if (on)
		termbuf.c_lflag |= EXTPROC;
	else
		termbuf.c_lflag &= ~EXTPROC;
# endif
#endif	/* TIOCEXT */
}

	int
tty_isecho()
{
#ifndef USE_TERMIO
	return (termbuf.sg.sg_flags & ECHO);
#else
	return (termbuf.c_lflag & ECHO);
#endif
}
#endif	/* LINEMODE */

	void
tty_setecho(on)
	int on;
{
#ifndef	USE_TERMIO
	if (on)
		termbuf.sg.sg_flags |= ECHO|CRMOD;
	else
		termbuf.sg.sg_flags &= ~(ECHO|CRMOD);
#else
	if (on)
		termbuf.c_lflag |= ECHO;
	else
		termbuf.c_lflag &= ~ECHO;
#endif
}

#if	defined(LINEMODE) && defined(KLUDGELINEMODE)
	int
tty_israw()
{
#ifndef USE_TERMIO
	return(termbuf.sg.sg_flags & RAW);
#else
	return(!(termbuf.c_lflag & ICANON));
#endif
}
#endif	/* defined(LINEMODE) && defined(KLUDGELINEMODE) */

	void
tty_binaryin(on)
	int on;
{
#ifndef	USE_TERMIO
	if (on)
		termbuf.lflags |= LPASS8;
	else
		termbuf.lflags &= ~LPASS8;
#else
	if (on) {
		termbuf.c_iflag &= ~ISTRIP;
	} else {
		termbuf.c_iflag |= ISTRIP;
	}
#endif
}

	void
tty_binaryout(on)
	int on;
{
#ifndef	USE_TERMIO
	if (on)
		termbuf.lflags |= LLITOUT;
	else
		termbuf.lflags &= ~LLITOUT;
#else
	if (on) {
		termbuf.c_cflag &= ~(CSIZE|PARENB);
		termbuf.c_cflag |= CS8;
		termbuf.c_oflag &= ~OPOST;
	} else {
		termbuf.c_cflag &= ~CSIZE;
		termbuf.c_cflag |= CS7|PARENB;
		termbuf.c_oflag |= OPOST;
	}
#endif
}

	int
tty_isbinaryin()
{
#ifndef	USE_TERMIO
	return(termbuf.lflags & LPASS8);
#else
	return(!(termbuf.c_iflag & ISTRIP));
#endif
}

	int
tty_isbinaryout()
{
#ifndef	USE_TERMIO
	return(termbuf.lflags & LLITOUT);
#else
	return(!(termbuf.c_oflag&OPOST));
#endif
}

#ifdef	LINEMODE
	int
tty_isediting()
{
#ifndef USE_TERMIO
	return(!(termbuf.sg.sg_flags & (CBREAK|RAW)));
#else
	return(termbuf.c_lflag & ICANON);
#endif
}

	int
tty_istrapsig()
{
#ifndef USE_TERMIO
	return(!(termbuf.sg.sg_flags&RAW));
#else
	return(termbuf.c_lflag & ISIG);
#endif
}

	void
tty_setedit(on)
	int on;
{
#ifndef USE_TERMIO
	if (on)
		termbuf.sg.sg_flags &= ~CBREAK;
	else
		termbuf.sg.sg_flags |= CBREAK;
#else
	if (on)
		termbuf.c_lflag |= ICANON;
	else
		termbuf.c_lflag &= ~ICANON;
#endif
}

	void
tty_setsig(on)
	int on;
{
#ifndef	USE_TERMIO
	if (on)
		;
#else
	if (on)
		termbuf.c_lflag |= ISIG;
	else
		termbuf.c_lflag &= ~ISIG;
#endif
}
#endif	/* LINEMODE */

	int
tty_issofttab()
{
#ifndef	USE_TERMIO
	return (termbuf.sg.sg_flags & XTABS);
#else
# ifdef	OXTABS
	return (termbuf.c_oflag & OXTABS);
# endif
# ifdef	TABDLY
	return ((termbuf.c_oflag & TABDLY) == TAB3);
# endif
#endif
}

	void
tty_setsofttab(on)
	int on;
{
#ifndef	USE_TERMIO
	if (on)
		termbuf.sg.sg_flags |= XTABS;
	else
		termbuf.sg.sg_flags &= ~XTABS;
#else
	if (on) {
# ifdef	OXTABS
		termbuf.c_oflag |= OXTABS;
# endif
# ifdef	TABDLY
		termbuf.c_oflag &= ~TABDLY;
		termbuf.c_oflag |= TAB3;
# endif
	} else {
# ifdef	OXTABS
		termbuf.c_oflag &= ~OXTABS;
# endif
# ifdef	TABDLY
		termbuf.c_oflag &= ~TABDLY;
		termbuf.c_oflag |= TAB0;
# endif
	}
#endif
}

	int
tty_islitecho()
{
#ifndef	USE_TERMIO
	return (!(termbuf.lflags & LCTLECH));
#else
# ifdef	ECHOCTL
	return (!(termbuf.c_lflag & ECHOCTL));
# endif
# ifdef	TCTLECH
	return (!(termbuf.c_lflag & TCTLECH));
# endif
# if	!defined(ECHOCTL) && !defined(TCTLECH)
	return (0);	/* assumes ctl chars are echoed '^x' */
# endif
#endif
}

	void
tty_setlitecho(on)
	int on;
{
#ifndef	USE_TERMIO
	if (on)
		termbuf.lflags &= ~LCTLECH;
	else
		termbuf.lflags |= LCTLECH;
#else
# ifdef	ECHOCTL
	if (on)
		termbuf.c_lflag &= ~ECHOCTL;
	else
		termbuf.c_lflag |= ECHOCTL;
# endif
# ifdef	TCTLECH
	if (on)
		termbuf.c_lflag &= ~TCTLECH;
	else
		termbuf.c_lflag |= TCTLECH;
# endif
#endif
}

	int
tty_iscrnl()
{
#ifndef	USE_TERMIO
	return (termbuf.sg.sg_flags & CRMOD);
#else
	return (termbuf.c_iflag & ICRNL);
#endif
}

/*
 * A table of available terminal speeds
 */
struct termspeeds {
	int	speed;
	int	value;
} termspeeds[] = {
	{ 0,     B0 },    { 50,    B50 },   { 75,    B75 },
	{ 110,   B110 },  { 134,   B134 },  { 150,   B150 },
	{ 200,   B200 },  { 300,   B300 },  { 600,   B600 },
	{ 1200,  B1200 }, { 1800,  B1800 }, { 2400,  B2400 },
	{ 4800,  B4800 }, { 9600,  B9600 }, { 19200, B9600 },
	{ 38400, B9600 }, { -1,    B9600 }
};

	void
tty_tspeed(val)
	int val;
{
	register struct termspeeds *tp;

	for (tp = termspeeds; (tp->speed != -1) && (val > tp->speed); tp++)
		;
	cfsetospeed(&termbuf, tp->value);
}

	void
tty_rspeed(val)
	int val;
{
	register struct termspeeds *tp;

	for (tp = termspeeds; (tp->speed != -1) && (val > tp->speed); tp++)
		;
	cfsetispeed(&termbuf, tp->value);
}

#if	defined(CRAY2) && defined(UNICOS5)
	int
tty_isnewmap()
{
	return((termbuf.c_oflag & OPOST) && (termbuf.c_oflag & ONLCR) &&
			!(termbuf.c_oflag & ONLRET));
}
#endif

#ifdef	CRAY
# ifndef NEWINIT
extern	struct utmp wtmp;
extern char wtmpf[];
# else	/* NEWINIT */
int	gotalarm;

	/* ARGSUSED */
	void
nologinproc(sig)
	int sig;
{
	gotalarm++;
}
# endif	/* NEWINIT */
#endif /* CRAY */

#ifndef	NEWINIT
# ifdef	CRAY
extern void utmp_sig_init P((void));
extern void utmp_sig_reset P((void));
extern void utmp_sig_wait P((void));
extern void utmp_sig_notify P((int));
# endif
#endif

/*
 * getptyslave()
 *
 * Open the slave side of the pty, and do any initialization
 * that is necessary.  The return value is a file descriptor
 * for the slave side.
 */
	int
getptyslave()
{
	register int t = -1;
#if defined(M_UNIX) || defined(M_XENIX) /* for SCO */
	struct termio tiobuf; 
	struct termio *tiop = &tiobuf;
#endif /* SCO */

#if	!defined(CRAY) || !defined(NEWINIT)
# ifdef	LINEMODE
	int waslm;
# endif
# ifdef	TIOCGWINSZ
	struct winsize ws;
	extern int def_row, def_col;
# endif
	extern int def_tspeed, def_rspeed;
	/*
	 * Opening the slave side may cause initilization of the
	 * kernel tty structure.  We need remember the state of
	 * 	if linemode was turned on
	 *	terminal window size
	 *	terminal speed
	 * so that we can re-set them if we need to.
	 */
# ifdef	LINEMODE
	waslm = tty_linemode();
# endif


	/*
	 * Make sure that we don't have a controlling tty, and
	 * that we are the session (process group) leader.
	 */
# ifdef	TIOCNOTTY
	t = open(_PATH_TTY, O_RDWR);
	if (t >= 0) {
		(void) ioctl(t, TIOCNOTTY, (char *)0);
		(void) close(t);
	}
# endif


# ifdef	CRAY
	/*
	 * Wait for our parent to get the utmp stuff to get done.
	 */
	utmp_sig_wait();
# endif

	if ((t = open (line, O_RDWR | O_NOCTTY)) == -1)
		fatal (net, gettxt(":499", "could not open slave pty"));
	ttyp = t;
	if (ioctl (t, I_PUSH, "ptem") == -1)
		fatalperror (net, "ioctl I_PUSH ptem");
	if (ioctl (t, I_PUSH, "ldterm") == -1)
		fatalperror (net, "ioctl I_PUSH ldterm");
	if (ioctl (t, I_PUSH, "ttcompat") == -1)
		fatalperror (net, "ioctl I_PUSH ttcompat");

	/*
	 * set up the tty modes as we like them to be.
	 */
	init_termbuf();
#if defined(M_UNIX) || defined(M_XENIX)  /* for SCO */
	(void) ioctl(t, TCGETA, (char *)tiop);
    {							/* SCO L001 begin */
	struct termio tio;
	/*
	 * SCO L001: stopio() prevents new reads, writes and ioctls,
	 * but until recent mods did not abort any outstanding reads;
	 * to avoid reliance on those mods, temporarily change termio
	 * settings to wakeup outstanding reads and make them return
	 */
	tio = *tiop;			/* temporary structure copy */
	tio.c_line = 0;			/* force to line discipline 0 */
	tio.c_lflag &= ~ICANON;		/* force over to raw processing */
	tio.c_cc[VMIN] = 0;		/* make read return even if 0 chars */
	tio.c_cc[VTIME] = 0;		/* make read return without timeout */
	(void) ioctl(t, TCSETA, &tio);	/* set those parameters */
	(void) ioctl(t, TCFLSH, 0);	/* wakeup any outstanding reads */
    }							/* SCO L001 end */
	tiop->c_iflag |= (BRKINT | IGNPAR | 
			ISTRIP | ICRNL | IXON | IXANY);
	tiop->c_oflag |= (OPOST | ONLCR | TAB3);
	tiop->c_lflag |= (ISIG | ICANON | ECHO | ECHOK);
	tiop->c_cflag |= (EXTB | CS8 | CREAD);
	(void) ioctl(t, TCSETA, (char *)tiop);
#endif /* SCO */
# ifdef	TIOCGWINSZ
	if (def_row || def_col) {
		bzero((char *)&ws, sizeof(ws));
		ws.ws_col = def_col;
		ws.ws_row = def_row;
		(void)ioctl(t, TIOCSWINSZ, (char *)&ws);
	}
# endif

	/*
	 * Settings for sgtty based systems
	 */
# ifndef	USE_TERMIO
	termbuf.sg.sg_flags |= CRMOD|ANYP|ECHO|XTABS;
# endif	/* USE_TERMIO */

	/*
	 * Settings for UNICOS
	 */
# ifdef	CRAY
	termbuf.c_oflag = OPOST|ONLCR|TAB3;
	termbuf.c_iflag = IGNPAR|ISTRIP|ICRNL|IXON;
	termbuf.c_lflag = ISIG|ICANON|ECHO|ECHOE|ECHOK;
	termbuf.c_cflag = EXTB|HUPCL|CS8;
# endif

	/*
	 * Settings for all other termios/termio based
	 * systems, other than 4.4BSD.  In 4.4BSD the
	 * kernel does the initial terminal setup.
	 */
# if defined(USE_TERMIO) && !defined(CRAY) && (BSD <= 43)
#  ifndef	OXTABS
#   define OXTABS	0
#  endif
	termbuf.c_lflag |= ECHO;
	termbuf.c_oflag |= ONLCR|OXTABS;
	termbuf.c_iflag |= ICRNL;
	termbuf.c_iflag &= ~IXOFF;
# endif /* defined(USE_TERMIO) && !defined(CRAY) && (BSD <= 43) */
	tty_rspeed((def_rspeed > 0) ? def_rspeed : 9600);
	tty_tspeed((def_tspeed > 0) ? def_tspeed : 9600);
# ifdef	LINEMODE
	if (waslm)
		tty_setlinemode(1);
# endif	/* LINEMODE */

	/*
	 * Set the tty modes, and make this our controlling tty.
	 */
#if !defined(M_UNIX) && !defined(M_XENIX)
	set_termbuf();
#endif
	if (login_tty(t) == -1)
		fatalperror(net, "login_tty");
#endif	/* !defined(CRAY) || !defined(NEWINIT) */
	if (net > 2)
		(void) close(net);
	if (pty > 2)
		(void) close(pty);
}

#if	!defined(CRAY) || !defined(NEWINIT)
#ifndef	O_NOCTTY
#define	O_NOCTTY	0
#endif
/*
 * Open the specified slave side of the pty,
 * making sure that we have a clean tty.
 */
	int
cleanopen(line)
	char *line;
{
	register int t;

	/*
	 * Make sure that other people can't open the
	 * slave side of the connection.
	 */
	(void) chown(line, 0, 0);
	(void) chmod(line, 0600);

# if defined(M_UNIX)					/* SCO L001 */
	(void) stopio(line);				/* SCO L001 */
# endif							/* SCO L001 */
# if !defined(CRAY) && (BSD > 43)
	(void) revoke(line);
# endif
	t = open(line, O_RDWR|O_NOCTTY);
	if (t < 0)
		return(-1);

#if !defined(M_UNIX)					/* SCO L001 */

	/*
	 * Hangup anybody else using this ttyp, then reopen it for
	 * ourselves.
	 */
# if !defined(CRAY) && (BSD <= 43)
	(void) sigset(SIGHUP, SIG_IGN);
# ifndef SYSV
	vhangup();
# endif
	(void) sigset(SIGHUP, SIG_DFL);
	t = open(line, O_RDWR|O_NOCTTY);
	if (t < 0)
		return(-1);
# endif
#endif /* !defined(M_UNIX) */				/* SCO L001 */
# if	defined(CRAY) && defined(TCVHUP)
	{
		register int i;
		(void) signal(SIGHUP, SIG_IGN);
		(void) ioctl(t, TCVHUP, (char *)0);
		(void) signal(SIGHUP, SIG_DFL);
		setpgrp();
		i = open(line, O_RDWR);
		if (i < 0)
			return(-1);
		(void) close(t);
		t = i;
	}
# endif	/* defined(CRAY) && defined(TCVHUP) */
	return(t);
}
#endif	/* !defined(CRAY) || !defined(NEWINIT) */

#if BSD <= 43
	int
login_tty(t)
	int t;
{
	if (setpgrp() < 0)
		fatalperror(net, "setpgrp()");
# ifdef	TIOCSCTTY
	if (ioctl(t, TIOCSCTTY, (char *)0) < 0)
		fatalperror(net, "ioctl(sctty)");
# else
	close(open(line, O_RDWR));
# endif
	if (t != 0)
		(void) dup2(t, 0);
	if (t != 1)
		(void) dup2(t, 1);
	if (t != 2)
		(void) dup2(t, 2);
	if (t > 2)
		(void) close(t);
	return(0);
}
#endif	/* BSD <= 43 */

#ifdef	NEWINIT
char *gen_id = "fe";
#endif

int global_pid;

/*
 * startslave(host)
 *
 * Given a hostname, do whatever
 * is necessary to startup the login process on the slave side of the pty.
 */

/* ARGSUSED */
	void
startslave(host, autologin, autoname)
	char *host;
	int autologin;
	char *autoname;
{
	register int i;
	long time();
	char name[256];
#if defined(M_XENIX)
	struct stat statb;
	register struct utmp *ut;
	extern struct utmp *getutent();
	int multi=0, cnt=0, Mline, Mmult, Mutline;
	int serstate, ownpid;			/* S018 */
	char	lbuf[25];	
#endif

#ifdef	NEWINIT
	extern char *ptyip;
	struct init_request request;
	void nologinproc();
	register int n;
#endif	/* NEWINIT */

#if	defined(AUTHENTICATE)
	if (!autoname || !autoname[0])
		autologin = 0;

	if (autologin < auth_level) {
		fatal(net, gettxt(":500", "Authorization failed"));
		exit(1);
	}
#endif

#ifndef	NEWINIT
# ifdef	CRAY
	utmp_sig_init();
# endif	/* CRAY */

	if ((i = fork()) < 0)
		fatalperror(net, "fork");
	if (i) {
		global_pid = i;
# ifdef	CRAY
		/*
		 * Cray parent will create utmp entry for child and send
		 * signal to child to tell when done.  Child waits for signal
		 * before doing anything important.
		 */
		register int pid = i;
		void sigjob P((int));

		setpgrp();
		utmp_sig_reset();		/* reset handler to default */
		/*
		 * Create utmp entry for child
		 */
		(void) time(&wtmp.ut_time);
		wtmp.ut_type = LOGIN_PROCESS;
		wtmp.ut_pid = pid;
		SCPYN(wtmp.ut_user, "LOGIN");
		SCPYN(wtmp.ut_host, host);
		SCPYN(wtmp.ut_line, line + sizeof("/dev/") - 1);
		SCPYN(wtmp.ut_id, wtmp.ut_line+3);
		pututline(&wtmp);
		endutent();
		if ((i = open(wtmpf, O_WRONLY|O_APPEND)) >= 0) {
			(void) write(i, (char *)&wtmp, sizeof(struct utmp));
			(void) close(i);
		}
		(void) signal(WJSIGNAL, sigjob);
		utmp_sig_notify(pid);
# endif	/* CRAY */
		/*
		 * set terminit flag so future window size changes will be
		 * done (see "SVR4.2 Note" in termstat.c)
		 */
		set_terminit();
	} else {
		getptyslave();
		/*
		 * now do deferred terminal mode changes
		 * (see "SVR4.2 Note" in termstat.c)
		 */
		do_deferred();
		start_login(host, autologin, autoname);
		/*NOTREACHED*/
	}
#else	/* NEWINIT */

	/*
	 * Init will start up login process if we ask nicely.  We only wait
	 * for it to start up and begin normal telnet operation.
	 */
	if ((i = open(INIT_FIFO, O_WRONLY)) < 0) {
		char tbuf[128];
		(void) sprintf(tbuf, gettxt(":501", "Can't open %s\n"), INIT_FIFO);
		fatalperror(net, tbuf);
	}
	memset((char *)&request, 0, sizeof(request));
	request.magic = INIT_MAGIC;
	SCPYN(request.gen_id, gen_id);
	SCPYN(request.tty_id, &line[8]);
	SCPYN(request.host, host);
	SCPYN(request.term_type, terminaltype ? terminaltype : "network");
#if	!defined(UNICOS5)
	request.signal = SIGCLD;
	request.pid = getpid();
#endif
#ifdef BFTPDAEMON
	/*
	 * Are we working as the bftp daemon?
	 */
	if (bftpd) {
		SCPYN(request.exec_name, BFTPPATH);
	}
#endif /* BFTPDAEMON */
	if (write(i, (char *)&request, sizeof(request)) < 0) {
		char tbuf[128];
		(void) sprintf(tbuf, gettxt(":502", "Can't write to %s\n"), INIT_FIFO);
		fatalperror(net, tbuf);
	}
	(void) close(i);
	(void) sigset(SIGALRM, nologinproc);
	for (i = 0; ; i++) {
		char tbuf[128];
		alarm(15);
		n = read(pty, ptyip, BUFSIZ);
		if (i == 3 || n >= 0 || !gotalarm)
			break;
		gotalarm = 0;
		sprintf(tbuf, gettxt(":503", "telnetd: waiting for /etc/init to start login process on %s\r\n"), line);
		(void) write(net, tbuf, strlen(tbuf));
	}
	if (n < 0 && gotalarm)
		fatal(net, gettxt(":504", "/etc/init didn't start login process"));
	pcc += n;
	alarm(0);
	(void) sigset(SIGALRM, SIG_DFL);

	return;
#endif	/* NEWINIT */
}

char	*envinit[4];
extern char **environ;

	void
init_env()
{
	extern char *getenv();
	char **envp;

	envp = envinit;
	if (*envp = getenv("TZ"))
		*envp++ -= 3;
#ifdef	CRAY
	else
		*envp++ = "TZ=GMT0";
#endif
	/* if "LANG=X" is found in our current environment, *envp
	 * will be non null and point to X.  We need the envinit
	 * element to point to LANG=X, so if we find one then
	 *  - move the pointer back so it points to all of LANG=X
	 * increment to the next slot in the env array
	 */
	if (*envp = getenv("LANG"))
		*envp++ -= 5;
	*envp = 0;
	environ = envinit;
}

#ifndef	NEWINIT

/*
 * start_login(host)
 *
 * Assuming that we are now running as a child processes, this
 * function will turn us into the login process.
 */

	void
start_login(host, autologin, name)
	char *host;
	int autologin;
	char *name;
{
#define	ARG_COUNT	256
	char buf_invoke[ARG_COUNT];
	int  retval;
	struct utmpx ut;
		
	/* System V login expects a utmp entry to already be there */

	bzero ((char *) &ut, sizeof (ut));
	(void) strncpy(ut.ut_user, ".telnet", sizeof(ut.ut_user));
	(void) strncpy(ut.ut_line, line, sizeof(ut.ut_line));
	ut.ut_pid = (o_pid_t)getpid();
	ut.ut_id[0] = 't';
	ut.ut_id[1] = 'n';
	ut.ut_id[2] = SC_WILDC;
	ut.ut_id[3] = SC_WILDC;
	ut.ut_type = LOGIN_PROCESS;
	ut.ut_exit.e_termination = 0;
	ut.ut_exit.e_exit = 0;
	(void) time (&ut.ut_tv.tv_sec);
	if (makeutx(&ut) == NULL)
		syslog(LOG_INFO, gettxt(":505", "makeutx failed %m"));

	strcpy(buf_invoke, "/usr/lib/iaf/in.login/scheme -H ");
	strncat(buf_invoke, host, ARG_COUNT-strlen(buf_invoke));
	strncat(buf_invoke, " ",  ARG_COUNT-strlen(buf_invoke));
	if (terminaltype) {
		strncat(buf_invoke, "TERM=",  ARG_COUNT-strlen(buf_invoke));
		strncat(buf_invoke, (char *) terminaltype,
			ARG_COUNT-strlen(buf_invoke));
	}
	else {
		strncat(buf_invoke, "-",  ARG_COUNT-strlen(buf_invoke));
	}

	retval = invoke(0, buf_invoke);

	if (0 == retval) {
		if (set_id((char *)0) != 0) {
			syslog(LOG_ERR, gettxt(":506", "set_id() failed"));
			fatalperror(net,gettxt(":506", "set_id() failed"));
			exit(2);
		}
		set_env();
		execl("/usr/bin/shserv", "shserv", 0);
		syslog(LOG_ERR, gettxt(":507", "Unable to exec /usr/bin/shserv: %m"));
		fatalperror(net, gettxt(":508", "Unable to exec /usr/bin/shserv"));
		/*NOTREACHED*/
	}
	else {
		syslog(LOG_ERR, gettxt(":509", "Unable to invoke login scheme"));
		fatal(net, gettxt(":509", "Unable to invoke login scheme"));
		/*NOTREACHED*/
	}
}

#endif	/* NEWINIT */

/*
 * cleanup()
 *
 * This is the routine to call when we are all through, to
 * clean up anything that needs to be cleaned up.
 */
	/* ARGSUSED */
	void
cleanup(sig)
	int sig;
{
#ifndef	CRAY
# if (BSD > 43) || defined(convex)
	char *p;

	p = line + sizeof("/dev/") - 1;
	if (logout(p))
		logwtmp(p, "", "");
	(void)chmod(line, 0666);
	(void)chown(line, 0, 0);
	*p = 'p';
	(void)chmod(line, 0666);
	(void)chown(line, 0, 0);
	(void) shutdown(net, 2);
	exit(1);
# else
	void rmut();

	rmut();
# ifndef SYSV
	vhangup();	/* XXX */
# endif
	(void) shutdown(net, 2);
	exit(1);
# endif
#else	/* CRAY */
# ifdef	NEWINIT
	(void) shutdown(net, 2);
	exit(1);
# else	/* NEWINIT */
	static int incleanup = 0;
	register int t;

	/*
	 * 1: Pick up the zombie, if we are being called
	 *    as the signal handler.
	 * 2: If we are a nested cleanup(), return.
	 * 3: Try to clean up TMPDIR.
	 * 4: Fill in utmp with shutdown of process.
	 * 5: Close down the network and pty connections.
	 * 6: Finish up the TMPDIR cleanup, if needed.
	 */
	if (sig == SIGCHLD)
		while (waitpid(-1, 0, WNOHANG) > 0)
			;	/* VOID */
	t = sigblock(sigmask(SIGCHLD));
	if (incleanup) {
		sigsetmask(t);
		return;
	}
	incleanup = 1;
	sigsetmask(t);

	t = cleantmp(&wtmp);
	setutent();	/* just to make sure */
	rmut(line);
	close(pty);
	(void) shutdown(net, 2);
	if (t == 0)
		cleantmp(&wtmp);
	exit(1);
# endif	/* NEWINT */
#endif	/* CRAY */
}

#if	defined(CRAY) && !defined(NEWINIT)
/*
 * _utmp_sig_rcv
 * utmp_sig_init
 * utmp_sig_wait
 *	These three functions are used to coordinate the handling of
 *	the utmp file between the server and the soon-to-be-login shell.
 *	The server actually creates the utmp structure, the child calls
 *	utmp_sig_wait(), until the server calls utmp_sig_notify() and
 *	signals the future-login shell to proceed.
 */
static int caught=0;		/* NZ when signal intercepted */
static void (*func)();		/* address of previous handler */

	void
_utmp_sig_rcv(sig)
	int sig;
{
	caught = 1;
	(void) sigset(SIGUSR1, func);
}

	void
utmp_sig_init()
{
	/*
	 * register signal handler for UTMP creation
	 */
	if ((int)(func = sigset(SIGUSR1, _utmp_sig_rcv)) == -1)
		fatalperror(net, "telnetd/signal");
}

	void
utmp_sig_reset()
{
	(void) sigset(SIGUSR1, func);	/* reset handler to default */
}

	void
utmp_sig_wait()
{
	/*
	 * Wait for parent to write our utmp entry.
	 */
	sigoff();
	while (caught == 0) {
		pause();	/* wait until we get a signal (sigon) */
		sigoff();	/* turn off signals while we check caught */
	}
	sigon();		/* turn on signals again */
}

	void
utmp_sig_notify(pid)
{
	kill(pid, SIGUSR1);
}

static int gotsigjob = 0;

	/*ARGSUSED*/
	void
sigjob(sig)
	int sig;
{
	register int jid;
	register struct jobtemp *jp;

	while ((jid = waitjob(NULL)) != -1) {
		if (jid == 0) {
			return;
		}
		gotsigjob++;
		jobend(jid, NULL, NULL);
	}
}

/*
 * Clean up the TMPDIR that login created.
 * The first time this is called we pick up the info
 * from the utmp.  If the job has already gone away,
 * then we'll clean up and be done.  If not, then
 * when this is called the second time it will wait
 * for the signal that the job is done.
 */
	int
cleantmp(wtp)
	register struct utmp *wtp;
{
	struct utmp *utp;
	static int first = 1;
	register int mask, omask, ret;
	extern struct utmp *getutid P((struct utmp *));

	mask = sigmask(WJSIGNAL);

	if (first == 0) {
		omask = sigblock(mask);
		while (gotsigjob == 0)
			sigpause(omask);
		return(1);
	}
	first = 0;
	setutent();	/* just to make sure */

	utp = getutid(wtp);
	if (utp == 0) {
		syslog(LOG_ERR, gettxt(":510", "Can't get /etc/utmp entry to clean TMPDIR"));
		return(-1);
	}
	/*
	 * Nothing to clean up if the user shell was never started.
	 */
	if (utp->ut_type != USER_PROCESS || utp->ut_jid == 0)
		return(1);

	/*
	 * Block the WJSIGNAL while we are in jobend().
	 */
	omask = sigblock(mask);
	ret = jobend(utp->ut_jid, utp->ut_tpath, utp->ut_user);
	sigsetmask(omask);
	return(ret);
}

	int
jobend(jid, path, user)
	register int jid;
	register char *path;
	register char *user;
{
	static int saved_jid = 0;
	static char saved_path[sizeof(wtmp.ut_tpath)+1];
	static char saved_user[sizeof(wtmp.ut_user)+1];

	if (path) {
		strncpy(saved_path, path, sizeof(wtmp.ut_tpath));
		strncpy(saved_user, user, sizeof(wtmp.ut_user));
		saved_path[sizeof(saved_path)] = '\0';
		saved_user[sizeof(saved_user)] = '\0';
	}
	if (saved_jid == 0) {
		saved_jid = jid;
		return(0);
	}
	cleantmpdir(jid, saved_path, saved_user);
	return(1);
}

/*
 * Fork a child process to clean up the TMPDIR
 */
cleantmpdir(jid, tpath, user)
	register int jid;
	register char *tpath;
	register char *user;
{
	switch(fork()) {
	case -1:
		syslog(LOG_ERR, gettxt(":511", "TMPDIR cleanup(%s): fork() failed: %m\n"),
							tpath);
		break;
	case 0:
		execl(CLEANTMPCMD, CLEANTMPCMD, user, tpath, 0);
		syslog(LOG_ERR, gettxt(":512", "TMPDIR cleanup(%s): execl(%s) failed: %m\n"),
							tpath, CLEANTMPCMD);
		exit(1);
	default:
		/*
		 * Forget about child.  We will exit, and
		 * /etc/init will pick it up.
		 */
		break;
	}
}
#endif	/* defined(CRAY) && !defined(NEWINIT) */

/*
 * rmut()
 *
 * This is the function called by cleanup() to
 * remove the utmp entry for this person.
 */

#if	!defined(CRAY) && BSD <= 43
	void
rmut()
{
	register f;
	int found = 0;
	struct utmp *u, *utmp;
	int nutmp;
	struct stat statbf;

	f = open(utmpf, O_RDWR);
	if (f >= 0) {
		(void) fstat(f, &statbf);
		utmp = (struct utmp *)malloc((unsigned)statbf.st_size);
		if (!utmp)
			syslog(LOG_ERR, gettxt(":513", "utmp malloc failed"));
		if (statbf.st_size && utmp) {
			nutmp = read(f, (char *)utmp, (int)statbf.st_size);
			nutmp /= sizeof(struct utmp);
		
			for (u = utmp ; u < &utmp[nutmp] ; u++) {
				if (SCMPN(u->ut_line, line+5) ||
				    u->ut_name[0]==0)
					continue;
#ifndef L_SET
#define L_SET 0
#endif
                                (void) lseek(f, ((long)u)-((long)utmp), L_SET);
                                SCPYN(u->ut_name, "");
#ifdef UT_HOST
                                SCPYN(u->ut_host, "");
#endif
				u->ut_type = DEAD_PROCESS;
				(void) time(&u->ut_time);
				(void) write(f, (char *)u, sizeof(wtmp));
				found++;
			}
		}
		(void) close(f);
	}
	if (found) {
		f = open(wtmpf, O_WRONLY|O_APPEND);
		if (f >= 0) {
			SCPYN(wtmp.ut_line, line+5);
			SCPYN(wtmp.ut_id, line+8);
			wtmp.ut_pid = global_pid;
			wtmp.ut_type = DEAD_PROCESS;
			SCPYN(wtmp.ut_name, "");
#ifdef UT_HOST
                        SCPYN(wtmp.ut_host, "");
#endif
			(void) time(&wtmp.ut_time);
			(void) write(f, (char *)&wtmp, sizeof(wtmp));
			(void) close(f);
		}
	}
	(void) chmod(line, 0666);
	(void) chown(line, 0, 0);
	line[strlen("/dev/")] = 'p';
	(void) chmod(line, 0666);
	(void) chown(line, 0, 0);
}  /* end of rmut */
#endif	/* CRAY */
