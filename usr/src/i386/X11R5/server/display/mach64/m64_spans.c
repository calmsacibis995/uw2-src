/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_spans.c	1.2"

/***
 ***	NAME
 ***
 *** 		m64_spans.c : spans routines for the M64 display
 *** 	library.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "m64_spans.h"
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
export boolean m64_fillspans_debug = 0;
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
#include "m64_opt.h"
#include "m64_regs.h"
#include "m64_gbls.h"
#include "m64_gs.h"
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
m64_fillspans_solid(SIint32 count, register SIPointP points_p,
	register SIint32 *widths_p)
{
	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();

	unsigned long *register_values_p = screen_state_p->register_state.registers;
	const SIPointP points_fence_p = points_p + count;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p)); 
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));

	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		SGFillSolidFG ||
		graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		SGFillSolidBG);

#if defined(__DEBUG__)
	if (m64_fillspans_debug)
	{
		(void)fprintf(debug_stream_p,
		"(m64_spansfill_solid)\n{\n"
		"\tcount = %ld\n"
		"\tpoints_p = %p\n"
		"\twidths_p = %p\n"
		"\n}\n",
		count,
		(void*)points_p,
		(void*)widths_p);
	}
#endif
	ASSERT(!(M64_IS_FIFO_OVERFLOW()));

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

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
	 * Program the DP_SRC register to select the fg/bg color register
	 * to be the source for the rectangle fill.
	 * Program the rectangle height which is a constant for all spans.
	 */
	M64_WAIT_FOR_FIFO(3);
	*(register_base_address_p + M64_REGISTER_DP_SRC_OFFSET) = 
		(graphics_state_p->generic_state.si_graphics_state.SGfillmode == 
		SGFillSolidFG ? DP_SRC_FRGD_COLOR << DP_SRC_DP_FRGD_SRC_SHIFT : 0) |
		DP_SRC_DP_MONO_SRC_ALWAYS_1 ;
	*(register_base_address_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET) = 	
		*(register_values_p + M64_REGISTER_GUI_TRAJ_CNTL_OFFSET);
	*(register_base_address_p + M64_REGISTER_DST_HEIGHT_OFFSET) = 1;

	ASSERT(points_fence_p > points_p);


	/*
	 * We will use rectfill to draw each span
	 */
	do
	{
		register const int width = *widths_p++;
		if( width > 0)
		{
			/*
			 * fill a solid span frm points_p->(x,y) of length width.
			 */
			M64_WAIT_FOR_FIFO(2);
			*(register_base_address_p + M64_REGISTER_DST_Y_X_OFFSET) = 
				points_p->y | ((unsigned)(points_p->x) << DST_Y_X_DST_X_SHIFT);
			*(register_base_address_p + M64_REGISTER_DST_WID_OFFSET) = width;
		}
	} while (++points_p < points_fence_p);

#if (defined(__DEBUG__))
	M64_WAIT_FOR_GUI_ENGINE_IDLE();
#endif

	ASSERT(!(M64_IS_FIFO_OVERFLOW()));
	return (SI_SUCCEED);
}

STATIC SIBool
m64_fillspans_tile(SIint32 count, SIPointP points_p, 
								  SIint32 *widths_p)
{

	M64_CURRENT_SCREEN_STATE_DECLARE();
	
	M64_CURRENT_GRAPHICS_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));

	return (SI_FAIL);
}

STATIC SIBool
m64_fillspans_stipple(SIint32 count, SIPointP points_p, SIint32 *widths_p)
{

	M64_CURRENT_SCREEN_STATE_DECLARE();
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		 (struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));

	return(SI_FAIL);
}

/*
 * Switch pointers to the correct fill spans routine at graphics state
 * change time.
 */
function void
m64_fillspans__gs_change__(void)
{
	M64_CURRENT_GRAPHICS_STATE_DECLARE();
	M64_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *)graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(M64_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(M64_GRAPHICS_STATE, graphics_state_p));
	
	switch (graphics_state_p->generic_state.si_graphics_state.SGfillmode)
	{
		case SGFillSolidFG:
		case SGFillSolidBG:
			if (screen_state_p->options_p->spansfill_options &
				M64_OPTIONS_SPANSFILL_OPTIONS_USE_SOLID_FILL)
			{
				screen_state_p->generic_state.screen_functions_p->
					si_fillspans = m64_fillspans_solid;
			}
			break;

		case SGFillTile:
			if (screen_state_p->options_p->spansfill_options &
				M64_OPTIONS_SPANSFILL_OPTIONS_USE_TILE_FILL)
			{
				screen_state_p->generic_state.screen_functions_p->
					si_fillspans = m64_fillspans_stipple;
			}
		break;

		case SGFillStipple:
			if (screen_state_p->options_p->spansfill_options &
				M64_OPTIONS_SPANSFILL_OPTIONS_USE_STIPPLE_FILL)
			{
				screen_state_p->generic_state.screen_functions_p->
					si_fillspans = m64_fillspans_tile;
			}
			break;
		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
	}	
	return;
}

function void
m64_fillspans__initialize__(SIScreenRec *si_screen_p,
	struct m64_options_structure * options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;
	SIFlagsP flags_p = si_screen_p->flagsPtr;

	if (options_p->spansfill_options &
		M64_OPTIONS_SPANSFILL_OPTIONS_USE_SOLID_FILL)
	{
		flags_p->SIavail_spans |= SPANS_AVAIL;
	}

	if (options_p->spansfill_options &
		M64_OPTIONS_SPANSFILL_OPTIONS_USE_STIPPLE_FILL)
	{
		flags_p->SIavail_spans |= (STIPPLE_AVAIL | OPQSTIPPLE_AVAIL |
								   SPANS_AVAIL);
	}

	if (options_p->spansfill_options &
		M64_OPTIONS_SPANSFILL_OPTIONS_USE_TILE_FILL)
	{
		flags_p->SIavail_spans |= (TILE_AVAIL | SPANS_AVAIL);
	}
}
