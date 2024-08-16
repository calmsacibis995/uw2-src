/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_tile.c	1.3"
/***
 ***	NAME
 ***
 ***	SYNOPSIS
 ***
 ***	DESCRIPTION
 ***
 *** 	Tiling can be done from system memory, offscreen memory and if the
 ***	tile is a two color one, using stippling. In general all two
 ***	color tiles are converted to stipples (controlled by  an
 ***	option)  and the stippling module is entrusted with the work
 ***	of downloading, filling.
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

#include "stdenv.h"
#include "sidep.h"
#include "p9k_opt.h"

/***
 ***	Constants.
 ***/

#define	P9000_TILE_STATE_STAMP												\
	(('P' << 0) + ('9' << 1) + ('0' << 2) + ('0' << 3) + ('0' << 4) +		\
	 ('_' << 5) + ('T' << 6) + ('I' << 7) + ('L' << 8) + ('E' << 9) +		\
	 ('_' << 10) + ('S' << 11) + ('T' << 12) + ('A' << 13) + ('T' << 14) +	\
	 ('E' << 15) + 0 )

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
export enum debug_level p9000_tile_debug = DEBUG_LEVEL_NONE;
#endif

/*
 *	Current module state.
 */


PRIVATE

/***
 ***	Includes.
 ***/

#include "p9k_gs.h"
#include "g_omm.h"
#include "p9k_state.h"
#include "p9k_stpl.h"
#include "p9k_regs.h"
#include "p9k_gbls.h"

/***
 ***	Private declarations.
 ***/

struct p9000_tile_state
{

	/*
	 * All two color tiles are handled as stipples
	 */
	struct p9000_stipple_state stipple_state;

	boolean is_two_color_tile;

	unsigned char color1;
	unsigned char color2;

	SIbitmap si_stipple;

	SIbitmapP si_tile_p;

	/*
	 * Pointer to offscreen area descriptor structure
	 */

	struct omm_allocation *offscreen_allocation_p;

	/*
	 * Dimensions of the tile in the offscreen area
	 */

	int offscreen_width;
	int offscreen_height;

	/*
	 * The draw function for this tile.
	 */

	SIBool (*tile_function_p)
		(struct p9000_tile_state *tile_state_p,
		int count, SIRectOutlineP rect_p);

#if (defined(__DEBUG__))
	int stamp;
#endif

};

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/


/*
 * @doc:p9000_tile_fill_rectangle_system_memory:
 *
 * @enddoc
 */

STATIC SIBool
p9000_tile_fill_rectangle_system_memory(
	struct p9000_tile_state *tile_state_p,
	int count,				
	SIRectOutlineP rect_p)		
{
	int tile_width;	
	int tile_height;
	int source_step;
	unsigned int raster;
	unsigned char *source_bits_p;
	SIbitmapP tile_p = tile_state_p->si_tile_p;
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
							 graphics_state_p));

#if defined(__DEBUG__)
	if (DEBUG_LEVEL_MATCH(p9000_tile,SCAFFOLDING))
	{
		(void)fprintf(debug_stream_p,
			"(p9000_tile_fill_rectangle_system_memory){\n"
			"\t count = %d\n"
			"\t rect_p = %p\n"
			"\t tile_state_p = %p\n"
			"}\n",
			count,(void*)rect_p,
			 (void*)tile_state_p);
	}
