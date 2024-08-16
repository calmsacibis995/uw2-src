/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_line.c	1.4"

/***
 ***	NAME
 ***
 ***		p9k_line.c : Line drawing code for the P9000 display
 ***					  library.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "p9k_line.h"
 ***
 ***	DESCRIPTION
 ***
 ***	@doc:p9k_line.c:
 ***	
 ***	This module implements zero-width line, segment and rectangle
 ***	drawing for the P9000 display library.  The P9000 engine supports
 ***	only solid lines directly in hardware.  Dashed lines have to
 ***	be built over this foundation.
 ***	
 ***	The hardware further supports zero-width lines with capstyles 
 ***	of CapButt, and CapProjecting.  Lines with capstyles of
 ***	CapNotLast need special handling in order to achieve
 ***	compliance to the X protocol.
 ***
 ***	@enddoc
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

#include <sidep.h>
#include "stdenv.h"
#include "p9k_opt.h"

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
export enum debug_level p9000_line_debug = DEBUG_LEVEL_NONE;
#endif

/*
 *	Current module state.
 */

PRIVATE

/***
 ***	Private declarations.
 ***/

struct p9000_line_state
{
	
	/*
	 * Flag to indicate if a given dash pattern will fit into the
	 * pattern registers.
	 */

	boolean is_small_dash_pattern;
	
	/*
	 * Pattern registers.
	 */
	
	unsigned long pattern_registers[P9000_DEFAULT_PATTERN_REGISTER_COUNT];

#if (defined(__DEBUG__))
	int stamp;
#endif 

};

/***
 ***	Includes.
 ***/

#include "p9k_state.h"
#include "p9k_gs.h"
#include "p9k_regs.h"
#include "p9k_gbls.h"
#include "p9k_misc.h"

/***
 ***	Constants.
 ***/

#define P9000_LINE_STATE_STAMP								\
	(('P' << 0) + ('9' << 1) + ('0' << 2) + ('0' << 3) +	\
	 ('0' << 4) + ('_' << 5) + ('L' << 6) + ('I' << 7) +	\
	 ('N' << 8) + ('E' << 9) + ('_' << 10) + ('S' << 11) + 	\
	 ('T' << 12) + ('A' << 13) + ('T' << 14))


/***
 ***	Macros.
 ***/


/*
 * Macros useful when checking segments against clip bounds.
 */

#define OUT_LEFT	0x08
#define	OUT_RIGHT	0x04
#define	OUT_ABOVE	0x02
#define OUT_BELOW	0x01

/*
 * @doc:OUTCODE:
 *
 * set flags to mark whether the given point (`x', `y') is out of the
 * clip rectangle specified by `left', `right', `top', `bottom'.  The
 * clip rectangle is specified in inclusive coordinates.
 * 
 * @enddoc
 */

#define OUTCODE(result, x, y, left, right, top, bottom)	\
	{													\
		(result) = 0;									\
		if ((x) < (left))								\
			(result) |= OUT_LEFT;						\
		else if ((x) > (right))							\
			(result) |= OUT_RIGHT;						\
		if ((y) < (top))								\
			(result) |= OUT_ABOVE;						\
		else if ((y) > (bottom))						\
			(result) |= OUT_BELOW;						\
	}


/* 
 * Common macros for bresenham line drawing.
 */

#define ABS(X) (((X) >= 0) ? (X) : -(X))
#define SIGN(X) (((X) > 0) ? 1 : (((X) < 0) ? -1 : 0))

/*
 * @doc:COMPUTE_LAST_BUT_ONE_POINT_AND_CLIP_REGISTERS:
 *
 * Used for drawing CapNotLast lines.  In order to avoid upsetting the
 * graphics engines bresenham trajectory, we need to compute the next
 * to last point of the line and set the clip rectangle so that the
 * actual last point of the line is not drawn.  If the last point of
 * the line is out of the current clip rectangle, then we should clip
 * the line till the clip rectangle coordinates as usual.
 *
 * This macro, modifies parameters `X2' and `Y2' to be the coordinates
 * of the next to last point of the line being drawn.  The required
 * clip coordinates `clip_{left,right,top,bottom}' are set to the
 * required clipping rectangle.
 * 
 * @enddoc
 */

#define COMPUTE_LAST_BUT_ONE_POINT_AND_CLIP_REGISTERS(X1, Y1, X2, Y2,	\
													  X2_ORIG, Y2_ORIG)	\
	{																	\
		int dx = (X1) - (X2);											\
		int dy = (Y1) - (Y2);											\
		int adx = ABS(dx);												\
		int ady = ABS(dy);												\
		int signdx = SIGN(dx);											\
		int signdy = SIGN(dy);											\
		int e;															\
																		\
		if (adx > ady)													\
		{																\
			e = (ady << 1) - adx;										\
			if (adx > 1)												\
			{															\
				if (((signdx > 0) && (e < 0)) ||						\
					((signdx <= 0) && (e <= 0)))						\
				{														\
					(X2) += signdx;										\
				}														\
				else													\
				{														\
					(X2) += signdx;										\
					(Y2) += signdy;										\
				}														\
			}															\
		}																\
		else															\
		{																\
			e = (adx << 1) - ady;										\
			if (ady > 1)												\
			{															\
				if (!(((signdx > 0) && (e < 0)) ||						\
					  ((signdx <= 0) && (e <= 0))))						\
				{														\
					(X2) += signdx;										\
				}														\
				(Y2) += signdy;											\
			}															\
		}																\
																		\
		if ((X2) != (X2_ORIG))											\
		{																\
			if (signdx > 0)												\
			{															\
				clip_left = ((X2) < screen_clip_left) ?					\
					screen_clip_left : ((X2));							\
				clip_right = screen_clip_right;							\
																		\
			}															\
			else														\
			{															\
				clip_left = screen_clip_left;							\
				clip_right = ((X2) > screen_clip_right) ?				\
					screen_clip_right : ((X2));							\
			}															\
		}																\
		else															\
		{																\
			clip_left = screen_clip_left;								\
			clip_right = screen_clip_right;								\
		}																\
																		\
		if ((Y2) != (Y2_ORIG))											\
		{																\
			if (signdy > 0)												\
			{															\
				clip_top = ((Y2) < screen_clip_top) ?					\
					screen_clip_top : ((Y2));							\
				clip_bottom = screen_clip_bottom;						\
			}															\
			else														\
			{															\
				clip_top = screen_clip_top;								\
				clip_bottom = ((Y2) > screen_clip_bottom) ?				\
					screen_clip_bottom : ((Y2));						\
			}															\
		}																\
		else															\
		{																\
			clip_top = screen_clip_top;									\
			clip_bottom = screen_clip_bottom;							\
		}																\
	}

/***
 ***	Functions.
 ***/


/*
 * @doc:p9000_line_segment_clip_segment:
 * 
 * Clip a segment to the clip rectangle.  The segment is drawn from
 * (`xl', `yt') to (`xr', `yb').  The clipping rectangle is specified
 * in inclusive coordinates.
 *
 * This function returns the pointer to an `SISegment' structure
 * containing the clipped line segment, or NULL, if the line is
 * entirely outside the clipping rectangle.
 * 
 * @enddoc
 */

STATIC SISegmentP
p9000_line_segment_clip_segment(
	int xl, int yt, int xr, int yb, 
	const int left_clip, const int top_clip, 
	const int right_clip, const int bottom_clip,
	SISegmentP clip_segment_p)
{
	int oc1, oc2;

	if (xl == xr && yt == yb)
	{
		return NULL;
	}
	
	OUTCODE(oc1, xl, yt, left_clip, right_clip, top_clip, bottom_clip);
	OUTCODE(oc2, xr, yb, left_clip, right_clip, top_clip, bottom_clip);
	
	if (oc1 & oc2)
	{
		return NULL;
	}
	
	while (oc1 | oc2)
	{
		int oc;					/* out code */
		int x, y;				/* clipped coordinates */
		int flag;
		
		if (oc1 & oc2)
		{
			return NULL;		/* out of clip box */
		}
		
		if (xl == xr && yt == yb)
		{
			/* can't have both points the same
			   AND out of the clip box */
			return NULL;
		}
		
		ASSERT((xl != xr) || (yt != yb));
		
		if (oc1)
		{
			oc = oc1;
			flag = 1;
		}
		else
		{
			oc = oc2;
			flag = 0;
		}
		
		if (oc & OUT_LEFT)		/* crosses left edge */
		{
			ASSERT(xr != xl);
			y = yt + (yb - yt) * (left_clip - xl) / (xr - xl);
			x = left_clip;
		}
		else if (oc & OUT_RIGHT) /* crosses right edge */
		{
			ASSERT(xr != xl);
			y = yt + (yb - yt) * (right_clip - xl) / (xr - xl);
			x = right_clip;
		}
		else if (oc & OUT_BELOW) /* crosses bottom edge */
		{
			ASSERT(yt != yb);
			x = xl + (xr - xl) * (bottom_clip - yt) / (yb - yt);
			y = bottom_clip;
		}
		else if (oc & OUT_ABOVE) /* crosses top edge */
		{
			ASSERT(yt != yb);
			x = xl + (xr - xl) * (top_clip - yt) / (yb - yt);
			y = top_clip;
		}
#if (defined(__DEBUG__))
		else
		{
			/*CONSTANTCONDITION*/
			ASSERT(0);
		}
#endif

		/*
		 * Recompute the outcodes.
		 */

		if (flag)
		{
			xl = x;
			yt = y;
			OUTCODE(oc1, xl, yt, left_clip, right_clip, top_clip,
					bottom_clip);
		}
		else 
		{
			xr = x;
			yb = y;
			OUTCODE(oc2, xr, yb, left_clip, right_clip, top_clip,
					bottom_clip); 
		}
	}
	
	
	ASSERT(!oc1 && !oc2);
	
	/*
	 * Fill up returned parameters.
	 */

	clip_segment_p->x1 = (short) xl;
	clip_segment_p->y1 = (short) yt;
	clip_segment_p->x2 = (short) xr;
	clip_segment_p->y2 = (short) yb;

	return clip_segment_p;

}

/*
 * @doc:p9000_draw_clipped_segment:
 *
 * Draws a clipped line from `x1', `y1' to `x2', `y2'.  This 
 * function is called when the graphics engine indicates that one or
 * more coordinates of the line being drawn is out of range of
 * coordinates that it supports.
 *
 * The given line is first clipped to the current clip rectangle and
 * then drawn using the graphics engine as usual.  Use is made of the
 * meta coordinate engine's parameter translation facility, so the
 * caller pass relative or absolute coordinates for the line endpoints.
 * 
 * @enddoc 
 */

STATIC void
p9000_draw_clipped_segment(int x1, int y1, int x2, int y2)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();

	int status;					/* status returned by the graphics engine */
	
	/*
	 * Coordinates are out of range ... clip and draw in software.
	 */
		
	const int top_clip =
		screen_state_p->generic_state.screen_clip_top;
	const int bottom_clip =
		screen_state_p->generic_state.screen_clip_bottom;
	const int left_clip =
		screen_state_p->generic_state.screen_clip_left;
	const int right_clip =
		screen_state_p->generic_state.screen_clip_right;

	SISegment clipped_segment;
		
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, SCAFFOLDING))
	{
		(void) fprintf (debug_stream_p, 
						"(p9000_draw_clipped_segment)\n"
						"{\n"
						"\tx1 = %d\n"
						"\ty1 = %d\n"
						"\tx2 = %d\n"
						"\ty2 = %d\n"
						"}\n",
						x1, y1, x2, y2);
	}
#endif

	/*
	 * Clip the segment.
	 */

	if (p9000_line_segment_clip_segment(x1,y1,
										x2,y2, 
										left_clip, top_clip, 
										right_clip, bottom_clip, 
										&clipped_segment))
	{
		x1 = clipped_segment.x1;
		y1 = clipped_segment.y1;
		x2 = clipped_segment.x2;
		y2 = clipped_segment.y2;
	}
	else
	{
		/*
		 * Nothing to draw.
		 */

		return;
		
	}

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
	{
		(void) fprintf (debug_stream_p, 
						"# Clipped line\n"
						"{\n"
						"\tx1 = %d\n"
						"\ty1 = %d\n"
						"\tx2 = %d\n"
						"\ty2 = %d\n"
						"}\n",
						x1, y1, x2, y2);
	}
#endif

	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_CINDEX,
		0);

	/*
	 * Write the meta coordinate registers.  x1, y1 and x2, y2.
	 */

	P9000_WRITE_META_COORDINATE_REGISTER(
				P9000_META_COORDINATE_VTYPE_LINE,
				P9000_META_COORDINATE_ABS,
				P9000_META_COORDINATE_YX_XY,
				P9000_PACK_XY(x1,y1));
											 
	P9000_WRITE_META_COORDINATE_REGISTER(
				P9000_META_COORDINATE_VTYPE_LINE,
 				P9000_META_COORDINATE_ABS,
				P9000_META_COORDINATE_YX_XY,
				P9000_PACK_XY(x2,y2));
											 
	/*
	 * Initiate the line draw.
	 */

	do 
	{
		status = P9000_INITIATE_QUAD_COMMAND();
	} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);
		
	/*
	 * This time we should not be needing to do the blit in
	 * software.
	 */

	ASSERT(!(status & P9000_STATUS_QUAD_SOFTWARE));
}

