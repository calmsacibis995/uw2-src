/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)S3:S3/s3_fill.c	1.7"

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
export boolean s3_fill_debug = FALSE;
export boolean s3_fill_tile_debug = FALSE;
export boolean s3_fill_stipple_debug = FALSE;
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

#include "s3_globals.h"
#include "s3_options.h"
#include "s3_state.h"
#include "s3_gs.h"
#include "s3_asm.h"

/***
 ***	Constants.
 ***/
#define S3_FILL_SOLID_DEPENDENCIES\
	(S3_INVALID_FG_ROP |\
	 S3_INVALID_FOREGROUND_COLOR |\
	 S3_INVALID_BACKGROUND_COLOR |\
	 S3_INVALID_WRT_MASK |\
	 S3_INVALID_CLIP_RECTANGLE)

#define S3_FILL_TILE_DEPENDENCIES\
	(S3_INVALID_FG_ROP |\
	 S3_INVALID_WRT_MASK |\
	 S3_INVALID_CLIP_RECTANGLE)

#define S3_FILL_TILE_PATBLT_DEPENDENCIES \
	(S3_INVALID_FG_ROP |\
	 S3_INVALID_WRT_MASK)

#define S3_FILL_STIPPLE_TRANSPARENT_DEPENDENCIES\
	(S3_INVALID_FG_ROP |\
	 S3_INVALID_FOREGROUND_COLOR |\
	 S3_INVALID_WRT_MASK |\
	 S3_INVALID_CLIP_RECTANGLE)

#define S3_FILL_STIPPLE_OPAQUE_DEPENDENCIES\
	(S3_INVALID_FG_ROP |\
	 S3_INVALID_BG_ROP |\
	 S3_INVALID_FOREGROUND_COLOR |\
	 S3_INVALID_BACKGROUND_COLOR |\
	 S3_INVALID_WRT_MASK |\
	 S3_INVALID_CLIP_RECTANGLE)

#define S3_FILL_STIPPLE_TRANSPARENT_PATBLT_DEPENDENCIES\
	(S3_INVALID_FG_ROP |\
	 S3_INVALID_FOREGROUND_COLOR |\
	 S3_INVALID_WRT_MASK)

#define S3_FILL_STIPPLE_OPAQUE_PATBLT_DEPENDENCIES\
	(S3_INVALID_FG_ROP |\
	 S3_INVALID_BG_ROP |\
	 S3_INVALID_FOREGROUND_COLOR |\
	 S3_INVALID_BACKGROUND_COLOR |\
	 S3_INVALID_WRT_MASK)

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/
/*
 * s3_fill_rectangle_offscreen_helper
 * Helper for stippling and tiling rectangles from offscreen source
 * Assumptions: offscreen_bitmap_width x offscreen_bitmap_height
 * contains integral number of tile/stipples.
 */
STATIC void
s3_fill_rectangle_offscreen_helper(
	SIbitmapP source_bitmap_p,
	int offscreen_bitmap_x_location, 	/* source tile parameters */
	int offscreen_bitmap_y_location,
	int offscreen_bitmap_width,
	int offscreen_bitmap_height,
	int destination_x,					/* destination tile parameters */
	int destination_y,
	int destination_width,
	int destination_height,
	int flags)
{
	int		fractional_width_start;
	int		fractional_width_end;
	int		middle_widths_count;
	int		fractional_height_start;
	int		fractional_height_end;
	int		middle_heights_count;
	int		offscreen_fractional_x_position;
	int		offscreen_fractional_y_position;

	int		current_destination_x = destination_x;

	S3_CURRENT_SCREEN_STATE_DECLARE();
	const unsigned short command = screen_state_p->cmd_flags |
					S3_CMD_TYPE_BITBLT |
					S3_CMD_WRITE |
					S3_CMD_DRAW |
					S3_CMD_DIR_TYPE_AXIAL |
					S3_CMD_AXIAL_X_LEFT_TO_RIGHT|
					S3_CMD_AXIAL_X_MAJOR |
					(flags & S3_ASM_TRANSFER_ACROSS_PLANE ?
						S3_CMD_PX_MD_ACROSS_PLANE :
						S3_CMD_PX_MD_THRO_PLANE ) |
					S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;
					
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));

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
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP, 
			current_destination_x);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP, 
			destination_y);
	/*
	 * Do the first vertical strip.
	 */
	if (fractional_width_start > 0)
	{
		S3_WAIT_FOR_FIFO(2);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
			offscreen_fractional_x_position);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 
			fractional_width_start - 1);

		if(fractional_height_start > 0)
		{
			S3_WAIT_FOR_FIFO(3);
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
				offscreen_fractional_y_position);
			S3_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
				((fractional_height_start - 1) & S3_MULTIFUNC_VALUE_BITS));
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
				command);
		}

		if(middle_heights_count > 0)
		{
			int tmp_middle_heights_count = middle_heights_count;

			if (screen_state_p->use_mmio)
			{
				S3_WAIT_FOR_FIFO(2);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
					offscreen_bitmap_y_location);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
					((offscreen_bitmap_height - 1) & S3_MULTIFUNC_VALUE_BITS));
				do
				{
					S3_WAIT_FOR_FIFO(1);
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}while(--tmp_middle_heights_count > 0);
			}
			else
			{
				S3_WAIT_FOR_FIFO(2);
				S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
					offscreen_bitmap_y_location);
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
					((offscreen_bitmap_height - 1) & S3_MULTIFUNC_VALUE_BITS));
				do
				{
					S3_WAIT_FOR_FIFO(1);
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}while(--tmp_middle_heights_count > 0);
			}
		}

		if(fractional_height_end > 0)
		{
			S3_WAIT_FOR_FIFO(3);
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
				offscreen_bitmap_y_location);
			S3_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
				((fractional_height_end - 1) & S3_MULTIFUNC_VALUE_BITS));
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
				command);
		}
		current_destination_x += fractional_width_start;
	}

	/*
	 * Do the middle vertical strips that take full offscreen tile widths.
	 */
	if (middle_widths_count > 0)
	{
		S3_WAIT_FOR_FIFO(2);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
			offscreen_bitmap_x_location);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 
			offscreen_bitmap_width - 1);

		if(screen_state_p->use_mmio)
		{
			do
			{
				S3_WAIT_FOR_FIFO(2);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP, destination_y);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP, 
					current_destination_x);
				if(fractional_height_start > 0)
				{
					S3_WAIT_FOR_FIFO(3);
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
						offscreen_fractional_y_position);
					S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
					((fractional_height_start - 1) & S3_MULTIFUNC_VALUE_BITS));

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}

				if(middle_heights_count > 0)
				{
					int tmp_middle_heights_count = middle_heights_count;

					S3_WAIT_FOR_FIFO(2);
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
						offscreen_bitmap_y_location);

					S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
					((offscreen_bitmap_height - 1) & S3_MULTIFUNC_VALUE_BITS));
					do
					{
						S3_WAIT_FOR_FIFO(1);
						S3_MMIO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_CMD, command);
					}while(--tmp_middle_heights_count > 0);
				}

				if(fractional_height_end > 0)
				{
					S3_WAIT_FOR_FIFO(3);
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
						offscreen_bitmap_y_location);

					S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
					((fractional_height_end - 1) & S3_MULTIFUNC_VALUE_BITS));

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}
				current_destination_x += offscreen_bitmap_width;
			} while (--middle_widths_count > 0);
		}
		else
		{
			do
			{
				S3_WAIT_FOR_FIFO(2);
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP, destination_y);
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP, 
					current_destination_x);
				if(fractional_height_start > 0)
				{
					S3_WAIT_FOR_FIFO(3);
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
						offscreen_fractional_y_position);
					S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
					((fractional_height_start - 1) & S3_MULTIFUNC_VALUE_BITS));

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}

				if(middle_heights_count > 0)
				{
					int tmp_middle_heights_count = middle_heights_count;

					S3_WAIT_FOR_FIFO(2);
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
						offscreen_bitmap_y_location);

					S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
					((offscreen_bitmap_height - 1) & S3_MULTIFUNC_VALUE_BITS));
					do
					{
						S3_WAIT_FOR_FIFO(1);
						S3_IO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_CMD, command);
					}while(--tmp_middle_heights_count > 0);
				}

				if(fractional_height_end > 0)
				{
					S3_WAIT_FOR_FIFO(3);
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
						offscreen_bitmap_y_location);

					S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
					((fractional_height_end - 1) & S3_MULTIFUNC_VALUE_BITS));

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}
				current_destination_x += offscreen_bitmap_width;
			} while (--middle_widths_count > 0);
		}
	}

	/*
	 * Do the last fractional vertical strip.
	 */
	if (fractional_width_end > 0)
	{
		S3_WAIT_FOR_FIFO(4);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP, 
				current_destination_x);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP, 
				destination_y);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
			offscreen_bitmap_x_location);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 
			fractional_width_end - 1);

		if(fractional_height_start > 0)
		{
			S3_WAIT_FOR_FIFO(3);
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
				offscreen_fractional_y_position);
			S3_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
				((fractional_height_start - 1) & S3_MULTIFUNC_VALUE_BITS));
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
				command);
		}

		if(middle_heights_count > 0)
		{
			int tmp_middle_heights_count = middle_heights_count;

			if (screen_state_p->use_mmio)
			{
				S3_WAIT_FOR_FIFO(2);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
					offscreen_bitmap_y_location);
				S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
				((offscreen_bitmap_height - 1) & S3_MULTIFUNC_VALUE_BITS));
				do
				{
					S3_WAIT_FOR_FIFO(1);
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}while(--tmp_middle_heights_count);
			}
			else
			{
				S3_WAIT_FOR_FIFO(2);
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
					offscreen_bitmap_y_location);
				S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
				((offscreen_bitmap_height - 1) & S3_MULTIFUNC_VALUE_BITS));
				do
				{
					S3_WAIT_FOR_FIFO(1);
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}while(--tmp_middle_heights_count);
			}
		}

		if(fractional_height_end > 0)
		{
			S3_WAIT_FOR_FIFO(3);
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
				offscreen_bitmap_y_location);
			S3_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
				((fractional_height_end - 1) & S3_MULTIFUNC_VALUE_BITS));
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
				command);
		}
	}
}

