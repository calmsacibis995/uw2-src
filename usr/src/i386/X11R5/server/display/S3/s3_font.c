/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)S3:S3/s3_font.c	1.8"

/***
 ***	NAME
 ***
 ***		s3_font.c : Font handling code for the S3 display
 ***	library.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "s3_font.h"
 ***
 ***	DESCRIPTION
 ***
 ***	RETURNS
 ***
 ***	MACRO VARIABLES
 ***
 ***		__DEBUG__ : Enable debugging and assertion checking.
 ***
 ***	FILES
 ***
 ***
 ***	SEE ALSO
 ***
 ***	CAVEATS
 ***
 ***	BUGS
 ***
 ***	AUTHORS
 ***
 ***	HISTORY
 ***
 ***/

PUBLIC

/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <sidep.h>
#include "stdenv.h"
#include "s3_options.h"
#include "s3_globals.h"

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
export boolean s3_font_debug = FALSE;
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

/*
 * Constants.
 */

/*
 * Flags controlling the drawing.
 */
#define S3_FONT_DO_TRANSPARENT_STIPPLING 		(0x0001 << 0U)
#define S3_FONT_DO_OPAQUE_STIPPLING			(0x0001 << 1U)

/*
 * Font kinds.
 */
#define DEFINE_FONT_TYPES()\
	DEFINE_FONT_TYPE(NULL),\
	DEFINE_FONT_TYPE(SMALL_TERMINAL_FONT),\
	DEFINE_FONT_TYPE(TERMINAL_FONT),\
	DEFINE_FONT_TYPE(NON_TERMINAL_FONT),\
	DEFINE_FONT_TYPE(PACKED_SMALL_TERMINAL_FONT),\
	DEFINE_FONT_TYPE(COUNT)

#if (defined(__DEBUG__))
STATIC const char *const s3_font_kind_to_dump[] = 
{
#define DEFINE_FONT_TYPE(NAME)\
	# NAME
	DEFINE_FONT_TYPES()
#undef DEFINE_FONT_TYPE
};
#endif

#if (defined(__DEBUG__))
#define S3_FONT_DEBUG_STAMP 		0xdeadbeef
#endif

#define S3_GLYPH_STAMP\
	(('S' << 0) + ('3' << 1) + ('_' << 2) + ('G' << 3) +\
	 ('L' << 4) + ('Y' << 5) + ('P' << 6) + ('H' << 7) +\
	 ('_' << 8) + ('S' << 9) + ('T' << 10) + ('A' << 11) +\
	 ('M' << 12) + ('P' << 13))

#define S3_FONT_STAMP\
	(('S' << 0) + ('3' << 1) + ('_' << 2) + ('F' << 3) +\
	 ('O' << 4) + ('N' << 5) + ('T' << 6) + ('_' << 7) +\
	 ('S' << 8) + ('T' << 9) + ('A' << 10) + ('M' << 11) +\
	 ('P' << 12))

#define S3_FONT_STATE_STAMP\
	(('S' << 0) + ('3' << 1) + ('_' << 2) + ('F' << 3) +\
	 ('O' << 4) + ('N' << 5) + ('T' << 6) + ('_' << 7) +\
	 ('S' << 8) + ('T' << 9) + ('A' << 10) + ('T' << 11) +\
	 ('E' << 12) + ('_' << 13) + ('S' << 14) + ('T' << 15) +\
	 ('A' << 16) + ('M' << 17) + ('P' << 18))

#define S3_POLYTEXT_DEPENDENCIES\
	(S3_INVALID_FOREGROUND_COLOR |\
	 S3_INVALID_BACKGROUND_COLOR |\
	 S3_INVALID_FG_ROP |\
	 S3_INVALID_CLIP_RECTANGLE |\
	 S3_INVALID_WRT_MASK)

#define S3_IMAGETEXT_DEPENDENCIES\
	(S3_INVALID_FOREGROUND_COLOR |\
	 S3_INVALID_BACKGROUND_COLOR |\
	 S3_INVALID_CLIP_RECTANGLE |\
	 S3_INVALID_WRT_MASK)

/*
 * Macros.
 */
#define S3_CURRENT_FONT_STATE_DECLARE()\
	struct s3_font_state *font_state_p =\
		screen_state_p->font_state_p

#define S3_FONT_DOWNLOAD_GLYPH(SCREENSTATEP, GLYPHP, ALLOCATIONP, COMMAND)	\
{																			\
	ASSERT((ALLOCATIONP)->width <= (GLYPHP)->glyph_padded_width); 			\
	ASSERT((ALLOCATIONP)->height <= 										\
		((GLYPHP)->si_glyph_p->SFascent + (GLYPHP)->si_glyph_p->SFdescent));\
	S3_WAIT_FOR_FIFO(6);													\
	if ((SCREENSTATEP)->use_mmio)											\
	{																		\
		S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_WRT_MASK,\
			(ALLOCATIONP)->planemask);										\
		S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,	\
			(ALLOCATIONP)->x);												\
		S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,	\
			(ALLOCATIONP)->y);												\
		S3_MMIO_SET_ENHANCED_REGISTER(										\
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 					\
			(GLYPHP)->glyph_padded_width - 1);								\
		S3_MMIO_SET_ENHANCED_REGISTER(										\
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,					\
			S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |				\
			(((GLYPHP)->si_glyph_p->SFascent +								\
			(GLYPHP)->si_glyph_p->SFdescent-1) & S3_MULTIFUNC_VALUE_BITS));	\
		S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,		\
			COMMAND);														\
		S3_WAIT_FOR_FIFO(8);												\
		(*SCREENSTATEP->screen_write_bits_p)(								\
			(SCREENSTATEP)->pixtrans_register,								\
			(GLYPHP)->glyph_pixtrans_transfers,								\
			(GLYPHP)->glyph_bits_p);										\
	}																		\
	else																	\
	{																		\
		S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_WRT_MASK,	\
			(ALLOCATIONP)->planemask);										\
		S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,		\
			(ALLOCATIONP)->x);												\
		S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,		\
			(ALLOCATIONP)->y);												\
		S3_IO_SET_ENHANCED_REGISTER(										\
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 					\
			(GLYPHP)->glyph_padded_width - 1);								\
		S3_IO_SET_ENHANCED_REGISTER(										\
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,					\
			S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |				\
			(((GLYPHP)->si_glyph_p->SFascent +								\
			(GLYPHP)->si_glyph_p->SFdescent-1) & S3_MULTIFUNC_VALUE_BITS));	\
		S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,		\
			COMMAND);														\
		S3_WAIT_FOR_FIFO(8);												\
		(*SCREENSTATEP->screen_write_bits_p)(								\
			(SCREENSTATEP)->pixtrans_register,								\
			(GLYPHP)->glyph_pixtrans_transfers,								\
			(GLYPHP)->glyph_bits_p);										\
	}																		\
	S3_WAIT_FOR_FIFO(1);													\
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_WRT_MASK,			\
	(SCREENSTATEP)->register_state.s3_enhanced_commands_registers.write_mask);\
}

#define S3_MMIO_FONT_RENDER_NON_TERMINAL_GLYPH_FROM_MEMORY(SCREENSTATEP,\
	GLYPHP, XLEFT, YTOP, GLYPHHEIGHT, CMD_REG)\
	{\
		S3_WAIT_FOR_FIFO(5);\
		S3_MMIO_SET_ENHANCED_REGISTER(\
			S3_ENHANCED_COMMAND_REGISTER_CUR_X, XLEFT);\
		S3_MMIO_SET_ENHANCED_REGISTER(\
			S3_ENHANCED_COMMAND_REGISTER_CUR_Y, YTOP);\
		S3_MMIO_SET_ENHANCED_REGISTER(\
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,\
			(GLYPHP)->glyph_padded_width - 1);\
		S3_MMIO_SET_ENHANCED_REGISTER(\
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,\
			S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |\
			((GLYPHHEIGHT) & S3_MULTIFUNC_VALUE_BITS));\
		S3_MMIO_SET_ENHANCED_REGISTER(\
			S3_ENHANCED_COMMAND_REGISTER_CMD,CMD_REG);\
		S3_INLINE_WAIT_FOR_FIFO(8);\
		(*SCREENSTATEP->screen_write_bits_p) \
			(SCREENSTATEP->pixtrans_register,\
		  	(GLYPHP)->glyph_pixtrans_transfers,\
			(void *)(GLYPHP)->glyph_bits_p);\
	}

#define S3_MMIO_FONT_RENDER_TERMINAL_GLYPH_FROM_MEMORY(SCREENSTATEP,	\
	GLYPHP, XORIGIN, YTOP, HEIGHT, CMD_REG)								\
	{																	\
	 	int glyph_end_x = (XORIGIN) + (GLYPHP)->si_glyph_p->SFwidth;	\
		if (glyph_end_x > (XORIGIN))									\
		{																\
			S3_WAIT_FOR_FIFO(1);										\
			if (glyph_end_x <=											\
			(SCREENSTATEP)->generic_state.screen_clip_right)			\
			{															\
				S3_MMIO_SET_ENHANCED_REGISTER(							\
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,		\
					(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX|		\
					(glyph_end_x & S3_MULTIFUNC_VALUE_BITS)));			\
			}	                                                   		\
			else														\
			{	                                                  		\
				S3_MMIO_SET_ENHANCED_REGISTER(							\
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,		\
					(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX|		\
					((SCREENSTATEP)->generic_state.screen_clip_right	\
					& S3_MULTIFUNC_VALUE_BITS)));						\
			}                                                     		\
		}																\
		S3_WAIT_FOR_FIFO(5);											\
		S3_MMIO_SET_ENHANCED_REGISTER(									\
			S3_ENHANCED_COMMAND_REGISTER_CUR_X, XORIGIN);				\
		S3_MMIO_SET_ENHANCED_REGISTER(									\
			S3_ENHANCED_COMMAND_REGISTER_CUR_Y, YTOP);					\
		S3_MMIO_SET_ENHANCED_REGISTER(									\
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,					\
			(GLYPHP)->glyph_padded_width - 1);							\
		S3_MMIO_SET_ENHANCED_REGISTER(									\
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,				\
			S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |			\
			((HEIGHT) & S3_MULTIFUNC_VALUE_BITS));						\
		S3_MMIO_SET_ENHANCED_REGISTER(									\
			S3_ENHANCED_COMMAND_REGISTER_CMD,CMD_REG);					\
		S3_INLINE_WAIT_FOR_FIFO(8);										\
		(*SCREENSTATEP->screen_write_bits_p) 							\
			(SCREENSTATEP->pixtrans_register,							\
		  	(GLYPHP)->glyph_pixtrans_transfers,							\
			(void *)(GLYPHP)->glyph_bits_p);							\
	}

#define S3_IO_FONT_RENDER_NON_TERMINAL_GLYPH_FROM_MEMORY(SCREENSTATEP,	\
	GLYPHP, XLEFT, YTOP, GLYPHHEIGHT, CMD_REG)							\
	{																	\
		S3_WAIT_FOR_FIFO(5);											\
		S3_IO_SET_ENHANCED_REGISTER(									\
			S3_ENHANCED_COMMAND_REGISTER_CUR_X, XLEFT);					\
		S3_IO_SET_ENHANCED_REGISTER(									\
			S3_ENHANCED_COMMAND_REGISTER_CUR_Y, YTOP);					\
		S3_IO_SET_ENHANCED_REGISTER(									\
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,					\
			(GLYPHP)->glyph_padded_width - 1);							\
		S3_IO_SET_ENHANCED_REGISTER(									\
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,				\
			S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |			\
			((GLYPHHEIGHT) & S3_MULTIFUNC_VALUE_BITS));					\
		S3_IO_SET_ENHANCED_REGISTER(									\
			S3_ENHANCED_COMMAND_REGISTER_CMD,CMD_REG);					\
		S3_INLINE_WAIT_FOR_FIFO(8);										\
		(*SCREENSTATEP->screen_write_bits_p) 							\
			(SCREENSTATEP->pixtrans_register,							\
		  	(GLYPHP)->glyph_pixtrans_transfers,							\
			(void *)(GLYPHP)->glyph_bits_p);							\
	}

#define S3_IO_FONT_RENDER_TERMINAL_GLYPH_FROM_MEMORY(SCREENSTATEP,		\
	GLYPHP, XORIGIN, YTOP, HEIGHT, CMD_REG)								\
	{																	\
	 	int glyph_end_x = (XORIGIN) + (GLYPHP)->si_glyph_p->SFwidth;	\
		if (glyph_end_x > (XORIGIN))									\
		{																\
			S3_WAIT_FOR_FIFO(1);										\
			if (glyph_end_x <=											\
			(SCREENSTATEP)->generic_state.screen_clip_right)			\
			{															\
				S3_IO_SET_ENHANCED_REGISTER(							\
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,		\
					(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX|		\
					(glyph_end_x & S3_MULTIFUNC_VALUE_BITS)));			\
			}	                                                   		\
			else														\
			{	                                                  		\
				S3_IO_SET_ENHANCED_REGISTER(							\
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,		\
					(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX|		\
					((SCREENSTATEP)->generic_state.screen_clip_right	\
					& S3_MULTIFUNC_VALUE_BITS)));						\
			}                                                     		\
		}																\
		S3_WAIT_FOR_FIFO(5);											\
		S3_IO_SET_ENHANCED_REGISTER(									\
			S3_ENHANCED_COMMAND_REGISTER_CUR_X, XORIGIN);				\
		S3_IO_SET_ENHANCED_REGISTER(									\
			S3_ENHANCED_COMMAND_REGISTER_CUR_Y, YTOP);					\
		S3_IO_SET_ENHANCED_REGISTER(									\
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,					\
			(GLYPHP)->glyph_padded_width - 1);							\
		S3_IO_SET_ENHANCED_REGISTER(									\
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,				\
			S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |			\
			((HEIGHT) & S3_MULTIFUNC_VALUE_BITS));						\
		S3_IO_SET_ENHANCED_REGISTER(									\
			S3_ENHANCED_COMMAND_REGISTER_CMD,CMD_REG);					\
		S3_INLINE_WAIT_FOR_FIFO(8);										\
		(*SCREENSTATEP->screen_write_bits_p) 							\
			(SCREENSTATEP->pixtrans_register,							\
		  	(GLYPHP)->glyph_pixtrans_transfers,							\
			(void *)(GLYPHP)->glyph_bits_p);							\
	}

