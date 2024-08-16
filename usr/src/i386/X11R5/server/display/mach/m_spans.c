/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m_spans.c	1.4"

/***
 ***	NAME
 ***
 *** 		mach_spans.c : spans routines for the MACH display
 *** 	library.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "m_spans.h"
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
 ***	SEE ALSO
 ***
 ***	CAVEATS
 ***
 ***	BUGS
 ***
 ***	HISTORY
 ***
 ***/

#ident	"@(#)mach:mach/m_spans.c	1.2"

PUBLIC

/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <sidep.h>
#include "stdenv.h"
#include "m_globals.h"
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
export boolean mach_fillspans_debug = FALSE;
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

/***
 ***	Includes.
 ***/

#include "m_state.h"
#include "m_gs.h"
#include "m_asm.h"

/***
 ***	Constants.
 ***/

#define MACH_FILLSPANS_SOLID_FG_DEPENDENCIES\
	(MACH_INVALID_FG_ROP|MACH_INVALID_FOREGROUND_COLOR|MACH_INVALID_WRT_MASK)

#define MACH_FILLSPANS_SOLID_BG_DEPENDENCIES\
	(MACH_INVALID_FG_ROP|MACH_INVALID_BACKGROUND_COLOR|MACH_INVALID_WRT_MASK)

#define MACH_FILLSPANS_TILE_DEPENDENCIES\
	(MACH_INVALID_FG_ROP|MACH_INVALID_WRT_MASK)

#define MACH_FILLSPANS_STIPPLE_TRANSPARENT_DEPENDENCIES\
	(MACH_INVALID_FG_ROP|\
	 MACH_INVALID_WRT_MASK|\
	 MACH_INVALID_FOREGROUND_COLOR)

#define MACH_FILLSPANS_STIPPLE_OPAQUE_DEPENDENCIES\
	(MACH_INVALID_FG_ROP|\
	 MACH_INVALID_BG_ROP|\
	 MACH_INVALID_WRT_MASK|\
	 MACH_INVALID_FOREGROUND_COLOR|\
	 MACH_INVALID_BACKGROUND_COLOR)

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/

STATIC SIBool
mach_fillspans_solid(SIint32 count, register SIPointP points_p,
					 register SIint32 *widths_p)
{

	unsigned short dp_config;
	const SIPointP points_fence_p = points_p + count;
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE,
							 graphics_state_p));

	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		   SGFillSolidFG ||
		   graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		   SGFillSolidBG);

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}
	
	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	/*
	 * Set DP CONFIG depending on the fill mode (FG/BG)
	 */

	if (graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		SGFillSolidFG)
	{
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
									 MACH_FILLSPANS_SOLID_FG_DEPENDENCIES);
	}
	else
	{
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
									 MACH_FILLSPANS_SOLID_BG_DEPENDENCIES);
	}

	/*
	 * Reset the clip rectangles.
	 */

	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
#if (defined(__DEBUG__))
		if (mach_fillspans_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_fillspans_solid) resetting clip rectangle {\n"
"\tfrom (%hd,%hd),(%hd,%hd)\n"
"\tto (%d,%d),(%d,%d)\n"
"}\n",
					   
				   screen_state_p->register_state.ext_scissor_l,
				   screen_state_p->register_state.ext_scissor_t,
				   screen_state_p->register_state.ext_scissor_r,
				   screen_state_p->register_state.ext_scissor_b,
				   0,
				   0,
				   screen_state_p->generic_state.screen_virtual_width-1,
				   screen_state_p->generic_state.screen_virtual_height-1);
		}
#endif
	  
		MACH_STATE_SET_ATI_CLIP_RECTANGLE(screen_state_p, 0, 0,
						screen_state_p->generic_state.screen_virtual_width,
						screen_state_p->generic_state.screen_virtual_height);
		
		ASSERT(!MACH_IS_IO_ERROR());
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
		MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_CLIP);
								/* mark deviation from SI's clip */
	}

	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_WRITE | 
		MACH_DP_CONFIG_ENABLE_DRAW | 
		MACH_DP_CONFIG_MONO_SRC_ONE |
		((graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		  SGFillSolidFG) ? 
		 MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR :
		 MACH_DP_CONFIG_FG_COLOR_SRC_BKGD_COLOR); 
		
#if (defined(__DEBUG__))
	if (mach_fillspans_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_fillspans_solid)\n"
"{\n"
"\tcount = %ld\n"
"\tpoints_p = %p\n"
"\twidths_p  = %p\n"
"\tdp_config = 0x%x\n"					  
"}\n",
					   count, (void *) points_p, (void *) widths_p,
					   dp_config);
	}
#endif

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);
	MACH_WAIT_FOR_FIFO(16);

	ASSERT(points_fence_p > points_p);
	
	do
	{
		register const int width = *widths_p++;
		if( width > 0)
		{
			MACH_WAIT_FOR_FIFO(3);
			outw(MACH_REGISTER_CUR_X, points_p->x);
			outw(MACH_REGISTER_CUR_Y, points_p->y);
			outw(MACH_REGISTER_SCAN_X, points_p->x + width);
								/* Draw Initiator */
		}
		
	} while (++points_p < points_fence_p);

	return (SI_SUCCEED);
}

