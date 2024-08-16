/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m_line.c	1.7"

/***
 ***	NAME
 ***
 ***		mach_line.c : Line drawing code for the MACH display
 ***					  library.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "m_line.h"
 ***
 ***	DESCRIPTION
 ***
 ***	This module implements zero-width line, segment and rectangle
 ***	drawing for the MACH display library.
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
export boolean mach_line_debug = FALSE;
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

#define MACH_SOLID_LINE_DEPENDENCIES\
	(MACH_INVALID_FG_ROP|\
	 MACH_INVALID_FOREGROUND_COLOR|\
	 MACH_INVALID_CLIP|\
	 MACH_INVALID_WRT_MASK)

#define MACH_DASHED_LINE_DEPENDENCIES\
	(MACH_INVALID_FG_ROP|\
	 MACH_INVALID_FOREGROUND_COLOR|\
	 MACH_INVALID_CLIP|\
	 MACH_INVALID_WRT_MASK|\
	 MACH_INVALID_PATTERN_REGISTERS)

#define MACH_DOUBLE_DASHED_LINE_DEPENDENCIES\
	(MACH_INVALID_FG_ROP|\
	 MACH_INVALID_BG_ROP|\
	 MACH_INVALID_FOREGROUND_COLOR|\
	 MACH_INVALID_BACKGROUND_COLOR|\
	 MACH_INVALID_CLIP|\
	 MACH_INVALID_WRT_MASK|\
	 MACH_INVALID_PATTERN_REGISTERS)

#define MACH_LINE_LOCAL_BUFFER_SIZE\
	(2 * DEFAULT_MACH_MAX_FIFO_BLOCKING_FACTOR)

/***
 ***	Macros.
 ***/

#if ((DEFAULT_MACH_MAX_FIFO_BLOCKING_FACTOR - 1) &\
	 DEFAULT_MACH_MAX_FIFO_BLOCKING_FACTOR)
#error "fifo blocking factor not a power of two!."
#endif /* DEFAULT_MAX_FIFO_BLOCKING_FACTOR */


#define MACH_LINE_DRAW(X_ORG, Y_ORG, SI_POINTS_P, COUNT, OPERATION)\
{\
	unsigned short __computed_points[MACH_LINE_LOCAL_BUFFER_SIZE];\
	register SIPointP __si_points_p = (SI_POINTS_P);\
	const SIPointP __si_points_last_fence_p =\
		__si_points_p + (COUNT);\
	const int __blocking_factor =\
		mach_graphics_engine_fifo_blocking_factor >> 1;\
	const SIPointP __si_points_first_fence_p =\
		__si_points_p + \
		((__blocking_factor & (__blocking_factor - 1)) ?\
		 ((COUNT) / __blocking_factor) : \
		 ((COUNT) & ~(__blocking_factor - 1)));\
	if (__si_points_first_fence_p > __si_points_p)\
	{\
		 const unsigned short *__tmp_fence = __computed_points +\
			 mach_graphics_engine_fifo_blocking_factor;\
		do\
		{\
			register unsigned short *__tmp_p = __computed_points;\
			do\
			{\
				*__tmp_p++ = ((X_ORG) OPERATION __si_points_p->x);\
				*__tmp_p = ((Y_ORG) OPERATION __si_points_p++->y);\
			} while (++__tmp_p < __tmp_fence);\
			mach_register_wait_for_fifo(\
				mach_graphics_engine_fifo_blocking_factor);\
			mach_asm_inline_repz_outw(MACH_REGISTER_LINEDRAW,\
				mach_graphics_engine_fifo_blocking_factor,\
				__computed_points);\
		} while (__si_points_p < __si_points_first_fence_p);\
	}\
	if (__si_points_last_fence_p > __si_points_first_fence_p)\
	{\
		register int __excess_count =\
			(__si_points_last_fence_p - __si_points_first_fence_p) << 1;\
		register unsigned short *__tmp_p = __computed_points;\
		const unsigned short *__tmp_fence = __computed_points +\
			__excess_count;\
		do\
		{\
			*__tmp_p++ = ((X_ORG) OPERATION __si_points_p->x);\
			*__tmp_p = ((Y_ORG) OPERATION __si_points_p++->y);\
		} while (++__tmp_p < __tmp_fence);\
		mach_register_wait_for_fifo(__excess_count);\
		mach_asm_inline_repz_outw(MACH_REGISTER_LINEDRAW,\
						   __excess_count, __computed_points);\
	}\
}

/*
 * Macros useful when checking segments against clip bounds.
 */

#define OUT_LEFT	0x08
#define	OUT_RIGHT	0x04
#define	OUT_ABOVE	0x02
#define OUT_BELOW	0x01

#define OUTCODE(result, x, y, left, right, top, bottom)\
			result = 0; \
			if (x < left) \
				result |= OUT_LEFT; \
			else if (x > right) \
				result |= OUT_RIGHT;\
			if (y < top)\
				result |= OUT_ABOVE;\
			else if (y > bottom)\
				result |= OUT_BELOW;

/***
 ***	Functions.
 ***/


/*
 * mach_line_segment_clip_segment
 * 
 * Clip a segment to the clip rectangle.  The segment is drawn from
 * (xl,yt) to (xr,yb).
 *
 */

SISegmentP
mach_line_segment_clip_segment(
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
		int oc;
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
	
	clip_segment_p->x1 = xl;
	clip_segment_p->y1 = yt;
	clip_segment_p->x2 = xr;
	clip_segment_p->y2 = yb;

	return clip_segment_p;

}

#define MACH_LINE_SET_LINEDRAW_OPT(value)					\
if (saved_linedraw_opt != (linedraw_opt | (value)))			\
{															\
	MACH_WAIT_FOR_FIFO(1);									\
	outw(MACH_REGISTER_LINEDRAW_OPT, saved_linedraw_opt = 	\
		 (linedraw_opt | (value)));							\
}


