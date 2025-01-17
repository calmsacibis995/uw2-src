/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ksh:include/terminal.h	1.3.6.3"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/cmd/ksh/include/terminal.h,v 1.1 91/02/28 17:40:23 ccs Exp $"


/*
 * terminal interface
 */

#ifndef _terminal_
#define _terminal_	1
#ifdef _termios_
#   include	<termios.h>
#   ifdef sgi	/* special hack to eliminate ^M problem */
#	ifndef ECHOCTL
#	    define ECHOCTL	ECHOE
#	endif /* ECHOCTL */
#	ifndef CNSUSP
#	    define CNSUSP	CNSWTCH
#	endif /* CNSUSP */
#   endif /* sgi */
#else
#   ifdef _sys_termios_
#	include	<sys/termios.h>
#	define _termios_
#   endif /* _sys_termios_ */
#endif /* _termios_ */

#ifdef _termios_
#   ifndef TCSANOW
#	define TCSANOW		TCSETS
#	define TCSADRAIN	TCSETSW
#	define TCSAFLUSH	TCSETSF
#	define tcgetattr(fd,tty)	ioctl(fd, TCGETS, tty)
#	define tcsetattr(fd,action,tty)	ioctl(fd, action, tty)
#	define cfgetospeed(tp)		((tp)->c_cflag & CBAUD)
#   endif /* TCSANOW */
    /* the following is because of an ultrix bug */
#   if defined(TCSADFLUSH) && !defined(TCSAFLUSH)
#	define TCSAFLUSH	TCSADFLUSH
#   endif
#   undef TIOCGETC
#   undef _termio_
#   undef _sys_termio_
#   undef _sgtty_
#   undef _sys_ioctl_
#   undef _sys_bsdtty_
#else
#   undef OLDTERMIO
#endif /* _termios_ */

#ifdef _termio_
#   include	<termio.h>
#else
#   ifdef _sys_termio_
#	include	<sys/termio.h>
#   define _termio_ 1
#   endif /* _sys_termio_ */
#endif /* _termio_ */
#ifdef _termio_
#   define termios termio
#   undef _sgtty_
#   undef TIOCGETC
#   undef _sys_ioctl_
#   define tcgetattr(fd,tty)		ioctl(fd, TCGETA, tty)
#   define tcsetattr(fd,action,tty)	ioctl(fd, action, tty)
#endif /* _termio_ */

#ifdef _sys_bsdtty_
#   include	<sys/bsdtty.h>
#endif /* _sys_bsdtty_ */

#ifdef _sgtty_
#   include	<sgtty.h>
#   ifdef _sys_nttyio_
#	ifndef LPENDIN
#	    include	<sys/nttyio.h>
#	endif /* LPENDIN */
#   endif /* _sys_nttyio_ */
#   define termios sgttyb
#   undef _sys_ioctl_
#   ifdef TIOCSETN
#	undef TCSETAW
#   endif /* TIOCSETN */
#   ifdef _SELECT_
#	ifndef included_sys_time_
#	    ifdef _sys_Time_
#		include	<sys/time.h>
#	    endif /* _sys_Time_ */
#	    define included_sys_time_
#	endif /* included_sys_time_ */
	extern const int tty_speeds[];
#   endif /* _SELECT_ */
#   ifdef TIOCGETP
#	define tcgetattr(fd,tty)		ioctl(fd, TIOCGETP, tty)
#	define tcsetattr(fd,action,tty)	ioctl(fd, action, tty)
#   else
#	define tcgetattr(fd,tty)	gtty(fd, tty)
#	define tcsetattr(fd,action,tty)	stty(fd, tty)
#   endif /* TIOCGETP */
#endif /* _sgtty_ */

#ifndef TCSANOW
#   ifdef TCSETAW
#	define TCSANOW	TCSETA
#	ifdef u370
	/* delays are too long, don't wait for output to drain */
#	    define TCSADRAIN	TCSETA
#	else
#	   define TCSADRAIN	TCSETAW
#	endif /* u370 */
#	define TCSAFLUSH	TCSETAF
#   else
#	ifdef TIOCSETN
#	    define TCSANOW	TIOCSETN
#	    define TCSADRAIN	TIOCSETN
#	    define TCSAFLUSH	TIOCSETP
#	endif /* TIOCSETN */
#   endif /* TCSETAW */
#endif /* TCSANOW */
#endif /* _terminal_ */

#ifndef _termios_
#   define cfgetospeed(tp)	((tp)->c_cflag & CBAUD)
#endif /* _termios_ */
/* set ECHOCTL if driver can echo control charaters as ^c */
#ifdef LCTLECH
#   ifndef ECHOCTL
#	define ECHOCTL	LCTLECH
#   endif /* !ECHOCTL */
#endif /* LCTLECH */
#ifdef LNEW_CTLECH
#   ifndef ECHOCTL
#	define ECHOCTL  LNEW_CTLECH
#   endif /* !ECHOCTL */
#endif /* LNEW_CTLECH */
#ifdef LNEW_PENDIN
#   ifndef PENDIN
#	define PENDIN LNEW_PENDIN
#  endif /* !PENDIN */
#endif /* LNEW_PENDIN */
#ifndef ECHOCTL
#   ifndef VEOL2
#	define RAWONLY	1
#   endif /* !VEOL2 */
#endif /* !ECHOCTL */

#ifdef _sys_filio_
#   ifndef FIONREAD
#	include	<sys/filio.h>
#   endif /* FIONREAD */
#endif /* _sys_filio_ */
/* set FIORDCHK if you can check for characters in input queue */
#ifdef FIONREAD
#   ifndef FIORDCHK
#	define FIORDCHK	FIONREAD
#   endif /* !FIORDCHK */
#endif /* FIONREAD */

#ifdef PROTO
    extern int	tty_alt(int);
    extern void tty_cooked(int);
    extern int	tty_get(int,struct termios*);
    extern int	tty_raw(int);
    extern int	tty_check(int);
#else
    extern int	tty_alt();
    extern void tty_cooked();
    extern int	tty_get();
    extern int	tty_raw();
    extern int	tty_check();
#endif /* PROTO */
extern int	tty_set();
