/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)S3:S3/s3_line.c	1.6"

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
#include "s3_globals.h"
#include "s3_options.h"

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
export boolean s3_line_debug = FALSE;
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

#include "s3_state.h"
#include "s3_gs.h"

/***
 ***	Constants.
 ***/
#define S3_SOLID_LINE_DEPENDENCIES\
	(S3_INVALID_FG_ROP|\
	 S3_INVALID_FOREGROUND_COLOR|\
	 S3_INVALID_CLIP_RECTANGLE|\
	 S3_INVALID_WRT_MASK)

#define S3_DASHED_LINE_DEPENDENCIES\
	(S3_INVALID_FG_ROP|\
	 S3_INVALID_FOREGROUND_COLOR|\
	 S3_INVALID_CLIP_RECTANGLE|\
	 S3_INVALID_WRT_MASK)

#define S3_DOUBLE_DASHED_LINE_DEPENDENCIES\
	(S3_INVALID_FG_ROP|\
	 S3_INVALID_BG_ROP|\
	 S3_INVALID_FOREGROUND_COLOR|\
	 S3_INVALID_BACKGROUND_COLOR|\
	 S3_INVALID_CLIP_RECTANGLE|\
	 S3_INVALID_WRT_MASK)

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
 * For drawing lines that are aligned with either the x or y axis.
 * Horizontal or vertical lines.
 */
#define S3_IO_DRAW_ALIGNED_LINE(LENGTH,COMMAND)\
		S3_IO_SET_ENHANCED_REGISTER(\
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, LENGTH);\
		S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,COMMAND); 

#define S3_MMIO_DRAW_ALIGNED_LINE(LENGTH,COMMAND)\
		S3_MMIO_SET_ENHANCED_REGISTER(\
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, LENGTH);\
		S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,COMMAND); 
#define S3_DRAW_ALIGNED_LINE(SCREENSTATEP,LENGTH,COMMAND)\
	S3_WAIT_FOR_FIFO(2);\
	if ((SCREENSTATEP)->use_mmio)\
	{\
		S3_MMIO_DRAW_ALIGNED_LINE(LENGTH,COMMAND);\
	}\
	else\
	{\
		S3_IO_DRAW_ALIGNED_LINE(LENGTH,COMMAND);\
	}

/*
 * Macro for drawing sloped lines.
 */
#define S3_IO_DRAW_SLOPED_LINE(LENGTH, DIAGONALSTEP, AXIALSTEP, ERRORTERM,\
	COMMAND)\
	S3_IO_SET_ENHANCED_REGISTER(\
		S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, LENGTH );\
	S3_IO_SET_ENHANCED_REGISTER(\
		S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP, DIAGONALSTEP);\
	S3_IO_SET_ENHANCED_REGISTER(\
		S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP, AXIALSTEP);\
	S3_IO_SET_ENHANCED_REGISTER(\
		S3_ENHANCED_COMMAND_REGISTER_ERR_TERM, ERRORTERM);\
	S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD, COMMAND);

#define S3_MMIO_DRAW_SLOPED_LINE(LENGTH, DIAGONALSTEP, AXIALSTEP, ERRORTERM,\
	COMMAND)\
	S3_MMIO_SET_ENHANCED_REGISTER(\
		S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, LENGTH );\
	S3_MMIO_SET_ENHANCED_REGISTER(\
		S3_ENHANCED_COMMAND_REGISTER_DESTX_DIASTP, DIAGONALSTEP);\
	S3_MMIO_SET_ENHANCED_REGISTER(\
		S3_ENHANCED_COMMAND_REGISTER_DESTY_AXSTP, AXIALSTEP);\
	S3_MMIO_SET_ENHANCED_REGISTER(\
		S3_ENHANCED_COMMAND_REGISTER_ERR_TERM, ERRORTERM);\
	S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD, COMMAND);

#define S3_DRAW_SLOPED_LINE(SCREENSTATEP, LENGTH, DIAGONALSTEP, AXIALSTEP,\
	ERRORTERM, COMMAND)\
	S3_WAIT_FOR_FIFO(5);\
	if ((SCREENSTATEP)->use_mmio)\
	{\
		S3_MMIO_DRAW_SLOPED_LINE(LENGTH, DIAGONALSTEP, AXIALSTEP, ERRORTERM,\
			COMMAND);\
	}\
	else\
	{\
		S3_IO_DRAW_SLOPED_LINE(LENGTH, DIAGONALSTEP, AXIALSTEP, ERRORTERM,\
			COMMAND);\
	}

/*
 * Dashed line related stuff.
 */

/*
 * Macro for rotating the dash pattern.
 */
 #define ROTATE_PATTERN(PAT,COUNT,PATTERN_LENGTH)\
	(PAT) = ((PAT)<< (COUNT)) | ((PAT) >> (PATTERN_LENGTH - COUNT))

/*
 * Macro for swapping bytes.
 */
#define SWAP_BYTES(DATA)\
	((DATA) >> 8) | ((DATA) << 8)

/*
 * For drawing dashed lines that are aligned with either the x or y axis.
 * Horizontal or vertical lines.
 */
#define S3_DRAW_DASHED_ALIGNED_LINE(SCREENSTATEP,LINESTATEP,LENGTH,COMMAND) \
{\
	register int tmp = 0;\
	register unsigned short dash_pattern = (LINESTATEP)->dash_pattern;\
	register int count=(DEFAULT_S3_LINE_DASH_PATTERN_LENGTH %\
		(LINESTATEP)->dash_pattern_length);\
	S3_WAIT_FOR_FIFO(2);\
	if((SCREENSTATEP)->use_mmio)\
	{\
		S3_MMIO_DRAW_ALIGNED_LINE(LENGTH,COMMAND);\
		do{\
			S3_MMIO_SET_ENHANCED_REGISTER(\
			S3_ENHANCED_COMMAND_REGISTER_PIX_TRANS, SWAP_BYTES(dash_pattern));\
			ROTATE_PATTERN(dash_pattern,count,(LINESTATEP)->\
				dash_pattern_length);\
			tmp += DEFAULT_S3_LINE_DASH_PATTERN_LENGTH;\
		}while(tmp < LENGTH);\
	}\
	else\
	{\
		S3_IO_DRAW_ALIGNED_LINE(LENGTH,COMMAND);\
		do{\
			S3_IO_SET_ENHANCED_REGISTER(\
			S3_ENHANCED_COMMAND_REGISTER_PIX_TRANS, SWAP_BYTES(dash_pattern));\
			ROTATE_PATTERN(dash_pattern,count,(LINESTATEP)->\
				dash_pattern_length);\
			tmp += DEFAULT_S3_LINE_DASH_PATTERN_LENGTH;\
		}while(tmp < LENGTH);\
	}\
}

/*
 * Macro for drawing sloped lines.
 */
