/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_fill.c	1.5"

/***
 ***	NAME
 ***
 ***	SYNOPSIS
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

#include "stdenv.h"
#include "sidep.h"

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
export boolean m64_fill_debug=0;
export boolean m64_fill_solid_debug = 0;
export boolean m64_fill_tile_debug = 0;
export boolean m64_fill_stipple_debug = 0;
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
#include "g_omm.h"
#include "m64_gbls.h"
#include "m64_opt.h"
#include "m64_regs.h"
#include "m64_asm.h"
#include "m64_state.h"
#include "m64_gs.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/
#define M64_ROTATE_PATTERN_REGISTER_BITS(STIPPLE_STATE_P,X_OFFSET,Y_OFFSET)	\
{																			\
	int i = 0;																\
	unsigned char tmp_bits;													\
	unsigned long pattern_registers[DEFAULT_M64_PATTERN_REGISTERS_COUNT];	\
	unsigned char rotated_pattern_register_bytes[							\
		DEFAULT_M64_PATTERN_REGISTER_BYTES_COUNT];							\
	pattern_registers[0] = (STIPPLE_STATE_P)->pattern_register_0;			\
	pattern_registers[1] = (STIPPLE_STATE_P)->pattern_register_1;			\
	ASSERT((X_OFFSET) < 8 && (Y_OFFSET) < 8);								\
	do																		\
	{																		\
		unsigned char *bits_p = ((unsigned char *)pattern_registers +		\
			(((DEFAULT_M64_SMALL_STIPPLE_HEIGHT - Y_OFFSET) + i) & 			\
			(DEFAULT_M64_SMALL_STIPPLE_HEIGHT - 1)));						\
		tmp_bits = *bits_p << (X_OFFSET) | 									\
			(*bits_p >> (DEFAULT_M64_SMALL_STIPPLE_WIDTH - (X_OFFSET)));	\
		rotated_pattern_register_bytes[i] = tmp_bits;						\
	} while (++i < DEFAULT_M64_PATTERN_REGISTER_BYTES_COUNT);				\
	(STIPPLE_STATE_P)->pattern_register_0 =									\
		*((unsigned long *)rotated_pattern_register_bytes);					\
	(STIPPLE_STATE_P)->pattern_register_1 =									\
		*((unsigned long *)(rotated_pattern_register_bytes + 4));			\
}

/***
 ***	Functions.
 ***/

STATIC SIBool
m64_fill_rectangle_solid(SIint32 x_origin, SIint32 y_origin, 
	SIint32 count, SIRectOutlineP rect_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;
	const int clip_left 	= screen_state_p->generic_state.screen_clip_left;
	const int clip_right 	= screen_state_p->generic_state.screen_clip_right;
	const int clip_top 		= screen_state_p->generic_state.screen_clip_top;
	const int clip_bottom 	= screen_state_p->generic_state.screen_clip_bottom;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));


#if (defined(__DEBUG__))
	if (m64_fill_solid_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_fill_rectangle_solid) {\n"
			"\tx_origin = %ld\n"
			"\ty_origin = %ld\n"
			"\tcount = %ld\n"
			"\trect_p = %p\n"
			"\t}\n",
			x_origin, y_origin, count, (void *) rect_p);
	}
#endif

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}
	
	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGfillmode == 
		SGFillSolidFG || 
		graphics_state_p->generic_state.si_graphics_state.SGfillmode == 
		SGFillSolidBG);

	/*
	 * Check the clipping rectangle and set it coorectly to the one
	 * specified by SI. Remember we do not do Software clipping.
	 */
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

	/*
	 * At this point of time the following registers should contain 
	 * meaning values : put an assert later.
	 * DP_PIX_WID 
	 * WRITE_MASK
	 * FG_COLOR
	 * BG_COLOR
	 */
	/*
	 * Program the DP_SRC register to select the fg/bg color register
	 * to be the source for the rectangle fill.
	 */
	M64_WAIT_FOR_FIFO(2);
	*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
		(graphics_state_p->generic_state.si_graphics_state.SGfillmode == 
		SGFillSolidFG ? DP_SRC_FRGD_COLOR << DP_SRC_DP_FRGD_SRC_SHIFT : 0) |
		DP_SRC_DP_MONO_SRC_ALWAYS_1 ;
	*(register_base_address_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) = 	
		*(register_values_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET);


	/*
	 * loop through the list of rectangles.
	 */
	for (;count--;rect_p++)
	{
		int xl 		= 	rect_p->x + x_origin;
		int yt 		= 	rect_p->y + y_origin;
		int xr 		= 	xl + rect_p->width - 1;
		int yb 		= 	yt + rect_p->height - 1;
		unsigned int width;
		unsigned int height;
		unsigned int dst_height_wid;
		unsigned int dst_y_x;

#if (defined(__DEBUG__))
		if (m64_fill_solid_debug)
		{
			(void) fprintf(debug_stream_p,
				"(m64_fill_rectangle_solid) {\n"
				"\t\txl = %d\n"
				"\t\tyt = %d\n"
				"\t\txr = %d\n"
				"\t\tyb = %d\n"
				"\t}\n",
				xl, yt, xr, yb);
		}
#endif

		if ((xl > clip_right) || (xr < clip_left) || 
			(yt > clip_bottom) || (yb < clip_top) ||
			(xl > xr) || (yt > yb))
		{
#if (defined(__DEBUG__))
			if (m64_fill_solid_debug)
			{
				(void) fprintf(debug_stream_p,
					"(m64_fill_rectangle_solid) {\n"
					"\t\t(skipping rectangle.\n\t}\n");
			}
#endif

			continue;
		}

		/*
         * Software clip the destination rectangle to clipping rectangle
		 * bounds. 
         */
        if (xl < clip_left)
        {
            xl = clip_left;
        }
		if (xr > clip_right)
        {
            xr = clip_right;
        }
        if (yt < clip_top)
        {
            yt = clip_top;
        }
        if (yb > clip_bottom)
        {
            yb = clip_bottom;
        }

		height = yb - yt + 1;
		width =  xr - xl + 1;

		if ((width <= 0) || (height <= 0))
		{
			continue;
		}

		dst_height_wid	=	 (height | ((unsigned)width << 
			DST_HEIGHT_WID_DST_WID_SHIFT)) & DST_HEIGHT_WID_BITS; 
		dst_y_x	= 	(yt | 
			(xl << DST_Y_X_DST_X_SHIFT)) & DST_Y_X_BITS;

		ASSERT(width > 0 && height > 0);

		/*
		 * Draw the rectangle.
		 */
		M64_WAIT_FOR_FIFO(2);
		*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = dst_y_x;
		*(register_base_address_p + M64_REGISTER_DST_HEIGHT_WID_OFFSET) = 
			dst_height_wid;
	}

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
#if (defined(__DEBUG__))	
	M64_WAIT_FOR_GUI_ENGINE_IDLE();
#endif

	return (SI_SUCCEED);
}

STATIC SIBool
m64_fill_rectangle_through_mono_pattern_registers(
	SIint32 x_origin, SIint32 y_origin,
	SIint32	count, SIRectOutlineP rect_p, 
	struct m64_stipple_state *stipple_state_p,
	int	flags)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();

	unsigned long *register_values_p = screen_state_p->register_state.registers;
	const int clip_left 	= screen_state_p->generic_state.screen_clip_left;
	const int clip_right 	= screen_state_p->generic_state.screen_clip_right;
	const int clip_top 		= screen_state_p->generic_state.screen_clip_top;
	const int clip_bottom 	= screen_state_p->generic_state.screen_clip_bottom;

	ASSERT(count > 0);
	ASSERT(stipple_state_p->stipple_downloaded == TRUE);
	ASSERT(stipple_state_p->is_small_stipple == TRUE);
	ASSERT(!M64_IS_FIFO_OVERFLOW());

