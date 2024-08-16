/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m_fill.c	1.6"

/***
 ***	NAME
 ***		
 ***			m_fill.c : filled rectangle code for the MACH display
 ***	library.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "m_fill.h"
 ***	
 ***	DESCRIPTION
 ***
 ***	This module implements rectangle fill functionality for the
 ***	MACH display library.  It handles solid fills, tiled fills and
 ***	stippled fills.  Small tiles are stipples are handled
 ***	separately as are tiles and stipples which are stored in
 ***	offscreen memory.
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
export boolean mach_fill_debug = FALSE;
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

#include "m_globals.h"
#include "m_opt.h"
#include "m_state.h"
#include "m_gs.h"

/***
 ***	Constants.
 ***/

#define MACH_FILL_SOLID_DEPENDENCIES\
	(MACH_INVALID_FG_ROP |\
	 MACH_INVALID_FOREGROUND_COLOR |\
	 MACH_INVALID_BACKGROUND_COLOR |\
	 MACH_INVALID_WRT_MASK |\
	 MACH_INVALID_CLIP)

#define MACH_FILL_TILE_DEPENDENCIES\
	(MACH_INVALID_FG_ROP |\
	 MACH_INVALID_WRT_MASK |\
	 MACH_INVALID_CLIP)

#define MACH_FILL_STIPPLE_TRANSPARENT_DEPENDENCIES\
	(MACH_INVALID_FG_ROP |\
	 MACH_INVALID_FOREGROUND_COLOR |\
	 MACH_INVALID_WRT_MASK |\
	 MACH_INVALID_CLIP)

#define MACH_FILL_STIPPLE_OPAQUE_DEPENDENCIES\
	(MACH_INVALID_FG_ROP |\
	 MACH_INVALID_BG_ROP |\
	 MACH_INVALID_FOREGROUND_COLOR |\
	 MACH_INVALID_BACKGROUND_COLOR |\
	 MACH_INVALID_WRT_MASK |\
	 MACH_INVALID_CLIP)

#define MACH_FILL_TILE_OFFSCREEN_DEPENDENCIES\
	 (MACH_INVALID_CLIP |\
	  MACH_INVALID_WRT_MASK |\
	  MACH_INVALID_FG_ROP)

#define MACH_FILL_STIPPLE_TRANSPARENT_OFFSCREEN_DEPENDENCIES\
	 (MACH_INVALID_CLIP |\
	  MACH_INVALID_WRT_MASK |\
	  MACH_INVALID_FG_ROP |\
	  MACH_INVALID_FOREGROUND_COLOR)


#define MACH_FILL_STIPPLE_OPAQUE_OFFSCREEN_DEPENDENCIES\
	 (MACH_INVALID_CLIP |\
	  MACH_INVALID_WRT_MASK |\
	  MACH_INVALID_FG_ROP |\
	  MACH_INVALID_BG_ROP |\
	  MACH_INVALID_FOREGROUND_COLOR |\
	  MACH_INVALID_BACKGROUND_COLOR)

#define FORCE_COORDINATES_TO_CILP_RECTANGLE(xl, yt, xr, yb,\
	clip_left, clip_top, clip_right, clip_bottom)\
	if ((xl) < (clip_left))\
	{\
		(xl) = (clip_left);\
	}\
	if ((xr) > (clip_right))\
	{\
		(xr) = (clip_right);\
	}\
	if ((yt) < (clip_top))\
	{\
		(yt) = (clip_top);\
	}\
	if ((yb) > (clip_bottom))\
	{\
		(yb) = (clip_bottom);\
	}\

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/



/*
 * Fill rectangle solid.   This code does not use any IBM mode drawing
 * commands.
 *
 * This code has been duplicated in
 * `mach_fill_rectangle_compat_solid'.
 */
STATIC SIBool
mach_fill_rectangle_solid(SIint32 x_origin, SIint32 y_origin,
						  SIint32 count, SIRectOutlineP rect_p)
{
	unsigned short dp_config;

	MACH_CURRENT_SCREEN_STATE_DECLARE();

	MACH_CURRENT_GRAPHICS_STATE_DECLARE();

	const int clip_left =
		screen_state_p->generic_state.screen_clip_left;

	const int clip_right =
		screen_state_p->generic_state.screen_clip_right;

	const int clip_top =
		screen_state_p->generic_state.screen_clip_top;

	const int clip_bottom =
		screen_state_p->generic_state.screen_clip_bottom;

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

	ASSERT(!MACH_IS_IO_ERROR());

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	if (graphics_state_p->generic_state.si_graphics_state.SGfillmode
		!= SGFillSolidFG &&
		graphics_state_p->generic_state.si_graphics_state.SGfillmode
		!= SGFillSolidBG)
	{
		return (SI_FAIL);
	}

	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_MONO_SRC_ONE |
		MACH_DP_CONFIG_WRITE |
		MACH_DP_CONFIG_ENABLE_DRAW |
		(((graphics_state_p->generic_state.si_graphics_state.SGfillmode
		   == SGFillSolidFG) ?
		  MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR :
		  MACH_DP_CONFIG_FG_COLOR_SRC_BKGD_COLOR));

#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_fill_rectangle_solid)\n"
"{\n"
"\tx_origin = %ld\n"
"\ty_origin = %ld\n"
"\tcount = %ld\n"
"\trect_p = %p\n"
"\tdp_config = 0x%x\n",
					   x_origin, y_origin, count, (void *) rect_p,
					   dp_config);
	}
#endif

	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	MACH_STATE_SYNCHRONIZE_STATE(screen_state_p, 
								 MACH_FILL_SOLID_DEPENDENCIES);

	/*
	 * Program the DP_CONFIG value needed.
	 */
	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

	/*
	 * Top to bottom blits.
	 */
	MACH_WAIT_FOR_FIFO(1);
	outw(MACH_REGISTER_SRC_Y_DIR, 1);

	for (;count--;rect_p++)
	{
		register int xl = rect_p->x + x_origin;	/* inclusive */
		register int xr = xl + rect_p->width; /* exclusive */
		register int yt = rect_p->y + y_origin;	/* inclusive */
		register int yb = yt + rect_p->height; /* exclusive */


#if (defined(__DEBUG__))
		if (mach_fill_debug)
		{
			(void) fprintf(debug_stream_p,
"\t{\n"
"\t\txl = %d\n"
"\t\tyt = %d\n"
"\t\txr = %d\n"
"\t\tyb = %d\n"
"\t}\n",
						   xl, yt, xr, yb);
		}
#endif
	   
		/*
		 * Check against the clip rectangle.
		 */
		if ((xl >= xr) || (yt >= yb) || (xl > clip_right) ||
			(xr <= clip_left) || (yt > clip_bottom) || (yb <= clip_top))
		{
#if (defined(__DEBUG__))
			if (mach_fill_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\t(out of clip bounds)\n");
			}
#endif
			continue;
		}


		MACH_WAIT_FOR_FIFO(5);
		outw(MACH_REGISTER_CUR_X, xl);
		outw(MACH_REGISTER_DEST_X_START, xl);
		outw(MACH_REGISTER_DEST_X_END, xr);
		outw(MACH_REGISTER_CUR_Y, yt);
		outw(MACH_REGISTER_DEST_Y_END, yb);	/* Blit starts */

		ASSERT(!MACH_IS_IO_ERROR());
	}

	/*
	 * Using blits destroy the color pattern registers.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);

#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	ASSERT(!MACH_IS_IO_ERROR());

	return (SI_SUCCEED);
}

/*
 * Fill Solid in IBM 8514 mode.  Faster, but does not work in
 * modes with displayed X greater than 1024.
 */
STATIC SIBool
mach_fill_ibm_rectangle_solid(SIint32 x_origin, SIint32 y_origin,
						  SIint32 count, SIRectOutlineP rect_p)
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();

	const int clip_left =
		screen_state_p->generic_state.screen_clip_left;

	const int clip_right =
		screen_state_p->generic_state.screen_clip_right;

	const int clip_top =
		screen_state_p->generic_state.screen_clip_top;

	const int clip_bottom =
		screen_state_p->generic_state.screen_clip_bottom;

	int height = -1;
	int width = -1;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE,
							 graphics_state_p));
	
	ASSERT(screen_state_p->options_p->rectfill_options &
		   MACH_OPTIONS_RECTFILL_OPTIONS_USE_IBM_MODE);

	/*
	 * For now :
	 */
	if (graphics_state_p->generic_state.si_graphics_state.SGfillmode
		!= SGFillSolidFG &&
		graphics_state_p->generic_state.si_graphics_state.SGfillmode
		!= SGFillSolidBG)
	{
		return (SI_FAIL);
	}

	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		   SGFillSolidFG ||
		   graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		   SGFillSolidBG);

	/*
	 * Switch to IBM context.
	 */
	MACH_STATE_SWITCH_TO_IBM_CONTEXT(screen_state_p);
	
	/*
	 * Synchronize registers with the graphics state.
	 */
	MACH_STATE_SYNCHRONIZE_STATE(screen_state_p, 
								 MACH_FILL_SOLID_DEPENDENCIES);

#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_fill_ibm_rectangle_solid)\n"
"{\n"
"\tx_origin = %ld\n"
"\ty_origin = %ld\n"
"\tcount = %ld\n"
"\trect_p = %p\n",
					   x_origin, y_origin, count, (void *) rect_p);
	}
