/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64_gbls.h	1.2"

#if (! defined(__M64_GBLS_INCLUDED__))

#define __M64_GBLS_INCLUDED__



/***
 ***	Public declarations.
 ***/

/***
 ***	Includes.
 ***/

#include <sidep.h>
#include "stdenv.h"

/***
 ***	Constants.
 ***/

/***
 ***	Macros.
 ***/

/***
 ***	Types.
 ***/


/***
 ***	Variables.
 ***/
extern int m64_graphics_engine_micro_delay_count ;
extern int m64_graphics_engine_loop_timeout_count ;
extern int m64_graphics_engine_number_of_fifo_entries_free ;

/*
 *	Debugging variables.
 */

#if (defined(__DEBUG__))
extern boolean m64_globals_debug ;
#endif

/*
 *	Current module state.
 */

extern SIBool
m64_no_operation_fail()
;

extern SIBool
m64_no_operation_succeed()
;

extern SIvoid
m64_no_operation_void()
;


#endif
