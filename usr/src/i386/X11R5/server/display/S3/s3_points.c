/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/s3_points.c	1.3"
/***
 ***	NAME
 ***
 ***		s3_points.c : point plotting routines for the s3 library.
 ***		
 ***	SYNOPSIS
 ***
 ***		#include "s3_points.c"
 ***
 ***	DESCRIPTION
 ***
 ***		This module implements point plotting functionality for
 ***	the S3 display library.
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
export boolean s3_polypoint_debug = FALSE;
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
#include "s3_regs.h"

/***
 ***	Constants.
 ***/
#define S3_POLYPOINT_DEPENDENCIES \
	(S3_INVALID_FOREGROUND_COLOR | S3_INVALID_WRT_MASK | S3_INVALID_FG_ROP)

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/

STATIC SIBool
s3_plot_points(SIint32 count, SIPointP points_p)
{
	
	S3_CURRENT_SCREEN_STATE_DECLARE();

	const SIPointP si_points_last_fence_p = points_p + count;

	const unsigned short radial_line_command = 
		screen_state_p->cmd_flags |
		S3_CMD_TYPE_LINEDRAW | 
		S3_CMD_DRAW | 
		S3_CMD_PX_MD_ACROSS_PLANE | 
		S3_CMD_DIR_TYPE_RADIAL | 
		S3_CMD_WRITE;

	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

#if (defined(__DEBUG__))
	if (s3_polypoint_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s3_plot_points) {\n"
				"\tcount = %ld\n"
				"\tpoints_p = %p\n"
				"}\n",
				count, (void *) points_p);
	}
#endif
		
	if (count <= 0)
	{
		return(SI_SUCCEED);
	}

	/*
	 * Reset the clip rectangle to full screen.
	 */
	if (screen_state_p->generic_state.screen_current_clip != 
		GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
#if (defined(__DEBUG__))
		if (s3_polypoint_debug)
		{
			(void) fprintf(debug_stream_p,
				"(s3_plot_points) resetting clip rectangle {\n"
				"\tfrom (%hd,%hd),(%hd,%hd)\n"
				"\tto (%d,%d),(%d,%d)\n"
				"}\n",
				screen_state_p->
					register_state.s3_enhanced_commands_registers.scissor_l,
				screen_state_p->
					register_state.s3_enhanced_commands_registers.scissor_t,
				screen_state_p->
					register_state.s3_enhanced_commands_registers.scissor_r,
				screen_state_p->
					register_state.s3_enhanced_commands_registers.scissor_b,
			   	0,
			   	0,
				screen_state_p->generic_state.screen_virtual_width-1,
				screen_state_p->generic_state.screen_virtual_height-1);
		}
#endif
		S3_STATE_SET_CLIP_RECTANGLE(screen_state_p, 0, 0,
			screen_state_p->generic_state.screen_virtual_width,
			screen_state_p->generic_state.screen_virtual_height);
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
		S3_STATE_SET_FLAGS(screen_state_p, S3_INVALID_CLIP_RECTANGLE);
	}


	S3_STATE_SYNCHRONIZE_STATE(screen_state_p, S3_POLYPOINT_DEPENDENCIES);

	/*
	 * Program the foreground mix register.
	 * For plotting a point, a 1 pixel long radial line at 0 degree is drawn.
	 */
	S3_WAIT_FOR_FIFO(3);
	S3_SET_FG_COLOR_SOURCE(S3_CLR_SRC_FRGD_COLOR);
	S3_SET_PIX_CNTL_MIX_REGISTER_SELECT(PIX_CNTL_DT_EX_SRC_FRGD_MIX);
	S3_SET_ENHANCED_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT,0);

	if(screen_state_p->use_mmio)
	{
		register SIPointP current_point_p = points_p;

		do 
		{
			S3_INLINE_WAIT_FOR_FIFO(3);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, current_point_p->x);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y,current_point_p->y);
			S3_MMIO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CMD, radial_line_command);
		}while( ++current_point_p < si_points_last_fence_p);
	}
	else
	{
		register SIPointP current_point_p = points_p;

		do 
		{
			S3_INLINE_WAIT_FOR_FIFO(3);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_X, current_point_p->x);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CUR_Y,current_point_p->y);
			S3_IO_SET_ENHANCED_REGISTER(
				S3_ENHANCED_COMMAND_REGISTER_CMD, radial_line_command);
		}while(++current_point_p < si_points_last_fence_p);
	}

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return (SI_SUCCEED);
}

function void
s3_polypoint__gs_change__()
{
	S3_CURRENT_GRAPHICS_STATE_DECLARE();

	S3_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(S3_SCREEN_STATE, screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(S3_GRAPHICS_STATE, graphics_state_p));
	
	if (screen_state_p->options_p->pointdraw_options &
		S3_OPTIONS_POINTDRAW_OPTIONS_USE_PLOT_POINT)
	{
		screen_state_p->generic_state.screen_functions_p->
			si_plot_points = s3_plot_points;
	}
	else
	{
		screen_state_p->generic_state.screen_functions_p->si_plot_points =
			graphics_state_p->generic_si_functions.si_plot_points;
	}
	return;
}

function void
s3_polypoint__initialize__(SIScreenRec *si_screen_p,
							 struct s3_options_structure *
							 options_p)
{
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	SIFunctionsP functions_p = si_screen_p->funcsPtr;

	if (options_p->pointdraw_options &
		S3_OPTIONS_POINTDRAW_OPTIONS_USE_PLOT_POINT)
	{
		flags_p->SIavail_point = PLOTPOINT_AVAIL;
		functions_p->si_plot_points = s3_plot_points;
	}
}