#define MACH_LINE_DRAW_ATI_SEGMENT(xl, yt, xr, yb, isCapNotLast, linedraw_opt)\
{																			\
	if ((xl) == (xr))														\
	{																		\
		if ((yb) == (yt))													\
		{																	\
			if (!(isCapNotLast))											\
			{																\
				MACH_LINE_SET_LINEDRAW_OPT(									\
					   MACH_LINEDRAW_OPT_DIR_TYPE_DEGREE |					\
					   MACH_LINEDRAW_OPT_0_DEGREE_RADIAL_LINE);				\
				MACH_WAIT_FOR_FIFO(1);										\
				outw(MACH_REGISTER_BRES_COUNT, 1);							\
			}																\
		}																	\
		else if ((yb) < (yt))												\
		{																	\
			MACH_LINE_SET_LINEDRAW_OPT(										\
			   MACH_LINEDRAW_OPT_DIR_TYPE_DEGREE |							\
			   MACH_LINEDRAW_OPT_90_DEGREE_RADIAL_LINE);					\
			MACH_WAIT_FOR_FIFO(1);											\
			outw(MACH_REGISTER_BRES_COUNT, (yt) - (yb));					\
		}																	\
		else																\
		{																	\
			MACH_LINE_SET_LINEDRAW_OPT(										\
			   MACH_LINEDRAW_OPT_DIR_TYPE_DEGREE |							\
			   MACH_LINEDRAW_OPT_270_DEGREE_RADIAL_LINE);					\
			MACH_WAIT_FOR_FIFO(1);											\
			outw(MACH_REGISTER_BRES_COUNT, (yb) - (yt));					\
		}																	\
	} 																		\
	else if ((yt) == (yb))													\
	{																		\
		if ((xr) < (xl))													\
		{																	\
			MACH_LINE_SET_LINEDRAW_OPT(										\
			   MACH_LINEDRAW_OPT_DIR_TYPE_DEGREE |							\
			   MACH_LINEDRAW_OPT_180_DEGREE_RADIAL_LINE);					\
			MACH_WAIT_FOR_FIFO(1);											\
			outw(MACH_REGISTER_BRES_COUNT, (xl) - (xr));					\
		}																	\
		else 																\
		{																	\
			MACH_LINE_SET_LINEDRAW_OPT(										\
			   MACH_LINEDRAW_OPT_DIR_TYPE_DEGREE |							\
			   MACH_LINEDRAW_OPT_0_DEGREE_RADIAL_LINE);						\
			MACH_WAIT_FOR_FIFO(1);											\
			outw(MACH_REGISTER_BRES_COUNT, (xr) - (xl));					\
		}																	\
	}																		\
	else																	\
	{																		\
		int adx;															\
		int ady;															\
		int error_term;														\
		int max_term, min_term;												\
		int linedraw_command = 0;											\
		if ((xr) > (xl))													\
		{																	\
			adx = ((xr) - (xl));											\
			linedraw_command |= MACH_LINEDRAW_OPT_XPOS;						\
		} 																	\
		else 																\
		{																	\
			adx = ((xl) - (xr));											\
		}																	\
		if ((yb) > (yt))													\
		{																	\
			ady = ((yb) - (yt));											\
			linedraw_command |= MACH_LINEDRAW_OPT_YPOS;						\
		}																	\
		else 																\
		{																	\
			ady = ((yt) - (yb));											\
		}																	\
		ASSERT(ady >= 0 && adx >= 0);										\
		if (ady > adx)														\
		{																	\
			linedraw_command |= MACH_LINEDRAW_OPT_YMAJOR;					\
			min_term = adx;													\
			max_term = ady;													\
		}																	\
		else																\
		{																	\
			min_term = ady;													\
			max_term = adx;													\
		}																	\
		error_term = ((xl) < (xr)) ? ((2 * min_term) - max_term - 1) :		\
		(2 * min_term - max_term);											\
		ASSERT(error_term >= -4096 && error_term <= 4095);					\
		MACH_LINE_SET_LINEDRAW_OPT(linedraw_command);						\
		MACH_WAIT_FOR_FIFO(4);												\
		outw(MACH_REGISTER_BRES_INCR1, 2 * min_term);						\
		outw(MACH_REGISTER_BRES_INCR2, 2 * (min_term - max_term));			\
		outw(MACH_REGISTER_BRES_D, error_term);								\
		outw(MACH_REGISTER_BRES_COUNT, max_term);							\
	}																		\
}
	
#define MACH_LINE_SET_LENGTH(length)			\
if (saved_length != (length))					\
{												\
	MACH_WAIT_FOR_FIFO(1);						\
	outw(MACH_REGISTER_MAJ_AXIS_PCNT, 			\
		 saved_length = (length));				\
}

#define MACH_LINE_DRAW_IBM_SEGMENT(xl, yt, xr, yb, isCapNotLast)	\
{																	\
	if (xl == xr)													\
	{																\
		register unsigned short draw_command =						\
			radial_line_command;									\
		register int length;										\
		if (yb == yt)												\
		{															\
			if (!isCapNotLast)										\
			{														\
				MACH_LINE_SET_LENGTH(0);							\
				MACH_WAIT_FOR_FIFO(1);								\
				outw(MACH_REGISTER_CMD, draw_command |				\
					 MACH_CMD_0_DEGREE_RADIAL_LINE);				\
			}														\
		}															\
		else														\
		{															\
			ASSERT(yb != yt);										\
			if (yb < yt)											\
			{														\
				length = yt - yb;									\
				draw_command |= MACH_CMD_90_DEGREE_RADIAL_LINE;		\
			}														\
			else													\
			{														\
				length = yb - yt;									\
				draw_command |= MACH_CMD_270_DEGREE_RADIAL_LINE;	\
			}														\
			ASSERT(length >= 0);									\
			MACH_LINE_SET_LENGTH(length);							\
			MACH_WAIT_FOR_FIFO(1);									\
			outw(MACH_REGISTER_CMD, draw_command);					\
		}															\
	}																\
	else if (yt == yb)												\
	{																\
		register unsigned short draw_command =						\
			radial_line_command;									\
		register int length;										\
		ASSERT(xl != xr);											\
		if (xr < xl)												\
		{															\
			length = xl - xr;										\
			draw_command |= MACH_CMD_180_DEGREE_RADIAL_LINE;		\
		}															\
		else														\
		{															\
			length = xr - xl;										\
			draw_command |= MACH_CMD_0_DEGREE_RADIAL_LINE;			\
		}															\
		ASSERT(length >= 0);										\
		MACH_LINE_SET_LENGTH(length);								\
		MACH_WAIT_FOR_FIFO(1);										\
		outw(MACH_REGISTER_CMD, draw_command);						\
	}																\
	else															\
	{																\
		MACH_WAIT_FOR_FIFO(5);										\
		outw(MACH_REGISTER_LINEDRAW_INDEX, 0);						\
		outw(MACH_REGISTER_LINEDRAW, xl);							\
		outw(MACH_REGISTER_LINEDRAW, yt);							\
		outw(MACH_REGISTER_LINEDRAW, xr);							\
		outw(MACH_REGISTER_LINEDRAW, yb);							\
	}																\
}

																			
/*																			
 * mach_one_bit_segment_one_bit_helper										
 * 																			
 * Helper function for segment drawing for the MACH library.				
 */																			
 																			
