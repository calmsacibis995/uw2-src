/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_font.c	1.4"

/***
 ***	NAME
 ***
 ***		m64_font.c : Font handling code for the M64 display
 ***	library.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "m64_font.h"
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

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/

/***
 ***	Variables.
 ***/

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
export boolean m64_font_debug = 0;
#endif

PRIVATE

/***
 ***	Private declarations.
 ***/

/***
 ***	Includes.
 ***/
#include "g_omm.h"
#include "g_state.h"
#include "m64_gbls.h"
#include "m64_opt.h"
#include "m64_regs.h"
#include "m64_gs.h"
#include "m64_asm.h"
#include "m64_state.h"

/***
 ***	Constants.
 ***/
/*
 * Flags controlling the drawing.
 */
#define M64_FONT_DO_POLYTEXT 			(0x0001 << 0U)
#define M64_FONT_DO_IMAGETEXT			(0x0001 << 1U)

/*
 * Font kinds.
 */
#define DEFINE_FONT_TYPES()													\
	DEFINE_FONT_TYPE(NULL),													\
	DEFINE_FONT_TYPE(TERMINAL_FONT),										\
	DEFINE_FONT_TYPE(NON_TERMINAL_FONT),									\
	DEFINE_FONT_TYPE(OFFSCREEN_PACKED_TERMINAL_FONT),						\
	DEFINE_FONT_TYPE(COUNT)

#if (defined(__DEBUG__))
STATIC const char *const m64_font_kind_to_dump[] = 
{
#define DEFINE_FONT_TYPE(NAME)\
	#NAME
	DEFINE_FONT_TYPES()
#undef DEFINE_FONT_TYPE
};
#endif

#if (defined(__DEBUG__))
#define M64_FONT_DEBUG_STAMP 		0xdeadbeef
#endif

#define M64_GLYPH_STAMP\
	(('M' << 0) + ('6' << 1) + ('4' << 2) + ('_' << 3) + ('G' << 4) +\
	('L' << 5) + ('Y' << 6) + ('P' << 7) + ('H' << 8) + ('_' << 9) + \
	('S' << 10) + ('T' << 11) + ('A' << 12) + ('M' << 13) + ('P' << 14))

#define M64_FONT_STAMP\
	(('M' << 0) + ('6' << 1) + ('4' << 2) + ('_' << 3) + ('F' << 4) + \
	('O' << 5) + ('N' << 6) + ('T' << 7) + ('_' << 8) + ('S' << 9) + \
	('T' << 10) + ('A' << 11) + ('M' << 12) + ('P' << 13))

#define M64_FONT_STATE_STAMP\
	(('M' << 0) + ('6' << 1) + ('4' << 2) + ('_' << 3) + ('F' << 4) +\
	('O' << 5) + ('N' << 6) + ('T' << 7) + ('_' << 8) + ('S' << 9) + \
	('T' << 10) + ('A' << 11) + ('T' << 12) + ('E' << 13) + ('_' << 14) + \
	('S' << 15) + ('T' << 16) + ('A' << 17) + ('M' << 18) + ('P' << 19))

/***
 ***	Macros.
 ***/
#define M64_CURRENT_FONT_STATE_DECLARE()\
	struct m64_font_state *font_state_p = screen_state_p->font_state_p

/***
 ***	Types.
 ***/

enum m64_font_kind
{
#define DEFINE_FONT_TYPE(NAME)\
	M64_FONT_KIND_ ## NAME
		DEFINE_FONT_TYPES()
#undef DEFINE_FONT_TYPE
};

/*
 * Cached glyph information.
 */

struct m64_glyph
{
	/*
	 * Information about the glyph which will come in handy during the 
	 * drawing time. 
	 * height_width		: composite of height and width for this glyph.
	 * source_y_x		: composite of offscreen y/x location for this glyph.
	 * transfer_length 	: number of long transfers for this glyph.	
	 * source_offset	: qword offset in omm for this packed terminal font.
	 */
	unsigned int	height_width;
	unsigned int	source_y_x;		/*NOTUSED*/
	unsigned int	source_offset;
	int				transfer_length; 
	boolean			is_null_glyph;

	/*
	 * Saved SI glyph information
	 */
	SIGlyphP si_glyph_p;

	/*
	 * 	Pointer to the beginning long word of bits for this glyph.
	 */
	unsigned long *glyph_bits_p;

	/*
	 * Offscreen memory allocation.
	 */
	struct omm_allocation *offscreen_allocation_p;

#if (defined(__DEBUG__))
	int stamp;
#endif
};

/*
 * Structure of a downloaded font.
 */
struct m64_font
{
	/*
	 * The type of this font.
	 */
	enum m64_font_kind font_kind;
	
	/*
	 * SI's font information
	 */
	SIFontInfo si_font_info;
	
	/*
	 * Pointer to the array of m64_glyph structures.
	 */
	struct m64_glyph *font_glyphs_p;
	
	/*
	 * Pointer to the glyph bits for this font.
	 */
	unsigned long *font_glyph_bits_p;

	/*
	 * Offscreen area where the packed terminal fonts are downloaded.
	 */
	struct omm_allocation	*offscreen_packed_terminal_fonts_allocation_p;
	unsigned long			offscreen_packed_terminal_fonts_height_width;

	/*
	 * The font draw function for this font.
	 */
	void (*draw_glyphs) (
		struct m64_font *font_p,
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
struct m64_font_state
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
	struct m64_font **font_list_pp;
	
#if (defined(__DEBUG__))
	int stamp;
#endif
};

/***
 ***	Functions.
 ***/

/*
 * Drawing non terminal glyphs.
 */
STATIC void
m64_font_draw_non_terminal_glyphs(
	struct m64_font *font_p,
	short *glyph_list_start_p,
	short *glyph_list_end_p,
	int x_origin,
	int y_origin,
	int string_width,
	int draw_flags)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;
	
