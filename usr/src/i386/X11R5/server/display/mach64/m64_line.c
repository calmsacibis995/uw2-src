/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_line.c	1.8"

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
export boolean m64_line_debug = 0;
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

#include "g_state.h"
#include "g_omm.h"
#include "m64_gbls.h"
#include "m64_opt.h"
#include "m64_regs.h"
#include "m64_state.h"
#include "m64_gs.h"

/***
 ***	Constants.
 ***/
#define OUT_LEFT    0x08
#define OUT_RIGHT   0x04
#define OUT_ABOVE   0x02
#define OUT_BELOW   0x01

/***
 ***	Macros.
 ***/
#define OUTCODE(result, x, y, left, right, top, bottom)\
        if ((x) < (left)) \
            result |= OUT_LEFT; \
        else if ((x) > (right)) \
            result |= OUT_RIGHT;\
        if ((y) < (top))\
            result |= OUT_ABOVE;\
        else if ((y) > (bottom))\
            result |= OUT_BELOW;

/*
 * Code directly imported from page 3-17 of mach64 programmers guide.
 */
#define M64_DRAW_LINE(x1,y1,x2,y2)											\
{																			\
	unsigned long dx, dy, small, large, gui_traj_cntl;						\
	gui_traj_cntl = *(register_values_p+M64_REGISTER_GUI_TRAJ_CNTL_OFFSET);	\
	if (x1 < x2)															\
	{																		\
		dx = x2 - x1;														\
	}																		\
	else																	\
	{																		\
		dx = x1 - x2;														\
		gui_traj_cntl &= ~GUI_TRAJ_CNTL_DST_X_DIR_LEFT_TO_RIGHT;			\
	}																		\
	if (y1 < y2)															\
	{																		\
		dy = y2 - y1;														\
	}																		\
	else																	\
	{																		\
		dy = y1 - y2;														\
		gui_traj_cntl &= ~GUI_TRAJ_CNTL_DST_Y_DIR_TOP_TO_BOTTOM;			\
	}																		\
	if (dx < dy)															\
	{																		\
		small = dx;															\
		large = dy;															\
		gui_traj_cntl |= GUI_TRAJ_CNTL_DST_Y_MAJOR_Y_MAJOR_LINE;			\
	}																		\
	else																	\
	{																		\
		small = dy;															\
		large = dx;															\
	}																		\
	M64_WAIT_FOR_FIFO(6);													\
	*(register_base_address_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) = 		\
		gui_traj_cntl;														\
	*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = 				\
		(((unsigned)x1 << DST_Y_X_DST_X_SHIFT) | y1);						\
	*(register_base_address_p + M64_REGISTER_DST_BRES_ERR_OFFSET) =			\
		(small << 1U) - large;												\
	*(register_base_address_p + M64_REGISTER_DST_BRES_INC_OFFSET) = 		\
		small << 1U; 														\
	*(register_base_address_p + M64_REGISTER_DST_BRES_DEC_OFFSET) =			\
		0x3FFFF - ((large - small) << 1);									\
	*(register_base_address_p + M64_REGISTER_DST_BRES_LNTH_OFFSET) =		\
		large + 1;															\
}


/***
 ***	Functions.
 ***/
/*
 * m64_line_clip_endpoints_to_physical_space:
 * 		Clip a line to the device coordinate space. Return TRUE if 
 * 		clipping yeilds coordinates within the device coordinate
 *		space else return FALSE. Inputs are addresses of the endpoints
 *		which contain the initial values and the resultant clipped 
 *		values are replaced in them.
 */
STATIC boolean
m64_line_clip_endpoints_to_physical_space(int *x1, int *y1, int *x2, int *y2)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();

	int	oc1 = 0, oc2 = 0;
	int	xl,yt,xr,yb;

	int	top_clip = 0, left_clip = 0;
	int	bottom_clip = screen_state_p->generic_state.screen_physical_height - 1;
	int	right_clip = screen_state_p->generic_state.screen_physical_width - 1;

	xl = *x1; yt = *y1; xr = *x2; yb = *y2;

	OUTCODE(oc1,xl,yt,left_clip,right_clip,top_clip,bottom_clip);
	OUTCODE(oc2,xr,yb,left_clip,right_clip,top_clip,bottom_clip);

	ASSERT (!(oc1 & oc2));

	while (oc1 | oc2)
	{
		int oc;
		int x, y;				/* clipped coordinates */
		int flag;
		
		if (oc1 & oc2)			/* line totally out of physical bounds.*/
		{
			return (FALSE);
		}
		
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
			ASSERT(xl != xr);
			y = yt + (yb - yt) * (left_clip - xl) / (xr - xl);
			x = left_clip;
		}
		else if (oc & OUT_RIGHT) /* crosses right edge */
		{
			ASSERT(xl != xr);
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
		if (flag)
		{
			xl = x;
			yt = y;
			oc1 = 0;
			OUTCODE(oc1, xl, yt, left_clip, right_clip, top_clip,
					bottom_clip);
		}
		else 
		{
			xr = x;
			yb = y;
			oc2 = 0;
			OUTCODE(oc2, xr, yb, left_clip, right_clip, top_clip,
					bottom_clip); 
		}
	}
	
	
	ASSERT(!oc1 && !oc2);
	
	*x1 = xl;
	*y1 = yt;
	*x2 = xr;
	*y2 = yb;

	return TRUE;
}