STATIC SIBool
mach_fillspans_tile_system_memory(SIint32 count, SIPointP points_p, 
								  SIint32 *widths_p)
{

	unsigned short dp_config;
	SIbitmapP	si_tile_p;		/*Pointer to the Tile 					*/
	int		spanwidth;			/*The width of the current span line	*/
	short	source_x, source_y, destination_x, destination_y;
								/*The source and destination coordinates*/
	int		ppw;				/*Number of pixels in a pixtrans word	*/
	char	*source_pixels_p;	/*Pointer to the source.				*/
								/*Correctly speaking this has to be a 	*/
								/*ptr to the data type 'pixtrans width'	*/
	int		numwords;			/*Number of words in one transfer		*/
	int		x_span_end;			/*XCoord of right end of span line(incl)*/
	short	scan_x_value;		/*Value to put into SCAN_X register		*/

	struct mach_tile_state *tile_state_p;
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE,
							 graphics_state_p));

	ASSERT(!MACH_IS_IO_ERROR());
	
	if (count <= 0)
	{
		return (SI_SUCCEED);
	}
	
	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	tile_state_p = &(graphics_state_p->current_tile_state);
	
	/*
	 * Download and process the tile if necessary.
	 */
	if (tile_state_p->is_downloaded == FALSE)
	{
		mach_graphics_state_download_tile(
			screen_state_p,
			graphics_state_p,
			graphics_state_p->generic_state.si_graphics_state.SGtile);
	}
	
	ASSERT(IS_OBJECT_STAMPED(MACH_TILE_STATE, tile_state_p));

	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
#if (defined(__DEBUG__))
		if (mach_fillspans_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_fillspans_tile_system_memory) resetting clip rectangle {\n"
"\tfrom (%hd,%hd),(%hd,%hd)\n"
"\tto (%d,%d),(%d,%d)\n"
"}\n",
					   
				   screen_state_p->register_state.ext_scissor_l,
				   screen_state_p->register_state.ext_scissor_t,
				   screen_state_p->register_state.ext_scissor_r,
				   screen_state_p->register_state.ext_scissor_b,
				   0,
				   0,
				   screen_state_p->generic_state.screen_virtual_width-1,
				   screen_state_p->generic_state.screen_virtual_height-1);
		}
#endif
	  
		MACH_STATE_SET_ATI_CLIP_RECTANGLE(screen_state_p, 0, 0,
						screen_state_p->generic_state.screen_virtual_width,
						screen_state_p->generic_state.screen_virtual_height);
		
		ASSERT(!MACH_IS_IO_ERROR());
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;

		MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_CLIP);
								/* mark deviation from SI's clip */
	}

	/*
	 * Synchronize registers with the graphics state.
	 */
	MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
								 MACH_FILLSPANS_TILE_DEPENDENCIES);
	
	si_tile_p =
		graphics_state_p->generic_state.si_graphics_state.SGtile;

	ppw = screen_state_p->pixels_per_pixtrans;
	
	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_WRITE |
		MACH_DP_CONFIG_ENABLE_DRAW |
		MACH_DP_CONFIG_FG_COLOR_SRC_HOST;

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

#if (defined(__DEBUG__))
	if (mach_fillspans_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_fillspans_tile_system_memory)\n"
"{\n"
"\tcount = %ld\n"
"\tpoints_p = %p\n"
"\twidths_p = %p\n"
"\tdp_config = 0x%hx\n"
"}\n",
					   count, (void *) points_p,
					   (void *) widths_p, dp_config);
	}
