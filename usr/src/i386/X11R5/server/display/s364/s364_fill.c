/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_fill.c	1.3"

/***
 ***	NAME
 ***
 ***		s364_fill.c : File for SI rectangle fill entry points and support
 ***                      routines.
 ***
 ***	SYNOPSIS
 ***
 ***	DESCRIPTION
 ***
 ***		This module handles only the rectangle fill entry points of
 ***	the SI defined polygon fill entry points.
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
export boolean s364_fill_debug=0;
export boolean s364_fill_solid_debug = 0;
export boolean s364_fill_tile_debug = 0;
export boolean s364_fill_stipple_debug = 0;
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
#include "s364_gbls.h"
#include "s364_opt.h"
#include "s364_regs.h"
#include "s364_asm.h"
#include "s364_state.h"
#include "s364_gs.h"

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
 * PURPOSE
 *
 *		Routine to fill rectangles with foreground or background color
 * when the fill mode is solid.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_fill_rectangle_solid(SIint32 x_origin, SIint32 y_origin, 
	SIint32 count, SIRectOutlineP rect_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();

	const int clip_left =
		screen_state_p->generic_state.screen_clip_left;
	const int clip_right =
		screen_state_p->generic_state.screen_clip_right;
	const int clip_top =
		screen_state_p->generic_state.screen_clip_top;
	const int clip_bottom =
		screen_state_p->generic_state.screen_clip_bottom;

	/*
	 * the command to programmed in the command register.
	 */
	const unsigned short command =
		S3_CMD_WRITE |
		S3_CMD_PX_MD_THRO_PLANE |  
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |
		S3_CMD_BUS_WIDTH_32 |
		S3_CMD_TYPE_RECTFILL;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));


#if (defined(__DEBUG__))
	if (s364_fill_solid_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_fill_rectangle_solid) {\n"
			"\tx_origin = %ld\n"
			"\ty_origin = %ld\n"
			"\tcount = %ld\n"
			"\trect_p = %p\n"
			"\t}\n",
			x_origin, y_origin, count, (void *) rect_p);
	}
#endif

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

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
	S364_STATE_SET_CLIP_RECTANGLE_TO_SI_CLIP();

	/*
	 * At this point of time the following registers should contain 
	 * the correct values for
	 * WRITE_MASK
	 * FG_COLOR
	 * BG_COLOR
	 */

	S3_INLINE_WAIT_FOR_FIFO(1);

	/*
  	 * Pixcntl is set to use foreground mix register, by default.
	 */
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
		enhanced_cmds_p->frgd_mix |
		((graphics_state_p->generic_state.si_graphics_state.SGfillmode == 
			SGFillSolidFG) ? S3_CLR_SRC_FRGD_COLOR: S3_CLR_SRC_BKGD_COLOR),
		unsigned short);

	/*
	 * loop through the list of rectangles.
	 */
	for (;count--;rect_p++)
	{
		register short xl = rect_p->x + x_origin;
		register short xr = xl + rect_p->width - 1; /* inclusive */
		register short yt = rect_p->y + y_origin;
		register short yb = yt + rect_p->height - 1; /* inclusive */
		int width;
		int height;


#if (defined(__DEBUG__))
		if (s364_fill_debug)
		{
			(void) fprintf(debug_stream_p,
				"(s3_fill_rectangle_solid) {\n"
				"\t\txl = %d\n"
				"\t\tyt = %d\n"
				"\t\txr = %d\n"
				"\t\tyb = %d\n"
				"\t}\n",
				xl, yt, xr, yb);
		}
#endif

		if ((xl > xr) || (yt > yb) || (xl > clip_right) ||
			(xr < clip_left) || (yt > clip_bottom) || (yb < clip_top))
		{
#if (defined(__DEBUG__))
			if (s364_fill_debug)
			{
				(void) fprintf(debug_stream_p,
				"(s3_fill_rectangle_solid) {\n"
							   "\t\t(out of clip bounds)\n\t}\n");
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
            xl = (short) clip_left;
        }
		if (xr > clip_right)
        {
            xr = (short) clip_right;
        }
        if (yt < clip_top)
        {
            yt = (short) clip_top;
        }
        if (yb > clip_bottom)
        {
            yb = (short) clip_bottom;
        }

		height = yb - yt + 1;
		width =  xr - xl + 1;

		if (width <= 0 || height <= 0)
		{
			continue;
		}

		S3_INLINE_WAIT_FOR_FIFO(5);

		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_CUR_X, xl, unsigned short);
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_CUR_Y, yt, unsigned short);

		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
			((height -1) & S3_MULTIFUNC_VALUE_BITS), unsigned short);
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 
			(width - 1), unsigned short);

		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
			command, unsigned short);
	}

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

#if (defined(__DEBUG__))	
	S3_WAIT_FOR_GE_IDLE();
#endif

	return (SI_SUCCEED);
}

/*
 * PURPOSE
 *
 *	Fill rectangles using the currnet graphics state's stipple pattern.
 * This is called when the stipple is reducable to 8x8 or the stipple 
 * size is less than or equal to 8x8.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_fill_rectangle_stipple_ge_patblt(
	SIint32 x_origin, SIint32 y_origin,
	SIint32	count, SIRectOutlineP rect_p, 
	struct s364_stipple_state *stipple_state_p,
	int	flags)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();

	/*
	 * the command to programmed in the command register.
	 */
	const unsigned short command =
		S3_CMD_WRITE | 
		S3_CMD_PX_MD_ACROSS_PLANE |
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |
		S3_CMD_LSB_FIRST |
		S3_CMD_BUS_WIDTH_32 |
		S3_CMD_TYPE_PATTERNFILL;


    const int screen_clip_left = 
		screen_state_p->generic_state.screen_clip_left;
    const int screen_clip_right = 
		screen_state_p->generic_state.screen_clip_right;
    const int screen_clip_top = 
		screen_state_p->generic_state.screen_clip_top;
    const int screen_clip_bottom = 
		screen_state_p->generic_state.screen_clip_bottom;
	int	offscreen_stipple_x;
	int	offscreen_stipple_y;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));

#if (defined(__DEBUG__))
	if (s364_fill_tile_debug || s364_fill_stipple_debug)
	{
		(void) fprintf(debug_stream_p,
		   "(s364_fill_rectangle_tile_ge_patblt)\n"
		   "{\n"
		   "\tx_origin = %ld\n"
		   "\ty_origin = %ld\n"
		   "\tcount = %ld\n"
		   "\trect_p = %p\n",
		   x_origin,
		   y_origin,
		   count,
		   (void *) rect_p);
	}