#define GET_NEXT_DASH()											\
	{															\
		if (++dash_index >= dash_list_length)					\
		{														\
			dash_index = 0;										\
		}														\
		dash_remaining = dash_list_p[dash_index];				\
		if ((this_dash_length = dash_remaining) > line_length)	\
		{														\
			dash_remaining -= line_length;						\
			this_dash_length = line_length;						\
		}														\
	}

#define DO_H_LINE(x, y, width)								\
	{														\
		P9000_WRITE_META_COORDINATE_REGISTER(				\
			P9000_META_COORDINATE_VTYPE_LINE,				\
			P9000_META_COORDINATE_ABS,						\
			P9000_META_COORDINATE_YX_XY,					\
			P9000_PACK_XY((x), (y)));						\
		P9000_WRITE_META_COORDINATE_REGISTER(				\
			P9000_META_COORDINATE_VTYPE_LINE,				\
			P9000_META_COORDINATE_ABS,						\
			P9000_META_COORDINATE_YX_XY,					\
			P9000_PACK_XY(((x) + (width)), (y)));			\
		do { status = P9000_INITIATE_QUAD_COMMAND(); }		\
		while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);	\
	}

#define DO_V_LINE(x, y, height)								\
	{														\
		P9000_WRITE_META_COORDINATE_REGISTER(				\
			P9000_META_COORDINATE_VTYPE_LINE,				\
			P9000_META_COORDINATE_ABS,						\
			P9000_META_COORDINATE_YX_XY,					\
			P9000_PACK_XY((x), (y)));						\
		P9000_WRITE_META_COORDINATE_REGISTER(				\
			P9000_META_COORDINATE_VTYPE_LINE,				\
			P9000_META_COORDINATE_ABS,						\
			P9000_META_COORDINATE_YX_XY,					\
			P9000_PACK_XY((x), ((y) + (height))));			\
		do { status = P9000_INITIATE_QUAD_COMMAND(); }		\
		while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);	\
	}


/*
 * @doc:p9000_line_segment_dashed_horizontal_helper:
 *
 * Draw a dashed line from x_left to x_right, both points inclusive.
 *
 * @enddoc
 */

STATIC void
p9000_line_segment_dashed_horizontal_helper(
	int x_left,
	int x_right,
	int y,
	boolean is_double_dash,
	unsigned int foreground_raster,
	unsigned int background_raster,											
	int dash_used_up,
	int dash_list_length,
	long *dash_list_p)
{

	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	
	int line_length = (x_right - x_left) + 1;
	
	int this_dash_length;

	int dash_remaining;
	
	int dash_index = 0;
	
	int status;					/* graphics engine status */
	int current_x = x_left;
	
	ASSERT(line_length > 0);
	ASSERT(dash_index >= 0 && dash_index < dash_list_length);
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_line_segment_dashed_horizontal_helper)\n"
					   "{\n"
					   "\tx_left = %d\n"
					   "\tx_right = %d\n"
					   "\ty = %d\n"
					   "\tis_double_dash = %s\n"
					   "\tdash_used_up = %d\n"
					   "\tdash_list_length  =%d\n"
					   "\tdash_list_p = %p\n"
					   "}\n",
					   x_left, x_right, y,
					   boolean_to_dump[is_double_dash],
					   dash_used_up, dash_list_length, 
					   (void *) dash_list_p);
	}
#endif

	dash_remaining = dash_list_p[dash_index] - dash_used_up;
	
	if ((this_dash_length = dash_remaining) >= line_length)
	{
		this_dash_length = line_length;
		dash_remaining -= line_length;
	}
	
	/*
	 * Start off with the foreground color.
	 */

	P9000_STATE_SET_RASTER(register_state_p, foreground_raster);

	for(;;)
	{
		if (dash_index & 1)
		{
			/*
			 * Odd dash.
			 */
		
			if (is_double_dash)
			{
				/*
				 * Draw the dash length in the background color.
				 */

				P9000_STATE_SET_RASTER(register_state_p, background_raster);
			
				DO_H_LINE(current_x, y, this_dash_length);
			
				P9000_STATE_SET_RASTER(register_state_p, foreground_raster);
			}
		}
		else
		{

			/*
			 * Even dash, draw in the foreground color.
			 */

			DO_H_LINE(current_x, y, this_dash_length);
		
		}
	
		/*
		 * Advance X coordinate.
		 */

		current_x += this_dash_length;

		/*
		 * Check if we have finished the line yet.
		 */

		line_length -= this_dash_length;
		
		if (line_length <= 0)
		{
			break;
		}
		
		GET_NEXT_DASH();
		
	}
}


/*
 * @doc:p9000_line_segment_dashed_vertical_helper:
 *
 * Draw a dashed line from y_top to y_bottom, both points inclusive.
 *
 * @enddoc
 */

STATIC void
p9000_line_segment_dashed_vertical_helper(
	int y_top,
	int y_bottom,
	int x,
	boolean is_double_dash,
	unsigned int foreground_raster,
	unsigned int background_raster,											
	int dash_used_up,
	int dash_list_length,
	long *dash_list_p)
{

	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	
	int line_length = (y_bottom - y_top) + 1;
	
	int this_dash_length;

	int dash_remaining;
	
	int dash_index = 0;
	
	int status;					/* graphics engine status */
	int current_y = y_top;
	
	ASSERT(line_length > 0);
	ASSERT(dash_index >= 0 && dash_index < dash_list_length);
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_line_segment_dashed_vertical_helper)\n"
					   "{\n"
					   "\ty_top = %d\n"
					   "\ty_bottom = %d\n"
					   "\tx = %d\n"
					   "\tis_double_dash = %s\n"
					   "\tdash_used_up = %d\n"
					   "\tdash_list_length  =%d\n"
					   "\tdash_list_p = %p\n"
					   "}\n",
					   y_top, y_bottom, x,
					   boolean_to_dump[is_double_dash],
					   dash_used_up, dash_list_length, 
					   (void *) dash_list_p);
	}
#endif

	dash_remaining = dash_list_p[dash_index] - dash_used_up;
	
	if ((this_dash_length = dash_remaining) >= line_length)
	{
		this_dash_length = line_length;
		dash_remaining -= line_length;
	}
	
	/*
	 * Start off with the foreground color.
	 */

	P9000_STATE_SET_RASTER(register_state_p, foreground_raster);

	for(;;)
	{
		if (dash_index & 1)
		{
			/*
			 * Odd dash.
			 */
		
			if (is_double_dash)
			{
				/*
				 * Draw the dash length in the background color.
				 */

				P9000_STATE_SET_RASTER(register_state_p, background_raster);
			
				DO_V_LINE(x, current_y, this_dash_length);
			
				P9000_STATE_SET_RASTER(register_state_p, foreground_raster);
			}
		}
		else
		{

			/*
			 * Even dash, draw in the foreground color.
			 */

			DO_V_LINE(x, current_y, this_dash_length);
		
		}
	
		/*
		 * Advance X coordinate.
		 */

		current_y += this_dash_length;

		/*
		 * Check if we have finished the line yet.
		 */

		line_length -= this_dash_length;
		
		if (line_length <= 0)
		{
			break;
		}
		
		GET_NEXT_DASH();
		
	}

}

/*																			
 * @doc:p9000_one_bit_segment_one_bit_helper:
 * 																			
 * Helper function for segment drawing for the P9000 library.
 * 
 * CapNotLast segments need special care, as the graphics engine does
 * not support CapNotLast drawing directly in the hardware.
 * 
 * Clipping of lines that go outside the graphics engines device
 * coordinate space is not directly done, but relies on the parameter
 * engine indicating the out-of-coordinate request using the
 * appropriate status bits returned from the line draw command.  This
 * greatly speeds up the path for drawing lines which are falling
 * inside the device coordinates.
 *
 * The graphics engine clip state is taken to be correctly set before
 * this function is invoked.
 * 
 * @enddoc 
 */
 																			
STATIC void
p9000_line_segment_one_bit_helper(SIint32 xorg, 
								  SIint32 yorg, 
								  register SISegmentP segment_p, 
								  SIint32 count,
								  const int isCapNotLast,
								  const int is_dashed_segment)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();

	register int status;

	register const SISegmentP segmentFence = segment_p + count;

#if (defined(__DEBUG__))
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
#endif 

	ASSERT(is_dashed_segment == TRUE ||
		   (graphics_state_p->generic_state.si_graphics_state.SGlinestyle ==
			SGLineSolid));

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					  "(p9000_line_segment_one_bit_helper)\n"
					  "{\n"
					  "\txorg = %ld\n"
					  "\tyorg = %ld\n"
					  "\tsegment_p = 0x%p\n"
					  "\tcount = %ld\n"
					  "\tisCapNotLast = %d\n"
					  "}\n",
					  xorg, yorg, (void *) segment_p, count,
					  isCapNotLast);
	}
#endif
					  
	/*
	 * Null the current meta-coordinate register index.
	 */
	
	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_CINDEX,
		0);
	
	ASSERT(P9000_READ_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_CINDEX) == 0); /* check */


	if (isCapNotLast)
	{

		/*
		 * CapNotLast lines need special handling when using the
		 * oversized mode of the graphics engine ... since in
		 * oversized mode, all pixels on the line including the
		 * endpoint are drawn, we need to compute the line's
		 * last-but-one endpoint and set the clip rectangle to prevent
		 * drawing of the last point into the frame buffer.
		 */

		const int screen_clip_left =
			screen_state_p->generic_state.screen_clip_left;
		const int screen_clip_right =
			screen_state_p->generic_state.screen_clip_right;
		const int screen_clip_top =
			screen_state_p->generic_state.screen_clip_top;
		const int screen_clip_bottom =
			screen_state_p->generic_state.screen_clip_bottom;

		int clip_left;
		int clip_right;
		int clip_top;
		int clip_bottom;

		boolean is_clip_to_graphics_state = TRUE;
		
		/*
		 * Setup the X, Y origin to (0, 0), as we will be working with
		 * absolute coordinates.
		 */

		P9000_WRITE_PARAMETER_CONTROL_REGISTER(
			P9000_PARAMETER_CONTROL_W_OFF_XY,
			0);

		do
		{
			
			/*
			 * Compute the endpoints of the line.
			 */

			int x1 = segment_p->x1 + xorg;
			int x2 = segment_p->x2 + xorg;
			int y1 = segment_p->y1 + yorg;
			int y2 = segment_p->y2 + yorg;
			
			int x2_orig = x2;
			int y2_orig = y2;
			
			int oc1, oc2;
			
			
			OUTCODE(oc1, x1, y1, 
					screen_clip_left, screen_clip_right, 
					screen_clip_top, screen_clip_bottom);
			OUTCODE(oc2, x2, y2, 
					screen_clip_left, screen_clip_right,
					screen_clip_top, screen_clip_bottom);

			if (oc1 & oc2)
			{
				continue;
			}
			
			/*
			 * Special case horizontal and vertical lines.
			 */

			if (x1 == x2 || y1 == y2)
			{
				if (x1 == x2)
				{
					/*
					 * A vertical line.
					 */

					if (y1 < y2)
					{
						/*
						 * Top to bottom.
						 */
						
						y2_orig --;
						   
					}
					else
					{
						/*
						 * Bottom to top.
						 */
						
						y2_orig ++;

					}
					
				}
				else
				{
					/*
					 * Horizontal line.
					 */

					if (x1 < x2)
					{
						/*
						 * Left to right
						 */
						
						x2_orig --;
					}
					else
					{
						/*
						 * Right to left.
						 */

						x2_orig ++;
					}
				}
				
				/*
				 * Re synchronize to graphics state if necessary.
				 */

				if (!is_clip_to_graphics_state)
				{
					
					P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();
					P9000_REGISTER_SET_CLIPPING_RECTANGLE(
						screen_clip_left, screen_clip_top,
						screen_clip_right, screen_clip_bottom);
					is_clip_to_graphics_state = TRUE;
				}
			}
			else
			{
				/*
				 * Diagonal line.
				 */

				COMPUTE_LAST_BUT_ONE_POINT_AND_CLIP_REGISTERS(x1, y1, x2,
															  y2, x2_orig,
															  y2_orig);
			
				/*
				 * Reprogram the clip window to clip to the current clip.
				 */

				P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();

				P9000_REGISTER_SET_CLIPPING_RECTANGLE(
					clip_left, clip_top, clip_right, clip_bottom);
				
				/*
				 * Mark that the clip registers have been tampered with.
				 */

				is_clip_to_graphics_state = FALSE;
			}


			/*
			 * Write the meta coordinate registers for the line.
			 */

			P9000_WRITE_META_COORDINATE_REGISTER(
				P9000_META_COORDINATE_VTYPE_LINE,
				P9000_META_COORDINATE_ABS,
				P9000_META_COORDINATE_YX_XY,
				P9000_PACK_XY(x1, y1));
											 
			/*
			 * We need to draw to the original coordinates, to avoid
			 * upsetting the graphics engine's bresenham trajectory.
			 * The clip rectangle will have been set to ensure that
			 * the last pixel of the line is not actually drawn. 
			 */

			P9000_WRITE_META_COORDINATE_REGISTER(
				P9000_META_COORDINATE_VTYPE_LINE,
				P9000_META_COORDINATE_ABS,
				P9000_META_COORDINATE_YX_XY,
				P9000_PACK_XY(x2_orig, y2_orig));

			if (is_dashed_segment)
			{
				
				/*
				 * Program pattern origin.
				 */

				P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();
			
				P9000_WRITE_DRAWING_ENGINE_REGISTER(
					P9000_DRAWING_ENGINE_PAT_ORIGINX, 
					(x1) % P9000_DEFAULT_PATTERN_REGISTER_WIDTH);

				P9000_WRITE_DRAWING_ENGINE_REGISTER(
					P9000_DRAWING_ENGINE_PAT_ORIGINY, 
					(y1) % P9000_DEFAULT_PATTERN_REGISTER_HEIGHT);

			}
			
			/*
			 * Initiate the draw operation
			 */

			do
			{
				status = P9000_INITIATE_QUAD_COMMAND();
			} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);
			
			/*
			 * Check if the drawing needs to be re-tried.
			 */

			if (status & P9000_STATUS_QUAD_SOFTWARE)
			{
				p9000_draw_clipped_segment(x1, y1, x2_orig, y2_orig);
			}
			
		} while (++segment_p < segmentFence);
		

		/*
		 * Mark that the clip registers have been modified.
		 */

		if (!is_clip_to_graphics_state)
		{
			P9000_STATE_INVALIDATE_CLIP_RECTANGLE(screen_state_p);
		}
	}
	else						/* not isCapNotLast */
	{

		/*
		 * The graphics engine draws thin lines in concordance with
		 * the X11 protocol specification for non CapNotLast lines.
		 */

		/*
		 * Setup the X, Y origin to (xorg, yorg).
		 */

		P9000_WRITE_PARAMETER_CONTROL_REGISTER(
			P9000_PARAMETER_CONTROL_W_OFF_XY,
			P9000_PACK_XY(xorg, yorg));

		do
		{
			
			/*
			 * Write the meta coordinate registers.  x1, y1 and x2, y2.
			 */

			P9000_WRITE_META_COORDINATE_REGISTER(
				P9000_META_COORDINATE_VTYPE_LINE,
				P9000_META_COORDINATE_ABS,
				P9000_META_COORDINATE_YX_XY,
				P9000_PACK_XY(segment_p->x1, segment_p->y1));
											 
			P9000_WRITE_META_COORDINATE_REGISTER(
				P9000_META_COORDINATE_VTYPE_LINE,
				P9000_META_COORDINATE_ABS,
				P9000_META_COORDINATE_YX_XY,
				P9000_PACK_XY(segment_p->x2, segment_p->y2));
											 
			if (is_dashed_segment)
			{

				/*
				 * Reprogram the pattern origin to correspond to
				 * the start of the line.
				 */

				P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();
			
				P9000_WRITE_DRAWING_ENGINE_REGISTER(
					P9000_DRAWING_ENGINE_PAT_ORIGINX, 
					(segment_p->x1 + xorg) % 
					P9000_DEFAULT_PATTERN_REGISTER_WIDTH);

				P9000_WRITE_DRAWING_ENGINE_REGISTER(
					P9000_DRAWING_ENGINE_PAT_ORIGINY, 
					(segment_p->y1 + yorg) %
					P9000_DEFAULT_PATTERN_REGISTER_HEIGHT);

			}
			
			/*
			 * Initiate the draw operation
			 */

			do
			{
				status = P9000_INITIATE_QUAD_COMMAND();
			} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);
			
			/*
			 * Check if the drawing needs to be re-tried.
			 */

			if (status & P9000_STATUS_QUAD_SOFTWARE)
			{
				/*
				 * We have to pass down absolute coordinates
				 */

				P9000_WRITE_PARAMETER_CONTROL_REGISTER(
					P9000_PARAMETER_CONTROL_W_OFF_XY,0);

				p9000_draw_clipped_segment(segment_p->x1 + xorg,
										   segment_p->y1 + yorg,
										   segment_p->x2 + xorg,
										   segment_p->y2 + yorg);

				P9000_WRITE_PARAMETER_CONTROL_REGISTER(
					P9000_PARAMETER_CONTROL_W_OFF_XY,
					P9000_PACK_XY(xorg, yorg));
			}

		} while (++segment_p < segmentFence);
	}
}