#endif
	
	/*
	 * Do the tiling for each span.
	 */
	while (count--)
	{
		spanwidth = *widths_p++;

		/*
		 * if width is zero ignore
		 */
		if (spanwidth > 0)
		{
			destination_x = points_p->x;
			destination_y = points_p->y;
			x_span_end = destination_x + spanwidth - 1;

			/*
			 * Compute the X and Y coordinate in the source tile that 
			 * corresponds to these destination coordinates. source_x and
			 * source_y could be negative if BorgX has a value greater than 
			 * destination_x. In that case make source_x = |source_x|.
			 */
			source_x = (destination_x - si_tile_p->BorgX) % si_tile_p->Bwidth;
			source_y = (destination_y - si_tile_p->BorgY) % si_tile_p->Bheight;

			if ( source_x < 0 )
			{
				source_x += si_tile_p->Bwidth;
			}

			if ( source_y < 0 )
			{
				source_y += si_tile_p->Bheight;
			}

			/*
			 * Set the left scissor rectangle.
			 */
			MACH_WAIT_FOR_FIFO(1);
			outw(MACH_REGISTER_EXT_SCISSOR_L,	destination_x);

			/*
			 * Since reads of the source that are not aligned at a
			 * pixtrans boundary could cause a performance degradation
			 * adjust the source to the previous pixtrans word boundary.
			 */
			if (ppw)
			{
				int 			delta = 0;
				unsigned long	pixelmask = ppw -1;	

				/*
				 * check if the source origin is a pixtrans word boundary
				 */
				if (source_x & pixelmask) /* not a pixtrans word boundary */
				{
					/*
					 * Compute the number of pixels it is off from the
					 * previous word boundary and move source_x back to 
					 * previous pixtrans word boundary .
					 * adjust destination_x by the corresponding amount
					 */
					delta = source_x & pixelmask;
					source_x &= ~pixelmask; 
					destination_x -=  delta;  
				}
			}

			/*
			 * Do the first width outside the loop, since we are not
			 * guaranteed to start at offset 0 of the tile line.
			 */

			source_pixels_p = ((char *)si_tile_p->Bptr +
							   (tile_state_p->source_step * source_y));

			if (ppw)
			{
				source_pixels_p +=  
					((source_x >>
					  screen_state_p->pixels_per_pixtrans_shift)) <<
					  ((unsigned) screen_state_p->pixtrans_width >> 4U);
				
			}
			else
			{
				source_pixels_p += ((source_x * si_tile_p->BbitsPerPixel) >>
					screen_state_p->pixtrans_width_shift) <<
					((unsigned)screen_state_p->pixtrans_width >> 4U);
			}

			/*
			 * Number of words to transfer 
			 *
			 * case a:
			 * -----------------------------------
			 * |  tile line y |  tile line y  | ..     
			 * -----------------------------------
			 *   ^                              ^
			 *   |                              |
			 *   destination_x                  x_span_end
			 *
			 * case b:
			 * -----------------------------------
			 * |  tile line y |  tile line y | ...     
			 * -----------------------------------
			 *   ^       ^                  
			 *   |       |                 
			 *   destination_x   
			 *           |
			 *           x_span_end                   
			 *
			 */

			if ((destination_x +  (si_tile_p->Bwidth -source_x - 1))
				< x_span_end)
			{
				scan_x_value = destination_x + 
					(si_tile_p->Bwidth - source_x);

				/* 
				 * case a: More transfers to come for this span.
				 */
				if (ppw)
				{
					 numwords =
					 tile_state_p->number_of_pixtrans_words_per_tile_width - 
					 (source_x >> screen_state_p->pixels_per_pixtrans_shift);
				}
				else
				{
					numwords = 
					tile_state_p->number_of_pixtrans_words_per_tile_width - 
					((source_x * si_tile_p->BbitsPerPixel) >>
					 screen_state_p->pixels_per_pixtrans_shift);
				}
			}
			else
			{
				/* 
				 * This is the only transfer for this span.
				 * Here we know that destination_x is aligned at a pixtrans word
				 */
				scan_x_value = x_span_end + 1;
				numwords = x_span_end - destination_x + 1;
				if (ppw)
				{
					numwords = (numwords + ppw - 1) & ~(ppw-1); 
					numwords  = numwords >>
						screen_state_p->pixels_per_pixtrans_shift;
				}
				else
				{
					numwords = (numwords * si_tile_p->BbitsPerPixel) >>
						screen_state_p->pixels_per_pixtrans_shift;
				}
			}


			/*
			 * set the right clipping rectangle to x_span_end
			 */
			MACH_WAIT_FOR_FIFO(5);
			outw (MACH_REGISTER_EXT_SCISSOR_R,scan_x_value - 1);
			outw (MACH_REGISTER_CUR_X,destination_x);
			outw (MACH_REGISTER_CUR_Y,destination_y);
			outw (MACH_REGISTER_SCAN_X,scan_x_value); 
								/* right edge excl, draw initiatior */

			MACH_WAIT_FOR_FIFO(16);

			(*screen_state_p->screen_write_pixels_p)
				(screen_state_p->pixtrans_register,
				 numwords,
				 source_pixels_p);

			/*
			 * Set destination_x to the right value and also increment
			 * source_pixels_p to correspond to the beginning of the tile line.
			 */
			destination_x = scan_x_value;
			source_pixels_p = 
				((char *)si_tile_p->Bptr + 
				 (source_y * tile_state_p->source_step)); 

			/*
			 * Tile the section of the span that will take full tiles
			 */
			while((scan_x_value = destination_x + si_tile_p->Bwidth)
				  <= (x_span_end + 1))
			{
				MACH_WAIT_FOR_FIFO(5);
				outw (MACH_REGISTER_EXT_SCISSOR_R,scan_x_value-1);
				outw (MACH_REGISTER_CUR_X,destination_x);
				outw (MACH_REGISTER_CUR_Y,destination_y);
				outw (MACH_REGISTER_SCAN_X,scan_x_value);
								/*right edge excl, draw initiatior*/

				numwords = 
					tile_state_p->number_of_pixtrans_words_per_tile_width;

				MACH_WAIT_FOR_FIFO(16);

				(*screen_state_p->screen_write_pixels_p)
					(screen_state_p->pixtrans_register,
					 numwords,
					 source_pixels_p);
				destination_x = scan_x_value;
			}

			/*
			 * check if there is any leftover portion of the span to be done
			 */
			if (destination_x <= x_span_end)
			{
				scan_x_value = x_span_end + 1;

				numwords = x_span_end - destination_x + 1;
				if (ppw)
				{
					numwords = (numwords + (ppw-1)) & ~(ppw-1);
					numwords  = numwords >>
						screen_state_p->pixels_per_pixtrans_shift;
				}
				else
				{
					numwords = (numwords * si_tile_p->BbitsPerPixel) >>
						screen_state_p->pixtrans_width_shift;
				}
				MACH_WAIT_FOR_FIFO(5);
				outw (MACH_REGISTER_EXT_SCISSOR_R,scan_x_value-1);
				outw (MACH_REGISTER_CUR_X,destination_x);
				outw (MACH_REGISTER_CUR_Y,destination_y);
				outw (MACH_REGISTER_SCAN_X,scan_x_value); 
				MACH_WAIT_FOR_FIFO(16);
				(*screen_state_p->screen_write_pixels_p)
					(screen_state_p->pixtrans_register,
					 numwords,
					 source_pixels_p);
			}

		}
		points_p++;
	}

	/*
	 * restore the left and right ends of the clipping rectangle to
	 * full screen.
	 */

	MACH_WAIT_FOR_FIFO(2);
	outw(MACH_REGISTER_EXT_SCISSOR_L,
		 screen_state_p->register_state.ext_scissor_l);
	outw(MACH_REGISTER_EXT_SCISSOR_R,
		 screen_state_p->register_state.ext_scissor_r);

	/*
	 * The use of the pixtrans register has destroyed the pattern
	 * registers.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);
	
	ASSERT(!MACH_IS_IO_ERROR());
	
	return (SI_SUCCEED);
}


/*
 * mach_fillspans_tile_pattern_registers
 *
 * Fill a set of spans using the pattern registers.  This routine is
 * suitable for tiles with widths less than the total number of pixels
 * which would fit into the pattern registers.
 */