#define S3_MMIO_FONT_RENDER_NON_TERMINAL_GLYPH_FROM_SCREEN(GLYPHP,		\
	ALLOCATIONP, XLEFT, GLYPHWIDTH, YTOP, GLYPHHEIGHT, COMMAND)			\
{																		\
	S3_WAIT_FOR_FIFO(8);												\
	S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_RD_MASK,	\
		(ALLOCATIONP)->planemask);										\
	S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,	\
		(ALLOCATIONP)->x);												\
	S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 	\
		(ALLOCATIONP)->y);												\
	S3_MMIO_SET_ENHANCED_REGISTER(										\
		S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP,(XLEFT));				\
	S3_MMIO_SET_ENHANCED_REGISTER(										\
		S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP,(YTOP));				\
	S3_MMIO_SET_ENHANCED_REGISTER(										\
		S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,(GLYPHWIDTH));		\
	S3_MMIO_SET_ENHANCED_REGISTER(										\
		S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,					\
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |				\
		((GLYPHHEIGHT) & S3_MULTIFUNC_VALUE_BITS));						\
	S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,		\
		COMMAND);														\
}

#define S3_IO_FONT_RENDER_NON_TERMINAL_GLYPH_FROM_SCREEN(GLYPHP,		\
	ALLOCATIONP, XLEFT, GLYPHWIDTH, YTOP, GLYPHHEIGHT, COMMAND)			\
{																		\
	S3_WAIT_FOR_FIFO(8);												\
	S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_RD_MASK,	\
		(ALLOCATIONP)->planemask);										\
	S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,		\
		(ALLOCATIONP)->x);												\
	S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 	\
		(ALLOCATIONP)->y);												\
	S3_IO_SET_ENHANCED_REGISTER(										\
		S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP,(XLEFT));				\
	S3_IO_SET_ENHANCED_REGISTER(										\
		S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP,(YTOP));				\
	S3_IO_SET_ENHANCED_REGISTER(										\
		S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,(GLYPHWIDTH));		\
	S3_IO_SET_ENHANCED_REGISTER(										\
		S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,					\
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |				\
		((GLYPHHEIGHT) & S3_MULTIFUNC_VALUE_BITS));						\
	S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,		\
		(COMMAND));														\
}


#define S3_MMIO_FONT_RENDER_TERMINAL_GLYPH_FROM_SCREEN(ALLOCATIONP, 	\
	XORIGIN, WIDTH, YTOP, HEIGHT, COMMAND)								\
{																		\
	S3_WAIT_FOR_FIFO(8);												\
	S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_RD_MASK,	\
		(ALLOCATIONP)->planemask);										\
	S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,	\
		(ALLOCATIONP)->x);												\
	S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,	\
		(ALLOCATIONP)->y);												\
	S3_MMIO_SET_ENHANCED_REGISTER(										\
		S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP,(XORIGIN));			\
	S3_MMIO_SET_ENHANCED_REGISTER(										\
		S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP,(YTOP));				\
	S3_MMIO_SET_ENHANCED_REGISTER(										\
		S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,(WIDTH));			\
	S3_MMIO_SET_ENHANCED_REGISTER(										\
	S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,                    	\
	S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |              	\
	((HEIGHT) & S3_MULTIFUNC_VALUE_BITS));             					\
	S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,		\
		COMMAND);														\
}

#define S3_IO_FONT_RENDER_TERMINAL_GLYPH_FROM_SCREEN(ALLOCATIONP, 		\
	XORIGIN, WIDTH, YTOP, HEIGHT, COMMAND)								\
{																		\
	S3_WAIT_FOR_FIFO(8);												\
	S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_RD_MASK,	\
		(ALLOCATIONP)->planemask);										\
	S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,		\
		(ALLOCATIONP)->x);												\
	S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,		\
		(ALLOCATIONP)->y);												\
	S3_IO_SET_ENHANCED_REGISTER(										\
		S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP,(XORIGIN));			\
	S3_IO_SET_ENHANCED_REGISTER(										\
		S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP,(YTOP));				\
	S3_IO_SET_ENHANCED_REGISTER(										\
		S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,(WIDTH));			\
	S3_IO_SET_ENHANCED_REGISTER(										\
	S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,                    	\
	S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |              	\
	((HEIGHT) & S3_MULTIFUNC_VALUE_BITS));             					\
	S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,		\
		COMMAND);														\
}

/*
 * Types.
 */

/*
 * Cached glyph information.
 */

struct s3_glyph
{
	/*
	 * Plane mask if it is a terminal font.
	 */
	unsigned int	small_terminal_font_planemask;

	/*
	 * X coordinate of the glyph in case of termnal font.
	 */

	int small_terminal_font_x_coordinate;
	
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



enum s3_font_kind
{
#define DEFINE_FONT_TYPE(NAME)\
	S3_FONT_KIND_ ## NAME
		DEFINE_FONT_TYPES()
#undef DEFINE_FONT_TYPE
};


/*
 * Structure of a downloaded font.
 */
struct s3_font
{
	/*
	 * The type of this font.
	 */
	enum s3_font_kind font_kind;
	
	/*
	 * SI's font information
	 */
	SIFontInfo si_font_info;
	
	/*
	 * Pointer to the array of s3_glyph structures.
	 */
	struct s3_glyph *font_glyphs_p;
	
	/*
	 * Pointer to the inverted glyph bits for all the glyphs.
	 */
	unsigned char *font_glyph_bits_p;

	/*
	 * Offscreen allocation for terminal fonts.
	 */
	struct omm_allocation *small_terminal_fonts_allocation_p;
	

	/*
	 * Used incase of  terminal fonts which can be assembled at
 	 * drawing time.
	 */

	int *font_packed_glyph_bits_p;

	/*
	 * Number of terminal glyph widths that can be packed into a
	 * word.
	 */ 

	int number_of_glyph_lines_per_word;

	/*
	 * The font draw function for this font.
	 */
	void (*draw_glyphs)
		(struct s3_font *font_p,
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

struct s3_font_state
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
	struct s3_font **font_list_pp;
	
#if (defined(__DEBUG__))
	int stamp;
#endif
};

/***
 ***	Includes.
 ***/

#include "g_omm.h"
#include "s3_state.h"
#include "s3_gs.h"


/***
 ***	Functions.
 ***/

STATIC SIBool
s3_font_is_font_downloadable( SIint32 font_index, SIFontInfoP fontinfo_p)
{
	
	S3_CURRENT_SCREEN_STATE_DECLARE();

	S3_CURRENT_FONT_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(S3_FONT_STATE, font_state_p));
	
#if (defined(__DEBUG__))
	if (s3_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_font_is_font_downloadable)\n"
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
		if (s3_font_debug)
		{
			(void) fprintf(debug_stream_p,
				"(s3_font_is_font_downloadable)\n"
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
	 	   S3_OPTIONS_FONTDRAW_OPTIONS_DRAW_TERMINAL_FONTS) &&
		 (fontinfo_p->SFflag & SFTerminalFont)) ||
	    (!(screen_state_p->options_p->fontdraw_options & 
	 	   S3_OPTIONS_FONTDRAW_OPTIONS_DRAW_NON_TERMINAL_FONTS) &&
		 (!(fontinfo_p->SFflag & SFTerminalFont))) ||
		((screen_state_p->options_p->
		  max_number_of_glyphs_in_downloadable_font > 0) && 
		 (fontinfo_p->SFnumglyph > screen_state_p->options_p->
		  max_number_of_glyphs_in_downloadable_font)))
	{
#if (defined(__DEBUG__))
		if (s3_font_debug)
		{
			(void) fprintf(debug_stream_p,
				"(s3_font_is_font_downloadable)\n"
				"{\n"
				"\t# Failing because font-type not downloadable by option.\n"
				"\tfont type = %s\n"
				"\tnumber of glyphs = %ld\n"
				"}\n",
				(fontinfo_p->SFflag & SFTerminalFont) ? "Terminal Font.\n" :
				   "NonTerminal Font.\n", fontinfo_p->SFnumglyph);
		}
#endif
		return (SI_FAIL);
	}

	return (SI_SUCCEED);
}

/*
 *Drawing small packed terminal glyphs from system memory
 */

STATIC void
s3_font_draw_packed_terminal_glyphs_mmio(
	struct s3_font *font_p,
	short *glyph_list_start_p,
	short *glyph_list_end_p,
	int x_origin,
	int y_origin,
	int string_width,
	int draw_flags)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	const unsigned char * font_glyph_bits_p = font_p->font_glyph_bits_p;
	const int * font_packed_glyph_bits_p =
			 (int*)font_p->font_packed_glyph_bits_p;
	const int width = font_p->si_font_info.SFmax.SFwidth;
	const int height = font_p->si_font_info.SFmax.SFdescent + 
		font_p->si_font_info.SFmax.SFascent;
	const int y_position =
		 (y_origin - font_p->si_font_info.SFmax.SFascent);
	const int words_per_glyph = height;
	const int	glyph_lines_per_word =
		 font_p->number_of_glyph_lines_per_word;
	int	packed_transfer_count = 
		(glyph_list_end_p - glyph_list_start_p) / glyph_lines_per_word;
	volatile int *pixtrans_base_p =
			(int*)screen_state_p->mmio_base_address;
	struct s3_glyph *const font_glyphs_p = font_p->font_glyphs_p;


	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	ASSERT(font_packed_glyph_bits_p);

#if (defined(__DEBUG__))
	if (s3_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_font_draw_packed_terminal_glyphs_mmio) {\n"
			"\tfont_p = %p\n"
			"\tglyph_list_start_p = %p\n"
			"\tglyph_list_end_p = %p\n"
			"\tx_origin = %d\n"
			"\ty_origin = %d\n"
			"\tstring_width = %d\n"
			"\tglyph_lines_per_word = %d\n"
			"\tdraw_flags = 0x%x\n"
			"}\n",
			(void *) font_p,
			(void *) glyph_list_start_p,
			(void *) glyph_list_end_p,
			x_origin, y_origin,
			string_width,
			glyph_lines_per_word,
			draw_flags);
	}
#endif

	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(
		PIX_CNTL_DT_EX_SRC_CPU_DATA);

	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
	S3_SET_BG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);

	S3_WAIT_FOR_FIFO(2);

	S3_MMIO_SET_ENHANCED_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
		(width*glyph_lines_per_word - 1));

	S3_MMIO_SET_ENHANCED_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
		((height - 1) & S3_MULTIFUNC_VALUE_BITS));


	if (packed_transfer_count > 0) 
	{
		unsigned int word;

#define	GLYPH_LINE(GLYPH_NO,SHIFT)										\
		(*(g_bits_p[GLYPH_NO] + tmp_height)) <<	(SHIFT)

		switch(glyph_lines_per_word)
		{
			case 5:
				ASSERT(width == 6);
				do
				{
					int tmp_height = 0;
					int *g_bits_p[5];

					g_bits_p[0] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[0]);

					g_bits_p[1] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[1]);

					g_bits_p[2] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[2]);

					g_bits_p[3] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[3]);

					g_bits_p[4] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[4]);

					S3_INLINE_WAIT_FOR_FIFO(3);

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
						y_position);

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_X,
						x_origin);

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD,
						S3_CMD_WRITE | 
						S3_CMD_PX_MD_ACROSS_PLANE |
						S3_CMD_DRAW |
						S3_CMD_BUS_WIDTH_16 |
						S3_CMD_DIR_TYPE_AXIAL | 
						S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
						S3_CMD_AXIAL_X_MAJOR |
						S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |
						S3_CMD_USE_PIXTRANS |
						S3_CMD_TYPE_RECTFILL);

					S3_INLINE_WAIT_FOR_FIFO(8);


						
					do
					{
#define	GLYPH0_OFFSET	26
#define	GLYPH1_OFFSET	20
#define	GLYPH2_OFFSET	14
#define	GLYPH3_OFFSET	 8
#define	GLYPH4_OFFSET	 2
						word =
							GLYPH_LINE(0,GLYPH0_OFFSET) | 
							GLYPH_LINE(1,GLYPH1_OFFSET) | 
							GLYPH_LINE(2,GLYPH2_OFFSET) | 
							GLYPH_LINE(3,GLYPH3_OFFSET) | 
							GLYPH_LINE(4,GLYPH4_OFFSET); 

						*((int*)pixtrans_base_p) =
							(word << 16) | (word >> 16);


#undef	GLYPH0_OFFSET	
#undef	GLYPH1_OFFSET
#undef	GLYPH2_OFFSET
#undef	GLYPH3_OFFSET
#undef	GLYPH4_OFFSET

					}while(++tmp_height < height);


					/*
					 * Adjust the origin for the next glyph
					 */

					x_origin += (width*glyph_lines_per_word);
					glyph_list_start_p += glyph_lines_per_word;
				}while(--packed_transfer_count > 0);
				break;
			case 4:
				ASSERT(width == 8 || width == 7);