#endif

	ASSERT(!S3_IS_FIFO_OVERFLOW());
	ASSERT(count > 0);
	ASSERT(stipple_state_p->is_small_stipple == TRUE);


	/*
	 * Set the clipping rectangle to select the entire drawing area.
	 */
	S364_STATE_SET_CLIP_RECTANGLE_TO_VIRTUAL_SCREEN();

	/*
	 * Make sure the stipple dimensions are a power of 2
	 */
	ASSERT(!(stipple_state_p->offscreen_width & 
		(stipple_state_p->offscreen_width -1)));
	ASSERT(!(stipple_state_p->offscreen_height & 
		(stipple_state_p->offscreen_height -1)));

	offscreen_stipple_x = stipple_state_p->offscreen_location_x;
	offscreen_stipple_y = stipple_state_p->offscreen_location_y;

	if (!OMM_LOCK(stipple_state_p->offscreen_location_p))
	{
#if	defined(__DEBUG__)
		if (s364_fill_debug)
		{
			(void)fprintf(debug_stream_p,"#OMM lock failed!");
		}
#endif
		return (SI_FAIL);
	}
	
	S3_INLINE_WAIT_FOR_FIFO(4);

	/*
	 * Set the pixel control register to use of stipple data for selecting
	 * the foreground and background mix registers.
	 */
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
		PIX_CNTL_DT_EX_SRC_VID_MEM, unsigned short);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
		enhanced_cmds_p->frgd_mix | S3_CLR_SRC_FRGD_COLOR, unsigned short);

	/*
	 * If stipple_type is not zero the request is for a type of stippling
	 * different from the Graphics State. If forcetype is SGOPQStipple
	 * then perform opaque stippling, else perform transparent stippling.
	 * Remember the rops are already set correctly for opaque stippling.
	 */

	if (graphics_state_p->generic_state.si_graphics_state.SGstplmode
		== SGStipple)
	{
		/*
		 * For transparent case program BG_ROP to be DST.
		 */
		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,
			S3_MIX_FN_LEAVE_C_AS_IS, unsigned short);
	}
	else 
	{
		ASSERT(graphics_state_p->generic_state.si_graphics_state.SGstplmode
			== SGOPQStipple);

		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,
			enhanced_cmds_p->bkgd_mix, unsigned short);
	}

	/*
	 * The offscreen stipple occupies only one plane, set the readmask
	 * for reading only that plane
	 */
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_RD_MASK,
		stipple_state_p->offscreen_location_p->planemask, unsigned long);

	S3_INLINE_WAIT_FOR_FIFO(2);
	S3_UPDATE_MMAP_REGISTER( S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
		offscreen_stipple_x, unsigned short);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
		offscreen_stipple_y, unsigned short);

	do
	{
		/*
		 * destination rectangle coordinates.
		 */
		int x_left = rect_p->x + x_origin;
		int x_right = x_left + rect_p->width - 1;
		int y_top = rect_p->y + y_origin;
		int y_bottom = y_top + rect_p->height - 1;
		int	width,height;

		/*
		 * Check if the rectangle is outside the current clipping
		 * bounds. Discard the rectangle if so.
		 */
		if((x_left > screen_clip_right) || (x_right < screen_clip_left) ||
		   (y_top > screen_clip_bottom) || (y_bottom < screen_clip_top) ||
		   (y_bottom < y_top) || ( x_right < x_left ))
		{
#if (defined(__DEBUG__))
			if (s364_fill_tile_debug || s364_fill_stipple_debug)
			{
				(void) fprintf(debug_stream_p,
					"\t\tRectangle out of bounds\n");
			}
#endif
			++rect_p;
			continue;
		}
			
		/*
         * Software clip the destination rectangle to clipping rectangle
		 * bounds. 
         */
        if (x_left < screen_clip_left)
        {
            x_left = screen_clip_left;
        }
		if (x_right > screen_clip_right)
        {
            x_right = screen_clip_right;
        }
        if (y_top < screen_clip_top)
        {
            y_top = screen_clip_top;
        }
        if (y_bottom > screen_clip_bottom)
        {
            y_bottom = screen_clip_bottom;
        }

		height = y_bottom - y_top + 1;
		width =  x_right - x_left + 1;

		if (width <= 0 || height <= 0)
		{
			++rect_p;
			continue;
		}

		/*
		 * The clip rectangle has to be set to the inner rectangle
		 */
		S3_INLINE_WAIT_FOR_FIFO(5);
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP,
			x_left, unsigned short);
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP,
			y_top, unsigned short);
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
			(width - 1), unsigned short);
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
			((height - 1) & S3_MULTIFUNC_VALUE_BITS), unsigned short);
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_CMD, command, unsigned short);

		++rect_p;

	} while (--count);

#if (defined(__DEBUG__))
	if (s364_fill_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	OMM_UNLOCK(stipple_state_p->offscreen_location_p);

	S3_INLINE_WAIT_FOR_FIFO(2);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_RD_MASK,
		enhanced_cmds_p->read_mask, unsigned long);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
		PIX_CNTL_DT_EX_SRC_FRGD_MIX, unsigned short);

	ASSERT(!S3_IS_FIFO_OVERFLOW());
	return (SI_SUCCEED);
}

/*
 * s364_fill_rectangle_offscreen_helper
 *
 * PURPOSE
 *
 * Helper for stippling and tiling rectangles from offscreen source
 * Assumptions: offscreen_bitmap_width x offscreen_bitmap_height
 * contains integral number of tile/stipples.
 *
 * RETURN VALUE
 *
 *		None.
 */
STATIC void
s364_fill_rectangle_offscreen_helper( SIbitmapP source_bitmap_p,
	int offscreen_bitmap_x_location, int offscreen_bitmap_y_location,
	int offscreen_bitmap_width, int offscreen_bitmap_height,
	int destination_x, int destination_y,
	int destination_width, int destination_height, unsigned short command_flag)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();

	int		fractional_width_start;
	int		fractional_width_end;
	int		middle_widths_count;
	int		fractional_height_start;
	int		fractional_height_end;
	int		middle_heights_count;
	int		offscreen_fractional_x_position;
	int		offscreen_fractional_y_position;

	int		current_destination_x = destination_x;

	const unsigned short command =
		S3_CMD_WRITE |
		command_flag |
		S3_CMD_DIR_TYPE_AXIAL |
		S3_CMD_DRAW |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |
		S3_CMD_LSB_FIRST |
		S3_CMD_BUS_WIDTH_32 |
		S3_CMD_TYPE_BITBLT;
					
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));

	/*
	 * Compute the coordinates in the offscreen bitmap corresponding 
	 * to the first destination pixel ( top left ).
	 */
	fractional_width_start = ( destination_x - source_bitmap_p->BorgX)  % 
		offscreen_bitmap_width;

	if( fractional_width_start < 0)
	{
		fractional_width_start += offscreen_bitmap_width;
	}

	offscreen_fractional_x_position = 
		offscreen_bitmap_x_location + fractional_width_start;

	if (fractional_width_start > 0)
	{
		fractional_width_start = 
			offscreen_bitmap_width - fractional_width_start;
		fractional_width_start = 
			(fractional_width_start > destination_width?
		  	 destination_width : fractional_width_start);
	}

	fractional_height_start = ( destination_y - source_bitmap_p->BorgY)  % 
		offscreen_bitmap_height;

	if( fractional_height_start < 0)
	{
		fractional_height_start += offscreen_bitmap_height;
	}

	offscreen_fractional_y_position = 
		offscreen_bitmap_y_location + fractional_height_start;

	if (fractional_height_start > 0)
	{
		fractional_height_start = 
			offscreen_bitmap_height - fractional_height_start;
		fractional_height_start = 
			(fractional_height_start > destination_height?
			 destination_height : fractional_height_start);
	}

	/*
	 * Compute the number of full offscreen heights and widths that
	 * would fit in the destination rectangle.
	 */
	middle_widths_count = (destination_width - fractional_width_start) /
		offscreen_bitmap_width;
	middle_heights_count = (destination_height - fractional_height_start) /
		offscreen_bitmap_height;

	/*
	 * Compute the fractional quantities at the right and bottom ends.
	 */
	fractional_width_end = (destination_width - fractional_width_start) %
		offscreen_bitmap_width;
	fractional_height_end = (destination_height - fractional_height_start) %
		offscreen_bitmap_height;

	/*
	 * Program the top left of the destination coordinates.
	 */
	S3_WAIT_FOR_FIFO(2);

	S3_UPDATE_MMAP_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP,
		current_destination_x, unsigned short);
	S3_UPDATE_MMAP_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP,
		destination_y, unsigned short);

#if (defined(__DEBUG__))
	if (s364_fill_stipple_debug || s364_fill_tile_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_fill_rectangle_offscreen_helper)\n"
			"{\n"
		   	"\toffscreen_bitmap_x_location = %d\n"
		  	"\toffscreen_bitmap_y_location = %d\n"
		   	"\toffscreen_bitmap_x_width = %d\n"
		  	"\toffscreen_bitmap_y_height = %d\n"
		  	"\tdestination_x = %d\n"
		  	"\tdestination_y = %d\n"
		  	"\tdestination_width = %d\n"
		  	"\tdestination_height = %d\n"
		  	"\tcommand_flag = 0x%x\n"
			"\tfractional_width_start = %d\n"
			"\tfractional_width_end = %d\n"
			"\tmiddle_widths_count = %d\n"
			"\tfractional_height_start = %d\n"
			"\tfractional_height_end = %d\n"
			"\tmiddle_heights_count = %d\n"
			"\toffscreen_fractional_x_position = %ld\n"
			"\toffscreen_fractional_y_position = %ld\n"
			"}\n",
			offscreen_bitmap_x_location,
			offscreen_bitmap_y_location,
			offscreen_bitmap_width,
			offscreen_bitmap_height,
			destination_x,
			destination_y,
			destination_width,
			destination_height,
			command_flag,
			fractional_width_start,
			fractional_width_end,
			middle_widths_count,
			fractional_height_start,
			fractional_height_end,
			middle_heights_count,
			offscreen_fractional_x_position,
			offscreen_fractional_y_position);
	}