#define S3_DRAW_DASHED_SLOPED_LINE(SCREENSTATEP,LINESTATEP,LENGTH,DIAGONALSTEP,\
AXIALSTEP, ERRORTERM, COMMAND) \
{\
	register int tmp = 0;\
	register unsigned short dash_pattern =(LINESTATEP)->dash_pattern;\
	register int count=(DEFAULT_S3_LINE_DASH_PATTERN_LENGTH % \
		(LINESTATEP)->dash_pattern_length);\
	S3_WAIT_FOR_FIFO(5);\
	if ((SCREENSTATEP)->use_mmio)\
	{\
		S3_MMIO_DRAW_SLOPED_LINE(LENGTH, DIAGONALSTEP, AXIALSTEP, ERRORTERM,\
			COMMAND);\
		do{\
			S3_MMIO_SET_ENHANCED_REGISTER(\
			S3_ENHANCED_COMMAND_REGISTER_PIX_TRANS, SWAP_BYTES(dash_pattern));\
			ROTATE_PATTERN(dash_pattern,count,(LINESTATEP)->\
				dash_pattern_length);\
			tmp += DEFAULT_S3_LINE_DASH_PATTERN_LENGTH;\
		}while(tmp < LENGTH);\
	}\
	else\
	{\
		S3_IO_DRAW_SLOPED_LINE(LENGTH, DIAGONALSTEP, AXIALSTEP, ERRORTERM,\
			COMMAND);\
		do{\
			S3_IO_SET_ENHANCED_REGISTER(\
			S3_ENHANCED_COMMAND_REGISTER_PIX_TRANS, SWAP_BYTES(dash_pattern));\
			ROTATE_PATTERN(dash_pattern,count,(LINESTATEP)->\
				dash_pattern_length);\
			tmp += DEFAULT_S3_LINE_DASH_PATTERN_LENGTH;\
		}while(tmp < LENGTH);\
	}\
}

/***
 ***	Functions.
 ***/

/*
 * s3_line_clip_endpoints_to_physical_space:
 * 		Clip a line to the device coordinate space. Return TRUE if 
 * 		clipping yeilds coordinates within the device coordinate
 *		space else return FALSE. Inputs are addresses of the endpoints
 *		which contain the initial values and the resultant clipped 
 *		values are replaced in them.
 */
STATIC boolean
s3_line_clip_endpoints_to_physical_space(int *x1, int *y1, int *x2, int *y2)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();

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

/*
 * s3_one_bit_line_dashed:
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
s3_line_one_bit_dashed(SIint32 xorg, SIint32 yorg, SIint32 count,
	       SIPointP ptsIn, SIint32 isCapNotLast, SIint32 coordMode)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURRENT_GRAPHICS_STATE_DECLARE();

	struct s3_line_state *line_state_p;

	/*
	 * absolute values of (x2-x1) and (y2-y1).
	 */
	int		absolute_x_diff;
	int		absolute_y_diff;

	int		error_term,error_adjust;
	int		axial_step,diagonal_step,length;

	unsigned short axial_line_command = 
		screen_state_p->cmd_flags |
		S3_CMD_TYPE_LINEDRAW | 
		S3_CMD_BUS_WIDTH_16 |
		S3_CMD_USE_PIXTRANS |
		S3_CMD_DRAW | 
		S3_CMD_LAST_PXOF |
		S3_CMD_PX_MD_ACROSS_PLANE | 
		S3_CMD_DIR_TYPE_AXIAL |
		S3_CMD_WRITE ;

	unsigned short radial_line_command = 
		screen_state_p->cmd_flags |
		S3_CMD_TYPE_LINEDRAW | 
		S3_CMD_BUS_WIDTH_16 |
		S3_CMD_USE_PIXTRANS |
		S3_CMD_DRAW | 
		S3_CMD_LAST_PXOF |
		S3_CMD_PX_MD_ACROSS_PLANE | 
		S3_CMD_DIR_TYPE_RADIAL | 
		S3_CMD_WRITE;

	unsigned short command;

	int  	x1,y1,x2,y2;

	int		unclipped_x1,unclipped_y1;
	int 	oc1 = 0, oc2 = 0;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));

	ASSERT((graphics_state_p->generic_state.si_graphics_state.
		SGlinestyle == SGLineDash)||(graphics_state_p->generic_state.
		si_graphics_state.SGlinestyle == SGLineDblDash));

#if (defined(__DEBUG__))
	if (s3_line_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_line_one_bit_dashed) {\n"
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


	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	if (count <= 1)
	{
		return (SI_SUCCEED);
	}

	if (graphics_state_p->
		generic_state.si_graphics_state.SGlinestyle == SGLineDash)
	{
		S3_STATE_SYNCHRONIZE_STATE(screen_state_p, S3_DASHED_LINE_DEPENDENCIES);
		
		/*
	 	 * Program the foreground and the background mix registers.
	 	 */
		S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
		S3_STATE_SET_BG_ROP(screen_state_p, S3_MIX_FN_LEAVE_C_AS_IS);
	}
	else if (graphics_state_p->
			 generic_state.si_graphics_state.SGlinestyle ==
			 SGLineDblDash)
	{
		S3_STATE_SYNCHRONIZE_STATE(screen_state_p, 
			S3_DOUBLE_DASHED_LINE_DEPENDENCIES);
		/*
	 	 * Program the foreground and the background mix registers.
	 	 */
		S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
		S3_SET_BG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);
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
		
	line_state_p = &(graphics_state_p->current_line_state);
	if(line_state_p->is_pattern_valid == FALSE)
	{
		ASSERT(line_state_p->is_pattern_valid == TRUE);
		return (SI_FAIL);
	}

	/*
	 * Select the mix register. In this case CPU data determines the 
	 * the mix register.
	 */
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_CPU_DATA);

	/*
	 * Set the current x and current y
	 */
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
			if (s3_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"s3_line_one_bit_dashed(%d,%d,%d,%d){\n"
					"\tline completely outside physical coords space\n"
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
			if (s3_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"s3_line_one_bit_dashed(){\n"
					"\tclipping segment from (%d,%d,%d,%d)\n",
					x1,y1,x2,y2);
			}
#endif
			result = s3_line_clip_endpoints_to_physical_space(&x1,&y1,&x2,&y2);
#if (defined(__DEBUG__))
			if (s3_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"\tto (%d,%d,%d,%d)\n"
					"}\n",
					x1,y1,x2,y2);
			}