STATIC SIBool
mach_fillspans_tile_pattern_registers(SIint32 count, SIPointP points_p, 
									 SIint32 *widths_p)
{

	unsigned short dp_config;
	struct mach_tile_state *tile_state_p;
	SIbitmapP expanded_tile_p;
	int tile_width;
	int tile_height;
	int number_of_patt_data_registers_per_tile_width;
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();

	const int tile_x_origin =
		graphics_state_p->generic_state.si_graphics_state.SGtile->BorgX;
	const int tile_y_origin =
		graphics_state_p->generic_state.si_graphics_state.SGtile->BorgY;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE,
							 graphics_state_p));

	tile_state_p = &(graphics_state_p->current_tile_state);
	
	ASSERT(tile_state_p->is_small_tile == TRUE);

	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	/*
	 * download and process the tile if necessary
	 */
	
	if (tile_state_p->is_downloaded == FALSE)
	{
		mach_graphics_state_download_tile(
			screen_state_p,
			graphics_state_p,
			graphics_state_p->generic_state.si_graphics_state.SGtile);
	}

	ASSERT(IS_OBJECT_STAMPED(MACH_TILE_STATE, tile_state_p));
	
	expanded_tile_p = 
		graphics_state_p->current_tile_state.expanded_tile_p;

	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
#if (defined(__DEBUG__))
		if (mach_fillspans_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_fillspans_tile_pattern_registers) resetting clip rectangle {\n"
"\tfrom (%hd,%hd),(%hd,%hd)\n"
"\tto (%d,%d),(%d,%d)\n"
"}\n",
					   
				   screen_state_p->register_state.ext_scissor_l,
				   screen_state_p->register_state.ext_scissor_t,
				   screen_state_p->register_state.ext_scissor_r,
				   screen_state_p->register_state.ext_scissor_b,
				   0,
				   0,
				   screen_state_p->generic_state.screen_virtual_width-1,
				   screen_state_p->generic_state.screen_virtual_height-1);
		}
#endif
	  
		MACH_STATE_SET_ATI_CLIP_RECTANGLE(screen_state_p, 0, 0,
						screen_state_p->generic_state.screen_virtual_width,
						screen_state_p->generic_state.screen_virtual_height);
		
		ASSERT(!MACH_IS_IO_ERROR());
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
		MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_CLIP);
								/* mark deviation from SI's clip */
	}

	/*
	 * Synchronize registers with the graphics state.
	 */
	MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
								 MACH_FILLSPANS_TILE_DEPENDENCIES);
	
	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_FG_COLOR_SRC_PATT |
		MACH_DP_CONFIG_ENABLE_DRAW |
		MACH_DP_CONFIG_WRITE;
	
	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);
	
#if (defined(__DEBUG__))
	if (mach_fillspans_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_fillspans_tile_pattern_registers)\n"
"{\n"
"\tcount = %ld\n"
"\tpoints_p = %p\n"
"\twidths_p = %p\n"
"\tdp_config = 0x%x\n",

					   count, (void *) points_p, (void *) widths_p,
					   dp_config);
	}
#endif

	/*
	 * Program the tile length into the color pattern register.
	 */
	tile_width = expanded_tile_p->Bwidth;
	tile_height = expanded_tile_p->Bheight;

	MACH_WAIT_FOR_FIFO(1);
	outw(MACH_REGISTER_PATT_LENGTH, tile_width - 1);
	
	/*
	 * The pattern data registers are 16 bits wide.
	 */
	number_of_patt_data_registers_per_tile_width = 
		tile_state_p->number_of_patt_data_registers_per_tile_width;
			
	ASSERT(number_of_patt_data_registers_per_tile_width > 0 &&
		   number_of_patt_data_registers_per_tile_width <=
		   DEFAULT_MACH_NUMBER_OF_COLOR_PATTERN_DATA_REGISTERS);
	
	for(;count--; points_p++,widths_p++)
	{
		int spanwidth = *widths_p;
		
		if (spanwidth > 0)
		{
			int i;
			int destination_x, destination_y;
			int source_x, source_y;
			int x_span_end;
			unsigned char *tile_words_p;
			
			destination_x = points_p->x;
			destination_y = points_p->y;
			x_span_end = destination_x + spanwidth;
			/*
			 * Get the source coordinates corresponding to (x,y)
			 */
			if ((tile_width & (tile_width - 1)))
			{
				source_x = (destination_x - tile_x_origin) % tile_width;
			}
			else
			{
				/*
				 * power of two
				 */
				source_x =  (destination_x - tile_x_origin) &
					(tile_width - 1);
			}
			
			if (source_x < 0)
			{
				source_x += expanded_tile_p->Bwidth;
			}
			
			if ((tile_height & (tile_height - 1)))
			{
				source_y = (destination_y - tile_y_origin) % tile_height;
			}
			else
			{
				/*
				 * height is a power of two.
				 */
				source_y = (destination_y - tile_y_origin) &
					(tile_height - 1);
			}		
			
			if (source_y < 0)
			{
				source_y += expanded_tile_p->Bheight;
			}
			
			/*
			 * Point to the start of the correct row of the tile.
			 */
			tile_words_p = ((unsigned char *) expanded_tile_p->Bptr +
							(tile_state_p->expanded_source_step * source_y));
	
			/*
			 * Fill in the color pattern registers.
			 */
			MACH_WAIT_FOR_FIFO(1);
			outw(MACH_REGISTER_PATT_DATA_INDEX, 0);
			
			for(i = 0; i < number_of_patt_data_registers_per_tile_width;
				i++, tile_words_p +=2)
			{
				MACH_WAIT_FOR_FIFO(1);
				outw(MACH_REGISTER_PATT_DATA, 
					 *((unsigned short *) ((void *) tile_words_p)));
			}

			/*
			 * Tile the span.
			 */
			MACH_WAIT_FOR_FIFO(4);
			outw(MACH_REGISTER_PATT_INDEX, source_x);
			outw(MACH_REGISTER_CUR_X, destination_x);
			outw(MACH_REGISTER_CUR_Y, destination_y);
			outw(MACH_REGISTER_SCAN_X, x_span_end);
			
		}
	}