#endif

	/*
	 * Do the first vertical strip.
	 */
	if (fractional_width_start > 0)
	{
		S3_WAIT_FOR_FIFO(2);

		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
			offscreen_fractional_x_position, unsigned short);
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
			(fractional_width_start - 1), unsigned short);

		if(fractional_height_start > 0)
		{
			S3_WAIT_FOR_FIFO(3);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
				offscreen_fractional_y_position, unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
				((fractional_height_start - 1) & S3_MULTIFUNC_VALUE_BITS), 
				unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CMD, command, unsigned short);
		}

		if(middle_heights_count > 0)
		{
			int tmp_middle_heights_count = middle_heights_count;

			S3_WAIT_FOR_FIFO(2);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
				offscreen_bitmap_y_location, unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
				((offscreen_bitmap_height - 1) & 
					S3_MULTIFUNC_VALUE_BITS),
				unsigned short);

			do
			{
				S3_WAIT_FOR_FIFO(1);

				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CMD,
					command, unsigned short);
			}while(--tmp_middle_heights_count > 0);
		}

		if(fractional_height_end > 0)
		{
			S3_WAIT_FOR_FIFO(3);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
				offscreen_bitmap_y_location, unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
				((fractional_height_end - 1) & S3_MULTIFUNC_VALUE_BITS),
				unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CMD, command, unsigned short);
		}
		current_destination_x += fractional_width_start;
	}

	/*
	 * Do the middle vertical strips that take full offscreen tile widths.
	 */
	if (middle_widths_count > 0)
	{
		S3_WAIT_FOR_FIFO(2);

		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
			offscreen_bitmap_x_location, unsigned short);
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
			(offscreen_bitmap_width - 1), unsigned short);

		do
		{
			S3_WAIT_FOR_FIFO(2);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP,
				current_destination_x, unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP,
				destination_y, unsigned short);

			if(fractional_height_start > 0)
			{
				S3_WAIT_FOR_FIFO(3);

				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
					offscreen_fractional_y_position, unsigned short);
				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
					((fractional_height_start - 1) & 
						S3_MULTIFUNC_VALUE_BITS), 
					unsigned short);

				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CMD,
					command, unsigned short);
			}

			if(middle_heights_count > 0)
			{
				int tmp_middle_heights_count = middle_heights_count;

				S3_WAIT_FOR_FIFO(2);

				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
					offscreen_bitmap_y_location, unsigned short);
				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
					((offscreen_bitmap_height - 1) & S3_MULTIFUNC_VALUE_BITS),
					unsigned short);

				do
				{
					S3_WAIT_FOR_FIFO(1);

					S3_UPDATE_MMAP_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, 
						command, unsigned short);
				}while(--tmp_middle_heights_count > 0);
			}

			if(fractional_height_end > 0)
			{
				S3_INLINE_WAIT_FOR_FIFO(3);

				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
					offscreen_bitmap_y_location, unsigned short);
				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
					((fractional_height_end - 1) & 
						S3_MULTIFUNC_VALUE_BITS),
					unsigned short);

				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CMD,
					command, unsigned short);
			}
			current_destination_x += offscreen_bitmap_width;
		} while (--middle_widths_count > 0);
	}

	/*
	 * Do the last fractional vertical strip.
	 */
	if (fractional_width_end > 0)
	{
		S3_WAIT_FOR_FIFO(4);

		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP,
			current_destination_x, unsigned short);
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP,
			destination_y, unsigned short);
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
			offscreen_bitmap_x_location, unsigned short);
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
			(fractional_width_end - 1), unsigned short);

		if(fractional_height_start > 0)
		{
			S3_WAIT_FOR_FIFO(3);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
				offscreen_fractional_y_position, unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
				((fractional_height_start - 1) & S3_MULTIFUNC_VALUE_BITS),
				unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CMD,
				command, unsigned short);
		}

		if(middle_heights_count > 0)
		{
			int tmp_middle_heights_count = middle_heights_count;

			S3_WAIT_FOR_FIFO(2);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
				offscreen_bitmap_y_location, unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
				((offscreen_bitmap_height - 1) & S3_MULTIFUNC_VALUE_BITS),
				unsigned short);

			do
			{
				S3_WAIT_FOR_FIFO(1);

				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CMD,
					command, unsigned short);
			}while(--tmp_middle_heights_count);
		}

		if(fractional_height_end > 0)
		{
			S3_WAIT_FOR_FIFO(3);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
				offscreen_bitmap_y_location, unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
				((fractional_height_end - 1) & S3_MULTIFUNC_VALUE_BITS),
				unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CMD,
				command, unsigned short);
		}
	}

	return;
}
/*
 * PURPOSE
 *
 * Fillrect routine for SGfillmode = SGfillTile and tile downloaded in
 * offscreen memory.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_fill_rectangle_tile_offscreen_memory( SIint32 x_origin, SIint32 y_origin,
	SIint32	count, SIRectOutlineP rect_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();

	int offscreen_tile_x_location;
	int offscreen_tile_y_location;
	int offscreen_tile_height;
	int offscreen_tile_width;

    const int screen_clip_left = 
		screen_state_p->generic_state.screen_clip_left;
    const int screen_clip_right = 
		screen_state_p->generic_state.screen_clip_right;
    const int screen_clip_top = 
		screen_state_p->generic_state.screen_clip_top;
    const int screen_clip_bottom = 
		screen_state_p->generic_state.screen_clip_bottom;
	
	struct s364_tile_state *tile_state_p;	
	SIbitmap *si_tile_p;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));
	
	tile_state_p = &(graphics_state_p->current_tile_state);

	si_tile_p =
		graphics_state_p->generic_state.si_graphics_state.SGtile;
	
	ASSERT(IS_OBJECT_STAMPED(S364_TILE_STATE, tile_state_p));
#if (defined(__DEBUG__))
	if (s364_fill_debug)
	{
		(void) fprintf(debug_stream_p,
		   "(s364_fill_rectangle_tile_offscreen_memory)\n"
		   "{\n"
		   "\tx_origin = %ld\n"
		   "\ty_origin = %ld\n"
		   "\tcount = %ld\n"
		   "\trect_p = %p\n",
		   x_origin,
		   y_origin,
		   count,
		   (void *) rect_p);
	}
#endif
	ASSERT(!S3_IS_FIFO_OVERFLOW());

	if (!tile_state_p->tile_downloaded) 
	{
		s364_graphics_state_download_tile(screen_state_p,
			graphics_state_p,si_tile_p);
	}	

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	if (!OMM_LOCK(tile_state_p->offscreen_location_p))
	{
		/*
		 * We have to try system memory tiling
		 */
#if	defined(__DEBUG__)
		if (s364_fill_debug)
		{
			(void)fprintf(debug_stream_p,"#OMM lock failed!");
		}
