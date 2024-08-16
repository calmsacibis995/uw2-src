/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k_debug.c	1.2"

#if (defined(__DEBUG__))

#include <stdlib.h>

#include "stdenv.h"

extern FILE *debug_stream_p;
extern enum debug_level debug;

extern enum debug_level p9000_debug;
extern enum debug_level p9000_arc_debug;
extern enum debug_level p9000_asm_debug;
extern enum debug_level p9000_blt_debug;
extern enum debug_level p9000_colormap_debug;
extern enum debug_level p9000_cursor_debug;
extern enum debug_level p9000_font_string_debug;
extern enum debug_level p9000_font_debug;
extern enum debug_level p9000_globals_debug;
extern enum debug_level p9000_graphics_state_debug;
extern enum debug_level p9000_line_debug;
extern enum debug_level p9000_options_debug;
extern enum debug_level p9000_point_debug;
extern enum debug_level p9000_register_debug;
extern enum debug_level p9000_state_debug;
extern enum debug_level generic_debug;
extern enum debug_level generic_graphics_state_debug;
extern enum debug_level generic_omm_request_debug;
extern enum debug_level generic_omm_debug;
extern enum debug_level generic_registers_debug;
extern enum debug_level generic_state_debug;
extern enum debug_level p9000_solid_debug;
extern enum debug_level p9000_tile_debug;
extern enum debug_level p9000_stipple_debug;
extern enum debug_level p9000_scanline_debug;
extern enum debug_level p9k_dacs_debug;
extern enum debug_level p9000_clocks_debug;
extern enum debug_level p9000_misc_debug;

void
p9000_debug_control(boolean is_debug)
{
	if (debug_stream_p == NULL && (debug_stream_p =
		fopen(getenv("debug_stream"), "w")) == NULL)
	{
		debug_stream_p = stdout;
	}

	debug = ((is_debug  
		&& atoi(getenv("debug"))) != DEBUG_LEVEL_NONE);
	p9000_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_debug"))));
	p9000_arc_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_arc_debug"))));
	p9000_asm_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_asm_debug"))));
	p9000_blt_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_blt_debug"))));
	p9000_colormap_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_colormap_debug"))));
	p9000_cursor_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_cursor_debug"))));
	p9000_font_string_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_font_string_debug"))));
	p9000_font_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_font_debug"))));
	p9000_globals_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_globals_debug"))));
	p9000_graphics_state_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_graphics_state_debug"))));
	p9000_line_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_line_debug"))));
	p9000_options_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_options_debug"))));
	p9000_point_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_point_debug"))));
	p9000_register_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_register_debug"))));
	p9000_state_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_state_debug"))));
	generic_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("generic_debug"))));
	generic_graphics_state_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("generic_graphics_state_debug"))));
	generic_omm_request_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("generic_omm_request_debug"))));
	generic_omm_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("generic_omm_debug"))));
	generic_registers_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("generic_registers_debug"))));
	generic_state_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("generic_state_debug"))));
	p9000_solid_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_solid_debug"))));
	p9000_tile_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_tile_debug"))));
	p9000_stipple_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_stipple_debug"))));
	p9000_scanline_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_scanline_debug"))));
	p9k_dacs_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9k_dacs_debug"))));
	p9000_clocks_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_clocks_debug"))));
	p9000_misc_debug = ((debug != DEBUG_LEVEL_NONE) ? debug :
(atoi(getenv("p9000_misc_debug"))));
}

#endif
