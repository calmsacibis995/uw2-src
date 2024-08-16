/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m_font.c	1.6"

/***
 ***	NAME
 ***
 ***		mach_font.c : Font handling code for the MACH display
 ***	library.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "m_font.h"
 ***
 ***	DESCRIPTION
 ***
 ***	Font handling is done in a 3-pronged manner.  All kinds of
 ***	fonts are accepted by the font drawing code -- there are no
 ***	builtin limits on the glyph sizes, number of characters
 ***	in a font etc.  Options exist to control these -- please see
 ***	the option description files.  Right-to-left drawing fonts are
 ***	also handled, though this section of the code has not really
 ***	been fully tested.
 ***
 ***	The first layer of font support consists of downloading the
 ***	glyphs into system memory.  Inversion of the font bits is done
 ***	at this stage to speed up drawing.
 ***
 ***	The second layer of font support consists of using offscreen
 ***	memory as a font cache.  Caching is done on a
 ***	per-glyph/per-use basis in order to maximize usage of
 ***	offscreen memory.  The font blitting is done using ATI style
 ***	blits.
 ***
 ***	The third layer of font support optimizes text drawing for
 ***	terminal fonts -- this uses IBM mode stipple fills and
 ***	attempts to download every glyph of the font into offscreen
 ***	memory at font download time.  Glyphs are kept contiguously
 ***	in this mode.  Font drawing is done using assembler routines
 ***	in this mode.  There are various restrictions on IBM mode text
 ***	drawing which make this mode infeasible for hi-resolution
 ***	modes.
 ***	
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***	SEE ALSO
 ***
 ***	m_opt.gen : option description file for the MACH display
 ***				library.
 ***	g_omm.c : offscreen memory manager.
 ***	
 ***	CAVEATS
 ***
 ***	IBM mode assembler routines for font drawing make use of the
 ***    offsets of fields in a `mach_glyph' structure.
 ***   
 ***	BUGS
 ***
 ***	HISTORY
 ***
 ***/

#ident	"@(#)mach:mach/m_font.c	1.2"

PUBLIC

/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <sidep.h>
#include "stdenv.h"
#include "m_opt.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

/*
 *	Forward declarations.
 */


/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export boolean mach_font_debug = FALSE;	
export boolean mach_font_string_debug = FALSE; /* dump incoming strings */
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

/*
 * Cached glyph information.
 *
 * CAVEAT: assembler text routines for ibm mode drawing use the
 * offsets of the `ibm_mode_planemask' and `ibm_mode_x_coordinate'
 * within a `mach_glyph' structure.  Please change the code if this
 * position of these fields in the structure changes.
 */
struct mach_glyph
{
	/*
	 * Rotated plane mask used for IBM blits.
	 */
	unsigned int ibm_mode_planemask;

	/*
	 * X coordinate of the glyph in IBM mode.
	 */
	int ibm_mode_x_coordinate;
	
	/*
	 * Saved SI glyph information
	 */
	SIGlyphP si_glyph_p;

	/*
	 * number of pixtrans transfers in the entire glyph.
	 */
	int glyph_pixtrans_transfers;
	
	/*
	 * The width of the glyph rounded up to the nearest pixtrans
	 * boundary.  This is specified in pixels and used as destination
	 * width when downloading the glyph into offscreen memory.
	 */
	int glyph_padded_width;
	
	/*
	 * Offscreen memory allocation.
	 */
	struct omm_allocation *offscreen_allocation_p;

	/*
	 * pointer to the start of the inverted bits for this glyph.
	 * (packed to the nearest pixtrans word width).  This points into
	 * the array pointed to by `font_glyph_bits_p' in the font data
	 * structure. 
	 */
	unsigned char *glyph_bits_p;
	
#if (defined(__DEBUG__))
	int stamp;
#else
	int pad;					/* pad to 32 bytes */
#endif
};


/*
 * Font kinds.
 */
#define DEFINE_FONT_TYPES()\
	DEFINE_FONT_TYPE(NULL),\
	DEFINE_FONT_TYPE(TERMINAL_FONT),\
	DEFINE_FONT_TYPE(NON_TERMINAL_FONT),\
	DEFINE_FONT_TYPE(IBM_MODE_TERMINAL_FONT),\
	DEFINE_FONT_TYPE(COUNT)

enum mach_font_kind
{
#define DEFINE_FONT_TYPE(NAME)\
	MACH_FONT_KIND_ ## NAME
		DEFINE_FONT_TYPES()
#undef DEFINE_FONT_TYPE
};


/*
 * Structure of a downloaded font.
 */
struct mach_font
{

	/*
	 * The type of this font.
	 */
	enum mach_font_kind font_kind;
	
	/*
	 * SI's font information
	 */
	SIFontInfo si_font_info;
	
	/*
	 * Pointer to the array of mach_glyph structures.
	 */
	struct mach_glyph *font_glyphs_p;
	
	/*
	 * Pointer to the inverted glyph bits for all the glyphs.
	 */
	unsigned char *font_glyph_bits_p;

	/*
	 * Offscreen allocation for IBM mode font rendering.
	 */
	struct omm_allocation *ibm_mode_allocation_p;
	
	/*
	 * The draw functions for this font.
	 */
	void (*draw_ati_glyphs)
		(struct mach_font *font_p,
		 short *glyph_start_p, 
		 short *glyph_end_p,
		 int x_origin,
		 int y_origin, 
		 int width,
		 int draw_flags);

	void (*draw_ibm_glyphs)
		(struct mach_font *font_p,
		 short *glyph_start_p, 
		 short *glyph_end_p,
		 int x_origin,
		 int y_origin, 
		 int width,
		 int draw_flags);

#if (defined(__DEBUG__))
	int stamp;
#endif
};

/*
 * The state of the font handling code.
 */

struct mach_font_state
{
	/*
	 * Parameters controlling the downloadability and offscreen
	 * cacheability of this font.
	 */
	int max_number_of_downloadable_fonts;
	int max_supported_glyph_width;
	int max_supported_glyph_height;

	/*
	 * List of downloaded fonts.
	 */
	struct mach_font **font_list_pp;
	
#if (defined(__DEBUG__))
	int stamp;
#endif
};

/***
 ***	Includes.
 ***/

#include "g_omm.h"
#include "m_state.h"
#include "m_gs.h"

/***
 ***	Constants.
 ***/

/*
 * Flags controlling the drawing.
 */
#define MACH_FONT_DO_TRANSPARENT_STIPPLING 		(0x0001 << 0U)
#define MACH_FONT_DO_OPAQUE_STIPPLING			(0x0001 << 1U)
#define MACH_FONT_USE_IBM_BLITS					(0x0001 << 2U)

#if (defined(__DEBUG__))
STATIC const char *const mach_font_kind_to_dump[] = 
{
#define DEFINE_FONT_TYPE(NAME)\
	# NAME
	DEFINE_FONT_TYPES()
#undef DEFINE_FONT_TYPE
};
#endif

#if (defined(__DEBUG__))
#define MACH_FONT_DEBUG_STAMP 		0xdeadbeef
#endif

#define MACH_GLYPH_STAMP\
	(('M' << 0) + ('A' << 1) + ('C' << 2) + ('H' << 3) +\
	 ('_' << 4) + ('G' << 5) + ('L' << 6) + ('Y' << 7) +\
	 ('P' << 8) + ('H' << 9) + ('_' << 10) + ('S' << 11) +\
	 ('T' << 12) + ('A' << 13) + ('M' << 14))

#define MACH_FONT_STAMP\
	(('M' << 0) + ('A' << 1) + ('C' << 2) + ('H' << 3) +\
	 ('_' << 4) + ('F' << 5) + ('O' << 6) + ('N' << 7) +\
	 ('T' << 8) + ('_' << 9) + ('S' << 10) + ('T' << 11) +\
	 ('A' << 12) + ('M' << 13))

#define MACH_FONT_STATE_STAMP\
	(('M' << 0) + ('A' << 1) + ('C' << 2) + ('H' << 3) +\
	 ('_' << 4) + ('F' << 5) + ('O' << 6) + ('N' << 7) +\
	 ('T' << 8) + ('_' << 9) + ('S' << 10) + ('T' << 11) +\
	 ('A' << 12) + ('T' << 13) + ('E' << 14) + ('_' << 15) +\
	 ('S' << 16) + ('T' << 17) + ('A' << 18) + ('M' << 19))

#define MACH_POLYTEXT_DEPENDENCIES\
	(MACH_INVALID_FOREGROUND_COLOR |\
	 MACH_INVALID_BACKGROUND_COLOR |\
	 MACH_INVALID_FG_ROP |\
	 MACH_INVALID_CLIP |\
	 MACH_INVALID_WRT_MASK)

#define MACH_IMAGETEXT_DEPENDENCIES\
	(MACH_INVALID_FOREGROUND_COLOR |\
	 MACH_INVALID_BACKGROUND_COLOR |\
	 MACH_INVALID_CLIP |\
	 MACH_INVALID_WRT_MASK)

/***
 ***	Macros.
 ***/

#define MACH_CURRENT_FONT_STATE_DECLARE()\
	struct mach_font_state *font_state_p =\
		screen_state_p->font_state_p

/*
 * Download a glyph onto screen.  This macro assumes that ATi context
 * has been estabilished.
 */
#define MACH_FONT_DOWNLOAD_GLYPH(SCREEN_STATE_P, GLYPH_P, ALLOCATION_P)\
{																	\
	ASSERT((SCREEN_STATE_P)->current_graphics_engine_mode ==		\
		   MACH_GE_MODE_ATI_MODE);									\
    ASSERT((ALLOCATION_P)->width >= (GLYPH_P)->glyph_padded_width); \
    ASSERT((ALLOCATION_P)->height >= 			                    \
		   (GLYPH_P)->si_glyph_p->SFascent + 	                    \
		   (GLYPH_P)->si_glyph_p->SFdescent);                       \
	MACH_WAIT_FOR_FIFO(6);											\
	outw(MACH_REGISTER_WRT_MASK, (ALLOCATION_P)->planemask);		\
	outw(MACH_REGISTER_CUR_X, (ALLOCATION_P)->x);					\
	outw(MACH_REGISTER_CUR_Y, (ALLOCATION_P)->y);					\
	outw(MACH_REGISTER_DEST_X_START, (ALLOCATION_P)->x);			\
	outw(MACH_REGISTER_DEST_X_END, (ALLOCATION_P)->x +				\
		 (GLYPH_P)->glyph_padded_width);							\
	outw(MACH_REGISTER_DEST_Y_END, (ALLOCATION_P)->y +				\
		 (GLYPH_P)->si_glyph_p->SFascent +							\
		 (GLYPH_P)->si_glyph_p->SFdescent);							\
	MACH_WAIT_FOR_FIFO(16);											\
	MACH_BULK_TRANSFER_TO_GRAPHICS_ENGINE(							\
		  (SCREEN_STATE_P)->pixtrans_register,						\
		  (GLYPH_P)->glyph_pixtrans_transfers,						\
		  (GLYPH_P)->glyph_bits_p,									\
		  ((SCREEN_STATE_P)->pixtrans_width >> 4),					\
		  (*(SCREEN_STATE_P)->screen_write_bits_p));				\
	MACH_STATE_SET_FLAGS(SCREEN_STATE_P, MACH_INVALID_WRT_MASK);	\
}