STATIC SIBool
m64_line_segment_one_bit_solid(SIint32 xorg, SIint32 yorg, SIint32 count,
		  SISegmentP psegsIn, SIint32 isCapNotLast)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;
    const int top_clip = screen_state_p->generic_state.screen_clip_top;
    const int bottom_clip = screen_state_p->generic_state.screen_clip_bottom;
    const int left_clip = screen_state_p->generic_state.screen_clip_left;
    const int right_clip = screen_state_p->generic_state.screen_clip_right;

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
	
	if (count < 1)
	{
		return (SI_SUCCEED);
	}

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));

	ASSERT(graphics_state_p->generic_state.si_graphics_state.
		SGlinestyle == SGLineSolid);

#if (defined(__DEBUG__))
	if (m64_line_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_one_bit_segmnet_solid) {\n"
			"\txorg = %ld\n"
			"\tyorg = %ld\n"
			"\tcount = %ld\n"
			"\tpsegsIn = %p\n"
			"\tisCapNotLast = %ld\n"
			"}\n",
			xorg, yorg, count, (void *) psegsIn, isCapNotLast);
	}
#endif

	/*
	 * Set the clipping rectangle to graphics state.
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
	 * Change the value of the default to include last pel on. Remember
	 * to undo this change at the end.
	 */
	if(!isCapNotLast)
	{
		register_values_p[M64_REGISTER_GUI_TRAJ_CNTL_OFFSET] |= 
			GUI_TRAJ_CNTL_DST_LAST_PEL;
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
	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		SGFillSolidFG || 
		graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		SGFillSolidBG);

	M64_WAIT_FOR_FIFO(1);
	*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
		(graphics_state_p->generic_state.si_graphics_state.SGfillmode == 
		SGFillSolidFG ? DP_SRC_FRGD_COLOR << DP_SRC_DP_FRGD_SRC_SHIFT : 0) |
		DP_SRC_DP_MONO_SRC_ALWAYS_1 ;

	do
	{
		int x1 = psegsIn->x1 + xorg;
		int y1 = psegsIn->y1 + yorg;
		int x2 = psegsIn->x2 + xorg;
		int y2 = psegsIn->y2 + yorg;
        register int oc1 = 0, oc2 = 0;

		psegsIn++;

		/*
		 * Check the segments against the physical coordinate space.
		 */
		OUTCODE(oc1,x1,y1,
			0, screen_state_p->generic_state.screen_physical_width - 1,
			0, screen_state_p->generic_state.screen_physical_height - 1);
		OUTCODE(oc2,x2,y2,
			0, screen_state_p->generic_state.screen_physical_width - 1,
			0, screen_state_p->generic_state.screen_physical_height - 1);

		if (oc1 & oc2) 	/* completely outside the physical bounds */
		{
#if (defined(__DEBUG__))
			if (m64_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"m64_line_segment_one_bit_solid(%d,%d,%d,%d){\n"
					"\tsegment completely outside physical coords space\n"
					"}\n",
					x1,y1,x2,y2);
			}
#endif
			continue;
		}

		if (oc1 | oc2)
		{
			int 	result;

			/* Reset for further use. */
			oc1 = oc2 = 0;

#if (defined(__DEBUG__))
			if (m64_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"m64_line_segment_one_bit_solid(){\n"
					"\tclipping segment from (%d,%d,%d,%d)\n",
					x1,y1,x2,y2);
			}
#endif
			result = m64_line_clip_endpoints_to_physical_space(&x1,&y1,&x2,&y2);
#if (defined(__DEBUG__))
			if (m64_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"\tto (%d,%d,%d,%d)\n"
					"}\n",
					x1,y1,x2,y2);
			}
#endif
			/* Clipping yeilded a line outside the coordinate space. */
			if (result == FALSE)
			{
				continue;
			}
		}

		/*
		 * Check the segments against the clip rectangle.
		 */
		ASSERT(oc1 == 0 && oc2 == 0);
		OUTCODE(oc1,x1,y1,left_clip,right_clip,top_clip,bottom_clip);
		OUTCODE(oc2,x2,y2,left_clip,right_clip,top_clip,bottom_clip);

		if (oc1 & oc2) 	/* completely outside the clipping rectangle */
		{
#if (defined(__DEBUG__))
			if (m64_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"m64_line_segment_one_bit_solid(%d,%d,%d,%d){\n"
					"\tsegment completely outside clipping rectangle\n"
					"}\n",
					x1,y1,x2,y2);
			}
#endif
			continue;
		}
		
		/*
		 * Draw the line segment from x1,y1 to x2,y2.
		 */
		M64_DRAW_LINE(x1,y1,x2,y2);
	}while(--count);

	/*
	 * Restore last pel flag.
	 */
	register_values_p[M64_REGISTER_GUI_TRAJ_CNTL_OFFSET] &= 
			~GUI_TRAJ_CNTL_DST_LAST_PEL;

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
#if (defined(__DEBUG__))	
	M64_WAIT_FOR_GUI_ENGINE_IDLE();
#endif
	return (SI_SUCCEED);
}

/*
 * m64_line_one_bit_solid:
 *
 * Semantics of one bit lines are as follows:
 * If 'isCapNotLast', draw lines with the last pixel left undrawn.
 * If not isCapNotLast, then draw all lines except the very last one
 * with their last pixels undrawn and draw the last line with the last
 * pixel drawn.
 * Line with coincident endpoints are treated in the following manner:
 * If isCapNotLast nothing is drawn, if not draw a single pixel.  We
 * will be using the device-dependency trapdoor here to escape this
 * bit of onerousness.
 */
STATIC SIBool
m64_line_one_bit_solid(SIint32 xorg, SIint32 yorg, SIint32 count, 
	       SIPointP ptsIn, SIint32 isCapNotLast, SIint32 coordMode)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;

	int  	x1,y1,x2,y2;

	int		unclipped_x1,unclipped_y1;
	int 	oc1 = 0, oc2 = 0;

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));

	ASSERT(graphics_state_p->generic_state.si_graphics_state.
		SGlinestyle == SGLineSolid);
	
