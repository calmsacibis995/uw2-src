/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/***
 ***	NAME
 ***
 ***		m_scanline.c : scanline routines for the MACH display
 ***	library.
 ***	
 ***	SYNOPSIS
 ***
 ***		#include "m_scanline.h"
 ***	
 ***	DESCRIPTION
 ***
 ***	This module implements get- and set-scanline functionality for
 ***	the MACH display library.
 ***	
 ***	Scanline access is done using the SCAN_X command command for
 ***	speed.  4 bit modes need care (pixels need to be expanded).
 ***	This expansion is done by the appropriate
 ***	`screen_write_pixels' and `screen_read_pixels' functions.
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

#ident	"@(#)mach:mach/m_scanline.c	1.2"

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
#include "m_state.h"
#include "m_gs.h"

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
export boolean mach_scanline_debug = FALSE;
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

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Functions.
 ***/

STATIC SILine
mach_get_scanline(const SIint32 scanline_y)
{
	struct mach_screen_state *screen_state_p =
		(struct mach_screen_state *) generic_current_screen_state_p;

	int loop_count;
	
	unsigned short dp_config;
	
	ASSERT(generic_current_screen_state_p);
	ASSERT(generic_current_screen_state_p->screen_scanline_p);

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 screen_state_p));

	ASSERT(!MACH_IS_IO_ERROR());
	ASSERT(!MACH_IS_DATA_READY());

	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	/*
	 * Check if we need to reset the clip bounds.
	 */
	if (screen_state_p->generic_state.screen_current_clip !=
		 GENERIC_CLIP_TO_VIRTUAL_SCREEN)
	{
#if (defined(__DEBUG__))
		if (mach_scanline_debug)
		{
			(void) fprintf(debug_stream_p,

"(mach_get_scanline) resetting clip rectangle {\n"
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
	
	MACH_STATE_SET_FG_ROP(screen_state_p, MACH_MIX_FN_PAINT);
	
	/*
	 * GetScanline using SCAN_TO_X 
	 */

	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_FG_COLOR_SRC_BLIT |
		MACH_DP_CONFIG_MONO_SRC_ONE|
		MACH_DP_CONFIG_ENABLE_DRAW;
	
#if (defined(__DEBUG__))
	if(mach_scanline_debug)
	{
		(void) fprintf(debug_stream_p, 
"(mach_get_scanline)\n"
"{\n"
"\tscanline_y = %ld\n"
"\tdp_config = 0x%x\n",
					   scanline_y, dp_config);
	}
#endif

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);
	
	MACH_WAIT_FOR_FIFO(3);

	outw(MACH_REGISTER_CUR_X, 0); 	
	outw(MACH_REGISTER_CUR_Y, scanline_y); 	
	outw(MACH_REGISTER_SCAN_X,
		 screen_state_p->generic_state.screen_virtual_width);

	loop_count = 
		(screen_state_p->generic_state.screen_virtual_width *
		 screen_state_p->generic_state.screen_depth) >>
			 screen_state_p->pixtrans_width_shift;

#if (defined(__DEBUG__))
	if (mach_scanline_debug)
	{
		(void) fprintf(debug_stream_p,
"\tloop_count = %d\n",
					   loop_count);
	}
#endif

	ASSERT(loop_count >= 0);
	MACH_WAIT_FOR_DATA_READY();
	ASSERT(!MACH_IS_IO_ERROR());

	(*screen_state_p->screen_read_pixels_p)
		(screen_state_p->pixtrans_register,
		 loop_count,
		 screen_state_p->generic_state.screen_scanline_p);
	
#if (defined(__DEBUG__))
	if (mach_scanline_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif
		
	ASSERT(!MACH_IS_IO_ERROR());
	if (MACH_IS_DATA_READY())
	{
		int count = 0;
		while (MACH_IS_DATA_READY())
		{
			(void) inw(MACH_REGISTER_PIX_TRANS);
			count ++;
			
		}
#if (defined(__DEBUG__))
		if (mach_scanline_debug)
		{
			(void) fprintf(debug_stream_p, "\t\t*extra_count = %d\n",
						   count);
		}
#endif

	}

	/*
	 * Since we used the FIFO to transfer pixels, the pattern register
	 * contents are invalid.
	 */
	MACH_STATE_SET_FLAGS(screen_state_p,
						 MACH_INVALID_PATTERN_REGISTERS);
	
	return (generic_current_screen_state_p->screen_scanline_p);
}


STATIC SIvoid
mach_set_scanline(const SIint32 scanline_y, const SILine scanline_p)
{
	struct mach_screen_state *screen_state_p =
		(struct mach_screen_state *) generic_current_screen_state_p;
	
	unsigned short dp_config;
	
	int loop_count;
	
	ASSERT(generic_current_screen_state_p);
	ASSERT(generic_current_screen_state_p->screen_scanline_p);
	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	
	ASSERT(IS_OBJECT_STAMPED(MACH_SCREEN_STATE,
							 screen_state_p));

	ASSERT(!MACH_IS_IO_ERROR());
	
	ASSERT(screen_state_p->generic_state.screen_current_clip >=
		   GENERIC_CLIP_TO_VIRTUAL_SCREEN);

	dp_config = screen_state_p->dp_config_flags |
		MACH_DP_CONFIG_FG_COLOR_SRC_HOST |
		MACH_DP_CONFIG_MONO_SRC_ONE |
		MACH_DP_CONFIG_ENABLE_DRAW |
		MACH_DP_CONFIG_WRITE;

#if (defined(__DEBUG__))
	if(mach_scanline_debug)
	{
		(void) fprintf(debug_stream_p,
"(mach_set_scanline)\n"
"{\n"
"\tscanline_y = %ld\n"
"\tscanline_p = %p\n"
"\tdp_config = 0x%x\n",
					   scanline_y, (void *) scanline_p,
					   dp_config);
	}
#endif

	/*
	 * Switch to ATI context.
	 */
	MACH_STATE_SWITCH_TO_ATI_CONTEXT(screen_state_p);

	MACH_STATE_SET_FG_ROP(screen_state_p, MACH_MIX_FN_PAINT);
	
	/*
	 * SetScanline using SCAN_TO_X.
	 */

	MACH_STATE_SET_DP_CONFIG(screen_state_p, dp_config);
	
	MACH_WAIT_FOR_FIFO(3);
	outw(MACH_REGISTER_CUR_X, 0);
	outw(MACH_REGISTER_CUR_Y, scanline_y);
	outw(MACH_REGISTER_SCAN_X,
		 screen_state_p->generic_state.screen_virtual_width);
	

	loop_count = 
		(screen_state_p->generic_state.screen_virtual_width *
		 screen_state_p->generic_state.screen_depth) >>
			 screen_state_p->pixtrans_width_shift;

#if (defined(__DEBUG__))
	if (mach_scanline_debug)
	{
		(void) fprintf(debug_stream_p,
"\tloop_count = %d\n",
					   loop_count);
	}
#endif

	ASSERT(loop_count >= 0);
	MACH_WAIT_FOR_FIFO(16);
	ASSERT(!MACH_IS_IO_ERROR());

	(*screen_state_p->screen_write_pixels_p)
		(screen_state_p->pixtrans_register, loop_count,
		 screen_state_p->generic_state.screen_scanline_p);
	
#if (defined(__DEBUG__))
	if (mach_scanline_debug)
	{
		(void) fprintf(debug_stream_p, "}\n");
	}
#endif
	
	ASSERT(!MACH_IS_IO_ERROR());
	ASSERT(!MACH_IS_DATA_READY());
	
	return;

}


/*
 * mach_free_scanline
 *
 * we don't actually free the scanline at any point as this was
 * allocated 
 */
STATIC SIvoid
mach_free_scanline(void)
{
#if (defined(__DEBUG__))
	if(mach_scanline_debug)
	{
		(void) fprintf(debug_stream_p, "(mach_free_scanline)\n"
"{}\n");
	}
#endif

	return;
}

function void
mach_scanline__initialize__(SIScreenRec *si_screen_p,
							struct mach_options_structure * options_p)
{
	SIFunctionsP functions_p = si_screen_p->funcsPtr;

	ASSERT(IS_OBJECT_STAMPED(GENERIC_SCREEN_STATE,
							 generic_current_screen_state_p));
	
#if (defined(__DEBUG__))
	if(mach_scanline_debug)
	{
		(void) fprintf(debug_stream_p, 
"(mach_scanline__initialize__)\n"
"{\n"
"\tfunctions_p = %p\n"
"\toptions_p = %p\n"
"}\n",
					(void *) functions_p,
				    (void *) options_p);
	}
#endif

	functions_p->si_getsl = mach_get_scanline;
	functions_p->si_setsl = mach_set_scanline;
	functions_p->si_freesl = mach_free_scanline;

}