/*
 * Render a font glyph using ati extended blits from the offscreen
 * font cache to the screen.
 */
#define MACH_FONT_ATI_RENDER_NON_TERMINAL_GLYPH_FROM_SCREEN(GLYPHP,\
			ALLOCATION_P, XLEFT, GLYPHWIDTH, YTOP, YBOTTOM)\
{																		\
	 MACH_WAIT_FOR_FIFO(10);											\
	 outw(MACH_REGISTER_RD_MASK, (ALLOCATION_P)->planemask);			\
	 outw(MACH_REGISTER_SRC_X, (ALLOCATION_P)->x);						\
	 outw(MACH_REGISTER_SRC_Y, (ALLOCATION_P)->y);						\
	 outw(MACH_REGISTER_SRC_X_START, (ALLOCATION_P)->x);				\
	 outw(MACH_REGISTER_SRC_X_END, (ALLOCATION_P)->x + (GLYPHWIDTH));	\
	 outw(MACH_REGISTER_CUR_X, (XLEFT));								\
	 outw(MACH_REGISTER_DEST_X_START, (XLEFT));							\
	 outw(MACH_REGISTER_DEST_X_END, (XLEFT) + (GLYPHWIDTH));			\
	 outw(MACH_REGISTER_CUR_Y, (YTOP));									\
	 outw(MACH_REGISTER_DEST_Y_END, (YBOTTOM));							\
}

/*
 * Render a font glyph from system memory onto screen using ATI
 * extended blit mechanisms.
 */
#define MACH_FONT_ATI_RENDER_NON_TERMINAL_GLYPH_FROM_MEMORY(SCREENSTATEP,\
			GLYPHP, XLEFT, YTOP, YBOTTOM)\
{																\
	 ASSERT((SCREENSTATEP)->current_graphics_engine_mode ==	\
			MACH_GE_MODE_ATI_MODE);								\
	 MACH_WAIT_FOR_FIFO(5);										\
	 outw(MACH_REGISTER_CUR_X, (XLEFT));						\
	 outw(MACH_REGISTER_DEST_X_START, (XLEFT));					\
	 outw(MACH_REGISTER_DEST_X_END, (XLEFT) +					\
		  (GLYPHP)->glyph_padded_width);						\
	 outw(MACH_REGISTER_CUR_Y, YTOP);							\
	 outw(MACH_REGISTER_DEST_Y_END, YBOTTOM);					\
	 MACH_WAIT_FOR_FIFO(16);									\
	 MACH_BULK_TRANSFER_TO_GRAPHICS_ENGINE(						\
		  (SCREENSTATEP)->pixtrans_register,					\
		  (GLYPHP)->glyph_pixtrans_transfers,					\
		  (GLYPHP)->glyph_bits_p,								\
		  ((SCREENSTATEP)->pixtrans_width >> 4),				\
		  (*(SCREENSTATEP)->screen_write_bits_p));				\
}

/*
 * Render a terminal glyph from an offscreen cache location onto
 * screen.
 */
#define MACH_FONT_ATI_RENDER_TERMINAL_GLYPH_FROM_SCREEN(ALLOCATION_P,\
			XORIGIN, WIDTH, YTOP, YBOTTOM)\
{																	\
	 MACH_WAIT_FOR_FIFO(10);										\
	 outw(MACH_REGISTER_RD_MASK, (ALLOCATION_P)->planemask);		\
	 outw(MACH_REGISTER_SRC_X, (ALLOCATION_P)->x);					\
	 outw(MACH_REGISTER_SRC_Y, (ALLOCATION_P)->y);					\
	 outw(MACH_REGISTER_SRC_X_START, (ALLOCATION_P)->x);			\
	 outw(MACH_REGISTER_SRC_X_END, (ALLOCATION_P)->x + (WIDTH));	\
	 outw(MACH_REGISTER_CUR_X, (XORIGIN));							\
	 outw(MACH_REGISTER_DEST_X_START, (XORIGIN));					\
	 outw(MACH_REGISTER_DEST_X_END, (XORIGIN) + (WIDTH));			\
	 outw(MACH_REGISTER_CUR_Y, (YTOP));								\
	 outw(MACH_REGISTER_DEST_Y_END, (YBOTTOM));						\
}

/*
 * Render a terminal glyph from system memory onto screen.
 */
#define MACH_FONT_ATI_RENDER_TERMINAL_GLYPH_FROM_MEMORY(SCREENSTATEP,\
			GLYPHP, XORIGIN, YTOP, YBOTTOM)                         \
{																	\
	 int glyph_end_x = 												\
		 (XORIGIN) + (GLYPHP)->si_glyph_p->SFwidth;					\
	 ASSERT((SCREENSTATEP)->current_graphics_engine_mode ==			\
			MACH_GE_MODE_ATI_MODE);									\
	 if (glyph_end_x > (XORIGIN))                                   \
	 {																\
		 MACH_WAIT_FOR_FIFO(1);										\
		 if (glyph_end_x <=											\
			 (SCREENSTATEP)->generic_state.screen_clip_right)		\
		 {															\
			 outw(MACH_REGISTER_EXT_SCISSOR_R, 						\
				  glyph_end_x);										\
		 }                                                          \
		 else                                                       \
		 {                                                          \
			 outw(MACH_REGISTER_EXT_SCISSOR_R,                      \
				  (SCREENSTATEP)->generic_state.screen_clip_right); \
		 }                                                          \
	 }                                                              \
 	 MACH_WAIT_FOR_FIFO(5);											\
	 outw(MACH_REGISTER_CUR_X, XORIGIN);							\
	 outw(MACH_REGISTER_DEST_X_START, XORIGIN);						\
	 outw(MACH_REGISTER_DEST_X_END, XORIGIN +						\
		  (GLYPHP)->glyph_padded_width);							\
	 outw(MACH_REGISTER_CUR_Y, YTOP);								\
	 outw(MACH_REGISTER_DEST_Y_END, YBOTTOM);						\
	 MACH_WAIT_FOR_FIFO(16);										\
	 MACH_BULK_TRANSFER_TO_GRAPHICS_ENGINE(							\
		  (SCREENSTATEP)->pixtrans_register,						\
		  (GLYPHP)->glyph_pixtrans_transfers,						\
		  (GLYPHP)->glyph_bits_p,									\
		  ((SCREENSTATEP)->pixtrans_width >> 4),					\
		  (*(SCREENSTATEP)->screen_write_bits_p));					\
}

/***
 ***	Functions.
 ***/

/*
 * Download a glyph into offscreen memory.
 */

STATIC SIBool
mach_font_is_font_downloadable(
	SIint32 font_index, 
	SIFontInfoP fontinfo_p)
{
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();

	MACH_CURRENT_FONT_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(MACH_FONT_STATE, font_state_p));
	
#if (defined(__DEBUG__))
	if (mach_font_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_font_is_font_downloadable)\n"
"{\n"
"\tfont_index = %ld\n"
"\tfontinfo_p = %p\n"
"}\n",
					   font_index, (void *) fontinfo_p);
	}
#endif
	

	/*
	 * Check if the index is in bounds.
	 * and
	 * check if the font is of a size suitable for downloading.
	 */
	if ((font_index < 0 || font_index >=
		font_state_p->max_number_of_downloadable_fonts) ||
		((fontinfo_p->SFmax.SFascent + fontinfo_p->SFmax.SFdescent) >
		  font_state_p->max_supported_glyph_height ) ||
		((fontinfo_p->SFmax.SFrbearing - fontinfo_p->SFmin.SFlbearing) >
		  font_state_p->max_supported_glyph_width))
	{
#if (defined(__DEBUG__))
		if (mach_font_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_font_is_font_downloadable)\n"
"{\n"
"\t# Failing because index/font size is out of bounds.\n"
"}\n");
		}
#endif
		return (SI_FAIL);
	}

	/*
	 * Do the following checks also.
	 * Option says dont draw terminal fonts and if we are downloaded a
	 * terminal font or option says dont draw non terminal font and we
	 * are downloaded a nonterminal font ( any font with SFterminal not 
	 * set) return failure.
	 */
	if ((!(screen_state_p->options_p->fontdraw_options & 
	 	   MACH_OPTIONS_FONTDRAW_OPTIONS_DRAW_TERMINAL_FONTS) &&
		 (fontinfo_p->SFflag & SFTerminalFont)) ||
	    (!(screen_state_p->options_p->fontdraw_options & 
	 	   MACH_OPTIONS_FONTDRAW_OPTIONS_DRAW_NON_TERMINAL_FONTS) &&
		 (!(fontinfo_p->SFflag & SFTerminalFont))) ||
		((screen_state_p->options_p->
		  max_number_of_glyphs_in_downloadable_font > 0) && 
		 (fontinfo_p->SFnumglyph > screen_state_p->options_p->
		  max_number_of_glyphs_in_downloadable_font)))
	{
#if (defined(__DEBUG__))
		if (mach_font_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_font_is_font_downloadable)\n"
"{\n"
"\t# Failing because font-type not downloadable by option.\n"
"\tfont type = %s\n"
"\tnumber of glyphs = %ld\n"
"}\n",
		(fontinfo_p->SFflag & SFTerminalFont) ? "Terminal Font.\n" :
						   "NonTerminal Font.\n",
						   fontinfo_p->SFnumglyph);
		}
#endif
		return (SI_FAIL);
	}

	return (SI_SUCCEED);
}


STATIC void
mach_font_draw_ibm_terminal_glyphs(
	struct mach_font *font_p,
	short *glyph_list_start_p,
	short *glyph_list_end_p,
	int x_origin,
	int y_origin,
	int string_width,
	int draw_flags)
{

	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	const struct mach_glyph *const font_glyphs_p =
		font_p->font_glyphs_p;
	const struct omm_allocation *const allocation_p =
		font_p->ibm_mode_allocation_p;
	
	const int width = font_p->si_font_info.SFmax.SFwidth;
	register const int y_top = y_origin - font_p->si_font_info.SFmax.SFascent;
	const int y_bottom = y_origin +
		font_p->si_font_info.SFmax.SFdescent; /* exclusive */

	const unsigned short blit_command = 
		(MACH_CMD_BLIT_CMD | 
		 MACH_CMD_YPOS |
		 MACH_CMD_YMAJOR |
		 MACH_CMD_XPOS |
		 MACH_CMD_DRAW |
		 MACH_CMD_DATA_WIDTH_USE_16 |
		 MACH_CMD_PIXEL_MODE_NIBBLE |
		 MACH_CMD_WRITE);

	unsigned short current_planemask = 0U;

	register int current_x = x_origin;

#if (defined(EXTRA_FUNCTIONALITY))
	int previous_blit_width;
	int blit_width;
#endif /* EXTRA_FUNCTIONALITY */

	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_FONT, font_p));
	ASSERT(font_p->font_kind == MACH_FONT_KIND_IBM_MODE_TERMINAL_FONT);
	ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION, allocation_p));

	ASSERT(allocation_p->x >= 0 && 
		   allocation_p->x + allocation_p->width <=
		   screen_state_p->generic_state.screen_physical_width);
	ASSERT(allocation_p->y >= 0 &&
		   allocation_p->y + allocation_p->height <= 
		   screen_state_p->generic_state.screen_physical_height);
		
	ASSERT(allocation_p->height >= 0 && allocation_p->width >= 0);
			