#endif

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	ASSERT(!MACH_IS_IO_ERROR());

	/*
	 * set frgd mix to be use frgd color or bkgd color as requested.
	 */
	MACH_WAIT_FOR_FIFO(1);
	outw(MACH_REGISTER_FRGD_MIX,
		 (screen_state_p->register_state.alu_fg_fn |
		  ((graphics_state_p->generic_state.si_graphics_state.SGfillmode
			== SGFillSolidFG) ?
		   MACH_IBM_SELECT_FRGD_COLOR : 
		   MACH_IBM_SELECT_BKGD_COLOR)));

	for (;count--;rect_p++)
	{
		register int xl = rect_p->x + x_origin;
		register int xr = xl + rect_p->width - 1; /* inclusive */
		register int yt = rect_p->y + y_origin;
		register int yb = yt + rect_p->height - 1; /* inclusive */


#if (defined(__DEBUG__))
		if (mach_fill_debug)
		{
			(void) fprintf(debug_stream_p,
"\t{\n"
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
			if (mach_fill_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\t(out of clip bounds)\n");
			}
#endif

			continue;
		}


		if (height != rect_p->height)
		{
			height = rect_p->height;
			MACH_WAIT_FOR_FIFO(1);
			outw(MACH_REGISTER_MULTI_FN,
				 MACH_MF_RECT_HEIGHT | 
				 ((height - 1) & MACH_MF_VALUE));
		}

		if (width != rect_p->width)
		{
			width = rect_p->width;
			MACH_WAIT_FOR_FIFO(1);
			outw(MACH_REGISTER_MAJ_AXIS_PCNT, width - 1);
		}

		if (width <= 0 || height <= 0)
		{
			continue;
		}
		
		MACH_WAIT_FOR_FIFO(3);
		outw(MACH_REGISTER_CUR_X, xl);
		outw(MACH_REGISTER_CUR_Y, yt);
		outw(MACH_REGISTER_CMD,
			 MACH_CMD_FILL_RECT_HOR_CMD | MACH_CMD_YPOS |
			 MACH_CMD_YMAJOR | MACH_CMD_XPOS | MACH_CMD_DRAW |
			 MACH_CMD_PIXEL_MODE_NIBBLE | MACH_CMD_WRITE);
	}

#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	/*
	 * The blit would have destroyed the pattern registers.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_PATTERN_REGISTERS);
	
	ASSERT(!MACH_IS_IO_ERROR());

	return (SI_SUCCEED);
}

/*
 * mach_fill_rectangle_tile_system_memory
 * 
 * Fillrect routine for SGfillmode = SGfillTile.  This routine uses
 * the PIXTRANS register for its operation and is a general purpose
 * entry point for drawing operations.
 */

STATIC SIBool
mach_fill_rectangle_tile_system_memory(
	SIint32 x_origin,
	SIint32 y_origin,
	SIint32	count,
	SIRectOutlineP rect_p)
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
	unsigned short dp_config;

	/*
	 * Ptr to the (0,y) coords in source tile.
	 * Ptr to the (x,y) coords in source tile.
	 * Ptr to the (x,0) coords in source tile.
	 */
	char        *src_y_ptr;
	char 		*src_xy_ptr;
	char		*src_x_ptr;

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
		return ( SI_SUCCEED );
	}

	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);
	
	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGfillmode
			== SGFillTile);

	tile_state_p = &(graphics_state_p->current_tile_state);

	/*
	 * download and process the tile if necessary.
	 */
	if (tile_state_p->is_downloaded == FALSE)
	{
		mach_graphics_state_download_tile(
			screen_state_p,
			graphics_state_p,
			graphics_state_p->generic_state.si_graphics_state.SGtile);
	}
	
	ASSERT(IS_OBJECT_STAMPED(MACH_TILE_STATE, tile_state_p));

	/*
	 * Synchronize registers with the graphics state.
	 */
	MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
								 MACH_FILL_TILE_DEPENDENCIES);

	si_tile_p =
		graphics_state_p->generic_state.si_graphics_state.SGtile;

	ppw = screen_state_p->pixels_per_pixtrans;

	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_WRITE |
		MACH_DP_CONFIG_ENABLE_DRAW |
		MACH_DP_CONFIG_FG_COLOR_SRC_HOST;

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_fill_rectangle_tile_system_memory)\n"
"{\n"
"\tx_origin = %ld\n"
"\ty_origin = %ld\n"
"\tcount = %ld\n"
"\trect_p = %p\n"
"\tdp_config = 0x%x\n",

					   x_origin, y_origin, count, (void *) rect_p,
					   dp_config);
	}
#endif

	MACH_WAIT_FOR_FIFO(1);
	outw (MACH_REGISTER_SRC_Y_DIR, 1);

	/*
	 * Tile one rectangle at at time.
	 */
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
		if (mach_fill_debug)
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
			if (mach_fill_debug)
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
		 * This assumes software pre-clipping has been done.
		 */
		MACH_WAIT_FOR_FIFO(1);
		outw(MACH_REGISTER_EXT_SCISSOR_L,	destination_x);

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
			/*
			 * More transfers are there for this span
			 */
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

		/*
		 * Tile the destination rectangle.
		 * Finish with the start words first.
		 */
		{
			char	*src_tile_p ;

			curdx = destination_x;
			curdy = destination_y;
			cursy = start_y;

			MACH_WAIT_FOR_FIFO(4);
			outw (MACH_REGISTER_EXT_SCISSOR_R,start_scissor);
			outw (MACH_REGISTER_CUR_X, curdx);
			outw (MACH_REGISTER_DEST_X_START, curdx);
			outw (MACH_REGISTER_DEST_X_END, curdx+start_scan_x_length);

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
				 * Set destination blit rectangles from top to bottom
				 * that would take this portion of the tile.
				 */
				MACH_WAIT_FOR_FIFO(2);
				outw(MACH_REGISTER_CUR_Y, curdy);
				outw(MACH_REGISTER_DEST_Y_END,curdy+tilelines);
								/* start the blit */

				tmplines = tilelines;
				MACH_WAIT_FOR_FIFO(16);
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

		ASSERT(!MACH_IS_IO_ERROR());

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
					MACH_WAIT_FOR_FIFO(6);
					outw(MACH_REGISTER_EXT_SCISSOR_R,
						  (tmpdx + si_tile_p->Bwidth - 1));
					outw(MACH_REGISTER_CUR_X, tmpdx);
					outw(MACH_REGISTER_DEST_X_START, tmpdx);
					outw(MACH_REGISTER_DEST_X_END,
						  (tmpdx + mid_scan_x_length));
					outw(MACH_REGISTER_CUR_Y, curdy);
					outw(MACH_REGISTER_DEST_Y_END,curdy+tilelines);
								/* start the blit */

					MACH_WAIT_FOR_FIFO(16);
					while (tmplines--)
					{
						(*screen_state_p->screen_write_pixels_p)
							(screen_state_p->pixtrans_register,
							 tile_state_p->number_of_pixtrans_words_per_tile_width,
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

		ASSERT(!MACH_IS_IO_ERROR());

		if (endwords)
		{
			char	*src_line_ptr;

			curdy = destination_y;
			cursy = start_y;

			/*
			 * Setup the Constant portion of the Destination Blit rectangle.
			 */
			MACH_WAIT_FOR_FIFO(4);
			outw (MACH_REGISTER_EXT_SCISSOR_R,rect_x_right);
			outw (MACH_REGISTER_CUR_X, curdx);
			outw (MACH_REGISTER_DEST_X_START, curdx);
			outw (MACH_REGISTER_DEST_X_END, curdx+end_scan_x_length);

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
				 * setup the Remaining part of the Destination Blit rectangle.
				 */
				MACH_WAIT_FOR_FIFO(2);
				outw (MACH_REGISTER_CUR_Y, curdy);
				outw (MACH_REGISTER_DEST_Y_END,curdy+tilelines);
								/* start the blit */
				tmplines = tilelines;
				MACH_WAIT_FOR_FIFO(16);

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

		ASSERT(!MACH_IS_IO_ERROR());

	}

#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	/*
	 * Synchronize clip registers.
	 */

	MACH_WAIT_FOR_FIFO(2);
	outw(MACH_REGISTER_EXT_SCISSOR_L,
		 screen_state_p->register_state.ext_scissor_l);
	outw(MACH_REGISTER_EXT_SCISSOR_R,
		 screen_state_p->register_state.ext_scissor_r);

	/*
	 * Blit invalidate pattern registers.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);
	
	ASSERT(!MACH_IS_IO_ERROR());

	return (SI_SUCCEED);
}

#define BLIT_ONCE(XL, X_OFFSET, Y, Y_OFFSET, LINES, ALLOCATION)\
{\
	MACH_WAIT_FOR_FIFO(5);\
	outw(MACH_REGISTER_CUR_X, (XL));\
	outw(MACH_REGISTER_CUR_Y, (Y));\
	outw(MACH_REGISTER_SRC_X,\
		 (ALLOCATION)->x + (X_OFFSET));\
	outw(MACH_REGISTER_SRC_Y,\
		 (ALLOCATION)->y + (Y_OFFSET));\
	outw(MACH_REGISTER_DEST_Y_END,\
		 (Y) + (LINES));\
}

/*
 * Helper function to blit tiles/stipples from offscreen memory
 */
STATIC void
mach_fill_rectangle_offscreen_helper(
	struct omm_allocation *allocation_p, /* the allocation */
	int offscreen_width,		/* offscreen dimensions */
	int offscreen_height,
	SIbitmapP si_bitmap_p,		/* tile/stipple being used */
	int x_left,					/* inclusive destination rectangle */
	int y_top,					/* coordinates */
	int x_right,				/* exclusive L-R rectangle coordinates */
	int y_bottom)				/* -do- */
{

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
	ASSERT(!MACH_IS_IO_ERROR());

#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_fill_rectangle_offscreen_helper)\n"
"{\n"
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
"\ty_bottom = %d\n",
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
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p,
"\t# Intermediate parameters\n"
"\ttile_start_x_offset = %d\n"
"\ttile_start_y_offset = %d\n"
"\trect_start_x_pixels = %d\n"
"\trect_end_x_pixels = %d\n"
"\trect_offscreen_widths = %d\n"
"\trect_start_y_lines = %d\n"
"\trect_end_y_lines = %d\n"
"\trect_offscreen_heights = %d\n",
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
		
		MACH_WAIT_FOR_FIFO(4);
		outw(MACH_REGISTER_SRC_X_START, allocation_p->x +
			 tile_start_x_offset);
		outw(MACH_REGISTER_SRC_X_END, allocation_p->x +
			 tile_start_x_offset + rect_start_x_pixels);
		outw(MACH_REGISTER_DEST_X_START, x_left);
		outw(MACH_REGISTER_DEST_X_END, x_left + rect_start_x_pixels);
		
		if (rect_start_y_lines)
		{
			BLIT_ONCE(x_left, tile_start_x_offset, current_y,
					  tile_start_y_offset, rect_start_y_lines,
					  allocation_p);
			current_y += rect_start_y_lines;
		}
		while (tmp_heights--)
		{
			BLIT_ONCE(x_left, tile_start_x_offset, current_y, 0,
					  offscreen_height, allocation_p);
			current_y += offscreen_height;
		}
		if (rect_end_y_lines)
		{
			BLIT_ONCE(x_left, tile_start_x_offset, current_y, 0,
					  rect_end_y_lines, allocation_p);
		}

		x_left += rect_start_x_pixels;

		ASSERT(current_y + rect_end_y_lines == y_bottom);
	}
	
	while(rect_offscreen_widths --)
	{
		int current_y = y_top;
		int tmp_heights = rect_offscreen_heights;
		
		MACH_WAIT_FOR_FIFO(4);
		outw(MACH_REGISTER_SRC_X_START, allocation_p->x);
		outw(MACH_REGISTER_SRC_X_END, allocation_p->x +
			 offscreen_width);
		outw(MACH_REGISTER_DEST_X_START, x_left);
		outw(MACH_REGISTER_DEST_X_END, x_left + offscreen_width);
		
		if (rect_start_y_lines)
		{
			BLIT_ONCE(x_left, 0, current_y,
					  tile_start_y_offset, rect_start_y_lines,
					  allocation_p);
			current_y += rect_start_y_lines;
		}
		while (tmp_heights--)
		{
			BLIT_ONCE(x_left, 0, current_y, 0,
					  offscreen_height, allocation_p);
			current_y += offscreen_height;
		}
		if (rect_end_y_lines)
		{
			BLIT_ONCE(x_left, 0, current_y, 0,
					  rect_end_y_lines, allocation_p);
		}

		ASSERT(current_y + rect_end_y_lines == y_bottom);

		x_left += offscreen_width;

	}
	
	if (rect_end_x_pixels)
	{
		int current_y = y_top;
		int tmp_heights = rect_offscreen_heights;
		
		MACH_WAIT_FOR_FIFO(4);
		outw(MACH_REGISTER_SRC_X_START, allocation_p->x);
		outw(MACH_REGISTER_SRC_X_END, allocation_p->x +
			 rect_end_x_pixels);
		outw(MACH_REGISTER_DEST_X_START, x_left);
		outw(MACH_REGISTER_DEST_X_END, x_left + rect_end_x_pixels);
		
		if (rect_start_y_lines)
		{
			BLIT_ONCE(x_left, 0, current_y,
					  tile_start_y_offset, rect_start_y_lines,
					  allocation_p);
			current_y += rect_start_y_lines;
		}
		while (tmp_heights--)
		{
			BLIT_ONCE(x_left, 0, current_y, 0,
					  offscreen_height, allocation_p);
			current_y += offscreen_height;
		}
		if (rect_end_y_lines)
		{
			BLIT_ONCE(x_left, 0, current_y, 0,
					  rect_end_y_lines, allocation_p);
		}

		ASSERT(current_y + rect_end_y_lines == y_bottom);
		
	}

	ASSERT(x_left + rect_end_x_pixels == x_right);
	
#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p,"}\n");
	}
#endif

	ASSERT(!MACH_IS_IO_ERROR());
	
}

#undef BLIT_ONCE

/*
 * mach_fill_rectangle_small_tile
 * 
 * Fill a tiled rectangle with a small tile.  Small tiles have row
 * widths which fit into the pattern registers.
 */
STATIC SIBool
mach_fill_rectangle_small_tile(SIint32 x_origin,
	SIint32 y_origin,
	SIint32	count,
	SIRectOutlineP rect_p)
{
	struct mach_tile_state *tile_state_p;
	SIbitmapP expanded_tile_p;

	int tile_width;
	int tile_height;
	int number_of_patt_data_registers_per_tile_width;
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	/*
	 * Clip coordinates
	 */
	const int clip_left =
		screen_state_p->generic_state.screen_clip_left;
	const int clip_right =
		screen_state_p->generic_state.screen_clip_right;
	const int clip_top =
		screen_state_p->generic_state.screen_clip_top;
	const int clip_bottom =
		screen_state_p->generic_state.screen_clip_bottom;


#if (defined(__DEBUG__))
	const boolean are_pattern_registers_useable =
		(screen_state_p->options_p->rectfill_options &
		 MACH_OPTIONS_RECTFILL_OPTIONS_USE_PATTERN_REGISTERS) ? TRUE :
			 FALSE; 
#endif

	boolean is_offscreen_memory_useable =
		(screen_state_p->options_p->rectfill_options &
		 MACH_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY) ? TRUE :
			 FALSE;

	const unsigned short dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_FG_COLOR_SRC_PATT |
		MACH_DP_CONFIG_ENABLE_DRAW |
		MACH_DP_CONFIG_WRITE;

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

	ASSERT(is_offscreen_memory_useable ||
		   are_pattern_registers_useable);

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


	ASSERT(tile_state_p->is_small_tile == TRUE);

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

	/*
	 * Synchronize registers with the graphics state.
	 */

	MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
								 MACH_FILL_TILE_DEPENDENCIES);

	expanded_tile_p = tile_state_p->expanded_tile_p;

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_fill_rectangle_small_tile)\n"
"{\n"
"\tx_origin = %ld\n"
"\ty_origin = %ld\n"
"\tcount = %ld\n"
"\trect_p = %p\n"
"\tdp_config = 0x%x\n",

					   x_origin, y_origin, count, (void *) rect_p,
					   dp_config);
	}