/*
 * @doc:p9000_line_segment_one_bit_dashed_helper
 *
 * Draw a series of dashed segments with a large dash pattern.
 * We special case horizontal and vertical lines and use a generic
 * bresenham algorithm only for sloped lines.
 * 
 * We owe the MIT R5 cfb code.
 *
 * @enddoc
 */

STATIC void
p9000_line_segment_dashed_helper(
	SIint32 xorg, 
	SIint32 yorg, 
	register SISegmentP segment_p, 
	SIint32 count,
	const int isCapNotLast)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	P9000_FRAMEBUFFER_DECLARE();
	
	const int screen_clip_left =
		screen_state_p->generic_state.screen_clip_left;
	const int screen_clip_right =
		screen_state_p->generic_state.screen_clip_right;
	const int screen_clip_top =
		screen_state_p->generic_state.screen_clip_top;
	const int screen_clip_bottom =
		screen_state_p->generic_state.screen_clip_bottom;

	const SISegmentP segment_fence_p = segment_p + count;
	
	const int framebuffer_stride =
		(screen_state_p->generic_state.screen_physical_width) >> 2;
	

	/*
	 * If the line style is CapNotLast, we have to draw one less
	 * pixel at the very end.
	 */

	const int length_correction = (isCapNotLast) ? -1 : 0;

	boolean is_double_dash = 
		(graphics_state_p->generic_state.si_graphics_state.SGlinestyle
		 == SGLineDblDash);

	/*
	 * SI specified dash pattern.
	 */

	long *dash_list_p = 
		(long *) (graphics_state_p->generic_state.si_graphics_state.SGline);

	const int dash_list_length = 
		(graphics_state_p->generic_state.si_graphics_state.SGlineCNT);
	
	const unsigned int foreground_raster =
		(P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,
										  graphics_state_p) |
		 P9000_RASTER_QUAD_OVERSIZED_MODE);

	
	const unsigned int background_raster =
		(P9000_STATE_CALCULATE_BG_MINTERM(screen_state_p,
										  graphics_state_p) |
		 P9000_RASTER_QUAD_OVERSIZED_MODE);

	const int foreground_color = 
		(graphics_state_p->generic_state.si_graphics_state.SGfg);
	
	const int background_color = 
		(graphics_state_p->generic_state.si_graphics_state.SGbg);
	
	const int dash_index = 0;
	int dash_index_tmp;
	const int dash_offset = 0;
	int dash_offset_tmp;
	
	ASSERT(count >= 1);

	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGlinestyle 
		   != SGLineSolid);
	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE, graphics_state_p));
	
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_line_segment_dashed_helper)\n"
					   "{\n"
					   "\txorg = %ld\n"
					   "\tyorg = %ld\n"
					   "\tsegment_p = %p\n"
					   "\tcount = %ld\n"
					   "\tisCapNotLast = %d\n"
					   "}\n",
					   xorg, yorg, (void *) segment_p, count,
					   isCapNotLast);
	}
#endif 

	/*
	 * Working in absolute coordinates.
	 */

	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_W_OFF_XY,
		0);

	/*
	 * Null the current meta-coordinate register index.
	 */
	
	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_CINDEX,
		0);
	
	ASSERT(P9000_READ_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_CINDEX) == 0); /* check */


	do
	{
		
		int x1 = segment_p->x1 + xorg; /* line coordinates */
		int x2 = segment_p->x2 + xorg;
		int y1 = segment_p->y1 + yorg;
		int y2 = segment_p->y2 + yorg;
			
		int oc1, oc2;			/* clip outcodes */

		int adx, ady, signdx, signdy; /* bresenham terms */
		int e, e1, e2;
		
		int axis;
		int line_length;
		
		if (y1 == y2)
		{

			/*
			 * Draw a horizontal line from left to right.
			 */

			int x_left = (x1 < x2) ? x1 : x2;
			int x_right = ((x1 < x2) ? x2 : x1) + length_correction;
			
			/*
			 * The protocol recommends that we ignore segments with
			 * coincident endpoints if the linestyle is CapNotLast.
			 */

			if (x_right < x_left)
			{
				continue;
			}
			
			p9000_line_segment_dashed_horizontal_helper(
				x_left, x_right, y1, is_double_dash, 
				foreground_raster, background_raster, 
				0, dash_list_length, dash_list_p);

		}
		else if (x1 == x2)
		{
			int y_top = (y1 < y2) ? y1 : y2;
			int y_bottom = ((y1 < y2) ? y2 : y1) + length_correction;
			
			ASSERT(y_bottom >= y_top);
			
			p9000_line_segment_dashed_vertical_helper(
				y_top, y_bottom, x1, is_double_dash, 
				foreground_raster, background_raster, 
				0, dash_list_length, dash_list_p);

		}
		else
		{
			/*
			 * Draw a sloped line using the CFB code.
			 */

			/*
			 * Reset the dash parameters.
			 */
		
			dash_index_tmp = dash_index;
			dash_offset_tmp = dash_offset;
		
			/*
			 * Compute bresenham parameters.
			 */

			adx = x2 - x1;
			ady = y2 - y1;
			signdx = SIGN(adx);
			signdy = SIGN(ady);
			adx = ABS(adx);
			ady = ABS(ady);
		
			if (adx > ady)
			{
				axis = P9000_MISC_CFB_X_AXIS;
				e1 = (ady << 1);
				e2 = e1 - (adx << 1);
				e = e1 - adx;
				line_length = adx;
			}
			else
			{
				axis = P9000_MISC_CFB_Y_AXIS;
				e1 = (adx << 1);
				e2 = e1 - (ady << 1);
				e = e1 - ady;
				line_length = ady;
			}
		
			/*
			 * Check this line against the clip rectangle.
			 */

			OUTCODE(oc1, x1, y1, 
					screen_clip_left, screen_clip_right, 
					screen_clip_top, screen_clip_bottom);
			OUTCODE(oc2, x2, y2, 
					screen_clip_left, screen_clip_right,
					screen_clip_top, screen_clip_bottom);

			if (oc1 & oc2)
			{
				/*
				 * Out of clip box.
				 */

				continue;
			}
			else if (oc1 | oc2)
			{

				/*
				 * Need to clip the line to the current clip coordinates.
				 */

				int err;		/* modified bresenham error term */
				int clipdx, clipdy;	/* clipped distances */
			
				SISegment clipped_segment;
			
				if (p9000_line_segment_clip_segment(
						x1, y1, x2, y2, screen_clip_left, screen_clip_top,
						screen_clip_right, screen_clip_bottom,
						&clipped_segment) == NULL)
				{
					continue;
				}
			
				/*
				 * Step the dash around to the clipped line.
				 */

				dash_index_tmp = dash_index;
				dash_offset_tmp = dash_offset;
			
				if (oc1)
				{
					int skip_length;
				
					if (axis == P9000_MISC_CFB_X_AXIS)
					{
						skip_length = ABS(x1 - clipped_segment.x1);
					}
					else
					{
						skip_length = ABS(y1 - clipped_segment.y1);
					}
				
					p9000_miStepDash(skip_length, &dash_index_tmp, dash_list_p,
									 dash_list_length, &dash_offset_tmp);
				}
			
				if (axis == P9000_MISC_CFB_X_AXIS)
				{
					line_length = ABS(clipped_segment.x1 -
									  clipped_segment.x2);
				}
				else
				{
					line_length = ABS(clipped_segment.y1 -
									  clipped_segment.y2);
				}
			
				if (oc2 != 0 || !isCapNotLast)
				{
					line_length++;
				}

				if (line_length)
				{
					if (oc1)
					{
						clipdx = ABS(clipped_segment.x1 - x1);
						clipdy = ABS(clipped_segment.y1 - y1);

						if (axis == P9000_MISC_CFB_X_AXIS)
						{
							err = e + ((clipdy * e2) + 
									   ((clipdx - clipdy) * e1));
						}
						else
						{
							err = e + ((clipdx * e2) +
									   ((clipdy - clipdx) * e1));
						}
					}
					else
					{
						err = e;
					}
				
					/*
					 * Wait for the graphics engine to become idle.
					 */

					P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();
	
					p9000_cfbBresD(&dash_index_tmp, dash_list_p,
								   dash_list_length, &dash_offset_tmp,
								   is_double_dash, (void *) p9000_framebuffer_p, 
								   framebuffer_stride, signdx, signdy, axis,
								   clipped_segment.x1, clipped_segment.y1, err,
								   e1, e2, line_length, foreground_color, 
								   background_color);
				}
			}
			else
			{
				/*
				 * No need to clip, can directly draw line into the
				 * framebuffer.
				 */

				if (!isCapNotLast)
				{
					line_length++;
				}

				dash_index_tmp = dash_index;
				dash_offset_tmp = dash_offset;
			
				/*
				 * Wait for the graphics engine to become idle.
				 */

				P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();
	
				p9000_cfbBresD(&dash_index_tmp, dash_list_p, 
							   dash_list_length, &dash_offset_tmp, 
							   is_double_dash, (void *) p9000_framebuffer_p, 
							   framebuffer_stride, signdx, signdy, 
							   axis, x1, y1, e, e1, e2, line_length, 
							   foreground_color, background_color);
			}
		
		}

#ifdef DELETE
		if (y1 == y2)
		{

			/*
			 * Draw a horizontal line from left to right.
			 */

			int x_left = (x1 < x2) ? x1 : x2;
			int x_right = ((x1 < x2) ? x2 : x1) + length_correction;
			
			/*
			 * The protocol recommends that we ignore segments with
			 * coincident endpoints if the linestyle is CapNotLast.
			 */

			if (x_right < x_left)
			{
				continue;
			}
			
			p9000_line_segment_dashed_horizontal_helper(
				x_left, x_right, y1, is_double_dash, 
				foreground_raster, background_raster, 
				0, dash_length, dash_pattern_p, &current_dash);

		}
		else if (x1 == x2)
		{
			int y_top = (y1 < y2) ? y1 : y2;
			int y_bottom = ((y1 < y2) ? y2 : y1) + length_correction;
			
			ASSERT(y_bottom >= y_top);
			
			p9000_line_segment_dashed_vertical_helper(
				y_top, y_bottom, x1, is_double_dash, 
				foreground_raster, background_raster, 
				0, dash_length, dash_pattern_p, &current_dash);

		}
		else
		{

			P9000_FRAMEBUFFER_DECLARE();
			
			/*
			 * Compute bresenham parameters.
			 */

			P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();
			
			p9000_cfbBresD(&current_dash, dash_list_p, dash_list_length,
				&dash_offset, is_double_dash, p9000_framebuffer_p,
				(screen_state_p->generic_state.screen_physical_width >> 2),
				signdx, signdx, axis, x1, y1, e, e1, e2, line_length,
				foreground_color, background_color);

		}
#endif

	} while (++segment_p  < segment_fence_p);

}