#endif


	tile_width = tile_p->Bwidth;
	tile_height = tile_p->Bheight;
	source_step = (tile_width + 3) & ~3;

	/*
	 * The source for the drawing operation is BLIT
	 */

	raster = 
		P9000_STATE_CALCULATE_BLIT_MINTERM(screen_state_p,graphics_state_p);
	
	P9000_STATE_SET_RASTER(register_state_p,raster);

	/*
	 * Synchronize to SI specified values.
	 */

	P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
									  (P9000_STATE_CHANGE_PLANEMASK));

	P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
									  (P9000_STATE_CHANGE_PLANEMASK));

	for (;count--;rect_p++)
	{
		int x_offset;
		int y_offset;
		int start_width = 0;
		int remaining_width = 0;
		int full_tiles_count = 0;
		register int x_left = rect_p->x;			
		register int y_top  = rect_p->y;		
		register int x_right = x_left + rect_p->width;
		register int y_bottom = y_top + rect_p->height;

		if ((rect_p->width <= 0) || (rect_p->height <= 0))
		{
			continue;
		}

#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_tile,INTERNAL))
		{
			(void) fprintf(debug_stream_p,
						   "\t{\n"
						   "\t\tleft_x = %d\n"
						   "\t\tright_x = %d\n"
						   "\t\ttop_y = %d\n"
						   "\t\twidth = %d\n"
						   "\t\theight = %d\n"
						   "\t}\n",
						   x_left, x_right, y_top,
						   rect_p->width,rect_p->height);
		}
#endif
		
		/*
		 * If the rectangle outside the clipping bounds, ignore it.
		 */

		/*JK: keeping the clip in local variables is better (avoids
		  an pointer fetch every time).  Compiler is dumb ... 
		  */

		if((x_left > screen_state_p->generic_state.screen_clip_right) ||
		   (x_right < screen_state_p->generic_state.screen_clip_left) ||
		   (y_top > screen_state_p->generic_state.screen_clip_bottom) ||
		   (y_bottom < screen_state_p->generic_state.screen_clip_top) ||
		   (y_bottom < y_top) || ( x_right < x_left ))
		{
			continue;
		}

		/*
		 * Clip to the nearest clip boundary.
		 */
		 
		if (x_left <
			screen_state_p->generic_state.screen_clip_left)
		{
			x_left =
				screen_state_p->generic_state.screen_clip_left;
		}
		if (x_right >
			screen_state_p->generic_state.screen_clip_right)
		{
			x_right =
				screen_state_p->generic_state.screen_clip_right + 1;
		}
		if (y_top <
			screen_state_p->generic_state.screen_clip_top)
		{
			y_top =
				screen_state_p->generic_state.screen_clip_top;
		}

		if (y_bottom >
			screen_state_p->generic_state.screen_clip_bottom)
		{
			y_bottom =
				screen_state_p->generic_state.screen_clip_bottom + 1;
		}

		P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();

		P9000_REGISTER_SET_CLIPPING_RECTANGLE(
		  x_left, y_top, (x_right - 1), (y_bottom - 1));
		
		x_offset = (x_left - tile_p->BorgX) % tile_width;
		y_offset = (y_top - tile_p->BorgY) % tile_height;

		remaining_width =  rect_p->width;
	
		if (x_offset < 0)
		{
			x_offset += tile_width;
		}

		if (y_offset < 0)
		{
			y_offset += tile_height;
		}
	
		if (x_offset > 0)
		{
			int tmp_y_offset = y_offset;
			int height = rect_p->height;

			remaining_width -= (tile_width - x_offset);

			/*
			 *Move back to long word boundary
			 */
			
			if (x_offset & 3)
			{
				x_left -= (x_offset & 3);
				x_offset &= ~3;
			}
			
			start_width = tile_width - x_offset;
			
			
			/*
			 * Setup for pixel8 command:
			 * x0: Left edge of the block to be transferred
			 * x1,y1: Point at which to begin transfer
			 * x2: Right edge of the block to be transferred
			 * y3:  Y increment per after every scanline
			 * 
			 */
			
			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_0,
				P9000_PARAMETER_COORDINATE_X_32,
				P9000_PARAMETER_COORDINATE_REL,
				x_left);
				

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_1,
				P9000_PARAMETER_COORDINATE_XY_16,
				P9000_PARAMETER_COORDINATE_REL,
				P9000_PACK_XY(x_left,y_top));
			 

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_2,
				P9000_PARAMETER_COORDINATE_X_32,
				P9000_PARAMETER_COORDINATE_REL,
				x_left + start_width);

			P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
				P9000_PARAMETER_COORDINATE_REG_3,
				P9000_PARAMETER_COORDINATE_Y_32,
				P9000_PARAMETER_COORDINATE_REL,1);

			x_left += start_width;
			
			start_width = (start_width + 3) & ~3; 

			P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();
			
			source_bits_p =
				((unsigned char*)tile_p->Bptr) +
					(source_step * tmp_y_offset) + x_offset; 
			do
			{
				int count = start_width >> 2;
				unsigned long *data_p = (unsigned long*)source_bits_p;
				
				do	
				{
					
					P9000_PIXEL8_COMMAND(
						*data_p++,
						(P9000_ADDRESS_SWAP_HALF_WORDS|
						 P9000_ADDRESS_SWAP_BYTES));

				}while(--count > 0);
			
				++tmp_y_offset;
				tmp_y_offset %= tile_height;

				source_bits_p =
					((unsigned char*)tile_p->Bptr) +
						(source_step * tmp_y_offset) + x_offset; 

			}while(--height > 0);
		}

		/*
		 * Calculate number of full tiles
		 */

		full_tiles_count =  (remaining_width / tile_width);

		if (remaining_width % tile_width)
		{
			++full_tiles_count;
		}
			
		if (full_tiles_count > 0)
		{

			do
			{

				int tmp_y_offset = y_offset;
				int height = rect_p->height;

				int rounded_tile_width = 
					(remaining_width > tile_width) ?
						tile_width : remaining_width;
						
				remaining_width -= tile_width;
					

				P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
					P9000_PARAMETER_COORDINATE_REG_0,
					P9000_PARAMETER_COORDINATE_X_32,
					P9000_PARAMETER_COORDINATE_REL,
					x_left);
						

				P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
					P9000_PARAMETER_COORDINATE_REG_1,
					P9000_PARAMETER_COORDINATE_XY_16,
					P9000_PARAMETER_COORDINATE_REL,
					P9000_PACK_XY(x_left,y_top));
					 

				P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
					P9000_PARAMETER_COORDINATE_REG_2,
					P9000_PARAMETER_COORDINATE_X_32,
					P9000_PARAMETER_COORDINATE_REL,
					x_left + rounded_tile_width);

				P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
					P9000_PARAMETER_COORDINATE_REG_3,
					P9000_PARAMETER_COORDINATE_Y_32,
					P9000_PARAMETER_COORDINATE_REL,1);

				source_bits_p =
					((unsigned char*)tile_p->Bptr) +
						(source_step * tmp_y_offset); 

				rounded_tile_width = (rounded_tile_width + 3) &  ~3;

				do
				{
					int count = rounded_tile_width >> 2;
					unsigned long *data_p = (unsigned long*)source_bits_p;
					
					do
					{
						P9000_PIXEL8_COMMAND(
							*data_p++,
							(P9000_ADDRESS_SWAP_HALF_WORDS|
							 P9000_ADDRESS_SWAP_BYTES));

					} while (--count > 0);

					++tmp_y_offset;

					tmp_y_offset %= tile_height;

					source_bits_p =
						((unsigned char*)tile_p->Bptr) +
							(source_step * tmp_y_offset); 

				} while (--height);

				x_left += tile_width;

			} while (--full_tiles_count > 0);
				
			ASSERT(remaining_width <= 0);

		}
	}
	
	/*
	 * We have changed the clipping rectangle, mark it as invalid
	 */

	P9000_STATE_INVALIDATE_CLIP_RECTANGLE(screen_state_p);

	return SI_SUCCEED;
	
}



#define BLIT_ONCE(Y, Y_OFFSET, HEIGHT, ALLOCATION)\
{\
	unsigned int __status;\
	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(\
		P9000_PARAMETER_COORDINATE_REG_0,\
		P9000_PARAMETER_COORDINATE_Y_32,\
		P9000_PARAMETER_COORDINATE_REL,\
		((ALLOCATION)->y + (Y_OFFSET)));\
	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(\
		P9000_PARAMETER_COORDINATE_REG_1,\
		P9000_PARAMETER_COORDINATE_Y_32,\
		P9000_PARAMETER_COORDINATE_REL,\
		((ALLOCATION)->y + (Y_OFFSET) + (HEIGHT) - 1));\
	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(\
		P9000_PARAMETER_COORDINATE_REG_2,\
		P9000_PARAMETER_COORDINATE_Y_32,\
		P9000_PARAMETER_COORDINATE_REL,\
		(Y));\
	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(\
		P9000_PARAMETER_COORDINATE_REG_3,\
		P9000_PARAMETER_COORDINATE_Y_32,\
		P9000_PARAMETER_COORDINATE_REL,\
		((Y) + (HEIGHT) - 1));\
	do\
	{\
		__status = P9000_INITIATE_BLIT_COMMAND();\
		ASSERT(!(__status & P9000_STATUS_BLIT_SOFTWARE));\
	} while (__status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);\
}


/*
 * @doc:p9000_tile_fill_rectangle_offscreen_helper:
 *
 * Helper function to blit tiles from offscreen memory
 * 
 * @enddoc
 */