STATIC SIBool
s3_fill_rectangle_solid(SIint32 x_origin, SIint32 y_origin, 
						  SIint32 count, SIRectOutlineP rect_p)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURRENT_GRAPHICS_STATE_DECLARE();

	const int clip_left =
		screen_state_p->generic_state.screen_clip_left;
	const int clip_right =
		screen_state_p->generic_state.screen_clip_right;
	const int clip_top =
		screen_state_p->generic_state.screen_clip_top;
	const int clip_bottom =
		screen_state_p->generic_state.screen_clip_bottom;

	int		height = -1;
	int		width = -1;

#if (defined(__DEBUG__))
	if (s3_fill_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_fill_rectangle_solid) {\n"
			"\tx_origin = %ld\n"
			"\ty_origin = %ld\n"
			"\tcount = %ld\n"
			"\trect_p = %p\n"
			"\t}\n",
			x_origin, y_origin, count, (void *) rect_p);
	}
#endif
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE,
							 screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE,
							 graphics_state_p));

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}
	
#if (defined(__DEBUG__))
	if (graphics_state_p->generic_state.si_graphics_state.SGfillmode
		!= SGFillSolidFG &&
		graphics_state_p->generic_state.si_graphics_state.SGfillmode
		!= SGFillSolidBG)
	{
		/*CONSTANTCONDITION*/
		ASSERT(0);
	}
#endif

	/*
	 * Synchronize registers with the graphics state.
	 */
	S3_STATE_SYNCHRONIZE_STATE(screen_state_p, S3_FILL_SOLID_DEPENDENCIES);

	/*
	 * Select the foreground mix register and program foreground color
	 * source as either bkgd color or frgd color as appropriate.
	 */
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);

	S3_SET_FG_COLOR_SOURCE(
		(graphics_state_p->generic_state.si_graphics_state.SGfillmode == 
			SGFillSolidFG) ? S3_CLR_SRC_FRGD_COLOR: S3_CLR_SRC_BKGD_COLOR);

	/*
	 * loop through the list of rectangles.
	 */
	for (;count--;rect_p++)
	{
		register int xl = rect_p->x + x_origin;
		register int xr = xl + rect_p->width - 1; /* inclusive */
		register int yt = rect_p->y + y_origin;
		register int yb = yt + rect_p->height - 1; /* inclusive */


#if (defined(__DEBUG__))
		if (s3_fill_debug)
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
			if (s3_fill_debug)
			{
				(void) fprintf(debug_stream_p,
				"(s3_fill_rectangle_solid) {\n"
							   "\t\t(out of clip bounds)\n\t}\n");
			}
#endif

			continue;
		}

		if(screen_state_p->use_mmio)
		{
			if (height != rect_p->height)
			{
				height = rect_p->height;
				S3_WAIT_FOR_FIFO(1);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
					((height -1) & S3_MULTIFUNC_VALUE_BITS));
			}

			if (width != rect_p->width)
			{
				width = rect_p->width;
				S3_WAIT_FOR_FIFO(1);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, width - 1);
			}

			if (width <= 0 || height <= 0)
			{
				continue;
			}

			S3_INLINE_WAIT_FOR_FIFO(3);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, xl);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y, yt);

			S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
				screen_state_p->cmd_flags | 
				S3_CMD_PX_MD_ACROSS_PLANE |  
				S3_CMD_TYPE_RECTFILL |
				S3_CMD_WRITE | 
				S3_CMD_DRAW |
				S3_CMD_DIR_TYPE_AXIAL | 
				S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
				S3_CMD_AXIAL_X_MAJOR |
				S3_CMD_AXIAL_Y_TOP_TO_BOTTOM);
		}
		else
		{
			if (height != rect_p->height)
			{
				height = rect_p->height;
				S3_WAIT_FOR_FIFO(1);
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
					((height -1) & S3_MULTIFUNC_VALUE_BITS));
			}

			if (width != rect_p->width)
			{
				width = rect_p->width;
				S3_WAIT_FOR_FIFO(1);
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, width - 1);
			}

			if (width <= 0 || height <= 0)
			{
				continue;
			}

			S3_INLINE_WAIT_FOR_FIFO(3);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, xl);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y, yt);

			S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
				screen_state_p->cmd_flags | 
				S3_CMD_PX_MD_ACROSS_PLANE |  
				S3_CMD_TYPE_RECTFILL |
				S3_CMD_WRITE | 
				S3_CMD_DRAW |
				S3_CMD_DIR_TYPE_AXIAL | 
				S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
				S3_CMD_AXIAL_X_MAJOR |
				S3_CMD_AXIAL_Y_TOP_TO_BOTTOM);
		}
	}

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	return (SI_SUCCEED);
}

/*
 * Fillrect routine for SGfillmode = SGfillTile
 */

STATIC SIBool
s3_fill_rectangle_tile_system_memory( SIint32 x_origin, SIint32 y_origin,
	SIint32	count, SIRectOutlineP rect_p)
{
	/*
	 * Pointer to the Tile
	 */
	SIbitmapP		si_tile_p;

	/*
	 * Coords of the rectangle(inclusive)
	 */
	int			rect_x_left,rect_x_right,rect_y_top,rect_y_bottom;

	/*
	 * Number of pixels in a pixtrans word
	 */
	int			ppw;

	/*
	 * The source and destination coordinates
	 */
	int			start_x,start_y,destination_x,destination_y;


	/*
	 * Number of pixtrans words at the start.
	 * Number of tile widths in the middle.
	 * Number of pixtrans words at the end.
	 */
	int			startwords;
	int			midwidths;
	int			endwords;

	/*
	 * Length in pixels of the 1st transfer.
	 * Length in pixels of the last transfer.
	 * Length in pixels of the middle transfers.
	 */
	int		start_scan_x_length;
	int		end_scan_x_length;
	int		mid_scan_x_length;

	/*
	 * Where to stop the first transfer
	 */
	int		start_scissor;

	/*
	 * Ptr to the (0,y) coords in source tile.
	 * Ptr to the (x,y) coords in source tile.
	 * Ptr to the (x,0) coords in source tile.
	 */
	char        *src_y_ptr;
	char 		*src_xy_ptr;
	char		*src_x_ptr;

	unsigned short command;


	struct s3_tile_state *tile_state_p;

	S3_CURRENT_SCREEN_STATE_DECLARE();

	struct s3_enhanced_commands_register_state *s3_enhanced_registers_p =
		&(screen_state_p->register_state.s3_enhanced_commands_registers);

	S3_CURRENT_GRAPHICS_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
			(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE, graphics_state_p));

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	if (count <= 0)
	{
		return ( SI_SUCCEED );
	}

	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGfillmode == 
		SGFillTile);

	tile_state_p = &(graphics_state_p->current_tile_state);

	ASSERT(IS_OBJECT_STAMPED(S3_TILE_STATE, tile_state_p));

	si_tile_p =
		graphics_state_p->generic_state.si_graphics_state.SGtile;

	/*
	 * Make sure that the tilestate is already initialized, if not 
	 * call the download function now.
	 */
	if (!tile_state_p->tile_downloaded) 
	{
		s3_graphics_state_download_tile(screen_state_p,
			graphics_state_p,tile_state_p->si_tile_p);
	}	

	/*
	 * Synchronize registers with the graphics state.
	 */
	S3_STATE_SYNCHRONIZE_STATE(screen_state_p,
								 S3_FILL_TILE_DEPENDENCIES);

	ppw = screen_state_p->pixels_per_pixtrans;

#if (defined(__DEBUG__))
	if (s3_fill_debug || s3_fill_tile_debug)
	{
		(void) fprintf(debug_stream_p,
					"(s3_fill_rectangle_tile_system_memory)\n"
					"{\n"
					"\tx_origin = %ld\n"
					"\ty_origin = %ld\n"
					"\tcount = %ld\n"
					"\trect_p = %p\n",
					x_origin, y_origin, count, (void *) rect_p);
	}