#define		RENDER_4_GLYPHS()								\
{															\
	int tmp_height = 0;										\
	int *g_bits_p[4];										\
	g_bits_p[0] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[0]);								\
	g_bits_p[1] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[1]);								\
	g_bits_p[2] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[2]);								\
	g_bits_p[3] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[3]);								\
	S3_INLINE_WAIT_FOR_FIFO(3);								\
	S3_MMIO_SET_ENHANCED_REGISTER(							\
		S3_ENHANCED_COMMAND_REGISTER_CUR_Y,					\
		y_position);										\
	S3_MMIO_SET_ENHANCED_REGISTER(							\
		S3_ENHANCED_COMMAND_REGISTER_CUR_X,					\
		x_origin);											\
	S3_MMIO_SET_ENHANCED_REGISTER(							\
		S3_ENHANCED_COMMAND_REGISTER_CMD,					\
		S3_CMD_WRITE | 										\
		S3_CMD_PX_MD_ACROSS_PLANE |							\
		S3_CMD_DRAW |										\
		S3_CMD_BUS_WIDTH_16 |								\
		S3_CMD_DIR_TYPE_AXIAL | 							\
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |						\
		S3_CMD_AXIAL_X_MAJOR |								\
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |						\
		S3_CMD_USE_PIXTRANS |								\
		S3_CMD_TYPE_RECTFILL);								\
	S3_INLINE_WAIT_FOR_FIFO(8);								\
	do														\
	{														\
		word = 												\
		GLYPH_LINE(0,GLYPH0_OFFSET) | 						\
		GLYPH_LINE(1,GLYPH1_OFFSET) | 						\
		GLYPH_LINE(2,GLYPH2_OFFSET) | 						\
		GLYPH_LINE(3,GLYPH3_OFFSET);						\
		*((int*)pixtrans_base_p) =							\
			(word << 16) | (word >> 16);					\
	}while(++tmp_height < height);							\
	x_origin += (width*glyph_lines_per_word);				\
	glyph_list_start_p += glyph_lines_per_word;				\
}

				if (width == 8)
				{
#define	GLYPH0_OFFSET	24
#define	GLYPH1_OFFSET	16
#define	GLYPH2_OFFSET	 8
#define	GLYPH3_OFFSET	 0
					do
					{
						RENDER_4_GLYPHS();
					}while(--packed_transfer_count > 0);
#undef	GLYPH0_OFFSET	
#undef	GLYPH1_OFFSET
#undef	GLYPH2_OFFSET
#undef	GLYPH3_OFFSET
				}
				else
				{
#define	GLYPH0_OFFSET	25
#define	GLYPH1_OFFSET	18
#define	GLYPH2_OFFSET	11
#define	GLYPH3_OFFSET	 4
					do
					{
						RENDER_4_GLYPHS();
					}while(--packed_transfer_count > 0);
#undef	GLYPH0_OFFSET	
#undef	GLYPH1_OFFSET
#undef	GLYPH2_OFFSET
#undef	GLYPH3_OFFSET
				}
				break;

			case 3:

#define		RENDER_3_GLYPHS()								\
{															\
	int tmp_height = 0;										\
	int *g_bits_p[3];										\
	g_bits_p[0] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[0]);								\
	g_bits_p[1] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[1]);								\
	g_bits_p[2] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[2]);								\
	S3_INLINE_WAIT_FOR_FIFO(3);								\
	S3_MMIO_SET_ENHANCED_REGISTER(							\
		S3_ENHANCED_COMMAND_REGISTER_CUR_Y,					\
		y_position);										\
	S3_MMIO_SET_ENHANCED_REGISTER(							\
		S3_ENHANCED_COMMAND_REGISTER_CUR_X,					\
		x_origin);											\
	S3_MMIO_SET_ENHANCED_REGISTER(							\
		S3_ENHANCED_COMMAND_REGISTER_CMD,					\
		S3_CMD_WRITE | 										\
		S3_CMD_PX_MD_ACROSS_PLANE |							\
		S3_CMD_DRAW |										\
		S3_CMD_BUS_WIDTH_16 |								\
		S3_CMD_DIR_TYPE_AXIAL | 							\
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |						\
		S3_CMD_AXIAL_X_MAJOR |								\
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |						\
		S3_CMD_USE_PIXTRANS |								\
		S3_CMD_TYPE_RECTFILL);								\
	S3_INLINE_WAIT_FOR_FIFO(8);								\
	do														\
	{														\
		word = 												\
		GLYPH_LINE(0,GLYPH0_OFFSET) | 						\
		GLYPH_LINE(1,GLYPH1_OFFSET) | 						\
		GLYPH_LINE(2,GLYPH2_OFFSET); 						\
		*((int*)pixtrans_base_p) =							\
			(word << 16) | (word >> 16);					\
	}while(++tmp_height < height);							\
	x_origin += (width*glyph_lines_per_word);				\
	glyph_list_start_p += glyph_lines_per_word;				\
}
				ASSERT(width == 10 || width == 9);

				if (width == 10)
				{
#define	GLYPH0_OFFSET	22
#define	GLYPH1_OFFSET	12
#define	GLYPH2_OFFSET	2
					do
					{
						RENDER_3_GLYPHS();
					}while(--packed_transfer_count > 0);
#undef	GLYPH0_OFFSET	
#undef	GLYPH1_OFFSET
#undef	GLYPH2_OFFSET
				}
				else
				{
#define	GLYPH0_OFFSET	23
#define	GLYPH1_OFFSET	14
#define	GLYPH2_OFFSET	5
					do
					{
						RENDER_3_GLYPHS();
					}while(--packed_transfer_count > 0);
#undef	GLYPH0_OFFSET	
#undef	GLYPH1_OFFSET
#undef	GLYPH2_OFFSET
				}

				break;
#undef	GLYPH_LINE

			default:
				ASSERT(glyph_lines_per_word >= 5);

				do
				{
					int tmp_height = 0;
					int *g_bits_p[5];

					g_bits_p[0] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[0]);

					g_bits_p[1] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[1]);

					g_bits_p[2] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[2]);

					g_bits_p[3] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[3]);

					g_bits_p[4] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[4]);

					S3_INLINE_WAIT_FOR_FIFO(3);

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
						y_position);

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_X,
						x_origin);

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD,
						S3_CMD_WRITE | 
						S3_CMD_PX_MD_ACROSS_PLANE |
						S3_CMD_DRAW |
						S3_CMD_BUS_WIDTH_16 |
						S3_CMD_DIR_TYPE_AXIAL | 
						S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
						S3_CMD_AXIAL_X_MAJOR |
						S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |
						S3_CMD_USE_PIXTRANS |
						S3_CMD_TYPE_RECTFILL);

					S3_INLINE_WAIT_FOR_FIFO(8);

#define	GLYPH_LINE(GLYPH_NO)										\
			(*(g_bits_p[GLYPH_NO] + tmp_height)) <<					\
				(32 - ((GLYPH_NO+1) * width))
						
					do
					{
						word = 
							GLYPH_LINE(0) | 
							GLYPH_LINE(1) | 
							GLYPH_LINE(2) | 
							GLYPH_LINE(3) | 
							GLYPH_LINE(4); 

						*((volatile int*)pixtrans_base_p) =
							(word >> 16) | (word << 16);

					}while(++tmp_height < height);

					/*
					 * Adjust the origin for the next glyph
					 */

					x_origin += (width*glyph_lines_per_word);
					glyph_list_start_p += glyph_lines_per_word;

				}while(--packed_transfer_count > 0);
			break;
#undef	GLYPH_LINE
#undef	RENDER_4_GLYPHS
#undef	RENDER_3_GLYPHS
		}
	}



	/*
	 * Draw the remaining glyphs one at a time
	 */

	if(glyph_list_start_p < glyph_list_end_p)
	{
		S3_WAIT_FOR_FIFO(1);

		S3_MMIO_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
			(width - 1));
		do
		{
			S3_INLINE_WAIT_FOR_FIFO(3);

			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
				y_position);

			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X,
				x_origin);

			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CMD,
				S3_CMD_WRITE | 
				S3_CMD_PX_MD_ACROSS_PLANE |
				S3_CMD_DRAW |
				S3_CMD_BUS_WIDTH_16 |
				S3_CMD_DIR_TYPE_AXIAL | 
				S3_CMD_LSB_FIRST |
				S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
				S3_CMD_AXIAL_X_MAJOR |
				S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |
				S3_CMD_USE_PIXTRANS |
				S3_CMD_TYPE_RECTFILL);

			S3_INLINE_WAIT_FOR_FIFO(8);

			(*screen_state_p->screen_write_bits_p)(
				screen_state_p->pixtrans_register,
				font_glyphs_p[*glyph_list_start_p].glyph_pixtrans_transfers,
				font_glyphs_p[*glyph_list_start_p].glyph_bits_p);

			x_origin += width;
			++glyph_list_start_p;
		}while(glyph_list_start_p < glyph_list_end_p);
	}



}
	
/*
 *Drawing small packed terminal glyphs from system memory
 */

#define	WRITE_PIXTRANS(DATA)\
	outw(screen_state_p->pixtrans_register,DATA)
	
STATIC void
s3_font_draw_packed_terminal_glyphs_io(
	struct s3_font *font_p,
	short *glyph_list_start_p,
	short *glyph_list_end_p,
	int x_origin,
	int y_origin,
	int string_width,
	int draw_flags)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	const unsigned char * font_glyph_bits_p = font_p->font_glyph_bits_p;
	const int * font_packed_glyph_bits_p =
			 (int*)font_p->font_packed_glyph_bits_p;
	const int width = font_p->si_font_info.SFmax.SFwidth;
	const int height = font_p->si_font_info.SFmax.SFdescent + 
		font_p->si_font_info.SFmax.SFascent;
	const int y_position =
		 (y_origin - font_p->si_font_info.SFmax.SFascent);
	const int words_per_glyph = height;
	const int	glyph_lines_per_word =
		 font_p->number_of_glyph_lines_per_word;
	int	packed_transfer_count = 
		(glyph_list_end_p - glyph_list_start_p) / glyph_lines_per_word;
	struct s3_glyph *const font_glyphs_p = font_p->font_glyphs_p;


	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	ASSERT(font_packed_glyph_bits_p);

#if (defined(__DEBUG__))
	if (s3_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_font_draw_packed_terminal_glyphs_io) {\n"
			"\tfont_p = %p\n"
			"\tglyph_list_start_p = %p\n"
			"\tglyph_list_end_p = %p\n"
			"\tx_origin = %d\n"
			"\ty_origin = %d\n"
			"\tstring_width = %d\n"
			"\tglyph_lines_per_word = %d\n"
			"\tdraw_flags = 0x%x\n"
			"}\n",
			(void *) font_p,
			(void *) glyph_list_start_p,
			(void *) glyph_list_end_p,
			x_origin, y_origin,
			string_width,
			glyph_lines_per_word,
			draw_flags);
	}
#endif

	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(
		PIX_CNTL_DT_EX_SRC_CPU_DATA);

	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
	S3_SET_BG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);

	S3_WAIT_FOR_FIFO(2);

	S3_IO_SET_ENHANCED_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
		(width*glyph_lines_per_word - 1));

	S3_IO_SET_ENHANCED_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
		((height - 1) & S3_MULTIFUNC_VALUE_BITS));


	if (packed_transfer_count > 0) 
	{
		unsigned int word;

#define	GLYPH_LINE(GLYPH_NO,SHIFT)										\
		(*(g_bits_p[GLYPH_NO] + tmp_height)) <<	(SHIFT)

		switch(glyph_lines_per_word)
		{
			case 5:
				ASSERT(width == 6);
				do
				{
					int tmp_height = 0;
					int *g_bits_p[5];

					g_bits_p[0] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[0]);

					g_bits_p[1] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[1]);

					g_bits_p[2] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[2]);

					g_bits_p[3] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[3]);

					g_bits_p[4] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[4]);

					S3_INLINE_WAIT_FOR_FIFO(3);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
						y_position);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_X,
						x_origin);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD,
						S3_CMD_WRITE | 
						S3_CMD_PX_MD_ACROSS_PLANE |
						S3_CMD_DRAW |
						S3_CMD_BUS_WIDTH_16 |
						S3_CMD_DIR_TYPE_AXIAL | 
						S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
						S3_CMD_AXIAL_X_MAJOR |
						S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |
						S3_CMD_USE_PIXTRANS |
						S3_CMD_TYPE_RECTFILL);

					S3_INLINE_WAIT_FOR_FIFO(8);


						
					do
					{
#define	GLYPH0_OFFSET	26
#define	GLYPH1_OFFSET	20
#define	GLYPH2_OFFSET	14
#define	GLYPH3_OFFSET	 8
#define	GLYPH4_OFFSET	 2
						word =
							GLYPH_LINE(0,GLYPH0_OFFSET) | 
							GLYPH_LINE(1,GLYPH1_OFFSET) | 
							GLYPH_LINE(2,GLYPH2_OFFSET) | 
							GLYPH_LINE(3,GLYPH3_OFFSET) | 
							GLYPH_LINE(4,GLYPH4_OFFSET); 

							WRITE_PIXTRANS((word>>16));
							WRITE_PIXTRANS((word&0xFFFF));


#undef	GLYPH0_OFFSET	
#undef	GLYPH1_OFFSET
#undef	GLYPH2_OFFSET
#undef	GLYPH3_OFFSET
#undef	GLYPH4_OFFSET

					}while(++tmp_height < height);


					/*
					 * Adjust the origin for the next glyph
					 */

					x_origin += (width*glyph_lines_per_word);
					glyph_list_start_p += glyph_lines_per_word;
				}while(--packed_transfer_count > 0);
				break;
			case 4:
				ASSERT(width == 8 || width == 7);

