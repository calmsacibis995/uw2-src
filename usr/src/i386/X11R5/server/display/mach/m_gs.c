/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m_gs.c	1.7"

/***
 ***	NAME
 ***
 ***        mach_gs.c : handling graphics state changes for
 ***                    the Mach series chipsets.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "m_gs.h"
 ***
 ***	DESCRIPTION
 ***
 ***	Graphics state download and retrival are handled by calling
 ***	the generic libraries entry point and then handling the
 ***	chipset specifics.  Downloading of tiles and stipples need
 ***	special care: tiles need to be expanded in 4 bit mode, while
 ***	stipples need to be inverted.
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
 ***	CAVEATS
 ***
 ***	BUGS
 ***
 ***	HISTORY
 ***
 ***/

#ident	"@(#)mach:mach/m_gs.c	1.3"

PUBLIC

/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/
#include <sidep.h>
#include "g_state.h"
#include "g_gs.h"
#include "g_omm.h"
#include "m_state.h"
#include "m_opt.h"
#include "stdenv.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

#define MACH_CURRENT_GRAPHICS_STATE_DECLARE()\
	struct mach_graphics_state *graphics_state_p =\
	(struct mach_graphics_state *) generic_current_screen_state_p->\
	screen_current_graphics_state_p


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
export boolean mach_graphics_state_debug = FALSE;
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

extern void mach__gs_change__(void);

/***
 ***	Includes.
 ***/

#include "m_globals.h"
#include "m_regs.h"
#include "m_asm.h"
#include <memory.h>

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 *** 	Functions.
 ***/

/*
 * Helper function to download tiles and stipples to offscreen memory.
 *
 */

STATIC void
mach_graphics_state_offscreen_helper(
	struct mach_screen_state *screen_state_p,
	struct omm_allocation *allocation_p,
	int allocation_width,
	int allocation_height,
	const int source_step,
    int rounded_tile_width,
	SIbitmapP si_tile_p,
	void (*write_function_p)(const int, const int, void *),
	int *offscreen_width_p,
	int *offscreen_height_p)
{

	const int tile_height = si_tile_p->Bheight;

	const int tile_transfers =
		(source_step << 3) >>
		 screen_state_p->pixtrans_width_shift;

	int start_x;
	int start_y;
	int exact_tile_width;

	ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION, allocation_p));

	ASSERT(screen_state_p->register_state.alu_fg_fn ==
		   MACH_MIX_FN_PAINT);
	ASSERT((si_tile_p->BbitsPerPixel != 1) ||
		   (screen_state_p->register_state.alu_bg_fn ==
			MACH_MIX_FN_PAINT));
	
	ASSERT(screen_state_p->current_graphics_engine_mode ==
		   MACH_GE_MODE_ATI_MODE);


	/*
	 * We no longer rotate the tile/stipple at download time.  Set the
	 * clip rectangle to be allocation width, and download the
	 * tile/stipple into offscreen memory.
	 */

	MACH_STATE_SET_ATI_CLIP_RECTANGLE(screen_state_p,
		allocation_p->x, allocation_p->y,
		allocation_p->x + allocation_width,
	    allocation_p->y + allocation_height);

	mach_asm_transfer_helper(
		(void *) si_tile_p->Bptr,
		source_step,
		tile_transfers,
		tile_height,
        allocation_p->x,
	    allocation_p->y,
		rounded_tile_width,
		tile_height,
		write_function_p,
		screen_state_p->pixtrans_register,
		MACH_ASM_TRANSFER_TO_VIDEO_MEMORY);

	/*
	 * We now duplicate the tile / stipple into the offscreen area if
	 * necessary.
	 */

	if (si_tile_p->BbitsPerPixel == 1)
	{
		/*
		 * Duplicate a stipple.  Set the read mask to the planemask of the
		 * allocated area.   The write mask has already been set
		 * correctly for the previous download operation.  Set up
		 * DP_CONFIG to do screen to screen stippling.
		 */
		
		MACH_STATE_SET_RD_MASK(screen_state_p,
							   allocation_p->planemask);
		
		MACH_STATE_SET_DP_CONFIG(screen_state_p,
			(screen_state_p->dp_config_flags |
			 MACH_DP_CONFIG_WRITE |
			 MACH_DP_CONFIG_ENABLE_DRAW |
			 MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR |
			 MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
			 MACH_DP_CONFIG_READ_MODE_MONO_DATA |
			 MACH_DP_CONFIG_MONO_SRC_BLIT));

		ASSERT(screen_state_p->register_state.alu_fg_fn ==
			   MACH_MIX_FN_PAINT);
		ASSERT(screen_state_p->register_state.alu_bg_fn ==
			   MACH_MIX_FN_PAINT);
	}
	else 
	{
		/*
		 * Duplicate a tile.  Set the DP_CONFIG register to do screen
		 * to screen pixmap blits.  The rop's have already been
		 * programmed correctly for the download operation which just
		 * completed. 
		 */

		MACH_STATE_SET_DP_CONFIG(screen_state_p,
			(screen_state_p->dp_config_flags |
			 MACH_DP_CONFIG_WRITE |
			 MACH_DP_CONFIG_ENABLE_DRAW |
			 MACH_DP_CONFIG_FG_COLOR_SRC_BLIT));

		ASSERT(screen_state_p->register_state.wrt_mask == 
			   ((1 << screen_state_p->generic_state.screen_depth) - 1));
		ASSERT(screen_state_p->register_state.alu_fg_fn ==
			   MACH_MIX_FN_PAINT);
	}
	
	/*
	 * We have to work with exact tile widths now.
	 */

	exact_tile_width = si_tile_p->Bwidth;

	*offscreen_width_p =
		(allocation_width / si_tile_p->Bwidth) *
		si_tile_p->Bwidth; /* integral tile widths */

	/*
	 * Duplicate horizontally
	 */

	start_x = allocation_p->x + exact_tile_width;

	while (start_x < allocation_p->x + allocation_width)
	{
		mach_asm_move_screen_bits(allocation_p->x,
		    allocation_p->y,
			start_x, allocation_p->y,
			exact_tile_width, tile_height);
			start_x += exact_tile_width;
	}

	/*
	 * Duplicate vertically.
	 */

	*offscreen_height_p =
		(allocation_height / si_tile_p->Bheight) *
		si_tile_p->Bheight;	/* integral tile heights */

	start_y = allocation_p->y + tile_height;

	while (start_y < allocation_p->y + allocation_height)
	{
		mach_asm_move_screen_bits(allocation_p->x,
			allocation_p->y,
			allocation_p->x, start_y,
			allocation_width, tile_height);
		start_y += tile_height;
	}

	/*
	 * Mark everything that has deviated from the graphics
	 * state, which the associated macros had not handled.
	 */
	screen_state_p->generic_state.screen_current_clip =
		GENERIC_CLIP_NULL;
	MACH_STATE_SET_FLAGS(screen_state_p,
						 (MACH_INVALID_CLIP |
						  MACH_INVALID_PATTERN_REGISTERS));

}