/*
 * @doc:p9000_line_one_bit_helper_rop:
 * 
 * Draws a series of connected lines.  Since these lines have to join
 * correctly at their endpoints, we need to draw the intermediate
 * lines as if a cap style of CapNotLast had been specified for them.
 *
 * This means that for all line segments other than the last segment,
 * we need to avoid drawing the very last point on the line.  The last
 * line segment of this polyline is treated according the value of the
 * parameter `isCapNotLast', if not set, a Cap{Butt,Projecting} line
 * is drawn by explicitly drawing the end point of the line segment.
 * 
 * @endoc
 */

STATIC void
p9000_line_one_bit_helper_rop(SIint32 xorg, 
						  SIint32 yorg, 
						  register SIPointP points_p, 
						  SIint32 count,
						  const SIint32 coordMode,
						  const SIint32 isCapNotLast)

{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();


	register const SIPointP pointsFence = points_p + count;
	register int current_x, current_y;
	register int previous_x;
	register int previous_y;

	int status;
	
	const int screen_clip_left =
		screen_state_p->generic_state.screen_clip_left;
	const int screen_clip_right =
		screen_state_p->generic_state.screen_clip_right;
	const int screen_clip_top =
		screen_state_p->generic_state.screen_clip_top;
	const int screen_clip_bottom =
		screen_state_p->generic_state.screen_clip_bottom;

	int clip_left;
	int clip_right;
	int clip_top;
	int clip_bottom;
		
	ASSERT(count > 1);
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					  "(p9000_line_one_bit_helper_rop)\n"
					  "{\n"
					  "\txorg = %ld\n"
					  "\tyorg = %ld\n"
					  "\tpoints_p = 0x%p\n"
					  "\tcount = %ld\n"
					  "\tcoordMode = %ld\n"
					  "\tisCapNotLast = %d\n"
					  "}\n",
					  xorg, yorg, (void *) points_p, count,
					  coordMode, isCapNotLast);
	}
#endif

	/*
	 * Setup the X, Y origin to (0, 0), as we will be working with
	 * absolute coordinates.
	 */

	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_W_OFF_XY,
		0);

	/*
	 * Null the current meta-coordinate register index.
	 */
	
	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_CINDEX,
		0);
	
	ASSERT(P9000_READ_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_CINDEX) == 0); /* check */


	/*
	 * Onto drawing the lines.  Get the current point.
	 */

	current_x = points_p->x + xorg;
	current_y = points_p->y + yorg;

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
	{
		(void) fprintf(debug_stream_p,
					   "\t{\n"
					   "\t\t(%d, %d)\n",
					   current_x, current_y);
	}
#endif

	/*
	 * Write the meta coordinate registers for the first point.
	 */

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_X,
		current_x);

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_Y,
		current_y);

	/*
	 * Move onto the next point of the polyline.
	 */
	
	points_p++;
	
	switch(coordMode)
	{
	case SICoordModePrevious:

		do
		{
			int end_x;			/* endpoints of the line */
			int end_y;
			
			/*
			 * Save the previous point
			 */

			previous_x = current_x;
			previous_y = current_y;
			
			/*
			 * Get the next point.
			 */

			end_x = (current_x += points_p->x);
			end_y = (current_y += points_p->y);
			
			COMPUTE_LAST_BUT_ONE_POINT_AND_CLIP_REGISTERS(previous_x,
				  previous_y, end_x, end_y, current_x, current_y);
			
			/*
			 * Reprogram the clip window to clip to the current clip.
			 */

			P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();

 			P9000_REGISTER_SET_CLIPPING_RECTANGLE(
				clip_left, clip_top, clip_right, clip_bottom);
			
			/*
			 * Write the meta coordinate registers for the current
			 * point. 
			 */

			P9000_WRITE_META_COORDINATE_REGISTER(
				P9000_META_COORDINATE_VTYPE_LINE, 
				P9000_META_COORDINATE_ABS,
				P9000_META_COORDINATE_YX_X,
				current_x);												 

			P9000_WRITE_META_COORDINATE_REGISTER(
				P9000_META_COORDINATE_VTYPE_LINE, 
				P9000_META_COORDINATE_ABS,
				P9000_META_COORDINATE_YX_Y,
				current_y);												 
											 
#if (defined(__DEBUG__))
			if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
			{
				(void) fprintf(debug_stream_p,
							   "\t\t(%d, %d)\n",
							   current_x, current_y);
			}
#endif

			/*
			 * Initiate the draw operation
			 */

			do
			{
				status = P9000_INITIATE_QUAD_COMMAND();
			} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);
			

			/*
			 * Check if the drawing needs to be re-tried.
			 */

			if (status & P9000_STATUS_QUAD_SOFTWARE)
			{
				p9000_draw_clipped_segment(previous_x, previous_y,
										current_x, current_y);

				P9000_WRITE_META_COORDINATE_REGISTER(
					P9000_META_COORDINATE_VTYPE_LINE, 
					P9000_META_COORDINATE_ABS,
					P9000_META_COORDINATE_YX_X,
					current_x);												 

				P9000_WRITE_META_COORDINATE_REGISTER(
					P9000_META_COORDINATE_VTYPE_LINE, 
					P9000_META_COORDINATE_ABS,
					P9000_META_COORDINATE_YX_Y,
					current_y);												 
			}
			
			
		} while (++points_p < pointsFence);
		

		break;
		
	case SICoordModeOrigin:

		/*
		 * Pump the points down to the graphics engine, one by one.
		 */

		do
		{
			int end_x;			/* end points of the line */
			int end_y;
			
			/*
			 * save the previous coordinates.
			 */

			previous_x = current_x;
			previous_y = current_y;
			
			/*
			 * Get the current end points.
			 */

			end_x = (current_x = (points_p->x + xorg));
			end_y = (current_y = (points_p->y + yorg));
			
			/*
			 * Compute the next to last point of the line and set the
			 * clip registers to not draw the last point.
			 */

			COMPUTE_LAST_BUT_ONE_POINT_AND_CLIP_REGISTERS(previous_x,
				  previous_y, end_x, end_y, current_x, current_y);
			
			/*
			 * Reprogram the clip window to clip to the current clip.
			 */

			P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();

 			P9000_REGISTER_SET_CLIPPING_RECTANGLE(
				clip_left, clip_top, clip_right, clip_bottom);

			/*
			 * Write the meta coordinate registers for the current
			 * point. 
			 */

			P9000_WRITE_META_COORDINATE_REGISTER(
				P9000_META_COORDINATE_VTYPE_LINE, 
				P9000_META_COORDINATE_ABS,
				P9000_META_COORDINATE_YX_X,
				current_x);

			P9000_WRITE_META_COORDINATE_REGISTER(
				P9000_META_COORDINATE_VTYPE_LINE, 
				P9000_META_COORDINATE_ABS,
				P9000_META_COORDINATE_YX_Y,
				current_y);
											 
#if (defined(__DEBUG__))
			if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
			{
				(void) fprintf(debug_stream_p,
							   "\t\t(%d, %d)\n",
							   current_x, current_y);
			}
#endif

			/*
			 * Initiate the draw operation
			 */

			do
			{
				status = P9000_INITIATE_QUAD_COMMAND();
			} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);
			
			/*
			 * Check if the drawing needs to be re-tried.
			 */

			if (status & P9000_STATUS_QUAD_SOFTWARE)
			{

				/*
				 * Try and redraw the line.
				 */

				p9000_draw_clipped_segment(previous_x, previous_y, 
										   current_x, current_y); 

				P9000_WRITE_META_COORDINATE_REGISTER(
					P9000_META_COORDINATE_VTYPE_LINE, 
					P9000_META_COORDINATE_ABS,
					P9000_META_COORDINATE_YX_X,
					current_x);												 

				P9000_WRITE_META_COORDINATE_REGISTER(
					P9000_META_COORDINATE_VTYPE_LINE, 
					P9000_META_COORDINATE_ABS,
					P9000_META_COORDINATE_YX_Y,
					current_y);												 
			}
			

		} while (++points_p < pointsFence);

		break;
		
	default:
		/*CONSTANTCONDITION*/
		ASSERT(0);
		break;
	}
	
	/*
	 * If the line style is not CapNotLast, then draw the last
	 * point again. 
	 */

	if (!isCapNotLast)
	{

		/*
		 * Draw a single point at the end of the line again.
		 */

			
#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
		{
			(void) fprintf(debug_stream_p, 
						   "\t\tPoint: (%d, %d)\n",
						   current_x, current_y);
		}
#endif
			
		/*
		 * Draw the last line.
		 */

		P9000_WRITE_META_COORDINATE_REGISTER(
			 P9000_META_COORDINATE_VTYPE_POINT,
			 P9000_META_COORDINATE_ABS,
			 P9000_META_COORDINATE_YX_XY,
			 P9000_PACK_XY(current_x, current_y));
			
											 
		/*
		 * Initiate the draw operation for the point.  We don't check
		 * the return code for the operation as an out of coordinate
		 * point is in any case not visible.
		 */

		do
		{
			status = P9000_INITIATE_QUAD_COMMAND();
		} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);
			
	}
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
	{
		(void) fprintf(debug_stream_p, "\t}\n");
	}
#endif

}


/*
 * @doc:p9000_line_one_bit_helper_gxcopy:
 *
 * Draw as series of connected one-bit wide lines with a rop of
 * GXcopy.  Since we are working in GXcopy mode, we can safely ignore
 * the line joins, as the worst that would happen is that the graphics
 * engine would plot the endpoints twice.  Since drawing a CapNotLast
 * line involves considerable computation and a wait for the graphics
 * engine to stop drawing lines, this is a win on the speed of drawing.
 * 
 * The last line of the polyline is drawn as specified by the
 * CapNotLast parameter.
 * 
 * @enddoc
 */

STATIC void
p9000_line_one_bit_helper_gxcopy(
	SIint32 xorg, 
	SIint32 yorg, 
	register SIPointP points_p, 
	SIint32 count,
	const SIint32 coordMode,
	const SIint32 isCapNotLast)
{

	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
#if (defined(__DEBUG__))
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	const SIPointP last_points_p = points_p + count;
#endif 

	register const SIPointP pointsFence = points_p + (count - 1);
	register int current_x;		/* ending points of a line */
	register int current_y; 
	register int previous_x;	/* starting points of a line */
	register int previous_y;
	int end_x;					/* end points of the last line */
	int end_y;
	
	int status;
	
	const int screen_clip_left =
		screen_state_p->generic_state.screen_clip_left;
	const int screen_clip_right =
		screen_state_p->generic_state.screen_clip_right;
	const int screen_clip_top =
		screen_state_p->generic_state.screen_clip_top;
	const int screen_clip_bottom =
		screen_state_p->generic_state.screen_clip_bottom;

	int clip_left;
	int clip_right;
	int clip_top;
	int clip_bottom;
		
	ASSERT(count > 1);
	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGmode ==
		   GXcopy);
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					  "(p9000_line_one_bit_helper)\n"
					  "{\n"
					  "\txorg = %ld\n"
					  "\tyorg = %ld\n"
					  "\tpoints_p = 0x%p\n"
					  "\tcount = %ld\n"
					  "\tcoordMode = %ld\n"
					  "\tisCapNotLast = %d\n"
					  "}\n",
					  xorg, yorg, (void *) points_p, count,
					  coordMode, isCapNotLast);
	}
#endif

	/*
	 * Setup the X, Y origin.
	 */

	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_W_OFF_XY,
		P9000_PACK_XY(xorg, yorg));

	/*
	 * Null the current meta-coordinate register index.
	 */
	
	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_CINDEX,
		0);
	
	ASSERT(P9000_READ_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_CINDEX) == 0); /* check */


	/*
	 * Onto drawing the lines.  Get the line starting points.
	 */

	current_x = previous_x = points_p->x;
	current_y = previous_y = points_p->y;

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
	{
		(void) fprintf(debug_stream_p,
					   "\t{\n"
					   "\t\t(%d, %d)\n",
					   current_x, current_y);
	}
