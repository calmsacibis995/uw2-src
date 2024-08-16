/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m_points.c	1.3"

/***
 ***	NAME
 ***
 ***		mach_points.c : point plotting routines for the mach library.
 ***		
 ***	SYNOPSIS
 ***
 ***		#include "m_points.c"
 ***
 ***	DESCRIPTION
 ***
 ***		This module implements point plotting functionality for
 ***	the MACH display library.
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
export boolean mach_polypoint_debug = FALSE;
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

#define MACH_POLYPOINT_DEPENDENCIES\
	(MACH_INVALID_FOREGROUND_COLOR |\
	 MACH_INVALID_WRT_MASK |\
	 MACH_INVALID_FG_ROP)

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/

STATIC SIBool
mach_plot_points(SIint32 count, SIPointP points_p)
{
	unsigned short dp_config;
	
	MACH_CURRENT_SCREEN_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		(struct generic_screen_state *) screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE, screen_state_p));

	ASSERT(!MACH_IS_IO_ERROR());
	
	if (count <= 0)
	{
		return (SI_SUCCEED);
	}
	
	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	/*
	 * Reset the clip rectangle to full screen
	 */
	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{

#if (defined(__DEBUG__))
		if (mach_polypoint_debug)
		{
			(void) fprintf(debug_stream_p,
"(mach_plot_points) resetting clip rectangle {\n"
"\tfrom (%hd,%hd),(%hd,%hd)\n"
"\tto (%d,%d),(%d,%d)\n"
"}\n",
					   
				   screen_state_p->register_state.ext_scissor_l,
				   screen_state_p->register_state.ext_scissor_t,
				   screen_state_p->register_state.ext_scissor_r,
				   screen_state_p->register_state.ext_scissor_b,
				   0,
				   0,
				   screen_state_p->generic_state.screen_virtual_width-1,
				   screen_state_p->generic_state.screen_virtual_height-1);
		}
#endif
	  
		MACH_STATE_SET_ATI_CLIP_RECTANGLE(screen_state_p, 0, 0,
						screen_state_p->generic_state.screen_virtual_width,
						screen_state_p->generic_state.screen_virtual_height);
		
		ASSERT(!MACH_IS_IO_ERROR());
		
		screen_state_p->generic_state.screen_current_clip =
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
		MACH_STATE_SET_FLAGS(screen_state_p, MACH_INVALID_CLIP);
								/* mark deviation from SI's clip */
	}

	/*
	 * Synchronize registers with the graphics state.
	 */
	MACH_STATE_SYNCHRONIZE_STATE(screen_state_p,
								 MACH_POLYPOINT_DEPENDENCIES);
	
	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_FG_COLOR_SRC_FRGD_COLOR |
		MACH_DP_CONFIG_BG_COLOR_SRC_BKGD_COLOR |
		MACH_DP_CONFIG_MONO_SRC_ONE |
		MACH_DP_CONFIG_ENABLE_DRAW |
		MACH_DP_CONFIG_WRITE;

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);
	
#if (defined(__DEBUG__))
	if (mach_polypoint_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_plot_points)\n"
"{\n"
"\tcount = %ld\n"
"\tpoints_p = %p\n"
"\tdp_config = 0x%x\n"
"}\n",
					   count, (void *) points_p, dp_config);
		
	}
#endif

	/*
	 * Call the point plotter routine.
	 */
	mach_asm_plot_points(count, points_p);
	
	ASSERT(!MACH_IS_IO_ERROR());
	
	return (SI_SUCCEED);
	
}

function void
mach_polypoint__gs_change__()
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
	
	if (screen_state_p->options_p->pointdraw_options &
		MACH_OPTIONS_POINTDRAW_OPTIONS_USE_PLOT_POINT)
	{
		screen_state_p->generic_state.screen_functions_p->
			si_plot_points = mach_plot_points;
	}
	else
	{
		screen_state_p->generic_state.screen_functions_p->si_plot_points =
			graphics_state_p->generic_si_functions.si_plot_points;
	}
}

	
function void
mach_polypoint__initialize__(SIScreenRec *si_screen_p,
							 struct mach_options_structure *
							 options_p)
{
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	SIFunctionsP functions_p = si_screen_p->funcsPtr;

	if (options_p->pointdraw_options &
		MACH_OPTIONS_POINTDRAW_OPTIONS_USE_PLOT_POINT)
	{
		flags_p->SIavail_point = PLOTPOINT_AVAIL;
		functions_p->si_plot_points = mach_plot_points;
	}
}