STATIC void
p9000_tile_fill_rectangle_offscreen_helper(
	struct omm_allocation *allocation_p, /* the allocation */
	int offscreen_width,		/* offscreen dimensions */
	int offscreen_height,
	SIbitmapP si_bitmap_p,		/* tile/stipple being used */
	int x_left,					/* inclusive destination rectangle */
	int y_top,					/* coordinates */
	int x_right,				/* exclusive L-R rectangle coordinates */
	int y_bottom)				/* -do- */
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();

	/* 
	 * width and height of the unexpanded tile 
	 */

	int tile_width = si_bitmap_p->Bwidth; 
	int tile_height = si_bitmap_p->Bheight;
	
	int tile_start_x_offset;	/* offset of first pixel to be blitted */
	int tile_start_y_offset;	/* offset of first line to be blitted */
	int rect_start_x_pixels;
	int rect_start_y_lines;
	int rect_offscreen_widths;	/* number of whole widths */
	int rect_offscreen_heights;	/* number of whole heights */
	int rect_end_x_pixels;			/* number of ending pixels */
	int rect_end_y_lines;			/* number of ending lines */
	int rect_width;
	int rect_height;
	int tile_x_origin;			/* BorgX of the tile/stipple */
	int tile_y_origin;			/* BorgY of the tile/stipple */
	
	
	ASSERT(IS_OBJECT_STAMPED(OMM_ALLOCATION, allocation_p));
	ASSERT(offscreen_width >= 0 && offscreen_height >= 0);

	ASSERT(x_left <= x_right);
	ASSERT(y_top <= y_bottom);

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_tile,SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
			"(p9000_fill_rectangle_offscreen_helper){\n"
			"\tallocation_p = %p\n"
			"\tallocation_x = %d\n"
			"\tallocation_y = %d\n"
			"\tallocation_planemask = 0x%x\n"
			"\toffscreen_width = %d\n"
			"\toffscreen_height = %d\n"
			"\ttile_width = %d\n"
			"\ttile_height = %d\n"
			"\tx_left = %d\n"
			"\ty_top = %d\n"
			"\tx_right = %d\n"
			"\ty_bottom = %d\n"
			"}\n",
		   (void *) allocation_p,
		   allocation_p->x,
		   allocation_p->y,
		   allocation_p->planemask,
		   offscreen_width,
		   offscreen_height,
		   tile_width,
		   tile_height,
		   x_left,
		   y_top,
		   x_right,
		   y_bottom);
	}
#endif

	rect_width = x_right - x_left;
	rect_height = y_bottom - y_top;

	/*
	 * Get the pixel corresponding to (xl, yt) in the offscreen area.
	 */

	/*
	 * Compute the x origin of the tile in the range 0..tile_width.
	 */
		
	if ((tile_x_origin = (si_bitmap_p->BorgX % tile_width)) < 0)
	{
		tile_x_origin += tile_width;
	}
		
	/*
	 * Compute the y origin of the tile in the range 0..tile_height.
	 */
		
	if ((tile_y_origin = (si_bitmap_p->BorgY % tile_height)) < 0)
	{
		tile_y_origin += tile_height;
	}
		
	/*
	 * compute the place to start tiling from offscreen cache.
	 */

	if ((tile_start_x_offset = (x_left - tile_x_origin)) < 0)
	{
		tile_start_x_offset += tile_width;
	}
	

	if (tile_width & (tile_width - 1)) /* not a power of two */
	{

		tile_start_x_offset %= tile_width;

	}
	else						/* power of two */
	{

		tile_start_x_offset &= (tile_width - 1);

	}
	
   	ASSERT(tile_start_x_offset >= 0);
	
	
	if ((tile_start_y_offset = (y_top - tile_y_origin)) < 0)
	{
		tile_start_y_offset += tile_height;
	}
	
	if (tile_height & (tile_height - 1)) /* not a power of two */
	{

		tile_start_y_offset %= tile_height;

	}
	else						/* a power of two */
	{

		tile_start_y_offset &= (tile_height - 1);

	}
	
	/*
	 * Can we accomodate the whole rectangle blit from the offscreen
	 * area?
	 */

	if (tile_start_x_offset + rect_width <= offscreen_width)
	{

		rect_start_x_pixels = rect_width;
		rect_end_x_pixels = 0;
		rect_offscreen_widths = 0;

	}
	else
	{
		
		rect_start_x_pixels = offscreen_width - tile_start_x_offset;
		rect_end_x_pixels = 
			(rect_width - rect_start_x_pixels) % offscreen_width;
		
		ASSERT(((rect_width - rect_start_x_pixels - rect_end_x_pixels)
				% offscreen_width) == 0);

		rect_offscreen_widths = 
			(rect_width - rect_start_x_pixels - rect_end_x_pixels) /
			offscreen_width;
		
	}
	
	/*
	 * Compute the corresponding parameters for the offscreen height.
	 */

	if (tile_start_y_offset + rect_height <= offscreen_height)
	{

		rect_start_y_lines = rect_height;
		rect_end_y_lines = 0;
		rect_offscreen_heights = 0;

	}
	else
	{
		rect_start_y_lines = offscreen_height - tile_start_y_offset;
		rect_end_y_lines = 
			(rect_height - rect_start_y_lines) % offscreen_height;
		
		ASSERT(((rect_height - rect_start_y_lines - rect_end_y_lines)
				% offscreen_height) == 0);

		rect_offscreen_heights = 
			(rect_height - rect_start_y_lines - rect_end_y_lines) /
			offscreen_height;
		
	}

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_tile,INTERNAL))
	{
		(void) fprintf(debug_stream_p,
			"{\n"
			"\t# Intermediate parameters\n"
			"\ttile_start_x_offset = %d\n"
			"\ttile_start_y_offset = %d\n"
			"\trect_start_x_pixels = %d\n"
			"\trect_end_x_pixels = %d\n"
			"\trect_offscreen_widths = %d\n"
			"\trect_start_y_lines = %d\n"
			"\trect_end_y_lines = %d\n"
			"\trect_offscreen_heights = %d\n"
			"}\n",
		   tile_start_x_offset,
		   tile_start_y_offset,
		   rect_start_x_pixels,
		   rect_end_x_pixels,
		   rect_offscreen_widths,
		   rect_start_y_lines,
		   rect_end_y_lines,
		   rect_offscreen_heights);
	}