#endif

	/*
	 * Program the tile length into the color pattern register.
	 */

	tile_width = expanded_tile_p->Bwidth;
	tile_height = expanded_tile_p->Bheight;

	MACH_WAIT_FOR_FIFO(2);
	outw(MACH_REGISTER_SRC_Y_DIR, 1); /* increment src y */
	outw(MACH_REGISTER_PATT_LENGTH, tile_width - 1);

	number_of_patt_data_registers_per_tile_width =
		tile_state_p->number_of_patt_data_registers_per_tile_width;

	ASSERT(number_of_patt_data_registers_per_tile_width <= 16 &&
		   number_of_patt_data_registers_per_tile_width > 0);

	if ((is_offscreen_memory_useable == TRUE) &&
		!OMM_LOCK(tile_state_p->offscreen_location_p))
	{
		is_offscreen_memory_useable = FALSE;
	}
		
	for(;count--;rect_p++)
	{
		/*
		 * rectangle coordinates.
		 */
		int x_left = rect_p->x + x_origin;
		int x_right = rect_p->x + rect_p->width - 1;
		int y_top = rect_p->y + y_origin;
		int y_bottom = rect_p->y + rect_p->height - 1;
		int current_y;			/* y coord on screen of line being tiled */
		int source_x;			/* starting point of tile */
		int source_y;			/* tile line to draw */
		int tmp_count, i;		/* temporaries */
		int number_of_tile_lines; /* number of tile lines to draw */
		unsigned char *tile_words_p; /* pointer to tile words */

#if (defined(__DEBUG__))
		if (mach_fill_debug)
		{
			(void) fprintf(debug_stream_p,
"\t{\n"
"\t\tx_left = %d\n"
"\t\ty_top = %d\n"
"\t\tx_right = %d\n"
"\t\ty_bottom = %d\n"
"\t}\n",
		   	x_left, y_top, x_right, y_bottom);
		}
#endif

		/*
		 * Check if the rectangle is outside the current clipping
		 * bounds.
		 */

		if((x_left > screen_state_p->generic_state.screen_clip_right) ||
		   (x_right < screen_state_p->generic_state.screen_clip_left) ||
		   (y_top > screen_state_p->generic_state.screen_clip_bottom) ||
		   (y_bottom < screen_state_p->generic_state.screen_clip_top) ||
		   (y_bottom < y_top) || ( x_right < x_left ))
		{
#if (defined(__DEBUG__))
			if (mach_fill_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\tRectangle out of bounds\n");
			}
#endif
			continue;
		}

		/*
		 * Clip and check for bounds.
		 */

		FORCE_COORDINATES_TO_CILP_RECTANGLE(x_left,y_top,x_right,y_bottom,
			clip_left,clip_top,clip_right,clip_bottom);

		/*
		 * Check if the tile is present in offscreen memory and if the
		 * tiled destination area would fit into the offscreen
		 * dimensions.  If so attempt to tile the region using a
		 * screen to screen blit.
		 * 
		 * Doing this for large areas would be slower than using the
		 * pattern registers if the blit has to be repeated more than
		 * 2-3 times. 
		 */

		if ((is_offscreen_memory_useable == TRUE) &&
			tile_state_p->offscreen_width >= rect_p->width)
		{
			const unsigned short dp_config_screen_to_screen =
				screen_state_p->dp_config_flags |
				(MACH_DP_CONFIG_FG_COLOR_SRC_BLIT |
				 MACH_DP_CONFIG_ENABLE_DRAW |
				 MACH_DP_CONFIG_WRITE |
				 MACH_DP_CONFIG_MONO_SRC_ONE);

			MACH_STATE_SET_DP_CONFIG(screen_state_p,
									 dp_config_screen_to_screen);
			
			mach_fill_rectangle_offscreen_helper(
				tile_state_p->offscreen_location_p,
				tile_state_p->offscreen_width,
                tile_state_p->offscreen_height,
				graphics_state_p->generic_state.si_graphics_state.SGtile,
				x_left, 
				y_top, 
				x_right + 1,
				y_bottom + 1);

			continue;			/* next rectangle */
			
		}
		else
		{
			MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);
		}
		
			
		/*
		 * No offscreen location to tile from or the area was too
		 * large.
		 *
		 * Tile the rectangle one tile line at a time. This implies
		 * that all the lines of the destination rectangle that would take
		 * this line of tile would be tiled before moving on to the
		 * next line of the tile.
		 */

		/*
		 * Get the source coordinates corresponding to (xl, yt)
		 */
		if ((tile_width & (tile_width - 1)))
		{
			source_x = (x_left - tile_x_origin) % tile_width;
		}
		else
		{
			/*
			 * power of two
			 */
			source_x =  (x_left - tile_x_origin) & (tile_width - 1);
		}

		if (source_x < 0)
		{
			source_x += expanded_tile_p->Bwidth;
		}

		if ((tile_height & (tile_height - 1)))
		{
			source_y = (y_top - tile_y_origin) % tile_height;
		}
		else
		{
			/*
			 * height is a power of two.
			 */
			source_y = (y_top - tile_y_origin) & (tile_height - 1);
		}

		if (source_y < 0)
		{
			source_y += expanded_tile_p->Bheight;
		}

		number_of_tile_lines =
			((y_bottom - y_top + 1) > tile_height) ? tile_height :
			(y_bottom - y_top + 1);

		tile_words_p = ((unsigned char *) expanded_tile_p->Bptr +
			 (tile_state_p->expanded_source_step * source_y));

		for (tmp_count = 0;
			 tmp_count < number_of_tile_lines;
			 tmp_count ++)
		{
			unsigned short *tmp_tile_words_p =
				(void *) tile_words_p;

			/*
			 * Download this line of tile into the color pattern
			 * registers.
			 */
			MACH_WAIT_FOR_FIFO(1);
			outw(MACH_REGISTER_PATT_DATA_INDEX, 0);

			for (i = 0;
				 i < number_of_patt_data_registers_per_tile_width;
				 i++, tmp_tile_words_p++)
			{
				MACH_WAIT_FOR_FIFO(1);
				outw(MACH_REGISTER_PATT_DATA,
					 *tmp_tile_words_p);
			}

			/*
			 * Fill in all lines which correspond to this line of tile.
			 */

			for(current_y = y_top + tmp_count;
				current_y <= y_bottom;
				current_y += tile_height)
			{
				MACH_WAIT_FOR_FIFO(4);
				outw(MACH_REGISTER_PATT_INDEX, source_x);
				outw(MACH_REGISTER_CUR_X, x_left);
				outw(MACH_REGISTER_CUR_Y, current_y);
				outw(MACH_REGISTER_SCAN_X, x_right + 1);
			}

			/*
			 * update source y and the tile words pointer.
			 */
			source_y ++;

			if (source_y == tile_height)
			{
				source_y = 0;
				tile_words_p = (unsigned char *) expanded_tile_p->Bptr;
			}
			else
			{
				tile_words_p += tile_state_p->expanded_source_step;
			}
		}
	}

