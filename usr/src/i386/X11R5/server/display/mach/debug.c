/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/debug.c	1.3"
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

extern boolean mach_debug;
extern boolean mach_asm_debug;
extern boolean mach_bitblt_debug;
extern boolean mach_colormap_debug;
extern boolean mach_cursor_debug;
extern boolean mach_fill_debug;
extern boolean mach_font_string_debug;
extern boolean mach_font_debug;
extern boolean mach_globals_debug;
extern boolean mach_graphics_state_debug;
extern boolean mach_line_debug;
extern boolean mach_options_debug;
extern boolean mach_polypoint_debug;
extern boolean mach_register_debug;
extern boolean mach_scanline_debug;
extern boolean mach_fillspans_debug;
extern boolean mach_state_switch_debug;
extern boolean mach_state_debug;
extern boolean generic_debug;
extern boolean generic_graphics_state_debug;
extern boolean generic_omm_request_debug;
extern boolean generic_omm_debug;
extern boolean generic_registers_debug;
extern boolean generic_state_debug;

void
mach_debug_control(boolean is_debug)
{
	if (debug_stream_p == NULL && (debug_stream_p =
		fopen(getenv("debug_stream"), "w")) == NULL)
	{
		debug_stream_p = stdout;
	}

	debug = (boolean) (is_debug
		&& (getenv("debug") != NULL));
	mach_debug = (boolean) (debug
		|| (getenv("mach_debug") != NULL));
	mach_asm_debug = (boolean) (debug
		|| (getenv("mach_asm_debug") != NULL));
	mach_bitblt_debug = (boolean) (debug
		|| (getenv("mach_bitblt_debug") != NULL));
	mach_colormap_debug = (boolean) (debug
		|| (getenv("mach_colormap_debug") != NULL));
	mach_cursor_debug = (boolean) (debug
		|| (getenv("mach_cursor_debug") != NULL));
	mach_fill_debug = (boolean) (debug
		|| (getenv("mach_fill_debug") != NULL));
	mach_font_string_debug = (boolean) (debug
		|| (getenv("mach_font_string_debug") != NULL));
	mach_font_debug = (boolean) (debug
		|| (getenv("mach_font_debug") != NULL));
	mach_globals_debug = (boolean) (debug
		|| (getenv("mach_globals_debug") != NULL));
	mach_graphics_state_debug = (boolean) (debug
		|| (getenv("mach_graphics_state_debug") != NULL));
	mach_line_debug = (boolean) (debug
		|| (getenv("mach_line_debug") != NULL));
	mach_options_debug = (boolean) (debug
		|| (getenv("mach_options_debug") != NULL));
	mach_polypoint_debug = (boolean) (debug
		|| (getenv("mach_polypoint_debug") != NULL));
	mach_register_debug = (boolean) (debug
		|| (getenv("mach_register_debug") != NULL));
	mach_scanline_debug = (boolean) (debug
		|| (getenv("mach_scanline_debug") != NULL));
	mach_fillspans_debug = (boolean) (debug
		|| (getenv("mach_fillspans_debug") != NULL));
	mach_state_switch_debug = (boolean) (debug
		|| (getenv("mach_state_switch_debug") != NULL));
	mach_state_debug = (boolean) (debug
		|| (getenv("mach_state_debug") != NULL));
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
}

#endif