#endif
		return (SI_FAIL);
	}

	/*
	 * Select the color source to be display memory.
  	 * Pixcntl is set to use foreground mix register, by default.
	 */

	S3_WAIT_FOR_FIFO(1);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
		enhanced_cmds_p->frgd_mix | S3_CLR_SRC_VIDEO_DATA, unsigned short);

	offscreen_tile_x_location = tile_state_p->offscreen_location_x;
	offscreen_tile_y_location = tile_state_p->offscreen_location_y;
	offscreen_tile_width = si_tile_p->Bwidth;
	offscreen_tile_height = si_tile_p->Bheight;
	
	/*
	 * Set the clipping rectangle to select the entire drawing area.
	 */
	S364_STATE_SET_CLIP_RECTANGLE_TO_VIRTUAL_SCREEN();

	/*
	 * loop through the list of rectangles.
	 */
	do
	{
		/*
		 * destination rectangle coordinates.
		 */
		int x_left = rect_p->x + x_origin;
		int x_right = x_left + rect_p->width - 1; /* inclusive */
		int y_top = rect_p->y + y_origin;
		int y_bottom = y_top + rect_p->height - 1; /* inclusive */

		/*
		 * Check if the rectangle is outside the current clipping
		 * bounds. Discard the rectangle if so.
		 */
		if((x_left > screen_clip_right) || (x_right < screen_clip_left) ||
		   (y_top > screen_clip_bottom) || (y_bottom < screen_clip_top) ||
		   (y_bottom < y_top) || ( x_right < x_left ))
		{
#if (defined(__DEBUG__))
			if (s364_fill_debug)
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
        if (x_left < screen_clip_left)
        {
            x_left = screen_clip_left;
        }
		if (x_right > screen_clip_right)
        {
            x_right = screen_clip_right;
        }
        if (y_top < screen_clip_top)
        {
            y_top = screen_clip_top;
        }
        if (y_bottom > screen_clip_bottom)
        {
            y_bottom = screen_clip_bottom;
        }

		s364_fill_rectangle_offscreen_helper(
			si_tile_p,
			offscreen_tile_x_location, offscreen_tile_y_location,
			offscreen_tile_width, offscreen_tile_height,
			x_left, y_top, 
			x_right - x_left + 1, y_bottom - y_top + 1,
			S3_CMD_PX_MD_THRO_PLANE);

		++rect_p;
	}while(--count);	

#if (defined(__DEBUG__))
	if(s364_fill_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	ASSERT(!S3_IS_FIFO_OVERFLOW());
	return (SI_SUCCEED);
}
	
/*
 * PURPOSE
 *
 * Fillrect routine for SGfillmode = SGfillTile and tile downloaded in
 * system memory. i.e., tile not in offscreen/pattern registers.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_fill_rectangle_tile_system_memory( SIint32 x_origin, SIint32 y_origin,
	SIint32	count, SIRectOutlineP rect_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();

	unsigned int long_word_boundary_mask = 
		(1U << screen_state_p->pixels_per_long_shift) - 1;

    const int screen_clip_left = 
		screen_state_p->generic_state.screen_clip_left;
    const int screen_clip_right = 
		screen_state_p->generic_state.screen_clip_right;
    const int screen_clip_top = 
		screen_state_p->generic_state.screen_clip_top;
    const int screen_clip_bottom = 
		screen_state_p->generic_state.screen_clip_bottom;

	SIbitmap 					*si_tile_p;
	struct s364_tile_state 	*tile_state_p;

	int						tile_height;		/*in pixels */
	int						tile_width;			/*in pixels */
	unsigned short			rounded_tile_width;
	unsigned short			tile_width_in_longs;

	/*
	 * Pointer to the (0,0) coords in source tile.
	 * Pointer to the (destination_x,0) coords in source tile.
	 */
	unsigned long			*source_start_p;
	unsigned long			*source_x_offset_p;

	const unsigned short command =
		S3_CMD_WRITE | 
		S3_CMD_PX_MD_THRO_PLANE |
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |
		S3_CMD_USE_WAIT_YES |
		S3_CMD_BUS_WIDTH_32 |
		S3_CMD_LSB_FIRST |
		S3_CMD_TYPE_RECTFILL;

	tile_state_p = &(graphics_state_p->current_tile_state);
	si_tile_p = graphics_state_p->generic_state.si_graphics_state.SGtile;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_TILE_STATE, tile_state_p));
	ASSERT(!S3_IS_FIFO_OVERFLOW());

#if (defined(__DEBUG__))
	if (s364_fill_tile_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_fill_rectangle_tile_system_memory)\n"
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
	 * Make sure that the tilestate is already initialized, if not 
	 * call the download function now.
	 */
	if (!tile_state_p->tile_downloaded) 
	{
		s364_graphics_state_download_tile(screen_state_p,
			graphics_state_p,
			graphics_state_p->generic_state.si_graphics_state.SGtile);
	}	

	ASSERT(tile_state_p->is_reduced_tile == FALSE);
	ASSERT(tile_state_p->tile_downloaded == TRUE);

	/*
	 * SI tile's width, height and bitmap width rounded off to longwords.
	 */
	tile_width = si_tile_p->Bwidth;
	tile_height = si_tile_p->Bheight;
	rounded_tile_width = ((unsigned)(tile_width + long_word_boundary_mask) & 
		~long_word_boundary_mask);
	tile_width_in_longs = rounded_tile_width >>
		screen_state_p->pixels_per_long_shift;

	/*
	 * pointer to beginning of the tile data.
	 */
	source_start_p = (unsigned long *)(si_tile_p->Bptr);

	S3_WAIT_FOR_FIFO(1);

	/*
	 * Select the color source to be cpu data.
  	 * Pixcntl is set to use foreground mix register, by default.
	 */
	S3_UPDATE_MMAP_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
		screen_state_p->register_state.s3_enhanced_commands_registers.
		frgd_mix | S3_CLR_SRC_CPU_DATA, unsigned short);

	/*
	 * tile the rectangles in the list one at a time.
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
		 * offsets into the tile for the partial widths.
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
		if((x_left > screen_clip_right) || (x_right < screen_clip_left) ||
		   (y_top > screen_clip_bottom) || (y_bottom < screen_clip_top) ||
		   (y_bottom < y_top) || ( x_right < x_left ))
		{
#if (defined(__DEBUG__))
			if (s364_fill_tile_debug)
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
        if (x_left < screen_clip_left)
        {
            x_left = screen_clip_left;
        }
		if (x_right > screen_clip_right)
        {
            x_right = screen_clip_right;
        }
        if (y_top < screen_clip_top)
        {
            y_top = screen_clip_top;
        }
        if (y_bottom > screen_clip_bottom)
        {
            y_bottom = screen_clip_bottom;
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
		S3_WAIT_FOR_FIFO(3);
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_SCISSORS_T_INDEX |
			(y_top & S3_MULTIFUNC_VALUE_BITS),unsigned short);
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_SCISSORS_B_INDEX |
			(y_bottom & S3_MULTIFUNC_VALUE_BITS),unsigned short);
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX |
			(x_left & S3_MULTIFUNC_VALUE_BITS),unsigned short);

		destination_x = x_left;
		destination_y = y_top;

		/*
		 * Calculate offsets into the source tile	
		 */
		x_offset = ( x_left - si_tile_p->BorgX) % tile_width;

		if( x_offset < 0)
		{
			x_offset += tile_width;
		}

		/*
		 * Push x_offset back to a longword boundary
		 */
		if (x_offset & long_word_boundary_mask)
		{
			destination_x -= (x_offset & long_word_boundary_mask);
			x_offset &= ~long_word_boundary_mask;
		}
		
		y_offset = ( y_top - si_tile_p->BorgY)  % tile_height;
		if( y_offset < 0)
		{
			y_offset += tile_height;
		}

		/*
		 * Compute the first longword source pointer  to transfer.
		 */
		source_x_offset_p = source_start_p + ((unsigned long )x_offset >> 
			screen_state_p->pixels_per_long_shift);

		/*
		 * Compute the following parameters.
		 * 1. The number of starting longwords before encountering the first
		 *    complete tile width.
		 * 2. The number of middle longwords, the full tile width transfers.
		 * 3. The number of end longwords, the last transfer which is less than
		 *    one full tile width.
		 */
		if((destination_x +  (tile_width - x_offset) - 1) < x_right)
		{
			/*
			 * More than 1 transfer is there for this rectangle.
			 */
			if (x_offset > 0)
			{
				startwords = tile_width_in_longs - 
					((unsigned long)x_offset >> 
					screen_state_p->pixels_per_long_shift);
				startwidth_x_right = 
					destination_x + tile_width - x_offset -1;
			}
			else
			{
				startwords = 0;
				startwidth_x_right = destination_x - 1;
			}


			midwidths = (x_right - startwidth_x_right) / tile_width;
			endwords = (x_right - startwidth_x_right) % tile_width;
			
			if ( endwords )
			{
				/*
				 * endwords in terms of host data long words.
				 */
				endwidth_x_left = x_right - endwords + 1;
				endwords = ((endwords + long_word_boundary_mask) & 
					~long_word_boundary_mask) >>
					screen_state_p->pixels_per_long_shift;

				ASSERT(endwidth_x_left ==
					(startwidth_x_right + 1 + (midwidths*tile_width)));
			}
		}
		else
		{
			/*
			 * This is the only transfer for this rectangle.
			 */
			startwords = x_right - destination_x + 1;
			startwords = ((startwords + long_word_boundary_mask) & 
				~long_word_boundary_mask) >>
				screen_state_p->pixels_per_long_shift;
			startwidth_x_right = x_right;
			midwidths = 0;
			endwords = 0;
		}