#endif

	/*
	 * Write the meta coordinate registers for the first point.
	 */

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_X,
		current_x);

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_Y,
		current_y);

	/*
	 * Move onto the next point of the polyline.
	 */
	
	points_p++;
	
	if (count > 2)				/* more than 2 lines */
	{
		/*
		 * Do all the intermediate lines upto but excluding the last line.
		 */

		switch(coordMode)
		{
		case SICoordModePrevious:

			do
			{
				/*
				 * Save the previous point
				 */

				previous_x = current_x;
				previous_y = current_y;
			
				/*
				 * Get the next point.
				 */

				current_x += points_p->x;
				current_y += points_p->y;
			
				/*
				 * Write the meta coordinate registers for the current
				 * point. 
				 */

				P9000_WRITE_META_COORDINATE_REGISTER(
					 P9000_META_COORDINATE_VTYPE_LINE, 
					 P9000_META_COORDINATE_ABS,
					 P9000_META_COORDINATE_YX_X,
					 current_x);												 

				P9000_WRITE_META_COORDINATE_REGISTER(
					 P9000_META_COORDINATE_VTYPE_LINE, 
					 P9000_META_COORDINATE_ABS,
					 P9000_META_COORDINATE_YX_Y,
					 current_y);												 
											 
#if (defined(__DEBUG__))
				if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
				{
					(void) fprintf(debug_stream_p,
								   "\t\t(%d, %d)\n",
								   current_x, current_y);
				}
#endif

				/*
				 * Initiate the draw operation
				 */

				do
				{
					status = P9000_INITIATE_QUAD_COMMAND();
				} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);
			

				/*
				 * Check if the drawing needs to be re-tried.
				 */

				if (status & P9000_STATUS_QUAD_SOFTWARE)
				{

					P9000_WRITE_PARAMETER_CONTROL_REGISTER(
						P9000_PARAMETER_CONTROL_W_OFF_XY,
						0);

					p9000_draw_clipped_segment(
						previous_x + xorg,
						previous_y + yorg,
						current_x + xorg,
						current_y + yorg);

					P9000_WRITE_PARAMETER_CONTROL_REGISTER(
						P9000_PARAMETER_CONTROL_W_OFF_XY,
						P9000_PACK_XY(xorg, yorg));

					P9000_WRITE_META_COORDINATE_REGISTER(
						 P9000_META_COORDINATE_VTYPE_LINE, 
						 P9000_META_COORDINATE_ABS,
						 P9000_META_COORDINATE_YX_X,
						 current_x);												 

					P9000_WRITE_META_COORDINATE_REGISTER(
						 P9000_META_COORDINATE_VTYPE_LINE, 
						 P9000_META_COORDINATE_ABS,
						 P9000_META_COORDINATE_YX_Y,
						 current_y);												 
				}
			
			
			} while (++points_p < pointsFence);
		

			/*
			 * Save the current endpoints as absolute coordinates.
			 */

			previous_x = current_x + xorg;
			previous_y = current_y + yorg;

			break;
		
		case SICoordModeOrigin:

			/*
			 * Pump the points down to the graphics engine, one by one.
			 */

			do
			{
				/*
				 * Get the current end points.
				 */

				current_x = points_p->x;
				current_y = points_p->y;
			
				/*
				 * Write the meta coordinate registers for the current
				 * point. 
				 */

				P9000_WRITE_META_COORDINATE_REGISTER(
					 P9000_META_COORDINATE_VTYPE_LINE, 
					 P9000_META_COORDINATE_ABS,
					 P9000_META_COORDINATE_YX_X,
					 current_x);

				P9000_WRITE_META_COORDINATE_REGISTER(
					 P9000_META_COORDINATE_VTYPE_LINE, 
					 P9000_META_COORDINATE_ABS,
					 P9000_META_COORDINATE_YX_Y,
					 current_y);
											 
#if (defined(__DEBUG__))
				if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
				{
					(void) fprintf(debug_stream_p,
								   "\t\t(%d, %d)\n",
								   current_x, current_y);
				}
#endif

				/*
				 * Initiate the draw operation
				 */

				do
				{
					status = P9000_INITIATE_QUAD_COMMAND();
				} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);
			
				/*
				 * Check if the drawing needs to be re-tried.
				 */

				if (status & P9000_STATUS_QUAD_SOFTWARE)
				{

					P9000_WRITE_PARAMETER_CONTROL_REGISTER(
						P9000_PARAMETER_CONTROL_W_OFF_XY,
						0);

					/*
					 * Try and redraw the line.
					 */

					p9000_draw_clipped_segment((points_p - 1)->x + xorg,
						(points_p - 1)->y + yorg,
						current_x + xorg,
						current_y + yorg);

					P9000_WRITE_PARAMETER_CONTROL_REGISTER(
						P9000_PARAMETER_CONTROL_W_OFF_XY,
						P9000_PACK_XY(xorg, yorg));

					P9000_WRITE_META_COORDINATE_REGISTER(
						 P9000_META_COORDINATE_VTYPE_LINE, 
						 P9000_META_COORDINATE_ABS,
						 P9000_META_COORDINATE_YX_X,
						 current_x);												 

					P9000_WRITE_META_COORDINATE_REGISTER(
						 P9000_META_COORDINATE_VTYPE_LINE, 
						 P9000_META_COORDINATE_ABS,
						 P9000_META_COORDINATE_YX_Y,
						 current_y);												 
				}
			

			} while (++points_p < pointsFence);
		
			/*
			 * Save the current endpoints, and convert them to
			 * absolute coordinates.
			 */

			previous_x = current_x + xorg;
			previous_y = current_y + yorg;
			
			break;
		
		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
		}
	}


	/*
	 * Draw the last line.  Get the end points.
	 */

	if (coordMode == SICoordModeOrigin)
	{
		end_x = current_x = points_p->x + xorg;
		end_y = current_y = points_p->y + yorg;
	}
	else						/* CoordModePrevious */
	{
		end_x = current_x = (previous_x + points_p->x);
		end_y = current_y = (previous_y + points_p->y);
	}
	
	if (isCapNotLast)
	{

		/*
		 * Don't draw the last point of this line.
		 */

		COMPUTE_LAST_BUT_ONE_POINT_AND_CLIP_REGISTERS((previous_x + xorg), 
			(previous_y + yorg), end_x, end_y, current_x, current_y);

		
		/*
		 * Reprogram the clip window to clip to the current clip.
		 */

		P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();

		P9000_REGISTER_SET_CLIPPING_RECTANGLE(
				clip_left, clip_top, clip_right, clip_bottom);
			
		P9000_STATE_INVALIDATE_CLIP_RECTANGLE(screen_state_p);
		
	}
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
	{
		(void) fprintf(debug_stream_p, 
					   "\t\tLast line to: (%d, %d)\n",
					   current_x, current_y);
	}
#endif
			
	/*
	 * Setup the X, Y origin to be (0, 0), as we are working with
	 * absolute screen coordinates.
	 */

	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_W_OFF_XY,
		0);

	/*
	 * Draw the last line.
	 */

	P9000_WRITE_META_COORDINATE_REGISTER(
		 P9000_META_COORDINATE_VTYPE_LINE, 
		 P9000_META_COORDINATE_ABS,
		 P9000_META_COORDINATE_YX_X,
		 current_x);												 

	P9000_WRITE_META_COORDINATE_REGISTER(
		 P9000_META_COORDINATE_VTYPE_LINE, 
		 P9000_META_COORDINATE_ABS,
		 P9000_META_COORDINATE_YX_Y,
		 current_y);												 
											 
	/*
	 * Initiate the draw operation for the last line.
	 */

	do
	{
		status = P9000_INITIATE_QUAD_COMMAND();
	} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);
			

	/*
	 * Redraw the line if out of device coordinate space.
	 */

	if (status & P9000_STATUS_QUAD_SOFTWARE)
	{
		p9000_draw_clipped_segment(previous_x, previous_y,
								current_x, current_y);
	}
	
	ASSERT(++points_p == last_points_p);
	

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
	{
		(void) fprintf(debug_stream_p, "\t}\n");
	}
#endif

	
}

/*
 * @doc:p9000_line_one_bit_helper_dashed:
 *
 * 
 * @enddoc
 */

STATIC void
p9000_line_one_bit_helper_dashed(
	SIint32 xorg, 
	SIint32 yorg, 
	register SIPointP points_p, 
	SIint32 count,
	const SIint32 coordMode,
	const SIint32 isCapNotLast)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	
	P9000_MEMORY_BASE_DECLARE();
	P9000_FRAMEBUFFER_DECLARE();
	
	const int screen_clip_left =
		screen_state_p->generic_state.screen_clip_left;
	const int screen_clip_right =
		screen_state_p->generic_state.screen_clip_right;
	const int screen_clip_top =
		screen_state_p->generic_state.screen_clip_top;
	const int screen_clip_bottom =
		screen_state_p->generic_state.screen_clip_bottom;

	const int framebuffer_stride =
		(screen_state_p->generic_state.screen_physical_width) >> 2;
	
	boolean is_double_dash = 
		(graphics_state_p->generic_state.si_graphics_state.SGlinestyle
		 == SGLineDblDash);

	/*
	 * SI specified dash pattern.
	 */

	long *dash_list_p = 
		(long *) (graphics_state_p->generic_state.si_graphics_state.SGline);

	const int dash_list_length = 
		(graphics_state_p->generic_state.si_graphics_state.SGlineCNT);
	
	const int foreground_color = 
		(graphics_state_p->generic_state.si_graphics_state.SGfg);
	
	const int background_color = 
		(graphics_state_p->generic_state.si_graphics_state.SGbg);
	
	int dash_index = 0;
	int dash_index_tmp;
	int dash_offset = 0;
	int dash_offset_tmp;
	
	int tmp_count = count;

	int x1, y1, x2, y2;			/* line endpoints */
	int x1_orig, y1_orig;
	

	ASSERT(count >= 1);

	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGlinestyle 
		   != SGLineSolid);
	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE, graphics_state_p));
	
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_line_one_bit_helper_dashed)\n"
					   "{\n"
					   "\txorg = %ld\n"
					   "\tyorg = %ld\n"
					   "\tpoints_p = %p\n"
					   "\tcount = %ld\n"
					   "\tcoordMode = %ld\n"
					   "\tisCapNotLast = %d\n"
					   "}\n",
					   xorg, yorg, (void *) points_p, count, coordMode,
					   isCapNotLast);
	}
#endif 

	x1_orig = (x2 = points_p->x + xorg);
	y1_orig = (y2 = points_p->y + yorg);
	
	/*
	 * Wait for the graphics engine to become idle.
	 */

	P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();
	
	while (--tmp_count)
	{
		
		int oc1, oc2;			/* clip outcodes */

		int adx, ady, signdx, signdy; /* bresenham terms */
		int e, e1, e2;
		
		int axis;
		int line_length;
		int unclipped_length;

		x1 = x2;
		y1 = y2;
		points_p ++;
		
		if (coordMode == SICoordModePrevious)
		{
			xorg = x1;
			yorg = y1;
		}
		
		x2 = points_p->x + xorg;
		y2 = points_p->y + yorg;
		
		adx = x2 - x1;
		ady = y2 - y1;
		signdx = SIGN(adx);
		signdy = SIGN(ady);
		adx = ABS(adx);
		ady = ABS(ady);
		
		if (adx > ady)
		{
			axis = P9000_MISC_CFB_X_AXIS;
			e1 = (ady << 1);
			e2 = e1 - (adx << 1);
			e = e1 - adx;
			unclipped_length = adx;
		}
		else
		{
			axis = P9000_MISC_CFB_Y_AXIS;
			e1 = (adx << 1);
			e2 = e1 - (ady << 1);
			e = e1 - ady;
			unclipped_length = ady;
		}
		
		/*
		 * Check this line against the clip rectangle.
		 */

		OUTCODE(oc1, x1, y1, 
				screen_clip_left, screen_clip_right, 
				screen_clip_top, screen_clip_bottom);
		OUTCODE(oc2, x2, y2, 
				screen_clip_left, screen_clip_right,
				screen_clip_top, screen_clip_bottom);

		if (oc1 & oc2)
		{
			/*
			 * Out of clip box.
			 */

			continue;
		}
		else if (oc1 | oc2)
		{

			/*
			 * Need to clip the line to the current clip coordinates.
			 */

			int err;			/* modified bresenham error term */
			int clipdx, clipdy;	/* clipped distances */
			
			SISegment clipped_segment;
			
			if (p9000_line_segment_clip_segment(
				x1, y1, x2, y2, screen_clip_left, screen_clip_top,
				screen_clip_right, screen_clip_bottom,
				&clipped_segment) == NULL)
			{
				continue;
			}
			
			/*
			 * Step the dash around to the clipped line.
			 */

			dash_index_tmp = dash_index;
			dash_offset_tmp = dash_offset;
				
			if (oc1)
			{
				int skip_length;

				if (axis == P9000_MISC_CFB_X_AXIS)
				{
					skip_length = ABS(x1 - clipped_segment.x1);
				}
				else
				{
					skip_length = ABS(y1 - clipped_segment.y1);
				}
				
				p9000_miStepDash(skip_length, &dash_index_tmp, dash_list_p,
						   dash_list_length, &dash_offset_tmp);
			}
			
			if (axis == P9000_MISC_CFB_X_AXIS)
			{
				line_length = ABS(clipped_segment.x1 -
								  clipped_segment.x2);
			}
			else
			{
				line_length = ABS(clipped_segment.y1 -
								  clipped_segment.y2);
			}
			
			line_length += (oc2 != 0);

			if (line_length)
			{
				if (oc1)
				{
					clipdx = ABS(clipped_segment.x1 - x1);
					clipdy = ABS(clipped_segment.y1 - y1);

					if (axis == P9000_MISC_CFB_X_AXIS)
					{
						err = e + ((clipdy * e2) + 
								   ((clipdx - clipdy) * e1));
					}
					else
					{
						err = e + ((clipdx * e2) +
								   ((clipdy - clipdx) * e1));
					}
				}
				else
				{
					err = e;
				}
				
				p9000_cfbBresD(&dash_index_tmp, dash_list_p,
							   dash_list_length, &dash_offset_tmp,
							   is_double_dash, (void *) p9000_framebuffer_p, 
							   framebuffer_stride, signdx, signdy, axis,
							   clipped_segment.x1, clipped_segment.y1, err,
							   e1, e2, line_length, foreground_color, 
							   background_color);
			}
		}
		else
		{
			/*
			 * No need to clip, can directly draw line into the
			 * framebuffer.
			 */

			p9000_cfbBresD(&dash_index, dash_list_p, 
						   dash_list_length, &dash_offset, 
						   is_double_dash, (void *) p9000_framebuffer_p, 
						   framebuffer_stride, signdx, signdy, 
						   axis, x1, y1, e, e1, e2, unclipped_length,
						   foreground_color, background_color);

			goto dont_step;

		}

		/*
		 * Walk the dash list to the next line.
		 */
		
		p9000_miStepDash(unclipped_length, &dash_index, dash_list_p, 
						 dash_list_length, &dash_offset);
	
	dont_step:
		
		;
		
	}
	
	/*
	 * Draw the last point.
	 */
	
	if (!isCapNotLast &&
		((dash_index & 1) == 0 || is_double_dash) &&
		((points_p->x + xorg != x1_orig) ||
		 (points_p->x + yorg != y1_orig) ||
		 (count == 2)))
	{
		if ((x2 >= screen_clip_left) &&
			(y2 >= screen_clip_top) &&
			(x2 < screen_clip_right) &&
			(y2 < screen_clip_bottom))
		{
			*(p9000_framebuffer_p +
			  ((y2 * (framebuffer_stride << 2)) + x2)) =
				  (unsigned char) 
					  ((dash_index & 1) ? background_color :
					   foreground_color);
		}
		
	}
}


