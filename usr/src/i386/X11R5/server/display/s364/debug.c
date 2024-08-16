/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/debug.c	1.2"
#if (defined(__DEBUG__))

#include <stdio.h>
#include <stdlib.h>

typedef enum
{
	FALSE,
	TRUE
} boolean;

extern FILE *debug_stream_p;
extern boolean debug;

extern boolean generic_debug;
extern boolean generic_graphics_state_debug;
extern boolean generic_omm_request_debug;
extern boolean generic_omm_debug;
extern boolean generic_registers_debug;
extern boolean generic_state_debug;
extern boolean s364_options_debug;
extern boolean s364_register_debug;
extern boolean s364_state_debug;
extern boolean s364_debug;
extern boolean s364_graphics_state_debug;
extern boolean s364_video_blank_debug;
extern boolean s364_colormap_debug;
extern boolean s364_mischw_debug;
extern boolean s364_globals_debug;
extern boolean s364_fill_stipple_debug;
extern boolean s364_fill_tile_debug;
extern boolean s364_fill_solid_debug;
extern boolean s364_fill_debug;
extern boolean s364_ms_stplblt_debug;
extern boolean s364_ss_bitblt_debug;
extern boolean s364_sm_bitblt_debug;
extern boolean s364_ms_bitblt_debug;
extern boolean s364_bitblt_debug;
extern boolean s364_font_debug;
extern boolean s364_line_debug;
extern boolean s364_points_debug;
extern boolean s364_arc_debug;
extern boolean s364_asm_debug;
extern boolean s364_cursor_debug;
extern boolean s364_options_debug;
extern boolean s364_scanline_debug;
extern boolean s364_fillspans_debug;

void
s364_debug_control(boolean is_debug)
{
	if (debug_stream_p == NULL && (debug_stream_p =
		fopen(getenv("debug_stream"), "w")) == NULL)
	{
		debug_stream_p = stdout;
	}

	debug = (boolean) (is_debug
		&& (getenv("debug") != NULL));
	generic_debug = (boolean) (debug
		|| (getenv("generic_debug") != NULL));
	generic_graphics_state_debug = (boolean) (debug
		|| (getenv("generic_graphics_state_debug") != NULL));
	generic_omm_request_debug = (boolean) (debug
		|| (getenv("generic_omm_request_debug") != NULL));
	generic_omm_debug = (boolean) (debug
		|| (getenv("generic_omm_debug") != NULL));
	generic_registers_debug = (boolean) (debug
		|| (getenv("generic_registers_debug") != NULL));
	generic_state_debug = (boolean) (debug
		|| (getenv("generic_state_debug") != NULL));
	s364_options_debug = (boolean) (debug
		|| (getenv("s364_options_debug") != NULL));
	s364_register_debug = (boolean) (debug
		|| (getenv("s364_register_debug") != NULL));
	s364_state_debug = (boolean) (debug
		|| (getenv("s364_state_debug") != NULL));
	s364_debug = (boolean) (debug
		|| (getenv("s364_debug") != NULL));
	s364_graphics_state_debug = (boolean) (debug
		|| (getenv("s364_graphics_state_debug") != NULL));
	s364_video_blank_debug = (boolean) (debug
		|| (getenv("s364_video_blank_debug") != NULL));
	s364_colormap_debug = (boolean) (debug
		|| (getenv("s364_colormap_debug") != NULL));
	s364_mischw_debug = (boolean) (debug
		|| (getenv("s364_mischw_debug") != NULL));
	s364_globals_debug = (boolean) (debug
		|| (getenv("s364_globals_debug") != NULL));
	s364_fill_stipple_debug = (boolean) (debug
		|| (getenv("s364_fill_stipple_debug") != NULL));
	s364_fill_tile_debug = (boolean) (debug
		|| (getenv("s364_fill_tile_debug") != NULL));
	s364_fill_solid_debug = (boolean) (debug
		|| (getenv("s364_fill_solid_debug") != NULL));
	s364_fill_debug = (boolean) (debug
		|| (getenv("s364_fill_debug") != NULL));
	s364_ms_stplblt_debug = (boolean) (debug
		|| (getenv("s364_ms_stplblt_debug") != NULL));
	s364_ss_bitblt_debug = (boolean) (debug
		|| (getenv("s364_ss_bitblt_debug") != NULL));
	s364_sm_bitblt_debug = (boolean) (debug
		|| (getenv("s364_sm_bitblt_debug") != NULL));
	s364_ms_bitblt_debug = (boolean) (debug
		|| (getenv("s364_ms_bitblt_debug") != NULL));
	s364_bitblt_debug = (boolean) (debug
		|| (getenv("s364_bitblt_debug") != NULL));
	s364_font_debug = (boolean) (debug
		|| (getenv("s364_font_debug") != NULL));
	s364_line_debug = (boolean) (debug
		|| (getenv("s364_line_debug") != NULL));
	s364_points_debug = (boolean) (debug
		|| (getenv("s364_points_debug") != NULL));
	s364_arc_debug = (boolean) (debug
		|| (getenv("s364_arc_debug") != NULL));
	s364_asm_debug = (boolean) (debug
		|| (getenv("s364_asm_debug") != NULL));
	s364_cursor_debug = (boolean) (debug
		|| (getenv("s364_cursor_debug") != NULL));
	s364_options_debug = (boolean) (debug
		|| (getenv("s364_options_debug") != NULL));
	s364_scanline_debug = (boolean) (debug
		|| (getenv("s364_scanline_debug") != NULL));
	s364_fillspans_debug = (boolean) (debug
		|| (getenv("s364_fillspans_debug") != NULL));
}

#endif