#if (defined(__DEBUG__))
	if (m64_fill_stipple_debug || m64_fill_tile_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_fill_rectangle_through_mono_pattern_registers)\n"
			"{\n"
			"\tx_origin = %ld\n"
			"\ty_origin = %ld\n"
			"\tcount = %ld\n"
			"\trect_p = 0x%x\n"
			"\tstipple_state_p = 0x%x\n"
			"}\n",
			x_origin, y_origin, count, (unsigned) rect_p,
			(unsigned) stipple_state_p);
	}
#endif

	/*
	 * Check the clipping rectangle and set it correctly to the one
	 * specified by SI. Remember we do not do Software clipping.
	 */
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

	/*
	 * Program the gui trajectory control for 8x8 mono-pattern.
	 * Program the pattern registers with the stipple data.
	 * Program the dp_source for frgd color, bkgd color and
	 * mono source to pattern registers.
	 * Program the bg mix to noop for transparent stippling. 
	 */
	M64_WAIT_FOR_FIFO(5);
	*(register_base_address_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) = 	
		*(register_values_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) |
		GUI_TRAJ_CNTL_PAT_MONO_EN;

	*(register_base_address_p + M64_REGISTER_PAT_REG0_OFFSET) = 	
		stipple_state_p->pattern_register_0;

	*(register_base_address_p + M64_REGISTER_PAT_REG1_OFFSET) = 	
		stipple_state_p->pattern_register_1;

	*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
		(DP_SRC_BKGD_COLOR) |
		(DP_SRC_FRGD_COLOR <<  DP_SRC_DP_FRGD_SRC_SHIFT) |
		(DP_SRC_DP_MONO_SRC_PATTERN_REGISTERS << DP_SRC_DP_MONO_SRC_SHIFT);

	if ( flags == SGStipple)
	{
		*(register_base_address_p + M64_REGISTER_DP_MIX_OFFSET) =
			(*(register_values_p + M64_REGISTER_DP_MIX_OFFSET) & 
			~DP_MIX_DP_BKGD_MIX) | DP_MIX_GXnoop;
	}
#if (defined(__DEBUG__))
	else 
	{
		ASSERT( flags == SGOPQStipple);
	}
#endif

	do
	{
		int destination_width;
		int destination_height;

		/*
		 * destination rectangle coordinates.
		 */
		int x_left = rect_p->x + x_origin;
		int x_right = x_left + rect_p->width - 1;
		int y_top = rect_p->y + y_origin;
		int y_bottom = y_top + rect_p->height - 1;

		/*
		 * Check if the rectangle is outside the current clipping
		 * bounds. Discard the rectangle if so.
		 */
		if((x_left > clip_right) || (x_right < clip_left) ||
		   (y_top > clip_bottom) || (y_bottom < clip_top) ||
		   (y_bottom < y_top) || ( x_right < x_left ))
		{
#if (defined(__DEBUG__))
			if (m64_fill_debug)
			{
				(void) fprintf(debug_stream_p,"\tRectangle out of bounds\n");
			}
#endif
			++rect_p;
			continue;
		}
			
		/*
         * Software clip the destination rectangle to clipping rectangle
		 * bounds. 
         */
        if (x_left < clip_left)
        {
            x_left = clip_left;
        }
		if (x_right > clip_right)
        {
            x_right = clip_right;
        }
        if (y_top < clip_top)
        {
            y_top = clip_top;
        }
        if (y_bottom > clip_bottom)
        {
            y_bottom = clip_bottom;
        }

		destination_height = y_bottom - y_top + 1;
		destination_width =  x_right - x_left +1;

		/*
		 * Set the clipping rectangle.
		 */
		M64_WAIT_FOR_FIFO(2);

		*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = 
			y_top | (x_left << DST_Y_X_DST_X_SHIFT);
		*(register_base_address_p + M64_REGISTER_DST_HEIGHT_WID_OFFSET) = 
			destination_height |
			(destination_width << DST_HEIGHT_WID_DST_WID_SHIFT); 

		++rect_p;
	}while(--count);	

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));

	return(SI_SUCCEED);
}

/*
 * Fillrect routine for SGfillmode = SGfillTile and tile downloaded in
 * system memory. i.e., tile not in offscreen/pattern registers.
 */
STATIC SIBool
m64_fill_rectangle_tile_system_memory( SIint32 x_origin, SIint32 y_origin,
	SIint32	count, SIRectOutlineP rect_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_FRAMEBUFFER_BASE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	const int destination_stride = screen_state_p->framebuffer_stride;
	const int depth_shift = screen_state_p->generic_state.screen_depth_shift;
	const int pixels_per_long = 1 << screen_state_p->pixels_per_long_shift;

	const int clip_left 	= screen_state_p->generic_state.screen_clip_left;
	const int clip_right 	= screen_state_p->generic_state.screen_clip_right;
	const int clip_top 		= screen_state_p->generic_state.screen_clip_top;
	const int clip_bottom 	= screen_state_p->generic_state.screen_clip_bottom;

	/*
	 * tile related variables.
	 */
	SIbitmapP				si_tile_p;
	struct m64_tile_state 	*tile_state_p;	
	int						tile_width;
	int 					tile_height;
	unsigned long 			*source_start_p;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT((((int) framebuffer_p) & 0x03) == 0);

#if (defined(__DEBUG__))
	if (m64_fill_tile_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_fill_rectangle_tile_system_memory)\n"
			"{\n"
			"\tx_origin = %ld\n"
			"\ty_origin = %ld\n"
			"\tcount = %ld\n"
			"\trect_p = %p\n"
			"}\n",
			x_origin, y_origin, count, (void *) rect_p);
	}