#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	/*
	 * Pattern register contents are now out of sync with the graphics
	 * state.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);
	
	if (is_offscreen_memory_useable == TRUE)
	{
		/*EMPTY*/
		OMM_UNLOCK(tile_state_p->offscreen_location_p);
	}
	
	ASSERT(!MACH_IS_IO_ERROR());

	return (SI_SUCCEED);
}

STATIC SIBool
mach_fill_rectangle_tile_offscreen_memory(SIint32 x_origin,
	SIint32 y_origin,
	SIint32 count,
	SIRectOutlineP rect_p)
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();

	const struct mach_tile_state *tile_state_p = 
		&(graphics_state_p->current_tile_state);

	struct omm_allocation *allocation_p;

	/*
	 * Clip coordinates
	 */

	const int clip_left =
		screen_state_p->generic_state.screen_clip_left;
	const int clip_right =
		screen_state_p->generic_state.screen_clip_right;
	const int clip_top =
		screen_state_p->generic_state.screen_clip_top;
	const int clip_bottom =
		screen_state_p->generic_state.screen_clip_bottom;

	const int tile_width =		/* actual width of the tile */
		graphics_state_p->generic_state.si_graphics_state.SGtile->Bwidth;

	const int tile_height =		/* actual height of the tile */
		graphics_state_p->generic_state.si_graphics_state.SGtile->Bheight;
	
	int allocation_width;
	
	int allocation_height;

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
	 * Switch to ATI mode.
	 */

	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);
	
	/*
	 * download and process the tile if necessary.
	 */

	if (tile_state_p->is_downloaded == FALSE)
	{
		mach_graphics_state_download_tile(
			screen_state_p,
			graphics_state_p,
			graphics_state_p->generic_state.si_graphics_state.SGtile);
	}
	
	ASSERT(IS_OBJECT_STAMPED(MACH_TILE_STATE,
							 tile_state_p));
	
	ASSERT(tile_width > 0);
	ASSERT(tile_height > 0);
	
	/*
	 * Examine the current offscreen allocation.
	 */

	allocation_p =
		graphics_state_p->current_tile_state.offscreen_location_p;
	
	ASSERT(!allocation_p ||
		   IS_OBJECT_STAMPED(OMM_ALLOCATION, allocation_p));

	if (!OMM_LOCK(allocation_p))
	{
		/*
		 * OMM locking failed : revert to the standard
		 * method of drawing.
		 */

		return mach_fill_rectangle_tile_system_memory(
				x_origin, y_origin,
				count, rect_p);
	}
	
	allocation_width =
		graphics_state_p->current_tile_state.offscreen_width;
	
	allocation_height =
		graphics_state_p->current_tile_state.offscreen_height;
	
	ASSERT(allocation_width > 0);
	ASSERT(allocation_height > 0);

	MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
								 MACH_FILL_TILE_OFFSCREEN_DEPENDENCIES);

	MACH_WAIT_FOR_FIFO(1);
	outw(MACH_REGISTER_SRC_Y_DIR, 1); /* increment src y */

	MACH_STATE_SET_DP_CONFIG(screen_state_p,
		(screen_state_p->dp_config_flags |
		 MACH_DP_CONFIG_WRITE |
		 MACH_DP_CONFIG_FG_COLOR_SRC_BLIT |
		 MACH_DP_CONFIG_ENABLE_DRAW));
	
    for(; count--; rect_p++)
	{
		int xl = rect_p->x + x_origin; /* inclusive */
		int xr = xl + rect_p->width - 1; /* inclusive */
		int yt = rect_p->y + y_origin; /* inclusive */
		int yb = yt + rect_p->height - 1; /* inclusive */
		
#if (defined(__DEBUG__))
		if (mach_fill_debug)
		{
			(void) fprintf(debug_stream_p,
"\t{\n"
"\t\txl = %d\n"
"\t\tyt = %d\n"
"\t\txr = %d\n"
"\t\tyb = %d\n"
"\t}\n",
						   xl, yt, xr, yb);
		}
#endif
		
		if ((xl > xr) || (yt > yb) || (xl > clip_right) ||
			(xr <= clip_left) || (yt > clip_bottom) || (yb <= clip_top))
		{
#if (defined(__DEBUG__))
			if (mach_fill_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\t(out of clip bounds)\n");
			}
#endif
			continue;
		}

		/*
		 * Clip and check for bounds.
		 */

		FORCE_COORDINATES_TO_CILP_RECTANGLE(xl,yt,xr,yb,
			clip_left,clip_top,clip_right,clip_bottom);

		/*
		 * Call the offscreen helper to tile the rectangle.
		 */

		mach_fill_rectangle_offscreen_helper(
		    tile_state_p->offscreen_location_p,
			tile_state_p->offscreen_width,
			tile_state_p->offscreen_height,
			graphics_state_p->generic_state.si_graphics_state.SGtile,
			xl,
			yt,
			xr + 1,
			yb + 1);											 

	}

	/*
	 * Pattern register contents are now out of sync with the graphics
	 * state.
	 */

	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);
	

	ASSERT(!MACH_IS_IO_ERROR());
	
	return (SI_SUCCEED);
}



STATIC SIBool
mach_fill_rectangle_small_stipple(SIint32 x_origin,
	SIint32 y_origin,
	SIint32	count,
	SIRectOutlineP rect_p)
{
	struct mach_stipple_state *stipple_state_p;
	SIbitmapP inverted_stipple_p;

	int stipple_width;
	int stipple_height;
	int number_of_patt_data_registers_per_stipple_width;

	MACH_CURRENT_SCREEN_STATE_DECLARE();

	/*
	 * Clip coordinates
	 */
	const int clip_left =
		screen_state_p->generic_state.screen_clip_left;
	const int clip_right =
		screen_state_p->generic_state.screen_clip_right;
	const int clip_top =
		screen_state_p->generic_state.screen_clip_top;
	const int clip_bottom =
		screen_state_p->generic_state.screen_clip_bottom;

#if (defined(__DEBUG__))
	const boolean are_pattern_registers_useable =
		(screen_state_p->options_p->rectfill_options &
		 MACH_OPTIONS_RECTFILL_OPTIONS_USE_PATTERN_REGISTERS) ? TRUE :
			 FALSE; 
#endif

	boolean is_offscreen_memory_useable =
		(screen_state_p->options_p->rectfill_options &
		 MACH_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY) ? TRUE :
			 FALSE; 

	const unsigned short dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR |
		MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		MACH_DP_CONFIG_ENABLE_DRAW |
		MACH_DP_CONFIG_MONO_SRC_PATT |
		MACH_DP_CONFIG_READ_MODE_MONO_DATA |
		MACH_DP_CONFIG_WRITE;

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

	ASSERT(is_offscreen_memory_useable ||
		   are_pattern_registers_useable);

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
	 * download the stipple if necessary.
	 */
	if (stipple_state_p->is_downloaded == FALSE)
	{
		mach_graphics_state_download_stipple(screen_state_p,
			graphics_state_p, 
			graphics_state_p->generic_state.si_graphics_state.SGstipple);
	}
		
	ASSERT(IS_OBJECT_STAMPED(MACH_STIPPLE_STATE, stipple_state_p));
	ASSERT(stipple_state_p->is_small_stipple == TRUE);

	inverted_stipple_p = &(stipple_state_p->inverted_stipple);

	if (graphics_state_p->generic_state.si_graphics_state.SGstplmode
		== SGStipple)
	{
		/*
		 * Synchronize registers with the graphics state.
		 */
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
							 MACH_FILL_STIPPLE_TRANSPARENT_DEPENDENCIES);
		MACH_STATE_SET_BG_ROP(screen_state_p,
							  MACH_MIX_FN_LEAVE_ALONE);
	}
	else
	{
		/*
		 * Synchronize registers with the graphics state.
		 */
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
							 MACH_FILL_STIPPLE_OPAQUE_DEPENDENCIES);
	}

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_fill_rectangle_small_stipple)\n"
"{\n"
"\tx_origin = %ld\n"
"\ty_origin = %ld\n"
"\tcount = %ld\n"
"\trect_p = %p\n"
"\tdp_config = 0x%x\n",

					   x_origin, y_origin, count, (void *) rect_p,
					   dp_config);
	}
#endif

	/*
	 * Program the stipple length into the color pattern register.
	 */
	stipple_width = inverted_stipple_p->Bwidth;
	stipple_height = inverted_stipple_p->Bheight;

	MACH_WAIT_FOR_FIFO(2);
	outw(MACH_REGISTER_PATT_LENGTH, stipple_width - 1);
	outw(MACH_REGISTER_SRC_Y_DIR, 1); /* increment src y */

	/*
	 * The pattern data registers are 16 bits wide.
	 */
	number_of_patt_data_registers_per_stipple_width =
		(stipple_state_p->number_of_pixtrans_words_per_stipple_width <<
		 screen_state_p->pixtrans_width_shift) >>
			 DEFAULT_MACH_PATTERN_REGISTER_WIDTH_SHIFT;

	ASSERT(number_of_patt_data_registers_per_stipple_width <= 2 &&
		   number_of_patt_data_registers_per_stipple_width > 0);


	if ((is_offscreen_memory_useable == TRUE) &&
		!OMM_LOCK(stipple_state_p->offscreen_location_p))
	{
		is_offscreen_memory_useable = FALSE;
	}