#if (defined(__DEBUG__))
	if (mach_fillspans_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif
	
	/*
	 * mark that the pattern registers have been trifled with.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);
	
	ASSERT(!MACH_IS_IO_ERROR());

	return (SI_SUCCEED);
}

/*
 * mach_fillspans_stipple_pattern_registers
 *
 * stipple a set of spans using the pattern registers.
 */
STATIC SIBool
mach_fillspans_stipple_pattern_registers(SIint32 count, SIPointP points_p, 
									 SIint32 *widths_p)
{
	unsigned short dp_config;
	struct mach_stipple_state *stipple_state_p;
	SIbitmapP inverted_stipple_p;
	int stipple_width;
	int stipple_height;
	int number_of_patt_data_registers_per_stipple_width;
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();
	
	/*
	 * CHIPSET BUG:
	 * 16 bit modes dont seem to use the second set of monopattern registers.
	 */
	int			number_of_monopattern_registers = 
				(screen_state_p->generic_state.screen_depth == 16 ?
				 DEFAULT_MACH_NUMBER_OF_MONOCHROME_PATTERN_DATA_REGISTERS - 1 :
				 DEFAULT_MACH_NUMBER_OF_MONOCHROME_PATTERN_DATA_REGISTERS);

	const int stipple_x_origin =
		graphics_state_p->generic_state.si_graphics_state.SGstipple->BorgX;
	const int stipple_y_origin =
		graphics_state_p->generic_state.si_graphics_state.SGstipple->BorgY;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE,
							 graphics_state_p));

	stipple_state_p = &(graphics_state_p->current_stipple_state);
	
	ASSERT(stipple_state_p->is_small_stipple == TRUE);
	
	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	/*
	 * download and process the stipple if necessary
	 */
	if (stipple_state_p->is_downloaded == FALSE)
	{
		mach_graphics_state_download_stipple(
			screen_state_p,
			graphics_state_p,
			graphics_state_p->generic_state.si_graphics_state.SGstipple);
	}
		
	ASSERT(IS_OBJECT_STAMPED(MACH_STIPPLE_STATE, stipple_state_p));
	
	inverted_stipple_p =
		&(stipple_state_p->inverted_stipple);

	if (screen_state_p->generic_state.screen_current_clip <
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
#if (defined(__DEBUG__))
		if (mach_fillspans_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_fillspans_stipple_pattern_registers) resetting clip rectangle {\n"
"\tfrom (%hd,%hd),(%hd,%hd)\n"
"\tto (%d,%d),(%d,%d)\n"
"}\n",
					   
				   screen_state_p->register_state.ext_scissor_l,
				   screen_state_p->register_state.ext_scissor_t,
				   screen_state_p->register_state.ext_scissor_r,
				   screen_state_p->register_state.ext_scissor_b,
				   0,
				   0,
				   screen_state_p->generic_state.screen_virtual_width-1,
				   screen_state_p->generic_state.screen_virtual_height-1);
		}
#endif
	  
		MACH_STATE_SET_ATI_CLIP_RECTANGLE(screen_state_p, 0, 0,
					screen_state_p->generic_state.screen_virtual_width,
					screen_state_p->generic_state.screen_virtual_height);
		
		ASSERT(!MACH_IS_IO_ERROR());
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
		MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_CLIP);
								/* mark deviation from SI's clip */
	}

	/*
	 * Synchronize registers with the graphics state.
	 */
	if (graphics_state_p->generic_state.si_graphics_state.SGstplmode 
		== SGStipple)
	{
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
							 MACH_FILLSPANS_STIPPLE_TRANSPARENT_DEPENDENCIES);
		MACH_STATE_SET_BG_ROP(screen_state_p,
							  MACH_MIX_FN_LEAVE_ALONE);
	}
	else						/* Opaque Stippling */
	{
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
							 MACH_FILLSPANS_STIPPLE_OPAQUE_DEPENDENCIES);
	}
	
	
	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR |
		MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		MACH_DP_CONFIG_ENABLE_DRAW |
		MACH_DP_CONFIG_MONO_SRC_PATT |
		MACH_DP_CONFIG_READ_MODE_MONO_DATA |
		MACH_DP_CONFIG_WRITE;
	
	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);
	
#if (defined(__DEBUG__))
	if (mach_fillspans_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_fillspans_stipple_pattern_registers)\n"
"{\n"
"\tcount = %ld\n"
"\tpoints_p = %p\n"
"\twidths_p = %p\n"
"\tdp_config = 0x%x\n",

					   count, (void *) points_p, (void *) widths_p,
					   dp_config);
	}
#endif

	/*
	 * Program the stipple length into the color pattern register.
	 */
	stipple_width = inverted_stipple_p->Bwidth;
	stipple_height = inverted_stipple_p->Bheight;

	MACH_WAIT_FOR_FIFO(1);
	outw(MACH_REGISTER_PATT_LENGTH, stipple_width - 1);
	
	/*
	 * The pattern data registers are 16 bits wide.
	 */
	number_of_patt_data_registers_per_stipple_width = 
		(stipple_state_p->number_of_pixtrans_words_per_stipple_width <<
		 screen_state_p->pixtrans_width_shift) >>
			 DEFAULT_MACH_PATTERN_REGISTER_WIDTH_SHIFT;

	ASSERT(number_of_patt_data_registers_per_stipple_width > 0 &&
		   number_of_patt_data_registers_per_stipple_width <=
#ifdef DELETE
		   DEFAULT_MACH_NUMBER_OF_MONOCHROME_PATTERN_DATA_REGISTERS);