#define		RENDER_4_GLYPHS()								\
{															\
	int tmp_height = 0;										\
	int *g_bits_p[4];										\
	g_bits_p[0] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[0]);								\
	g_bits_p[1] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[1]);								\
	g_bits_p[2] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[2]);								\
	g_bits_p[3] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[3]);								\
	S3_INLINE_WAIT_FOR_FIFO(3);								\
	S3_IO_SET_ENHANCED_REGISTER(							\
		S3_ENHANCED_COMMAND_REGISTER_CUR_Y,					\
		y_position);										\
	S3_IO_SET_ENHANCED_REGISTER(							\
		S3_ENHANCED_COMMAND_REGISTER_CUR_X,					\
		x_origin);											\
	S3_IO_SET_ENHANCED_REGISTER(							\
		S3_ENHANCED_COMMAND_REGISTER_CMD,					\
		S3_CMD_WRITE | 										\
		S3_CMD_PX_MD_ACROSS_PLANE |							\
		S3_CMD_DRAW |										\
		S3_CMD_BUS_WIDTH_16 |								\
		S3_CMD_DIR_TYPE_AXIAL | 							\
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |						\
		S3_CMD_AXIAL_X_MAJOR |								\
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |						\
		S3_CMD_USE_PIXTRANS |								\
		S3_CMD_TYPE_RECTFILL);								\
	S3_INLINE_WAIT_FOR_FIFO(8);								\
	do														\
	{														\
		word = 												\
		GLYPH_LINE(0,GLYPH0_OFFSET) | 						\
		GLYPH_LINE(1,GLYPH1_OFFSET) | 						\
		GLYPH_LINE(2,GLYPH2_OFFSET) | 						\
		GLYPH_LINE(3,GLYPH3_OFFSET);						\
		WRITE_PIXTRANS((word>>16));							\
		WRITE_PIXTRANS((word&0xFFFF));						\
	}while(++tmp_height < height);							\
	x_origin += (width*glyph_lines_per_word);				\
	glyph_list_start_p += glyph_lines_per_word;				\
}

				if (width == 8)
				{
#define	GLYPH0_OFFSET	24
#define	GLYPH1_OFFSET	16
#define	GLYPH2_OFFSET	 8
#define	GLYPH3_OFFSET	 0
					do
					{
						RENDER_4_GLYPHS();
					}while(--packed_transfer_count > 0);
#undef	GLYPH0_OFFSET	
#undef	GLYPH1_OFFSET
#undef	GLYPH2_OFFSET
#undef	GLYPH3_OFFSET
				}
				else
				{
#define	GLYPH0_OFFSET	25
#define	GLYPH1_OFFSET	18
#define	GLYPH2_OFFSET	11
#define	GLYPH3_OFFSET	 4
					do
					{
						RENDER_4_GLYPHS();
					}while(--packed_transfer_count > 0);
#undef	GLYPH0_OFFSET	
#undef	GLYPH1_OFFSET
#undef	GLYPH2_OFFSET
#undef	GLYPH3_OFFSET
				}
				break;

			case 3:

#define		RENDER_3_GLYPHS()								\
{															\
	int tmp_height = 0;										\
	int *g_bits_p[3];										\
	g_bits_p[0] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[0]);								\
	g_bits_p[1] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[1]);								\
	g_bits_p[2] = 											\
		(int*)(font_packed_glyph_bits_p +					\
		words_per_glyph *									\
		glyph_list_start_p[2]);								\
	S3_INLINE_WAIT_FOR_FIFO(3);								\
	S3_IO_SET_ENHANCED_REGISTER(							\
		S3_ENHANCED_COMMAND_REGISTER_CUR_Y,					\
		y_position);										\
	S3_IO_SET_ENHANCED_REGISTER(							\
		S3_ENHANCED_COMMAND_REGISTER_CUR_X,					\
		x_origin);											\
	S3_IO_SET_ENHANCED_REGISTER(							\
		S3_ENHANCED_COMMAND_REGISTER_CMD,					\
		S3_CMD_WRITE | 										\
		S3_CMD_PX_MD_ACROSS_PLANE |							\
		S3_CMD_DRAW |										\
		S3_CMD_BUS_WIDTH_16 |								\
		S3_CMD_DIR_TYPE_AXIAL | 							\
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |						\
		S3_CMD_AXIAL_X_MAJOR |								\
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |						\
		S3_CMD_USE_PIXTRANS |								\
		S3_CMD_TYPE_RECTFILL);								\
	S3_INLINE_WAIT_FOR_FIFO(8);								\
	do														\
	{														\
		word = 												\
		GLYPH_LINE(0,GLYPH0_OFFSET) | 						\
		GLYPH_LINE(1,GLYPH1_OFFSET) | 						\
		GLYPH_LINE(2,GLYPH2_OFFSET); 						\
		WRITE_PIXTRANS((word>>16));							\
		WRITE_PIXTRANS((word&0xFFFF));						\
	}while(++tmp_height < height);							\
	x_origin += (width*glyph_lines_per_word);				\
	glyph_list_start_p += glyph_lines_per_word;				\
}
				ASSERT(width == 10 || width == 9);

				if (width == 10)
				{
#define	GLYPH0_OFFSET	22
#define	GLYPH1_OFFSET	12
#define	GLYPH2_OFFSET	2
					do
					{
						RENDER_3_GLYPHS();
					}while(--packed_transfer_count > 0);
#undef	GLYPH0_OFFSET	
#undef	GLYPH1_OFFSET
#undef	GLYPH2_OFFSET
				}
				else
				{
#define	GLYPH0_OFFSET	23
#define	GLYPH1_OFFSET	14
#define	GLYPH2_OFFSET	5
					do
					{
						RENDER_3_GLYPHS();
					}while(--packed_transfer_count > 0);
#undef	GLYPH0_OFFSET	
#undef	GLYPH1_OFFSET
#undef	GLYPH2_OFFSET
				}

				break;
#undef	GLYPH_LINE

			default:
				ASSERT(glyph_lines_per_word >= 5);

				do
				{
					int tmp_height = 0;
					int *g_bits_p[5];

					g_bits_p[0] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[0]);

					g_bits_p[1] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[1]);

					g_bits_p[2] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[2]);

					g_bits_p[3] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[3]);

					g_bits_p[4] = 
						(int*)(font_packed_glyph_bits_p + words_per_glyph *
						glyph_list_start_p[4]);

					S3_INLINE_WAIT_FOR_FIFO(3);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
						y_position);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_X,
						x_origin);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD,
						S3_CMD_WRITE | 
						S3_CMD_PX_MD_ACROSS_PLANE |
						S3_CMD_DRAW |
						S3_CMD_BUS_WIDTH_16 |
						S3_CMD_DIR_TYPE_AXIAL | 
						S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
						S3_CMD_AXIAL_X_MAJOR |
						S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |
						S3_CMD_USE_PIXTRANS |
						S3_CMD_TYPE_RECTFILL);

					S3_INLINE_WAIT_FOR_FIFO(8);

#define	GLYPH_LINE(GLYPH_NO)										\
			(*(g_bits_p[GLYPH_NO] + tmp_height)) <<					\
				(32 - ((GLYPH_NO+1) * width))
						
					do
					{

						word = 
							GLYPH_LINE(0) | 
							GLYPH_LINE(1) | 
							GLYPH_LINE(2) | 
							GLYPH_LINE(3) | 
							GLYPH_LINE(4); 
						WRITE_PIXTRANS((word>>16));
						WRITE_PIXTRANS((word&0xFFFF));

					}while(++tmp_height < height);

					/*
					 * Adjust the origin for the next glyph
					 */

					x_origin += (width*glyph_lines_per_word);
					glyph_list_start_p += glyph_lines_per_word;

				}while(--packed_transfer_count > 0);
			break;
#undef	GLYPH_LINE
#undef	RENDER_4_GLYPHS
#undef	RENDER_3_GLYPHS
		}
	}



	/*
	 * Draw the remaining glyphs one at a time
	 */

	if(glyph_list_start_p < glyph_list_end_p)
	{
		S3_WAIT_FOR_FIFO(1);

		S3_IO_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
			(width - 1));
		do
		{
			S3_INLINE_WAIT_FOR_FIFO(3);

			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
				y_position);

			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X,
				x_origin);

			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CMD,
				S3_CMD_WRITE | 
				S3_CMD_PX_MD_ACROSS_PLANE |
				S3_CMD_DRAW |
				S3_CMD_BUS_WIDTH_16 |
				S3_CMD_DIR_TYPE_AXIAL | 
				S3_CMD_LSB_FIRST |
				S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
				S3_CMD_AXIAL_X_MAJOR |
				S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |
				S3_CMD_USE_PIXTRANS |
				S3_CMD_TYPE_RECTFILL);

			S3_INLINE_WAIT_FOR_FIFO(8);

			(*screen_state_p->screen_write_bits_p)(
				screen_state_p->pixtrans_register,
				font_glyphs_p[*glyph_list_start_p].glyph_pixtrans_transfers,
				font_glyphs_p[*glyph_list_start_p].glyph_bits_p);

			x_origin += width;
			++glyph_list_start_p;
		}while(glyph_list_start_p < glyph_list_end_p);
	}



}
#undef	WRITE_PIXTRANS

/*
 * Drawing non terminal glyphs, some of which are not in offscreen
 * memory. 
 */

