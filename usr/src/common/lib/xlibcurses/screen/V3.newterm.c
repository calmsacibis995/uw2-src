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

#ident	"@(#)curses:common/lib/xlibcurses/screen/V3.newterm.c	1.1.2.3"
#ident  "$Header: V3.newterm.c 1.2 91/06/26 $"

#include	"curses_inc.h"
extern	int	_outchar();

#ifdef	_VR3_COMPAT_CODE
extern	void	_update_old_y_area();

#undef	newterm
SCREEN	*
newterm(type, outfptr, infptr)
char	*type;
FILE	*outfptr, *infptr;
{
    _y16update = _update_old_y_area;
    return (newscreen(type, 0, 0, 0, outfptr, infptr));
}
#endif	/* _VR3_COMPAT_CODE */