#endif

	/*
	 * initialize the tile parameteres.
	 */
	tile_state_p = &(graphics_state_p->current_tile_state);

	ASSERT(tile_state_p->is_reduced_tile == FALSE);
	ASSERT(tile_state_p->tile_downloaded == TRUE);

	si_tile_p = graphics_state_p->generic_state.si_graphics_state.SGtile;
	tile_width = si_tile_p->Bwidth;
	tile_height = si_tile_p->Bheight;
	source_start_p = (unsigned long *)si_tile_p->Bptr;

	if (si_tile_p->BbitsPerPixel == 24)
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
	}

	ASSERT(tile_width > 0);
	ASSERT(tile_height > 0);
	/*
	 * Tile the rectangles in the list one at a time.
	 */
	ASSERT(count > 0);
	do
	{
		/*
		 * destination rectangle coordinates.
		 */
		int x_left = rect_p->x + x_origin;
		int x_right = x_left + rect_p->width - 1;
		int y_top = rect_p->y + y_origin;
		int y_bottom = y_top + rect_p->height - 1;
		int width;
		int height;

		/*
		 * offsets into the tile for the partial widths.
		 */
		int x_offset;
		int y_offset;

		/*
		 * The left partial tile width.
		 * Number of tile widths in the middle.
		 * The right partial tile width.
		 */
		int	startwidth;
		int	midwidths;
		int	endwidth;

		/*
		 * the pointer to the offset into framebuffer.
		 * the offset on the destination.
		 * the offset in the source bitmap.
		 */
		unsigned long   *tmp_frame_buffer_p; 
		unsigned int	tmp_destination_offset;
		unsigned int	source_xy_offset;
		unsigned int	source_x_offset;
		unsigned int	source_y_offset;
		unsigned long	*source_xy_offset_p;
		unsigned long	*source_x_offset_p;
		unsigned long	*source_y_offset_p;
	 
		/*
		 * Temporaries.
		 */
		int	curdx;
		int	curdy;
		int	cursy;
		unsigned int tmp_source_offset;
		unsigned long *tmp_source_p;

		/*
		 * Check if the rectangle is outside the current clipping
		 * bounds. Discard the rectangle if so.
		 */
		if((x_left > clip_right) || (x_right < clip_left) ||
		   (y_top > clip_bottom) || (y_bottom < clip_top) ||
		   (y_bottom < y_top) || ( x_right < x_left ))
		{
#if (defined(__DEBUG__))
			if (m64_fill_stipple_debug)
			{
				(void) fprintf(debug_stream_p,"\tRectangle out of bounds\n");
			}
#endif
			++rect_p;
			continue;
		}
			
		/*
         * Software clip the destination rectangle to clipping rectangle
		 * bounds. 
         */
        if (x_left < clip_left)
        {
            x_left = clip_left;
        }
		if (x_right > clip_right)
        {
            x_right = clip_right;
        }
        if (y_top < clip_top)
        {
            y_top = clip_top;
        }
        if (y_bottom > clip_bottom)
        {
            y_bottom = clip_bottom;
        }

		width = x_right - x_left + 1;
		height = y_bottom - y_top + 1;


		/*
		 * Calculate offsets into the source tile	
		 */
		x_offset = ( x_left - si_tile_p->BorgX)  % tile_width;

		if( x_offset < 0)
		{
			x_offset += tile_width;
		}


		y_offset = ( y_top - si_tile_p->BorgY)  % tile_height;

		if( y_offset < 0)
		{
			y_offset += tile_height;
		}

		/*
		 * Compute the pixel offsets into the source bitmap 
		 * corresponding to the top left pixel of the destination.
		 */
		source_x_offset = x_offset ;
		source_y_offset = y_offset * 
			((unsigned)(tile_state_p->source_step << 3U) >> depth_shift);
		source_xy_offset = source_y_offset + source_x_offset;

		/*
		 * Compute the longword address in the source bitmap containing
		 * these pixels.
		 */
		source_x_offset_p = source_start_p + 
			((unsigned)(source_x_offset  << depth_shift) >> 5U);
		source_y_offset_p = source_start_p + 
			((unsigned)(source_y_offset << depth_shift) >> 5U);
		source_xy_offset_p = source_start_p + 
			((unsigned)(source_xy_offset << depth_shift) >> 5U);

		/*
		 * Convert source and destination offsets to offsets into 
		 * first longword.
	 	 */
		source_x_offset &= pixels_per_long - 1;
		source_y_offset &= pixels_per_long - 1;
		source_xy_offset &= pixels_per_long - 1;

		/*
		 * Compute the following parameters.
		 * 1. The left partial width before encountering the first
		 *    complete tile width.
		 * 2. The number of middle widths, the full tile width transfers.
		 * 3. The end partial width, the right width which is less than
		 *    one full tile width.
		 */
		if((x_left +  (tile_width - x_offset) - 1) < x_right)
		{
			/*
			 * More than 1 transfer is there for this rectangle.
			 */
			if (x_offset > 0)
			{
				startwidth = tile_width - x_offset;
			}
			else
			{
				startwidth = 0;
			}

			midwidths = (width - startwidth) / tile_width;
			endwidth = (width - startwidth) % tile_width;
		}
		else
		{
			/*
			 * This is the only transfer for this rectangle.
			 */
			startwidth = width;
			midwidths = 0;
			endwidth = 0;
		}


#if (defined(__DEBUG__))
	if (m64_fill_tile_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_fill_rectangle_tile_system_memory)\n"
			"{\n"
			"\tx_left = %d\n"
			"\tx_right = %d\n"
			"\ty_top = %d\n"
			"\ty_bottom = %d\n"
			"\twidth = %d\n"
			"\theight = %d\n"
			"\tx_offset = %d\n"
			"\ty_offset = %d\n"
			"\tsource_xy_offset = %ld\n"
			"\tsource_x_offset = %ld\n"
			"\tsource_y_offset = %ld\n"
			"\tstartwidth = %ld\n"
			"\tmidwidths = %ld\n"
			"\tendwidth = %ld\n"
			"}\n",
			x_left, x_right, y_top, y_bottom, width, height,
			x_offset, y_offset,
			source_xy_offset, source_x_offset, source_y_offset, 
			startwidth, midwidths, endwidth);
	}
#endif
		/*
		 * Tile the destination rectangle.
		 * Finish with the start words first.
		 * initialize the current x coordinate to start with.
		 */
		curdx = x_left;

		if (startwidth > 0)
		{
			curdy = y_top;
			cursy = y_offset;
			tmp_source_offset = source_xy_offset;
			tmp_source_p = source_xy_offset_p;
	
			ASSERT(curdy <= y_bottom);
			do
			{
				int		tilelines;
				tmp_destination_offset = 
					x_left + (curdy * 
					(((unsigned)destination_stride << 3U) >> depth_shift));
				tmp_frame_buffer_p = (unsigned long *) framebuffer_p + 
					(unsigned) (tmp_destination_offset >> 
					screen_state_p->pixels_per_long_shift); 
				tmp_destination_offset &= pixels_per_long - 1;

				tilelines = ((curdy + tile_height - cursy) > y_bottom )
					? y_bottom - curdy + 1 : tile_height - cursy;

				ASSERT(tilelines > 0);
				screen_state_p->transfer_pixels_p(tmp_source_p,
					tmp_frame_buffer_p, 
					tmp_source_offset , tmp_destination_offset,
					tile_state_p->source_step, destination_stride,
					startwidth, tilelines, si_tile_p->BbitsPerPixel, 
					graphics_state_p->generic_state.si_graphics_state.SGmode,
					graphics_state_p->generic_state.si_graphics_state.SGpmask,
					screen_state_p->pixels_per_long_shift);

				curdy += tilelines;
				cursy = 0;
				tmp_source_offset = source_x_offset;
				tmp_source_p = source_x_offset_p;

			} while ( curdy <= y_bottom );

			curdx += startwidth;
		}

		if ( midwidths )
		{
			/*
			 * do the midwidths next.
			 */
			ASSERT(midwidths > 0);
			do
			{
				curdy = y_top;
				cursy = y_offset;
				tmp_source_offset = source_y_offset;
				tmp_source_p = source_y_offset_p;

				ASSERT(curdy <= y_bottom);
				do
				{
					int		tilelines;
					tmp_destination_offset = 
						curdx + (curdy * 
						(((unsigned)destination_stride << 3U) >> depth_shift));
					tmp_frame_buffer_p = 
						(unsigned long *) framebuffer_p + 
						(unsigned) (tmp_destination_offset >> 
						screen_state_p->pixels_per_long_shift);
					tmp_destination_offset &= pixels_per_long - 1;

					tilelines = ((curdy + tile_height - cursy) > y_bottom )
						? y_bottom - curdy + 1 : tile_height - cursy;

					ASSERT(tilelines > 0);
					screen_state_p->transfer_pixels_p(tmp_source_p,
						tmp_frame_buffer_p, 
						tmp_source_offset , tmp_destination_offset,
						tile_state_p->source_step, destination_stride,
						tile_width, tilelines, si_tile_p->BbitsPerPixel, 
						graphics_state_p->
							generic_state.si_graphics_state.SGmode,
						graphics_state_p->
							generic_state.si_graphics_state.SGpmask,
						screen_state_p->pixels_per_long_shift);

					curdy += tilelines;
					cursy = 0;
					tmp_source_offset = 0;
					tmp_source_p = source_start_p;

				} while ( curdy <= y_bottom );

				curdx += tile_width;
			} while ( --midwidths > 0);
		}

		if (endwidth)
		{
			curdy = y_top;
			cursy = y_offset;
			tmp_source_offset = source_y_offset;
			tmp_source_p = source_y_offset_p;

			ASSERT(curdy <= y_bottom);
			do
			{
				int		tilelines;
				tmp_destination_offset = curdx + (curdy * 
					(((unsigned)destination_stride << 3U) >> depth_shift));
				tmp_frame_buffer_p = (unsigned long *) framebuffer_p + 
					(unsigned)(tmp_destination_offset >> 
					screen_state_p->pixels_per_long_shift); 
				tmp_destination_offset &= pixels_per_long - 1;

				tilelines = ((curdy + tile_height - cursy) > y_bottom )
					? y_bottom - curdy + 1 : tile_height - cursy;

				ASSERT(tilelines > 0);
				screen_state_p->transfer_pixels_p(tmp_source_p,
					tmp_frame_buffer_p, 
					tmp_source_offset , tmp_destination_offset,
					tile_state_p->source_step, destination_stride,
					endwidth, tilelines, si_tile_p->BbitsPerPixel, 
					graphics_state_p->generic_state.si_graphics_state.SGmode,
					graphics_state_p->generic_state.si_graphics_state.SGpmask,
					screen_state_p->pixels_per_long_shift);

				curdy += tilelines;
				cursy = 0;
				tmp_source_offset = 0;
				tmp_source_p = source_start_p;

			} while ( curdy <= y_bottom );
		}
		
		++rect_p;

	} while (--count > 0);

	return (SI_SUCCEED);
}

