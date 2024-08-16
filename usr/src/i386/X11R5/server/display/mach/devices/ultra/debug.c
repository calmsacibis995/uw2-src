/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/devices/ultra/debug.c	1.3"

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

extern boolean lfb_options_debug;
extern boolean lfb_debug;
extern boolean lfb_gs_debug;
extern boolean lfb_bitblt_debug;
extern boolean lfb_points_debug;
extern boolean lfb_arc_debug;

void
lfb_debug_control(boolean is_debug)
{
	if (debug_stream_p == NULL && (debug_stream_p =
		fopen(getenv("debug_stream"), "w")) == NULL)
	{
		debug_stream_p = stdout;
	}

	debug = (boolean) (is_debug
		&& (getenv("debug") != NULL));
	lfb_options_debug = (boolean) (debug
		|| (getenv("lfb_options_debug") != NULL));
	lfb_debug = (boolean) (debug
		|| (getenv("lfb_debug") != NULL));
	lfb_gs_debug = (boolean) (debug
		|| (getenv("lfb_gs_debug") != NULL));
	lfb_bitblt_debug = (boolean) (debug
		|| (getenv("lfb_bitblt_debug") != NULL));
	lfb_points_debug = (boolean) (debug
		|| (getenv("lfb_points_debug") != NULL));
	lfb_arc_debug = (boolean) (debug
		|| (getenv("lfb_arc_debug") != NULL));
}

#endif