#if (defined(__DEBUG__))
	if (m64_line_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_line_one_bit_solid) {\n"
			"\txorg = %ld\n"
			"\tyorg = %ld\n"
			"\tcount = %ld\n"
			"\tpsegsIn = %p\n"
			"\tisCapNotLast = %ld\n"
			"\tcoordMode = %ld\n"
			"}\n",
			xorg, yorg, count, (void *) ptsIn,isCapNotLast,coordMode);
	}
#endif

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));

	if (count <= 1)
	{
		return (SI_SUCCEED);
	}

	/*
	 * Set the clipping rectangle to graphics state.
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
	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		SGFillSolidFG || 
		graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		SGFillSolidBG);

	M64_WAIT_FOR_FIFO(1);
	*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
		(graphics_state_p->generic_state.si_graphics_state.SGfillmode == 
		SGFillSolidFG ? DP_SRC_FRGD_COLOR << DP_SRC_DP_FRGD_SRC_SHIFT : 0) |
		DP_SRC_DP_MONO_SRC_ALWAYS_1 ;
		
	x2 = ptsIn->x + xorg;
	y2 = ptsIn->y + yorg;
	unclipped_x1 = x2;
	unclipped_y1 = y2;
	OUTCODE(oc2,x2,y2,
		0, screen_state_p->generic_state.screen_physical_width - 1,
		0, screen_state_p->generic_state.screen_physical_height - 1);

	do 
	{
		oc1 = oc2;
		oc2 = 0;
		x1 = unclipped_x1;
		y1 = unclipped_y1;
		if( coordMode == SICoordModePrevious)
		{
			xorg = unclipped_x1;
			yorg = unclipped_y1;
		}
		++ptsIn;
		x2 = ptsIn->x + xorg;
		y2 = ptsIn->y + yorg;
		unclipped_x1 = x2;
		unclipped_y1 = y2;

		/*
		 * Check the segments against the physical coordinate space.
		 */
		OUTCODE(oc2,x2,y2,
			0, screen_state_p->generic_state.screen_physical_width - 1,
			0, screen_state_p->generic_state.screen_physical_height - 1);

		if (oc1 & oc2) 	/* completely outside the physical bounds */
		{
#if (defined(__DEBUG__))
			if (m64_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"m64_line_one_bit_solid(%d,%d,%d,%d){\n"
					"\tsegment completely outside physical coords space\n"
					"}\n",
					x1,y1,x2,y2);
			}
#endif
			continue;
		}
		else if (oc1 | oc2)	/* line intersects some physical bound. */
		{
			int 	result;

#if (defined(__DEBUG__))
			if (m64_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"m64_line_one_bit_solid(){\n"
					"\tclipping segment from (%d,%d,%d,%d)\n",
					x1,y1,x2,y2);
			}
#endif
			result = m64_line_clip_endpoints_to_physical_space(&x1,&y1,&x2,&y2);
#if (defined(__DEBUG__))
			if (m64_line_debug)
			{
				(void) fprintf(debug_stream_p, 
					"\tto (%d,%d,%d,%d)\n" 
					"}\n",
					x1,y1,x2,y2);
			}
#endif
			/* Clipping yeilded a line outside the physical coordinate space.*/
			if (result != TRUE)
			{
				continue;
			}
		}
		/* else line is within the physical coordinate space */

		/*
		 * At this point we have some part/whole of the line within 
		 * physical coordinate space. Set the current x and current y
		 * Draw the line from x1,y1 to x2,y2.
		 */
		M64_DRAW_LINE(x1,y1,x2,y2);

	} while( --count > 1);

	/*
	 * Draw the last point of the last line, if CapNotLast is FALSE
	 * and if the last point is within the device coordinate space
	 * and if the ending pixels of the last line were not clipped.
	 */
	if(!isCapNotLast)
	{
		if((!oc2) && (x2 == unclipped_x1) && (y2 == unclipped_y1))
		{
			int	x = x2 + 1;
			int y = y2 + 1;

			M64_DRAW_LINE(x2,y2,x,y);
		}
	}	

	ASSERT(!M64_IS_FIFO_OVERFLOW());

#if (defined(__DEBUG__))	
	M64_WAIT_FOR_GUI_ENGINE_IDLE();
#endif
	return (SI_SUCCEED);
}


STATIC SIBool
m64_line_segment_one_bit_dashed(SIint32 xorg, SIint32 yorg, SIint32 count,
		  SISegmentP psegsIn, SIint32 isCapNotLast)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;
    const int top_clip = screen_state_p->generic_state.screen_clip_top;
    const int bottom_clip = screen_state_p->generic_state.screen_clip_bottom;
    const int left_clip = screen_state_p->generic_state.screen_clip_left;
    const int right_clip = screen_state_p->generic_state.screen_clip_right;

	struct m64_line_state *line_state_p = 
		&(graphics_state_p->current_line_state);

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
	

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));

	ASSERT((graphics_state_p->generic_state.si_graphics_state.
		SGlinestyle == SGLineDash)||(graphics_state_p->generic_state.
		si_graphics_state.SGlinestyle == SGLineDblDash));

#if (defined(__DEBUG__))
	if (m64_line_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_line_segment_one_bit_dashed) {\n"
			"\txorg = %ld\n"
			"\tyorg = %ld\n"
			"\tcount = %ld\n"
			"\tpsegsIn = %p\n"
			"\tisCapNotLast = %ld\n"
			"}\n",
			xorg, yorg, count, (void *) psegsIn, isCapNotLast);
	}
