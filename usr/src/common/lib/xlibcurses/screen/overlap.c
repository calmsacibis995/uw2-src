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

#ident	"@(#)curses:common/lib/xlibcurses/screen/overlap.c	1.5.2.3"
#ident  "$Header: overlap.c 1.2 91/06/26 $"
#include	"curses_inc.h"

/*
 * This routine writes Srcwin on Dstwin.
 * Only the overlapping region is copied.
 */

_overlap(Srcwin, Dstwin, Overlay)
register	WINDOW	*Srcwin, *Dstwin;
int			Overlay;
{
    register	int	sby, sbx, sey, sex, dby, dbx, dey, dex,
			top, bottom, left, right;

#ifdef	DEBUG
    if (outf)
	fprintf(outf, "OVERWRITE(0%o, 0%o);\n", Srcwin, Dstwin);
#endif	/* DEBUG */

    sby = Srcwin->_begy;	dby = Dstwin->_begy;
    sbx = Srcwin->_begx;	dbx = Dstwin->_begx;
    sey = sby + Srcwin->_maxy;	dey = dby + Dstwin->_maxy;
    sex = sbx + Srcwin->_maxx;	dex = dbx + Dstwin->_maxx;

    if (sey < dby || sby > dey || sex < dbx || sbx > dex)
	return (ERR);

    top = _MAX(sby, dby);	bottom = _MIN(sey, dey);
    left = _MAX(sbx, dbx);	right = _MIN(sex, dex);

    sby = top - sby;		sbx = left - sbx;
    dey = bottom - dby - 1;	dex = right - dbx - 1;
    dby = top - dby;		dbx = left - dbx;

    return (copywin(Srcwin, Dstwin, sby, sbx, dby, dbx, dey, dex, Overlay));
}
