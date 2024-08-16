/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)xcplxcurses:scanw.c	1.1.2.3"
#ident  "$Header: scanw.c 1.1 91/07/09 $"

/*
 *	$Header: scanw.c 1.1 91/07/09 $
 */
/*
 *	MODIFICATION HISTORY
 *	M000	15 Mar 1985	ncm
 *	- Removed references to _IOSTRG, it is no longer used.
 *	M001	19 Mar 1985	ncm
 *	- Changed include file from curses.ext to ext.h
 */
/*
 * scanw and friends
 *
 * 1/26/81 (Berkeley) @(#)scanw.c	1.1
 */

# include	"ext.h"		/* M001 */

/*
 *	This routine implements a scanf on the standard screen.
 */
scanw(fmt, args)
char	*fmt;
int	args; {

	return _sscans(stdscr, fmt, &args);
}
/*
 *	This routine implements a scanf on the given window.
 */
wscanw(win, fmt, args)
WINDOW	*win;
char	*fmt;
int	args; {

	return _sscans(win, fmt, &args);
}
/*
 *	This routine actually executes the scanf from the window.
 *
 *	This is really a modified version of "sscanf".  As such,
 * it assumes that sscanf interfaces with the other scanf functions
 * in a certain way.  If this is not how your system works, you
 * will have to modify this routine to use the interface that your
 * "sscanf" uses.
 */
_sscans(win, fmt, args)
WINDOW	*win;
char	*fmt;
int	*args; {

	char	buf[256];
	FILE	junk;

	/* junk._flag = _IOREAD|_IOSTRG;	M000 */
	if (wgetstr(win, buf) == ERR)
		return ERR;
	junk._file = -1;
	junk._flag = _IOREAD;
	junk._base = junk._ptr = (unsigned char *)buf;
	junk._cnt = strlen(buf);
	return _doscan(&junk, fmt, args);
}