#if (defined(__DEBUG__))
	if (mach_font_debug)
	{
		(void) fprintf(debug_stream_p,
"# IBM terminal glyphs.\n"
"{\n"
"\tfont_p = %p\n"
"\tglyph_list_start_p = %p\n"
"\tglyph_list_end_p = %p\n"
"\tx_origin = %d\n"
"\ty_origin = %d\n"
"\tstring_width = %d\n"
"\tdraw_flags = 0x%x\n"
"\tglyph width = %d\n"
"\tglyph height = %d\n"
"\t{\n"
"\t\tallocation_x = %d\n"
"\t\tallocation_y = %d\n"
"\t\tallocation_width = %d\n"
"\t\tallocation_height = %d\n"
"\t\tallocation_planemask = 0x%x\n"
"\t}\n",
					   (void *) font_p, (void *) glyph_list_start_p,
					   (void *) glyph_list_end_p, x_origin, y_origin,
					   string_width, draw_flags, width, y_bottom - y_top,
					   allocation_p->x, allocation_p->y,
					   allocation_p->width, allocation_p->height,
					   allocation_p->planemask);
	}
#endif

	ASSERT(screen_state_p->current_graphics_engine_mode ==
		   MACH_GE_MODE_IBM_MODE);

	MACH_WAIT_FOR_FIFO(4);
	
	/*
	 * Set the CUR_Y parameter.
	 */
	outw(MACH_REGISTER_CUR_Y, allocation_p->y);

	/*
	 * Set width and height of each glyph.
	 */
	outw(MACH_REGISTER_MAJ_AXIS_PCNT, width - 1);

	outw(MACH_REGISTER_MULTI_FN, 
		 MACH_MF_RECT_HEIGHT | 
		 (y_bottom - y_top - 1) & MACH_MF_VALUE);

	/*
	 * Set the pixel control value to monochrome read.
	 */
	outw(MACH_REGISTER_MULTI_FN,
		 MACH_MF_ALU_CONFIG |
		 (MACH_MF_ALU_CONFIG_MONO_SRC_BLIT & MACH_MF_VALUE));

	do
	{
		register const struct mach_glyph *const glyph_p =
			&(font_glyphs_p[*glyph_list_start_p]);
		
		ASSERT(IS_OBJECT_STAMPED(MACH_GLYPH, glyph_p));
		ASSERT(glyph_p->offscreen_allocation_p == NULL);
		
#if (defined(__DEBUG__))
		if (mach_font_debug)
		{
			(void) fprintf(debug_stream_p,
"\t{\n"
"\t\tglyph_index = %hd\n"
"\t\tibm_mode_x = %d\n"
"\t\tibm_mode_planemask = 0x%x\n"
"\t}\n",
						   *glyph_list_start_p,
						   glyph_p->ibm_mode_x_coordinate,
						   glyph_p->ibm_mode_planemask);
		}
#endif


		ASSERT(current_x + width >=
			   screen_state_p->generic_state.screen_clip_left &&
			   current_x <=
			   screen_state_p->generic_state.screen_clip_right);
		
		ASSERT(glyph_p->ibm_mode_planemask != 0 &&
			   !(glyph_p->ibm_mode_planemask & 
				 (glyph_p->ibm_mode_planemask - 1)));

		/*
		 * Update the plane mask if we have switched planes.
		 */

		if (glyph_p->ibm_mode_planemask != current_planemask)
		{
			MACH_WAIT_FOR_FIFO(5);
			outw(MACH_REGISTER_RD_MASK, 
				 current_planemask = glyph_p->ibm_mode_planemask);
			outw(MACH_REGISTER_CUR_X, glyph_p->ibm_mode_x_coordinate);
								/* SRC_X */
			outw(MACH_REGISTER_DIASTP, current_x); /* DEST_X */
			
			outw(MACH_REGISTER_AXSTP, y_top); /* DEST_Y */
			outw(MACH_REGISTER_CMD, blit_command); /* DRAW INITIATOR */
		}
		else
		{
			MACH_WAIT_FOR_FIFO(4);
		
			outw(MACH_REGISTER_CUR_X, glyph_p->ibm_mode_x_coordinate);
								/* SRC_X */
			outw(MACH_REGISTER_DIASTP, current_x); /* DEST_X */
			outw(MACH_REGISTER_AXSTP, y_top); /* DEST_Y */
			outw(MACH_REGISTER_CMD, blit_command); /* DRAW INITIATOR */
		}

		current_x += width;
		
	} while (++glyph_list_start_p < glyph_list_end_p);

#if (defined(EXTRA_FUNCTIONALITY))
	/*
	 * A primitive text caching algorithm : the glyph cache is created
	 * statically at font download time (as opposed to dynamically
	 * based on the actual glyph usage).
	 *
	 * Text drawing involves looking ahead to see how much of the
	 * cached text fits into this text and blitting a large area
	 * instead of glyph by glyph.
	 *
	 * For the layout chosen at glyph download time : various
	 * benchmarks like xbench and x11perf speed up tremendously.
	 * However since the caching is unnatural real-world
	 * applications are unlikely to benefit, so this code should not
	 * go into production servers!
	 *
	 * Later, hopefully there will be a proper text caching algorithm
	 * in place and we will be able to offer the users some real
	 * benefits!
	 */

	outw(MACH_REGISTER_RD_MASK, current_planemask =
		font_glyphs_p[*glyph_list_start_p].ibm_mode_planemask);

	previous_blit_width = -1;

	do
	{
		register const struct mach_glyph *const glyph_p =
			&(font_glyphs_p[*glyph_list_start_p]);
		SIint16 *tmp_p = glyph_list_start_p + 1;
		
		blit_width = width;
		
		while ((tmp_p < glyph_list_end_p) &&
			   ((*(tmp_p - 1) + 1) == *tmp_p) &&
			   (font_glyphs_p[*tmp_p].ibm_mode_planemask ==
				current_planemask))
		{
			tmp_p ++;
			blit_width += width;
		}
		
		if (previous_blit_width != blit_width)
		{
			MACH_WAIT_FOR_FIFO(1);
			outw(MACH_REGISTER_MAJ_AXIS_PCNT, 
				 (previous_blit_width = blit_width) - 1);
		}
		
		/*
		 * Update the plane mask if we have switched planes.
		 */

		if (glyph_p->ibm_mode_planemask !=
			current_planemask)
		{
			MACH_WAIT_FOR_FIFO(6);
			outw(MACH_REGISTER_RD_MASK,
				 current_planemask = glyph_p->ibm_mode_planemask);
			outw(MACH_REGISTER_CUR_X, glyph_p->ibm_mode_x_coordinate);
								/* SRC_X */
			outw(MACH_REGISTER_DIASTP, current_x); /* DEST_X */
			
			outw(MACH_REGISTER_AXSTP, y_top); /* DEST_Y */
			outw(MACH_REGISTER_CMD, blit_command); /* DRAW INITIATOR */
		}
		else
		{
			MACH_WAIT_FOR_FIFO(4);
		
			outw(MACH_REGISTER_CUR_X, glyph_p->ibm_mode_x_coordinate);
								/* SRC_X */
			outw(MACH_REGISTER_DIASTP, current_x); /* DEST_X */
			outw(MACH_REGISTER_AXSTP, y_top); /* DEST_Y */
			outw(MACH_REGISTER_CMD, blit_command); /* DRAW INITIATOR */
		}
		
		current_x += blit_width;

		glyph_list_start_p = tmp_p;
		
	} while (glyph_list_start_p < glyph_list_end_p);

#endif /* EXTRA_FUNCTIONALITY */
	
#if (defined(__DEBUG__))
	if (mach_font_debug)
	{
		(void) fprintf(debug_stream_p,
					   "}\n");
		
	}
#endif

	/*
	 * Restore the pixel cntl register and the read mask register.
	 */
	MACH_WAIT_FOR_FIFO(2);
	outw(MACH_REGISTER_MULTI_FN,
		 (MACH_MF_ALU_CONFIG | 0));
	outw(MACH_REGISTER_RD_MASK,
		 screen_state_p->register_state.rd_mask);
	
	ASSERT(!MACH_IS_IO_ERROR());

}


/*
 * Drawing non terminal glyphs, some of which are not in offscreen
 * memory. 
 */