#endif
			/* Clipping yeilded a line outside the physical coordinate space. */
			if (result != TRUE)
			{
				continue;
			}
		}
		/* else line is within the physical coordinate space */

		/*
		 * At this point we have some part/whole of the line within 
		 * physical coordinate space. Set the current x and current y
		 */
		S3_WAIT_FOR_FIFO(2);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,x1);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,y1);

		/*
  		 * Handle vertical and horizontal lines seperately, since 
		 * less register programming is required in these cases.
		 */
		if( x1 == x2)
		{
			/* 
			 * Vertical line.
			 */
			command = radial_line_command;
			if( y2 < y1 )
			{
				/*
				 * Draw line from bottom to top
				 */
				length = y1 - y2 ;
				command |= S3_CMD_90_DGR_RADIAL_LINE;
			}
			else /* if (y2 > y1) */
			{
				/*
				 * Draw the line from top to bottom
				 */
				length = y2 - y1 ;
				command |= S3_CMD_270_DGR_RADIAL_LINE;
			}
			S3_DRAW_DASHED_ALIGNED_LINE(screen_state_p,line_state_p,
				length,command);
		}
		else if (y1 == y2)
		{
			/*
			 * Horizontal line.
			 */
			command = radial_line_command;
			if(x2 < x1)
			{
				/*
				 * Draw line from right to left
				 */
				length = x1 - x2 ;
				command |= S3_CMD_180_DGR_RADIAL_LINE;
			}
			else /* if(x2 > x1) */
			{
				/*
				 * Draw line from left to right 
				 */
				length = x2 - x1 ;
				command |= S3_CMD_0_DGR_RADIAL_LINE;
			}
			S3_DRAW_DASHED_ALIGNED_LINE(screen_state_p,line_state_p,
				length,command);
		}
		else  
		{
			/*
			 * Lines that are neither vertical nor horizontal.
			 * Sloped lines.
			 */
			command = axial_line_command;

			if((absolute_x_diff = x2 - x1 ) < 0 )
			{
				absolute_x_diff = -absolute_x_diff;
				error_adjust = -1;
			}
			else
			{
				command |= S3_CMD_AXIAL_X_LEFT_TO_RIGHT;
				error_adjust = 0;
			}

			if((absolute_y_diff = y2 - y1 ) < 0 )
			{
				absolute_y_diff = -absolute_y_diff;
			}
			else
			{
				command |= S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;
			}

			if( absolute_x_diff > absolute_y_diff )
			{
				length = absolute_x_diff;
				axial_step = absolute_y_diff << 1;
				diagonal_step = axial_step - (absolute_x_diff <<1);
				error_term = axial_step - absolute_x_diff;
			}
			else
			{
				length = absolute_y_diff;
				axial_step = absolute_x_diff << 1;
				diagonal_step = axial_step - (absolute_y_diff << 1);
				error_term  = axial_step - absolute_y_diff;
				command |= S3_CMD_AXIAL_Y_MAJOR;
			}

			S3_DRAW_DASHED_SLOPED_LINE(screen_state_p, line_state_p,
				length, diagonal_step, axial_step, error_term + error_adjust, 
				command);

		}
	}while( --count > 1);

	/*
	 * Draw the last point of the last line, if CapNotLast is FALSE
	 * and if the last point is within the device coordinate space
	 * and if the ending pixels of the last line were not clipped.
	 */
	if(!isCapNotLast)
	{
		if ((!oc2) && ( x2 == unclipped_x1) && (y2 == unclipped_y1))
		{
			S3_WAIT_FOR_FIFO(4);
			if (screen_state_p->use_mmio)
			{
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X,x2);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_Y,y2);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,1);
				S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
					radial_line_command & ~(S3_CMD_USE_PIXTRANS));
			}
			else
			{
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X,x2);
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_Y,y2);
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,1);
				S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
					radial_line_command & ~(S3_CMD_USE_PIXTRANS));
			}
		}
	}	
	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return (SI_SUCCEED);
}

STATIC SIBool
s3_line_segment_one_bit_solid(SIint32 xorg, SIint32 yorg, SIint32 count,
		  SISegmentP psegsIn, SIint32 isCapNotLast)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURRENT_GRAPHICS_STATE_DECLARE();

	/*
	 * absolute values of (x2-x1) and (y2-y1).
	 */
	int		absolute_x_diff;
	int		absolute_y_diff;

	int		error_term,error_adjust;
	int		axial_step,diagonal_step,length;

	short axial_line_command = 
		screen_state_p->cmd_flags |
		S3_CMD_TYPE_LINEDRAW | 
		S3_CMD_DRAW | 
		S3_CMD_PX_MD_ACROSS_PLANE | 
		S3_CMD_DIR_TYPE_AXIAL |
		S3_CMD_WRITE ;

	short radial_line_command = 
		screen_state_p->cmd_flags |
		S3_CMD_TYPE_LINEDRAW | 
		S3_CMD_DRAW | 
		S3_CMD_PX_MD_ACROSS_PLANE | 
		S3_CMD_DIR_TYPE_RADIAL | 
		S3_CMD_WRITE;

	short command;

    const int top_clip = screen_state_p->generic_state.screen_clip_top;
    const int bottom_clip = screen_state_p->generic_state.screen_clip_bottom;
    const int left_clip = screen_state_p->generic_state.screen_clip_left;
    const int right_clip = screen_state_p->generic_state.screen_clip_right;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));

	ASSERT(graphics_state_p->generic_state.si_graphics_state.
		SGlinestyle == SGLineSolid);

#if (defined(__DEBUG__))
	if (s3_line_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_one_bit_segmnet_solid) {\n"
			"\txorg = %ld\n"
			"\tyorg = %ld\n"
			"\tcount = %ld\n"
			"\tpsegsIn = %p\n"
			"\tisCapNotLast = %ld\n"
			"}\n",
			xorg, yorg, count, (void *) psegsIn, isCapNotLast);
	}
#endif

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	
	if (count < 1)
	{
		return (SI_SUCCEED);
	}
	
	S3_STATE_SYNCHRONIZE_STATE(screen_state_p, S3_SOLID_LINE_DEPENDENCIES);

	if(isCapNotLast)
	{
		radial_line_command |= S3_CMD_LAST_PXOF;
		axial_line_command |= S3_CMD_LAST_PXOF;
	}

	/*
	 * Select the frgd mix logic and the foreground color register as
	 * the frgd color source.
	 */
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);

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
			if (s3_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"s3_line_segment_one_bit_solid(%d,%d,%d,%d){\n"
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
			if (s3_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"s3_line_segment_one_bit_solid(){\n"
					"\tclipping segment from (%d,%d,%d,%d)\n",
					x1,y1,x2,y2);
			}
#endif
			result = s3_line_clip_endpoints_to_physical_space(&x1,&y1,&x2,&y2);
#if (defined(__DEBUG__))
			if (s3_line_debug)
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
			if (s3_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"s3_line_segment_one_bit_solid(%d,%d,%d,%d){\n"
					"\tsegment completely outside clipping rectangle\n"
					"}\n",
					x1,y1,x2,y2);
			}