#endif

	if (count < 1)
	{
		return (SI_SUCCEED);
	}

	/*
	 * Download the line pattern  into offscreen memory if either the
	 * foreground/background colors have changed or the pattern has
	 * not been downloaded at all so far.
	 */
	if ((line_state_p->is_pattern_valid == FALSE) || 
		(graphics_state_p->generic_state.si_graphics_state.SGfg != 
			line_state_p->foreground_color) || 
		(graphics_state_p->generic_state.si_graphics_state.SGbg != 
			line_state_p->background_color))
	{
		line_state_p->is_pattern_valid = FALSE;
		m64_graphics_state_download_line_pattern(screen_state_p, 
			graphics_state_p);
	}

	if (!(OMM_LOCK(line_state_p->allocation_p)) || 
		line_state_p->is_pattern_valid == FALSE)
	{
		return(SI_FAIL);
	}
	ASSERT(line_state_p->is_pattern_valid == TRUE);

	/*
	 * Set the clipping rectangle to graphics state.
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
	 * Set the graphics engine to do dashed lines.
	 * See page 3-13 of the programmers manual.
	 * Our assumption here is that the dash pattern is max 32 bits in length.
	 */
	M64_WAIT_FOR_FIFO(3);
	*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
		(DP_SRC_BLIT_SRC << DP_SRC_DP_FRGD_SRC_SHIFT) |
		DP_SRC_DP_MONO_SRC_ALWAYS_1;
	*(register_base_address_p + M64_REGISTER_SRC_Y_X_OFFSET) = 
		line_state_p->allocation_p->y | 
		((unsigned)(line_state_p->allocation_p->x + 32 - 1) << 
		SRC_Y_X_SRC_X_SHIFT);
	*(register_base_address_p + M64_REGISTER_SRC_HEIGHT1_WID1_OFFSET) = 
		1 | 
		(line_state_p->dash_pattern_length << SRC_HEIGHT1_WID1_SRC_WID1_SHIFT);
	
	/*
	 * For simple dashed lines the off pairs are not drawn. Prohibit this
	 * by the use of the color compare function.
	 */
	if (graphics_state_p->generic_state.si_graphics_state.SGlinestyle == 
		SGLineDash)
	{
		M64_WAIT_FOR_FIFO(3);
		*(register_base_address_p + M64_REGISTER_CLR_CMP_CNTL_OFFSET) = 
			CLR_CMP_CNTL_CLR_CMP_FN_COMPARE_EQUAL | 
			CLR_CMP_CNTL_CLR_CMP_SRC_COMPARE_SOURCE;
		*(register_base_address_p + M64_REGISTER_CLR_CMP_CLR_OFFSET) = 
			*(register_values_p + M64_REGISTER_DP_BKGD_CLR_OFFSET);
		*(register_base_address_p + M64_REGISTER_CLR_CMP_MASK_OFFSET) = 
			(~0U >> (32U - screen_state_p->generic_state.screen_depth));
	}

	register_values_p[M64_REGISTER_GUI_TRAJ_CNTL_OFFSET] |= 
		GUI_TRAJ_CNTL_SRC_PATT_EN ;
	/*
	 * Change the value of the default to include last pel on. Remember
	 * to undo this change at the end.
	 */
	if(!isCapNotLast)
	{
		register_values_p[M64_REGISTER_GUI_TRAJ_CNTL_OFFSET] |= 
			GUI_TRAJ_CNTL_DST_LAST_PEL;
	}

	do
	{
		int x1 = psegsIn->x1 + xorg;
		int y1 = psegsIn->y1 + yorg;
		int x2 = psegsIn->x2 + xorg;
		int y2 = psegsIn->y2 + yorg;
        register int oc1 = 0, oc2 = 0;

		psegsIn++;

		/*
		 * Check the segments against the physical coordinate space.
		 */
		OUTCODE(oc1,x1,y1,
			0, screen_state_p->generic_state.screen_physical_width - 1,
			0, screen_state_p->generic_state.screen_physical_height - 1);
		OUTCODE(oc2,x2,y2,
			0, screen_state_p->generic_state.screen_physical_width - 1,
			0, screen_state_p->generic_state.screen_physical_height - 1);

		if (oc1 & oc2) 	/* completely outside the physical bounds */
		{
#if (defined(__DEBUG__))
			if (m64_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"m64_line_segment_one_bit_solid(%d,%d,%d,%d){\n"
					"\tsegment completely outside physical coords space\n"
					"}\n",
					x1,y1,x2,y2);
			}
#endif
			continue;
		}

		if (oc1 | oc2)
		{
			int 	result;

			/* Reset for further use. */
			oc1 = oc2 = 0;

#if (defined(__DEBUG__))
			if (m64_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"m64_line_segment_one_bit_solid(){\n"
					"\tclipping segment from (%d,%d,%d,%d)\n",
					x1,y1,x2,y2);
			}
#endif
			result = m64_line_clip_endpoints_to_physical_space(&x1,&y1,&x2,&y2);
#if (defined(__DEBUG__))
			if (m64_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"\tto (%d,%d,%d,%d)\n"
					"}\n",
					x1,y1,x2,y2);
			}
#endif
			/* Clipping yeilded a line outside the coordinate space. */
			if (result == FALSE)
			{
				continue;
			}
		}

		/*
		 * Check the segments against the clip rectangle.
		 */
		ASSERT(oc1 == 0 && oc2 == 0);
		OUTCODE(oc1,x1,y1,left_clip,right_clip,top_clip,bottom_clip);
		OUTCODE(oc2,x2,y2,left_clip,right_clip,top_clip,bottom_clip);

		if (oc1 & oc2) 	/* completely outside the clipping rectangle */
		{
#if (defined(__DEBUG__))
			if (m64_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"m64_line_segment_one_bit_solid(%d,%d,%d,%d){\n"
					"\tsegment completely outside clipping rectangle\n"
					"}\n",
					x1,y1,x2,y2);
			}