/*
 * mach_graphics_state_download_tile
 * 
 * prepare a tile for use by drawing code -- this could involve
 * downloading the tile into offscreen memory, rotating the tile,
 * expanding a small tile for use in 4-bit mode etc.
 */
function void
mach_graphics_state_download_tile(struct mach_screen_state *screen_state_p,
								  struct mach_graphics_state *graphics_state_p,
								  SIbitmapP si_tile_p)
{
	struct mach_tile_state *tile_state_p =
		&(graphics_state_p->current_tile_state);

	int ppw;
	int pixels_per_pattern_register;

	ASSERT(tile_state_p->is_downloaded == FALSE);

	ASSERT(screen_state_p->current_graphics_engine_mode ==
		   MACH_GE_MODE_ATI_MODE);
	
	tile_state_p->source_step =
		(((si_tile_p->Bwidth +
		   ((32 >> screen_state_p->generic_state.screen_depth_shift) - 1)) &
		   ~((32 >> screen_state_p->generic_state.screen_depth_shift) - 1)) <<
		 screen_state_p->generic_state.screen_depth_shift) >> 3;

	/*
	 * compute the number of pixtrans words in one tile width.
	 * in the case where depth <= pixtrans width, this will have a
	 * positive value.
	 */
	ppw = screen_state_p->pixels_per_pixtrans;

	if (ppw)
	{
		tile_state_p->number_of_pixtrans_words_per_tile_width =
			((si_tile_p->Bwidth + (ppw-1)) & ~(ppw - 1)) >>
			screen_state_p->pixels_per_pixtrans_shift;
	}
	else
	{
		tile_state_p->number_of_pixtrans_words_per_tile_width =
			(si_tile_p->BbitsPerPixel * si_tile_p->Bwidth) >>
			screen_state_p->pixtrans_width_shift;
	}

	/*
	 * Compute additional stuff if usage of the pattern registers is
	 * allowed.
	 */

	if (((screen_state_p->options_p->rectfill_options &
		  MACH_OPTIONS_RECTFILL_OPTIONS_USE_PATTERN_REGISTERS) ||
		 (screen_state_p->options_p->spansfill_options &
		  MACH_OPTIONS_SPANSFILL_OPTIONS_USE_PATTERN_REGISTERS)))
	{
		
		/*
		 * Handling 4 bit deep cases.
		 */
		if (si_tile_p->BbitsPerPixel == 4)
		{
			/*
			 * In 4 bit mode, we need to expand the small tile before it
			 * can be used in the pattern registers.
			 */
			pixels_per_pattern_register = 2;

		}
		else
		{
			/*
			 * Valid upto 16 bpp.  This will become `0' for 32bpp modes.
			 */
			pixels_per_pattern_register =
				DEFAULT_MACH_PATTERN_REGISTER_WIDTH >>
					screen_state_p->generic_state.screen_depth_shift;
		}

		if (pixels_per_pattern_register != 0)
		{
		
			tile_state_p->number_of_patt_data_registers_per_tile_width =
				((si_tile_p->Bwidth + (pixels_per_pattern_register - 1)) &
				 ~(pixels_per_pattern_register - 1)) /
					 pixels_per_pattern_register;
		}
		else						/* 24bpp/32bpp modes */
		{
			tile_state_p->number_of_patt_data_registers_per_tile_width =
				0;
		}
	
		/*
		 * check if a tile width can be placed entirely within the color
		 * pattern registers.
		 */

		if (si_tile_p->Bwidth <=
			(DEFAULT_MACH_NUMBER_OF_COLOR_PATTERN_DATA_REGISTERS *
			 pixels_per_pattern_register))
		{
			int expanded_size;
			unsigned char *source_p;
			unsigned char *destination_p;
			int count;

			ASSERT(tile_state_p->is_small_tile == TRUE);

			if (si_tile_p->BbitsPerPixel == 4)
			{
				if (tile_state_p->expanded_tile_p)
				{
					free_memory(tile_state_p->expanded_tile_p->Bptr);
					free_memory(tile_state_p->expanded_tile_p);
					tile_state_p->expanded_tile_p = NULL;
				}

				expanded_size = tile_state_p->source_step *
					si_tile_p->Bheight * 2;

				tile_state_p->expanded_tile_p =
					allocate_memory(sizeof(SIbitmap));

				/*
				 * copy out tile details.  Note that the origin may change
				 * so this must be checked at drawing time.
				 */
				*tile_state_p->expanded_tile_p = *si_tile_p;

				tile_state_p->expanded_tile_p->Bptr =
					allocate_memory(expanded_size);

				/*
				 * We need to inform the drawing code the step between
				 * rows has increased too.
				 */
				tile_state_p->expanded_source_step =
					(tile_state_p->source_step << 1);

				/*
				 * Copy out the pixels, expanding each nibble to a byte.
				 */
				source_p = (unsigned char *) si_tile_p->Bptr;
				destination_p = (unsigned char *)
					tile_state_p->expanded_tile_p->Bptr;

				for (count = 0;
					 count < (expanded_size >> 1);
					 count++, source_p++)
				{
					*destination_p++ = ((*source_p) & 0x0FU);
					*destination_p++ = (((*source_p) & 0xF0U) >> 4U);
				}
			}
			else
			{
				/*
				 * in the 8 bit case, we don't need to expand the tile.
				 * as the pattern registers support contiguous pixels.
				 */
				tile_state_p->expanded_tile_p = si_tile_p;
				tile_state_p->expanded_source_step =
					tile_state_p->source_step;
			}
		}
	}
	

	/*
	 * Download the tile into offscreen memory if possible.
	 */

	if ((screen_state_p->options_p->rectfill_options &
		 MACH_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY))
	{

		const int allocation_width =
			(si_tile_p->Bwidth <
			 screen_state_p->options_p->offscreen_tile_padded_width) ?
			 screen_state_p->options_p->offscreen_tile_padded_width :
			 si_tile_p->Bwidth;

		const int allocation_height =
			(si_tile_p->Bheight <
			 screen_state_p->options_p->offscreen_tile_padded_height) ?
			 screen_state_p->options_p->offscreen_tile_padded_height :
			 si_tile_p->Bheight;

		/*
		 * Allocate an offscreen area if there is none associated with
		 * this tile.
		 */

		if (tile_state_p->offscreen_location_p == NULL)
		{
			tile_state_p->offscreen_location_p =
				omm_allocate(allocation_width, allocation_height,
						 si_tile_p->BbitsPerPixel, OMM_LONG_TERM_ALLOCATION);
		}
		else
		{

			/*
			 * If the current offscreen area is smaller than what we
			 * want, or if the current area is too large, we allocate
			 * a new area. 
			 */

			if ((tile_state_p->offscreen_location_p->width <
				 allocation_width) ||
				(tile_state_p->offscreen_location_p->height <
				 allocation_height) ||
				((tile_state_p->offscreen_location_p->width -
				 allocation_width) >
				  screen_state_p->options_p->offscreen_tile_padded_width) ||
				((tile_state_p->offscreen_location_p->height -
				  allocation_height) >
				 screen_state_p->options_p->offscreen_tile_padded_height))
			{
				
				ASSERT(tile_state_p->offscreen_location_p);
				(void) omm_free(tile_state_p->offscreen_location_p);
				tile_state_p->offscreen_location_p =
					omm_allocate(allocation_width, allocation_height,
						 si_tile_p->BbitsPerPixel,
								 OMM_LONG_TERM_ALLOCATION);
			}

			/*
			 * else we reuse the same area of this tile.
			 */
			
		}

		ASSERT(!tile_state_p->offscreen_location_p || 
			   ((tile_state_p->offscreen_location_p->width >=
				 allocation_width) &&
				(tile_state_p->offscreen_location_p->height >=
				 allocation_height)));
		
		if (OMM_LOCK(tile_state_p->offscreen_location_p))
		{

			ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION,
									 tile_state_p->offscreen_location_p));

			/*
			 * Set up DP_CONFIG for a memory to screen transfer.
			 */

			MACH_STATE_SET_DP_CONFIG(screen_state_p,
				(screen_state_p->dp_config_flags |
				 MACH_DP_CONFIG_FG_COLOR_SRC_HOST |
				 MACH_DP_CONFIG_ENABLE_DRAW |
				 MACH_DP_CONFIG_MONO_SRC_ONE |
				 MACH_DP_CONFIG_WRITE));

			/*
			 * Switch to ATI context.
			 */

			MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

			/*
			 * Reset the planemasks and the FG rop.
			 */

			MACH_STATE_SET_FG_ROP(screen_state_p, MACH_MIX_FN_PAINT);
			MACH_STATE_SET_WRT_MASK(screen_state_p, ~0U);
			MACH_STATE_SET_RD_MASK(screen_state_p, ~0U);

			mach_graphics_state_offscreen_helper(
				screen_state_p,
				tile_state_p->offscreen_location_p,
				allocation_width,
				allocation_height,
				tile_state_p->source_step,
				((tile_state_p->source_step << 3) >>
				 screen_state_p->generic_state.screen_depth_shift),
				si_tile_p,
				(screen_state_p->screen_write_pixels_p),
				&(tile_state_p->offscreen_width),
				&(tile_state_p->offscreen_height));
		}
		else					/* OMM LOCK failed */
		{
			/*
			 * Free the offscreen location if present.
			 */

			if (tile_state_p->offscreen_location_p != NULL)
			{
				(void) omm_free(tile_state_p->offscreen_location_p);
			}
			
			tile_state_p->offscreen_location_p = NULL;
		}
	}

	/*
	 * Mark that we have finished processing this tile.
	 */
	tile_state_p->is_downloaded = TRUE;
	
	STAMP_OBJECT(MACH_TILE_STATE, tile_state_p);
}