#endif
			continue;
		}
		
		/*
		 * Program CUR_X, CUR_Y registers.
		 */
		S3_WAIT_FOR_FIFO(2);
		if (screen_state_p->use_mmio)
		{
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, x1);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y, y1);
		}
		else
		{
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, x1);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y, y1);
		}

		/*
  		 * Handle vertical and horizontal lines seperately, since only
		 * less register programming is required in these cases.
		 */
		if( x1 == x2)
		{
			/* 
			 * Vertical line.
			 */
			command = radial_line_command;
			if( y2 < y1 )
			{
				/*
				 * Draw line from bottom to top
				 */
				length = y1 - y2;
				command |= S3_CMD_90_DGR_RADIAL_LINE;
			}
			else /* if (y2 > y1) */
			{
				/*
				 * Draw the line from top to bottom
				 */
				length = y2 - y1;
				command |= S3_CMD_270_DGR_RADIAL_LINE;
			}
			S3_DRAW_ALIGNED_LINE(screen_state_p,length,command);
		}
		else if (y1 == y2)
		{
			/*
			 * Horizontal line.
			 */
			command = radial_line_command;
			if(x2 < x1)
			{
				/*
				 * Draw line from right to left
				 */
				length = x1 - x2;
				command |= S3_CMD_180_DGR_RADIAL_LINE;
			}
			else /* if(x2 > x1) */
			{
				/*
				 * Draw line from left to right 
				 */
				length = x2 - x1;
				command |= S3_CMD_0_DGR_RADIAL_LINE;
			}
			S3_DRAW_ALIGNED_LINE(screen_state_p,length,command);
		}
		else  
		{
			/*
			 * Lines that are neither purely vertical or horizontal.
			 * Sloped lines.
			 */
			command = axial_line_command;

			if((absolute_x_diff = x2 - x1) < 0 )
			{
				absolute_x_diff = -absolute_x_diff;
				error_adjust = -1;
			}
			else
			{
				command |= S3_CMD_AXIAL_X_LEFT_TO_RIGHT;
				error_adjust = 0;
			}

			if((absolute_y_diff = y2 - y1) < 0 )
			{
				absolute_y_diff = -absolute_y_diff;
			}
			else
			{
				command |= S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;
			}

			if( absolute_x_diff > absolute_y_diff )
			{
				length = absolute_x_diff;
				axial_step = absolute_y_diff << 1;
				diagonal_step = axial_step - (absolute_x_diff <<1);
				error_term = axial_step - absolute_x_diff;
			}
			else
			{
				length = absolute_y_diff;
				axial_step = absolute_x_diff << 1;
				diagonal_step = axial_step - (absolute_y_diff << 1);
				error_term  = axial_step - absolute_y_diff;
				command |= S3_CMD_AXIAL_Y_MAJOR;
			}

			S3_DRAW_SLOPED_LINE(screen_state_p, length, diagonal_step, 
				axial_step, error_term + error_adjust, command);
		}
	}while(--count);

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return (SI_SUCCEED);
}

STATIC SIBool
s3_line_segment_one_bit_dashed(SIint32 xorg, SIint32 yorg, SIint32 count,
		  SISegmentP psegsIn, SIint32 isCapNotLast)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURRENT_GRAPHICS_STATE_DECLARE();

	struct s3_line_state *line_state_p;

	/*
	 * absolute values of (x2-x1) and (y2-y1).
	 */
	int		absolute_x_diff;
	int		absolute_y_diff;

	int		error_term,error_adjust;
	int		axial_step,diagonal_step,length;

	unsigned short axial_line_command = 
		screen_state_p->cmd_flags |
		S3_CMD_TYPE_LINEDRAW | 
		S3_CMD_BUS_WIDTH_16 |
		S3_CMD_USE_PIXTRANS |
		S3_CMD_DRAW | 
		S3_CMD_PX_MD_ACROSS_PLANE | 
		S3_CMD_DIR_TYPE_AXIAL |
		S3_CMD_WRITE ;

	unsigned short radial_line_command = 
		screen_state_p->cmd_flags |
		S3_CMD_TYPE_LINEDRAW | 
		S3_CMD_BUS_WIDTH_16 |
		S3_CMD_USE_PIXTRANS |
		S3_CMD_DRAW | 
		S3_CMD_PX_MD_ACROSS_PLANE | 
		S3_CMD_DIR_TYPE_RADIAL | 
		S3_CMD_WRITE;

	short command;

    const int top_clip = screen_state_p->generic_state.screen_clip_top;
    const int bottom_clip = screen_state_p->generic_state.screen_clip_bottom;
    const int left_clip = screen_state_p->generic_state.screen_clip_left;
    const int right_clip = screen_state_p->generic_state.screen_clip_right;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));

	ASSERT((graphics_state_p->generic_state.si_graphics_state.
		SGlinestyle == SGLineDash) ||(graphics_state_p->generic_state.
		si_graphics_state.SGlinestyle == SGLineDblDash)); 

#if (defined(__DEBUG__))
	if (s3_line_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_line_segment_one_bit_dashed) {\n"
			"\txorg = %ld\n"
			"\tyorg = %ld\n"
			"\tcount = %ld\n"
			"\tpsegsIn = %p\n"
			"\tisCapNotLast = %ld\n"
			"}\n",
			xorg, yorg, count, (void *) psegsIn, isCapNotLast);
	}
#endif

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	
	if (count < 1)
	{
		return (SI_SUCCEED);
	}
	

	if (graphics_state_p->
		generic_state.si_graphics_state.SGlinestyle == SGLineDash)
	{
		S3_STATE_SYNCHRONIZE_STATE(screen_state_p, 
			S3_DASHED_LINE_DEPENDENCIES);
		
		/*
	 	 * Program the foreground and the background mix registers.
	 	 */
		S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
		S3_STATE_SET_BG_ROP(screen_state_p, S3_MIX_FN_LEAVE_C_AS_IS);
	}
	else if (graphics_state_p->
			 generic_state.si_graphics_state.SGlinestyle ==
			 SGLineDblDash)
	{
		S3_STATE_SYNCHRONIZE_STATE(screen_state_p, 
			S3_DOUBLE_DASHED_LINE_DEPENDENCIES);

		/*
	 	 * Program the foreground and the background mix registers.
	 	 */
		S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
		S3_SET_BG_COLOR_SOURCE(S3_CLR_SRC_BKGD_COLOR);
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
		
	line_state_p = &(graphics_state_p->current_line_state);
	if(line_state_p->is_pattern_valid == FALSE)
	{
		ASSERT(line_state_p->is_pattern_valid == TRUE);
		return (SI_FAIL);
	}

	/*
	 * Select the mix register. In this case CPU data determines the 
	 * the mix register.
	 */
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_CPU_DATA);

	if(isCapNotLast)
	{
		radial_line_command |= S3_CMD_LAST_PXOF;
		axial_line_command |= S3_CMD_LAST_PXOF;
	}

	do
	{
		int x1 = psegsIn->x1 + xorg;
		int y1 = psegsIn->y1 + yorg;
		int x2 = psegsIn->x2 + xorg;
		int y2 = psegsIn->y2 + yorg;
        register int oc1 = 0, oc2 = 0;

#if (defined(__DEBUG__))
		if (s3_line_debug)
		{
			(void) fprintf(debug_stream_p,
				"(s3_line_segment_one_bit_dashed){\n"
				"\t from %d,%d to %d,%d\n"
				"}\n",
				x1,y1,x2,y2);
		}
#endif
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
			if (s3_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"s3_line_segment_one_bit_dashed(%d,%d,%d,%d){\n"
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
			if (s3_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"s3_line_segment_one_bit_dashed(){\n"
					"\tclipping segment from (%d,%d,%d,%d)\n",
					x1,y1,x2,y2);
			}
#endif
			result = s3_line_clip_endpoints_to_physical_space(&x1,&y1,&x2,&y2);
#if (defined(__DEBUG__))
			if (s3_line_debug)
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
			if (s3_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"s3_line_segment_one_bit_dashed(%d,%d,%d,%d){\n"
					"\tsegment completely outside clipping rectangle\n"
					"}\n",
					x1,y1,x2,y2);
			}