	struct m64_glyph *const font_glyphs_p = font_p->font_glyphs_p;

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));

	ASSERT(font_p->font_kind == M64_FONT_KIND_NON_TERMINAL_FONT ||
		 font_p->font_kind == M64_FONT_KIND_TERMINAL_FONT);

	/*
	 * If doing image text fill a rectangle of coordinates 
	 * UL (x, y - font_p->si_font_info.SFlascent)
	 * LR (x + string_width, y +
	 *		font_p->si_font_info.SFldescent) with the background color.
	 */
	if (draw_flags & M64_FONT_DO_IMAGETEXT)
	{
	
		unsigned long 	dst_y_x;
		unsigned long	dst_height_wid;
		unsigned long	gui_traj_cntl = 
			*(register_values_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) ;

		if (string_width < 0)
		{
			gui_traj_cntl &= ~GUI_TRAJ_CNTL_DST_X_DIR_LEFT_TO_RIGHT;
		}

		dst_y_x = (y_origin - font_p->si_font_info.SFlascent) | 
			((unsigned)(string_width > 0 ? x_origin : x_origin - 1) << 
			DST_Y_X_DST_X_SHIFT);

		dst_height_wid = (font_p->si_font_info.SFldescent + 
			font_p->si_font_info.SFlascent) | 
			((unsigned)(string_width > 0 ? string_width : -string_width) <<
				DST_HEIGHT_WID_DST_WID_SHIFT);
		ASSERT(((dst_height_wid >> DST_HEIGHT_WID_DST_WID_SHIFT) > 0) &&
			((dst_height_wid & 0x7FFF) > 0));

		M64_WAIT_FOR_FIFO(4);
		*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
			DP_SRC_DP_MONO_SRC_ALWAYS_1 ;
		*(register_base_address_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) = 
			gui_traj_cntl;
		*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = 
		dst_y_x;
		*(register_base_address_p + M64_REGISTER_DST_HEIGHT_WID_OFFSET) = 
			dst_height_wid;
	}

	/*
	 * Program the DP_SRC/DP_MIX register to do transparent stippling.
	 * In case the offscreen non terminal glyphs are handled then 
	 * the DPSRC register programming has to go inside the do loop.
	 */
	M64_WAIT_FOR_FIFO(4);
	*(register_base_address_p + M64_REGISTER_DP_MIX_OFFSET) = 
		(*(register_values_p + M64_REGISTER_DP_MIX_OFFSET) & 
		~DP_MIX_DP_BKGD_MIX) | DP_MIX_GXnoop ;
	*(register_base_address_p + M64_REGISTER_DP_PIX_WID_OFFSET) =
		register_values_p[M64_REGISTER_DP_PIX_WID_OFFSET] & 
		~DP_PIX_WID_DP_HOST_PIX_WID;
	*(register_base_address_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) = 	
		*(register_values_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) |
		GUI_TRAJ_CNTL_HOST_BYTE_ALIGN;
	*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
		(DP_SRC_BKGD_COLOR) |
		(DP_SRC_FRGD_COLOR <<  DP_SRC_DP_FRGD_SRC_SHIFT) |
		(DP_SRC_DP_MONO_SRC_HOST_DATA << DP_SRC_DP_MONO_SRC_SHIFT);
	/*
	 * Draw each glyph.
	 */
	do
	{
		const int glyph_index = *glyph_list_start_p;
		
		register struct m64_glyph *const glyph_p = font_glyphs_p + glyph_index;
		register unsigned int x_left = 
			x_origin + glyph_p->si_glyph_p->SFlbearing; 
		register unsigned int y_top = 
			y_origin - glyph_p->si_glyph_p->SFascent;	

#if (defined(__DEBUG__))
		int glyph_width  = glyph_p->si_glyph_p->SFrbearing - 
			glyph_p->si_glyph_p->SFlbearing; 
		int glyph_height = glyph_p->si_glyph_p->SFdescent + 
			glyph_p->si_glyph_p->SFascent;	
#endif


		ASSERT(IS_OBJECT_STAMPED(M64_GLYPH, glyph_p));

#if (defined(__DEBUG__))
		if (m64_font_debug)
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
#endif /*}*/

		/*
		 * There could be some glyphs with null width and height.
		 * ignore them. Stipple the other glyphs through the host
		 * data registers.
		 */
		if (glyph_p->is_null_glyph == FALSE)
		{
		/* 
		 * at least one pixel should be visible : we can't make a
		 * similar check for the y coordinates as there could be a
		 * individual characters which don't get to be seen.
		 */		
#if (defined(__DEBUG__))
			ASSERT((x_left + glyph_width >
				screen_state_p->generic_state.screen_clip_left) &&
				(x_left <=
				screen_state_p->generic_state.screen_clip_right));
#endif
			ASSERT(glyph_p->offscreen_allocation_p == NULL);
			ASSERT(glyph_p->transfer_length > 0);
			ASSERT(((glyph_p->height_width >> DST_HEIGHT_WID_DST_WID_SHIFT)
				> 0) && ((glyph_p->height_width & 0x7FFF) > 0));
			ASSERT((glyph_p->height_width & 0x7FFF) == glyph_height);
			ASSERT(glyph_p->glyph_bits_p);

			M64_WAIT_FOR_FIFO(2);
			*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) =
				y_top | (x_left << DST_Y_X_DST_X_SHIFT); 
			*(register_base_address_p + 
				M64_REGISTER_DST_HEIGHT_WID_OFFSET) = glyph_p->height_width;

			M64_ASM_TRANSFER_THRO_HOST_DATA(glyph_p->transfer_length, 
				glyph_p->glyph_bits_p);

		}

#if (defined(__DEBUG__)) /*{*/
		if (m64_font_debug)
		{
			(void) fprintf(debug_stream_p, "\t}\n");
		}
#endif
		
		/*
		 * Adjust the origin for the next glyph
		 */
		x_origin += glyph_p->si_glyph_p->SFwidth;

	} while (++glyph_list_start_p < glyph_list_end_p);

	/*
	 * Restore the background mix and the DP_PIXWID register value.
	 */
	M64_WAIT_FOR_FIFO(2);
	*(register_base_address_p + M64_REGISTER_DP_MIX_OFFSET) = 
		*(register_values_p + M64_REGISTER_DP_MIX_OFFSET);
	*(register_base_address_p + M64_REGISTER_DP_PIX_WID_OFFSET) =
		*(register_values_p + M64_REGISTER_DP_PIX_WID_OFFSET);


