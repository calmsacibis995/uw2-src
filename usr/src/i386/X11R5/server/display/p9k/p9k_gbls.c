/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_gbls.c	1.1"
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
export enum debug_level p9000_globals_debug = DEBUG_LEVEL_NONE;
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

#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

#include "p9k_state.h"
#include "p9k_regs.h"

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
 * Null routines.
 */


/*
 * @doc:p9000_global_no_operation_fail:
 *
 * @enddoc
 */

function SIBool
p9000_global_no_operation_fail(void)
{
	return (SI_FAIL);
}

/*
 * @doc:p9000_global_no_operation_succeed:
 *
 * @enddoc
 */

function SIBool
p9000_global_no_operation_succeed(void)
{
	return (SI_SUCCEED);
}

/*
 * @doc:p9000_global_no_operation_void:
 *
 * @enddoc
 */

function SIvoid
p9000_global_no_operation_void(void)
{
	return;
}

/*
 * @doc:p9000_global_graphics_engine_timeout_handler:
 *
 * @enddoc
 */

function void
p9000_global_graphics_engine_timeout_handler(
	char *file_name_p, 
	int line_number)
{

	(void) fprintf(stderr, P9000_MESSAGE_GRAPHICS_ENGINE_TIMEOUT,
				   file_name_p, line_number);

	(void) fflush(stderr);

#if defined(__DEBUG__)

	/*
	 * Flush the critical open streams, and call 'abort()'.
	 */

	(void) fflush(stdout);
	(void) fflush(debug_stream_p);
	(void) fflush(stderr);

	abort();

#else

	kill(getpid(), SIGTERM);

	pause();

#endif

}

/*
 * @doc:p9000_global_reissue_quad_command:
 *
 * Clip the given coordinates to the current visible window and
 * re-issue a QUAD command.  Coordinates `xl' and `yt' are inclusive,
 * while `xr' and `yb' are exclusive.
 *
 * @enddoc
 */

function void
p9000_global_reissue_quad_command(int xl, int yt, int xr, int yb)
{

	P9000_CURRENT_SCREEN_STATE_DECLARE();
	P9000_MEMORY_BASE_DECLARE();

	/*
	 * Get the clip coordinates.  Convert right and lower coordinates
	 * to exclusive coordinates.
	 */

	const int clip_top =
		screen_state_p->generic_state.screen_clip_top;
	const int clip_left =
			screen_state_p->generic_state.screen_clip_left;
	const int clip_right =
		screen_state_p->generic_state.screen_clip_right + 1;
	const int clip_bottom =
		screen_state_p->generic_state.screen_clip_bottom + 1;
	
	
	int status;
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_globals, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_global_reissue_quad_command)\n"
					   "{\n"
					   "\txl = %d\n"
					   "\tyt = %d\n"
					   "\txr = %d\n"
					   "\tyb = %d\n"
					   "}\n",
					   xl, yt, xr, yb);
	}