#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		struct omm_allocation *allocation_p =
			stipple_state_p->offscreen_location_p;
		
		(void) fprintf(debug_stream_p,
"(mach_fill_rectangle_small_stipple)\n"
"{\n"
"\t# Offscreen location dimensions\n"
"\tx = %d\n"
"\ty = %d\n"
"\twidth = %d\n"
"\theight = %d\n"
"\tplanemask = 0x%x\n"
"}\n",
					   allocation_p->x, allocation_p->y, allocation_p->width,
					   allocation_p->height, allocation_p->planemask);
		
	}
#endif
	
	for(;count--;rect_p++)
	{
		/*
		 * rectangle coordinates.
		 */
		int x_left = rect_p->x + x_origin;
		int x_right = rect_p->x + rect_p->width - 1;
		int y_top = rect_p->y + y_origin;
		int y_bottom = rect_p->y + rect_p->height - 1;
		int current_y;	/* y coord on screen of line being stippled */
		int source_x;	/* starting point of stipple */
		int source_y;	/* stipple line to draw */
		int tmp_count, i;	/* temporaries */
		int number_of_stipple_lines; /* number of stipple lines to draw */
		unsigned char *stipple_words_p; /* pointer to stipple words */

#if (defined(__DEBUG__))
		if (mach_fill_debug)
		{
			(void) fprintf(debug_stream_p,
"\t{\n"
"\t\tx_left = %d\n"
"\t\ty_top = %d\n"
"\t\tx_right = %d\n"
"\t\ty_bottom = %d\n"
"\t}\n",
		   	x_left, y_top, x_right, y_bottom);
		}
#endif

		/*
		 * Check if the rectangle is outside the current clipping
		 * bounds.
		 */
		if((x_left > screen_state_p->generic_state.screen_clip_right) ||
		   (x_right < screen_state_p->generic_state.screen_clip_left) ||
		   (y_top > screen_state_p->generic_state.screen_clip_bottom) ||
		   (y_bottom < screen_state_p->generic_state.screen_clip_top) ||
		   (y_bottom < y_top) || ( x_right < x_left ))
		{
#if (defined(__DEBUG__))
			if (mach_fill_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\tRectangle out of bounds\n");
			}
#endif
			continue;
		}

		/*
		 * Clip and check for bounds.
		 */

		FORCE_COORDINATES_TO_CILP_RECTANGLE(x_left,y_top,x_right,y_bottom,
			clip_left,clip_top,clip_right,clip_bottom);

		/*
		 * Check if the stipple is present in offscreen memory and if the
		 * stippled destination area would fit into the offscreen
		 * dimensions.  If so attempt to stipple the region using a
		 * screen to screen blit.
		 * 
		 * Doing this for large areas would be slower than using the
		 * pattern registers if the blit has to be repeated more than
		 * 2-3 times. 
		 */
		if (is_offscreen_memory_useable &&
			stipple_state_p->offscreen_width >= rect_p->width)
		{
			const unsigned short dp_config_screen_to_screen =
				screen_state_p->dp_config_flags |
				(MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR |
				 MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
				 MACH_DP_CONFIG_ENABLE_DRAW |
				 MACH_DP_CONFIG_WRITE |
				 MACH_DP_CONFIG_READ_MODE_MONO_DATA |
				 MACH_DP_CONFIG_MONO_SRC_BLIT);

			MACH_STATE_SET_DP_CONFIG(screen_state_p,
									 dp_config_screen_to_screen);
			
			/*
			 * FRGD, BKGD colors, the WRT_MASK and the relevant ROPS
			 * have already been set by the synchronize state calls.
			 * Set the RD mask to the plane of the stipple.
			 */

			MACH_STATE_SET_RD_MASK(screen_state_p,
						   stipple_state_p->offscreen_location_p->planemask);
			
			mach_fill_rectangle_offscreen_helper(
				stipple_state_p->offscreen_location_p,
				stipple_state_p->offscreen_width,
				stipple_state_p->offscreen_height,
				graphics_state_p->generic_state.si_graphics_state.
					SGstipple,
				x_left, 
				y_top, 
				x_right + 1,
				y_bottom + 1);

			continue;			/* next rectangle */
			
		}
		else
		{
			MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);
		}
		
			
		/*
		 * No offscreen location to stipple from, or the area was too
		 * large to effectively stipple from offscreen memory.
		 *
		 * Stipple the rectangle one stipple line at a time. This implies
		 * that all the lines of the destination rectangle that would take
		 * this line of stipple would be stippled before moving on to the
		 * next line of the stipple.
		 */

		/*
		 * Get the source coordinates corresponding to (xl, yt)
		 */
		if ((stipple_width & (stipple_width - 1)))
		{
			source_x = (x_left - stipple_x_origin) %
				stipple_width;
		}
		else
		{
			/*
			 * power of two
			 */
			source_x =  (x_left - stipple_x_origin) &
				(stipple_width - 1);
		}

		if (source_x < 0)
		{
			source_x += inverted_stipple_p->Bwidth;
		}

		if ((stipple_height & (stipple_height - 1)))
		{
			source_y = (y_top - stipple_y_origin) %
				stipple_height;
		}
		else
		{
			/*
			 * height is a power of two.
			 */
			source_y = (y_top - stipple_y_origin) &
				(stipple_height - 1);
		}

		if (source_y < 0)
		{
			source_y += inverted_stipple_p->Bheight;
		}

		number_of_stipple_lines =
			((y_bottom - y_top + 1) > stipple_height) ? stipple_height :
			(y_bottom - y_top + 1);

		stipple_words_p = ((unsigned char *) inverted_stipple_p->Bptr +
			 (stipple_state_p->source_step * source_y));

		for (tmp_count = 0;
			 tmp_count < number_of_stipple_lines;
			 tmp_count ++)
		{
			unsigned short *tmp_stipple_words_p =
				(void *) stipple_words_p;

			/*
			 * Download this line of stipple into the monochrome pattern
			 * registers.
			 */
			MACH_WAIT_FOR_FIFO(1);
			outw(MACH_REGISTER_PATT_DATA_INDEX,
				 DEFAULT_MACH_NUMBER_OF_COLOR_PATTERN_DATA_REGISTERS);

			for (i = 0;
				 i < number_of_patt_data_registers_per_stipple_width;
				 i++, tmp_stipple_words_p++)
			{
				MACH_WAIT_FOR_FIFO(1);
				outw(MACH_REGISTER_PATT_DATA,
					 *tmp_stipple_words_p);
			}

			/*
			 * Fill in all lines which correspond to this line of stipple.
			 */

			for(current_y = y_top + tmp_count;
				current_y <= y_bottom;
				current_y += stipple_height)
			{
				MACH_WAIT_FOR_FIFO(4);
				outw(MACH_REGISTER_PATT_INDEX, source_x);
				outw(MACH_REGISTER_CUR_X, x_left);
				outw(MACH_REGISTER_CUR_Y, current_y);
				outw(MACH_REGISTER_SCAN_X, x_right + 1);
			}

			/*
			 * update source y and the stipple words pointer.
			 */
			source_y ++;

			if (source_y == stipple_height)
			{
				source_y = 0;
				stipple_words_p = (unsigned char *) inverted_stipple_p->Bptr;
			}
			else
			{
				stipple_words_p += stipple_state_p->source_step;
			}
		}
	}


#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	/*
	 * Since we've used up the monochrome pattern registers, mark the
	 * contents of this as out of sync with the graphics state.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);

	/*
	 * Unlock offscreen memory.
	 */
	if (is_offscreen_memory_useable == TRUE)
	{
		/*EMPTY*/
		OMM_UNLOCK(stipple_state_p->offscreen_location_p);
	}
	
	ASSERT(!MACH_IS_IO_ERROR());

	return (SI_SUCCEED);
}


/*
 * Stippling entry point for (large stipple) rectangles.
 */
STATIC SIBool
mach_fill_rectangle_stipple_system_memory(
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
	unsigned short dp_config;

	/*
	 * Ptr to the (0,y) coords in source stipple.
	 * Ptr to the (x,y) coords in source stipple.
	 * Ptr to the (x,0) coords in source stipple.
	 */
	char		*src_y_ptr;
	char		*src_xy_ptr;
	char		*src_x_ptr;

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
		return ( SI_SUCCEED );
	}

	/*
	 * Really an assert.
	 */
	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGfillmode
			== SGFillStipple );

	stipple_state_p = &(graphics_state_p->current_stipple_state);

	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);
	
	
	/*
	 * Download and process the stipple if necessary
	 */

	if (stipple_state_p->is_downloaded == FALSE)
	{
		mach_graphics_state_download_stipple(screen_state_p,
			graphics_state_p,
			graphics_state_p->generic_state.si_graphics_state.SGstipple);
	}
	
	ASSERT(IS_OBJECT_STAMPED(MACH_STIPPLE_STATE, stipple_state_p));

	/*
	 * Determine the type of drawing and synchronize the appropriate
	 * registers. 
	 */
	if (graphics_state_p->generic_state.si_graphics_state.SGstplmode
		== SGStipple)
	{
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
							 MACH_FILL_STIPPLE_TRANSPARENT_DEPENDENCIES);
		MACH_STATE_SET_BG_ROP(screen_state_p,
							  MACH_MIX_FN_LEAVE_ALONE);
	}
	else
	{
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
							 MACH_FILL_STIPPLE_OPAQUE_DEPENDENCIES);
	}

	source_stipple_p =
		&(stipple_state_p->inverted_stipple);


	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_WRITE |
		MACH_DP_CONFIG_ENABLE_DRAW |
		MACH_DP_CONFIG_READ_MODE_MONO_DATA |
		MACH_DP_CONFIG_MONO_SRC_HOST |
		MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR;

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_fill_rectangle_stipple_system_memory)\n"
"{\n"
"\tx_origin = %ld\n"
"\ty_origin = %ld\n"
"\tcount = %ld\n"
"\trect_p = %p\n"
"\tdp_config = 0x%x\n",
					   x_origin, y_origin, count, (void *) rect_p,
					   dp_config);
	}
