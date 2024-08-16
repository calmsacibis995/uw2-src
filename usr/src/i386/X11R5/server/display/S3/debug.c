/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/debug.c	1.3"
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

extern boolean s3_debug;
extern boolean s3_state_debug;
extern boolean s3_register_debug;
extern boolean s3_scanline_debug;
extern boolean s3_colormap_debug;
extern boolean s3_cursor_debug;
extern boolean s3_fill_stipple_debug;
extern boolean s3_fill_tile_debug;
extern boolean s3_fill_debug;
extern boolean s3_font_debug;
extern boolean s3_globals_debug;
extern boolean s3_graphics_state_debug;
extern boolean s3_line_debug;
extern boolean s3_options_debug;
extern boolean s3_polypoint_debug;
extern boolean s3_asm_debug;
extern boolean s3_arc_debug;
extern boolean s3_fillspans_debug;
extern boolean s3_ss_stplblt_debug;
extern boolean s3_sm_stplblt_debug;
extern boolean s3_ms_stplblt_debug;
extern boolean s3_ss_bitblt_debug;
extern boolean s3_sm_bitblt_debug;
extern boolean s3_ms_bitblt_debug;
extern boolean s3_bitblt_debug;
extern boolean generic_debug;
extern boolean generic_omm_request_debug;
extern boolean generic_omm_debug;
extern boolean generic_registers_debug;
extern boolean generic_state_debug;

void
s3_debug_control(boolean is_debug)
{
	if (debug_stream_p == NULL && (debug_stream_p =
		fopen(getenv("debug_stream"), "w")) == NULL)
	{
		debug_stream_p = stdout;
	}

	debug = (boolean) (is_debug
		&& (getenv("debug") != NULL));
	s3_debug = (boolean) (debug
		|| (getenv("s3_debug") != NULL));
	s3_state_debug = (boolean) (debug
		|| (getenv("s3_state_debug") != NULL));
	s3_register_debug = (boolean) (debug
		|| (getenv("s3_register_debug") != NULL));
	s3_scanline_debug = (boolean) (debug
		|| (getenv("s3_scanline_debug") != NULL));
	s3_colormap_debug = (boolean) (debug
		|| (getenv("s3_colormap_debug") != NULL));
	s3_cursor_debug = (boolean) (debug
		|| (getenv("s3_cursor_debug") != NULL));
	s3_fill_stipple_debug = (boolean) (debug
		|| (getenv("s3_fill_stipple_debug") != NULL));
	s3_fill_tile_debug = (boolean) (debug
		|| (getenv("s3_fill_tile_debug") != NULL));
	s3_fill_debug = (boolean) (debug
		|| (getenv("s3_fill_debug") != NULL));
	s3_font_debug = (boolean) (debug
		|| (getenv("s3_font_debug") != NULL));
	s3_globals_debug = (boolean) (debug
		|| (getenv("s3_globals_debug") != NULL));
	s3_graphics_state_debug = (boolean) (debug
		|| (getenv("s3_graphics_state_debug") != NULL));
	s3_line_debug = (boolean) (debug
		|| (getenv("s3_line_debug") != NULL));
	s3_options_debug = (boolean) (debug
		|| (getenv("s3_options_debug") != NULL));
	s3_polypoint_debug = (boolean) (debug
		|| (getenv("s3_polypoint_debug") != NULL));
	s3_asm_debug = (boolean) (debug
		|| (getenv("s3_asm_debug") != NULL));
	s3_arc_debug = (boolean) (debug
		|| (getenv("s3_arc_debug") != NULL));
	s3_fillspans_debug = (boolean) (debug
		|| (getenv("s3_fillspans_debug") != NULL));
	s3_ss_stplblt_debug = (boolean) (debug
		|| (getenv("s3_ss_stplblt_debug") != NULL));
	s3_sm_stplblt_debug = (boolean) (debug
		|| (getenv("s3_sm_stplblt_debug") != NULL));
	s3_ms_stplblt_debug = (boolean) (debug
		|| (getenv("s3_ms_stplblt_debug") != NULL));
	s3_ss_bitblt_debug = (boolean) (debug
		|| (getenv("s3_ss_bitblt_debug") != NULL));
	s3_sm_bitblt_debug = (boolean) (debug
		|| (getenv("s3_sm_bitblt_debug") != NULL));
	s3_ms_bitblt_debug = (boolean) (debug
		|| (getenv("s3_ms_bitblt_debug") != NULL));
	s3_bitblt_debug = (boolean) (debug
		|| (getenv("s3_bitblt_debug") != NULL));
	generic_debug = (boolean) (debug
		|| (getenv("generic_debug") != NULL));
	generic_omm_request_debug = (boolean) (debug
		|| (getenv("generic_omm_request_debug") != NULL));
	generic_omm_debug = (boolean) (debug
		|| (getenv("generic_omm_debug") != NULL));
	generic_registers_debug = (boolean) (debug
		|| (getenv("generic_registers_debug") != NULL));
	generic_state_debug = (boolean) (debug
		|| (getenv("generic_state_debug") != NULL));
}

#endif