#if (defined(__DEBUG__))
	if (s364_fill_tile_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s364_tile_rectangle_system_memory){\n"
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
			curdy = destination_y;
			cursy = y_offset;
			tmp_source_p = source_x_offset_p + 
				(y_offset * tile_width_in_longs);


			S3_INLINE_WAIT_FOR_FIFO(6);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
				(startwidth_x_right & S3_MULTIFUNC_VALUE_BITS),unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
				(unsigned short)destination_x, unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
				(unsigned short)destination_y, unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
				((startwords << screen_state_p->pixels_per_long_shift) - 1),
				unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
				((height - 1) & S3_MULTIFUNC_VALUE_BITS), unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CMD, command, unsigned short);

			/*
			 * Wait for the initiator to get executed.
			 */
			S3_WAIT_FOR_ALL_FIFO_FREE();

			do
			{
				int		tilelines;
				int		tmplines;

				tilelines = ((curdy + tile_height - cursy) > y_bottom )
					? y_bottom - curdy + 1 : tile_height - cursy;

				tmplines = tilelines;
				ASSERT(tmplines > 0);

				do
				{

					S3_ASM_TRANSFER_THRO_PIXTRANS(startwords,tmp_source_p);
					
					tmp_source_p += tile_width_in_longs;
					++cursy;
				} while ( --tmplines > 0);

				curdy += tilelines;
				cursy = 0;
				tmp_source_p = source_x_offset_p;

			} while ( curdy <= y_bottom );
		}
		ASSERT(!(S3_IS_FIFO_OVERFLOW()));

		if ( midwidths )
		{
			int	const partial_top_lines = 
				(tile_height - y_offset) > (height) ? height : 
				tile_height - y_offset;

			int	const mid_heights_count = (height - partial_top_lines) / 
				tile_height; 

			int const partial_bottom_lines = (height) - 
				(mid_heights_count * tile_height) - partial_top_lines;

			int const total_partial_top_transfers = 
				partial_top_lines * tile_width_in_longs;

			int const total_partial_bottom_transfers = 
				partial_bottom_lines * tile_width_in_longs;

			unsigned long * const top_partial_source_p = source_start_p + 
				(y_offset * tile_width_in_longs);

			ASSERT(height >= partial_top_lines);
			ASSERT((height) == (partial_bottom_lines + 
				partial_top_lines + (mid_heights_count * tile_height)));

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

				S3_INLINE_WAIT_FOR_FIFO(6);

				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
					((curdx + tile_width - 1) & S3_MULTIFUNC_VALUE_BITS),
					unsigned short);
				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
					(unsigned short)curdx, unsigned short);
				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
					(unsigned short)y_top, unsigned short);
				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
					(rounded_tile_width - 1), unsigned short);
				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
					((height - 1) & S3_MULTIFUNC_VALUE_BITS), unsigned short);
				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CMD,
					command, unsigned short);

				/*
				 * Wait for the initiator to get executed.
				 */
				S3_WAIT_FOR_ALL_FIFO_FREE();

				if (total_partial_top_transfers > 0)
				{
					S3_ASM_TRANSFER_THRO_PIXTRANS(
						total_partial_top_transfers,
						top_partial_source_p);
				}

				if (mid_heights_count > 0)
				{
					int		i = mid_heights_count;
					do
					{
						S3_ASM_TRANSFER_THRO_PIXTRANS(
							tile_state_p->transfer_length_in_longwords,
							source_start_p);
					} while (--i > 0); 
				}

				if (total_partial_bottom_transfers > 0)
				{
					S3_ASM_TRANSFER_THRO_PIXTRANS(
						total_partial_bottom_transfers,
						source_start_p);
				}

				curdx += tile_width;
			} while ( --midwidths > 0);
		}

		if ( endwords )
		{
			unsigned long *tmp_source_p;

			S3_INLINE_WAIT_FOR_FIFO(6);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
				(x_right & S3_MULTIFUNC_VALUE_BITS),unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
				endwidth_x_left , unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
				(unsigned short)y_top, unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
				((endwords << screen_state_p->pixels_per_long_shift) - 1),
				unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
				((height - 1) & S3_MULTIFUNC_VALUE_BITS), unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CMD, command, unsigned short);

			/*
			 * Wait for the initiator to get executed.
			 */
			S3_WAIT_FOR_ALL_FIFO_FREE();

			curdy = destination_y;
			cursy = y_offset;
			tmp_source_p = 
				source_start_p + (y_offset * tile_width_in_longs);

			do
			{
				int		tilelines;
				int		tmplines;

				tilelines = ((curdy + tile_height - cursy) > y_bottom )
					? y_bottom - curdy + 1 : tile_height - cursy;

				tmplines = tilelines;
				ASSERT(tmplines > 0);
				do
				{

					S3_ASM_TRANSFER_THRO_PIXTRANS(endwords,tmp_source_p);
					
					tmp_source_p += tile_width_in_longs;
					++cursy;
				} while ( --tmplines > 0);

				curdy += tilelines;
				cursy = 0;
				tmp_source_p = source_start_p;

			} while ( curdy <= y_bottom );
		}
		ASSERT(!(S3_IS_FIFO_OVERFLOW()));

		/*
		 * advance to the next destination rectangle.
		 */
		++rect_p;
	}
	while(--count);	

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	/*
	 * Invalidate the clipping rectangle.
	 */
	screen_state_p->generic_state.screen_current_clip = GENERIC_CLIP_NULL;

	return (SI_SUCCEED);
}
/*
 * PURPOSE
 *
 * Fillrect routine for SGfillmode = SGfillStipple and the stipple has been
 * downloaded in the offscreen memory.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_fill_rectangle_stipple_offscreen_memory( 
	SIint32 x_origin, SIint32 y_origin, SIint32	count, SIRectOutlineP rect_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	int offscreen_stipple_x_location;
	int offscreen_stipple_y_location;
	int offscreen_stipple_height;
	int offscreen_stipple_width;

    const int screen_clip_left = 
		screen_state_p->generic_state.screen_clip_left;
    const int screen_clip_right = 
		screen_state_p->generic_state.screen_clip_right;
    const int screen_clip_top = 
		screen_state_p->generic_state.screen_clip_top;
    const int screen_clip_bottom = 
		screen_state_p->generic_state.screen_clip_bottom;
	
	struct s364_stipple_state	*stipple_state_p;	
	SIbitmap 					*si_stipple_p;
	
	ASSERT(!S3_IS_FIFO_OVERFLOW());
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));
	
	stipple_state_p = &(graphics_state_p->current_stipple_state);

	si_stipple_p =
		graphics_state_p->generic_state.si_graphics_state.SGstipple;
	
	ASSERT(IS_OBJECT_STAMPED(S364_STIPPLE_STATE, stipple_state_p));
#if (defined(__DEBUG__))
	if (s364_fill_stipple_debug)
	{
		(void) fprintf(debug_stream_p,
		   "(s364_fill_rectangle_stipple_offscreen_memory)\n"
		   "{\n"
		   "\tx_origin = %ld\n"
		   "\ty_origin = %ld\n"
		   "\tcount = %ld\n"
		   "\trect_p = %p\n",
		   x_origin,
		   y_origin,
		   count,
		   (void *) rect_p);
	}
#endif

	if (!stipple_state_p->stipple_downloaded) 
	{
		s364_graphics_state_download_stipple(screen_state_p,
			graphics_state_p,
			graphics_state_p->generic_state.si_graphics_state.SGstipple);
	}	

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	if (!OMM_LOCK(stipple_state_p->offscreen_location_p))
	{
		/*
		 * We have to try system memory stippling
		 */
#if	defined(__DEBUG__)
		if (s364_fill_stipple_debug)
		{
			(void)fprintf(debug_stream_p,"#OMM lock failed!");
		}