/*
 * mach_invert_stipple_bits
 *
 * Perform byte-wise inversion of the graphics state's stipple, so
 * that at run-time this step can be avoided.
 */
STATIC boolean
mach_invert_stipple_bits(struct mach_screen_state *screen_state_p,
						 SIbitmapP si_stipple_p,
						 SIbitmapP new_bitmap_p)
{

	int stipple_row_size = ((si_stipple_p->Bwidth + 31) & ~31) >> 3;
	int chars_in_stipple;
	unsigned char *source_bits_p, *destination_bits_p;

	/*
	 * get rid of allocated space.
	 */
	if (new_bitmap_p->Bptr)
	{
		free_memory(new_bitmap_p->Bptr);
		new_bitmap_p->Bptr = NULL;
	}

	/*
	 * copy out the stipple details.  Note that SI can change the
	 * origin of the stipple at any time, so we need to check these at
	 * drawing time.
	 */
	*new_bitmap_p = *si_stipple_p;

	chars_in_stipple = stipple_row_size * new_bitmap_p->Bheight;


	new_bitmap_p->Bptr = allocate_memory(chars_in_stipple);

	if (!new_bitmap_p->Bptr)
	{
		return FALSE;
	}

	source_bits_p = (unsigned char *) si_stipple_p->Bptr;
	destination_bits_p = (unsigned char *) new_bitmap_p->Bptr;

	while(chars_in_stipple--)
	{
		*destination_bits_p++ =
			(screen_state_p->byte_invert_table_p[*source_bits_p++]);
	}

	return TRUE;
}