STATIC void
s3_font_draw_non_terminal_glyphs(
	struct s3_font *font_p,
	short *glyph_list_start_p,
	short *glyph_list_end_p,
	int x_origin,
	int y_origin,
	int string_width,
	int draw_flags)
{

	S3_CURRENT_SCREEN_STATE_DECLARE();
	
	struct s3_glyph *const font_glyphs_p = font_p->font_glyphs_p;
	
	/*
	 * command register value for memory to screen stippling.
	 */
	const unsigned short cmd_memory_to_screen =
		screen_state_p->cmd_flags  |
		S3_CMD_TYPE_RECTFILL |
		S3_CMD_WRITE |
		S3_CMD_DRAW |
		S3_CMD_USE_PIXTRANS |
		S3_CMD_DIR_TYPE_AXIAL |
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_PX_MD_ACROSS_PLANE |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;

	const unsigned short cmd_screen_to_screen =
		screen_state_p->cmd_flags  |
		S3_CMD_TYPE_BITBLT |
		S3_CMD_WRITE |
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL |
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_PX_MD_ACROSS_PLANE |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	ASSERT(font_p->font_kind == S3_FONT_KIND_NON_TERMINAL_FONT);
	
	/*
	 * Check if we have something to do.
	 */
	if (string_width == 0)
	{
		return;
	}
	/*
	 * This is being done before the function is called. Remove this
	 * later.
	 */
	S3_STATE_SET_BG_ROP(screen_state_p, S3_MIX_FN_LEAVE_C_AS_IS);

	/*
	 * Fill the background rectangle if the stippling mode is opaque
	 * stippling.
	 */
	if (draw_flags & S3_FONT_DO_OPAQUE_STIPPLING)
	{
	
		const unsigned short cmd_rect_fill = 
				screen_state_p->cmd_flags | 
				S3_CMD_PX_MD_ACROSS_PLANE |  
				S3_CMD_TYPE_RECTFILL |
				S3_CMD_WRITE | 
				S3_CMD_DRAW |
				S3_CMD_DIR_TYPE_AXIAL | 
				(string_width > 0 ? S3_CMD_AXIAL_X_LEFT_TO_RIGHT :
					S3_CMD_AXIAL_X_RIGHT_TO_LEFT) |
				S3_CMD_AXIAL_X_MAJOR |
				S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;
	
		/*
		 * Fill a rectangle of coordinates 
		 * UL (x, y - font_p->si_font_info.SFlascent)
		 * LR (x + string_width, y +
		 *		font_p->si_font_info.SFldescent) with the background color.
		 */
		S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);
		S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);

		S3_WAIT_FOR_FIFO(5);

		if (screen_state_p->use_mmio)
		{
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
				(string_width > 0 ? x_origin : x_origin - 1));

			S3_MMIO_SET_ENHANCED_REGISTER( 
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
				y_origin - font_p->si_font_info.SFlascent);

			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 
				((string_width > 0 ? string_width : -string_width) - 1));

			S3_MMIO_SET_ENHANCED_REGISTER( 
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
				((font_p->si_font_info.SFldescent + 
				  font_p->si_font_info.SFlascent - 1) & 
				 S3_MULTIFUNC_VALUE_BITS));

			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CMD,
				cmd_rect_fill);
		}
		else
		{
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
				(string_width > 0 ? x_origin : x_origin - 1));

			S3_IO_SET_ENHANCED_REGISTER( 
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
				y_origin - font_p->si_font_info.SFlascent);

			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 
				((string_width > 0 ? string_width : -string_width) - 1));

			S3_IO_SET_ENHANCED_REGISTER( 
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
				((font_p->si_font_info.SFldescent + 
				  font_p->si_font_info.SFlascent - 1) & 
				 S3_MULTIFUNC_VALUE_BITS));

			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CMD,
				cmd_rect_fill);
		}
	}

	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
	S3_SET_BG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);
	/*
	 * Draw each glyph.
	 */
	if (screen_state_p->use_mmio)
	{
		do
		{
			const int glyph_index = *glyph_list_start_p;
			
			register struct s3_glyph *const glyph_p = 
				font_glyphs_p + glyph_index;
			
			register int x_left = x_origin + glyph_p->si_glyph_p->SFlbearing; 

			register int glyph_width  = glyph_p->si_glyph_p->SFrbearing - 
				glyph_p->si_glyph_p->SFlbearing - 1; 

			register int y_top = y_origin - glyph_p->si_glyph_p->SFascent;	

			register int glyph_height = glyph_p->si_glyph_p->SFdescent + 
				glyph_p->si_glyph_p->SFascent - 1;	
			
			ASSERT(IS_OBJECT_STAMPED(S3_GLYPH, glyph_p));

			/* 
			 * at least one pixel should be visible : we can't make a
			 * similar check for the y coordinates as there could be a
			 * individual characters which don't get to be seen.
			 */		
			ASSERT((x_left + glyph_width >=
					screen_state_p->generic_state.screen_clip_left) &&
				   (x_left <=
					screen_state_p->generic_state.screen_clip_right));
			
#if (defined(__DEBUG__))
			if (s3_font_debug)
			{
				(void) fprintf(debug_stream_p,
					"\t{\n"
					"\t\tglyph_index = %d\n"
					"\t\tx_origin = %d\n"
					"\t\tx_left = %d\n"
					"\t\tglyph_width = %d\n"
					"\t\ty_top = %d\n"
					"\t\tglyph_height = %d\n",
					glyph_index, x_origin, x_left, glyph_width, 
					y_top, glyph_height);
			}
#endif
			/*
			 * There could be some glyphs with null width and height.
			 * ignore them.
			 */
			if ((glyph_height < 0) || (glyph_width < 0))
			{
				x_origin += glyph_p->si_glyph_p->SFwidth;
				continue;
			}

			/*
			 * If this glyph is present in offscreen memor, we can 
			 * stipple useng screen to screen stipple blit.
			 */
			if (OMM_LOCK(glyph_p->offscreen_allocation_p))
			{
				/*
				 * Glyph is present in the offscreen memory.
				 */
				struct omm_allocation *allocation_p = 
					glyph_p->offscreen_allocation_p;
#if (defined(__DEBUG__))
				if (s3_font_debug)
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
					screen_state_p->generic_state.screen_physical_height);
				
				/*
				 * CPU data determines the mix register selected.
				 */
				S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_VID_MEM);

#if (defined(__DEBUG__))
				if (s3_font_debug)
				{
					(void) fprintf(debug_stream_p,
					"\t\t# Stippling from video memory.\n");
				}
#endif

				S3_MMIO_FONT_RENDER_NON_TERMINAL_GLYPH_FROM_SCREEN(glyph_p,
					allocation_p, x_left, glyph_width, y_top, glyph_height, 
					cmd_screen_to_screen);
				
				OMM_UNLOCK(allocation_p);
			}
			else
			{
				/*
				 * This glyph is not present in offscreen memory : 
				 * blit using the pixtrans registers.
				 */

				/*
				 * CPU data determines the mix register selected.
				 */
				S3_SET_PIX_CNTL_MIX_REGISTER_SELECT( 
					PIX_CNTL_DT_EX_SRC_CPU_DATA);

#if (defined(__DEBUG__))
				if (s3_font_debug)
				{
					(void) fprintf(debug_stream_p,
					"\t\t# Stippling through the pixtrans register.\n");
				}
#endif
				S3_MMIO_FONT_RENDER_NON_TERMINAL_GLYPH_FROM_MEMORY(
					screen_state_p, glyph_p, x_left, y_top, 
					glyph_height, cmd_memory_to_screen);
			}

#if (defined(__DEBUG__))
			if (s3_font_debug)
			{
				(void) fprintf(debug_stream_p, "\t}\n");
			}
#endif
			
			/*
			 * Adjust the origin for the next glyph
			 */
			x_origin += glyph_p->si_glyph_p->SFwidth;

		} while (++glyph_list_start_p < glyph_list_end_p);
	}
	else
	{
		do
		{
			const int glyph_index = *glyph_list_start_p;
			
			register struct s3_glyph *const glyph_p = 
				font_glyphs_p + glyph_index;
			
			register int x_left = x_origin + glyph_p->si_glyph_p->SFlbearing; 

			register int glyph_width  = glyph_p->si_glyph_p->SFrbearing - 
				glyph_p->si_glyph_p->SFlbearing - 1; 

			register int y_top = y_origin - glyph_p->si_glyph_p->SFascent;	

			register int glyph_height = glyph_p->si_glyph_p->SFdescent + 
				glyph_p->si_glyph_p->SFascent - 1;	
			
			ASSERT(IS_OBJECT_STAMPED(S3_GLYPH, glyph_p));

			/* 
			 * at least one pixel should be visible : we can't make a
			 * similar check for the y coordinates as there could be a
			 * individual characters which don't get to be seen.
			 */		
			ASSERT((x_left + glyph_width >=
					screen_state_p->generic_state.screen_clip_left) &&
				   (x_left <=
					screen_state_p->generic_state.screen_clip_right));
			
#if (defined(__DEBUG__))
			if (s3_font_debug)
			{
				(void) fprintf(debug_stream_p,
					"\t{\n"
					"\t\tglyph_index = %d\n"
					"\t\tx_origin = %d\n"
					"\t\tx_left = %d\n"
					"\t\tglyph_width = %d\n"
					"\t\ty_top = %d\n"
					"\t\tglyph_height = %d\n",
					glyph_index, x_origin, x_left, glyph_width, 
					y_top, glyph_height);
			}
#endif

			/*
			 * There could be some glyphs with null width and height.
			 * ignore them.
			 */
			if ((glyph_height < 0) || (glyph_width < 0))
			{
				x_origin += glyph_p->si_glyph_p->SFwidth;
				continue;
			}

			/*
			 * If this glyph is present in offscreen memor, we can 
			 * stipple useng screen to screen stipple blit.
			 */
			if (OMM_LOCK(glyph_p->offscreen_allocation_p))
			{
				/*
				 * Glyph is present in the offscreen memory.
				 */
				struct omm_allocation *allocation_p = 
					glyph_p->offscreen_allocation_p;
#if (defined(__DEBUG__))
				if (s3_font_debug)
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
					screen_state_p->generic_state.screen_physical_height);
				
				/*
				 * CPU data determines the mix register selected.
				 */
				S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_VID_MEM);

#if (defined(__DEBUG__))
				if (s3_font_debug)
				{
					(void) fprintf(debug_stream_p,
					"\t\t# Stippling from video memory.\n");
				}
#endif

				S3_IO_FONT_RENDER_NON_TERMINAL_GLYPH_FROM_SCREEN(glyph_p,
					allocation_p, x_left, glyph_width, y_top, glyph_height, 
					cmd_screen_to_screen);
				
				OMM_UNLOCK(allocation_p);
			}
			else
			{
				/*
				 * This glyph is not present in offscreen memory : 
				 * blit using the pixtrans registers.
				 */

				/*
				 * CPU data determines the mix register selected.
				 */
				S3_SET_PIX_CNTL_MIX_REGISTER_SELECT( 
					PIX_CNTL_DT_EX_SRC_CPU_DATA);

#if (defined(__DEBUG__))
				if (s3_font_debug)
				{
					(void) fprintf(debug_stream_p,
					"\t\t# Stippling through the pixtrans register.\n");
				}
#endif
				S3_IO_FONT_RENDER_NON_TERMINAL_GLYPH_FROM_MEMORY(
					screen_state_p, glyph_p, x_left, y_top, 
					glyph_height, cmd_memory_to_screen);
			}

#if (defined(__DEBUG__))
			if (s3_font_debug)
			{
				(void) fprintf(debug_stream_p, "\t}\n");
			}
#endif
			
			/*
			 * Adjust the origin for the next glyph
			 */
			x_origin += glyph_p->si_glyph_p->SFwidth;

		} while (++glyph_list_start_p < glyph_list_end_p);
	}

	S3_WAIT_FOR_FIFO(1);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_RD_MASK,
	(screen_state_p)->register_state.s3_enhanced_commands_registers.read_mask);

#if (defined(__DEBUG__))
	if (s3_font_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif
	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
}


STATIC void
s3_font_draw_terminal_glyphs(
	struct s3_font *font_p,
	short *glyph_list_start_p,
	short *glyph_list_end_p,
	int x_origin,
	int y_origin,
	int string_width,
	int draw_flags)
{

	S3_CURRENT_SCREEN_STATE_DECLARE();
	struct s3_enhanced_commands_register_state *s3_enhanced_registers_p =
		&(screen_state_p->register_state.s3_enhanced_commands_registers);
	
	struct s3_glyph *const font_glyphs_p = font_p->font_glyphs_p;

	/*
	 * command register values for memory to screen and 
	 * screen to screen stippling.
	 */
	const unsigned short cmd_memory_to_screen =
		screen_state_p->cmd_flags  |
		S3_CMD_TYPE_RECTFILL |
		S3_CMD_WRITE |
		S3_CMD_DRAW |
		S3_CMD_USE_PIXTRANS |
		S3_CMD_DIR_TYPE_AXIAL |
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_PX_MD_ACROSS_PLANE |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;

	const unsigned short cmd_screen_to_screen =
		screen_state_p->cmd_flags  |
		S3_CMD_TYPE_BITBLT |
		S3_CMD_WRITE |
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL |
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_PX_MD_ACROSS_PLANE |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;


	const int width = font_p->si_font_info.SFmax.SFwidth;
	const int y_top = y_origin - font_p->si_font_info.SFmax.SFascent;
	const int height = font_p->si_font_info.SFmax.SFascent +
		font_p->si_font_info.SFmax.SFdescent - 1;

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	ASSERT((font_p->font_kind == S3_FONT_KIND_TERMINAL_FONT)||
		(font_p->font_kind == S3_FONT_KIND_SMALL_TERMINAL_FONT));
	
	ASSERT((y_top <= screen_state_p->generic_state.screen_clip_bottom) &&
		   ((y_top + height) >= screen_state_p->generic_state.screen_clip_top));

	/*
	 * If width or height is invalid, do not attempt to draw.
	 * Note that height here is already decremented by 1 to be directly 
	 * programmed into the minor axis pixel count register.
	 */
	if ((height < 0) || (width <= 0))
	{
		return;
	}

	/*
	 * Foreground color from the frgd register and the background color
	 * from the bkgd register.
	 */
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
	S3_SET_BG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);
	/*
	 * Check if this is a small terminal font. In this case the drawing
	 * is much simpler/faster.
	 */
	if(font_p->font_kind == S3_FONT_KIND_SMALL_TERMINAL_FONT)
	{
		const struct omm_allocation *const allocation_p = 
			font_p->small_terminal_fonts_allocation_p;
		unsigned short 	current_planemask = 0U;
		register int	current_x = x_origin;

		ASSERT(allocation_p != NULL);
		ASSERT(allocation_p->x >= 0 &&
			allocation_p->x + allocation_p->width <= 
			screen_state_p->generic_state.screen_physical_width);
		ASSERT(allocation_p->y >= 0 &&
			allocation_p->y + allocation_p->height <= 
			screen_state_p->generic_state.screen_physical_height);
		
		ASSERT(allocation_p->height >= 0 && allocation_p->width >= 0);

#if (defined(__DEBUG__))
		if (s3_font_debug)
		{
			(void) fprintf(debug_stream_p,
				"(s3_font_draw_terminal_glyphs,small_terminal_fonts) {\n"
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
				string_width, draw_flags, width, height,
				allocation_p->x, allocation_p->y,
				allocation_p->width, allocation_p->height,
				allocation_p->planemask);
		}
#endif
		/*
		 * Offscreen memory data determines the mix register selected.
		 */
		S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_VID_MEM);

		/*
		 * Program the parameters that are loop invariant.
		 */
		S3_WAIT_FOR_FIFO(3);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
			allocation_p->y);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
			width -1 );
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
			((height) & S3_MULTIFUNC_VALUE_BITS));
	
		if (screen_state_p->use_mmio)
		{
			do
			{
				register const struct s3_glyph *const glyph_p =
					&(font_glyphs_p[*glyph_list_start_p]);

				ASSERT(IS_OBJECT_STAMPED(S3_GLYPH, glyph_p));
				ASSERT(glyph_p->offscreen_allocation_p == NULL);
#if (defined(__DEBUG__))
				if (s3_font_debug)
				{
					(void) fprintf(debug_stream_p,
						"\t{\n"
						"\t\tglyph_index = %hd\n"
						"\t\tterminal_font_x_coordinate = %d\n"
						"\t\ttermial_font_planemask = 0x%x\n"
						"\t}\n",
						*glyph_list_start_p,
						glyph_p->small_terminal_font_x_coordinate,
						glyph_p->small_terminal_font_planemask);
				}
#endif

				ASSERT(current_x + width >=
				   screen_state_p->generic_state.screen_clip_left &&
				   current_x <= 
				   screen_state_p->generic_state.screen_clip_right);
				
				ASSERT(glyph_p->small_terminal_font_planemask != 0);
				
				/*
				 * Update the plane mask if we have switched the plane.
				 */
				if(glyph_p->small_terminal_font_planemask != current_planemask)
				{
					current_planemask = glyph_p->small_terminal_font_planemask;
					S3_WAIT_FOR_FIFO(1);
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_RD_MASK, 
						current_planemask);
				}
				S3_INLINE_WAIT_FOR_FIFO(4);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X,
					glyph_p->small_terminal_font_x_coordinate);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP, current_x);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP, y_top);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CMD, cmd_screen_to_screen);
				
				current_x += width;
			} while (++glyph_list_start_p < glyph_list_end_p);
		}
		else
		{
			do
			{
				register const struct s3_glyph *const glyph_p =
					&(font_glyphs_p[*glyph_list_start_p]);

				ASSERT(IS_OBJECT_STAMPED(S3_GLYPH, glyph_p));
				ASSERT(glyph_p->offscreen_allocation_p == NULL);
#if (defined(__DEBUG__))
				if (s3_font_debug)
				{
					(void) fprintf(debug_stream_p,
						"\t{\n"
						"\t\tglyph_index = %hd\n"
						"\t\tsmall_terminal_font_x_coordinate = %d\n"
						"\t\ttermial_font_planemask = 0x%x\n"
						"\t}\n",
						*glyph_list_start_p,
						glyph_p->small_terminal_font_x_coordinate,
						glyph_p->small_terminal_font_planemask);
				}
#endif

				ASSERT(current_x + width >=
				   screen_state_p->generic_state.screen_clip_left &&
				   current_x <= 
				   screen_state_p->generic_state.screen_clip_right);
				
				ASSERT(glyph_p->small_terminal_font_planemask != 0);
				
				/*
				 * Update the plane mask if we have switched the plane.
				 */
				if(glyph_p->small_terminal_font_planemask != current_planemask)
				{
					current_planemask = glyph_p->small_terminal_font_planemask;
					S3_WAIT_FOR_FIFO(1);
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_RD_MASK,
						current_planemask);
				}
				S3_INLINE_WAIT_FOR_FIFO(4);
				S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,
					glyph_p->small_terminal_font_x_coordinate);
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP, current_x);
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP, y_top);
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CMD, cmd_screen_to_screen);
				
				current_x += width;
			} while (++glyph_list_start_p < glyph_list_end_p);
		}