#endif
			continue;
		}
		
		/*
		 * Program CUR_X, CUR_Y registers.
		 */
		S3_WAIT_FOR_FIFO(2);
		if (screen_state_p->use_mmio)
		{
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, x1);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y, y1);
		}
		else
		{
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, x1);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y, y1);
		}

		/*
  		 * Handle vertical and horizontal lines seperately, since 
		 * less register programming is required in these cases.
		 */
		if( x1 == x2)
		{
			/* 
			 * Vertical line.
			 */
			command = radial_line_command;
			if( y2 < y1 )
			{
				/*
				 * Draw line from bottom to top
				 */
				length = y1 - y2;
				command |= S3_CMD_90_DGR_RADIAL_LINE;
			}
			else /* if (y2 > y1) */
			{
				/*
				 * Draw the line from top to bottom
				 */
				length = y2 - y1;
				command |= S3_CMD_270_DGR_RADIAL_LINE;
			}
			S3_DRAW_DASHED_ALIGNED_LINE(screen_state_p,line_state_p,length,
				command);
		}
		else if (y1 == y2)
		{
			/*
			 * Horizontal line.
			 */
			command = radial_line_command;
			if(x2 < x1)
			{
				/*
				 * Draw line from right to left
				 */
				length = x1 - x2;
				command |= S3_CMD_180_DGR_RADIAL_LINE;
			}
			else /* if(x2 > x1) */
			{
				/*
				 * Draw line from left to right 
				 */
				length = x2 - x1;
				command |= S3_CMD_0_DGR_RADIAL_LINE;
			}
			S3_DRAW_DASHED_ALIGNED_LINE(screen_state_p,line_state_p,length,
				command);
		}
		else  
		{
			/*
			 * Lines that are neither purely vertical or horizontal.
			 * Sloped lines.
			 */
			command = axial_line_command;

			if((absolute_x_diff = x2 - x1) < 0 )
			{
				absolute_x_diff = -absolute_x_diff;
				error_adjust = -1;
			}
			else
			{
				command |= S3_CMD_AXIAL_X_LEFT_TO_RIGHT;
				error_adjust = 0;
			}

			if((absolute_y_diff = y2 - y1) < 0 )
			{
				absolute_y_diff = -absolute_y_diff;
			}
			else
			{
				command |= S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;
			}

			if( absolute_x_diff > absolute_y_diff )
			{
				length = absolute_x_diff;
				axial_step = absolute_y_diff << 1;
				diagonal_step = axial_step - (absolute_x_diff <<1);
				error_term = axial_step - absolute_x_diff;
			}
			else
			{
				length = absolute_y_diff;
				axial_step = absolute_x_diff << 1;
				diagonal_step = axial_step - (absolute_y_diff << 1);
				error_term  = axial_step - absolute_y_diff;
				command |= S3_CMD_AXIAL_Y_MAJOR;
			}

			S3_DRAW_DASHED_SLOPED_LINE(screen_state_p, line_state_p,length, 
				diagonal_step, axial_step, error_term + error_adjust, command);
		}
	}while(--count);

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
#if (defined(__DEBUG__))
	if (s3_line_debug)
	{
		(void) fprintf(debug_stream_p,
			"Quitting dashed segment\n");
	}
#endif

	return (SI_SUCCEED);
}


/*
 * s3_line_one_bit_solid:
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
s3_line_one_bit_solid(SIint32 xorg, SIint32 yorg, SIint32 count, 
	       SIPointP ptsIn, SIint32 isCapNotLast, SIint32 coordMode)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURRENT_GRAPHICS_STATE_DECLARE();

	/*
	 * absolute values of (x2-x1) and (y2-y1).
	 */
	int		absolute_x_diff;
	int		absolute_y_diff;

	int		error_term,error_adjust;
	int		axial_step,diagonal_step,length;

	short axial_line_command = 
		screen_state_p->cmd_flags |
		S3_CMD_TYPE_LINEDRAW | 
		S3_CMD_DRAW | 
		S3_CMD_LAST_PXOF |
		S3_CMD_PX_MD_ACROSS_PLANE | 
		S3_CMD_DIR_TYPE_AXIAL |
		S3_CMD_WRITE ;

	short radial_line_command = 
		screen_state_p->cmd_flags |
		S3_CMD_TYPE_LINEDRAW | 
		S3_CMD_DRAW | 
		S3_CMD_LAST_PXOF |
		S3_CMD_PX_MD_ACROSS_PLANE | 
		S3_CMD_DIR_TYPE_RADIAL | 
		S3_CMD_WRITE;

	short 	command;

	int  	x1,y1,x2,y2;

	int		unclipped_x1,unclipped_y1;
	int 	oc1 = 0, oc2 = 0;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));

	ASSERT(graphics_state_p->generic_state.si_graphics_state.
		SGlinestyle == SGLineSolid);
	
#if (defined(__DEBUG__))
	if (s3_line_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_line_one_bit_solid) {\n"
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

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	if (count <= 1)
	{
		return (SI_SUCCEED);
	}

	S3_STATE_SYNCHRONIZE_STATE(screen_state_p, S3_SOLID_LINE_DEPENDENCIES);
	
	/*
	 * Select the frgd mix logic and the foreground color register as
	 * the frgd color source.
	 */
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);

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
			if (s3_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"s3_line_one_bit_solid(%d,%d,%d,%d){\n"
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
			if (s3_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"s3_line_one_bit_solid(){\n"
					"\tclipping segment from (%d,%d,%d,%d)\n",
					x1,y1,x2,y2);
			}
#endif
			result = s3_line_clip_endpoints_to_physical_space(&x1,&y1,&x2,&y2);