#endif
	/*
	 * Tile one rectangle at at time.
	 */
	command = 
		screen_state_p->cmd_flags | 
		S3_CMD_TYPE_RECTFILL |
		S3_CMD_WRITE | 
		S3_CMD_USE_PIXTRANS | 
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;

     /*
	  * Choose the foreground mix logic and set cpu as source as color data. 
	  */
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_CPU_DATA);

	for (; count--; rect_p++)
	{
		int		curdx;
		int		curdy;
		int		cursy;

		/*
		 * get the coordinates of the destination rectangle.
		 */
		rect_x_left = rect_p->x + x_origin;
		rect_x_right = rect_x_left + rect_p->width - 1;
		rect_y_top = rect_p->y + y_origin;
		rect_y_bottom = rect_y_top + rect_p->height - 1;

#if (defined(__DEBUG__))
		if (s3_fill_debug || s3_fill_tile_debug)
		{
			(void) fprintf(debug_stream_p,
				"\t{\n"
				"\t\trect_x_left = %d\n"
				"\t\trect_y_top = %d\n"
				"\t\trect_x_right = %d\n"
				"\t\trect_y_bottom = %d\n"
				"\t}\n",
				rect_x_left, rect_y_top, rect_x_right, rect_y_bottom);
		}
#endif


		/*
		 * If the rectangle outside the clipping bounds, ignore it.
		 */
		if((rect_x_left > screen_state_p->generic_state.screen_clip_right) ||
		   (rect_x_right < screen_state_p->generic_state.screen_clip_left) ||
		   (rect_y_top > screen_state_p->generic_state.screen_clip_bottom) ||
		   (rect_y_bottom < screen_state_p->generic_state.screen_clip_top) ||
		   (rect_y_bottom < rect_y_top) || ( rect_x_right < rect_x_left ))
		{
#if (defined(__DEBUG__))
			if (s3_fill_debug || s3_fill_tile_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\tRectangle out of bounds\n");
			}
#endif
			continue;
		}

		/*
		 * Clip the rectangle to the clipping rectangle.
		 */
		if (rect_x_left <
			screen_state_p->generic_state.screen_clip_left)
		{
			rect_x_left =
				screen_state_p->generic_state.screen_clip_left;
		}
		if (rect_x_right >
			screen_state_p->generic_state.screen_clip_right)
		{
			rect_x_right =
				screen_state_p->generic_state.screen_clip_right;
		}
		if (rect_y_top <
			screen_state_p->generic_state.screen_clip_top)
		{
			rect_y_top =
				screen_state_p->generic_state.screen_clip_top;
		}
		if (rect_y_bottom >
			screen_state_p->generic_state.screen_clip_bottom)
		{
			rect_y_bottom =
				screen_state_p->generic_state.screen_clip_bottom;
		}

		destination_x = rect_x_left;
		destination_y = rect_y_top;

		/*
		 * Get the source coordinates corresponding to (rect_x_left,rect_y_top)
		 */
		start_x = (destination_x - si_tile_p->BorgX) % si_tile_p->Bwidth;
		start_y = (destination_y - si_tile_p->BorgY) % si_tile_p->Bheight;

		if ( start_x < 0 )
		{
			start_x += si_tile_p->Bwidth;
		}

		if ( start_y < 0 )
		{
			start_y += si_tile_p->Bheight;
		}

		/*
		 * Program the left scissor. Assumes that software pre-clipping 
		 * has been done.
		 */
		S3_WAIT_FOR_FIFO(1);
		S3_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		 	(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX | 
		 	(destination_x & S3_MULTIFUNC_VALUE_BITS)));
		
		/*
		 * Move destination_x back to a pixtrans word boundary.
		 */
		if (ppw)
		{
			int 			delta = 0;
			unsigned long	pixelmask = ppw - 1;

			/*
			 * check if the source origin is a pixtrans word boundary
			 */
			if (start_x & pixelmask) /* not a pixtrans word boundary */
			{
				/*
				 * Compute the number of pixels it is off from the
				 * previous word boundary and move start_x back to
				 * previous pixtrans word boundary .
				 * adjust destination_x by the corresponding amount
				 */
				delta = start_x & pixelmask;
				start_x &= ~pixelmask;
				destination_x -=  delta;
			}
		}

		/*
		 * Compute the source pointer to start with.
		 */
		src_y_ptr = (char *) si_tile_p->Bptr +
							  (tile_state_p->source_step * start_y);
		src_x_ptr = (char *) si_tile_p->Bptr;

		if (ppw)
		{
			src_xy_ptr = src_y_ptr +
				((start_x>>screen_state_p->pixels_per_pixtrans_shift) <<
				((unsigned)screen_state_p->pixtrans_width >> 4U));
			src_x_ptr = src_x_ptr +
				((start_x>>screen_state_p->pixels_per_pixtrans_shift) <<
				((unsigned)screen_state_p->pixtrans_width >> 4U));
		}
		else
		{
			src_xy_ptr = src_y_ptr +
				(((start_x * si_tile_p->BbitsPerPixel) >>
				   screen_state_p->pixtrans_width_shift) <<
				   ((unsigned)screen_state_p->pixtrans_width >> 4U));

			src_x_ptr = src_x_ptr +
				(((start_x * si_tile_p->BbitsPerPixel) >>
				   screen_state_p->pixtrans_width_shift) <<
				   ((unsigned)screen_state_p->pixtrans_width >> 4U));
		}

		/*
		 * Compute the following parameters.
		 * 1. The number of starting words before encountering the first
		 *    complete tile width.
		 * 2. The number of middle words - the full tile width transfers.
		 * 3. The number of end words - the last transfer which is less than
		 *    one full tile width.
		 */
		if ((destination_x +  (si_tile_p->Bwidth - start_x) - 1)
			< rect_x_right )
		{
			
			int	tmpdx = destination_x;

			if (ppw)
			{
				 startwords =
					tile_state_p->number_of_pixtrans_words_per_tile_width -
				   	(start_x >> screen_state_p->pixels_per_pixtrans_shift);
				 start_scan_x_length = startwords <<
					screen_state_p->pixels_per_pixtrans_shift;
			}
			else
			{
				start_scan_x_length = si_tile_p->Bwidth - start_x;
				startwords =
					tile_state_p->number_of_pixtrans_words_per_tile_width -
						((start_x * si_tile_p->BbitsPerPixel)
						 >> screen_state_p->pixels_per_pixtrans_shift);
			}
			tmpdx += si_tile_p->Bwidth - start_x;
			start_scissor = tmpdx - 1;

			midwidths = (rect_x_right - tmpdx + 1) / si_tile_p->Bwidth;
			endwords = (rect_x_right - tmpdx + 1) % si_tile_p->Bwidth;

			if (midwidths)
			{
				if ( ppw )
				{
					mid_scan_x_length = (si_tile_p->Bwidth + (ppw-1))
						& ~(ppw-1);
				}
				else
				{
					mid_scan_x_length = si_tile_p->Bwidth;
				}
				tmpdx += midwidths * si_tile_p->Bwidth;
			}

			if (endwords)
			{
				if (ppw)
				{
					endwords = (endwords + (ppw-1)) & ~(ppw-1);
					endwords  = endwords >>
								screen_state_p->pixels_per_pixtrans_shift;
					end_scan_x_length = endwords <<
								screen_state_p->pixels_per_pixtrans_shift;
				}
				else
				{
					end_scan_x_length = rect_x_right - tmpdx + 1;
					endwords = (endwords * si_tile_p->BbitsPerPixel) >>
								screen_state_p->pixels_per_pixtrans_shift;
				}
			}
		}
		else
		{
			/*
			 * This is the only transfer for this span.
			 * Here we know that destination_x is aligned at a pixtrans word.
			 */
			startwords = rect_x_right - destination_x + 1;
			start_scissor = rect_x_right;
			if (ppw)
			{
				startwords = (startwords + ppw - 1) & ~(ppw-1);
				startwords  = startwords >>
								screen_state_p->pixels_per_pixtrans_shift;
				start_scan_x_length = startwords <<
								screen_state_p->pixels_per_pixtrans_shift;
			}
			else
			{
				start_scan_x_length = startwords;
				startwords = (startwords * si_tile_p->BbitsPerPixel) >>
								screen_state_p->pixels_per_pixtrans_shift;
			}
			midwidths = 0;
			endwords = 0;
		}


		ASSERT(!(S3_IS_FIFO_OVERFLOW()));
		/*
		 * Tile the destination rectangle.
		 * Finish with the start words first.
		 */
		{
			char	*src_tile_p ;

			curdx = destination_x;
			curdy = destination_y;
			cursy = start_y;

			S3_WAIT_FOR_FIFO(3);
			if (screen_state_p->use_mmio)
			{
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
					(start_scissor & S3_MULTIFUNC_VALUE_BITS)));

				S3_MMIO_SET_ENHANCED_REGISTER
					(S3_ENHANCED_COMMAND_REGISTER_CUR_X, curdx);

				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
					start_scan_x_length - 1);
			}
			else
			{
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
					(start_scissor & S3_MULTIFUNC_VALUE_BITS)));

				S3_IO_SET_ENHANCED_REGISTER
					(S3_ENHANCED_COMMAND_REGISTER_CUR_X, curdx);

				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
					start_scan_x_length - 1);
			}
			src_tile_p = src_xy_ptr;

			ASSERT(startwords > 0);

			while (curdy <= rect_y_bottom)
			{
				SIint32		tilelines;
				SIint32		tmplines;

				tilelines = ((curdy + si_tile_p->Bheight - cursy) >
							rect_y_bottom )
					? rect_y_bottom - curdy + 1
					: si_tile_p->Bheight - cursy;
				/*
				 *Set destination blit rectangles from top to bottom
				 * that would take this portion of the tile.
				 */
				S3_WAIT_FOR_FIFO(3);
				if (screen_state_p->use_mmio)
				{
					S3_MMIO_SET_ENHANCED_REGISTER
						(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, curdy);

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
						S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
						((tilelines - 1) & S3_MULTIFUNC_VALUE_BITS));

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}
				else
				{
					S3_IO_SET_ENHANCED_REGISTER
						(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, curdy);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
						S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
						((tilelines - 1) & S3_MULTIFUNC_VALUE_BITS));

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}
				tmplines = tilelines;
				S3_WAIT_FOR_FIFO(8);
				while ( tmplines-- )
				{
					(*screen_state_p->screen_write_pixels_p)
						(screen_state_p->pixtrans_register,
						startwords,
						src_tile_p);

					src_tile_p += tile_state_p->source_step;
				}
				curdy += tilelines;

				/*
				 * Now start from the first line of the tile.
				 */
				src_tile_p = src_x_ptr;
				cursy = 0;
			}
			curdx = start_scissor + 1;
		}

		ASSERT(!(S3_IS_FIFO_OVERFLOW()));
		if (midwidths)
		{
			char	*src_tile_p;

			curdy = destination_y;
			cursy = start_y;

			src_tile_p = src_y_ptr;
			while (curdy <= rect_y_bottom)
			{
				int		tilelines;
				int		tmpwidths;
				int		tmpdx;

				tmpdx = curdx;
				tilelines =
					((curdy + si_tile_p->Bheight - cursy) >
					 rect_y_bottom ) ? rect_y_bottom - curdy + 1
						: si_tile_p->Bheight - cursy;
				tmpwidths = midwidths;

				ASSERT(tile_state_p->
					   number_of_pixtrans_words_per_tile_width > 0);

				while (tmpwidths--)
				{
					int		tmplines;
					char 	*src_line_p;

					tmplines = tilelines;
					src_line_p = src_tile_p;

					/*
					 * Set up a destination Blit for this portion of the
					 * tile.
					 */
					S3_WAIT_FOR_FIFO(6);
					if (screen_state_p->use_mmio)
					{
						S3_MMIO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
							(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
							((tmpdx + si_tile_p->Bwidth - 1) &
							 S3_MULTIFUNC_VALUE_BITS)));

						S3_MMIO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_CUR_X, tmpdx);

						S3_MMIO_SET_ENHANCED_REGISTER(
							  S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
								 mid_scan_x_length - 1);

						S3_MMIO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_CUR_Y,curdy);

						S3_MMIO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
							S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
							((tilelines - 1) & S3_MULTIFUNC_VALUE_BITS));

						S3_MMIO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_CMD, command);
					}
					else
					{
						S3_IO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
							(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
							((tmpdx + si_tile_p->Bwidth - 1) &
							 S3_MULTIFUNC_VALUE_BITS)));

						S3_IO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_CUR_X, tmpdx);

						S3_IO_SET_ENHANCED_REGISTER(
							  S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
								 mid_scan_x_length - 1);

						S3_IO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_CUR_Y,curdy);

						S3_IO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
							S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
							((tilelines - 1) & S3_MULTIFUNC_VALUE_BITS));

						S3_IO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_CMD, command);
					}

					S3_WAIT_FOR_FIFO(8);
					while (tmplines--)
					{
						(*screen_state_p->screen_write_pixels_p)
							(screen_state_p->pixtrans_register,
							 tile_state_p->
								number_of_pixtrans_words_per_tile_width,
							 src_line_p);

						src_line_p += tile_state_p->source_step;
					}
					tmpdx += si_tile_p->Bwidth;
				}
				curdy += tilelines;

				/*
				 * First line of the tile.
				 */
				src_tile_p = (char *) si_tile_p->Bptr;
				cursy = 0;
			}
			curdx += midwidths*si_tile_p->Bwidth;
		}

		ASSERT(!(S3_IS_FIFO_OVERFLOW()));
		if (endwords)
		{
			char	*src_line_ptr;

			curdy = destination_y;
			cursy = start_y;

			/*
			 * Setup the Constant portion of the Destination Blit rectangle.
			 */
			S3_WAIT_FOR_FIFO(3);
			if (screen_state_p->use_mmio)
			{
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
					(rect_x_right & S3_MULTIFUNC_VALUE_BITS)));

				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X, curdx);

				S3_MMIO_SET_ENHANCED_REGISTER(
					 S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
					 (end_scan_x_length - 1));
			}
			else
			{
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
					(rect_x_right & S3_MULTIFUNC_VALUE_BITS)));

				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X, curdx);

				S3_IO_SET_ENHANCED_REGISTER(
					 S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
					 (end_scan_x_length - 1));
			}

			src_line_ptr = src_y_ptr;
			 
			while (curdy <= rect_y_bottom)
			{
				int	tilelines;
				int	tmplines;

				tilelines =
					((curdy + si_tile_p->Bheight - cursy) > rect_y_bottom )
						? rect_y_bottom - curdy + 1
						: si_tile_p->Bheight - cursy;

				/*
				 * setup the Remaining part of the Destination Blit
				 *  rectangle.
				 */
				S3_WAIT_FOR_FIFO(3);
				if (screen_state_p->use_mmio)
				{
					S3_MMIO_SET_ENHANCED_REGISTER
						 (S3_ENHANCED_COMMAND_REGISTER_CUR_Y, curdy);

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
						S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
						((tilelines - 1) & S3_MULTIFUNC_VALUE_BITS));

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}
				else
				{
					S3_IO_SET_ENHANCED_REGISTER
						 (S3_ENHANCED_COMMAND_REGISTER_CUR_Y, curdy);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
						S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX |
						((tilelines - 1) & S3_MULTIFUNC_VALUE_BITS));

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}

				tmplines = tilelines;
				S3_WAIT_FOR_FIFO(8);
				ASSERT(endwords > 0);
				while (tmplines--)
				{
					(*screen_state_p->screen_write_pixels_p)
						(screen_state_p->pixtrans_register,
						 endwords,
						 src_line_ptr);

					src_line_ptr += tile_state_p->source_step;
				}

				src_line_ptr = (char *) si_tile_p->Bptr;
				cursy = 0;
				curdy += tilelines;
			}
		}
	}