#endif
		   number_of_monopattern_registers);
	
	for(;count--; points_p++,widths_p++)
	{
		int spanwidth = *widths_p;
		
		if (spanwidth > 0)
		{
			int i;
			int destination_x, destination_y;
			int source_x, source_y;
			int x_span_end;
			unsigned char *stipple_words_p;
			
			destination_x = points_p->x;
			destination_y = points_p->y;
			x_span_end = destination_x + spanwidth;

			/*
			 * Get the source coordinates corresponding to (x,y)
			 */

			if ((stipple_width & (stipple_width - 1)))
			{
				source_x = (destination_x - stipple_x_origin)
					% stipple_width;
			}
			else
			{
				/*
				 * power of two
				 */
				source_x =  (destination_x - stipple_x_origin) &
					(stipple_width - 1);
			}
			
			if (source_x < 0)
			{
				source_x += inverted_stipple_p->Bwidth;
			}
			
			if ((stipple_height & (stipple_height - 1)))
			{
				source_y = (destination_y - stipple_y_origin)
					% stipple_height;
			}
			else
			{
				/*
				 * height is a power of two.
				 */
				source_y = (destination_y - stipple_y_origin) &
					(stipple_height - 1);
			}		
			
			if (source_y < 0)
			{
				source_y += inverted_stipple_p->Bheight;
			}
			
			/*
			 * Point to the start of the correct row of the stipple.
			 */

			stipple_words_p = ((unsigned char *) inverted_stipple_p->Bptr +
							   (stipple_state_p->source_step * source_y));
			
			/*
			 * Fill in the color pattern registers.
			 */

			MACH_WAIT_FOR_FIFO(1);
			outw(MACH_REGISTER_PATT_DATA_INDEX, 
				 DEFAULT_MACH_NUMBER_OF_COLOR_PATTERN_DATA_REGISTERS);
			
			for(i = 0; i < number_of_patt_data_registers_per_stipple_width;
				i++, stipple_words_p +=2)
			{
				MACH_WAIT_FOR_FIFO(1);
				outw(MACH_REGISTER_PATT_DATA, 
					 *((unsigned short *) ((void *) stipple_words_p)));
			}

			/*
			 * Stipple the span.
			 */
			MACH_WAIT_FOR_FIFO(4);
			outw(MACH_REGISTER_PATT_INDEX, source_x);
			outw(MACH_REGISTER_CUR_X, destination_x);
			outw(MACH_REGISTER_CUR_Y, destination_y);
			outw(MACH_REGISTER_SCAN_X, x_span_end);
			
		}
	}

#if (defined(__DEBUG__))
	if (mach_fillspans_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif
	
	/*
	 * Since we have overwritten the pattern registers, mark these as
	 * invalid.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);

	return (SI_SUCCEED);
}


/*
 * mach_fillspans_stipple_system_memory
 *
 * Stipple a set of spans using a large stipple.  This routine is
 * meant to handle stipples of all sizes.
 */
STATIC SIBool
mach_fillspans_stipple_system_memory(SIint32 count, 
									 SIPointP points_p, SIint32 *widths_p)
{
	unsigned short dp_config;
	int pixtrans_width, pixtrans_width_shift;
	
	SIbitmapP	source_stipple_p;	/*Pointer to the Tile 					*/
	int		spanwidth;			/*The width of the current span line	*/
	short		source_x, source_y, destination_x, destination_y;
								/*The source and destination coordinates*/
	unsigned char	*source_bits_p;	/*Pointer to the source.				*/
									/*Correctly speaking this has to be a 	*/
									/*ptr to the data type 'pixtrans width'	*/
	int		numwords;			/*Number of words in one transfer		*/
	int		x_span_end;			/*XCoord of right end of span line(incl)*/
	short		scan_x_value;		/*Value to put into SCAN_X register		*/

	struct mach_stipple_state *stipple_state_p;
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();
	
	const int stipple_x_origin =
		graphics_state_p->generic_state.si_graphics_state.SGstipple->BorgX;
	const int stipple_y_origin =
		graphics_state_p->generic_state.si_graphics_state.SGstipple->BorgY;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE,
							 graphics_state_p));

	ASSERT(!MACH_IS_IO_ERROR());
	
	if (count <= 0)
	{
		return (SI_SUCCEED);
	}
	
	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	stipple_state_p = &(graphics_state_p->current_stipple_state);

	/*
	 * download and process the stipple if necessary
	 */
	if (stipple_state_p->is_downloaded == FALSE)
	{
		mach_graphics_state_download_stipple(
			screen_state_p,
			graphics_state_p,
			graphics_state_p->generic_state.si_graphics_state.SGstipple);
	}
	
	ASSERT(IS_OBJECT_STAMPED(MACH_STIPPLE_STATE, stipple_state_p));

	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
#if (defined(__DEBUG__))
		if (mach_fillspans_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_fillspans_stipple_system_memory) resetting clip rectangle {\n"
"\tfrom (%hd,%hd),(%hd,%hd)\n"
"\tto (%d,%d),(%d,%d)\n"
"}\n",
					   
				   screen_state_p->register_state.ext_scissor_l,
				   screen_state_p->register_state.ext_scissor_t,
				   screen_state_p->register_state.ext_scissor_r,
				   screen_state_p->register_state.ext_scissor_b,
				   0,
				   0,
				   screen_state_p->generic_state.screen_virtual_width-1,
				   screen_state_p->generic_state.screen_virtual_height-1);
		}