#if (defined(__DEBUG__)) /*{*/
	if (m64_font_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif
	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
	return;
}

/*
 * Drawing glyphs that are stored in a packed fashion in the offscreen memory.
 */
STATIC void
m64_font_draw_offscreen_packed_terminal_glyphs(
	struct m64_font *font_p,
	short *glyph_list_start_p,
	short *glyph_list_end_p,
	int x_origin,
	int y_origin,
	int string_width,
	int draw_flags)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;
	
	struct m64_glyph *const font_glyphs_p = font_p->font_glyphs_p;
	unsigned long height_width = font_p->
		offscreen_packed_terminal_fonts_height_width;

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE,screen_state_p));
	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
	ASSERT(font_p->font_kind == M64_FONT_KIND_OFFSCREEN_PACKED_TERMINAL_FONT);
	
#if (defined(__DEBUG__))
	if (m64_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_font_draw_offscreen_packed_terminal_glyphs) {\n"
			"\tfont_p = %p\n"
			"\tglyph_list_start_p = %p\n"
			"\tglyph_list_end_p = %p\n"
			"\tx_origin = %d\n"
			"\ty_origin = %d\n"
			"\tstring_width = %d\n"
			"\tdraw_flags = 0x%x\n"
			"}\n",
			(void *) font_p, (void *) glyph_list_start_p,
			(void *) glyph_list_end_p, x_origin, y_origin,
			string_width, draw_flags);
	}
#endif

	/*
	 * In case of terminal fonts if one glyph is a null glyph every 
	 * other glyph is also a null glyph.
	 */
	if (font_glyphs_p[0].is_null_glyph == TRUE)
	{
		return;
	}

	/*
	 * Dont do opaque stippling here. If bg rop is Noop the engine
	 * seems to use blocked writes to video memory.
	 * If doing image text fill a rectangle of coordinates 
	 * UL (x, y - font_p->si_font_info.SFlascent)
	 * LR (x + string_width, y +
	 *		font_p->si_font_info.SFldescent) with the background color.
	 */
	if (draw_flags & M64_FONT_DO_IMAGETEXT)
	{
	
		unsigned long 	dst_y_x;
		unsigned long	dst_height_wid;
		unsigned long	gui_traj_cntl = 
			*(register_values_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) ;

		if (string_width < 0)
		{
			gui_traj_cntl &= ~GUI_TRAJ_CNTL_DST_X_DIR_LEFT_TO_RIGHT;
		}

		dst_y_x = (y_origin - font_p->si_font_info.SFlascent) | 
			((unsigned)(string_width > 0 ? x_origin : x_origin - 1) << 
			DST_Y_X_DST_X_SHIFT);

		dst_height_wid = (font_p->si_font_info.SFldescent + 
			font_p->si_font_info.SFlascent) | 
			((unsigned)(string_width > 0 ? string_width : -string_width) <<
				DST_HEIGHT_WID_DST_WID_SHIFT);
		ASSERT(((dst_height_wid >> DST_HEIGHT_WID_DST_WID_SHIFT) > 0) &&
			((dst_height_wid & 0x7FFF) > 0));

		M64_WAIT_FOR_FIFO(4);
		*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
			DP_SRC_DP_MONO_SRC_ALWAYS_1 ;
		*(register_base_address_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) = 
			gui_traj_cntl;
		*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = dst_y_x;
		*(register_base_address_p + M64_REGISTER_DST_HEIGHT_WID_OFFSET) = 
			dst_height_wid;
	}

	/*
	 * Program the DP_SRC/DP_MIX register to do screen to screen 
	 * linear stippling.
	 */
	M64_WAIT_FOR_FIFO(5);
	*(register_base_address_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) = 	
		*(register_values_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) |
		GUI_TRAJ_CNTL_DST_X_TILE | GUI_TRAJ_CNTL_SRC_LINEAR_EN | 
		GUI_TRAJ_CNTL_SRC_BYTE_ALIGN ;

	*(register_base_address_p + M64_REGISTER_DP_MIX_OFFSET) = 
		(*(register_values_p + M64_REGISTER_DP_MIX_OFFSET) & 
		~DP_MIX_DP_BKGD_MIX) | DP_MIX_GXnoop ;

	*(register_base_address_p + M64_REGISTER_DP_PIX_WID_OFFSET) =
		(register_values_p[M64_REGISTER_DP_PIX_WID_OFFSET] & 
		~(DP_PIX_WID_DP_HOST_PIX_WID | DP_PIX_WID_DP_SRC_PIX_WID));

	*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
		(DP_SRC_BKGD_COLOR) | (DP_SRC_FRGD_COLOR <<  DP_SRC_DP_FRGD_SRC_SHIFT) |
		(DP_SRC_DP_MONO_SRC_BLIT_SRC << DP_SRC_DP_MONO_SRC_SHIFT);
	
	*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = 
		(unsigned)(y_origin - font_p->si_font_info.SFmax.SFascent) | 
		((unsigned)x_origin << DST_Y_X_DST_X_SHIFT);

	ASSERT(font_p->offscreen_packed_terminal_fonts_allocation_p != NULL);

	do
	{
#if (defined(__DEBUG__))
		register const struct m64_glyph *const glyph_p =
			&(font_glyphs_p[*glyph_list_start_p]);

		ASSERT(IS_OBJECT_STAMPED(M64_GLYPH, glyph_p));
		ASSERT(glyph_p->offscreen_allocation_p == NULL);
#endif

		/*
		 * Program the sourcepitch, destination_x, and width_height 
		 * register.
		 */
		M64_WAIT_FOR_FIFO(2);

		*(register_base_address_p + M64_REGISTER_SRC_OFF_PITCH_OFFSET) = 
			*(register_values_p + M64_REGISTER_SRC_OFF_PITCH_OFFSET) | 
			((struct m64_glyph *)&(font_glyphs_p[*glyph_list_start_p]))->
			source_offset;

		*(register_base_address_p + M64_REGISTER_DST_HEIGHT_WID_OFFSET) = 
			height_width;

	} while (++glyph_list_start_p < glyph_list_end_p);

	/*
	 * Restore the registers that have changed.
	 */
	M64_WAIT_FOR_FIFO(3);
	*(register_base_address_p + M64_REGISTER_DP_MIX_OFFSET) = 
		*(register_values_p + M64_REGISTER_DP_MIX_OFFSET);
	*(register_base_address_p + M64_REGISTER_DP_PIX_WID_OFFSET) =
		*(register_values_p + M64_REGISTER_DP_PIX_WID_OFFSET);
	*(register_base_address_p + M64_REGISTER_SRC_OFF_PITCH_OFFSET) =
		*(register_values_p + M64_REGISTER_SRC_OFF_PITCH_OFFSET);
	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
	return;
}