#if (defined(__DEBUG__))
	if (s3_fill_debug || s3_fill_tile_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	/*
	 * Synchronize clip registers.
	 */

	S3_WAIT_FOR_FIFO(2);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX |
		(s3_enhanced_registers_p->scissor_l & S3_MULTIFUNC_VALUE_BITS)));
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
		(s3_enhanced_registers_p->scissor_r & S3_MULTIFUNC_VALUE_BITS)));

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return (SI_SUCCEED);
}
	
STATIC SIBool
s3_fill_rectangle_tile_ge_patblt(
	SIint32 x_origin, 
	SIint32 y_origin,
	SIint32	count, 
	SIRectOutlineP rect_p)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURRENT_GRAPHICS_STATE_DECLARE();

	struct s3_tile_state *tile_state_p;
	int	offscreen_tile_x;
	int	offscreen_tile_y;
	int command;

    const int screen_clip_left = 
		screen_state_p->generic_state.screen_clip_left;
    const int screen_clip_right = 
		screen_state_p->generic_state.screen_clip_right;
    const int screen_clip_top = 
		screen_state_p->generic_state.screen_clip_top;
    const int screen_clip_bottom = 
		screen_state_p->generic_state.screen_clip_bottom;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE, graphics_state_p));

#if (defined(__DEBUG__))
	if (s3_fill_debug)
	{
		(void) fprintf(debug_stream_p,
		   "(s3_fill_rectangle_tile_ge_patblt)\n"
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

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	tile_state_p = &(graphics_state_p->current_tile_state);
	ASSERT(IS_OBJECT_STAMPED(S3_TILE_STATE, tile_state_p));
	ASSERT(tile_state_p->is_small_tile == TRUE);

    /*
     * Make sure that the tile is already downloaded, if not
     * download it now
     */

	if ((!tile_state_p->tile_downloaded) ||
		(tile_state_p->tile_origin_x != 
		graphics_state_p->generic_state.si_graphics_state.SGtile->BorgX) ||
		(tile_state_p->tile_origin_y != 
		graphics_state_p->generic_state.si_graphics_state.SGtile->BorgY)) 
    {
		if (tile_state_p->is_reduced_tile)
		{
			ASSERT((&tile_state_p->reduced_tile) ==
					tile_state_p->si_tile_p);
			/*
			 * Update origin
			 */

			tile_state_p->si_tile_p->BorgX =
				graphics_state_p->generic_state.
				si_graphics_state.SGtile->BorgX;

			tile_state_p->si_tile_p->BorgY =
				graphics_state_p->generic_state.
				si_graphics_state.SGtile->BorgY;
		}

        s3_graphics_state_download_tile(screen_state_p,
            graphics_state_p,
			tile_state_p->si_tile_p);
    }

	/*
	 * Synchronize registers with the graphics state.
	 */
	S3_STATE_SYNCHRONIZE_STATE(screen_state_p, 
		S3_FILL_TILE_PATBLT_DEPENDENCIES);

	/*
	 * Set the clipping rectangle to select the entire drawing area.
	 */
	if (screen_state_p->generic_state.screen_current_clip !=
		GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
		S3_STATE_SET_CLIP_RECTANGLE(screen_state_p, 0, 0,
			screen_state_p->generic_state.screen_virtual_width,
			screen_state_p->generic_state.screen_virtual_height);

		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
		S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);
	}

	ASSERT(!(tile_state_p->offscreen_width &
		(tile_state_p->offscreen_width -1)));
	ASSERT(!(tile_state_p->offscreen_height &
		(tile_state_p->offscreen_height -1)));

	offscreen_tile_x = tile_state_p->offscreen_location_x;
	offscreen_tile_y = tile_state_p->offscreen_location_y;

	if (!OMM_LOCK(tile_state_p->offscreen_location_p))
	{
		/*
		 * We have to try system memory tiling
		 */
#if	defined(__DEBUG__)
		if (s3_fill_debug)
		{
			(void)fprintf(debug_stream_p,"#OMM lock failed!");
		}
#endif
		return (s3_fill_rectangle_tile_system_memory(x_origin,
													 y_origin,
													 count,
													 rect_p));
	}
	
	/*
	 * Choose FRGD_MIX as the mix register, video memory is the 
	 * foreground color source
	 */
  
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_VIDEO_DATA);

	command = screen_state_p->cmd_flags | 
		S3_CMD_TYPE_PATTERNFILL |
		S3_CMD_PX_MD_THRO_PLANE |
		S3_CMD_WRITE | 
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;

	S3_WAIT_FOR_FIFO(2);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,
		offscreen_tile_x);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
		offscreen_tile_y);

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

#if (defined(__DEBUG__))
		if (s3_fill_debug)
		{
			(void) fprintf(debug_stream_p,
			   "\t{\n"
			   "\t\tx_left = %d\n"
			   "\t\ty_top = %d\n"
			   "\t\twidth = %d\n"
			   "\t\theight = %d\n"
			   "\t}\n",
			   x_left, y_top, rect_p->width, rect_p->height);
		}
#endif

		/*
		 * Check if the rectangle is outside the current clipping
		 * bounds. Discard the rectangle if so.
		 */
		if((x_left > screen_clip_right) || (x_right < screen_clip_left) ||
		   (y_top > screen_clip_bottom) || (y_bottom < screen_clip_top) ||
		   (y_bottom < y_top) || ( x_right < x_left ))
		{
#if (defined(__DEBUG__))
			if (s3_fill_debug)
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

		if (((width = x_right - x_left) < 0) || 
			((height = y_bottom - y_top) < 0))
		{
			++rect_p;
			continue;
		}
		/*
		 * The clip rectangle has to be set to the inner rectangle
		 */
		if (screen_state_p->use_mmio)
		{
			S3_WAIT_FOR_FIFO(5);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP,
				x_left);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP,
				y_top);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
				width );
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
				(height & S3_MULTIFUNC_VALUE_BITS));
			S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
				command);
		}
		else
		{
			S3_WAIT_FOR_FIFO(5);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP,
				x_left);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP,
				y_top);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
				width );
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
				(height & S3_MULTIFUNC_VALUE_BITS));
			S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
				command);
		}
		++rect_p;
	}while (--count);

#if (defined(__DEBUG__))
	if (s3_fill_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	OMM_UNLOCK(tile_state_p->offscreen_location_p);

	ASSERT(!S3_IS_FIFO_OVERFLOW());
	return (SI_SUCCEED);
}

/*
 * Tiling from offscreen memory without involving the ge's patblt 
 * capability.
 */

STATIC SIBool
s3_fill_rectangle_tile_offscreen_memory(
	SIint32 x_origin, 
	SIint32 y_origin,
	SIint32	count, 
	SIRectOutlineP rect_p)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURRENT_GRAPHICS_STATE_DECLARE();

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
	
	struct s3_tile_state *tile_state_p;	
	SIbitmap *si_tile_p;
	

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE, graphics_state_p));
	
	tile_state_p = &(graphics_state_p->current_tile_state);

	si_tile_p =
		graphics_state_p->generic_state.si_graphics_state.SGtile;
	
	ASSERT(IS_OBJECT_STAMPED(S3_TILE_STATE, tile_state_p));
