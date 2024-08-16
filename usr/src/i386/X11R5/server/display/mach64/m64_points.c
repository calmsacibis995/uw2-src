/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_points.c	1.2"

/***
 ***	NAME
 ***
 ***		m64_points.c : point plotting routines for the m64 library.
 ***		
 ***	SYNOPSIS
 ***
 ***		#include "m64_points.c"
 ***
 ***	DESCRIPTION
 ***
 ***		This module implements point plotting functionality for
 ***	the M64 display library.
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
export boolean m64_points_debug = 0;
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
#include "m64_gbls.h"
#include "m64_opt.h"
#include "m64_gs.h"
#include "m64_regs.h"
#include "m64_state.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/

STATIC SIBool
m64_plot_points(SIint32 count, SIPointP points_p)
{
	
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	unsigned long *register_values_p = screen_state_p->register_state.registers;
	const SIPointP si_points_last_fence_p = points_p + count;
	register SIPointP current_point_p = points_p;

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));

#if (defined(__DEBUG__))
	if (m64_points_debug)
	{
		(void) fprintf(debug_stream_p,
			"(m64_plot_points) {\n"
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

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
	/*
	 * Set the clipping rectangle to cover virtual screen, only if
	 * required.
	 */
	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
		M64_WAIT_FOR_FIFO(2);
		register_base_address_p[M64_REGISTER_SC_LEFT_RIGHT_OFFSET] = 
		((unsigned)(screen_state_p->generic_state.screen_virtual_width - 1) <<
			SC_LEFT_RIGHT_SC_RIGHT_SHIFT) & SC_LEFT_RIGHT_BITS;
		register_base_address_p[M64_REGISTER_SC_TOP_BOTTOM_OFFSET] =
		((unsigned)(screen_state_p->generic_state.screen_virtual_height - 1) <<
			SC_TOP_BOTTOM_SC_BOTTOM_SHIFT) & SC_TOP_BOTTOM_BITS;
		screen_state_p->generic_state.screen_current_clip = 
			GENERIC_CLIP_TO_VIRTUAL_SCREEN;
	}

	/*
	 * Select the foreground color source and the trajectory control register
	 * values.
	 */
	M64_WAIT_FOR_FIFO(2);
	*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
		DP_SRC_FRGD_COLOR << DP_SRC_DP_FRGD_SRC_SHIFT |
		DP_SRC_DP_MONO_SRC_ALWAYS_1 ;
	*(register_base_address_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) = 	
		*(register_values_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET);
		
	do 
	{
		/*
		 * draw the points as one bit filled rectangles.
		 */
		M64_WAIT_FOR_FIFO(2);
		*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = 
			current_point_p->y | 
			((unsigned)current_point_p->x << DST_Y_X_DST_X_SHIFT);
		*(register_base_address_p + M64_REGISTER_DST_HEIGHT_WID_OFFSET) = 
			0x00010001;
	}while( ++current_point_p < si_points_last_fence_p);

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
	return (SI_SUCCEED);
}

#ifdef DELETE
function void
m64_polypoint__gs_change__()
{
	M64_CURRENT_GRAPHICS_STATE_DECLARE();

	M64_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));
	
	return;
}
#endif

function void
m64_polypoint__initialize__(SIScreenRec *si_screen_p,
							 struct m64_options_structure *
							 options_p)
{
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	SIFunctionsP functions_p = si_screen_p->funcsPtr;

	if (options_p->pointdraw_options &
		M64_OPTIONS_POINTDRAW_OPTIONS_USE_PLOT_POINT)
	{
		flags_p->SIavail_point = PLOTPOINT_AVAIL;
		functions_p->si_plot_points = m64_plot_points;
	}
}