#endif
			continue;
		}
		
		/*
		 * Draw the line segment from x1,y1 to x2,y2.
		 */
		M64_DRAW_LINE(x1,y1,x2,y2);
	}while(--count);

	/*
	 * Restore last pel flag.
	 */
	register_values_p[M64_REGISTER_GUI_TRAJ_CNTL_OFFSET] &= 
			~(GUI_TRAJ_CNTL_DST_LAST_PEL | GUI_TRAJ_CNTL_SRC_PATT_EN);

	/*
	 * Disable the color compare operations.
	 */
	M64_WAIT_FOR_FIFO(1);
	*(register_base_address_p + M64_REGISTER_CLR_CMP_CNTL_OFFSET) = 0;

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
#if (defined(__DEBUG__))	
	M64_WAIT_FOR_GUI_ENGINE_IDLE();
#endif
	return (SI_SUCCEED);
}

/*
 * m64_one_bit_line_dashed:
 *
 * Semantics of one bit dashed lines for fillmode = fillsolid are 
 * as follows:
 * DoubleDash : The full path of the line is drawn but the even dashes are
 *              filled with foreground color and odd dashes are drawn with
 *              background color. Butt cap-style used where even and odd 
 *              dashes meet. This means that the endpoint of the dashes
 *              is always drawn where they meet and the last endpoint is 
 *              undrawn if isCapNotLast is true.
 * OnOffDash  : Only even dashes are drawn, the cap-style applies to all
 *              internal ends of the individual dashes ( except NotLast
 *              is treated as Butt). This means that the endpoint of the
 *              dash is always drawn and the last endpoint os not drawn if
 *              isCapNotLast is true.
 * Line with coincident endpoints are treated in the following manner:
 * If isCapNotLast nothing is drawn, if not draw a single pixel.  
 * This is device dependent for zero width lines.
 */
STATIC SIBool
m64_line_one_bit_dashed(SIint32 xorg, SIint32 yorg, SIint32 count,
	       SIPointP ptsIn, SIint32 isCapNotLast, SIint32 coordMode)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;

	struct m64_line_state *line_state_p = 
		&(graphics_state_p->current_line_state);
	int  	x1,y1,x2,y2;

	int		unclipped_x1,unclipped_y1;
	int 	oc1 = 0, oc2 = 0;

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT((graphics_state_p->generic_state.si_graphics_state.
		SGlinestyle == SGLineDash)||(graphics_state_p->generic_state.
		si_graphics_state.SGlinestyle == SGLineDblDash));

#if (defined(__DEBUG__))
	if (m64_line_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_line_one_bit_dashed) {\n"
			"\txorg = %ld\n"
			"\tyorg = %ld\n"
			"\tcount = %ld\n"
			"\tpsegsIn = %p\n"
			"\tisCapNotLast = %ld\n"
			"\tcoordMode = %ld\n"
			"}\n",
			xorg, yorg, count, (void *) ptsIn,isCapNotLast,coordMode);
	}