#if (defined(__DEBUG__))
	if (s3_fill_debug)
	{
		(void) fprintf(debug_stream_p,
		   "(s3_fill_rectangle_tile_offscreen_memory)\n"
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
		s3_graphics_state_download_tile(screen_state_p,
			graphics_state_p,
			tile_state_p->si_tile_p);
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
		if (s3_fill_debug)
		{
			(void)fprintf(debug_stream_p,"#OMM lock failed!");
		}
#endif
		return (s3_fill_rectangle_tile_system_memory(x_origin,
					y_origin, count, rect_p));
	}

	S3_STATE_SYNCHRONIZE_STATE(screen_state_p, 
				S3_FILL_TILE_PATBLT_DEPENDENCIES);	   	
	/*
	 * Choose FRGD_MIX as the mix register, video memory is the 
	 * foreground color source
	 */
  
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_VIDEO_DATA);
	
	S3_STATE_SET_RD_MASK(screen_state_p,(1 << si_tile_p->BbitsPerPixel) - 1);
	
	offscreen_tile_x_location = tile_state_p->offscreen_location_x;
	offscreen_tile_y_location = tile_state_p->offscreen_location_y;
	offscreen_tile_width = 
		(tile_state_p->offscreen_width/si_tile_p->Bwidth) * 
		si_tile_p->Bwidth;
	offscreen_tile_height = 
		(tile_state_p->offscreen_height/si_tile_p->Bheight) * 
		si_tile_p->Bheight;
	
	/*
	 * Set the clipping rectangle to select the entire drawing area.
	 */
	if (screen_state_p->generic_state.screen_current_clip !=
		GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
		S3_STATE_SET_CLIP_RECTANGLE(screen_state_p, 0, 0,
			screen_state_p->generic_state.screen_virtual_width,
			screen_state_p->generic_state.screen_virtual_height);

		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
		S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);
	}

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
		if((x_left > screen_state_p->generic_state.screen_clip_right) ||
		   (x_right < screen_state_p->generic_state.screen_clip_left) ||
		   (y_top > screen_state_p->generic_state.screen_clip_bottom) ||
		   (y_bottom < screen_state_p->generic_state.screen_clip_top) ||
		   (y_bottom < y_top) || ( x_right < x_left ))
		{
#if (defined(__DEBUG__))
			if (s3_fill_debug)
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

		s3_fill_rectangle_offscreen_helper(
			si_tile_p,
			offscreen_tile_x_location, offscreen_tile_y_location,
			offscreen_tile_width, offscreen_tile_height,
			x_left, y_top, 
			x_right - x_left + 1, y_bottom - y_top + 1,
			S3_ASM_TRANSFER_THRO_PLANE);
		++rect_p;
	}while(--count);	

#if (defined(__DEBUG__))
	if(s3_fill_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	ASSERT(!S3_IS_FIFO_OVERFLOW());
	return (SI_SUCCEED);
}


/*
 * Stippling entry point for rectangles.
 */
STATIC SIBool
s3_fill_rectangle_stipple_system_memory(
	SIint32 x_origin, 
	SIint32 y_origin,
	SIint32	count, 
	SIRectOutlineP rect_p)
{
	/*
	 * Pointer to the stipple
	 */
	SIbitmapP		source_stipple_p;

	/*
	 * Coords of the rectangle(inclusive)
	 */
	int			rect_x_left,rect_x_right,rect_y_top,rect_y_bottom;

	/*
	 * The source and destination coordinates
	 */
	int			start_x,start_y,destination_x,destination_y;

	/*
	 * Number of pixtrans words at the start.
	 * Number of stipple widths in the middle.
	 * Number of pixtrans words at the end.
	 */
	int			startwords;
	int			midwidths;
	int			endwords;

	/*
	 * Length in pixels of the 1st transfer.
	 * Length in pixels of the last transfer.
	 * Length in pixels of the middle transfers.
	 */
	int		start_scan_x_length;
	int		end_scan_x_length;
	int		mid_scan_x_length;

	/*
	 * Where to stop the first transfer
	 */
	int		start_scissor;
	unsigned short command;
	/*
	 * Ptr to the (0,y) coords in source stipple.
	 * Ptr to the (x,y) coords in source stipple.
	 * Ptr to the (x,0) coords in source stipple.
	 */
	char		*src_y_ptr;
	char		*src_xy_ptr;
	char		*src_x_ptr;

	struct s3_stipple_state *stipple_state_p;

	S3_CURRENT_SCREEN_STATE_DECLARE();
	struct s3_enhanced_commands_register_state *s3_enhanced_registers_p =
		&(screen_state_p->register_state.s3_enhanced_commands_registers);

	S3_CURRENT_GRAPHICS_STATE_DECLARE();

	/*
	 * Pointer to the si's stipple.
	 */
	SIbitmapP		si_stipple_p =
		graphics_state_p->generic_state.si_graphics_state.SGstipple;


	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE,
							 screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE,
							 graphics_state_p));

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	if (count <= 0)
	{
		return ( SI_SUCCEED );
	}

	/*
	 * Really an assert.
	 */
	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGfillmode
			== SGFillStipple );

	stipple_state_p = &(graphics_state_p->current_stipple_state);
	ASSERT(IS_OBJECT_STAMPED(S3_STIPPLE_STATE, stipple_state_p));

	/*
	 * Make sure that the stipplestate is already initialized, if not 
	 * call the download function now.
	 */
	if (!stipple_state_p->stipple_downloaded) 
	{
		s3_graphics_state_download_stipple(screen_state_p,
			graphics_state_p,
			stipple_state_p->si_stipple_p);
	}	

	source_stipple_p = &(stipple_state_p->inverted_stipple);

	if (graphics_state_p->generic_state.si_graphics_state.SGstplmode
		== SGStipple)
	{
		/*
		 * Synchronize registers with the graphics state.
		 */
		S3_STATE_SYNCHRONIZE_STATE(screen_state_p,
							 S3_FILL_STIPPLE_TRANSPARENT_DEPENDENCIES);
		S3_STATE_SET_BG_ROP(screen_state_p, S3_MIX_FN_LEAVE_C_AS_IS);
	}
	else
	{
		/*
		 * Synchronize registers with the graphics state.
		 */
		S3_STATE_SYNCHRONIZE_STATE(screen_state_p,
							 S3_FILL_STIPPLE_OPAQUE_DEPENDENCIES);
	}



#if (defined(__DEBUG__))
	if (s3_fill_debug || s3_fill_stipple_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_fill_rectangle_stipple_system_memory)\n"
			"{\n"
			"\tx_origin = %ld\n"
			"\ty_origin = %ld\n"
			"\tcount = %ld\n"
			"\trect_p = %p\n",
			x_origin, y_origin, count, (void *) rect_p);
	}
	