STATIC void
mach_line_segment_one_bit_helper(register SIint32 xorg, 
								 register SIint32 yorg, 
								 register SISegmentP psegsIn, 
								 register SIint32 count,
								 const int isCapNotLast)
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();
	
	const int top_clip =
		screen_state_p->generic_state.screen_clip_top;
	const int bottom_clip =
		screen_state_p->generic_state.screen_clip_bottom;
	const int left_clip =
		screen_state_p->generic_state.screen_clip_left;
	const int right_clip =
		screen_state_p->generic_state.screen_clip_right;

	const unsigned short linedraw_opt =	/* constant part of linedraw_opt */
		(isCapNotLast ? MACH_LINEDRAW_OPT_LAST_PEL_OFF : 0);

	int saved_linedraw_opt = -1; /* illegal value */

	boolean use_ibm_mode =
		((screen_state_p->options_p->linedraw_options &
		 MACH_OPTIONS_LINEDRAW_OPTIONS_USE_IBM_MODE) &&
		(screen_state_p->generic_state.screen_depth != 16)) 
		 ? TRUE : FALSE;
	
	ASSERT(screen_state_p->current_graphics_engine_mode ==
		   MACH_GE_MODE_ATI_MODE);

	if (right_clip <= DEFAULT_MACH_MAX_IBM_LEFT_X &&
		graphics_state_p->generic_state.si_graphics_state.SGlinestyle ==
		SGLineSolid && (use_ibm_mode == TRUE))
	{
		/*
		 * Since the drawing falls in the IBM coordinate space,
		 * we have the possibility of using the faster IBM mode of
		 * drawing. 
		 *
		 * The PIXEL_CNTL register is already set for color writes
		 * (the default).  The FRGD_MIX register is already set for
		 * drawing the foreground color.
		 */
		
		int saved_length = -1; /* illegal value */
			
		const unsigned short radial_line_command =
			(MACH_CMD_WRITE |
			 MACH_CMD_PIXEL_MODE_NIBBLE |
			 (isCapNotLast ? MACH_CMD_LAST_PEL_OFF : 0) |
			 MACH_CMD_DIR_TYPE_DEGREE |
			 MACH_CMD_DRAW |
			 MACH_CMD_DATA_WIDTH_USE_16 |
			 MACH_CMD_LSB_FIRST |
			 MACH_CMD_DRAW_LINE_CMD);
		
		/*
		 * Program the linedraw opt register correctly as we may need to
		 * use the ATi registers if the line is not horizontal or
		 * vertical. 
		 */
		
		MACH_WAIT_FOR_FIFO(1);
		outw(MACH_REGISTER_LINEDRAW_OPT, linedraw_opt);
		
		while (count--)
		{
			register int xl = xorg + psegsIn->x1;
			register int xr = xorg + psegsIn->x2;
			register int yt = yorg + psegsIn->y1;
			register int yb = yorg + psegsIn->y2;
			register int oc1, oc2;
			
			OUTCODE(oc1, xl, yt, left_clip, right_clip, top_clip, bottom_clip);
			OUTCODE(oc2, xr, yb, left_clip, right_clip, top_clip, bottom_clip);

			if (oc1 & oc2)
			{
#if (defined(__DEBUG__))
				if (mach_line_debug)
				{
					(void) fprintf(debug_stream_p,
								   "\t\t(out of clip bounds)\n");
				}
#endif
				psegsIn++;
				continue;
			}
#if (defined(__DEBUG__))
			else
			{
				if (mach_line_debug)
				{
				
					(void) fprintf(debug_stream_p,
"\t{\n"
"\t\t%d,%d -> %d,%d\n"
"\t}\n",
								   xl, yt, xr, yb);
				}
			}
#endif

			/*
			 * Check if the line lies outside the device coordinate
			 * space, we need to further clip it
			 */
			
			OUTCODE(oc1, xl, yt, DEFAULT_MACH_MIN_ATI_COORDINATE,
					DEFAULT_MACH_MAX_ATI_COORDINATE,
					DEFAULT_MACH_MIN_ATI_COORDINATE,
					DEFAULT_MACH_MAX_ATI_COORDINATE);
			OUTCODE(oc2, xr, yb, DEFAULT_MACH_MIN_ATI_COORDINATE,
					DEFAULT_MACH_MAX_ATI_COORDINATE,
					DEFAULT_MACH_MIN_ATI_COORDINATE,
					DEFAULT_MACH_MAX_ATI_COORDINATE);

			if (oc1 | oc2)
			{
				/*
				 * Need to pre-clip the line to within the device
				 * coordinate space.
				 */
				
				SISegment clipped_segment;
				
				if (mach_line_segment_clip_segment(xl, yt,
					 xr, yb, left_clip, top_clip, right_clip,
				     bottom_clip, &clipped_segment))
				{
					
					xl = clipped_segment.x1;
					yt = clipped_segment.y1;
					xr = clipped_segment.x2;
					yb = clipped_segment.y2;
				}
				else
				{
					/*
					 * Nothing to draw.
					 */

					psegsIn++;
					continue;
				}
			}
			

			ASSERT((xl >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
				   (xl <= DEFAULT_MACH_MAX_ATI_COORDINATE));
			ASSERT((xr >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
				   (xr <= DEFAULT_MACH_MAX_ATI_COORDINATE));
			ASSERT((yt >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
				   (yt <= DEFAULT_MACH_MAX_ATI_COORDINATE));
			ASSERT((yb >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
				   (yb <= DEFAULT_MACH_MAX_ATI_COORDINATE));

			/*
			 * Program the CUR_X and CUR_Y registers.
			 */

			MACH_WAIT_FOR_FIFO(2);
			
			outw(MACH_REGISTER_CUR_X, xl);
			outw(MACH_REGISTER_CUR_Y, yt);
			
			/*
			 * Draw the line segment.
			 */

			MACH_LINE_DRAW_IBM_SEGMENT(xl, yt, xr, yb, isCapNotLast);

			psegsIn++;
		}
	}
	else
	{
		/*
		 * Ati mode operations are necessary here as the drawing
		 * engine needs to access outside the IBM coordinate space.
		 */
		
		while (count--)
		{
			register int xl = xorg + psegsIn->x1;
			register int xr = xorg + psegsIn->x2;
			register int yt = yorg + psegsIn->y1;
			register int yb = yorg + psegsIn->y2;
			register int oc1, oc2;
			
			OUTCODE(oc1, xl, yt, left_clip, right_clip, top_clip, bottom_clip);
			OUTCODE(oc2, xr, yb, left_clip, right_clip, top_clip, bottom_clip);

			if (oc1 & oc2)
			{
#if (defined(__DEBUG__))
				if (mach_line_debug)
				{
					(void) fprintf(debug_stream_p,
								   "\t\t(out of clip bounds)\n");
				}
#endif
				psegsIn++;
				continue;
			}
#if (defined(__DEBUG__))
			else
			{
				if (mach_line_debug)
				{
				
					(void) fprintf(debug_stream_p,
"\t{\n"
"\t\t%d,%d -> %d,%d\n"
"\t}\n",
								   xl, yt, xr, yb);
				}
			}
#endif
			
			/*
			 * Check if the line lies outside the device coordinate
			 * space, we need to further clip it
			 */
			
			OUTCODE(oc1, xl, yt, DEFAULT_MACH_MIN_ATI_COORDINATE,
					DEFAULT_MACH_MAX_ATI_COORDINATE,
					DEFAULT_MACH_MIN_ATI_COORDINATE,
					DEFAULT_MACH_MAX_ATI_COORDINATE);
			OUTCODE(oc2, xr, yb, DEFAULT_MACH_MIN_ATI_COORDINATE,
					DEFAULT_MACH_MAX_ATI_COORDINATE,
					DEFAULT_MACH_MIN_ATI_COORDINATE,
					DEFAULT_MACH_MAX_ATI_COORDINATE);

			if (oc1 | oc2)
			{
				/*
				 * Need to pre-clip the line to within the device
				 * coordinate space.
				 */
				
				SISegment clipped_segment;
				
				if(mach_line_segment_clip_segment(xl, yt,
					 xr, yb, left_clip, top_clip, right_clip,
				     bottom_clip, &clipped_segment))
				{
					xl = clipped_segment.x1;
					yt = clipped_segment.y1;
					xr = clipped_segment.x2;
					yb = clipped_segment.y2;
				}
				else
				{
					/*
					 * Nothing to be drawn.
					 */
					psegsIn++;
					continue;
				}
			}
			

			ASSERT((xl >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
				   (xl <= DEFAULT_MACH_MAX_ATI_COORDINATE));
			ASSERT((xr >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
				   (xr <= DEFAULT_MACH_MAX_ATI_COORDINATE));
			ASSERT((yt >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
				   (yt <= DEFAULT_MACH_MAX_ATI_COORDINATE));
			ASSERT((yb >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
				   (yb <= DEFAULT_MACH_MAX_ATI_COORDINATE));

			MACH_WAIT_FOR_FIFO(3);
		
			outw(MACH_REGISTER_PATT_INDEX, 0);
			outw(MACH_REGISTER_CUR_X, xl);
			outw(MACH_REGISTER_CUR_Y, yt);

			MACH_LINE_DRAW_ATI_SEGMENT(xl,yt,xr,yb,isCapNotLast,linedraw_opt);
		
			psegsIn ++;
		}
	}
}

/*
 * mach_line_one_bit_helper
 * 
 * A small helper function to draw lines : all line drawing operations
 * have the same manner of drawing through the LINEDRAW register.
 * This helper function helps to avoid duplication of this code.
 */

STATIC void
mach_line_one_bit_helper(register SIint32 xorg, 
						 register SIint32 yorg, 
						 register SIPointP ptsIn, 
						 register SIint32 count,
						 SIint32 coordMode,
						 SIint32 isCapNotLast)

{

	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	int oc1_unclipped, oc2_unclipped;
	int oc1_clipped, oc2_clipped;
	int x1, y1, x2, y2;
	int x_unclipped, y_unclipped;
	register int tmp_count;
	boolean is_graphics_engine_out_of_sync;	/* does the graphics
											   engine have the correct
											   x1, y1 */
	const int top_clip =
		screen_state_p->generic_state.screen_clip_top;
	const int bottom_clip =
		screen_state_p->generic_state.screen_clip_bottom;
	const int left_clip =
		screen_state_p->generic_state.screen_clip_left;
	const int right_clip =
		screen_state_p->generic_state.screen_clip_right;

	
	ASSERT(screen_state_p->current_graphics_engine_mode ==
		   MACH_GE_MODE_ATI_MODE);

	ASSERT(count > 1);
	
	switch(coordMode)
	{
	case SICoordModePrevious:

		x_unclipped = (xorg += ptsIn->x);
		y_unclipped = (yorg += ptsIn++->y);

		OUTCODE(oc1_unclipped, x_unclipped, y_unclipped, left_clip,
				right_clip, top_clip, bottom_clip);
		oc2_unclipped = oc1_unclipped;
		
		is_graphics_engine_out_of_sync = TRUE;

		tmp_count = count - 1;
		
		while (--tmp_count > 0)
		{
			x1 = x_unclipped; 
			y1 = y_unclipped; 
			oc1_unclipped = oc2_unclipped;
			x2 = x_unclipped = (xorg += ptsIn->x); 
			y2 = y_unclipped = (yorg += ptsIn++->y);
			
			OUTCODE(oc2_unclipped, x2, y2, left_clip, right_clip, top_clip,
					bottom_clip);

			if (oc1_unclipped & oc2_unclipped)
			{
#if (defined(__DEBUG__))
				if (mach_line_debug)
				{
					(void) fprintf(debug_stream_p,
								   "\t\t(out of clip bounds)\n");
				}
#endif

				is_graphics_engine_out_of_sync = TRUE;

				continue;

			}
#if (defined(__DEBUG__))
			else
			{
				if (mach_line_debug)
				{
					(void) fprintf(debug_stream_p,
"\t{\n"
"\t\t%d,%d -> %d,%d\n"
"\t}\n",
								   x1, y1, x2, y2);
				}
			}
#endif

			/*
			 * Check if the line lies outside the device coordinate
			 * space, we need to further clip it
			 */
			
			OUTCODE(oc1_clipped, x1, y1, DEFAULT_MACH_MIN_ATI_COORDINATE,
					DEFAULT_MACH_MAX_ATI_COORDINATE,
					DEFAULT_MACH_MIN_ATI_COORDINATE,
					DEFAULT_MACH_MAX_ATI_COORDINATE);
			OUTCODE(oc2_clipped, x2, y2, DEFAULT_MACH_MIN_ATI_COORDINATE,
					DEFAULT_MACH_MAX_ATI_COORDINATE,
					DEFAULT_MACH_MIN_ATI_COORDINATE,
					DEFAULT_MACH_MAX_ATI_COORDINATE);

			if (oc1_clipped | oc2_clipped)
			{
				/*
				 * Need to pre-clip the line to within the device
				 * coordinate space.
				 */
				
				SISegment clipped_segment;
			
				if (mach_line_segment_clip_segment(x1, y1,
					x2, y2, left_clip, top_clip, right_clip,
					bottom_clip, &clipped_segment))
				{
					x1 = clipped_segment.x1;
					y1 = clipped_segment.y1;
					x2 = clipped_segment.x2;
					y2 = clipped_segment.y2;

					MACH_WAIT_FOR_FIFO(5);
					outw(MACH_REGISTER_LINEDRAW_INDEX, 0);
					outw(MACH_REGISTER_LINEDRAW, x1);
					outw(MACH_REGISTER_LINEDRAW, y1);
					outw(MACH_REGISTER_LINEDRAW, x2);
					outw(MACH_REGISTER_LINEDRAW, y2);
					
					is_graphics_engine_out_of_sync = TRUE;
					
				}

				continue;		/* next point */
			}
			
			ASSERT((x1 >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
				   (x1 <= DEFAULT_MACH_MAX_ATI_COORDINATE));
			ASSERT((x2 >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
				   (x2 <= DEFAULT_MACH_MAX_ATI_COORDINATE));
			ASSERT((y1 >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
				   (y1 <= DEFAULT_MACH_MAX_ATI_COORDINATE));
			ASSERT((y2 >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
				   (y2 <= DEFAULT_MACH_MAX_ATI_COORDINATE));

			/*
			 * Check if the graphics engine has been correctly
			 * programmed.
			 */

			if (is_graphics_engine_out_of_sync == TRUE)
			{
				is_graphics_engine_out_of_sync = FALSE;
				
				MACH_WAIT_FOR_FIFO(3);
				outw(MACH_REGISTER_LINEDRAW_INDEX, 0);
				outw(MACH_REGISTER_LINEDRAW, x1);
				outw(MACH_REGISTER_LINEDRAW, y1);
			}
			
			/*
			 * Draw the line.
			 */

			MACH_WAIT_FOR_FIFO(2);
			
			outw(MACH_REGISTER_LINEDRAW, x2);
			outw(MACH_REGISTER_LINEDRAW, y2);
	
		}
		
		oc1_unclipped = oc2_unclipped;

		break;
		
	case SICoordModeOrigin:

		x_unclipped = xorg + ptsIn->x;
		y_unclipped = yorg + ptsIn++->y;

		OUTCODE(oc1_unclipped, x_unclipped, y_unclipped, left_clip,
				right_clip, top_clip, bottom_clip);
		oc2_unclipped = oc1_unclipped;
		
		is_graphics_engine_out_of_sync = TRUE;
		
		tmp_count = count - 1;
		
		while (--tmp_count > 0)
		{
			x1 = x_unclipped; 
			y1 = y_unclipped; 
			oc1_unclipped = oc2_unclipped;
			x2 = x_unclipped = xorg + ptsIn->x; 
			y2 = y_unclipped = yorg + ptsIn++->y;
			
			OUTCODE(oc2_unclipped, x2, y2, left_clip, right_clip, top_clip,
					bottom_clip);

			if (oc1_unclipped & oc2_unclipped)
			{
#if (defined(__DEBUG__))
				if (mach_line_debug)
				{
					(void) fprintf(debug_stream_p,
								   "\t\t(out of clip bounds)\n");
				}
#endif

				is_graphics_engine_out_of_sync = TRUE;
				
				continue;

			}
#if (defined(__DEBUG__))
			else
			{
				if (mach_line_debug)
				{
					(void) fprintf(debug_stream_p,
"\t{\n"
"\t\t%d,%d -> %d,%d\n"
"\t}\n",
								   x1, y1, x2, y2);
				}
			}
#endif

			/*
			 * Check if the line lies outside the device coordinate
			 * space, we need to further clip it
			 */
			
			OUTCODE(oc1_clipped, x1, y1, DEFAULT_MACH_MIN_ATI_COORDINATE,
					DEFAULT_MACH_MAX_ATI_COORDINATE,
					DEFAULT_MACH_MIN_ATI_COORDINATE,
					DEFAULT_MACH_MAX_ATI_COORDINATE);
			OUTCODE(oc2_clipped, x2, y2, DEFAULT_MACH_MIN_ATI_COORDINATE,
					DEFAULT_MACH_MAX_ATI_COORDINATE,
					DEFAULT_MACH_MIN_ATI_COORDINATE,
					DEFAULT_MACH_MAX_ATI_COORDINATE);

			if (oc1_clipped | oc2_clipped)
			{
				/*
				 * Need to pre-clip the line to within the device
				 * coordinate space.
				 */
				
				SISegment clipped_segment;
			
				if (mach_line_segment_clip_segment(x1, y1,
					x2, y2, left_clip, top_clip, right_clip,
					bottom_clip, &clipped_segment))
				{
					x1 = clipped_segment.x1;
					y1 = clipped_segment.y1;
					x2 = clipped_segment.x2;
					y2 = clipped_segment.y2;

					MACH_WAIT_FOR_FIFO(5);
					outw(MACH_REGISTER_LINEDRAW_INDEX, 0);
					outw(MACH_REGISTER_LINEDRAW, x1);
					outw(MACH_REGISTER_LINEDRAW, y1);
					outw(MACH_REGISTER_LINEDRAW, x2);
					outw(MACH_REGISTER_LINEDRAW, y2);
					
					is_graphics_engine_out_of_sync = TRUE;
					
				}

				continue;		/* next point */
			}
			
			ASSERT((x1 >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
				   (x1 <= DEFAULT_MACH_MAX_ATI_COORDINATE));
			ASSERT((x2 >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
				   (x2 <= DEFAULT_MACH_MAX_ATI_COORDINATE));
			ASSERT((y1 >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
				   (y1 <= DEFAULT_MACH_MAX_ATI_COORDINATE));
			ASSERT((y2 >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
				   (y2 <= DEFAULT_MACH_MAX_ATI_COORDINATE));

			/*
			 * Check if the graphics engine has been correctly
			 * programmed.
			 */

			if (is_graphics_engine_out_of_sync == TRUE)
			{
				is_graphics_engine_out_of_sync = FALSE;
				
				MACH_WAIT_FOR_FIFO(3);
				outw(MACH_REGISTER_LINEDRAW_INDEX, 0);
				outw(MACH_REGISTER_LINEDRAW, x1);
				outw(MACH_REGISTER_LINEDRAW, y1);
			}
			
			/*
			 * Draw the line.
			 */

			MACH_WAIT_FOR_FIFO(2);
			
			outw(MACH_REGISTER_LINEDRAW, x2);
			outw(MACH_REGISTER_LINEDRAW, y2);
	
		}
		
		oc1_unclipped = oc2_unclipped;

		break;
		
	default:
		/*CONSTANTCONDITION*/
		ASSERT(0);
		break;
	}
	
	if (!isCapNotLast)
	{
		MACH_WAIT_FOR_FIFO(1);
		outw(MACH_REGISTER_LINEDRAW_OPT, 0);
	}
	
	x1 = x_unclipped;
	y1 = y_unclipped;
	x2 = xorg + ptsIn->x;
	y2 = yorg + ptsIn->y;
	
	OUTCODE(oc2_unclipped, x2, y2, left_clip, right_clip, top_clip,
			bottom_clip);
	
	if (oc1_unclipped & oc2_unclipped)
	{
#if (defined(__DEBUG__))
		if (mach_line_debug)
		{
			(void) fprintf(debug_stream_p,
						   "\t\t(out of clip bounds)\n");
		}
#endif

	}
	else
	{
#if (defined(__DEBUG__))
		if (mach_line_debug)
		{
			(void) fprintf(debug_stream_p,
"\t{\n"
"\t\t%d,%d -> %d,%d\n"
"\t}\n",
						   x1, y1, x2, y2);
		}
#endif
		
		/*
		 * Check if the line lies outside the device coordinate
		 * space, we need to further clip it
		 */
			
		OUTCODE(oc1_clipped, x1, y1, DEFAULT_MACH_MIN_ATI_COORDINATE,
				DEFAULT_MACH_MAX_ATI_COORDINATE,
				DEFAULT_MACH_MIN_ATI_COORDINATE,
				DEFAULT_MACH_MAX_ATI_COORDINATE);
		OUTCODE(oc2_clipped, x2, y2, DEFAULT_MACH_MIN_ATI_COORDINATE,
				DEFAULT_MACH_MAX_ATI_COORDINATE,
				DEFAULT_MACH_MIN_ATI_COORDINATE,
				DEFAULT_MACH_MAX_ATI_COORDINATE);
	
		if (oc1_clipped | oc2_clipped)
		{
			/*
			 * Need to pre-clip the line to within the device
			 * coordinate space.
			 */
		
			SISegment clipped_segment;
			
			if (mach_line_segment_clip_segment(x1, y1,
				   x2, y2, left_clip, top_clip, right_clip,
				   bottom_clip, &clipped_segment))
			{
				x1 = clipped_segment.x1;
				y1 = clipped_segment.y1;
				x2 = clipped_segment.x2;
				y2 = clipped_segment.y2;

			}
			else
			{
				return;
			}

		}

		ASSERT((x1 >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
			   (x1 <= DEFAULT_MACH_MAX_ATI_COORDINATE));
		ASSERT((x2 >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
			   (x2 <= DEFAULT_MACH_MAX_ATI_COORDINATE));
		ASSERT((y1 >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
			   (y1 <= DEFAULT_MACH_MAX_ATI_COORDINATE));
		ASSERT((y2 >= DEFAULT_MACH_MIN_ATI_COORDINATE) &&
			   (y2 <= DEFAULT_MACH_MAX_ATI_COORDINATE));
	
		MACH_WAIT_FOR_FIFO(5);
		outw(MACH_REGISTER_LINEDRAW_INDEX, 0);
		outw(MACH_REGISTER_LINEDRAW, x1);
		outw(MACH_REGISTER_LINEDRAW, y1);
		outw(MACH_REGISTER_LINEDRAW, x2);
		outw(MACH_REGISTER_LINEDRAW, y2);
	
	} /* inside current_clip box */

	return;

}

#undef OUTCODE
#undef OUT_ABOVE
#undef OUT_LEFT
#undef OUT_RIGHT
#undef OUT_BELOW


/*
 * mach_line_one_bit_dashed:
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
mach_line_one_bit_dashed(SIint32 xorg, SIint32 yorg, SIint32 count, 
	       SIPointP ptsIn, SIint32 isCapNotLast, SIint32 coordMode)
{
	unsigned short dp_config;

#if (defined(__DEBUG__))
	struct mach_line_state *line_state_p;
#endif

	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE, graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		 (struct generic_graphics_state *) graphics_state_p));
	
#if (defined(__DEBUG__))
	line_state_p = &(graphics_state_p->current_line_state);
	
	ASSERT(IS_OBJECT_STAMPED(MACH_LINE_STATE, line_state_p));
#endif

	ASSERT(!MACH_IS_IO_ERROR());

	
#if (defined(__DEBUG__))
	if (mach_line_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_one_bit_line_dashed_solid)\n"
"{\n"
"\txorg = %ld\n"
"\tyorg = %ld\n"
"\tcount = %ld\n"
"\tptsIn = %p\n"
"\tisCapNotLast = %ld\n"
"\tcoordMode = %ld\n",
					   xorg, yorg, count, (void *) ptsIn,
					   isCapNotLast, coordMode);
	}
#endif

	ASSERT((graphics_state_p->
		    generic_state.si_graphics_state.SGlinestyle == SGLineDash)||
		   (graphics_state_p->
		    generic_state.si_graphics_state.SGlinestyle == SGLineDblDash));
	
	ASSERT((graphics_state_p->
		    generic_state.si_graphics_state.SGfillmode == SGFillSolidFG) ||
		   (graphics_state_p->
		    generic_state.si_graphics_state.SGfillmode != SGFillSolidBG));
	
	if (count <= 1)
	{
		return (SI_SUCCEED);
	}
	
#if (defined(__DEBUG__))
	if (mach_line_debug)
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
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	if (graphics_state_p->
		generic_state.si_graphics_state.SGlinestyle == SGLineDash)
	{
		
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
									 MACH_DASHED_LINE_DEPENDENCIES);
		MACH_STATE_SET_BG_ROP(screen_state_p,
							  MACH_MIX_FN_LEAVE_ALONE);
		
	}
	else if (graphics_state_p->
			 generic_state.si_graphics_state.SGlinestyle ==
			 SGLineDblDash)
	{
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
									 MACH_DOUBLE_DASHED_LINE_DEPENDENCIES);
	}
	else
	{
		/*
		 * Should be either dashed or double dashed.
		 */
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return (SI_FAIL);
	}
		

	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR |
		MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		MACH_DP_CONFIG_ENABLE_DRAW |
		MACH_DP_CONFIG_MONO_SRC_PATT |
		MACH_DP_CONFIG_READ_MODE_MONO_DATA |
		MACH_DP_CONFIG_WRITE;

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

	/*
	 * Last pixel is not drawn:  all clip exceptions are disabled,
	 * using extended mode line draws.
	 */
	MACH_WAIT_FOR_FIFO(3);
	outw(MACH_REGISTER_LINEDRAW_OPT, 
		 MACH_LINEDRAW_OPT_LAST_PEL_OFF);
	outw(MACH_REGISTER_LINEDRAW_INDEX, 0);
	outw(MACH_REGISTER_PATT_INDEX, 0);


	/*
	 * Pump the line endpoints. The dash pattern would automatically
	 * be stippled from the monochrome pattern registers.
	 */

	mach_line_one_bit_helper(xorg, yorg, ptsIn, count, coordMode,
							 isCapNotLast);
#if (defined(__DEBUG__))
	if (mach_line_debug)
	{
		(void) fprintf(debug_stream_p,
					   "}\n");
	}
#endif

	ASSERT(!MACH_IS_IO_ERROR());

	return (SI_SUCCEED);
}

/*
 * mach_line_segment_one_bit_solid
 *
 * Segment draw code in solid fill mode for the MACH display library.
 */

STATIC SIBool
mach_line_segment_one_bit_solid(SIint32 xorg, SIint32 yorg, SIint32 count,
		  SISegmentP psegsIn, SIint32 isCapNotLast)
{
	unsigned short dp_config;
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();

#if (defined(__DEBUG__))
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();
#endif

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	
	ASSERT(!MACH_IS_IO_ERROR());
	
#if (defined(__DEBUG__))
	if (mach_line_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_one_bit_segment_solid)\n"
"{\n"
"\txorg = %ld\n"
"\tyorg = %ld\n"
"\tcount = %ld\n"
"\tpsegsIn = %p\n"
"\tisCapNotLast = %ld\n",
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
	
	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	/*
	 * Synchronize registers with the graphics state.
	 */
	MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
								 MACH_SOLID_LINE_DEPENDENCIES);

	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_WRITE | 
		MACH_DP_CONFIG_ENABLE_DRAW |
		MACH_DP_CONFIG_MONO_SRC_ONE |
		MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR;

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

	/*
	 * isCapNotLast applies for all the segments here.
	 */

	MACH_WAIT_FOR_FIFO(1);

	mach_line_segment_one_bit_helper(xorg, yorg, psegsIn, 
									 count, isCapNotLast);

#if (defined(__DEBUG__))
	if (mach_line_debug)
	{
		(void) fprintf(debug_stream_p,
					   "}\n");
	}
#endif

	ASSERT(!MACH_IS_IO_ERROR());
	
	return (SI_SUCCEED);
}

/*
 * mach_line_segment_one_bit_dashed
 *
 * Dashed segment drawing code for the MACH display library.
 */

STATIC SIBool
mach_line_segment_one_bit_dashed(SIint32 xorg, SIint32 yorg, SIint32 count,
    SISegmentP psegsIn, SIint32 isCapNotLast)
{
	unsigned short dp_config;
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	
	ASSERT(!MACH_IS_IO_ERROR());
	
#if (defined(__DEBUG__))
	if (mach_line_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_one_bit_segment_dashed)\n"
"{\n"
"\txorg = %ld\n"
"\tyorg = %ld\n"
"\tcount = %ld\n"
"\tpsegsIn = %p\n"
"\tisCapNotLast = %ld\n",
					   xorg, yorg, count, (void *) psegsIn,
					   isCapNotLast);
	}
#endif

	ASSERT((graphics_state_p->
		    generic_state.si_graphics_state.SGfillmode == SGFillSolidFG) ||
		   (graphics_state_p->
		    generic_state.si_graphics_state.SGfillmode == SGFillSolidBG));
	
	ASSERT((graphics_state_p->
		    generic_state.si_graphics_state.SGlinestyle == SGLineDash)||
		   (graphics_state_p->
		    generic_state.si_graphics_state.SGlinestyle == SGLineDblDash));

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}
	
	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	if (graphics_state_p->
		generic_state.si_graphics_state.SGlinestyle == SGLineDash)
	{
		
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
									 MACH_DASHED_LINE_DEPENDENCIES);
		MACH_STATE_SET_BG_ROP(screen_state_p,
							  MACH_MIX_FN_LEAVE_ALONE);
		
	}
	else if (graphics_state_p->
			 generic_state.si_graphics_state.SGlinestyle ==
			 SGLineDblDash)
	{
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
									 MACH_DOUBLE_DASHED_LINE_DEPENDENCIES);
	}
	else
	{
		/*
		 * Should be either dashed or double dashed.
		 */
		/*CONSTANTCONDITION*/
		ASSERT(0);
		return (SI_FAIL);
	}
		
	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR |
		MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		MACH_DP_CONFIG_ENABLE_DRAW |
		MACH_DP_CONFIG_MONO_SRC_PATT |
		MACH_DP_CONFIG_READ_MODE_MONO_DATA |
		MACH_DP_CONFIG_WRITE;

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

	/*
	 * isCapNotLast applies for all the segments here.
	 */
	MACH_WAIT_FOR_FIFO(1);

	mach_line_segment_one_bit_helper(xorg, yorg, psegsIn, count, 
									 isCapNotLast);

#if (defined(__DEBUG__))
	if (mach_line_debug)
	{
		(void) fprintf(debug_stream_p,
					   "}\n");
	}
#endif

	ASSERT(!MACH_IS_IO_ERROR());
	
	return (SI_SUCCEED);
}

/*
 * mach_one_bit_rectangle. 
 */
STATIC SIBool
mach_line_rectangle_one_bit(SIint32 x1, SIint32 y1, SIint32 x2, SIint32 y2)
{
	unsigned short	dp_config;

	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		   (struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE, graphics_state_p));

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
	if (mach_line_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_one_bit_rectangle)\n"
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
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	if (graphics_state_p->
		generic_state.si_graphics_state.SGlinestyle == SGLineSolid)
	{
		MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
									 MACH_SOLID_LINE_DEPENDENCIES);

		dp_config = screen_state_p->dp_config_flags |
			MACH_DP_CONFIG_WRITE | 
			MACH_DP_CONFIG_ENABLE_DRAW |
			MACH_DP_CONFIG_MONO_SRC_ONE |
			MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
			MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR;
	}
	else /* Dashed or Double Dashed lines */
	{

		if (graphics_state_p->
			generic_state.si_graphics_state.SGlinestyle == SGLineDash)
		{
			
			MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
										 MACH_DASHED_LINE_DEPENDENCIES);
			MACH_STATE_SET_BG_ROP(screen_state_p,
								  MACH_MIX_FN_LEAVE_ALONE);
			
		}
		else if (graphics_state_p->
				 generic_state.si_graphics_state.SGlinestyle ==
				 SGLineDblDash)
		{
			MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
										 MACH_DOUBLE_DASHED_LINE_DEPENDENCIES);
		}

		dp_config = screen_state_p->dp_config_flags |
			MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR |
			MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
			MACH_DP_CONFIG_ENABLE_DRAW |
			MACH_DP_CONFIG_MONO_SRC_PATT |
			MACH_DP_CONFIG_READ_MODE_MONO_DATA |
			MACH_DP_CONFIG_WRITE;

	}

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

	/*
	 * Last pixel is not drawn:  all clip exceptions are disabled,
	 * using extended mode line draws.
	 */
	MACH_WAIT_FOR_FIFO(3);
	outw(MACH_REGISTER_CUR_X, x1);
	outw(MACH_REGISTER_CUR_Y, y1);
	outw(MACH_REGISTER_PATT_INDEX, 0);
	

	MACH_WAIT_FOR_FIFO(8);
	
	/* Draw the first line x1,y1 to x2,y1 */
	outw(MACH_REGISTER_LINEDRAW_OPT, 
		 MACH_LINEDRAW_OPT_LAST_PEL_OFF |
		 MACH_LINEDRAW_OPT_DIR_TYPE_DEGREE | 
		 MACH_LINEDRAW_OPT_0_DEGREE_RADIAL_LINE);
	outw(MACH_REGISTER_BRES_COUNT, x2 - x1);

	/* Draw the second line from x2, y1 to x2, y2 */
	outw(MACH_REGISTER_LINEDRAW_OPT, 
		 MACH_LINEDRAW_OPT_LAST_PEL_OFF |
		 MACH_LINEDRAW_OPT_DIR_TYPE_DEGREE | 
		 MACH_LINEDRAW_OPT_270_DEGREE_RADIAL_LINE);
	outw(MACH_REGISTER_BRES_COUNT, y2 - y1);

	/* Draw the third line from x2, y2 to x1, y2 */
	outw(MACH_REGISTER_LINEDRAW_OPT, 
		 MACH_LINEDRAW_OPT_LAST_PEL_OFF |
		 MACH_LINEDRAW_OPT_DIR_TYPE_DEGREE | 
		 MACH_LINEDRAW_OPT_180_DEGREE_RADIAL_LINE);
	outw(MACH_REGISTER_BRES_COUNT, x2 - x1);

	/* Draw the last line from x1, y2 to x1, y1 */
	outw(MACH_REGISTER_LINEDRAW_OPT, 
		 MACH_LINEDRAW_OPT_LAST_PEL_OFF |
		 MACH_LINEDRAW_OPT_DIR_TYPE_DEGREE | 
		 MACH_LINEDRAW_OPT_90_DEGREE_RADIAL_LINE);
	outw(MACH_REGISTER_BRES_COUNT, y2 - y1);

	ASSERT(!MACH_IS_IO_ERROR());

	return (SI_SUCCEED);
}

/*
 * mach_line_one_bit_solid:
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
mach_line_one_bit_solid(SIint32 xorg, SIint32 yorg, SIint32 count, 
	       SIPointP ptsIn, SIint32 isCapNotLast, SIint32 coordMode)
{
	unsigned short dp_config;
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();

#if (defined(__DEBUG__))
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();
#endif

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	
	ASSERT(!MACH_IS_IO_ERROR());
	
#if (defined(__DEBUG__))
	if (mach_line_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_one_bit_line_solid)\n"
"{\n"
"\txorg = %ld\n"
"\tyorg = %ld\n"
"\tcount = %ld\n"
"\tptsIn = %p\n"
"\tisCapNotLast = %ld\n"
"\tcoordMode = %ld\n",
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
	
#if (defined(__DEBUG__))
	if (mach_line_debug)
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
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
								 MACH_SOLID_LINE_DEPENDENCIES);


	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_WRITE | 
		MACH_DP_CONFIG_ENABLE_DRAW |
		MACH_DP_CONFIG_MONO_SRC_ONE |
		MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR;

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

	/*
	 * Last pixel is not drawn:  all clip exceptions are disabled,
	 * using extended mode line draws.
	 */
	MACH_WAIT_FOR_FIFO(2);
	outw(MACH_REGISTER_LINEDRAW_OPT, 
		 MACH_LINEDRAW_OPT_LAST_PEL_OFF);
	outw(MACH_REGISTER_LINEDRAW_INDEX, 0);

	/*
	 * block transfer middle points
	 * first the next TOX coordinate and the next TOY coordinate
	 * These lines are drawn with the last pixel drawn disabled
	 * leave the last point. 
	 */
	
	mach_line_one_bit_helper(xorg, yorg, ptsIn, count, coordMode,
							 isCapNotLast);
#if (defined(__DEBUG__))
	if (mach_line_debug)
	{
		(void) fprintf(debug_stream_p,
					   "}\n");
	}
#endif

	ASSERT(!MACH_IS_IO_ERROR());

	return (SI_SUCCEED);
	
}

/*
 * Functions for backward compatibility support.
 */
STATIC SIBool
mach_line_compat_one_bit_solid(SIint32 count, SIPointP ptsIn)
{
	unsigned short dp_config;
	short 			*points_p = (short *) ptsIn;
	int 			tmp_count = (count - 1) << 1;

	MACH_CURRENT_SCREEN_STATE_DECLARE();

#if (defined(__DEBUG__))
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();
#endif
	
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));
	
	ASSERT(!MACH_IS_IO_ERROR());
	
#if (defined(__DEBUG__))
	if (mach_line_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_line_compat_one_bit_solid)\n"
"{\n"
"\tcount = %ld\n"
"\tptsIn = %p\n",
					   count, (void *) ptsIn);
	}
#endif

	ASSERT(graphics_state_p->
		   generic_state.si_graphics_state.SGlinestyle == SGLineSolid);
	
	if (count <= 1)
	{
		return (SI_SUCCEED);
	}

#if (defined(__DEBUG__))
	if (mach_line_debug)
	{
		int i;
		SIPointP tmp_points_p;
		
		for (i = 0, tmp_points_p = ptsIn; i < count - 1; i++, tmp_points_p++)
		{
			(void) fprintf(debug_stream_p,
						   "\t\t(%d,%d) -> (%d,%d)\n",
						   tmp_points_p->x, 
						   tmp_points_p->y,
						   (tmp_points_p+1)->x, 
						   (tmp_points_p+1)->y);
		}
	}
#endif /* DEBUG */

	/*
	 * Switch to ATI context.
	 */

	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
								 MACH_SOLID_LINE_DEPENDENCIES);


	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_WRITE | 
		MACH_DP_CONFIG_ENABLE_DRAW |
		MACH_DP_CONFIG_MONO_SRC_ONE |
		MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR;

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);

	/*
	 * Last pixel is not drawn:  all clip exceptions are disabled,
	 * using extended mode line draws.
	 */

	MACH_WAIT_FOR_FIFO(2);
	outw(MACH_REGISTER_LINEDRAW_OPT, MACH_LINEDRAW_OPT_LAST_PEL_OFF);
	outw(MACH_REGISTER_LINEDRAW_INDEX, 0);

	/*
	 * block transfer points
	 * first the next TOX coordinate and the next TOY coordinate
	 * These lines are drawn with the last pixel drawn disabled
	 * leave the last point. 
	 *
	 * draw lines by writing to the LINEDRAW register.
	 * the register will auto-increment to handle moves and
	 * linedraws correctly.
	 *
	 */

	while(tmp_count > mach_graphics_engine_fifo_blocking_factor)
	{
		mach_register_wait_for_fifo(mach_graphics_engine_fifo_blocking_factor);
		mach_asm_inline_repz_outw(MACH_REGISTER_LINEDRAW,
								  mach_graphics_engine_fifo_blocking_factor,
								  points_p);
		points_p += mach_graphics_engine_fifo_blocking_factor;
		tmp_count -= mach_graphics_engine_fifo_blocking_factor;
	}
	if (tmp_count > 0)
	{
		mach_register_wait_for_fifo(tmp_count);
		mach_asm_inline_repz_outw(MACH_REGISTER_LINEDRAW,
								  tmp_count,
								  points_p);
		points_p += tmp_count;
	}
	
	/*
	 * Draw the last point
	 */
	MACH_WAIT_FOR_FIFO(3);
	outw(MACH_REGISTER_LINEDRAW_OPT, 0x00);
	outw(MACH_REGISTER_LINEDRAW, *points_p++); 
	outw(MACH_REGISTER_LINEDRAW, *points_p); 
		