#endif
		return (SI_FAIL);
	}

	/*
	 * Set the pixel control register to use stipple data downloaded
	 * in the offscreen memory for selecting the foreground and 
	 * background mix registers.
	 */
	S3_UPDATE_MMAP_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
		PIX_CNTL_DT_EX_SRC_VID_MEM, unsigned short);

	S3_UPDATE_MMAP_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
		enhanced_cmds_p->frgd_mix | S3_CLR_SRC_FRGD_COLOR,
		unsigned short);

	/*
	 * If stipple_type is not zero the request is for a type of stippling
	 * different from the Graphics State. If forcetype is SGOPQStipple
	 * then perform opaque stippling, else perform transparent stippling.
	 */

	if (graphics_state_p->generic_state.si_graphics_state.SGstplmode
		== SGStipple)
	{
		/*
		 * For transparent case program BG_ROP to be DST.
		 */
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,
			S3_MIX_FN_LEAVE_C_AS_IS, unsigned short);
	}
	else 
	{
		ASSERT(graphics_state_p->generic_state.si_graphics_state.SGstplmode
			== SGOPQStipple);

		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,
			enhanced_cmds_p->bkgd_mix, unsigned short);
	}

	/*
	 * The offscreen stipple occupies only one plane, set the readmask
	 * for reading only that plane
	 */
	S3_UPDATE_MMAP_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_RD_MASK,
		stipple_state_p->offscreen_location_p->planemask,
		unsigned long);

	offscreen_stipple_x_location = stipple_state_p->offscreen_location_x;
	offscreen_stipple_y_location = stipple_state_p->offscreen_location_y;
	offscreen_stipple_width = si_stipple_p->Bwidth;
	offscreen_stipple_height = si_stipple_p->Bheight;
	
	/*
	 * Set the clipping rectangle to select the entire drawing area.
	 */
	S364_STATE_SET_CLIP_RECTANGLE_TO_VIRTUAL_SCREEN();

	/*
	 * loop through the list of rectangles.
	 */
	do
	{
		/*
		 * destination rectangle coordinates.
		 */
		int x_left = rect_p->x + x_origin;
		int x_right = x_left + rect_p->width - 1; /* inclusive */
		int y_top = rect_p->y + y_origin;
		int y_bottom = y_top + rect_p->height - 1; /* inclusive */

		/*
		 * Check if the rectangle is outside the current clipping
		 * bounds. Discard the rectangle if so.
		 */
		if((x_left > screen_clip_right) || (x_right < screen_clip_left) ||
		   (y_top > screen_clip_bottom) || (y_bottom < screen_clip_top) ||
		   (y_bottom < y_top) || ( x_right < x_left ))
		{
#if (defined(__DEBUG__))
			if (s364_fill_debug)
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
        if (x_left < screen_clip_left)
        {
            x_left = screen_clip_left;
        }
		if (x_right > screen_clip_right)
        {
            x_right = screen_clip_right;
        }
        if (y_top < screen_clip_top)
        {
            y_top = screen_clip_top;
        }
        if (y_bottom > screen_clip_bottom)
        {
            y_bottom = screen_clip_bottom;
        }

		s364_fill_rectangle_offscreen_helper(
			si_stipple_p,
			offscreen_stipple_x_location, offscreen_stipple_y_location,
			offscreen_stipple_width, offscreen_stipple_height,
			x_left, y_top, 
			x_right - x_left + 1, y_bottom - y_top + 1,
			S3_CMD_PX_MD_ACROSS_PLANE);

		++rect_p;
	}while(--count);	

#if (defined(__DEBUG__))
	if(s364_fill_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	ASSERT(!S3_IS_FIFO_OVERFLOW());

	S3_INLINE_WAIT_FOR_FIFO(2);
	S3_UPDATE_MMAP_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_RD_MASK,
		enhanced_cmds_p->read_mask, unsigned short);
	S3_UPDATE_MMAP_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
		PIX_CNTL_DT_EX_SRC_FRGD_MIX, unsigned short);

	ASSERT(!S3_IS_FIFO_OVERFLOW());
	return (SI_SUCCEED);
}

/*
 * PURPOSE
 *
 * Fillrect routine for SGfillmode = SGfillStipple. This routine does
 * stippling from system memory to the screen.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_fill_rectangle_stipple_system_memory( SIint32 x_origin, SIint32 y_origin,
	SIint32	count, SIRectOutlineP rect_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();

    const int screen_clip_left = 
		screen_state_p->generic_state.screen_clip_left;
    const int screen_clip_right = 
		screen_state_p->generic_state.screen_clip_right;
    const int screen_clip_top = 
		screen_state_p->generic_state.screen_clip_top;
    const int screen_clip_bottom = 
		screen_state_p->generic_state.screen_clip_bottom;

	SIbitmap 					*si_stipple_p;
	struct s364_stipple_state 	*stipple_state_p;

	int 						stipple_height;		/*in pixels */
	int							stipple_width;		/*in pixels */
	unsigned short				rounded_stipple_width;
	unsigned short				stipple_width_in_longs;

	/*
	 * Pointer to the (0,0) coords in source stipple.
	 * Pointer to the (destination_x,0) coords in source stipple.
	 */
	unsigned long			*source_start_p;
	unsigned long			*source_x_offset_p;

	const unsigned short command =
		S3_CMD_WRITE | 
		S3_CMD_PX_MD_ACROSS_PLANE |
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM |
		S3_CMD_USE_WAIT_YES |
		S3_CMD_BUS_WIDTH_32 |
		S3_CMD_LSB_FIRST |
		S3_CMD_TYPE_RECTFILL;

	stipple_state_p = &(graphics_state_p->current_stipple_state);
	si_stipple_p = graphics_state_p->generic_state.si_graphics_state.SGstipple;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_STIPPLE_STATE, stipple_state_p));
	ASSERT(!S3_IS_FIFO_OVERFLOW());

	ASSERT(stipple_state_p->is_small_stipple == FALSE);
	ASSERT(stipple_state_p->stipple_downloaded == TRUE);