/*
 * Fillrect routine for SGfillmode = SGfillTile and tile downloaded in
 * offscreen memory.
 */
STATIC SIBool
m64_fill_rectangle_tile_offscreen_memory( SIint32 x_origin, SIint32 y_origin,
	SIint32	count, SIRectOutlineP rect_p)
{

	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;
	const int clip_left 	= screen_state_p->generic_state.screen_clip_left;
	const int clip_right 	= screen_state_p->generic_state.screen_clip_right;
	const int clip_top 		= screen_state_p->generic_state.screen_clip_top;
	const int clip_bottom 	= screen_state_p->generic_state.screen_clip_bottom;
	struct m64_tile_state 	*tile_state_p;	
	SIbitmap 				*si_tile_p;
	int 					offscreen_tile_x_location;
	int 					offscreen_tile_y_location;
	int 					offscreen_tile_height;
	int 					offscreen_tile_width;

#if (defined(__DEBUG__))
	if (m64_fill_tile_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_fill_rectangle_tile_offscreen_memory)\n"
			"{\n"
			"\tx_origin = %ld\n"
			"\ty_origin = %ld\n"
			"\tcount = %ld\n"
			"\trect_p = %p\n"
			"}\n",
			x_origin, y_origin, count, (void *) rect_p);
	}
#endif
	tile_state_p = &(graphics_state_p->current_tile_state);

	si_tile_p =
		graphics_state_p->generic_state.si_graphics_state.SGtile;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_TILE_STATE, tile_state_p));

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}
	
	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGfillmode == 
		SGFillTile);

	ASSERT(!M64_IS_FIFO_OVERFLOW());

	ASSERT(tile_state_p->tile_downloaded == TRUE);

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	/*
	 * Check the clipping rectangle and set it coorectly to the one
	 * specified by SI. Remember we do not do Software clipping.
	 */
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

	/*
	 * Program the DP_SRC register to select the fg/bg color register
	 * to be the source for the rectangle fill.
	 */
	M64_WAIT_FOR_FIFO(2);
	*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
		(DP_SRC_BLIT_SRC << DP_SRC_DP_FRGD_SRC_SHIFT) |
		DP_SRC_DP_MONO_SRC_ALWAYS_1;
	*(register_base_address_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) = 	
		*(register_values_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) | 
		GUI_TRAJ_CNTL_SRC_PATT_EN | GUI_TRAJ_CNTL_SRC_PATT_ROT_EN;


	offscreen_tile_x_location = tile_state_p->offscreen_location_x;
	offscreen_tile_y_location = tile_state_p->offscreen_location_y;
	offscreen_tile_width = si_tile_p->Bwidth;
	offscreen_tile_height = si_tile_p->Bheight;

	do
	{
		int destination_width;
		int destination_height;
		int	fractional_height_start; 
		int fractional_width_start; 
		int offscreen_fractional_x_position;
		int offscreen_fractional_y_position;

		/*
		 * destination rectangle coordinates.
		 */
		int x_left = rect_p->x + x_origin;
		int x_right = x_left + rect_p->width - 1;
		int y_top = rect_p->y + y_origin;
		int y_bottom = y_top + rect_p->height - 1;

		/*
		 * Check if the rectangle is outside the current clipping
		 * bounds. Discard the rectangle if so.
		 */
		if((x_left > clip_right) || (x_right < clip_left) ||
		   (y_top > clip_bottom) || (y_bottom < clip_top) ||
		   (y_bottom < y_top) || ( x_right < x_left ))
		{
#if (defined(__DEBUG__))
			if (m64_fill_tile_debug)
			{
				(void) fprintf(debug_stream_p,"\tRectangle out of bounds\n");
			}
#endif
			++rect_p;
			continue;
		}
			
		/*
         * Software clip the destination rectangle to clipping rectangle
		 * bounds. 
         */
        if (x_left < clip_left)
        {
            x_left = clip_left;
        }
		if (x_right > clip_right)
        {
            x_right = clip_right;
        }
        if (y_top < clip_top)
        {
            y_top = clip_top;
        }
        if (y_bottom > clip_bottom)
        {
            y_bottom = clip_bottom;
        }

		destination_height = y_bottom - y_top + 1;
		destination_width =  x_right - x_left +1;

		fractional_width_start = ( x_left - si_tile_p->BorgX)  % 
			offscreen_tile_width;
		if( fractional_width_start < 0)
		{
			fractional_width_start += offscreen_tile_width;
		}
		offscreen_fractional_x_position = 
			offscreen_tile_x_location + fractional_width_start;
		if (fractional_width_start >= 0)
		{
			fractional_width_start = 
				offscreen_tile_width - fractional_width_start;
			fractional_width_start = 
				(fractional_width_start > destination_width?
				 destination_width : fractional_width_start);
		}

		fractional_height_start = ( y_top - si_tile_p->BorgY)  % 
			offscreen_tile_height;
		if( fractional_height_start < 0)
		{
			fractional_height_start += offscreen_tile_height;
		}
		offscreen_fractional_y_position = 
			offscreen_tile_y_location + fractional_height_start;
		if (fractional_height_start >= 0)
		{
			fractional_height_start = 
				offscreen_tile_height - fractional_height_start;
			fractional_height_start = 
				(fractional_height_start > destination_height?
				 destination_height : fractional_height_start);
		}

		ASSERT(fractional_width_start > 0 && fractional_height_start > 0);

		M64_WAIT_FOR_FIFO(6);
		*(register_base_address_p + M64_REGISTER_SRC_Y_X_OFFSET) = 
			offscreen_fractional_y_position | 
			(offscreen_fractional_x_position << SRC_Y_X_SRC_X_SHIFT);
		*(register_base_address_p + M64_REGISTER_SRC_HEIGHT1_WID1_OFFSET) = 
			fractional_height_start |
			(fractional_width_start << SRC_HEIGHT1_WID1_SRC_WID1_SHIFT);

		*(register_base_address_p + M64_REGISTER_SRC_Y_X_START_OFFSET) = 
			offscreen_tile_y_location |
			(offscreen_tile_x_location << SRC_Y_X_START_SRC_X_START_SHIFT);
		*(register_base_address_p + M64_REGISTER_SRC_HEIGHT2_WID2_OFFSET) = 
			offscreen_tile_height | 
			(offscreen_tile_width << SRC_HEIGHT2_WID2_SRC_WID2_SHIFT);

		ASSERT(destination_height > 0 && destination_width > 0);
		*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = 
			y_top | (x_left << DST_Y_X_DST_X_SHIFT);
		*(register_base_address_p + M64_REGISTER_DST_HEIGHT_WID_OFFSET) = 
			destination_height |
			(destination_width << DST_HEIGHT_WID_DST_WID_SHIFT); 

		++rect_p;
	}while(--count);	

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
	return (SI_SUCCEED);
}
	
/*
 * Fillrect routine for SGfillmode = SGfillStipple
 */