#if (defined(__DEBUG__))
	if (mach_line_debug)
	{
		(void) fprintf(debug_stream_p,
					   "}\n");
	}
#endif

	ASSERT(!MACH_IS_IO_ERROR());
	
	return (SI_SUCCEED);
}

STATIC SIBool
mach_line_compat_one_bit_dashed(SIint32 count,  SIPointP ptsIn)
{

#if (defined(__DEBUG__))
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();
#endif

	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE, graphics_state_p));
	
#if (defined(__DEBUG__))
	if (mach_line_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_line_compat_one_bit_dashed)\n"
"{\n"
"\tcount = %ld\n"
"\tptsIn = %p\n"
"}\n",
					   count, (void *) ptsIn);
	}
#endif

	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGlinestyle != 
		   SGLineSolid);
	
	/*
	 * Call the core libraries drawing code for dashed and double
	 * dashed lines.
	 */ 
	return mach_line_one_bit_dashed (0, 0, count, 
									 ptsIn, TRUE /* isCapNotLast*/, 
									 SICoordModeOrigin /* coordMode */);
}

/*
 * For the segment draw routines, we only need to do minor
 * transformations on the inputs.
 */
STATIC SIBool
mach_line_segment_compat_one_bit_solid(SIint32 count, SIPointP ptsIn)
{

#if (defined(__DEBUG__))
	if (mach_line_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_line_segment_compat_one_bit_solid)\n"
"{\n"
"\tcount = %ld\n"
"\tptsIn = %p\n"
"}\n",
					   count, (void *) ptsIn);
	}