#endif

	/*
	 * Check that we are not being called spuriously.
	 */

	ASSERT(P9000_IS_X_COORDINATE_OUT_OF_BOUNDS(xl) ||
		   P9000_IS_X_COORDINATE_OUT_OF_BOUNDS(xr) ||
		   P9000_IS_Y_COORDINATE_OUT_OF_BOUNDS(yt) ||
		   P9000_IS_Y_COORDINATE_OUT_OF_BOUNDS(yb));

	/*
	 * Check for visibility.
	 */
	
	if (xl >= clip_right || xr <= clip_left ||
		yt >= clip_bottom || yb <= clip_top)
	{
		return;
	}
	
	/*
	 * Clip to useable corrdinates.
	 */
	
	if (xl < clip_left) 	{ xl = clip_left; }
	if (xr > clip_right) 	{ xr = clip_right; }

	if (yt < clip_top) 		{ yt = clip_top; }
	if (yb > clip_bottom) 	{ yb = clip_bottom; }
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_globals, INTERNAL))
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
	 * Re-issue quad.
	 */

	/*
	 * xl, yt
	 */

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_0,
		P9000_PARAMETER_COORDINATE_X_32,
		P9000_PARAMETER_COORDINATE_REL,
		xl);

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_0,
		P9000_PARAMETER_COORDINATE_Y_32,
		P9000_PARAMETER_COORDINATE_REL,
		yt);
		
	/*
	 * xl, yb
	 */

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_1,
		P9000_PARAMETER_COORDINATE_X_32,
		P9000_PARAMETER_COORDINATE_REL,
		xl);

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_1,
		P9000_PARAMETER_COORDINATE_Y_32,
		P9000_PARAMETER_COORDINATE_REL,
		yb);

	/*
	 * xr, yb
	 */

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_2,
		P9000_PARAMETER_COORDINATE_X_32,
		P9000_PARAMETER_COORDINATE_REL,
		xr);

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_2,
		P9000_PARAMETER_COORDINATE_Y_32,
		P9000_PARAMETER_COORDINATE_REL,
		yb);

	/*
	 * xr, yt
	 */

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_3,
		P9000_PARAMETER_COORDINATE_X_32,
		P9000_PARAMETER_COORDINATE_REL,
		xr);

	P9000_WRITE_PARAMETER_COORDINATE_REGISTER(
		P9000_PARAMETER_COORDINATE_REG_3,
		P9000_PARAMETER_COORDINATE_Y_32,
		P9000_PARAMETER_COORDINATE_REL,
		yt);
		
	/*
	 * Initiate the quad draw.
	 */

	do
	{
		status = P9000_INITIATE_QUAD_COMMAND();
	} while (status & P9000_STATUS_DONT_ISSUE_QUAD_BLIT);
		

	/*
	 * We shouldn't have an exception this time around.
	 */

	ASSERT(!(status & P9000_STATUS_QUAD_SOFTWARE));
	
	
}

/*
 * @doc:p9000_global_apply_SI_1_1_rect_fill_function:
 *
 * Convert rectangle parameters from SI 1_0 format to the newer
 * SI_1_1 format and apply the given function to them.
 *
 * @enddoc
 */

function SIBool
p9000_global_apply_SI_1_1_rect_fill_function(
	void *module_private_p,
	SIint32 count,
	SIRectP rect_p,
	SIBool (*function_p)(void *module_private_p,
						 int count, 
						 SIRectOutlineP rect_p))
{
	int ret_val;
	boolean is_local_allocation;
	
	SIRectOutlineP p_new_rect, tmp_new_rect_p;
	SIRectP tmp_old_rect_p;
	SIint32 tmpcount;

	SIRectOutline				/* for fast allocations */
		localRectangles[
			P9000_DEFAULT_SI_1_0_COMPATIBILITY_LOCAL_RECTANGLE_COUNT];
	
#if (defined(__DEBUG__))
	if (DEBUG_LEVEL_MATCH(p9000_globals, SCAFFOLDING))
	{
		(void) fprintf(debug_stream_p,
					   "(p9000_global_apply_SI_1_1_rect_fill_function)\n"
					   "{\n"
					   "\tcount = %ld\n"
					   "\trect_p = 0x%p\n"
					   "\tfunction_p = 0x%p\n"
					   "}\n",
					   count, (void *) rect_p, (void *) function_p);
	}
#endif

	ASSERT(count > 0);

	/*
	 * Poor man's alloca ...
	 */

	if (count > P9000_DEFAULT_SI_1_0_COMPATIBILITY_LOCAL_RECTANGLE_COUNT)
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
	tmp_old_rect_p = rect_p;
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
	
	ASSERT(function_p != NULL);
	
	ret_val =
		(*function_p) (module_private_p,count, p_new_rect);


	if (is_local_allocation == FALSE)
	{
		free_memory (p_new_rect);
	}
	
	return (ret_val);

}