STATIC SIBool
m64_fill_rectangle_stipple_system_memory( SIint32 x_origin, SIint32 y_origin,
	SIint32	count, SIRectOutlineP rect_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();

	unsigned long *register_values_p = screen_state_p->register_state.registers;
	const int clip_left 	= screen_state_p->generic_state.screen_clip_left;
	const int clip_right 	= screen_state_p->generic_state.screen_clip_right;
	const int clip_top 		= screen_state_p->generic_state.screen_clip_top;
	const int clip_bottom 	= screen_state_p->generic_state.screen_clip_bottom;

	SIbitmap 					*si_stipple_p;
	struct m64_stipple_state 	*stipple_state_p;

	long 						stipple_height;		/*in pixels */
	long 						stipple_width;		/*in pixels */
	unsigned long				stipple_width_in_longs;

	/*
	 * In a way directly programmable into the register.
	 */
	unsigned long				shifted_stipple_width_in_pixels;
		
	/*
	 * Pointer to the (0,0) coords in source stipple.
	 * Pointer to the (destination_x,0) coords in source stipple.
	 */
	unsigned long			*source_start_p;
	unsigned long			*source_x_offset_p;

	stipple_state_p = &(graphics_state_p->current_stipple_state);
	si_stipple_p = graphics_state_p->generic_state.si_graphics_state.SGstipple;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_STIPPLE_STATE, stipple_state_p));
	ASSERT(!M64_IS_FIFO_OVERFLOW());

	ASSERT(stipple_state_p->is_small_stipple == FALSE);
	ASSERT(stipple_state_p->stipple_downloaded == TRUE);

#if (defined(__DEBUG__))
	if (m64_fill_stipple_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_fill_rectangle_stipple_system_memory)\n"
			"{\n"
			"\tx_origin = %ld\n"
			"\ty_origin = %ld\n"
			"\tcount = %ld\n"
			"\trect_p = %p\n"
			"}\n",
			x_origin, y_origin, count, (void *) rect_p);
	}
#endif

	/*
	 * SI stipple's width, height and bitmap width rounded off to longwords.
	 */
	stipple_width = si_stipple_p->Bwidth;
	stipple_height = si_stipple_p->Bheight;
	stipple_width_in_longs = ((unsigned)(stipple_width + 31) & ~31) >> 5U;
	shifted_stipple_width_in_pixels = ((unsigned)(stipple_width + 31) & ~31) <<
		DST_HEIGHT_WID_DST_WID_SHIFT;

	/*
	 * pointer to beginning of the stipple data.
	 */
	source_start_p = (unsigned long *)si_stipple_p->Bptr;

	/*
	 * Prepare the GUI engine for host based stippling.
	 * If transparent stipple-fill,set bkgd rop to GXnoop.
	 * Remember the rops are already set correctly for opaque stippling.
	 */
	M64_WAIT_FOR_FIFO(4);
	*(register_base_address_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) = 	
		*(register_values_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET);

	*(register_base_address_p + M64_REGISTER_DP_PIX_WID_OFFSET) =
		register_values_p[M64_REGISTER_DP_PIX_WID_OFFSET] & 
		~DP_PIX_WID_DP_HOST_PIX_WID;

	*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
		(DP_SRC_BKGD_COLOR) |
		(DP_SRC_FRGD_COLOR <<  DP_SRC_DP_FRGD_SRC_SHIFT) |
		(DP_SRC_DP_MONO_SRC_HOST_DATA << DP_SRC_DP_MONO_SRC_SHIFT);

	if ( graphics_state_p->generic_state.si_graphics_state.SGstplmode
		== SGStipple)
	{
		*(register_base_address_p + M64_REGISTER_DP_MIX_OFFSET) =
			(*(register_values_p + M64_REGISTER_DP_MIX_OFFSET) & 
			~DP_MIX_DP_BKGD_MIX) | DP_MIX_GXnoop;
	}
#if (defined(__DEBUG__))
	else 
	{
		ASSERT( graphics_state_p->generic_state.si_graphics_state.SGstplmode
			== SGOPQStipple);
	}
