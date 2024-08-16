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

#ident	"@(#)curses:common/lib/xlibcurses/screen/ring.c	1.7.2.4"
#ident  "$Header: ring.c 1.2 91/06/26 $"

#include	"curses_inc.h"

_ring(bf)
bool	bf;
{
#ifdef __STDC__
    extern	int	_outch(int);
#else
    extern	int	_outch();
#endif
    static	char	offsets[2] = {45 /* flash_screen */, 1 /* bell */ };
    char	**str_array = (char **) cur_strs;
#ifdef	DEBUG
    if (outf)
	fprintf(outf, "_ring().\n");
#endif	/* DEBUG */
    _PUTS(str_array[offsets[bf]] ? str_array[offsets[bf]] : str_array[offsets[1 - bf]], 0);
    (void) fflush(SP->term_file);
    if (_INPUTPENDING)
	(void) doupdate();
    return (OK);
}