STATIC void
mach_font_draw_ati_non_terminal_glyphs(
	struct mach_font *font_p,
	short *glyph_list_start_p,
	short *glyph_list_end_p,
	int x_origin,
	int y_origin,
	int string_width,
	int draw_flags)
{

	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	struct mach_glyph *const font_glyphs_p =
		font_p->font_glyphs_p;
	
	/*
	 * dp config value for screen to screen stippling.
	 */
	const unsigned short dp_config_screen_to_screen =
		screen_state_p->dp_config_flags |
		(MACH_DP_CONFIG_WRITE |
		 MACH_DP_CONFIG_ENABLE_DRAW |
		 MACH_DP_CONFIG_MONO_SRC_BLIT |
		 MACH_DP_CONFIG_READ_MODE_MONO_DATA |
		 MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		 MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR);

	/*
	 * dp config value for memory to screen stippling.
	 */
	const unsigned short dp_config_memory_to_screen =
		screen_state_p->dp_config_flags |
		(MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		 MACH_DP_CONFIG_MONO_SRC_HOST |
		 MACH_DP_CONFIG_ENABLE_DRAW |
		 MACH_DP_CONFIG_READ_MODE_MONO_DATA |
		 MACH_DP_CONFIG_WRITE |
		 MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR);

	ASSERT(font_p->font_kind ==
		   MACH_FONT_KIND_NON_TERMINAL_FONT);
	
	ASSERT(screen_state_p->current_graphics_engine_mode ==
		   MACH_GE_MODE_ATI_MODE);
	
#if (defined(__DEBUG__))
	if (mach_font_debug)
	{
		(void) fprintf(debug_stream_p, "# ATI non-terminal glyphs.\n");
	}
#endif

	MACH_STATE_SET_BG_ROP(screen_state_p,
						  MACH_MIX_FN_LEAVE_ALONE);
		

	/*
	 * Fill the background rectangle if the stippling mode is opaque
	 * stippling.
	 */
	if (draw_flags & MACH_FONT_DO_OPAQUE_STIPPLING)
	{
	
		const unsigned short dp_config_rect_fill =
			screen_state_p->dp_config_flags |
			(MACH_DP_CONFIG_MONO_SRC_ONE |
			 MACH_DP_CONFIG_WRITE |
			 MACH_DP_CONFIG_ENABLE_DRAW |
			 MACH_DP_CONFIG_FG_COLOR_SRC_BKGD_COLOR);
	
		MACH_STATE_SET_DP_CONFIG(screen_state_p,
								 dp_config_rect_fill);

		/*
		 * Fill a rectangle of coordinates 
		 * UL (x, y - font_p->si_font_info.SFlascent)
		 * LR (x + string_width, y +
		 *		font_p->si_font_info.SFldescent)
		 */
		MACH_WAIT_FOR_FIFO(5);
		outw(MACH_REGISTER_CUR_X, x_origin);
		outw(MACH_REGISTER_DEST_X_START, x_origin);
		outw(MACH_REGISTER_DEST_X_END, x_origin + string_width);
		outw(MACH_REGISTER_CUR_Y, 
			 y_origin - font_p->si_font_info.SFlascent);
		outw(MACH_REGISTER_DEST_Y_END,
			 y_origin + font_p->si_font_info.SFldescent);
		
		ASSERT(!MACH_IS_IO_ERROR());
	}

	/*
	 * All glyphs are drawn top to bottom.
	 */
	MACH_WAIT_FOR_FIFO(1);
	outw(MACH_REGISTER_SRC_Y_DIR, 1);
	
	/*
	 * Draw each glyph.
	 */
	do
	{
		const int glyph_index = *glyph_list_start_p;
		
		register struct mach_glyph *const glyph_p = 
			font_glyphs_p + glyph_index;
		
		register int x_left = 
			x_origin + glyph_p->si_glyph_p->SFlbearing; /* inclusive */
		register int glyph_width  = 
			glyph_p->si_glyph_p->SFrbearing - 
			glyph_p->si_glyph_p->SFlbearing; 
								/* exclusive */
		register int y_top = 
			y_origin - glyph_p->si_glyph_p->SFascent;	/* inclusive */
		register int y_bottom = 
			y_origin + glyph_p->si_glyph_p->SFdescent; /* exclusive */
		
		ASSERT(IS_OBJECT_STAMPED(MACH_GLYPH, glyph_p));

		/* 
		 * at least one pixel should be visible : we can't make a
		 * similar check for the y coordinates as there could be a
		 * individual characters which don't get to be seen.
		 */		
		ASSERT((x_left + glyph_width >
				screen_state_p->generic_state.screen_clip_left) &&
			   (x_left <=
				screen_state_p->generic_state.screen_clip_right));
		
#if (defined(__DEBUG__))
		if (mach_font_debug)
		{
			(void) fprintf(debug_stream_p,
"\t{\n"
"\t\tglyph_index = %d\n"
"\t\tx_origin = %d\n"
"\t\tx_left = %d\n"
"\t\tglyph_width = %d\n"
"\t\ty_top = %d\n"
"\t\ty_bottom = %d\n",
						   glyph_index, x_origin, x_left, glyph_width,
						   y_top, y_bottom);
		}
#endif

		/*
		 * If this glyph is present in offscreen memory, we can
		 * stipple using a screen to screen stipple blit.
		 */
		if (OMM_LOCK(glyph_p->offscreen_allocation_p))
		{
			/*
			 * The glyph is present in offscreen memory,
			 */
			struct omm_allocation *allocation_p =
				glyph_p->offscreen_allocation_p;

#if (defined(__DEBUG__))
			if (mach_font_debug)
			{
				(void) fprintf(debug_stream_p,
"\t\t# Offscreen memory\n"
"\t\tallocation_p = %p\n"
"\t\toffscreen_x = %d\n"
"\t\toffscreen_y = %d\n"
"\t\toffscreen_planemask = 0x%x\n",
							   (void *) allocation_p,
							   allocation_p->x, allocation_p->y,
							   allocation_p->planemask);
				
			}
#endif
			
			ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION, allocation_p));
			
		
			ASSERT(allocation_p->x >= 0 && 
				   allocation_p->x + allocation_p->width <=
				   screen_state_p->generic_state.screen_physical_width);
			ASSERT(allocation_p->y >= 0 &&
				   allocation_p->y + allocation_p->height <= 
				   (DEFAULT_MACH_MAX_ATI_COORDINATE + 1)); /* for now */
		
			MACH_STATE_SET_DP_CONFIG(screen_state_p,
									 dp_config_screen_to_screen);
			
			MACH_FONT_ATI_RENDER_NON_TERMINAL_GLYPH_FROM_SCREEN(glyph_p, 
				allocation_p, x_left, glyph_width, y_top, y_bottom);

			OMM_UNLOCK(allocation_p);
			
		}
		else
		{
			/*
			 * This glyph is not present in offscreen memory : 
			 * blit using the pixtrans registers.
			 */
#if (defined(__DEBUG__))
			if (mach_font_debug)
			{
				(void) fprintf(debug_stream_p,
"\t\t# Stippling through the pixtrans register.\n");
			}
#endif

			/*
			 * Reprogram DP CONFIG if necessary.
			 */
			MACH_STATE_SET_DP_CONFIG(screen_state_p,
									 dp_config_memory_to_screen);
			
			MACH_FONT_ATI_RENDER_NON_TERMINAL_GLYPH_FROM_MEMORY(screen_state_p,
					glyph_p, x_left, y_top, y_bottom);
			
			
		}

#if (defined(__DEBUG__))
		if (mach_font_debug)
		{
			(void) fprintf(debug_stream_p, "\t}\n");
		}
#endif
		
		/*
		 * Adjust the origin for the next glyph
		 */
		x_origin += glyph_p->si_glyph_p->SFwidth;

	} while (++glyph_list_start_p < glyph_list_end_p);
	
	ASSERT(!MACH_IS_IO_ERROR());

	MACH_STATE_SET_FLAGS(screen_state_p, 
						 (MACH_INVALID_RD_MASK |
						  MACH_INVALID_PATTERN_REGISTERS));

	/*
	 * Restore the read mask.
	 */

	MACH_WAIT_FOR_FIFO(1);
	outw(MACH_REGISTER_RD_MASK,
		 screen_state_p->register_state.rd_mask);
	
}

STATIC void
mach_font_draw_ati_terminal_glyphs(
	struct mach_font *font_p,
	short *glyph_list_start_p,
	short *glyph_list_end_p,
	int x_origin,
	int y_origin,
	int string_width,
	int draw_flags)
{

	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	struct mach_glyph *const font_glyphs_p =
		font_p->font_glyphs_p;
	
	/*
	 * dp config value for screen to screen stippling, 
	 */
	const unsigned short dp_config_screen_to_screen =
		screen_state_p->dp_config_flags |
		(MACH_DP_CONFIG_WRITE |
		 MACH_DP_CONFIG_ENABLE_DRAW |
		 MACH_DP_CONFIG_MONO_SRC_BLIT |
		 MACH_DP_CONFIG_READ_MODE_MONO_DATA |
		 MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		 MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR);

	/*
	 * dp config value for memory to screen stippling.
	 */
	const unsigned short dp_config_memory_to_screen =
		screen_state_p->dp_config_flags |
		(MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		 MACH_DP_CONFIG_MONO_SRC_HOST |
		 MACH_DP_CONFIG_ENABLE_DRAW |
		 MACH_DP_CONFIG_READ_MODE_MONO_DATA |
		 MACH_DP_CONFIG_WRITE |
		 MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR);


	const int width = font_p->si_font_info.SFmax.SFwidth;
	const int y_top = y_origin - font_p->si_font_info.SFmax.SFascent;
	const int y_bottom = y_origin +
		font_p->si_font_info.SFmax.SFdescent; /* exclusive */

	ASSERT(font_p->font_kind ==
		   MACH_FONT_KIND_TERMINAL_FONT);
	
	ASSERT(screen_state_p->current_graphics_engine_mode ==
		   MACH_GE_MODE_ATI_MODE);

	ASSERT((y_top <=
			screen_state_p->generic_state.screen_clip_bottom) &&
		   (y_bottom >
			screen_state_p->generic_state.screen_clip_top));
		
#if (defined(__DEBUG__))
	if (mach_font_debug)
	{
		(void) fprintf(debug_stream_p, "# ATI terminal glyphs.\n");
	}
#endif

	/*
	 * All glyphs are drawn top to bottom.
	 */
	MACH_WAIT_FOR_FIFO(1);
	outw(MACH_REGISTER_SRC_Y_DIR, 1);
	
	/*
	 * Draw each glyph.
	 */
	do
	{
		const int glyph_index = *glyph_list_start_p;
		
		register struct mach_glyph *const glyph_p = 
			font_glyphs_p + glyph_index;
		
		ASSERT(IS_OBJECT_STAMPED(MACH_GLYPH, glyph_p));
		
		/* 
		 * at least one pixel should be visible 
		 */		
		ASSERT((x_origin + width >
				screen_state_p->generic_state.screen_clip_left) &&
			   (x_origin <=
				screen_state_p->generic_state.screen_clip_right));
		
#if (defined(__DEBUG__))
		if (mach_font_debug)
		{
			(void) fprintf(debug_stream_p,
						   "\t{\n"
						   "\t\tglyph_index = %d\n",
						   glyph_index);
		}
#endif

		
		/*
		 * If this glyph is present in offscreen memory, we can
		 * stipple using a screen to screen stipple blit.
		 */
		if (OMM_LOCK(glyph_p->offscreen_allocation_p))
		{
			/*
			 * The glyph is present in offscreen memory,
			 */
			struct omm_allocation *allocation_p =
				glyph_p->offscreen_allocation_p;

#if (defined(__DEBUG__))
			if (mach_font_debug)
			{
				(void) fprintf(debug_stream_p,
"\t\t# Offscreen memory\n"
"\t\tallocation_p = %p\n"
"\t\toffscreen_x = %d\n"
"\t\toffscreen_y = %d\n"
"\t\toffscreen_planemask = 0x%x\n",
							   (void *) allocation_p,
							   allocation_p->x, allocation_p->y,
							   allocation_p->planemask);
				
			}
#endif
			
		
			ASSERT(allocation_p->x >= 0 && 
				   allocation_p->x + allocation_p->width <=
				   screen_state_p->generic_state.screen_physical_width);
			ASSERT(allocation_p->y >= 0 &&
				   allocation_p->y + allocation_p->height <= 
				   (DEFAULT_MACH_MAX_ATI_COORDINATE + 1)); /* for now */
			ASSERT(allocation_p->height >= 0 && allocation_p->width >= 0);
			
			MACH_STATE_SET_DP_CONFIG(screen_state_p,
									 dp_config_screen_to_screen);
			
			ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION, allocation_p));
			
			MACH_FONT_ATI_RENDER_TERMINAL_GLYPH_FROM_SCREEN(allocation_p, 
					x_origin, width, y_top,	y_bottom);
			
			OMM_UNLOCK(allocation_p);
			
		}
		else
		{
			/*
			 * The glyph is not present in offscreen memory : 
			 * blit using the pixtrans registers.
			 */
#if (defined(__DEBUG__))
			if (mach_font_debug)
			{
				(void) fprintf(debug_stream_p,
"\t\t# Stippling through the pixtrans register.\n");
			}
#endif

			/*
			 * Reprogram DP CONFIG if necessary.
			 */
			MACH_STATE_SET_DP_CONFIG(screen_state_p,
									 dp_config_memory_to_screen);
			
 
			MACH_FONT_ATI_RENDER_TERMINAL_GLYPH_FROM_MEMORY(screen_state_p,
				glyph_p, x_origin, y_top, y_bottom);
			
		}