#if (defined(__DEBUG__))
	if (s364_fill_stipple_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_fill_rectangle_stipple_system_memory)\n"
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
	 * Make sure that the stipplestate is already initialized, if not 
	 * call the download function now.
	 */
	if (!stipple_state_p->stipple_downloaded) 
	{
		s364_graphics_state_download_stipple(screen_state_p,
			graphics_state_p,
			graphics_state_p->generic_state.si_graphics_state.SGstipple);
	}	

	/*
	 * SI stipple's width, height and bitmap width rounded off to longwords.
	 */
	stipple_width = si_stipple_p->Bwidth;
	stipple_height = si_stipple_p->Bheight;
	rounded_stipple_width = ((unsigned)(stipple_width + 31) & ~31);
	stipple_width_in_longs = rounded_stipple_width >> 5;

	/*
	 * pointer to beginning of the stipple data.
	 */
	source_start_p = (unsigned long *)(stipple_state_p->inverted_stipple.Bptr);

	S3_WAIT_FOR_FIFO(3);

	/*
	 * Set the pixel control register to use cpu data for selecting
	 * the foreground and background mix registers.
	 */
	S3_UPDATE_MMAP_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
		PIX_CNTL_DT_EX_SRC_CPU_DATA, unsigned short);

	S3_UPDATE_MMAP_REGISTER(
		S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
		enhanced_cmds_p->frgd_mix | S3_CLR_SRC_FRGD_COLOR, 
		unsigned short);

	/*
	 * If stipple_type is not zero the request is for a type of stippling
	 * different from the Graphics State. If forcetype is SGOPQStipple
	 * then perform opaque stippling, else perform transparent stippling.
	 */
	if (graphics_state_p->generic_state.si_graphics_state.SGstplmode
		== SGStipple)
	{
		/*
		 * For transparent case program BG_ROP to be DST.
		 */
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,
			S3_MIX_FN_LEAVE_C_AS_IS, unsigned short);
	}
	else 
	{
		ASSERT(graphics_state_p->generic_state.si_graphics_state.SGstplmode
			== SGOPQStipple);

		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_BKGD_MIX,
			enhanced_cmds_p->bkgd_mix, unsigned short);
	}

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
		if((x_left > screen_clip_right) || (x_right < screen_clip_left) ||
		   (y_top > screen_clip_bottom) || (y_bottom < screen_clip_top) ||
		   (y_bottom < y_top) || ( x_right < x_left ))
		{
#if (defined(__DEBUG__))
			if (s364_fill_stipple_debug)
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
        if (x_left < screen_clip_left)
        {
            x_left = screen_clip_left;
        }
		if (x_right > screen_clip_right)
        {
            x_right = screen_clip_right;
        }
        if (y_top < screen_clip_top)
        {
            y_top = screen_clip_top;
        }
        if (y_bottom > screen_clip_bottom)
        {
            y_bottom = screen_clip_bottom;
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
		S3_WAIT_FOR_FIFO(3);
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_SCISSORS_T_INDEX |
			(y_top & S3_MULTIFUNC_VALUE_BITS),unsigned short);
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_SCISSORS_B_INDEX |
			(y_bottom & S3_MULTIFUNC_VALUE_BITS),unsigned short);
		S3_UPDATE_MMAP_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX |
			(x_left & S3_MULTIFUNC_VALUE_BITS),unsigned short);

		destination_x = x_left;
		destination_y = y_top;

		/*
		 * Calculate offsets into the source stipple	
		 */
		x_offset = ( x_left - si_stipple_p->BorgX) % stipple_width;

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
	if (s364_fill_stipple_debug)
	{
		(void) fprintf(debug_stream_p, 
			"(s364_stipple_rectangle_system_memory){\n"
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
			curdy = destination_y;
			cursy = y_offset;
			tmp_source_p = source_x_offset_p + 
				(y_offset * stipple_width_in_longs);


			S3_INLINE_WAIT_FOR_FIFO(6);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
				(startwidth_x_right & S3_MULTIFUNC_VALUE_BITS),unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
				(unsigned short)destination_x, unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
				(unsigned short)destination_y, unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
				((startwords << 5U) - 1), unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
				((height - 1) & S3_MULTIFUNC_VALUE_BITS), unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CMD, command, unsigned short);

			/*
			 * Wait for the initiator to get executed.
			 */
			S3_WAIT_FOR_ALL_FIFO_FREE();

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

					S3_ASM_TRANSFER_THRO_PIXTRANS(startwords,tmp_source_p);
					
					tmp_source_p += stipple_width_in_longs;
					++cursy;
				} while ( --tmplines > 0);

				curdy += stipplelines;
				cursy = 0;
				tmp_source_p = source_x_offset_p;

			} while ( curdy <= y_bottom );
		}
		ASSERT(!(S3_IS_FIFO_OVERFLOW()));

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

				S3_INLINE_WAIT_FOR_FIFO(6);

				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
					((curdx + stipple_width - 1) & S3_MULTIFUNC_VALUE_BITS),
					unsigned short);
				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
					(unsigned short)curdx, unsigned short);
				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
					(unsigned short)y_top, unsigned short);
				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
					(rounded_stipple_width - 1), unsigned short);
				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
					((height - 1) & S3_MULTIFUNC_VALUE_BITS), unsigned short);
				S3_UPDATE_MMAP_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CMD,
					command, unsigned short);

				/*
				 * Wait for the initiator to get executed.
				 */
				S3_WAIT_FOR_ALL_FIFO_FREE();

				if (total_partial_top_transfers > 0)
				{
					S3_ASM_TRANSFER_THRO_PIXTRANS(
						total_partial_top_transfers,
						top_partial_source_p);
				}

				if (mid_heights_count > 0)
				{
					int		i = mid_heights_count;
					do
					{
						S3_ASM_TRANSFER_THRO_PIXTRANS(
							stipple_state_p->transfer_length_in_longwords,
							source_start_p);
					} while (--i > 0); 
				}

				if (total_partial_bottom_transfers > 0)
				{
					S3_ASM_TRANSFER_THRO_PIXTRANS(
						total_partial_bottom_transfers,
						source_start_p);
				}

				curdx += stipple_width;
			} while ( --midwidths > 0);
		}

		if ( endwords )
		{
			unsigned long *tmp_source_p;

			S3_INLINE_WAIT_FOR_FIFO(6);

			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
				(x_right & S3_MULTIFUNC_VALUE_BITS),unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
				endwidth_x_left , unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
				(unsigned short)y_top, unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
				((endwords << 5U) - 1), unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
				((height - 1) & S3_MULTIFUNC_VALUE_BITS), unsigned short);
			S3_UPDATE_MMAP_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CMD, command, unsigned short);

			/*
			 * Wait for the initiator to get executed.
			 */
			S3_WAIT_FOR_ALL_FIFO_FREE();

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

					S3_ASM_TRANSFER_THRO_PIXTRANS(endwords,tmp_source_p);
					
					tmp_source_p += stipple_width_in_longs;
					++cursy;
				} while ( --tmplines > 0);

				curdy += stipplelines;
				cursy = 0;
				tmp_source_p = source_start_p;

			} while ( curdy <= y_bottom );
		}

		/*
		 * advance to the next destination rectangle.
		 */
		++rect_p;
	}
	while(--count);	

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	/*
	 * Restore the registers that have changed.
	 */
	S3_WAIT_FOR_FIFO(1);
	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_PIX_CNTL_INDEX |
		PIX_CNTL_DT_EX_SRC_FRGD_MIX, unsigned short);

	/*
	 * Invalidate the clipping rectangle.
	 */
	screen_state_p->generic_state.screen_current_clip = GENERIC_CLIP_NULL;

	return (SI_SUCCEED);
}

/*
 * PURPOSE
 *
 *		Routine to fill rectangles with the current graphics state's tile 
 * pattern when the fill mode is tile. This is an SI entry point for 
 * filling with tiles.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_fill_rectangle_tile( SIint32 x_origin, SIint32 y_origin,
	SIint32	count, SIRectOutlineP rect_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_ENHANCED_REGISTERS_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	struct s364_tile_state 	*tile_state_p;	
	SIbitmap 				*si_tile_p;

	ASSERT(!S3_IS_FIFO_OVERFLOW());
	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));

	ASSERT(screen_state_p->options_p->rectfill_options &
		S364_OPTIONS_RECTFILL_OPTIONS_USE_TILE_FILL_RECT != 0);

	tile_state_p = &(graphics_state_p->current_tile_state);
	si_tile_p = graphics_state_p->generic_state.si_graphics_state.SGtile;

	if (count <= 0)
	{
		return(SI_SUCCEED);
	}

	/*
	 * Make sure that the stipplestate is already initialized, if not 
	 * call the download function now.
	 */
	if ((!tile_state_p->tile_downloaded) ||
		(tile_state_p->tile_origin_x != 
		graphics_state_p->generic_state.si_graphics_state.SGtile->BorgX) ||
		(tile_state_p->tile_origin_y != 
		graphics_state_p->generic_state.si_graphics_state.SGtile->BorgY)) 
	{
		s364_graphics_state_download_tile(screen_state_p,
			graphics_state_p, si_tile_p);
	}	

	ASSERT(!S3_IS_FIFO_OVERFLOW());

	if (tile_state_p->tile_downloaded == FALSE)
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return(SI_FAIL);
	}

	if ((screen_state_p->options_p->rectfill_options &
		 S364_OPTIONS_RECTFILL_OPTIONS_USE_GE_MONO_PATFILL) &&
		(screen_state_p->options_p->rectfill_options &
		S364_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY) &&
		(screen_state_p->options_p->rectfill_options &
		 S364_OPTIONS_RECTFILL_OPTIONS_USE_TILE_SHRINK) &&
		(tile_state_p->is_reduced_tile == TRUE))
	{
		/*
		 * Tiling using the mono patblt feature.
		 */

		SIBool	return_value;

		/*
		 * Set the foreground and background colors for the two color tile
		 * and restore them after the call.
		 * PROBLEM: WAIT FOR FIFO
		 */
		S3_WAIT_FOR_FIFO(2);

		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_COLOR,
			tile_state_p->color1, unsigned long);

		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_COLOR,
			tile_state_p->color2, unsigned long);

		ASSERT(!S3_IS_FIFO_OVERFLOW());
		return_value =  s364_fill_rectangle_stipple_ge_patblt(
			x_origin, y_origin, count, rect_p, 
			&(tile_state_p->reduced_tile_state), SGOPQStipple);

		S3_WAIT_FOR_FIFO(2);

		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_COLOR,
			enhanced_cmds_p->frgd_color, unsigned long);

		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_BKGD_COLOR,
			enhanced_cmds_p->bkgd_color, unsigned long);

		return (return_value);
	}
	else if ((screen_state_p->options_p->rectfill_options &
		S364_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY) &&
		OMM_LOCK(tile_state_p->offscreen_location_p))
	{
		/*
		 * Tiling using offscreen memory.
		 */
		return (s364_fill_rectangle_tile_offscreen_memory(
			x_origin, y_origin, count, rect_p));
	}
	else
	{
		/*
		 * System memory based tiling to be added later, for now
		 * return fail.
		 */
		return (s364_fill_rectangle_tile_system_memory( 
			x_origin, y_origin, count, rect_p));
	}
}