#endif
	command = screen_state_p->cmd_flags | 
		S3_CMD_TYPE_RECTFILL |
		S3_CMD_WRITE | 
		S3_CMD_USE_PIXTRANS | 
		S3_CMD_DRAW |
		S3_CMD_PX_MD_ACROSS_PLANE |	
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;

		/*
		 * Cpu data decides which mix engine to select. 
		 * Program each foreground mix engind to select foreground color
		 * and the background mix engine to select the background color.
		 */
		S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_CPU_DATA);
		S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
		S3_SET_BG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);

	for (; count--; rect_p++ )
	{
		int		curdx;
		int		curdy;
		int		cursy;

		/*
		 * get the coordinates of the destination rectangle.
		 */
		rect_x_left = rect_p->x + x_origin;
		rect_x_right = rect_x_left + rect_p->width - 1;
		rect_y_top = rect_p->y + y_origin;
		rect_y_bottom = rect_y_top + rect_p->height - 1;

#if (defined(__DEBUG__))
		if (s3_fill_debug || s3_fill_stipple_debug)
		{
			(void) fprintf(debug_stream_p,
				"\t{\n"
				"\t\trect_x_left = %d\n"
				"\t\trect_y_top = %d\n"
				"\t\trect_x_right = %d\n"
				"\t\trect_y_bottom = %d\n"
				"\t}\n",
				rect_x_left, rect_y_top, rect_x_right, rect_y_bottom);
		}
#endif


		/*
		 * If the rectangle outside the clipping bounds, ignore it.
		 */
		if((rect_x_left > screen_state_p->generic_state.screen_clip_right) ||
			(rect_x_right < screen_state_p->generic_state.screen_clip_left) ||
			(rect_y_top > screen_state_p->generic_state.screen_clip_bottom) ||
			(rect_y_bottom < screen_state_p->generic_state.screen_clip_top) ||
			(rect_y_bottom < rect_y_top) || ( rect_x_right < rect_x_left ))
		{
#if (defined(__DEBUG__))
			if (s3_fill_debug || s3_fill_stipple_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\tRectangle out of bounds\n");
			}
#endif
			continue;
		}

		/*
		 * Clip the rectangle to the clipping rectangle.
		 */
		if (rect_x_left < screen_state_p->generic_state.screen_clip_left)
		{
			rect_x_left = screen_state_p->generic_state.screen_clip_left;
		}

		if (rect_x_right > screen_state_p->generic_state.screen_clip_right)
		{
			rect_x_right = screen_state_p->generic_state.screen_clip_right;
		}

		if (rect_y_top < screen_state_p->generic_state.screen_clip_top)
		{
			rect_y_top = screen_state_p->generic_state.screen_clip_top;
		}

		if (rect_y_bottom > screen_state_p->generic_state.screen_clip_bottom)
		{
			rect_y_bottom = screen_state_p->generic_state.screen_clip_bottom;
		}

		destination_x = rect_x_left;
		destination_y = rect_y_top;

		/*
		 * Get the source coordinates corresponding to (rect_x_left,rect_y_top)
		 */
		start_x = (destination_x - si_stipple_p->BorgX) %
			si_stipple_p->Bwidth;
		start_y = (destination_y - si_stipple_p->BorgY) %
			si_stipple_p->Bheight;

		if ( start_x < 0 )
		{
			start_x += source_stipple_p->Bwidth;
		}
		if ( start_y < 0 )
		{
			start_y += source_stipple_p->Bheight;
		}

		/*
		 * Set the left scissor to correspond to the left edge of the
		 * rectangle. Assumes software clipping is done already.
		 */
		S3_WAIT_FOR_FIFO(1);
		S3_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
			(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX |
			(destination_x & S3_MULTIFUNC_VALUE_BITS)));
		/*
		 * Move destination_x back to a pixtrans word boundary.
		 */
		if ( start_x & (screen_state_p->pixtrans_width - 1) )
		{
			/* 
			 * Not a pixtrans word boundary 
			 * Compute the number of pixels it is off from the
			 * previous word boundary and move start_x back to
			 * previous pixtrans word boundary .
			 * adjust destination_x by the corresponding amount
			 */
			destination_x -= start_x & (screen_state_p->pixtrans_width - 1 );
			start_x &= ~(screen_state_p->pixtrans_width - 1 );
		}

		/*
		 * Compute the source pointer to start with.
		 */
		src_y_ptr = (char *)source_stipple_p->Bptr +
							  (stipple_state_p->source_step * start_y);
		src_x_ptr = (char *) source_stipple_p->Bptr;
		src_xy_ptr = src_y_ptr +
					 ((start_x >> screen_state_p->pixtrans_width_shift) <<
					  ((unsigned)screen_state_p->pixtrans_width >> 4U));
		src_x_ptr = src_x_ptr +
					((start_x >> screen_state_p->pixtrans_width_shift) <<
					  ((unsigned)screen_state_p->pixtrans_width >> 4U));

		/*
		 * Compute the following parameters.
		 * 1. The number of starting words before encountering the first
		 *    complete stipple width.
		 * 2. The number of middle words - the full stipple width transfers.
		 * 3. The number of end words - the last transfer which is less than
		 *    one full stipple width.
		 */
		if((destination_x +  (source_stipple_p->Bwidth -start_x) - 1) <
		   rect_x_right)
		{
			/*
			 * More transfers are there for this span
			 */
			int	tmpdx = destination_x;

			startwords =
				 stipple_state_p->number_of_pixtrans_words_per_stipple_width -
				(start_x >> screen_state_p->pixtrans_width_shift);

			start_scan_x_length =
				(startwords << screen_state_p->pixtrans_width_shift);

			tmpdx += source_stipple_p->Bwidth - start_x;
			start_scissor = tmpdx - 1;

			midwidths = (rect_x_right - tmpdx + 1) / source_stipple_p->Bwidth;
			endwords = (rect_x_right - tmpdx + 1) % source_stipple_p->Bwidth;

			if ( midwidths )
			{
				mid_scan_x_length =
					 (source_stipple_p->Bwidth +
						 (screen_state_p->pixtrans_width-1)) &
						 ~(screen_state_p->pixtrans_width - 1);
				tmpdx += midwidths * source_stipple_p->Bwidth;
			}

			if ( endwords )
			{
				endwords = (endwords + (screen_state_p->pixtrans_width-1)) &
					 ~(screen_state_p->pixtrans_width - 1);
				endwords  = endwords >> screen_state_p->pixtrans_width_shift;
				end_scan_x_length = endwords <<
					screen_state_p->pixtrans_width_shift;
			}
		}
		else
		{
			/*
			 * This is the only transfer for this span.
			 * Here we know that destination_x is aligned at a pixtrans word.
			 */
			startwords = rect_x_right - destination_x + 1;
			start_scissor = rect_x_right;
			startwords = (startwords + screen_state_p->pixtrans_width - 1)
									& ~(screen_state_p->pixtrans_width - 1);
			startwords  = startwords >> screen_state_p->pixtrans_width_shift;
			start_scan_x_length = startwords <<
				screen_state_p->pixtrans_width_shift;
			midwidths = 0;
			endwords = 0;
		}

		/*
		 * Stipple the destination rectangle.
		 * Finish with the start words first.
		 */
		{
			char	*src_stipple_p ;

			curdx = destination_x;
			curdy = destination_y;
			cursy = start_y;

			S3_WAIT_FOR_FIFO(3);
			if (screen_state_p->use_mmio)
			{
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
					(start_scissor & S3_MULTIFUNC_VALUE_BITS)));

				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X, curdx);

				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
					start_scan_x_length - 1);
			}
			else
			{
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
					(start_scissor & S3_MULTIFUNC_VALUE_BITS)));

				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X, curdx);

				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
					start_scan_x_length - 1);
			}

			src_stipple_p = src_xy_ptr;
			while ( curdy <= rect_y_bottom )
			{
				int		stipplelines;
				int		tmplines;

				stipplelines =
					((curdy + source_stipple_p->Bheight - cursy) >
					 rect_y_bottom )
						? rect_y_bottom - curdy + 1
						: source_stipple_p->Bheight - cursy;
				/*
				 * Set destination blit rectangles from top to bottom
				 * that would take this portion of the stipple.
				 */
				S3_WAIT_FOR_FIFO(3);
				if (screen_state_p->use_mmio)
				{
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
						curdy);

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
						S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
						((stipplelines - 1) & S3_MULTIFUNC_VALUE_BITS));

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}
				else
				{
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
						curdy);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
						S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
						((stipplelines - 1) & S3_MULTIFUNC_VALUE_BITS));

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}
				tmplines = stipplelines;

				S3_WAIT_FOR_FIFO(8);
				while ( tmplines-- )
				{

					(*screen_state_p->screen_write_bits_p)
						(screen_state_p->pixtrans_register,
						 startwords, 
						 (void *) src_stipple_p);
					src_stipple_p += stipple_state_p->source_step;
				}
				curdy += stipplelines;
				/*
				 * Now start from the first line of the stipple.
				 */
				src_stipple_p = src_x_ptr;
				cursy = 0;
			}
			curdx = start_scissor + 1;
		}

		if (midwidths)
		{
			char		*src_stipple_p;

			curdy = destination_y;
			cursy = start_y;

			src_stipple_p = src_y_ptr;
			while ( curdy <= rect_y_bottom )
			{
				int		stipplelines;
				int		tmpwidths;
				int		tmpdx;

				tmpdx = curdx;
				stipplelines =
					((curdy + source_stipple_p->Bheight - cursy) >
					 rect_y_bottom )
						? rect_y_bottom - curdy + 1
						: source_stipple_p->Bheight - cursy;
				tmpwidths = midwidths;

				while (tmpwidths--)
				{
					int		tmplines;
					char 	*src_line_p;

					tmplines = stipplelines;
					src_line_p = src_stipple_p;

					/*
					 * Set up a destination Blit for this portion of the
					 * stipple.
					 */
					S3_WAIT_FOR_FIFO(7);
					if (screen_state_p->use_mmio)
					{
						S3_MMIO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
							(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
							((tmpdx + source_stipple_p->Bwidth - 1) &
								 S3_MULTIFUNC_VALUE_BITS)));

						S3_MMIO_SET_ENHANCED_REGISTER (
							S3_ENHANCED_COMMAND_REGISTER_CUR_X, tmpdx);
						S3_MMIO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_CUR_Y, curdy);

						S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
						  (mid_scan_x_length - 1));
						S3_MMIO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
							S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
							((stipplelines - 1) & S3_MULTIFUNC_VALUE_BITS));

						S3_MMIO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_CMD, command);
					}
					else
					{
						S3_IO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
							(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
							((tmpdx + source_stipple_p->Bwidth - 1) &
								 S3_MULTIFUNC_VALUE_BITS)));

						S3_IO_SET_ENHANCED_REGISTER (
							S3_ENHANCED_COMMAND_REGISTER_CUR_X, tmpdx);
						S3_IO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_CUR_Y, curdy);

						S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
						  (mid_scan_x_length - 1));
						S3_IO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
							S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
							((stipplelines - 1) & S3_MULTIFUNC_VALUE_BITS));

						S3_IO_SET_ENHANCED_REGISTER(
							S3_ENHANCED_COMMAND_REGISTER_CMD, command);
					}

					S3_WAIT_FOR_FIFO(8);
					while ( tmplines-- )
					{
						(*screen_state_p->screen_write_bits_p)
							(screen_state_p->pixtrans_register,
							stipple_state_p->
								number_of_pixtrans_words_per_stipple_width,
							 (void *) src_line_p);
						src_line_p += stipple_state_p->source_step;
					}
					tmpdx += source_stipple_p->Bwidth;
				}
				curdy += stipplelines;

				/*
				 * First line of the stipple.
				 */
				src_stipple_p = (char *) source_stipple_p->Bptr;
				cursy = 0;
			}
			curdx += midwidths*source_stipple_p->Bwidth;
		}

		if (endwords)
		{
			char	*src_line_ptr;

			curdy = destination_y;
			cursy = start_y;

			/*
			 * Setup the Constant portion of the
			 * Destination Blit rectangle.
			 */
			S3_WAIT_FOR_FIFO(3);
			if (screen_state_p->use_mmio)
			{
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX | 
						(rect_x_right & S3_MULTIFUNC_VALUE_BITS)));

				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X, curdx);

				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
					end_scan_x_length - 1);
			}
			else
			{
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
					(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX | 
						(rect_x_right & S3_MULTIFUNC_VALUE_BITS)));

				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X, curdx);

				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
					end_scan_x_length - 1);
			}

			src_line_ptr = src_y_ptr;

			while ( curdy <= rect_y_bottom )
			{
				int	stipplelines;
				int	tmplines;

				stipplelines =
					((curdy + source_stipple_p->Bheight - cursy) >
					 rect_y_bottom )
						? rect_y_bottom - curdy + 1
						: source_stipple_p->Bheight - cursy;

				/*
				 * setup the Remaining part of the
				 * Destination Blit rectangle.
				 */
				S3_WAIT_FOR_FIFO(3);
				if (screen_state_p->use_mmio)
				{
					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y, curdy);

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
						S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
						((stipplelines - 1)  & S3_MULTIFUNC_VALUE_BITS));

					S3_MMIO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}
				else
				{
					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CUR_Y, curdy);

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
						S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
						((stipplelines - 1)  & S3_MULTIFUNC_VALUE_BITS));

					S3_IO_SET_ENHANCED_REGISTER(
						S3_ENHANCED_COMMAND_REGISTER_CMD, command);
				}
				tmplines = stipplelines;

				S3_WAIT_FOR_FIFO(8);
				while ( tmplines-- )
				{
					(*screen_state_p->screen_write_bits_p)
						(screen_state_p->pixtrans_register,
						 endwords, 
						 (void *) src_line_ptr);
					src_line_ptr += stipple_state_p->source_step;
				}

				src_line_ptr = (char *) source_stipple_p->Bptr;
				cursy = 0;
				curdy += stipplelines;
			}
		}
	}