#endif

	MACH_WAIT_FOR_FIFO(1);

	outw (MACH_REGISTER_SRC_Y_DIR, 1);

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
		if (mach_fill_debug)
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
		if(
		   (rect_x_left > screen_state_p->generic_state.screen_clip_right) ||
		   (rect_x_right < screen_state_p->generic_state.screen_clip_left) ||
		   (rect_y_top > screen_state_p->generic_state.screen_clip_bottom) ||
		   (rect_y_bottom < screen_state_p->generic_state.screen_clip_top) ||
		   (rect_y_bottom < rect_y_top) || ( rect_x_right < rect_x_left ))
		{
#if (defined(__DEBUG__))
			if (mach_fill_debug)
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
		start_x = (destination_x - stipple_x_origin) %
			source_stipple_p->Bwidth;
		start_y = (destination_y - stipple_y_origin) %
			source_stipple_p->Bheight;

		if ( start_x < 0 )
		{
			start_x += source_stipple_p->Bwidth;
		}
		if ( start_y < 0 )
		{
			start_y += source_stipple_p->Bheight;
		}

		/*
		 * This assumes software pre-clipping has been done.
		 */
		MACH_WAIT_FOR_FIFO(1);
		outw(MACH_REGISTER_EXT_SCISSOR_L,	destination_x);

		/*
		 * Move destination_x back to a pixtrans word boundary.
		 */
		if ( start_x & (screen_state_p->pixtrans_width - 1) )
								/* not a pixtrans word boundary */
		{
			/*
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

			startwords = stipple_state_p->number_of_pixtrans_words_per_stipple_width -
				(start_x >> screen_state_p->pixtrans_width_shift);

			start_scan_x_length =
				(startwords << screen_state_p->pixtrans_width_shift);

			tmpdx += source_stipple_p->Bwidth - start_x;
			start_scissor = tmpdx - 1;

			midwidths = (rect_x_right - tmpdx + 1) / source_stipple_p->Bwidth;
			endwords = (rect_x_right - tmpdx + 1) % source_stipple_p->Bwidth;

			if ( midwidths )
			{
				mid_scan_x_length = (source_stipple_p->Bwidth + (screen_state_p->pixtrans_width-1))
											& ~(screen_state_p->pixtrans_width - 1);
				tmpdx += midwidths * source_stipple_p->Bwidth;
			}

			if ( endwords )
			{
				endwords = (endwords + (screen_state_p->pixtrans_width-1))
											& ~(screen_state_p->pixtrans_width - 1);
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

			MACH_WAIT_FOR_FIFO(4);
			outw (MACH_REGISTER_EXT_SCISSOR_R,start_scissor);
			outw (MACH_REGISTER_CUR_X, curdx);
			outw (MACH_REGISTER_DEST_X_START, curdx);
			outw (MACH_REGISTER_DEST_X_END, curdx+start_scan_x_length);

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
				MACH_WAIT_FOR_FIFO(2);
				outw (MACH_REGISTER_CUR_Y, curdy);
				outw (MACH_REGISTER_DEST_Y_END,curdy+stipplelines);
								/* start the blit */

				tmplines = stipplelines;
				MACH_WAIT_FOR_FIFO(16);
				while ( tmplines-- )
				{

					MACH_BULK_TRANSFER_TO_GRAPHICS_ENGINE(
					    screen_state_p->pixtrans_register,
					    startwords,
					    src_stipple_p,
					    (screen_state_p->pixtrans_width >> 4),
					    (*screen_state_p->screen_write_bits_p));


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
					MACH_WAIT_FOR_FIFO(6);
					outw(MACH_REGISTER_EXT_SCISSOR_R,
						 (tmpdx + source_stipple_p->Bwidth - 1));
					outw (MACH_REGISTER_CUR_X, tmpdx);
					outw (MACH_REGISTER_DEST_X_START, tmpdx);
					outw (MACH_REGISTER_DEST_X_END,
						  (tmpdx + mid_scan_x_length));
					outw (MACH_REGISTER_CUR_Y, curdy);
					outw (MACH_REGISTER_DEST_Y_END,
						  curdy+stipplelines); /* start the blit */

					MACH_WAIT_FOR_FIFO(16);
					while ( tmplines-- )
					{
						MACH_BULK_TRANSFER_TO_GRAPHICS_ENGINE(
					    	screen_state_p->pixtrans_register,
							stipple_state_p->
								number_of_pixtrans_words_per_stipple_width,
						    src_line_p,
					  	  	(screen_state_p->pixtrans_width >> 4),
					    	(*screen_state_p->screen_write_bits_p));

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
			 * Setup the Constant portion of the Destination Blit rectangle.
			 */
			MACH_WAIT_FOR_FIFO(4);
			outw (MACH_REGISTER_EXT_SCISSOR_R,rect_x_right);
			outw (MACH_REGISTER_CUR_X, curdx);
			outw (MACH_REGISTER_DEST_X_START, curdx);
			outw (MACH_REGISTER_DEST_X_END, curdx+end_scan_x_length);

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
				 * setup the Remaining part of the Destination Blit rectangle.
				 */
				MACH_WAIT_FOR_FIFO(2);
				outw (MACH_REGISTER_CUR_Y, curdy);
				outw (MACH_REGISTER_DEST_Y_END,curdy+stipplelines);
								/* start the blit */
				tmplines = stipplelines;

				MACH_WAIT_FOR_FIFO(16);
				while ( tmplines-- )
				{
					MACH_BULK_TRANSFER_TO_GRAPHICS_ENGINE(
					    screen_state_p->pixtrans_register,
					    endwords,
					    src_line_ptr,
					    (screen_state_p->pixtrans_width >> 4),
					    (*screen_state_p->screen_write_bits_p));

					src_line_ptr += stipple_state_p->source_step;
				}

				src_line_ptr = (char *) source_stipple_p->Bptr;
				cursy = 0;
				curdy += stipplelines;
			}
		}
	}


#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	/*
	 * Synchronize clip registers.
	 */

	MACH_WAIT_FOR_FIFO(2);
	outw(MACH_REGISTER_EXT_SCISSOR_L,
		 screen_state_p->register_state.ext_scissor_l);
	outw(MACH_REGISTER_EXT_SCISSOR_R,
		 screen_state_p->register_state.ext_scissor_r);

	/*
	 * Pattern register contents are now out of sync with the graphics
	 * state.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);
	
	ASSERT(!MACH_IS_IO_ERROR());

	return (SI_SUCCEED);
}


STATIC SIBool
mach_fill_rectangle_stipple_offscreen_memory(SIint32 x_origin,
	SIint32 y_origin,
	SIint32 count,
	SIRectOutlineP rect_p)
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();

	const struct mach_stipple_state *stipple_state_p = 
		&(graphics_state_p->current_stipple_state);

	struct omm_allocation *allocation_p;

	/*
	 * Clip coordinates
	 */

	const int clip_left =
		screen_state_p->generic_state.screen_clip_left;
	const int clip_right =
		screen_state_p->generic_state.screen_clip_right;
	const int clip_top =
		screen_state_p->generic_state.screen_clip_top;
	const int clip_bottom =
		screen_state_p->generic_state.screen_clip_bottom;

	const int stipple_width =		/* actual width of the stipple */
		graphics_state_p->generic_state.si_graphics_state.SGstipple->Bwidth;

	const int stipple_height =		/* actual height of the stipple */
		graphics_state_p->generic_state.si_graphics_state.SGstipple->Bheight;
	
	int allocation_width;
	
	int allocation_height;

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


	ASSERT(stipple_width > 0);
	ASSERT(stipple_height > 0);
	
	/*
	 * Switch to ATI context.
	 */

	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);
	
	/*
	 * download and process the stipple if necessary.
	 */
	if (stipple_state_p->is_downloaded == FALSE)
	{
		mach_graphics_state_download_stipple(
			screen_state_p,
			graphics_state_p,
			graphics_state_p->generic_state.si_graphics_state.SGstipple);
	}
	

	/*
	 * Examine the current offscreen allocation.
	 */

	allocation_p =
		graphics_state_p->current_stipple_state.offscreen_location_p;
	

	ASSERT(!allocation_p ||
		   IS_OBJECT_STAMPED(OMM_ALLOCATION, allocation_p));

	/*
	 * Do we have a valid offscreen region?
	 */
	if (!OMM_LOCK(allocation_p))
	{
		/*
		 * OMM locking failed : revert to the standard
		 * method of drawing.
		 */
		return mach_fill_rectangle_stipple_system_memory(
				x_origin, y_origin,
				count, rect_p);
	}
	
	allocation_width =
		graphics_state_p->current_stipple_state.offscreen_width;
		
	allocation_height = 
		graphics_state_p->current_stipple_state.offscreen_height;
	
	ASSERT(allocation_width > 0);
	ASSERT(allocation_height > 0);

	if (graphics_state_p->generic_state.si_graphics_state.SGstplmode
		== SGStipple)
	{
		/*
		 * Synchronize registers with the graphics state.
		 */
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
						 MACH_FILL_STIPPLE_TRANSPARENT_OFFSCREEN_DEPENDENCIES);
		MACH_STATE_SET_BG_ROP(screen_state_p,
							  MACH_MIX_FN_LEAVE_ALONE);
	}
	else
	{
		/*
		 * Synchronize registers with the graphics state.
		 */
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
						 MACH_FILL_STIPPLE_OPAQUE_OFFSCREEN_DEPENDENCIES);
	}

	MACH_WAIT_FOR_FIFO(1);
	outw(MACH_REGISTER_SRC_Y_DIR, 1); /* increment src y */

	MACH_STATE_SET_DP_CONFIG(screen_state_p,
		(screen_state_p->dp_config_flags |
		 MACH_DP_CONFIG_WRITE |
		 MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR |
		 MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		 MACH_DP_CONFIG_READ_MODE_MONO_DATA |
		 MACH_DP_CONFIG_MONO_SRC_BLIT |
		 MACH_DP_CONFIG_ENABLE_DRAW));
	
	MACH_STATE_SET_RD_MASK(screen_state_p,
						   allocation_p->planemask);
	
    for(; count--; rect_p++)
	{
		int xl = rect_p->x + x_origin; /* inclusive */
		int xr = xl + rect_p->width - 1; /* inclusive */
		int yt = rect_p->y + y_origin; /* inclusive */
		int yb = yt + rect_p->height - 1; /* inclusive */
		
#if (defined(__DEBUG__))
		if (mach_fill_debug)
		{
			(void) fprintf(debug_stream_p,
"\t{\n"
"\t\txl = %d\n"
"\t\tyt = %d\n"
"\t\txr = %d\n"
"\t\tyb = %d\n"
"\t}\n",
						   xl, yt, xr, yb);
		}
#endif
		
		if ((xl > xr) || (yt > yb) || (xl > clip_right) ||
			(xr <= clip_left) || (yt > clip_bottom) || (yb <= clip_top))
		{
#if (defined(__DEBUG__))
			if (mach_fill_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\t(out of clip bounds)\n");
			}
#endif
			continue;
		}

		/*
		 * Clip and check for bounds.
		 */
		FORCE_COORDINATES_TO_CILP_RECTANGLE(xl,yt,xr,yb,
			clip_left,clip_top,clip_right,clip_bottom);
		
		/*
		 * Call the offscreen helper to stipple the rectangle.
		 */

		mach_fill_rectangle_offscreen_helper(
			stipple_state_p->offscreen_location_p,
			stipple_state_p->offscreen_width,
			stipple_state_p->offscreen_height,
			graphics_state_p->generic_state.si_graphics_state.SGstipple,
			xl,
			yt,
			xr + 1,
			yb + 1);											 
	}

	/*
	 * Pattern register contents are now out of sync with the graphics
	 * state.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);
	

	ASSERT(!MACH_IS_IO_ERROR());
	
	return (SI_SUCCEED);
}

/*
 * Backward compatibility support.
 * 
 * Duplicated from `mach_fill_ibm_rectangle_solid'.
 */