#if (defined(__DEBUG__))
			if (s3_line_debug)
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
		 */
		S3_WAIT_FOR_FIFO(2);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,x1);
		S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,y1);

		/*
  		 * Handle vertical and horizontal lines seperately, since
		 * less register programming is required in these cases.
		 */
		if( x1 == x2)
		{
			/* 
			 * Vertical line.
			 */
			command = radial_line_command;
			if( y2 < y1 )
			{
				/*
				 * Draw line from bottom to top
				 */
				length = y1 - y2 ;
				command |= S3_CMD_90_DGR_RADIAL_LINE;
			}
			else /* if (y2 > y1) */
			{
				/*
				 * Draw the line from top to bottom
				 */
				length = y2 - y1 ;
				command |= S3_CMD_270_DGR_RADIAL_LINE;
			}
			S3_DRAW_ALIGNED_LINE(screen_state_p,length,command);
		}
		else if (y1 == y2)
		{
			/*
			 * Horizontal line.
			 */
			command = radial_line_command;
			if(x2 < x1)
			{
				/*
				 * Draw line from right to left
				 */
				length = x1 - x2 ;
				command |= S3_CMD_180_DGR_RADIAL_LINE;
			}
			else /* if(x2 > x1) */
			{
				/*
				 * Draw line from left to right 
				 */
				length = x2 - x1 ;
				command |= S3_CMD_0_DGR_RADIAL_LINE;
			}
			S3_DRAW_ALIGNED_LINE(screen_state_p,length,command);
		}
		else  
		{
			/*
			 * Lines that are neither purely vertical or horizontal.
			 * Sloped lines.
			 */
			command = axial_line_command;

			if((absolute_x_diff = x2 - x1 ) < 0 )
			{
				absolute_x_diff = -absolute_x_diff;
				error_adjust = -1;
			}
			else
			{
				command |= S3_CMD_AXIAL_X_LEFT_TO_RIGHT;
				error_adjust = 0;
			}

			if((absolute_y_diff = y2 - y1 ) < 0 )
			{
				absolute_y_diff = -absolute_y_diff;
			}
			else
			{
				command |= S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;
			}

			if( absolute_x_diff > absolute_y_diff )
			{
				length = absolute_x_diff;
				axial_step = absolute_y_diff << 1;
				diagonal_step = axial_step - (absolute_x_diff <<1);
				error_term = axial_step - absolute_x_diff;
			}
			else
			{
				length = absolute_y_diff;
				axial_step = absolute_x_diff << 1;
				diagonal_step = axial_step - (absolute_y_diff << 1);
				error_term  = axial_step - absolute_y_diff;
				command |= S3_CMD_AXIAL_Y_MAJOR;
			}

			S3_DRAW_SLOPED_LINE(screen_state_p, length, diagonal_step, 
				axial_step, error_term + error_adjust, command);

		}
	}while( --count > 1);

	/*
	 * Draw the last point of the last line, if CapNotLast is FALSE
	 * and if the last point is within the device coordinate space
	 * and if the ending pixels of the last line were not clipped.
	 */
	if(!isCapNotLast)
	{
		if ((!oc2) && ( x2 == unclipped_x1) && (y2 == unclipped_y1))
		{
			S3_WAIT_FOR_FIFO(4);
			if (screen_state_p->use_mmio)
			{
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X,x2);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_Y,y2);
				S3_MMIO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,1);
				S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
					radial_line_command);
			}
			else
			{
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_X,x2);
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_CUR_Y,y2);
				S3_IO_SET_ENHANCED_REGISTER(
					S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,1);
				S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
					radial_line_command);
			}
		}
	}	

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return (SI_SUCCEED);
}

/*
 * s3_one_bit_rectangle. 
 */
STATIC SIBool
s3_line_rectangle_one_bit(SIint32 x1, SIint32 y1, SIint32 x2, SIint32 y2)
{
	/*
	 * For dashed lines call the line segment routine.
	 */
	SISegment rect_segs[4];

	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURRENT_GRAPHICS_STATE_DECLARE();

	unsigned short radial_line_command =  
		screen_state_p->cmd_flags |
		S3_CMD_TYPE_LINEDRAW | 
		S3_CMD_LAST_PXOF |
		S3_CMD_DRAW | 
		S3_CMD_PX_MD_ACROSS_PLANE | 
		S3_CMD_DIR_TYPE_RADIAL | 
		S3_CMD_WRITE;

	int		oc1 = 0,oc2 = 0;

	/* 
	 * whether to call thro line segments or not.
	 */
	boolean	call_through_line_segment = FALSE;  
	 
	
	const	unsigned short width = x2 - x1;
	const 	unsigned short height = y2 - y1;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));

#if (defined(__DEBUG__))
	if (s3_line_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_one_bit_rectangle) {\n"
			"\tx1 = %ld\n"
			"\ty1 = %ld\n"
			"\tx2 = %ld\n"
			"\ty2 = %ld\n"
			"}\n",
			x1, y1, x2, y2);
	}
#endif

	OUTCODE(oc1,x1,y1,
		0, screen_state_p->generic_state.screen_physical_width - 1,
		0, screen_state_p->generic_state.screen_physical_height - 1);
	OUTCODE(oc2,x2,y2,
		0, screen_state_p->generic_state.screen_physical_width - 1,
		0, screen_state_p->generic_state.screen_physical_height - 1);

	if (oc1 & oc2) 	/* completely outside the physical bounds */
	{
#if (defined(__DEBUG__))
		if (s3_line_debug)
		{
			(void) fprintf(debug_stream_p,
				"s3_line_one_rectangle_one_bit(%ld,%ld,%ld,%ld){\n"
				"\trectangle completely outside physical coords space\n"
				"}\n",
				x1,y1,x2,y2);
		}
#endif
		return (SI_SUCCEED) ;
	}

	if ((graphics_state_p->generic_state.si_graphics_state.SGlinestyle ==
		SGLineDash) ||
		(graphics_state_p->generic_state.si_graphics_state.SGlinestyle ==
		SGLineDblDash))
	{
		if (graphics_state_p->current_line_state.is_pattern_valid == TRUE)
		{
			call_through_line_segment = TRUE;
		}
		else
		{
			return(SI_FAIL);
		}
	}

	/*
	 *  Only solid lines to be handled now. Dashed lines are handled
	 *  above.
	 */
	ASSERT(graphics_state_p->
		   generic_state.si_graphics_state.SGlinestyle == SGLineSolid);

	/*
	 * if line solid and out intersecting physical bounds.
	 */
	if (oc1 | oc2)
	{
		call_through_line_segment = TRUE;
	}

	if (call_through_line_segment == TRUE)
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
	
	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	S3_STATE_SYNCHRONIZE_STATE(screen_state_p, S3_SOLID_LINE_DEPENDENCIES);
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);

	if(screen_state_p->use_mmio)
	{
		S3_WAIT_FOR_FIFO(2);
		S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X, x1);
		S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, y1);

		S3_INLINE_WAIT_FOR_FIFO(8);
		S3_MMIO_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,width);
		S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
			radial_line_command | S3_CMD_0_DGR_RADIAL_LINE);

		S3_MMIO_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,height);
		S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
			radial_line_command | S3_CMD_270_DGR_RADIAL_LINE);

		S3_MMIO_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,width);
		S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
			radial_line_command | S3_CMD_180_DGR_RADIAL_LINE);

		S3_MMIO_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,height);
		S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
			radial_line_command | S3_CMD_90_DGR_RADIAL_LINE);
	}
	else
	{
		S3_WAIT_FOR_FIFO(2);
		S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X, x1);
		S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, y1);

		S3_INLINE_WAIT_FOR_FIFO(8);
		S3_IO_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,width);
		S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
			radial_line_command | S3_CMD_0_DGR_RADIAL_LINE);

		S3_IO_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,height);
		S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
			radial_line_command | S3_CMD_270_DGR_RADIAL_LINE);

		S3_IO_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,width);
		S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
			radial_line_command | S3_CMD_180_DGR_RADIAL_LINE);

		S3_IO_SET_ENHANCED_REGISTER(
			S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,height);
		S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
			radial_line_command | S3_CMD_90_DGR_RADIAL_LINE);
	}

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return (SI_SUCCEED);
}