STATIC SIBool
m64_font_draw_glyphs( const SIint32 font_index, 
	const SIint32 x, const SIint32 y,
	SIint32 glyph_count, SIint16 *glyph_list_p, SIint32 draw_method)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	M64_CURRENT_FONT_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;
	
	/*
	 * The font descriptor.
	 */
	struct m64_font 	*font_p;
	struct m64_glyph 	*font_glyphs_p;

	/*
	 * parameters for the glyphs to be rendered.
	 */
	const SIint16 		*last_glyph_p = glyph_list_p + glyph_count;
	SIint16 			*displayed_glyph_start_p;
	SIint16 			*displayed_glyph_end_p;
	int 				end_x_origin;
	int 				string_width;
	int 				draw_flags;

	/* 
	 * starting point of string to be rendered.
	 */
	int 				x_origin = x;

	/*
	 * current clip bounds.
	 */
	const int clip_left = screen_state_p->generic_state.screen_clip_left;
	const int clip_right = screen_state_p->generic_state.screen_clip_right;
	const int clip_top = screen_state_p->generic_state.screen_clip_top;
	const int clip_bottom = screen_state_p->generic_state.screen_clip_bottom;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_FONT_STATE, font_state_p));

#if (defined(__DEBUG__))
	if (m64_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_font_draw_glyphs)\n"
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
#endif /*}*/

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));

	if (glyph_count <= 0)
	{
		return (SI_SUCCEED);
	}

	if (font_index < 0 || font_index >=
		font_state_p->max_number_of_downloadable_fonts) 
	{
#if (defined(__DEBUG__))
		if (m64_font_debug)
		{
		(void) fprintf(debug_stream_p,
			"(m64_font_draw_glyphs)\n"
			"{\n"
			"\t# Failing because index is out of bounds.\n"
			"\tfont_index = %ld\n"
			"}\n", 
			font_index);
		}
#endif
		return (SI_FAIL);
	}

	if ((x < 0) || (y < 0))
	{
		return (SI_FAIL);
	}

	/*
	 * Determine the type of draw : opaque or transparent stippling.
	 */
	if (draw_method == 0)
	{
		draw_method =
			graphics_state_p->generic_state.si_graphics_state.SGstplmode;
	}

	/*
	 * Set the draw flags.
	 */
	draw_flags = (draw_method == SGStipple) ? M64_FONT_DO_POLYTEXT : 
		M64_FONT_DO_IMAGETEXT;
	
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

	ASSERT(IS_OBJECT_STAMPED(M64_FONT, font_p));

	font_glyphs_p = font_p->font_glyphs_p;
	
	ASSERT(font_glyphs_p != NULL);
	
	/*
	 * Check for the y-coordinates visibility.
	 */
	if ((y - font_p->si_font_info.SFmax.SFascent > clip_bottom) ||
		(y + font_p->si_font_info.SFmax.SFdescent <= clip_top))
	{
#if (defined(__DEBUG__)) /*{*/
		if (m64_font_debug)
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

		if (x_start > clip_right || x_end <= clip_left )
		{
#if (defined(__DEBUG__)) /*{*/
			if (m64_font_debug)
			{
				(void) fprintf(debug_stream_p,
				"\t#X coordinate of string is out of clip rectangle.\n"
				"}\n");
			}
#endif
			return (SI_SUCCEED);
		}
		
		if (x_start < clip_left)
		{
			left_extra_glyph_count = (clip_left - x_start) / width;
		}

		if ((x_end - 1)  > clip_right)
		{
			right_extra_glyph_count = (x_end - clip_right - 1) / width;
		}
		
		if (width > 0)
		{
			displayed_glyph_start_p = glyph_list_p + left_extra_glyph_count;
			displayed_glyph_end_p = glyph_list_p + glyph_count -
				right_extra_glyph_count;
		}
		else					/* width < 0 */
		{	
			displayed_glyph_start_p = glyph_list_p + right_extra_glyph_count;
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
			register const struct m64_glyph *glyph_p = 
				&(font_glyphs_p[*displayed_glyph_start_p]);
		
			if ((x_origin + glyph_p->si_glyph_p->SFrbearing - 1) >= clip_left)
			{
				if ((x_origin + glyph_p->si_glyph_p->SFlbearing) <= clip_right)
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
#if (defined(__DEBUG__)) /*{*/
			if (m64_font_debug)
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
			register const struct m64_glyph *const glyph_p = 
				&(font_glyphs_p[*displayed_glyph_end_p]);
			
			if (((end_x_origin + glyph_p->si_glyph_p->SFrbearing - 1) <
				 	clip_left) ||
				((end_x_origin + glyph_p->si_glyph_p->SFlbearing) > 
					clip_right))
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
#if (defined(__DEBUG__)) /*{*/
			if (m64_font_debug)
			{
				(void) fprintf(debug_stream_p,
				"\t# No glyphs are displayable.\n"
				"}\n");
			}
#endif
			return (SI_SUCCEED);
		}
	}

#if (defined(__DEBUG__)) /*{*/
	if (m64_font_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	/*
	 * Set the correct clipping rectangle and then call the 
	 * appropriate drawing function for this font.
	 */
	ASSERT(font_p->draw_glyphs);
	
	if(screen_state_p->generic_state.screen_current_clip != 
		GENERIC_CLIP_TO_GRAPHICS_STATE)
	{
		M64_WAIT_FOR_FIFO(2);
		register_base_address_p[M64_REGISTER_SC_LEFT_RIGHT_OFFSET] = 
			register_values_p[M64_REGISTER_SC_LEFT_RIGHT_OFFSET];
		 register_base_address_p[M64_REGISTER_SC_TOP_BOTTOM_OFFSET] = 
		 	register_values_p[M64_REGISTER_SC_TOP_BOTTOM_OFFSET];
		screen_state_p->generic_state.screen_current_clip = 
			GENERIC_CLIP_TO_GRAPHICS_STATE;
	}
	if (string_width != 0)
	{
		(*font_p->draw_glyphs) (font_p, displayed_glyph_start_p, 
			 displayed_glyph_end_p, x_origin, y, string_width, draw_flags);
	}

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
	return (SI_SUCCEED);
}

STATIC SIBool
m64_font_is_font_downloadable( SIint32 font_index, SIFontInfoP fontinfo_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_FONT_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_FONT_STATE, font_state_p));
	
