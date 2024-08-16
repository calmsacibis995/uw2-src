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

#ident	"@(#)xcplxcurses:curses.c	1.1.2.3"
#ident  "$Header: curses.c 1.1 91/07/09 $"

/*
 *	@(#) curses.c 1.1 90/03/30 lxcurses:curses.c
 */
/*
 * Define global variables
 *
 * @(#)curses.c	1.2 (Berkeley) 5/9/83
 */
# include	"xcurses.h"

bool	_echoit		= TRUE,	/* set if stty indicates ECHO		*/
	_rawmode	= FALSE,/* set if stty indicates RAW mode	*/
	My_term		= FALSE,/* set if user specifies terminal type	*/
	_endwin		= FALSE;/* set if endwin has been called	*/

char	ttytype[10],		/* long name of tty			*/
	*Def_term	= "unknown";	/* default terminal type	*/

int	_tty_ch		= 1,	/* file channel which is a tty		*/
	LINES,			/* number of lines allowed on screen	*/
	COLS,			/* number of columns allowed on screen	*/
	_res_flg;		/* sgtty flags for reseting later	*/

WINDOW	*stdscr		= (WINDOW *)NULL,
	*curscr		= (WINDOW *)NULL;

# ifdef DEBUG
FILE	*outf;			/* debug output file			*/
# endif

SGTTY	_tty;			/* tty modes				*/

bool	AM, BS, CA, DA, DB, EO, GT, HZ, IN, MI, MS, NC, OS, UL, XN,
	NONL, UPPERCASE, normtty, _pfast;

char	*AL, *BC, *BT, *CD, *CE, *CL, *CM, *CR, *DC, *DL, *DM, *DO,
	*ED, *EI, *HO, *IC, *IM, *IP, *LL, *MA, *ND, *NL, *SE, *SF,
	*SO, *SR, *TA, *TE, *TI, *UC, *UE, *UP, *US, *VB, *VE, *VS, PC;
