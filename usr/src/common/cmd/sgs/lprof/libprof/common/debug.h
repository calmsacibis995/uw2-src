/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lprof:libprof/common/debug.h	1.1"
/*
*	file: debug.h
*	desc: Debug macros for the profiler.
*	date: 11/09/88
*/
#include "stdio.h"

#ifdef ddt
int	debug_value;
#define DEBUG(exp)	exp; fflush(stdout)
#define DEBUG_LOC(name)	printf("Location: %s\n",name); fflush(stdout)

#define NO_DEBUG(exp)
#define NO_DEBUG_LOC(name)

#else

#define DEBUG(exp)
#define DEBUG_LOC(name)

#define NO_DEBUG(exp)
#define NO_DEBUG_LOC(name)
#endif