/*
 * mach_graphics_state_download_stipple
 *
 * Prepare a stipple for usage by drawing code.  This may involve
 * downloading the stipple to offscreen memory, inverting the stipple
 * bits to conform to the expected order of the drawing engine etc.
 */
function void
mach_graphics_state_download_stipple(
	struct mach_screen_state *screen_state_p,
	struct mach_graphics_state *graphics_state_p,
	SIbitmapP si_stipple_p)
{

	struct mach_stipple_state *stipple_state_p =
		&(graphics_state_p->current_stipple_state);

	int pixtrans_width;

	ASSERT(stipple_state_p->is_downloaded == FALSE);
	ASSERT(screen_state_p->current_graphics_engine_mode ==
		   MACH_GE_MODE_ATI_MODE);

	/*
	 * round off to the next long word.
	 */
	stipple_state_p->source_step =
		((si_stipple_p->Bwidth + 31) & ~31) >> 3;

	/*
	 * compute the number of pixtrans words in one stipple width.  in
	 * the case where depth <= pixtrans width, this will have a
	 * positive value. 
	 */
	pixtrans_width = screen_state_p->pixtrans_width;


	stipple_state_p->number_of_pixtrans_words_per_stipple_width =
		((si_stipple_p->Bwidth + (pixtrans_width - 1)) &
		 ~(pixtrans_width - 1)) >>
			 screen_state_p->pixtrans_width_shift;

	/*
	 * invert the stipple.
	 */

	(void) mach_invert_stipple_bits(screen_state_p, si_stipple_p,
									&(stipple_state_p->inverted_stipple));

	/*
	 * Download the stipple into offscreen memory if possible.
	 */

	if ((screen_state_p->options_p->rectfill_options &
		 MACH_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY))
	{
		const int allocation_width =
			(si_stipple_p->Bwidth <
			 screen_state_p->options_p->offscreen_stipple_padded_width) ?
			 screen_state_p->options_p->offscreen_stipple_padded_width :
			 si_stipple_p->Bwidth;

		const int allocation_height =
			(si_stipple_p->Bheight <
			 screen_state_p->options_p->offscreen_stipple_padded_height) ?
			 screen_state_p->options_p->offscreen_stipple_padded_height :
			 si_stipple_p->Bheight;

		/*
		 * Allocate an offscreen area if there is none associated with
		 * this stipple.
		 */

		if (stipple_state_p->offscreen_location_p == NULL)
		{
			stipple_state_p->offscreen_location_p =
				omm_allocate(allocation_width, allocation_height,
						 si_stipple_p->BbitsPerPixel, 
							 OMM_LONG_TERM_ALLOCATION);
		}
		else
		{

			/*
			 * If the current offscreen area is smaller than what we
			 * want, we allocate a new area.
			 */

			if ((stipple_state_p->offscreen_location_p->width <
				 allocation_width) ||
				(stipple_state_p->offscreen_location_p->height <
				 allocation_height) ||
				((stipple_state_p->offscreen_location_p->width -
				  allocation_width) >
				 screen_state_p->options_p->offscreen_stipple_padded_width) ||
				((stipple_state_p->offscreen_location_p->height -
				  allocation_height) >
				 screen_state_p->options_p->offscreen_stipple_padded_height))
			{

				ASSERT(stipple_state_p->offscreen_location_p);
				(void) omm_free(stipple_state_p->offscreen_location_p); 
				stipple_state_p->offscreen_location_p =
					omm_allocate(allocation_width, allocation_height,
						 si_stipple_p->BbitsPerPixel,
								 OMM_LONG_TERM_ALLOCATION);
			}

			/*
			 * else we reuse the same area of this stipple.
			 */
			
		}

		ASSERT(!stipple_state_p->offscreen_location_p ||
			   ((stipple_state_p->offscreen_location_p->width >=
				 allocation_width) &&
				(stipple_state_p->offscreen_location_p->height >=
				 allocation_height)));
		
		if (OMM_LOCK(stipple_state_p->offscreen_location_p))
		{
			ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION,
									 stipple_state_p->offscreen_location_p));

#if (defined(__DEBUG__))
	if (mach_graphics_state_debug)
	{
		struct omm_allocation * allocation_p =
			stipple_state_p->offscreen_location_p;
		
		(void) fprintf(debug_stream_p,
"(mach_gs_download_stipple)\n"
"{\n"
"\t# Offscreen location\n"
"\tx = %d\n"
"\ty = %d\n"
"\twidth = %d\n"
"\theight = %d\n"
"\tplanemask = %d\n"
"}\n",
					   allocation_p->x, allocation_p->y,
					   allocation_p->width, allocation_p->height,
					   allocation_p->planemask);
		
	}
#endif
			/*
			 * Switch to ATI context.
			 */
			MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

			/*
			 * Set up DP_CONFIG for a memory to screen transfer.
			 */
			MACH_STATE_SET_DP_CONFIG(screen_state_p,
				(screen_state_p->dp_config_flags |
				 MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR |
				 MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
				 MACH_DP_CONFIG_ENABLE_DRAW |
				 MACH_DP_CONFIG_MONO_SRC_HOST |
				 MACH_DP_CONFIG_READ_MODE_MONO_DATA |
				 MACH_DP_CONFIG_WRITE));

			MACH_STATE_SET_FRGD_COLOR(screen_state_p, ~0U);
			MACH_STATE_SET_BKGD_COLOR(screen_state_p, 0);

			MACH_STATE_SET_FG_ROP(screen_state_p, MACH_MIX_FN_PAINT);
			MACH_STATE_SET_BG_ROP(screen_state_p, MACH_MIX_FN_PAINT);

			MACH_STATE_SET_WRT_MASK(screen_state_p,
						stipple_state_p->offscreen_location_p->planemask);

			mach_graphics_state_offscreen_helper(
				screen_state_p,
				stipple_state_p->offscreen_location_p,
				allocation_width,
				allocation_height,
				stipple_state_p->source_step,
                (stipple_state_p->source_step << 3), /* width in bits */
				&(stipple_state_p->inverted_stipple),
				(screen_state_p->screen_write_bits_p),
				&(stipple_state_p->offscreen_width),
				&(stipple_state_p->offscreen_height));
		}
		else					/* OMM_LOCK failed */
		{

			/*
			 * Free the offscreen location if present.
			 */

			if (stipple_state_p->offscreen_location_p)
			{
				(void) omm_free(stipple_state_p->offscreen_location_p);
			}
			
			stipple_state_p->offscreen_location_p = NULL;
		}
	}

	/*
	 * mark that this stipple has been processed
	 */
	stipple_state_p->is_downloaded = TRUE;
	
	STAMP_OBJECT(MACH_STIPPLE_STATE, stipple_state_p);

}