#if (defined(__DEBUG__))
		if (s3_font_debug)
		{
			(void) fprintf(debug_stream_p,
			"}\n");
		}
#endif
	}
	else
	{
		if (screen_state_p->use_mmio)
		{
			do
			{
				const int glyph_index = *glyph_list_start_p;
				
				register struct s3_glyph *const glyph_p = 
					font_glyphs_p + glyph_index;
				
				ASSERT(IS_OBJECT_STAMPED(S3_GLYPH, glyph_p));
				
				/* 
				 * at least one pixel should be visible 
				 */		
				ASSERT((x_origin + width >=
						screen_state_p->generic_state.screen_clip_left) &&
					   (x_origin <=
						screen_state_p->generic_state.screen_clip_right));
				
#if (defined(__DEBUG__))
				if (s3_font_debug)
				{
						(void) fprintf(debug_stream_p,
									   "\t{\n"
									   "\t\tglyph_index = %d\n",
									   glyph_index);
					}
#endif
					if (OMM_LOCK(glyph_p->offscreen_allocation_p))
					{
						/*
						 * Use screen to screen stippling if the glyph is 
						 * present in offscreen memory.
						 */
						struct omm_allocation *allocation_p = 
							glyph_p->offscreen_allocation_p;
#if (defined(__DEBUG__))
						if (s3_font_debug)
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
						screen_state_p->generic_state.screen_physical_height);
						ASSERT(allocation_p->height >= 0 && 
						allocation_p->width >= 0);
						ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION, allocation_p));
						
						/*
						 * Video memory determines the mix register selected.
						 */
						S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(
							PIX_CNTL_DT_EX_SRC_VID_MEM);
						
						S3_MMIO_FONT_RENDER_TERMINAL_GLYPH_FROM_SCREEN(
							allocation_p, x_origin, (width-1), y_top, height,
							cmd_screen_to_screen);

						OMM_UNLOCK(allocation_p);
					}
					else
					{
						/*
						 * The glyph is not present in offscreen memory : 
						 * blit using the pixtrans registers.
						 */
						/*
						 * CPU data determines the mix register selected.
						 */
						S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(
							PIX_CNTL_DT_EX_SRC_CPU_DATA);

#if (defined(__DEBUG__))
						if (s3_font_debug)
						{
							(void) fprintf(debug_stream_p,
								"\t\t# Stippling thro pixtrans register.\n");
						}
#endif
						S3_MMIO_FONT_RENDER_TERMINAL_GLYPH_FROM_MEMORY(
							screen_state_p, glyph_p, x_origin, y_top, height, 
							cmd_memory_to_screen);
					}
				/*
				 * Adjust the origin for the next glyph
				 */
				x_origin += width;
#if (defined(__DEBUG__))
				if (s3_font_debug)
				{
					(void) fprintf(debug_stream_p, "\t}\n");
				}
#endif
			} while (++glyph_list_start_p < glyph_list_end_p);
		}
		else
		{
			/*
			 * Pure IO writes.
			 */
			do
			{
				const int glyph_index = *glyph_list_start_p;
				
				register struct s3_glyph *const glyph_p = 
					font_glyphs_p + glyph_index;
				
				ASSERT(IS_OBJECT_STAMPED(S3_GLYPH, glyph_p));
				
				/* 
				 * at least one pixel should be visible 
				 */		
				ASSERT((x_origin + width >=
						screen_state_p->generic_state.screen_clip_left) &&
					   (x_origin <=
						screen_state_p->generic_state.screen_clip_right));
				
#if (defined(__DEBUG__))
				if (s3_font_debug)
				{
						(void) fprintf(debug_stream_p,
									   "\t{\n"
									   "\t\tglyph_index = %d\n",
									   glyph_index);
					}
#endif
					if (OMM_LOCK(glyph_p->offscreen_allocation_p))
					{
						/*
						 * Use screen to screen stippling if the glyph is 
						 * present in offscreen memory.
						 */
						struct omm_allocation *allocation_p = 
							glyph_p->offscreen_allocation_p;
#if (defined(__DEBUG__))
						if (s3_font_debug)
						{
							(void) fprintf(debug_stream_p,
								"\t\t# (big terminal glyphs) Offscreen memory\n"
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
						screen_state_p->generic_state.screen_physical_height);
						ASSERT(allocation_p->height >= 0 && 
						allocation_p->width >= 0);
						ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION, allocation_p));
						
						/*
						 * Video memory determines the mix register selected.
						 */
						S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(
							PIX_CNTL_DT_EX_SRC_VID_MEM);
						
						S3_IO_FONT_RENDER_TERMINAL_GLYPH_FROM_SCREEN(
							allocation_p, x_origin, (width-1), y_top, height,
							cmd_screen_to_screen);

						OMM_UNLOCK(allocation_p);
					}
					else
					{
						/*
						 * The glyph is not present in offscreen memory : 
						 * blit using the pixtrans registers.
						 */
						/*
						 * CPU data determines the mix register selected.
						 */
						S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(
							PIX_CNTL_DT_EX_SRC_CPU_DATA);

#if (defined(__DEBUG__))
						if (s3_font_debug)
						{
							(void) fprintf(debug_stream_p,
								"\t\t# big terminal glyphs\n"
								"\t\t# Stippling thro pixtrans register.\n");
						}
#endif
			 			S3_IO_FONT_RENDER_TERMINAL_GLYPH_FROM_MEMORY(
							screen_state_p, glyph_p, x_origin, y_top, height, 
							cmd_memory_to_screen);
					}
				/*
				 * Adjust the origin for the next glyph
				 */
				x_origin += width;
#if (defined(__DEBUG__))
				if (s3_font_debug)
				{
					(void) fprintf(debug_stream_p, "\t}\n");
				}
#endif
			} while (++glyph_list_start_p < glyph_list_end_p);
		}
	}
	
	S3_WAIT_FOR_FIFO(1);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_RD_MASK,
	(screen_state_p)->register_state.s3_enhanced_commands_registers.read_mask);

	/*
	 * We may have reset the clip rectangle while drawing terminal
	 * glyphs.  Reprogram the clip rectangle left and right from the
	 * shadow copy.
	 */
	S3_WAIT_FOR_FIFO(2);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX |
		(s3_enhanced_registers_p->scissor_l & S3_MULTIFUNC_VALUE_BITS)));
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
		(s3_enhanced_registers_p->scissor_r & S3_MULTIFUNC_VALUE_BITS)));
	
#if (defined(__DEBUG__))
	if (s3_font_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
}

/*
 * s3_font_download_font
 *
 * Download a font from the SI layer into the SDD's system memory.
 */

STATIC SIBool
s3_font_download_font(
	SIint32 font_index,
	SIFontInfoP font_info_p,
	SIGlyphP glyph_list_p)
{
	struct s3_font	*font_p;
	int					max_glyph_width_in_pixels;
	const int			max_glyph_height_in_pixels = 
							font_info_p->SFmax.SFascent + 
							font_info_p->SFmax.SFdescent;
	int					size_of_max_glyph_in_bytes;
	const SIGlyphP		last_si_glyph_p = 
							glyph_list_p + font_info_p->SFnumglyph;
	SIGlyphP			current_si_glyph_p;
	struct s3_glyph		*current_s3_glyph_p;
	unsigned char		*glyph_bits_p;
	int *current_packed_glyph_bits_p = NULL;

#if (defined(__DEBUG__))
	const unsigned int           s3_debug_stamp = S3_FONT_DEBUG_STAMP;
#endif

	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURRENT_FONT_STATE_DECLARE();
	
	const int pixtrans_width_in_bytes = screen_state_p->pixtrans_width >> 3;
	
	boolean is_terminal_font = FALSE; /* is the font a terminal font */
	boolean is_small_terminal_font = FALSE;
	boolean is_packed_small_terminal_font = FALSE;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_FONT_STATE, font_state_p));


#if (defined(__DEBUG__))
	if (s3_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_font_download_font)\n"
			"{\n"
			"\tfont_index = %ld\n"
			"\tfont_info_p = %p\n"
			"\tglyph_list_p = %p\n",
			font_index, (void *) font_info_p, (void *) glyph_list_p);
	}
