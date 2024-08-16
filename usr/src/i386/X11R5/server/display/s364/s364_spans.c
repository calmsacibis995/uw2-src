/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364_spans.c	1.2"

/***
 ***	NAME
 ***
 *** 		s364_spans.c : spans routines for the S364 display
 *** 	library.
 ***
 ***	SYNOPSIS
 ***
 ***		#include s364_spans.h"
 ***
 ***	DESCRIPTION
 ***		This module implements span filling (filling of lists of
 ***	horizontal lines) SI entry points.
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
export boolean s364_fillspans_debug = 0;
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
#include "s364_opt.h"
#include "s364_regs.h"
#include "s364_gbls.h"
#include "s364_gs.h"
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
 * PURPOSE
 *
 *	This routine fills lists of horizontal lines ( spans ) using the 
 * current fill mode. Rectangle fill operation is used to do this.
 * 
 * RETURN VALUE
 *
 *		SI_SUCCEED	on success
 *		SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_fillspans_solid(SIint32 count, register SIPointP points_p,
	register SIint32 *widths_p)
{
	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_REGISTER_BASE_ADDRESS_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();

	const SIPointP points_fence_p = points_p + count;
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p)); 
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));


	ASSERT(graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		SGFillSolidFG ||
		graphics_state_p->generic_state.si_graphics_state.SGfillmode ==
		SGFillSolidBG);

#if defined(__DEBUG__)
	if (s364_fillspans_debug)
	{
		(void)fprintf(debug_stream_p,
		"(s364_spansfill_solid)\n{\n"
		"\tcount = %ld\n"
		"\tpoints_p = %p\n"
		"\twidths_p = %p\n"
		"\n}\n",
		count,
		(void*)points_p,
		(void*)widths_p);
	}
#endif
	ASSERT(!(S3_IS_FIFO_OVERFLOW()));

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

    /*
     * Set the clipping rectangle to cover virtual screen, only if
     * required.
     */
    S364_STATE_SET_CLIP_RECTANGLE_TO_VIRTUAL_SCREEN();

	/*
  	 * Pixcntl is set to use foreground mix register, by default.
	 */
	S3_WAIT_FOR_FIFO(2);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_FRGD_MIX,
		screen_state_p->register_state.s3_enhanced_commands_registers.
		frgd_mix | 
		((graphics_state_p->generic_state.si_graphics_state.SGfillmode == 
			SGFillSolidFG) ? S3_CLR_SRC_FRGD_COLOR: S3_CLR_SRC_BKGD_COLOR),
		unsigned short);

	S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MULTIFUNC_CNTL,
		S3_ENHANCED_COMMAND_REGISTER_MIN_AXIS_PCNT_INDEX | 
		(0 & S3_MULTIFUNC_VALUE_BITS), unsigned short);

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

			S3_INLINE_WAIT_FOR_FIFO(4);

			S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_X, 
								((unsigned) points_p->x), unsigned short);
			S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_CUR_Y, 
								((unsigned) points_p->y), unsigned short);
			S3_UPDATE_MMAP_REGISTER(S3_ENHANCED_COMMAND_REGISTER_MAJ_AXIS_PCNT, 
							((unsigned) (width - 1)), unsigned short);

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
		}
	} while (++points_p < points_fence_p);

#if (defined(__DEBUG__))
	S3_WAIT_FOR_GE_IDLE();
#endif

	ASSERT(!(S3_IS_FIFO_OVERFLOW()));
	return (SI_SUCCEED);
}

/*
 * PURPOSE
 *
 *	This routine fills lists of horizontal lines ( spans ) using the 
 * current tile pattern. 
 * 
 * RETURN VALUE
 *
 *		SI_SUCCEED	on success
 *		SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_fillspans_tile(SIint32 count, SIPointP points_p, 
								  SIint32 *widths_p)
{

	S364_CURRENT_SCREEN_STATE_DECLARE();
	
	S364_CURRENT_GRAPHICS_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));

	return (SI_FAIL);
}

/*
 * PURPOSE
 *
 *	This routine fills lists of horizontal lines ( spans ) using the 
 * current stipple pattern. 
 * 
 * RETURN VALUE
 *
 *		SI_SUCCEED	on success
 *		SI_FAIL		on failure
 *
 */
STATIC SIBool
s364_fillspans_stipple(SIint32 count, SIPointP points_p, SIint32 *widths_p)
{

	S364_CURRENT_SCREEN_STATE_DECLARE();
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		 generic_current_screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		 (struct generic_graphics_state *) graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));

	return(SI_FAIL);
}

/*
 * PURPOSE
 *
 * Switch pointers to the correct fill spans routine at graphics state
 * change time.
 *
 * RETURN VALUE
 *
 *		None.
 *
 */
function void
s364_fillspans__gs_change__(void)
{
	S364_CURRENT_GRAPHICS_STATE_DECLARE();
	S364_CURRENT_SCREEN_STATE_DECLARE();

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
		   (struct generic_graphics_state *) screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(GENERIC_GRAPHICS_STATE,
		(struct generic_graphics_state *)graphics_state_p));

	ASSERT(IS_OBJECT_STAMPED(S364_SCREEN_STATE, screen_state_p));
	ASSERT(IS_OBJECT_STAMPED(S364_GRAPHICS_STATE, graphics_state_p));
	
	switch (graphics_state_p->generic_state.si_graphics_state.SGfillmode)
	{
		case SGFillSolidFG:
		case SGFillSolidBG:
			if (screen_state_p->options_p->spansfill_options &
				S364_OPTIONS_SPANSFILL_OPTIONS_USE_SOLID_FILL)
			{
				screen_state_p->generic_state.screen_functions_p->
					si_fillspans = s364_fillspans_solid;
			}
			break;

		case SGFillTile:
#ifdef DELETE
			if (screen_state_p->options_p->spansfill_options &
				S364_OPTIONS_SPANSFILL_OPTIONS_USE_TILE_FILL)
			{
				screen_state_p->generic_state.screen_functions_p->
					si_fillspans = s364_fillspans_stipple;
			}
#endif
		break;

		case SGFillStipple:
#ifdef DELETE
			if (screen_state_p->options_p->spansfill_options &
				S364_OPTIONS_SPANSFILL_OPTIONS_USE_STIPPLE_FILL)
			{
				screen_state_p->generic_state.screen_functions_p->
					si_fillspans = s364_fillspans_tile;
			}
#endif
			break;
		default:
			/*CONSTANTCONDITION*/
			ASSERT(0);
			break;
	}	



	return;
}

/*
 * s364_fillspans__initialize__
 *
 * PURPOSE
 *
 * Initializing the fillspans module. This function is called from the 
 * munch generated function in the module s364__init__.c at the time
 * of chipset initialization. 
 *
 * RETURN VALUE
 *
 *		None.
 */
function void
s364_fillspans__initialize__(SIScreenRec *si_screen_p,
	struct s364_options_structure * options_p)
{
	SIFlagsP flags_p = si_screen_p->flagsPtr;

	if (options_p->spansfill_options &
		S364_OPTIONS_SPANSFILL_OPTIONS_USE_SOLID_FILL)
	{
		flags_p->SIavail_spans |= SPANS_AVAIL;
	}

}