/*
 * @doc:p9000_line_segment_one_bit_solid:
 *
 * Segment draw code in solid fill mode for the P9000 display
 * library.
 * 
 * @enddoc
 */

STATIC SIBool
p9000_line_segment_one_bit_solid(SIint32 xorg, SIint32 yorg, SIint32 count,
		  SISegmentP psegsIn, SIint32 isCapNotLast)
{
	unsigned int raster;
	
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();

	P9000_CURRENT_GRAPHICS_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
			 (struct generic_screen_state *) generic_screen_current_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_one_bit_segment_solid)\n"
					   "{\n"
					   "\txorg = %ld\n"
					   "\tyorg = %ld\n"
					   "\tcount = %ld\n"
					   "\tpsegsIn = %p\n"
					   "\tisCapNotLast = %ld\n"
					   "}\n",
					   xorg, yorg, count, (void *) psegsIn,
					   isCapNotLast);
	}
#endif

#if (defined(__DEBUG__))
	ASSERT(graphics_state_p->
		   generic_state.si_graphics_state.SGlinestyle == SGLineSolid);
#endif

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	if (!P9000_REGISTER_IS_VALID_ORIGIN(xorg,yorg))
	{
		return SI_FAIL;
	}

	/*
	 * We need to switch the drawing engine into oversized mode in
	 * order to draw zero-width lines.  Program the raster bit
	 * accordingly.
	 */

	raster = 
		P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,graphics_state_p);

	P9000_STATE_SET_RASTER(register_state_p,
		raster | P9000_RASTER_QUAD_OVERSIZED_MODE);

	/*
	 * Set the clip rectangle to the graphics state.
	 */

	P9000_STATE_SET_CLIP_RECTANGLE_TO_SI_CLIP(screen_state_p);

	/*
	 * Synchronize other registers with SI specified values.
	 */

	P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
		(P9000_STATE_CHANGE_FOREGROUND_COLOR |
		 P9000_STATE_CHANGE_PLANEMASK));

	P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
		(P9000_STATE_CHANGE_FOREGROUND_COLOR |
		 P9000_STATE_CHANGE_PLANEMASK));

	/*
	 * Call the the helper. `isCapNotLast' applies for all the
	 * segments here. 
	 */

	p9000_line_segment_one_bit_helper(xorg, yorg, psegsIn, 
									  count, isCapNotLast, FALSE);

	return (SI_SUCCEED);
}


/*
 * @doc:p9000_line_segment_one_bit_dashed:
 *
 * Segment draw code in solid fill mode for the P9000 display
 * library.
 *
 * @endoc
 */

STATIC SIBool
p9000_line_segment_one_bit_dashed(SIint32 xorg, SIint32 yorg, SIint32 count,
		  SISegmentP psegsIn, SIint32 isCapNotLast)
{
	unsigned int raster;
	
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();

	P9000_CURRENT_GRAPHICS_STATE_DECLARE();

	struct p9000_line_state *line_state_p = 
		graphics_state_p->line_state_p;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
			 (struct generic_screen_state *) generic_screen_current_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE, 
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_LINE_STATE, 
							 (struct p9000_line_state *) line_state_p));
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_one_bit_segment_dashed)\n"
					   "{\n"
					   "\txorg = %ld\n"
					   "\tyorg = %ld\n"
					   "\tcount = %ld\n"
					   "\tpsegsIn = %p\n"
					   "\tisCapNotLast = %ld\n"
					   "}\n",
					   xorg, yorg, count, (void *) psegsIn,
					   isCapNotLast);
	}
#endif

#if (defined(__DEBUG__))
	ASSERT((graphics_state_p->
			generic_state.si_graphics_state.SGlinestyle ==
			SGLineDash) ||
		   (graphics_state_p->
			generic_state.si_graphics_state.SGlinestyle == SGLineDblDash));
#endif

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	if (!P9000_REGISTER_IS_VALID_ORIGIN(xorg,yorg))
	{
		return SI_FAIL;
	}

	/*
	 * Set the clip rectangle to the graphics state.
	 */

	P9000_STATE_SET_CLIP_RECTANGLE_TO_SI_CLIP(screen_state_p);

	/*
	 * Synchronize the registers to the SI specified values.
	 */

	P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
		(P9000_STATE_CHANGE_FOREGROUND_COLOR |
		 P9000_STATE_CHANGE_BACKGROUND_COLOR |
		 P9000_STATE_CHANGE_PLANEMASK));

	P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
		(P9000_STATE_CHANGE_FOREGROUND_COLOR |
		 P9000_STATE_CHANGE_BACKGROUND_COLOR |
		 P9000_STATE_CHANGE_PLANEMASK));
		
	if (line_state_p->is_small_dash_pattern == TRUE)
	{
		
		/*
		 * Reload the pattern registers if necessary.
		 */

		P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();
		
		if (register_state_p->pattern_registers_p !=
			line_state_p->pattern_registers)
		{
			int i;
		
			const unsigned int swap_flag = 
				P9000_ADDRESS_SWAP_HALF_WORDS |
				P9000_ADDRESS_SWAP_BYTES |
				P9000_ADDRESS_SWAP_BITS;

			/*
			 * Fill the graphics engine's pattern registers with
			 * the line pattern.
			 */
		
#if (defined(__DEBUG__))
			if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
			{
				(void) fprintf(debug_stream_p, 
							   "\treprogramming pattern:\n");
			}
#endif
						   
			for (i = 0; i < P9000_DEFAULT_PATTERN_REGISTER_COUNT; i++)
			{

#if (defined(__DEBUG__))
				if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
				{
					(void) fprintf(debug_stream_p, 
								   "\t\t[%d] 0x%x -> ", 
								   i, 
								   line_state_p->pattern_registers[i]);
				}
#endif

				P9000_WRITE_PATTERN_REGISTER(i,
					 line_state_p->pattern_registers[i],
					 swap_flag);

#if (defined(__DEBUG__))
				if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
				{
					(void) fprintf(debug_stream_p, 
								   "\t\t0x%x\n", 
								   P9000_READ_PATTERN_REGISTER(i, swap_flag));
				
				}
#endif

			}
		
			/*
			 * point to the current pattern loaded.
			 */

			register_state_p->pattern_registers_p =
				line_state_p->pattern_registers;

		}

		/*
		 * We need to switch the drawing engine into oversized mode in
		 * order to draw zero-width lines.  
		 */

		if (graphics_state_p->generic_state.si_graphics_state.SGlinestyle ==
			 SGLineDblDash)
		{
			
			/*
			 * Perform opaque stippling of the pattern registers.
			 */

			raster =
				((P9000_SOURCE_MASK &
				  P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,
												   graphics_state_p)) |
				 (~P9000_SOURCE_MASK & 
				  P9000_STATE_CALCULATE_BG_MINTERM(screen_state_p,
												   graphics_state_p)) | 
				 P9000_RASTER_USE_PATTERN); 
		}
		else 
		{

			ASSERT(graphics_state_p->generic_state.si_graphics_state.
				   SGlinestyle == SGLineDash);
			
			/*
			 * Perform transparent stippling.
			 */

			raster = 
				((P9000_SOURCE_MASK &
				  P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,
												   graphics_state_p)) | 
				 (~P9000_SOURCE_MASK & P9000_DESTINATION_MASK) | 
				 P9000_RASTER_USE_PATTERN);
			

		}
		
		P9000_STATE_SET_RASTER(register_state_p,
							   raster | P9000_RASTER_QUAD_OVERSIZED_MODE);

		/*
		 * Call the the helper. `isCapNotLast' applies for all the
		 * segments here. 
		 */

		p9000_line_segment_one_bit_helper(xorg, yorg, psegsIn, 
										  count, isCapNotLast, TRUE);

	}
	else
	{
		
		/*
		 * The dash pattern was such that it would not fit into
		 * the graphics engines pattern registers.  Draw it again
		 * using a method based on the CFB code.
		 */

		if ((graphics_state_p->generic_state.si_graphics_state.SGmode
			 != GXcopy) ||
			((graphics_state_p->generic_state.si_graphics_state.SGpmask 
			  & 0xFF) != 0xFF))
		{
			return (SI_FAIL);
		}
			
		p9000_line_segment_dashed_helper(xorg, yorg, psegsIn,
										 count, isCapNotLast);
		
	}
	

	return (SI_SUCCEED);

}

/*
 * @doc:p9000_line_one_bit_solid:
 *
 * Draw a set of lines with a solid fill style.  Depending on the
 * current raster operation we draw using the faster GXcopy style of
 * linedraw or a more accurate line draw method.
 * 
 * @enddoc
 */

STATIC SIBool
p9000_line_one_bit_solid(SIint32 xorg, SIint32 yorg, SIint32 count, 
	       SIPointP ptsIn, SIint32 isCapNotLast, SIint32 coordMode)
{
	unsigned int raster;
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		(struct generic_screen_state *) generic_screen_current_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));
	


#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_one_bit_line_solid)\n"
					   "{\n"
					   "\txorg = %ld\n"
					   "\tyorg = %ld\n"
					   "\tcount = %ld\n"
					   "\tptsIn = %p\n"
					   "\tisCapNotLast = %ld\n"
					   "\tcoordMode = %ld\n"
					   "}\n",
					   xorg, yorg, count, (void *) ptsIn,
					   isCapNotLast, coordMode);
	}
#endif

	ASSERT(graphics_state_p->
		   generic_state.si_graphics_state.SGlinestyle == SGLineSolid);
	
	if (count <= 1)
	{
		return (SI_SUCCEED);
	}

	if (!P9000_REGISTER_IS_VALID_ORIGIN(xorg,yorg))
	{
		return SI_FAIL;
	}
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
	{
		int i;
		SIPointP tmp_points_p;
		
		for (i = 0, tmp_points_p = ptsIn; i < count - 1; i++, tmp_points_p++)
		{
			(void) fprintf(debug_stream_p,
						   "\t\t(%ld,%ld) -> (%ld,%ld)\n",
						   tmp_points_p->x + xorg, 
						   tmp_points_p->y + yorg,
						   (tmp_points_p+1)->x + xorg, 
						   (tmp_points_p+1)->y + yorg);
		}
	}
#endif /* DEBUG */

	/*
	 * We need to set the engine to draw in oversized mode for
	 * drawing one pixel wide lines.
	 */

	raster = 
		P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,graphics_state_p);

	P9000_STATE_SET_RASTER(register_state_p,
		raster | P9000_RASTER_QUAD_OVERSIZED_MODE);

	P9000_STATE_SET_CLIP_RECTANGLE_TO_SI_CLIP(screen_state_p);

	P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
		(P9000_STATE_CHANGE_FOREGROUND_COLOR |
		 P9000_STATE_CHANGE_PLANEMASK));

	P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
		(P9000_STATE_CHANGE_FOREGROUND_COLOR |
		 P9000_STATE_CHANGE_PLANEMASK));

	/*
 	 * Call the appropriate helper to draw the lines.
	 */

	if (graphics_state_p->generic_state.si_graphics_state.SGmode ==
		GXcopy)
	{

		/*
		 * Faster drawing for GXcopy lines.
		 */

		p9000_line_one_bit_helper_gxcopy(xorg, yorg, ptsIn, count, 
										 coordMode, isCapNotLast);
	}
	else
	{
		/*
		 * Drawing lines in strict accordance to the protocol.
		 */

		p9000_line_one_bit_helper_rop(xorg, yorg, ptsIn, count, 
									  coordMode, isCapNotLast);
	}

	return (SI_SUCCEED);
	
}