#endif
	  
		MACH_STATE_SET_ATI_CLIP_RECTANGLE(screen_state_p, 0, 0,
						screen_state_p->generic_state.screen_virtual_width,
						screen_state_p->generic_state.screen_virtual_height);
		
		ASSERT(!MACH_IS_IO_ERROR());
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;

		MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_CLIP);
								/* mark deviation from SI's clip */
	}

	/*
	 * Synchronize registers with the graphics state.
	 */
	if (graphics_state_p->generic_state.si_graphics_state.SGstplmode 
		== SGStipple)
	{
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
							 MACH_FILLSPANS_STIPPLE_TRANSPARENT_DEPENDENCIES);
		MACH_STATE_SET_BG_ROP(screen_state_p,
							  MACH_MIX_FN_LEAVE_ALONE);
	}
	else						/* Opaque Stippling */
	{
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
							 MACH_FILLSPANS_STIPPLE_OPAQUE_DEPENDENCIES);
	}
	
	
	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_WRITE |
		MACH_DP_CONFIG_ENABLE_DRAW |
		MACH_DP_CONFIG_READ_MODE_MONO_DATA |
		MACH_DP_CONFIG_MONO_SRC_HOST |
		MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR;

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);
	
	pixtrans_width = screen_state_p->pixtrans_width;
	pixtrans_width_shift = screen_state_p->pixtrans_width_shift;
	
	source_stipple_p = &(stipple_state_p->inverted_stipple);

#if (defined(__DEBUG__))
	if (mach_fillspans_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_fillspans_stipple_system_memory)\n"
"{\n"
"\tcount = %ld\n"
"\tpoints_p = %p\n"
"\twidths_p = %p\n"
"\tdp_config = 0x%hx\n"
"}\n",
					   count,
					   (void *) points_p, (void *) widths_p,
					   dp_config);
		
	}
#endif
	/*
	 * Do the stippling one span at a time.
	 */
	while (count--)
	{
		spanwidth = *widths_p++;
		/*
		 * if width is zero ignore
		 */
		if (spanwidth > 0)
		{
			destination_x = points_p->x;
			destination_y = points_p->y;
			x_span_end = destination_x + spanwidth - 1;

			/*
			 * Compute the X and Y coordinate in the source stipple that 
			 * corresponds to these destination coordinates. source_x and
			 * source_y could be negative if BorgX has a value greater than 
			 * destination_x. In that make source_x = |source_x| .
			 */
			source_x = (destination_x - stipple_x_origin) %
				source_stipple_p->Bwidth;

			source_y = (destination_y - stipple_y_origin) %
				source_stipple_p->Bheight;

			if ( source_x < 0 )
			{
				source_x += source_stipple_p->Bwidth;
			}
			if ( source_y < 0 )
			{
				source_y += source_stipple_p->Bheight;
			}

			/*
			 * Set the left scissor rectangle.
			 */
			MACH_WAIT_FOR_FIFO(1);
			outw(MACH_REGISTER_EXT_SCISSOR_L,	destination_x);

			/*
			 * Since reads of the source that are not aligned at a
			 * pixtrans boundary could cause a performance degradation
			 * adjust the source to the previous pixtrans word boundary.
			 * First check if the source origin is a pixtrans word boundary.
			 */
			if (source_x & (pixtrans_width - 1)) 
				/* not a pixtrans word boundary */
			{
				/*
				 * Compute the number of pixels it is off from the
				 * previous word boundary and move source_x back to 
				 * previous pixtrans word boundary .
				 * adjust destination_x by the corresponding amount
				 */
				destination_x -= source_x & ( pixtrans_width - 1 );
				source_x &= ~( pixtrans_width - 1 );
			}

			/*
			 * Do the first width outside the loop, since we are not
			 * guaranteed to start at offset 0 of the stipple line.
			 */

			source_bits_p = (((unsigned char *)source_stipple_p->Bptr) +
				(stipple_state_p->source_step * source_y));

			source_bits_p +=  (source_x >> pixtrans_width_shift) * 
				(pixtrans_width / 8);

			if ((destination_x + 
				(source_stipple_p->Bwidth - source_x - 1)) < x_span_end)
			{
				scan_x_value = destination_x +
					(source_stipple_p->Bwidth - source_x);
				numwords =
				stipple_state_p->number_of_pixtrans_words_per_stipple_width - 
					(source_x >> pixtrans_width_shift);
			}
			else
			{
				/* 
				 * This is the only transfer for this span.
				 * Here we know that destination_x is aligned at a
				 * pixtrans word 
				 */
				scan_x_value = x_span_end + 1;
				numwords = x_span_end - destination_x + 1;
				numwords = (numwords + pixtrans_width - 1) & 
										~(pixtrans_width-1); 
				numwords  = numwords >> pixtrans_width_shift;
			}

			/*
			 * set the right clipping rectangle to x_span_end
			 */
			MACH_WAIT_FOR_FIFO(5);
			outw (MACH_REGISTER_EXT_SCISSOR_R,scan_x_value-1);
			outw (MACH_REGISTER_CUR_X,destination_x);
			outw (MACH_REGISTER_CUR_Y,destination_y);
			outw (MACH_REGISTER_SCAN_X,scan_x_value); 
								/* right edge excl, draw initiatior */
			MACH_WAIT_FOR_FIFO(16);

			MACH_BULK_TRANSFER_TO_GRAPHICS_ENGINE(
				screen_state_p->pixtrans_register,
				numwords,
				source_bits_p, (pixtrans_width >> 4), 
				(*screen_state_p->screen_write_bits_p));
			
			/*
			 * Set destination_x to the right value and also increment
			 * source_bits_p to correspond to the beginning of the
			 * stipple line. 
			 */
			destination_x = scan_x_value;
			source_bits_p = (((unsigned char *)source_stipple_p->Bptr)
				+ (stipple_state_p->source_step * source_y));

			numwords =
				 stipple_state_p->number_of_pixtrans_words_per_stipple_width;

			/*
			 * Stipple the section of the span that will take full widths.
			 */
			while ( (scan_x_value = destination_x +
					 source_stipple_p->Bwidth) <= (x_span_end + 1))
			{
				MACH_WAIT_FOR_FIFO(5);
				outw (MACH_REGISTER_EXT_SCISSOR_R,scan_x_value-1);
				outw (MACH_REGISTER_CUR_X,destination_x);
				outw (MACH_REGISTER_CUR_Y,destination_y);
				outw (MACH_REGISTER_SCAN_X,scan_x_value);
								/*right edge excl, draw initiatior*/

				MACH_WAIT_FOR_FIFO(16);
				MACH_BULK_TRANSFER_TO_GRAPHICS_ENGINE(
					screen_state_p->pixtrans_register,
				    numwords,
    				source_bits_p, (pixtrans_width >> 4),
					(*screen_state_p->screen_write_bits_p));
				destination_x = scan_x_value;
			}

			/*
			 * check if there is any leftover portion in the span to be done
			 */
			if ( destination_x <= x_span_end )
			{
				scan_x_value = x_span_end + 1;

				numwords = x_span_end - destination_x + 1;
				numwords = (numwords + (pixtrans_width - 1)) & 
									~(pixtrans_width - 1);
				numwords  = numwords >> pixtrans_width_shift;

				MACH_WAIT_FOR_FIFO(5);
				outw (MACH_REGISTER_EXT_SCISSOR_R,scan_x_value-1);
				outw (MACH_REGISTER_CUR_X,destination_x);
				outw (MACH_REGISTER_CUR_Y,destination_y);
				outw (MACH_REGISTER_SCAN_X,scan_x_value); 
				MACH_WAIT_FOR_FIFO(16);

				MACH_BULK_TRANSFER_TO_GRAPHICS_ENGINE(
					screen_state_p->pixtrans_register,
				    numwords,
    				source_bits_p, (pixtrans_width >> 4),
                    (*screen_state_p->screen_write_bits_p));
			}
		}
		points_p++;
	}

	/*
	 * restore the left and right ends of the clipping rectangle
	 * to full screen
	 */
	MACH_WAIT_FOR_FIFO(2);
	outw(MACH_REGISTER_EXT_SCISSOR_L,
		 screen_state_p->register_state.ext_scissor_l);
	outw(MACH_REGISTER_EXT_SCISSOR_R, 
		 screen_state_p->register_state.ext_scissor_r);

	/*
	 * The blit has erased the pattern registers.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);
	
	ASSERT(!MACH_IS_IO_ERROR());
	
	return (SI_SUCCEED);
}

/*
 * Switch pointers to the correct fill spans routine at graphics state
 * change time.
 */