/*
 * Further optimizations are possible by singling out the horizontal
 * and vertical lines ( See the r5 entry point for solid linedraw ).
 */
STATIC SIBool
s3_line_compat_one_bit_solid(SIint32 count,  SIPointP ptsIn)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
#if (defined(__DEBUG__))
	S3_CURRENT_GRAPHICS_STATE_DECLARE();
#endif
	unsigned short axial_line_command = 
		screen_state_p->cmd_flags |
		S3_CMD_TYPE_LINEDRAW | 
		S3_CMD_DRAW | 
		S3_CMD_LAST_PXOF |
		S3_CMD_PX_MD_ACROSS_PLANE | 
		S3_CMD_DIR_TYPE_AXIAL |
		S3_CMD_WRITE ;

	short radial_line_command = 
		screen_state_p->cmd_flags |
		S3_CMD_TYPE_LINEDRAW | 
		S3_CMD_DRAW | 
		S3_CMD_LAST_PXOF |
		S3_CMD_PX_MD_ACROSS_PLANE | 
		S3_CMD_DIR_TYPE_RADIAL | 
		S3_CMD_WRITE;
	/*
	 * absolute values of (x2-x1) and (y2-y1).
	 */
	int		absolute_x_diff;
	int		absolute_y_diff;

	int		error_term,error_adjust;
	int		axial_step,diagonal_step,length;

	int  	x1,y1,x2,y2;
	int	 	unclipped_x1,unclipped_y1;
	int 	oc1 = 0, oc2 = 0;
	unsigned short command;

	
#if (defined(__DEBUG__))
	if (s3_line_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_line_compat_one_bit_solid)\n"
			"{\n"
			"\tcount = %ld\n"
			"\tptsIn = %p\n"
			"}\n",
			count, (void *) ptsIn);
	}
#endif

	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE, graphics_state_p));
	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGlinestyle == 
		   SGLineSolid);
	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	if (count <= 1)
	{
		return (SI_SUCCEED);
	}

	S3_STATE_SYNCHRONIZE_STATE(screen_state_p, S3_SOLID_LINE_DEPENDENCIES);
	
	/*
	 * Select the frgd mix logic and the foreground color register as
	 * the frgd color source.
	 */
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);

	x2 = ptsIn->x;
	y2 = ptsIn->y;
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

		++ptsIn;

		x2 = ptsIn->x;
		y2 = ptsIn->y;
		unclipped_x1 = x2;
		unclipped_y1 = y2;

		/*
		 * Check the lines against the physical coordinate space.
		 */
		OUTCODE(oc2,x2,y2,
			0, screen_state_p->generic_state.screen_physical_width - 1,
			0, screen_state_p->generic_state.screen_physical_height - 1);

		if (oc1 & oc2) 	/* completely outside the physical bounds */
		{
#if (defined(__DEBUG__))
			if (s3_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"s3_line_compat_one_bit_solid(%d,%d,%d,%d){\n"
					"\tline completely outside physical coords space\n"
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
			if (s3_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"s3_line_compat_one_bit_solid(){\n"
					"\tclipping line from (%d,%d,%d,%d)\n",
					x1,y1,x2,y2);
			}
#endif
			result = s3_line_clip_endpoints_to_physical_space(&x1,&y1,&x2,&y2);
#if (defined(__DEBUG__))
			if (s3_line_debug)
			{
				(void) fprintf(debug_stream_p,
					"\tto (%d,%d,%d,%d)\n"
					"}\n",
					x1,y1,x2,y2);
			}
#endif
			/* Clipping yeilded a line within the physical coordinate space. */
			if (result == TRUE)
			{
				/*
				 * Set the current x and current y
				 */
				S3_WAIT_FOR_FIFO(2);
				S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,x1);
				S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,y1);
			}
			else
			{
				continue;
			}
		}
		else /* line within the physical coordinate space */
		{
			/*
			 * Set the current x and current y
			 */
			S3_WAIT_FOR_FIFO(2);
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X,x1);
			S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y,y1);
		}

		/*
		 * Lines that are neither purely vertical or horizontal.
		 * Sloped lines.
		 */
		command = axial_line_command;

		if((absolute_x_diff = x2 - x1 ) < 0 )
		{
			absolute_x_diff = -absolute_x_diff;
			error_adjust = -1;
		}
		else
		{
			command |= S3_CMD_AXIAL_X_LEFT_TO_RIGHT;
			error_adjust = 0;
		}

		if((absolute_y_diff = y2 - y1 ) < 0 )
		{
			absolute_y_diff = -absolute_y_diff;
		}
		else
		{
			command |= S3_CMD_AXIAL_Y_TOP_TO_BOTTOM;
		}

		if( absolute_x_diff > absolute_y_diff )
		{
			length = absolute_x_diff;
			axial_step = absolute_y_diff << 1;
			diagonal_step = axial_step - (absolute_x_diff <<1);
			error_term = axial_step - absolute_x_diff;
		}
		else
		{
			length = absolute_y_diff;
			axial_step = absolute_x_diff << 1;
			diagonal_step = axial_step - (absolute_y_diff << 1);
			error_term  = axial_step - absolute_y_diff;
			command |= S3_CMD_AXIAL_Y_MAJOR;
		}

		S3_DRAW_SLOPED_LINE(screen_state_p, length, diagonal_step, 
			axial_step, error_term + error_adjust, command);
	} while( --count > 1 );

	/*
	 * Draw the last point of the last line. There are no capnotlast 
	 * semantics to be followed in the r4 entry point. But draw the
	 * last point only if it is within the physical coordinate space.
	 */
	if ((!oc2) && ( x2 == unclipped_x1) && (y2 == unclipped_y1))
	{
		S3_WAIT_FOR_FIFO(4);
		if (screen_state_p->use_mmio)
		{
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X,x2);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y,y2);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,1);
			S3_MMIO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
				radial_line_command);
		}
		else
		{
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X,x2);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y,y2);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,1);
			S3_IO_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
				radial_line_command);
		}
	}

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return(SI_SUCCEED);
}