/*
 * @doc:p9000_line_one_bit_dashed:
 *
 * Draw a set of dashed lines.  Depending on the current raster
 * operation we draw using the faster GXcopy style of linedraw or a
 * more accurate line draw method which is also slower for non GXcopy
 * lines.
 * 
 * @enddoc 
 */

STATIC SIBool
p9000_line_one_bit_dashed(SIint32 xorg, SIint32 yorg, SIint32 count, 
	       SIPointP ptsIn, SIint32 isCapNotLast, SIint32 coordMode)
{
	unsigned int raster;
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();

	struct p9000_line_state *line_state_p = 
		graphics_state_p->line_state_p;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		(struct generic_screen_state *) generic_screen_current_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE, graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_LINE_STATE, line_state_p));

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_one_bit_line_dashed)\n"
					   "{\n"
					   "\txorg = %ld\n"
					   "\tyorg = %ld\n"
					   "\tcount = %ld\n"
					   "\tptsIn = %p\n"
					   "\tisCapNotLast = %ld\n"
					   "\tcoordMode = %ld\n"
					   "}\n",
					   xorg, yorg, count, (void *) ptsIn,
					   isCapNotLast, coordMode);
	}
#endif

	ASSERT((graphics_state_p->
			generic_state.si_graphics_state.SGlinestyle == SGLineDash) ||
		   (graphics_state_p->
			generic_state.si_graphics_state.SGlinestyle == SGLineDblDash));
	
	if (count <= 1)
	{
		return (SI_SUCCEED);
	}

	if (!P9000_REGISTER_IS_VALID_ORIGIN(xorg,yorg))
	{
		return SI_FAIL;
	}
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
	{
		int i;
		SIPointP tmp_points_p;
		
		for (i = 0, tmp_points_p = ptsIn; i < count - 1; i++, tmp_points_p++)
		{
			(void) fprintf(debug_stream_p,
						   "\t\t(%ld,%ld) -> (%ld,%ld)\n",
						   tmp_points_p->x + xorg, 
						   tmp_points_p->y + yorg,
						   (tmp_points_p+1)->x + xorg, 
						   (tmp_points_p+1)->y + yorg);
		}
	}
#endif /* DEBUG */

	
	if (line_state_p->is_small_dash_pattern)
	{
		
		/*
		 * We need to set the engine to draw in oversized mode for
		 * drawing one pixel wide lines.  Further we need to set
		 * use-pattern bit to enable drawing using the pattern registers.
		 */

		if (graphics_state_p->generic_state.si_graphics_state.SGlinestyle ==
			 SGLineDblDash)
		{
			
			/*
			 * Perform opaque stippling of the pattern registers.
			 */

			raster =
				((P9000_SOURCE_MASK &
				  P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,
												   graphics_state_p)) |
				 (~P9000_SOURCE_MASK & 
				  P9000_STATE_CALCULATE_BG_MINTERM(screen_state_p,
												   graphics_state_p)) | 
				 P9000_RASTER_USE_PATTERN); 
		}
		else 
		{

			ASSERT(graphics_state_p->generic_state.si_graphics_state.
				   SGlinestyle == SGLineDash);
			
			/*
			 * Perform transparent stippling.
			 */

			raster = 
				((P9000_SOURCE_MASK &
				  P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,
												   graphics_state_p)) | 
				 (~P9000_SOURCE_MASK & P9000_DESTINATION_MASK) | 
				 P9000_RASTER_USE_PATTERN);
			

		}
		
		P9000_STATE_SET_RASTER(register_state_p,
							   raster |
							   P9000_RASTER_QUAD_OVERSIZED_MODE);

		P9000_STATE_SET_CLIP_RECTANGLE_TO_SI_CLIP(screen_state_p);

		/*
		 * Synchronize the registers to the SI specified values.
		 */

		P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
			(P9000_STATE_CHANGE_FOREGROUND_COLOR |
			 P9000_STATE_CHANGE_BACKGROUND_COLOR |
			 P9000_STATE_CHANGE_PLANEMASK));

		P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
			(P9000_STATE_CHANGE_FOREGROUND_COLOR |
			 P9000_STATE_CHANGE_BACKGROUND_COLOR |
			 P9000_STATE_CHANGE_PLANEMASK));
	
		/*
		 * Fill the pattern registers and set the pattern origin
		 * so that the first point of the line will get the pixel
		 * at (0, 0) of the pattern registers. 
		 */

		P9000_REGISTER_WAIT_FOR_ENGINE_IDLE();
		
		if (register_state_p->pattern_registers_p !=
			line_state_p->pattern_registers)
		{
			int i;
		
			const unsigned int swap_flag = 
				P9000_ADDRESS_SWAP_HALF_WORDS |
				P9000_ADDRESS_SWAP_BYTES |
				P9000_ADDRESS_SWAP_BITS;

			/*
			 * Fill the graphics engine's pattern registers with
			 * the line pattern.
			 */
		
#if (defined(__DEBUG__))
			if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
			{
				(void) fprintf(debug_stream_p, 
							   "\treprogramming pattern:\n");
			}
#endif
						   
			for (i = 0; i < P9000_DEFAULT_PATTERN_REGISTER_COUNT; i++)
			{

#if (defined(__DEBUG__))
				if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
				{
					(void) fprintf(debug_stream_p, 
								   "\t\t[%d] 0x%x -> ", 
								   i, 
								   line_state_p->pattern_registers[i]);
				}
#endif

				P9000_WRITE_PATTERN_REGISTER(i,
					 line_state_p->pattern_registers[i],
					 swap_flag);

#if (defined(__DEBUG__))
				if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
				{
					(void) fprintf(debug_stream_p, 
								   "\t\t0x%x\n", 
								   P9000_READ_PATTERN_REGISTER(i, swap_flag));
				
				}
#endif

			}
		
			/*
			 * point to the current pattern loaded.
			 */

			register_state_p->pattern_registers_p =
				line_state_p->pattern_registers;

		}
		
		/*
		 * Program pattern origin.
		 */

		P9000_WRITE_DRAWING_ENGINE_REGISTER(
			P9000_DRAWING_ENGINE_PAT_ORIGINX, 
			(xorg + ptsIn->x) % P9000_DEFAULT_PATTERN_REGISTER_WIDTH);

		P9000_WRITE_DRAWING_ENGINE_REGISTER(
			P9000_DRAWING_ENGINE_PAT_ORIGINY, 
			(yorg + ptsIn->y) % P9000_DEFAULT_PATTERN_REGISTER_HEIGHT);

		/*
		 * Call the appropriate helper to draw the lines.
		 */

		if (graphics_state_p->generic_state.si_graphics_state.SGmode ==
			GXcopy)
		{

			/*
			 * Faster drawing for GXcopy lines.
			 */

			p9000_line_one_bit_helper_gxcopy(xorg, yorg, ptsIn, count, 
											 coordMode, isCapNotLast);
		}
		else
		{
			/*
			 * Drawing lines in strict accordance to the protocol.
			 */

			p9000_line_one_bit_helper_rop(xorg, yorg, ptsIn, count, 
										  coordMode, isCapNotLast);
		}

	}
	else
	{

		/*
		 * The dash pattern is such that we cannot use the pattern
		 * registers.  Attempt to draw the pattern using a method
		 * based on the CFB drawing code.
		 */
		
		if ((graphics_state_p->generic_state.si_graphics_state.SGmode
			 != GXcopy) ||
			((graphics_state_p->generic_state.si_graphics_state.SGpmask & 
			  0xFF) != 0xFF))
		{
			return (SI_FAIL);
		}
			
		p9000_line_one_bit_helper_dashed(xorg, yorg, ptsIn, count,
										 coordMode, isCapNotLast);

	}
	
	
	return (SI_SUCCEED);
	
}

/*
 * @doc:p9000_one_bit_rectangle:
 *
 * Draw a one bit wide rectangle with the given coordinates.  The X
 * protocol specifies that the rectangle is to be drawn as a series of
 * polylines, respecting the current joinstyle and capstyle.  Since
 * the end and start points of the rectangle coincide, this becomes
 * the same as drawing the polyline with a capstyle of CapNotLast.
 * 
 * @enddoc
 */

STATIC SIBool
p9000_line_rectangle_one_bit(SIint32 x1, SIint32 y1, SIint32 x2, SIint32 y2)
{

	int width = x2 - x1;
	int height = y2 - y1;
	unsigned int raster;
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	
	int status;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
			 (struct generic_screen_state *) generic_screen_current_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		   (struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE, graphics_state_p));

	ASSERT((graphics_state_p->
		    generic_state.si_graphics_state.SGlinestyle == SGLineDash)||
		   (graphics_state_p->
		    generic_state.si_graphics_state.SGlinestyle == SGLineDblDash)||
		   (graphics_state_p->
		    generic_state.si_graphics_state.SGlinestyle == SGLineSolid));
	
	ASSERT((graphics_state_p->
		    generic_state.si_graphics_state.SGfillmode == SGFillSolidFG) ||
		   (graphics_state_p->
		    generic_state.si_graphics_state.SGfillmode != SGFillSolidBG));

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_one_bit_rectangle)\n"
					   "{\n"
					   "\tx1 = %ld\n"
					   "\ty1 = %ld\n"
					   "\tx2 = %ld\n"
					   "\ty2 = %ld\n"
					   "}\n",
					   x1,y1,x2,y2);
	}
#endif

	/*
	 * We can't handle dashed one bit rects, so check for this and
	 * return SI_FAIL.
	 */

	if (graphics_state_p->generic_state.si_graphics_state.SGlinestyle !=
		SGLineSolid)
	{
		return (SI_FAIL);
	}
	
	
	raster = 
		P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,graphics_state_p);

	P9000_STATE_SET_RASTER(register_state_p,
		raster | P9000_RASTER_QUAD_OVERSIZED_MODE);

	P9000_STATE_SET_CLIP_RECTANGLE_TO_SI_CLIP(screen_state_p);
	
	P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
		(P9000_STATE_CHANGE_FOREGROUND_COLOR |
		 P9000_STATE_CHANGE_PLANEMASK));

	P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
		(P9000_STATE_CHANGE_FOREGROUND_COLOR |
		 P9000_STATE_CHANGE_PLANEMASK));

	/*
	 * Setup the X, Y origin.
	 */

	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_W_OFF_XY,
		0);

	/*
	 * Null the current meta-coordinate register index.
	 */
	
	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_CINDEX,
		0);
	
	ASSERT(P9000_READ_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_CINDEX) == 0); /* check */


	/* Line 1: Draw a line from x1,y1 to x1 + width, y1 */

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_X,
		x1);

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_Y,
		y1);

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_X,
		x1 + width);

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_Y,
		y1);

	do
	{
		status = P9000_INITIATE_QUAD_COMMAND();
	} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);

	if (status & P9000_STATUS_QUAD_SOFTWARE)
	{
		p9000_draw_clipped_segment(x1, y1, x1 + width, y1);
	}
	
	/* Line 2: Draw a line from x1 + width ,y1 to x1 + width, y1 + height */

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_X,
		x1 + width);

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_Y,
		y1);

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_X,
		x1 + width);

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_Y,
		y1 + height);

	do
	{
		status = P9000_INITIATE_QUAD_COMMAND();
	} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);

	if (status & P9000_STATUS_QUAD_SOFTWARE)
	{
		p9000_draw_clipped_segment(
			x1 + width, y1, x1 + width, y1 + height);
	}

	/* Draw a line from x1 + width,y1 + height to x1, y1 + height */

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_X,
		x1 + width);

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_Y,
		y1 + height);

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_X,
		x1);

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_Y,
		y1 + height);

	do
	{
		status = P9000_INITIATE_QUAD_COMMAND();
	} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);

	if (status & P9000_STATUS_QUAD_SOFTWARE)
	{
		p9000_draw_clipped_segment(
			x1 + width, y1 + height, x1, y1 + height);
	}

	/* Draw a line from x1,y1 + height to x1, y1*/

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_X,
		x1);

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_Y,
		y1 + height);

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_X,
		x1);

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_Y,
		y1);

	do
	{
		status = P9000_INITIATE_QUAD_COMMAND();
	} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);

	if (status & P9000_STATUS_QUAD_SOFTWARE)
	{
		p9000_draw_clipped_segment(
			x1, y1 + height, x1, y1);
	}

	return (SI_SUCCEED);
}

/** 
 ** Backward compatibility support.
 **/

/*
 * @doc:p9000_line_compat_one_bit_solid:
 *
 * Draw solid lines for SI 1_0.  Since the lines are already
 * translated by SI into CapButt format, we draw the lines in a
 * straight forward manner.
 * 
 * @enddoc
 */

STATIC SIBool
p9000_line_compat_one_bit_solid(SIint32 count, SIPointP points_p)
{

	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	unsigned int raster;
	int current_x, current_y;
	int status;
	const SIPointP pointsFence = points_p + count;
	
	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_line_compat_one_bit_solid)\n"
					   "{\n"
					   "\tcount = %ld\n"
					   "\tptsIn = %p\n"
					   "}\n",
					   count, (void *) points_p);
	}