#if (defined(__DEBUG__))
	if (m64_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_font_is_font_downloadable)\n"
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
		if (m64_font_debug)
		{
			(void) fprintf(debug_stream_p,
				"(m64_font_is_font_downloadable)\n"
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
	 	   M64_OPTIONS_FONTDRAW_OPTIONS_DRAW_TERMINAL_FONTS) &&
		 (fontinfo_p->SFflag & SFTerminalFont)) ||
	    (!(screen_state_p->options_p->fontdraw_options & 
	 	   M64_OPTIONS_FONTDRAW_OPTIONS_DRAW_NON_TERMINAL_FONTS) &&
		 (!(fontinfo_p->SFflag & SFTerminalFont))) ||
		((screen_state_p->options_p->
		  max_number_of_glyphs_in_downloadable_font > 0) && 
		 (fontinfo_p->SFnumglyph > screen_state_p->options_p->
		  max_number_of_glyphs_in_downloadable_font)))
	{
#if (defined(__DEBUG__))
		if (m64_font_debug)
		{
			(void) fprintf(debug_stream_p,
				"(m64_font_is_font_downloadable)\n"
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
 * m64_font_download_font
 *
 * Download a font from the SI layer into the SDD's system memory.
 */
STATIC SIBool
m64_font_download_font( SIint32 font_index, SIFontInfoP font_info_p, 
	SIGlyphP glyph_list_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_FONT_STATE_DECLARE();
	M64_CURRENT_FRAMEBUFFER_BASE_DECLARE();

	struct m64_font		*font_p;
	const int			screen_depth = 
						screen_state_p->generic_state.screen_depth;
	const int			max_glyph_height_in_pixels = 
							font_info_p->SFmax.SFascent + 
							font_info_p->SFmax.SFdescent;
	const int			max_glyph_width_in_pixels = 
							font_info_p->SFmax.SFrbearing - 
							font_info_p->SFmin.SFlbearing;
	unsigned int		size_of_max_glyph_in_longs;
	const SIGlyphP		last_si_glyph_p = 
							glyph_list_p + font_info_p->SFnumglyph;
	SIGlyphP			current_si_glyph_p;
	struct m64_glyph	*current_m64_glyph_p;
	unsigned long		*glyph_bits_p;
	enum m64_font_kind	font_kind = M64_FONT_KIND_NULL;
	unsigned int		offscreen_packed_terminal_font_bytes_per_glyph_line;
	unsigned int		offscreen_packed_terminal_font_bytes_per_glyph;


#if (defined(__DEBUG__))
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	const unsigned int	m64_debug_stamp = M64_FONT_DEBUG_STAMP;
#endif
	

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_FONT_STATE, font_state_p));
	ASSERT(screen_depth != 24);

#if (defined(__DEBUG__))
	if (m64_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_font_download_font)\n"
			"{\n"
			"\tfont_index = %ld\n"
			"\tfont_info_p = %p\n"
			"\tglyph_list_p = %p\n",
			font_index, (void *) font_info_p, (void *) glyph_list_p);
	}