#endif

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	if ((font_index < 0) || 
		(font_index >=
		 font_state_p->max_number_of_downloadable_fonts))
	{
#if (defined(__DEBUG__))
		if (s3_font_debug)
		{
		(void) fprintf(debug_stream_p,
			"(s3_font_download_font)\n"
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
		if (s3_font_debug)
		{
		(void) fprintf(debug_stream_p,
			"(s3_font_download_font)\n"
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
	 * Allocate zeroed memory for one s3_font structure and
	 * tuck away the pointer in the s3 font state structure.
	 */
	font_p = allocate_and_clear_memory(sizeof(struct s3_font));

	font_state_p->font_list_pp[font_index] = font_p;

	/*
	 * Copy the fontinfo structure.
	 */
	font_p->si_font_info = *font_info_p;

	ASSERT(font_info_p->SFnumglyph > 0);

	is_terminal_font = 
		(font_info_p->SFflag & SFTerminalFont) ? TRUE : FALSE;
	
	/*
	 * Allocate space for the s3_glyph structures .
	 */
	font_p->font_glyphs_p = 
		allocate_and_clear_memory(font_info_p->SFnumglyph *
								  sizeof(struct s3_glyph));
	/*
	 * Compute the space required to hold the largest character in the
	 * font and round off max_glyph_width_in_pixels to integral
	 * number of pixtrans words.
	 */
	max_glyph_width_in_pixels = 
		((font_info_p->SFmax.SFrbearing - font_info_p->SFmin.SFlbearing) + 
		 screen_state_p->pixtrans_width - 1) & 
		 ~(screen_state_p->pixtrans_width - 1);

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
								  + sizeof(s3_debug_stamp));
	*((int *) (font_p->font_glyph_bits_p + 
			   (size_of_max_glyph_in_bytes * font_info_p->SFnumglyph))) =
		  s3_debug_stamp;
#else
	/*
	 * Allocate space for the glyph bits.
	 */
	font_p->font_glyph_bits_p = 
	    allocate_and_clear_memory((size_of_max_glyph_in_bytes *
								   font_info_p->SFnumglyph) );

		
#endif

	/*
	 * Check if we can assemble glyphs at drawing time
	 * (*Life is easier without 4 bit modes*)
	 */

	if (is_terminal_font &&
		(screen_state_p->options_p->fontdraw_options & 
		S3_OPTIONS_FONTDRAW_OPTIONS_ASSEMBLE_GLYPHS) &&
		screen_state_p->bus_width == S3_BUS_WIDTH_16 &&
		screen_state_p->generic_state.screen_depth >= 8)
	{

		font_p->number_of_glyph_lines_per_word = 
			32 / 
			(font_info_p->SFmax.SFrbearing - font_info_p->SFmin.SFlbearing);

		/*
		 * Its not worth packing 2 glyphs in a word
		 */

		if (font_p->number_of_glyph_lines_per_word > 2)
		{
			current_packed_glyph_bits_p = 
				font_p->font_packed_glyph_bits_p = 
				allocate_and_clear_memory(
				(sizeof(int) * max_glyph_height_in_pixels *
			   font_info_p->SFnumglyph));

			is_packed_small_terminal_font = TRUE;
		}

	}


	/*
	 * Now for each glyph in the font fill in the s3_glyph structure
	 * and also compute and fill the inverted bits.
	 */
	current_si_glyph_p = glyph_list_p;
	current_s3_glyph_p = font_p->font_glyphs_p;
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
		current_s3_glyph_p->si_glyph_p = allocate_memory(sizeof(SIGlyph));
		*current_s3_glyph_p->si_glyph_p = *current_si_glyph_p;

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
		current_s3_glyph_p->glyph_padded_width = 
			(glyph_width_in_pixels + screen_state_p->pixtrans_width - 1) &
			~(screen_state_p->pixtrans_width - 1); 

		padded_width_in_bytes = current_s3_glyph_p->glyph_padded_width >> 3;

		/*
		 * Compute the total pixtrans register transfers for 
		 * rendering this glyph from system memory.
		 */
		numwords_per_glyph_line = 
			current_s3_glyph_p->glyph_padded_width >>
			screen_state_p->pixtrans_width_shift;
		current_s3_glyph_p->glyph_pixtrans_transfers =
			numwords_per_glyph_line *
			glyph_height_in_pixels;

		/*
		 * pointer to the bits.
		 */
		current_s3_glyph_p->glyph_bits_p = glyph_bits_p;  

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

			if (is_packed_small_terminal_font)
			{
				int tmp_bits = 0;
				int packed_data = 0;
				int full_bytes = glyph_width_in_pixels / 8;
				int partial_bits_count = glyph_width_in_pixels % 8;
				int partial_bits_mask = 
					(1 << partial_bits_count) - 1;
				int height = 
					current_si_glyph_p->SFascent +
					current_si_glyph_p->SFdescent; 
				unsigned char *src_p = 
				 (unsigned char *)current_si_glyph_p->SFglyph.Bptr;
				int *dst_p =
					current_packed_glyph_bits_p;
				int glyph_line_mask =
					 (1 << glyph_width_in_pixels) - 1;
				int numbytes_per_glyph_bitmap_line =
					((glyph_width_in_pixels + 31) & ~31) >> 3;

				
				while(height)
				{
					int tmp_full_bytes = full_bytes;
					unsigned char *tmp_p = src_p;


					while(tmp_full_bytes > 0)
					{
						tmp_bits = 
							*tmp_p++;

						tmp_bits = 
							screen_state_p->byte_invert_table_p[tmp_bits];

						packed_data <<=  8;
						packed_data |= tmp_bits & 0xFF;
						--tmp_full_bytes;
					}

					if (partial_bits_count > 0)
					{
						tmp_bits = 
							*tmp_p;

						tmp_bits = 
							screen_state_p->byte_invert_table_p[tmp_bits];

						tmp_bits >>= (8 - partial_bits_count);

						packed_data <<= partial_bits_count;

						packed_data |=
							 tmp_bits & partial_bits_mask;

					}

					--height;
					*dst_p++ = packed_data & glyph_line_mask;
					src_p += numbytes_per_glyph_bitmap_line;
				}
		
			}
			
		}
		
		STAMP_OBJECT(S3_GLYPH, current_s3_glyph_p);

		/*
		 * Move over to the next glyph.
		 */

		glyph_bits_p += size_of_max_glyph_in_bytes;
		++current_s3_glyph_p;
		++current_si_glyph_p;

		if (is_packed_small_terminal_font)
		{
			current_packed_glyph_bits_p +=
				 max_glyph_height_in_pixels;
		}
	}

	/*
	 * Download Terminal fonts that can be classified as "small" into 
	 * offscreen memory right away. Others are done at drawing time.
	 * Try to keep the fonts in the same plane to avoid programming
	 * the planemask seperately at font rendering time.
	 */
	if ((screen_state_p->options_p->fontdraw_options &
		S3_OPTIONS_FONTDRAW_OPTIONS_USE_OFFSCREEN_MEMORY) &&
		(is_terminal_font == TRUE) &&
		(is_packed_small_terminal_font == FALSE))
	{
		int planes_required = 1; /* planes of offscreen memory
									required for the font */

		const int glyph_width_in_pixels =
			font_p->si_font_info.SFmax.SFrbearing -
			font_p->si_font_info.SFmin.SFlbearing;
		
		int glyphs_per_plane =	font_p->si_font_info.SFnumglyph;
		
		struct omm_allocation *allocation_p = NULL;
		
		/*
		 * Loop till we get a suitable block of offscreen memory or
		 * we cross the depth of the screen.
		 */
		while(planes_required <= screen_state_p->generic_state.screen_depth)
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
			glyphs_per_plane = (font_p->si_font_info.SFnumglyph  + 
				planes_required - 1 ) / planes_required;
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
			const unsigned short command =
				screen_state_p->cmd_flags |
				S3_CMD_TYPE_RECTFILL |
				S3_CMD_WRITE |
				S3_CMD_DRAW |
				S3_CMD_USE_PIXTRANS |
				S3_CMD_DIR_TYPE_AXIAL |
				S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
				S3_CMD_AXIAL_X_MAJOR |
				S3_CMD_PX_MD_ACROSS_PLANE |
				S3_CMD_AXIAL_Y_TOP_TO_BOTTOM ;

#if (defined(__DEBUG__))
			if (s3_font_debug)
			{
				(void) fprintf(debug_stream_p,
					   "#small terminal fonts allocation{\n"
					   "\t font id = %ld\n"
					   "\t number_of_glyphs = %ld\n"
					   "\t glyphs_per_plane = %d, planes_required = %d\n"
					   "\t allocation = {\n"
					   "\t\tx = %d, y = %d, w = %d, h = %d, mask = 0x%x\n"
					   "\t}\n"
					   "}\n",
					   font_index, 
					   font_p->si_font_info.SFnumglyph,
					   glyphs_per_plane, planes_required,
					   allocation_p->x, allocation_p->y,
					   allocation_p->width, allocation_p->height, 
					   allocation_p->planemask);
			}
#endif
			
			ASSERT(glyphs_per_plane*planes_required >= 
				   font_p->si_font_info.SFnumglyph);
			ASSERT(allocation_p && (allocation_p->x >= 0 && 
				   allocation_p->x + allocation_p->width <=
				   screen_state_p->generic_state.screen_physical_width));
			ASSERT(allocation_p && (allocation_p->y >= 0 &&
				   allocation_p->y + allocation_p->height <= 
				   (screen_state_p->generic_state.screen_physical_height)));

			ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION, allocation_p));

			font_p->small_terminal_fonts_allocation_p = allocation_p;
			is_small_terminal_font = TRUE;

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
			 * Reset the clip rectangle to allocated offscreen memory.
			 */
			screen_state_p->generic_state.screen_current_clip
				= GENERIC_CLIP_NULL;
			
			S3_STATE_SET_CLIP_RECTANGLE( screen_state_p,
				allocation_p->x, allocation_p->y,
				allocation_p->x + allocation_p->width,
				allocation_p->y + allocation_p->height);
			
			S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);
			
			S3_SET_PIX_CNTL_MIX_REGISTER_SELECT( PIX_CNTL_DT_EX_SRC_CPU_DATA);

			S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
			S3_SET_BG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);

			S3_STATE_SET_FRGD_COLOR(screen_state_p, (unsigned short) ~0U);
			S3_STATE_SET_BKGD_COLOR(screen_state_p, 0);
			
			S3_STATE_SET_FG_ROP(screen_state_p, S3_MIX_FN_N);
			S3_STATE_SET_BG_ROP(screen_state_p, S3_MIX_FN_N);
			
			for (count = 0; count < font_p->si_font_info.SFnumglyph; ++count)
			{
			
				struct s3_glyph *const glyph_p = 
					&(font_p->font_glyphs_p[count]);
				
				ASSERT(IS_OBJECT_STAMPED(S3_GLYPH, glyph_p));

				glyph_p->small_terminal_font_planemask = current_planemask;

				glyph_p->small_terminal_font_x_coordinate = 
					current_x_coordinate;

				ASSERT(current_x_coordinate + glyph_width_in_pixels <= 
					max_x_coordinate);
				
				ASSERT(current_planemask & allocation_p->planemask);

				S3_WAIT_FOR_FIFO(6);
				if (screen_state_p->use_mmio)
				{
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_WRT_MASK,
						current_planemask);
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_X,
						current_x_coordinate);
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
						allocation_p->y);
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
						glyph_p->glyph_padded_width - 1);
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
						S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
						((max_glyph_height_in_pixels -1) & 
						S3_MULTIFUNC_VALUE_BITS));
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}
				else
				{
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_WRT_MASK,
						current_planemask);
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_X,
						current_x_coordinate);
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
						allocation_p->y);
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
						glyph_p->glyph_padded_width - 1);
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
						S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
						((max_glyph_height_in_pixels -1) & 
						S3_MULTIFUNC_VALUE_BITS));
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}

				S3_WAIT_FOR_FIFO(8);
				(*screen_state_p->screen_write_bits_p)(
					screen_state_p->pixtrans_register,
					glyph_p->glyph_pixtrans_transfers,
					glyph_p->glyph_bits_p);
				
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
			 * Resynchronize the hardware write mask register with the in
			 * memory copy.
			 */
			S3_WAIT_FOR_FIFO(1);
			S3_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_WRT_MASK,
				screen_state_p->register_state.
				s3_enhanced_commands_registers.write_mask);
		}
	}

	/*
	 * Set the pointers to the font draw functions.
	 */

	if (is_terminal_font)
	{
		if (is_packed_small_terminal_font)
		{
			if(S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_WRITE_PIXTRANS(
					screen_state_p) &&
			S3_MEMORY_REGISTER_ACCESS_MODE_MMIO_WRITE_ENHANCED_REGS(
					screen_state_p))
			{
				font_p->draw_glyphs =
					 s3_font_draw_packed_terminal_glyphs_mmio;
			}
			else
			{
				font_p->draw_glyphs =
					 s3_font_draw_packed_terminal_glyphs_io;
			}

			font_p->font_kind = 
				S3_FONT_KIND_PACKED_SMALL_TERMINAL_FONT;
		}
		else
		{
			if (is_small_terminal_font)
			{
				font_p->font_kind = S3_FONT_KIND_SMALL_TERMINAL_FONT;
			}
			else
			{
				font_p->font_kind = S3_FONT_KIND_TERMINAL_FONT;
			}

			font_p->draw_glyphs = s3_font_draw_terminal_glyphs;
		}
	}
	else
	{
		font_p->font_kind = S3_FONT_KIND_NON_TERMINAL_FONT;
		font_p->draw_glyphs = s3_font_draw_non_terminal_glyphs;
	}

#if (defined(__DEBUG__))
	if (s3_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"\t\tfont_kind = %s\n"
			"}\n",
			s3_font_kind_to_dump[font_p->font_kind]);
	}
#endif
	
	STAMP_OBJECT(S3_FONT, font_p);
	
	ASSERT(IS_OBJECT_STAMPED(S3_FONT_STATE, font_state_p));

	ASSERT(*((int *) (font_p->font_glyph_bits_p + 
					  (size_of_max_glyph_in_bytes *
					   font_info_p->SFnumglyph))) == s3_debug_stamp);


	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return (SI_SUCCEED);
}


STATIC SIBool
s3_font_free_font(SIint32 font_index)
{
	
	struct s3_font	*font_p;
	int					numglyphs;
	struct s3_glyph	*current_s3_glyph_p;

	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURRENT_FONT_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_FONT_STATE, font_state_p));

#if (defined(__DEBUG__))
	if (s3_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_font_free_font)\n"
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
		if (s3_font_debug)
		{
		(void) fprintf(debug_stream_p,
			"(s3_font_free_font)\n"
			"{\n"
			"\t# Failing because index is out of bounds.\n"
			"}\n");
		}