#endif

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));

	if (count <= 1)
	{
		return (SI_SUCCEED);
	}

	/*
	 * Download the line pattern  into offscreen memory if either the
	 * foreground/background colors have changed or the pattern has
	 * not been downloaded at all so far.
	 */
	if ((line_state_p->is_pattern_valid == FALSE) || 
		(graphics_state_p->generic_state.si_graphics_state.SGfg != 
			line_state_p->foreground_color) || 
		(graphics_state_p->generic_state.si_graphics_state.SGbg != 
			line_state_p->background_color))
	{
		line_state_p->is_pattern_valid = FALSE;
		m64_graphics_state_download_line_pattern(screen_state_p, 
			graphics_state_p);
	}

	if (!(OMM_LOCK(line_state_p->allocation_p)) || 
		line_state_p->is_pattern_valid == FALSE)
	{
		return(SI_FAIL);
	}
	ASSERT(line_state_p->is_pattern_valid == TRUE);

	/*
	 * Set the clipping rectangle to graphics state.
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
	 * Set the graphics engine to do dashed lines.
	 * See page 3-13 of the programmers manual.
	 * Assumption is that the dash pattern is maximum 32 bits in length.
	 */
	M64_WAIT_FOR_FIFO(3);
	*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
		(DP_SRC_BLIT_SRC << DP_SRC_DP_FRGD_SRC_SHIFT) |
		DP_SRC_DP_MONO_SRC_ALWAYS_1;
	*(register_base_address_p + M64_REGISTER_SRC_Y_X_OFFSET) = 
		line_state_p->allocation_p->y | 
		((unsigned)(line_state_p->allocation_p->x + 32 - 1) << 
		SRC_Y_X_SRC_X_SHIFT);
	*(register_base_address_p + M64_REGISTER_SRC_HEIGHT1_WID1_OFFSET) = 
		1 | 
		(line_state_p->dash_pattern_length << SRC_HEIGHT1_WID1_SRC_WID1_SHIFT);
	
	/*
	 * For simple dashed lines the off pairs are not drawn. Prohibit this
	 * by the use of the color compare function.
	 */
	if (graphics_state_p->generic_state.si_graphics_state.SGlinestyle == 
		SGLineDash)
	{
		M64_WAIT_FOR_FIFO(3);
		*(register_base_address_p + M64_REGISTER_CLR_CMP_CNTL_OFFSET) = 
			CLR_CMP_CNTL_CLR_CMP_FN_COMPARE_EQUAL | 
			CLR_CMP_CNTL_CLR_CMP_SRC_COMPARE_SOURCE;
		*(register_base_address_p + M64_REGISTER_CLR_CMP_CLR_OFFSET) = 
			*(register_values_p + M64_REGISTER_DP_BKGD_CLR_OFFSET);
		*(register_base_address_p + M64_REGISTER_CLR_CMP_MASK_OFFSET) = 
			(~0U >> (32U - screen_state_p->generic_state.screen_depth));
	}
	/*
	 * GUI Traj cntl register should be programmed with the pattern enable.
	 */
	*(register_values_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) |= 
		GUI_TRAJ_CNTL_SRC_PATT_EN;

	x2 = ptsIn->x + xorg;
	y2 = ptsIn->y + yorg;
	unclipped_x1 = x2;
	unclipped_y1 = y2;
	OUTCODE(oc2,x2,y2,
		0, screen_state_p->generic_state.screen_physical_width - 1,
		0, screen_state_p->generic_state.screen_physical_height - 1);

	do 
	{
		oc1 = oc2;
		oc2 = 0;
		x1 = unclipped_x1;
		y1 = unclipped_y1;
		if( coordMode == SICoordModePrevious)
		{
			xorg = unclipped_x1;
			yorg = unclipped_y1;
		}
		++ptsIn;
		x2 = ptsIn->x + xorg;
		y2 = ptsIn->y + yorg;
		unclipped_x1 = x2;
		unclipped_y1 = y2;

		/*
		 * Check the segments against the physical coordinate space.
		 */
		OUTCODE(oc2,x2,y2,
			0, screen_state_p->generic_state.screen_physical_width - 1,
			0, screen_state_p->generic_state.screen_physical_height - 1);

		if (oc1 & oc2) 	/* completely outside the physical bounds */
		{
#if (defined(__DEBUG__))
			if (m64_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"m64_line_one_bit_solid(%d,%d,%d,%d){\n"
					"\tsegment completely outside physical coords space\n"
					"}\n",
					x1,y1,x2,y2);
			}
#endif
			continue;
		}
		else if (oc1 | oc2)	/* line intersects some physical bound. */
		{
			int 	result;

#if (defined(__DEBUG__))
			if (m64_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"m64_line_one_bit_solid(){\n"
					"\tclipping segment from (%d,%d,%d,%d)\n",
					x1,y1,x2,y2);
			}
#endif
			result = m64_line_clip_endpoints_to_physical_space(&x1,&y1,&x2,&y2);
#if (defined(__DEBUG__))
			if (m64_line_debug)
			{
				(void) fprintf(debug_stream_p, 
					"\tto (%d,%d,%d,%d)\n" 
					"}\n",
					x1,y1,x2,y2);
			}
#endif
			/* Clipping yeilded a line outside the physical coordinate space.*/
			if (result != TRUE)
			{
				continue;
			}
		}
		/* else line is within the physical coordinate space */

		/*
		 * At this point we have some part/whole of the line within 
		 * physical coordinate space. Set the current x and current y
		 * Draw the line from x1,y1 to x2,y2.
		 */
		M64_DRAW_LINE(x1,y1,x2,y2);

	} while( --count > 1);

	/*
	 * Draw the last point of the last line, if CapNotLast is FALSE
	 * and if the last point is within the device coordinate space
	 * and if the ending pixels of the last line were not clipped.
	 */
	if(!isCapNotLast)
	{
		if((!oc2) && (x2 == unclipped_x1) && (y2 == unclipped_y1))
		{
			int	x = x2 + 1;
			int y = y2 + 1;

			M64_DRAW_LINE(x2,y2,x,y);
		}
	}	

	ASSERT(!M64_IS_FIFO_OVERFLOW());

	/*
	 * Restore the GUI traj control value.
	 */
	*(register_values_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) &= 
		~GUI_TRAJ_CNTL_SRC_PATT_EN;
	
	/*
	 * Disable the color compare operations.
	 */
	M64_WAIT_FOR_FIFO(1);
	*(register_base_address_p + M64_REGISTER_CLR_CMP_CNTL_OFFSET) = 0;

#if (defined(__DEBUG__))	
	M64_WAIT_FOR_GUI_ENGINE_IDLE();
#endif
	return (SI_SUCCEED);
}

/*
 * m64_one_bit_rectangle. 
 */
STATIC SIBool
m64_line_rectangle_one_bit(SIint32 x1, SIint32 y1, SIint32 x2, SIint32 y2)
{

	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;

	/*
	 * For dashed lines call the line segment routine.
	 */
	SISegment rect_segs[4];
	int		oc1 = 0,oc2 = 0;
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));

#if (defined(__DEBUG__))
	if (m64_line_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_one_bit_rectangle) {\n"
			"\tx1 = %ld\n"
			"\ty1 = %ld\n"
			"\tx2 = %ld\n"
			"\ty2 = %ld\n"
			"}\n",
			x1, y1, x2, y2);
	}
