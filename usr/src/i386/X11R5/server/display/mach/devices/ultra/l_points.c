/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/devices/ultra/l_points.c	1.3"

/***
 ***	NAME
 ***
 ***		l_points.c : point plotting code for the ultra+lfb
 ***	display layer.
 ***
 ***	SYNOPSIS
 ***
 ***		#include "l_points.h"
 ***
 ***	DESCRIPTION
 ***
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

#include <sidep.h>
#include "l_opt.h"
#include "stdenv.h"

#if (defined(__DEBUG__))
export boolean lfb_points_debug;
#endif

PRIVATE

#include "ultra.h"
#include "l_globals.h"
#include "l_gs.h"

#define	LFB_POINTS_STATE_STAMP\
	(('L' << 0) + ('F' << 1) + ('B' << 2) + ('_' << 3) +\
	 ('P' << 4) + ('O' << 5) + ('I' << 6) + ('N' << 7) +\
	 ('T' << 8) + ('S' << 9) + ('_' << 10) + ('S' << 11) +\
	 ('T' << 12) + ('A' << 13) + ('T' << 14) + ('E' << 15))


/*
 * lfb_plot_points_16
 * 
 * plot points entry point for 16 bit mode
 *
 */

STATIC SIBool
lfb_plot_points_16(SIint32 count, register SIPointP points_p)
{

	LFB_CURRENT_SCREEN_STATE_DECLARE();
	LFB_CURRENT_GRAPHICS_STATE_DECLARE();

	register unsigned short *const frame_buffer_p = 
		screen_state_p->frame_buffer_p;

	register SIPointP points_fence_p = 
		points_p + count;

	register int	offset;		/* linear offset of a point */

	const int	stride =  (screen_state_p->frame_buffer_stride) >> 1;
								/* scanline to scanline stride */
	
	const unsigned short foreground_color = 
		graphics_state_p->mach_state.generic_state.
			si_graphics_state.SGfg;
	
	/*
	 * Clip coordinates.
	 */

	const int  max_screen_x = 
		screen_state_p->mach_state.generic_state.screen_virtual_width;

	const int max_screen_y =
		screen_state_p->mach_state.generic_state.screen_virtual_height;
	
	ASSERT(points_p);
	ASSERT(graphics_state_p->mach_state.generic_state.
		   si_graphics_state.SGmode == GXcopy);
	ASSERT((graphics_state_p->mach_state.generic_state.
			si_graphics_state.SGpmask & 0xFFFF) == 0xFFFF);
	
	ASSERT(screen_state_p->mach_state.generic_state.screen_depth == 16);
	
#if	(defined(__DEBUG__))
	if (lfb_points_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_plot_points_16)\n"
"{\n"
"\tcount = %ld\n"
"\tpoints_p = %p\n"
"}\n",
					  count,
					  (void*) points_p);
	}
#endif

	if (count <= 0)
	{
		return SI_SUCCEED;
	}

	MACH_WAIT_FOR_ENGINE_IDLE();

	switch (stride)
	{
	case 1024:					/* very common case */
	
		do
		{
			/*
			 * Compute offset.
			 */
			
			if (points_p->y < max_screen_y && 
				points_p->y >= 0 &&
				points_p->x >= 0 &&
				points_p->x < max_screen_x)
			{
				
				offset = (points_p->y << 10) + points_p->x;

				/*
				 * Write the pixel.
				 */

				*(frame_buffer_p + offset) = foreground_color;
				
			}
			
			
		} while (++points_p < points_fence_p);

		break;
		
	default:
		
		do
		{
			/*
			 * Compute the offset
			 */

			if (points_p->y < max_screen_y && 
				points_p->y >= 0 &&
				points_p->x >= 0 &&
				points_p->x < max_screen_x)
			{
				
				offset = (points_p->y  * stride) + points_p->x ;

				/*
				 * Write the pixel
				 */

				*(frame_buffer_p + offset) = foreground_color;

			}
			
		} while (++points_p < points_fence_p);

		break;
	}
	
	return (SI_SUCCEED);

}

/*
 * lfb_plot_points_8
 * 
 * plot points entry point for 8 bit mode
 *
 */

STATIC SIBool
lfb_plot_points_8(SIint32 count, register SIPointP points_p)
{

	LFB_CURRENT_SCREEN_STATE_DECLARE();
	LFB_CURRENT_GRAPHICS_STATE_DECLARE();

	register unsigned char *const frame_buffer_p = 
		screen_state_p->frame_buffer_p;

	register SIPointP points_fence_p = 
		points_p + count;

	register int	offset;		/* linear offset of a point */

	const int	stride =  screen_state_p->frame_buffer_stride;
								/* scanline to scanline stride */
	
	const unsigned char	foreground_color = 
		graphics_state_p->mach_state.generic_state.
			si_graphics_state.SGfg;
	
	/*
	 * Clip coordinates.
	 */

	const int  max_screen_x = 
		screen_state_p->mach_state.generic_state.screen_virtual_width;

	const int max_screen_y =
		screen_state_p->mach_state.generic_state.screen_virtual_height;
	
	ASSERT(points_p);
	ASSERT(graphics_state_p->mach_state.generic_state.
		   si_graphics_state.SGmode == GXcopy);
	ASSERT((graphics_state_p->mach_state.generic_state.
			si_graphics_state.SGpmask & 0xFF) == 0xFF);
	
	ASSERT(screen_state_p->mach_state.generic_state.screen_depth == 8);
	
#if	(defined(__DEBUG__))
	if (lfb_points_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_plot_points_8)\n"
"{\n"
"\tcount = %ld\n"
"\tpoints_p = %p\n"
"}\n",
					  count,
					  (void*) points_p);
	}