#if (defined(__DEBUG__))
		if (mach_font_debug)
		{
			(void) fprintf(debug_stream_p, "\t}\n");
		}
#endif
		
		/*
		 * Adjust the origin for the next glyph
		 */
		x_origin += width;

	} while (++glyph_list_start_p < glyph_list_end_p);
	
	ASSERT(!MACH_IS_IO_ERROR());

	MACH_STATE_SET_FLAGS(screen_state_p, 
						 (MACH_INVALID_RD_MASK |
						  MACH_INVALID_PATTERN_REGISTERS));

	/*
	 * We may have reset the clip rectangle while drawing ATI terminal
	 * glyphs.  Reprogram the clip rectangle left and right from the
	 * shadow copy.  Also restore the shadow copy of the read mask.
	 */
	
	MACH_WAIT_FOR_FIFO(3);
	outw(MACH_REGISTER_EXT_SCISSOR_L,
		 screen_state_p->register_state.ext_scissor_l);
	outw(MACH_REGISTER_EXT_SCISSOR_R,
		 screen_state_p->register_state.ext_scissor_r);
	outw(MACH_REGISTER_RD_MASK,
		 screen_state_p->register_state.rd_mask);
	
}


/*
 * mach_font_download_font
 *
 * Download a font from the SI layer into the SDD's system memory.
 * Optionally download the glyph bits into offscreen memory if
 * suitable for IBM mode drawing.
 */

STATIC SIBool
mach_font_download_font(
	SIint32 font_index,
	SIFontInfoP font_info_p,
	SIGlyphP glyph_list_p)
{
	struct mach_font	*font_p;
	int					max_glyph_width_in_pixels;
	const int			max_glyph_height_in_pixels = 
							font_info_p->SFmax.SFascent + 
							font_info_p->SFmax.SFdescent;
	int					size_of_max_glyph_in_bytes;
	const SIGlyphP		last_si_glyph_p = 
							glyph_list_p + font_info_p->SFnumglyph;
	SIGlyphP			current_si_glyph_p;
	struct mach_glyph	*current_mach_glyph_p;
	unsigned char		*glyph_bits_p;

#if (defined(__DEBUG__))
	const unsigned int           mach_debug_stamp = MACH_FONT_DEBUG_STAMP;
#endif

	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURRENT_FONT_STATE_DECLARE();
	
	const int pixtrans_width_in_bytes = 
		screen_state_p->pixtrans_width >> 3;
	
	boolean is_terminal_font = FALSE; /* is the font a terminal font */
	boolean is_ibm_font = FALSE; /* can we use IBM mode blits */

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_FONT_STATE, font_state_p));


#if (defined(__DEBUG__))
	if (mach_font_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_font_download_font)\n"
"{\n"
"\tfont_index = %ld\n"
"\tfont_info_p = %p\n"
"\tglyph_list_p = %p\n",
					   font_index, (void *) font_info_p,
					   (void *) glyph_list_p);
	}
#endif
	

	if ((font_index < 0) || 
		(font_index >=
		 font_state_p->max_number_of_downloadable_fonts))
	{
#if (defined(__DEBUG__))
		if (mach_font_debug)
		{
		(void) fprintf(debug_stream_p,
"(mach_font_download_font)\n"
"{\n"
"\t# Failing because index is out of bounds.\n"
"}\n");
		}
#endif
		return (SI_FAIL);
	}

	if (font_info_p->SFnumglyph <= 0)
	{

#if (defined(__DEBUG__))
		if (mach_font_debug)
		{
		(void) fprintf(debug_stream_p,
"(mach_font_download_font)\n"
"{\n"
"\t# Failing because of negative or zero number of glyphs\n"
"\tnumber of glyphs = %ld\n"
"}\n",
					   font_info_p->SFnumglyph);
		}
#endif
		return (SI_FAIL);
	}
	
	ASSERT ((font_state_p->font_list_pp[font_index]) == NULL);

	/*
	 * Allocate zeroed memory for one mach_font structure and
	 * tuck away the pointer in the mach font state structure.
	 */
	font_p = allocate_and_clear_memory(sizeof(struct mach_font));

	font_state_p->font_list_pp[font_index] = font_p;

	/*
	 * Copy the fontinfo structure.
	 */
	font_p->si_font_info = *font_info_p;

	ASSERT(font_info_p->SFnumglyph > 0);

	is_terminal_font = 
		(font_info_p->SFflag & SFTerminalFont) ? TRUE : FALSE;
	
	/*
	 * Allocate space for the mach_glyph structures .
	 */
	font_p->font_glyphs_p = 
		allocate_and_clear_memory(font_info_p->SFnumglyph *
								  sizeof(struct mach_glyph));
	/*
	 * Compute the space required to hold the largest character in the
	 * font and round off max_glyph_width_in_pixels to integral
	 * number of pixtrans words.
	 */
	max_glyph_width_in_pixels = 
		((font_info_p->SFmax.SFrbearing - font_info_p->SFmin.SFlbearing) + 
		 screen_state_p->pixtrans_width - 1) & 
		 ~(screen_state_p->pixtrans_width - 1);

	ASSERT(max_glyph_width_in_pixels >= 8);
	
	size_of_max_glyph_in_bytes = 
		((unsigned)max_glyph_width_in_pixels >> 3U) * 
		max_glyph_height_in_pixels ;


#if (defined(__DEBUG__))
	/*
	 * Allocate space for the glyph bits.
	 */
	font_p->font_glyph_bits_p = 
		allocate_and_clear_memory((size_of_max_glyph_in_bytes *
								  font_info_p->SFnumglyph) 
								  + sizeof(mach_debug_stamp));
	*((int *) (font_p->font_glyph_bits_p + 
			   (size_of_max_glyph_in_bytes * font_info_p->SFnumglyph))) =
		  mach_debug_stamp;
#else
	/*
	 * Allocate space for the glyph bits.
	 */
	font_p->font_glyph_bits_p = 
	    allocate_and_clear_memory((size_of_max_glyph_in_bytes *
								   font_info_p->SFnumglyph) );