STATIC void
mach_graphics_state_compute_line_pattern(
	struct mach_screen_state *screen_state_p,
	struct mach_graphics_state *graphics_state_p,
	SIint32 *dash_list_p,
    SIint32 dash_count)
{

	struct mach_line_state *line_state_p =
		&(graphics_state_p->current_line_state);

	/*
	 * Compute the dash pattern to be programmed into the
	 * monopattern registers.
	 */
	unsigned int	monopattern_register_value = 0;
	unsigned int 	i;
	unsigned int	totalbits;
	/*
	 * CHIPSET BUG:
	 * 16 bit modes dont seem to use the second set of monopattern registers.
	 */
	int			number_of_monopattern_registers = 
				(screen_state_p->generic_state.screen_depth == 16 ?
				 DEFAULT_MACH_NUMBER_OF_MONOCHROME_PATTERN_DATA_REGISTERS - 1 :
				 DEFAULT_MACH_NUMBER_OF_MONOCHROME_PATTERN_DATA_REGISTERS);

	/*
	 * Compute the value that has to go into the mono pattern
	 * registers. Note that the the bits are in the most significant
	 * bit in byte would become the left most screen bit.
	 */
	line_state_p->is_pattern_valid = FALSE;

	for (i = 0,totalbits=0;
		 i < dash_count;
		 i++)
	{
		unsigned int tmp = (unsigned int) ~0U;

		/*
		 * If the total number of bits in the line pattern
		 * crosses those available in the pattern registes,
		 * we have to give up.
		 */
		if((totalbits += dash_list_p[i]) >
		   (DEFAULT_MACH_PATTERN_REGISTER_WIDTH *
#ifdef DELETE
			DEFAULT_MACH_NUMBER_OF_MONOCHROME_PATTERN_DATA_REGISTERS))
#endif
			number_of_monopattern_registers))
		{
			return;
		}

		monopattern_register_value <<= dash_list_p[i];

		if(i & 1)
		{
			monopattern_register_value  &= (tmp <<
											dash_list_p[i]);
		}
		else
		{
			monopattern_register_value  |=
				~(tmp << dash_list_p[i]);

		}
	}

	monopattern_register_value <<= (32 - totalbits);

	/*
	 * We need to concatenate the dash list with itself if the dash
	 * count is odd, according to the X protocol.
	 */

	if (dash_count & 1)
	{
		monopattern_register_value |=
			(monopattern_register_value >> totalbits);
		totalbits <<= 1;
	}

	/*
	 * The complete number of bits should fit in the pattern
	 * registers.
	 */
	if (totalbits >
		(DEFAULT_MACH_PATTERN_REGISTER_WIDTH *
#ifdef DELETE
		 DEFAULT_MACH_NUMBER_OF_MONOCHROME_PATTERN_DATA_REGISTERS))
