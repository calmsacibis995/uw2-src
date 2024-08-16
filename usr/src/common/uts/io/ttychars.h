/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _IO_TTYCHARS_H	/* wrapper symbol for kernel use */
#define _IO_TTYCHARS_H	/* subject to change without notice */

#ident	"@(#)kern:io/ttychars.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * User visible structures and constants related to terminal handling.
 * BSD compatibility.
 */

struct ttychars {
	char	tc_erase;	/* erase last character */
	char	tc_kill;	/* erase entire line */
	char	tc_intrc;	/* interrupt */
	char	tc_quitc;	/* quit */
	char	tc_startc;	/* start output */
	char	tc_stopc;	/* stop output */
	char	tc_eofc;	/* end-of-file */
	char	tc_brkc;	/* input delimiter (like nl) */
	char	tc_suspc;	/* stop process signal */
	char	tc_dsuspc;	/* delayed stop process signal */
	char	tc_rprntc;	/* reprint line */
	char	tc_flushc;	/* flush output (toggles) */
	char	tc_werasc;	/* word erase */
	char	tc_lnextc;	/* literal next character */
};

#ifndef _IO_TERMIOS_H

#define	CTRL(c)	((c)&037)

/* default BSD special characters */
#define	CERASE	0177
#define	CKILL	CTRL('u')
#define	CINTR	CTRL('c')
#define	CQUIT	034		/* FS, ^\ */
#define	CSTART	CTRL('q')
#define	CSTOP	CTRL('s')
#define	CEOF	CTRL('d')
#define	CEOT	CEOF
#define	CBRK	0377
#define	CSUSP	CTRL('z')
#define	CDSUSP	CTRL('y')
#define	CRPRNT	CTRL('r')
#define	CFLUSH	CTRL('o')
#define	CWERASE	CTRL('w')
#define	CLNEXT	CTRL('v')

#endif /* _IO_TERMIOS_H */

#if defined(__cplusplus)
	}
#endif

#endif /* _IO_TTYCHARS_H */