#endif

	/*
	 * Now for each glyph in the font fill in the mach_glyph structure
	 * and also compute and fill the inverted bits.
	 */
	current_si_glyph_p = glyph_list_p;
	current_mach_glyph_p = font_p->font_glyphs_p;
	glyph_bits_p = font_p->font_glyph_bits_p;

	while (current_si_glyph_p < last_si_glyph_p)
	{

		int			glyph_width_in_pixels;
		int			glyph_height_in_pixels;
		int			numwords_per_glyph_line;
		int			si_glyph_line_step_in_bytes;
		int			padded_width_in_bytes;
		unsigned int glyph_end_mask;
		
		/*
		 * Copy the SIGlyph structure after allocating space for it.
		 */
		current_mach_glyph_p->si_glyph_p = 
			allocate_memory(sizeof(SIGlyph));
		*current_mach_glyph_p->si_glyph_p = 
			*current_si_glyph_p;

		glyph_width_in_pixels = 
			current_si_glyph_p->SFrbearing - current_si_glyph_p->SFlbearing; 
		glyph_height_in_pixels = 
			current_si_glyph_p->SFascent + current_si_glyph_p->SFdescent; 
		si_glyph_line_step_in_bytes = 
			(unsigned)((glyph_width_in_pixels + 31) & ~31) >> 3U;

		/*
		 * Compute the glyph end mask
		 */
		glyph_end_mask = 
			~0U >> (32U - (glyph_width_in_pixels &
						   (screen_state_p->pixtrans_width - 1)));

		/*
		 * Compute the width in pixels padded to the number of 
		 * bits in a pixtrans word.
		 */
		current_mach_glyph_p->glyph_padded_width = 
			(glyph_width_in_pixels + screen_state_p->pixtrans_width - 1) &
			~(screen_state_p->pixtrans_width - 1); 

		padded_width_in_bytes = 
			current_mach_glyph_p->glyph_padded_width >> 3;

		/*
		 * Compute the total pixtrans register transfers for 
		 * rendering this glyph from system memory.
		 */
		numwords_per_glyph_line = 
			current_mach_glyph_p->glyph_padded_width >>
			screen_state_p->pixtrans_width_shift;
		current_mach_glyph_p->glyph_pixtrans_transfers =
			numwords_per_glyph_line *
			glyph_height_in_pixels;

		/*
		 * pointer to the bits.
		 */
		current_mach_glyph_p->glyph_bits_p = glyph_bits_p;  


		/*
		 * Copy the glyph bits. Invert them byte at a time before
		 * copying them into sdd memory.
		 */
		if ((glyph_height_in_pixels > 0) && 
			( glyph_width_in_pixels > 0)) 
		{
			register unsigned char  *dst_p = glyph_bits_p;
			unsigned char	*glyph_line_p = 
				(unsigned char *) current_si_glyph_p->SFglyph.Bptr;

			do
			{
				register unsigned char *src_p = glyph_line_p;
				register int	tmpwidth = /* upto the last pixtrans width */
					padded_width_in_bytes - pixtrans_width_in_bytes;
				unsigned char *mask_p = 
					(unsigned char *) &glyph_end_mask;
				
				ASSERT(tmpwidth >= 0);
				if (tmpwidth > 0)
				{
					do
					{
						*dst_p++ = 
							(screen_state_p->byte_invert_table_p[*src_p++]);
					} while(--tmpwidth > 0);
				
				}
				/*
				 * Apply the mask for the last pixtrans word.
				 */
				tmpwidth = pixtrans_width_in_bytes;
				ASSERT(tmpwidth > 0);
				
				do
				{
					*dst_p =
						(screen_state_p->byte_invert_table_p
						 [(*dst_p & ~(*mask_p)) | (*src_p & *mask_p)]);
					++dst_p;
					++mask_p;
					++src_p;
				}
				while (--tmpwidth > 0);
				
				/*
				 * Go to the next line of the source glyph.
				 */
				glyph_line_p += si_glyph_line_step_in_bytes;

			} while(--glyph_height_in_pixels > 0); 
			
		}
		
		STAMP_OBJECT(MACH_GLYPH, current_mach_glyph_p);
		/*
		 * Move over to the next glyph.
		 */
		glyph_bits_p += size_of_max_glyph_in_bytes;
		++current_mach_glyph_p;
		++current_si_glyph_p;
	}

	/*
	 * Try to download the font into offscreen memory in a format
	 * suitable for use by the IBM mode drawing code.  For some
	 * reason, this benefits only terminal fonts.
	 */
	/* IBM mode drawing does not work for 16bpp modes. */
	if ((screen_state_p->options_p->fontdraw_options &
		 MACH_OPTIONS_FONTDRAW_OPTIONS_USE_IBM_MODE) &&
		(screen_state_p->options_p->fontdraw_options &
		 MACH_OPTIONS_FONTDRAW_OPTIONS_USE_OFFSCREEN_MEMORY) &&
		(is_terminal_font == TRUE) &&
		(screen_state_p->generic_state.screen_virtual_width <= 
		 (DEFAULT_MACH_MAX_IBM_LEFT_X + 1)) &&
		(screen_state_p->generic_state.screen_depth != 16))
	{
		int planes_required = 1; /* planes of offscreen memory
									required for the font */

		const int glyph_width_in_pixels =
			font_p->si_font_info.SFmax.SFrbearing -
			font_p->si_font_info.SFmin.SFlbearing;
		
		int glyphs_per_plane =	/* convert to a multiple of two */
			(font_p->si_font_info.SFnumglyph + 1) & ~1;
		
		struct omm_allocation *allocation_p = NULL;
		
		/*
		 * Loop till we get a suitable block of offscreen memory or
		 * we cross the depth of the screen.
		 */
		while(planes_required <=
			  screen_state_p->generic_state.screen_depth)
		{
			if ((((glyph_width_in_pixels * glyphs_per_plane) +
				  (screen_state_p->pixtrans_width - 1)) & 
				 ~(screen_state_p->pixtrans_width - 1)) <=
				screen_state_p->generic_state.screen_physical_width)
			{
				/*
				 * Try and get an offscreen memory allocation.
				 */
				int current_area_width = /* round up to a pixtrans boundary */
					((glyph_width_in_pixels * glyphs_per_plane) +
					 (screen_state_p->pixtrans_width - 1)) &
						 ~(screen_state_p->pixtrans_width - 1);
				
				if (allocation_p = 
					omm_allocate(current_area_width,
								 max_glyph_height_in_pixels,
								 planes_required,
								 OMM_LONG_TERM_ALLOCATION))
				{
					break;		/* Success */
				}
			}
			
			/*
			 * Try again with less horizontal width and more planes.
			 */
			planes_required++;

			glyphs_per_plane = 
				(font_p->si_font_info.SFnumglyph + 
				 planes_required - 1) / planes_required;
		}
		
		if (allocation_p != NULL)
		{
			int count;
			unsigned int current_planemask;
			int current_x_coordinate = allocation_p->x;
			int current_glyph_index = 0;
#if (defined(__DEBUG__))
			int max_x_coordinate = allocation_p->x +
				allocation_p->width;
#endif
			const unsigned short dp_config_download =
				screen_state_p->dp_config_flags |
					(MACH_DP_CONFIG_MONO_SRC_HOST |
					 MACH_DP_CONFIG_ENABLE_DRAW |
					 MACH_DP_CONFIG_READ_MODE_MONO_DATA |
					 MACH_DP_CONFIG_WRITE |
					 MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR |
					 MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR);
			
			
			ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION, allocation_p));
			ASSERT((allocation_p->x >= 0 && 
					allocation_p->x + allocation_p->width <=
					screen_state_p->generic_state.screen_physical_width));
			ASSERT((allocation_p->y >= 0 &&
					allocation_p->y + allocation_p->height <= 
					(DEFAULT_MACH_MAX_ATI_COORDINATE + 1))); /* for now */

			font_p->ibm_mode_allocation_p = allocation_p;
			is_ibm_font = TRUE;

			/*
			 * Get the first bit of the planemask
			 */
			for(current_planemask = 0x1U; 
				current_planemask && 
				!(current_planemask & allocation_p->planemask);
				current_planemask <<= 1)
			{
				;
			}
			
			ASSERT(current_planemask != 0);
			
			/*
			 * Time to download the glyphs.
			 */

			/*
			 * Switch to ATI mode.
			 */
			MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);
			
			/*
			 * Reset the clip rectangle to physical memory.
			 */
			screen_state_p->generic_state.screen_current_clip
				= GENERIC_CLIP_NULL;
			
			MACH_STATE_SET_ATI_CLIP_RECTANGLE(
				  screen_state_p,
				  allocation_p->x, allocation_p->y,
				  allocation_p->x + allocation_p->width,
				  allocation_p->y + allocation_p->height);
			
			MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_CLIP);
			
			/*
			 * Set up DP_CONFIG, the foreground and background colors.
			 * Reset the foreground and background rops.
			 */
			
			MACH_STATE_SET_FRGD_COLOR(screen_state_p, 
									  (unsigned short) ~0U);
			MACH_STATE_SET_BKGD_COLOR(screen_state_p, 0);
			
			MACH_STATE_SET_FG_ROP(screen_state_p,
								  MACH_MIX_FN_PAINT);
			MACH_STATE_SET_BG_ROP(screen_state_p,
								  MACH_MIX_FN_PAINT);
			
			MACH_STATE_SET_DP_CONFIG(screen_state_p,
									 dp_config_download);
			
			for (count = 0; count < font_p->si_font_info.SFnumglyph;
				 count ++)
			{
			
				struct mach_glyph *const glyph_p = 
					&(font_p->font_glyphs_p[count]);
				
				ASSERT(IS_OBJECT_STAMPED(MACH_GLYPH, glyph_p));

				ASSERT(!MACH_IS_IO_ERROR());
				
				/*
				 * Compute the rotated
				 * mask for IBM blits.
				 */
				glyph_p->ibm_mode_planemask =
					(((current_planemask << 1) & 0xFE) |
					 (current_planemask >> 7) & 0x01);

				glyph_p->ibm_mode_x_coordinate =
					current_x_coordinate;

				ASSERT(current_x_coordinate + glyph_width_in_pixels <=
					   max_x_coordinate);
				
				MACH_WAIT_FOR_FIFO(6);
				outw(MACH_REGISTER_WRT_MASK, current_planemask);
				outw(MACH_REGISTER_CUR_X, current_x_coordinate);
				outw(MACH_REGISTER_CUR_Y, allocation_p->y);
				outw(MACH_REGISTER_DEST_X_START, current_x_coordinate);
				outw(MACH_REGISTER_DEST_X_END, 
					 current_x_coordinate + glyph_p->glyph_padded_width);
				outw(MACH_REGISTER_DEST_Y_END, 
					 allocation_p->y + max_glyph_height_in_pixels);
								/* BLIT STARTS */
				MACH_WAIT_FOR_FIFO(16);
				MACH_BULK_TRANSFER_TO_GRAPHICS_ENGINE(
					screen_state_p->pixtrans_register,
					glyph_p->glyph_pixtrans_transfers,
					glyph_p->glyph_bits_p,
					(screen_state_p->pixtrans_width >> 4),
					(*screen_state_p->screen_write_bits_p));
				
				/*
				 * Goto the next glyph.
				 */
				if (current_glyph_index >= (glyphs_per_plane - 1))
				{
					current_glyph_index = 0;
					current_x_coordinate = allocation_p->x;
					current_planemask <<= 1;
				}
				else
				{
					current_glyph_index++;
					current_x_coordinate += glyph_width_in_pixels;
				}
			}
			
			/*
			 * Resynchronize the hardware wrt mask register with the in
			 * memory copy.
			 */
			MACH_WAIT_FOR_FIFO(1);
			outw(MACH_REGISTER_WRT_MASK,
				 screen_state_p->register_state.wrt_mask);
			MACH_STATE_SET_FLAGS(screen_state_p, 
				 MACH_INVALID_PATTERN_REGISTERS);
		}
	}

	/*
	 * Set the pointers to the font draw functions.
	 */

	if (is_terminal_font)
	{
		/*
		 * we can use IBM drawing if the screen resolutions are in the
		 * IBM engines range.
		 */
		if (is_ibm_font)
		{
			font_p->font_kind = MACH_FONT_KIND_IBM_MODE_TERMINAL_FONT;
			font_p->draw_ati_glyphs = NULL;
			font_p->draw_ibm_glyphs = 
				mach_font_draw_ibm_terminal_glyphs;
		}
		else
		{
			font_p->font_kind = MACH_FONT_KIND_TERMINAL_FONT;
			font_p->draw_ati_glyphs =
				mach_font_draw_ati_terminal_glyphs;
			font_p->draw_ibm_glyphs = NULL;
		}
	}
	else
	{
		/*
		 * We don't distinguish between IBM and ATi non terminal fonts.
		 */
		font_p->font_kind = MACH_FONT_KIND_NON_TERMINAL_FONT;
		font_p->draw_ati_glyphs =
			mach_font_draw_ati_non_terminal_glyphs;
		font_p->draw_ibm_glyphs = NULL;
	}


#if (defined(__DEBUG__))
	if (mach_font_debug)
	{
		(void) fprintf(debug_stream_p,
"\t\tfont_kind = %s\n"
"}\n",
					   mach_font_kind_to_dump[font_p->font_kind]);
	}
#endif
	
	STAMP_OBJECT(MACH_FONT, font_p);
	
	ASSERT(IS_OBJECT_STAMPED(MACH_FONT_STATE, font_state_p));

	ASSERT(*((int *) (font_p->font_glyph_bits_p + 
					  (size_of_max_glyph_in_bytes *
					   font_info_p->SFnumglyph))) ==
		   mach_debug_stamp);

	return (SI_SUCCEED);
}


STATIC SIBool
mach_font_free_font(SIint32 font_index)
{
	
	struct mach_font	*font_p;
	int					numglyphs;
	struct mach_glyph	*current_mach_glyph_p;

	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURRENT_FONT_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_FONT_STATE, font_state_p));

#if (defined(__DEBUG__))
	if (mach_font_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_font_free_font)\n"
"{\n"
"\tfont_index = %ld\n"
"}\n",
					   font_index);
	}
#endif
	
	if ((font_index < 0) || 
		(font_index >= 
		 font_state_p->max_number_of_downloadable_fonts))
	{

#if (defined(__DEBUG__))
		if (mach_font_debug)
		{
		(void) fprintf(debug_stream_p,
"(mach_font_free_font)\n"
"{\n"
"\t# Failing because index is out of bounds.\n"
"}\n");
		}
#endif
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return (SI_FAIL);
	}

	font_p = font_state_p->font_list_pp[font_index];

	ASSERT(IS_OBJECT_STAMPED(MACH_FONT,font_p));

	ASSERT(font_p->font_glyph_bits_p);
	
	/*
	 * Free the space allocated for the inverted bits;
	 */

	free_memory(font_p->font_glyph_bits_p);

	/*
	 * Free the memory allocated for the mach_glyph_structures.
	 */

	numglyphs = font_p->si_font_info.SFnumglyph;
	current_mach_glyph_p = font_p->font_glyphs_p;

	if (font_p->font_kind == MACH_FONT_KIND_IBM_MODE_TERMINAL_FONT)
	{

		/*
		 * Free the block of space allocated to the IBM mode font.
		 */

		ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION,
								 font_p->ibm_mode_allocation_p));
		
		(void) omm_free(font_p->ibm_mode_allocation_p);
		
	}

	ASSERT(font_p->font_kind == MACH_FONT_KIND_TERMINAL_FONT ||
		   font_p->font_kind == MACH_FONT_KIND_NON_TERMINAL_FONT ||
		   font_p->font_kind == MACH_FONT_KIND_IBM_MODE_TERMINAL_FONT);

	/*
	 * Loop over each glyph and free any offscreen allocations
	 * made.
	 */

	if (numglyphs > 0)
	{
		do
		{
			ASSERT(IS_OBJECT_STAMPED(MACH_GLYPH,
									 current_mach_glyph_p));

			ASSERT(font_p->font_kind !=
				   MACH_FONT_KIND_IBM_MODE_TERMINAL_FONT ||
				   current_mach_glyph_p->offscreen_allocation_p ==
				   NULL);
			
			if (current_mach_glyph_p->offscreen_allocation_p)
			{
				ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION,
					   current_mach_glyph_p->offscreen_allocation_p));

				(void) omm_free(current_mach_glyph_p->
								offscreen_allocation_p);
			}

			free_memory(current_mach_glyph_p->si_glyph_p);

			++current_mach_glyph_p;
		} while(--numglyphs > 0);
	}

	ASSERT(font_p->font_glyphs_p);
	
	free_memory(font_p->font_glyphs_p);

	ASSERT(font_p);
	
	free_memory(font_p);

	font_state_p->font_list_pp[font_index] = NULL;

	ASSERT(IS_OBJECT_STAMPED(MACH_FONT_STATE, font_state_p));

	return (SI_SUCCEED);
}