function void
mach_fillspans__gs_change__(void)
{
	
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();

	MACH_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE,
							 graphics_state_p));
	
	screen_state_p->generic_state.screen_functions_p->si_fillspans =
		graphics_state_p->generic_si_functions.si_fillspans;

	switch (graphics_state_p->generic_state.si_graphics_state.SGfillmode)
	{
	case SGFillSolidFG:
	case SGFillSolidBG:
		if (screen_state_p->options_p->spansfill_options &
			MACH_OPTIONS_SPANSFILL_OPTIONS_USE_SOLID_FILL)
		{
			screen_state_p->generic_state.screen_functions_p->
				si_fillspans = mach_fillspans_solid;
		}
		break;

	case SGFillTile:

		if (screen_state_p->options_p->spansfill_options &
			MACH_OPTIONS_SPANSFILL_OPTIONS_USE_TILE_FILL)
		{
			if ((screen_state_p->options_p->spansfill_options &
				 MACH_OPTIONS_SPANSFILL_OPTIONS_USE_PATTERN_REGISTERS)
				&& graphics_state_p->current_tile_state.is_small_tile)
			{
				screen_state_p->generic_state.screen_functions_p->
					si_fillspans = mach_fillspans_tile_pattern_registers;
			}
			else
			{
				screen_state_p->generic_state.screen_functions_p->
					si_fillspans = mach_fillspans_tile_system_memory;
			}
		}
		break;

	case SGFillStipple:
		if (screen_state_p->options_p->spansfill_options &
			  MACH_OPTIONS_SPANSFILL_OPTIONS_USE_STIPPLE_FILL)
		{
			if ((screen_state_p->options_p->spansfill_options &
				MACH_OPTIONS_SPANSFILL_OPTIONS_USE_PATTERN_REGISTERS) && 
				graphics_state_p->current_stipple_state.is_small_stipple)
			{
				screen_state_p->generic_state.screen_functions_p->
					si_fillspans = mach_fillspans_stipple_pattern_registers;
			}
			else
			{
				screen_state_p->generic_state.screen_functions_p->
					si_fillspans = mach_fillspans_stipple_system_memory;
			}
		}
		break;

	default:
		/*CONSTANTCONDITION*/
		ASSERT(0);
		break;
	}	
}

function void
mach_fillspans__initialize__(SIScreenRec *si_screen_p,
							 struct mach_options_structure *
							 options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	
	flags_p->SIavail_spans = 0;
	functions_p->si_fillspans = mach_no_operation_fail;

	if (options_p->spansfill_options &
		MACH_OPTIONS_SPANSFILL_OPTIONS_USE_SOLID_FILL)
	{
		flags_p->SIavail_spans |= SPANS_AVAIL;
	}

	if (options_p->spansfill_options &
		MACH_OPTIONS_SPANSFILL_OPTIONS_USE_STIPPLE_FILL)
	{
		flags_p->SIavail_spans |= (STIPPLE_AVAIL | OPQSTIPPLE_AVAIL |
								   SPANS_AVAIL);
	}

	if (options_p->spansfill_options &
		MACH_OPTIONS_SPANSFILL_OPTIONS_USE_TILE_FILL)
	{
		flags_p->SIavail_spans |= (TILE_AVAIL | SPANS_AVAIL);
	}
	
}