#if (defined(__DEBUG__))
	if (s3_fill_debug || s3_fill_stipple_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	/*
	 * Synchronize clip registers.
	 */

	S3_WAIT_FOR_FIFO(2);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_L_INDEX |
		(s3_enhanced_registers_p->scissor_l & S3_MULTIFUNC_VALUE_BITS)));
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		(S3_ENHANCED_COMMAND_REGISTER_SCISSORS_R_INDEX |
		(s3_enhanced_registers_p->scissor_r & S3_MULTIFUNC_VALUE_BITS)));

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return (SI_SUCCEED);
}


STATIC SIBool
s3_fill_rectangle_stipple_ge_patblt(
	SIint32 x_origin, 
	SIint32 y_origin,
	SIint32	count, 
	SIRectOutlineP rect_p)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURRENT_GRAPHICS_STATE_DECLARE();

	struct s3_stipple_state *stipple_state_p;
	int	offscreen_stipple_x;
	int	offscreen_stipple_y;
	int command;

    const int screen_clip_left = 
		screen_state_p->generic_state.screen_clip_left;
    const int screen_clip_right = 
		screen_state_p->generic_state.screen_clip_right;
    const int screen_clip_top = 
		screen_state_p->generic_state.screen_clip_top;
    const int screen_clip_bottom = 
		screen_state_p->generic_state.screen_clip_bottom;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE, graphics_state_p));

#if (defined(__DEBUG__))
	if (s3_fill_debug)
	{
		(void) fprintf(debug_stream_p,
		   "(s3_fill_rectangle_tile_ge_patblt)\n"
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

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	stipple_state_p = &(graphics_state_p->current_stipple_state);

	ASSERT(IS_OBJECT_STAMPED(S3_STIPPLE_STATE, stipple_state_p));
	ASSERT(stipple_state_p->is_small_stipple == TRUE);

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
		if (stipple_state_p->is_reduced_stipple)
		{
			ASSERT((&stipple_state_p->reduced_stipple) ==
					stipple_state_p->si_stipple_p);
			/*
			 * Update origin in reduced stipple
			 */

			stipple_state_p->si_stipple_p->BorgX =
				graphics_state_p->generic_state.
				si_graphics_state.SGstipple->BorgX;

			stipple_state_p->si_stipple_p->BorgY =
				graphics_state_p->generic_state.
				si_graphics_state.SGstipple->BorgY;
		}

		s3_graphics_state_download_stipple(screen_state_p,
			graphics_state_p,
			stipple_state_p->si_stipple_p);
	}	

	/*
	 * Set the clipping rectangle to select the entire drawing area.
	 */
	if (screen_state_p->generic_state.screen_current_clip !=
		GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
		S3_STATE_SET_CLIP_RECTANGLE(screen_state_p, 0, 0,
			screen_state_p->generic_state.screen_virtual_width,
			screen_state_p->generic_state.screen_virtual_height);

		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
		S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);
	}

	/*
	 * Synchronize registers with the graphics state.
	 */
	if (graphics_state_p->generic_state.si_graphics_state.SGstplmode
		== SGStipple)
	{
		/*
		 * Synchronize registers with the graphics state.
		 */
		S3_STATE_SYNCHRONIZE_STATE(screen_state_p,
			S3_FILL_STIPPLE_TRANSPARENT_PATBLT_DEPENDENCIES);
		S3_STATE_SET_BG_ROP(screen_state_p, S3_MIX_FN_LEAVE_C_AS_IS);
	}
	else
	{
		/*
		 * Synchronize registers with the graphics state.
		 */
		S3_STATE_SYNCHRONIZE_STATE(screen_state_p,
			S3_FILL_STIPPLE_OPAQUE_PATBLT_DEPENDENCIES);
	}

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
		/*
		 * We have to try system memory stippling
		 */
#if	defined(__DEBUG__)
		if (s3_fill_debug)
		{
			(void)fprintf(debug_stream_p,"#OMM lock failed!");
		}
#endif
		return (s3_fill_rectangle_stipple_system_memory(x_origin,
													 y_origin,
													 count,
													 rect_p));
	}
	
	/*
	 * Stippling from offscreen memory, therefore decide the mix
	 * register based on video data
	 */
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_VID_MEM);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
	S3_SET_BG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);

	/*
	 * The offscreen stipple occupies only one plane, set the readmask
	 * for reading only that plane
	 */
	S3_STATE_SET_RD_MASK(screen_state_p,
		 stipple_state_p->offscreen_location_p->planemask);

	command = 
		screen_state_p->cmd_flags | 
		S3_CMD_TYPE_PATTERNFILL |
		S3_CMD_WRITE | 
		S3_CMD_DRAW |
		S3_CMD_DIR_TYPE_AXIAL | 
		S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
		S3_CMD_AXIAL_X_MAJOR |
		S3_CMD_PX_MD_ACROSS_PLANE |
		S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;

	S3_WAIT_FOR_FIFO(2);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,
		offscreen_stipple_x);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,
		offscreen_stipple_y);

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

#if (defined(__DEBUG__))
		if (s3_fill_debug)
		{
			(void) fprintf(debug_stream_p,
			   "\t{\n"
			   "\t\tx_left = %d\n"
			   "\t\ty_top = %d\n"
			   "\t\twidth = %d\n"
			   "\t\theight = %d\n"
			   "\t}\n",
			   x_left, y_top, rect_p->width, rect_p->height);
		}
#endif

		/*
		 * Check if the rectangle is outside the current clipping
		 * bounds. Discard the rectangle if so.
		 */
		if((x_left > screen_clip_right) || (x_right < screen_clip_left) ||
		   (y_top > screen_clip_bottom) || (y_bottom < screen_clip_top) ||
		   (y_bottom < y_top) || ( x_right < x_left ))
		{
#if (defined(__DEBUG__))
			if (s3_fill_debug)
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

		if (((width = x_right - x_left) < 0) || 
			((height = y_bottom - y_top) < 0))
		{
			++rect_p;
			continue;
		}
		/*
		 * The clip rectangle has to be set to the inner rectangle
		 */
		if (screen_state_p->use_mmio)
		{
			S3_WAIT_FOR_FIFO(5);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP,
				x_left);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP,
				y_top);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
				width );
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
				(height & S3_MULTIFUNC_VALUE_BITS));
			S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
				command);
		}
		else
		{
			S3_WAIT_FOR_FIFO(5);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP,
				x_left);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP,
				y_top);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,
				width );
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
				S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
				(height & S3_MULTIFUNC_VALUE_BITS));
			S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
				command);
		}
		++rect_p;
	} while (--count);

#if (defined(__DEBUG__))
	if (s3_fill_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	OMM_UNLOCK(stipple_state_p->offscreen_location_p);

	ASSERT(!S3_IS_FIFO_OVERFLOW());
	return (SI_SUCCEED);
}

/*
 * Stippling from offscreen memory without involving the ge's patblt 
 * capability.
 */

STATIC SIBool
s3_fill_rectangle_stipple_offscreen_memory(
	SIint32 x_origin, 
	SIint32 y_origin,
	SIint32	count, 
	SIRectOutlineP rect_p)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURRENT_GRAPHICS_STATE_DECLARE();

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
	
	struct s3_stipple_state *stipple_state_p;	
	SIbitmap *si_stipple_p;
	

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE, graphics_state_p));
	
	stipple_state_p = &(graphics_state_p->current_stipple_state);

	si_stipple_p =
		graphics_state_p->generic_state.si_graphics_state.SGstipple;
	
	ASSERT(IS_OBJECT_STAMPED(S3_STIPPLE_STATE, stipple_state_p));
#if (defined(__DEBUG__))
	if (s3_fill_debug)
	{
		(void) fprintf(debug_stream_p,
		   "(s3_fill_rectangle_stipple_offscreen_memory)\n"
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

	if (!stipple_state_p->stipple_downloaded) 
	{
		s3_graphics_state_download_stipple(screen_state_p,
			graphics_state_p,
			stipple_state_p->si_stipple_p);
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
		if (s3_fill_debug)
		{
			(void)fprintf(debug_stream_p,"#OMM lock failed!");
		}
#endif
		return (s3_fill_rectangle_stipple_system_memory(x_origin,
					y_origin, count, rect_p));
	}

	/*
	 * Synchronize registers with the graphics state.
	 */
	if (graphics_state_p->generic_state.si_graphics_state.SGstplmode
		== SGStipple)
	{
		/*
		 * Synchronize registers with the graphics state.
		 */
		S3_STATE_SYNCHRONIZE_STATE(screen_state_p,
			S3_FILL_STIPPLE_TRANSPARENT_PATBLT_DEPENDENCIES);
		S3_STATE_SET_BG_ROP(screen_state_p, S3_MIX_FN_LEAVE_C_AS_IS);
	}
	else
	{
		/*
		 * Synchronize registers with the graphics state.
		 */
		S3_STATE_SYNCHRONIZE_STATE(screen_state_p,
			S3_FILL_STIPPLE_OPAQUE_PATBLT_DEPENDENCIES);
	}

	/*
	 * Stippling from offscreen memory, therefore decide the mix
	 * register based on video data
	 */
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_VID_MEM);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
	S3_SET_BG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);

	/*
	 * The offscreen stipple occupies only one plane, set the readmask
	 * for reading only that plane
	 */
	S3_STATE_SET_RD_MASK(screen_state_p,
		 stipple_state_p->offscreen_location_p->planemask);
	
	offscreen_stipple_x_location = stipple_state_p->offscreen_location_x;
	offscreen_stipple_y_location = stipple_state_p->offscreen_location_y;
	offscreen_stipple_width = 
		(stipple_state_p->offscreen_width/si_stipple_p->Bwidth) *
		si_stipple_p->Bwidth;
	offscreen_stipple_height = 
		(stipple_state_p->offscreen_height/si_stipple_p->Bheight) *
		si_stipple_p->Bheight;
	
	/*
	 * Set the clipping rectangle to select the entire drawing area.
	 */
	if (screen_state_p->generic_state.screen_current_clip !=
		GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
		S3_STATE_SET_CLIP_RECTANGLE(screen_state_p, 0, 0,
			screen_state_p->generic_state.screen_virtual_width,
			screen_state_p->generic_state.screen_virtual_height);

		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
		S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);
	}

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
		if((x_left > screen_state_p->generic_state.screen_clip_right) ||
		   (x_right < screen_state_p->generic_state.screen_clip_left) ||
		   (y_top > screen_state_p->generic_state.screen_clip_bottom) ||
		   (y_bottom < screen_state_p->generic_state.screen_clip_top) ||
		   (y_bottom < y_top) || ( x_right < x_left ))
		{
#if (defined(__DEBUG__))
			if (s3_fill_debug)
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

		s3_fill_rectangle_offscreen_helper(
			si_stipple_p,
			offscreen_stipple_x_location, offscreen_stipple_y_location,
			offscreen_stipple_width, offscreen_stipple_height,
			x_left, y_top, 
			x_right - x_left + 1, y_bottom - y_top + 1,
			S3_ASM_TRANSFER_ACROSS_PLANE);
		++rect_p;
	}while(--count);	