STATIC SIBool
mach_fill_rectangle_ibm_compat_solid(SIint32 count, SIRectP pRect)
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();

	MACH_CURRENT_GRAPHICS_STATE_DECLARE();

	const int clip_left =
		screen_state_p->generic_state.screen_clip_left;

	const int clip_right =
		screen_state_p->generic_state.screen_clip_right;

	const int clip_top =
		screen_state_p->generic_state.screen_clip_top;

	const int clip_bottom =
		screen_state_p->generic_state.screen_clip_bottom;

	int height = -1;
	int width = -1;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE,
							 graphics_state_p));

	ASSERT(screen_state_p->options_p->rectfill_options &
		   MACH_OPTIONS_RECTFILL_OPTIONS_USE_IBM_MODE);

	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		   SGFillSolidFG ||
		   graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		   SGFillSolidBG);

	ASSERT(!MACH_IS_IO_ERROR());

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	if (graphics_state_p->generic_state.si_graphics_state.SGfillmode
		!= SGFillSolidFG &&
		graphics_state_p->generic_state.si_graphics_state.SGfillmode
		!= SGFillSolidBG)
	{
		return (SI_FAIL);
	}


#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_fill_rectangle_ibm_compat_solid)\n"
"{\n"
"\tcount = %ld\n"
"\tpRect = %p\n"
"\tclip rectangle = (%d,%d,%d,%d)\n",
					   count, (void *) pRect,
					   clip_left, clip_top, 
					   clip_right, clip_bottom);
	}
#endif

	/*
	 * Switch to IBM context.
	 */
	MACH_STATE_SWITCH_TO_IBM_CONTEXT(screen_state_p);
	
	/*
	 * Synchronize to graphics state.
	 */
	MACH_STATE_SYNCHRONIZE_STATE(screen_state_p, 
								 MACH_FILL_SOLID_DEPENDENCIES);
	
	/*
	 * set frgd mix to be use frgd color or bkgd color as requested.
	 */
	MACH_WAIT_FOR_FIFO(1);
	outw(MACH_REGISTER_FRGD_MIX,
		 (screen_state_p->register_state.alu_fg_fn |
		  ((graphics_state_p->generic_state.si_graphics_state.SGfillmode
			== SGFillSolidFG) ?
		   MACH_IBM_SELECT_FRGD_COLOR : 
		   MACH_IBM_SELECT_BKGD_COLOR)));

	for (;count--;pRect++)
	{
		register int xl = pRect->ul.x; /* inclusive coordinate */
		register int xr = pRect->lr.x; /* exclusive coordinate */
		register int yt = pRect->ul.y; /* inclusive coordinate */
		register int yb = pRect->lr.y; /* exclusive coordinate */

#if (defined(__DEBUG__))
		if (mach_fill_debug)
		{
			(void) fprintf(debug_stream_p,
"\t{\n"
"\t\txl = %d\n"
"\t\tyt = %d\n"
"\t\txr = %d\n"
"\t\tyb = %d\n"
"\t}\n",
						   xl, yt, xr, yb);
		}
#endif
	   
		/*
		 * Check against the clip rectangle.
		 */
		if ((xl >= xr) || (yt >= yb) || (xl > clip_right) ||
			(xr <= clip_left) || (yt > clip_bottom) || (yb <= clip_top))
		{
#if (defined(__DEBUG__))
			if (mach_fill_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\t(out of clip bounds)\n");
			}
#endif
			continue;
		}

		/*
		 * Draw the rectangle in IBM mode.
		 */

		if (height != (yb - yt))
		{
			height = (yb - yt);
			MACH_WAIT_FOR_FIFO(1);
			outw(MACH_REGISTER_MULTI_FN,
				 MACH_MF_RECT_HEIGHT | ((height - 1) & MACH_MF_VALUE));
		}
		if (width != (xr - xl))
		{
			width = xr - xl;
			MACH_WAIT_FOR_FIFO(1);
			outw(MACH_REGISTER_MAJ_AXIS_PCNT, width - 1);
		}

		if (width <= 0 || height <= 0)
		{
			continue;
		}
			
		MACH_WAIT_FOR_FIFO(3);
		outw(MACH_REGISTER_CUR_X, xl);
		outw(MACH_REGISTER_CUR_Y, yt);
		outw(MACH_REGISTER_CMD,
			 MACH_CMD_FILL_RECT_HOR_CMD | MACH_CMD_YPOS |
			 MACH_CMD_YMAJOR | MACH_CMD_XPOS | MACH_CMD_DRAW |
			 MACH_CMD_PIXEL_MODE_NIBBLE | MACH_CMD_WRITE);

		ASSERT(!MACH_IS_IO_ERROR());
	}

	/*
	 * Using blits destroy the color pattern registers.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);

#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	ASSERT(!MACH_IS_IO_ERROR());

	return (SI_SUCCEED);

}

STATIC SIBool
mach_fill_rectangle_ati_compat_solid(SIint32 count, SIRectP pRect)
{
	unsigned short dp_config;

	MACH_CURRENT_SCREEN_STATE_DECLARE();

	MACH_CURRENT_GRAPHICS_STATE_DECLARE();

	const int clip_left =
		screen_state_p->generic_state.screen_clip_left;

	const int clip_right =
		screen_state_p->generic_state.screen_clip_right;

	const int clip_top =
		screen_state_p->generic_state.screen_clip_top;

	const int clip_bottom =
		screen_state_p->generic_state.screen_clip_bottom;

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

	ASSERT(!MACH_IS_IO_ERROR());

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	if (graphics_state_p->generic_state.si_graphics_state.SGfillmode
		!= SGFillSolidFG &&
		graphics_state_p->generic_state.si_graphics_state.SGfillmode
		!= SGFillSolidBG)
	{
		return (SI_FAIL);
	}

	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_MONO_SRC_ONE |
		MACH_DP_CONFIG_WRITE |
		MACH_DP_CONFIG_ENABLE_DRAW |
		(((graphics_state_p->generic_state.si_graphics_state.SGfillmode
		   == SGFillSolidFG) ?
		  MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR :
		  MACH_DP_CONFIG_FG_COLOR_SRC_BKGD_COLOR));

#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_fill_rectangle_ati_compat_solid)\n"
"{\n"
"\tcount = %ld\n"
"\tpRect = %p\n"
"\tdp_config = 0x%x\n"
"\tclip rectangle = (%d,%d,%d,%d)\n",
					   count, (void *) pRect,
					   dp_config,
					   clip_left, clip_top,
					   clip_right, clip_bottom);
	}
#endif

	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	MACH_STATE_SYNCHRONIZE_STATE(screen_state_p, 
								 MACH_FILL_SOLID_DEPENDENCIES);

	/*
	 * Program the DP_CONFIG value needed.
	 */
	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

	/*
	 * Top to bottom blits.
	 */
	MACH_WAIT_FOR_FIFO(1);
	outw(MACH_REGISTER_SRC_Y_DIR, 1);

	for (;count--;pRect++)
	{
		register int xl = pRect->ul.x; /* inclusive coordinate */
		register int xr = pRect->lr.x; /* exclusive coordinate */
		register int yt = pRect->ul.y; /* inclusive coordinate */
		register int yb = pRect->lr.y; /* exclusive coordinate */

#if (defined(__DEBUG__))
		if (mach_fill_debug)
		{
			(void) fprintf(debug_stream_p,
"\t{\n"
"\t\txl = %d\n"
"\t\tyt = %d\n"
"\t\txr = %d\n"
"\t\tyb = %d\n"
"\t}\n",
						   xl, yt, xr, yb);
		}
#endif
	   
		/*
		 * Check against the clip rectangle.
		 */
		if ((xl >= xr) || (yt >= yb) || (xl > clip_right) ||
			(xr <= clip_left) || (yt > clip_bottom) || (yb <= clip_top))
		{
#if (defined(__DEBUG__))
			if (mach_fill_debug)
			{
				(void) fprintf(debug_stream_p,
							   "\t\t(out of clip bounds)\n");
			}
#endif
			continue;
		}

		MACH_WAIT_FOR_FIFO(5);
		outw(MACH_REGISTER_CUR_X, xl);
		outw(MACH_REGISTER_DEST_X_START, xl);
		outw(MACH_REGISTER_DEST_X_END, xr);
		outw(MACH_REGISTER_CUR_Y, yt);
		outw(MACH_REGISTER_DEST_Y_END, yb);	/* Blit starts */

		ASSERT(!MACH_IS_IO_ERROR());
	}

	/*
	 * Using blits destroy the color pattern registers.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);

#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif

	ASSERT(!MACH_IS_IO_ERROR());

	return (SI_SUCCEED);
}

/*
 * mach_fill_rectangle_compat_other
 * 
 * Handling all other fill styles in ATI compatibility mode.
 */