#endif

	/*
	 * Stipple the rectangles in the list one at a time.
	 */
	ASSERT(count > 0);
	do
	{
		/*
		 * destination rectangle coordinates.
		 */
		int x_left = rect_p->x + x_origin;
		int x_right = x_left + rect_p->width - 1;
		int y_top = rect_p->y + y_origin;
		int y_bottom = y_top + rect_p->height - 1;
		int width;
		int height;

		/*
		 * destination x coordinate from which to start stippling.
		 */
		int 					destination_x;
		int 					destination_y;

		/*
		 * Number of host data words at the start.
		 * Number of host data widths in the middle.
		 * Number of host data words at the end.
		 */
		unsigned long			startwords;
		unsigned long			midwidths;
		unsigned long			endwords;


		/*
		 * offsets into the stipple for the partial widths.
		 */
		int x_offset;
		int y_offset;

		/*
		 * end of startwidth,  start of endwidth
		 */
		int startwidth_x_right;
		int endwidth_x_left;

		/*
		 * temporaries.
		 */
		int	curdx;
		int	curdy;
		int	cursy;

		/*
		 * Check if the rectangle is outside the current clipping
		 * bounds. Discard the rectangle if so.
		 */
		if((x_left > clip_right) || (x_right < clip_left) ||
		   (y_top > clip_bottom) || (y_bottom < clip_top) ||
		   (y_bottom < y_top) || ( x_right < x_left ))
		{
#if (defined(__DEBUG__))
			if (m64_fill_stipple_debug)
			{
				(void) fprintf(debug_stream_p,"\tRectangle out of bounds\n");
			}
#endif
			++rect_p;
			continue;
		}
			
		/*
         * Software clip the destination rectangle to clipping rectangle
		 * bounds. 
         */
        if (x_left < clip_left)
        {
            x_left = clip_left;
        }
		if (x_right > clip_right)
        {
            x_right = clip_right;
        }
        if (y_top < clip_top)
        {
            y_top = clip_top;
        }
        if (y_bottom > clip_bottom)
        {
            y_bottom = clip_bottom;
        }

		/*
		 * The new (clipped?) width and height.
		 */
		width = x_right - x_left + 1;
		height = y_bottom - y_top + 1;

		/*
		 * Set the clipping rectangle to correspond to the top, left and
		 * the bottom edges of the destination rectangle.
		 */
		M64_WAIT_FOR_FIFO(2);
		register_base_address_p[M64_REGISTER_SC_TOP_BOTTOM_OFFSET] = 
			(y_top | (y_bottom << SC_TOP_BOTTOM_SC_BOTTOM_SHIFT));
		register_base_address_p[M64_REGISTER_SC_LEFT_OFFSET] = x_left;

		destination_x = x_left;
		destination_y = y_top;

		/*
		 * Calculate offsets into the source stipple	
		 */
		x_offset = ( x_left - si_stipple_p->BorgX)  % stipple_width;

		if( x_offset < 0)
		{
			x_offset += stipple_width;
		}

		/*
		 * Push x_offset back to a longword boundary
		 */
		if (x_offset & 31)
		{
			destination_x -= (x_offset & 31);
			x_offset &= ~31;
		}
		
		y_offset = ( y_top - si_stipple_p->BorgY)  % stipple_height;
		if( y_offset < 0)
		{
			y_offset += stipple_height;
		}

		/*
		 * Compute the first longword source pointer  to transfer.
		 */
		source_x_offset_p = source_start_p + ((unsigned long )x_offset >> 5U);

		/*
		 * Compute the following parameters.
		 * 1. The number of starting longwords before encountering the first
		 *    complete stipple width.
		 * 2. The number of middle longwords, the full stipple width transfers.
		 * 3. The number of end longwords, the last transfer which is less than
		 *    one full stipple width.
		 */
		if((destination_x +  (stipple_width - x_offset) - 1) < x_right)
		{
			/*
			 * More than 1 transfer is there for this rectangle.
			 */
			if (x_offset > 0)
			{
				startwords = stipple_width_in_longs - 
					((unsigned long)x_offset >> 5U);
				startwidth_x_right = 
					destination_x + stipple_width - x_offset -1;
			}
			else
			{
				startwords = 0;
				startwidth_x_right = destination_x - 1;
			}


			midwidths = (x_right - startwidth_x_right) / stipple_width;
			endwords = (x_right - startwidth_x_right) % stipple_width;
			
			if ( endwords )
			{
				/*
				 * endwords in terms of host data long words.
				 */
				endwidth_x_left = x_right - endwords + 1;
				endwords = ((endwords + 31) & ~31) >> 5U;

				ASSERT(endwidth_x_left ==
					(startwidth_x_right + 1 + (midwidths*stipple_width)));
			}
		}
		else
		{
			/*
			 * This is the only transfer for this rectangle.
			 */
			startwords = x_right - destination_x + 1;
			startwords = ((startwords + 31) & ~31) >> 5U;
			startwidth_x_right = x_right;
			midwidths = 0;
			endwords = 0;
		}

#if (defined(__DEBUG__))
	if (m64_fill_stipple_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(m64_stipple_rectangle_system_memory){\n"
			"\tx_left = %d\n"
			"\tx_right = %d\n"
			"\ty_top = %d\n"
			"\ty_bottom = %d\n"
			"\twidth = %d\n"
			"\theight = %d\n"
			"\tdestination_x = %d\n"
			"\tdestination_y = %d\n"
			"\tstartwords = %d\n"
			"\tmidwidths = %d\n"
			"\tendwords = %d\n"
			"\tx_offset = %d\n"
			"\ty_offset = %d\n"
			"\tstartwidth_x_right = %d\n"
			"\tendwidth_x_left = %d\n"
			"\tsource_start_p = 0x%x\n"
			"\tsource_x_offset_p = 0x%x\n"
			"}\n",
			x_left, x_right, y_top, y_bottom, width, height, destination_x,
			destination_y, startwords, midwidths, endwords, x_offset,
			y_offset, startwidth_x_right, endwidth_x_left,
			(unsigned)source_start_p, (unsigned)source_x_offset_p);
		}
#endif
		/*
		 * Having computed the parameters, start the drawing.
		 * Start with startwords, the fractional width in the left. 
		 */
		if(startwords > 0)
		{
			unsigned long *tmp_source_p;

			M64_WAIT_FOR_FIFO(3);
			register_base_address_p[M64_REGISTER_SC_RIGHT_OFFSET] = 
				startwidth_x_right; 
			*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = 
				y_top | (destination_x << DST_Y_X_DST_X_SHIFT);
			*(register_base_address_p + M64_REGISTER_DST_HEIGHT_WID_OFFSET) = 
				height | ((startwords << 5U) << DST_HEIGHT_WID_DST_WID_SHIFT);

			/*
			 * Wait for the initiator to get executed.
			 */
			M64_WAIT_FOR_FIFO(16);

			curdy = destination_y;
			cursy = y_offset;
			tmp_source_p = source_x_offset_p + 
				(y_offset * stipple_width_in_longs);

			do
			{
				int		stipplelines;
				int		tmplines;

				stipplelines = ((curdy + stipple_height - cursy) > y_bottom )
					? y_bottom - curdy + 1 : stipple_height - cursy;

				tmplines = stipplelines;
				ASSERT(tmplines > 0);
				do
				{
					M64_ASM_TRANSFER_THRO_HOST_DATA(startwords,tmp_source_p);
					
					tmp_source_p += stipple_width_in_longs;
					++cursy;
				} while ( --tmplines > 0);

				curdy += stipplelines;
				cursy = 0;
				tmp_source_p = source_x_offset_p;

			} while ( curdy <= y_bottom );
		}
		ASSERT(!(M64_IS_FIFO_OVERFLOW()));

		if ( midwidths )
		{
			int	const partial_top_lines = 
				(stipple_height - y_offset) > (height) ? height : 
				stipple_height - y_offset;

			int	const mid_heights_count = (height - partial_top_lines) / 
				stipple_height; 

			int const partial_bottom_lines = (height) - 
				(mid_heights_count * stipple_height) - partial_top_lines;

			int const total_partial_top_transfers = 
				partial_top_lines * stipple_width_in_longs;

			int const total_partial_bottom_transfers = 
				partial_bottom_lines * stipple_width_in_longs;

			unsigned long * const top_partial_source_p = source_start_p + 
				(y_offset * stipple_width_in_longs);

			ASSERT(height >= partial_top_lines);
			ASSERT((height) == (partial_bottom_lines + 
				partial_top_lines + (mid_heights_count * stipple_height)));

			curdx = startwidth_x_right + 1;

			/*
			 * do the midwidths next.
			 */
			ASSERT(midwidths > 0);
			ASSERT((total_partial_top_transfers > 0) || 
				(total_partial_bottom_transfers > 0) ||
				(mid_heights_count > 0));
			do
			{
				curdy = destination_y;

				M64_WAIT_FOR_FIFO(3);

				register_base_address_p[M64_REGISTER_SC_RIGHT_OFFSET] = 
					(curdx + stipple_width - 1);

				*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = 
					y_top | (curdx << DST_Y_X_DST_X_SHIFT);

				*(register_base_address_p + 
					M64_REGISTER_DST_HEIGHT_WID_OFFSET) = (height) | 
					shifted_stipple_width_in_pixels;
	
				/*
				 * Wait for the initiator to get executed.
				 */
				M64_WAIT_FOR_FIFO(16);

				if (total_partial_top_transfers > 0)
				{
					M64_ASM_TRANSFER_THRO_HOST_DATA(
						total_partial_top_transfers,
						top_partial_source_p);
				}

				if (mid_heights_count > 0)
				{
					int		i = mid_heights_count;
					do
					{
						M64_ASM_TRANSFER_THRO_HOST_DATA(
							stipple_state_p->transfer_length_in_longwords,
							source_start_p);
					} while (--i > 0); 
				}

				if (total_partial_bottom_transfers > 0)
				{
					M64_ASM_TRANSFER_THRO_HOST_DATA(
						total_partial_bottom_transfers,
						source_start_p);
				}

				curdx += stipple_width;
			} while ( --midwidths > 0);
		}
		ASSERT(!(M64_IS_FIFO_OVERFLOW()));

		if ( endwords )
		{
			unsigned long *tmp_source_p;

			M64_WAIT_FOR_FIFO(3);
			register_base_address_p[M64_REGISTER_SC_RIGHT_OFFSET] = x_right;

			*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = 
				y_top | (endwidth_x_left << DST_Y_X_DST_X_SHIFT);

			*(register_base_address_p + M64_REGISTER_DST_HEIGHT_WID_OFFSET) = 
				(height) | ((endwords << 5U) << DST_HEIGHT_WID_DST_WID_SHIFT);

			/*
			 * Wait for the initiator to get executed.
			 */
			M64_WAIT_FOR_FIFO(16);

			curdy = destination_y;
			cursy = y_offset;
			tmp_source_p = 
				source_start_p + (y_offset * stipple_width_in_longs);

			do
			{
				int		stipplelines;
				int		tmplines;

				stipplelines = ((curdy + stipple_height - cursy) > y_bottom )
					? y_bottom - curdy + 1 : stipple_height - cursy;

				tmplines = stipplelines;
				ASSERT(tmplines > 0);
				do
				{
					M64_ASM_TRANSFER_THRO_HOST_DATA(endwords,tmp_source_p);
					
					tmp_source_p += stipple_width_in_longs;
					++cursy;
				} while ( --tmplines > 0);

				curdy += stipplelines;
				cursy = 0;
				tmp_source_p = source_start_p;

			} while ( curdy <= y_bottom );
		}
		ASSERT(!(M64_IS_FIFO_OVERFLOW()));

		/*
		 * advance to the next destination rectangle.
		 */
		++rect_p;
	}
	while(--count);	

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));

	/*
	 * Restore the registers that have changed.
	 */
	M64_WAIT_FOR_FIFO(2);
	register_base_address_p[M64_REGISTER_DP_PIX_WID_OFFSET] =
		register_values_p[M64_REGISTER_DP_PIX_WID_OFFSET];
	register_base_address_p[M64_REGISTER_DP_MIX_OFFSET] =
		register_values_p[M64_REGISTER_DP_MIX_OFFSET];

	/*
	 * Invalidate the clipping rectangle.
	 */
	screen_state_p->generic_state.screen_current_clip = 
		M64_INVALID_CLIP_RECTANGLE;

	return (SI_SUCCEED);
}