#if (defined(__DEBUG__))
	if(s3_fill_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	ASSERT(!S3_IS_FIFO_OVERFLOW());
	return (SI_SUCCEED);
}

STATIC SIBool
s3_fill_rectangle_compat(SIint32 count, SIRectP pRect)
{
	int		ret_val;
	boolean is_local_allocation;
	
	SIRectOutlineP	p_new_rect, tmp_new_rect_p;
	SIRectP			tmp_old_rect_p;
	SIint32 		tmpcount;

	SIBool (*called_function_p)(SIint32, SIint32, SIint32,
								SIRectOutlineP);
	SIRectOutline				/* for fast allocations */
		localRectangles[DEFAULT_S3_COMPATIBILITY_LOCAL_RECTANGLE_COUNT];
	
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURRENT_GRAPHICS_STATE_DECLARE();

#if (defined(__DEBUG__))
	if (s3_fill_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_fill_rectangle_compat_other)\n"
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
	if (count > DEFAULT_S3_COMPATIBILITY_LOCAL_RECTANGLE_COUNT)
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
			S3_OPTIONS_RECTFILL_OPTIONS_USE_SOLID_FILL_RECT)
		{
			called_function_p = s3_fill_rectangle_solid; 
		}
		break;

	case SGFillTile :
		if (screen_state_p->options_p->rectfill_options &
			S3_OPTIONS_RECTFILL_OPTIONS_USE_TILE_FILL_RECT)
		{
			if ((screen_state_p->options_p->rectfill_options &
				S3_OPTIONS_RECTFILL_OPTIONS_USE_GE_PATFILL) &&
				(screen_state_p->options_p->rectfill_options &
				S3_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY)&&
				graphics_state_p->current_tile_state.is_small_tile)
			{
				called_function_p = s3_fill_rectangle_tile_ge_patblt;
			}
			else if(screen_state_p->options_p->rectfill_options &
				S3_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY)
			{
				called_function_p = s3_fill_rectangle_tile_offscreen_memory;
			}
			else
			{
				called_function_p = s3_fill_rectangle_tile_system_memory;
			}
		}
		break;

	case SGFillStipple:
		if (screen_state_p->options_p->rectfill_options &
			S3_OPTIONS_RECTFILL_OPTIONS_USE_STIPPLE_FILL_RECT)
		{
			if((screen_state_p->options_p->override_ss_stippling ==
				S3_OPTIONS_OVERRIDE_SS_STIPPLING_YES) &&
				((graphics_state_p->generic_state.si_graphics_state.
					SGmode == GXinvert) || 
				(graphics_state_p->generic_state.si_graphics_state.
					SGmode == GXset) || 
				(graphics_state_p->generic_state.si_graphics_state.
					SGmode == GXclear)))
			{
					called_function_p = 
						s3_fill_rectangle_stipple_system_memory;
			}
			else if ((screen_state_p->options_p->rectfill_options &
				S3_OPTIONS_RECTFILL_OPTIONS_USE_GE_PATFILL) &&
				(screen_state_p->options_p->rectfill_options &
				S3_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY)&&
				graphics_state_p->current_stipple_state.is_small_stipple)
			{
				called_function_p = s3_fill_rectangle_stipple_ge_patblt;
			}
			else if(screen_state_p->options_p->rectfill_options &
				S3_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY)
			{
				called_function_p = s3_fill_rectangle_stipple_offscreen_memory;
			}
			else
			{
				called_function_p = s3_fill_rectangle_stipple_system_memory;
			}
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
s3_fill__gs_change__(void)
{
	S3_CURRENT_GRAPHICS_STATE_DECLARE();

	S3_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE,
							 screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE,
							 graphics_state_p));

#if (defined(__DEBUG__))
	if (s3_fill_debug)
	{
		(void) fprintf(debug_stream_p,
					   "(s3_fill__gs_change__) {}\n");
	}
#endif

	/*
	 * Set the appropriate pointer to the fill function.
	 */
	screen_state_p->generic_state.screen_functions_p->
		si_poly_fillrect =
			graphics_state_p->generic_si_functions.si_poly_fillrect;

	if (screen_state_p->generic_state.screen_sdd_version_number >=
		DM_SI_VERSION_1_1)
	{
		switch (graphics_state_p->generic_state.si_graphics_state.SGfillmode)
		{
		case SGFillSolidFG:
		case SGFillSolidBG :
			if (screen_state_p->options_p->rectfill_options &
				S3_OPTIONS_RECTFILL_OPTIONS_USE_SOLID_FILL_RECT)
			{
				screen_state_p->generic_state.screen_functions_p->
					si_poly_fillrect = s3_fill_rectangle_solid;
			}
			break;

		case SGFillTile:
			if (screen_state_p->options_p->rectfill_options &
				S3_OPTIONS_RECTFILL_OPTIONS_USE_TILE_FILL_RECT)
			{
				if((screen_state_p->options_p->rectfill_options &
					S3_OPTIONS_RECTFILL_OPTIONS_USE_GE_PATFILL) &&
				   (screen_state_p->options_p->rectfill_options &
					S3_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY)&&
					graphics_state_p->current_tile_state.is_small_tile)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_poly_fillrect = s3_fill_rectangle_tile_ge_patblt;
				}
				else if (screen_state_p->options_p->rectfill_options &
					S3_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_poly_fillrect =
						s3_fill_rectangle_tile_offscreen_memory;
				}
				else
				{
					screen_state_p->generic_state.screen_functions_p->
						si_poly_fillrect =
						s3_fill_rectangle_tile_system_memory;
				}
			}
			break;
		/*
		 * Some S3 chips seem to have problems with screen to screen 
		 * stippling with GXinvert rop. If the user prefers correctness 
		 * to speed allow it.
		 */
		case SGFillStipple:
			if (screen_state_p->options_p->rectfill_options &
				S3_OPTIONS_RECTFILL_OPTIONS_USE_STIPPLE_FILL_RECT)
			{
				if((screen_state_p->options_p->override_ss_stippling ==
					S3_OPTIONS_OVERRIDE_SS_STIPPLING_YES) &&
					((graphics_state_p->generic_state.si_graphics_state.
						SGmode == GXinvert) || 
					(graphics_state_p->generic_state.si_graphics_state.
						SGmode == GXset) || 
					(graphics_state_p->generic_state.si_graphics_state.
						SGmode == GXclear)))
				{
					screen_state_p->generic_state.screen_functions_p->
						si_poly_fillrect =
						s3_fill_rectangle_stipple_system_memory;
				}
				else if((screen_state_p->options_p->rectfill_options &
					S3_OPTIONS_RECTFILL_OPTIONS_USE_GE_PATFILL) &&
				   	(screen_state_p->options_p->rectfill_options &
					S3_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY) &&
					graphics_state_p->current_stipple_state.is_small_stipple)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_poly_fillrect = s3_fill_rectangle_stipple_ge_patblt;
				}
				else if (screen_state_p->options_p->rectfill_options &
					S3_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_poly_fillrect = 
						s3_fill_rectangle_stipple_offscreen_memory;
				}
				else
				{
					screen_state_p->generic_state.screen_functions_p->
						si_poly_fillrect =
						s3_fill_rectangle_stipple_system_memory;
				}
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
			s3_fill_rectangle_compat;
	}
}

/*
 * Initialization.
 */
function void
s3_fill__initialize__(SIScreenRec *si_screen_p,
						struct s3_options_structure *options_p)
{
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	int tile_best_width = 0, tile_best_height = 0;
	int stipple_best_width = 0, stipple_best_height = 0;

	flags_p->SIavail_fpoly = RECTANGLE_AVAIL;

	if (options_p->tile_best_size)
	{
		if (sscanf(options_p->tile_best_size, "%ix%i",
				   &tile_best_width, &tile_best_height) != 2)
		{
			(void) fprintf(stderr,
						   S3_BAD_BEST_TILE_SIZE_SPECIFICATION_OPTION,
						   options_p->tile_best_size);
			tile_best_width = tile_best_height = 0;
		}
	}

	if (options_p->stipple_best_size)
	{
		if (sscanf(options_p->stipple_best_size, "%ix%i",
				   &stipple_best_width, &stipple_best_height) != 2)
		{
			(void) fprintf(stderr,
						   S3_BAD_BEST_STIPPLE_SIZE_SPECIFICATION_OPTION,
						   options_p->stipple_best_size);
			stipple_best_width = stipple_best_height = 0;
		}
	}

	flags_p->SItilewidth = tile_best_width ? tile_best_width :
		DEFAULT_S3_BEST_TILE_WIDTH;
	flags_p->SItileheight = tile_best_height ? tile_best_height :
		DEFAULT_S3_BEST_TILE_HEIGHT;
	flags_p->SIstipplewidth = stipple_best_width ? stipple_best_width
		: DEFAULT_S3_BEST_STIPPLE_WIDTH;
	flags_p->SIstippleheight = stipple_best_height ?
		stipple_best_height : DEFAULT_S3_BEST_STIPPLE_HEIGHT;

	functions_p->si_poly_fconvex = s3_no_operation_fail;
	functions_p->si_poly_fgeneral = s3_no_operation_fail;
	functions_p->si_poly_fillrect = s3_no_operation_fail;

}