#endif
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return (SI_FAIL);
	}

	font_p = font_state_p->font_list_pp[font_index] ;

	ASSERT(IS_OBJECT_STAMPED(S3_FONT,font_p));

	ASSERT(font_p->font_glyph_bits_p);
	
	/*
	 * Free the space allocated for the inverted bits;
	 */

	free_memory(font_p->font_glyph_bits_p);

	if (font_p->font_kind ==
			S3_FONT_KIND_PACKED_SMALL_TERMINAL_FONT)
	{
		free_memory(font_p->font_packed_glyph_bits_p);
	}

	/*
	 * Free the offscreen space allocated for the terminal fonts.
	 */
	if ((font_p->font_kind == S3_FONT_KIND_SMALL_TERMINAL_FONT) &&
		(screen_state_p->options_p->fontdraw_options &
		S3_OPTIONS_FONTDRAW_OPTIONS_USE_OFFSCREEN_MEMORY))
	{
		ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION,
			font_p->small_terminal_fonts_allocation_p));
		(void) omm_free(font_p->small_terminal_fonts_allocation_p);
	}
	/*
	 * Free the memory allocated for the s3_glyph_structures.
	 */
	numglyphs = font_p->si_font_info.SFnumglyph;
	current_s3_glyph_p = font_p->font_glyphs_p;

	ASSERT(font_p->font_kind == S3_FONT_KIND_TERMINAL_FONT ||
		   font_p->font_kind == S3_FONT_KIND_NON_TERMINAL_FONT ||
		   font_p->font_kind == S3_FONT_KIND_PACKED_SMALL_TERMINAL_FONT ||
		   font_p->font_kind == S3_FONT_KIND_SMALL_TERMINAL_FONT);
		
	/*
	 * Loop over each glyph and free any offscreen allocations
	 * made.
	 */
	if (numglyphs > 0)
	{
		do
		{
			ASSERT(IS_OBJECT_STAMPED(S3_GLYPH, current_s3_glyph_p));
#if (defined(__DEBUG))
			if(font_p->font_kind == S3_FONT_KIND_SMALL_TERMINAL_FONT)
			{
				ASSERT(current_s3_glyph_p->offscreen_allocation_p == NULL);
			}
#endif
			if (current_s3_glyph_p->offscreen_allocation_p)
			{
				ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION,
					current_s3_glyph_p->offscreen_allocation_p));
				(void) omm_free(current_s3_glyph_p->offscreen_allocation_p);
			}

			free_memory(current_s3_glyph_p->si_glyph_p);
			++current_s3_glyph_p;

		} while(--numglyphs > 0);
	}
	
	ASSERT(font_p->font_glyphs_p);
	
	free_memory(font_p->font_glyphs_p);

	ASSERT(font_p);
	
	free_memory(font_p);

	font_state_p->font_list_pp[font_index] = NULL;

	ASSERT(IS_OBJECT_STAMPED(S3_FONT_STATE, font_state_p));

	return (SI_SUCCEED);
}

STATIC SIBool
s3_font_draw_glyphs(
	const SIint32 font_index, 
	const SIint32 x,
	const SIint32 y,
	SIint32 glyph_count,
	SIint16 *glyph_list_p,
	SIint32 draw_method)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURRENT_GRAPHICS_STATE_DECLARE();
	S3_CURRENT_FONT_STATE_DECLARE();
	
	/*
	 * The font descriptor.
	 */
	struct s3_font *font_p;
	struct s3_glyph *font_glyphs_p;

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
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE, graphics_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(S3_FONT_STATE, font_state_p));

#if (defined(__DEBUG__))
	if (s3_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_font_draw_glyphs)\n"
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
	
#endif

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	if (glyph_count <= 0)
	{
		return (SI_SUCCEED);
	}

	if (font_index < 0 || font_index >=
		font_state_p->max_number_of_downloadable_fonts) 
	{
#if (defined(__DEBUG__))
		if (s3_font_debug)
		{
		(void) fprintf(debug_stream_p,
			"(s3_font_draw_glyphs)\n"
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
		S3_OPTIONS_FONTDRAW_OPTIONS_USE_OFFSCREEN_MEMORY) 
			? TRUE : FALSE;

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

	ASSERT(IS_OBJECT_STAMPED(S3_FONT, font_p));

	font_glyphs_p = font_p->font_glyphs_p;
	
	ASSERT(font_glyphs_p != NULL);
	
	/*
	 * Check for the y-coordinates visibility.
	 */
	if ((y - font_p->si_font_info.SFmax.SFascent > screen_clip_bottom) ||
		(y + font_p->si_font_info.SFmax.SFdescent <= screen_clip_top))
	{
#if (defined(__DEBUG__))
		if (s3_font_debug)
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
		int x_start = x;
		int x_end = x_start + (width * glyph_count);
		
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
			if (s3_font_debug)
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

		if ((x_end - 1)  > screen_clip_right)
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
			register const struct s3_glyph *glyph_p = 
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
			if (s3_font_debug)
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
			register const struct s3_glyph *const glyph_p = 
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
			if (s3_font_debug)
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
	if (s3_font_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif


	/*
	 * Set the draw flags.
	 */
	draw_flags = (draw_method == SGStipple) ?
		S3_FONT_DO_TRANSPARENT_STIPPLING : 
		S3_FONT_DO_OPAQUE_STIPPLING;
	
	/*
	 * Download those glyphs that are not of the font kind 
	 * SMALL_TERMINAL_FONTS. These would have been downloaded at 
	 * font download time itself.
	 *  And if font_kind is PACKED_SMALL_TERMINAL then the glphs
	 *  are rendered from system memory.
	 */
	if ((use_offscreen_memory == TRUE) && 
		(font_p->font_kind != S3_FONT_KIND_SMALL_TERMINAL_FONT) &&
		(font_p->font_kind != S3_FONT_KIND_PACKED_SMALL_TERMINAL_FONT))
	{
		SIint16 *tmp_glyph_index_p = displayed_glyph_start_p;
		const unsigned short cmd_memory_to_screen = 
			screen_state_p->cmd_flags|
			S3_CMD_TYPE_RECTFILL |
			S3_CMD_WRITE |
			S3_CMD_DRAW |
			S3_CMD_USE_PIXTRANS |
			S3_CMD_DIR_TYPE_AXIAL |
			S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
			S3_CMD_AXIAL_X_MAJOR |
			S3_CMD_PX_MD_ACROSS_PLANE |
			S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;

		screen_state_p->generic_state.screen_current_clip 
			= GENERIC_CLIP_TO_VIDEO_MEMORY;
		S3_STATE_SET_CLIP_RECTANGLE( screen_state_p,
			0, 0,
			screen_state_p->generic_state.screen_physical_width,
			screen_state_p->generic_state.screen_physical_height);
		S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);

		S3_SET_PIX_CNTL_MIX_REGISTER_SELECT( PIX_CNTL_DT_EX_SRC_CPU_DATA);

		S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
		S3_SET_BG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);

		S3_STATE_SET_FRGD_COLOR(screen_state_p, (unsigned short) ~0U);
		S3_STATE_SET_BKGD_COLOR(screen_state_p, 0);

		S3_STATE_SET_FG_ROP(screen_state_p, S3_MIX_FN_N);
		S3_STATE_SET_BG_ROP(screen_state_p, S3_MIX_FN_N);


		/*
		 * Loop through the list of glyphs to be drawn and download
		 * the necessary glyphs.
		 */
		do
		{
			struct s3_glyph *const glyph_p =
				&(font_glyphs_p[*tmp_glyph_index_p]);
			
			/*
			 * Download the glyph if it is not already present in 
			 * the offscreen memory.
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
#if (defined(__DEBUG__))
					if (s3_font_debug)
					{
						(void) fprintf(debug_stream_p,
							"\t\t# Offscreen allocation\n"
							"\t\tglyph_index = %hd\n"
							"\t\tallocation_p = %p\n"
							"\t\tallocation_x = %d\n"
							"\t\tallocation_y = %d\n"
							"\t\tallocation_planemask = 0x%x\n",
							*tmp_glyph_index_p,(void *) allocation_p,
							allocation_p->x, allocation_p->y,
							allocation_p->planemask);
					}
#endif
					/*
					 * Download the glyph. 
					 * (only if it is not null dimensioned).
					 */
					if((glyph_p->glyph_padded_width > 0) &&
					   (glyph_p->si_glyph_p->SFascent + 
						glyph_p->si_glyph_p->SFdescent > 0))
					{
						S3_FONT_DOWNLOAD_GLYPH(screen_state_p, glyph_p, 
							allocation_p, cmd_memory_to_screen);
					}
				}
				else
				{
					/*
					 * Offscreen allocation failed, 
					 * Stop asking for more memory.
					 */
#if (defined(__DEBUG__))
					if (s3_font_debug)
					{
						(void) fprintf(debug_stream_p,
							"\t\t# Allocation of offscreen area failed.\n"
							"\t\tglyph_index = %hd\n",
							*tmp_glyph_index_p);
					}
#endif
					break;
				}
			}
		} while (++tmp_glyph_index_p < displayed_glyph_end_p);
	}

	if (draw_flags & S3_FONT_DO_TRANSPARENT_STIPPLING)
	{
		/*
		 * Transparent stippling (polytext).
		 */
#if (defined(__DEBUG__))
		if (s3_font_debug)
		{
			(void) fprintf(debug_stream_p,
			"(s3_font_draw_glyphs)\n"
			"{\n"
			"\tTransparent stippling.\n");
		}
#endif
		/*
		 * Synchronize registers with the graphics state.
		 */
		S3_STATE_SYNCHRONIZE_STATE(screen_state_p,
			 S3_POLYTEXT_DEPENDENCIES);

		S3_STATE_SET_BG_ROP(screen_state_p, S3_MIX_FN_LEAVE_C_AS_IS);
	}
	else if (draw_flags & S3_FONT_DO_OPAQUE_STIPPLING)
	{
	
#if (defined(__DEBUG__))
		if (s3_font_debug)
		{
			(void) fprintf(debug_stream_p,
			"(s3_font_draw_glyphs)\n"
			"{\n"
			"\tOpaque stippling.\n");
		}
#endif
		/*
		 * Synchronize registers with the graphics state.
		 */
		S3_STATE_SYNCHRONIZE_STATE(screen_state_p, S3_IMAGETEXT_DEPENDENCIES);

		/*
		 * The foreground and background rops have to be reset to 
		 * GXcopy.
		 */
		S3_STATE_SET_FG_ROP(screen_state_p,S3_MIX_FN_N);
		S3_STATE_SET_BG_ROP(screen_state_p,S3_MIX_FN_N);
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

	ASSERT(font_p->draw_glyphs);
	
	(*font_p->draw_glyphs) (font_p, displayed_glyph_start_p, 
		 displayed_glyph_end_p, x_origin, 
		 y, string_width, draw_flags);

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return (SI_SUCCEED);
}

function void
s3_font__gs_change__()
{
	S3_CURRENT_GRAPHICS_STATE_DECLARE();

	S3_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE, graphics_state_p));
	
	/*
	 * Some S3 chips seem to have problems with screen to screen 
	 * stippling with GXinvert rop. If the user prefers correctness 
	 * to speed allow it. Note that there is a terrible hack being
	 * done here to overcome an SI bug, namely si doesnt check the
	 * return value of the fontstplblt function, and hence if we
	 * return failure the string is never drawn. Hence force si
	 * to draw the string by changing the si_flags_p variable.
	 */
	if((screen_state_p->options_p->override_ss_stippling ==
		S3_OPTIONS_OVERRIDE_SS_STIPPLING_YES) &&
		((graphics_state_p->generic_state.si_graphics_state.
			SGmode == GXinvert) || 
		(graphics_state_p->generic_state.si_graphics_state.
			SGmode == GXset) || 
		(graphics_state_p->generic_state.si_graphics_state.
			SGmode == GXclear)))
	{
#if (defined(__DEBUG__))
		if(s3_font_debug)
		{
			(void) fprintf(debug_stream_p,
				"(s3_font__gs_change__){\n"
				"\tRop set to GXinvert/set/clear, patching to fail function\n"
				"\tSIavail_font set to 0\n"
				"}\n");
		}
#endif
		screen_state_p->generic_state.screen_functions_p->
			si_font_stplblt = s3_no_operation_fail;
		screen_state_p->generic_state.screen_flags_p->SIavail_font = 0;
	}
	else
	{
#if (defined(__DEBUG__))
		if(s3_font_debug)
		{
			(void) fprintf(debug_stream_p,
				"(s3_font__gs_change__){\n"
				"\tRop not GXinvert/set/clear, back to s3_font_draw_glyphs\n"
				"\tSIavail_font = FONT_AVAIL|STIPPLE_AVAIL|OPQSTIPPLE_AVAIL\n"
				"}\n");
		}
#endif
		screen_state_p->generic_state.screen_functions_p->
			si_font_stplblt = s3_font_draw_glyphs;
		screen_state_p->generic_state.screen_flags_p->SIavail_font = 
			(FONT_AVAIL | STIPPLE_AVAIL | OPQSTIPPLE_AVAIL);
	}
	return;
}

function void
s3_font__initialize__(SIScreenRec *si_screen_p,
						struct s3_options_structure * options_p)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	int glyph_max_width, glyph_max_height;
	struct s3_font_state *font_state_p;
	
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		(struct generic_screen_state *) screen_state_p));

	/*
	 * Create space for the font state and fill in control values.
	 */
	font_state_p = 
		allocate_and_clear_memory(sizeof(struct s3_font_state));

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
				sizeof(struct s3_font *));
	}
	
	if (sscanf(options_p->glyph_cache_size, "%ix%i", &glyph_max_width,
			   &glyph_max_height) != 2)
	{
		(void) fprintf(stderr,
					   S3_CANNOT_PARSE_GLYPH_CACHE_SIZE_MESSAGE,
					   options_p->glyph_cache_size);
		glyph_max_width = DEFAULT_S3_MAX_GLYPH_WIDTH;
		glyph_max_height = DEFAULT_S3_MAX_GLYPH_HEIGHT;
	}

	font_state_p->max_supported_glyph_width = glyph_max_width;
	font_state_p->max_supported_glyph_height = glyph_max_height;
	

	STAMP_OBJECT(S3_FONT_STATE, font_state_p);

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

	functions_p->si_font_check = s3_font_is_font_downloadable;
	functions_p->si_font_download = s3_font_download_font;
	functions_p->si_font_free = s3_font_free_font;
	functions_p->si_font_stplblt = s3_font_draw_glyphs;
}