#endif

	if (count <= 0)
	{
		return SI_SUCCEED;
	}

	MACH_WAIT_FOR_ENGINE_IDLE();

	switch (stride)
	{
	case 1024:					/* very common case */
	
		do
		{
			/*
			 * Compute offset.
			 */
			
			if (points_p->y < max_screen_y && 
				points_p->y >= 0 &&
				points_p->x >= 0 &&
				points_p->x < max_screen_x)
			{
				
				offset = (points_p->y << 10) + points_p->x;

				/*
				 * Write the pixel.
				 */

				*(frame_buffer_p + offset) = foreground_color;
				
			}
			
			
		} while (++points_p < points_fence_p);

		break;
		
	default:
		
		do
		{
			/*
			 * Compute the offset
			 */

			if (points_p->y < max_screen_y && 
				points_p->y >= 0 &&
				points_p->x >= 0 &&
				points_p->x < max_screen_x)
			{
				
				offset = (points_p->y  * stride) + points_p->x ;

				/*
				 * Write the pixel
				 */

				*(frame_buffer_p + offset) = foreground_color;

			}
			
		} while (++points_p < points_fence_p);

		break;
	}
	
	return (SI_SUCCEED);

}

/*
 * lfb_plot_points_4
 *
 * plot points entry point for 4 bit mode
 *
 */

STATIC SIBool
lfb_plot_points_4(SIint32 count, SIPointP points_p)
{
	LFB_CURRENT_SCREEN_STATE_DECLARE();
	LFB_CURRENT_GRAPHICS_STATE_DECLARE();
	
	register char *const frame_buffer_p = 
		screen_state_p->frame_buffer_p;

	register int	offset;

	const int	stride = 
		screen_state_p->frame_buffer_stride;
	
	const unsigned char	foreground_color =
		graphics_state_p->mach_state.generic_state.
			si_graphics_state.SGfg;
	
	ASSERT(points_p);
	ASSERT(graphics_state_p->mach_state.generic_state.
		   si_graphics_state.SGmode == GXcopy);
	ASSERT((graphics_state_p->mach_state.generic_state.
			si_graphics_state.SGpmask & 0x0F) == 0x0F);
	ASSERT(screen_state_p->mach_state.generic_state.screen_depth == 4);
	

#if	defined(__DEBUG__)
	if (lfb_points_debug)
	{
		(void)fprintf(debug_stream_p,
"(lfb_plot_points_4)\n"
"{\n"
"\tcount = %ld\n"
"\tpoints_p = %p\n"
"}\n",
					  count,
					  (void*)points_p);
	}
#endif

	if (count <= 0)
	{
		return (SI_SUCCEED);
	}

	MACH_WAIT_FOR_ENGINE_IDLE();

	do
	{
		offset = (points_p->y  * stride) + (points_p->x / 2);
		
		if (points_p->x & 1)	/* preserve MS nibble */
		{
			*(frame_buffer_p + offset) = 
				((foreground_color) & 0x0F) | 
				(*(frame_buffer_p + offset) & 0xF0);
		}
		else					/* preserve LS nibble */
		{
			*(frame_buffer_p + offset) = 
				((foreground_color << 4) & 0xF0) | 
				(*(frame_buffer_p + offset) & 0x0F);
		}
			
		++points_p;

	} while (--count);
		
	return (SI_SUCCEED);

}

/*
 * lfb_points__gs_change__
 */
function void
lfb_points__gs_change__()
{
	LFB_CURRENT_SCREEN_STATE_DECLARE();
	LFB_CURRENT_GRAPHICS_STATE_DECLARE();
	
	ASSERT(IS_OBJECT_STAMPED(LFB_SCREEN_STATE, screen_state_p));

	/*
	 * If  current graphics state says that current mode is GXcopy
	 * and all planes are enabled then we can do points operations
	 * through direct writes to the frame buffer.
	 */

	if ((screen_state_p->is_lfb_enabled == TRUE) &&
		(graphics_state_p->mach_state.generic_state.
		 si_graphics_state.SGmode== GXcopy) &&
		((graphics_state_p->mach_state.generic_state.
		  si_graphics_state.SGpmask &
		  LFB_ALL_PLANES_ENABLED_MASK(screen_state_p))) ==
		LFB_ALL_PLANES_ENABLED_MASK(screen_state_p))
	{
		switch (screen_state_p->mach_state.generic_state.screen_depth)
		{
		case 16: /*16bpp*/
			if (screen_state_p->options_p->pointdraw_options &
				LFB_OPTIONS_POINTDRAW_OPTIONS_USE_PLOT_POINTS)
			{
				
				generic_current_screen_state_p->screen_functions_p->
					si_plot_points =
					lfb_plot_points_16;
			}
			
			break;

		case 8: /*8bpp*/
			if (screen_state_p->options_p->pointdraw_options &
				LFB_OPTIONS_POINTDRAW_OPTIONS_USE_PLOT_POINTS)
			{
				
				generic_current_screen_state_p->screen_functions_p->
					si_plot_points =
					lfb_plot_points_8;
			}
			
			break;

		case 4:	/*4bpp*/

			if (screen_state_p->options_p->pointdraw_options &
				LFB_OPTIONS_POINTDRAW_OPTIONS_USE_PLOT_POINTS)
			{
				
				generic_current_screen_state_p->screen_functions_p->
					si_plot_points =
					lfb_plot_points_4;
			}
			
			break;
			
		default: /*CONSTANTCONDITION*/
			ASSERT(0);
			break;
			
		}
	}
}