#endif	/*}*/

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));

	if ((font_index < 0) || 
		(font_index >= font_state_p->max_number_of_downloadable_fonts))
	{
#if (defined(__DEBUG__)) /*{*/
		if (m64_font_debug)
		{
		(void) fprintf(debug_stream_p,
			"(m64_font_download_font)\n"
			"\t{\n"
			"\t\t# Failing because index is out of bounds.\n"
			"\t}\n"
			"}\n");
		}
#endif
		return (SI_FAIL);
	}

	if (font_info_p->SFnumglyph <= 0)
	{
#if (defined(__DEBUG__)) /*{*/
		if (m64_font_debug)
		{
		(void) fprintf(debug_stream_p,
			"(m64_font_download_font)\n"
			"\t{\n"
			"\t\t# Failing because of negative or zero number of glyphs\n"
			"\t\tnumber of glyphs = %ld\n"
			"\t}\n"
			"}\n",
			font_info_p->SFnumglyph);
		}
#endif
		return (SI_FAIL);
	}
	
	ASSERT ((font_state_p->font_list_pp[font_index]) == NULL);

	/*
	 * Allocate zeroed memory for one m64_font structure and
	 * tuck away the pointer in the m64 font state structure.
	 */
	font_p = allocate_and_clear_memory(sizeof(struct m64_font));

	font_state_p->font_list_pp[font_index] = font_p;

	/*
	 * Copy the fontinfo structure.
	 */
	font_p->si_font_info = *font_info_p;

	ASSERT(font_info_p->SFnumglyph > 0);

	/*
	 * Determine the font kind. Try to see if a terminal font can be
	 * downloaded linearly in a contiguous offscreen memory area
	 * in which case try check if offscreen space is available and classify
	 * the font as offscreen_packed_terminal_font.
	 */
	font_kind = (font_info_p->SFflag & SFTerminalFont) ? 
		M64_FONT_KIND_TERMINAL_FONT : M64_FONT_KIND_NON_TERMINAL_FONT;
	if((screen_state_p->options_p->fontdraw_options &
			M64_OPTIONS_FONTDRAW_OPTIONS_USE_OFFSCREEN_MEMORY) &&
		(screen_state_p->options_p->fontdraw_options & 
			M64_OPTIONS_FONTDRAW_OPTIONS_DRAW_PACKED_TERMINAL_FONTS) &&
		(font_kind == M64_FONT_KIND_TERMINAL_FONT))
	{
		unsigned	offscreen_packed_terminal_font_bytes_per_font;
		unsigned	offscreen_packed_terminal_font_offscreen_width;
		unsigned	offscreen_packed_terminal_font_offscreen_height;
		struct omm_allocation *allocation_p = NULL;

		/*
		 * parameters of the offscreen area for downloading this font.
		 */

		/*
		 * number of bytes for one line of the glyph.
		 */
		offscreen_packed_terminal_font_bytes_per_glyph_line = 
			((unsigned)(font_info_p->SFmax.SFwidth + 7) & ~7) >> 3U;
			
		/*
		 * This is the number of offscreen bytes per font glyph. It should 
		 * be integeral number of 64 pixels for monochrome modes so that
		 * source offset can be specified correctly in terms of 64 bit words.
		 */
		offscreen_packed_terminal_font_bytes_per_glyph = 
			((offscreen_packed_terminal_font_bytes_per_glyph_line *
			max_glyph_height_in_pixels) + 7) & ~7;

		offscreen_packed_terminal_font_bytes_per_font = 
			font_info_p->SFnumglyph *
			offscreen_packed_terminal_font_bytes_per_glyph;

		offscreen_packed_terminal_font_offscreen_width = 
			screen_state_p->generic_state.screen_physical_width;

		offscreen_packed_terminal_font_offscreen_height = 
			(((offscreen_packed_terminal_font_bytes_per_font >> 2U) <<
			screen_state_p->pixels_per_long_shift) + 
			screen_state_p->generic_state.screen_physical_width - 1) /
			screen_state_p->generic_state.screen_physical_width;
		
#if (defined(__DEBUG__))
		if (m64_font_debug)
		{
			(void) fprintf(debug_stream_p, "(m64_font_download_font) {\n"
			"offscreen_packed_terminal_font_bytes_per_glyph_line = %d\n"
			"offscreen_packed_terminal_font_bytes_per_glyph = %d\n"
			"offscreen_packed_terminal_font_bytes_per_font = %d\n"
			"offscreen_packed_terminal_font_offscreen_width = %d\n"
			"offscreen_packed_terminal_font_offscreen_height = %d\n"
			"}\n",
			offscreen_packed_terminal_font_bytes_per_glyph_line ,
			offscreen_packed_terminal_font_bytes_per_glyph, 
			offscreen_packed_terminal_font_bytes_per_font,
			offscreen_packed_terminal_font_offscreen_width,
			offscreen_packed_terminal_font_offscreen_height);
		}
#endif 

		allocation_p = omm_allocate(
				offscreen_packed_terminal_font_offscreen_width,
				offscreen_packed_terminal_font_offscreen_height,
				screen_depth, OMM_LONG_TERM_ALLOCATION);
			
		if (allocation_p != NULL)
		{
			font_p->offscreen_packed_terminal_fonts_allocation_p = 
				allocation_p;
			font_kind = M64_FONT_KIND_OFFSCREEN_PACKED_TERMINAL_FONT;
		}
	}

	/*
	 * Allocate space for the m64_glyph structures.
	 */
	font_p->font_glyphs_p = allocate_and_clear_memory(
		font_info_p->SFnumglyph * sizeof(struct m64_glyph));

	current_si_glyph_p = glyph_list_p;
	current_m64_glyph_p = font_p->font_glyphs_p;

	if (font_kind == M64_FONT_KIND_OFFSCREEN_PACKED_TERMINAL_FONT)
	{
		struct omm_allocation * const allocation_p = 
			font_p->offscreen_packed_terminal_fonts_allocation_p;

		unsigned int const quadwords_per_glyph = 
			offscreen_packed_terminal_font_bytes_per_glyph >> 3U;

		unsigned int const glyph_bitmap_width_in_bytes = 
			((unsigned)(max_glyph_width_in_pixels + 31) & ~31) >> 3U;

		unsigned int  quadword_offset;
		unsigned long *dst_p;


		/* 
		 * If width is a full offscreen line then allocations X should be 0.
		 */
		ASSERT(allocation_p != NULL && allocation_p->x == 0); 


		quadword_offset = (((unsigned)allocation_p->y *
			screen_state_p->generic_state.screen_physical_width) >> 
			screen_state_p->pixels_per_long_shift) >> 1U;

		dst_p = (unsigned long*)framebuffer_p + (quadword_offset << 1U);

		/*
		 * download the glyphs into offscreen memory and initialize the
		 * glyph structures for all glyphs.
		 */
		font_p->offscreen_packed_terminal_fonts_height_width = 
			(unsigned)max_glyph_height_in_pixels | 
			((unsigned)max_glyph_width_in_pixels << 
				DST_HEIGHT_WID_DST_WID_SHIFT);
			
		while (current_si_glyph_p < last_si_glyph_p)
		{
			/*
			 * Copy the SIGlyph structure after allocating space for it.
			 */
			current_m64_glyph_p->si_glyph_p = allocate_memory(sizeof(SIGlyph));
			*current_m64_glyph_p->si_glyph_p = *current_si_glyph_p;


		 	ASSERT((current_si_glyph_p->SFrbearing - 
				current_si_glyph_p->SFlbearing) >= 0 && 
				(current_si_glyph_p->SFascent + 
				current_si_glyph_p->SFdescent) >= 0) ;

		 	if (((current_si_glyph_p->SFrbearing - 
				current_si_glyph_p->SFlbearing) <= 0) ||
				((current_si_glyph_p->SFascent + 
				current_si_glyph_p->SFdescent) <= 0))
			{
				current_m64_glyph_p->is_null_glyph = TRUE;
			}
			else
			{
				/*
				 * Valid glyph if width/height are both > 0.
				 */
				current_m64_glyph_p->is_null_glyph = FALSE;
			}
			current_m64_glyph_p->source_offset = quadword_offset;

			/*
			 * Download the glyph.
			 */
			screen_state_p->transfer_pixels_p(
				(unsigned long *)current_si_glyph_p->SFglyph.Bptr, dst_p,
				0 , 0,
				glyph_bitmap_width_in_bytes,
				offscreen_packed_terminal_font_bytes_per_glyph_line,
				offscreen_packed_terminal_font_bytes_per_glyph_line, 
				max_glyph_height_in_pixels, 
				8,
				GXcopy, 0xFF, 3U);
				
			STAMP_OBJECT(M64_GLYPH, current_m64_glyph_p);

			/*
			 * Move over to the next glyph.
			 */
			dst_p += (quadwords_per_glyph << 1U);
			quadword_offset += quadwords_per_glyph ;
			++current_m64_glyph_p;
			++current_si_glyph_p;
		}
	}
	else
	{
		/*
		 * Regular terminal and non terminal fonts.
		 */
		/*
		 * Compute the space required to hold the largest character in the
		 * font. Determine width of the widest glyph rounded of to the nearest
		 * long word and multiply it with the height of the tallest glyph 
		 * in this font.
		 */
		size_of_max_glyph_in_longs =  max_glyph_height_in_pixels *
			(((unsigned)(max_glyph_width_in_pixels + 31) & ~31) >> 5U);

#if (defined(__DEBUG__))
		/*
		 * Allocate space for the glyph bits.
		 */
		font_p->font_glyph_bits_p = allocate_and_clear_memory(
			((size_of_max_glyph_in_longs << 2U) * font_info_p->SFnumglyph) + 
				sizeof(m64_debug_stamp));

		*(font_p->font_glyph_bits_p + 
			(size_of_max_glyph_in_longs * font_info_p->SFnumglyph)) = 
			m64_debug_stamp;
#else
		/*
		 * Allocate space for the glyph bits.
		 */
		font_p->font_glyph_bits_p = allocate_and_clear_memory(
			((size_of_max_glyph_in_longs << 2U) * font_info_p->SFnumglyph));
#endif

		glyph_bits_p = font_p->font_glyph_bits_p;
		/*
		 * Initialize the m64_glyph structures for all glyphs in the font.
		 */
		while (current_si_glyph_p < last_si_glyph_p)
		{
			int			glyph_width_in_pixels;
			int			glyph_height_in_pixels;
			int			numbytes_per_glyph_line;
			int			numbytes_per_glyph_bitmap_line;
			
			/*
			 * Copy the SIGlyph structure after allocating space for it.
			 */
			current_m64_glyph_p->si_glyph_p = allocate_memory(sizeof(SIGlyph));
			*current_m64_glyph_p->si_glyph_p = *current_si_glyph_p;

			glyph_width_in_pixels = 
				current_si_glyph_p->SFrbearing - current_si_glyph_p->SFlbearing; 
			glyph_height_in_pixels = 
				current_si_glyph_p->SFascent + current_si_glyph_p->SFdescent; 

			numbytes_per_glyph_line = 
				(unsigned)((glyph_width_in_pixels + 7) & ~7) >> 3U;

			numbytes_per_glyph_bitmap_line = 
				(unsigned)((glyph_width_in_pixels + 31) & ~31) >> 3U;

			current_m64_glyph_p->is_null_glyph = TRUE;

			if ((glyph_height_in_pixels > 0) && ( glyph_width_in_pixels > 0)) 
			{
				/* 
				 * for copying the glyph bits.
				 */
				unsigned char *dst_p, *src_p;
				int	i;

				/*
				 * Valid glyph if width/height are both > 0.
				 */
				current_m64_glyph_p->is_null_glyph = FALSE;

				/*
				 * Compute the total longword transfers for rendering this 
				 * glyph from system memory.
				 */
				current_m64_glyph_p->transfer_length = 
					(unsigned)(((numbytes_per_glyph_line * 
					glyph_height_in_pixels) + 3) & ~3 ) >> 2U;

				ASSERT(glyph_width_in_pixels == 
					current_si_glyph_p->SFglyph.Bwidth);
				/*
				 * height_width register to be programmed with this value.
				 */
				current_m64_glyph_p->height_width =  
					(unsigned)glyph_height_in_pixels |
					(((unsigned)glyph_width_in_pixels) << 
					DST_HEIGHT_WID_DST_WID_SHIFT);

				/*
				 * pointer to the bits.
				 */
				current_m64_glyph_p->glyph_bits_p = glyph_bits_p;  

				/*
				 * Copy the glyph bits. Pack them to so that unnecessary
				 * bytes are not copied.
				 */
			  	dst_p = (unsigned char *)glyph_bits_p;
				src_p = (unsigned char *)current_si_glyph_p->SFglyph.Bptr;
				while(glyph_height_in_pixels--)
				{
					unsigned char *tmp_p;

					i = numbytes_per_glyph_line;
					tmp_p = src_p;
					while(i--)
					{
						*dst_p++ = *tmp_p++;
					}
					src_p += numbytes_per_glyph_bitmap_line;
				}
			}
				
			STAMP_OBJECT(M64_GLYPH, current_m64_glyph_p);

			/*
			 * Move over to the next glyph.
			 */
			glyph_bits_p += size_of_max_glyph_in_longs;
			++current_m64_glyph_p;
			++current_si_glyph_p;
		}
		ASSERT(m64_debug_stamp == *(font_p->font_glyph_bits_p + 
			(size_of_max_glyph_in_longs * font_info_p->SFnumglyph)));
	}

	/*
	 * Set the pointers to the font draw functions.
	 */
	switch (font_kind)
	{
		case M64_FONT_KIND_TERMINAL_FONT:
			font_p->font_kind = M64_FONT_KIND_TERMINAL_FONT;
			font_p->draw_glyphs = m64_font_draw_non_terminal_glyphs;
		break;
		case M64_FONT_KIND_NON_TERMINAL_FONT:
			font_p->font_kind = M64_FONT_KIND_NON_TERMINAL_FONT;
			font_p->draw_glyphs = m64_font_draw_non_terminal_glyphs;
		break;
		case M64_FONT_KIND_OFFSCREEN_PACKED_TERMINAL_FONT:
			font_p->font_kind = M64_FONT_KIND_OFFSCREEN_PACKED_TERMINAL_FONT;
			font_p->draw_glyphs = 
				m64_font_draw_offscreen_packed_terminal_glyphs;
			break;
		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
#if (defined(__DEBUG__)) /*{*/
			if (m64_font_debug)
			{
				(void) fprintf(debug_stream_p,
					"\t\tfont_kind = %s\n"
					"}\n",
					m64_font_kind_to_dump[font_p->font_kind]);
			}
#endif
			return(SI_FAIL);
	}