#endif
		 number_of_monopattern_registers))
	{
		return;
	}

	i = monopattern_register_value;


	monopattern_register_value = (((i >> 24U) & 0x000000ffU) |
								  ((i >>  8U) & 0x0000ff00U) |
								  ((i <<  8U) & 0x00ff0000U) |
								  ((i << 24U) & 0xff000000U));

#if (defined(__DEBUG__))
	if (mach_graphics_state_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_graphics_state_compute_line_pattern)\n"
"{\n"
"\tmonopattern_register = 0x%x\n"
"\tmonopattern_length   = %d\n"
"}\n",
					   monopattern_register_value, totalbits);
	}
#endif

	line_state_p->dash_pattern = monopattern_register_value;
	line_state_p->dash_pattern_length = totalbits;

	line_state_p->is_pattern_valid = TRUE;

	STAMP_OBJECT(MACH_LINE_STATE, line_state_p);

}

/*
 * mach_graphics_state_download_state
 *
 * Downloading of a graphics state from SI.  After calling the generic
 * layers `download_state' method, we save any downloaded tile and
 * stipples and perform whatever transformations are needed for these.
 */

STATIC SIBool
mach_graphics_state_download_state(SIint32 state_index,
								   SIint32 state_flag,
								   SIGStateP state_p)
{

	MACH_CURRENT_GRAPHICS_STATE_DECLARE();
	MACH_CURRENT_SCREEN_STATE_DECLARE();

	struct mach_graphics_state *new_graphics_state_p;

	/*
	 * CHIPSET BUG:
	 * 16 bit modes dont seem to use the second set of monopattern registers.
	 */
	int		number_of_monopattern_registers = 
			(screen_state_p->generic_state.screen_depth == 16 ?
			 DEFAULT_MACH_NUMBER_OF_MONOCHROME_PATTERN_DATA_REGISTERS - 1 :
			 DEFAULT_MACH_NUMBER_OF_MONOCHROME_PATTERN_DATA_REGISTERS);

#if (defined(__DEBUG__))
	if (mach_graphics_state_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_graphics_state_download_state)\n"
"{\n"
"\tstate_index = %ld\n"
"\tstate_flag = 0x%lx\n"
"\tstate_p = %p\n"
"}\n",
					   state_index,
					   state_flag,
					   (void *) state_p);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
	    (struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE,
							 graphics_state_p));

	ASSERT(state_index >= 0 && state_index <
		   generic_current_screen_state_p->screen_number_of_graphics_states);

	/*
	 * Call the generic libraries download state.
	 */

	if ((*graphics_state_p->generic_si_functions.si_download_state)
		(state_index, state_flag, state_p) != SI_SUCCEED)
	{
#if (defined(__DEBUG__))
		if (mach_graphics_state_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_download_state)\n"
"{\n"
"\tgeneric download state failed.\n"
"}\n"
						   );
		}
#endif

		return (SI_FAIL);
	}

	/*
	 * retrieve the new graphics state.
	 */
	new_graphics_state_p = (struct mach_graphics_state *)
		screen_state_p->generic_state.screen_graphics_state_list_pp
			[state_index];

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		   (struct generic_graphics_state *) new_graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE,
							 new_graphics_state_p));


	/*
	 * Downloading of tiles and stipples needs special care : SI
	 * attempts to download tiles and stipples for every graphics
	 * state, irrespective of whether the impending drawing operation
	 * would use it or not, and also irrespective of whether the tile
	 * had previously been downloaded.  This makes the penalty for
	 * even simple graphics state changes very high as the associated
	 * tile/stipple will get downloaded everytime the graphics state
	 * is changed.  In order to prevent this we adopt a `lazy'
	 * strategy of not actually downloading and processing the tile or
	 * stipple until and unless an impending drawing operation
	 * requires the tile or stipple.
	 */

	/*
	 * Mark the tile as needing processing.
	 */
	if (state_flag & SetSGtile)
	{

		/*
		 * Mark the tile as being suitable for use in the pattern
		 * registers if the user has allowed useage of the pattern
		 * registers, and if the tile dimensions are suitably small.
		 */

		if (((screen_state_p->options_p->rectfill_options &
			  MACH_OPTIONS_RECTFILL_OPTIONS_USE_PATTERN_REGISTERS) ||
			 (screen_state_p->options_p->spansfill_options &
			  MACH_OPTIONS_SPANSFILL_OPTIONS_USE_PATTERN_REGISTERS)))
		{
			
			int pixels_per_pattern_register;
		
			/*
			 * Handling 4 bit deep cases.
			 */
			if (state_p->SGtile->BbitsPerPixel == 4)
			{
				/*
				 * In 4 bit mode, we need to expand the small tile before
				 * it can be used in the pattern registers.  Thus each 16
				 * bit pattern register actually holds only 2 pixels. 
				 */
				pixels_per_pattern_register = 2;

			}
			else
			{
				/*
				 * 8 and 16 bit cases.  Modes with bpp greater than 16
				 * can't use the graphics engine in the MACH32 chipset.
				 */
				pixels_per_pattern_register =
					DEFAULT_MACH_PATTERN_REGISTER_WIDTH >>
						screen_state_p->generic_state.screen_depth_shift;
			}

			if (state_p->SGtile->Bwidth <=
				(DEFAULT_MACH_NUMBER_OF_COLOR_PATTERN_DATA_REGISTERS *
				 pixels_per_pattern_register))
			{
				new_graphics_state_p->current_tile_state.is_small_tile =
					TRUE;
			}
			else
			{
				new_graphics_state_p->current_tile_state.is_small_tile =
					FALSE;
			}
		}
		else
		{
			new_graphics_state_p->current_tile_state.is_small_tile =
				FALSE;			/* don't use pattern registers */
		}
		
		/*
		 * Defer the actual downloading till the first point of use.
		 */
		new_graphics_state_p->current_tile_state.is_downloaded =
			FALSE;
		
	}

	/*
	 * Mark the stipple as needed processing.
	 */
	if (state_flag & SetSGstipple)
	{
		/*
		 * check if the stipple can be placed in the monochrome pattern
		 * registers.
		 */
		if ((screen_state_p->options_p->spansfill_options &
			 MACH_OPTIONS_SPANSFILL_OPTIONS_USE_PATTERN_REGISTERS) &&
			(state_p->SGstipple->Bwidth <= /* depth == 1 here */
#ifdef DELETE
			 (DEFAULT_MACH_NUMBER_OF_MONOCHROME_PATTERN_DATA_REGISTERS *
#endif
			 (number_of_monopattern_registers *
			  DEFAULT_MACH_PATTERN_REGISTER_WIDTH)))
		{
			new_graphics_state_p->current_stipple_state.is_small_stipple = 
				TRUE; 
		}
		else
		{
			new_graphics_state_p->current_stipple_state.is_small_stipple = 
				FALSE;
		}

		/*
		 * Defer the actual downloading till the first point of use.
		 */
		new_graphics_state_p->current_stipple_state.is_downloaded = 
			FALSE;
		
	}

	/*
	 * Dashed line support.
	 */
	if (state_flag & SetSGline)
	{
		mach_graphics_state_compute_line_pattern(screen_state_p,
												 new_graphics_state_p,
												 state_p->SGline,
												 state_p->SGlineCNT);
	}

	return (SI_SUCCEED);

}

