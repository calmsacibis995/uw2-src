/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vga256:vga256/devices/gd54xx/font.h	1.5"

/*
 *	Copyright (c) 1991, 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 */
#ifndef __FONT_H__

#define FONT_COUNT 	8		/* max # of downloadable fonts */
#define FONT_NUMGLYPHS	256		/* max # of glyphs per font */
#define FONT_MAXWIDTH	16		/* max width of a downloadable font */
#define FONT_MAXHEIGHT	16		/* max width of a downloadable font */
#define FONT_MAXSIZE	32		/* max possible converted glyph size */
#define FONT_DB_START	1024*789	/*off-screen address from where the fonts
									  will be downloaded */
#define FONT_MEMORY_SIZE	256*32	/*Maximum memory required to download a
									  a font in off-screen memory*/
#define BLTDATA unsigned short

#define FREE(p)	(p ? free(p) : 0)

typedef struct gd_font {
    int w;		/* width of glyphs in pixels */
    int h;		/* height of glyphs in pixels */
    int ascent;		/* distance from baseline to top of glyph */
	int descent;	/* distance from baseline to bottom of glyph*/
    int lsize;		/* no. of bytes stored for each line of a glyph */
    int size;		/* no. of bytes stored for each glyph */
} gd_font_rec;


#define __FONT_H__
#endif /* __FONT_H__ */