/*
 * PURPOSE
 *
 *		Routine to fill rectangles with the current graphics state's stipple 
 * pattern when the fill mode is stipple. This is a SI entry point for filling
 * rectangles with stipples.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_fill_rectangle_stipple( SIint32 x_origin, SIint32 y_origin,
	SIint32	count, SIRectOutlineP rect_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	struct s364_stipple_state 	*stipple_state_p;	
	SIbitmap 				*si_stipple_p;

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));

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
	 * Make sure that the stipplestate is already initialized, if not 
	 * call the download function now.
	 */
	if ((!stipple_state_p->stipple_downloaded) ||
		(stipple_state_p->stipple_origin_x != 
		graphics_state_p->generic_state.si_graphics_state.SGstipple->BorgX) ||
		(stipple_state_p->stipple_origin_y != 
		graphics_state_p->generic_state.si_graphics_state.SGstipple->BorgY)) 
	{
		s364_graphics_state_download_stipple(screen_state_p,
			graphics_state_p,
			graphics_state_p->generic_state.si_graphics_state.SGstipple);
	}	

	ASSERT(!S3_IS_FIFO_OVERFLOW());
	ASSERT(screen_state_p->options_p->rectfill_options &
		S364_OPTIONS_RECTFILL_OPTIONS_USE_STIPPLE_FILL_RECT != 0);

	if (stipple_state_p->stipple_downloaded == FALSE)
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return(SI_FAIL);
	}

	if ((screen_state_p->options_p->rectfill_options &
		 S364_OPTIONS_RECTFILL_OPTIONS_USE_GE_MONO_PATFILL) &&
		(screen_state_p->options_p->rectfill_options &
		S364_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY) &&
		(OMM_LOCK(stipple_state_p->offscreen_location_p)) &&
		(stipple_state_p->is_small_stipple == TRUE))
	{
		ASSERT(!S3_IS_FIFO_OVERFLOW());
		return (s364_fill_rectangle_stipple_ge_patblt(
			x_origin, y_origin, count, rect_p, stipple_state_p,
			graphics_state_p->generic_state.si_graphics_state.SGstplmode));
	}
	else if ((screen_state_p->options_p->rectfill_options &
		S364_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY) &&
		OMM_LOCK(stipple_state_p->offscreen_location_p))
	{
		/*
		 * Stippling using offscreen memory.
		 */
		ASSERT(!S3_IS_FIFO_OVERFLOW());
		return (s364_fill_rectangle_stipple_offscreen_memory(
			x_origin, y_origin, count, rect_p));
	}
	else
	{
		/*
		 * System memory based stippling.
		 */
		return(s364_fill_rectangle_stipple_system_memory(
			x_origin, y_origin, count, rect_p));
	}
	/*NOTREACHED*/
	/*CONSTANTCONDITION*/
	ASSERT(0);
	return(SI_FAIL);

}

/*
 * PURPOSE
 *
 *	Backward compatible to SI 1.0 (R4) fill rectangle entry point.
 * This routine also checks for the current fill mode and calls the
 * apropriate R5 entry point.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_fill_rectangle_compat(SIint32 count, SIRectP pRect)
{
	int		ret_val;
	boolean is_local_allocation;
	
	SIRectOutlineP	p_new_rect, tmp_new_rect_p;
	SIRectP			tmp_old_rect_p;
	SIint32 		tmpcount;

	SIBool (*called_function_p)(SIint32, SIint32, SIint32,
								SIRectOutlineP);
	SIRectOutline				/* for fast allocations */
		localRectangles[DEFAULT_S364_COMPATIBILITY_LOCAL_RECTANGLE_COUNT];
	
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();

#if (defined(__DEBUG__))
	if (s364_fill_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_fill_rectangle_compat)\n"
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
	if (count > DEFAULT_S364_COMPATIBILITY_LOCAL_RECTANGLE_COUNT)
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
				S364_OPTIONS_RECTFILL_OPTIONS_USE_SOLID_FILL_RECT)
			{
				called_function_p = s364_fill_rectangle_solid; 
			}
			break;

		case SGFillTile :
			if (screen_state_p->options_p->rectfill_options &
				S364_OPTIONS_RECTFILL_OPTIONS_USE_TILE_FILL_RECT)
			{
				called_function_p = s364_fill_rectangle_tile;
			}
			break;

		case SGFillStipple:
			if (screen_state_p->options_p->rectfill_options &
				S364_OPTIONS_RECTFILL_OPTIONS_USE_STIPPLE_FILL_RECT)
			{
				called_function_p = s364_fill_rectangle_stipple;
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
 * PURPOSE
 *
 * Graphics state change.
 * We change the function pointers for SI fill rectangle entry points
 * depending on whether the fill mode is solid, tile or stipple.
 *
 * RETURN VALUE
 *
 * None.
 *
 */
function void
s364_fill__gs_change__(void)
{
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	S364_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));
	
#if (defined(__DEBUG__))
	if (s364_fill_debug)
	{
		(void) fprintf(debug_stream_p, "(s364_fill__gs_change__) {}\n");
	}
#endif

	/*
	 * Set the default pointer to the fill function.
	 */
	screen_state_p->generic_state.screen_functions_p->si_poly_fillrect =
		(SIBool (*)(SIint32 , SIint32 , SIint32, SIRectOutlineP))
		s364_no_operation_fail;

	if (screen_state_p->generic_state.screen_sdd_version_number >=
		DM_SI_VERSION_1_1)
	{
		switch (graphics_state_p->generic_state.si_graphics_state.SGfillmode)
		{
			case SGFillSolidFG:
			case SGFillSolidBG :
				if (screen_state_p->options_p->rectfill_options &
					S364_OPTIONS_RECTFILL_OPTIONS_USE_SOLID_FILL_RECT)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_poly_fillrect = s364_fill_rectangle_solid;
				}
			break;

		case SGFillTile:
			if (screen_state_p->options_p->rectfill_options &
				S364_OPTIONS_RECTFILL_OPTIONS_USE_TILE_FILL_RECT)
			{
				screen_state_p->generic_state.screen_functions_p->
					si_poly_fillrect = s364_fill_rectangle_tile;
			}
			break;
		case SGFillStipple:
			if (screen_state_p->options_p->rectfill_options &
				S364_OPTIONS_RECTFILL_OPTIONS_USE_STIPPLE_FILL_RECT)
			{
				screen_state_p->generic_state.screen_functions_p->
					si_poly_fillrect = s364_fill_rectangle_stipple;
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
			s364_fill_rectangle_compat;
	}

	return;
}

/*
 * s364_fill__initialize__
 *
 * PURPOSE
 *
 * Initializing the fill module. This function is called from the 
 * munch generated function in the module s364__init__.c at the time
 * of chipset initialization. 
 *
 * RETURN VALUE
 *
 *		None.
 */
function void
s364_fill__initialize__(SIScreenRec *si_screen_p,
	struct s364_options_structure *options_p)
{
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	int tile_best_width = 0, tile_best_height = 0;
	int stipple_best_width = 0, stipple_best_height = 0;

	flags_p->SIavail_fpoly = RECTANGLE_AVAIL |
		TILE_AVAIL | OPQSTIPPLE_AVAIL | STIPPLE_AVAIL;

	if (options_p->tile_best_size)
	{
		if (sscanf(options_p->tile_best_size, "%ix%i",
				   &tile_best_width, &tile_best_height) != 2)
		{
			(void) fprintf(stderr,
				S364_BAD_BEST_TILE_SIZE_SPECIFICATION_OPTION_MESSAGE,
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
				S364_BAD_BEST_STIPPLE_SIZE_SPECIFICATION_OPTION_MESSAGE,
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