#endif
	
	/*
	 * Time to start the blit.
	 */

	if (rect_start_x_pixels)
	{
		int current_y = y_top;
		int tmp_heights = rect_offscreen_heights;
		
		
		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_0,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			(allocation_p->x + tile_start_x_offset));

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_1,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			(allocation_p->x + tile_start_x_offset +
			 rect_start_x_pixels) - 1);
		
		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_2,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			x_left);

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_3,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			(x_left + rect_start_x_pixels) - 1);

		if (rect_start_y_lines)
		{
			BLIT_ONCE(current_y,
				tile_start_y_offset, rect_start_y_lines,allocation_p);

			current_y += rect_start_y_lines;
		}

		while (tmp_heights--)
		{
			BLIT_ONCE(current_y, 0,
					  offscreen_height, allocation_p);
			current_y += offscreen_height;
		}
		if (rect_end_y_lines)
		{
			BLIT_ONCE(current_y, 0,
					  rect_end_y_lines, allocation_p);
		}

		x_left += rect_start_x_pixels;

		ASSERT(current_y + rect_end_y_lines == y_bottom);
	}
	
	while(rect_offscreen_widths --)
	{
		int current_y = y_top;
		int tmp_heights = rect_offscreen_heights;
		
		
		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_0,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			(allocation_p->x));

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_1,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			(allocation_p->x + offscreen_width) - 1);
		
		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_2,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			x_left);

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_3,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			(x_left + offscreen_width) - 1);

		if (rect_start_y_lines)
		{
			BLIT_ONCE(current_y,
					  tile_start_y_offset, rect_start_y_lines,
					  allocation_p);
			current_y += rect_start_y_lines;
		}
		while (tmp_heights--)
		{
			BLIT_ONCE(current_y, 0,
					  offscreen_height, allocation_p);
			current_y += offscreen_height;
		}
		if (rect_end_y_lines)
		{
			BLIT_ONCE(current_y, 0,
					  rect_end_y_lines, allocation_p);
		}

		ASSERT(current_y + rect_end_y_lines == y_bottom);

		x_left += offscreen_width;

	}
	
	if (rect_end_x_pixels)
	{
		int current_y = y_top;
		int tmp_heights = rect_offscreen_heights;
		
		
		
		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_0,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			(allocation_p->x));

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_1,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			(allocation_p->x  + rect_end_x_pixels) - 1);
		
		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_2,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			x_left);

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_3,
			P9000_PARAMETER_COORDINATE_X_32,
			P9000_PARAMETER_COORDINATE_REL,
			(x_left + rect_end_x_pixels) - 1);

		if (rect_start_y_lines)
		{
			BLIT_ONCE(current_y,
					  tile_start_y_offset, rect_start_y_lines,
					  allocation_p);
			current_y += rect_start_y_lines;
		}
		while (tmp_heights--)
		{
			BLIT_ONCE(current_y, 0,
					  offscreen_height, allocation_p);
			current_y += offscreen_height;
		}
		if (rect_end_y_lines)
		{
			BLIT_ONCE(current_y, 0,
					  rect_end_y_lines, allocation_p);
		}

		ASSERT(current_y + rect_end_y_lines == y_bottom);
		
	}

	ASSERT(x_left + rect_end_x_pixels == x_right);
}


/*
 * @doc:p9000_tile_fill_rectangle_offscreen_memory:
 * 
 * @endoc
 */

STATIC SIBool
p9000_tile_fill_rectangle_offscreen_memory(
	struct p9000_tile_state *tile_state_p,
	int count,				
	SIRectOutlineP rect_p)	
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	unsigned int raster;
	SIbitmapP tile_p = tile_state_p->si_tile_p;
	struct omm_allocation *allocation_p;

	/*
	 * Clip coordinates
	 */

	const int screen_clip_left =
		screen_state_p->generic_state.screen_clip_left;
	const int screen_clip_right =
		screen_state_p->generic_state.screen_clip_right;
	const int screen_clip_top =
		screen_state_p->generic_state.screen_clip_top;
	const int screen_clip_bottom =
		screen_state_p->generic_state.screen_clip_bottom;

	const int tile_width =		/* actual width of the tile */
		tile_p->Bwidth;

	const int tile_height =		/* actual height of the tile */
		tile_p->Bheight;
	
	int allocation_width;
	
	int allocation_height;

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
							 graphics_state_p));

	
	ASSERT(IS_OBJECT_STAMPED(P9000_TILE_STATE,
							 tile_state_p));
	
	ASSERT(tile_width > 0);
	ASSERT(tile_height > 0);
	
	/*
	 * Examine the current offscreen allocation.
	 */

	allocation_p =
		tile_state_p->offscreen_allocation_p;
	
	ASSERT(!allocation_p ||
		   IS_OBJECT_STAMPED(OMM_ALLOCATION, allocation_p));

	
	allocation_width =
		tile_state_p->offscreen_width;
	
	allocation_height =
		tile_state_p->offscreen_height;
	
	ASSERT(allocation_width > 0);
	ASSERT(allocation_height > 0);

	/*
	 * The source for the drawing operation is BLIT
	 */

	raster = 
		P9000_STATE_CALCULATE_BLIT_MINTERM(screen_state_p,graphics_state_p);
	
	P9000_STATE_SET_RASTER(register_state_p,raster);

	/*
	 * Synchronize the registers to the SI specified values.
	 */

	P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
									  (P9000_STATE_CHANGE_PLANEMASK));

	P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
									  (P9000_STATE_CHANGE_PLANEMASK));

    for(; count--; rect_p++)
	{
		int xl = rect_p->x;
		int xr = xl + rect_p->width;
		int yt = rect_p->y;
		int yb = yt + rect_p->height;
		
		
		if ((xl > screen_clip_right) ||
			(xr <= screen_clip_left) ||
			(yt > screen_clip_bottom) ||
			(yb <= screen_clip_top) ||
			(yb <= yt) || (xr <= xl ))
		{
			continue;
		}

		/*
		 * Software clip the destination rectangle to clipping rectangle
		 * bounds. 
		 */

		if (xl < screen_clip_left)
		{
			xl = screen_clip_left;
		}

		if (xr > screen_clip_right)
		{
			xr = screen_clip_right + 1;
		}

		if (yt < screen_clip_top)
		{
			yt = screen_clip_top;
		}

		if (yb > screen_clip_bottom)
		{
			yb = screen_clip_bottom + 1;
		}

		/*
		 * Call the offscreen helper to tile the rectangle.
		 */

		p9000_tile_fill_rectangle_offscreen_helper(
		    tile_state_p->offscreen_allocation_p,
			tile_state_p->offscreen_width,
			tile_state_p->offscreen_height,
			tile_p,
			xl, yt, xr, yb);											 

	}

	return (SI_SUCCEED);
}


/*
 * @doc:p9000_tile_fill_rectangle_two_color_tile:
 *
 * @enddoc
 */

