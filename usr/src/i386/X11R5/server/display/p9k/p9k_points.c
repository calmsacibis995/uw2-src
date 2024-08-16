/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_points.c	1.1"
/***
 ***	NAME
 ***
 ***		p9k_points.c : point plotting for the P9000 display
 ***	library. 
 ***
 ***	SYNOPSIS
 ***
 ***		#include "p9k_points.h"
 ***
 ***	DESCRIPTION
 ***
 ***		If the planemask is all ones, and the GC mode GXCopy, use
 ***	a fast LFB style helper to draw the points, else go thru the
 ***	graphics engine.
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
export enum debug_level p9000_point_debug = DEBUG_LEVEL_NONE;
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

#include "p9k_state.h"
#include "p9k_gs.h"
#include "p9k_regs.h"
#include "p9k_gbls.h"
#include "p9k_opt.h"

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
 * @doc:p9000_plot_points:
 *
 * Plot points.
 *
 * @enddoc
 */

STATIC SIBool
p9000_points_plot_points(SIint32 count, SIPointP points_p)
{
	
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_REGISTER_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();

	unsigned int raster;
	register int status;
	const SIPointP points_fence_p = points_p + count;
	
	/*
	 * Assertions.
	 */

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
							 graphics_state_p));

#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_point, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_points_plot_points)\n"
					   "{\n"
					   "\tcount = %ld\n"
					   "\tpoints_p = 0x%p\n"
					   "}\n",
					   count, (void *) points_p);
	}
#endif
					   
	/*
	 * Check arguments.
	 */

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	P9000_STATE_SET_CLIP_RECTANGLE_TO_VIRTUAL_SCREEN(screen_state_p);
	
	raster = P9000_STATE_CALCULATE_FG_MINTERM(screen_state_p,
											  graphics_state_p);
	
	P9000_STATE_SET_RASTER(register_state_p, raster |
						   P9000_RASTER_QUAD_OVERSIZED_MODE);

	P9000_STATE_SYNCHRONIZE_REGISTERS(screen_state_p,
		(P9000_STATE_CHANGE_FOREGROUND_COLOR |
		 P9000_STATE_CHANGE_PLANEMASK));

	P9000_ASSERT_IS_STATE_SYNCHRONIZED(screen_state_p,
		(P9000_STATE_CHANGE_FOREGROUND_COLOR |
		 P9000_STATE_CHANGE_PLANEMASK));

	/*
	 * Setup the X, Y origin to be (0, 0).
	 */

	P9000_WRITE_PARAMETER_CONTROL_REGISTER(
		P9000_PARAMETER_CONTROL_W_OFF_XY,
		0);

	
	do
	{

		/*
		 * Program the meta-coordinate registers for drawing this
		 * point. 
		 */
		
		P9000_WRITE_META_COORDINATE_REGISTER(
			P9000_META_COORDINATE_VTYPE_POINT,
			P9000_META_COORDINATE_ABS,
			P9000_META_COORDINATE_YX_XY,
			P9000_PACK_XY(points_p->x, points_p->y));
		
#if (defined(__DEBUG__))
		if (DEBUG_LEVEL_MATCH(p9000_point, INTERNAL))
		{
			(void) fprintf(debug_stream_p,
						   "\t\t(%d, %d)\n",
						   points_p->x, points_p->y);
		}
#endif

		/*
		 * Initiate the draw operation.  We don't need to check the
		 * return status of the drawing operation as we can ignore the
		 * point if it falls outside the graphics engine coordinates.
		 */

		do
		{
			status = P9000_INITIATE_QUAD_COMMAND();
		} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);


	} while (++points_p < points_fence_p);
	
	return (SI_SUCCEED);
}


/*
 * @doc:p9000_polypoint__gs_change__:
 *
 * Handle graphics state changes.
 *
 * @enddoc
 */

function void
p9000_polypoint__gs_change__()
{
	
	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_CURRENT_GRAPHICS_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));

	ASSERT(IS_OBJECT_STAMPED(P9000_SCREEN_STATE,
							 screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
							 (struct generic_graphics_state *)
							 graphics_state_p));
	ASSERT(IS_OBJECT_STAMPED(P9000_GRAPHICS_STATE,
							 graphics_state_p));
	
	if (screen_state_p->options_p->pointdraw_options &
		P9000_OPTIONS_POINTDRAW_OPTIONS_USE_PLOT_POINT)
	{
		screen_state_p->generic_state.screen_functions_p->
			si_plot_points = p9000_points_plot_points;
	}
	else
	{
		screen_state_p->generic_state.screen_functions_p->si_plot_points =
			graphics_state_p->generic_si_functions.si_plot_points;
	}
}

/*
 * @doc:p9000_polypoint__initialize__:
 *
 * Initialize the point module.
 *
 * @enddoc
 */

function void
p9000_polypoint__initialize__(SIScreenRec *si_screen_p,
							 struct p9000_options_structure *
							 options_p)
{
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	SIFunctionsP functions_p = si_screen_p->funcsPtr;

	if (options_p->pointdraw_options &
		P9000_OPTIONS_POINTDRAW_OPTIONS_USE_PLOT_POINT)
	{
		flags_p->SIavail_point = PLOTPOINT_AVAIL;
		functions_p->si_plot_points = p9000_points_plot_points;
	}
}