#if (defined(__DEBUG__))
#include <ctype.h>

STATIC void
mach_font_glyph_list_dump_helper(
	int font_id,
	int glyph_count, 
	short *glyph_list_p)
{

	(void) fprintf(debug_stream_p,"%d\t\"", font_id);
	
	while(--glyph_count > 0)
	{
		unsigned int c = *glyph_list_p;
		(void) fprintf(debug_stream_p, 
					   (isalnum(c) ? "%c " : "\\%3.3o "), c);
		glyph_list_p++;
	}
	(void) fprintf(debug_stream_p, "\"\n");
}
#endif /* DEBUG */
	
/*
 * SI's entry point.
 */

STATIC SIBool
mach_font_draw_glyphs(
	const SIint32 font_index, 
	const SIint32 x,
	const SIint32 y,
	SIint32 glyph_count,
	SIint16 *glyph_list_p,
	SIint32 draw_method)
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();
	MACH_CURRENT_FONT_STATE_DECLARE();
	
	/*
	 * The font descriptor.
	 */
	struct mach_font *font_p;
	struct mach_glyph *font_glyphs_p;

	int x_origin = x;			/* starting point of string */

	int use_offscreen_memory;
	
	const int screen_clip_left =
		screen_state_p->generic_state.screen_clip_left;
	const int screen_clip_right =
		screen_state_p->generic_state.screen_clip_right;
	const int screen_clip_top =
		screen_state_p->generic_state.screen_clip_top;
	const int screen_clip_bottom = 
		screen_state_p->generic_state.screen_clip_bottom;

	const SIint16 *last_glyph_p = glyph_list_p + glyph_count;
	
	SIint16 *displayed_glyph_start_p;
	SIint16 *displayed_glyph_end_p;
	
	int end_x_origin;
	int string_width;
	
	int draw_flags;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE, graphics_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(MACH_FONT_STATE, font_state_p));

#if (defined(__DEBUG__))
	if (mach_font_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_font_draw_glyphs)\n"
"{\n"
"\tfont_index = %ld\n"
"\tx = %ld\n"
"\ty = %ld\n"
"\tglyph_count = %ld\n"
"\tglyph_list_p = %p\n"
"\tdraw_method = %ld\n",
					   font_index, x, y, glyph_count,
					   (void *) glyph_list_p, draw_method);
	}
	
	/*
	 * Dump the glyph list
	 */
	if (mach_font_string_debug)
	{
		mach_font_glyph_list_dump_helper(font_index, glyph_count, 
										 glyph_list_p);
	}
	
#endif

	if (glyph_count <= 0)
	{
		return (SI_SUCCEED);
	}
	
	if (font_index < 0 || font_index >=
		font_state_p->max_number_of_downloadable_fonts) 
	{
#if (defined(__DEBUG__))
		if (mach_font_debug)
		{
		(void) fprintf(debug_stream_p,
"(mach_font_draw_glyphs)\n"
"{\n"
"\t# Failing because index is out of bounds.\n"
"\tfont_index = %ld\n"
"}\n",
					   font_index);
		}
#endif
		return (SI_FAIL);
	}


	use_offscreen_memory = 
		(screen_state_p->options_p->fontdraw_options &
		 MACH_OPTIONS_FONTDRAW_OPTIONS_USE_OFFSCREEN_MEMORY) ?
		 TRUE : FALSE;
	
	/*
	 * Determine the type of draw : opaque or transparent stippling.
	 */
	if (draw_method == 0)
	{
		draw_method =
			graphics_state_p->generic_state.si_graphics_state.SGstplmode;
	}
	
	if ((draw_method == SGStipple) &&
		(graphics_state_p->generic_state.si_graphics_state.SGfillmode
		 != SGFillSolidFG) &&
		(graphics_state_p->generic_state.si_graphics_state.SGfillmode
		 != SGFillSolidBG))
	{
		/*
		 * We can't handle polytext requests with the fill style set
		 * to tiling or stippling.
		 */

		return (SI_FAIL);
	}
		
	font_p = *(font_state_p->font_list_pp + font_index);

	ASSERT(IS_OBJECT_STAMPED(MACH_FONT, font_p));

	font_glyphs_p = font_p->font_glyphs_p;
	
	ASSERT(font_glyphs_p != NULL);
	
	/*
	 * Check for the y-coordinates visibility.
	 */
	if ((y - font_p->si_font_info.SFmax.SFascent > screen_clip_bottom) ||
		(y + font_p->si_font_info.SFmax.SFdescent <= screen_clip_top))
	{
#if (defined(__DEBUG__))
		if (mach_font_debug)
		{
			(void) fprintf(debug_stream_p,
"\t# Y coordinates check indicates nothing is to be drawn.\n"
"}\n");
		}
#endif
		return (SI_SUCCEED);
	}
	
	/*
	 * Compute clipping parameters.
	 */
	if (font_p->si_font_info.SFflag & SFTerminalFont)
	{
		/*
		 * Terminal glyphs.
		 *
		 */
		int width = font_p->si_font_info.SFmax.SFwidth;
		int left_extra_glyph_count = 0;
		int right_extra_glyph_count = 0;
		int x_start = x;		/* inclusive */
		int x_end = x_start + (width * glyph_count); /* exclusive */
		
		if (width < 0)
		{
			/*
			 * Right to left
			 */
			int tmp;

			/*
			 * Swap start and end and adjust for the first character.
			 */
			tmp = x_start;
			x_start = x_end;
			x_end = tmp;
			
			x_start -= width;
			x_end += width;
			
		}

		if (x_start > screen_clip_right || x_end <= screen_clip_left )
		{
#if (defined(__DEBUG__))
			if (mach_font_debug)
			{
				(void) fprintf(debug_stream_p,
"\t#X coordinate of string is out of clip rectangle.\n"
"}\n");
			}
#endif
			return (SI_SUCCEED);
		}
		
		if (x_start < screen_clip_left)
		{
			left_extra_glyph_count = (screen_clip_left - x_start) /
				width;
		}

		if ((x_end - 1) > screen_clip_right)
		{
			right_extra_glyph_count = (x_end - screen_clip_right - 1) /
				width;
		}
		
		if (width > 0)
		{
			displayed_glyph_start_p = glyph_list_p +
				left_extra_glyph_count;
			displayed_glyph_end_p = glyph_list_p + glyph_count -
				right_extra_glyph_count;
		}
		else					/* width < 0 */
		{	
			displayed_glyph_start_p = glyph_list_p +
				right_extra_glyph_count;
			displayed_glyph_end_p = glyph_list_p + glyph_count -
				left_extra_glyph_count;
		}
		
		/*
		 * The string width parameter is not used for terminal fonts,
		 * as there is no need for a separate background rectangle
		 * fill.  Instead we do opaque stippling.
		 * However we still need to check if the drawing code would go
		 * outside the IBM 8514 clip rectangle.
		 */
		string_width = 
			(displayed_glyph_end_p - displayed_glyph_start_p) * width;
		
		x_origin +=  (displayed_glyph_start_p - glyph_list_p) * width;

	}
	else
	{
		
		/*
		 * Non-terminal glyphs.
		 */
		
		/*
		 * Compute the set of visible glyphs given the current clip region,
		 * ensure that these glyphs are downloaded into offscreen memory
		 * (if the usage of offscreen memory is allowed).
		 * The total width in pixels of the glyphs visible is returned.
		 */
		displayed_glyph_start_p = glyph_list_p;

		/*
		 * Look for the first visible glyph.
		 */
		do
		{
			register const struct mach_glyph *glyph_p = 
				&(font_glyphs_p[*displayed_glyph_start_p]);
		
			if ((x_origin + 
				 glyph_p->si_glyph_p->SFrbearing - 1) >= 
				screen_clip_left)
			{
				if ((x_origin +
					 glyph_p->si_glyph_p->SFlbearing) <=
					screen_clip_right)
				{
					break;
				}
			}
			
			x_origin += glyph_p->si_glyph_p->SFwidth;
			
		} while (++displayed_glyph_start_p < last_glyph_p);
		
		/*
		 * Have we exhausted the glyph list already?
		 */
		if (displayed_glyph_start_p == last_glyph_p)
		{
			/*
			 * Nothing to draw.
			 */
#if (defined(__DEBUG__))
			if (mach_font_debug)
			{
				(void) fprintf(debug_stream_p,
"\t#No glyph visible in clip region.\n"
"}\n");
			}
#endif
			return (SI_SUCCEED);
		}
		
		/*
		 * Look for the last (partially?) visible character.
		 */
		
		displayed_glyph_end_p = displayed_glyph_start_p;
		
		end_x_origin = x_origin;

		do
		{
			register const struct mach_glyph *const glyph_p = 
				&(font_glyphs_p[*displayed_glyph_end_p]);
			
			if (((end_x_origin + 
				  glyph_p->si_glyph_p->SFrbearing - 1) <
				 screen_clip_left) ||
				((end_x_origin +
				  glyph_p->si_glyph_p->SFlbearing) >
				 screen_clip_right))
			{
				break;
			}
			
			end_x_origin += glyph_p->si_glyph_p->SFwidth;

		} while (++displayed_glyph_end_p < last_glyph_p);
			

		/*
		 * Compute the string width.  This will overshoot, for clipped
		 * text, but the hardware clip will ensure that wrong does
		 * not actually occur.
		 *
		 * *BUG* This is incorrect for right to left text
		 */

		string_width = end_x_origin - x_origin;
		
		ASSERT(displayed_glyph_start_p <= displayed_glyph_end_p);
		
		/*
		 * Check if *something* is visible.
		 */
		if (displayed_glyph_start_p == displayed_glyph_end_p)
		{
#if (defined(__DEBUG__))
			if (mach_font_debug)
			{
				(void) fprintf(debug_stream_p,
"\t# No glyphs are displayable.\n"
"}\n");
			}
#endif
			return (SI_SUCCEED);
		}
	}

