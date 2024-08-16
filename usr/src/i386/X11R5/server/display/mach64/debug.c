/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/debug.c	1.3"

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
extern boolean m64_options_debug;
extern boolean m64_registers_debug;
extern boolean m64_state_debug;
extern boolean m64_debug;
extern boolean m64_graphics_state_debug;
extern boolean m64_video_blank_debug;
extern boolean m64_colormap_debug;
extern boolean m64_mischw_debug;
extern boolean m64_globals_debug;
extern boolean m64_fill_stipple_debug;
extern boolean m64_fill_tile_debug;
extern boolean m64_fill_solid_debug;
extern boolean m64_fill_debug;
extern boolean m64_ms_stplblt_debug;
extern boolean m64_ss_bitblt_debug;
extern boolean m64_sm_bitblt_debug;
extern boolean m64_ms_bitblt_debug;
extern boolean m64_bitblt_debug;
extern boolean m64_font_debug;
extern boolean m64_line_debug;
extern boolean m64_points_debug;
extern boolean m64_arc_debug;
extern boolean m64_asm_debug;
extern boolean m64_cursor_debug;
extern boolean m64_options_debug;
extern boolean m64_scanline_debug;
extern boolean m64_fillspans_debug;

void
m64_debug_control(boolean is_debug)
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
	m64_options_debug = (boolean) (debug
		|| (getenv("m64_options_debug") != NULL));
	m64_registers_debug = (boolean) (debug
		|| (getenv("m64_registers_debug") != NULL));
	m64_state_debug = (boolean) (debug
		|| (getenv("m64_state_debug") != NULL));
	m64_debug = (boolean) (debug
		|| (getenv("m64_debug") != NULL));
	m64_graphics_state_debug = (boolean) (debug
		|| (getenv("m64_graphics_state_debug") != NULL));
	m64_video_blank_debug = (boolean) (debug
		|| (getenv("m64_video_blank_debug") != NULL));
	m64_colormap_debug = (boolean) (debug
		|| (getenv("m64_colormap_debug") != NULL));
	m64_mischw_debug = (boolean) (debug
		|| (getenv("m64_mischw_debug") != NULL));
	m64_globals_debug = (boolean) (debug
		|| (getenv("m64_globals_debug") != NULL));
	m64_fill_stipple_debug = (boolean) (debug
		|| (getenv("m64_fill_stipple_debug") != NULL));
	m64_fill_tile_debug = (boolean) (debug
		|| (getenv("m64_fill_tile_debug") != NULL));
	m64_fill_solid_debug = (boolean) (debug
		|| (getenv("m64_fill_solid_debug") != NULL));
	m64_fill_debug = (boolean) (debug
		|| (getenv("m64_fill_debug") != NULL));
	m64_ms_stplblt_debug = (boolean) (debug
		|| (getenv("m64_ms_stplblt_debug") != NULL));
	m64_ss_bitblt_debug = (boolean) (debug
		|| (getenv("m64_ss_bitblt_debug") != NULL));
	m64_sm_bitblt_debug = (boolean) (debug
		|| (getenv("m64_sm_bitblt_debug") != NULL));
	m64_ms_bitblt_debug = (boolean) (debug
		|| (getenv("m64_ms_bitblt_debug") != NULL));
	m64_bitblt_debug = (boolean) (debug
		|| (getenv("m64_bitblt_debug") != NULL));
	m64_font_debug = (boolean) (debug
		|| (getenv("m64_font_debug") != NULL));
	m64_line_debug = (boolean) (debug
		|| (getenv("m64_line_debug") != NULL));
	m64_points_debug = (boolean) (debug
		|| (getenv("m64_points_debug") != NULL));
	m64_arc_debug = (boolean) (debug
		|| (getenv("m64_arc_debug") != NULL));
	m64_asm_debug = (boolean) (debug
		|| (getenv("m64_asm_debug") != NULL));
	m64_cursor_debug = (boolean) (debug
		|| (getenv("m64_cursor_debug") != NULL));
	m64_options_debug = (boolean) (debug
		|| (getenv("m64_options_debug") != NULL));
	m64_scanline_debug = (boolean) (debug
		|| (getenv("m64_scanline_debug") != NULL));
	m64_fillspans_debug = (boolean) (debug
		|| (getenv("m64_fillspans_debug") != NULL));
}

#endif