STATIC SIBool
p9000_tile_fill_rectangle_two_color_tile(
	struct p9000_tile_state *tile_state_p,
	int count,				
	SIRectOutlineP rect_p)	
{
	unsigned int raster;
	SIbitmapP tile_p = tile_state_p->si_tile_p;
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();

	/*JK: asserts, scaffolding etc please*/

	raster =
		(P9000_SOURCE_MASK &
		 P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,
										  graphics_state_p)) |
		(~P9000_SOURCE_MASK &
		 P9000_STATE_CALCULATE_BG_MINTERM(screen_state_p,
										  graphics_state_p)) |
		P9000_RASTER_USE_PATTERN;

	P9000_STATE_SET_RASTER(register_state_p, raster);

	/*
	 * Synchronize the registers to the SI specified values.
	 */

	P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
		 (P9000_STATE_CHANGE_PLANEMASK));

	P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
									   (P9000_STATE_CHANGE_PLANEMASK));

	/*
	 * Program color registers
	 */

	if (register_state_p->fground != tile_state_p->color1 ||
		register_state_p->bground != tile_state_p->color2)
	{
		
		P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();
		
		P9000_REGISTER_SET_FOREGROUND_COLOR(tile_state_p->color1);
		P9000_REGISTER_SET_BACKGROUND_COLOR(tile_state_p->color2);

		register_state_p->fground = tile_state_p->color1;
		register_state_p->bground = tile_state_p->color2;

		/*
		 * Mark that we have deviated from SI specified fg and bg.
		 */

		screen_state_p->changed_state.flags |= 
			(P9000_STATE_CHANGE_FOREGROUND_COLOR |
			 P9000_STATE_CHANGE_BACKGROUND_COLOR);
	}
	

	/*
	 * BorgX and BorgY could have changed, reassign
	 */

	tile_state_p->si_stipple.BorgX =
		tile_p->BorgX;

	tile_state_p->si_stipple.BorgY =
		tile_p->BorgY;

	/*
	 * Call the the stipple helper.
	 */

	tile_state_p->stipple_state.stipple_function_p(
		&(tile_state_p->stipple_state),
		count, rect_p);

	return (SI_SUCCEED);
}


/*
 * @doc:p9000_tile_offscreen_download_helper:
 *
 * @enddoc
 */

STATIC void
p9000_tile_offscreen_download_helper(struct p9000_tile_state *tile_state_p)
{
	int status;
	int height;
	int tile_width;
	int tile_height;
	int source_step;
	int start_x, start_y;
	int offscreen_x_location;
	int offscreen_y_location;
	SIbitmapP tile_p = NULL;
	unsigned char *source_bits_p = NULL;
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
		
	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_TILE_STATE,
							 tile_state_p));

#if defined(__DEBUG__)
	if (DEBUG_LEVEL_MATCH(p9000_tile,SCAFFOLDING))
	{
		(void)fprintf(debug_stream_p,
			"(p9000_tile_offscreen_download_helper){\n"
			"\ttile_state_p = %p\n"
			"\toffscreen_width = %d\n"
			"\toffscreen_height = %d\n"
			"}\n",
			(void*)tile_state_p,
			tile_state_p->offscreen_width,
			tile_state_p->offscreen_height);
	}
#endif

	tile_p = tile_state_p->si_tile_p;

	ASSERT(screen_state_p->generic_state.screen_depth == 8);

	source_step = (tile_p->Bwidth + 3) & ~3  ;

	source_bits_p = (unsigned char *)tile_p->Bptr;

	offscreen_x_location = start_x = tile_state_p->offscreen_allocation_p->x;
	offscreen_y_location = start_y = tile_state_p->offscreen_allocation_p->y;

	tile_height = tile_p->Bheight;
	tile_width = tile_p->Bwidth;


	/*
	 * Set raster to GXcopy
	 */

	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();
	P9000_REGISTER_SET_RASTER(P9000_SOURCE_MASK);

	/*
	 * Set Clip rectangle
	 * (Don't forget to restore this before returning)
	 * Remember GE  clipping is inclusive on the right and
	 * bottom sides also
	 */

	P9000_REGISTER_SET_CLIPPING_RECTANGLE(start_x, start_y,
		(start_x + tile_state_p->offscreen_width - 1),
		(start_y + tile_state_p->offscreen_height - 1));

	P9000_STATE_INVALIDATE_CLIP_RECTANGLE(screen_state_p);
	
	/*
	 * Set planemask
	 */

	P9000_REGISTER_SET_PLANE_MASK(
		((1 << screen_state_p->generic_state.screen_depth) - 1));

	/*
	 * Program device coordinates and start pumping data using
	 * pixel8 command
	 */
	
	/*
	 * Setup for pixel8 command:
	 * x0: Left edge of the block to be transferred
	 * x1,y1: Point at which to begin transfer
 	 * x2: Right edge of the block to be transferred
	 * y3:  Y increment per after every scanline
	 * 
	 */

	
	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_0,
		P9000_PARAMETER_COORDINATE_X_32,
		P9000_PARAMETER_COORDINATE_ABS,
		start_x);

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_1,
		P9000_PARAMETER_COORDINATE_XY_16,
		P9000_PARAMETER_COORDINATE_ABS,
		P9000_PACK_XY(start_x,start_y));
	 

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_2,
		P9000_PARAMETER_COORDINATE_X_32,
		P9000_PARAMETER_COORDINATE_ABS,
		start_x + tile_width);

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_3,
		P9000_PARAMETER_COORDINATE_Y_32,
		P9000_PARAMETER_COORDINATE_ABS,1);

	
	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();

	height = tile_height;

	ASSERT(height > 0);

	do
	{
		int count = source_step >>  2;
		unsigned long *data_p = (unsigned long *)source_bits_p;

		do
		{
			P9000_PIXEL8_COMMAND(
				*data_p++,
				(P9000_ADDRESS_SWAP_HALF_WORDS|
				 P9000_ADDRESS_SWAP_BYTES));
				;

#if defined(__DEBUG__)
			status = P9000_READ_STATUS_REGISTER();
			ASSERT(!(status & P9000_STATUS_PIXEL_SOFTWARE));
#endif
		} while (--count > 0);

		source_bits_p += source_step;

	} while (--height > 0);


	start_x += tile_width;
	start_y += tile_height;

	/*
	 * Now duplicate the offscreen tile  horizontally as well as
	 * vertically
 	 */

	while(start_x  <
		  (offscreen_x_location + 
		   tile_state_p->offscreen_width))
	{
		
		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_0,
			P9000_PARAMETER_COORDINATE_XY_16,
			P9000_PARAMETER_COORDINATE_ABS,
			P9000_PACK_XY(offscreen_x_location,offscreen_y_location));

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_1,
			P9000_PARAMETER_COORDINATE_XY_16,
			P9000_PARAMETER_COORDINATE_ABS,
			P9000_PACK_XY((offscreen_x_location + tile_width - 1),
			(offscreen_y_location + tile_height - 1)));
		 
		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_2,
			P9000_PARAMETER_COORDINATE_XY_16,
			P9000_PARAMETER_COORDINATE_ABS,
			P9000_PACK_XY(start_x,offscreen_y_location));

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_3,
			P9000_PARAMETER_COORDINATE_XY_16,
			P9000_PARAMETER_COORDINATE_ABS,
			P9000_PACK_XY((start_x + tile_width - 1),
			(offscreen_y_location + tile_height -1)));

		do
		{
			status = P9000_INITIATE_BLIT_COMMAND();
			ASSERT(!(status & P9000_STATUS_BLIT_SOFTWARE));

		} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);

		start_x += tile_width;
	}

	/*
	 * Now duplicate vertically
	 */
	
	while (start_y < (offscreen_y_location + tile_state_p->offscreen_height))
	{

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_0,
			P9000_PARAMETER_COORDINATE_XY_16,
			P9000_PARAMETER_COORDINATE_ABS,
			P9000_PACK_XY(offscreen_x_location,offscreen_y_location));

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_1,
			P9000_PARAMETER_COORDINATE_XY_16,
			P9000_PARAMETER_COORDINATE_ABS,
			P9000_PACK_XY((offscreen_x_location +
			 tile_state_p->offscreen_width - 1),
			(offscreen_y_location + tile_height - 1)));
		 
		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_2,
			P9000_PARAMETER_COORDINATE_XY_16,
			P9000_PARAMETER_COORDINATE_ABS,
			P9000_PACK_XY(offscreen_x_location,start_y));

		P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
			P9000_PARAMETER_COORDINATE_REG_3,
			P9000_PARAMETER_COORDINATE_XY_16,
			P9000_PARAMETER_COORDINATE_ABS,
			P9000_PACK_XY((offscreen_x_location +
			tile_state_p->offscreen_width - 1),
			(start_y + tile_height - 1)));

		do
		{
			status = P9000_INITIATE_BLIT_COMMAND();
			ASSERT(!(status & P9000_STATUS_BLIT_SOFTWARE));
		} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);
		
		start_y += tile_height;

	}	


	/*
	 * Restore planemask
	 */

	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();

	P9000_REGISTER_SET_PLANE_MASK(graphics_state_p->generic_state.
		si_graphics_state.SGpmask & 
		((1 << screen_state_p->generic_state.screen_depth) - 1));

}