#endif

	OUTCODE(oc1,x1,y1,
		screen_state_p->generic_state.screen_clip_left,
		screen_state_p->generic_state.screen_clip_right,
		screen_state_p->generic_state.screen_clip_top,
		screen_state_p->generic_state.screen_clip_bottom);
	OUTCODE(oc2,x2,y2,
		screen_state_p->generic_state.screen_clip_left,
		screen_state_p->generic_state.screen_clip_right,
		screen_state_p->generic_state.screen_clip_top,
		screen_state_p->generic_state.screen_clip_bottom);

	if (oc1 & oc2) 	/* completely outside the physical bounds */
	{
#if (defined(__DEBUG__))
		if (m64_line_debug)
		{
			(void) fprintf(debug_stream_p,
				"m64_line_one_rectangle_one_bit(%ld,%ld,%ld,%ld){\n"
				"\trectangle completely outside physical coords space\n"
				"}\n",
				x1,y1,x2,y2);
		}
#endif
		return (SI_SUCCEED) ;
	}

	/*
	 * Call the line segment code if:
	 * Line style is not fill solid, for we do not have dashed lines now.
	 * clipping to physical coordinate space is to be done.
	 */
	if ((graphics_state_p->generic_state.si_graphics_state.SGlinestyle ==
		SGLineDash) ||
		(graphics_state_p->generic_state.si_graphics_state.SGlinestyle ==
		SGLineDblDash) ||
		(oc1 | oc2))
	{
		rect_segs[0].x1 = (SIint16)x1; rect_segs[0].y1 = (SIint16)y1; 
		rect_segs[0].x2 = (SIint16)x2; rect_segs[0].y2 = (SIint16)y1; 
		rect_segs[1].x1 = (SIint16)x2; rect_segs[1].y1 = (SIint16)y1;
		rect_segs[1].x2 = (SIint16)x2; rect_segs[1].y2 = (SIint16)y2; 
		rect_segs[2].x1 = (SIint16)x2; rect_segs[2].y1 = (SIint16)y2; 
		rect_segs[2].x2 = (SIint16)x1; rect_segs[2].y2 = (SIint16)y2;
		rect_segs[3].x1 = (SIint16)x1; rect_segs[3].y1 = (SIint16)y2; 
		rect_segs[3].x2 = (SIint16)x1; rect_segs[3].y2 = (SIint16)y1;
		return (*screen_state_p->generic_state.screen_functions_p->
			si_line_onebitseg)(0, 0, 4, rect_segs, 1);
	}
	else
	{
		/*
		 * Use 4 rectangle fills to draw the rectangle outline.
		 */
		int	width = x2 - x1 + 1; 
		int	height = y2 - y1 + 1;

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

		M64_WAIT_FOR_FIFO(10);
		*(register_base_address_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) = 
			*(register_values_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET);

		*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
			(graphics_state_p->generic_state.si_graphics_state.SGfillmode == 
			SGFillSolidFG ? DP_SRC_FRGD_COLOR << 
			DP_SRC_DP_FRGD_SRC_SHIFT : 0) | DP_SRC_DP_MONO_SRC_ALWAYS_1 ;

		ASSERT(width > 0 && height > 0);

		/*
		 * Horizontal lines drawn left to right. 
		 */
		*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = 
			(unsigned)y1 | ((unsigned)x1 << DST_Y_X_DST_X_SHIFT);
		*(register_base_address_p + M64_REGISTER_DST_HEIGHT_WID_OFFSET) = 
			1U | (width << DST_HEIGHT_WID_DST_WID_SHIFT);

		*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = 
			(unsigned)y2 | ((unsigned)x1 << DST_Y_X_DST_X_SHIFT);
		*(register_base_address_p + M64_REGISTER_DST_HEIGHT_WID_OFFSET) = 
			1U | (width << DST_HEIGHT_WID_DST_WID_SHIFT);

		if (height > 2)
		{
			*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = 
				(unsigned)(y1 + 1) | ((unsigned)x1 << DST_Y_X_DST_X_SHIFT);
			*(register_base_address_p + M64_REGISTER_DST_HEIGHT_WID_OFFSET) = 
				(height - 2) | (1U << DST_HEIGHT_WID_DST_WID_SHIFT);
			*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = 
				(unsigned)(y1 + 1) | ((unsigned)x2 << DST_Y_X_DST_X_SHIFT);
			*(register_base_address_p + M64_REGISTER_DST_HEIGHT_WID_OFFSET) = 
				(height - 2) | (1U << DST_HEIGHT_WID_DST_WID_SHIFT);
		}
	}
	
	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
#if (defined(__DEBUG__))	
	M64_WAIT_FOR_GUI_ENGINE_IDLE();
#endif
	return (SI_SUCCEED);
}

STATIC SIBool
m64_line_compat_one_bit_solid(SIint32 count,  SIPointP ptsIn)
{
	/*
	 * isCapNotLast is TRUE and coordmode is SICoordModeOrigin.
	 */
	return(m64_line_one_bit_solid(0, 0, count, ptsIn, TRUE ,
		SICoordModeOrigin));
}

STATIC SIBool
m64_line_compat_one_bit_dashed(SIint32 count,  SIPointP ptsIn)
{
	/*
	 * isCapNotLast is TRUE and coordmode is SICoordModeOrigin.
	 */
	return(m64_line_one_bit_dashed(0, 0, count, ptsIn, TRUE ,
		SICoordModeOrigin));
}

STATIC SIBool
m64_line_segment_compat_one_bit_solid(SIint32 count, SIPointP ptsIn)
{
	return(m64_line_segment_one_bit_solid(0, 0, count >> 1, 
			(SISegmentP) ptsIn, FALSE));
}

STATIC SIBool
m64_line_segment_compat_one_bit_dashed(SIint32 count, SIPointP ptsIn)
{
	return(m64_line_segment_one_bit_dashed(0, 0, count >> 1, 
			(SISegmentP) ptsIn, FALSE));
}

/*
 * Graphics state change.
 * 
 * We change the function pointers depending on whether the line style
 * is solid or dashed.
 */