STATIC SIBool
mach_fill_rectangle_compat_other(SIint32 count, SIRectP pRect)
{
	int		ret_val;
	boolean is_local_allocation;
	
	SIRectOutlineP	p_new_rect, tmp_new_rect_p;
	SIRectP			tmp_old_rect_p;
	SIint32 		tmpcount;

	SIBool (*called_function_p)(SIint32, SIint32, SIint32,
								SIRectOutlineP) = NULL;
	SIRectOutline				/* for fast allocations */
		localRectangles[DEFAULT_MACH_COMPATIBILITY_LOCAL_RECTANGLE_COUNT];
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();

#if (defined(__DEBUG__))
	if (mach_fill_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_fill_rectangle_compat_other)\n"
"{\n"
"\tcount = %ld\n"
"\tpRect = %p\n",
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
	if (count > DEFAULT_MACH_COMPATIBILITY_LOCAL_RECTANGLE_COUNT)
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
			MACH_OPTIONS_RECTFILL_OPTIONS_USE_SOLID_FILL_RECT)
		{
			if ((screen_state_p->options_p->rectfill_options &
				 MACH_OPTIONS_RECTFILL_OPTIONS_USE_IBM_MODE) &&
				(screen_state_p->generic_state.screen_virtual_width <=
				 (DEFAULT_MACH_MAX_IBM_LEFT_X + 1)))
			{
				called_function_p = mach_fill_ibm_rectangle_solid;
								/* IBM mode drawing code */
			}
			else
			{
				called_function_p = mach_fill_rectangle_solid; 
								/* ATI drawing code */
			}
			/* IBM mode drawing does not work in 16bpp mode. */
			if (screen_state_p->generic_state.screen_depth == 16)
			{
				called_function_p = mach_fill_rectangle_solid; 
								/* ATI drawing code */
			}
		}
		break;

	case SGFillTile :
		if (screen_state_p->options_p->rectfill_options &
			MACH_OPTIONS_RECTFILL_OPTIONS_USE_TILE_FILL_RECT)
		{
			if (((screen_state_p->options_p->rectfill_options &
				  MACH_OPTIONS_RECTFILL_OPTIONS_USE_PATTERN_REGISTERS) ||
				 (screen_state_p->options_p->rectfill_options &
				  MACH_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY)) &&
				graphics_state_p->current_tile_state.is_small_tile)
			{
				called_function_p =
					mach_fill_rectangle_small_tile;
			}
			else if ((screen_state_p->options_p->rectfill_options &
					  MACH_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY) &&
					 (screen_state_p->options_p->rectfill_options &
					  MACH_OPTIONS_RECTFILL_OPTIONS_USE_LARGE_OFFSCREEN_AREAS))
			{
				/*
				 * large tiles.
				 */

				called_function_p =
					mach_fill_rectangle_tile_offscreen_memory;
			}
			else				/* generic tiling routine */
			{
				called_function_p =
					mach_fill_rectangle_tile_system_memory;
			}
		}
		break;

	case SGFillStipple:
		if (screen_state_p->options_p->rectfill_options &
			MACH_OPTIONS_RECTFILL_OPTIONS_USE_STIPPLE_FILL_RECT)
		{
			if (((screen_state_p->options_p->rectfill_options &
				   MACH_OPTIONS_RECTFILL_OPTIONS_USE_PATTERN_REGISTERS) || 
				 (screen_state_p->options_p->rectfill_options &
				  MACH_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY)) &&
				graphics_state_p->current_stipple_state.is_small_stipple)
			{
				called_function_p =
					mach_fill_rectangle_small_stipple;
			}
			else if ((screen_state_p->options_p->rectfill_options &
				 MACH_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY) &&
					 (screen_state_p->options_p->rectfill_options &
			     MACH_OPTIONS_RECTFILL_OPTIONS_USE_LARGE_OFFSCREEN_AREAS))
			{
				/*
				 * Large stipples.
				 */

				called_function_p =
					mach_fill_rectangle_stipple_offscreen_memory;
			}
			else
			{
				called_function_p =
					mach_fill_rectangle_stipple_system_memory;
			}
		}
		break;
		
	default :
		/*CONSTANTCONDITION*/
		ASSERT(0);
		break;
		
	}

	ASSERT(called_function_p != NULL);
	
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
mach_fill__gs_change__(void)
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
				MACH_OPTIONS_RECTFILL_OPTIONS_USE_SOLID_FILL_RECT)
			{
				if ((screen_state_p->options_p->rectfill_options &
					 MACH_OPTIONS_RECTFILL_OPTIONS_USE_IBM_MODE) && 
					(screen_state_p->generic_state.screen_virtual_width <=
					 (DEFAULT_MACH_MAX_IBM_LEFT_X + 1)))
				{
					screen_state_p->generic_state.screen_functions_p->
						si_poly_fillrect = mach_fill_ibm_rectangle_solid;
				}
				else			/* 1025 and higher */
				{
					screen_state_p->generic_state.screen_functions_p->
						si_poly_fillrect = mach_fill_rectangle_solid;
				}
				/* IBM mode drawing does not work in 16bpp mode. */
				if (screen_state_p->generic_state.screen_depth == 16)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_poly_fillrect = mach_fill_rectangle_solid;
				}
			}
			break;

		case SGFillTile :
			if (screen_state_p->options_p->rectfill_options &
				MACH_OPTIONS_RECTFILL_OPTIONS_USE_TILE_FILL_RECT)
			{
				if (((screen_state_p->options_p->rectfill_options &
					  MACH_OPTIONS_RECTFILL_OPTIONS_USE_PATTERN_REGISTERS)
						  || (screen_state_p->options_p->rectfill_options &
					  MACH_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY)) && 
						 graphics_state_p->current_tile_state.is_small_tile)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_poly_fillrect =
							mach_fill_rectangle_small_tile;
				}
				else if ((screen_state_p->options_p->rectfill_options &
					 MACH_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY) &&
						 (screen_state_p->options_p->rectfill_options &
					 MACH_OPTIONS_RECTFILL_OPTIONS_USE_LARGE_OFFSCREEN_AREAS))
				{
					/*
					 * Large tiles.
					 */

					screen_state_p->generic_state.screen_functions_p->
						si_poly_fillrect =
							mach_fill_rectangle_tile_offscreen_memory;
				}
				else			/* generic tiling */
				{
					screen_state_p->generic_state.screen_functions_p->
						si_poly_fillrect =
							mach_fill_rectangle_tile_system_memory;
				}
			}
			break;

		case SGFillStipple:
			if (screen_state_p->options_p->rectfill_options &
				MACH_OPTIONS_RECTFILL_OPTIONS_USE_STIPPLE_FILL_RECT)
			{
				if (((screen_state_p->options_p->rectfill_options &
				  MACH_OPTIONS_RECTFILL_OPTIONS_USE_PATTERN_REGISTERS) ||
					 (screen_state_p->options_p->rectfill_options &
				  MACH_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY)) &&
					graphics_state_p->current_stipple_state.
					is_small_stipple)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_poly_fillrect =
							mach_fill_rectangle_small_stipple;
				}
				else if ((screen_state_p->options_p->rectfill_options &
					  MACH_OPTIONS_RECTFILL_OPTIONS_USE_OFFSCREEN_MEMORY) &&
						 (screen_state_p->options_p->rectfill_options &
					  MACH_OPTIONS_RECTFILL_OPTIONS_USE_LARGE_OFFSCREEN_AREAS))
				{
					/*
					 * Large stipples.
					 */

					screen_state_p->generic_state.screen_functions_p->
						si_poly_fillrect =
							mach_fill_rectangle_stipple_offscreen_memory;
				}
				else
				{
					screen_state_p->generic_state.screen_functions_p->
						si_poly_fillrect =
							mach_fill_rectangle_stipple_system_memory;
				}
			}
			break;

		default :
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
			
		}
	}
	else
	{
		/*
		 * Backward compatibility mode support.
		 */
		switch (graphics_state_p->generic_state.si_graphics_state.SGfillmode)
		{
		case SGFillSolidFG:
		case SGFillSolidBG:
			/* IBM mode drawing does not work in 16bpp mode. */
			screen_state_p->generic_state.screen_functions_p->
				si_poly_fillrect = 
					(SIBool (*)(SIint32, SIint32,
								SIint32,
								SIRectOutlineP))
					(((screen_state_p->options_p->rectfill_options &
					   MACH_OPTIONS_RECTFILL_OPTIONS_USE_IBM_MODE) && 
					  ((screen_state_p->generic_state.screen_virtual_width <=
					   (DEFAULT_MACH_MAX_IBM_LEFT_X + 1)) &&
					   screen_state_p->generic_state.screen_depth != 16)) ?
					 mach_fill_rectangle_ibm_compat_solid : 
					 mach_fill_rectangle_ati_compat_solid);
			break;
			
		default:
			screen_state_p->generic_state.screen_functions_p->
				si_poly_fillrect = 
					(SIBool (*)(SIint32, SIint32,
								SIint32,
								SIRectOutlineP))
						mach_fill_rectangle_compat_other;
			break;
			
		}
	}
}

/*
 * Initialization.
 */
function void
mach_fill__initialize__(SIScreenRec *si_screen_p,
						struct mach_options_structure *options_p)
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
						   MACH_BAD_BEST_TILE_SIZE_SPECIFICATION_OPTION,
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
						   MACH_BAD_BEST_STIPPLE_SIZE_SPECIFICATION_OPTION,
						   options_p->stipple_best_size);
			stipple_best_width = stipple_best_height = 0;
		}
	}

	flags_p->SItilewidth = tile_best_width ? tile_best_width :
		DEFAULT_MACH_BEST_TILE_WIDTH;
	flags_p->SItileheight = tile_best_height ? tile_best_height :
		DEFAULT_MACH_BEST_TILE_HEIGHT;
	flags_p->SIstipplewidth = stipple_best_width ? stipple_best_width
		: DEFAULT_MACH_BEST_STIPPLE_WIDTH;
	flags_p->SIstippleheight = stipple_best_height ?
		stipple_best_height : DEFAULT_MACH_BEST_STIPPLE_HEIGHT;

	functions_p->si_poly_fconvex = mach_no_operation_fail;
	functions_p->si_poly_fgeneral = mach_no_operation_fail;

	functions_p->si_poly_fillrect = mach_no_operation_fail;

}