/*
 * @doc:p9000_tile_check_and_convert_tile_to_stipple:
 * 
 * If the tile has only two colors then the function converts
 * the tile to stipple and stores the stipple bits as well as
 * the two colors in the tile state
 *
 * @enddoc
 */

STATIC boolean
p9000_tile_check_and_convert_tile_to_stipple(
	struct p9000_tile_state * tile_state_p)
{
	int tile_width;
	int tile_height;
	int source_step;
	unsigned char color1;
	unsigned char color2;
	SIbitmapP si_tile_p = NULL;
	unsigned char *source_bits_p;
	unsigned long *stipple_bits_p;

	ASSERT(IS_OBJECT_STAMPED(P9000_TILE_STATE,tile_state_p));

	si_tile_p = tile_state_p->si_tile_p;

	/*
	 * Assume each pixel is a byte wide
	 */
	
	ASSERT(si_tile_p->BbitsPerPixel == 8);

	tile_width = si_tile_p->Bwidth;
	tile_height = si_tile_p->Bheight;

	source_step =  (si_tile_p->Bwidth + 3) & ~3;
	
	source_bits_p = (unsigned char*)si_tile_p->Bptr;

	/*
	 * If the existing space for the stipple can fit this
	 * stipple then use it, otherwise free and allocate new
	 * space.
	 */

	if ((tile_state_p->si_stipple.Bwidth < si_tile_p->Bwidth) ||
		(tile_state_p->si_stipple.Bheight < si_tile_p->Bheight) ||
		!tile_state_p->si_stipple.Bptr)
	{

		/*
		 * Free existing si bitmap 
		 */

		if (tile_state_p->si_stipple.Bptr)
		{
			free_memory(tile_state_p->si_stipple.Bptr);
			tile_state_p->si_stipple.Bptr = NULL;
		}

		/*
		 * Allocate si bitmap
		 */

		tile_state_p->si_stipple.Bptr = 
			allocate_and_clear_memory((((si_tile_p->Bwidth + 31) & ~31) >> 3) *
			si_tile_p->Bheight);
	}


	tile_state_p->si_stipple.Bwidth =
		si_tile_p->Bwidth;

	tile_state_p->si_stipple.Bheight =
		si_tile_p->Bheight;

	tile_state_p->si_stipple.BorgX =
		si_tile_p->BorgX;

	tile_state_p->si_stipple.BorgY =
		si_tile_p->BorgY;

	tile_state_p->si_stipple.BbitsPerPixel =  1;

	
	stipple_bits_p = 
		(unsigned long*) tile_state_p->si_stipple.Bptr;
	

	color1 = color2 = *source_bits_p;

	do
	{
		int bit_count = 0;
		unsigned long bit = 0;
		unsigned long tmp_word = 0;
		register int count = tile_width;
		register unsigned char  *data_p = 
			source_bits_p;

		do
		{
			if (*data_p == color1)
			{
				bit = 1;
			}
			else
			{
				if (*data_p == color2)
				{
			
					bit = 0;
				}
				else
				{
					if (color1 == color2)
					{
						color2 = *data_p;
						bit = 0;
					}
					else
					{

						/*
						 * There are more than two colors in this
						 * tile
						 */

						free_memory(tile_state_p->si_stipple.Bptr);
						tile_state_p->si_stipple.Bptr = NULL;
	
						return FALSE;
					}
				}
			}
			
			++data_p;

			tmp_word |=  (bit << bit_count);
			
			++bit_count;

			if (bit_count == 32)
			{

				*stipple_bits_p++ = tmp_word;
				tmp_word = 0;
				bit_count = 0;

			}

		} while (--count);
		
		/*
		 * Every row must begin on a long word boundary
		 */

		if (bit_count != 0)
		{
			*stipple_bits_p++ = tmp_word;
			tmp_word = 0;
			bit_count = 0;
		}
			
		source_bits_p +=  source_step;

	} while (--tile_height > 0);

	/*
	 * All is well, it is a two color tile.
	 */