#endif

	ASSERT(graphics_state_p->
		   generic_state.si_graphics_state.SGlinestyle == SGLineSolid);
	
	if (count <= 1)
	{
		return (SI_SUCCEED);
	}

	/*
	 * Draw the line as if in isCapNotLast mode, with coordMode origin.
	 */

	/*
	 * Setup the X, Y origin.
	 */

	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_W_OFF_XY,
		0);										   

	/*
	 * Null the current meta-coordinate register index.
	 */
	
	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_CINDEX,
		0);
	
	ASSERT(P9000_READ_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_CINDEX) == 0); /* check */

	/*
	 * We need to switch the drawing engine into oversized mode in
	 * order to draw zero-width lines.  Program the raster bit
	 * accordingly.
	 */

	raster = 
		P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,graphics_state_p);

	P9000_STATE_SET_RASTER(register_state_p,
		raster | P9000_RASTER_QUAD_OVERSIZED_MODE);

	P9000_STATE_SET_CLIP_RECTANGLE_TO_SI_CLIP(screen_state_p);

	P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
		(P9000_STATE_CHANGE_FOREGROUND_COLOR |
		 P9000_STATE_CHANGE_PLANEMASK));
	
	P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
		(P9000_STATE_CHANGE_FOREGROUND_COLOR |
		 P9000_STATE_CHANGE_PLANEMASK));

	/*
	 * Onto drawing the lines.
	 */

	current_x = points_p->x;
	current_y = points_p->y;

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))

	{
		(void) fprintf(debug_stream_p,
					   "\t{\n"
					   "\t\t(%d, %d)\n",
					   current_x, current_y);
	}
#endif

	/*
	 * Write the meta coordinate registers for the first point.
	 */

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_X,
		current_x);

	P9000_WRITE_META_COORDINATE_REGISTER(
		P9000_META_COORDINATE_VTYPE_LINE,
		P9000_META_COORDINATE_ABS,
		P9000_META_COORDINATE_YX_Y,
		current_y);

	/*
	 * Move onto the next point of the polyline.
	 */
	
	points_p++;
	
	/*
	 * Pump the points down to the graphics engine, one by one.
	 */

	do
	{
		/*
		 * Get the current end points.
		 */

		current_x = points_p->x;
		current_y = points_p->y;
			
		/*
		 * Write the meta coordinate registers for the current
		 * point. 
		 */

		P9000_WRITE_META_COORDINATE_REGISTER(
			P9000_META_COORDINATE_VTYPE_LINE, 
			P9000_META_COORDINATE_ABS,
			P9000_META_COORDINATE_YX_X,
			current_x);
		P9000_WRITE_META_COORDINATE_REGISTER(
			P9000_META_COORDINATE_VTYPE_LINE, 
			P9000_META_COORDINATE_ABS,
			P9000_META_COORDINATE_YX_Y,
			current_y);
											 
#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
		{
			(void) fprintf(debug_stream_p,
						   "\t\t(%d, %d)\n",
						   current_x, current_y);
		}
#endif

		/*
		 * Initiate the draw operation
		 */

		do
		{
			status = P9000_INITIATE_QUAD_COMMAND();
		} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);
			
		/*
		 * Check if the drawing needs to be re-tried.
		 */

		if (status & P9000_STATUS_QUAD_SOFTWARE)
		{

			/*
			 * Try and redraw the line.
			 */
			
			p9000_draw_clipped_segment(
									(points_p - 1)->x,
									(points_p - 1)->y,
								   current_x, current_y);
		}
			

	} while (++points_p < pointsFence);
		
	return (SI_SUCCEED);
}


/*
 * @doc:p9000_line_segment_compat_one_bit_solid:
 *
 * For the segment draw routines, we only need to do minor
 * transformations on the inputs.
 * 
 * @enddoc
 */

STATIC SIBool
p9000_line_segment_compat_one_bit_solid(SIint32 count, SIPointP ptsIn)
{

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_line_segment_compat_one_bit_solid)\n"
					   "{\n"
					   "\tcount = %ld\n"
					   "\tptsIn = %p\n"
					   "}\n",
					   count, (void *) ptsIn);
	}
#endif
	
	return p9000_line_segment_one_bit_solid(0, 0, count >> 1, 
										   (SISegmentP) ptsIn,
										   FALSE);
}

#define EVEN(X)			(((X) & 1) == 0)
#define ROTATE(PATT, I)	(((PATT) << (I)) | ((PATT) >> (16 - (I))))

/*
 * @doc:p9000_line_state_update_dash_pattern
 *
 * @enddoc
 */

STATIC void
p9000_line_state_update_dash_pattern(
	struct p9000_graphics_state *graphics_state_p)
{

	P9000_CURRENT_SCREEN_STATE_DECLARE();
	
	SIint32 *dash_list_p =
		graphics_state_p->generic_state.si_graphics_state.SGline;

	int dash_length = 0;
	unsigned short dash_pattern = 0;
	unsigned short *pattern_registers_p;
	
	struct p9000_line_state *line_state_p;

	int i;
	
	/*
	 * Allocate space for the line state if necessary.
	 */

	if (graphics_state_p->line_state_p == NULL)
	{
		graphics_state_p->line_state_p =
			allocate_and_clear_memory(sizeof(struct p9000_line_state));

		STAMP_OBJECT(P9000_LINE_STATE,
					 (struct p9000_line_state *) 
					 graphics_state_p->line_state_p);
	}

	/*
	 * Get the current line state.
	 */

	line_state_p = graphics_state_p->line_state_p;
	
	if (line_state_p == NULL)
	{
		return;
	}
	
	ASSERT(IS_OBJECT_STAMPED(P9000_LINE_STATE, line_state_p));
	ASSERT((graphics_state_p->generic_state.si_graphics_state.SGlineCNT &
			1) == 0);
	
	line_state_p->is_small_dash_pattern = FALSE;

	if (!(screen_state_p->options_p->linedraw_options &
		  P9000_OPTIONS_LINEDRAW_OPTIONS_USE_PATTERN_REGISTERS))
	{
		return;
	}
	
	for (i = 0; i <
		 graphics_state_p->generic_state.si_graphics_state.SGlineCNT;
		 i ++)
	{
		if ((dash_length + dash_list_p[i]) >
			P9000_DEFAULT_PATTERN_REGISTER_WIDTH)
		{
			return;
		}

		/*
		 * Get the mask of the pattern.
		 */

		if (EVEN(i))
		{
			unsigned short bits = 
				((1 << dash_list_p[i]) - 1) << dash_length;
		
			dash_pattern |= (bits);
		}

		/*
		 * increment the current bit position.
		 */

		dash_length += dash_list_p[i];
		
	}

	/*
	 * duplicate the dash pattern to fit 16 bits.
	 */

	switch (dash_length)
	{
	case 1:
		dash_pattern |= (dash_pattern << 1);
		/*FALLTHROUGH*/
	case 2:
		dash_pattern |= (dash_pattern << 2);
		/*FALLTHROUGH*/
	case 4:
		dash_pattern |= (dash_pattern << 4);
		/*FALLTHROUGH*/
	case 8:
		dash_pattern |= (dash_pattern << 8);
		/*FALLTHROUGH*/
	case 16:
		break;
	default:
		return;
	}
	
	/*
	 * Fill up the pattern register array.
	 */

	line_state_p->is_small_dash_pattern = TRUE;

	pattern_registers_p = (unsigned short *)
		line_state_p->pattern_registers;
	
	for (i = 0; i < P9000_DEFAULT_PATTERN_REGISTER_HEIGHT; 
		 i ++, pattern_registers_p++)
	{
		*pattern_registers_p = ROTATE(dash_pattern, i);
	}

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_line, INTERNAL))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_line_state_update_dash_pattern)\n"
					   "\tdash_pattern = 0x%x\n",
					   dash_pattern);
	}
#endif

}

/*
 * @doc:p9000_line__gs_change__:
 *
 * Graphics state handling for the line module.
 * 
 * We change the function pointers depending on whether the line style
 * is solid or dashed.
 * 
 * Backward Server compatibility support:  we need to swap the
 * appropriate pointers, for the drawing functions.
 *
 * @enddoc
 */

function void
p9000_line__gs_change__(void)
{
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		(struct generic_screen_state *) generic_screen_current_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE, screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		   (struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE, graphics_state_p));
	
	/*
	 * Restore the defaults.
	 */

	screen_state_p->generic_state.screen_functions_p->
		si_line_onebitline =
			graphics_state_p->generic_si_functions.si_line_onebitline;

	screen_state_p->generic_state.screen_functions_p->
		si_line_onebitseg = 
			graphics_state_p->generic_si_functions.si_line_onebitseg;

	screen_state_p->generic_state.screen_functions_p->
		si_line_onebitrect = 
			graphics_state_p->generic_si_functions.si_line_onebitrect;
	
	/* 
	 * Switch function pointers as needed.
	 */

	if (screen_state_p->generic_state.screen_sdd_version_number >=
		DM_SI_VERSION_1_1)
	{

		/*
		 * Determine if the dashed pattern has changed.
		 */

		if ((graphics_state_p->generic_state.si_state_flags & SetSGline) &&
			(screen_state_p->options_p->linedraw_options &
			 P9000_OPTIONS_LINEDRAW_OPTIONS_USE_DASHED_LINES))
		{
			p9000_line_state_update_dash_pattern(graphics_state_p);
		}
	
		/*
		 * SI 1.1's calling sequence.
		 */

		if ((graphics_state_p->generic_state.si_graphics_state.SGfillmode
			 == SGFillSolidFG) ||
			(graphics_state_p->generic_state.si_graphics_state.SGfillmode
			 == SGFillSolidBG))
		{

			if (graphics_state_p->generic_state.si_graphics_state.SGlinestyle
				 == SGLineSolid)
			{
				if (screen_state_p->options_p->linedraw_options &
					P9000_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_DRAW)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitline = 
							p9000_line_one_bit_solid;
				}

				if (screen_state_p->options_p->linedraw_options &
					P9000_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitseg =
							p9000_line_segment_one_bit_solid;
				}

				if (screen_state_p->options_p->linedraw_options &
					P9000_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_RECTANGLES)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitrect =
							p9000_line_rectangle_one_bit;
				}
			}
			else				/* patterned draws */
			{
				if (screen_state_p->options_p->linedraw_options &
					P9000_OPTIONS_LINEDRAW_OPTIONS_USE_DASHED_LINES)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitline = 
							p9000_line_one_bit_dashed;
				}

				if (screen_state_p->options_p->linedraw_options &
					P9000_OPTIONS_LINEDRAW_OPTIONS_USE_DASHED_SEGMENTS)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitseg = 
							p9000_line_segment_one_bit_dashed;
				}

			}
		}
	}
	else
	{

		/*
		 * Handle SI 1.0 servers.
		 */

		/*
		 * compensate for a bug in SI 1.0 : the SGlinestyle field is
		 * sometimes `0' an illegal value.   Set this to
		 * SGLineSolid, to allow the rest of the code to work.
		 *
		 * SI 1.0 servers do not supported dashed lines in the SDD.
		 */

		if (graphics_state_p->generic_state.si_graphics_state.SGlinestyle == 0)
		{
			graphics_state_p->generic_state.si_graphics_state.SGlinestyle =
				SGLineSolid;
		}
		
		if ((graphics_state_p->generic_state.si_graphics_state.SGfillmode
			 == SGFillSolidFG) ||
			(graphics_state_p->generic_state.si_graphics_state.SGfillmode
			 == SGFillSolidBG))
		{
			if (graphics_state_p->generic_state.si_graphics_state.SGlinestyle
				== SGLineSolid)
			{
				if (screen_state_p->options_p->linedraw_options &
				 P9000_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_DRAW)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitline = 
					    (SIBool (*)(SIint32, SIint32, SIint32, SIPointP,
								SIint32, SIint32))
					    p9000_line_compat_one_bit_solid;
				}

				if (screen_state_p->options_p->linedraw_options &
					P9000_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitseg =  
				        (SIBool (*)(SIint32, SIint32, 
								SIint32, SISegmentP, SIint32))
					    p9000_line_segment_compat_one_bit_solid;
				}

				if (screen_state_p->options_p->linedraw_options &
					P9000_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_RECTANGLES)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitrect =
							p9000_line_rectangle_one_bit;
				}
			}
			else				/* no patterned draws */
			{
				/*EMPTY*/
				;
			}
		}
	}
}


/*
 * @doc:p9000_line__initialize__:
 * 
 *  Initialize the line module.
 *
 * @endoc
 */

function void
p9000_line__initialize__(SIScreenRec *si_screen_p,
						struct p9000_options_structure * options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	
	flags_p->SIavail_line = 0;

	if (options_p->linedraw_options &
		P9000_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_DRAW)
	{
		flags_p->SIavail_line |= ONEBITLINE_AVAIL;
		functions_p->si_line_onebitline = p9000_line_one_bit_solid;
	}

	if (options_p->linedraw_options &
		P9000_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_RECTANGLES)
	{
		flags_p->SIavail_line |= ONEBITRECT_AVAIL;
		functions_p->si_line_onebitrect = p9000_line_rectangle_one_bit;
	}

	if (options_p->linedraw_options &
		P9000_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW)
	{
		flags_p->SIavail_line |= ONEBITSEG_AVAIL;
		functions_p->si_line_onebitseg = p9000_line_segment_one_bit_solid;
	}

	/*
	 * Add dashed line support if allowed.
	 */

	if (options_p->linedraw_options &
		P9000_OPTIONS_LINEDRAW_OPTIONS_USE_DASHED_LINES)
	{
		flags_p->SIavail_line |= (DASH_AVAIL | DBLDASH_AVAIL);
	}
	
}