#endif
	
	return mach_line_segment_one_bit_solid(0, 0, count >> 1, 
										   (SISegmentP) ptsIn,
										   FALSE);
}

STATIC SIBool
mach_line_segment_compat_one_bit_dashed(SIint32 count, SIPointP ptsIn)
{

#if (defined(__DEBUG__))
	if (mach_line_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_line_segment_compat_one_bit_dashed)\n"
"{\n"
"\tcount = %ld\n"
"\tptsIn = %p\n"
"}\n",
					   count, (void *) ptsIn);
	}
#endif
	
	return mach_line_segment_one_bit_dashed(0, 0, count >> 1, 
											(SISegmentP) ptsIn,
											FALSE);
}

/*
 * Graphics state change.
 * 
 * We change the function pointers depending on whether the line style
 * is solid or dashed.
 * 
 * Backward Server compatibility support:
 *
 */

function void
mach_line__gs_change__(void)
{
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	MACH_CURRENT_GRAPHICS_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		   (struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(MACH_GRAPHICS_STATE, graphics_state_p));
	
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
	 * We do not handle dashed lines when fill mode is not
	 * FillSolid, the problem being non horizontal lines are not
	 * easily handled by the drawing engine's mechanisms.
	 * Rectangle outlines similarly have to be turned off if the line
	 * pattern is too large.
	 */

	if (screen_state_p->generic_state.screen_sdd_version_number >=
		DM_SI_VERSION_1_1)
	{
		/*
		 * New SDD's calling sequence.
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
					MACH_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_DRAW)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitline = 
							mach_line_one_bit_solid;
				}

				if (screen_state_p->options_p->linedraw_options &
					MACH_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitseg =
							mach_line_segment_one_bit_solid;
				}

				if (screen_state_p->options_p->linedraw_options &
					MACH_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_RECTANGLES)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitrect =
							mach_line_rectangle_one_bit;
				}
			}
			else if ((screen_state_p->options_p->linedraw_options & 
					  MACH_OPTIONS_LINEDRAW_OPTIONS_USE_PATTERN_REGISTERS) &&
					 ((graphics_state_p->
					   generic_state.si_graphics_state.SGlinestyle ==
					   SGLineDash) ||
					  (graphics_state_p->
					   generic_state.si_graphics_state.SGlinestyle ==
					   SGLineDblDash)) &&
					 (graphics_state_p->current_line_state.is_pattern_valid
					  == TRUE))
			{
				if (screen_state_p->options_p->linedraw_options &
					MACH_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_DRAW)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitline = 
							mach_line_one_bit_dashed;
				}
				
				if (screen_state_p->options_p->linedraw_options &
					MACH_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitseg =
							mach_line_segment_one_bit_dashed;
				}

				if (screen_state_p->options_p->linedraw_options &
					MACH_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_RECTANGLES)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitrect =
							mach_line_rectangle_one_bit;
				}
			}
			else
			{
				/*EMPTY*/
				;
			}
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
		
		if ((graphics_state_p->generic_state.si_graphics_state.SGfillmode
			 == SGFillSolidFG) ||
			(graphics_state_p->generic_state.si_graphics_state.SGfillmode
			 == SGFillSolidBG))
		{
			if (graphics_state_p->generic_state.si_graphics_state.SGlinestyle
				== SGLineSolid)
			{
				if (screen_state_p->options_p->linedraw_options &
				 MACH_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_DRAW)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitline = 
					    (SIBool (*)(SIint32, SIint32, SIint32, SIPointP,
								SIint32, SIint32))
					    mach_line_compat_one_bit_solid;
				}

				if (screen_state_p->options_p->linedraw_options &
					MACH_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitseg =  
				        (SIBool (*)(SIint32, SIint32, 
								SIint32, SISegmentP, SIint32))
					    mach_line_segment_compat_one_bit_solid;
				}

				if (screen_state_p->options_p->linedraw_options &
					MACH_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_RECTANGLES)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitrect =
							mach_line_rectangle_one_bit;
				}
			}
			else if ((screen_state_p->options_p->linedraw_options & 
					  MACH_OPTIONS_LINEDRAW_OPTIONS_USE_PATTERN_REGISTERS) &&
					 ((graphics_state_p->
					   generic_state.si_graphics_state.SGlinestyle ==
					   SGLineDash) ||
					  (graphics_state_p->
					   generic_state.si_graphics_state.SGlinestyle ==
					   SGLineDblDash)) &&
					 (graphics_state_p->current_line_state.is_pattern_valid
					  == TRUE))
			{
				if (screen_state_p->options_p->linedraw_options &
					MACH_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_DRAW)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitline = 
    					(SIBool (*)(SIint32, SIint32, SIint32, SIPointP,
								SIint32, SIint32))
	    				mach_line_compat_one_bit_dashed;
				}
				
				if (screen_state_p->options_p->linedraw_options &
					MACH_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitseg = 
    					(SIBool (*)(SIint32, SIint32, SIint32, 
								SISegmentP, SIint32))
	    				mach_line_segment_compat_one_bit_dashed;
				}

				if (screen_state_p->options_p->linedraw_options &
					MACH_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_RECTANGLES)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitrect = 
	    				mach_line_rectangle_one_bit;
				}
			}
			else
			{
				/*EMPTY*/
				;
			}
		}
	}
}