#if (defined(EXTRA_FUNCTIONALITY))

/*
 * mach_graphics_state_get_state
 *
 * Retrieve the current state for persual by SI.  This function is a
 * wrapper around the generic layers `get_state'.
 */

STATIC SIBool
mach_graphics_state_get_state(SIint32 state_index,
							   SIint32 state_flag,
							   SIGStateP state_p)
{

	MACH_CURRENT_GRAPHICS_STATE_DECLARE();

#if (defined(__DEBUG__))
	if (mach_graphics_state_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_graphics_state_get_state)\n"
"{\n"
"\tstate_index = %ld\n"
"\tstate_flag = 0x%lx\n"
"\tstate_p = %p\n"
"}\n",
					   state_index,
					   state_flag,
					   (void *) state_p);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
	    (struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE,
							 graphics_state_p));

	/*
	 * check the index passed down.
	 */

	if (state_index < 0 || state_index >=
		generic_current_screen_state_p->screen_number_of_graphics_states)
	{
		return (SI_FAIL);
	}

	/*
	 * Call the generic libraries `get_state' method.
	 */

	if ((*graphics_state_p->generic_si_functions.si_get_state)
		(state_index, state_flag, state_p) != SI_SUCCEED)
	{
#if (defined(__DEBUG__))
		if (mach_graphics_state_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_graphics_state_get_state)\n"
"{\n"
"\tgeneric get state failed.\n"
"}\n"
						   );
		}
#endif
		return (SI_FAIL);
	}

	return (SI_SUCCEED);

}

#endif /* EXTRA_FUNCTIONALITY */

/*
 * mach_graphics_state_select_state
 *
 * Selection of a graphics state involves informing all modules that
 * are interested in this event using a call to the generated
 * function "mach__gs_change__()".
 * Preliminary to this, we need to set the current graphics state
 * pointer to point to the newly selected state.
 *
 */

STATIC SIBool
mach_graphics_state_select_state(SIint32 state_index)
{

	MACH_CURRENT_GRAPHICS_STATE_DECLARE();

#if (defined(__DEBUG__))
	if (mach_graphics_state_debug)
	{

	(void) fprintf(debug_stream_p,
"(mach_graphics_state_select_state)\n"
"{\n"
"\tstate_index = %ld\n"
"}\n",
				   state_index);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
	    (struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE,
							 graphics_state_p));
	ASSERT(state_index >= 0 && state_index <
		   generic_current_screen_state_p->screen_number_of_graphics_states);

	/*
	 * check the index passed down.
	 */
	if (state_index < 0 || state_index >=
		generic_current_screen_state_p->screen_number_of_graphics_states)
	{
		return (SI_FAIL);
	}

	/*
	 * retrieve the graphics state requested.
	 */
	graphics_state_p = (struct mach_graphics_state *)
		generic_current_screen_state_p->
		screen_graphics_state_list_pp[state_index];

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE,
							 graphics_state_p));

	/*
	 * Check if anything was downloaded in the this state after the
	 * last call to select state.
	 */

	if (graphics_state_p->generic_state.si_state_flags == 0 &&
		(graphics_state_p ==
		 (struct mach_graphics_state *) 
		 generic_current_screen_state_p->screen_current_graphics_state_p))
	{
		return (SI_SUCCEED);
	}
	
	/*
	 * make the requested graphics state `current'.
	 */
	generic_current_screen_state_p->screen_current_graphics_state_p =
		(struct generic_graphics_state *) graphics_state_p;

	/*
	 * Now let each module handle its graphics state changes.
	 */
	mach__gs_change__();

	/*
	 * Mark this module as having been downloaded.
	 */

	graphics_state_p->generic_state.si_state_flags = 0;

	return (SI_SUCCEED);
}