STATIC SIBool
m64_fill_rectangle_tile( SIint32 x_origin, SIint32 y_origin,
	SIint32	count, SIRectOutlineP rect_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	struct m64_tile_state 	*tile_state_p;	
	SIbitmap 				*si_tile_p;

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));

	tile_state_p = &(graphics_state_p->current_tile_state);
	si_tile_p = graphics_state_p->generic_state.si_graphics_state.SGtile;

	if (count <= 0)
	{
		return(SI_SUCCEED);
	}

	/*
	 * First call the tile download function if the tile is not
	 * downloaded already.
	 */
	if (!tile_state_p->tile_downloaded) 
	{
		m64_graphics_state_download_tile(screen_state_p, graphics_state_p,
			si_tile_p);
	}	

	ASSERT(screen_state_p->options_p->rectfill_options &
		M64_OPTIONS_RECTFILL_OPTIONS_USE_TILE_FILL_RECT != 0);

	if (tile_state_p->tile_downloaded == FALSE)
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return(SI_FAIL);
	}

	if ((screen_state_p->options_p->rectfill_options &
		 M64_OPTIONS_RECTFILL_OPTIONS_USE_MONO_PATTERN) &&
		(screen_state_p->options_p->rectfill_options &
		 M64_OPTIONS_RECTFILL_OPTIONS_USE_TILE_SHRINK) &&
		(tile_state_p->is_reduced_tile == TRUE))
	{
		/*
		 * Tiling using mono pattern registers.
		 */

		M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();

		int	two_color_tile_origin_x;
		int	two_color_tile_origin_y;

		SIBool	return_value;
		
		ASSERT(!(si_tile_p->Bwidth & (si_tile_p->Bwidth - 1)) && 
			!(si_tile_p->Bheight & (si_tile_p->Bheight - 1)));

		two_color_tile_origin_x = (si_tile_p->BorgX) & 
			((si_tile_p->Bwidth < DEFAULT_M64_SMALL_STIPPLE_WIDTH) ?
			(si_tile_p->Bwidth - 1) : (DEFAULT_M64_SMALL_STIPPLE_WIDTH - 1));
		two_color_tile_origin_y = (si_tile_p->BorgY) & 
			((si_tile_p->Bheight < DEFAULT_M64_SMALL_STIPPLE_HEIGHT) ?
			(si_tile_p->Bheight - 1) : (DEFAULT_M64_SMALL_STIPPLE_HEIGHT - 1));

		/*
		 * Check if the BorgX and BorgY has changed, in which 
		 * case call the function to rotate the pattern registers.
		 */
		if ((tile_state_p->reduced_tile_state.stipple_origin_x != 
				two_color_tile_origin_x) ||
			(tile_state_p->reduced_tile_state.stipple_origin_y != 
				two_color_tile_origin_y))
		{
			int 	relative_x_origin = 
						two_color_tile_origin_x -
						tile_state_p->reduced_tile_state.stipple_origin_x;
			int		relative_y_origin = 
						two_color_tile_origin_y -
						tile_state_p->reduced_tile_state.stipple_origin_y;

			if (relative_x_origin < 0) 
			{
				relative_x_origin += DEFAULT_M64_SMALL_STIPPLE_WIDTH;
			}
			if (relative_y_origin < 0) 
			{
				relative_y_origin += DEFAULT_M64_SMALL_STIPPLE_HEIGHT;
			}

			M64_ROTATE_PATTERN_REGISTER_BITS(
				&(tile_state_p->reduced_tile_state), relative_x_origin, 
				relative_y_origin);

			tile_state_p->reduced_tile_state.stipple_origin_x = 
				two_color_tile_origin_x;
			tile_state_p->reduced_tile_state.stipple_origin_y = 
				two_color_tile_origin_y;
		}

		/*
		 * Set the foreground and background colors for the two color tile
		 * and restore them after the call.
		 */
		M64_WAIT_FOR_FIFO(2);
		*(register_base_address_p + M64_REGISTER_DP_FRGD_CLR_OFFSET) =
			tile_state_p->color1;
		*(register_base_address_p + M64_REGISTER_DP_BKGD_CLR_OFFSET) =
			tile_state_p->color2;

		return_value =  m64_fill_rectangle_through_mono_pattern_registers(
			x_origin, y_origin, count, rect_p, 
			&(tile_state_p->reduced_tile_state), SGOPQStipple);

		M64_WAIT_FOR_FIFO(2);
		*(register_base_address_p + M64_REGISTER_DP_FRGD_CLR_OFFSET) =
			graphics_state_p->generic_state.si_graphics_state.SGfg;
		*(register_base_address_p + M64_REGISTER_DP_BKGD_CLR_OFFSET) =
			graphics_state_p->generic_state.si_graphics_state.SGbg;

		return (return_value);
	}
	else if ((screen_state_p->options_p->rectfill_options &
		M64_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY) &&
		OMM_LOCK(tile_state_p->offscreen_location_p))
	{
		/*
		 * Tiling using offscreen memory.
		 */
		return(m64_fill_rectangle_tile_offscreen_memory(x_origin, y_origin,
			count, rect_p));
	}
	else
	{
		return(m64_fill_rectangle_tile_system_memory(x_origin, y_origin,
			count, rect_p));
	}
}

STATIC SIBool
m64_fill_rectangle_stipple( SIint32 x_origin, SIint32 y_origin,
	SIint32	count, SIRectOutlineP rect_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	struct m64_stipple_state 	*stipple_state_p;	
	SIbitmap 				*si_stipple_p;

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));

	stipple_state_p = &(graphics_state_p->current_stipple_state);
	si_stipple_p = graphics_state_p->generic_state.si_graphics_state.SGstipple;

	if (count <= 0)
	{
		return(SI_SUCCEED);
	}

	/*
	 * We can't handle pixmaps of depth != 1
	 */
	if (si_stipple_p->BbitsPerPixel != 1)
	{
		return (SI_FAIL);
	}

	/*
	 * First call the stipple download function if the stipple is not
	 * downloaded already.
	 */
	if (!stipple_state_p->stipple_downloaded) 
	{
		m64_graphics_state_download_stipple(screen_state_p, graphics_state_p,
			si_stipple_p);
	}	

	ASSERT(screen_state_p->options_p->rectfill_options &
		M64_OPTIONS_RECTFILL_OPTIONS_USE_STIPPLE_FILL_RECT != 0);

	if (stipple_state_p->stipple_downloaded == FALSE)
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return(SI_FAIL);
	}

	if ((screen_state_p->options_p->rectfill_options &
		 M64_OPTIONS_RECTFILL_OPTIONS_USE_MONO_PATTERN) &&
		(stipple_state_p->is_small_stipple == TRUE))
	{
		/*
		 * Stippling using pattern registers.
		 */
		int	stipple_origin_x = (si_stipple_p->BorgX) & 
			(DEFAULT_M64_SMALL_STIPPLE_WIDTH - 1);
		int	stipple_origin_y = (si_stipple_p->BorgY) & 
			(DEFAULT_M64_SMALL_STIPPLE_HEIGHT - 1);
		
		/*
		 * Check if the BorgX and BorgY has changed, in which 
		 * case call the function to rotate the pattern registers.
		 */
		if ((stipple_state_p->stipple_origin_x != stipple_origin_x) ||
			(stipple_state_p->stipple_origin_y != stipple_origin_y))
		{
			int 	relative_x_origin = 
						stipple_origin_x - stipple_state_p->stipple_origin_x;
			int		relative_y_origin = 
						stipple_origin_y - stipple_state_p->stipple_origin_y;

			if (relative_x_origin < 0) 
			{
				relative_x_origin += DEFAULT_M64_SMALL_STIPPLE_WIDTH;
			}
			if (relative_y_origin < 0) 
			{
				relative_y_origin += DEFAULT_M64_SMALL_STIPPLE_HEIGHT;
			}

			M64_ROTATE_PATTERN_REGISTER_BITS(stipple_state_p, 
					relative_x_origin, relative_y_origin);

			stipple_state_p->stipple_origin_x = stipple_origin_x;
			stipple_state_p->stipple_origin_y = stipple_origin_y;
		}

		return (m64_fill_rectangle_through_mono_pattern_registers(
			x_origin, y_origin, count, rect_p, stipple_state_p,
			graphics_state_p->generic_state.si_graphics_state.SGstplmode));
	}
	else
	{
		/*
		 * Stippling through system memory.
		 */
		return (m64_fill_rectangle_stipple_system_memory(
			x_origin, y_origin, count, rect_p));
	}
	/*NOTREACHED*/
	/*CONSTANTCONDITION*/
	ASSERT(0);
	return(SI_FAIL);
}