#if (defined(__DEBUG__))
	if (mach_font_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif


	/*
	 * Set the draw flags.
	 */
	draw_flags = (draw_method == SGStipple) ?
		MACH_FONT_DO_TRANSPARENT_STIPPLING : 
		MACH_FONT_DO_OPAQUE_STIPPLING;
	
	draw_flags |= 
		(font_p->font_kind == MACH_FONT_KIND_IBM_MODE_TERMINAL_FONT) ?
		MACH_FONT_USE_IBM_BLITS : 0;

	/*
	 * Download glyphs if we are to use and the font is not an
	 * IBM mode font.  IBM mode fonts are placed in offscreen memory
	 * at font download time.
	 */
	if (use_offscreen_memory == TRUE && 
		!(draw_flags & MACH_FONT_USE_IBM_BLITS))
	{
		SIint16 *tmp_glyph_index_p = displayed_glyph_start_p;
		
		/*
		 * Loop through the list of glyphs to be drawn and download the
		 * glyphs necessary.
		 */

		do
		{
			struct mach_glyph *const glyph_p =
				&(font_glyphs_p[*tmp_glyph_index_p]);
			
			ASSERT(IS_OBJECT_STAMPED(MACH_GLYPH, glyph_p));
			
			/*
			 * Download the glyph if necessary
			 */
			if (glyph_p->offscreen_allocation_p == NULL)
			{
				if((glyph_p->offscreen_allocation_p = 
					omm_allocate(glyph_p->glyph_padded_width,
								 glyph_p->si_glyph_p->SFascent +
								 glyph_p->si_glyph_p->SFdescent,
								 1, /* depth */
								 OMM_LONG_TERM_ALLOCATION)) != NULL)
				{
					const struct omm_allocation *const allocation_p =
						glyph_p->offscreen_allocation_p;
					
					const unsigned short dp_config_download =
						screen_state_p->dp_config_flags |
							(MACH_DP_CONFIG_MONO_SRC_HOST |
							 MACH_DP_CONFIG_ENABLE_DRAW |
							 MACH_DP_CONFIG_READ_MODE_MONO_DATA |
							 MACH_DP_CONFIG_WRITE |
							 MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR |
							 MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR);
					
					ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION, allocation_p));
					ASSERT(allocation_p->x >= 0 && 
						   allocation_p->x + allocation_p->width <=
						   screen_state_p->generic_state.screen_physical_width);
					ASSERT(allocation_p->y >= 0 &&
						   allocation_p->y + allocation_p->height <= 
						   screen_state_p->generic_state.screen_physical_height);
					/*
					 * Switch to ATI context.
					 */
					MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);
		
					/*
					 * Reset the clip rectangle to physical memory.
					 */
					screen_state_p->generic_state.screen_current_clip
						= GENERIC_CLIP_TO_VIDEO_MEMORY;
					
					MACH_STATE_SET_ATI_CLIP_RECTANGLE(
					  screen_state_p,
					  0, 0,
					  screen_state_p->generic_state.screen_physical_width,
					  screen_state_p->generic_state.screen_physical_height);
					
					MACH_STATE_SET_FLAGS(screen_state_p,
										 MACH_INVALID_CLIP);
					
#if (defined(__DEBUG__))
					if (mach_font_debug)
					{
						(void) fprintf(debug_stream_p,
"\t\t# Offscreen allocation\n"
"\t\tglyph_index = %hd\n"
"\t\tallocation_p = %p\n"
"\t\tallocation_x = %d\n"
"\t\tallocation_y = %d\n"
"\t\tallocation_planemask = 0x%x\n",
									   *tmp_glyph_index_p,
									   (void *) allocation_p,
									   allocation_p->x, allocation_p->y,
									   allocation_p->planemask);
						
					}
#endif
					
					/*
					 * Download the glyph into the offscreen location.
					 */
					
					/*
					 * Set up DP_CONFIG, the foreground and background colors.
					 * Reset the foreground and background rops.
					 */
					
					MACH_STATE_SET_FRGD_COLOR(screen_state_p, 
											  (unsigned short) ~0U);
					MACH_STATE_SET_BKGD_COLOR(screen_state_p, 0);
					
					MACH_STATE_SET_FG_ROP(screen_state_p,
										  MACH_MIX_FN_PAINT);
					MACH_STATE_SET_BG_ROP(screen_state_p,
										  MACH_MIX_FN_PAINT);
					
					MACH_STATE_SET_DP_CONFIG(screen_state_p,
											 dp_config_download);
					
					MACH_FONT_DOWNLOAD_GLYPH(screen_state_p, glyph_p,
											 allocation_p);
					
					/*
					 * Resynchronize the write mask.
					 */
					MACH_WAIT_FOR_FIFO(1);
					outw(MACH_REGISTER_WRT_MASK,
						 screen_state_p->register_state.wrt_mask);
					
					ASSERT(!MACH_IS_IO_ERROR());
					
				}
				else
				{
					/*
					 * Offscreen allocation failed ...
					 * Stop asking for more memory, and switch off IBM
					 * font drawing.  The generic ATI font draw modes will
					 * take over.
					 */
#if (defined(__DEBUG__))
					if (mach_font_debug)
					{
						(void) fprintf(debug_stream_p,
"\t\t# Allocation of offscreen area failed.\n"
"\t\tglyph_index = %hd\n",
									   *tmp_glyph_index_p);
					}
#endif
					draw_flags &= ~(MACH_FONT_USE_IBM_BLITS);
					break;
				}
			}
		} while (++ tmp_glyph_index_p < displayed_glyph_end_p);
	}


	/*
	 * Switch the graphics engine to the correct mode.
	 */
	if (draw_flags & MACH_FONT_USE_IBM_BLITS)
	{
		/*
		 * Switch to IBM context.
		 */
		MACH_STATE_SWITCH_TO_IBM_CONTEXT(screen_state_p);
	}
	else
	{
		/*
		 * Switch to ATI context.
		 */
		MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);
	}
		
	if (draw_flags & MACH_FONT_DO_TRANSPARENT_STIPPLING)
	{
		
		/*
		 * Transparent stippling (polytext).
		 */

#if (defined(__DEBUG__))
		if (mach_font_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_font_draw_glyphs)\n"
"{\n"
"\tTransparent stippling.\n");
		}
#endif

		/*
		 * Synchronize registers with the graphics state.
		 */
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
			 MACH_POLYTEXT_DEPENDENCIES);

		if (screen_state_p->current_graphics_engine_mode ==
			MACH_GE_MODE_ATI_MODE)
		{
			MACH_STATE_SET_BG_ROP(screen_state_p,
								  MACH_MIX_FN_LEAVE_ALONE);
		}
		else					/* IBM mode */
		{
			screen_state_p->register_state.alu_bg_fn =
				MACH_MIX_FN_LEAVE_ALONE;
			MACH_WAIT_FOR_FIFO(1);
			outw(MACH_REGISTER_BKGD_MIX, MACH_IBM_SELECT_BKGD_COLOR |
				 MACH_MIX_FN_LEAVE_ALONE);
		}
	}
	else if (draw_flags & MACH_FONT_DO_OPAQUE_STIPPLING)
	{
	
#if (defined(__DEBUG__))
		if (mach_font_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_font_draw_glyphs)\n"
"{\n"
"\tOpaque stippling.\n");
		}
#endif
		/*
		 * Synchronize registers with the graphics state.
		 */
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
									 MACH_IMAGETEXT_DEPENDENCIES);
		if (screen_state_p->current_graphics_engine_mode ==
			MACH_GE_MODE_IBM_MODE)
		{
			/*
			 * set shadow registers and IBM mix registers to carry out
			 * a GXcopy operation.
			 */
			screen_state_p->register_state.alu_fg_fn = 
			screen_state_p->register_state.alu_bg_fn = 
				MACH_MIX_FN_PAINT;
			MACH_WAIT_FOR_FIFO(2);
			outw(MACH_REGISTER_FRGD_MIX, MACH_IBM_SELECT_FRGD_COLOR |
				 MACH_MIX_FN_PAINT);
			outw(MACH_REGISTER_BKGD_MIX, MACH_IBM_SELECT_BKGD_COLOR |
				 MACH_MIX_FN_PAINT);
		}
		else					/* ATI mode */
		{
			
			/*
			 * The foreground and background rops have to be reset to 
			 * GXcopy.
			 */
			MACH_STATE_SET_FG_ROP(screen_state_p, MACH_MIX_FN_PAINT);
			MACH_STATE_SET_BG_ROP(screen_state_p, MACH_MIX_FN_PAINT);
		}
	}
	else
	{
		/*
		 * Should never come here!
		 */
		/*CONSTANTCONDITION*/
		ASSERT(0);
	}


	/*
	 * Call the appropriate drawing function for this font.
	 */

	ASSERT((draw_flags & MACH_FONT_USE_IBM_BLITS) ?
		   (font_p->draw_ibm_glyphs) : (font_p->draw_ati_glyphs));
	
	((draw_flags & MACH_FONT_USE_IBM_BLITS) ?
	 (*font_p->draw_ibm_glyphs) : (*font_p->draw_ati_glyphs))
		(font_p, displayed_glyph_start_p, 
		 displayed_glyph_end_p, x_origin, 
		 y, string_width, draw_flags);

#if (defined(__DEBUG__))
	if (mach_font_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	return (SI_SUCCEED);

}


function void
mach_font__initialize__(SIScreenRec *si_screen_p,
						struct mach_options_structure * options_p)
{
	int glyph_max_width, glyph_max_height;
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();

	struct mach_font_state *font_state_p;
	
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		(struct generic_screen_state *) screen_state_p));

	/*
	 * Create space for the font state and fill in control values.
	 */
	font_state_p = 
		allocate_and_clear_memory(sizeof(struct mach_font_state));

	/*
	 * How many fonts the SDD should support at a given time.
	 */
	font_state_p->max_number_of_downloadable_fonts = 
		options_p->number_of_downloadable_fonts;

	/*
	 * Create space for font pointers.
	 */
	if (font_state_p->max_number_of_downloadable_fonts > 0)
	{
		font_state_p->font_list_pp =
			allocate_and_clear_memory(
			    font_state_p->max_number_of_downloadable_fonts * 
				sizeof(struct mach_font *));
	}
	
	if (sscanf(options_p->glyph_cache_size, "%ix%i", &glyph_max_width,
			   &glyph_max_height) != 2)
	{
		(void) fprintf(stderr,
					   MACH_CANNOT_PARSE_GLYPH_CACHE_SIZE_MESSAGE,
					   options_p->glyph_cache_size);
		glyph_max_width = DEFAULT_MACH_MAX_GLYPH_WIDTH;
		glyph_max_height = DEFAULT_MACH_MAX_GLYPH_HEIGHT;
	}

	font_state_p->max_supported_glyph_width = glyph_max_width;
	font_state_p->max_supported_glyph_height = glyph_max_height;
	

	STAMP_OBJECT(MACH_FONT_STATE, font_state_p);

	/*
	 * Tuck away pointer into the screen state.
	 */
	screen_state_p->font_state_p = font_state_p;

	/*
	 * Store parameters for SI.
	 */
	flags_p->SIfontcnt  =
		font_state_p->max_number_of_downloadable_fonts;
	
	flags_p->SIavail_font = 
		(FONT_AVAIL | STIPPLE_AVAIL | OPQSTIPPLE_AVAIL);
	functions_p->si_font_check = mach_font_is_font_downloadable;
	functions_p->si_font_download = mach_font_download_font;
	functions_p->si_font_free = mach_font_free_font;
	functions_p->si_font_stplblt = mach_font_draw_glyphs;

}