function void
m64_line__gs_change__(void)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));
	
	screen_state_p->generic_state.screen_functions_p->si_line_onebitline =
		(SIBool (*)(SIint32, SIint32, SIint32, SIPointP, SIint32, SIint32))
		m64_no_operation_fail;
	screen_state_p->generic_state.screen_functions_p->si_line_onebitseg = 
		(SIBool (*)(SIint32, SIint32, SIint32, SISegmentP , SIint32))
		m64_no_operation_fail;
	screen_state_p->generic_state.screen_functions_p->si_line_onebitrect = 
		(SIBool (*)(SIint32, SIint32, SIint32, SIint32))
		m64_no_operation_fail;

	/*
	 * We dont handle lines that have fill styles other than fillsolid. 
	 */
	if (!((graphics_state_p->generic_state.si_graphics_state.SGfillmode
		 == SGFillSolidFG) ||
		(graphics_state_p->generic_state.si_graphics_state.SGfillmode
		 == SGFillSolidBG)))
	{
		return;
	}

	/*
	 * Line rectangles are the same for the old and new servers.
	 */
	if(screen_state_p->options_p->linedraw_options & 
		  M64_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_RECTANGLES)
	{
		screen_state_p->generic_state.screen_functions_p->
			si_line_onebitrect = m64_line_rectangle_one_bit;
	}

	/*
	 * New SDD's calling sequence.
	 */
	if (screen_state_p->generic_state.screen_sdd_version_number >=
		DM_SI_VERSION_1_1)
	{

		if (graphics_state_p->generic_state.si_graphics_state.SGlinestyle
			 == SGLineSolid)
		{
			if (screen_state_p->options_p->linedraw_options &
				M64_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_DRAW)
			{
				screen_state_p->generic_state.screen_functions_p->
					si_line_onebitline = m64_line_one_bit_solid;
			}
			if (screen_state_p->options_p->linedraw_options &
				M64_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW)
			{
				screen_state_p->generic_state.screen_functions_p->
					si_line_onebitseg =  m64_line_segment_one_bit_solid;
			}
		}
		else if ((screen_state_p->options_p->linedraw_options & 
				  M64_OPTIONS_LINEDRAW_OPTIONS_USE_DASHED_LINE) &&
				 ((graphics_state_p->
				   generic_state.si_graphics_state.SGlinestyle ==
				   SGLineDash) ||
				  (graphics_state_p->
				   generic_state.si_graphics_state.SGlinestyle ==
				   SGLineDblDash)))
		{
			screen_state_p->generic_state.screen_functions_p->
				si_line_onebitline = m64_line_one_bit_dashed;
			screen_state_p->generic_state.screen_functions_p->
				si_line_onebitseg = m64_line_segment_one_bit_dashed;
		}
	}
	else
	{
		/*
		 * Handle backward compatibility.
		 */

		/*
		 * compensate for a bug in SI : the SGlinestyle field is
		 * sometimes `0' an illegal value.   Set this to
		 * SGLineSolid, to allow the rest of the code to work.
		 */
		if (graphics_state_p->generic_state.si_graphics_state.SGlinestyle == 0)
		{
			graphics_state_p->generic_state.si_graphics_state.SGlinestyle =
				SGLineSolid;
		}
		
		if (graphics_state_p->generic_state.si_graphics_state.SGlinestyle
			== SGLineSolid)
		{
			if (screen_state_p->options_p->linedraw_options &
				M64_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_DRAW)
			{
				screen_state_p->generic_state.screen_functions_p->
					si_line_onebitline =
					(SIBool (*)(SIint32, SIint32, SIint32, SIPointP,
					SIint32, SIint32)) m64_line_compat_one_bit_solid;
			}
			if (screen_state_p->options_p->linedraw_options &
				M64_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW)
			{
				screen_state_p->generic_state.screen_functions_p->
					si_line_onebitseg =  
					(SIBool (*)(SIint32, SIint32, 
							SIint32, SISegmentP, SIint32))
					m64_line_segment_compat_one_bit_solid;
			}
		}
		else if ((screen_state_p->options_p->linedraw_options & 
				  M64_OPTIONS_LINEDRAW_OPTIONS_USE_DASHED_LINE) &&
				 ((graphics_state_p->
				   generic_state.si_graphics_state.SGlinestyle ==
				   SGLineDash) ||
				  (graphics_state_p-> 
				   generic_state.si_graphics_state.SGlinestyle ==
				   SGLineDblDash)))
		{
			/* NO dashed, double dashed lines yet.  */
			if (screen_state_p->options_p->linedraw_options &
				M64_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW)
			{
				screen_state_p->generic_state.screen_functions_p->
					si_line_onebitseg =  
					(SIBool (*)(SIint32, SIint32, 
							SIint32, SISegmentP, SIint32))
					m64_line_segment_compat_one_bit_dashed;
			}
		}
	}
	return;
}

function void
m64_line__initialize__(SIScreenRec *si_screen_p,
						struct m64_options_structure * options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;

	if (options_p->linedraw_options &
		M64_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_DRAW)
	{
		flags_p->SIavail_line |= ONEBITLINE_AVAIL;
	}
	if (options_p->linedraw_options &
		M64_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_RECTANGLES)
	{
		flags_p->SIavail_line |= ONEBITRECT_AVAIL;
	}
	if (options_p->linedraw_options &
		M64_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW)
	{
		flags_p->SIavail_line |= ONEBITSEG_AVAIL;
	}
	if (options_p->linedraw_options &
		M64_OPTIONS_LINEDRAW_OPTIONS_USE_DASHED_LINE)
	{
		flags_p->SIavail_line |= (DASH_AVAIL | DBLDASH_AVAIL);
	}
	return;
}
