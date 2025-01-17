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

#ident	"@(#)ucb:common/ucbcmd/plot/vfont.h	1.2"
#ident	"$Header: $"

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved. The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
 * All Rights Reserved.
 */


/*
 * The structures header and dispatch define the format of a font file.
 *
 * A font file contains one struct 'header', an array of NUM_DISPATCH struct
 * 'dispatch'es, then an array of bytes containing bit maps.
 *
 * See vfont(5) for more details.
 */

#ifndef _vfont_h
#define _vfont_h

struct header {
	short		magic;		/* Magic number VFONT_MAGIC */
	unsigned short	size;		/* Total # bytes of bitmaps */
	short		maxx;		/* Maximum horizontal glyph size */
	short		maxy;		/* Maximum vertical   glyph size */
	short		xtend;		/* (unused?) */
}; 
#define	VFONT_MAGIC	0436

struct dispatch {
	unsigned short	addr;		/* &(glyph) - &(start of bitmaps) */
	short		nbytes;		/* # bytes of glyphs (0 if no glyph) */
	char		up, down, left, right;	/* Widths from baseline point */
	short		width;		/* Logical width, used by troff */
};
#define	NUM_DISPATCH	256

#endif /*!_vfont_h*/