/*
 * Initialize this module.
 */
function void
mach_graphics_state__initialize__(SIScreenRec *si_screen_p,
								  struct mach_options_structure *options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));

	/*
	 * Confirm that the offscreen allocation requests are a power of
	 * two.  Use the default other wise.
	 */

	if ((options_p->offscreen_tile_padded_height &
		 (options_p->offscreen_tile_padded_height - 1)) ||
		(options_p->offscreen_tile_padded_width &
		 (options_p->offscreen_tile_padded_width - 1)))
	{
		(void) fprintf(stderr,
			   MACH_OFFSCREEN_TILE_DIMENSION_IS_NOT_A_POWER_OF_TWO_MESSAGE,
			   options_p->offscreen_tile_padded_width,
			   options_p->offscreen_tile_padded_height,
			   DEFAULT_MACH_OFFSCREEN_TILE_PADDED_WIDTH,
			   DEFAULT_MACH_OFFSCREEN_TILE_PADDED_HEIGHT);
		options_p->offscreen_tile_padded_height =
			DEFAULT_MACH_OFFSCREEN_TILE_PADDED_HEIGHT;
		options_p->offscreen_tile_padded_width =
			DEFAULT_MACH_OFFSCREEN_TILE_PADDED_WIDTH;

	}


	if ((options_p->offscreen_stipple_padded_height &
		 (options_p->offscreen_stipple_padded_height - 1)) ||
		(options_p->offscreen_stipple_padded_width &
		 (options_p->offscreen_stipple_padded_width - 1)))
	{
		(void) fprintf(stderr,
			   MACH_OFFSCREEN_STIPPLE_DIMENSION_IS_NOT_A_POWER_OF_TWO_MESSAGE,
			   options_p->offscreen_stipple_padded_width,
			   options_p->offscreen_stipple_padded_height,
			   DEFAULT_MACH_OFFSCREEN_STIPPLE_PADDED_WIDTH,
			   DEFAULT_MACH_OFFSCREEN_STIPPLE_PADDED_HEIGHT);
		options_p->offscreen_stipple_padded_height =
			DEFAULT_MACH_OFFSCREEN_STIPPLE_PADDED_HEIGHT;
		options_p->offscreen_stipple_padded_width =
			DEFAULT_MACH_OFFSCREEN_STIPPLE_PADDED_WIDTH;

	}

	/*
	 * Check if we need to fill in the graphics state list.
	 */
	if (generic_current_screen_state_p->screen_graphics_state_list_pp ==
		NULL)
	{
		int count;

#if (defined(__DEBUG__))
		if (mach_graphics_state_debug)
		{
			(void) fprintf(debug_stream_p,
						   "(mach_graphics_state__initialize__) "
						   "allocating space for graphics states.\n");
		}
#endif

		si_screen_p->flagsPtr->SIstatecnt =
			generic_current_screen_state_p->screen_number_of_graphics_states =
			(options_p->number_of_graphics_states != 
			 MACH_OPTIONS_NUMBER_OF_GRAPHICS_STATES_DEFAULT) ?
				 options_p->number_of_graphics_states :
				 DEFAULT_MACH_NUMBER_OF_GRAPHICS_STATES;
		generic_current_screen_state_p->screen_graphics_state_list_pp =
			allocate_and_clear_memory(si_screen_p->flagsPtr->SIstatecnt *
									  sizeof(struct mach_graphics_state *));

		for (count = 0; count < si_screen_p->flagsPtr->SIstatecnt;
			 count ++)
		{
			struct mach_graphics_state *graphics_state_p;

			graphics_state_p =
					allocate_and_clear_memory(
					sizeof(struct mach_graphics_state));
			generic_current_screen_state_p->
				screen_graphics_state_list_pp[count] =
					(struct generic_graphics_state *) graphics_state_p;

#if (defined(__DEBUG__))
			STAMP_OBJECT(GENERIC_GRAPHICS_STATE,
						 (struct generic_graphics_state *)
						 graphics_state_p);
			STAMP_OBJECT(MACH_GRAPHICS_STATE,
						 graphics_state_p);
#endif
			/*
			 * Force initialize GS's to reprogram the chipset.
			 */
			graphics_state_p->generic_state.si_state_flags = ~0U;

		}
		/*
		 * Initialize Graphics state index `0' to a known value.
		 */
		generic_current_screen_state_p->screen_current_graphics_state_p =
			generic_current_screen_state_p->screen_graphics_state_list_pp[0];

	}
#if (defined(__DEBUG__))
	else
	{
		if (mach_graphics_state_debug)
		{
			(void) fprintf(debug_stream_p,
						   "(mach_graphics_state__initialize__) no "
						   "allocation at chipset level.\n");
		}
	}
#endif

	functions_p->si_download_state = mach_graphics_state_download_state;

#if (defined(EXTRA_FUNCTIONALITY))
	/*
	 * We don't really need this as of now.
	 */
 	functions_p->si_get_state = mach_graphics_state_get_state; 
#endif /* EXTRA_FUNCTIONALITY */

	functions_p->si_select_state = mach_graphics_state_select_state;

}