	tile_state_p->color1 = color1;
	tile_state_p->color2 = color2;

	return TRUE;
}


/*
 * @doc:p9000_tile_download:
 *
 * @enddoc
 */

STATIC void
p9000_tile_download(
	struct p9000_screen_state *screen_state_p,
	struct p9000_tile_state *tile_state_p,
	SIbitmapP si_bitmap_p)
{

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_TILE_STATE,
							 tile_state_p));

	/*
	 * We don't have to bother about zero width/height tiles
	 */

	if ((si_bitmap_p->Bwidth <= 0) ||
		(si_bitmap_p->Bheight <= 0))
	{
		return;
	}

	tile_state_p->si_tile_p =
		si_bitmap_p;

	/*
	 * Our tiling strategy is as follows:
	 * If the user option says use-two-color-tile-speedup, then convert
	 * all  two colored tiles to stipples and let the stippling module
	 * handle the filling. This is because even system memory stippling
	 * is faster than offscreen memory tiling.
	 * If a given tile is not a two color one, the see if offscreen memory
	 * tiling is feasible. If this does not work settle for system memory
	 * tiling.
	 */

	if ((screen_state_p->options_p->rectfill_options &
		P9000_OPTIONS_RECTFILL_OPTIONS_USE_TWO_COLOR_TILE_SPEEDUP) &&
		(si_bitmap_p->Bwidth <= screen_state_p->options_p->two_color_tile_maximum_width) &&
		(si_bitmap_p->Bheight <= screen_state_p->options_p->two_color_tile_maximum_height) &&
		(p9000_tile_check_and_convert_tile_to_stipple(tile_state_p) == TRUE))
	{

		tile_state_p->is_two_color_tile = TRUE;

		/*
		 * Call the stipple module download function
		 */

		p9000_stipple_download_stipple(screen_state_p,
			&(tile_state_p->stipple_state),
			&(tile_state_p->si_stipple));

		tile_state_p->tile_function_p =
			p9000_tile_fill_rectangle_two_color_tile;

		return;
	}

	tile_state_p->is_two_color_tile = FALSE;

	if ((screen_state_p->options_p->rectfill_options &
		 P9000_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY) &&
		(si_bitmap_p->Bwidth <= screen_state_p->options_p->
		 maximum_offscreen_downloadable_tile_width) &&
		(si_bitmap_p->Bheight <= screen_state_p->options_p->
		 maximum_offscreen_downloadable_tile_height))
	 {

		/*
	 	 * Expand the tile in offscreen. This expansion is
		 * done only in multiples of tile_widths and
		 * tile heights
		 */

		 tile_state_p->offscreen_width = 
			 ((si_bitmap_p->Bwidth > screen_state_p->
			   options_p->offscreen_tile_padded_width) ?
			  si_bitmap_p->Bwidth : 
			  screen_state_p->
			  options_p->offscreen_tile_padded_width);

		 tile_state_p->offscreen_height = 
			 ((si_bitmap_p->Bheight > screen_state_p->
			   options_p->offscreen_tile_padded_height) ?
			  si_bitmap_p->Bheight : 
			  screen_state_p->
			  options_p->offscreen_tile_padded_height);
		
		 tile_state_p->offscreen_width = 
			 (tile_state_p->offscreen_width / si_bitmap_p->Bwidth) *
				 si_bitmap_p->Bwidth;

		 tile_state_p->offscreen_height = 
			 (tile_state_p->offscreen_height / si_bitmap_p->Bheight) *
				 si_bitmap_p->Bheight;

		/* 
		 * If the currently allocated offscreen area is big enough
		 * to hold this tile (after expansion) then go ahead and
		 * use it, if it is small or memory has not been already
		 * allocated then call omm_allocate  
		 */

		if ((tile_state_p->offscreen_allocation_p == NULL) ||
			(tile_state_p->offscreen_allocation_p->width <
			 tile_state_p->offscreen_width) ||
			(tile_state_p->offscreen_allocation_p->height <
			 tile_state_p->offscreen_height))
		{
			if (tile_state_p->offscreen_allocation_p)
			{
				omm_free(tile_state_p->offscreen_allocation_p);

				tile_state_p->offscreen_allocation_p = 
					NULL;
			}
			
			tile_state_p->offscreen_allocation_p =
				omm_allocate(tile_state_p->offscreen_width,
							 tile_state_p->offscreen_height,
							 si_bitmap_p->BbitsPerPixel,
							 OMM_LONG_TERM_ALLOCATION);

			/*
			 * Too bad, can't use offscreen memory
			 */

			if (tile_state_p->offscreen_allocation_p == NULL)
			{

#if defined(__DEBUG__)
				if (DEBUG_LEVEL_MATCH(p9000_tile,INTERNAL))
				{
					(void)fprintf(debug_stream_p,
						"(p9000_tile_download){\n"
						"\t#Unable to allocate offscreen area\n"
						"\t#Switching to system memory tiling\n"
						"}\n");
				}
#endif

				tile_state_p->tile_function_p =
					p9000_tile_fill_rectangle_offscreen_memory;

				return;
			}
			
		}

		 
		/*
		 * Call the download helper routine
		 */

		 p9000_tile_offscreen_download_helper(tile_state_p);
		
		tile_state_p->tile_function_p =
			p9000_tile_fill_rectangle_offscreen_memory;

	}
	else
	{	

		/* 
		 * Settle for system memory tiling
		 */

		tile_state_p->tile_function_p =
			p9000_tile_fill_rectangle_system_memory;
	}

}


/*
 * @doc:p9000_tile_fill_rectangles:
 *
 * @enddoc
 */

STATIC SIBool
p9000_tile_fill_rectangles(SIint32 x_origin, SIint32 y_origin,
	SIint32	count, SIRectOutlineP rect_p)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	struct p9000_tile_state *tile_state_p = NULL;


	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
							 graphics_state_p));
	
	if (count < 0)
	{
		return (SI_SUCCEED);
	}

	if (!P9000_REGISTER_IS_VALID_ORIGIN(x_origin,y_origin))
	{
		return SI_FAIL;
	}
	/*
	 * See if we have to download the tile
	 */

	if (graphics_state_p->is_tile_downloaded == FALSE)
	{ 


		if (graphics_state_p->tile_state_p == NULL)
		{
			graphics_state_p->tile_state_p =
				tile_state_p = allocate_and_clear_memory(
				sizeof(struct p9000_tile_state));

			STAMP_OBJECT(P9000_TILE_STATE,tile_state_p);

			STAMP_OBJECT(P9000_STIPPLE_STATE,
				(&tile_state_p->stipple_state));
		}

		p9000_tile_download(screen_state_p,
			graphics_state_p->tile_state_p,
			graphics_state_p->generic_state.si_graphics_state.SGtile);

		graphics_state_p->is_tile_downloaded = TRUE;

	}
	
	tile_state_p = graphics_state_p->tile_state_p;

	ASSERT(IS_OBJECT_STAMPED(P9000_TILE_STATE,tile_state_p));

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_tile,SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
			"(p9000_tile_fill_rectangle) {\n"
			"\tx_origin = %ld\n"
			"\ty_origin = %ld\n"
			"\tcount = %ld\n"
			"\trect_p = %p\n"
			"\t}\n",
			x_origin, y_origin, count, (void *) rect_p);
	}