STATIC SIBool
s3_line_compat_one_bit_dashed(SIint32 count,  SIPointP ptsIn)
{

#if (defined(__DEBUG__))
	S3_CURRENT_GRAPHICS_STATE_DECLARE();
#endif

	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE, graphics_state_p));
	
#if (defined(__DEBUG__))
	if (s3_line_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_line_compat_one_bit_dashed)\n"
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
	return s3_line_one_bit_dashed (0, 0, count, 
									 ptsIn, TRUE /* isCapNotLast*/, 
									 SICoordModeOrigin /* coordMode */);
}

/*
 * For the segment draw routines, we only need to do minor
 * transformations on the inputs.
 */
STATIC SIBool
s3_line_segment_compat_one_bit_solid(SIint32 count, SIPointP ptsIn)
{

#if (defined(__DEBUG__))
	if (s3_line_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_line_segment_compat_one_bit_solid)\n"
			"{\n"
			"\tcount = %ld\n"
			"\tptsIn = %p\n"
			"}\n",
			 count, (void *) ptsIn);
	}
#endif
	
	return s3_line_segment_one_bit_solid(0, 0, count >> 1, 
			(SISegmentP) ptsIn, FALSE);
}

STATIC SIBool
s3_line_segment_compat_one_bit_dashed(SIint32 count, SIPointP ptsIn)
{

#if (defined(__DEBUG__))
	if (s3_line_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_line_segment_compat_one_bit_dashed)\n"
			"{\n"
			"\tcount = %ld\n"
			"\tptsIn = %p\n"
			"}\n",
			count, (void *) ptsIn);
	}
#endif
	
	return s3_line_segment_one_bit_dashed(0, 0, count >> 1, 
					(SISegmentP) ptsIn, FALSE); 
}


/*
 * Graphics state change.
 * 
 * We change the function pointers depending on whether the line style
 * is solid or dashed.
 */

function void
s3_line__gs_change__(void)
{
	S3_CURRENT_SCREEN_STATE_DECLARE();
	S3_CURRENT_GRAPHICS_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		   (struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE, graphics_state_p));
	
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
	 * Line rectangles are the same for the old and new servers.
	 */
	if(screen_state_p->options_p->linedraw_options & 
		  S3_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_RECTANGLES)
	{
		screen_state_p->generic_state.screen_functions_p->
			si_line_onebitrect = s3_line_rectangle_one_bit;
	}

	/*
	 * We do not handle dashed lines when fill mode is not
	 * FillSolid, the problem being non horizontal lines are not
	 * easily handled by the drawing engine's mechanisms.
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
				screen_state_p->generic_state.screen_functions_p->
					si_line_onebitline = s3_line_one_bit_solid;
				screen_state_p->generic_state.screen_functions_p->
					si_line_onebitseg =  s3_line_segment_one_bit_solid;
			}
			else if ((screen_state_p->options_p->linedraw_options & 
					  S3_OPTIONS_LINEDRAW_OPTIONS_USE_DASHED_LINE) &&
					 ((graphics_state_p->
					   generic_state.si_graphics_state.SGlinestyle ==
					   SGLineDash) ||
					  (graphics_state_p->
					   generic_state.si_graphics_state.SGlinestyle ==
					   SGLineDblDash)) &&
					  (graphics_state_p->current_line_state.
						is_pattern_valid == TRUE))
			{
				screen_state_p->generic_state.screen_functions_p->
					si_line_onebitline = s3_line_one_bit_dashed;
				screen_state_p->generic_state.screen_functions_p->
					si_line_onebitseg = s3_line_segment_one_bit_dashed;
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
					S3_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_DRAW)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitline =
						(SIBool (*)(SIint32, SIint32, SIint32, SIPointP,
						SIint32, SIint32)) s3_line_compat_one_bit_solid;
				}
				if (screen_state_p->options_p->linedraw_options &
					S3_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitseg =  
				        (SIBool (*)(SIint32, SIint32, 
								SIint32, SISegmentP, SIint32))
					    s3_line_segment_compat_one_bit_solid;
				}
			}
			else if ((screen_state_p->options_p->linedraw_options & 
					  S3_OPTIONS_LINEDRAW_OPTIONS_USE_DASHED_LINE) &&
					 ((graphics_state_p->
					   generic_state.si_graphics_state.SGlinestyle ==
					   SGLineDash) ||
					  (graphics_state_p->
					   generic_state.si_graphics_state.SGlinestyle ==
					   SGLineDblDash)) &&
					  (graphics_state_p->current_line_state.
						is_pattern_valid == TRUE))
			{
				/* NO dashed, double dashed lines yet.  */
				if (screen_state_p->options_p->linedraw_options &
					S3_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW)
				{
					screen_state_p->generic_state.screen_functions_p->
						si_line_onebitseg =  
				        (SIBool (*)(SIint32, SIint32, 
								SIint32, SISegmentP, SIint32))
					    s3_line_segment_compat_one_bit_dashed;
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
s3_line__initialize__(SIScreenRec *si_screen_p,
						struct s3_options_structure * options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;

	flags_p->SIavail_line = 0;
	functions_p->si_line_onebitline = s3_no_operation_fail;
	functions_p->si_line_onebitseg = s3_no_operation_fail;
	functions_p->si_line_onebitrect = s3_no_operation_fail;

	if (options_p->linedraw_options &
		S3_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_DRAW)
	{
		flags_p->SIavail_line |= ONEBITLINE_AVAIL;
		functions_p->si_line_onebitline = s3_line_one_bit_solid;
	}
	if (options_p->linedraw_options &
		S3_OPTIONS_LINEDRAW_OPTIONS_USE_LINE_RECTANGLES)
	{
		flags_p->SIavail_line |= ONEBITRECT_AVAIL;
		functions_p->si_line_onebitrect = s3_line_rectangle_one_bit;
	}
	if (options_p->linedraw_options &
		S3_OPTIONS_LINEDRAW_OPTIONS_USE_SEGMENT_DRAW)
	{
		flags_p->SIavail_line |= ONEBITSEG_AVAIL;
		functions_p->si_line_onebitseg = s3_line_segment_one_bit_solid;
	}

	if (options_p->linedraw_options &
		S3_OPTIONS_LINEDRAW_OPTIONS_USE_DASHED_LINE)
	{
		flags_p->SIavail_line |= (DASH_AVAIL | DBLDASH_AVAIL);
	}
}

