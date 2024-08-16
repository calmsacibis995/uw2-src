/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)S3:S3/devices/s3_init/debug.c	1.1"

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

extern boolean s3_init_debug;

void
s3_init_debug_control(boolean is_debug)
{
	if (debug_stream_p == NULL && (debug_stream_p =
		fopen(getenv("debug_stream"), "w")) == NULL)
	{
		debug_stream_p = stdout;
	}

	debug = (boolean) (is_debug
		&& (getenv("debug") != NULL));
	s3_init_debug = (boolean) (debug
		|| (getenv("s3_init_debug") != NULL));
}

#endif