#endif



	/*
	 *  Program the window origin
	 */

	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_W_OFF_XY,
		P9000_PACK_XY(x_origin, y_origin));

	P9000_STATE_SET_CLIP_RECTANGLE_TO_SI_CLIP(screen_state_p);

	/*
	 * Synchronize the registers to the SI specified values.
	 */

	P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
									  (P9000_STATE_CHANGE_PLANEMASK));

	P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
									   (P9000_STATE_CHANGE_PLANEMASK)); 

	/*
	 * All set, call the drawing function now
	 */

	return (*tile_state_p->tile_function_p)
		(tile_state_p,count,rect_p);

}


/*
 * @doc:p9000_tile_SI_1_0_tile_rectangles:
 *
 * Tile a series of rectangles handed down in SI_1_0 format.
 * This function basically sets up for drawing like the R5 entry
 * point, but defers calling the tile function entry point to the
 * convertor function.
 * 
 * @enddoc
 */

STATIC SIBool
p9000_tile_SI_1_0_tile_rectangles(
	SIint32 count,
	SIRectP rect_p)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	struct p9000_tile_state *tile_state_p = NULL;

	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
							 graphics_state_p));
	
	if (count < 0)
	{
		return SI_SUCCEED;
	}

	/*
	 * See if we have to download the tile
	 */

	if (graphics_state_p->is_tile_downloaded == FALSE)
	{ 
		if (graphics_state_p->tile_state_p == NULL)
		{
			graphics_state_p->tile_state_p =
				tile_state_p = allocate_and_clear_memory(
				sizeof(struct p9000_tile_state));

			STAMP_OBJECT(P9000_TILE_STATE,tile_state_p);

			STAMP_OBJECT(P9000_STIPPLE_STATE,
				(&tile_state_p->stipple_state));
		}


		p9000_tile_download(screen_state_p,
			graphics_state_p->tile_state_p,
			graphics_state_p->generic_state.si_graphics_state.SGtile);

		graphics_state_p->is_tile_downloaded = TRUE;

	}
	
	tile_state_p = graphics_state_p->tile_state_p;

	ASSERT(IS_OBJECT_STAMPED(P9000_TILE_STATE,tile_state_p));

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_tile,SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
			"(p9000_tile_fill_rectangle) {\n"
			"\tcount = %ld\n"
			"\trect_p = %p\n"
			"\t}\n",
			 count, (void *) rect_p);
	}
#endif

	/*
	 *  Program the window origin
	 */

	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_W_OFF_XY,0);

	P9000_STATE_SET_CLIP_RECTANGLE_TO_SI_CLIP(screen_state_p);

	/*
	 * Synchronize the registers to the SI specified values.
	 */

	P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
									  (P9000_STATE_CHANGE_PLANEMASK));

	P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
									   (P9000_STATE_CHANGE_PLANEMASK));

	/*
	 * All set, call the drawing function now
	 */

	return p9000_global_apply_SI_1_1_rect_fill_function(
		tile_state_p,
		count, rect_p,
		((SIBool (*)(void*, int, SIRectOutlineP))
			tile_state_p->tile_function_p));

}

/*
 * @doc:p9000_tile__gs_change__:
 *
 * If the  SGFillMode is SGFillTile then setup the si pointers to
 * the tiling functions.
 *
 * @enddoc
 */

function void
p9000_tile__gs_change__(void)
{

	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
							 graphics_state_p));


	if (graphics_state_p->generic_state.si_graphics_state.SGfillmode != 
		SGFillTile)
	{
		return;
	}

	if(!(screen_state_p->options_p->rectfill_options & 
		 P9000_OPTIONS_RECTFILL_OPTIONS_USE_TILE_FILL_RECT))
	{
		
		screen_state_p->generic_state.screen_functions_p->
			si_poly_fillrect =
			graphics_state_p->generic_si_functions.si_poly_fillrect;

		return;

	}

	if (screen_state_p->generic_state.screen_sdd_version_number >=
		DM_SI_VERSION_1_1) /*R5 and above*/
	{
		screen_state_p->generic_state.screen_functions_p->
			si_poly_fillrect = p9000_tile_fill_rectangles;
		
	}
	else /*R4 and below*/
	{
		screen_state_p->generic_state.screen_functions_p->
			si_poly_fillrect = 
				(SIBool (*)(SIint32, SIint32, SIint32, SIRectOutlineP))
					p9000_tile_SI_1_0_tile_rectangles;
	}

}




/*
 * @doc:p9000_tile__initialize__:
 * 
 * @enddoc
 */


function void
p9000_tile__initialize__(
	SIScreenRec *si_screen_p,
	struct p9000_options_structure *options_p)
{
	int tile_best_width = 0;
	int tile_best_height = 0;
	SIFlagsP flags_p = si_screen_p->flagsPtr;

	if (options_p->rectfill_options &
		P9000_OPTIONS_RECTFILL_OPTIONS_USE_TILE_FILL_RECT)
	{
		flags_p->SIavail_fpoly |= TILE_AVAIL;
	}

	if (options_p->tile_best_size)
	{
		if (sscanf(options_p->tile_best_size, "%ix%i",
				   &tile_best_width, &tile_best_height) != 2)
		{
			(void) fprintf(stderr,
						   P9000_MESSAGE_BAD_BEST_TILE_SIZE_SPECIFICATION,
						   options_p->tile_best_size);

			tile_best_width = P9000_DEFAULT_BEST_TILE_WIDTH;

			tile_best_height = P9000_DEFAULT_BEST_TILE_HEIGHT;
		}
	}

	flags_p->SItilewidth = tile_best_width ? tile_best_width :
		P9000_DEFAULT_BEST_TILE_WIDTH;

	flags_p->SItileheight = tile_best_height ? tile_best_height :
		P9000_DEFAULT_BEST_TILE_HEIGHT;

}
