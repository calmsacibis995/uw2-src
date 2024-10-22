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

#ident	"@(#)curses:common/lib/xlibcurses/screen/curses.c	1.14.2.3"
#ident  "$Header: curses.c 1.2 91/06/26 $"

/* Define global variables */

#include	"curses_inc.h"

WINDOW	*stdscr, *curscr, *_virtscr;
int	LINES, COLS, TABSIZE, COLORS, COLOR_PAIRS;
short	curs_errno = -1;
int	(*_setidln)(), (*_useidln)(), (*_quick_ptr)();
int	(*_do_slk_ref)(), (*_do_slk_tch)(), (*_do_slk_noref)();
void	(*_rip_init)();		/* to initialize rip structures */
void	(*_slk_init)();		/* to initialize slk structures */
SCREEN	*SP;
MOUSE_STATUS Mouse_status = {-1, -1, {BUTTON_RELEASED, BUTTON_RELEASED, BUTTON_RELEASED}, 0};

#ifdef	_VR3_COMPAT_CODE
void	(*_y16update)();
chtype	*acs32map;

#undef	acs_map
_ochtype	*acs_map;
#else	/* _VR3_COMPAT_CODE */
chtype		*acs_map;
#endif	/* _VR3_COMPAT_CODE */

char	*curses_version = "SVR4", curs_parm_err[32];
bool	_use_env = TRUE;

#ifdef	DEBUG
FILE	*outf = stderr;		/* debug output file */
#endif	/* DEBUG */

short	_csmax,		/* max size of a multi-byte character */
	_scrmax;	/* max size of a multi-column character */
bool	_mbtrue;	/* a true multi-byte character */
