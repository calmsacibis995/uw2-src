/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/devices/wdc3x/font.h	1.1"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */

/*
 * Copyright (c) 1992,1993 Pittsburgh Powercomputing Corporation (PPc).
 * All Rights Reserved.
 */

#define FONT_COUNT 	8		/* max # of downloadable fonts */
#define FONT_NUMGLYPHS	256		/* max # of glyphs perr font */
#define FONT_MAXWIDTH	32		/* max width of a downloadable font */
#define FONT_MAXHEIGHT	32		/* max width of a downloadable font */
#define FONT_MAXSIZE	(8*32)		/* max possible converted glyph size */

#define BLTDATA unsigned short

typedef struct wd90_font {
    int w;		/* width of glyphs in pixels */
    int h;		/* height of glyphs in pixels */
    int ascent;		/* distance from baseline to top of glyph */
    int lsize;		/* no. of bytes stored for each line of a glyph */
    int size;		/* no. of bytes stored for each glyph */
    BLTDATA *data;	/* converted data */
} wd90_font_rec;

extern wd90_font_rec wd90_fonts[FONT_COUNT];

extern SIBool wd90c31_font_check();
extern SIBool wd90c31_font_download();
extern SIBool wd90c31_font_stplblt();
extern SIBool wd90c31_font_free();