#if (defined(__DEBUG__)) /*{*/
	if (m64_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"\t\tfont_kind = %s\n"
			"}\n",
			m64_font_kind_to_dump[font_p->font_kind]);
	}
#endif
	
	STAMP_OBJECT(M64_FONT, font_p);

	ASSERT(IS_OBJECT_STAMPED(M64_FONT_STATE, font_state_p));
	ASSERT(!(M64_IS_FIFO_OVERFLOW()));

	return(SI_SUCCEED);
}

STATIC SIBool
m64_font_free_font(SIint32 font_index)
{
	struct m64_font		*font_p;
	struct m64_glyph	*current_m64_glyph_p;
	int					numglyphs;

	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_FONT_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 (struct generic_screen_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_FONT_STATE, font_state_p));

#if (defined(__DEBUG__))
	if (m64_font_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_font_free_font)\n"
			"{\n"
			"\tfont_index = %ld\n"
			"}\n",
			font_index);
	}
#endif
	
	if ((font_index < 0) || 
		(font_index >= font_state_p->max_number_of_downloadable_fonts))
	{
#if (defined(__DEBUG__))
		if (m64_font_debug)
		{
		(void) fprintf(debug_stream_p,
			"(m64_font_free_font)\n"
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

	ASSERT(IS_OBJECT_STAMPED(M64_FONT,font_p));
	
	if (font_p->font_glyph_bits_p)
	{
		/*
		 * Free the space allocated for the font glyph bits.
		 */
		free_memory(font_p->font_glyph_bits_p);
	}

	/*
	 * Free the memory allocated for the m64_glyph_structures.
	 */
	numglyphs = font_p->si_font_info.SFnumglyph;
	current_m64_glyph_p = font_p->font_glyphs_p;

	ASSERT(font_p->font_kind == M64_FONT_KIND_TERMINAL_FONT ||
		font_p->font_kind == M64_FONT_KIND_NON_TERMINAL_FONT ||
		font_p->font_kind == M64_FONT_KIND_OFFSCREEN_PACKED_TERMINAL_FONT);
		
	/*
	 * Loop over each glyph and free memory allocated for them.
	 */
	if (numglyphs > 0)
	{
		do
		{
			ASSERT(IS_OBJECT_STAMPED(M64_GLYPH, current_m64_glyph_p));
			free_memory(current_m64_glyph_p->si_glyph_p);
			++current_m64_glyph_p;
		} while(--numglyphs > 0);
	}
	
	/*
	 * Free the offscreen memory allocated for the packed offscreen 
	 * terminal fonts.
	 */
	if ( font_p->font_kind == M64_FONT_KIND_OFFSCREEN_PACKED_TERMINAL_FONT)
	{
		omm_free(font_p->offscreen_packed_terminal_fonts_allocation_p);
	}

	ASSERT(font_p->font_glyphs_p);
	free_memory(font_p->font_glyphs_p);

	ASSERT(font_p);
	
	free_memory(font_p);

	font_state_p->font_list_pp[font_index] = NULL;

	ASSERT(IS_OBJECT_STAMPED(M64_FONT_STATE, font_state_p));

	return (SI_SUCCEED);
}

function void
m64_font__gs_change__()
{
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	M64_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		(struct generic_graphics_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));
	
	return;
}

function void
m64_font__initialize__(SIScreenRec *si_screen_p,
	struct m64_options_structure * options_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	int glyph_max_width, glyph_max_height;
	struct m64_font_state *font_state_p;
	
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		(struct generic_screen_state *) screen_state_p));

	/*
	 * Create space for the font state and fill in control values.
	 */
	font_state_p = allocate_and_clear_memory(sizeof(struct m64_font_state));

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
		font_state_p->font_list_pp = allocate_and_clear_memory(
			font_state_p->max_number_of_downloadable_fonts * 
			sizeof(struct m64_font *));
	}
	
	if (sscanf(options_p->glyph_cache_size, "%ix%i", &glyph_max_width,
			   &glyph_max_height) != 2)
	{
		(void) fprintf(stderr, M64_CANNOT_PARSE_GLYPH_CACHE_SIZE_MESSAGE,
			options_p->glyph_cache_size); 
		glyph_max_width = DEFAULT_M64_MAX_GLYPH_WIDTH;
		glyph_max_height = DEFAULT_M64_MAX_GLYPH_HEIGHT;
	}

	font_state_p->max_supported_glyph_width = glyph_max_width;
	font_state_p->max_supported_glyph_height = glyph_max_height;
	
	STAMP_OBJECT(M64_FONT_STATE, font_state_p);

	/*
	 * Tuck away pointer into the screen state.
	 */
	screen_state_p->font_state_p = font_state_p;

	/*
	 * Store parameters for SI.
	 */
	flags_p->SIfontcnt  = font_state_p->max_number_of_downloadable_fonts;
	flags_p->SIavail_font = (FONT_AVAIL | STIPPLE_AVAIL | OPQSTIPPLE_AVAIL);

	functions_p->si_font_check = m64_font_is_font_downloadable;
	functions_p->si_font_download = m64_font_download_font;
	functions_p->si_font_free = m64_font_free_font;
	functions_p->si_font_stplblt = m64_font_draw_glyphs;
}
