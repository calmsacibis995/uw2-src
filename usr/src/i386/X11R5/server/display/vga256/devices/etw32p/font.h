/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)vga256:vga256/devices/etw32p/font.h	1.3"

/*
 *	Copyright (c) 1992, 1993 USL
 *	All Rights Reserved 
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF USL
 *	The copyright notice above does not evidence any 
 *	actual or intended publication of such source code.
 *
 */

/*
 * Copyright (c) 1992,1993 Pittsburgh Powercomputing Corporation (PPc).
 * All Rights Reserved.
 */
#define FONT_COUNT 	8		/* max # of downloadable fonts */
#define FONT_NUMGLYPHS	256		/* max # of glyphs perr font */
#define FONT_GLYPH_MAXWIDTH	32		/* max width of a downloadable font */
#define FONT_GLYPH_MAXHEIGHT	32		/* max width of a downloadable font */
#define FONT_MAXSIZE	(8*32)		/* max possible converted glyph size */

#define BLTDATA char

/*
 *  Since non-terminal font support been added, it is necessary that
 *  ascent, descent, lbearing, rbearing of each glyph is stored safely
 *  during download time itself.  
 */
typedef struct etw32p_font {

    int lsize;	     
	/*  
	 *  no. of bytes for a glyphline. For terminal fonts, lsize is
	 *  same for all glyphs.  For non-terminal fonts, lsize holds
	 *  lsize of maximum width font.
	 */

    int size;       
	/*  
	 *  no. of bytes for a glyph. For terminal fonts, size is
	 *  same for all glyphs.  For non-terminal fonts, size holds
	 *  size of maximum width font.
	 */

	int ascent[FONT_NUMGLYPHS];
	int descent[FONT_NUMGLYPHS];
	int lbearing[FONT_NUMGLYPHS];
	int rbearing[FONT_NUMGLYPHS];
	/*
	 *  all above will be used only for non-terminal fonts
	 */
	int w[FONT_NUMGLYPHS];
	/*
	 *  w is width in 'pixel' for a glyph. It is same for all glyphs in 
	 *  terminal fonts and only w[0] contains width.  But non-terminal fonts, 
	 *  each glyph may have different width.  Hence entire w array will
	 *  have necessary information.
	 */

	int is_terminal_font;
	/*
	 *  Flag to distinguish between terminal and non-terminal font.
	 *  if it is non-zero, then it is terminal-font.
	 */

	int lascent;   /* logic ascent for a font.    */
	int max_height;
	/*
	 *  For terminal fonts, it is just ascent+descent value of any glyph.
	 *  For non-terminal fonts, it is  lascent+ldescent for the font.
	 */

    BLTDATA *data;	
	/*
	 *  holds glyph information for a font.
	 */


} etw32p_font_rec;


extern etw32p_font_rec etw32p_fonts[FONT_COUNT];

extern SIBool etw32p_font_check();
extern SIBool etw32p_font_download();
extern SIBool etw32p_font_stplblt();
extern SIBool etw32p_font_free();
extern SIBool etw32p_non_terminal_font_download();
extern SIBool etw32p_non_terminal_font_stplblt();