function void
mach_line__initialize__(SIScreenRec *si_screen_p,
						struct mach_options_structure * options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	
	flags_p->SIavail_line = 0;
	functions_p->si_line_onebitline = mach_no_operation_fail;
	functions_p->si_line_onebitseg = mach_no_operation_fail;
	functions_p->si_line_onebitrect = mach_no_operation_fail;

	if (options_p->linedraw_options &
		MACH_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_DRAW)
	{
		flags_p->SIavail_line |= ONEBITLINE_AVAIL;
		functions_p->si_line_onebitline = mach_line_one_bit_solid;
	}
	if (options_p->linedraw_options &
		MACH_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_RECTANGLES)
	{
		flags_p->SIavail_line |= ONEBITRECT_AVAIL;
		functions_p->si_line_onebitrect = mach_line_rectangle_one_bit;
	}
	if (options_p->linedraw_options &
		MACH_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW)
	{
		flags_p->SIavail_line |= ONEBITSEG_AVAIL;
		functions_p->si_line_onebitseg = mach_line_segment_one_bit_solid;
	}
	if (options_p->linedraw_options &
		MACH_OPTIONS_LINEDRAW_OPTIONS_USE_PATTERN_REGISTERS)
	{
		flags_p->SIavail_line |= (DASH_AVAIL | DBLDASH_AVAIL);
	}
}