STATIC SIBool
m64_fill_rectangle_compat(SIint32 count, SIRectP pRect)
{
	int		ret_val;
	boolean is_local_allocation;
	
	SIRectOutlineP	p_new_rect, tmp_new_rect_p;
	SIRectP			tmp_old_rect_p;
	SIint32 		tmpcount;

	SIBool (*called_function_p)(SIint32, SIint32, SIint32,
								SIRectOutlineP);
	SIRectOutline				/* for fast allocations */
		localRectangles[DEFAULT_M64_COMPATIBILITY_LOCAL_RECTANGLE_COUNT];
	
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();

#if (defined(__DEBUG__))
	if (m64_fill_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_fill_rectangle_compat)\n"
			"{\n"
			"\tcount = %ld\n"
			"\tpRect = %p\n"
			"}\n",
			count, (void *) pRect);
	}
#endif

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	/*
	 * Poor man's alloca ...
	 */
	if (count > DEFAULT_M64_COMPATIBILITY_LOCAL_RECTANGLE_COUNT)
	{
		is_local_allocation = FALSE;
		p_new_rect = allocate_memory(sizeof(SIRectOutline) * count);
	}
	else
	{
		is_local_allocation = TRUE;
		p_new_rect = &localRectangles[0];
	}
	

	tmp_new_rect_p = p_new_rect;
	tmp_old_rect_p = pRect;
	tmpcount = count;

	/*
	 * Reformat the arguments.
	 */
	while (tmpcount--)
	{
		tmp_new_rect_p->x = tmp_old_rect_p->ul.x;
		tmp_new_rect_p->y = tmp_old_rect_p->ul.y;
		tmp_new_rect_p->width = tmp_old_rect_p->lr.x - 
			tmp_old_rect_p->ul.x;
		tmp_new_rect_p->height = tmp_old_rect_p->lr.y - 
			tmp_old_rect_p->ul.y;
		
		tmp_new_rect_p++;
		tmp_old_rect_p++;
	}

	/*
	 * Determine the function to call ...
	 */
	switch (graphics_state_p->generic_state.si_graphics_state.SGfillmode)
	{
		case SGFillSolidFG:
		case SGFillSolidBG :
			if (screen_state_p->options_p->rectfill_options &
				M64_OPTIONS_RECTFILL_OPTIONS_USE_SOLID_FILL_RECT)
			{
				called_function_p = m64_fill_rectangle_solid; 
			}
			break;

		case SGFillTile :
			if (screen_state_p->options_p->rectfill_options &
				M64_OPTIONS_RECTFILL_OPTIONS_USE_TILE_FILL_RECT)
			{
				called_function_p = m64_fill_rectangle_tile;
			}
			break;

		case SGFillStipple:
			if (screen_state_p->options_p->rectfill_options &
				M64_OPTIONS_RECTFILL_OPTIONS_USE_STIPPLE_FILL_RECT)
			{
				called_function_p = m64_fill_rectangle_stipple;
			}
			break;
		default :
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
	}

	ret_val =
		(*called_function_p) (0, 0, count, p_new_rect);

	if (is_local_allocation == FALSE)
	{
		free_memory (p_new_rect);
	}
	
	return (ret_val);
}

/*
 * Handle a change of graphics state.
 */
function void
m64_fill__gs_change__(void)
{
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	M64_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));
	
#if (defined(__DEBUG__))
	if (m64_fill_debug)
	{
		(void) fprintf(debug_stream_p, "(m64_fill__gs_change__) {}\n");
	}
#endif

	/*
	 * Set the default pointer to the fill function.
	 */
	screen_state_p->generic_state.screen_functions_p->si_poly_fillrect =
		(SIBool (*)(SIint32 , SIint32 , SIint32, SIRectOutlineP))
		m64_no_operation_fail;


	if (screen_state_p->generic_state.screen_sdd_version_number >=
		DM_SI_VERSION_1_1)
	{
		switch (graphics_state_p->generic_state.si_graphics_state.SGfillmode)
		{
			case SGFillSolidFG:
			case SGFillSolidBG :
				if (screen_state_p->options_p->rectfill_options &
					M64_OPTIONS_RECTFILL_OPTIONS_USE_SOLID_FILL_RECT)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_poly_fillrect = m64_fill_rectangle_solid;
				}
			break;

		case SGFillTile:
			if (screen_state_p->options_p->rectfill_options &
				M64_OPTIONS_RECTFILL_OPTIONS_USE_TILE_FILL_RECT)
			{
				screen_state_p->generic_state.screen_functions_p->
					si_poly_fillrect = m64_fill_rectangle_tile;
			}
			break;
		case SGFillStipple:
			if (screen_state_p->options_p->rectfill_options &
				M64_OPTIONS_RECTFILL_OPTIONS_USE_STIPPLE_FILL_RECT)
			{
				screen_state_p->generic_state.screen_functions_p->
					si_poly_fillrect = m64_fill_rectangle_stipple;
			}
			break;
		default :
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
		}
	}
	else /* Backward compatibility support. */
	{
		screen_state_p->generic_state.screen_functions_p->si_poly_fillrect = 
			(SIBool (*)(SIint32, SIint32, SIint32, SIRectOutlineP))
			m64_fill_rectangle_compat;
	}
	return;
}

/*
 * Initialization.
 */
function void
m64_fill__initialize__(SIScreenRec *si_screen_p,
						struct m64_options_structure *options_p)
{
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	int tile_best_width = 0, tile_best_height = 0;
	int stipple_best_width = 0, stipple_best_height = 0;

	flags_p->SIavail_fpoly = RECTANGLE_AVAIL|
		TILE_AVAIL | OPQSTIPPLE_AVAIL | STIPPLE_AVAIL;

	if (options_p->tile_best_size)
	{
		if (sscanf(options_p->tile_best_size, "%ix%i",
				   &tile_best_width, &tile_best_height) != 2)
		{
			(void) fprintf(stderr,
				M64_BAD_BEST_TILE_SIZE_SPECIFICATION_OPTION_MESSAGE,
				options_p->tile_best_size);
		}
	}
	else
	{
		flags_p->SItilewidth = tile_best_width ;
		flags_p->SItileheight = tile_best_height ;
	}

	if (options_p->stipple_best_size)
	{
		if (sscanf(options_p->stipple_best_size, "%ix%i",
				   &stipple_best_width, &stipple_best_height) != 2)
		{
			(void) fprintf(stderr,
				M64_BAD_BEST_STIPPLE_SIZE_SPECIFICATION_OPTION_MESSAGE,
				options_p->stipple_best_size);
		}
	}
	else
	{
		flags_p->SIstipplewidth = stipple_best_width;
		flags_p->SIstippleheight = stipple_best_height;
	}

	return;
}
