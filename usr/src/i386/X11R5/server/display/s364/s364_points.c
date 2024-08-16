/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_points.c	1.3"

/***
 ***	NAME
 ***
 ***		s364_points.c : point plotting routines for the s364 library.
 ***		
 ***	SYNOPSIS
 ***
 ***		#include "s364_points.c"
 ***
 ***	DESCRIPTION
 ***
 ***		This module implements point plotting functionality for
 ***	the S364 display library.
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
export boolean s364_points_debug = 0;
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
#include "s364_gbls.h"
#include "s364_opt.h"
#include "s364_gs.h"
#include "s364_regs.h"
#include "s364_state.h"

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
 * SI entry point.
 * PURPOSE
 * 
 * 	This routine plots a set of points to absolute locations on the
 * screen.
 *
 * RETURN VALUE
 *
 *	SI_SUCCEED	on success
 *	SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_plot_points(SIint32 count, SIPointP points_p)
{
	
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	const SIPointP si_points_last_fence_p = points_p + count;
	register SIPointP current_point_p = points_p;

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));

#if (defined(__DEBUG__))
	if (s364_points_debug)
	{
		(void) fprintf(debug_stream_p,
			"(s364_plot_points) {\n"
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

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	/*
	 * Set the clipping rectangle to cover virtual screen, only if
	 * required.
	 */
	S364_STATE_SET_CLIP_RECTANGLE_TO_VIRTUAL_SCREEN();

	/*
  	 * Pixcntl is set to use foreground mix register, by default.
	 */
	S3_WAIT_FOR_FIFO(3);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
		(screen_state_p->register_state.s3_enhanced_commands_registers.
		frgd_mix | S3_CLR_SRC_FRGD_COLOR),
		unsigned short);

	S3_UPDATE_MMAP_REGISTER( S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
		(0 & S3_MULTIFUNC_VALUE_BITS), unsigned short);
	S3_UPDATE_MMAP_REGISTER( S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 
		(0 & S3_MULTIFUNC_VALUE_BITS), unsigned short);

	do 
	{
		/*
		 * draw the points as one bit filled rectangles.
		 */

		S3_INLINE_WAIT_FOR_FIFO(3);

		S3_UPDATE_MMAP_REGISTER( S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
								current_point_p->x, unsigned short);
		S3_UPDATE_MMAP_REGISTER( S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
								current_point_p->y, unsigned short);

		S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CMD,
			S3_CMD_PX_MD_THRO_PLANE |  
			S3_CMD_TYPE_RECTFILL |
			S3_CMD_DRAW |
			S3_CMD_WRITE |
			S3_CMD_DIR_TYPE_AXIAL | 
			S3_CMD_BUS_WIDTH_32 |
			S3_CMD_AXIAL_X_LEFT_TO_RIGHT |
			S3_CMD_AXIAL_X_MAJOR |
			S3_CMD_AXIAL_Y_TOP_TO_BOTTOM,
			unsigned short);

	}while( ++current_point_p < si_points_last_fence_p);

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return (SI_SUCCEED);
}

/*
 * s364_points__initialize__
 *
 * PURPOSE
 *
 * Initializing the points module. This function is called from the 
 * munch generated function in the module s364__init__.c at the time
 * of chipset initialization. 
 *
 * RETURN VALUE
 *
 *		None.
 */
function void
s364_polypoint__initialize__(SIScreenRec *si_screen_p,
	struct s364_options_structure * options_p)
{
	SIFlagsP flags_p = si_screen_p->flagsPtr;
	SIFunctionsP functions_p = si_screen_p->funcsPtr;

	if (options_p->pointdraw_options &
		S364_OPTIONS_POINTDRAW_OPTIONS_USE_PLOT_POINT)
	{
		flags_p->SIavail_point = PLOTPOINT_AVAIL;
		functions_p->si_plot_points = s364_plot_points;
	}
}
